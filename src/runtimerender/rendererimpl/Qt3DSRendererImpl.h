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
#ifndef QT3DS_RENDER_SHADER_GENERATOR_IMPL_H
#define QT3DS_RENDER_SHADER_GENERATOR_IMPL_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderer.h"
#include "Qt3DSRenderableObjects.h"
#include "Qt3DSRendererImplShaders.h"
#include "Qt3DSRendererImplLayerRenderData.h"
#include "foundation/Qt3DSFlags.h"
#include "Qt3DSRenderMesh.h"
#include "Qt3DSRenderModel.h"
#include "foundation/Qt3DSBounds3.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "Qt3DSRenderDefaultMaterial.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSInvasiveSet.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSDataRef.h"
#include "Qt3DSRenderLayer.h"
#include "Qt3DSRenderRay.h"
#include "Qt3DSRenderText.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderShaderCache.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "Qt3DSRendererImplLayerRenderHelper.h"
#include "Qt3DSRenderWidgets.h"
#include "Qt3DSRenderShaderCodeGenerator.h"
#include "Qt3DSRenderClippingFrustum.h"
#include "foundation/Qt3DSUnionCast.h"
#include "foundation/FastAllocator.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "Qt3DSRenderShaderKeys.h"
#include "Qt3DSRenderShaderCache.h"
#include "Qt3DSRenderProfiler.h"
#include "Qt3DSRenderDefaultMaterialShaderGenerator.h"
#include "Qt3DSRenderLight.h"

namespace qt3ds {
namespace render {

    inline bool FloatLessThan(QT3DSF32 lhs, QT3DSF32 rhs)
    {
        QT3DSF32 diff = lhs - rhs;
        if (fabs(diff) < .001)
            return false;
        return diff < 0.0f ? true : false;
    }
    inline bool ISRenderObjectPtrLessThan(const SRenderableObject *lhs,
                                          const SRenderableObject *rhs)
    {
        return FloatLessThan(lhs->m_CameraDistanceSq, rhs->m_CameraDistanceSq);
    }
    inline bool ISRenderObjectPtrGreatThan(const SRenderableObject *lhs,
                                           const SRenderableObject *rhs)
    {
        return FloatLessThan(rhs->m_CameraDistanceSq, lhs->m_CameraDistanceSq);
    }
    inline bool NonZero(float inValue) { return fabs(inValue) > .001f; }
    inline bool NonZero(QT3DSU32 inValue) { return inValue != 0; }
    inline bool IsZero(float inValue) { return fabs(inValue) < .001f; }
    inline bool IsNotOne(float inValue) { return fabs(1.0f - inValue) > .001f; }

    inline bool IsRectEdgeInBounds(QT3DSI32 inNewRectOffset, QT3DSI32 inNewRectWidth,
                                   QT3DSI32 inCurrentRectOffset, QT3DSI32 inCurrentRectWidth)
    {
        QT3DSI32 newEnd = inNewRectOffset + inNewRectWidth;
        QT3DSI32 currentEnd = inCurrentRectOffset + inCurrentRectWidth;
        return inNewRectOffset >= inCurrentRectOffset && newEnd <= currentEnd;
    }

    struct STextRenderHelper
    {
        STextShader *m_Shader;
        NVRenderInputAssembler &m_QuadInputAssembler;
        STextRenderHelper(STextShader *inShader, NVRenderInputAssembler &inQuadInputAssembler)
            : m_Shader(inShader)
            , m_QuadInputAssembler(inQuadInputAssembler)
        {
        }
    };

    struct SPickResultProcessResult : public Qt3DSRenderPickResult
    {
        SPickResultProcessResult(const Qt3DSRenderPickResult &inSrc)
            : Qt3DSRenderPickResult(inSrc)
            , m_WasPickConsumed(false)
        {
        }
        SPickResultProcessResult()
            : m_WasPickConsumed(false)
        {
        }
        bool m_WasPickConsumed;
    };

    struct STextShaderPtr
    {
        NVAllocatorCallback &m_Allocator;
        bool m_HasGeneratedShader;
        STextShader *m_Shader;
        STextShaderPtr(NVAllocatorCallback &alloc)
            : m_Allocator(alloc)
            , m_HasGeneratedShader(false)
            , m_Shader(NULL)
        {
        }
        bool HasGeneratedShader() { return m_HasGeneratedShader; }
        void Set(STextShader *inShader)
        {
            m_Shader = inShader;
            m_HasGeneratedShader = true;
        }
        ~STextShaderPtr()
        {
            if (m_Shader)
                NVDelete(m_Allocator, m_Shader);
        }
        operator STextShader *() { return m_Shader; }
    };

    class QT3DS_AUTOTEST_EXPORT Qt3DSRendererImpl : public IQt3DSRenderer,
                                                    public IRenderWidgetContext
    {
        typedef nvhash_map<SShaderDefaultMaterialKey, SShaderGeneratorGeneratedShader *> TShaderMap;
        typedef nvhash_map<SShaderDefaultMaterialKey, SRenderableDepthPrepassShader *>
            TShadowShaderMap;
        typedef nvhash_map<CRegisteredString, NVScopedRefCounted<NVRenderConstantBuffer>>
            TStrConstanBufMap;
        typedef nvhash_map<SRenderInstanceId, NVScopedRefCounted<SLayerRenderData>,
                           eastl::hash<SRenderInstanceId>> TInstanceRenderMap;
        typedef nvvector<SLayerRenderData *> TLayerRenderList;
        typedef nvvector<Qt3DSRenderPickResult> TPickResultArray;

        // Items to implement the widget context.
        typedef nvhash_map<CRegisteredString, NVScopedRefCounted<NVRenderVertexBuffer>>
            TStrVertBufMap;
        typedef nvhash_map<CRegisteredString, NVScopedRefCounted<NVRenderIndexBuffer>>
            TStrIndexBufMap;
        typedef nvhash_map<CRegisteredString, NVScopedRefCounted<NVRenderShaderProgram>>
            TStrShaderMap;
        typedef nvhash_map<CRegisteredString, NVScopedRefCounted<NVRenderInputAssembler>> TStrIAMap;

        typedef nvhash_map<long, SNode *> TBoneIdNodeMap;

        IQt3DSRenderContext &m_qt3dsContext;
        NVScopedRefCounted<NVRenderContext> m_Context;
        NVScopedRefCounted<IBufferManager> m_BufferManager;
        NVScopedRefCounted<IOffscreenRenderManager> m_OffscreenRenderManager;
        NVScopedRefCounted<IStringTable> m_StringTable;
        InvasiveSet<SShaderGeneratorGeneratedShader, SGGSGet, SGGSSet> m_LayerShaders;
        // For rendering bounding boxes.
        NVScopedRefCounted<NVRenderVertexBuffer> m_BoxVertexBuffer;
        NVScopedRefCounted<NVRenderIndexBuffer> m_BoxIndexBuffer;
        NVScopedRefCounted<NVRenderShaderProgram> m_BoxShader;
        NVScopedRefCounted<NVRenderShaderProgram> m_ScreenRectShader;

        NVScopedRefCounted<NVRenderVertexBuffer> m_AxisVertexBuffer;
        NVScopedRefCounted<NVRenderShaderProgram> m_AxisShader;

        // X,Y quad, broken down into 2 triangles and normalized over
        //-1,1.
        NVScopedRefCounted<NVRenderVertexBuffer> m_QuadVertexBuffer;
        NVScopedRefCounted<NVRenderIndexBuffer> m_QuadIndexBuffer;
        NVScopedRefCounted<NVRenderIndexBuffer> m_RectIndexBuffer;
        NVScopedRefCounted<NVRenderInputAssembler> m_QuadInputAssembler;
        NVScopedRefCounted<NVRenderInputAssembler> m_RectInputAssembler;
        NVScopedRefCounted<NVRenderAttribLayout> m_QuadAttribLayout;
        NVScopedRefCounted<NVRenderAttribLayout> m_RectAttribLayout;

        // X,Y triangle strip quads in screen coord dynamiclly setup
        NVScopedRefCounted<NVRenderVertexBuffer> m_QuadStripVertexBuffer;
        NVScopedRefCounted<NVRenderInputAssembler> m_QuadStripInputAssembler;
        NVScopedRefCounted<NVRenderAttribLayout> m_QuadStripAttribLayout;

        // X,Y,Z point which is used for instanced based rendering of points
        NVScopedRefCounted<NVRenderVertexBuffer> m_PointVertexBuffer;
        NVScopedRefCounted<NVRenderInputAssembler> m_PointInputAssembler;
        NVScopedRefCounted<NVRenderAttribLayout> m_PointAttribLayout;

        Option<NVScopedRefCounted<SLayerSceneShader>> m_SceneLayerShader;
        Option<NVScopedRefCounted<SLayerProgAABlendShader>> m_LayerProgAAShader;

        TShaderMap m_Shaders;
        TShaderMap m_DepthShaders;
        TShadowShaderMap m_ShadowMapShaders;
        TShadowShaderMap m_ShadowCubeShaders;
        TStrConstanBufMap m_ConstantBuffers; ///< store the the shader constant buffers
        // Option is true if we have attempted to generate the shader.
        // This does not mean we were successul, however.
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_DepthPrepassShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_DepthPrepassShaderDisplaced;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_DepthTessLinearPrepassShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>>
            m_DepthTessLinearPrepassShaderDisplaced;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_DepthTessPhongPrepassShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_DepthTessNPatchPrepassShader;
        Option<NVScopedRefCounted<STextDepthShader>> m_TextDepthPrepassShader;
        Option<NVScopedRefCounted<SDefaultAoPassShader>> m_DefaultAoPassShader;
        Option<NVScopedRefCounted<SDefaultAoPassShader>> m_FakeDepthShader;
        Option<NVScopedRefCounted<SDefaultAoPassShader>> m_FakeCubemapDepthShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_ParaboloidDepthShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_ParaboloidDepthTessLinearShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_ParaboloidDepthTessPhongShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_ParaboloidDepthTessNPatchShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_CubemapDepthShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_CubemapDepthTessLinearShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_CubemapDepthTessPhongShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_CubemapDepthTessNPatchShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>> m_OrthographicDepthShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>>
            m_OrthographicDepthTessLinearShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>>
            m_OrthographicDepthTessPhongShader;
        Option<NVScopedRefCounted<SRenderableDepthPrepassShader>>
            m_OrthographicDepthTessNPatchShader;
        Option<NVScopedRefCounted<SShadowmapPreblurShader>> m_CubeShadowBlurXShader;
        Option<NVScopedRefCounted<SShadowmapPreblurShader>> m_CubeShadowBlurYShader;
        Option<NVScopedRefCounted<SShadowmapPreblurShader>> m_OrthoShadowBlurXShader;
        Option<NVScopedRefCounted<SShadowmapPreblurShader>> m_OrthoShadowBlurYShader;

#ifdef ADVANCED_BLEND_SW_FALLBACK
        Option<NVScopedRefCounted<SAdvancedModeBlendShader>> m_AdvancedModeOverlayBlendShader;
        Option<NVScopedRefCounted<SAdvancedModeBlendShader>> m_AdvancedModeColorBurnBlendShader;
        Option<NVScopedRefCounted<SAdvancedModeBlendShader>> m_AdvancedModeColorDodgeBlendShader;
#endif
        // Text shaders may be generated on demand.
        STextShaderPtr m_TextShader;
        STextShaderPtr m_TextPathShader;
        STextShaderPtr m_TextWidgetShader;
        STextShaderPtr m_TextOnscreenShader;

        // Overlay used to render all widgets.
        NVRenderRect m_BeginFrameViewport;
        NVScopedRefCounted<NVRenderTexture2D> m_WidgetTexture;
        NVScopedRefCounted<NVRenderFrameBuffer> m_WidgetFBO;

#ifdef ADVANCED_BLEND_SW_FALLBACK
        // Advanced blend mode SW fallback
        CResourceTexture2D m_LayerBlendTexture;
        NVScopedRefCounted<NVRenderFrameBuffer> m_BlendFB;
#endif
        // Allocator for temporary data that is cleared after every layer.
        TInstanceRenderMap m_InstanceRenderMap;
        TLayerRenderList m_LastFrameLayers;
        volatile QT3DSI32 mRefCount;

        // Set from the first layer.
        TPickResultArray m_LastPickResults;

        // Temporary information stored only when rendering a particular layer.
        SLayerRenderData *m_CurrentLayer;
        QT3DSMat44 m_ViewProjection;
        eastl::string m_GeneratedShaderString;

        TStrVertBufMap m_WidgetVertexBuffers;
        TStrIndexBufMap m_WidgetIndexBuffers;
        TStrShaderMap m_WidgetShaders;
        TStrIAMap m_WidgetInputAssembler;

        TBoneIdNodeMap m_BoneIdNodeMap;

        bool m_PickRenderPlugins;
        bool m_LayerCachingEnabled;
        bool m_LayerGPuProfilingEnabled;
        SShaderDefaultMaterialKeyProperties m_DefaultMaterialShaderKeyProperties;

        QHash<SLayer *, SLayerRenderData *> m_initialPrepareData;
        QSet<SGraphObject *> m_materialClearDirty;

        bool m_alphaTest = false;
        float m_alphaOp = 1.0f;
        float m_alphaRef = 1.0f;

    public:
        Qt3DSRendererImpl(IQt3DSRenderContext &ctx);
        virtual ~Qt3DSRendererImpl();
        SShaderDefaultMaterialKeyProperties &DefaultMaterialShaderKeyProperties()
        {
            return m_DefaultMaterialShaderKeyProperties;
        }

        // NVRefCounted
        void addRef() override;
        void release() override;

        void EnableLayerCaching(bool inEnabled) override { m_LayerCachingEnabled = inEnabled; }
        bool IsLayerCachingEnabled() const override { return m_LayerCachingEnabled; }

        void EnableLayerGpuProfiling(bool inEnabled) override;
        bool IsLayerGpuProfilingEnabled() const override { return m_LayerGPuProfilingEnabled; }

        void setAlphaTest(bool enable, float op, float ref) override
        {
            m_alphaTest = enable;
            m_alphaOp = op;
            m_alphaRef = ref;
        }

        bool alphaTestEnabled() const
        {
            return m_alphaTest;
        }

        QT3DSVec2 alphaOpRef() const
        {
            return QT3DSVec2(m_alphaOp, m_alphaRef);
        }

        // Calls prepare layer for render
        // and then do render layer.
        bool PrepareLayerForRender(SLayer &inLayer, const QT3DSVec2 &inViewportDimensions,
                                   bool inRenderSiblings, const SRenderInstanceId id) override;
        void RenderLayer(SLayer &inLayer, const QT3DSVec2 &inViewportDimensions,
                         bool clear, QT3DSVec4 clearColor, bool inRenderSiblings,
                         const SRenderInstanceId id) override;
        void ChildrenUpdated(SNode &inParent) override;
        QT3DSF32 GetTextScale(const SText &inText) override;

        SCamera *GetCameraForNode(const SNode &inNode) const override;
        Option<SCuboidRect> GetCameraBounds(const SGraphObject &inObject) override;
        virtual SLayer *GetLayerForNode(const SNode &inNode) const;
        SLayerRenderData *GetOrCreateLayerRenderDataForNode(const SNode &inNode,
                                                            const SRenderInstanceId id = nullptr);

        IRenderWidgetContext &GetRenderWidgetContext()
        {
            return *this;
        }

        void BeginFrame() override;
        void EndFrame() override;

        void PickRenderPlugins(bool inPick) override { m_PickRenderPlugins = inPick; }
        Qt3DSRenderPickResult Pick(SLayer &inLayer, const QT3DSVec2 &inViewportDimensions,
                                   const QT3DSVec2 &inMouseCoords, bool inPickSiblings,
                                   bool inPickEverything,
                                   const SRenderInstanceId id) override;

        virtual Option<QT3DSVec2>
        FacePosition(SNode &inNode, NVBounds3 inBounds, const QT3DSMat44 &inGlobalTransform,
                     const QT3DSVec2 &inViewportDimensions, const QT3DSVec2 &inMouseCoords,
                     NVDataRef<SGraphObject *> inMapperObjects, SBasisPlanes::Enum inPlane) override;

        virtual Qt3DSRenderPickResult PickOffscreenLayer(SLayer &inLayer,
                                                        const QT3DSVec2 &inViewportDimensions,
                                                        const QT3DSVec2 &inMouseCoords,
                                                        bool inPickEverything);

        QT3DSVec3 UnprojectToPosition(SNode &inNode, QT3DSVec3 &inPosition,
                                           const QT3DSVec2 &inMouseVec) const override;
        QT3DSVec3 UnprojectWithDepth(SNode &inNode, QT3DSVec3 &inPosition,
                                          const QT3DSVec3 &inMouseVec) const override;
        QT3DSVec3 ProjectPosition(SNode &inNode, const QT3DSVec3 &inPosition) const override;

        Option<SLayerPickSetup> GetLayerPickSetup(SLayer &inLayer,
                                                          const QT3DSVec2 &inMouseCoords,
                                                          const QSize &inPickDims) override;

        Option<NVRenderRectF> GetLayerRect(SLayer &inLayer) override;

        void RunLayerRender(SLayer &inLayer, const QT3DSMat44 &inViewProjection) override;

        // Note that this allocator is completely reset on BeginFrame.
        NVAllocatorCallback &GetPerFrameAllocator() override
        {
            return m_qt3dsContext.GetPerFrameAllocator();
        }
        void RenderLayerRect(SLayer &inLayer, const QT3DSVec3 &inColor) override;
        void AddRenderWidget(IRenderWidget &inWidget) override;

        SScaleAndPosition GetWorldToPixelScaleFactor(SLayer &inLayer,
                                                             const QT3DSVec3 &inWorldPoint) override;
        SScaleAndPosition GetWorldToPixelScaleFactor(const SCamera &inCamera,
                                                     const QT3DSVec3 &inWorldPoint,
                                                     SLayerRenderData &inRenderData);

        void ReleaseLayerRenderResources(SLayer &inLayer, const SRenderInstanceId id) override;

        void RenderQuad(const QT3DSVec2 inDimensions, const QT3DSMat44 &inMVP,
                                NVRenderTexture2D &inQuadTexture) override;
        void RenderQuad() override;

        void RenderPointsIndirect() override;

        // render a screen aligned 2D text
        void RenderText2D(QT3DSF32 x, QT3DSF32 y, qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor,
                                  const char *text) override;
        bool PrepareTextureAtlasForRender();

        // render Gpu profiler values
        void RenderGpuProfilerStats(QT3DSF32 x, QT3DSF32 y,
                                            qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor) override;

        // Callback during the layer render process.
        void LayerNeedsFrameClear(SLayerRenderData &inLayer);
        void BeginLayerDepthPassRender(SLayerRenderData &inLayer);
        void EndLayerDepthPassRender();
        void BeginLayerRender(SLayerRenderData &inLayer);
        void EndLayerRender();
        void PrepareImageForIbl(SImage &inImage);
        void addMaterialDirtyClear(SGraphObject *obj);

        NVRenderShaderProgram *CompileShader(CRegisteredString inName, const char8_t *inVert,
                                             const char8_t *inFrame);

        NVRenderShaderProgram *GenerateShader(SSubsetRenderable &inRenderable,
                                              TShaderFeatureSet inFeatureSet, bool depth);
        NVRenderShaderProgram *GenerateShadowShader(SSubsetRenderable &inRenderable,
                                                    TShaderFeatureSet inFeatureSet,
                                                    RenderLightTypes::Enum lightType);
        SShaderGeneratorGeneratedShader *GetShader(SSubsetRenderable &inRenderable,
                                                   TShaderFeatureSet inFeatureSet, bool depth);
        SRenderableDepthPrepassShader *GetShadowShader(SSubsetRenderable &inRenderable,
                                                       TShaderFeatureSet inFeatureSet,
                                                       RenderLightTypes::Enum lightType);

        SDefaultAoPassShader *GetDefaultAoPassShader(TShaderFeatureSet inFeatureSet);
        SDefaultAoPassShader *GetFakeDepthShader(TShaderFeatureSet inFeatureSet);
        SDefaultAoPassShader *GetFakeCubeDepthShader(TShaderFeatureSet inFeatureSet);

        SRenderableDepthPrepassShader *GetParaboloidDepthShader(TessModeValues::Enum inTessMode);
        SRenderableDepthPrepassShader *GetParaboloidDepthNoTessShader();
        SRenderableDepthPrepassShader *GetParaboloidDepthTessLinearShader();
        SRenderableDepthPrepassShader *GetParaboloidDepthTessPhongShader();
        SRenderableDepthPrepassShader *GetParaboloidDepthTessNPatchShader();
        SRenderableDepthPrepassShader *GetCubeShadowDepthShader(TessModeValues::Enum inTessMode);
        SRenderableDepthPrepassShader *GetCubeDepthNoTessShader();
        SRenderableDepthPrepassShader *GetCubeDepthTessLinearShader();
        SRenderableDepthPrepassShader *GetCubeDepthTessPhongShader();
        SRenderableDepthPrepassShader *GetCubeDepthTessNPatchShader();
        SRenderableDepthPrepassShader *GetOrthographicDepthShader(TessModeValues::Enum inTessMode);
        SRenderableDepthPrepassShader *GetOrthographicDepthNoTessShader();
        SRenderableDepthPrepassShader *GetOrthographicDepthTessLinearShader();
        SRenderableDepthPrepassShader *GetOrthographicDepthTessPhongShader();
        SRenderableDepthPrepassShader *GetOrthographicDepthTessNPatchShader();

        SRenderableDepthPrepassShader *GetDepthPrepassShader(bool inDisplaced);
        SRenderableDepthPrepassShader *GetDepthTessPrepassShader(TessModeValues::Enum inTessMode,
                                                                 bool inDisplaced);
        SRenderableDepthPrepassShader *GetDepthTessLinearPrepassShader(bool inDisplaced);
        SRenderableDepthPrepassShader *GetDepthTessPhongPrepassShader();
        SRenderableDepthPrepassShader *GetDepthTessNPatchPrepassShader();
        STextDepthShader *GetTextDepthShader();
        STextRenderHelper GetShader(STextRenderable &inRenderable, bool inUsePathRendering);
        STextRenderHelper GetTextShader(bool inUsePathRendering);
        STextRenderHelper GetTextWidgetShader();
        STextRenderHelper GetOnscreenTextShader();
        SLayerSceneShader *GetSceneLayerShader();
        NVRenderShaderProgram *GetTextAtlasEntryShader();
        void GenerateXYQuad();
        void GenerateXYQuadStrip();
        void GenerateXYZPoint();
        eastl::pair<NVRenderVertexBuffer *, NVRenderIndexBuffer *> GetXYQuad();
        SLayerProgAABlendShader *GetLayerProgAABlendShader();
        SShadowmapPreblurShader *GetCubeShadowBlurXShader();
        SShadowmapPreblurShader *GetCubeShadowBlurYShader();
        SShadowmapPreblurShader *GetOrthoShadowBlurXShader();
        SShadowmapPreblurShader *GetOrthoShadowBlurYShader();

#ifdef ADVANCED_BLEND_SW_FALLBACK
        SAdvancedModeBlendShader *GetAdvancedBlendModeShader(AdvancedBlendModes::Enum blendMode);
        SAdvancedModeBlendShader *GetOverlayBlendModeShader();
        SAdvancedModeBlendShader *GetColorBurnBlendModeShader();
        SAdvancedModeBlendShader *GetColorDodgeBlendModeShader();
#endif
        SLayerRenderData *GetLayerRenderData() { return m_CurrentLayer; }
        SLayerGlobalRenderProperties GetLayerGlobalRenderProperties();
        void UpdateCbAoShadow(const SLayer *pLayer, const SCamera *pCamera,
                              CResourceTexture2D &inDepthTexture);

        NVRenderContext &GetContext() { return *m_Context; }

        IQt3DSRenderContext &GetQt3DSContext() { return m_qt3dsContext; }

        void DrawScreenRect(NVRenderRectF inRect, const QT3DSVec3 &inColor);
        // Binds an offscreen texture.  Widgets are rendered last.
        void SetupWidgetLayer();

#ifdef ADVANCED_BLEND_SW_FALLBACK
        NVScopedRefCounted<NVRenderTexture2D> GetLayerBlendTexture()
        {
            return m_LayerBlendTexture.GetTexture();
        }

        NVScopedRefCounted<NVRenderFrameBuffer> GetBlendFB()
        {
            return m_BlendFB;
        }
#endif
        // widget context implementation
        virtual NVRenderVertexBuffer &
        GetOrCreateVertexBuffer(CRegisteredString &inStr, QT3DSU32 stride,
                                NVConstDataRef<QT3DSU8> bufferData = NVConstDataRef<QT3DSU8>()) override;
        virtual NVRenderIndexBuffer &
        GetOrCreateIndexBuffer(CRegisteredString &inStr,
                               qt3ds::render::NVRenderComponentTypes::Enum componentType, size_t size,
                               NVConstDataRef<QT3DSU8> bufferData = NVConstDataRef<QT3DSU8>()) override;
        virtual NVRenderAttribLayout &
        CreateAttributeLayout(NVConstDataRef<qt3ds::render::NVRenderVertexBufferEntry> attribs) override;
        virtual NVRenderInputAssembler &
        GetOrCreateInputAssembler(CRegisteredString &inStr, NVRenderAttribLayout *attribLayout,
                                  NVConstDataRef<NVRenderVertexBuffer *> buffers,
                                  const NVRenderIndexBuffer *indexBuffer,
                                  NVConstDataRef<QT3DSU32> strides, NVConstDataRef<QT3DSU32> offsets) override;

        NVRenderVertexBuffer *GetVertexBuffer(CRegisteredString &inStr) override;
        NVRenderIndexBuffer *GetIndexBuffer(CRegisteredString &inStr) override;
        NVRenderInputAssembler *GetInputAssembler(CRegisteredString &inStr) override;

        NVRenderShaderProgram *GetShader(CRegisteredString inStr) override;
        NVRenderShaderProgram *CompileAndStoreShader(CRegisteredString inStr) override;
        IShaderProgramGenerator &GetProgramGenerator() override;

        STextDimensions MeasureText(const STextRenderInfo &inText) override;
        void RenderText(const STextRenderInfo &inText, const QT3DSVec3 &inTextColor,
                                const QT3DSVec3 &inBackgroundColor, const QT3DSMat44 &inMVP) override;

        // Given a node and a point in the node's local space (most likely its pivot point), we
        // return
        // a normal matrix so you can get the axis out, a transformation from node to camera
        // a new position and a floating point scale factor so you can render in 1/2 perspective
        // mode
        // or orthographic mode if you would like to.
        virtual SWidgetRenderInformation
        GetWidgetRenderInformation(SNode &inNode, const QT3DSVec3 &inPos,
                                   RenderWidgetModes::Enum inWidgetMode) override;

        Option<QT3DSVec2> GetLayerMouseCoords(SLayer &inLayer, const QT3DSVec2 &inMouseCoords,
                                                   const QT3DSVec2 &inViewportDimensions,
                                                   bool forceImageIntersect = false) const override;

    protected:
        Option<QT3DSVec2> GetLayerMouseCoords(SLayerRenderData &inLayer, const QT3DSVec2 &inMouseCoords,
                                           const QT3DSVec2 &inViewportDimensions,
                                           bool forceImageIntersect = false) const;
        SPickResultProcessResult ProcessPickResultList(bool inPickEverything);
        // If the mouse y coordinates need to be flipped we expect that to happen before entry into
        // this function
        void GetLayerHitObjectList(SLayerRenderData &inLayer, const QT3DSVec2 &inViewportDimensions,
                                   const QT3DSVec2 &inMouseCoords, bool inPickEverything,
                                   TPickResultArray &outIntersectionResult,
                                   NVAllocatorCallback &inTempAllocator);
        void IntersectRayWithSubsetRenderable(const SRay &inRay,
                                              SRenderableObject &inRenderableObject,
                                              TPickResultArray &outIntersectionResultList,
                                              NVAllocatorCallback &inTempAllocator);
    };
}
}

#endif
