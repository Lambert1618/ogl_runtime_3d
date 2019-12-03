/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "Qt3DSRenderShaderCache.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAtomic.h"
#include "StringTools.h"
#include "foundation/XML.h"
#include "foundation/IOStreams.h"
#include "foundation/StringConversionImpl.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "foundation/FileTools.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "Qt3DSRenderer.h"
#include <memory>
#include "foundation/Qt3DSTime.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "EASTL/sort.h"

#include <QtCore/qstring.h>
#include <QtCore/qdatastream.h>

using namespace qt3ds::render;

namespace {
using qt3ds::render::NVRenderContextScopedProperty;

struct ShaderType
{
    enum Enum { Vertex, TessControl, TessEval, Fragment, Geometry, Compute };
};

struct SShaderCacheKey
{
    CRegisteredString m_Key;
    eastl::vector<SShaderPreprocessorFeature> m_Features;
    size_t m_HashCode;

    SShaderCacheKey(CRegisteredString key = CRegisteredString())
        : m_Key(key)
        , m_HashCode(0)
    {
    }

    SShaderCacheKey(const SShaderCacheKey &other)
        : m_Key(other.m_Key)
        , m_Features(other.m_Features)
        , m_HashCode(other.m_HashCode)
    {
    }

    SShaderCacheKey &operator=(const SShaderCacheKey &other)
    {
        m_Key = other.m_Key;
        m_Features = other.m_Features;
        m_HashCode = other.m_HashCode;
        return *this;
    }

    void GenerateHashCode()
    {
        m_HashCode = m_Key.hash();
        m_HashCode = m_HashCode
            ^ HashShaderFeatureSet(toDataRef(m_Features.data(), QT3DSU32(m_Features.size())));
    }
    bool operator==(const SShaderCacheKey &inOther) const
    {
        return m_Key == inOther.m_Key && m_Features == inOther.m_Features;
    }
};
}

namespace eastl {
template <>
struct hash<SShaderCacheKey>
{
    size_t operator()(const SShaderCacheKey &inKey) const { return inKey.m_HashCode; }
};
}

namespace {

struct ShaderCache : public IShaderCache
{
    typedef nvhash_map<SShaderCacheKey, NVScopedRefCounted<NVRenderShaderProgram>> TShaderMap;
    NVRenderContext &m_RenderContext;
    IPerfTimer &m_PerfTimer;
    TShaderMap m_Shaders;
    Qt3DSString m_CacheFilePath;
    Qt3DSString m_VertexCode;
    Qt3DSString m_TessCtrlCode;
    Qt3DSString m_TessEvalCode;
    Qt3DSString m_GeometryCode;
    Qt3DSString m_FragmentCode;
    Qt3DSString m_InsertStr;
    Qt3DSString m_ContextTypeString;
    SShaderCacheKey m_TempKey;

    IInputStreamFactory &m_InputStreamFactory;
    bool m_ShaderCompilationEnabled = true;
    bool m_shadersInitializedFromCache = false;
    volatile QT3DSI32 mRefCount = 0;

    struct ShaderSource
    {
        QVector<SShaderPreprocessorFeature> features;
        CRegisteredString key;
        SShaderCacheProgramFlags flags;
        QByteArray vertexCode;
        QByteArray tessCtrlCode;
        QByteArray tessEvalCode;
        QByteArray geometryCode;
        QByteArray fragmentCode;
    };
    QVector<ShaderSource> m_shaderSourceCache;

    ShaderCache(NVRenderContext &ctx, IInputStreamFactory &inInputStreamFactory,
                IPerfTimer &inPerfTimer)
        : m_RenderContext(ctx)
        , m_PerfTimer(inPerfTimer)
        , m_Shaders(ctx.GetAllocator(), "ShaderCache::m_Shaders")
        , m_InputStreamFactory(inInputStreamFactory)
    {
    }
    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_RenderContext.GetAllocator())

    NVRenderShaderProgram *GetProgram(
            CRegisteredString inKey, NVConstDataRef<SShaderPreprocessorFeature> inFeatures) override
    {
        m_TempKey.m_Key = inKey;
        m_TempKey.m_Features.assign(inFeatures.begin(), inFeatures.end());
        m_TempKey.GenerateHashCode();
        TShaderMap::iterator theIter = m_Shaders.find(m_TempKey);
        if (theIter != m_Shaders.end())
            return theIter->second;
        return nullptr;
    }

    void AddBackwardCompatibilityDefines(ShaderType::Enum shaderType)
    {
        if (shaderType == ShaderType::Vertex || shaderType == ShaderType::TessControl
            || shaderType == ShaderType::TessEval || shaderType == ShaderType::Geometry) {
            m_InsertStr += "#define attribute in\n";
            m_InsertStr += "#define varying out\n";
        } else if (shaderType == ShaderType::Fragment) {
            m_InsertStr += "#define varying in\n";
            m_InsertStr += "#define texture2D texture\n";
            m_InsertStr += "#define gl_FragColor fragOutput\n";

            if (m_RenderContext.IsAdvancedBlendHwSupportedKHR())
                m_InsertStr += "layout(blend_support_all_equations) out;\n";

            m_InsertStr += "#ifndef NO_FRAG_OUTPUT\n";
            m_InsertStr += "out vec4 fragOutput;\n";
            m_InsertStr += "#endif\n";
        }
    }

    void AddShaderExtensionStrings(ShaderType::Enum shaderType, bool isGLES)
    {
        if (isGLES) {
            if (m_RenderContext.IsStandardDerivativesSupported())
                m_InsertStr += "#extension GL_OES_standard_derivatives : enable\n";
            else
                m_InsertStr += "#extension GL_OES_standard_derivatives : disable\n";
        }

        if (IQt3DSRenderer::IsGlEs3Context(m_RenderContext.GetRenderContextType())) {
            if (shaderType == ShaderType::TessControl || shaderType == ShaderType::TessEval) {
                m_InsertStr += "#extension GL_EXT_tessellation_shader : enable\n";
            } else if (shaderType == ShaderType::Geometry) {
                m_InsertStr += "#extension GL_EXT_geometry_shader : enable\n";
            } else if (shaderType == ShaderType::Vertex || shaderType == ShaderType::Fragment) {
                if (m_RenderContext.GetRenderBackendCap(render::NVRenderBackend::NVRenderBackendCaps::gpuShader5))
                    m_InsertStr += "#extension GL_EXT_gpu_shader5 : enable\n";
                if (m_RenderContext.IsAdvancedBlendHwSupportedKHR())
                    m_InsertStr += "#extension GL_KHR_blend_equation_advanced : enable\n";
            }
        } else {
            if (shaderType == ShaderType::Vertex || shaderType == ShaderType::Fragment
                || shaderType == ShaderType::Geometry) {
                if (m_RenderContext.GetRenderContextType() != NVRenderContextValues::GLES2) {
                    m_InsertStr += "#extension GL_ARB_gpu_shader5 : enable\n";
                    m_InsertStr += "#extension GL_ARB_shading_language_420pack : enable\n";
                }
                if (isGLES && m_RenderContext.IsTextureLodSupported())
                    m_InsertStr += "#extension GL_EXT_shader_texture_lod : enable\n";
                if (m_RenderContext.IsShaderImageLoadStoreSupported())
                    m_InsertStr += "#extension GL_ARB_shader_image_load_store : enable\n";
                if (m_RenderContext.IsAtomicCounterBufferSupported())
                    m_InsertStr += "#extension GL_ARB_shader_atomic_counters : enable\n";
                if (m_RenderContext.IsStorageBufferSupported())
                    m_InsertStr += "#extension GL_ARB_shader_storage_buffer_object : enable\n";
                if (m_RenderContext.IsAdvancedBlendHwSupportedKHR())
                    m_InsertStr += "#extension GL_KHR_blend_equation_advanced : enable\n";
            }
        }
    }

    void AddShaderPreprocessor(Qt3DSString &str, CRegisteredString inKey,
                               ShaderType::Enum shaderType,
                               NVConstDataRef<SShaderPreprocessorFeature> inFeatures,
                               bool forCache)
    {
        // Don't use shading language version returned by the driver as it might
        // differ from the context version. Instead use the context type to specify
        // the version string.
        bool isGlES = IQt3DSRenderer::IsGlEsContext(m_RenderContext.GetRenderContextType());
        m_InsertStr.clear();
        int minor = m_RenderContext.format().minorVersion();
        QString versionStr;
        QTextStream stream(&versionStr);
        stream << "#version ";
        const QT3DSU32 type = QT3DSU32(m_RenderContext.GetRenderContextType());
        switch (type) {
        case NVRenderContextValues::GLES2:
            stream << "1" << minor << "0\n";
            break;
        case NVRenderContextValues::GL2:
            stream << "1" << minor << "0\n";
            break;
        case NVRenderContextValues::GLES3PLUS:
        case NVRenderContextValues::GLES3:
            stream << "3" << minor << "0 es\n";
            break;
        case NVRenderContextValues::GL3:
            if (minor == 3)
                stream << "3" << minor << "0\n";
            else
                stream << "1" << 3 + minor << "0\n";
            break;
        case NVRenderContextValues::GL4:
            stream << "4" << minor << "0\n";
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }

        m_InsertStr.append(versionStr.toLatin1().data());

        if (inFeatures.size()) {
            for (QT3DSU32 idx = 0, end = inFeatures.size(); idx < end; ++idx) {
                SShaderPreprocessorFeature feature(inFeatures[idx]);
                m_InsertStr.append("#define ");
                m_InsertStr.append(inFeatures[idx].m_Name.c_str());
                m_InsertStr.append(" ");
                m_InsertStr.append(feature.m_Enabled ? "1" : "0");
                m_InsertStr.append("\n");
            }
        }

        if (isGlES) {
            if (!IQt3DSRenderer::IsGlEs3Context(m_RenderContext.GetRenderContextType())) {
                if (shaderType == ShaderType::Fragment) {
                    m_InsertStr += "#define fragOutput gl_FragData[0]\n";
                }
            } else {
                m_InsertStr += "#define texture2D texture\n";
            }

            // add extenions strings before any other non-processor token
            AddShaderExtensionStrings(shaderType, isGlES);

            // add precision qualifier depending on backend
            if (IQt3DSRenderer::IsGlEs3Context(m_RenderContext.GetRenderContextType())) {
                m_InsertStr.append("precision highp float;\n"
                                   "precision highp int;\n");
                if( m_RenderContext.GetRenderBackendCap(render::NVRenderBackend::NVRenderBackendCaps::gpuShader5) ) {
                    m_InsertStr.append("precision mediump sampler2D;\n"
                                       "precision mediump sampler2DArray;\n"
                                       "precision mediump sampler2DShadow;\n");
                    if (m_RenderContext.IsShaderImageLoadStoreSupported()) {
                        m_InsertStr.append("precision mediump image2D;\n");
                    }
                }

                AddBackwardCompatibilityDefines(shaderType);
            } else {
                // GLES2
                m_InsertStr.append("precision mediump float;\n"
                                   "precision mediump int;\n"
                                   "#define texture texture2D\n");
                if (m_RenderContext.IsTextureLodSupported())
                    m_InsertStr.append("#define textureLod texture2DLodEXT\n");
                else
                    m_InsertStr.append("#define textureLod(s, co, lod) texture2D(s, co)\n");
            }
        } else {
            if (!IQt3DSRenderer::IsGl2Context(m_RenderContext.GetRenderContextType())) {
                m_InsertStr += "#define texture2D texture\n";

                AddShaderExtensionStrings(shaderType, isGlES);

                m_InsertStr += "#if __VERSION__ >= 330\n";

                AddBackwardCompatibilityDefines(shaderType);

                m_InsertStr += "#else\n";
                if (shaderType == ShaderType::Fragment) {
                    m_InsertStr += "#define fragOutput gl_FragData[0]\n";
                }
                m_InsertStr += "#endif\n";
            }
        }

        // Adding shader name is useful debugging feature, but it is not particularly useful when
        // creating shaders for cache, and it can be long
        if (inKey.IsValid() && !forCache) {
            m_InsertStr += "//Shader name -";
            m_InsertStr += inKey.c_str();
            m_InsertStr += "\n";
        }

        if (shaderType == ShaderType::TessControl) {
            m_InsertStr += "#define TESSELLATION_CONTROL_SHADER 1\n";
            m_InsertStr += "#define TESSELLATION_EVALUATION_SHADER 0\n";
        } else if (shaderType == ShaderType::TessEval) {
            m_InsertStr += "#define TESSELLATION_CONTROL_SHADER 0\n";
            m_InsertStr += "#define TESSELLATION_EVALUATION_SHADER 1\n";
        }

        str.insert(0, m_InsertStr);
    }

    // Compile this program overwriting any existing ones.
    void addShaderPreprocessors(CRegisteredString inKey,
                                const SShaderCacheProgramFlags &inFlags,
                                NVConstDataRef<SShaderPreprocessorFeature> inFeatures,
                                bool separableProgram, bool forCache)
    {
        // Add defines and such so we can write unified shaders that work across platforms.
        // vertex and fragment shaders are optional for separable shaders
        if (!separableProgram || !m_VertexCode.isEmpty())
            AddShaderPreprocessor(m_VertexCode, inKey, ShaderType::Vertex, inFeatures, forCache);
        if (!separableProgram || !m_FragmentCode.isEmpty()) {
            AddShaderPreprocessor(m_FragmentCode, inKey, ShaderType::Fragment, inFeatures,
                                  forCache);
        }
        // optional shaders
        if (inFlags.IsTessellationEnabled()) {
            QT3DS_ASSERT(m_TessCtrlCode.size() && m_TessEvalCode.size());
            AddShaderPreprocessor(m_TessCtrlCode, inKey, ShaderType::TessControl, inFeatures,
                                  forCache);
            AddShaderPreprocessor(m_TessEvalCode, inKey, ShaderType::TessEval, inFeatures,
                                  forCache);
        }
        if (inFlags.IsGeometryShaderEnabled()) {
            AddShaderPreprocessor(m_GeometryCode, inKey, ShaderType::Geometry, inFeatures,
                                  forCache);
        }
    }

    NVRenderShaderProgram *
    ForceCompileProgram(CRegisteredString inKey, const char8_t *inVert, const char8_t *inFrag,
                        const char8_t *inTessCtrl, const char8_t *inTessEval, const char8_t *inGeom,
                        const SShaderCacheProgramFlags &inFlags,
                        NVConstDataRef<SShaderPreprocessorFeature> inFeatures,
                        QString &errors, bool separableProgram, bool fromDisk = false) override
    {
        if (m_ShaderCompilationEnabled == false)
            return nullptr;
        SShaderCacheKey tempKey(inKey);
        tempKey.m_Features.assign(inFeatures.begin(), inFeatures.end());
        tempKey.GenerateHashCode();

        eastl::pair<TShaderMap::iterator, bool> theInserter = m_Shaders.insert(tempKey);
        if (fromDisk) {
            qCInfo(TRACE_INFO) << "Loading from persistent shader cache: '<"
                << tempKey.m_Key << ">'";
        } else {
            qCInfo(TRACE_INFO) << "Compiling into shader cache: '"
                << tempKey.m_Key << ">'";
        }

        if (!inVert)
            inVert = "";
        if (!inTessCtrl)
            inTessCtrl = "";
        if (!inTessEval)
            inTessEval = "";
        if (!inGeom)
            inGeom = "";
        if (!inFrag)
            inFrag = "";

        QT3DS_PERF_SCOPED_TIMER(m_PerfTimer, "Shader Compilation")
        m_VertexCode.assign(inVert);
        m_TessCtrlCode.assign(inTessCtrl);
        m_TessEvalCode.assign(inTessEval);
        m_GeometryCode.assign(inGeom);
        m_FragmentCode.assign(inFrag);

        if (!fromDisk)
            addShaderPreprocessors(inKey, inFlags, inFeatures, separableProgram, false);

        NVRenderVertFragCompilationResult res = m_RenderContext
                .CompileSource(inKey, m_VertexCode.c_str(), QT3DSU32(m_VertexCode.size()),
                               m_FragmentCode.c_str(), QT3DSU32(m_FragmentCode.size()),
                               m_TessCtrlCode.c_str(), QT3DSU32(m_TessCtrlCode.size()),
                               m_TessEvalCode.c_str(), QT3DSU32(m_TessEvalCode.size()),
                               m_GeometryCode.c_str(), QT3DSU32(m_GeometryCode.size()),
                               separableProgram);
        theInserter.first->second = res.mShader;
        errors = res.errors;

        // This is unnecessary memory waste in final deployed product, so we don't store this
        // information when shaders were initialized from a cache.
        // Unfortunately it is not practical to just regenerate shader source from scratch, when we
        // want to export it, as the triggers and original sources are spread all over the place.
        if (!m_shadersInitializedFromCache && theInserter.first->second) {
            // Store sources for possible cache generation later
            ShaderSource ss;
            for (QT3DSU32 i = 0, end = inFeatures.size(); i < end; ++i)
                ss.features.append(inFeatures[i]);
            ss.key = inKey;
            ss.flags = inFlags;
            ss.vertexCode = inVert;
            ss.fragmentCode = inFrag;
            ss.tessCtrlCode = inTessCtrl;
            ss.tessEvalCode = inTessEval;
            ss.geometryCode = inGeom;
            m_shaderSourceCache.append(ss);
        }

        return theInserter.first->second;
    }

    virtual NVRenderShaderProgram *
    CompileProgram(CRegisteredString inKey, const char8_t *inVert, const char8_t *inFrag,
                   const char8_t *inTessCtrl, const char8_t *inTessEval, const char8_t *inGeom,
                   const SShaderCacheProgramFlags &inFlags,
                   NVConstDataRef<SShaderPreprocessorFeature> inFeatures,
                   QString &errors, bool separableProgram) override
    {
        NVRenderShaderProgram *theProgram = GetProgram(inKey, inFeatures);
        if (theProgram)
            return theProgram;

        NVRenderShaderProgram *retval =
            ForceCompileProgram(inKey, inVert, inFrag, inTessCtrl, inTessEval, inGeom, inFlags,
                                inFeatures, errors, separableProgram);
        return retval;
    }

    // Magic number to identify cache file type
    const quint32 shaderCacheFileId = 0x26a9b358;

    QByteArray exportShaderCache(bool binaryShaders) override
    {
        if (m_shadersInitializedFromCache) {
            qWarning() << __FUNCTION__ << "Warning: Shader cache export is not supported when"
                                          " shaders were originally imported from a cache file.";
            return {};
        }

        // The assumption is that cache was generated on the same environment it will be read.
        // Attempting to load a cache generated on another environment will likely lead to crash.

        QByteArray retval;
        QDataStream data(&retval, QIODevice::WriteOnly);
        bool saveBinary = binaryShaders && m_RenderContext.isBinaryProgramSupported();
        data << shaderCacheFileId;
        data << saveBinary;
        data << IShaderCache::shaderCacheVersion();
        data << m_shaderSourceCache.size();

        for (const auto &ss : qAsConst(m_shaderSourceCache))
        {
            data << QByteArray(ss.key.c_str());
            data << ss.features.size();
            for (int i = 0, end = ss.features.size(); i < end; ++i) {
                data << QByteArray(ss.features[i].m_Name.c_str());
                data << ss.features[i].m_Enabled;
            }

            if (saveBinary) {
                TShaderFeatureSet features(ss.features.constData(), QT3DSU32(ss.features.size()));
                NVRenderShaderProgram *program = GetProgram(ss.key, features);
                QT3DSU32 format;
                QByteArray binaryData;
                program->getProgramBinary(format, binaryData);
                data << format;
                data << binaryData;
            } else {
                m_VertexCode.assign(ss.vertexCode.constData());
                m_FragmentCode.assign(ss.fragmentCode.constData());
                m_TessCtrlCode.assign(ss.tessCtrlCode.constData());
                m_TessEvalCode.assign(ss.tessEvalCode.constData());
                m_GeometryCode.assign(ss.geometryCode.constData());
                addShaderPreprocessors(
                            ss.key, ss.flags,
                            qt3ds::foundation::toConstDataRef(
                                ss.features.constData(), static_cast<QT3DSU32>(ss.features.size())),
                            false, true);
                auto writeShaderElement = [&data](const QByteArray &shaderSource) {
                    QByteArray stripped = shaderSource;
                    int start = stripped.indexOf(QLatin1String("/*"));
                    while (start != -1) {
                        int end = stripped.indexOf(QLatin1String("*/"));
                        if (end == -1)
                            break; // Mismatched comment
                        stripped.replace(start, end - start + 2, QByteArray());
                        start = stripped.indexOf(QLatin1String("/*"));
                    }
                    data << stripped;
                };

                writeShaderElement(m_VertexCode.toUtf8());
                writeShaderElement(m_FragmentCode.toUtf8());
                writeShaderElement(m_TessCtrlCode.toUtf8());
                writeShaderElement(m_TessEvalCode.toUtf8());
                writeShaderElement(m_GeometryCode.toUtf8());
            }
        }
        return retval;
    }

    void importShaderCache(const QByteArray &shaderCache) override
    {
        #define BAILOUT(details) { \
            qWarning() << "importShaderCache failed to import shader cache:" << details; \
            return; \
        }

        if (shaderCache.isEmpty())
            BAILOUT("Shader cache Empty")

        QT3DS_PERF_SCOPED_TIMER(m_PerfTimer, "ShaderCache - Import")

        QDataStream data(shaderCache);
        quint32 type;
        quint32 version;
        bool isBinary;
        data >> type;
        if (type != shaderCacheFileId)
            BAILOUT("Not a shader cache")
        data >> isBinary;
        if (isBinary && !m_RenderContext.isBinaryProgramSupported())
            BAILOUT("Binary shaders are not supported")
        data >> version;
        if (version != IShaderCache::shaderCacheVersion())
            BAILOUT("Version mismatch")

        #undef BAILOUT

        IStringTable &stringTable(m_RenderContext.GetStringTable());

        int progCount;
        data >> progCount;
        m_shadersInitializedFromCache = progCount > 0;
        for (int i = 0; i < progCount; ++i) {
            QByteArray key;
            int featCount;
            CRegisteredString theKey;
            data >> key;
            data >> featCount;
            theKey = stringTable.RegisterStr(key);
            eastl::vector<SShaderPreprocessorFeature> features;
            for (int j = 0; j < featCount; ++j) {
                QByteArray featName;
                bool featVal;
                data >> featName;
                data >> featVal;
                CRegisteredString regName = stringTable.RegisterStr(featName);
                features.push_back(SShaderPreprocessorFeature(regName, featVal));
            }
            NVRenderShaderProgram *theShader = nullptr;
            if (isBinary) {
                QT3DSU32 format;
                QByteArray binary;
                data >> format;
                data >> binary;

                SShaderCacheKey tempKey(theKey);
                tempKey.m_Features.assign(features.begin(), features.end());
                tempKey.GenerateHashCode();

                qCInfo(TRACE_INFO) << "Loading binary program from shader cache: '<" << key << ">'";

                eastl::pair<TShaderMap::iterator, bool> theInserter = m_Shaders.insert(tempKey);
                theInserter.first->second
                    = m_RenderContext.CompileBinary(theKey, format, binary).mShader;
                theShader = theInserter.first->second;
            } else {
                QByteArray loadVertexData;
                QByteArray loadFragmentData;
                QByteArray loadTessControlData;
                QByteArray loadTessEvalData;
                QByteArray loadGeometryData;

                data >> loadVertexData;
                data >> loadFragmentData;
                data >> loadTessControlData;
                data >> loadTessEvalData;
                data >> loadGeometryData;

                if (!loadVertexData.isEmpty() && (!loadFragmentData.isEmpty()
                                                  || !loadGeometryData.isEmpty())) {
                    QString error;
                    theShader = ForceCompileProgram(
                                theKey, loadVertexData.constData(),
                                loadFragmentData.constData(),
                                loadTessControlData.constData(),
                                loadTessEvalData.constData(),
                                loadGeometryData.constData(),
                                SShaderCacheProgramFlags(),
                                qt3ds::foundation::toDataRef(
                                    features.data(), static_cast<QT3DSU32>(features.size())),
                                error, false, true);
                }
            }
            // If something doesn't save or load correctly, get the runtime to re-generate.
            if (!theShader) {
                qWarning() << __FUNCTION__ << "Failed to load a cached a shader:" << key;
                m_Shaders.erase(theKey);
            }
        }
    }

    void SetShaderCompilationEnabled(bool inEnableShaderCompilation) override
    {
        m_ShaderCompilationEnabled = inEnableShaderCompilation;
    }
};
}

size_t qt3ds::render::HashShaderFeatureSet(NVConstDataRef<SShaderPreprocessorFeature> inFeatureSet)
{
    size_t retval(0);
    for (QT3DSU32 idx = 0, end = inFeatureSet.size(); idx < end; ++idx) {
        // From previous implementation, it seems we need to ignore the order of the features.
        // But we need to bind the feature flag together with its name, so that the flags will
        // influence
        // the final hash not only by the true-value count.
        retval = retval
            ^ (inFeatureSet[idx].m_Name.hash() * eastl::hash<bool>()(inFeatureSet[idx].m_Enabled));
    }
    return retval;
}

bool SShaderPreprocessorFeature::operator<(const SShaderPreprocessorFeature &other) const
{
    return strcmp(m_Name.c_str(), other.m_Name.c_str()) < 0;
}

bool SShaderPreprocessorFeature::operator==(const SShaderPreprocessorFeature &other) const
{
    return m_Name == other.m_Name && m_Enabled == other.m_Enabled;
}

IShaderCache &IShaderCache::CreateShaderCache(NVRenderContext &inContext,
                                              IInputStreamFactory &inInputStreamFactory,
                                              IPerfTimer &inPerfTimer)
{
    return *QT3DS_NEW(inContext.GetAllocator(), ShaderCache)(inContext, inInputStreamFactory,
                                                          inPerfTimer);
}
