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

#include "render/backends/gl/Qt3DSRenderBackendGL4.h"
#include "render/backends/gl/Qt3DSRenderBackendInputAssemblerGL.h"
#include "render/backends/gl/Qt3DSRenderBackendShaderProgramGL.h"

#define NVRENDER_BACKEND_UNUSED(arg) (void)arg;

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError(#x, __FILE__, __LINE__)
#else
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError()
#endif

#define GL_CALL_EXTRA_FUNCTION(x) m_glExtraFunctions->x; RENDER_LOG_ERROR_PARAMS(x);

#if defined(QT_OPENGL_ES)
#define GL_CALL_NVPATH_EXT(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_QT3DS_EXT(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#else
#define GL_CALL_NVPATH_EXT(x) m_nvPathRendering->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_DIRECTSTATE_EXT(x) m_directStateAccess->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_QT3DS_EXT(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#endif

#ifndef GL_GEOMETRY_SHADER_EXT
#define GL_GEOMETRY_SHADER_EXT 0x8DD9
#endif

namespace qt3ds {
namespace render {

    /// constructor
    NVRenderBackendGL4Impl::NVRenderBackendGL4Impl(NVFoundationBase &fnd,
                                                   qt3ds::foundation::IStringTable &stringTable,
                                                   const QSurfaceFormat &format)
        : NVRenderBackendGL3Impl(fnd, stringTable, format)
    {
        eastl::string extTess("GL_ARB_tessellation_shader");
        eastl::string extGeometry("GL_EXT_geometry_shader4");
        eastl::string arbCompute("GL_ARB_compute_shader");
        eastl::string arbStorageBuffer("GL_ARB_shader_storage_buffer_object");
        eastl::string arbAtomicCounterBuffer("GL_ARB_shader_atomic_counters");
        eastl::string arbProgInterface("GL_ARB_program_interface_query");
        eastl::string arbShaderImageLoadStore("GL_ARB_shader_image_load_store");
        eastl::string nvPathRendering("GL_NV_path_rendering");
        eastl::string nvBlendAdvanced("GL_NV_blend_equation_advanced");
        eastl::string khrBlendAdvanced("GL_KHR_blend_equation_advanced");
        eastl::string nvBlendAdvancedCoherent("GL_NV_blend_equation_advanced_coherent");
        eastl::string khrBlendAdvancedCoherent("GL_KHR_blend_equation_advanced_coherent");

        eastl::string apiVersion(getVersionString());
        qCInfo(TRACE_INFO, "GL version: %s", apiVersion.c_str());

        // get extension count
        GLint numExtensions = 0;
        GL_CALL_EXTRA_FUNCTION(glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions));

        for (QT3DSI32 i = 0; i < numExtensions; i++) {
            char *extensionString = (char *)GL_CALL_EXTRA_FUNCTION(glGetStringi(GL_EXTENSIONS, i));

            // search for extension
            if (!m_backendSupport.caps.bits.bTessellationSupported
                && extTess.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bTessellationSupported = true;
            } else if (!m_backendSupport.caps.bits.bComputeSupported
                       && arbCompute.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bComputeSupported = true;
            } else if (!m_backendSupport.caps.bits.bGeometrySupported
                       && extGeometry.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bGeometrySupported = true;
            } else if (!m_backendSupport.caps.bits.bStorageBufferSupported
                       && arbStorageBuffer.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bStorageBufferSupported = true;
            } else if (!m_backendSupport.caps.bits.bAtomicCounterBufferSupported
                       && arbAtomicCounterBuffer.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bAtomicCounterBufferSupported = true;
            } else if (!m_backendSupport.caps.bits.bProgramInterfaceSupported
                       && arbProgInterface.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bProgramInterfaceSupported = true;
            } else if (!m_backendSupport.caps.bits.bShaderImageLoadStoreSupported
                       && arbShaderImageLoadStore.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bShaderImageLoadStoreSupported = true;
            } else if (!m_backendSupport.caps.bits.bNVPathRenderingSupported
                       && nvPathRendering.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bNVPathRenderingSupported = true;
            } else if (!m_backendSupport.caps.bits.bNVAdvancedBlendSupported
                       && nvBlendAdvanced.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bNVAdvancedBlendSupported = true;
            } else if (!m_backendSupport.caps.bits.bNVBlendCoherenceSupported
                       && nvBlendAdvancedCoherent.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bNVBlendCoherenceSupported = true;
            } else if (!m_backendSupport.caps.bits.bKHRAdvancedBlendSupported
                       && khrBlendAdvanced.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bKHRAdvancedBlendSupported = true;
            } else if (!m_backendSupport.caps.bits.bKHRBlendCoherenceSupported
                       && khrBlendAdvancedCoherent.compare(extensionString) == 0) {
                m_backendSupport.caps.bits.bKHRBlendCoherenceSupported = true;
            }
        }

        // always true for GL4.1 and GLES 3.1 devices
        m_backendSupport.caps.bits.bMsTextureSupported = true;
        m_backendSupport.caps.bits.bProgramPipelineSupported = true;
        m_backendSupport.caps.bits.bBinaryProgramsSupported = true;

        if (!isESCompatible()) {
            // TODO: investigate GL 4.0 support
            // we expect minimum GL 4.1 context anything beyond is handeled via extensions
            // Tessellation is always supported on none ES systems which support >=GL4
            m_backendSupport.caps.bits.bTessellationSupported = true;
            // geometry shader is always supported on none ES systems which support >=GL4 ( actually
            // 3.2 already )
            m_backendSupport.caps.bits.bGeometrySupported = true;
        } else {
            // always true for GLES 3.1 devices
            m_backendSupport.caps.bits.bComputeSupported = true;
            m_backendSupport.caps.bits.bProgramInterfaceSupported = true;
            m_backendSupport.caps.bits.bStorageBufferSupported = true;
            m_backendSupport.caps.bits.bAtomicCounterBufferSupported = true;
            m_backendSupport.caps.bits.bShaderImageLoadStoreSupported = true;
        }

#if !defined(QT_OPENGL_ES)
        // Initialize extensions
        m_nvPathRendering = QT3DS_NEW(m_Foundation.getAllocator(), QOpenGLExtension_NV_path_rendering)();
        m_nvPathRendering->initializeOpenGLFunctions();
        m_directStateAccess = QT3DS_NEW(m_Foundation.getAllocator(), QOpenGLExtension_EXT_direct_state_access)();
        m_directStateAccess->initializeOpenGLFunctions();
#endif
    }

    /// destructor
    NVRenderBackendGL4Impl::~NVRenderBackendGL4Impl()
    {
#if !defined(QT_OPENGL_ES)
        if (m_nvPathRendering)
            NVDelete(m_Foundation.getAllocator(), m_nvPathRendering);
        if (m_directStateAccess)
            NVDelete(m_Foundation.getAllocator(), m_directStateAccess);
#endif
    }

    void NVRenderBackendGL4Impl::DrawIndirect(NVRenderDrawMode::Enum drawMode, const void *indirect)
    {
        GL_CALL_EXTRA_FUNCTION(
            glDrawArraysIndirect(m_Conversion.fromDrawModeToGL(
                                     drawMode, m_backendSupport.caps.bits.bTessellationSupported),
                                 indirect));
    }

    void NVRenderBackendGL4Impl::DrawIndexedIndirect(NVRenderDrawMode::Enum drawMode,
                                                     NVRenderComponentTypes::Enum type,
                                                     const void *indirect)
    {
        GL_CALL_EXTRA_FUNCTION(glDrawElementsIndirect(
            m_Conversion.fromDrawModeToGL(drawMode,
                                          m_backendSupport.caps.bits.bTessellationSupported),
            m_Conversion.fromIndexBufferComponentsTypesToGL(type), indirect));
    }

    void NVRenderBackendGL4Impl::CreateTextureStorage2D(NVRenderBackendTextureObject to,
                                                        NVRenderTextureTargetType::Enum target,
                                                        QT3DSU32 levels,
                                                        NVRenderTextureFormats::Enum internalFormat,
                                                        size_t width, size_t height)
    {
        GLuint texID = HandleToID_cast(GLuint, size_t, to);
        GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
        GL_CALL_EXTRA_FUNCTION(glActiveTexture(GL_TEXTURE0));
        GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));

        // up to now compressed is not supported
        QT3DS_ASSERT(NVRenderTextureFormats::isUncompressedTextureFormat(internalFormat));

        GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;
        GLConversion::fromUncompressedTextureFormatToGL(GetRenderContextType(), internalFormat,
                                                        glformat, gltype, glInternalFormat);

        GL_CALL_EXTRA_FUNCTION(
            glTexStorage2D(glTarget, levels, glInternalFormat, (GLsizei)width, (GLsizei)height));

        GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
    }

    void NVRenderBackendGL4Impl::SetMultisampledTextureData2D(
        NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, size_t samples,
        NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height,
        bool fixedsamplelocations)
    {
        GLuint texID = HandleToID_cast(GLuint, size_t, to);
        GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
        GL_CALL_EXTRA_FUNCTION(glActiveTexture(GL_TEXTURE0));
        GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));

        NVRenderTextureSwizzleMode::Enum swizzleMode = NVRenderTextureSwizzleMode::NoSwizzle;
        internalFormat = m_Conversion.replaceDeprecatedTextureFormat(GetRenderContextType(),
                                                                     internalFormat, swizzleMode);

        GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;

        if (NVRenderTextureFormats::isUncompressedTextureFormat(internalFormat))
            GLConversion::fromUncompressedTextureFormatToGL(GetRenderContextType(), internalFormat,
                                                            glformat, gltype, glInternalFormat);
        else if (NVRenderTextureFormats::isDepthTextureFormat(internalFormat))
            m_Conversion.fromDepthTextureFormatToGL(GetRenderContextType(), internalFormat,
                                                    glformat, gltype, glInternalFormat);
        GL_CALL_EXTRA_FUNCTION(glTexStorage2DMultisample(glTarget, (GLsizei)samples, glInternalFormat,
                                             (GLsizei)width, (GLsizei)height,
                                             fixedsamplelocations));

        GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
    }

    NVRenderBackend::NVRenderBackendTessControlShaderObject
    NVRenderBackendGL4Impl::CreateTessControlShader(NVConstDataRef<QT3DSI8> source,
                                                    eastl::string &errorMessage, bool binary)
    {
#if !defined(QT_OPENGL_ES)
        GLuint shaderID = GL_CALL_EXTRA_FUNCTION(glCreateShader(GL_TESS_CONTROL_SHADER));
#else
        GLuint shaderID = 0;
#endif
        if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
            GL_CALL_EXTRA_FUNCTION(glDeleteShader(shaderID));
            shaderID = 0;
        }

        return (NVRenderBackend::NVRenderBackendTessControlShaderObject)shaderID;
    }

    NVRenderBackend::NVRenderBackendTessEvaluationShaderObject
    NVRenderBackendGL4Impl::CreateTessEvaluationShader(NVConstDataRef<QT3DSI8> source,
                                                       eastl::string &errorMessage, bool binary)
    {
#if !defined(QT_OPENGL_ES)
        GLuint shaderID = GL_CALL_EXTRA_FUNCTION(glCreateShader(GL_TESS_EVALUATION_SHADER));
#else
        GLuint shaderID = 0;
#endif

        if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
            GL_CALL_EXTRA_FUNCTION(glDeleteShader(shaderID));
            shaderID = 0;
        }

        return (NVRenderBackend::NVRenderBackendTessEvaluationShaderObject)shaderID;
    }

    NVRenderBackend::NVRenderBackendGeometryShaderObject
    NVRenderBackendGL4Impl::CreateGeometryShader(NVConstDataRef<QT3DSI8> source,
                                                 eastl::string &errorMessage, bool binary)
    {
#if defined(QT_OPENGL_ES)
        GLuint shaderID = GL_CALL_EXTRA_FUNCTION(glCreateShader(GL_GEOMETRY_SHADER_EXT));
#else
        GLuint shaderID = GL_CALL_EXTRA_FUNCTION(glCreateShader(GL_GEOMETRY_SHADER));
#endif
        if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
            GL_CALL_EXTRA_FUNCTION(glDeleteShader(shaderID));
            shaderID = 0;
        }

        return (NVRenderBackend::NVRenderBackendGeometryShaderObject)shaderID;
    }

    void NVRenderBackendGL4Impl::SetPatchVertexCount(NVRenderBackendInputAssemblerObject iao,
                                                     QT3DSU32 count)
    {
        QT3DS_ASSERT(iao);
        QT3DS_ASSERT(count);
        NVRenderBackendInputAssemblerGL *inputAssembler = (NVRenderBackendInputAssemblerGL *)iao;
        inputAssembler->m_PatchVertexCount = count;
    }

    void NVRenderBackendGL4Impl::SetMemoryBarrier(NVRenderBufferBarrierFlags barriers)
    {
        GL_CALL_EXTRA_FUNCTION(glMemoryBarrier(m_Conversion.fromMemoryBarrierFlagsToGL(barriers)));
    }

    void NVRenderBackendGL4Impl::BindImageTexture(NVRenderBackendTextureObject to, QT3DSU32 unit,
                                                  QT3DSI32 level, bool layered, QT3DSI32 layer,
                                                  NVRenderImageAccessType::Enum access,
                                                  NVRenderTextureFormats::Enum format)
    {
        GLuint texID = HandleToID_cast(GLuint, size_t, to);

        GL_CALL_EXTRA_FUNCTION(glBindImageTexture(unit, texID, level, layered, layer,
                                      m_Conversion.fromImageAccessToGL(access),
                                      m_Conversion.fromImageFormatToGL(format)));
    }

    QT3DSI32 NVRenderBackendGL4Impl::GetStorageBufferCount(NVRenderBackendShaderProgramObject po)
    {
        GLint numStorageBuffers = 0;
        QT3DS_ASSERT(po);
        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

#if defined(GL_VERSION_4_3) || defined (QT_OPENGL_ES_3_1)
        if (m_backendSupport.caps.bits.bProgramInterfaceSupported)
            GL_CALL_EXTRA_FUNCTION(glGetProgramInterfaceiv(programID, GL_SHADER_STORAGE_BLOCK,
                                               GL_ACTIVE_RESOURCES, &numStorageBuffers));
#endif
        return numStorageBuffers;
    }

    QT3DSI32
    NVRenderBackendGL4Impl::GetStorageBufferInfoByID(NVRenderBackendShaderProgramObject po,
                                                     QT3DSU32 id, QT3DSU32 nameBufSize, QT3DSI32 *paramCount,
                                                     QT3DSI32 *bufferSize, QT3DSI32 *length,
                                                     char *nameBuf)
    {
        GLint bufferIndex = GL_INVALID_INDEX;

        QT3DS_ASSERT(po);
        QT3DS_ASSERT(length);
        QT3DS_ASSERT(nameBuf);
        QT3DS_ASSERT(bufferSize);
        QT3DS_ASSERT(paramCount);

        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);
#if defined(GL_VERSION_4_3) || defined (QT_OPENGL_ES_3_1)
        if (m_backendSupport.caps.bits.bProgramInterfaceSupported) {
            GL_CALL_EXTRA_FUNCTION(glGetProgramResourceName(programID, GL_SHADER_STORAGE_BLOCK, id, nameBufSize,
                                                length, nameBuf));

            if (*length > 0) {
#define QUERY_COUNT 3
                GLsizei actualCount;
                GLenum props[QUERY_COUNT] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE,
                                              GL_NUM_ACTIVE_VARIABLES };
                GLint params[QUERY_COUNT];
                GL_CALL_EXTRA_FUNCTION(glGetProgramResourceiv(programID, GL_SHADER_STORAGE_BLOCK, id,
                                                  QUERY_COUNT, props, QUERY_COUNT, &actualCount,
                                                  params));

                QT3DS_ASSERT(actualCount == QUERY_COUNT);

                bufferIndex = params[0];
                *bufferSize = params[1];
                *paramCount = params[2];
            }
        }
#endif
        return bufferIndex;
    }

    void NVRenderBackendGL4Impl::ProgramSetStorageBuffer(QT3DSU32 index,
                                                         NVRenderBackendBufferObject bo)
    {
#if defined(GL_VERSION_4_3) || defined (QT_OPENGL_ES_3_1)
        GL_CALL_EXTRA_FUNCTION(
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, HandleToID_cast(GLuint, size_t, bo)));
#endif
    }

    QT3DSI32 NVRenderBackendGL4Impl::GetAtomicCounterBufferCount(NVRenderBackendShaderProgramObject po)
    {
        GLint numAtomicCounterBuffers = 0;
        QT3DS_ASSERT(po);
        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);
#if defined(GL_VERSION_4_3) || defined (QT_OPENGL_ES_3_1)
        if (m_backendSupport.caps.bits.bProgramInterfaceSupported)
            GL_CALL_EXTRA_FUNCTION(glGetProgramInterfaceiv(programID, GL_ATOMIC_COUNTER_BUFFER,
                                               GL_ACTIVE_RESOURCES, &numAtomicCounterBuffers));
#endif
        return numAtomicCounterBuffers;
    }

    QT3DSI32
    NVRenderBackendGL4Impl::GetAtomicCounterBufferInfoByID(NVRenderBackendShaderProgramObject po,
                                                           QT3DSU32 id, QT3DSU32 nameBufSize,
                                                           QT3DSI32 *paramCount, QT3DSI32 *bufferSize,
                                                           QT3DSI32 *length, char *nameBuf)
    {
        GLint bufferIndex = GL_INVALID_INDEX;

        QT3DS_ASSERT(po);
        QT3DS_ASSERT(length);
        QT3DS_ASSERT(nameBuf);
        QT3DS_ASSERT(bufferSize);
        QT3DS_ASSERT(paramCount);

        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);
#if defined(GL_VERSION_4_3) || defined (QT_OPENGL_ES_3_1)
        if (m_backendSupport.caps.bits.bProgramInterfaceSupported) {
            {
#define QUERY_COUNT 3
                GLsizei actualCount;
                GLenum props[QUERY_COUNT] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE,
                                              GL_NUM_ACTIVE_VARIABLES };
                GLint params[QUERY_COUNT];
                GL_CALL_EXTRA_FUNCTION(glGetProgramResourceiv(programID, GL_ATOMIC_COUNTER_BUFFER, id,
                                                  QUERY_COUNT, props, QUERY_COUNT, &actualCount,
                                                  params));

                QT3DS_ASSERT(actualCount == QUERY_COUNT);

                bufferIndex = params[0];
                *bufferSize = params[1];
                *paramCount = params[2];

                GLenum props1[1] = { GL_ATOMIC_COUNTER_BUFFER_INDEX };
                GL_CALL_EXTRA_FUNCTION(glGetProgramResourceiv(programID, GL_UNIFORM, id, 1, props1, 1,
                                                  &actualCount, params));

                QT3DS_ASSERT(actualCount == 1);

                *nameBuf = '\0';
                GL_CALL_EXTRA_FUNCTION(glGetProgramResourceName(programID, GL_UNIFORM, params[0], nameBufSize,
                                                    length, nameBuf));
            }
        }
#endif
        return bufferIndex;
    }

    void NVRenderBackendGL4Impl::ProgramSetAtomicCounterBuffer(QT3DSU32 index,
                                                               NVRenderBackendBufferObject bo)
    {
#if defined(GL_VERSION_4_3) || defined (QT_OPENGL_ES_3_1)
        GL_CALL_EXTRA_FUNCTION(
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, index, HandleToID_cast(GLuint, size_t, bo)));
#endif
    }

    void NVRenderBackendGL4Impl::SetConstantValue(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                                  NVRenderShaderDataTypes::Enum type, QT3DSI32 count,
                                                  const void *value, bool transpose)
    {
        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

        GLenum glType = m_Conversion.fromPropertyDataTypesToShaderGL(type);

        switch (glType) {
        case GL_FLOAT:
            GL_CALL_EXTRA_FUNCTION(glProgramUniform1fv(programID, id, count, (GLfloat *)value));
            break;
        case GL_FLOAT_VEC2:
            GL_CALL_EXTRA_FUNCTION(glProgramUniform2fv(programID, id, count, (GLfloat *)value));
            break;
        case GL_FLOAT_VEC3:
            GL_CALL_EXTRA_FUNCTION(glProgramUniform3fv(programID, id, count, (GLfloat *)value));
            break;
        case GL_FLOAT_VEC4:
            GL_CALL_EXTRA_FUNCTION(glProgramUniform4fv(programID, id, count, (GLfloat *)value));
            break;
        case GL_INT:
            GL_CALL_EXTRA_FUNCTION(glProgramUniform1iv(programID, id, count, (GLint *)value));
            break;
        case GL_BOOL:
            {
                GLint boolValue = *(GLboolean *)value;
                GL_CALL_EXTRA_FUNCTION(glProgramUniform1iv(programID, id, count, &boolValue));
            }
            break;
        case GL_INT_VEC2:
        case GL_BOOL_VEC2:
            GL_CALL_EXTRA_FUNCTION(glProgramUniform2iv(programID, id, count, (GLint *)value));
            break;
        case GL_INT_VEC3:
        case GL_BOOL_VEC3:
            GL_CALL_EXTRA_FUNCTION(glProgramUniform3iv(programID, id, count, (GLint *)value));
            break;
        case GL_INT_VEC4:
        case GL_BOOL_VEC4:
            GL_CALL_EXTRA_FUNCTION(glProgramUniform4iv(programID, id, count, (GLint *)value));
            break;
        case GL_FLOAT_MAT3:
            GL_CALL_EXTRA_FUNCTION(
                glProgramUniformMatrix3fv(programID, id, count, transpose, (GLfloat *)value));
            break;
        case GL_FLOAT_MAT4:
            GL_CALL_EXTRA_FUNCTION(
                glProgramUniformMatrix4fv(programID, id, count, transpose, (GLfloat *)value));
            break;
        case GL_IMAGE_2D:
        case GL_SAMPLER_2D:
        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_CUBE: {
            if (count <= 1) {
                GLint sampler = *(GLint *)value;
                GL_CALL_EXTRA_FUNCTION(glProgramUniform1i(programID, id, sampler));
            } else {
                GLint *sampler = (GLint *)value;
                GL_CALL_EXTRA_FUNCTION(glProgramUniform1iv(programID, id, count, sampler));
            }
        } break;
        default:
            qCCritical(INTERNAL_ERROR, "Unknown shader type format %d", type);
            QT3DS_ASSERT(false);
            break;
        }
    }

    NVRenderBackend::NVRenderBackendComputeShaderObject
    NVRenderBackendGL4Impl::CreateComputeShader(NVConstDataRef<QT3DSI8> source,
                                                eastl::string &errorMessage, bool binary)
    {
        GLuint shaderID = 0;
#if defined(GL_COMPUTE_SHADER)
        shaderID = m_glExtraFunctions->glCreateShader(GL_COMPUTE_SHADER);

        if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
            GL_CALL_EXTRA_FUNCTION(glDeleteShader(shaderID));
            shaderID = 0;
        }
#endif
        return (NVRenderBackend::NVRenderBackendComputeShaderObject)shaderID;
    }

    void NVRenderBackendGL4Impl::DispatchCompute(NVRenderBackendShaderProgramObject,
                                                 QT3DSU32 numGroupsX, QT3DSU32 numGroupsY,
                                                 QT3DSU32 numGroupsZ)
    {
        GL_CALL_EXTRA_FUNCTION(glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ));
    }

    NVRenderBackend::NVRenderBackendProgramPipeline NVRenderBackendGL4Impl::CreateProgramPipeline()
    {
        GLuint pipeline;
        GL_CALL_EXTRA_FUNCTION(glGenProgramPipelines(1, &pipeline));

        return NVRenderBackend::NVRenderBackendProgramPipeline(pipeline);
    }

    void NVRenderBackendGL4Impl::ReleaseProgramPipeline(NVRenderBackendProgramPipeline ppo)
    {
        GLuint pipeline = HandleToID_cast(GLuint, size_t, ppo);
        GL_CALL_EXTRA_FUNCTION(glDeleteProgramPipelines(1, &pipeline));
    }

    void NVRenderBackendGL4Impl::SetActiveProgramPipeline(NVRenderBackendProgramPipeline ppo)
    {
        GLuint pipeline = HandleToID_cast(GLuint, size_t, ppo);

        GL_CALL_EXTRA_FUNCTION(glBindProgramPipeline(pipeline));
    }

    void NVRenderBackendGL4Impl::SetProgramStages(NVRenderBackendProgramPipeline ppo,
                                                  NVRenderShaderTypeFlags flags,
                                                  NVRenderBackendShaderProgramObject po)
    {
        GLuint pipeline = HandleToID_cast(GLuint, size_t, ppo);
        GLuint programID = 0;

        if (po) {
            NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
            programID = static_cast<GLuint>(pProgram->m_ProgramID);
        }

        GL_CALL_EXTRA_FUNCTION(
            glUseProgramStages(pipeline, m_Conversion.fromShaderTypeFlagsToGL(flags), programID));
    }

    void NVRenderBackendGL4Impl::SetBlendEquation(const NVRenderBlendEquationArgument &pBlendEquArg)
    {
        if (m_backendSupport.caps.bits.bNVAdvancedBlendSupported ||
            m_backendSupport.caps.bits.bKHRAdvancedBlendSupported)
            GL_CALL_EXTRA_FUNCTION(glBlendEquation(m_Conversion.fromBlendEquationToGL(
                pBlendEquArg.m_RGBEquation, m_backendSupport.caps.bits.bNVAdvancedBlendSupported,
                m_backendSupport.caps.bits.bKHRAdvancedBlendSupported)));
    }

    void NVRenderBackendGL4Impl::SetBlendBarrier(void)
    {
        if (m_backendSupport.caps.bits.bNVAdvancedBlendSupported)
            GL_CALL_QT3DS_EXT(glBlendBarrierNV());
    }

    NVRenderBackend::NVRenderBackendPathObject
    NVRenderBackendGL4Impl::CreatePathNVObject(size_t range)
    {
        GLuint pathID = GL_CALL_NVPATH_EXT(glGenPathsNV((GLsizei)range));

        return NVRenderBackend::NVRenderBackendPathObject(pathID);
    }
    void NVRenderBackendGL4Impl::SetPathSpecification(NVRenderBackendPathObject inPathObject,
                                                      NVConstDataRef<QT3DSU8> inPathCommands,
                                                      NVConstDataRef<QT3DSF32> inPathCoords)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, inPathObject);
        GL_CALL_NVPATH_EXT(glPathCommandsNV(pathID, inPathCommands.size(), inPathCommands.begin(),
                                    inPathCoords.size(), GL_FLOAT, inPathCoords.begin()));
    }

    NVBounds3
    NVRenderBackendGL4Impl::GetPathObjectBoundingBox(NVRenderBackendPathObject inPathObject)
    {
        float data[4];
#if defined(GL_NV_path_rendering)
        GL_CALL_NVPATH_EXT(glGetPathParameterfvNV(
            HandleToID_cast(GLuint, size_t, inPathObject),
            GL_PATH_OBJECT_BOUNDING_BOX_NV, data));
#endif
        return NVBounds3(QT3DSVec3(data[0], data[1], 0.0f), QT3DSVec3(data[2], data[3], 0.0f));
    }

    NVBounds3 NVRenderBackendGL4Impl::GetPathObjectFillBox(NVRenderBackendPathObject inPathObject)
    {
        float data[4];
#if defined(GL_NV_path_rendering)
        GL_CALL_NVPATH_EXT(glGetPathParameterfvNV(
            HandleToID_cast(GLuint, size_t, inPathObject),
            GL_PATH_FILL_BOUNDING_BOX_NV, data));
#endif
        return NVBounds3(QT3DSVec3(data[0], data[1], 0.0f), QT3DSVec3(data[2], data[3], 0.0f));
    }

    NVBounds3 NVRenderBackendGL4Impl::GetPathObjectStrokeBox(NVRenderBackendPathObject inPathObject)
    {
        float data[4];
#if defined(GL_NV_path_rendering)
        GL_CALL_NVPATH_EXT(glGetPathParameterfvNV(
            HandleToID_cast(GLuint, size_t, inPathObject),
            GL_PATH_STROKE_BOUNDING_BOX_NV, data));
#endif
        return NVBounds3(QT3DSVec3(data[0], data[1], 0.0f), QT3DSVec3(data[2], data[3], 0.0f));
    }

    void NVRenderBackendGL4Impl::SetStrokeWidth(NVRenderBackendPathObject inPathObject,
                                                QT3DSF32 inStrokeWidth)
    {
#if defined(GL_NV_path_rendering)
        GL_CALL_NVPATH_EXT(glPathParameterfNV(HandleToID_cast(GLuint, size_t, inPathObject),
                                      GL_PATH_STROKE_WIDTH_NV, inStrokeWidth));
#endif
    }

    void NVRenderBackendGL4Impl::SetPathProjectionMatrix(const QT3DSMat44 inPathProjection)
    {
#if defined(QT_OPENGL_ES)
        NVRENDER_BACKEND_UNUSED(inPathProjection);
#else
        GL_CALL_DIRECTSTATE_EXT(glMatrixLoadfEXT(GL_PROJECTION, inPathProjection.front()));
#endif
    }

    void NVRenderBackendGL4Impl::SetPathModelViewMatrix(const QT3DSMat44 inPathModelview)
    {
#if defined(QT_OPENGL_ES)
        NVRENDER_BACKEND_UNUSED(inPathModelview);
#else
        GL_CALL_DIRECTSTATE_EXT(glMatrixLoadfEXT(GL_MODELVIEW, inPathModelview.front()));
#endif
    }

    void NVRenderBackendGL4Impl::SetPathStencilDepthOffset(QT3DSF32 inSlope, QT3DSF32 inBias)
    {
        GL_CALL_NVPATH_EXT(glPathStencilDepthOffsetNV(inSlope, inBias));
    }

    void NVRenderBackendGL4Impl::SetPathCoverDepthFunc(NVRenderBoolOp::Enum inDepthFunction)
    {
        GL_CALL_NVPATH_EXT(glPathCoverDepthFuncNV(m_Conversion.fromBoolOpToGL(inDepthFunction)));
    }

    void NVRenderBackendGL4Impl::StencilStrokePath(NVRenderBackendPathObject inPathObject)
    {
        GL_CALL_NVPATH_EXT(glStencilStrokePathNV(HandleToID_cast(GLuint, size_t, inPathObject), 0x1, (GLuint)~0));
    }

    void NVRenderBackendGL4Impl::StencilFillPath(NVRenderBackendPathObject inPathObject)
    {
#if defined(GL_NV_path_rendering)
        GL_CALL_NVPATH_EXT(glStencilFillPathNV(HandleToID_cast(GLuint, size_t, inPathObject),
                                       GL_COUNT_UP_NV, (GLuint)~0));
#endif
    }

    void NVRenderBackendGL4Impl::ReleasePathNVObject(NVRenderBackendPathObject po, size_t range)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);

        GL_CALL_NVPATH_EXT(glDeletePathsNV(pathID, (GLsizei)range));
    }

    void NVRenderBackendGL4Impl::StencilFillPathInstanced(
        NVRenderBackendPathObject po, size_t numPaths, NVRenderPathFormatType::Enum type,
        const void *charCodes, NVRenderPathFillMode::Enum fillMode, QT3DSU32 stencilMask,
        NVRenderPathTransformType::Enum transformType, const QT3DSF32 *transformValues)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);

        GL_CALL_NVPATH_EXT(glStencilFillPathInstancedNV(
            (GLsizei)numPaths, m_Conversion.fromPathTypeToGL(type), charCodes, pathID,
            m_Conversion.fromPathFillModeToGL(fillMode), stencilMask,
            m_Conversion.fromPathTransformToGL(transformType), transformValues));
    }

    void NVRenderBackendGL4Impl::StencilStrokePathInstancedN(
        NVRenderBackendPathObject po, size_t numPaths, NVRenderPathFormatType::Enum type,
        const void *charCodes, QT3DSI32 stencilRef, QT3DSU32 stencilMask,
        NVRenderPathTransformType::Enum transformType, const QT3DSF32 *transformValues)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);

        GL_CALL_NVPATH_EXT(glStencilStrokePathInstancedNV(
            (GLsizei)numPaths, m_Conversion.fromPathTypeToGL(type), charCodes, pathID, stencilRef,
            stencilMask, m_Conversion.fromPathTransformToGL(transformType), transformValues));
    }

    void NVRenderBackendGL4Impl::CoverFillPathInstanced(
        NVRenderBackendPathObject po, size_t numPaths, NVRenderPathFormatType::Enum type,
        const void *charCodes, NVRenderPathCoverMode::Enum coverMode,
        NVRenderPathTransformType::Enum transformType, const QT3DSF32 *transformValues)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);

        GL_CALL_NVPATH_EXT(glCoverFillPathInstancedNV(
            (GLsizei)numPaths, m_Conversion.fromPathTypeToGL(type), charCodes, pathID,
            m_Conversion.fromPathCoverModeToGL(coverMode),
            m_Conversion.fromPathTransformToGL(transformType), transformValues));
    }

    void NVRenderBackendGL4Impl::CoverStrokePathInstanced(
        NVRenderBackendPathObject po, size_t numPaths, NVRenderPathFormatType::Enum type,
        const void *charCodes, NVRenderPathCoverMode::Enum coverMode,
        NVRenderPathTransformType::Enum transformType, const QT3DSF32 *transformValues)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);

        GL_CALL_NVPATH_EXT(glCoverStrokePathInstancedNV(
            (GLsizei)numPaths, m_Conversion.fromPathTypeToGL(type), charCodes, pathID,
            m_Conversion.fromPathCoverModeToGL(coverMode),
            m_Conversion.fromPathTransformToGL(transformType), transformValues));
    }

    void NVRenderBackendGL4Impl::LoadPathGlyphs(
        NVRenderBackendPathObject po, NVRenderPathFontTarget::Enum fontTarget, const void *fontName,
        NVRenderPathFontStyleFlags fontStyle, size_t numGlyphs, NVRenderPathFormatType::Enum type,
        const void *charCodes, NVRenderPathMissingGlyphs::Enum handleMissingGlyphs,
        NVRenderBackendPathObject pathParameterTemplate, QT3DSF32 emScale)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);
        GLuint pathTemplateID = (pathParameterTemplate == NULL)
            ? ~0
            : HandleToID_cast(GLuint, size_t, pathParameterTemplate);

        GL_CALL_NVPATH_EXT(glPathGlyphsNV(pathID, m_Conversion.fromPathFontTargetToGL(fontTarget), fontName,
                                  m_Conversion.fromPathFontStyleToGL(fontStyle), (GLsizei)numGlyphs,
                                  m_Conversion.fromPathTypeToGL(type), charCodes,
                                  m_Conversion.fromPathMissingGlyphsToGL(handleMissingGlyphs),
                                  pathTemplateID, emScale));
    }

    NVRenderPathReturnValues::Enum NVRenderBackendGL4Impl::LoadPathGlyphsIndexed(
        NVRenderBackendPathObject po, NVRenderPathFontTarget::Enum fontTarget, const void *fontName,
        NVRenderPathFontStyleFlags fontStyle, QT3DSU32 firstGlyphIndex, size_t numGlyphs,
        NVRenderBackendPathObject pathParameterTemplate, QT3DSF32 emScale)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);
        GLuint pathTemplateID = (pathParameterTemplate == NULL)
            ? ~0
            : HandleToID_cast(GLuint, size_t, pathParameterTemplate);
        GLenum glRet = 0;

        glRet = GL_CALL_QT3DS_EXT(glPathGlyphIndexArrayNV(
                       pathID, m_Conversion.fromPathFontTargetToGL(fontTarget), fontName,
                       m_Conversion.fromPathFontStyleToGL(fontStyle), firstGlyphIndex,
                       (GLsizei)numGlyphs, pathTemplateID, emScale));

        return m_Conversion.fromGLToPathFontReturn(glRet);
    }

    NVRenderBackend::NVRenderBackendPathObject NVRenderBackendGL4Impl::LoadPathGlyphsIndexedRange(
        NVRenderPathFontTarget::Enum fontTarget, const void *fontName,
        NVRenderPathFontStyleFlags fontStyle, NVRenderBackendPathObject pathParameterTemplate,
        QT3DSF32 emScale, QT3DSU32 *count)
    {
        GLuint pathTemplateID = (pathParameterTemplate == NULL)
            ? ~0
            : HandleToID_cast(GLuint, size_t, pathParameterTemplate);
        GLenum glRet = 0;
        GLuint baseAndCount[2] = { 0, 0 };

        glRet = GL_CALL_QT3DS_EXT(glPathGlyphIndexRangeNV(m_Conversion.fromPathFontTargetToGL(fontTarget),
                                                   fontName,
                                                   m_Conversion.fromPathFontStyleToGL(fontStyle),
                                                   pathTemplateID, emScale, baseAndCount));

        if (count)
            *count = baseAndCount[1];

        return NVRenderBackend::NVRenderBackendPathObject(baseAndCount[0]);
    }

    void NVRenderBackendGL4Impl::LoadPathGlyphRange(
        NVRenderBackendPathObject po, NVRenderPathFontTarget::Enum fontTarget, const void *fontName,
        NVRenderPathFontStyleFlags fontStyle, QT3DSU32 firstGlyph, size_t numGlyphs,
        NVRenderPathMissingGlyphs::Enum handleMissingGlyphs,
        NVRenderBackendPathObject pathParameterTemplate, QT3DSF32 emScale)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);
        GLuint pathTemplateID = (pathParameterTemplate == NULL)
            ? ~0
            : HandleToID_cast(GLuint, size_t, pathParameterTemplate);

        GL_CALL_NVPATH_EXT(glPathGlyphRangeNV(
            pathID, m_Conversion.fromPathFontTargetToGL(fontTarget), fontName,
            m_Conversion.fromPathFontStyleToGL(fontStyle), firstGlyph, (GLsizei)numGlyphs,
            m_Conversion.fromPathMissingGlyphsToGL(handleMissingGlyphs), pathTemplateID, emScale));
    }

    void NVRenderBackendGL4Impl::GetPathMetrics(NVRenderBackendPathObject po, size_t numPaths,
                                                NVRenderPathGlyphFontMetricFlags metricQueryMask,
                                                NVRenderPathFormatType::Enum type,
                                                const void *charCodes, size_t stride,
                                                QT3DSF32 *metrics)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);

        GL_CALL_NVPATH_EXT(glGetPathMetricsNV(m_Conversion.fromPathMetricQueryFlagsToGL(metricQueryMask),
                                      (GLsizei)numPaths, m_Conversion.fromPathTypeToGL(type),
                                      charCodes, pathID, (GLsizei)stride, metrics));
    }

    void
    NVRenderBackendGL4Impl::GetPathMetricsRange(NVRenderBackendPathObject po, size_t numPaths,
                                                NVRenderPathGlyphFontMetricFlags metricQueryMask,
                                                size_t stride, QT3DSF32 *metrics)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);

        GL_CALL_NVPATH_EXT(glGetPathMetricRangeNV(m_Conversion.fromPathMetricQueryFlagsToGL(metricQueryMask),
                                   pathID, (GLsizei)numPaths, (GLsizei)stride, metrics));
    }

    void NVRenderBackendGL4Impl::GetPathSpacing(
        NVRenderBackendPathObject po, size_t numPaths, NVRenderPathListMode::Enum pathListMode,
        NVRenderPathFormatType::Enum type, const void *charCodes, QT3DSF32 advanceScale,
        QT3DSF32 kerningScale, NVRenderPathTransformType::Enum transformType, QT3DSF32 *spacing)
    {
        GLuint pathID = HandleToID_cast(GLuint, size_t, po);

        GL_CALL_NVPATH_EXT(glGetPathSpacingNV(m_Conversion.fromPathListModeToGL(pathListMode),
                                      (GLsizei)numPaths, m_Conversion.fromPathTypeToGL(type),
                                      charCodes, pathID, advanceScale, kerningScale,
                                      m_Conversion.fromPathTransformToGL(transformType), spacing));
    }
}
}
