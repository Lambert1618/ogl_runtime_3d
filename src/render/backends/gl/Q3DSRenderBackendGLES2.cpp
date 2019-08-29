/****************************************************************************
**
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

#include "render/backends/gl/Q3DSRenderBackendGLES2.h"
#include "render/backends/gl/Qt3DSRenderBackendInputAssemblerGL.h"
#include "render/backends/gl/Qt3DSRenderBackendRenderStatesGL.h"
#include "render/backends/gl/Qt3DSRenderBackendShaderProgramGL.h"

#include <QtGui/qopenglcontext.h>

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError(#x, __FILE__, __LINE__)
#else
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError()
#endif

#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
#define GL_CALL_TIMER_EXT(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_TESSELLATION_EXT(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_MULTISAMPLE_EXT(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_EXTRA_FUNCTION(x) m_glExtraFunctions->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_EXTENSION_FUNCTION(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#else
#define GL_CALL_TIMER_EXT(x)
#define GL_CALL_TESSELLATION_EXT(x)
#define GL_CALL_MULTISAMPLE_EXT(x)
#define GL_CALL_EXTRA_FUNCTION(x)  m_glExtraFunctions->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_EXTENSION_FUNCTION(x)
#endif

#ifndef GL_DEPTH_STENCIL_OES
#define GL_DEPTH_STENCIL_OES 0x84F9
#endif

namespace qt3ds {
namespace render {

/// constructor
NVRenderBackendGLES2Impl::NVRenderBackendGLES2Impl(NVFoundationBase &fnd,
                                                   qt3ds::foundation::IStringTable &stringTable,
                                                   const QSurfaceFormat &format)
    : NVRenderBackendGLBase(fnd, stringTable, format)
{
    QString exts3tc = QStringLiteral("GL_EXT_texture_compression_s3tc");
    QString extsdxt = QStringLiteral("GL_EXT_texture_compression_dxt1");
    QString extsAniso = QStringLiteral("GL_EXT_texture_filter_anisotropic");
    QString extsTexSwizzle = QStringLiteral("GL_ARB_texture_swizzle");
    QString extsFPRenderTarget = QStringLiteral("GL_EXT_color_buffer_float");
    QString extsTimerQuery = QStringLiteral("GL_EXT_timer_query");
    QString extsGpuShader5 = QStringLiteral("EXT_gpu_shader5");
    QString extDepthTexture = QStringLiteral("GL_OES_packed_depth_stencil");
    QString extvao = QStringLiteral("GL_OES_vertex_array_object");
    QString extStdDd = QStringLiteral("GL_OES_standard_derivatives");
    QString extTexLod = QStringLiteral("GL_EXT_shader_texture_lod");

    const char *languageVersion = GetShadingLanguageVersion();
    qCInfo(TRACE_INFO, "GLSL version: %s", languageVersion);

    eastl::string apiVersion(getVersionString());
    qCInfo(TRACE_INFO, "GL version: %s", apiVersion.c_str());

    eastl::string apiVendor(getVendorString());
    qCInfo(TRACE_INFO, "HW vendor: %s", apiVendor.c_str());

    eastl::string apiRenderer(getRendererString());
    qCInfo(TRACE_INFO, "Vendor renderer: %s", apiRenderer.c_str());

    // clear support bits
    m_backendSupport.caps.u32Values = 0;

    const char *extensions = getExtensionString();
    m_extensions = QString::fromLocal8Bit(extensions).split(" ");

    // get extension count
    GLint numExtensions = m_extensions.size();

    for (QT3DSI32 i = 0; i < numExtensions; i++) {

        const QString &extensionString = m_extensions.at(i);

        // search for extension
        if (!m_backendSupport.caps.bits.bDXTImagesSupported
            && (exts3tc.compare(extensionString) == 0 || extsdxt.compare(extensionString) == 0)) {
            m_backendSupport.caps.bits.bDXTImagesSupported = true;
        } else if (!m_backendSupport.caps.bits.bAnistropySupported
                   && extsAniso.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bAnistropySupported = true;
        } else if (!m_backendSupport.caps.bits.bFPRenderTargetsSupported
                   && extsFPRenderTarget.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bFPRenderTargetsSupported = true;
        } else if (!m_backendSupport.caps.bits.bTimerQuerySupported
                   && extsTimerQuery.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bTimerQuerySupported = true;
        } else if (!m_backendSupport.caps.bits.bGPUShader5ExtensionSupported
                   && extsGpuShader5.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bGPUShader5ExtensionSupported = true;
        } else if (!m_backendSupport.caps.bits.bTextureSwizzleSupported
                   && extsTexSwizzle.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bTextureSwizzleSupported = true;
        } else if (!m_backendSupport.caps.bits.bDepthStencilSupported
                   && extDepthTexture.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bDepthStencilSupported = true;
        } else if (!m_backendSupport.caps.bits.bVertexArrayObjectSupported
                   && extvao.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bVertexArrayObjectSupported = true;
        } else if (!m_backendSupport.caps.bits.bStandardDerivativesSupported
                   && extStdDd.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bStandardDerivativesSupported = true;
        } else if (!m_backendSupport.caps.bits.bTextureLodSupported
                   && extTexLod.compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bTextureLodSupported = true;
        }
    }

    qCInfo(TRACE_INFO, "OpenGL extensions: %s", extensions);

    // constant buffers support is always not true
    m_backendSupport.caps.bits.bConstantBufferSupported = false;

    // Binary programs are never supported
    m_backendSupport.caps.bits.bBinaryProgramsSupported = false;

    // query hardware
    GL_CALL_EXTRA_FUNCTION(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_MaxAttribCount));

    // internal state tracker
    m_pCurrentMiscState = QT3DS_NEW(m_Foundation.getAllocator(), NVRenderBackendMiscStateGL)();

    // finally setup caps based on device
    setAndInspectHardwareCaps();

    // Initialize extensions
#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
    m_qt3dsExtensions = new Qt3DSOpenGLES2Extensions;
    m_qt3dsExtensions->initializeOpenGLFunctions();
#endif
}
/// destructor
NVRenderBackendGLES2Impl::~NVRenderBackendGLES2Impl()
{
    if (m_pCurrentMiscState)
        NVDelete(m_Foundation.getAllocator(), m_pCurrentMiscState);
#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
    if (m_qt3dsExtensions)
        delete m_qt3dsExtensions;
#endif
}

void NVRenderBackendGLES2Impl::SetMultisampledTextureData2D(
        NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, size_t samples,
        NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height,
        bool fixedsamplelocations)
{
    NVRENDER_BACKEND_UNUSED(to);
    NVRENDER_BACKEND_UNUSED(target);
    NVRENDER_BACKEND_UNUSED(samples);
    NVRENDER_BACKEND_UNUSED(internalFormat);
    NVRENDER_BACKEND_UNUSED(width);
    NVRENDER_BACKEND_UNUSED(height);
    NVRENDER_BACKEND_UNUSED(fixedsamplelocations);
}

void NVRenderBackendGLES2Impl::SetTextureData3D(
        NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, QT3DSU32 level,
        NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height, size_t depth,
        QT3DSI32 border, NVRenderTextureFormats::Enum format, const void *hostPtr)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GL_CALL_EXTRA_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));
    bool conversionRequired = format != internalFormat;

    NVRenderTextureSwizzleMode::Enum swizzleMode = NVRenderTextureSwizzleMode::NoSwizzle;
    internalFormat = m_Conversion.replaceDeprecatedTextureFormat(GetRenderContextType(),
                                                                 internalFormat, swizzleMode);

    GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;

    if (NVRenderTextureFormats::isUncompressedTextureFormat(internalFormat)) {
        m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), internalFormat,
                                                       glformat, gltype, glInternalFormat);
    }

    if (conversionRequired) {
        GLenum dummy;
        m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                       gltype, dummy);
    } else if (NVRenderTextureFormats::isCompressedTextureFormat(internalFormat)) {
        m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                       gltype, glInternalFormat);
        glInternalFormat = m_Conversion.fromCompressedTextureFormatToGL(internalFormat);
    } else if (NVRenderTextureFormats::isDepthTextureFormat(format)) {
        m_Conversion.fromDepthTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                gltype, glInternalFormat);
    }

    GL_CALL_EXTRA_FUNCTION(glTexImage3D(glTarget, level, glInternalFormat, (GLsizei)width,
                                        (GLsizei)height, (GLsizei)depth, border, glformat,
                                        gltype, hostPtr));

    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

void NVRenderBackendGLES2Impl::SetTextureData2D(
        NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, QT3DSU32 level,
        NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height, QT3DSI32 border,
        NVRenderTextureFormats::Enum format, const void *hostPtr)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GL_CALL_EXTRA_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));
    bool conversionRequired = format != internalFormat;

    NVRenderTextureSwizzleMode::Enum swizzleMode = NVRenderTextureSwizzleMode::NoSwizzle;
    internalFormat = m_Conversion.replaceDeprecatedTextureFormat(GetRenderContextType(),
                                                                 internalFormat, swizzleMode);

    GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;

    if (NVRenderTextureFormats::isUncompressedTextureFormat(internalFormat)) {
        m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), internalFormat,
                                                       glformat, gltype, glInternalFormat);
        glInternalFormat = glformat;
    }

    if (conversionRequired) {
        GLenum dummy;
        m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                       gltype, dummy);
    } else if (NVRenderTextureFormats::isCompressedTextureFormat(internalFormat)) {
        m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                       gltype, glInternalFormat);
        glInternalFormat = m_Conversion.fromCompressedTextureFormatToGL(internalFormat);
    } else if (NVRenderTextureFormats::isDepthTextureFormat(format)) {
        m_Conversion.fromDepthTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                gltype, glInternalFormat);
        if (format == NVRenderTextureFormats::Depth24Stencil8) {
            glformat = GL_DEPTH_STENCIL_OES;
            gltype = GL_UNSIGNED_INT_24_8;
        }
        glInternalFormat = glformat;
    }

    Q_ASSERT(glformat == glInternalFormat);
    GL_CALL_EXTRA_FUNCTION(glTexImage2D(glTarget, level, glInternalFormat, (GLsizei)width,
                                        (GLsizei)height, border, glformat, gltype, hostPtr));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

void NVRenderBackendGLES2Impl::UpdateSampler(
        NVRenderBackendSamplerObject /* so */, NVRenderTextureTargetType::Enum target,
        NVRenderTextureMinifyingOp::Enum minFilter, NVRenderTextureMagnifyingOp::Enum magFilter,
        NVRenderTextureCoordOp::Enum wrapS, NVRenderTextureCoordOp::Enum wrapT,
        NVRenderTextureCoordOp::Enum wrapR, QT3DSF32 minLod, QT3DSF32 maxLod, QT3DSF32 lodBias,
        NVRenderTextureCompareMode::Enum compareMode, NVRenderTextureCompareOp::Enum compareFunc,
        QT3DSF32 anisotropy, QT3DSF32 *borderColor)
{

    // Satisfy the compiler
    // These are not available in GLES 3 and we don't use them right now
    QT3DS_ASSERT(lodBias == 0.0);
    QT3DS_ASSERT(!borderColor);
    NVRENDER_BACKEND_UNUSED(lodBias);
    NVRENDER_BACKEND_UNUSED(borderColor);
    NVRENDER_BACKEND_UNUSED(wrapR);
    NVRENDER_BACKEND_UNUSED(minLod);
    NVRENDER_BACKEND_UNUSED(maxLod);
    NVRENDER_BACKEND_UNUSED(compareMode);
    NVRENDER_BACKEND_UNUSED(compareFunc);

    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);

    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER,
                                           m_Conversion.fromTextureMinifyingOpToGL(minFilter)));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER,
                                           m_Conversion.fromTextureMagnifyingOpToGL(magFilter)));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_S,
                                           m_Conversion.fromTextureCoordOpToGL(wrapS)));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_T,
                                           m_Conversion.fromTextureCoordOpToGL(wrapT)));

    if (m_backendSupport.caps.bits.bAnistropySupported) {
        GL_CALL_EXTRA_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                               anisotropy));
    }
}

void NVRenderBackendGLES2Impl::UpdateTextureObject(NVRenderBackendTextureObject to,
                                                   NVRenderTextureTargetType::Enum target,
                                                   QT3DSI32 baseLevel, QT3DSI32 maxLevel)
{
    NVRENDER_BACKEND_UNUSED(to);

    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);

    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_BASE_LEVEL, baseLevel));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, maxLevel));
}

void NVRenderBackendGLES2Impl::UpdateTextureSwizzle(NVRenderBackendTextureObject to,
                                                    NVRenderTextureTargetType::Enum target,
                                                    NVRenderTextureSwizzleMode::Enum swizzleMode)
{
    NVRENDER_BACKEND_UNUSED(to);
    NVRENDER_BACKEND_UNUSED(target);
    NVRENDER_BACKEND_UNUSED(swizzleMode);
#if defined(QT_OPENGL_ES)
    if (m_backendSupport.caps.bits.bTextureSwizzleSupported) {
        GLint glSwizzle[4];
        GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
        m_Conversion.NVRenderConvertSwizzleModeToGL(swizzleMode, glSwizzle);

        // since ES3 spec has no GL_TEXTURE_SWIZZLE_RGBA set it separately
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_R, glSwizzle[0]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_G, glSwizzle[1]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_B, glSwizzle[2]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_A, glSwizzle[3]));

    }
#endif
}

QT3DSU32
NVRenderBackendGLES2Impl::GetDepthBits() const
{
    QT3DSI32 depthBits;
    GL_CALL_EXTRA_FUNCTION(
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                              GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits));

    return depthBits;
}

QT3DSU32
NVRenderBackendGLES2Impl::GetStencilBits() const
{
    QT3DSI32 stencilBits;
    GL_CALL_EXTRA_FUNCTION(
                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
                                                      GL_DEPTH_ATTACHMENT,
                                                      GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
                                                      &stencilBits));

    return stencilBits;
}

void NVRenderBackendGLES2Impl::GenerateMipMaps(NVRenderBackendTextureObject to,
                                               NVRenderTextureTargetType::Enum target,
                                               NVRenderHint::Enum /*genType*/)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GL_CALL_EXTRA_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));
    GL_CALL_EXTRA_FUNCTION(glGenerateMipmap(glTarget));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

bool NVRenderBackendGLES2Impl::SetInputAssembler(NVRenderBackendInputAssemblerObject iao,
                                                 NVRenderBackendShaderProgramObject po)
{
    if (iao == nullptr) {
        // unbind and return;
        GL_CALL_EXTENSION_FUNCTION(glBindVertexArrayOES(0));
        return true;
    }

    NVRenderBackendInputAssemblerGL *inputAssembler = (NVRenderBackendInputAssemblerGL *)iao;
    NVRenderBackendAttributeLayoutGL *attribLayout = inputAssembler->m_attribLayout;
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);
    NVDataRef<NVRenderBackendShaderInputEntryGL> shaderAttribBuffer;
    if (pProgram->m_shaderInput)
        shaderAttribBuffer = pProgram->m_shaderInput->m_ShaderInputEntries;

    if ((attribLayout->m_LayoutAttribEntries.size() < shaderAttribBuffer.size())
            || (inputAssembler->m_VertexbufferHandles.size() <= attribLayout->m_MaxInputSlot)) {
        return false;
    }

    if (inputAssembler->m_VaoID == 0) {
        // generate vao
        GL_CALL_EXTENSION_FUNCTION(glGenVertexArraysOES(1, &inputAssembler->m_VaoID));
        QT3DS_ASSERT(inputAssembler->m_VaoID);
    }

    if (inputAssembler->m_cachedShaderHandle != programID) {
        GL_CALL_EXTENSION_FUNCTION(glBindVertexArrayOES(inputAssembler->m_VaoID));
        inputAssembler->m_cachedShaderHandle = programID;

        QT3DS_FOREACH(idx, shaderAttribBuffer.size())
        {
            const NVRenderBackendShaderInputEntryGL &attrib(shaderAttribBuffer[idx]);
            NVRenderBackendLayoutEntryGL *entry =
                    attribLayout->getEntryByName(attrib.m_AttribName);

            if (entry) {
                NVRenderBackendLayoutEntryGL &entryData(*entry);
                if (entryData.m_Type != attrib.m_Type
                        || entryData.m_NumComponents != attrib.m_NumComponents) {
                    qCCritical(INVALID_OPERATION, "Attrib %s dn't match vertex layout",
                               attrib.m_AttribName.c_str());
                    QT3DS_ASSERT(false);
                    return false;
                } else {
                    entryData.m_AttribIndex = attrib.m_AttribLocation;
                }
            } else {
                qCWarning(WARNING, "Failed to Bind attribute %s", attrib.m_AttribName.c_str());
            }
        }

        // disable max possible used first
        // this is currently sufficient since we always re-arrange input attributes from 0
        for (QT3DSU32 i = 0; i < attribLayout->m_LayoutAttribEntries.size(); i++)
            GL_CALL_EXTRA_FUNCTION(glDisableVertexAttribArray(i));

        // setup all attribs
        QT3DS_FOREACH(idx, shaderAttribBuffer.size())
        {
            NVRenderBackendLayoutEntryGL *entry =
                    attribLayout->getEntryByName(shaderAttribBuffer[idx].m_AttribName);
            if (entry) {
                const NVRenderBackendLayoutEntryGL &entryData(*entry);
                GLuint id = HandleToID_cast(
                            GLuint, size_t,
                            inputAssembler->m_VertexbufferHandles.mData[entryData.m_InputSlot]);
                GL_CALL_EXTRA_FUNCTION(glBindBuffer(GL_ARRAY_BUFFER, id));
                GL_CALL_EXTRA_FUNCTION(glEnableVertexAttribArray(entryData.m_AttribIndex));
                GLuint offset = inputAssembler->m_offsets[entryData.m_InputSlot];
                GLuint stride = inputAssembler->m_strides[entryData.m_InputSlot];
                GL_CALL_EXTRA_FUNCTION(glVertexAttribPointer(
                                           entryData.m_AttribIndex, entryData.m_NumComponents, GL_FLOAT, GL_FALSE,
                                           stride, (const void *)(entryData.m_Offset + offset)));

            } else {
                GL_CALL_EXTRA_FUNCTION(glDisableVertexAttribArray(idx));
            }
        }

        // setup index buffer.
        if (inputAssembler->m_IndexbufferHandle) {
            GL_CALL_EXTRA_FUNCTION(glBindBuffer(
                                       GL_ELEMENT_ARRAY_BUFFER,
                                       HandleToID_cast(GLuint, size_t, inputAssembler->m_IndexbufferHandle)));
        } else {
            GL_CALL_EXTRA_FUNCTION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        }
    } else {
        GL_CALL_EXTENSION_FUNCTION(glBindVertexArrayOES(inputAssembler->m_VaoID));
    }
#ifdef _DEBUG
    if (inputAssembler->m_VaoID) {
        QT3DS_FOREACH(idx, shaderAttribBuffer.size())
        {
            const NVRenderBackendShaderInputEntryGL &attrib(shaderAttribBuffer[idx]);
            NVRenderBackendLayoutEntryGL *entry =
                    attribLayout->getEntryByName(attrib.m_AttribName);

            if (entry) {
                NVRenderBackendLayoutEntryGL &entryData(*entry);
                if (entryData.m_Type != attrib.m_Type
                        || entryData.m_NumComponents != attrib.m_NumComponents
                        || entryData.m_AttribIndex != attrib.m_AttribLocation) {
                    qCCritical(INVALID_OPERATION, "Attrib %s dn't match vertex layout",
                               attrib.m_AttribName.c_str());
                    QT3DS_ASSERT(false);
                }
            } else {
                qCWarning(WARNING, "Failed to Bind attribute %s", attrib.m_AttribName.c_str());
            }
        }
    }
#endif // _DEBUG

    return true;
}

void NVRenderBackendGLES2Impl::ReleaseInputAssembler(
        NVRenderBackend::NVRenderBackendInputAssemblerObject iao)
{
    NVRenderBackendInputAssemblerGL *inputAssembler = (NVRenderBackendInputAssemblerGL *)iao;
    if (inputAssembler->m_VaoID)
        GL_CALL_EXTENSION_FUNCTION(glDeleteVertexArraysOES(1, &inputAssembler->m_VaoID));
    NVDelete(m_Foundation.getAllocator(), inputAssembler);
}

void NVRenderBackendGLES2Impl::SetDrawBuffers(NVRenderBackendRenderTargetObject rto,
                                              NVConstDataRef<QT3DSI32> inDrawBufferSet)
{
    NVRENDER_BACKEND_UNUSED(rto);

    m_DrawBuffersArray.clear();

    for (QT3DSU32 idx = 0, end = inDrawBufferSet.size(); idx < end; ++idx) {
        if (inDrawBufferSet[idx] < 0)
            m_DrawBuffersArray.push_back(GL_NONE);
        else
            m_DrawBuffersArray.push_back(GL_COLOR_ATTACHMENT0 + inDrawBufferSet[idx]);
    }

    GL_CALL_EXTRA_FUNCTION(glDrawBuffers((int)m_DrawBuffersArray.size(),
                                         m_DrawBuffersArray.data()));
}

void NVRenderBackendGLES2Impl::SetReadBuffer(NVRenderBackendRenderTargetObject rto,
                                             NVReadFaces::Enum inReadFace)
{
    NVRENDER_BACKEND_UNUSED(rto);
    NVRENDER_BACKEND_UNUSED(inReadFace);
}

void NVRenderBackendGLES2Impl::RenderTargetAttach(NVRenderBackendRenderTargetObject,
                                                  NVRenderFrameBufferAttachments::Enum attachment,
                                                  NVRenderBackendTextureObject to, QT3DSI32 level,
                                                  QT3DSI32 layer)
{
    NVRENDER_BACKEND_UNUSED(attachment);
    NVRENDER_BACKEND_UNUSED(to);
    NVRENDER_BACKEND_UNUSED(level);
    NVRENDER_BACKEND_UNUSED(layer);
    Q_ASSERT(false);
}

void NVRenderBackendGLES2Impl::BlitFramebuffer(QT3DSI32 srcX0, QT3DSI32 srcY0, QT3DSI32 srcX1,
                                               QT3DSI32 srcY1, QT3DSI32 dstX0, QT3DSI32 dstY0,
                                               QT3DSI32 dstX1, QT3DSI32 dstY1,
                                               NVRenderClearFlags flags,
                                               NVRenderTextureMagnifyingOp::Enum filter)
{
    GL_CALL_EXTRA_FUNCTION(glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                                             dstY1,
                                             m_Conversion.fromClearFlagsToGL(flags),
                                             m_Conversion.fromTextureMagnifyingOpToGL(filter)));
}


NVRenderBackend::NVRenderBackendRenderTargetObject NVRenderBackendGLES2Impl::CreateRenderTarget()
{
    GLuint fboID = 0;
    GL_CALL_EXTRA_FUNCTION(glGenFramebuffers(1, &fboID));
    return (NVRenderBackend::NVRenderBackendRenderTargetObject)fboID;
}

void NVRenderBackendGLES2Impl::ReleaseRenderTarget(NVRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, size_t, rto);

    if (fboID)
        GL_CALL_EXTRA_FUNCTION(glDeleteFramebuffers(1, &fboID));
}

void NVRenderBackendGLES2Impl::RenderTargetAttach(NVRenderBackendRenderTargetObject /* rto */,
                                                  NVRenderFrameBufferAttachments::Enum attachment,
                                                  NVRenderBackendRenderbufferObject rbo)
{
    // rto must be the current render target
    GLuint rbID = HandleToID_cast(GLuint, size_t, rbo);

    GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);

    GL_CALL_EXTRA_FUNCTION(glFramebufferRenderbuffer(GL_FRAMEBUFFER, glAttach, GL_RENDERBUFFER,
                                                     rbID));
}

void NVRenderBackendGLES2Impl::RenderTargetAttach(NVRenderBackendRenderTargetObject /* rto */,
                                                  NVRenderFrameBufferAttachments::Enum attachment,
                                                  NVRenderBackendTextureObject to,
                                                  NVRenderTextureTargetType::Enum target)
{
    // rto must be the current render target
    GLuint texID = HandleToID_cast(GLuint, size_t, to);

    QT3DS_ASSERT(target == NVRenderTextureTargetType::Texture2D
                 || m_backendSupport.caps.bits.bMsTextureSupported);

    GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);

    if (attachment == NVRenderFrameBufferAttachments::DepthStencil) {
        GL_CALL_EXTRA_FUNCTION(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                      glTarget, texID, 0));
        GL_CALL_EXTRA_FUNCTION(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                      glTarget, texID, 0));
    } else {
        GL_CALL_EXTRA_FUNCTION(glFramebufferTexture2D(GL_FRAMEBUFFER, glAttach, glTarget, texID,
                                                      0));
    }
}

void NVRenderBackendGLES2Impl::SetRenderTarget(NVRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, size_t, rto);
    if (!fboID)
        fboID = QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext()->defaultFramebufferObject();

    GL_CALL_EXTRA_FUNCTION(glBindFramebuffer(GL_FRAMEBUFFER, fboID));
}

void NVRenderBackendGLES2Impl::SetReadTarget(NVRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, size_t, rto);
    if (!fboID)
        fboID = QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext()->defaultFramebufferObject();

    GL_CALL_EXTRA_FUNCTION(glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID));
}

bool NVRenderBackendGLES2Impl::RenderTargetIsValid(NVRenderBackendRenderTargetObject /* rto */)
{
    GLenum completeStatus = GL_CALL_EXTRA_FUNCTION(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    switch (completeStatus) {
#define HANDLE_INCOMPLETE_STATUS(x)                                                                \
    case x:                                                                                        \
    qCCritical(INTERNAL_ERROR, "Framebuffer is not complete: %s", #x);           \
    return false;
    HANDLE_INCOMPLETE_STATUS(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
            HANDLE_INCOMPLETE_STATUS(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
            HANDLE_INCOMPLETE_STATUS(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
            HANDLE_INCOMPLETE_STATUS(GL_FRAMEBUFFER_UNSUPPORTED)
        #undef HANDLE_INCOMPLETE_STATUS
    }
    return true;
}

NVRenderBackend::NVRenderBackendRenderbufferObject
NVRenderBackendGLES2Impl::CreateRenderbuffer(NVRenderRenderBufferFormats::Enum storageFormat,
                                             size_t width, size_t height)
{
    GLuint bufID = 0;

    GL_CALL_EXTRA_FUNCTION(glGenRenderbuffers(1, &bufID));
    GL_CALL_EXTRA_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, bufID));
    GL_CALL_EXTRA_FUNCTION(glRenderbufferStorage(GL_RENDERBUFFER,
                                                 GLConversion::fromRenderBufferFormatsToRenderBufferGL(storageFormat),
                                                 (GLsizei)width, (GLsizei)height));

    // check for error
    GLenum error = m_glFunctions->glGetError();
    if (error != GL_NO_ERROR) {
        qCCritical(GL_ERROR) << GLConversion::processGLError(error);
        QT3DS_ASSERT(false);
        GL_CALL_EXTRA_FUNCTION(glDeleteRenderbuffers(1, &bufID));
        bufID = 0;
    }

    GL_CALL_EXTRA_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    return (NVRenderBackend::NVRenderBackendRenderbufferObject)bufID;
}

void NVRenderBackendGLES2Impl::ReleaseRenderbuffer(NVRenderBackendRenderbufferObject rbo)
{
    GLuint bufID = HandleToID_cast(GLuint, size_t, rbo);

    if (bufID)
        GL_CALL_EXTRA_FUNCTION(glDeleteRenderbuffers(1, &bufID));
}

bool NVRenderBackendGLES2Impl::ResizeRenderbuffer(NVRenderBackendRenderbufferObject rbo,
                                                  NVRenderRenderBufferFormats::Enum storageFormat,
                                                  size_t width, size_t height)
{
    bool success = true;
    GLuint bufID = HandleToID_cast(GLuint, size_t, rbo);

    QT3DS_ASSERT(bufID);

    GL_CALL_EXTRA_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, bufID));
    GL_CALL_EXTRA_FUNCTION(glRenderbufferStorage(GL_RENDERBUFFER,
                                                 GLConversion::fromRenderBufferFormatsToRenderBufferGL(storageFormat),
                                                 (GLsizei)width, (GLsizei)height));

    // check for error
    GLenum error = m_glFunctions->glGetError();
    if (error != GL_NO_ERROR) {
        qCCritical(GL_ERROR) << GLConversion::processGLError(error);
        QT3DS_ASSERT(false);
        success = false;
    }

    return success;
}

void *NVRenderBackendGLES2Impl::MapBuffer(NVRenderBackendBufferObject,
                                          NVRenderBufferBindFlags bindFlags, size_t offset,
                                          size_t length, NVRenderBufferAccessFlags accessFlags)
{
    void *ret = nullptr;
    ret = GL_CALL_EXTRA_FUNCTION(
                glMapBufferRange(m_Conversion.fromBindBufferFlagsToGL(bindFlags), offset,
                                 length, m_Conversion.fromBufferAccessBitToGL(accessFlags)));

    return ret;
}

bool NVRenderBackendGLES2Impl::UnmapBuffer(NVRenderBackendBufferObject,
                                           NVRenderBufferBindFlags bindFlags)
{
    GLboolean ret;

    ret = GL_CALL_EXTRA_FUNCTION(glUnmapBuffer(m_Conversion.fromBindBufferFlagsToGL(bindFlags)));

    return (ret) ? true : false;
}

QT3DSI32 NVRenderBackendGLES2Impl::GetConstantBufferCount(NVRenderBackendShaderProgramObject po)
{
    QT3DS_ASSERT(po);
    GLint numUniformBuffers = 0;
    if (GetRenderBackendCap(NVRenderBackendCaps::ConstantBuffer)) {
        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

        GL_CALL_EXTRA_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_UNIFORM_BLOCKS,
                                              &numUniformBuffers));
    }
    return numUniformBuffers;
}

QT3DSI32 NVRenderBackendGLES2Impl::GetConstantBufferInfoByID(
                                                        NVRenderBackendShaderProgramObject po,
                                                        QT3DSU32 id, QT3DSU32 nameBufSize,
                                                        QT3DSI32 *paramCount, QT3DSI32 *bufferSize,
                                                        QT3DSI32 *length, char *nameBuf)
{
    QT3DS_ASSERT(po);
    QT3DS_ASSERT(length);
    QT3DS_ASSERT(nameBuf);

    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);
    GLuint blockIndex = GL_INVALID_INDEX;

    GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockName(programID, id, nameBufSize, length,
                                                       nameBuf));

    if (*length > 0) {
        blockIndex = GL_CALL_EXTRA_FUNCTION(glGetUniformBlockIndex(programID, nameBuf));
        if (blockIndex != GL_INVALID_INDEX) {
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockiv(programID, blockIndex,
                                                             GL_UNIFORM_BLOCK_DATA_SIZE, bufferSize));
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockiv(programID, blockIndex,
                                                             GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, paramCount));
        }
    }

    return blockIndex;
}

void NVRenderBackendGLES2Impl::GetConstantBufferParamIndices(NVRenderBackendShaderProgramObject po,
                                                             QT3DSU32 id, QT3DSI32 *indices)
{
    QT3DS_ASSERT(po);
    QT3DS_ASSERT(indices);

    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

    if (indices) {
        GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockiv(programID, id,
                                                         GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                                                         indices));
    }
}

void NVRenderBackendGLES2Impl::GetConstantBufferParamInfoByIndices(
        NVRenderBackendShaderProgramObject po, QT3DSU32 count, QT3DSU32 *indices, QT3DSI32 *type,
        QT3DSI32 *size, QT3DSI32 *offset)
{
    QT3DS_ASSERT(po);
    QT3DS_ASSERT(count);
    QT3DS_ASSERT(indices);

    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

    if (count && indices) {
        if (type) {
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, count, indices,
                                                         GL_UNIFORM_TYPE, type));
            // convert to UIC types
            QT3DS_FOREACH(idx, count)
            {
                type[idx] = m_Conversion.fromShaderGLToPropertyDataTypes(type[idx]);
            }
        }
        if (size) {
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, count, indices,
                                                         GL_UNIFORM_SIZE, size));
        }
        if (offset) {
            GL_CALL_EXTRA_FUNCTION(
                        glGetActiveUniformsiv(programID, count, indices, GL_UNIFORM_OFFSET, offset));
        }
    }
}

void NVRenderBackendGLES2Impl::ProgramSetConstantBlock(NVRenderBackendShaderProgramObject po,
                                                       QT3DSU32 blockIndex, QT3DSU32 binding)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

    GL_CALL_EXTRA_FUNCTION(glUniformBlockBinding(programID, blockIndex, binding));
}

void NVRenderBackendGLES2Impl::ProgramSetConstantBuffer(QT3DSU32 index,
                                                        NVRenderBackendBufferObject bo)
{
    QT3DS_ASSERT(bo);

    GLuint bufID = HandleToID_cast(GLuint, size_t, bo);
    GL_CALL_EXTRA_FUNCTION(glBindBufferBase(GL_UNIFORM_BUFFER, index, bufID));
}

NVRenderBackend::NVRenderBackendQueryObject NVRenderBackendGLES2Impl::CreateQuery()
{
    QT3DSU32 glQueryID = 0;

    return (NVRenderBackendQueryObject)glQueryID;
}

void NVRenderBackendGLES2Impl::ReleaseQuery(NVRenderBackendQueryObject)
{

}

void NVRenderBackendGLES2Impl::BeginQuery(NVRenderBackendQueryObject,
                                          NVRenderQueryType::Enum)
{

}

void NVRenderBackendGLES2Impl::EndQuery(NVRenderBackendQueryObject, NVRenderQueryType::Enum)
{

}

void NVRenderBackendGLES2Impl::GetQueryResult(NVRenderBackendQueryObject,
                                              NVRenderQueryResultType::Enum,
                                              QT3DSU32 *)
{

}

void NVRenderBackendGLES2Impl::GetQueryResult(NVRenderBackendQueryObject,
                                              NVRenderQueryResultType::Enum,
                                              QT3DSU64 *)
{

}

void NVRenderBackendGLES2Impl::SetQueryTimer(NVRenderBackendQueryObject)
{

}

NVRenderBackend::NVRenderBackendSyncObject
NVRenderBackendGLES2Impl::CreateSync(NVRenderSyncType::Enum, NVRenderSyncFlags)
{
    GLsync syncID = 0;
    return NVRenderBackendSyncObject(syncID);
}

void NVRenderBackendGLES2Impl::ReleaseSync(NVRenderBackendSyncObject)
{

}

void NVRenderBackendGLES2Impl::WaitSync(NVRenderBackendSyncObject, NVRenderCommandFlushFlags,
                                        QT3DSU64)
{

}
}
}
