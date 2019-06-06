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
#pragma once
#ifndef QT3DS_RENDER_BACKEND_GL_BASE_H
#define QT3DS_RENDER_BACKEND_GL_BASE_H

/// @file Qt3DSRenderBackendGLBase.h
///       NVRender OpenGL Core backend definition.

#include "foundation/Qt3DSContainers.h"
#include "foundation/StringTable.h"
#include "render/backends/Qt3DSRenderBackend.h"
#include "render/backends/gl/Qt3DSOpenGLUtil.h"
#include <EASTL/string.h>

#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLFunctions>
#include <QtOpenGLExtensions/QtOpenGLExtensions>

#define NVRENDER_BACKEND_UNUSED(arg) (void)arg;

// Enable this to log opengl errors instead of an assert
//#define RENDER_BACKEND_LOG_GL_ERRORS

namespace qt3ds {
namespace render {

    ///< forward declaration
    class NVRenderBackendRasterizerStateGL;
    class NVRenderBackendDepthStencilStateGL;

    using namespace foundation;

    typedef eastl::basic_string<char8_t, ForwardingAllocator> TContextStr;

    class NVRenderBackendGLBase : public NVRenderBackend
    {
    public:
        /// constructor
        NVRenderBackendGLBase(NVFoundationBase &fnd,
            qt3ds::foundation::IStringTable &stringTable,
            const QSurfaceFormat &format);
        /// destructor
        virtual ~NVRenderBackendGLBase();

    public:
        /// API Interface
        NVRenderContextType GetRenderContextType() const override;
        bool isESCompatible() const;

        const char *GetShadingLanguageVersion() override;
        /// get implementation depended values
        QT3DSU32 GetMaxCombinedTextureUnits() override;
        bool GetRenderBackendCap(NVRenderBackendCaps::Enum inCap) const override;
        QT3DSU32 GetDepthBits() const override;
        QT3DSU32 GetStencilBits() const override;
        void GetRenderBackendValue(NVRenderBackendQuery::Enum inQuery, QT3DSI32 *params) const override;

        /// state get/set functions
        void SetRenderState(bool bEnable, const NVRenderState::Enum value) override;
        bool GetRenderState(const NVRenderState::Enum value) override;
        virtual NVRenderBackendDepthStencilStateObject
        CreateDepthStencilState(bool enableDepth, bool depthMask, NVRenderBoolOp::Enum depthFunc,
                                bool enableStencil,
                                NVRenderStencilFunctionArgument &stencilFuncFront,
                                NVRenderStencilFunctionArgument &stencilFuncBack,
                                NVRenderStencilOperationArgument &depthStencilOpFront,
                                NVRenderStencilOperationArgument &depthStencilOpBack) override;
        virtual void
        ReleaseDepthStencilState(NVRenderBackendDepthStencilStateObject inDepthStencilState) override;
        virtual NVRenderBackendRasterizerStateObject
        CreateRasterizerState(QT3DSF32 depthBias, QT3DSF32 depthScale, NVRenderFaces::Enum cullFace) override;
        void ReleaseRasterizerState(NVRenderBackendRasterizerStateObject rasterizerState) override;
        virtual void
        SetDepthStencilState(NVRenderBackendDepthStencilStateObject inDepthStencilState) override;
        void SetRasterizerState(NVRenderBackendRasterizerStateObject rasterizerState) override;
        NVRenderBoolOp::Enum GetDepthFunc() override;
        void SetDepthFunc(const NVRenderBoolOp::Enum func) override;
        bool GetDepthWrite() override;
        void SetDepthWrite(bool bEnable) override;
        void SetColorWrites(bool bRed, bool bGreen, bool bBlue, bool bAlpha) override;
        void SetMultisample(bool bEnable) override;
        void GetBlendFunc(NVRenderBlendFunctionArgument *pBlendFuncArg) override;
        void SetBlendFunc(const NVRenderBlendFunctionArgument &blendFuncArg) override;
        void SetBlendEquation(const NVRenderBlendEquationArgument &pBlendEquArg) override;
        void SetBlendBarrier(void) override;
        void GetScissorRect(NVRenderRect *pRect) override;
        void SetScissorRect(const NVRenderRect &rect) override;
        void GetViewportRect(NVRenderRect *pRect) override;
        void SetViewportRect(const NVRenderRect &rect) override;

        void SetClearColor(const QT3DSVec4 *pClearColor) override;
        void Clear(NVRenderClearFlags flags) override;

        /// resource handling
        NVRenderBackendBufferObject CreateBuffer(size_t size,
                                                         NVRenderBufferBindFlags bindFlags,
                                                         NVRenderBufferUsageType::Enum usage,
                                                         const void *hostPtr = NULL) override;
        void BindBuffer(NVRenderBackendBufferObject bo, NVRenderBufferBindFlags bindFlags) override;
        void ReleaseBuffer(NVRenderBackendBufferObject bo) override;
        void UpdateBuffer(NVRenderBackendBufferObject bo, NVRenderBufferBindFlags bindFlags,
                                  size_t size, NVRenderBufferUsageType::Enum usage,
                                  const void *data) override;
        void UpdateBufferRange(NVRenderBackendBufferObject bo,
                                       NVRenderBufferBindFlags bindFlags, size_t offset,
                                       size_t size, const void *data) override;
        void *MapBuffer(NVRenderBackendBufferObject bo, NVRenderBufferBindFlags bindFlags,
                                size_t offset, size_t length,
                                NVRenderBufferAccessFlags accessFlags) override;
        bool UnmapBuffer(NVRenderBackendBufferObject bo, NVRenderBufferBindFlags bindFlags) override;
        void SetMemoryBarrier(NVRenderBufferBarrierFlags barriers) override;

        NVRenderBackendQueryObject CreateQuery() override;
        void ReleaseQuery(NVRenderBackendQueryObject qo) override;
        void BeginQuery(NVRenderBackendQueryObject qo, NVRenderQueryType::Enum type) override;
        void EndQuery(NVRenderBackendQueryObject qo, NVRenderQueryType::Enum type) override;
        void GetQueryResult(NVRenderBackendQueryObject qo,
                                    NVRenderQueryResultType::Enum resultType, QT3DSU32 *params) override;
        void GetQueryResult(NVRenderBackendQueryObject qo,
                                    NVRenderQueryResultType::Enum resultType, QT3DSU64 *params) override;
        void SetQueryTimer(NVRenderBackendQueryObject qo) override;

        NVRenderBackendSyncObject CreateSync(NVRenderSyncType::Enum tpye,
                                                     NVRenderSyncFlags syncFlags) override;
        void ReleaseSync(NVRenderBackendSyncObject so) override;
        void WaitSync(NVRenderBackendSyncObject so, NVRenderCommandFlushFlags syncFlags,
                              QT3DSU64 timeout) override;

        NVRenderBackendRenderTargetObject CreateRenderTarget() override;
        void ReleaseRenderTarget(NVRenderBackendRenderTargetObject rto) override;
        void RenderTargetAttach(NVRenderBackendRenderTargetObject rto,
                                        NVRenderFrameBufferAttachments::Enum attachment,
                                        NVRenderBackendRenderbufferObject rbo) override;
        void RenderTargetAttach(
            NVRenderBackendRenderTargetObject rto, NVRenderFrameBufferAttachments::Enum attachment,
            NVRenderBackendTextureObject to,
            NVRenderTextureTargetType::Enum target = NVRenderTextureTargetType::Texture2D) override;
        void RenderTargetAttach(NVRenderBackendRenderTargetObject rto,
                                        NVRenderFrameBufferAttachments::Enum attachment,
                                        NVRenderBackendTextureObject to, QT3DSI32 level, QT3DSI32 layer) override;
        void SetRenderTarget(NVRenderBackendRenderTargetObject rto) override;
        bool RenderTargetIsValid(NVRenderBackendRenderTargetObject rto) override;

        virtual NVRenderBackendRenderbufferObject
        CreateRenderbuffer(NVRenderRenderBufferFormats::Enum storageFormat, size_t width,
                           size_t height) override;
        void ReleaseRenderbuffer(NVRenderBackendRenderbufferObject rbo) override;
        bool ResizeRenderbuffer(NVRenderBackendRenderbufferObject rbo,
                                        NVRenderRenderBufferFormats::Enum storageFormat,
                                        size_t width, size_t height) override;

        NVRenderBackendTextureObject CreateTexture() override;
        void BindTexture(NVRenderBackendTextureObject to,
                                 NVRenderTextureTargetType::Enum target, QT3DSU32 unit) override;
        void BindImageTexture(NVRenderBackendTextureObject to, QT3DSU32 unit, QT3DSI32 level,
                                      bool layered, QT3DSI32 layer,
                                      NVRenderImageAccessType::Enum access,
                                      NVRenderTextureFormats::Enum format) override;
        void ReleaseTexture(NVRenderBackendTextureObject to) override;
        void SetTextureData2D(NVRenderBackendTextureObject to,
                                      NVRenderTextureTargetType::Enum target, QT3DSU32 level,
                                      NVRenderTextureFormats::Enum internalFormat, size_t width,
                                      size_t height, QT3DSI32 border,
                                      NVRenderTextureFormats::Enum format,
                                      const void *hostPtr = NULL) override;
        void SetTextureDataCubeFace(NVRenderBackendTextureObject to,
                                            NVRenderTextureTargetType::Enum target, QT3DSU32 level,
                                            NVRenderTextureFormats::Enum internalFormat,
                                            size_t width, size_t height, QT3DSI32 border,
                                            NVRenderTextureFormats::Enum format,
                                            const void *hostPtr = NULL) override;
        void CreateTextureStorage2D(NVRenderBackendTextureObject to,
                                            NVRenderTextureTargetType::Enum target, QT3DSU32 levels,
                                            NVRenderTextureFormats::Enum internalFormat,
                                            size_t width, size_t height) override;
        void SetTextureSubData2D(NVRenderBackendTextureObject to,
                                         NVRenderTextureTargetType::Enum target, QT3DSU32 level,
                                         QT3DSI32 xOffset, QT3DSI32 yOffset, size_t width, size_t height,
                                         NVRenderTextureFormats::Enum format,
                                         const void *hostPtr = NULL) override;
        void SetCompressedTextureData2D(NVRenderBackendTextureObject to,
                                                NVRenderTextureTargetType::Enum target, QT3DSU32 level,
                                                NVRenderTextureFormats::Enum internalFormat,
                                                size_t width, size_t height, QT3DSI32 border,
                                                size_t imageSize, const void *hostPtr = NULL) override;
        void SetCompressedTextureDataCubeFace(NVRenderBackendTextureObject to,
                                                      NVRenderTextureTargetType::Enum target,
                                                      QT3DSU32 level,
                                                      NVRenderTextureFormats::Enum internalFormat,
                                                      size_t width, size_t height, QT3DSI32 border,
                                                      size_t imageSize, const void *hostPtr = NULL) override;
        void SetCompressedTextureSubData2D(NVRenderBackendTextureObject to,
                                                   NVRenderTextureTargetType::Enum target,
                                                   QT3DSU32 level, QT3DSI32 xOffset, QT3DSI32 yOffset,
                                                   size_t width, size_t height,
                                                   NVRenderTextureFormats::Enum format,
                                                   size_t imageSize, const void *hostPtr = NULL) override;
        void SetMultisampledTextureData2D(NVRenderBackendTextureObject to,
                                                  NVRenderTextureTargetType::Enum target,
                                                  size_t samples,
                                                  NVRenderTextureFormats::Enum internalFormat,
                                                  size_t width, size_t height,
                                                  bool fixedsamplelocations) override = 0;

        void SetTextureData3D(NVRenderBackendTextureObject to,
                                      NVRenderTextureTargetType::Enum target, QT3DSU32 level,
                                      NVRenderTextureFormats::Enum internalFormat, size_t width,
                                      size_t height, size_t depth, QT3DSI32 border,
                                      NVRenderTextureFormats::Enum format,
                                      const void *hostPtr = NULL) override;

        void GenerateMipMaps(NVRenderBackendTextureObject to,
                                     NVRenderTextureTargetType::Enum target,
                                     NVRenderHint::Enum genType) override;

        virtual NVRenderTextureSwizzleMode::Enum
        GetTextureSwizzleMode(const NVRenderTextureFormats::Enum inFormat) const override;

        NVRenderBackendSamplerObject CreateSampler(
            NVRenderTextureMinifyingOp::Enum minFilter = NVRenderTextureMinifyingOp::Linear,
            NVRenderTextureMagnifyingOp::Enum magFilter = NVRenderTextureMagnifyingOp::Linear,
            NVRenderTextureCoordOp::Enum wrapS = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureCoordOp::Enum wrapT = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureCoordOp::Enum wrapR = NVRenderTextureCoordOp::ClampToEdge,
            QT3DSI32 minLod = -1000, QT3DSI32 maxLod = 1000, QT3DSF32 lodBias = 0.0,
            NVRenderTextureCompareMode::Enum compareMode = NVRenderTextureCompareMode::NoCompare,
            NVRenderTextureCompareOp::Enum compareFunc = NVRenderTextureCompareOp::LessThanOrEqual,
            QT3DSF32 anisotropy = 1.0, QT3DSF32 *borderColor = NULL) override;

        void UpdateSampler(
            NVRenderBackendSamplerObject so, NVRenderTextureTargetType::Enum target,
            NVRenderTextureMinifyingOp::Enum minFilter = NVRenderTextureMinifyingOp::Linear,
            NVRenderTextureMagnifyingOp::Enum magFilter = NVRenderTextureMagnifyingOp::Linear,
            NVRenderTextureCoordOp::Enum wrapS = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureCoordOp::Enum wrapT = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureCoordOp::Enum wrapR = NVRenderTextureCoordOp::ClampToEdge,
            QT3DSF32 minLod = -1000.0, QT3DSF32 maxLod = 1000.0, QT3DSF32 lodBias = 0.0,
            NVRenderTextureCompareMode::Enum compareMode = NVRenderTextureCompareMode::NoCompare,
            NVRenderTextureCompareOp::Enum compareFunc = NVRenderTextureCompareOp::LessThanOrEqual,
            QT3DSF32 anisotropy = 1.0, QT3DSF32 *borderColor = NULL) override;

        void UpdateTextureObject(NVRenderBackendTextureObject to,
                                         NVRenderTextureTargetType::Enum target, QT3DSI32 baseLevel,
                                         QT3DSI32 maxLevel) override;

        void UpdateTextureSwizzle(NVRenderBackendTextureObject to,
                                          NVRenderTextureTargetType::Enum target,
                                          NVRenderTextureSwizzleMode::Enum swizzleMode) override;

        void ReleaseSampler(NVRenderBackendSamplerObject so) override;

        virtual NVRenderBackendAttribLayoutObject
        CreateAttribLayout(NVConstDataRef<NVRenderVertexBufferEntry> attribs) override;
        void ReleaseAttribLayout(NVRenderBackendAttribLayoutObject ao) override;

        virtual NVRenderBackendInputAssemblerObject
        CreateInputAssembler(NVRenderBackendAttribLayoutObject attribLayout,
                             NVConstDataRef<NVRenderBackendBufferObject> buffers,
                             const NVRenderBackendBufferObject indexBuffer,
                             NVConstDataRef<QT3DSU32> strides, NVConstDataRef<QT3DSU32> offsets,
                             QT3DSU32 patchVertexCount) override;
        void ReleaseInputAssembler(NVRenderBackendInputAssemblerObject iao) override;

        bool SetInputAssembler(NVRenderBackendInputAssemblerObject iao,
                                       NVRenderBackendShaderProgramObject po) override = 0;
        void SetPatchVertexCount(NVRenderBackendInputAssemblerObject, QT3DSU32) override
        {
            QT3DS_ASSERT(false);
        }

        // shader
        virtual NVRenderBackendVertexShaderObject
        CreateVertexShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage, bool binary) override;
        virtual NVRenderBackendFragmentShaderObject
        CreateFragmentShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage, bool binary) override;
        virtual NVRenderBackendTessControlShaderObject
        CreateTessControlShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage,
                                bool binary) override;
        virtual NVRenderBackendTessEvaluationShaderObject
        CreateTessEvaluationShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage,
                                   bool binary) override;
        virtual NVRenderBackendGeometryShaderObject
        CreateGeometryShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage, bool binary) override;
        virtual NVRenderBackendComputeShaderObject
        CreateComputeShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage, bool binary) override;
        void ReleaseVertexShader(NVRenderBackendVertexShaderObject vso) override;
        void ReleaseFragmentShader(NVRenderBackendFragmentShaderObject fso) override;
        void ReleaseTessControlShader(NVRenderBackendTessControlShaderObject tcso) override;
        void ReleaseTessEvaluationShader(NVRenderBackendTessEvaluationShaderObject teso) override;
        void ReleaseGeometryShader(NVRenderBackendGeometryShaderObject gso) override;
        void ReleaseComputeShader(NVRenderBackendComputeShaderObject cso) override;
        void AttachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendVertexShaderObject vso) override;
        void AttachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendFragmentShaderObject fso) override;
        void AttachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendTessControlShaderObject tcso) override;
        void AttachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendTessEvaluationShaderObject teso) override;
        void AttachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendGeometryShaderObject gso) override;
        void AttachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendComputeShaderObject cso) override;
        void DetachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendVertexShaderObject vso) override;
        void DetachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendFragmentShaderObject fso) override;
        void DetachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendTessControlShaderObject tcso) override;
        void DetachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendTessEvaluationShaderObject teso) override;
        void DetachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendGeometryShaderObject gso) override;
        void DetachShader(NVRenderBackendShaderProgramObject po,
                                  NVRenderBackendComputeShaderObject cso) override;
        NVRenderBackendShaderProgramObject CreateShaderProgram(bool isSeparable) override;
        void ReleaseShaderProgram(NVRenderBackendShaderProgramObject po) override;
        bool LinkProgram(NVRenderBackendShaderProgramObject po,
                                 eastl::string &errorMessage) override;
        void SetActiveProgram(NVRenderBackendShaderProgramObject po) override;
        void DispatchCompute(NVRenderBackendShaderProgramObject po, QT3DSU32 numGroupsX,
                                     QT3DSU32 numGroupsY, QT3DSU32 numGroupsZ) override;
        NVRenderBackendProgramPipeline CreateProgramPipeline() override;
        void ReleaseProgramPipeline(NVRenderBackendProgramPipeline po) override;
        void SetActiveProgramPipeline(NVRenderBackendProgramPipeline po) override;
        void SetProgramStages(NVRenderBackendProgramPipeline ppo,
                                      NVRenderShaderTypeFlags flags,
                                      NVRenderBackendShaderProgramObject po) override;

        // uniforms
        QT3DSI32 GetConstantCount(NVRenderBackendShaderProgramObject po) override;
        QT3DSI32 GetConstantInfoByID(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                          QT3DSU32 bufSize, QT3DSI32 *numElem,
                                          NVRenderShaderDataTypes::Enum *type, QT3DSI32 *binding,
                                          char *nameBuf) override;
        void SetConstantValue(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                      NVRenderShaderDataTypes::Enum type, QT3DSI32 count,
                                      const void *value, bool transpose) override;

        // uniform buffers
        QT3DSI32 GetConstantBufferCount(NVRenderBackendShaderProgramObject po) override;
        QT3DSI32 GetConstantBufferInfoByID(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                                QT3DSU32 nameBufSize, QT3DSI32 *paramCount,
                                                QT3DSI32 *bufferSize, QT3DSI32 *length, char *nameBuf) override;
        void GetConstantBufferParamIndices(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                                   QT3DSI32 *indices) override;
        void GetConstantBufferParamInfoByIndices(NVRenderBackendShaderProgramObject po,
                                                         QT3DSU32 count, QT3DSU32 *indices, QT3DSI32 *type,
                                                         QT3DSI32 *size, QT3DSI32 *offset) override;
        void ProgramSetConstantBlock(NVRenderBackendShaderProgramObject po,
                                             QT3DSU32 blockIndex, QT3DSU32 binding) override;
        void ProgramSetConstantBuffer(QT3DSU32 index, NVRenderBackendBufferObject bo) override;

        // storage buffers
        QT3DSI32 GetStorageBufferCount(NVRenderBackendShaderProgramObject po) override;
        QT3DSI32 GetStorageBufferInfoByID(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                               QT3DSU32 nameBufSize, QT3DSI32 *paramCount,
                                               QT3DSI32 *bufferSize, QT3DSI32 *length, char *nameBuf) override;
        void ProgramSetStorageBuffer(QT3DSU32 index, NVRenderBackendBufferObject bo) override;

        // atomic counter buffers
        QT3DSI32 GetAtomicCounterBufferCount(NVRenderBackendShaderProgramObject po) override;
        QT3DSI32 GetAtomicCounterBufferInfoByID(NVRenderBackendShaderProgramObject po,
                                                     QT3DSU32 id, QT3DSU32 nameBufSize, QT3DSI32 *paramCount,
                                                     QT3DSI32 *bufferSize, QT3DSI32 *length,
                                                     char *nameBuf) override;
        void ProgramSetAtomicCounterBuffer(QT3DSU32 index, NVRenderBackendBufferObject bo) override;

        /// draw calls
        void Draw(NVRenderDrawMode::Enum drawMode, QT3DSU32 start, QT3DSU32 count) override;
        void DrawIndirect(NVRenderDrawMode::Enum drawMode, const void *indirect) override;
        void DrawIndexed(NVRenderDrawMode::Enum drawMode, QT3DSU32 count,
                                 NVRenderComponentTypes::Enum type, const void *indices) override;
        void DrawIndexedIndirect(NVRenderDrawMode::Enum drawMode,
                                         NVRenderComponentTypes::Enum type, const void *indirect) override;

        // read calls
        void ReadPixel(NVRenderBackendRenderTargetObject rto, QT3DSI32 x, QT3DSI32 y, QT3DSI32 width,
                               QT3DSI32 height, NVRenderReadPixelFormats::Enum inFormat, void *pixels) override;

        // NV path rendering
        NVRenderBackendPathObject CreatePathNVObject(size_t range) override;
        // Pathing requires gl4 backend.
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
        void ReleasePathNVObject(NVRenderBackendPathObject po, size_t range) override;

        void LoadPathGlyphs(NVRenderBackendPathObject, NVRenderPathFontTarget::Enum,
                                    const void *, NVRenderPathFontStyleFlags, size_t,
                                    NVRenderPathFormatType::Enum, const void *,
                                    NVRenderPathMissingGlyphs::Enum, NVRenderBackendPathObject,
                                    QT3DSF32) override;
        virtual NVRenderPathReturnValues::Enum
        LoadPathGlyphsIndexed(NVRenderBackendPathObject po, NVRenderPathFontTarget::Enum fontTarget,
                              const void *fontName, NVRenderPathFontStyleFlags fontStyle,
                              QT3DSU32 firstGlyphIndex, size_t numGlyphs,
                              NVRenderBackendPathObject pathParameterTemplate, QT3DSF32 emScale) override;
        virtual NVRenderBackendPathObject
        LoadPathGlyphsIndexedRange(NVRenderPathFontTarget::Enum, const void *,
                                   NVRenderPathFontStyleFlags,
                                   NVRenderBackend::NVRenderBackendPathObject, QT3DSF32, QT3DSU32 *) override;
        void LoadPathGlyphRange(NVRenderBackendPathObject, NVRenderPathFontTarget::Enum,
                                        const void *, NVRenderPathFontStyleFlags, QT3DSU32, size_t,
                                        NVRenderPathMissingGlyphs::Enum, NVRenderBackendPathObject,
                                        QT3DSF32) override;
        void GetPathMetrics(NVRenderBackendPathObject, size_t,
                                    NVRenderPathGlyphFontMetricFlags, NVRenderPathFormatType::Enum,
                                    const void *, size_t, QT3DSF32 *) override;
        void GetPathMetricsRange(NVRenderBackendPathObject, size_t,
                                         NVRenderPathGlyphFontMetricFlags, size_t, QT3DSF32 *) override;
        void GetPathSpacing(NVRenderBackendPathObject, size_t, NVRenderPathListMode::Enum,
                                    NVRenderPathFormatType::Enum, const void *, QT3DSF32, QT3DSF32,
                                    NVRenderPathTransformType::Enum, QT3DSF32 *) override;

        void StencilFillPathInstanced(NVRenderBackendPathObject, size_t,
                                              NVRenderPathFormatType::Enum, const void *,
                                              NVRenderPathFillMode::Enum, QT3DSU32,
                                              NVRenderPathTransformType::Enum, const QT3DSF32 *) override;
        void StencilStrokePathInstancedN(NVRenderBackendPathObject, size_t,
                                                 NVRenderPathFormatType::Enum, const void *, QT3DSI32,
                                                 QT3DSU32, NVRenderPathTransformType::Enum,
                                                 const QT3DSF32 *) override;
        void CoverFillPathInstanced(NVRenderBackendPathObject, size_t,
                                            NVRenderPathFormatType::Enum, const void *,
                                            NVRenderPathCoverMode::Enum,
                                            NVRenderPathTransformType::Enum, const QT3DSF32 *) override;
        void CoverStrokePathInstanced(NVRenderBackendPathObject, size_t,
                                              NVRenderPathFormatType::Enum, const void *,
                                              NVRenderPathCoverMode::Enum,
                                              NVRenderPathTransformType::Enum, const QT3DSF32 *) override;

        QSurfaceFormat format() const override
        {
            return m_format;
        }

    protected:
        virtual NVFoundationBase &GetFoundation() { return m_Foundation; }
        virtual bool compileSource(GLuint shaderID, NVConstDataRef<QT3DSI8> source,
                                   eastl::string &errorMessage, bool binary);
        virtual const char *getVersionString();
        virtual const char *getVendorString();
        virtual const char *getRendererString();
        virtual const char *getExtensionString();

        virtual void setAndInspectHardwareCaps();

    protected:
        volatile QT3DSI32 mRefCount; ///< reference count
        NVFoundationBase &m_Foundation; ///< Foundation class for allocations and other base things
        NVScopedRefCounted<qt3ds::foundation::IStringTable>
            m_StringTable; ///< pointer to a string table
        GLConversion m_Conversion; ///< Class for conversion from base type to GL types
        QStringList m_extensions; ///< contains the OpenGL extension string
        QT3DSI32 m_MaxAttribCount; ///< Maximum attributes which can be used
        nvvector<GLenum> m_DrawBuffersArray; ///< Contains the drawbuffer enums
        QSurfaceFormat m_format;

        NVRenderBackendRasterizerStateGL
            *m_pCurrentRasterizerState; ///< this holds the current rasterizer state
        NVRenderBackendDepthStencilStateGL
            *m_pCurrentDepthStencilState; ///< this holds the current depth stencil state

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
        void checkGLError(const char *function, const char *file, const unsigned int line) const;
#else
        void checkGLError() const;
#endif
        QOpenGLFunctions *m_glFunctions;
        QOpenGLExtraFunctions *m_glExtraFunctions;
        GLfloat m_maxAnisotropy;
    };
}
}

#endif
