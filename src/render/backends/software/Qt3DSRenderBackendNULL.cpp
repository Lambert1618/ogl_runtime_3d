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
#include "render/backends/software/Qt3DSRenderBackendNULL.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"

#include <QSurfaceFormat>

using namespace qt3ds::render;
using namespace qt3ds::foundation;
using namespace qt3ds;

namespace {
struct SNullBackend : public NVRenderBackend
{
    NVFoundationBase &m_Foundation;
    QT3DSI32 mRefCount;

    SNullBackend(NVFoundationBase &fnd)
        : m_Foundation(fnd)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    /// backend interface

    NVRenderContextType GetRenderContextType() const override
    {
        return NVRenderContextValues::NullContext;
    }
    const char *GetShadingLanguageVersion() override { return ""; }
    QT3DSU32 GetMaxCombinedTextureUnits() override { return 32; }
    bool GetRenderBackendCap(NVRenderBackendCaps::Enum) const override { return false; }
    void GetRenderBackendValue(NVRenderBackendQuery::Enum inQuery, QT3DSI32 *params) const override
    {
        if (params) {
            switch (inQuery) {
            case NVRenderBackendQuery::MaxTextureSize:
                *params = 4096;
                break;
            case NVRenderBackendQuery::MaxTextureArrayLayers:
                *params = 0;
                break;
            default:
                QT3DS_ASSERT(false);
                *params = 0;
                break;
            }
        }
    }
    QT3DSU32 GetDepthBits() const override { return 16; }
    QT3DSU32 GetStencilBits() const override { return 0; }
    void SetRenderState(bool, const NVRenderState::Enum) override {}
    bool GetRenderState(const NVRenderState::Enum) override { return false; }
    virtual NVRenderBackendDepthStencilStateObject
    CreateDepthStencilState(bool, bool, NVRenderBoolOp::Enum, bool,
                            NVRenderStencilFunctionArgument &, NVRenderStencilFunctionArgument &,
                            NVRenderStencilOperationArgument &, NVRenderStencilOperationArgument &) override
    {
        return NVRenderBackendDepthStencilStateObject(1);
    }
    void ReleaseDepthStencilState(NVRenderBackendDepthStencilStateObject) override {}
    NVRenderBackendRasterizerStateObject CreateRasterizerState(QT3DSF32, QT3DSF32,
                                                                       NVRenderFaces::Enum) override
    {
        return NVRenderBackendRasterizerStateObject(1);
    }
    void ReleaseRasterizerState(NVRenderBackendRasterizerStateObject) override {}
    void SetDepthStencilState(NVRenderBackendDepthStencilStateObject) override {}
    void SetRasterizerState(NVRenderBackendRasterizerStateObject) override {}
    NVRenderBoolOp::Enum GetDepthFunc() override { return NVRenderBoolOp::Equal; }
    void SetDepthFunc(const NVRenderBoolOp::Enum) override {}
    bool GetDepthWrite() override { return false; }

    void SetDepthWrite(bool) override {}
    void SetColorWrites(bool, bool, bool, bool) override {}
    void SetMultisample(bool) override {}
    void GetBlendFunc(NVRenderBlendFunctionArgument *) override {}
    void SetBlendFunc(const NVRenderBlendFunctionArgument &) override {}
    void SetBlendEquation(const NVRenderBlendEquationArgument &) override {}
    void SetBlendBarrier(void) override {}
    void GetScissorRect(NVRenderRect *) override {}
    void SetScissorRect(const NVRenderRect &) override {}
    void GetViewportRect(NVRenderRect *) override {}
    void SetViewportRect(const NVRenderRect &) override {}
    void SetClearColor(const QT3DSVec4 *) override {}
    void Clear(NVRenderClearFlags) override {}
    NVRenderBackendBufferObject CreateBuffer(size_t, NVRenderBufferBindFlags,
                                                     NVRenderBufferUsageType::Enum, const void *) override
    {
        return NVRenderBackendBufferObject(1);
    }
    void BindBuffer(NVRenderBackendBufferObject, NVRenderBufferBindFlags) override {}
    void ReleaseBuffer(NVRenderBackendBufferObject) override {}

    void UpdateBuffer(NVRenderBackendBufferObject, NVRenderBufferBindFlags, size_t,
                              NVRenderBufferUsageType::Enum, const void *) override
    {
    }
    void UpdateBufferRange(NVRenderBackendBufferObject, NVRenderBufferBindFlags, size_t, size_t,
                           const void *) override
    {
    }
    void *MapBuffer(NVRenderBackendBufferObject, NVRenderBufferBindFlags, size_t, size_t,
                            NVRenderBufferAccessFlags) override
    {
        return NULL;
    }
    bool UnmapBuffer(NVRenderBackendBufferObject, NVRenderBufferBindFlags) override { return true; }
    void SetMemoryBarrier(NVRenderBufferBarrierFlags) override {}
    NVRenderBackendQueryObject CreateQuery() override { return NVRenderBackendQueryObject(1); }
    void ReleaseQuery(NVRenderBackendQueryObject) override {}
    void BeginQuery(NVRenderBackendQueryObject, NVRenderQueryType::Enum) override {}
    void EndQuery(NVRenderBackendQueryObject, NVRenderQueryType::Enum) override {}
    void GetQueryResult(NVRenderBackendQueryObject, NVRenderQueryResultType::Enum,
                                QT3DSU32 *) override {}
    void GetQueryResult(NVRenderBackendQueryObject, NVRenderQueryResultType::Enum,
                                QT3DSU64 *) override {}
    void SetQueryTimer(NVRenderBackendQueryObject) override {}
    NVRenderBackendSyncObject CreateSync(NVRenderSyncType::Enum, NVRenderSyncFlags) override
    {
        return NVRenderBackendSyncObject(1);
    };
    void ReleaseSync(NVRenderBackendSyncObject) override {}
    void WaitSync(NVRenderBackendSyncObject, NVRenderCommandFlushFlags, QT3DSU64) override {}
    NVRenderBackendRenderTargetObject CreateRenderTarget() override
    {
        return NVRenderBackendRenderTargetObject(1);
    }
    void ReleaseRenderTarget(NVRenderBackendRenderTargetObject) override {}
    void RenderTargetAttach(NVRenderBackendRenderTargetObject,
                                    NVRenderFrameBufferAttachments::Enum,
                                    NVRenderBackendRenderbufferObject) override
    {
    }
    void RenderTargetAttach(NVRenderBackendRenderTargetObject,
                                    NVRenderFrameBufferAttachments::Enum,
                                    NVRenderBackendTextureObject, NVRenderTextureTargetType::Enum) override
    {
    }
    void RenderTargetAttach(NVRenderBackendRenderTargetObject,
                                    NVRenderFrameBufferAttachments::Enum,
                                    NVRenderBackendTextureObject, QT3DSI32, QT3DSI32) override
    {
    }
    void SetRenderTarget(NVRenderBackendRenderTargetObject) override {}
    bool RenderTargetIsValid(NVRenderBackendRenderTargetObject) override { return false; }
    void SetReadTarget(NVRenderBackendRenderTargetObject) override {}
    void SetDrawBuffers(NVRenderBackendRenderTargetObject, NVConstDataRef<QT3DSI32>) override {}
    void SetReadBuffer(NVRenderBackendRenderTargetObject, NVReadFaces::Enum) override {}

    void BlitFramebuffer(QT3DSI32, QT3DSI32, QT3DSI32, QT3DSI32, QT3DSI32, QT3DSI32, QT3DSI32, QT3DSI32,
                                 NVRenderClearFlags, NVRenderTextureMagnifyingOp::Enum) override
    {
    }
    NVRenderBackendRenderbufferObject CreateRenderbuffer(NVRenderRenderBufferFormats::Enum,
                                                                 size_t, size_t) override
    {
        return NVRenderBackendRenderbufferObject(1);
    }
    void ReleaseRenderbuffer(NVRenderBackendRenderbufferObject) override {}

    bool ResizeRenderbuffer(NVRenderBackendRenderbufferObject,
                                    NVRenderRenderBufferFormats::Enum, size_t, size_t) override
    {
        return false;
    }
    NVRenderBackendTextureObject CreateTexture() override { return NVRenderBackendTextureObject(1); }

    void SetTextureData2D(NVRenderBackendTextureObject, NVRenderTextureTargetType::Enum,
                                  QT3DSU32, NVRenderTextureFormats::Enum, size_t, size_t, QT3DSI32,
                                  NVRenderTextureFormats::Enum, const void *) override
    {
    }
    void SetTextureDataCubeFace(NVRenderBackendTextureObject,
                                        NVRenderTextureTargetType::Enum, QT3DSU32,
                                        NVRenderTextureFormats::Enum, size_t, size_t, QT3DSI32,
                                        NVRenderTextureFormats::Enum, const void *) override
    {
    }
    void CreateTextureStorage2D(NVRenderBackendTextureObject,
                                        NVRenderTextureTargetType::Enum, QT3DSU32,
                                        NVRenderTextureFormats::Enum, size_t, size_t) override
    {
    }
    void SetTextureSubData2D(NVRenderBackendTextureObject, NVRenderTextureTargetType::Enum,
                                     QT3DSU32, QT3DSI32, QT3DSI32, size_t, size_t,
                                     NVRenderTextureFormats::Enum, const void *) override
    {
    }
    void SetCompressedTextureData2D(NVRenderBackendTextureObject,
                                            NVRenderTextureTargetType::Enum, QT3DSU32,
                                            NVRenderTextureFormats::Enum, size_t, size_t, QT3DSI32,
                                            size_t, const void *) override
    {
    }
    void SetCompressedTextureDataCubeFace(NVRenderBackendTextureObject,
                                                  NVRenderTextureTargetType::Enum, QT3DSU32,
                                                  NVRenderTextureFormats::Enum, size_t, size_t,
                                                  QT3DSI32, size_t, const void *) override
    {
    }
    void SetCompressedTextureSubData2D(NVRenderBackendTextureObject,
                                               NVRenderTextureTargetType::Enum, QT3DSU32, QT3DSI32, QT3DSI32,
                                               size_t, size_t, NVRenderTextureFormats::Enum, size_t,
                                               const void *) override
    {
    }
    void SetMultisampledTextureData2D(NVRenderBackendTextureObject,
                                              NVRenderTextureTargetType::Enum, size_t,
                                              NVRenderTextureFormats::Enum, size_t, size_t, bool) override
    {
    }
    void SetTextureData3D(NVRenderBackendTextureObject, NVRenderTextureTargetType::Enum,
                                  QT3DSU32, NVRenderTextureFormats::Enum, size_t, size_t, size_t,
                                  QT3DSI32, NVRenderTextureFormats::Enum, const void *) override
    {
    }
    void GenerateMipMaps(NVRenderBackendTextureObject, NVRenderTextureTargetType::Enum,
                                 NVRenderHint::Enum) override
    {
    }
    void BindTexture(NVRenderBackendTextureObject, NVRenderTextureTargetType::Enum, QT3DSU32) override
    {
    }
    void BindImageTexture(NVRenderBackendTextureObject, QT3DSU32, QT3DSI32, bool, QT3DSI32,
                                  NVRenderImageAccessType::Enum, NVRenderTextureFormats::Enum) override
    {
    }
    void ReleaseTexture(NVRenderBackendTextureObject) override {}

    virtual NVRenderTextureSwizzleMode::Enum
    GetTextureSwizzleMode(const NVRenderTextureFormats::Enum) const override
    {
        return NVRenderTextureSwizzleMode::NoSwizzle;
    }

    virtual NVRenderBackendSamplerObject
    CreateSampler(NVRenderTextureMinifyingOp::Enum, NVRenderTextureMagnifyingOp::Enum,
                  NVRenderTextureCoordOp::Enum, NVRenderTextureCoordOp::Enum,
                  NVRenderTextureCoordOp::Enum, QT3DSI32, QT3DSI32, QT3DSF32,
                  NVRenderTextureCompareMode::Enum, NVRenderTextureCompareOp::Enum, QT3DSF32, QT3DSF32 *) override
    {
        return NVRenderBackendSamplerObject(1);
    }

    void UpdateSampler(NVRenderBackendSamplerObject, NVRenderTextureTargetType::Enum,
                               NVRenderTextureMinifyingOp::Enum, NVRenderTextureMagnifyingOp::Enum,
                               NVRenderTextureCoordOp::Enum, NVRenderTextureCoordOp::Enum,
                               NVRenderTextureCoordOp::Enum, QT3DSF32, QT3DSF32, QT3DSF32,
                               NVRenderTextureCompareMode::Enum, NVRenderTextureCompareOp::Enum,
                               QT3DSF32, QT3DSF32 *) override
    {
    }

    void UpdateTextureObject(NVRenderBackendTextureObject, NVRenderTextureTargetType::Enum,
                                     QT3DSI32, QT3DSI32) override
    {
    }

    void UpdateTextureSwizzle(NVRenderBackendTextureObject, NVRenderTextureTargetType::Enum,
                                      NVRenderTextureSwizzleMode::Enum) override
    {
    }

    void ReleaseSampler(NVRenderBackendSamplerObject) override {}

    virtual NVRenderBackendAttribLayoutObject
        CreateAttribLayout(NVConstDataRef<NVRenderVertexBufferEntry>) override
    {
        return NVRenderBackendAttribLayoutObject(1);
    }

    void ReleaseAttribLayout(NVRenderBackendAttribLayoutObject) override {}

    NVRenderBackendInputAssemblerObject CreateInputAssembler(
        NVRenderBackendAttribLayoutObject, NVConstDataRef<NVRenderBackendBufferObject>,
        const NVRenderBackendBufferObject, NVConstDataRef<QT3DSU32>, NVConstDataRef<QT3DSU32>, QT3DSU32) override
    {
        return NVRenderBackendInputAssemblerObject(1);
    }
    void ReleaseInputAssembler(NVRenderBackendInputAssemblerObject) override {}
    bool SetInputAssembler(NVRenderBackendInputAssemblerObject,
                                   NVRenderBackendShaderProgramObject) override
    {
        return false;
    }
    void SetPatchVertexCount(NVRenderBackendInputAssemblerObject, QT3DSU32) override {}
    NVRenderBackendVertexShaderObject CreateVertexShader(NVConstDataRef<QT3DSI8>,
                                                                 eastl::string &, bool) override
    {
        return NVRenderBackendVertexShaderObject(1);
    }
    void ReleaseVertexShader(NVRenderBackendVertexShaderObject) override {}
    NVRenderBackendFragmentShaderObject CreateFragmentShader(NVConstDataRef<QT3DSI8>,
                                                                     eastl::string &, bool) override
    {
        return NVRenderBackendFragmentShaderObject(1);
    }
    void ReleaseFragmentShader(NVRenderBackendFragmentShaderObject) override {}
    NVRenderBackendTessControlShaderObject CreateTessControlShader(NVConstDataRef<QT3DSI8>,
                                                                           eastl::string &, bool) override
    {
        return NVRenderBackendTessControlShaderObject(1);
    }
    void ReleaseTessControlShader(NVRenderBackendTessControlShaderObject) override {}
    virtual NVRenderBackendTessEvaluationShaderObject
    CreateTessEvaluationShader(NVConstDataRef<QT3DSI8>, eastl::string &, bool) override
    {
        return NVRenderBackendTessEvaluationShaderObject(1);
    }
    void ReleaseTessEvaluationShader(NVRenderBackendTessEvaluationShaderObject) override {}
    NVRenderBackendGeometryShaderObject CreateGeometryShader(NVConstDataRef<QT3DSI8>,
                                                                     eastl::string &, bool) override
    {
        return NVRenderBackendGeometryShaderObject(1);
    }
    void ReleaseGeometryShader(NVRenderBackendGeometryShaderObject) override {}
    NVRenderBackendComputeShaderObject CreateComputeShader(NVConstDataRef<QT3DSI8>,
                                                                   eastl::string &, bool) override
    {
        return NVRenderBackendComputeShaderObject(1);
    }
    void ReleaseComputeShader(NVRenderBackendComputeShaderObject) override {}
    void AttachShader(NVRenderBackendShaderProgramObject, NVRenderBackendVertexShaderObject) override
    {
    }
    void AttachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendFragmentShaderObject) override
    {
    }
    void AttachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendTessControlShaderObject) override
    {
    }
    void AttachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendTessEvaluationShaderObject) override
    {
    }
    void AttachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendGeometryShaderObject) override
    {
    }
    void AttachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendComputeShaderObject) override
    {
    }
    void DetachShader(NVRenderBackendShaderProgramObject, NVRenderBackendVertexShaderObject) override
    {
    }
    void DetachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendFragmentShaderObject) override
    {
    }
    void DetachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendTessControlShaderObject) override
    {
    }
    void DetachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendTessEvaluationShaderObject) override
    {
    }
    void DetachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendGeometryShaderObject) override
    {
    }
    void DetachShader(NVRenderBackendShaderProgramObject,
                              NVRenderBackendComputeShaderObject) override
    {
    }
    NVRenderBackendShaderProgramObject CreateShaderProgram(bool) override
    {
        return NVRenderBackendShaderProgramObject(1);
    }
    void ReleaseShaderProgram(NVRenderBackendShaderProgramObject) override {}
    NVRenderBackendProgramPipeline CreateProgramPipeline() override
    {
        return NVRenderBackendProgramPipeline(1);
    }
    void ReleaseProgramPipeline(NVRenderBackendProgramPipeline) override {}

    bool LinkProgram(NVRenderBackendShaderProgramObject, eastl::string &) override { return false; }
    void SetActiveProgram(NVRenderBackendShaderProgramObject) override {}
    void SetActiveProgramPipeline(NVRenderBackendProgramPipeline) override {}
    void SetProgramStages(NVRenderBackendProgramPipeline, NVRenderShaderTypeFlags,
                                  NVRenderBackendShaderProgramObject) override {}
    void DispatchCompute(NVRenderBackendShaderProgramObject, QT3DSU32, QT3DSU32, QT3DSU32) override {}
    QT3DSI32 GetConstantCount(NVRenderBackendShaderProgramObject) override { return 0; }
    QT3DSI32 GetConstantBufferCount(NVRenderBackendShaderProgramObject) override { return 0; }
    QT3DSI32 GetConstantInfoByID(NVRenderBackendShaderProgramObject, QT3DSU32, QT3DSU32, QT3DSI32 *,
                                      NVRenderShaderDataTypes::Enum *, QT3DSI32 *, char *) override
    {
        return 0;
    }

    QT3DSI32 GetConstantBufferInfoByID(NVRenderBackendShaderProgramObject, QT3DSU32, QT3DSU32,
                                            QT3DSI32 *, QT3DSI32 *, QT3DSI32 *, char *) override
    {
        return 0;
    }

    void GetConstantBufferParamIndices(NVRenderBackendShaderProgramObject, QT3DSU32, QT3DSI32 *) override
    {
    }
    void GetConstantBufferParamInfoByIndices(NVRenderBackendShaderProgramObject, QT3DSU32,
                                                     QT3DSU32 *, QT3DSI32 *, QT3DSI32 *, QT3DSI32 *) override {}
    void ProgramSetConstantBlock(NVRenderBackendShaderProgramObject, QT3DSU32, QT3DSU32) override {}
    void ProgramSetConstantBuffer(QT3DSU32, NVRenderBackendBufferObject) override {}

    QT3DSI32 GetStorageBufferCount(NVRenderBackendShaderProgramObject) override { return 0; };
    QT3DSI32 GetStorageBufferInfoByID(NVRenderBackendShaderProgramObject, QT3DSU32, QT3DSU32,
                                           QT3DSI32 *, QT3DSI32 *, QT3DSI32 *, char *) override
    {
        return -1;
    }
    void ProgramSetStorageBuffer(QT3DSU32, NVRenderBackendBufferObject) override {}

    QT3DSI32 GetAtomicCounterBufferCount(NVRenderBackendShaderProgramObject) override { return 0; }
    QT3DSI32 GetAtomicCounterBufferInfoByID(NVRenderBackendShaderProgramObject, QT3DSU32, QT3DSU32,
                                                 QT3DSI32 *, QT3DSI32 *, QT3DSI32 *, char *) override
    {
        return -1;
    };
    void ProgramSetAtomicCounterBuffer(QT3DSU32, NVRenderBackendBufferObject) override {}

    void SetConstantValue(NVRenderBackendShaderProgramObject, QT3DSU32,
                                  NVRenderShaderDataTypes::Enum, QT3DSI32, const void *, bool) override
    {
    }

    void Draw(NVRenderDrawMode::Enum, QT3DSU32, QT3DSU32) override {}
    void DrawIndirect(NVRenderDrawMode::Enum, const void *) override {}

    void DrawIndexed(NVRenderDrawMode::Enum, QT3DSU32, NVRenderComponentTypes::Enum,
                             const void *) override
    {
    }
    void DrawIndexedIndirect(NVRenderDrawMode::Enum, NVRenderComponentTypes::Enum,
                                     const void *) override
    {
    }

    void ReadPixel(NVRenderBackendRenderTargetObject, QT3DSI32, QT3DSI32, QT3DSI32, QT3DSI32,
                           NVRenderReadPixelFormats::Enum, void *) override
    {
    }

    NVRenderBackendPathObject CreatePathNVObject(size_t) override
    {
        return NVRenderBackendPathObject(1);
    };
    void SetPathSpecification(NVRenderBackendPathObject, NVConstDataRef<QT3DSU8>,
                                      NVConstDataRef<QT3DSF32>) override
    {
    }

    ///< Bounds of the fill and stroke
    NVBounds3 GetPathObjectBoundingBox(NVRenderBackendPathObject /*inPathObject*/) override
    {
        return NVBounds3();
    }
    NVBounds3 GetPathObjectFillBox(NVRenderBackendPathObject /*inPathObject*/) override
    {
        return NVBounds3();
    }
    NVBounds3 GetPathObjectStrokeBox(NVRenderBackendPathObject /*inPathObject*/) override
    {
        return NVBounds3();
    }

    /**
     *	Defaults to 0 if unset.
     */
    void SetStrokeWidth(NVRenderBackendPathObject /*inPathObject*/, QT3DSF32) override {}
    void SetPathProjectionMatrix(const QT3DSMat44 /*inPathProjection*/) override {}
    void SetPathModelViewMatrix(const QT3DSMat44 /*inPathModelview*/) override {}

    void SetPathStencilDepthOffset(QT3DSF32 /*inSlope*/, QT3DSF32 /*inBias*/) override {}
    void SetPathCoverDepthFunc(NVRenderBoolOp::Enum /*inDepthFunction*/) override {}
    void StencilStrokePath(NVRenderBackendPathObject /*inPathObject*/) override {}
    void StencilFillPath(NVRenderBackendPathObject /*inPathObject*/) override {}
    void ReleasePathNVObject(NVRenderBackendPathObject, size_t) override {}

    void LoadPathGlyphs(NVRenderBackendPathObject, NVRenderPathFontTarget::Enum,
                                const void *, NVRenderPathFontStyleFlags, size_t,
                                NVRenderPathFormatType::Enum, const void *,
                                NVRenderPathMissingGlyphs::Enum, NVRenderBackendPathObject, QT3DSF32) override
    {
    }
    virtual NVRenderPathReturnValues::Enum
    LoadPathGlyphsIndexed(NVRenderBackendPathObject, NVRenderPathFontTarget::Enum, const void *,
                          NVRenderPathFontStyleFlags, QT3DSU32, size_t, NVRenderBackendPathObject,
                          QT3DSF32) override
    {
        return NVRenderPathReturnValues::FontUnavailable;
    }
    NVRenderBackendPathObject LoadPathGlyphsIndexedRange(NVRenderPathFontTarget::Enum,
                                                                 const void *,
                                                                 NVRenderPathFontStyleFlags,
                                                                 NVRenderBackendPathObject, QT3DSF32,
                                                                 QT3DSU32 *) override
    {
        return NVRenderBackendPathObject(1);
    }
    void LoadPathGlyphRange(NVRenderBackendPathObject, NVRenderPathFontTarget::Enum,
                                    const void *, NVRenderPathFontStyleFlags, QT3DSU32, size_t,
                                    NVRenderPathMissingGlyphs::Enum, NVRenderBackendPathObject,
                                    QT3DSF32) override
    {
    }
    void GetPathMetrics(NVRenderBackendPathObject, size_t, NVRenderPathGlyphFontMetricFlags,
                                NVRenderPathFormatType::Enum, const void *, size_t, QT3DSF32 *) override
    {
    }
    void GetPathMetricsRange(NVRenderBackendPathObject, size_t,
                                     NVRenderPathGlyphFontMetricFlags, size_t, QT3DSF32 *) override
    {
    }
    void GetPathSpacing(NVRenderBackendPathObject, size_t, NVRenderPathListMode::Enum,
                                NVRenderPathFormatType::Enum, const void *, QT3DSF32, QT3DSF32,
                                NVRenderPathTransformType::Enum, QT3DSF32 *) override
    {
    }

    void StencilFillPathInstanced(NVRenderBackendPathObject, size_t,
                                          NVRenderPathFormatType::Enum, const void *,
                                          NVRenderPathFillMode::Enum, QT3DSU32,
                                          NVRenderPathTransformType::Enum, const QT3DSF32 *) override
    {
    }
    void StencilStrokePathInstancedN(NVRenderBackendPathObject, size_t,
                                             NVRenderPathFormatType::Enum, const void *, QT3DSI32,
                                             QT3DSU32, NVRenderPathTransformType::Enum, const QT3DSF32 *) override
    {
    }
    void CoverFillPathInstanced(NVRenderBackendPathObject, size_t,
                                        NVRenderPathFormatType::Enum, const void *,
                                        NVRenderPathCoverMode::Enum,
                                        NVRenderPathTransformType::Enum, const QT3DSF32 *) override
    {
    }
    void CoverStrokePathInstanced(NVRenderBackendPathObject, size_t,
                                          NVRenderPathFormatType::Enum, const void *,
                                          NVRenderPathCoverMode::Enum,
                                          NVRenderPathTransformType::Enum, const QT3DSF32 *) override
    {
    }
    QSurfaceFormat format() const override
    {
        return QSurfaceFormat();
    }
};
}

NVRenderBackend &NVRenderBackendNULL::CreateBackend(NVFoundationBase &foundation)
{
    return *QT3DS_NEW(foundation.getAllocator(), SNullBackend)(foundation);
}
