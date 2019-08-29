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

#include "render/backends/gl/Qt3DSRenderBackendGL3.h"
#include "render/backends/gl/Qt3DSRenderBackendInputAssemblerGL.h"
#include "render/backends/gl/Qt3DSRenderBackendRenderStatesGL.h"
#include "render/backends/gl/Qt3DSRenderBackendShaderProgramGL.h"

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError(#x, __FILE__, __LINE__)
#else
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError()
#endif

#define GL_CALL_EXTRA_FUNCTION(x) m_glExtraFunctions->x; RENDER_LOG_ERROR_PARAMS(x);

#if defined(QT_OPENGL_ES)
#define GL_CALL_TIMER_EXT(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_TESSELLATION_EXT(x) m_qt3dsExtensions->x; RENDER_LOG_ERROR_PARAMS(x);
#else
#define GL_CALL_TIMER_EXT(x) m_timerExtension->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_TESSELLATION_EXT(x) m_tessellationShader->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_MULTISAMPLE_EXT(x) m_multiSample->x; RENDER_LOG_ERROR_PARAMS(x);
#endif

namespace qt3ds {
namespace render {

#ifndef GL_PATCH_VERTICES
#define GL_PATCH_VERTICES 0x8E72
#endif

    /// constructor
    NVRenderBackendGL3Impl::NVRenderBackendGL3Impl(NVFoundationBase &fnd,
                                                   qt3ds::foundation::IStringTable &stringTable,
                                                   const QSurfaceFormat &format)
        : NVRenderBackendGLBase(fnd, stringTable, format)
    {
        eastl::string exts3tc("GL_EXT_texture_compression_s3tc");
        eastl::string extsdxt("GL_EXT_texture_compression_dxt1");
        eastl::string extsAniso("GL_EXT_texture_filter_anisotropic");
        eastl::string extsTexSwizzle("GL_ARB_texture_swizzle");
        eastl::string extsAstcHDR("GL_KHR_texture_compression_astc_hdr");
        eastl::string extsAstcLDR("GL_KHR_texture_compression_astc_ldr");
        eastl::string extsFPRenderTarget("GL_EXT_color_buffer_float");
        eastl::string extsTimerQuery("GL_EXT_timer_query");
        eastl::string extsGpuShader5("EXT_gpu_shader5");

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

        // get extension count
        GLint numExtensions = 0;
        GL_CALL_EXTRA_FUNCTION(glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions));

        eastl::string extensionBuffer;

        for (QT3DSI32 i = 0; i < numExtensions; i++) {
            char *extensionString = (char *)GL_CALL_EXTRA_FUNCTION(glGetStringi(GL_EXTENSIONS, i));

            m_extensions.push_back(QString::fromLocal8Bit(extensionString));

            if (extensionBuffer.size())
                extensionBuffer.append(" ");
            extensionBuffer.append(extensionString);

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
            }

        }

        qCInfo(TRACE_INFO, "OpenGL extensions: %s", extensionBuffer.c_str());

        // texture swizzle is always true
        m_backendSupport.caps.bits.bTextureSwizzleSupported = true;
        // depthstencil renderbuffer support is always true
        m_backendSupport.caps.bits.bDepthStencilSupported = true;
        // constant buffers support is always true
        m_backendSupport.caps.bits.bConstantBufferSupported = true;
        m_backendSupport.caps.bits.bStandardDerivativesSupported = true;
        m_backendSupport.caps.bits.bVertexArrayObjectSupported = true;
        m_backendSupport.caps.bits.bTextureLodSupported = true;
        m_backendSupport.caps.bits.bBinaryProgramsSupported = true;

        if (!isESCompatible()) {
            // render to float textures is always supported on none ES systems which support >=GL3
            m_backendSupport.caps.bits.bFPRenderTargetsSupported = true;
            // multisampled texture is always supported on none ES systems which support >=GL3
            m_backendSupport.caps.bits.bMsTextureSupported = true;
            // timer queries are always supported on none ES systems which support >=GL3
            m_backendSupport.caps.bits.bTimerQuerySupported = true;
        }

        // query hardware
       GL_CALL_EXTRA_FUNCTION(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_MaxAttribCount));

        // internal state tracker
        m_pCurrentMiscState = QT3DS_NEW(m_Foundation.getAllocator(), NVRenderBackendMiscStateGL)();

        // finally setup caps based on device
        setAndInspectHardwareCaps();

        // Initialize extensions
#if defined(QT_OPENGL_ES_2)
        m_qt3dsExtensions = new Qt3DSOpenGLES2Extensions;
        m_qt3dsExtensions->initializeOpenGLFunctions();
#else
        m_timerExtension = new QOpenGLExtension_ARB_timer_query;
        m_timerExtension->initializeOpenGLFunctions();
        m_tessellationShader = new QOpenGLExtension_ARB_tessellation_shader;
        m_tessellationShader->initializeOpenGLFunctions();
        m_multiSample = new QOpenGLExtension_ARB_texture_multisample;
        m_multiSample->initializeOpenGLFunctions();
        m_qt3dsExtensions = new Qt3DSOpenGLExtensions;
        m_qt3dsExtensions->initializeOpenGLFunctions();
#endif
    }
    /// destructor
    NVRenderBackendGL3Impl::~NVRenderBackendGL3Impl()
    {
        if (m_pCurrentMiscState)
            NVDelete(m_Foundation.getAllocator(), m_pCurrentMiscState);
#if !defined(QT_OPENGL_ES_2)
        if (m_timerExtension)
            delete m_timerExtension;
        if (m_tessellationShader)
            delete m_tessellationShader;
        if (m_multiSample)
            delete m_multiSample;
#endif
        if (m_qt3dsExtensions)
            delete m_qt3dsExtensions;
    }

    void NVRenderBackendGL3Impl::SetMultisampledTextureData2D(
        NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, size_t samples,
        NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height,
        bool fixedsamplelocations)
    {
// Not supported by ES 3 yet
#if defined(QT_OPENGL_ES)
        NVRENDER_BACKEND_UNUSED(to);
        NVRENDER_BACKEND_UNUSED(target);
        NVRENDER_BACKEND_UNUSED(samples);
        NVRENDER_BACKEND_UNUSED(internalFormat);
        NVRENDER_BACKEND_UNUSED(width);
        NVRENDER_BACKEND_UNUSED(height);
        NVRENDER_BACKEND_UNUSED(fixedsamplelocations);
#else
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

        GL_CALL_MULTISAMPLE_EXT(glTexImage2DMultisample(glTarget, (GLsizei)samples, glInternalFormat,
                                           (GLsizei)width, (GLsizei)height, fixedsamplelocations));

        GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
#endif
    }

    void NVRenderBackendGL3Impl::SetTextureData3D(
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

        if (NVRenderTextureFormats::isUncompressedTextureFormat(internalFormat))
            m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), internalFormat,
                                                           glformat, gltype, glInternalFormat);

        if (conversionRequired) {
            GLenum dummy;
            m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                           gltype, dummy);
        } else if (NVRenderTextureFormats::isCompressedTextureFormat(internalFormat)) {
            m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                           gltype, glInternalFormat);
            glInternalFormat = m_Conversion.fromCompressedTextureFormatToGL(internalFormat);
        } else if (NVRenderTextureFormats::isDepthTextureFormat(format))
            m_Conversion.fromDepthTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                    gltype, glInternalFormat);

        GL_CALL_EXTRA_FUNCTION(glTexImage3D(glTarget, level, glInternalFormat, (GLsizei)width, (GLsizei)height,
                                (GLsizei)depth, border, glformat, gltype, hostPtr));

        GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
    }

    void NVRenderBackendGL3Impl::UpdateSampler(
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

        GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);

        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER,
                                   m_Conversion.fromTextureMinifyingOpToGL(minFilter)));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER,
                                   m_Conversion.fromTextureMagnifyingOpToGL(magFilter)));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_S,
                                   m_Conversion.fromTextureCoordOpToGL(wrapS)));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_T,
                                   m_Conversion.fromTextureCoordOpToGL(wrapT)));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_R,
                                   m_Conversion.fromTextureCoordOpToGL(wrapR)));
        GL_CALL_EXTRA_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MIN_LOD, minLod));
        GL_CALL_EXTRA_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MAX_LOD, maxLod));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_COMPARE_MODE,
                                   m_Conversion.fromTextureCompareModeToGL(compareMode)));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_COMPARE_FUNC,
                                   m_Conversion.fromTextureCompareFuncToGL(compareFunc)));

        if (m_backendSupport.caps.bits.bAnistropySupported) {
            GL_CALL_EXTRA_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                                   qMin(anisotropy, m_maxAnisotropy)));
        }
    }

    void NVRenderBackendGL3Impl::UpdateTextureObject(NVRenderBackendTextureObject to,
                                                     NVRenderTextureTargetType::Enum target,
                                                     QT3DSI32 baseLevel, QT3DSI32 maxLevel)
    {
        NVRENDER_BACKEND_UNUSED(to);

        GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);

        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_BASE_LEVEL, baseLevel));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, maxLevel));
    }

    void NVRenderBackendGL3Impl::UpdateTextureSwizzle(NVRenderBackendTextureObject to,
                                                      NVRenderTextureTargetType::Enum target,
                                                      NVRenderTextureSwizzleMode::Enum swizzleMode)
    {
        NVRENDER_BACKEND_UNUSED(to);
        if (m_backendSupport.caps.bits.bTextureSwizzleSupported) {
            GLint glSwizzle[4];
            GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
            m_Conversion.NVRenderConvertSwizzleModeToGL(swizzleMode, glSwizzle);
#if defined(QT_OPENGL_ES)
            // since ES3 spec has no GL_TEXTURE_SWIZZLE_RGBA set it separately
            GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_R, glSwizzle[0]));
            GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_G, glSwizzle[1]));
            GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_B, glSwizzle[2]));
            GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_A, glSwizzle[3]));
#else
            GL_CALL_EXTRA_FUNCTION(glTexParameteriv(glTarget, GL_TEXTURE_SWIZZLE_RGBA, glSwizzle));
#endif
        }
    }

    QT3DSU32
    NVRenderBackendGL3Impl::GetDepthBits() const
    {
        QT3DSI32 depthBits;
        GL_CALL_EXTRA_FUNCTION(glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits));

        return depthBits;
    }

    QT3DSU32
    NVRenderBackendGL3Impl::GetStencilBits() const
    {
        QT3DSI32 stencilBits;
        GL_CALL_EXTRA_FUNCTION(glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                         GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
                                                         &stencilBits));

        return stencilBits;
    }

    void NVRenderBackendGL3Impl::GenerateMipMaps(NVRenderBackendTextureObject to,
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

    bool NVRenderBackendGL3Impl::SetInputAssembler(NVRenderBackendInputAssemblerObject iao,
                                                   NVRenderBackendShaderProgramObject po)
    {
        if (iao == NULL) {
            // unbind and return;
            GL_CALL_EXTRA_FUNCTION(glBindVertexArray(0));
            return true;
        }

        NVRenderBackendInputAssemblerGL *inputAssembler = (NVRenderBackendInputAssemblerGL *)iao;
        NVRenderBackendAttributeLayoutGL *attribLayout = inputAssembler->m_attribLayout;
        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);
        NVDataRef<NVRenderBackendShaderInputEntryGL> shaderAttribBuffer;
        if (pProgram->m_shaderInput)
            shaderAttribBuffer = pProgram->m_shaderInput->m_ShaderInputEntries;

        if (attribLayout->m_LayoutAttribEntries.size() < shaderAttribBuffer.size())
            return false;

        if (inputAssembler->m_VertexbufferHandles.size() <= attribLayout->m_MaxInputSlot) {
            QT3DS_ASSERT(false);
            return false;
        }

        if (inputAssembler->m_VaoID == 0) {
            // generate vao
            GL_CALL_EXTRA_FUNCTION(glGenVertexArrays(1, &inputAssembler->m_VaoID));
            QT3DS_ASSERT(inputAssembler->m_VaoID);
        }

        // set patch parameter count if changed
        if (m_backendSupport.caps.bits.bTessellationSupported
            && m_pCurrentMiscState->m_PatchVertexCount != inputAssembler->m_PatchVertexCount) {
            m_pCurrentMiscState->m_PatchVertexCount = inputAssembler->m_PatchVertexCount;
#if defined(QT_OPENGL_ES)
            GL_CALL_TESSELLATION_EXT(glPatchParameteriEXT(GL_PATCH_VERTICES, inputAssembler->m_PatchVertexCount));
#else
            GL_CALL_TESSELLATION_EXT(glPatchParameteri(GL_PATCH_VERTICES, inputAssembler->m_PatchVertexCount));
#endif
        }

        if (inputAssembler->m_cachedShaderHandle != programID) {
            GL_CALL_EXTRA_FUNCTION(glBindVertexArray(inputAssembler->m_VaoID));
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
                        qCCritical(INVALID_OPERATION, "Attrib %s doesn't match vertex layout",
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
            for (QT3DSU32 i = 0; i < attribLayout->m_LayoutAttribEntries.size(); i++) {
                GL_CALL_EXTRA_FUNCTION(glDisableVertexAttribArray(i));
            }

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
            GL_CALL_EXTRA_FUNCTION(glBindVertexArray(inputAssembler->m_VaoID));
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
                        qCCritical(INVALID_OPERATION, "Attrib %s doesn't match vertex layout",
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

    void NVRenderBackendGL3Impl::ReleaseInputAssembler(
            NVRenderBackend::NVRenderBackendInputAssemblerObject iao)
    {
        NVRenderBackendInputAssemblerGL *inputAssembler = (NVRenderBackendInputAssemblerGL *)iao;
        if (inputAssembler->m_VaoID)
            GL_CALL_EXTRA_FUNCTION(glDeleteVertexArrays(1, &inputAssembler->m_VaoID));
        NVDelete(m_Foundation.getAllocator(), inputAssembler);
    }

    void NVRenderBackendGL3Impl::SetDrawBuffers(NVRenderBackendRenderTargetObject rto,
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

        GL_CALL_EXTRA_FUNCTION(glDrawBuffers((int)m_DrawBuffersArray.size(), m_DrawBuffersArray.data()));
    }

    void NVRenderBackendGL3Impl::SetReadBuffer(NVRenderBackendRenderTargetObject rto,
                                               NVReadFaces::Enum inReadFace)
    {
        NVRENDER_BACKEND_UNUSED(rto);

        GL_CALL_EXTRA_FUNCTION(glReadBuffer(m_Conversion.fromReadFacesToGL(inReadFace)));
    }

    void NVRenderBackendGL3Impl::RenderTargetAttach(NVRenderBackendRenderTargetObject,
                                                    NVRenderFrameBufferAttachments::Enum attachment,
                                                    NVRenderBackendTextureObject to, QT3DSI32 level,
                                                    QT3DSI32 layer)
    {
        // rto must be the current render target
        GLuint texID = HandleToID_cast(GLuint, size_t, to);

        GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);

        GL_CALL_EXTRA_FUNCTION(glFramebufferTextureLayer(GL_FRAMEBUFFER, glAttach, texID, level, layer))
    }

    void NVRenderBackendGL3Impl::SetReadTarget(NVRenderBackendRenderTargetObject rto)
    {
        GLuint fboID = HandleToID_cast(GLuint, size_t, rto);
        if (!fboID)
            fboID = QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext()->defaultFramebufferObject();

        GL_CALL_EXTRA_FUNCTION(glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID));
    }

    void NVRenderBackendGL3Impl::BlitFramebuffer(QT3DSI32 srcX0, QT3DSI32 srcY0, QT3DSI32 srcX1, QT3DSI32 srcY1,
                                                 QT3DSI32 dstX0, QT3DSI32 dstY0, QT3DSI32 dstX1, QT3DSI32 dstY1,
                                                 NVRenderClearFlags flags,
                                                 NVRenderTextureMagnifyingOp::Enum filter)
    {
        GL_CALL_EXTRA_FUNCTION(glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                                     m_Conversion.fromClearFlagsToGL(flags),
                                     m_Conversion.fromTextureMagnifyingOpToGL(filter)));
    }

    void *NVRenderBackendGL3Impl::MapBuffer(NVRenderBackendBufferObject,
                                            NVRenderBufferBindFlags bindFlags, size_t offset,
                                            size_t length, NVRenderBufferAccessFlags accessFlags)
    {
        void *ret = NULL;
        ret = GL_CALL_EXTRA_FUNCTION(glMapBufferRange(m_Conversion.fromBindBufferFlagsToGL(bindFlags), offset,
            length, m_Conversion.fromBufferAccessBitToGL(accessFlags)));

        return ret;
    }

    bool NVRenderBackendGL3Impl::UnmapBuffer(NVRenderBackendBufferObject,
                                             NVRenderBufferBindFlags bindFlags)
    {
        GLboolean ret;

        ret = GL_CALL_EXTRA_FUNCTION(glUnmapBuffer(m_Conversion.fromBindBufferFlagsToGL(bindFlags)));

        return (ret) ? true : false;
    }

    QT3DSI32 NVRenderBackendGL3Impl::GetConstantBufferCount(NVRenderBackendShaderProgramObject po)
    {
        QT3DS_ASSERT(po);
        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

        GLint numUniformBuffers;
        GL_CALL_EXTRA_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBuffers));

        return numUniformBuffers;
    }

    QT3DSI32
    NVRenderBackendGL3Impl::GetConstantBufferInfoByID(NVRenderBackendShaderProgramObject po,
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

        GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockName(programID, id, nameBufSize, length, nameBuf));

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

    void
    NVRenderBackendGL3Impl::GetConstantBufferParamIndices(NVRenderBackendShaderProgramObject po,
                                                          QT3DSU32 id, QT3DSI32 *indices)
    {
        QT3DS_ASSERT(po);
        QT3DS_ASSERT(indices);

        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

        if (indices) {
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockiv(programID, id,
                                                 GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, indices));
        }
    }

    void NVRenderBackendGL3Impl::GetConstantBufferParamInfoByIndices(
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
                GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, count, indices, GL_UNIFORM_TYPE, type));
                // convert to UIC types
                QT3DS_FOREACH(idx, count)
                {
                    type[idx] = m_Conversion.fromShaderGLToPropertyDataTypes(type[idx]);
                }
            }
            if (size) {
                GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, count, indices, GL_UNIFORM_SIZE, size));
            }
            if (offset) {
                GL_CALL_EXTRA_FUNCTION(
                    glGetActiveUniformsiv(programID, count, indices, GL_UNIFORM_OFFSET, offset));
            }
        }
    }

    void NVRenderBackendGL3Impl::ProgramSetConstantBlock(NVRenderBackendShaderProgramObject po,
                                                         QT3DSU32 blockIndex, QT3DSU32 binding)
    {
        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

        GL_CALL_EXTRA_FUNCTION(glUniformBlockBinding(programID, blockIndex, binding));
    }

    void NVRenderBackendGL3Impl::ProgramSetConstantBuffer(QT3DSU32 index,
                                                          NVRenderBackendBufferObject bo)
    {
        QT3DS_ASSERT(bo);

        GLuint bufID = HandleToID_cast(GLuint, size_t, bo);
        GL_CALL_EXTRA_FUNCTION(glBindBufferBase(GL_UNIFORM_BUFFER, index, bufID));
    }

    NVRenderBackend::NVRenderBackendQueryObject NVRenderBackendGL3Impl::CreateQuery()
    {
        QT3DSU32 glQueryID = 0;

        GL_CALL_EXTRA_FUNCTION(glGenQueries(1, &glQueryID));

        return (NVRenderBackendQueryObject)glQueryID;
    }

    void NVRenderBackendGL3Impl::ReleaseQuery(NVRenderBackendQueryObject qo)
    {
        GLuint queryID = HandleToID_cast(GLuint, size_t, qo);

        GL_CALL_EXTRA_FUNCTION(glDeleteQueries(1, &queryID));
    }

    void NVRenderBackendGL3Impl::BeginQuery(NVRenderBackendQueryObject qo,
                                            NVRenderQueryType::Enum type)
    {
        GLuint queryID = HandleToID_cast(GLuint, size_t, qo);

        GL_CALL_EXTRA_FUNCTION(glBeginQuery(m_Conversion.fromQueryTypeToGL(type), queryID));
    }

    void NVRenderBackendGL3Impl::EndQuery(NVRenderBackendQueryObject, NVRenderQueryType::Enum type)
    {
        GL_CALL_EXTRA_FUNCTION(glEndQuery(m_Conversion.fromQueryTypeToGL(type)));
    }

    void NVRenderBackendGL3Impl::GetQueryResult(NVRenderBackendQueryObject qo,
                                                NVRenderQueryResultType::Enum resultType,
                                                QT3DSU32 *params)
    {
        GLuint queryID = HandleToID_cast(GLuint, size_t, qo);

        if (params)
            GL_CALL_EXTRA_FUNCTION(glGetQueryObjectuiv(
                queryID, m_Conversion.fromQueryResultTypeToGL(resultType), params));
    }

    void NVRenderBackendGL3Impl::GetQueryResult(NVRenderBackendQueryObject qo,
                                                NVRenderQueryResultType::Enum resultType,
                                                QT3DSU64 *params)
    {
        if (m_backendSupport.caps.bits.bTimerQuerySupported) {
            GLuint queryID = HandleToID_cast(GLuint, size_t, qo);

            if (params)
#if defined(QT_OPENGL_ES)
                GL_CALL_TIMER_EXT(glGetQueryObjectui64vEXT(
                    queryID, m_Conversion.fromQueryResultTypeToGL(resultType), params));
#else
                GL_CALL_TIMER_EXT(glGetQueryObjectui64v(
                    queryID, m_Conversion.fromQueryResultTypeToGL(resultType), params));
#endif
        }
    }

    void NVRenderBackendGL3Impl::SetQueryTimer(NVRenderBackendQueryObject qo)
    {
        if (m_backendSupport.caps.bits.bTimerQuerySupported) {
            GLuint queryID = HandleToID_cast(GLuint, size_t, qo);
#if defined(QT_OPENGL_ES)
            GL_CALL_TIMER_EXT(glQueryCounterEXT(queryID, GL_TIMESTAMP_EXT));
#else
            GL_CALL_TIMER_EXT(glQueryCounter(queryID, GL_TIMESTAMP));
#endif
        }
    }

    NVRenderBackend::NVRenderBackendSyncObject
    NVRenderBackendGL3Impl::CreateSync(NVRenderSyncType::Enum syncType, NVRenderSyncFlags)
    {
        GLsync syncID = 0;

        syncID = GL_CALL_EXTRA_FUNCTION(glFenceSync(m_Conversion.fromSyncTypeToGL(syncType), 0));

        return NVRenderBackendSyncObject(syncID);
    }

    void NVRenderBackendGL3Impl::ReleaseSync(NVRenderBackendSyncObject so)
    {
        GLsync syncID = (GLsync)so;

        GL_CALL_EXTRA_FUNCTION(glDeleteSync(syncID));
    }

    void NVRenderBackendGL3Impl::WaitSync(NVRenderBackendSyncObject so, NVRenderCommandFlushFlags,
                                          QT3DSU64)
    {
        GLsync syncID = (GLsync)so;

        GL_CALL_EXTRA_FUNCTION(glWaitSync(syncID, 0, GL_TIMEOUT_IGNORED));
    }
}
}
