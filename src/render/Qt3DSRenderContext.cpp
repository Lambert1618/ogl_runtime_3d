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

#include "foundation/Qt3DSMat44.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Utils.h"
#include "EASTL/set.h"
#include "EASTL/utility.h"
#include "render/Qt3DSRenderShaderProgram.h"

using namespace qt3ds;
using namespace qt3ds::render;
using namespace eastl;

namespace qt3ds {
namespace render {

    NVRenderContextImpl::NVRenderContextImpl(NVFoundationBase &fnd, NVRenderBackend &inBackend,
                                             IStringTable &inStrTable)
        : m_backend(inBackend)
        , m_DirtyFlags(0)
        , m_DefaultOffscreenRenderTarget((NVRenderBackend::NVRenderBackendRenderTargetObject)NULL)
        , m_DephBits(16)
        , m_StencilBits(8)
        , mRefCount(0)
        , m_Foundation(fnd)
        , m_StringTable(inStrTable)
        , m_VertToImpMap(m_Foundation.getAllocator(), "NVRenderContextImpl::m_VertToImpMap")
        , m_IndexToImpMap(m_Foundation.getAllocator(), "NVRenderContextImpl::m_IndexToImpMap")
        , m_ConstantToImpMap(m_Foundation.getAllocator(), "NVRenderContextImpl::m_ConstantToImpMap")
        , m_StorageToImpMap(m_Foundation.getAllocator(), "NVRenderContextImpl::m_StorageToImpMap")
        , m_AtomicCounterToImpMap(m_Foundation.getAllocator(),
                                  "NVRenderContextImpl::m_AtomicCounterToImpMap")
        , m_DrawIndirectToImpMap(m_Foundation.getAllocator(),
                                 "NVRenderContextImpl::m_DrawIndirectToImpMap")
        , m_DepthStencilStateToImpMap(m_Foundation.getAllocator(),
                                      "NVRenderContextImpl::m_DepthStencilStateToImpMap")
        , m_RasterizerStateToImpMap(m_Foundation.getAllocator(),
                                    "NVRenderContextImpl::m_RasterizerStateToImpMap")
        , m_PathFontSpecToImpMap(m_Foundation.getAllocator(),
                                 "NVRenderContextImpl::m_RasterizerStateToImpMap")
        , m_Tex2DToImpMap(m_Foundation.getAllocator(), "NVRenderContextImpl::m_Tex2DToImpMap")
        , m_Tex2DArrayToImpMap(m_Foundation.getAllocator(),
                               "NVRenderContextImpl::m_Tex2DArrayToImpMap")
        , m_TexCubeToImpMap(m_Foundation.getAllocator(), "NVRenderContextImpl::m_TexCubeToImpMap")
        , m_Image2DtoImpMap(m_Foundation.getAllocator(), "NVRenderContextImpl::m_Image2DtoImpMap")
        , m_ShaderToImpMap(m_Foundation.getAllocator(), "NVRenderContextImpl::m_ShaderToImpMap")
        , m_RenderBufferToImpMap(m_Foundation.getAllocator(),
                                 "NVRenderContextImpl::m_RenderBufferToImpMap")
        , m_FrameBufferToImpMap(m_Foundation.getAllocator(),
                                "NVRenderContextImpl::m_FrameBufferToImpMap")
        , m_NextTextureUnit(1)
        , m_NextConstantBufferUnit(1)
        , m_PropertyStack(m_Foundation.getAllocator(), "NVRenderContextImpl::m_PropertyStack")
    {

        m_StringTable.addRef();

        m_MaxTextureUnits = m_backend->GetMaxCombinedTextureUnits();
        m_MaxConstantBufferUnits = 16; // need backend query

        // get initial state
        memZero(&m_HardwarePropertyContext, sizeof(m_HardwarePropertyContext));

        // get default blending functions
        m_backend->GetBlendFunc(&m_HardwarePropertyContext.m_BlendFunction);
        // set default blend euqation
        m_HardwarePropertyContext.m_BlendEquation.m_RGBEquation = NVRenderBlendEquation::Add;
        m_HardwarePropertyContext.m_BlendEquation.m_AlphaEquation = NVRenderBlendEquation::Add;
        // default state
        m_HardwarePropertyContext.m_CullingEnabled =
            m_backend->GetRenderState(NVRenderState::CullFace);
        m_HardwarePropertyContext.m_DepthFunction = m_backend->GetDepthFunc();
        m_HardwarePropertyContext.m_BlendingEnabled =
            m_backend->GetRenderState(NVRenderState::Blend);
        m_HardwarePropertyContext.m_DepthWriteEnabled = m_backend->GetDepthWrite();
        m_HardwarePropertyContext.m_DepthTestEnabled =
            m_backend->GetRenderState(NVRenderState::DepthTest);
        m_HardwarePropertyContext.m_ScissorTestEnabled =
            m_backend->GetRenderState(NVRenderState::ScissorTest);
        m_backend->GetScissorRect(&m_HardwarePropertyContext.m_ScissorRect);
        m_backend->GetViewportRect(&m_HardwarePropertyContext.m_Viewport);

        DoSetClearColor(m_HardwarePropertyContext.m_ClearColor);
    }

    NVRenderContextImpl::~NVRenderContextImpl()
    {
        m_StringTable.release();
        QT3DS_ASSERT(m_VertToImpMap.size() == 0);
        m_VertToImpMap.clear();
        QT3DS_ASSERT(m_IndexToImpMap.size() == 0);
        m_IndexToImpMap.clear();
        QT3DS_ASSERT(m_ConstantToImpMap.size() == 0);
        m_ConstantToImpMap.clear();
        QT3DS_ASSERT(m_StorageToImpMap.size() == 0);
        m_StorageToImpMap.clear();
        QT3DS_ASSERT(m_DepthStencilStateToImpMap.size() == 0);
        m_DepthStencilStateToImpMap.clear();
        QT3DS_ASSERT(m_RasterizerStateToImpMap.size() == 0);
        m_RasterizerStateToImpMap.clear();
        QT3DS_ASSERT(m_PathFontSpecToImpMap.size() == 0);
        m_PathFontSpecToImpMap.clear();
        QT3DS_ASSERT(m_Tex2DToImpMap.size() == 0);
        m_Tex2DToImpMap.clear();
        QT3DS_ASSERT(m_Tex2DArrayToImpMap.size() == 0);
        m_Tex2DArrayToImpMap.clear();
        QT3DS_ASSERT(m_Image2DtoImpMap.size() == 0);
        m_Image2DtoImpMap.clear();
        QT3DS_ASSERT(m_ShaderToImpMap.size() == 0);
        m_ShaderToImpMap.clear();
        QT3DS_ASSERT(m_RenderBufferToImpMap.size() == 0);
        m_RenderBufferToImpMap.clear();
        QT3DS_ASSERT(m_FrameBufferToImpMap.size() == 0);
        m_FrameBufferToImpMap.clear();

        m_backend = NULL;
    }

    void NVRenderContextImpl::getMaxTextureSize(QT3DSU32 &oWidth, QT3DSU32 &oHeight)
    {
        QT3DSI32 theMaxTextureSize = 0;
        m_backend->GetRenderBackendValue(NVRenderBackend::NVRenderBackendQuery::MaxTextureSize,
                                         &theMaxTextureSize);

        oWidth = (QT3DSU32)theMaxTextureSize;
        oHeight = (QT3DSU32)theMaxTextureSize;
    }

    NVRenderDepthStencilState *NVRenderContextImpl::CreateDepthStencilState(
        bool enableDepth, bool depthMask, NVRenderBoolOp::Enum depthFunc, bool enableStencil,
        NVRenderStencilFunctionArgument &stencilFuncFront,
        NVRenderStencilFunctionArgument &stencilFuncBack,
        NVRenderStencilOperationArgument &depthStencilOpFront,
        NVRenderStencilOperationArgument &depthStencilOpBack)
    {
        NVRenderDepthStencilState *state = NVRenderDepthStencilState::Create(
            *this, enableDepth, depthMask, depthFunc, enableStencil, stencilFuncFront,
            stencilFuncBack, depthStencilOpFront, depthStencilOpBack);
        if (state)
            m_DepthStencilStateToImpMap.insert(
                make_pair(state->GetDepthStencilObjectHandle(), state));

        return state;
    }

    void NVRenderContextImpl::SetDepthStencilState(NVRenderDepthStencilState *inDepthStencilState)
    {
        if (inDepthStencilState) {
            m_backend->SetDepthStencilState(inDepthStencilState->GetDepthStencilObjectHandle());
            // currently we have a mixture therefore we need to update the context state
            SetDepthFunction(inDepthStencilState->GetDepthFunc());
            SetDepthWriteEnabled(inDepthStencilState->GetDepthMask());
            SetDepthTestEnabled(inDepthStencilState->GetDepthEnabled());
            SetStencilTestEnabled(inDepthStencilState->GetStencilEnabled());
        }
    }

    void NVRenderContextImpl::StateDestroyed(NVRenderDepthStencilState &state)
    {
        m_DepthStencilStateToImpMap.erase(state.GetDepthStencilObjectHandle());
    }

    NVRenderRasterizerState *
    NVRenderContextImpl::CreateRasterizerState(QT3DSF32 depthBias, QT3DSF32 depthScale,
                                               NVRenderFaces::Enum cullFace)
    {
        NVRenderRasterizerState *state =
            NVRenderRasterizerState::Create(*this, depthBias, depthScale, cullFace);
        if (state)
            m_RasterizerStateToImpMap.insert(make_pair(state->GetRasterizerObjectHandle(), state));

        return state;
    }

    void NVRenderContextImpl::SetRasterizerState(NVRenderRasterizerState *inRasterizerState)
    {
        if (inRasterizerState)
            m_backend->SetRasterizerState(inRasterizerState->GetRasterizerObjectHandle());
    }

    void NVRenderContextImpl::StateDestroyed(NVRenderRasterizerState &state)
    {
        m_RasterizerStateToImpMap.erase(state.GetRasterizerObjectHandle());
    }

    NVRenderVertexBuffer *
    NVRenderContextImpl::CreateVertexBuffer(NVRenderBufferUsageType::Enum usageType, size_t size,
                                            QT3DSU32 stride, NVConstDataRef<QT3DSU8> bufferData)
    {
        NVRenderVertexBuffer *buffer =
            NVRenderVertexBuffer::Create(*this, usageType, size, stride, bufferData);
        if (buffer)
            m_VertToImpMap.insert(make_pair(buffer->GetImplementationHandle(), buffer));
        return buffer;
    }

    NVRenderVertexBuffer *NVRenderContextImpl::GetVertexBuffer(const void *implementationHandle)
    {
        nvhash_map<const void *, NVRenderVertexBuffer *>::const_iterator entry =
            m_VertToImpMap.find(implementationHandle);
        if (entry != m_VertToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::BufferDestroyed(NVRenderVertexBuffer &buffer)
    {
        m_VertToImpMap.erase(buffer.GetImplementationHandle());
    }

    NVRenderIndexBuffer *
    NVRenderContextImpl::CreateIndexBuffer(qt3ds::render::NVRenderBufferUsageType::Enum usageType,
                                           qt3ds::render::NVRenderComponentTypes::Enum componentType,
                                           size_t size, NVConstDataRef<QT3DSU8> bufferData)
    {
        NVRenderIndexBuffer *buffer =
            NVRenderIndexBuffer::Create(*this, usageType, componentType, size, bufferData);

        if (buffer) {
            m_IndexToImpMap.insert(make_pair(buffer->GetImplementationHandle(), buffer));
        }

        return buffer;
    }

    NVRenderIndexBuffer *NVRenderContextImpl::GetIndexBuffer(const void *implementationHandle)
    {
        const nvhash_map<const void *, NVRenderIndexBuffer *>::iterator entry =
            m_IndexToImpMap.find(implementationHandle);
        if (entry != m_IndexToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::BufferDestroyed(NVRenderIndexBuffer &buffer)
    {
        m_IndexToImpMap.erase(buffer.GetImplementationHandle());
    }

    NVRenderConstantBuffer *
    NVRenderContextImpl::CreateConstantBuffer(const char *bufferName,
                                              qt3ds::render::NVRenderBufferUsageType::Enum usageType,
                                              size_t size, NVConstDataRef<QT3DSU8> bufferData)
    {
        NVRenderConstantBuffer *buffer =
            NVRenderConstantBuffer::Create(*this, bufferName, usageType, size, bufferData);

        if (buffer) {
            m_ConstantToImpMap.insert(make_pair(buffer->GetBufferName(), buffer));
        }

        return buffer;
    }

    NVRenderConstantBuffer *NVRenderContextImpl::GetConstantBuffer(CRegisteredString bufferName)
    {
        TContextConstantBufferMap::iterator entry = m_ConstantToImpMap.find(bufferName);
        if (entry != m_ConstantToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::BufferDestroyed(NVRenderConstantBuffer &buffer)
    {
        m_ConstantToImpMap.erase(buffer.GetBufferName());
    }

    QT3DSU32 NVRenderContextImpl::GetNextConstantBufferUnit()
    {
        QT3DSU32 retval = m_NextConstantBufferUnit;
        ++m_NextConstantBufferUnit;
        // Too many texture units for a single draw call.
        if (retval >= m_MaxConstantBufferUnits) {
            QT3DS_ASSERT(false);
            retval = retval % m_MaxConstantBufferUnits;
        }
        return retval;
    }

    NVRenderStorageBuffer *NVRenderContextImpl::CreateStorageBuffer(
        const char *bufferName, qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
        NVConstDataRef<QT3DSU8> bufferData, NVRenderDataBuffer *pBuffer)
    {
        NVRenderStorageBuffer *buffer =
            NVRenderStorageBuffer::Create(*this, bufferName, usageType, size, bufferData, pBuffer);

        if (buffer) {
            m_StorageToImpMap.insert(make_pair(buffer->GetBufferName(), buffer));
        }

        return buffer;
    }

    NVRenderStorageBuffer *NVRenderContextImpl::GetStorageBuffer(CRegisteredString bufferName)
    {
        TContextStorageBufferMap::iterator entry = m_StorageToImpMap.find(bufferName);
        if (entry != m_StorageToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::BufferDestroyed(NVRenderStorageBuffer &buffer)
    {
        m_StorageToImpMap.erase(buffer.GetBufferName());
    }

    NVRenderAtomicCounterBuffer *NVRenderContextImpl::CreateAtomicCounterBuffer(
        const char *bufferName, qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
        NVConstDataRef<QT3DSU8> bufferData)
    {
        NVRenderAtomicCounterBuffer *buffer =
            NVRenderAtomicCounterBuffer::Create(*this, bufferName, usageType, size, bufferData);

        if (buffer) {
            m_AtomicCounterToImpMap.insert(make_pair(buffer->GetBufferName(), buffer));
        }

        return buffer;
    }

    NVRenderAtomicCounterBuffer *
    NVRenderContextImpl::GetAtomicCounterBuffer(CRegisteredString bufferName)
    {
        TContextAtomicCounterBufferMap::iterator entry = m_AtomicCounterToImpMap.find(bufferName);
        if (entry != m_AtomicCounterToImpMap.end())
            return entry->second;
        return NULL;
    }

    NVRenderAtomicCounterBuffer *
    NVRenderContextImpl::GetAtomicCounterBufferByParam(CRegisteredString paramName)
    {
        // iterate through all atomic counter buffers
        for (TContextAtomicCounterBufferMap::iterator iter = m_AtomicCounterToImpMap.begin(),
                                                      end = m_AtomicCounterToImpMap.end();
             iter != end; ++iter) {
            if (iter->second && iter->second->ContainsParam(paramName))
                return iter->second;
        }

        return NULL;
    }

    void NVRenderContextImpl::BufferDestroyed(NVRenderAtomicCounterBuffer &buffer)
    {
        m_AtomicCounterToImpMap.erase(buffer.GetBufferName());
    }

    NVRenderDrawIndirectBuffer *NVRenderContextImpl::CreateDrawIndirectBuffer(
        qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
        NVConstDataRef<QT3DSU8> bufferData)
    {
        NVRenderDrawIndirectBuffer *buffer =
            NVRenderDrawIndirectBuffer::Create(*this, usageType, size, bufferData);

        if (buffer) {
            m_DrawIndirectToImpMap.insert(make_pair(buffer->GetBuffertHandle(), buffer));
        }

        return buffer;
    }

    NVRenderDrawIndirectBuffer *NVRenderContextImpl::GetDrawIndirectBuffer(
        NVRenderBackend::NVRenderBackendBufferObject implementationHandle)
    {
        TContextDrawIndirectBufferMap::iterator entry =
            m_DrawIndirectToImpMap.find(implementationHandle);
        if (entry != m_DrawIndirectToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::BufferDestroyed(NVRenderDrawIndirectBuffer &buffer)
    {
        m_DrawIndirectToImpMap.erase(buffer.GetBuffertHandle());
    }

    void NVRenderContextImpl::SetMemoryBarrier(NVRenderBufferBarrierFlags barriers)
    {
        m_backend->SetMemoryBarrier(barriers);
    }

    NVRenderOcclusionQuery *NVRenderContextImpl::CreateOcclusionQuery()
    {
        NVRenderOcclusionQuery *theQuery = NVRenderOcclusionQuery::Create(*this);

        return theQuery;
    }

    NVRenderTimerQuery *NVRenderContextImpl::CreateTimerQuery()
    {
        NVRenderTimerQuery *theQuery = NVRenderTimerQuery::Create(*this);

        return theQuery;
    }

    NVRenderSync *NVRenderContextImpl::CreateSync()
    {
        NVRenderSync *theSync = NVRenderSync::Create(*this);

        return theSync;
    }

    NVRenderTexture2D *NVRenderContextImpl::CreateTexture2D()
    {
        NVRenderTexture2D *retval = NVRenderTexture2D::Create(*this);
        if (retval)
            m_Tex2DToImpMap.insert(make_pair(retval->GetImplementationHandle(), retval));
        return retval;
    }

    NVRenderTexture2DArray *NVRenderContextImpl::CreateTexture2DArray()
    {
        NVRenderTexture2DArray *retval = NVRenderTexture2DArray::Create(*this);
        if (retval)
            m_Tex2DArrayToImpMap.insert(make_pair(retval->GetTextureObjectHandle(), retval));

        return retval;
    }

    NVRenderTextureCube *NVRenderContextImpl::CreateTextureCube()
    {
        NVRenderTextureCube *retval = NVRenderTextureCube::Create(*this);
        if (retval)
            m_TexCubeToImpMap.insert(make_pair(retval->GetTextureObjectHandle(), retval));

        return retval;
    }

    NVRenderTexture2D *NVRenderContextImpl::GetTexture2D(const void *implementationHandle)
    {
        const nvhash_map<const void *, NVRenderTexture2D *>::iterator entry =
            m_Tex2DToImpMap.find(implementationHandle);
        if (entry != m_Tex2DToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::TextureDestroyed(NVRenderTexture2D &buffer)
    {
        m_Tex2DToImpMap.erase(buffer.GetImplementationHandle());
        // We would like to find and catch any situations where this texture is being used
        // but that would require some real work that we don't want to do right now.
    }

    void NVRenderContextImpl::TextureDestroyed(NVRenderTexture2DArray &buffer)
    {
        m_Tex2DArrayToImpMap.erase(buffer.GetTextureObjectHandle());
    }

    void NVRenderContextImpl::TextureDestroyed(NVRenderTextureCube &buffer)
    {
        m_TexCubeToImpMap.erase(buffer.GetTextureObjectHandle());
    }

    NVRenderImage2D *NVRenderContextImpl::CreateImage2D(NVRenderTexture2D *inTexture,
                                                        NVRenderImageAccessType::Enum inAccess)
    {
        NVRenderImage2D *retval = NVRenderImage2D::Create(*this, inTexture, inAccess);
        if (retval)
            m_Image2DtoImpMap.insert(make_pair(retval->GetTextureObjectHandle(), retval));

        return retval;
    }

    void NVRenderContextImpl::ImageDestroyed(NVRenderImage2D &image)
    {
        m_Image2DtoImpMap.erase(image.GetTextureObjectHandle());
    }

    // IF this texture isn't on a texture unit, put it on one.
    // If it is on a texture unit, mark it as the most recently used texture.
    QT3DSU32 NVRenderContextImpl::GetNextTextureUnit()
    {
        QT3DSU32 retval = m_NextTextureUnit;
        ++m_NextTextureUnit;
        // Too many texture units for a single draw call.
        if (retval >= m_MaxTextureUnits) {
            QT3DS_ASSERT(false);
            retval = retval % m_MaxTextureUnits;
        }
        return retval;
    }

    NVRenderRenderBuffer *
    NVRenderContextImpl::CreateRenderBuffer(NVRenderRenderBufferFormats::Enum bufferFormat,
                                            QT3DSU32 width, QT3DSU32 height)
    {
        NVRenderRenderBuffer *retval =
            NVRenderRenderBuffer::Create(*this, bufferFormat, width, height);
        if (retval != NULL)
            m_RenderBufferToImpMap.insert(make_pair(retval->GetImplementationHandle(), retval));
        return retval;
    }

    NVRenderRenderBuffer *NVRenderContextImpl::GetRenderBuffer(const void *implementationHandle)
    {
        const nvhash_map<const void *, NVRenderRenderBuffer *>::iterator entry =
            m_RenderBufferToImpMap.find(implementationHandle);
        if (entry != m_RenderBufferToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::RenderBufferDestroyed(NVRenderRenderBuffer &buffer)
    {
        m_RenderBufferToImpMap.erase(buffer.GetImplementationHandle());
    }

    NVRenderFrameBuffer *NVRenderContextImpl::CreateFrameBuffer()
    {
        NVRenderFrameBuffer *retval = NVRenderFrameBuffer::Create(*this);
        if (retval != NULL)
            m_FrameBufferToImpMap.insert(make_pair(retval->GetImplementationHandle(), retval));
        return retval;
    }

    NVRenderFrameBuffer *NVRenderContextImpl::GetFrameBuffer(const void *implementationHandle)
    {
        const nvhash_map<const void *, NVRenderFrameBuffer *>::iterator entry =
            m_FrameBufferToImpMap.find(implementationHandle);
        if (entry != m_FrameBufferToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::FrameBufferDestroyed(NVRenderFrameBuffer &fb)
    {
        m_FrameBufferToImpMap.erase(fb.GetImplementationHandle());
        if (m_HardwarePropertyContext.m_FrameBuffer == &fb)
            m_HardwarePropertyContext.m_FrameBuffer = NULL;
    }

    NVRenderAttribLayout *
    NVRenderContextImpl::CreateAttributeLayout(NVConstDataRef<NVRenderVertexBufferEntry> attribs)
    {
        return QT3DS_NEW(GetFoundation().getAllocator(), NVRenderAttribLayout)(*this, attribs);
    }

    NVRenderInputAssembler *NVRenderContextImpl::CreateInputAssembler(
        NVRenderAttribLayout *attribLayout, NVConstDataRef<NVRenderVertexBuffer *> buffers,
        const NVRenderIndexBuffer *indexBuffer, NVConstDataRef<QT3DSU32> strides,
        NVConstDataRef<QT3DSU32> offsets, NVRenderDrawMode::Enum primType, QT3DSU32 patchVertexCount)
    {
        return QT3DS_NEW(GetFoundation().getAllocator(),
                      NVRenderInputAssembler)(*this, attribLayout, buffers, indexBuffer, strides,
                                              offsets, primType, patchVertexCount);
    }

    void NVRenderContextImpl::SetInputAssembler(NVRenderInputAssembler *inputAssembler)
    {
        if (m_HardwarePropertyContext.m_InputAssembler != inputAssembler) {
            DoSetInputAssembler(inputAssembler);
        }
    }

    NVRenderVertFragCompilationResult NVRenderContextImpl::CompileSource(
        const char *shaderName, NVConstDataRef<QT3DSI8> vertShader, NVConstDataRef<QT3DSI8> fragShader,
        NVConstDataRef<QT3DSI8> tessControlShaderSource,
        NVConstDataRef<QT3DSI8> tessEvaluationShaderSource, NVConstDataRef<QT3DSI8> geometryShaderSource,
        bool separateProgram, NVRenderShaderProgramBinaryType::Enum type, bool binaryProgram)
    {
        NVRenderVertFragCompilationResult result = NVRenderShaderProgram::Create(
            *this, shaderName, vertShader, fragShader, tessControlShaderSource,
            tessEvaluationShaderSource, geometryShaderSource, separateProgram, type, binaryProgram);

        if (result.mShader != NULL)
            m_ShaderToImpMap.insert(
                make_pair(result.mShader->GetShaderProgramHandle(), result.mShader));

        return result;
    }

    NVRenderVertFragCompilationResult NVRenderContextImpl::CompileBinary(
        const char *shaderName, NVRenderShaderProgramBinaryType::Enum type,
        NVDataRef<QT3DSI8> vertShader, NVDataRef<QT3DSI8> fragShader,
        NVDataRef<QT3DSI8> tessControlShaderSource, NVDataRef<QT3DSI8> tessEvaluationShaderSource,
        NVConstDataRef<QT3DSI8> geometryShaderSource)
    {
#ifndef _MACOSX
        NVRenderVertFragCompilationResult result = NVRenderShaderProgram::Create(
            *this, shaderName, vertShader, fragShader, tessControlShaderSource,
            tessEvaluationShaderSource, geometryShaderSource, false, type, true);

        if (result.mShader != NULL)
            m_ShaderToImpMap.insert(
                make_pair(result.mShader->GetShaderProgramHandle(), result.mShader));

        return result;
#else
        QT3DS_ASSERT(false);
        return NVRenderVertFragCompilationResult();
#endif
    }

    NVRenderVertFragCompilationResult
    NVRenderContextImpl::CompileComputeSource(const char *shaderName,
                                              NVConstDataRef<QT3DSI8> computeShaderSource)
    {
        NVRenderVertFragCompilationResult result =
            NVRenderShaderProgram::CreateCompute(*this, shaderName, computeShaderSource);

        if (result.mShader != NULL)
            m_ShaderToImpMap.insert(
                make_pair(result.mShader->GetShaderProgramHandle(), result.mShader));

        return result;
    }

    NVRenderShaderProgram *NVRenderContextImpl::GetShaderProgram(const void *implementationHandle)
    {
        const nvhash_map<const void *, NVRenderShaderProgram *>::iterator entry =
            m_ShaderToImpMap.find(implementationHandle);
        if (entry != m_ShaderToImpMap.end())
            return entry->second;
        return NULL;
    }

    void NVRenderContextImpl::ShaderDestroyed(NVRenderShaderProgram &shader)
    {
        m_ShaderToImpMap.erase(shader.GetShaderProgramHandle());
        if (m_HardwarePropertyContext.m_ActiveShader == &shader)
            SetActiveShader(NULL);
    }

    NVRenderProgramPipeline *NVRenderContextImpl::CreateProgramPipeline()
    {
        return QT3DS_NEW(GetFoundation().getAllocator(), NVRenderProgramPipeline)(*this,
                                                                               GetFoundation());
    }

    NVRenderPathSpecification *NVRenderContextImpl::CreatePathSpecification()
    {
        return NVRenderPathSpecification::CreatePathSpecification(*this);
    }

    NVRenderPathRender *NVRenderContextImpl::CreatePathRender(size_t range)
    {
        return NVRenderPathRender::Create(*this, range);
    }

    void NVRenderContextImpl::SetPathProjectionMatrix(const QT3DSMat44 inPathProjection)
    {
        m_backend->SetPathProjectionMatrix(inPathProjection);
    }

    void NVRenderContextImpl::SetPathModelViewMatrix(const QT3DSMat44 inPathModelview)
    {
        m_backend->SetPathModelViewMatrix(inPathModelview);
    }

    void NVRenderContextImpl::SetPathStencilDepthOffset(QT3DSF32 inSlope, QT3DSF32 inBias)
    {
        m_backend->SetPathStencilDepthOffset(inSlope, inBias);
    }
    void NVRenderContextImpl::SetPathCoverDepthFunc(NVRenderBoolOp::Enum inFunc)
    {
        m_backend->SetPathCoverDepthFunc(inFunc);
    }

    NVRenderPathFontSpecification *
    NVRenderContextImpl::CreatePathFontSpecification(CRegisteredString fontName)
    {
        // first check if it already exists
        nvhash_map<CRegisteredString, NVRenderPathFontSpecification *>::const_iterator entry =
            m_PathFontSpecToImpMap.find(fontName);
        if (entry != m_PathFontSpecToImpMap.end())
            return entry->second;

        // if not create new one
        NVRenderPathFontSpecification *pPathFontSpec =
            NVRenderPathFontSpecification::CreatePathFontSpecification(*this, fontName);

        if (pPathFontSpec)
            m_PathFontSpecToImpMap.insert(make_pair(fontName, pPathFontSpec));

        return pPathFontSpec;
    }

    void
    NVRenderContextImpl::ReleasePathFontSpecification(NVRenderPathFontSpecification &inPathSpec)
    {
        m_PathFontSpecToImpMap.erase(inPathSpec.GetFontName());
    }

    NVRenderPathFontItem *NVRenderContextImpl::CreatePathFontItem()
    {
        // if not create new one
        return NVRenderPathFontItem::CreatePathFontItem(*this);
    }

    void NVRenderContextImpl::SetClearColor(QT3DSVec4 inClearColor)
    {
        if (m_HardwarePropertyContext.m_ClearColor != inClearColor)
            DoSetClearColor(inClearColor);
    }

    void NVRenderContextImpl::SetBlendFunction(NVRenderBlendFunctionArgument inFunctions)
    {
        if (memcmp(&inFunctions, &m_HardwarePropertyContext.m_BlendFunction,
                   sizeof(NVRenderBlendFunctionArgument))) {
            DoSetBlendFunction(inFunctions);
        }
    }

    void NVRenderContextImpl::SetBlendEquation(NVRenderBlendEquationArgument inEquations)
    {
        if (memcmp(&inEquations, &m_HardwarePropertyContext.m_BlendEquation,
                   sizeof(NVRenderBlendEquationArgument))) {
            DoSetBlendEquation(inEquations);
        }
    }

    void NVRenderContextImpl::SetCullingEnabled(bool inEnabled)
    {
        if (inEnabled != m_HardwarePropertyContext.m_CullingEnabled) {
            DoSetCullingEnabled(inEnabled);
        }
    }

    void NVRenderContextImpl::SetDepthFunction(qt3ds::render::NVRenderBoolOp::Enum inFunction)
    {
        if (inFunction != m_HardwarePropertyContext.m_DepthFunction) {
            DoSetDepthFunction(inFunction);
        }
    }

    void NVRenderContextImpl::SetBlendingEnabled(bool inEnabled)
    {
        if (inEnabled != m_HardwarePropertyContext.m_BlendingEnabled) {
            DoSetBlendingEnabled(inEnabled);
        }
    }

    void NVRenderContextImpl::SetColorWritesEnabled(bool inEnabled)
    {
        if (inEnabled != m_HardwarePropertyContext.m_ColorWritesEnabled) {
            DoSetColorWritesEnabled(inEnabled);
        }
    }

    void NVRenderContextImpl::SetDepthWriteEnabled(bool inEnabled)
    {
        if (inEnabled != m_HardwarePropertyContext.m_DepthWriteEnabled) {
            m_HardwarePropertyContext.m_DepthWriteEnabled = inEnabled;
            m_backend->SetRenderState(inEnabled, NVRenderState::DepthWrite);
        }
    }

    void NVRenderContextImpl::SetDepthTestEnabled(bool inEnabled)
    {
        if (inEnabled != m_HardwarePropertyContext.m_DepthTestEnabled) {
            DoSetDepthTestEnabled(inEnabled);
        }
    }

    void NVRenderContextImpl::SetMultisampleEnabled(bool inEnabled)
    {
        if (inEnabled != m_HardwarePropertyContext.m_MultisampleEnabled) {
            DoSetMultisampleEnabled(inEnabled);
        }
    }

    void NVRenderContextImpl::SetStencilTestEnabled(bool inEnabled)
    {
        if (inEnabled != m_HardwarePropertyContext.m_StencilTestEnabled) {
            DoSetStencilTestEnabled(inEnabled);
        }
    }

    void NVRenderContextImpl::SetScissorTestEnabled(bool inEnabled)
    {
        if (inEnabled != m_HardwarePropertyContext.m_ScissorTestEnabled) {
            DoSetScissorTestEnabled(inEnabled);
        }
    }

    void NVRenderContextImpl::SetScissorRect(NVRenderRect inRect)
    {
        if (memcmp(&inRect, &m_HardwarePropertyContext.m_ScissorRect, sizeof(NVRenderRect))) {
            DoSetScissorRect(inRect);
        }
    }

    void NVRenderContextImpl::SetViewport(NVRenderRect inViewport)
    {
        if (memcmp(&inViewport, &m_HardwarePropertyContext.m_Viewport, sizeof(NVRenderRect))) {
            DoSetViewport(inViewport);
        }
    }

    void NVRenderContextImpl::SetActiveShader(NVRenderShaderProgram *inShader)
    {
        if (inShader != m_HardwarePropertyContext.m_ActiveShader)
            DoSetActiveShader(inShader);
    }

    void NVRenderContextImpl::SetActiveProgramPipeline(NVRenderProgramPipeline *inProgramPipeline)
    {
        if (inProgramPipeline != m_HardwarePropertyContext.m_ActiveProgramPipeline)
            DoSetActiveProgramPipeline(inProgramPipeline);
    }

    void NVRenderContextImpl::DispatchCompute(NVRenderShaderProgram *inShader, QT3DSU32 numGroupsX,
                                              QT3DSU32 numGroupsY, QT3DSU32 numGroupsZ)
    {
        QT3DS_ASSERT(inShader);

        if (inShader != m_HardwarePropertyContext.m_ActiveShader)
            DoSetActiveShader(inShader);

        m_backend->DispatchCompute(inShader->GetShaderProgramHandle(), numGroupsX, numGroupsY,
                                   numGroupsZ);

        OnPostDraw();
    }

    void NVRenderContextImpl::SetDrawBuffers(NVConstDataRef<QT3DSI32> inDrawBufferSet)
    {
        m_backend->SetDrawBuffers(
            (m_HardwarePropertyContext.m_FrameBuffer)
                ? m_HardwarePropertyContext.m_FrameBuffer->GetFrameBuffertHandle()
                : NULL,
            inDrawBufferSet);
    }

    void NVRenderContextImpl::SetReadBuffer(NVReadFaces::Enum inReadFace)
    {
        // currently NULL which means the read target must be set with setReadTarget
        m_backend->SetReadBuffer(NULL, inReadFace);
    }

    void NVRenderContextImpl::ReadPixels(NVRenderRect inRect,
                                         NVRenderReadPixelFormats::Enum inFormat,
                                         NVDataRef<QT3DSU8> inWriteBuffer)
    {
        // NULL means read from current render target
        m_backend->ReadPixel(NULL, inRect.m_X, inRect.m_Y, inRect.m_Width, inRect.m_Height,
                             inFormat, (void *)inWriteBuffer.begin());
    }

    void NVRenderContextImpl::SetRenderTarget(NVRenderFrameBuffer *inBuffer)
    {
        if (inBuffer != m_HardwarePropertyContext.m_FrameBuffer) {
            DoSetRenderTarget(inBuffer);
        }
    }

    void NVRenderContextImpl::SetReadTarget(NVRenderFrameBuffer *inBuffer)
    {
        if (inBuffer != m_HardwarePropertyContext.m_FrameBuffer) {
            DoSetReadTarget(inBuffer);
        }
    }

    void NVRenderContextImpl::ResetBlendState()
    {
        QT3DSI32_4 values;

        m_backend->SetRenderState(m_HardwarePropertyContext.m_BlendingEnabled,
                                  NVRenderState::Blend);
        const NVRenderBlendFunctionArgument &theBlendArg(m_HardwarePropertyContext.m_BlendFunction);
        m_backend->SetBlendFunc(theBlendArg);
    }

    // Pop the entire set of properties, potentially forcing the values
    // to opengl.
    void NVRenderContextImpl::PopPropertySet(bool inForceSetProperties)
    {
        if (!m_PropertyStack.empty()) {
            SNVGLHardPropertyContext &theTopContext(m_PropertyStack.back());
            if (inForceSetProperties) {
#define HANDLE_CONTEXT_HARDWARE_PROPERTY(setterName, propName)                                     \
    DoSet##setterName(theTopContext.m_##propName);

                ITERATE_HARDWARE_CONTEXT_PROPERTIES

#undef HANDLE_CONTEXT_HARDWARE_PROPERTY
            } else {
#define HANDLE_CONTEXT_HARDWARE_PROPERTY(setterName, propName)                                     \
    Set##setterName(theTopContext.m_##propName);

                ITERATE_HARDWARE_CONTEXT_PROPERTIES

#undef HANDLE_CONTEXT_HARDWARE_PROPERTY
            }
            m_PropertyStack.pop_back();
        }
    }

    void NVRenderContextImpl::Clear(NVRenderClearFlags flags)
    {
        if ((flags & NVRenderClearValues::Depth)
            && m_HardwarePropertyContext.m_DepthWriteEnabled == false) {
            QT3DS_ASSERT(false);
            SetDepthWriteEnabled(true);
        }
        m_backend->Clear(flags);
    }

    void NVRenderContextImpl::Clear(NVRenderFrameBuffer &fb, NVRenderClearFlags flags)
    {
        NVRenderFrameBuffer *previous = m_HardwarePropertyContext.m_FrameBuffer;
        if (previous != &fb)
            SetRenderTarget(&fb);

        Clear(flags);

        if (previous != &fb)
            SetRenderTarget(previous);
    }

    void NVRenderContextImpl::BlitFramebuffer(QT3DSI32 srcX0, QT3DSI32 srcY0, QT3DSI32 srcX1, QT3DSI32 srcY1,
                                              QT3DSI32 dstX0, QT3DSI32 dstY0, QT3DSI32 dstX1, QT3DSI32 dstY1,
                                              NVRenderClearFlags flags,
                                              NVRenderTextureMagnifyingOp::Enum filter)
    {
        m_backend->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, flags,
                                   filter);
    }

    bool
    NVRenderContextImpl::BindShaderToInputAssembler(const NVRenderInputAssembler *inputAssembler,
                                                    NVRenderShaderProgram *shader)
    {
        // setup the input assembler object
        return m_backend->SetInputAssembler(inputAssembler->GetInputAssemblerHandle(),
                                            shader->GetShaderProgramHandle());
    }

    bool NVRenderContextImpl::ApplyPreDrawProperties()
    {
        // Get the currently bound vertex and shader
        NVRenderInputAssembler *inputAssembler = this->m_HardwarePropertyContext.m_InputAssembler;
        NVRenderShaderProgram *shader = this->m_HardwarePropertyContext.m_ActiveShader;

        // we could render through a program pipline
        if (shader == NULL && this->m_HardwarePropertyContext.m_ActiveProgramPipeline)
            shader = this->m_HardwarePropertyContext.m_ActiveProgramPipeline->GetVertexStage();

        if (inputAssembler == NULL || shader == NULL) {
            qCCritical(INVALID_OPERATION,
                "Attempting to render no valid shader or input assembler setup");
            QT3DS_ASSERT(false);
            return false;
        }

        return BindShaderToInputAssembler(inputAssembler, shader);
    }

    void NVRenderContextImpl::OnPostDraw()
    {
        // reset input assembler binding
        m_backend->SetInputAssembler(NULL, 0);
        // Texture unit 0 is used for setting up and loading textures.
        // Bugs happen if we load a texture then setup the sampler.
        // Then we load another texture.  Because when loading we use texture unit 0,
        // the render bindings for the first texture are blown away.
        // Again, for this reason, texture unit 0 is reserved for loading textures.
        m_NextTextureUnit = 1;
        m_NextConstantBufferUnit = 0;
    }

    void NVRenderContextImpl::Draw(NVRenderDrawMode::Enum drawMode, QT3DSU32 count, QT3DSU32 offset)
    {
        if (!ApplyPreDrawProperties())
            return;

        NVRenderIndexBuffer *theIndexBuffer = const_cast<NVRenderIndexBuffer *>(
            m_HardwarePropertyContext.m_InputAssembler->GetIndexBuffer());
        if (theIndexBuffer == NULL)
            m_backend->Draw(drawMode, offset, count);
        else
            theIndexBuffer->Draw(drawMode, count, offset);

        OnPostDraw();
    }

    void NVRenderContextImpl::DrawIndirect(NVRenderDrawMode::Enum drawMode, QT3DSU32 offset)
    {
        if (!ApplyPreDrawProperties())
            return;

        NVRenderIndexBuffer *theIndexBuffer = const_cast<NVRenderIndexBuffer *>(
            m_HardwarePropertyContext.m_InputAssembler->GetIndexBuffer());
        if (theIndexBuffer == NULL)
            m_backend->DrawIndirect(drawMode, (const void *)offset);
        else
            theIndexBuffer->DrawIndirect(drawMode, offset);

        OnPostDraw();
    }

    QT3DSMat44
    NVRenderContext::ApplyVirtualViewportToProjectionMatrix(const QT3DSMat44 &inProjection,
                                                            const NVRenderRectF &inViewport,
                                                            const NVRenderRectF &inVirtualViewport)
    {
        if (inVirtualViewport == inViewport)
            return inProjection;
        // Run conversion to floating point once.
        NVRenderRectF theVirtualViewport(inVirtualViewport);
        NVRenderRectF theViewport(inViewport);
        if (theVirtualViewport.m_Width == 0 || theVirtualViewport.m_Height == 0
            || theViewport.m_Width == 0 || theViewport.m_Height == 0) {
            QT3DS_ASSERT(false);
            return inProjection;
        }
        QT3DSMat44 theScaleTransMat(QT3DSMat44::createIdentity());
        QT3DSF32 theHeightDiff = theViewport.m_Height - theVirtualViewport.m_Height;
        QT3DSF32 theViewportOffY = theVirtualViewport.m_Y - theViewport.m_Y;
        QT3DSVec2 theCameraOffsets = QT3DSVec2(theVirtualViewport.m_Width - theViewport.m_Width
                                             + (theVirtualViewport.m_X - theViewport.m_X) * 2.0f,
                                         theHeightDiff + (theViewportOffY - theHeightDiff) * 2.0f);
        QT3DSVec2 theCameraScale = QT3DSVec2(theVirtualViewport.m_Width / theViewport.m_Width,
                                       theVirtualViewport.m_Height / theViewport.m_Height);

        QT3DSVec3 theTranslation(theCameraOffsets.x / theViewport.m_Width,
                              theCameraOffsets.y / theViewport.m_Height, 0);
        theScaleTransMat.column3[0] = theTranslation.x;
        theScaleTransMat.column3[1] = theTranslation.y;
        theScaleTransMat.column0[0] = theCameraScale.x;
        theScaleTransMat.column1[1] = theCameraScale.y;

        return theScaleTransMat * inProjection;
    }

    NVRenderVertFragCompilationResult NVRenderContext::CompileSource(
        const char *shaderName, const char *vertShader, QT3DSU32 inVertLen, const char *fragShader,
        QT3DSU32 inFragLen, const char *tessControlShaderSource, QT3DSU32 inTCLen,
        const char *tessEvaluationShaderSource, QT3DSU32 inTELen, const char *geometryShaderSource,
        QT3DSU32 inGSLen, bool separableProgram)
    {
        return CompileSource(
            shaderName, NVConstDataRef<QT3DSI8>((const QT3DSI8 *)vertShader, inVertLen),
            NVConstDataRef<QT3DSI8>((const QT3DSI8 *)fragShader, inFragLen),
            NVConstDataRef<QT3DSI8>((const QT3DSI8 *)tessControlShaderSource, inTCLen),
            NVConstDataRef<QT3DSI8>((const QT3DSI8 *)tessEvaluationShaderSource, inTELen),
            NVConstDataRef<QT3DSI8>((const QT3DSI8 *)geometryShaderSource, inGSLen), separableProgram);
    }

    void NVRenderContextImpl::DoSetActiveShader(NVRenderShaderProgram *inShader)
    {
        m_HardwarePropertyContext.m_ActiveShader = NULL;
        if (inShader)
            m_backend->SetActiveProgram(inShader->GetShaderProgramHandle());
        else {
            m_backend->SetActiveProgram(NULL);
        }
        m_HardwarePropertyContext.m_ActiveShader = inShader;
    }

    void NVRenderContextImpl::DoSetActiveProgramPipeline(NVRenderProgramPipeline *inProgramPipeline)
    {
        if (inProgramPipeline) {
            // invalid any bound shader
            DoSetActiveShader(NULL);
            inProgramPipeline->Bind();
        } else
            m_backend->SetActiveProgramPipeline(NULL);

        m_HardwarePropertyContext.m_ActiveProgramPipeline = inProgramPipeline;
    }

    NVRenderContext &NVRenderContext::CreateNULL(NVFoundationBase &foundation,
                                                 IStringTable &inStringTable)
    {
        NVRenderContext *retval = NULL;

        // create backend
        NVScopedRefCounted<IStringTable> theStringTable(inStringTable);
        NVScopedRefCounted<NVRenderBackend> theBackend =
            NVRenderBackendNULL::CreateBackend(foundation);
        retval = QT3DS_NEW(foundation.getAllocator(), NVRenderContextImpl)(foundation, *theBackend,
                                                                        *theStringTable);
        return *retval;
    }
}
} // end namespace
