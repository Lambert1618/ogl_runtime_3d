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
#ifndef QT3DS_RENDER_CONTEXT_H
#define QT3DS_RENDER_CONTEXT_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSRefCounted.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "render/glg/Qt3DSGLImplObjects.h"
#include "render/backends/gl/Q3DSRenderBackendGLES2.h"
#include "render/backends/gl/Qt3DSRenderBackendGL3.h"
#include "render/backends/gl/Qt3DSRenderBackendGL4.h"
#include "render/backends/software/Qt3DSRenderBackendNULL.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "render/Qt3DSRenderIndexBuffer.h"
#include "render/Qt3DSRenderConstantBuffer.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "render/Qt3DSRenderRenderBuffer.h"
#include "render/Qt3DSRenderDepthStencilState.h"
#include "render/Qt3DSRenderRasterizerState.h"
#include "render/Qt3DSRenderDrawable.h"
#include "render/Qt3DSRenderInputAssembler.h"
#include "render/Qt3DSRenderAttribLayout.h"
#include "render/Qt3DSRenderImageTexture.h"
#include "render/Qt3DSRenderOcclusionQuery.h"
#include "render/Qt3DSRenderTimerQuery.h"
#include "render/Qt3DSRenderSync.h"
#include "render/Qt3DSRenderTexture2DArray.h"
#include "render/Qt3DSRenderTextureCube.h"
#include "render/Qt3DSRenderStorageBuffer.h"
#include "render/Qt3DSRenderAtomicCounterBuffer.h"
#include "render/Qt3DSRenderDrawIndirectBuffer.h"
#include "render/Qt3DSRenderProgramPipeline.h"
#include "render/Qt3DSRenderPathRender.h"
#include "render/Qt3DSRenderPathSpecification.h"
#include "render/Qt3DSRenderPathFontSpecification.h"
#include "render/Qt3DSRenderPathFontText.h"

#include <QSurfaceFormat>

// When SW fallback is defined we can support (some) object/layer advanced blend modes. If defined,
// the HW implementation is still preperred if available through extensions. SW fallback can't be
// used in custom shaders.
#define ADVANCED_BLEND_SW_FALLBACK
namespace qt3ds {
class NVFoundationBase;
}

namespace qt3ds {
namespace render {

    using namespace foundation;

    struct NVRenderShaderProgramBinaryType
    {
        enum Enum {
            Unknown = 0,
            NVBinary = 1,
        };
    };

    // context dirty flags
    struct NVRenderContextDirtyValues
    {
        enum Enum {
            InputAssembler = 1 << 0,
        };
    };

    typedef NVFlags<NVRenderContextDirtyValues::Enum, QT3DSU32> NVRenderContextDirtyFlags;
    typedef nvhash_map<CRegisteredString, NVRenderConstantBuffer *> TContextConstantBufferMap;
    typedef nvhash_map<CRegisteredString, NVRenderStorageBuffer *> TContextStorageBufferMap;
    typedef nvhash_map<CRegisteredString, NVRenderAtomicCounterBuffer *>
        TContextAtomicCounterBufferMap;
    typedef nvhash_map<NVRenderBackend::NVRenderBackendBufferObject, NVRenderDrawIndirectBuffer *>
        TContextDrawIndirectBufferMap;
    typedef nvhash_map<NVRenderBackend::NVRenderBackendDepthStencilStateObject,
                       NVRenderDepthStencilState *>
        TContextDepthStencilStateMap;
    typedef nvhash_map<NVRenderBackend::NVRenderBackendRasterizerStateObject,
                       NVRenderRasterizerState *>
        TContextRasterizerStateMap;
    typedef nvhash_map<NVRenderBackend::NVRenderBackendTextureObject, NVRenderTexture2DArray *>
        TContextTex2DArrayToImpMap;
    typedef nvhash_map<NVRenderBackend::NVRenderBackendTextureObject, NVRenderTextureCube *>
        TContextTexCubeToImpMap;
    typedef nvhash_map<NVRenderBackend::NVRenderBackendTextureObject, NVRenderImage2D *>
        TContextImage2DToImpMap;
    typedef nvhash_map<CRegisteredString, NVRenderPathFontSpecification *>
        TContextPathFontSpecificationMap;

    class QT3DS_AUTOTEST_EXPORT NVRenderContext : public NVRefCounted, public NVRenderDrawable
    {
    public:
        virtual NVRenderContextType GetRenderContextType() const = 0;
        virtual bool AreMultisampleTexturesSupported() const = 0;
        virtual bool GetConstantBufferSupport() const = 0;
        virtual void getMaxTextureSize(QT3DSU32 &oWidth, QT3DSU32 &oHeight) = 0;
        virtual const char *GetShadingLanguageVersion() = 0;
        // Get the bit depth of the currently bound depth buffer.
        virtual QT3DSU32 GetDepthBits() const = 0;
        virtual QT3DSU32 GetStencilBits() const = 0;
        virtual bool
        GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::Enum inCap) const = 0;
        virtual bool AreDXTImagesSupported() const = 0;
        virtual bool IsDepthStencilSupported() const = 0;
        virtual bool IsFpRenderTargetSupported() const = 0;
        virtual bool IsTessellationSupported() const = 0;
        virtual bool IsGeometryStageSupported() const = 0;
        virtual bool IsComputeSupported() const = 0;
        virtual bool IsSampleQuerySupported() const = 0;
        virtual bool IsTimerQuerySupported() const = 0;
        virtual bool IsCommandSyncSupported() const = 0;
        virtual bool IsTextureArraySupported() const = 0;
        virtual bool IsStorageBufferSupported() const = 0;
        virtual bool IsAtomicCounterBufferSupported() const = 0;
        virtual bool IsShaderImageLoadStoreSupported() const = 0;
        virtual bool IsProgramPipelineSupported() const = 0;
        virtual bool IsPathRenderingSupported() const = 0;
        virtual bool IsBlendCoherencySupported() const = 0;
        virtual bool IsAdvancedBlendHwSupported() const = 0;
        virtual bool IsAdvancedBlendHwSupportedKHR() const = 0;
        virtual bool IsStandardDerivativesSupported() const = 0;
        virtual bool IsTextureLodSupported() const = 0;
        virtual bool isBinaryProgramSupported() const = 0;
        virtual bool isSceneCameraView() const = 0;

        virtual void SetDefaultRenderTarget(QT3DSU64 targetID) = 0;
        virtual void SetDefaultDepthBufferBitCount(QT3DSI32 depthBits) = 0;
        virtual void setIsSceneCameraView(bool sceneCameraView) = 0;

        virtual NVRenderDepthStencilState *
        CreateDepthStencilState(bool enableDepth, bool depthMask, NVRenderBoolOp::Enum depthFunc,
                                bool enableStencil,
                                NVRenderStencilFunctionArgument &stencilFuncFront,
                                NVRenderStencilFunctionArgument &stencilFuncBack,
                                NVRenderStencilOperationArgument &depthStencilOpFront,
                                NVRenderStencilOperationArgument &depthStencilOpBack) = 0;

        virtual NVRenderRasterizerState *CreateRasterizerState(QT3DSF32 depthBias, QT3DSF32 depthScale,
                                                               NVRenderFaces::Enum cullFace) = 0;

        virtual NVRenderVertexBuffer *
        CreateVertexBuffer(NVRenderBufferUsageType::Enum usageType, size_t size, QT3DSU32 stride = 0,
                           NVConstDataRef<QT3DSU8> bufferData = NVConstDataRef<QT3DSU8>()) = 0;
        virtual NVRenderVertexBuffer *GetVertexBuffer(const void *implementationHandle) = 0;

        virtual NVRenderIndexBuffer *
        CreateIndexBuffer(NVRenderBufferUsageType::Enum usageType,
                          NVRenderComponentTypes::Enum componentType, size_t size,
                          NVConstDataRef<QT3DSU8> bufferData = NVConstDataRef<QT3DSU8>()) = 0;
        virtual NVRenderIndexBuffer *GetIndexBuffer(const void *implementationHandle) = 0;

        virtual NVRenderConstantBuffer *
        CreateConstantBuffer(const char *bufferName,
                             qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
                             NVConstDataRef<QT3DSU8> bufferData) = 0;
        virtual NVRenderConstantBuffer *GetConstantBuffer(CRegisteredString bufferName) = 0;

        virtual NVRenderStorageBuffer *
        CreateStorageBuffer(const char *bufferName,
                            qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
                            NVConstDataRef<QT3DSU8> bufferData, NVRenderDataBuffer *pBuffer) = 0;
        virtual NVRenderStorageBuffer *GetStorageBuffer(CRegisteredString bufferName) = 0;

        virtual NVRenderAtomicCounterBuffer *
        CreateAtomicCounterBuffer(const char *bufferName,
                                  qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
                                  NVConstDataRef<QT3DSU8> bufferData) = 0;
        virtual NVRenderAtomicCounterBuffer *
        GetAtomicCounterBuffer(CRegisteredString bufferName) = 0;
        virtual NVRenderAtomicCounterBuffer *
        GetAtomicCounterBufferByParam(CRegisteredString paramName) = 0;

        virtual NVRenderDrawIndirectBuffer *
        CreateDrawIndirectBuffer(qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
                                 NVConstDataRef<QT3DSU8> bufferData) = 0;
        virtual NVRenderDrawIndirectBuffer *GetDrawIndirectBuffer(
            NVRenderBackend::NVRenderBackendBufferObject implementationHandle) = 0;

        virtual void SetMemoryBarrier(NVRenderBufferBarrierFlags barriers) = 0;

        virtual NVRenderOcclusionQuery *CreateOcclusionQuery() = 0;
        virtual NVRenderTimerQuery *CreateTimerQuery() = 0;
        virtual NVRenderSync *CreateSync() = 0;

        virtual NVRenderTexture2D *CreateTexture2D() = 0;
        virtual NVRenderTexture2D *GetTexture2D(const void *implementationHandle) = 0;
        virtual NVRenderBackend *GetBackend() = 0;

        virtual NVRenderTexture2DArray *CreateTexture2DArray() = 0;

        virtual NVRenderTextureCube *CreateTextureCube() = 0;

        virtual NVRenderImage2D *CreateImage2D(NVRenderTexture2D *inTexture,
                                               NVRenderImageAccessType::Enum inAccess) = 0;

        virtual NVRenderRenderBuffer *
        CreateRenderBuffer(NVRenderRenderBufferFormats::Enum bufferFormat, QT3DSU32 width,
                           QT3DSU32 height) = 0;
        virtual NVRenderRenderBuffer *GetRenderBuffer(const void *implementationHandle) = 0;
        // Create a new frame buffer and set the current render target to that frame buffer.
        virtual NVRenderFrameBuffer *CreateFrameBuffer() = 0;
        virtual NVRenderFrameBuffer *GetFrameBuffer(const void *implementationHandle) = 0;

        virtual NVRenderAttribLayout *
        CreateAttributeLayout(NVConstDataRef<NVRenderVertexBufferEntry> attribs) = 0;

        virtual NVRenderInputAssembler *
        CreateInputAssembler(NVRenderAttribLayout *attribLayout,
                             NVConstDataRef<NVRenderVertexBuffer *> buffers,
                             const NVRenderIndexBuffer *indexBuffer, NVConstDataRef<QT3DSU32> strides,
                             NVConstDataRef<QT3DSU32> offsets,
                             NVRenderDrawMode::Enum primType = NVRenderDrawMode::Triangles,
                             QT3DSU32 patchVertexCount = 1) = 0;
        virtual void SetInputAssembler(NVRenderInputAssembler *inputAssembler) = 0;

        virtual NVRenderVertFragCompilationResult CompileSource(
            const char *shaderName, NVConstDataRef<QT3DSI8> vertShader,
            NVConstDataRef<QT3DSI8> fragShader,
            NVConstDataRef<QT3DSI8> tessControlShaderSource = NVConstDataRef<QT3DSI8>(),
            NVConstDataRef<QT3DSI8> tessEvaluationShaderSource = NVConstDataRef<QT3DSI8>(),
            NVConstDataRef<QT3DSI8> geometryShaderSource = NVConstDataRef<QT3DSI8>(),
            bool separateProgram = false,
            NVRenderShaderProgramBinaryType::Enum type = NVRenderShaderProgramBinaryType::Unknown,
            bool binaryProgram = false) = 0;

        // You must figure out inVertLen and inFragLen yourself, this object doesn't do that.
        NVRenderVertFragCompilationResult
        CompileSource(const char *shaderName, const char *vertShader, QT3DSU32 inVertLen,
                      const char *fragShader, QT3DSU32 inFragLen, const char * = NULL,
                      QT3DSU32 inTCLen = 0, const char *tessEvaluationShaderSource = NULL,
                      QT3DSU32 inTELen = 0, const char *geometryShaderSource = NULL, QT3DSU32 inGSLen = 0,
                      bool separableProgram = false);

        virtual NVRenderVertFragCompilationResult
        CompileBinary(const char *shaderName, NVRenderShaderProgramBinaryType::Enum type,
                      NVDataRef<QT3DSI8> vertShader, NVDataRef<QT3DSI8> fragShader,
                      NVDataRef<QT3DSI8> tessControlShaderSource = NVDataRef<QT3DSI8>(),
                      NVDataRef<QT3DSI8> tessEvaluationShaderSource = NVDataRef<QT3DSI8>(),
                      NVConstDataRef<QT3DSI8> geometryShaderSource = NVConstDataRef<QT3DSI8>()) = 0;
        virtual NVRenderVertFragCompilationResult
        CompileBinary(const char *shaderName, QT3DSU32 format, const QByteArray &binary) = 0;

        virtual NVRenderVertFragCompilationResult
        CompileComputeSource(const char *shaderName, NVConstDataRef<QT3DSI8> computeShaderSource) = 0;

        virtual NVRenderShaderProgram *GetShaderProgram(const void *implementationHandle) = 0;

        virtual NVRenderProgramPipeline *CreateProgramPipeline() = 0;

        virtual NVRenderPathSpecification *CreatePathSpecification() = 0;
        virtual NVRenderPathRender *CreatePathRender(size_t range = 1) = 0;
        virtual void SetPathProjectionMatrix(const QT3DSMat44 inPathProjection) = 0;
        virtual void SetPathModelViewMatrix(const QT3DSMat44 inPathModelview) = 0;
        virtual void SetPathStencilDepthOffset(QT3DSF32 inSlope, QT3DSF32 inBias) = 0;
        virtual void SetPathCoverDepthFunc(NVRenderBoolOp::Enum inFunc) = 0;

        virtual NVRenderPathFontSpecification *
        CreatePathFontSpecification(CRegisteredString fontName) = 0;
        virtual NVRenderPathFontItem *CreatePathFontItem() = 0;

        // Specific setters for the guaranteed-to-exist context properties to set them as fast as
        // possible.
        // Note that this bypasses the property manage so push/pop will have no effect (unless you
        // set these already
        // once after a push using the property manager.
        virtual void SetClearColor(QT3DSVec4 inClearColor) = 0;
        virtual QT3DSVec4 GetClearColor() const = 0;

        virtual void SetBlendFunction(NVRenderBlendFunctionArgument inFunctions) = 0;
        virtual NVRenderBlendFunctionArgument GetBlendFunction() const = 0;

        virtual void SetBlendEquation(NVRenderBlendEquationArgument inEquations) = 0;
        virtual NVRenderBlendEquationArgument GetBlendEquation() const = 0;

        virtual void SetCullingEnabled(bool inEnabled) = 0;
        virtual bool IsCullingEnabled() const = 0;

        virtual void SetDepthFunction(NVRenderBoolOp::Enum inFunction) = 0;
        virtual qt3ds::render::NVRenderBoolOp::Enum GetDepthFunction() const = 0;

        virtual void SetBlendingEnabled(bool inEnabled) = 0;
        virtual bool IsBlendingEnabled() const = 0;

        virtual void SetDepthWriteEnabled(bool inEnabled) = 0;
        virtual bool IsDepthWriteEnabled() const = 0;

        virtual void SetDepthTestEnabled(bool inEnabled) = 0;
        virtual bool IsDepthTestEnabled() const = 0;

        virtual void SetDepthStencilState(NVRenderDepthStencilState *inDepthStencilState) = 0;
        virtual void SetStencilTestEnabled(bool inEnabled) = 0;
        virtual bool IsStencilTestEnabled() const = 0;

        virtual void SetRasterizerState(NVRenderRasterizerState *inRasterizerState) = 0;

        virtual void SetScissorTestEnabled(bool inEnabled) = 0;
        virtual bool IsScissorTestEnabled() const = 0;

        virtual void SetScissorRect(NVRenderRect inRect) = 0;
        virtual NVRenderRect GetScissorRect() const = 0;

        virtual void SetViewport(NVRenderRect inViewport) = 0;
        virtual NVRenderRect GetViewport() const = 0;

        virtual void SetColorWritesEnabled(bool inEnabled) = 0;
        virtual bool IsColorWritesEnabled() const = 0;

        virtual void SetMultisampleEnabled(bool inEnabled) = 0;
        virtual bool IsMultisampleEnabled() const = 0;

        // Used during layer rendering because we can't set the *actual* viewport to what it should
        // be due to hardware problems.
        // Set during begin render.
        static QT3DSMat44
        ApplyVirtualViewportToProjectionMatrix(const QT3DSMat44 &inProjection,
                                               const NVRenderRectF &inViewport,
                                               const NVRenderRectF &inVirtualViewport);

        virtual void SetRenderTarget(NVRenderFrameBuffer *inBuffer) = 0;
        virtual void SetReadTarget(NVRenderFrameBuffer *inBuffer) = 0;
        virtual NVRenderFrameBuffer *GetRenderTarget() const = 0;

        virtual void SetActiveShader(NVRenderShaderProgram *inShader) = 0;
        virtual NVRenderShaderProgram *GetActiveShader() const = 0;

        virtual void SetActiveProgramPipeline(NVRenderProgramPipeline *inProgramPipeline) = 0;
        virtual NVRenderProgramPipeline *GetActiveProgramPipeline() const = 0;

        virtual void DispatchCompute(NVRenderShaderProgram *inShader, QT3DSU32 numGroupsX,
                                     QT3DSU32 numGroupsY, QT3DSU32 numGroupsZ) = 0;

        // Push the entire set of properties.
        virtual void PushPropertySet() = 0;
        // Pop the entire set of properties, potentially forcing the values
        // to opengl.  Will set the hardware state to whatever at the time of push.
        virtual void PopPropertySet(bool inForceSetProperties) = 0;

        // Ensure the hardware state matches the state we expect.
        virtual void ResetBlendState() = 0;

        // Draw buffers are what the system will render to.. Applies to the current render context.
        //-1 means none, else the integer is assumed to be an index past the draw buffer index.
        // This applies only to the currently bound framebuffer.
        virtual void SetDrawBuffers(NVConstDataRef<QT3DSI32> inDrawBufferSet) = 0;
        virtual void SetReadBuffer(NVReadFaces::Enum inReadFace) = 0;

        virtual void ReadPixels(NVRenderRect inRect, NVRenderReadPixelFormats::Enum inFormat,
                                NVDataRef<QT3DSU8> inWriteBuffer) = 0;

        // Return the property manager for this render context
        // virtual NVRenderPropertyManager& GetPropertyManager() = 0;
        // Clear the current render target
        virtual void Clear(NVRenderClearFlags flags) = 0;
        // Clear this framebuffer without changing the active frame buffer
        virtual void Clear(NVRenderFrameBuffer &framebuffer, NVRenderClearFlags flags) = 0;
        // copy framebuffer content between read target and render target
        virtual void BlitFramebuffer(QT3DSI32 srcX0, QT3DSI32 srcY0, QT3DSI32 srcX1, QT3DSI32 srcY1,
                                     QT3DSI32 dstX0, QT3DSI32 dstY0, QT3DSI32 dstX1, QT3DSI32 dstY1,
                                     NVRenderClearFlags flags,
                                     NVRenderTextureMagnifyingOp::Enum filter) = 0;

        // Render, applying these immediate property values just before render.  The hardware
        // properties are tracked for push/pop
        // but the shader properties are just set on the active shader *after* the hardware
        // properties have been set.  The shader properties are not tracked for push/pop.
        // This call is meant to come directly before each draw call to setup the last bit of state
        // before the draw operation.  Note that there isn't another way to set the immedate
        // property values for a given shader because it isn't clear which shader is active
        // when the properties are getting set.
        void Draw(NVRenderDrawMode::Enum drawMode, QT3DSU32 count, QT3DSU32 offset) override = 0;

        /**
         * @brief Draw the current active vertex buffer using an indirect buffer
         *		  This means the setup of the draw call is stored in a buffer bound to
         *		  NVRenderBufferBindValues::Draw_Indirect
         *
         * @param[in] drawMode	Draw mode (Triangles, ....)
         * @param[in] indirect	Offset into a indirect drawing setup buffer
         *
         * @return no return.
         */
        virtual void DrawIndirect(NVRenderDrawMode::Enum drawMode, QT3DSU32 offset) = 0;

        virtual NVFoundationBase &GetFoundation() = 0;
        virtual qt3ds::foundation::IStringTable &GetStringTable() = 0;
        virtual NVAllocatorCallback &GetAllocator() = 0;

        virtual QSurfaceFormat format() const = 0;

        virtual void resetStates() = 0;

        static NVRenderContext &CreateGL(NVFoundationBase &foundation, IStringTable &inStringTable,
                                         const QSurfaceFormat &format);

        static NVRenderContext &CreateNULL(NVFoundationBase &foundation,
                                           IStringTable &inStringTable);
    };

    // Now for scoped property access.
    template <typename TDataType>
    struct NVRenderContextScopedProperty
        : public NVRenderGenericScopedProperty<NVRenderContext, TDataType>
    {
        typedef typename NVRenderGenericScopedProperty<NVRenderContext, TDataType>::TGetter TGetter;
        typedef typename NVRenderGenericScopedProperty<NVRenderContext, TDataType>::TSetter TSetter;
        NVRenderContextScopedProperty(NVRenderContext &ctx, TGetter getter, TSetter setter)
            : NVRenderGenericScopedProperty<NVRenderContext, TDataType>(ctx, getter, setter)
        {
        }
        NVRenderContextScopedProperty(NVRenderContext &ctx, TGetter getter, TSetter setter,
                                      const TDataType &inNewValue)
            : NVRenderGenericScopedProperty<NVRenderContext, TDataType>(ctx, getter, setter,
                                                                        inNewValue)
        {
        }
    };

/**
 * A Render Context implementation class
 *
 */

#define ITERATE_HARDWARE_CONTEXT_PROPERTIES                                                        \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(RenderTarget, FrameBuffer)                                    \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ActiveShader, ActiveShader)                                   \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ActiveProgramPipeline, ActiveProgramPipeline)                 \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(InputAssembler, InputAssembler)                               \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(BlendFunction, BlendFunction)                                 \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(CullingEnabled, CullingEnabled)                               \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(DepthFunction, DepthFunction)                                 \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(BlendingEnabled, BlendingEnabled)                             \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(DepthWriteEnabled, DepthWriteEnabled)                         \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(DepthTestEnabled, DepthTestEnabled)                           \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(StencilTestEnabled, StencilTestEnabled)                       \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ScissorTestEnabled, ScissorTestEnabled)                       \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ScissorRect, ScissorRect)                                     \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(Viewport, Viewport)                                           \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ClearColor, ClearColor)

    // forward declarations

    class NVRenderContextImpl : public NVRenderContext, public NoCopy
    {
    public:
        // these variables represent the current hardware state of the render context.
        SNVGLHardPropertyContext m_HardwarePropertyContext;

    private:
        NVScopedRefCounted<NVRenderBackend> m_backend; ///< pointer to our render backend
        NVRenderContextDirtyFlags m_DirtyFlags; ///< context dirty flags

        NVRenderBackend::NVRenderBackendRenderTargetObject
            m_DefaultOffscreenRenderTarget; ///< this is a special target set from outside if we
                                            ///never render to a window directly (GL only)
        QT3DSI32 m_DephBits; ///< this is the depth bits count of the default window render target
        QT3DSI32 m_StencilBits; ///< this is the stencil bits count of the default window render target
        bool m_isSceneCameraView = true; ///< true in viewer and in editor when in scene camera view mode

    protected:
        volatile QT3DSI32 mRefCount;
        NVFoundationBase &m_Foundation;
        qt3ds::foundation::IStringTable &m_StringTable;

        nvhash_map<const void *, NVRenderVertexBuffer *> m_VertToImpMap;
        nvhash_map<const void *, NVRenderIndexBuffer *> m_IndexToImpMap;
        TContextConstantBufferMap m_ConstantToImpMap;
        TContextStorageBufferMap m_StorageToImpMap;
        TContextAtomicCounterBufferMap m_AtomicCounterToImpMap;
        TContextDrawIndirectBufferMap m_DrawIndirectToImpMap;
        TContextDepthStencilStateMap m_DepthStencilStateToImpMap;
        TContextRasterizerStateMap m_RasterizerStateToImpMap;
        TContextPathFontSpecificationMap m_PathFontSpecToImpMap;

        nvhash_map<const void *, NVRenderTexture2D *> m_Tex2DToImpMap;
        TContextTex2DArrayToImpMap m_Tex2DArrayToImpMap;
        TContextTexCubeToImpMap m_TexCubeToImpMap;
        TContextImage2DToImpMap m_Image2DtoImpMap;
        nvhash_map<const void *, NVRenderShaderProgram *> m_ShaderToImpMap;
        nvhash_map<const void *, NVRenderRenderBuffer *> m_RenderBufferToImpMap;
        nvhash_map<const void *, NVRenderFrameBuffer *> m_FrameBufferToImpMap;
        QT3DSU32 m_MaxTextureUnits;
        QT3DSU32 m_NextTextureUnit;
        QT3DSU32 m_MaxConstantBufferUnits;
        QT3DSU32 m_NextConstantBufferUnit;

        nvvector<SNVGLHardPropertyContext> m_PropertyStack;

        void DoSetClearColor(QT3DSVec4 inClearColor)
        {
            m_HardwarePropertyContext.m_ClearColor = inClearColor;
            m_backend->SetClearColor(&inClearColor);
        }

        void DoSetBlendFunction(NVRenderBlendFunctionArgument inFunctions)
        {
            QT3DSI32_4 values;
            m_HardwarePropertyContext.m_BlendFunction = inFunctions;

            m_backend->SetBlendFunc(inFunctions);
        }

        void DoSetBlendEquation(NVRenderBlendEquationArgument inEquations)
        {
            QT3DSI32_4 values;
            m_HardwarePropertyContext.m_BlendEquation = inEquations;

            m_backend->SetBlendEquation(inEquations);
        }

        void DoSetCullingEnabled(bool inEnabled)
        {
            m_HardwarePropertyContext.m_CullingEnabled = inEnabled;
            m_backend->SetRenderState(inEnabled, NVRenderState::CullFace);
        }

        void DoSetDepthFunction(qt3ds::render::NVRenderBoolOp::Enum inFunction)
        {
            m_HardwarePropertyContext.m_DepthFunction = inFunction;
            m_backend->SetDepthFunc(inFunction);
        }

        void DoSetBlendingEnabled(bool inEnabled)
        {
            m_HardwarePropertyContext.m_BlendingEnabled = inEnabled;
            m_backend->SetRenderState(inEnabled, NVRenderState::Blend);
        }

        void DoSetColorWritesEnabled(bool inEnabled)
        {
            m_HardwarePropertyContext.m_ColorWritesEnabled = inEnabled;
            m_backend->SetColorWrites(inEnabled, inEnabled, inEnabled, inEnabled);
        }

        void DoSetMultisampleEnabled(bool inEnabled)
        {
            m_HardwarePropertyContext.m_MultisampleEnabled = inEnabled;
            m_backend->SetMultisample(inEnabled);
        }

        void DoSetDepthWriteEnabled(bool inEnabled)
        {
            m_HardwarePropertyContext.m_DepthWriteEnabled = inEnabled;
            m_backend->SetDepthWrite(inEnabled);
        }

        void DoSetDepthTestEnabled(bool inEnabled)
        {
            m_HardwarePropertyContext.m_DepthTestEnabled = inEnabled;
            m_backend->SetRenderState(inEnabled, NVRenderState::DepthTest);
        }

        void DoSetStencilTestEnabled(bool inEnabled)
        {
            m_HardwarePropertyContext.m_StencilTestEnabled = inEnabled;
            m_backend->SetRenderState(inEnabled, NVRenderState::StencilTest);
        }

        void DoSetScissorTestEnabled(bool inEnabled)
        {
            m_HardwarePropertyContext.m_ScissorTestEnabled = inEnabled;
            m_backend->SetRenderState(inEnabled, NVRenderState::ScissorTest);
        }

        void DoSetScissorRect(NVRenderRect inRect)
        {
            m_HardwarePropertyContext.m_ScissorRect = inRect;
            m_backend->SetScissorRect(inRect);
        }

        void DoSetViewport(NVRenderRect inViewport)
        {
            m_HardwarePropertyContext.m_Viewport = inViewport;
            m_backend->SetViewportRect(inViewport);
        }

        // Circular dependencies between shader constants and shader programs preclude
        // implementation in header
        void DoSetActiveShader(NVRenderShaderProgram *inShader);
        void DoSetActiveProgramPipeline(NVRenderProgramPipeline *inProgramPipeline);

        void DoSetInputAssembler(NVRenderInputAssembler *inAssembler)
        {
            m_HardwarePropertyContext.m_InputAssembler = inAssembler;
            m_DirtyFlags |= NVRenderContextDirtyValues::InputAssembler;
        }

        void DoSetRenderTarget(NVRenderFrameBuffer *inBuffer)
        {
            if (inBuffer)
                m_backend->SetRenderTarget(inBuffer->GetFrameBuffertHandle());
            else
                m_backend->SetRenderTarget(m_DefaultOffscreenRenderTarget);

            m_HardwarePropertyContext.m_FrameBuffer = inBuffer;
        }

        void DoSetReadTarget(NVRenderFrameBuffer *inBuffer)
        {
            if (inBuffer)
                m_backend->SetReadTarget(inBuffer->GetFrameBuffertHandle());
            else
                m_backend->SetReadTarget(NVRenderBackend::NVRenderBackendRenderTargetObject(NULL));
        }

        bool BindShaderToInputAssembler(const NVRenderInputAssembler *inputAssembler,
                                        NVRenderShaderProgram *shader);
        bool ApplyPreDrawProperties();
        void OnPostDraw();

    public:
        NVRenderContextImpl(NVFoundationBase &fnd, NVRenderBackend &inBackend,
                            IStringTable &inStrTable);
        virtual ~NVRenderContextImpl();

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation);

        NVRenderBackend *GetBackend() override { return m_backend; }

        void getMaxTextureSize(QT3DSU32 &oWidth, QT3DSU32 &oHeight) override;

        const char *GetShadingLanguageVersion() override
        {
            return m_backend->GetShadingLanguageVersion();
        }

        NVRenderContextType GetRenderContextType() const override
        {
            return m_backend->GetRenderContextType();
        }

        QT3DSU32 GetDepthBits() const override
        {
            // only query this if a framebuffer is bound
            if (m_HardwarePropertyContext.m_FrameBuffer)
                return m_backend->GetDepthBits();
            else
                return m_DephBits;
        }

        QT3DSU32 GetStencilBits() const override
        {
            // only query this if a framebuffer is bound
            if (m_HardwarePropertyContext.m_FrameBuffer)
                return m_backend->GetStencilBits();
            else
                return m_StencilBits;
        }

        bool GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::Enum inCap) const override
        {
            return m_backend->GetRenderBackendCap(inCap);
        }

        bool AreMultisampleTexturesSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::MsTexture);
        }

        bool GetConstantBufferSupport() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::ConstantBuffer);
        }

        bool AreDXTImagesSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::DxtImages);
        }

        bool IsDepthStencilSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::DepthStencilTexture);
        }

        bool IsFpRenderTargetSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::FpRenderTarget);
        }

        bool IsTessellationSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::Tessellation);
        }

        bool IsGeometryStageSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::Geometry);
        }

        bool IsComputeSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::Compute);
        }

        bool IsSampleQuerySupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::SampleQuery);
        }

        bool IsTimerQuerySupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::TimerQuery);
        }

        bool IsCommandSyncSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::CommandSync);
        }
        bool IsTextureArraySupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::TextureArray);
        }
        bool IsStorageBufferSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::StorageBuffer);
        }
        bool IsAtomicCounterBufferSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::AtomicCounterBuffer);
        }
        bool IsShaderImageLoadStoreSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::ShaderImageLoadStore);
        }
        bool IsProgramPipelineSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::ProgramPipeline);
        }
        bool IsPathRenderingSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::PathRendering);
        }
        // Are blend modes really supported in HW?
        bool IsAdvancedBlendHwSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::AdvancedBlend);
        }
        bool IsAdvancedBlendHwSupportedKHR() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::AdvancedBlendKHR);
        }
        bool IsBlendCoherencySupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::BlendCoherency);
        }
        bool IsStandardDerivativesSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::StandardDerivatives);
        }
        bool IsTextureLodSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::TextureLod);
        }
        bool isBinaryProgramSupported() const override
        {
            return GetRenderBackendCap(NVRenderBackend::NVRenderBackendCaps::BinaryProgram);
        }

        bool isSceneCameraView() const override
        {
            return m_isSceneCameraView;
        }

        void SetDefaultRenderTarget(QT3DSU64 targetID) override
        {
            m_DefaultOffscreenRenderTarget =
                (NVRenderBackend::NVRenderBackendRenderTargetObject)targetID;
        }

        void SetDefaultDepthBufferBitCount(QT3DSI32 depthBits) override { m_DephBits = depthBits; }

        void setIsSceneCameraView(bool sceneCameraView) override {
            m_isSceneCameraView = sceneCameraView;
        }

        virtual NVRenderDepthStencilState *
        CreateDepthStencilState(bool enableDepth, bool depthMask, NVRenderBoolOp::Enum depthFunc,
                                bool enableStencil,
                                NVRenderStencilFunctionArgument &stencilFuncFront,
                                NVRenderStencilFunctionArgument &stencilFuncBack,
                                NVRenderStencilOperationArgument &depthStencilOpFront,
                                NVRenderStencilOperationArgument &depthStencilOpBack) override;
        void SetDepthStencilState(NVRenderDepthStencilState *inDepthStencilState) override;
        virtual void StateDestroyed(NVRenderDepthStencilState &state);

        NVRenderRasterizerState *CreateRasterizerState(QT3DSF32 depthBias, QT3DSF32 depthScale,
                                                               NVRenderFaces::Enum cullFace) override;
        void SetRasterizerState(NVRenderRasterizerState *inRasterizerState) override;
        virtual void StateDestroyed(NVRenderRasterizerState &state);

        NVRenderVertexBuffer *CreateVertexBuffer(NVRenderBufferUsageType::Enum usageType,
                                                         size_t size, QT3DSU32 stride,
                                                         NVConstDataRef<QT3DSU8> bufferData) override;
        NVRenderVertexBuffer *GetVertexBuffer(const void *implementationHandle) override;
        virtual void BufferDestroyed(NVRenderVertexBuffer &buffer);

        virtual NVRenderIndexBuffer *
        CreateIndexBuffer(qt3ds::render::NVRenderBufferUsageType::Enum usageType,
                          qt3ds::render::NVRenderComponentTypes::Enum componentType, size_t size,
                          NVConstDataRef<QT3DSU8> bufferData) override;
        NVRenderIndexBuffer *GetIndexBuffer(const void *implementationHandle) override;
        virtual void BufferDestroyed(NVRenderIndexBuffer &buffer);

        virtual NVRenderConstantBuffer *
        CreateConstantBuffer(const char *bufferName,
                             qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
                             NVConstDataRef<QT3DSU8> bufferData) override;
        NVRenderConstantBuffer *GetConstantBuffer(CRegisteredString bufferName) override;
        virtual void BufferDestroyed(NVRenderConstantBuffer &buffer);

        virtual QT3DSU32 GetNextConstantBufferUnit();

        virtual NVRenderStorageBuffer *
        CreateStorageBuffer(const char *bufferName,
                            qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
                            NVConstDataRef<QT3DSU8> bufferData, NVRenderDataBuffer *pBuffer) override;
        NVRenderStorageBuffer *GetStorageBuffer(CRegisteredString bufferName) override;
        virtual void BufferDestroyed(NVRenderStorageBuffer &buffer);

        virtual NVRenderAtomicCounterBuffer *
        CreateAtomicCounterBuffer(const char *bufferName,
                                  qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
                                  NVConstDataRef<QT3DSU8> bufferData) override;
        NVRenderAtomicCounterBuffer *GetAtomicCounterBuffer(CRegisteredString bufferName) override;
        virtual NVRenderAtomicCounterBuffer *
        GetAtomicCounterBufferByParam(CRegisteredString paramName) override;
        virtual void BufferDestroyed(NVRenderAtomicCounterBuffer &buffer);

        virtual NVRenderDrawIndirectBuffer *
        CreateDrawIndirectBuffer(qt3ds::render::NVRenderBufferUsageType::Enum usageType, size_t size,
                                 NVConstDataRef<QT3DSU8> bufferData) override;
        virtual NVRenderDrawIndirectBuffer *
        GetDrawIndirectBuffer(NVRenderBackend::NVRenderBackendBufferObject implementationHandle) override;
        virtual void BufferDestroyed(NVRenderDrawIndirectBuffer &buffer);

        void SetMemoryBarrier(NVRenderBufferBarrierFlags barriers) override;

        NVRenderOcclusionQuery *CreateOcclusionQuery() override;
        NVRenderTimerQuery *CreateTimerQuery() override;
        NVRenderSync *CreateSync() override;

        NVRenderTexture2D *CreateTexture2D() override;
        NVRenderTexture2D *GetTexture2D(const void *implementationHandle) override;
        virtual void TextureDestroyed(NVRenderTexture2D &buffer);

        NVRenderTexture2DArray *CreateTexture2DArray() override;
        virtual void TextureDestroyed(NVRenderTexture2DArray &buffer);

        NVRenderTextureCube *CreateTextureCube() override;
        virtual void TextureDestroyed(NVRenderTextureCube &buffer);

        virtual QT3DSU32 GetNextTextureUnit();

        NVRenderImage2D *CreateImage2D(NVRenderTexture2D *inTexture,
                                               NVRenderImageAccessType::Enum inAccess) override;
        virtual void ImageDestroyed(NVRenderImage2D &buffer);

        virtual NVRenderRenderBuffer *
        CreateRenderBuffer(NVRenderRenderBufferFormats::Enum bufferFormat, QT3DSU32 width,
                           QT3DSU32 height) override;
        NVRenderRenderBuffer *GetRenderBuffer(const void *implementationHandle) override;
        virtual void RenderBufferDestroyed(NVRenderRenderBuffer &buffer);

        NVRenderFrameBuffer *CreateFrameBuffer() override;
        NVRenderFrameBuffer *GetFrameBuffer(const void *implementationHandle) override;
        virtual void FrameBufferDestroyed(NVRenderFrameBuffer &fb);

        virtual NVRenderAttribLayout *
        CreateAttributeLayout(NVConstDataRef<NVRenderVertexBufferEntry> attribs) override;
        NVRenderInputAssembler *CreateInputAssembler(
            NVRenderAttribLayout *attribLayout, NVConstDataRef<NVRenderVertexBuffer *> buffers,
            const NVRenderIndexBuffer *indexBuffer, NVConstDataRef<QT3DSU32> strides,
            NVConstDataRef<QT3DSU32> offsets, NVRenderDrawMode::Enum primType, QT3DSU32 patchVertexCount) override;
        void SetInputAssembler(NVRenderInputAssembler *inputAssembler) override;

        NVRenderVertFragCompilationResult CompileSource(
            const char *shaderName, NVConstDataRef<QT3DSI8> vertShader,
            NVConstDataRef<QT3DSI8> fragShader,
            NVConstDataRef<QT3DSI8> tessControlShaderSource = NVConstDataRef<QT3DSI8>(),
            NVConstDataRef<QT3DSI8> tessEvaluationShaderSource = NVConstDataRef<QT3DSI8>(),
            NVConstDataRef<QT3DSI8> geometryShaderSource = NVConstDataRef<QT3DSI8>(),
            bool separateProgram = false,
            NVRenderShaderProgramBinaryType::Enum type = NVRenderShaderProgramBinaryType::Unknown,
            bool binaryProgram = false) override;

        virtual NVRenderVertFragCompilationResult
        CompileBinary(const char *shaderName, NVRenderShaderProgramBinaryType::Enum type,
                      NVDataRef<QT3DSI8> vertShader, NVDataRef<QT3DSI8> fragShader,
                      NVDataRef<QT3DSI8> tessControlShaderSource = NVDataRef<QT3DSI8>(),
                      NVDataRef<QT3DSI8> tessEvaluationShaderSource = NVDataRef<QT3DSI8>(),
                      NVConstDataRef<QT3DSI8> geometryShaderSource = NVConstDataRef<QT3DSI8>()) override;
        NVRenderVertFragCompilationResult CompileBinary(const char *shaderName, QT3DSU32 format,
                                                        const QByteArray &binary) override;
        virtual NVRenderVertFragCompilationResult
        CompileComputeSource(const char *shaderName, NVConstDataRef<QT3DSI8> computeShaderSource) override;

        NVRenderShaderProgram *GetShaderProgram(const void *implementationHandle) override;
        virtual void ShaderDestroyed(NVRenderShaderProgram &shader);

        NVRenderProgramPipeline *CreateProgramPipeline() override;
        NVRenderPathSpecification *CreatePathSpecification() override;
        NVRenderPathRender *CreatePathRender(size_t range = 1) override;
        void SetPathProjectionMatrix(const QT3DSMat44 inPathProjection) override;
        void SetPathModelViewMatrix(const QT3DSMat44 inPathModelview) override;
        void SetPathStencilDepthOffset(QT3DSF32 inSlope, QT3DSF32 inBias) override;
        void SetPathCoverDepthFunc(NVRenderBoolOp::Enum inFunc) override;

        virtual NVRenderPathFontSpecification *
        CreatePathFontSpecification(CRegisteredString fontName) override;
        virtual void ReleasePathFontSpecification(NVRenderPathFontSpecification &inPathSpec);
        NVRenderPathFontItem *CreatePathFontItem() override;

        void SetClearColor(QT3DSVec4 inClearColor) override;
        QT3DSVec4 GetClearColor() const override { return m_HardwarePropertyContext.m_ClearColor; }

        void SetBlendFunction(NVRenderBlendFunctionArgument inFunctions) override;
        NVRenderBlendFunctionArgument GetBlendFunction() const override
        {
            return m_HardwarePropertyContext.m_BlendFunction;
        }

        void SetBlendEquation(NVRenderBlendEquationArgument inEquations) override;
        NVRenderBlendEquationArgument GetBlendEquation() const override
        {
            return m_HardwarePropertyContext.m_BlendEquation;
        };

        void SetCullingEnabled(bool inEnabled) override;
        bool IsCullingEnabled() const override { return m_HardwarePropertyContext.m_CullingEnabled; }

        void SetDepthFunction(qt3ds::render::NVRenderBoolOp::Enum inFunction) override;
        qt3ds::render::NVRenderBoolOp::Enum GetDepthFunction() const override
        {
            return m_HardwarePropertyContext.m_DepthFunction;
        }

        void SetBlendingEnabled(bool inEnabled) override;
        bool IsBlendingEnabled() const override
        {
            return m_HardwarePropertyContext.m_BlendingEnabled;
        }

        void SetDepthWriteEnabled(bool inEnabled) override;
        bool IsDepthWriteEnabled() const override
        {
            return m_HardwarePropertyContext.m_DepthWriteEnabled;
        }
        void SetDepthTestEnabled(bool inEnabled) override;
        bool IsDepthTestEnabled() const override
        {
            return m_HardwarePropertyContext.m_DepthTestEnabled;
        }

        void SetStencilTestEnabled(bool inEnabled) override;
        bool IsStencilTestEnabled() const override
        {
            return m_HardwarePropertyContext.m_StencilTestEnabled;
        }

        void SetScissorTestEnabled(bool inEnabled) override;
        bool IsScissorTestEnabled() const override
        {
            return m_HardwarePropertyContext.m_ScissorTestEnabled;
        }
        void SetScissorRect(NVRenderRect inRect) override;
        NVRenderRect GetScissorRect() const override
        {
            return m_HardwarePropertyContext.m_ScissorRect;
        }

        void SetViewport(NVRenderRect inViewport) override;
        NVRenderRect GetViewport() const override { return m_HardwarePropertyContext.m_Viewport; }

        void SetColorWritesEnabled(bool inEnabled) override;
        bool IsColorWritesEnabled() const override
        {
            return m_HardwarePropertyContext.m_ColorWritesEnabled;
        }

        void SetMultisampleEnabled(bool inEnabled) override;
        bool IsMultisampleEnabled() const override
        {
            return m_HardwarePropertyContext.m_MultisampleEnabled;
        }

        void SetActiveShader(NVRenderShaderProgram *inShader) override;
        NVRenderShaderProgram *GetActiveShader() const override
        {
            return m_HardwarePropertyContext.m_ActiveShader;
        }

        void SetActiveProgramPipeline(NVRenderProgramPipeline *inProgramPipeline) override;
        NVRenderProgramPipeline *GetActiveProgramPipeline() const override
        {
            return m_HardwarePropertyContext.m_ActiveProgramPipeline;
        }

        void DispatchCompute(NVRenderShaderProgram *inShader, QT3DSU32 numGroupsX,
                                     QT3DSU32 numGroupsY, QT3DSU32 numGroupsZ) override;

        void SetDrawBuffers(NVConstDataRef<QT3DSI32> inDrawBufferSet) override;
        void SetReadBuffer(NVReadFaces::Enum inReadFace) override;

        void ReadPixels(NVRenderRect inRect, NVRenderReadPixelFormats::Enum inFormat,
                                NVDataRef<QT3DSU8> inWriteBuffer) override;

        void SetRenderTarget(NVRenderFrameBuffer *inBuffer) override;
        void SetReadTarget(NVRenderFrameBuffer *inBuffer) override;
        NVRenderFrameBuffer *GetRenderTarget() const override
        {
            return m_HardwarePropertyContext.m_FrameBuffer;
        }

        void ResetBlendState() override;

        // Push the entire set of properties.
        void PushPropertySet() override { m_PropertyStack.push_back(m_HardwarePropertyContext); }

        // Pop the entire set of properties, potentially forcing the values
        // to opengl.
        void PopPropertySet(bool inForceSetProperties) override;

        // clear current bound render target
        void Clear(NVRenderClearFlags flags) override;
        // clear passed in rendertarget
        void Clear(NVRenderFrameBuffer &fb, NVRenderClearFlags flags) override;

        // copy framebuffer content between read target and render target
        void BlitFramebuffer(QT3DSI32 srcX0, QT3DSI32 srcY0, QT3DSI32 srcX1, QT3DSI32 srcY1,
                                     QT3DSI32 dstX0, QT3DSI32 dstY0, QT3DSI32 dstX1, QT3DSI32 dstY1,
                                     NVRenderClearFlags flags,
                                     NVRenderTextureMagnifyingOp::Enum filter) override;

        void Draw(NVRenderDrawMode::Enum drawMode, QT3DSU32 count, QT3DSU32 offset) override;
        void DrawIndirect(NVRenderDrawMode::Enum drawMode, QT3DSU32 offset) override;

        NVFoundationBase &GetFoundation() override { return m_Foundation; }
        qt3ds::foundation::IStringTable &GetStringTable() override { return m_StringTable; }
        NVAllocatorCallback &GetAllocator() override { return m_Foundation.getAllocator(); }
        QSurfaceFormat format() const override
        {
            return m_backend->format();
        }
        virtual void resetStates()
        {
            PushPropertySet();
            PopPropertySet(true);
        }
    };
}
}
#endif
