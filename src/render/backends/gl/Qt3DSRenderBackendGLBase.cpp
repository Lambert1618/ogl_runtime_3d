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

#include "render/backends/gl/Qt3DSRenderBackendGLBase.h"
#include "render/backends/gl/Qt3DSRenderBackendInputAssemblerGL.h"
#include "render/backends/gl/Qt3DSRenderBackendShaderProgramGL.h"
#include "render/backends/gl/Qt3DSRenderBackendRenderStatesGL.h"
#include "foundation/StringTable.h"

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError(#x, __FILE__, __LINE__)
#else
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError()
#endif

#define GL_CALL_FUNCTION(x) m_glFunctions->x; RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_EXTRA_FUNCTION(x) m_glExtraFunctions->x; RENDER_LOG_ERROR_PARAMS(x);

namespace qt3ds {
namespace render {

#ifndef GL_PROGRAM_SEPARABLE
#define GL_PROGRAM_SEPARABLE 0x8258
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D
#define GL_UNSIGNED_INT_IMAGE_2D 0x9063
#endif

#ifndef GL_UNSIGNED_INT_ATOMIC_COUNTER
#define GL_UNSIGNED_INT_ATOMIC_COUNTER 0x92DB
#endif

/// constructor
NVRenderBackendGLBase::NVRenderBackendGLBase(NVFoundationBase &fnd,
                                             qt3ds::foundation::IStringTable &stringTable,
                                             const QSurfaceFormat &format)
    : mRefCount(0)
    , m_Foundation(fnd)
    , m_StringTable(stringTable)
    , m_Conversion()
    , m_MaxAttribCount(0)
    , m_DrawBuffersArray(m_Foundation.getAllocator(),
                         "NVRenderBackendGLBase::m_DrawBuffersArray")
    , m_format(format)
{
    m_glFunctions = new QOpenGLFunctions;
    m_glFunctions->initializeOpenGLFunctions();
    m_glExtraFunctions = new QOpenGLExtraFunctions;
    m_glExtraFunctions->initializeOpenGLFunctions();

    // internal state tracker
    m_pCurrentRasterizerState =
        QT3DS_NEW(m_Foundation.getAllocator(), NVRenderBackendRasterizerStateGL)();
    m_pCurrentDepthStencilState =
        QT3DS_NEW(m_Foundation.getAllocator(), NVRenderBackendDepthStencilStateGL)();

    m_glFunctions->glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_maxAnisotropy);
}
/// destructor
NVRenderBackendGLBase::~NVRenderBackendGLBase()
{
    if (m_pCurrentRasterizerState)
        NVDelete(m_Foundation.getAllocator(), m_pCurrentRasterizerState);
    if (m_pCurrentDepthStencilState)
        NVDelete(m_Foundation.getAllocator(), m_pCurrentDepthStencilState);
    if (m_glFunctions)
        delete m_glFunctions;
    if (m_glExtraFunctions)
        delete m_glExtraFunctions;
}

NVRenderContextType NVRenderBackendGLBase::GetRenderContextType() const
{
    if (m_format.renderableType() == QSurfaceFormat::OpenGLES) {
        if (m_format.majorVersion() == 2)
            return NVRenderContextValues::GLES2;

        if (m_format.majorVersion() == 3) {
            if (m_format.minorVersion() >= 1)
                return NVRenderContextValues::GLES3PLUS;
            else
                return NVRenderContextValues::GLES3;
        }
    } else if (m_format.majorVersion() == 2) {
        return NVRenderContextValues::GL2;
    } else if (m_format.majorVersion() == 3) {
        return NVRenderContextValues::GL3;
    } else if (m_format.majorVersion() == 4) {
        return NVRenderContextValues::GL4;
    }

    return NVRenderContextValues::NullContext;
}

bool NVRenderBackendGLBase::isESCompatible() const
{
    return m_format.renderableType() == QSurfaceFormat::OpenGLES;
}

const char *NVRenderBackendGLBase::GetShadingLanguageVersion()
{
    const char *retval = (const char *)GL_CALL_FUNCTION(
        glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (retval == nullptr)
        return "";

    return retval;
}

QT3DSU32
NVRenderBackendGLBase::GetMaxCombinedTextureUnits()
{
    QT3DSI32 maxUnits;
    GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits));
    return maxUnits;
}

bool NVRenderBackendGLBase::GetRenderBackendCap(
    NVRenderBackend::NVRenderBackendCaps::Enum inCap) const
{
    bool bSupported = false;

    switch (inCap) {
    case NVRenderBackendCaps::FpRenderTarget:
        bSupported = m_backendSupport.caps.bits.bFPRenderTargetsSupported;
        break;
    case NVRenderBackendCaps::DepthStencilTexture:
        bSupported = m_backendSupport.caps.bits.bDepthStencilSupported;
        break;
    case NVRenderBackendCaps::ConstantBuffer:
        bSupported = m_backendSupport.caps.bits.bConstantBufferSupported;
        break;
    case NVRenderBackendCaps::DxtImages:
        bSupported = m_backendSupport.caps.bits.bDXTImagesSupported;
        break;
    case NVRenderBackendCaps::MsTexture:
        bSupported = m_backendSupport.caps.bits.bMsTextureSupported;
        break;
    case NVRenderBackendCaps::TexSwizzle:
        bSupported = m_backendSupport.caps.bits.bTextureSwizzleSupported;
        break;
    case NVRenderBackendCaps::FastBlits:
        bSupported = m_backendSupport.caps.bits.bFastBlitsSupported;
        break;
    case NVRenderBackendCaps::Tessellation:
        bSupported = m_backendSupport.caps.bits.bTessellationSupported;
        break;
    case NVRenderBackendCaps::Compute:
        bSupported = m_backendSupport.caps.bits.bComputeSupported;
        break;
    case NVRenderBackendCaps::Geometry:
        bSupported = m_backendSupport.caps.bits.bGeometrySupported;
        break;
    case NVRenderBackendCaps::SampleQuery: {
        // On the following context sample query is not supported
        NVRenderContextType noSamplesQuerySupportedContextFlags(NVRenderContextValues::GL2
                                                                | NVRenderContextValues::GLES2);
        NVRenderContextType ctxType = GetRenderContextType();
        bSupported = !(ctxType & noSamplesQuerySupportedContextFlags);
    } break;
    case NVRenderBackendCaps::TimerQuery:
        bSupported = m_backendSupport.caps.bits.bTimerQuerySupported;
        break;
    case NVRenderBackendCaps::CommandSync: {
        // On the following context sync objects are not supported
        NVRenderContextType noSyncObjectSupportedContextFlags(NVRenderContextValues::GL2
                                                              | NVRenderContextValues::GLES2);
        NVRenderContextType ctxType = GetRenderContextType();
        bSupported = !(ctxType & noSyncObjectSupportedContextFlags);
    } break;
    case NVRenderBackendCaps::TextureArray: {
        // On the following context texture arrays are not supported
        NVRenderContextType noTextureArraySupportedContextFlags(NVRenderContextValues::GL2
                                                                | NVRenderContextValues::GLES2);
        NVRenderContextType ctxType = GetRenderContextType();
        bSupported = !(ctxType & noTextureArraySupportedContextFlags);
    } break;
    case NVRenderBackendCaps::StorageBuffer:
        bSupported = m_backendSupport.caps.bits.bStorageBufferSupported;
        break;
    case NVRenderBackendCaps::AtomicCounterBuffer:
        bSupported = m_backendSupport.caps.bits.bAtomicCounterBufferSupported;
        break;
    case NVRenderBackendCaps::ShaderImageLoadStore:
        bSupported = m_backendSupport.caps.bits.bShaderImageLoadStoreSupported;
        break;
    case NVRenderBackendCaps::ProgramPipeline:
        bSupported = m_backendSupport.caps.bits.bProgramPipelineSupported;
        break;
    case NVRenderBackendCaps::PathRendering:
        bSupported = m_backendSupport.caps.bits.bNVPathRenderingSupported;
        break;
    case NVRenderBackendCaps::AdvancedBlend:
        bSupported = m_backendSupport.caps.bits.bNVAdvancedBlendSupported |
                     m_backendSupport.caps.bits.bKHRAdvancedBlendSupported;
        break;
    case NVRenderBackendCaps::AdvancedBlendKHR:
        bSupported = m_backendSupport.caps.bits.bKHRAdvancedBlendSupported;
        break;
    case NVRenderBackendCaps::BlendCoherency:
        bSupported = m_backendSupport.caps.bits.bNVBlendCoherenceSupported |
                     m_backendSupport.caps.bits.bKHRBlendCoherenceSupported;
        break;
    case NVRenderBackendCaps::gpuShader5:
        bSupported = m_backendSupport.caps.bits.bGPUShader5ExtensionSupported;
        break;
    case NVRenderBackendCaps::VertexArrayObject:
        bSupported = m_backendSupport.caps.bits.bVertexArrayObjectSupported;
        break;
    case NVRenderBackendCaps::StandardDerivatives:
        bSupported = m_backendSupport.caps.bits.bStandardDerivativesSupported;
        break;
    case NVRenderBackendCaps::TextureLod:
        bSupported = m_backendSupport.caps.bits.bTextureLodSupported;
        break;
    case NVRenderBackendCaps::BinaryProgram:
        bSupported = m_backendSupport.caps.bits.bBinaryProgramsSupported;
        break;
    case NVRenderBackendCaps::CompressedTextureEtc1:
        bSupported = m_backendSupport.caps.bits.bTextureEtc1Supported;
        break;
    case NVRenderBackendCaps::CompressedTextureEtc2:
        bSupported = m_backendSupport.caps.bits.bTextureEtc2Supported;
        break;
    case NVRenderBackendCaps::CompressedTextureAstc:
        bSupported = m_backendSupport.caps.bits.bTextureAstcSupported;
        break;
    default:
        QT3DS_ASSERT(false);
        bSupported = false;
        break;
    }

    return bSupported;
}

void NVRenderBackendGLBase::GetRenderBackendValue(NVRenderBackendQuery::Enum inQuery,
                                                  QT3DSI32 *params) const
{
    if (!params)
        return;
    switch (inQuery) {
    case NVRenderBackendQuery::MaxTextureSize:
        GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_TEXTURE_SIZE, params));
        break;
    case NVRenderBackendQuery::MaxTextureArrayLayers: {
        NVRenderContextType noTextureArraySupportedContextFlags(
            NVRenderContextValues::GL2 | NVRenderContextValues::GLES2);
        NVRenderContextType ctxType = GetRenderContextType();
        if (!(ctxType & noTextureArraySupportedContextFlags)) {
            GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, params));
        } else {
            *params = 0;
        }
    } break;
    case NVRenderBackendQuery::MaxConstantBufferSlots: {
        NVRenderContextType noConstantBufferSupportedContextFlags(
            NVRenderContextValues::GL2 | NVRenderContextValues::GLES2);
        NVRenderContextType ctxType = GetRenderContextType();
        if (!(ctxType & noConstantBufferSupportedContextFlags)) {
            GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, params));
        } else {
            *params = 0;
        }
    } break;
    case NVRenderBackendQuery::MaxConstantBufferBlockSize: {
        NVRenderContextType noConstantBufferSupportedContextFlags(
            NVRenderContextValues::GL2 | NVRenderContextValues::GLES2);
        NVRenderContextType ctxType = GetRenderContextType();
        if (!(ctxType & noConstantBufferSupportedContextFlags)) {
            GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, params));
        } else {
            *params = 0;
        }
    } break;
    default:
        QT3DS_ASSERT(false);
        *params = 0;
        break;
    }
}

QT3DSU32
NVRenderBackendGLBase::GetDepthBits() const
{
    QT3DSI32 depthBits;
    GL_CALL_FUNCTION(glGetIntegerv(GL_DEPTH_BITS, &depthBits));
    return depthBits;
}

QT3DSU32
NVRenderBackendGLBase::GetStencilBits() const
{
    QT3DSI32 stencilBits;
    GL_CALL_FUNCTION(glGetIntegerv(GL_STENCIL_BITS, &stencilBits));
    return stencilBits;
}

void NVRenderBackendGLBase::SetMultisample(bool bEnable)
{
    QT3DS_ASSERT(m_backendSupport.caps.bits.bMsTextureSupported || !bEnable);
    // For GL ES explicit multisample enabling is not needed
    // and does not exist
    NVRenderContextType noMsaaEnableContextFlags(NVRenderContextValues::GLES2
                                                 | NVRenderContextValues::GLES3
                                                 | NVRenderContextValues::GLES3PLUS);
    NVRenderContextType ctxType = GetRenderContextType();
    if (!(ctxType & noMsaaEnableContextFlags)) {
        SetRenderState(bEnable, NVRenderState::Multisample);
    }
}

void NVRenderBackendGLBase::SetRenderState(bool bEnable, const NVRenderState::Enum value)
{
    if (value == NVRenderState::DepthWrite) {
        GL_CALL_FUNCTION(glDepthMask(bEnable));
    } else {
        if (bEnable) {
            GL_CALL_FUNCTION(glEnable(m_Conversion.fromRenderStateToGL(value)));
        } else {
            GL_CALL_FUNCTION(glDisable(m_Conversion.fromRenderStateToGL(value)));
        }
    }
}

NVRenderBackend::NVRenderBackendDepthStencilStateObject
NVRenderBackendGLBase::CreateDepthStencilState(
    bool enableDepth, bool depthMask, NVRenderBoolOp::Enum depthFunc, bool enableStencil,
    NVRenderStencilFunctionArgument &stencilFuncFront,
    NVRenderStencilFunctionArgument &stencilFuncBack,
    NVRenderStencilOperationArgument &depthStencilOpFront,
    NVRenderStencilOperationArgument &depthStencilOpBack)
{
    NVRenderBackendDepthStencilStateGL *retval =
        QT3DS_NEW(m_Foundation.getAllocator(), NVRenderBackendDepthStencilStateGL)(
            enableDepth, depthMask, depthFunc, enableStencil, stencilFuncFront, stencilFuncBack,
            depthStencilOpFront, depthStencilOpBack);

    return (NVRenderBackend::NVRenderBackendDepthStencilStateObject)retval;
}

void NVRenderBackendGLBase::ReleaseDepthStencilState(
    NVRenderBackendDepthStencilStateObject inDepthStencilState)
{
    NVRenderBackendDepthStencilStateGL *inputState =
        (NVRenderBackendDepthStencilStateGL *)inDepthStencilState;
    if (inputState)
        NVDelete(m_Foundation.getAllocator(), inputState);
}

NVRenderBackend::NVRenderBackendRasterizerStateObject
NVRenderBackendGLBase::CreateRasterizerState(QT3DSF32 depthBias, QT3DSF32 depthScale,
                                             NVRenderFaces::Enum cullFace)
{
    NVRenderBackendRasterizerStateGL *retval =
        QT3DS_NEW(m_Foundation.getAllocator(),
               NVRenderBackendRasterizerStateGL)(depthBias, depthScale, cullFace);

    return (NVRenderBackend::NVRenderBackendRasterizerStateObject)retval;
}

void NVRenderBackendGLBase::ReleaseRasterizerState(
    NVRenderBackendRasterizerStateObject rasterizerState)
{
    NVRenderBackendRasterizerStateGL *inputState =
        (NVRenderBackendRasterizerStateGL *)rasterizerState;
    if (inputState)
        NVDelete(m_Foundation.getAllocator(), inputState);
}

void NVRenderBackendGLBase::SetDepthStencilState(
    NVRenderBackendDepthStencilStateObject inDepthStencilState)
{
    NVRenderBackendDepthStencilStateGL *inputState =
        (NVRenderBackendDepthStencilStateGL *)inDepthStencilState;
    if (inputState && !(*m_pCurrentDepthStencilState == *inputState)) {
        // we check on a per single state base
        if (inputState->m_DepthEnable != m_pCurrentDepthStencilState->m_DepthEnable) {
            SetRenderState(inputState->m_DepthEnable, NVRenderState::DepthTest);
            m_pCurrentDepthStencilState->m_DepthEnable = inputState->m_DepthEnable;
        }
        if (inputState->m_StencilEnable != m_pCurrentDepthStencilState->m_StencilEnable) {
            SetRenderState(inputState->m_StencilEnable, NVRenderState::StencilTest);
            m_pCurrentDepthStencilState->m_StencilEnable = inputState->m_StencilEnable;
        }

        if (inputState->m_DepthMask != m_pCurrentDepthStencilState->m_DepthMask) {
            GL_CALL_FUNCTION(glDepthMask(inputState->m_DepthMask));
            m_pCurrentDepthStencilState->m_DepthMask = inputState->m_DepthMask;
        }

        if (inputState->m_DepthFunc != m_pCurrentDepthStencilState->m_DepthFunc) {
            GL_CALL_FUNCTION(glDepthFunc(m_Conversion.fromBoolOpToGL(inputState->m_DepthFunc)));
            m_pCurrentDepthStencilState->m_DepthFunc = inputState->m_DepthFunc;
        }

        if (!(inputState->m_DepthStencilOpFront
              == m_pCurrentDepthStencilState->m_DepthStencilOpFront)) {
            GL_CALL_FUNCTION(glStencilOpSeparate(
                GL_FRONT,
                m_Conversion.fromStencilOpToGL(inputState->m_DepthStencilOpFront.m_StencilFail),
                m_Conversion.fromStencilOpToGL(inputState->m_DepthStencilOpFront.m_DepthFail),
                m_Conversion.fromStencilOpToGL(inputState->m_DepthStencilOpFront.m_DepthPass)));
            m_pCurrentDepthStencilState->m_DepthStencilOpFront =
                inputState->m_DepthStencilOpFront;
        }

        if (!(inputState->m_DepthStencilOpBack
              == m_pCurrentDepthStencilState->m_DepthStencilOpBack)) {
            GL_CALL_FUNCTION(glStencilOpSeparate(
                GL_BACK,
                m_Conversion.fromStencilOpToGL(inputState->m_DepthStencilOpBack.m_StencilFail),
                m_Conversion.fromStencilOpToGL(inputState->m_DepthStencilOpBack.m_DepthFail),
                m_Conversion.fromStencilOpToGL(inputState->m_DepthStencilOpBack.m_DepthPass)));
            m_pCurrentDepthStencilState->m_DepthStencilOpBack =
                inputState->m_DepthStencilOpBack;
        }

        if (!(inputState->m_StencilFuncFront
              == m_pCurrentDepthStencilState->m_StencilFuncFront)) {
            GL_CALL_FUNCTION(glStencilFuncSeparate(
                GL_FRONT,
                m_Conversion.fromBoolOpToGL(inputState->m_StencilFuncFront.m_Function),
                inputState->m_StencilFuncFront.m_ReferenceValue,
                inputState->m_StencilFuncFront.m_Mask));
            m_pCurrentDepthStencilState->m_StencilFuncFront = inputState->m_StencilFuncFront;
        }

        if (!(inputState->m_StencilFuncBack
              == m_pCurrentDepthStencilState->m_StencilFuncBack)) {
            GL_CALL_FUNCTION(glStencilFuncSeparate(
                GL_BACK, m_Conversion.fromBoolOpToGL(inputState->m_StencilFuncBack.m_Function),
                inputState->m_StencilFuncBack.m_ReferenceValue,
                inputState->m_StencilFuncBack.m_Mask));
            m_pCurrentDepthStencilState->m_StencilFuncBack = inputState->m_StencilFuncBack;
        }
    }
}

void
NVRenderBackendGLBase::SetRasterizerState(NVRenderBackendRasterizerStateObject rasterizerState)
{
    NVRenderBackendRasterizerStateGL *inputState =
        (NVRenderBackendRasterizerStateGL *)rasterizerState;
    if (inputState && !(*m_pCurrentRasterizerState == *inputState)) {
        // store current state
        *m_pCurrentRasterizerState = *inputState;

        if (m_pCurrentRasterizerState->m_DepthBias != 0.0
            || m_pCurrentRasterizerState->m_DepthScale != 0.0) {
            GL_CALL_FUNCTION(glEnable(GL_POLYGON_OFFSET_FILL));
        } else {
            GL_CALL_FUNCTION(glDisable(GL_POLYGON_OFFSET_FILL));
        }

        GL_CALL_FUNCTION(glPolygonOffset(m_pCurrentRasterizerState->m_DepthBias,
                                   m_pCurrentRasterizerState->m_DepthScale));

        GL_CALL_FUNCTION(
            glCullFace(m_Conversion.fromFacesToGL(m_pCurrentRasterizerState->m_CullFace)));
    }
}

bool NVRenderBackendGLBase::GetRenderState(const NVRenderState::Enum value)
{
    bool enabled = GL_CALL_FUNCTION(glIsEnabled(m_Conversion.fromRenderStateToGL(value)));
    return enabled;
}

NVRenderBoolOp::Enum NVRenderBackendGLBase::GetDepthFunc()
{
    QT3DSI32 value;
    GL_CALL_FUNCTION(glGetIntegerv(GL_DEPTH_FUNC, &value));
    return m_Conversion.fromGLToBoolOp(value);
}

void NVRenderBackendGLBase::SetDepthFunc(const NVRenderBoolOp::Enum func)
{
    GL_CALL_FUNCTION(glDepthFunc(m_Conversion.fromBoolOpToGL(func)));
}

bool NVRenderBackendGLBase::GetDepthWrite()
{
    QT3DSI32 value;
    GL_CALL_FUNCTION(glGetIntegerv(GL_DEPTH_WRITEMASK, (GLint *)&value));
    return value ? true : false;
}

void NVRenderBackendGLBase::SetDepthWrite(bool bEnable)
{
    GL_CALL_FUNCTION(glDepthMask(bEnable));
}

void NVRenderBackendGLBase::SetColorWrites(bool bRed, bool bGreen, bool bBlue, bool bAlpha)
{
    GL_CALL_FUNCTION(glColorMask(bRed, bGreen, bBlue, bAlpha));
}

void NVRenderBackendGLBase::GetBlendFunc(NVRenderBlendFunctionArgument *pBlendFuncArg)
{
    QT3DS_ASSERT(pBlendFuncArg);
    QT3DSI32_4 values;

    GL_CALL_FUNCTION(glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *)&values.x));
    GL_CALL_FUNCTION(glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *)&values.y));
    GL_CALL_FUNCTION(glGetIntegerv(GL_BLEND_DST_RGB, (GLint *)&values.z));
    GL_CALL_FUNCTION(glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *)&values.w));

    pBlendFuncArg->m_SrcRGB = m_Conversion.fromGLToSrcBlendFunc(values.x);
    pBlendFuncArg->m_SrcAlpha = m_Conversion.fromGLToSrcBlendFunc(values.y);
    pBlendFuncArg->m_DstRGB = m_Conversion.fromGLToDstBlendFunc(values.z);
    pBlendFuncArg->m_DstAlpha = m_Conversion.fromGLToDstBlendFunc(values.w);
}

void NVRenderBackendGLBase::SetBlendFunc(const NVRenderBlendFunctionArgument &blendFuncArg)
{
    QT3DSI32_4 values;

    values.x = m_Conversion.fromSrcBlendFuncToGL(blendFuncArg.m_SrcRGB);
    values.y = m_Conversion.fromDstBlendFuncToGL(blendFuncArg.m_DstRGB);
    values.z = m_Conversion.fromSrcBlendFuncToGL(blendFuncArg.m_SrcAlpha);
    values.w = m_Conversion.fromDstBlendFuncToGL(blendFuncArg.m_DstAlpha);

    GL_CALL_FUNCTION(glBlendFuncSeparate(values.x, values.y, values.z, values.w));
}

void NVRenderBackendGLBase::SetBlendEquation(const NVRenderBlendEquationArgument &)
{
    // needs GL4 / GLES 3.1
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::SetBlendBarrier()
{
    // needs GL4 / GLES 3.1
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::GetScissorRect(NVRenderRect *pRect)
{
    QT3DS_ASSERT(pRect);
    GL_CALL_FUNCTION(glGetIntegerv(GL_SCISSOR_BOX, (GLint *)pRect));
}

void NVRenderBackendGLBase::SetScissorRect(const NVRenderRect &rect)
{
    GL_CALL_FUNCTION(glScissor(rect.m_X, rect.m_Y, rect.m_Width, rect.m_Height));
}

void NVRenderBackendGLBase::GetViewportRect(NVRenderRect *pRect)
{
    QT3DS_ASSERT(pRect);
    GL_CALL_FUNCTION(glGetIntegerv(GL_VIEWPORT, (GLint *)pRect));
}

void NVRenderBackendGLBase::SetViewportRect(const NVRenderRect &rect)
{
    GL_CALL_FUNCTION(glViewport(rect.m_X, rect.m_Y, rect.m_Width, rect.m_Height););
}

void NVRenderBackendGLBase::SetClearColor(const QT3DSVec4 *pClearColor)
{
    QT3DS_ASSERT(pClearColor);

    GL_CALL_FUNCTION(glClearColor(pClearColor->x, pClearColor->y,
        pClearColor->z, pClearColor->w));
}

void NVRenderBackendGLBase::Clear(NVRenderClearFlags flags)
{
    GL_CALL_FUNCTION(glClear(m_Conversion.fromClearFlagsToGL(flags)));
}

NVRenderBackend::NVRenderBackendBufferObject
NVRenderBackendGLBase::CreateBuffer(size_t size, NVRenderBufferBindFlags bindFlags,
                                    NVRenderBufferUsageType::Enum usage, const void *hostPtr)
{
    GLuint bufID = 0;

    GL_CALL_FUNCTION(glGenBuffers(1, &bufID));

    if (bufID && size) {
        GLenum target = m_Conversion.fromBindBufferFlagsToGL(bindFlags);
        if (target != GL_INVALID_ENUM) {
            GL_CALL_FUNCTION(glBindBuffer(target, bufID));
            GL_CALL_FUNCTION(glBufferData(target, size, hostPtr,
                                    m_Conversion.fromBufferUsageTypeToGL(usage)));
        } else {
            GL_CALL_FUNCTION(glDeleteBuffers(1, &bufID));
            bufID = 0;
            qCCritical(GL_ERROR) << GLConversion::processGLError(target);
        }
    }

    return (NVRenderBackend::NVRenderBackendBufferObject)bufID;
}

void NVRenderBackendGLBase::BindBuffer(NVRenderBackendBufferObject bo,
                                       NVRenderBufferBindFlags bindFlags)
{
    GLuint bufID = HandleToID_cast(GLuint, size_t, bo);
    GL_CALL_FUNCTION(glBindBuffer(m_Conversion.fromBindBufferFlagsToGL(bindFlags), bufID));
}

void NVRenderBackendGLBase::ReleaseBuffer(NVRenderBackendBufferObject bo)
{
    GLuint bufID = HandleToID_cast(GLuint, size_t, bo);
    GL_CALL_FUNCTION(glDeleteBuffers(1, &bufID));
}

void NVRenderBackendGLBase::UpdateBuffer(NVRenderBackendBufferObject bo,
                                         NVRenderBufferBindFlags bindFlags, size_t size,
                                         NVRenderBufferUsageType::Enum usage, const void *data)
{
    GLuint bufID = HandleToID_cast(GLuint, size_t, bo);
    GLenum target = m_Conversion.fromBindBufferFlagsToGL(bindFlags);
    GL_CALL_FUNCTION(glBindBuffer(target, bufID));
    GL_CALL_FUNCTION(glBufferData(target, size, data, m_Conversion.fromBufferUsageTypeToGL(usage)));
}

void NVRenderBackendGLBase::UpdateBufferRange(NVRenderBackendBufferObject bo,
                                              NVRenderBufferBindFlags bindFlags, size_t offset,
                                              size_t size, const void *data)
{
    GLuint bufID = HandleToID_cast(GLuint, size_t, bo);
    GLenum target = m_Conversion.fromBindBufferFlagsToGL(bindFlags);
    GL_CALL_FUNCTION(glBindBuffer(target, bufID));
    GL_CALL_FUNCTION(glBufferSubData(target, offset, size, data));
}

void *NVRenderBackendGLBase::MapBuffer(NVRenderBackendBufferObject, NVRenderBufferBindFlags,
                                       size_t, size_t, NVRenderBufferAccessFlags)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return nullptr;
}

bool NVRenderBackendGLBase::UnmapBuffer(NVRenderBackendBufferObject, NVRenderBufferBindFlags)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return true;
}

void NVRenderBackendGLBase::SetMemoryBarrier(NVRenderBufferBarrierFlags)
{
    // needs GL 4 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

NVRenderBackend::NVRenderBackendQueryObject NVRenderBackendGLBase::CreateQuery()
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return NVRenderBackendQueryObject(0);
}

void NVRenderBackendGLBase::ReleaseQuery(NVRenderBackendQueryObject)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::BeginQuery(NVRenderBackendQueryObject, NVRenderQueryType::Enum)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::EndQuery(NVRenderBackendQueryObject, NVRenderQueryType::Enum)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::GetQueryResult(NVRenderBackendQueryObject,
                                           NVRenderQueryResultType::Enum, QT3DSU32 *)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::GetQueryResult(NVRenderBackendQueryObject,
                                           NVRenderQueryResultType::Enum, QT3DSU64 *)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::SetQueryTimer(NVRenderBackendQueryObject)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

NVRenderBackend::NVRenderBackendSyncObject
    NVRenderBackendGLBase::CreateSync(NVRenderSyncType::Enum, NVRenderSyncFlags)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return NVRenderBackendSyncObject(0);
}

void NVRenderBackendGLBase::ReleaseSync(NVRenderBackendSyncObject)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::WaitSync(NVRenderBackendSyncObject, NVRenderCommandFlushFlags,
                                     QT3DSU64)
{
    // needs GL 3 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

NVRenderBackend::NVRenderBackendRenderTargetObject NVRenderBackendGLBase::CreateRenderTarget()
{
    GLuint fboID = 0;

    GL_CALL_FUNCTION(glGenFramebuffers(1, &fboID));

    return (NVRenderBackend::NVRenderBackendRenderTargetObject)fboID;
}

void NVRenderBackendGLBase::ReleaseRenderTarget(NVRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, size_t, rto);

    if (fboID) {
        GL_CALL_FUNCTION(glDeleteFramebuffers(1, &fboID));
    }
}

void NVRenderBackendGLBase::RenderTargetAttach(NVRenderBackendRenderTargetObject /* rto */,
                                               NVRenderFrameBufferAttachments::Enum attachment,
                                               NVRenderBackendRenderbufferObject rbo)
{
    // rto must be the current render target
    GLuint rbID = HandleToID_cast(GLuint, size_t, rbo);

    GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);

    GL_CALL_FUNCTION(glFramebufferRenderbuffer(GL_FRAMEBUFFER, glAttach, GL_RENDERBUFFER, rbID));
}

void NVRenderBackendGLBase::RenderTargetAttach(NVRenderBackendRenderTargetObject /* rto */,
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

    GL_CALL_FUNCTION(glFramebufferTexture2D(GL_FRAMEBUFFER, glAttach, glTarget, texID, 0))
}

void NVRenderBackendGLBase::RenderTargetAttach(NVRenderBackendRenderTargetObject,
                                               NVRenderFrameBufferAttachments::Enum,
                                               NVRenderBackendTextureObject, QT3DSI32, QT3DSI32)
{
    // Needs GL3 or GLES 3
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::SetRenderTarget(NVRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, size_t, rto);
    if (!fboID)
        fboID = QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext()->defaultFramebufferObject();

    GL_CALL_FUNCTION(glBindFramebuffer(GL_FRAMEBUFFER, fboID));
}

bool NVRenderBackendGLBase::RenderTargetIsValid(NVRenderBackendRenderTargetObject /* rto */)
{
    // rto must be the current render target
    GLenum completeStatus = GL_CALL_FUNCTION(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    switch (completeStatus) {
#define HANDLE_INCOMPLETE_STATUS(x)                                                             \
case x:                                                                                         \
    qCCritical(INTERNAL_ERROR, "Framebuffer is not complete: %s", #x);                          \
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
NVRenderBackendGLBase::CreateRenderbuffer(NVRenderRenderBufferFormats::Enum storageFormat,
                                          size_t width, size_t height)
{
    GLuint bufID = 0;

    GL_CALL_FUNCTION(glGenRenderbuffers(1, &bufID));
    GL_CALL_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, bufID));
    GL_CALL_FUNCTION(glRenderbufferStorage(GL_RENDERBUFFER,
                          GLConversion::fromRenderBufferFormatsToRenderBufferGL(storageFormat),
                          (GLsizei)width, (GLsizei)height));

    // check for error
    GLenum error = m_glFunctions->glGetError();
    if (error != GL_NO_ERROR) {
        qCCritical(GL_ERROR) << GLConversion::processGLError(error);
        QT3DS_ASSERT(false);
        GL_CALL_FUNCTION(glDeleteRenderbuffers(1, &bufID));
        bufID = 0;
    }

    GL_CALL_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    return (NVRenderBackend::NVRenderBackendRenderbufferObject)bufID;
}

void NVRenderBackendGLBase::ReleaseRenderbuffer(NVRenderBackendRenderbufferObject rbo)
{
    GLuint bufID = HandleToID_cast(GLuint, size_t, rbo);

    if (bufID) {
        GL_CALL_FUNCTION(glDeleteRenderbuffers(1, &bufID));
    }
}

bool NVRenderBackendGLBase::ResizeRenderbuffer(NVRenderBackendRenderbufferObject rbo,
                                               NVRenderRenderBufferFormats::Enum storageFormat,
                                               size_t width, size_t height)
{
    bool success = true;
    GLuint bufID = HandleToID_cast(GLuint, size_t, rbo);

    QT3DS_ASSERT(bufID);

    GL_CALL_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, bufID));
    GL_CALL_FUNCTION(glRenderbufferStorage(GL_RENDERBUFFER,
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

NVRenderBackend::NVRenderBackendTextureObject NVRenderBackendGLBase::CreateTexture()
{
    GLuint texID = 0;

    GL_CALL_FUNCTION(glGenTextures(1, &texID));
    return (NVRenderBackend::NVRenderBackendTextureObject)texID;
}

void NVRenderBackendGLBase::BindTexture(NVRenderBackendTextureObject to,
                                        NVRenderTextureTargetType::Enum target, QT3DSU32 unit)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GL_CALL_FUNCTION(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CALL_FUNCTION(glBindTexture(m_Conversion.fromTextureTargetToGL(target), texID));
}

void NVRenderBackendGLBase::BindImageTexture(NVRenderBackendTextureObject, QT3DSU32, QT3DSI32, bool,
                                             QT3DSI32, NVRenderImageAccessType::Enum,
                                             NVRenderTextureFormats::Enum)
{
    // needs GL 4 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::ReleaseTexture(NVRenderBackendTextureObject to)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GL_CALL_FUNCTION(glDeleteTextures(1, &texID));
}

void NVRenderBackendGLBase::SetTextureData2D(
    NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, QT3DSU32 level,
    NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height, QT3DSI32 border,
    NVRenderTextureFormats::Enum format, const void *hostPtr)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GL_CALL_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));
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

    GL_CALL_FUNCTION(glTexImage2D(glTarget, level, glInternalFormat, (GLsizei)width, (GLsizei)height,
                            border, glformat, gltype, hostPtr));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

// This will look very SetTextureData2D, but the target for glBindTexture will be different from
// the target for
// glTexImage2D.
void NVRenderBackendGLBase::SetTextureDataCubeFace(
    NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, QT3DSU32 level,
    NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height, QT3DSI32 border,
    NVRenderTextureFormats::Enum format, const void *hostPtr)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GLenum glTexTarget =
        m_Conversion.fromTextureTargetToGL(NVRenderTextureTargetType::TextureCube);
    GL_CALL_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_FUNCTION(glBindTexture(glTexTarget, texID));
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

    // for es2 internal format must be same as format
    if (GetRenderContextType() == NVRenderContextValues::GLES2)
        glInternalFormat = glformat;

    GL_CALL_FUNCTION(glTexImage2D(glTarget, level, glInternalFormat, (GLsizei)width, (GLsizei)height,
                            border, glformat, gltype, hostPtr));

    GL_CALL_FUNCTION(glBindTexture(glTexTarget, 0));
}

void NVRenderBackendGLBase::CreateTextureStorage2D(NVRenderBackendTextureObject,
                                                   NVRenderTextureTargetType::Enum, QT3DSU32,
                                                   NVRenderTextureFormats::Enum, size_t, size_t)
{
    // you need GL 4.2 or GLES 3.1
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::SetTextureSubData2D(NVRenderBackendTextureObject to,
                                                NVRenderTextureTargetType::Enum target,
                                                QT3DSU32 level, QT3DSI32 xOffset, QT3DSI32 yOffset,
                                                size_t width, size_t height,
                                                NVRenderTextureFormats::Enum format,
                                                const void *hostPtr)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GL_CALL_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));

    NVRenderTextureSwizzleMode::Enum swizzleMode = NVRenderTextureSwizzleMode::NoSwizzle;
    format = m_Conversion.replaceDeprecatedTextureFormat(GetRenderContextType(), format,
                                                         swizzleMode);

    GLenum glformat = 0, glInternalFormat = 0, gltype = 0;
    m_Conversion.fromUncompressedTextureFormatToGL(GetRenderContextType(), format, glformat,
                                                   gltype, glInternalFormat);
    GL_CALL_FUNCTION(glTexSubImage2D(glTarget, level, xOffset, yOffset, (GLsizei)width,
                               (GLsizei)height, glformat, gltype, hostPtr));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

void NVRenderBackendGLBase::SetCompressedTextureData2D(
    NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, QT3DSU32 level,
    NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height, QT3DSI32 border,
    size_t imageSize, const void *hostPtr)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GL_CALL_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));

    GLenum glformat = m_Conversion.fromCompressedTextureFormatToGL(internalFormat);
    GL_CALL_FUNCTION(glCompressedTexImage2D(glTarget, level, glformat, (GLsizei)width,
                                      (GLsizei)height, border, (GLsizei)imageSize, hostPtr));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

void NVRenderBackendGLBase::SetCompressedTextureDataCubeFace(
    NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, QT3DSU32 level,
    NVRenderTextureFormats::Enum internalFormat, size_t width, size_t height, QT3DSI32 border,
    size_t imageSize, const void *hostPtr)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GLenum glTexTarget =
        m_Conversion.fromTextureTargetToGL(NVRenderTextureTargetType::TextureCube);
    GL_CALL_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_FUNCTION(glBindTexture(glTexTarget, texID));

    GLenum glformat = m_Conversion.fromCompressedTextureFormatToGL(internalFormat);
    GL_CALL_FUNCTION(glCompressedTexImage2D(glTarget, level, glformat, (GLsizei)width,
                                      (GLsizei)height, border, (GLsizei)imageSize, hostPtr));

    GL_CALL_FUNCTION(glBindTexture(glTexTarget, 0));
}

void NVRenderBackendGLBase::SetCompressedTextureSubData2D(
    NVRenderBackendTextureObject to, NVRenderTextureTargetType::Enum target, QT3DSU32 level,
    QT3DSI32 xOffset, QT3DSI32 yOffset, size_t width, size_t height,
    NVRenderTextureFormats::Enum format, size_t imageSize, const void *hostPtr)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GL_CALL_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));

    GLenum glformat = m_Conversion.fromCompressedTextureFormatToGL(format);
    GL_CALL_FUNCTION(glCompressedTexSubImage2D(glTarget, level, xOffset, yOffset, (GLsizei)width,
                                         (GLsizei)height, glformat, (GLsizei)imageSize,
                                         hostPtr));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

void NVRenderBackendGLBase::SetTextureData3D(NVRenderBackendTextureObject,
                                             NVRenderTextureTargetType::Enum, QT3DSU32,
                                             NVRenderTextureFormats::Enum, size_t, size_t,
                                             size_t, QT3DSI32, NVRenderTextureFormats::Enum,
                                             const void *)
{
    // needs GL3 or GLES3
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::GenerateMipMaps(NVRenderBackendTextureObject to,
                                            NVRenderTextureTargetType::Enum target,
                                            NVRenderHint::Enum genType)
{
    GLuint texID = HandleToID_cast(GLuint, size_t, to);
    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);
    GL_CALL_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));

    GLenum glValue = GLConversion::fromHintToGL(genType);
    GL_CALL_FUNCTION(glHint(GL_GENERATE_MIPMAP_HINT, glValue));
    GL_CALL_FUNCTION(glGenerateMipmap(glTarget));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

NVRenderTextureSwizzleMode::Enum
NVRenderBackendGLBase::GetTextureSwizzleMode(const NVRenderTextureFormats::Enum inFormat) const
{
    NVRenderTextureSwizzleMode::Enum swizzleMode = NVRenderTextureSwizzleMode::NoSwizzle;
    m_Conversion.replaceDeprecatedTextureFormat(GetRenderContextType(), inFormat, swizzleMode);

    return swizzleMode;
}

NVRenderBackend::NVRenderBackendSamplerObject NVRenderBackendGLBase::CreateSampler(
    NVRenderTextureMinifyingOp::Enum minFilter, NVRenderTextureMagnifyingOp::Enum magFilter,
    NVRenderTextureCoordOp::Enum wrapS, NVRenderTextureCoordOp::Enum wrapT,
    NVRenderTextureCoordOp::Enum wrapR, QT3DSI32 minLod, QT3DSI32 maxLod, QT3DSF32 lodBias,
    NVRenderTextureCompareMode::Enum compareMode, NVRenderTextureCompareOp::Enum compareFunc,
    QT3DSF32 anisotropy, QT3DSF32 *borderColor)
{
    // Satisfy the compiler
    // We don"t setup the state here for GL
    // but we need to pass on the variables here
    // to satisfy the interface
    NVRENDER_BACKEND_UNUSED(minFilter);
    NVRENDER_BACKEND_UNUSED(magFilter);
    NVRENDER_BACKEND_UNUSED(wrapS);
    NVRENDER_BACKEND_UNUSED(wrapT);
    NVRENDER_BACKEND_UNUSED(wrapR);
    NVRENDER_BACKEND_UNUSED(minLod);
    NVRENDER_BACKEND_UNUSED(maxLod);
    NVRENDER_BACKEND_UNUSED(lodBias);
    NVRENDER_BACKEND_UNUSED(compareMode);
    NVRENDER_BACKEND_UNUSED(compareFunc);
    NVRENDER_BACKEND_UNUSED(anisotropy);
    NVRENDER_BACKEND_UNUSED(borderColor);

    // return a dummy handle
    return (NVRenderBackend::NVRenderBackendSamplerObject)0x0001;
}

void NVRenderBackendGLBase::UpdateSampler(
    NVRenderBackendSamplerObject /* so */, NVRenderTextureTargetType::Enum target,
    NVRenderTextureMinifyingOp::Enum minFilter, NVRenderTextureMagnifyingOp::Enum magFilter,
    NVRenderTextureCoordOp::Enum wrapS, NVRenderTextureCoordOp::Enum wrapT,
    NVRenderTextureCoordOp::Enum wrapR, QT3DSF32 minLod, QT3DSF32 maxLod, QT3DSF32 lodBias,
    NVRenderTextureCompareMode::Enum compareMode, NVRenderTextureCompareOp::Enum compareFunc,
    QT3DSF32 anisotropy, QT3DSF32 *borderColor)
{
    // Satisfy the compiler
    // These are not available in GLES 2 and we don't use them right now
    NVRENDER_BACKEND_UNUSED(wrapR);
    NVRENDER_BACKEND_UNUSED(lodBias);
    NVRENDER_BACKEND_UNUSED(minLod);
    NVRENDER_BACKEND_UNUSED(maxLod);
    NVRENDER_BACKEND_UNUSED(compareMode);
    NVRENDER_BACKEND_UNUSED(compareFunc);
    NVRENDER_BACKEND_UNUSED(borderColor);

    GLenum glTarget = m_Conversion.fromTextureTargetToGL(target);

    GL_CALL_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER,
                               m_Conversion.fromTextureMinifyingOpToGL(minFilter)));
    GL_CALL_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER,
                               m_Conversion.fromTextureMagnifyingOpToGL(magFilter)));
    GL_CALL_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_S,
                               m_Conversion.fromTextureCoordOpToGL(wrapS)));
    GL_CALL_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_T,
                               m_Conversion.fromTextureCoordOpToGL(wrapT)));
    if (m_backendSupport.caps.bits.bAnistropySupported) {
        GL_CALL_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                         qMin(m_maxAnisotropy, anisotropy)));
    }
}

void NVRenderBackendGLBase::UpdateTextureObject(NVRenderBackendTextureObject to,
                                                NVRenderTextureTargetType::Enum target,
                                                QT3DSI32 baseLevel, QT3DSI32 maxLevel)
{
    NVRENDER_BACKEND_UNUSED(to);
    NVRENDER_BACKEND_UNUSED(target);
    NVRENDER_BACKEND_UNUSED(baseLevel);
    NVRENDER_BACKEND_UNUSED(maxLevel);
}

void NVRenderBackendGLBase::UpdateTextureSwizzle(NVRenderBackendTextureObject to,
                                                 NVRenderTextureTargetType::Enum target,
                                                 NVRenderTextureSwizzleMode::Enum swizzleMode)
{
    NVRENDER_BACKEND_UNUSED(to);
    NVRENDER_BACKEND_UNUSED(target);

    // Nothing to do here still might be called
    QT3DS_ASSERT(swizzleMode == NVRenderTextureSwizzleMode::NoSwizzle);

    NVRENDER_BACKEND_UNUSED(swizzleMode);
}

void NVRenderBackendGLBase::ReleaseSampler(NVRenderBackendSamplerObject so)
{
    GLuint samplerID = HandleToID_cast(GLuint, size_t, so);
    if (!samplerID)
        return;
    // otherwise nothing to do
}

NVRenderBackend::NVRenderBackendAttribLayoutObject
NVRenderBackendGLBase::CreateAttribLayout(NVConstDataRef<NVRenderVertexBufferEntry> attribs)
{
    QT3DSU32 attribLayoutSize = sizeof(NVRenderBackendAttributeLayoutGL);
    QT3DSU32 entrySize = sizeof(NVRenderBackendLayoutEntryGL) * attribs.size();
    QT3DSU8 *newMem = (QT3DSU8 *)QT3DS_ALLOC(m_Foundation.getAllocator(),
                                             attribLayoutSize + entrySize,
                                             "BackendAttributeLayoutGL");
    NVDataRef<NVRenderBackendLayoutEntryGL> entryRef =
        PtrAtOffset<NVRenderBackendLayoutEntryGL>(newMem, attribLayoutSize, entrySize);
    QT3DSU32 maxInputSlot = 0;

    // copy data
    QT3DS_FOREACH(idx, attribs.size())
    {
        entryRef[idx].m_AttribName = m_StringTable->RegisterStr(attribs.mData[idx].m_Name);
        entryRef[idx].m_Normalize = 0;
        entryRef[idx].m_AttribIndex = 0; // will be set later
        entryRef[idx].m_Type = m_Conversion.fromComponentTypeAndNumCompsToAttribGL(
            attribs.mData[idx].m_ComponentType, attribs.mData[idx].m_NumComponents);
        entryRef[idx].m_NumComponents = attribs.mData[idx].m_NumComponents;
        entryRef[idx].m_InputSlot = attribs.mData[idx].m_InputSlot;
        entryRef[idx].m_Offset = attribs.mData[idx].m_FirstItemOffset;

        if (maxInputSlot < entryRef[idx].m_InputSlot)
            maxInputSlot = entryRef[idx].m_InputSlot;
    }

    NVRenderBackendAttributeLayoutGL *retval =
        new (newMem) NVRenderBackendAttributeLayoutGL(entryRef, maxInputSlot);

    return (NVRenderBackend::NVRenderBackendAttribLayoutObject)retval;
}

void NVRenderBackendGLBase::ReleaseAttribLayout(NVRenderBackendAttribLayoutObject ao)
{
    NVRenderBackendAttributeLayoutGL *attribLayout = (NVRenderBackendAttributeLayoutGL *)ao;

    NVDelete(m_Foundation.getAllocator(), attribLayout);
};

NVRenderBackend::NVRenderBackendInputAssemblerObject
NVRenderBackendGLBase::CreateInputAssembler(NVRenderBackendAttribLayoutObject attribLayout,
                                            NVConstDataRef<NVRenderBackendBufferObject> buffers,
                                            const NVRenderBackendBufferObject indexBuffer,
                                            NVConstDataRef<QT3DSU32> strides,
                                            NVConstDataRef<QT3DSU32> offsets,
                                            QT3DSU32 patchVertexCount)
{
    NVRenderBackendAttributeLayoutGL *attribLayoutGL =
        (NVRenderBackendAttributeLayoutGL *)attribLayout;

    NVRenderBackendInputAssemblerGL *retval = QT3DS_NEW(m_Foundation.getAllocator(),
                                                     NVRenderBackendInputAssemblerGL)(
        m_Foundation, attribLayoutGL, buffers, indexBuffer, strides, offsets, patchVertexCount);

    return (NVRenderBackend::NVRenderBackendInputAssemblerObject)retval;
}

void NVRenderBackendGLBase::ReleaseInputAssembler(NVRenderBackendInputAssemblerObject iao)
{
    NVRenderBackendInputAssemblerGL *inputAssembler = (NVRenderBackendInputAssemblerGL *)iao;
    NVDelete(m_Foundation.getAllocator(), inputAssembler);
}

bool NVRenderBackendGLBase::compileSource(GLuint shaderID, NVConstDataRef<QT3DSI8> source,
                                          eastl::string &errorMessage, bool binary)
{
    GLint shaderSourceSize = static_cast<GLint>(source.size());
    const char *shaderSourceData = (const char *)source.begin();
    GLint shaderStatus = GL_TRUE;

    if (!binary) {

        GL_CALL_FUNCTION(glShaderSource(shaderID, 1, &shaderSourceData, &shaderSourceSize));
        GL_CALL_FUNCTION(glCompileShader(shaderID));

        GLint logLen;
        GL_CALL_FUNCTION(glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderStatus));
        GL_CALL_FUNCTION(glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLen));

        // Check if some log exists. We also write warnings here
        // Should at least contain more than the null termination
        if (logLen > 2) {
            errorMessage.resize(logLen + 1);

            GLint lenWithoutNull;
            GL_CALL_FUNCTION(glGetShaderInfoLog(shaderID, logLen, &lenWithoutNull,
                                          (char *)errorMessage.c_str()));
        }
    } else {
        GL_CALL_FUNCTION(glShaderBinary(1, &shaderID, GL_NVIDIA_PLATFORM_BINARY_NV, shaderSourceData,
                       shaderSourceSize));
        GLenum binaryError = m_glFunctions->glGetError();
        if (binaryError != GL_NO_ERROR) {
            shaderStatus = GL_FALSE;
            qCCritical(GL_ERROR) << GLConversion::processGLError(binaryError);
        }
    }

    return (shaderStatus == GL_TRUE);
}

NVRenderBackend::NVRenderBackendVertexShaderObject
NVRenderBackendGLBase::CreateVertexShader(NVConstDataRef<QT3DSI8> source,
                                          eastl::string &errorMessage, bool binary)
{
    GLuint shaderID = GL_CALL_FUNCTION(glCreateShader(GL_VERTEX_SHADER));

    if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
        GL_CALL_FUNCTION(glDeleteShader(shaderID));
        shaderID = 0;
    }

    return (NVRenderBackend::NVRenderBackendVertexShaderObject)shaderID;
}

NVRenderBackend::NVRenderBackendFragmentShaderObject
NVRenderBackendGLBase::CreateFragmentShader(NVConstDataRef<QT3DSI8> source,
                                            eastl::string &errorMessage, bool binary)
{
    GLuint shaderID = GL_CALL_FUNCTION(glCreateShader(GL_FRAGMENT_SHADER));

    if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
        GL_CALL_FUNCTION(glDeleteShader(shaderID));
        shaderID = 0;
    }

    return (NVRenderBackend::NVRenderBackendFragmentShaderObject)shaderID;
}

NVRenderBackend::NVRenderBackendTessControlShaderObject
NVRenderBackendGLBase::CreateTessControlShader(NVConstDataRef<QT3DSI8> source,
                                               eastl::string &errorMessage, bool binary)
{
    // needs GL 4 or GLES EXT_tessellation_shader support
    NVRENDER_BACKEND_UNUSED(source);
    NVRENDER_BACKEND_UNUSED(errorMessage);
    NVRENDER_BACKEND_UNUSED(binary);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return (NVRenderBackend::NVRenderBackendTessControlShaderObject)0;
}

NVRenderBackend::NVRenderBackendTessEvaluationShaderObject
NVRenderBackendGLBase::CreateTessEvaluationShader(NVConstDataRef<QT3DSI8> source,
                                                  eastl::string &errorMessage, bool binary)
{
    // needs GL 4 or GLES EXT_tessellation_shader support
    NVRENDER_BACKEND_UNUSED(source);
    NVRENDER_BACKEND_UNUSED(errorMessage);
    NVRENDER_BACKEND_UNUSED(binary);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return (NVRenderBackend::NVRenderBackendTessEvaluationShaderObject)0;
}

NVRenderBackend::NVRenderBackendGeometryShaderObject
NVRenderBackendGLBase::CreateGeometryShader(NVConstDataRef<QT3DSI8> source,
                                            eastl::string &errorMessage, bool binary)
{
    // needs GL 4 or GLES EXT_geometry_shader support
    NVRENDER_BACKEND_UNUSED(source);
    NVRENDER_BACKEND_UNUSED(errorMessage);
    NVRENDER_BACKEND_UNUSED(binary);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return (NVRenderBackend::NVRenderBackendGeometryShaderObject)0;
}

NVRenderBackend::NVRenderBackendComputeShaderObject
NVRenderBackendGLBase::CreateComputeShader(NVConstDataRef<QT3DSI8> source,
                                           eastl::string &errorMessage, bool binary)
{
    // needs GL 4.3 or GLES3.1 support
    NVRENDER_BACKEND_UNUSED(source);
    NVRENDER_BACKEND_UNUSED(errorMessage);
    NVRENDER_BACKEND_UNUSED(binary);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return (NVRenderBackend::NVRenderBackendComputeShaderObject)0;
}

void NVRenderBackendGLBase::ReleaseVertexShader(NVRenderBackendVertexShaderObject vso)
{
    GLuint shaderID = HandleToID_cast(GLuint, size_t, vso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void NVRenderBackendGLBase::ReleaseFragmentShader(NVRenderBackendFragmentShaderObject fso)
{
    GLuint shaderID = HandleToID_cast(GLuint, size_t, fso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void
NVRenderBackendGLBase::ReleaseTessControlShader(NVRenderBackendTessControlShaderObject tcso)
{
    GLuint shaderID = HandleToID_cast(GLuint, size_t, tcso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void NVRenderBackendGLBase::ReleaseTessEvaluationShader(
    NVRenderBackendTessEvaluationShaderObject teso)
{
    GLuint shaderID = HandleToID_cast(GLuint, size_t, teso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void NVRenderBackendGLBase::ReleaseGeometryShader(NVRenderBackendGeometryShaderObject gso)
{
    GLuint shaderID = HandleToID_cast(GLuint, size_t, gso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void NVRenderBackendGLBase::ReleaseComputeShader(NVRenderBackendComputeShaderObject cso)
{
    GLuint shaderID = HandleToID_cast(GLuint, size_t, cso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void NVRenderBackendGLBase::AttachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendVertexShaderObject vso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, vso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::AttachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendFragmentShaderObject fso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, fso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::AttachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendTessControlShaderObject tcso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, tcso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::AttachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendTessEvaluationShaderObject teso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, teso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::AttachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendGeometryShaderObject gso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, gso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::AttachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendComputeShaderObject cso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, cso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::DetachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendVertexShaderObject vso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, vso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::DetachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendFragmentShaderObject fso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, fso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::DetachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendTessControlShaderObject tcso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, tcso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::DetachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendTessEvaluationShaderObject teso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, teso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::DetachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendGeometryShaderObject gso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, gso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

void NVRenderBackendGLBase::DetachShader(NVRenderBackendShaderProgramObject po,
                                         NVRenderBackendComputeShaderObject cso)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint shaderID = HandleToID_cast(GLuint, size_t, cso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_ProgramID), shaderID));
}

NVRenderBackend::NVRenderBackendShaderProgramObject
NVRenderBackendGLBase::CreateShaderProgram(bool isSeparable)
{
    NVRenderBackendShaderProgramGL *theProgram = nullptr;
    GLuint programID = GL_CALL_FUNCTION(glCreateProgram());

    if (programID) {
        theProgram =
            QT3DS_NEW(m_Foundation.getAllocator(), NVRenderBackendShaderProgramGL)(programID);

        if (!theProgram) {
            GL_CALL_FUNCTION(glDeleteProgram(programID));
        } else if (isSeparable && m_backendSupport.caps.bits.bProgramPipelineSupported) {
            GL_CALL_EXTRA_FUNCTION(glProgramParameteri(programID, GL_PROGRAM_SEPARABLE, GL_TRUE));
        }
    }

    return (NVRenderBackend::NVRenderBackendShaderProgramObject)theProgram;
}

void NVRenderBackendGLBase::ReleaseShaderProgram(NVRenderBackendShaderProgramObject po)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

    GL_CALL_FUNCTION(glDeleteProgram(programID));

    if (pProgram->m_shaderInput) {
        NVDelete(m_Foundation.getAllocator(), pProgram->m_shaderInput);
        pProgram->m_shaderInput = nullptr;
    }

    NVDelete(m_Foundation.getAllocator(), pProgram);
}

bool NVRenderBackendGLBase::linkProgram(NVRenderBackendShaderProgramObject po,
                                        eastl::string &errorMessage,
                                        QT3DSU32 binaryFormat, const QByteArray *binary)
{
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

    if (binary) {
        GL_CALL_EXTRA_FUNCTION(glProgramBinary(programID, GLenum(binaryFormat), binary->constData(),
                                               binary->size()));
    } else {
        GL_CALL_FUNCTION(glLinkProgram(programID));
    }

    // TODO: Move to a another function to be also used after binary link?
    GLint linkStatus, logLen;
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus));
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLen));

    // if successfully linked get the attribute information
    if (linkStatus) {
        // release old stuff
        if (pProgram->m_shaderInput) {
            NVDelete(m_Foundation.getAllocator(), pProgram->m_shaderInput);
            pProgram->m_shaderInput = nullptr;
        }

        GLint numAttribs;
        GL_CALL_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_ATTRIBUTES, &numAttribs));

        if (numAttribs) {
            NVRenderBackendShaderInputEntryGL *tempShaderInputEntry =
                (NVRenderBackendShaderInputEntryGL *)QT3DS_ALLOC(
                    m_Foundation.getAllocator(),
                    sizeof(NVRenderBackendShaderInputEntryGL) * m_MaxAttribCount,
                    "BackendShaderInputEntryGL");

            GLint maxLength;
            GL_CALL_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength));
            QT3DSI8 *nameBuf =
                (QT3DSI8 *)QT3DS_ALLOC(m_Foundation.getAllocator(), maxLength, "linkProgram");

            // fill in data
            QT3DSU32 count = 0;
            QT3DS_FOREACH(idx, numAttribs)
            {
                GLint size = 0;
                GLenum glType;
                NVRenderComponentTypes::Enum compType = NVRenderComponentTypes::Unknown;
                QT3DSU32 numComps = 0;

                GL_CALL_FUNCTION(glGetActiveAttrib(programID, idx, maxLength, nullptr, &size,
                                                   &glType, (char *)nameBuf));
                // Skip anything named with gl_
                if (memcmp(nameBuf, "gl_", 3) == 0)
                    continue;

                m_Conversion.fromAttribGLToComponentTypeAndNumComps(glType, compType, numComps);

                tempShaderInputEntry[count].m_AttribName =
                    m_StringTable->RegisterStr((char *)nameBuf);
                tempShaderInputEntry[count].m_AttribLocation =
                    GL_CALL_FUNCTION(glGetAttribLocation(programID, (char *)nameBuf));
                tempShaderInputEntry[count].m_Type = glType;
                tempShaderInputEntry[count].m_NumComponents = numComps;

                ++count;
            }

            // Now allocate space for the actuall entries
            QT3DSU32 shaderInputSize = sizeof(NVRenderBackendShaderInputGL);
            QT3DSU32 entrySize = sizeof(NVRenderBackendShaderInputEntryGL) * count;
            QT3DSU8 *newMem =
                (QT3DSU8 *)QT3DS_ALLOC(m_Foundation.getAllocator(), shaderInputSize + entrySize,
                                 "BackendShaderInputEntryGL");
            NVDataRef<NVRenderBackendShaderInputEntryGL> entryRef =
                PtrAtOffset<NVRenderBackendShaderInputEntryGL>(newMem, shaderInputSize,
                                                               entrySize);
            // fill data
            QT3DS_FOREACH(idx, count)
            {
                entryRef[idx].m_AttribName = tempShaderInputEntry[idx].m_AttribName;
                entryRef[idx].m_AttribLocation = tempShaderInputEntry[idx].m_AttribLocation;
                entryRef[idx].m_Type = tempShaderInputEntry[idx].m_Type;
                entryRef[idx].m_NumComponents = tempShaderInputEntry[idx].m_NumComponents;
            }

            // placement new
            NVRenderBackendShaderInputGL *shaderInput =
                new (newMem) NVRenderBackendShaderInputGL(entryRef);
            // set the pointer
            pProgram->m_shaderInput = shaderInput;

            QT3DS_FREE(m_Foundation.getAllocator(), nameBuf);
            QT3DS_FREE(m_Foundation.getAllocator(), tempShaderInputEntry);
        }
    }

    // Check if some log exists. We also write warnings here
    // Should at least contain more than the null termination
    if (logLen > 2) {
        errorMessage.resize(logLen + 1);

        GLint lenWithoutNull;
        GL_CALL_FUNCTION(glGetProgramInfoLog(programID, logLen, &lenWithoutNull,
                                       (char *)errorMessage.c_str()));
    }

    return (linkStatus == GL_TRUE);
}

void NVRenderBackendGLBase::SetActiveProgram(NVRenderBackendShaderProgramObject po)
{
    GLuint programID = 0;

    if (po) {
        NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
        programID = static_cast<GLuint>(pProgram->m_ProgramID);
    }

    GL_CALL_FUNCTION(glUseProgram(programID));
}

NVRenderBackend::NVRenderBackendProgramPipeline NVRenderBackendGLBase::CreateProgramPipeline()
{
    // needs GL 4 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
    return NVRenderBackend::NVRenderBackendProgramPipeline(0);
}

void NVRenderBackendGLBase::ReleaseProgramPipeline(NVRenderBackendProgramPipeline)
{
    // needs GL 4 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::SetActiveProgramPipeline(NVRenderBackendProgramPipeline)
{
    // needs GL 4 context
    //TODO: should be fixed?
    //        QT3DS_ASSERT(false);
}

void NVRenderBackendGLBase::SetProgramStages(NVRenderBackendProgramPipeline,
                                             NVRenderShaderTypeFlags,
                                             NVRenderBackendShaderProgramObject)
{
    // needs GL 4 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::DispatchCompute(NVRenderBackendShaderProgramObject, QT3DSU32, QT3DSU32,
                                            QT3DSU32)
{
    // needs GL 4 context
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

QT3DSI32 NVRenderBackendGLBase::GetConstantCount(NVRenderBackendShaderProgramObject po)
{
    QT3DS_ASSERT(po);
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

    GLint numUniforms;
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_UNIFORMS, &numUniforms));

    return numUniforms;
}

QT3DSI32 NVRenderBackendGLBase::GetConstantBufferCount(NVRenderBackendShaderProgramObject po)
{
    // needs GL3 and above
    NVRENDER_BACKEND_UNUSED(po);

    return 0;
}

QT3DSI32
NVRenderBackendGLBase::GetConstantInfoByID(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                           QT3DSU32 bufSize, QT3DSI32 *numElem,
                                           NVRenderShaderDataTypes::Enum *type, QT3DSI32 *binding,
                                           char *nameBuf)
{
    QT3DS_ASSERT(po);
    NVRenderBackendShaderProgramGL *pProgram = (NVRenderBackendShaderProgramGL *)po;
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);

    GLenum glType;
    GL_CALL_FUNCTION(glGetActiveUniform(programID, id, bufSize, nullptr, numElem, &glType, nameBuf));
    *type = m_Conversion.fromShaderGLToPropertyDataTypes(glType);

    QT3DSI32 uniformLoc = GL_CALL_FUNCTION(glGetUniformLocation(programID, nameBuf));

    // get unit binding point
    *binding = -1;
    if (uniformLoc != -1 && (glType == GL_IMAGE_2D || glType == GL_UNSIGNED_INT_IMAGE_2D
                             || glType == GL_UNSIGNED_INT_ATOMIC_COUNTER)) {
        GL_CALL_FUNCTION(glGetUniformiv(programID, uniformLoc, binding));
    }

    return uniformLoc;
}

QT3DSI32
NVRenderBackendGLBase::GetConstantBufferInfoByID(NVRenderBackendShaderProgramObject po,
                                                 QT3DSU32 id, QT3DSU32 nameBufSize,
                                                 QT3DSI32 *paramCount,
                                                 QT3DSI32 *bufferSize, QT3DSI32 *length,
                                                 char *nameBuf)
{
    // needs GL3 and above
    NVRENDER_BACKEND_UNUSED(po);
    NVRENDER_BACKEND_UNUSED(id);
    NVRENDER_BACKEND_UNUSED(nameBufSize);
    NVRENDER_BACKEND_UNUSED(paramCount);
    NVRENDER_BACKEND_UNUSED(bufferSize);
    NVRENDER_BACKEND_UNUSED(length);
    NVRENDER_BACKEND_UNUSED(nameBuf);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return -1;
}

void NVRenderBackendGLBase::GetConstantBufferParamIndices(NVRenderBackendShaderProgramObject po,
                                                          QT3DSU32 id, QT3DSI32 *indices)
{
    // needs GL3 and above
    NVRENDER_BACKEND_UNUSED(po);
    NVRENDER_BACKEND_UNUSED(id);
    NVRENDER_BACKEND_UNUSED(indices);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::GetConstantBufferParamInfoByIndices(
    NVRenderBackendShaderProgramObject po, QT3DSU32 count, QT3DSU32 *indices, QT3DSI32 *type,
    QT3DSI32 *size, QT3DSI32 *offset)
{
    // needs GL3 and above
    NVRENDER_BACKEND_UNUSED(po);
    NVRENDER_BACKEND_UNUSED(count);
    NVRENDER_BACKEND_UNUSED(indices);
    NVRENDER_BACKEND_UNUSED(type);
    NVRENDER_BACKEND_UNUSED(size);
    NVRENDER_BACKEND_UNUSED(offset);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::ProgramSetConstantBlock(NVRenderBackendShaderProgramObject po,
                                                    QT3DSU32 blockIndex, QT3DSU32 binding)
{
    // needs GL3 and above
    NVRENDER_BACKEND_UNUSED(po);
    NVRENDER_BACKEND_UNUSED(blockIndex);
    NVRENDER_BACKEND_UNUSED(binding);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::ProgramSetConstantBuffer(QT3DSU32 index,
                                                     NVRenderBackendBufferObject bo)
{
    // needs GL3 and above
    NVRENDER_BACKEND_UNUSED(index);
    NVRENDER_BACKEND_UNUSED(bo);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

QT3DSI32 NVRenderBackendGLBase::GetStorageBufferCount(NVRenderBackendShaderProgramObject po)
{
    // needs GL4 and above
    NVRENDER_BACKEND_UNUSED(po);

    return 0;
}

QT3DSI32
NVRenderBackendGLBase::GetStorageBufferInfoByID(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                                QT3DSU32 nameBufSize, QT3DSI32 *paramCount,
                                                QT3DSI32 *bufferSize, QT3DSI32 *length, char *nameBuf)
{
    // needs GL4 and above
    NVRENDER_BACKEND_UNUSED(po);
    NVRENDER_BACKEND_UNUSED(id);
    NVRENDER_BACKEND_UNUSED(nameBufSize);
    NVRENDER_BACKEND_UNUSED(paramCount);
    NVRENDER_BACKEND_UNUSED(bufferSize);
    NVRENDER_BACKEND_UNUSED(length);
    NVRENDER_BACKEND_UNUSED(nameBuf);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return -1;
}

void NVRenderBackendGLBase::ProgramSetStorageBuffer(QT3DSU32 index, NVRenderBackendBufferObject bo)
{
    // needs GL4 and above
    NVRENDER_BACKEND_UNUSED(index);
    NVRENDER_BACKEND_UNUSED(bo);
}

QT3DSI32 NVRenderBackendGLBase::GetAtomicCounterBufferCount(NVRenderBackendShaderProgramObject po)
{
    // needs GL4 and above
    NVRENDER_BACKEND_UNUSED(po);

    return 0;
}

QT3DSI32
NVRenderBackendGLBase::GetAtomicCounterBufferInfoByID(NVRenderBackendShaderProgramObject po,
                                                      QT3DSU32 id, QT3DSU32 nameBufSize,
                                                      QT3DSI32 *paramCount, QT3DSI32 *bufferSize,
                                                      QT3DSI32 *length, char *nameBuf)
{
    // needs GL4 and above
    NVRENDER_BACKEND_UNUSED(po);
    NVRENDER_BACKEND_UNUSED(id);
    NVRENDER_BACKEND_UNUSED(nameBufSize);
    NVRENDER_BACKEND_UNUSED(paramCount);
    NVRENDER_BACKEND_UNUSED(bufferSize);
    NVRENDER_BACKEND_UNUSED(length);
    NVRENDER_BACKEND_UNUSED(nameBuf);

    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return -1;
}

void NVRenderBackendGLBase::ProgramSetAtomicCounterBuffer(QT3DSU32 index,
                                                          NVRenderBackendBufferObject bo)
{
    // needs GL4 and above
    NVRENDER_BACKEND_UNUSED(index);
    NVRENDER_BACKEND_UNUSED(bo);
}

void NVRenderBackendGLBase::SetConstantValue(NVRenderBackendShaderProgramObject, QT3DSU32 id,
                                             NVRenderShaderDataTypes::Enum type, QT3DSI32 count,
                                             const void *value, bool transpose)
{
    GLenum glType = m_Conversion.fromPropertyDataTypesToShaderGL(type);

    switch (glType) {
    case GL_FLOAT:
        GL_CALL_FUNCTION(glUniform1fv(id, count, (GLfloat *)value));
        break;
    case GL_FLOAT_VEC2:
        GL_CALL_FUNCTION(glUniform2fv(id, count, (GLfloat *)value));
        break;
    case GL_FLOAT_VEC3:
        GL_CALL_FUNCTION(glUniform3fv(id, count, (GLfloat *)value));
        break;
    case GL_FLOAT_VEC4:
        GL_CALL_FUNCTION(glUniform4fv(id, count, (GLfloat *)value));
        break;
    case GL_INT:
        GL_CALL_FUNCTION(glUniform1iv(id, count, (GLint *)value));
        break;
    case GL_BOOL:
        {
            GLint boolValue = *(GLboolean *)value;
            GL_CALL_FUNCTION(glUniform1iv(id, count, &boolValue));
        }
        break;
    case GL_INT_VEC2:
    case GL_BOOL_VEC2:
        GL_CALL_FUNCTION(glUniform2iv(id, count, (GLint *)value));
        break;
    case GL_INT_VEC3:
    case GL_BOOL_VEC3:
        GL_CALL_FUNCTION(glUniform3iv(id, count, (GLint *)value));
        break;
    case GL_INT_VEC4:
    case GL_BOOL_VEC4:
        GL_CALL_FUNCTION(glUniform4iv(id, count, (GLint *)value));
        break;
    case GL_FLOAT_MAT3:
        GL_CALL_FUNCTION(glUniformMatrix3fv(id, count, transpose, (GLfloat *)value));
        break;
    case GL_FLOAT_MAT4:
        GL_CALL_FUNCTION(glUniformMatrix4fv(id, count, transpose, (GLfloat *)value));
        break;
    case GL_IMAGE_2D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE: {
        if (count > 1) {
            GLint *sampler = (GLint *)value;
            GL_CALL_FUNCTION(glUniform1iv(id, count, sampler));
        } else {
            GLint sampler = *(GLint *)value;
            GL_CALL_FUNCTION(glUniform1i(id, sampler));
        }
    } break;
    default:
        qCCritical(INTERNAL_ERROR, "Unknown shader type format %d", type);
        QT3DS_ASSERT(false);
        break;
    }
}

void NVRenderBackendGLBase::Draw(NVRenderDrawMode::Enum drawMode, QT3DSU32 start, QT3DSU32 count)
{
    GL_CALL_FUNCTION(glDrawArrays(m_Conversion.fromDrawModeToGL(
                                drawMode, m_backendSupport.caps.bits.bTessellationSupported),
                            start, count));
}

void NVRenderBackendGLBase::DrawIndirect(NVRenderDrawMode::Enum drawMode, const void *indirect)
{
    // needs GL4 and above
    NVRENDER_BACKEND_UNUSED(drawMode);
    NVRENDER_BACKEND_UNUSED(indirect);
}

void NVRenderBackendGLBase::DrawIndexed(NVRenderDrawMode::Enum drawMode, QT3DSU32 count,
                                        NVRenderComponentTypes::Enum type, const void *indices)
{
    GL_CALL_FUNCTION(glDrawElements(m_Conversion.fromDrawModeToGL(
                                  drawMode, m_backendSupport.caps.bits.bTessellationSupported),
                              count, m_Conversion.fromIndexBufferComponentsTypesToGL(type),
                              indices));
}

void NVRenderBackendGLBase::DrawIndexedIndirect(NVRenderDrawMode::Enum drawMode,
                                                NVRenderComponentTypes::Enum type,
                                                const void *indirect)
{
    // needs GL4 and above
    NVRENDER_BACKEND_UNUSED(drawMode);
    NVRENDER_BACKEND_UNUSED(type);
    NVRENDER_BACKEND_UNUSED(indirect);
}

void NVRenderBackendGLBase::ReadPixel(NVRenderBackendRenderTargetObject /* rto */, QT3DSI32 x,
                                      QT3DSI32 y, QT3DSI32 width, QT3DSI32 height,
                                      NVRenderReadPixelFormats::Enum inFormat, void *pixels)
{
    GLuint glFormat;
    GLuint glType;
    if (m_Conversion.fromReadPixelsToGlFormatAndType(inFormat, &glFormat, &glType)) {
        GL_CALL_FUNCTION(glReadPixels(x, y, width, height, glFormat, glType, pixels));
    }
}

NVRenderBackend::NVRenderBackendPathObject NVRenderBackendGLBase::CreatePathNVObject(size_t)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return NVRenderBackend::NVRenderBackendPathObject(0);
}

void NVRenderBackendGLBase::ReleasePathNVObject(NVRenderBackendPathObject, size_t)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::LoadPathGlyphs(NVRenderBackendPathObject,
                                           NVRenderPathFontTarget::Enum, const void *,
                                           NVRenderPathFontStyleFlags, size_t,
                                           NVRenderPathFormatType::Enum, const void *,
                                           NVRenderPathMissingGlyphs::Enum,
                                           NVRenderBackendPathObject, QT3DSF32)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::LoadPathGlyphRange(NVRenderBackendPathObject,
                                               NVRenderPathFontTarget::Enum, const void *,
                                               NVRenderPathFontStyleFlags, QT3DSU32, size_t,
                                               NVRenderPathMissingGlyphs::Enum,
                                               NVRenderBackendPathObject, QT3DSF32)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

NVRenderPathReturnValues::Enum NVRenderBackendGLBase::LoadPathGlyphsIndexed(
    NVRenderBackendPathObject, NVRenderPathFontTarget::Enum, const void *,
    NVRenderPathFontStyleFlags, QT3DSU32, size_t, NVRenderBackendPathObject, QT3DSF32)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return NVRenderPathReturnValues::FontUnavailable;
}

NVRenderBackend::NVRenderBackendPathObject NVRenderBackendGLBase::LoadPathGlyphsIndexedRange(
    NVRenderPathFontTarget::Enum, const void *, NVRenderPathFontStyleFlags,
    NVRenderBackend::NVRenderBackendPathObject, QT3DSF32, QT3DSU32 *)
{
    return NVRenderBackendPathObject(0);
}

void NVRenderBackendGLBase::GetPathMetrics(NVRenderBackendPathObject, size_t,
                                           NVRenderPathGlyphFontMetricFlags,
                                           NVRenderPathFormatType::Enum, const void *, size_t,
                                           QT3DSF32 *)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::GetPathMetricsRange(NVRenderBackendPathObject, size_t,
                                                NVRenderPathGlyphFontMetricFlags, size_t,
                                                QT3DSF32 *)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::GetPathSpacing(NVRenderBackendPathObject, size_t,
                                           NVRenderPathListMode::Enum,
                                           NVRenderPathFormatType::Enum, const void *, QT3DSF32,
                                           QT3DSF32, NVRenderPathTransformType::Enum, QT3DSF32 *)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::StencilFillPathInstanced(NVRenderBackendPathObject, size_t,
                                                     NVRenderPathFormatType::Enum, const void *,
                                                     NVRenderPathFillMode::Enum, QT3DSU32,
                                                     NVRenderPathTransformType::Enum,
                                                     const QT3DSF32 *)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::StencilStrokePathInstancedN(NVRenderBackendPathObject, size_t,
                                                        NVRenderPathFormatType::Enum,
                                                        const void *, QT3DSI32, QT3DSU32,
                                                        NVRenderPathTransformType::Enum,
                                                        const QT3DSF32 *)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::CoverFillPathInstanced(NVRenderBackendPathObject, size_t,
                                                   NVRenderPathFormatType::Enum, const void *,
                                                   NVRenderPathCoverMode::Enum,
                                                   NVRenderPathTransformType::Enum,
                                                   const QT3DSF32 *)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::CoverStrokePathInstanced(NVRenderBackendPathObject, size_t,
                                                     NVRenderPathFormatType::Enum, const void *,
                                                     NVRenderPathCoverMode::Enum,
                                                     NVRenderPathTransformType::Enum,
                                                     const QT3DSF32 *)
{
    // Needs GL 4 backend
    qCCritical(INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void NVRenderBackendGLBase::getProgramBinary(NVRenderBackend::NVRenderBackendShaderProgramObject po,
                                             QT3DSU32 &outFormat, QByteArray &outBinary)
{
    NVRenderBackendShaderProgramGL *pProgram
            = reinterpret_cast<NVRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_ProgramID);
    GLint binLen = 0;

    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_PROGRAM_BINARY_LENGTH, &binLen));

    outBinary.resize(binLen);

    GL_CALL_EXTRA_FUNCTION(glGetProgramBinary(programID, binLen, nullptr, &outFormat,
                                              outBinary.data()));
}

///< private calls
const char *NVRenderBackendGLBase::getVersionString()
{
    const char *retval = (const char *)GL_CALL_FUNCTION(glGetString(GL_VERSION));
    if (retval == nullptr)
        return "";

    return retval;
}

const char *NVRenderBackendGLBase::getVendorString()
{
    const char *retval = (const char *)GL_CALL_FUNCTION(glGetString(GL_VENDOR));
    if (retval == nullptr)
        return "";

    return retval;
}

const char *NVRenderBackendGLBase::getRendererString()
{
    const char *retval = (const char *)GL_CALL_FUNCTION(glGetString(GL_RENDERER));
    if (retval == nullptr)
        return "";

    return retval;
}

const char *NVRenderBackendGLBase::getExtensionString()
{
    const char *retval = (const char *)GL_CALL_FUNCTION(glGetString(GL_EXTENSIONS));
    if (retval == nullptr)
        return "";

    return retval;
}

/**
 * @brief This function inspects the various strings to setup
 *        HW capabilities of the device.
 *        We can do a lot of smart things here based on GL version
 *        renderer string and vendor.
 *
 * @return No return
 */
void NVRenderBackendGLBase::setAndInspectHardwareCaps()
{
    eastl::string apiVersion(getVersionString());
    qCInfo(TRACE_INFO, "GL version: %s", apiVersion.c_str());

    // we assume all GLES versions running on mobile with shared memory
    // this means framebuffer blits are slow and should be optimized or avoided
    if (apiVersion.find("OpenGL ES") == eastl::string::npos) {
        // no ES device
        m_backendSupport.caps.bits.bFastBlitsSupported = true;
    }
}

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
void NVRenderBackendGLBase::checkGLError(const char *function, const char *file,
                                         const unsigned int line) const
{
    GLenum error = m_glFunctions->glGetError();
    if (error != GL_NO_ERROR) {
        qCCritical(GL_ERROR) << GLConversion::processGLError(error) << " "
                             << function << " " << file << " " << line;
    }
}
#else
void NVRenderBackendGLBase::checkGLError() const
{
#if !defined(NDEBUG) || defined(_DEBUG)
    GLenum error = m_glFunctions->glGetError();
    if (error != GL_NO_ERROR)
        qCCritical(GL_ERROR) << GLConversion::processGLError(error);
#endif
}
#endif

}
}
