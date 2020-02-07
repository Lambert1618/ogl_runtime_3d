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

#include "Qt3DSRender.h"
#include "Qt3DSRenderer.h"
#include "Qt3DSRendererImpl.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderImage.h"
#include "Qt3DSRenderBufferManager.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "EASTL/sort.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderScene.h"
#include "Qt3DSRenderPresentation.h"
#include "Qt3DSRenderEffect.h"
#include "Qt3DSRenderEffectSystem.h"
#include "Qt3DSRenderResourceManager.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "Qt3DSRenderTextTextureCache.h"
#include "Qt3DSRenderTextTextureAtlas.h"
#include "Qt3DSRenderMaterialHelpers.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderRenderList.h"
#include "Qt3DSRenderPath.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "Qt3DSRenderDefaultMaterialShaderGenerator.h"
#include "backends/gl/Qt3DSOpenGLUtil.h"
#include <stdlib.h>

#ifdef _WIN32
#pragma warning(disable : 4355)
#endif

// Quick tests you can run to find performance problems

//#define QT3DS_RENDER_DISABLE_HARDWARE_BLENDING 1
//#define QT3DS_RENDER_DISABLE_LIGHTING 1
//#define QT3DS_RENDER_DISABLE_TEXTURING 1
//#define QT3DS_RENDER_DISABLE_TRANSPARENCY 1
//#define QT3DS_RENDER_DISABLE_FRUSTUM_CULLING 1

// If you are fillrate bound then sorting opaque objects can help in some circumstances
//#define QT3DS_RENDER_DISABLE_OPAQUE_SORT 1

using qt3ds::foundation::CRegisteredString;

namespace qt3ds {
namespace render {

    struct SRenderableImage;
    struct SShaderGeneratorGeneratedShader;
    struct SSubsetRenderable;
    using eastl::make_pair;
    using eastl::reverse;
    using eastl::stable_sort;

    SEndlType Endl;

    static SRenderInstanceId combineLayerAndId(const SLayer *layer, const SRenderInstanceId id)
    {
        uint64_t x = (uint64_t)layer;
        x += 31u * (uint64_t)id;
        return (SRenderInstanceId)x;
    }

    Qt3DSRendererImpl::Qt3DSRendererImpl(IQt3DSRenderContext &ctx)
        : m_qt3dsContext(ctx)
        , m_Context(ctx.GetRenderContext())
        , m_BufferManager(ctx.GetBufferManager())
        , m_OffscreenRenderManager(ctx.GetOffscreenRenderManager())
        , m_StringTable(ctx.GetStringTable())
        , m_LayerShaders(ctx.GetAllocator(), "Qt3DSRendererImpl::m_LayerShaders")
        , m_Shaders(ctx.GetAllocator(), "Qt3DSRendererImpl::m_Shaders")
        , m_DepthShaders(ctx.GetAllocator(), "Qt3DSRendererImpl::m_DepthShaders")
        , m_ShadowMapShaders(ctx.GetAllocator(), "Qt3DSRendererImpl::m_ShadowMapShaders")
        , m_ShadowCubeShaders(ctx.GetAllocator(), "Qt3DSRendererImpl::m_ShadowCubeShaders")
        , m_ConstantBuffers(ctx.GetAllocator(), "Qt3DSRendererImpl::m_ConstantBuffers")
        , m_TextShader(ctx.GetAllocator())
        , m_TextPathShader(ctx.GetAllocator())
        , m_TextWidgetShader(ctx.GetAllocator())
        , m_TextOnscreenShader(ctx.GetAllocator())
#ifdef ADVANCED_BLEND_SW_FALLBACK
        , m_LayerBlendTexture(ctx.GetResourceManager())
        , m_BlendFB(nullptr)
#endif
        , m_InstanceRenderMap(ctx.GetAllocator(), "Qt3DSRendererImpl::m_InstanceRenderMap")
        , m_LastFrameLayers(ctx.GetAllocator(), "Qt3DSRendererImpl::m_LastFrameLayers")
        , mRefCount(0)
        , m_LastPickResults(ctx.GetAllocator(), "Qt3DSRendererImpl::m_LastPickResults")
        , m_CurrentLayer(nullptr)
        , m_WidgetVertexBuffers(ctx.GetAllocator(), "Qt3DSRendererImpl::m_WidgetVertexBuffers")
        , m_WidgetIndexBuffers(ctx.GetAllocator(), "Qt3DSRendererImpl::m_WidgetIndexBuffers")
        , m_WidgetShaders(ctx.GetAllocator(), "Qt3DSRendererImpl::m_WidgetShaders")
        , m_WidgetInputAssembler(ctx.GetAllocator(), "Qt3DSRendererImpl::m_WidgetInputAssembler")
        , m_BoneIdNodeMap(ctx.GetAllocator(), "Qt3DSRendererImpl::m_BoneIdNodeMap")
        , m_PickRenderPlugins(true)
        , m_LayerCachingEnabled(true)
        , m_LayerGPuProfilingEnabled(false)
    {
    }
    Qt3DSRendererImpl::~Qt3DSRendererImpl()
    {
        m_LayerShaders.clear();
        for (TShaderMap::iterator iter = m_Shaders.begin(), end = m_Shaders.end(); iter != end;
             ++iter) {
            NVDelete(m_Context->GetAllocator(), iter->second);
        }
        for (TShaderMap::iterator iter = m_DepthShaders.begin(), end = m_DepthShaders.end();
             iter != end; ++iter) {
            NVDelete(m_Context->GetAllocator(), iter->second);
        }
        for (TShadowShaderMap::iterator iter = m_ShadowMapShaders.begin(),
             end = m_ShadowMapShaders.end(); iter != end; ++iter) {
            NVDelete(m_Context->GetAllocator(), iter->second);
        }
        for (TShadowShaderMap::iterator iter = m_ShadowCubeShaders.begin(),
             end = m_ShadowCubeShaders.end(); iter != end; ++iter) {
            NVDelete(m_Context->GetAllocator(), iter->second);
        }
        m_ShadowMapShaders.clear();
        m_ShadowCubeShaders.clear();
        m_Shaders.clear();
        m_DepthShaders.clear();
        m_InstanceRenderMap.clear();
        m_ConstantBuffers.clear();
    }

    void Qt3DSRendererImpl::addRef() { atomicIncrement(&mRefCount); }

    void Qt3DSRendererImpl::release() { QT3DS_IMPLEMENT_REF_COUNT_RELEASE(m_Context->GetAllocator()); }

    void Qt3DSRendererImpl::ChildrenUpdated(SNode &inParent)
    {
        if (inParent.m_Type == GraphObjectTypes::Layer) {
            TInstanceRenderMap::iterator theIter
                = m_InstanceRenderMap.find(static_cast<SRenderInstanceId>(&inParent));
            if (theIter == m_InstanceRenderMap.end()) {
                // The layer is not in main presentation, but it might be in subpresentation
                theIter = m_InstanceRenderMap.begin();
                while (theIter != m_InstanceRenderMap.end()) {
                    if (static_cast<SNode *>(&theIter->second.mPtr->m_Layer) == &inParent)
                        break;
                    theIter++;
                }
            }
            if (theIter != m_InstanceRenderMap.end()) {
                theIter->second->m_CamerasAndLights.clear();
                theIter->second->m_RenderableNodes.clear();
            }
        } else if (inParent.m_Parent)
            ChildrenUpdated(*inParent.m_Parent);
    }

    QT3DSF32 Qt3DSRendererImpl::GetTextScale(const SText &inText)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inText);
        if (theData)
            return theData->m_TextScale;
        return 1.0f;
    }

    static inline SLayer *GetNextLayer(SLayer &inLayer)
    {
        if (inLayer.m_NextSibling && inLayer.m_NextSibling->m_Type == GraphObjectTypes::Layer)
            return static_cast<SLayer *>(inLayer.m_NextSibling);
        return nullptr;
    }

    static inline void MaybePushLayer(SLayer &inLayer, nvvector<SLayer *> &outLayerList)
    {
        inLayer.CalculateGlobalVariables();
        if (inLayer.m_Flags.IsGloballyActive() && inLayer.m_Flags.IsLayerRenderToTarget())
            outLayerList.push_back(&inLayer);
    }
    static void BuildRenderableLayers(SLayer &inLayer, nvvector<SLayer *> &renderableLayers,
                                      bool inRenderSiblings)
    {
        MaybePushLayer(inLayer, renderableLayers);
        if (inRenderSiblings) {
            for (SLayer *theNextLayer = GetNextLayer(inLayer); theNextLayer;
                 theNextLayer = GetNextLayer(*theNextLayer))
                MaybePushLayer(*theNextLayer, renderableLayers);
        }
    }

    void Qt3DSRendererImpl::EnableLayerGpuProfiling(bool inEnabled)
    {
        if (m_LayerGPuProfilingEnabled != inEnabled) {
            TInstanceRenderMap::iterator theIter;
            for (theIter = m_InstanceRenderMap.begin(); theIter != m_InstanceRenderMap.end();
                 theIter++) {
                SLayerRenderData *data = theIter->second;
                if (!inEnabled)
                    data->m_LayerProfilerGpu = nullptr;
                else
                    data->CreateGpuProfiler();
            }
        }
        m_LayerGPuProfilingEnabled = inEnabled;
    }

    bool Qt3DSRendererImpl::PrepareLayerForRender(SLayer &inLayer,
                                                  const QT3DSVec2 &inViewportDimensions,
                                                  bool inRenderSiblings,
                                                  const SRenderInstanceId id)
    {
        (void)inViewportDimensions;
        nvvector<SLayer *> renderableLayers(m_qt3dsContext.GetPerFrameAllocator(), "LayerVector");
        // Found by fair roll of the dice.
        renderableLayers.reserve(4);

        BuildRenderableLayers(inLayer, renderableLayers, inRenderSiblings);

        bool retval = false;

        for (nvvector<SLayer *>::reverse_iterator iter = renderableLayers.rbegin(),
                                                  end = renderableLayers.rend();
             iter != end; ++iter) {
            // Store the previous state of if we were rendering a layer.
            SLayer *theLayer = *iter;
            SLayerRenderData *theRenderData = GetOrCreateLayerRenderDataForNode(*theLayer, id);

            if (theRenderData) {
                bool needsRender = theRenderData->PrepareForRender();
                if (id) {
                    if (m_initialPrepareData.contains(theLayer)) {
                        // Copy dirty state from the initial since the graph is
                        // not dirty for subsequent calls
                        auto &flags = theRenderData->m_LayerPrepResult->m_Flags;
                        const auto &initialFlags
                                = m_initialPrepareData[theLayer]->m_LayerPrepResult->m_Flags;
                        flags.SetWasDirty(flags.WasDirty() || initialFlags.WasDirty());
                        flags.SetLayerDataDirty(flags.WasLayerDataDirty()
                                                || initialFlags.WasLayerDataDirty());
                    } else {
                        m_initialPrepareData.insert(theLayer, theRenderData);
                    }
                }
                retval = retval || needsRender
                        || theRenderData->m_LayerPrepResult->m_Flags.WasDirty() ;
            } else {
                QT3DS_ASSERT(false);
            }
        }

        return retval;
    }

    void Qt3DSRendererImpl::RenderLayer(SLayer &inLayer, const QT3DSVec2 &inViewportDimensions,
                                        bool clear, QT3DSVec4 clearColor, bool inRenderSiblings,
                                        const SRenderInstanceId id)
    {
        (void)inViewportDimensions;
        nvvector<SLayer *> renderableLayers(m_qt3dsContext.GetPerFrameAllocator(), "LayerVector");
        // Found by fair roll of the dice.
        renderableLayers.reserve(4);

        BuildRenderableLayers(inLayer, renderableLayers, inRenderSiblings);

        NVRenderContext &theRenderContext(m_qt3dsContext.GetRenderContext());
        qt3ds::render::NVRenderFrameBuffer *theFB = theRenderContext.GetRenderTarget();
        for (nvvector<SLayer *>::reverse_iterator iter = renderableLayers.rbegin(),
             end = renderableLayers.rend();
             iter != end; ++iter) {
            SLayer *theLayer = *iter;
            SLayerRenderData *theRenderData = GetOrCreateLayerRenderDataForNode(*theLayer, id);
            SLayerRenderPreparationResult &prepRes(*theRenderData->m_LayerPrepResult);
            LayerBlendTypes::Enum layerBlend = prepRes.GetLayer()->GetLayerBlend();
#ifdef ADVANCED_BLEND_SW_FALLBACK
            if ((layerBlend == LayerBlendTypes::Overlay ||
                 layerBlend == LayerBlendTypes::ColorBurn ||
                 layerBlend == LayerBlendTypes::ColorDodge) &&
                 !theRenderContext.IsAdvancedBlendHwSupported() &&
                 !theRenderContext.IsAdvancedBlendHwSupportedKHR()) {
                // Create and set up FBO and texture for advanced blending SW fallback
                NVRenderRect viewport = theRenderContext.GetViewport();
                m_LayerBlendTexture.EnsureTexture(viewport.m_Width + viewport.m_X,
                                                  viewport.m_Height + viewport.m_Y,
                                                  NVRenderTextureFormats::RGBA8);
                if (m_BlendFB == nullptr)
                    m_BlendFB = theRenderContext.CreateFrameBuffer();
                m_BlendFB->Attach(NVRenderFrameBufferAttachments::Color0, *m_LayerBlendTexture);
                theRenderContext.SetRenderTarget(m_BlendFB);
                theRenderContext.SetScissorTestEnabled(false);
                QT3DSVec4 color(0.0f);
                if (clear)
                    color = clearColor;

                QT3DSVec4 origColor = theRenderContext.GetClearColor();
                theRenderContext.SetClearColor(color);
                theRenderContext.Clear(qt3ds::render::NVRenderClearValues::Color);
                theRenderContext.SetClearColor(origColor);
                theRenderContext.SetRenderTarget(theFB);
                break;
            } else {
                m_LayerBlendTexture.ReleaseTexture();
            }
#endif
        }
        for (nvvector<SLayer *>::reverse_iterator iter = renderableLayers.rbegin(),
                                                  end = renderableLayers.rend();
             iter != end; ++iter) {
            // Store the previous state of if we were rendering a layer.
            SLayer *theLayer = *iter;
            SLayerRenderData *theRenderData = GetOrCreateLayerRenderDataForNode(*theLayer, id);

            if (theRenderData) {
                if (theRenderData->m_LayerPrepResult->IsLayerVisible())
                    theRenderData->RunnableRenderToViewport(theFB);
            } else {
                QT3DS_ASSERT(false);
            }
        }
    }

    SLayer *Qt3DSRendererImpl::GetLayerForNode(const SNode &inNode) const
    {
        if (inNode.m_Type == GraphObjectTypes::Layer) {
            return &const_cast<SLayer &>(static_cast<const SLayer &>(inNode));
        }
        if (inNode.m_Parent)
            return GetLayerForNode(*inNode.m_Parent);
        return nullptr;
    }

    SLayerRenderData *Qt3DSRendererImpl::GetOrCreateLayerRenderDataForNode(const SNode &inNode,
                                                                           const SRenderInstanceId id)
    {
        const SLayer *theLayer = GetLayerForNode(inNode);
        if (theLayer) {
            TInstanceRenderMap::const_iterator theIter
                    = m_InstanceRenderMap.find(combineLayerAndId(theLayer, id));
            if (theIter != m_InstanceRenderMap.end())
                return const_cast<SLayerRenderData *>(theIter->second.mPtr);

            SLayerRenderData *theRenderData = QT3DS_NEW(m_Context->GetAllocator(), SLayerRenderData)(
                const_cast<SLayer &>(*theLayer), *this);
            m_InstanceRenderMap.insert(make_pair(combineLayerAndId(theLayer, id), theRenderData));

            // create a profiler if enabled
            if (IsLayerGpuProfilingEnabled() && theRenderData)
                theRenderData->CreateGpuProfiler();

            return theRenderData;
        }
        return nullptr;
    }

    SCamera *Qt3DSRendererImpl::GetCameraForNode(const SNode &inNode) const
    {
        SLayerRenderData *theLayer =
            const_cast<Qt3DSRendererImpl &>(*this).GetOrCreateLayerRenderDataForNode(inNode);
        if (theLayer)
            return theLayer->m_Camera;
        return nullptr;
    }

    Option<SCuboidRect> Qt3DSRendererImpl::GetCameraBounds(const SGraphObject &inObject)
    {
        if (GraphObjectTypes::IsNodeType(inObject.m_Type)) {
            const SNode &theNode = static_cast<const SNode &>(inObject);
            SLayerRenderData *theLayer = GetOrCreateLayerRenderDataForNode(theNode);
            if (theLayer->GetOffscreenRenderer() == false) {
                SCamera *theCamera = theLayer->m_Camera;
                if (theCamera)
                    return theCamera->GetCameraBounds(
                        theLayer->m_LayerPrepResult->GetLayerToPresentationViewport(),
                        theLayer->m_LayerPrepResult->GetPresentationDesignDimensions());
            }
        }
        return Option<SCuboidRect>();
    }

    void Qt3DSRendererImpl::DrawScreenRect(NVRenderRectF inRect, const QT3DSVec3 &inColor)
    {
        SCamera theScreenCamera;
        theScreenCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
        NVRenderRectF theViewport(m_Context->GetViewport());
        theScreenCamera.m_Flags.SetOrthographic(true);
        theScreenCamera.CalculateGlobalVariables(theViewport,
                                                 QT3DSVec2(theViewport.m_Width, theViewport.m_Height));
        GenerateXYQuad();
        if (!m_ScreenRectShader) {
            IShaderProgramGenerator &theGenerator(GetProgramGenerator());
            theGenerator.BeginProgram();
            IShaderStageGenerator &vertexGenerator(
                *theGenerator.GetStage(ShaderGeneratorStages::Vertex));
            IShaderStageGenerator &fragmentGenerator(
                *theGenerator.GetStage(ShaderGeneratorStages::Fragment));
            vertexGenerator.AddIncoming("attr_pos", "vec3");
            vertexGenerator.AddUniform("model_view_projection", "mat4");
            vertexGenerator.AddUniform("rectangle_dims", "vec3");
            vertexGenerator.Append("void main() {");
            vertexGenerator.Append(
                "\tgl_Position = model_view_projection * vec4(attr_pos * rectangle_dims, 1.0);");
            vertexGenerator.Append("}");
            fragmentGenerator.AddUniform("output_color", "vec3");
            fragmentGenerator.Append("void main() {");
            fragmentGenerator.Append("\tgl_FragColor.rgb = output_color;");
            fragmentGenerator.Append("\tgl_FragColor.a = 1.0;");
            fragmentGenerator.Append("}");
            // No flags enabled
            m_ScreenRectShader = theGenerator.CompileGeneratedShader(
                "DrawScreenRect", SShaderCacheProgramFlags(), TShaderFeatureSet());
        }
        if (m_ScreenRectShader) {
            // Fudge the rect by one pixel to ensure we see all the corners.
            if (inRect.m_Width > 1)
                inRect.m_Width -= 1;
            if (inRect.m_Height > 1)
                inRect.m_Height -= 1;
            inRect.m_X += 1;
            inRect.m_Y += 1;
            // Figure out the rect center.
            SNode theNode;

            QT3DSVec2 rectGlobalCenter = inRect.Center();
            QT3DSVec2 rectCenter(theViewport.ToNormalizedRectRelative(rectGlobalCenter));
            theNode.m_Position.x = rectCenter.x;
            theNode.m_Position.y = rectCenter.y;
            theNode.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
            theNode.CalculateGlobalVariables();
            QT3DSMat44 theViewProjection;
            theScreenCamera.CalculateViewProjectionMatrix(theViewProjection);
            QT3DSMat44 theMVP;
            QT3DSMat33 theNormal;
            theNode.CalculateMVPAndNormalMatrix(theViewProjection, theMVP, theNormal);
            m_Context->SetBlendingEnabled(false);
            m_Context->SetDepthWriteEnabled(false);
            m_Context->SetDepthTestEnabled(false);
            m_Context->SetCullingEnabled(false);
            m_Context->SetActiveShader(m_ScreenRectShader);
            m_ScreenRectShader->SetPropertyValue("model_view_projection", theMVP);
            m_ScreenRectShader->SetPropertyValue("output_color", inColor);
            m_ScreenRectShader->SetPropertyValue(
                "rectangle_dims", QT3DSVec3(inRect.m_Width / 2.0f, inRect.m_Height / 2.0f, 0.0f));
        }
        if (!m_RectInputAssembler) {
            QT3DS_ASSERT(m_QuadVertexBuffer);
            QT3DSU8 indexData[] = { 0, 1, 1, 2, 2, 3, 3, 0 };

            m_RectIndexBuffer = m_Context->CreateIndexBuffer(
                qt3ds::render::NVRenderBufferUsageType::Static,
                qt3ds::render::NVRenderComponentTypes::QT3DSU8, sizeof(indexData),
                toConstDataRef(indexData, sizeof(indexData)));

            qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
                qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                      qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
            };

            // create our attribute layout
            m_RectAttribLayout = m_Context->CreateAttributeLayout(toConstDataRef(theEntries, 1));

            QT3DSU32 strides = m_QuadVertexBuffer->GetStride();
            QT3DSU32 offsets = 0;
            m_RectInputAssembler = m_Context->CreateInputAssembler(
                m_RectAttribLayout, toConstDataRef(&m_QuadVertexBuffer.mPtr, 1), m_RectIndexBuffer,
                toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
        }

        m_Context->SetInputAssembler(m_RectInputAssembler);
        m_Context->Draw(NVRenderDrawMode::Lines, m_RectIndexBuffer->GetNumIndices(), 0);
    }

    void Qt3DSRendererImpl::SetupWidgetLayer()
    {
        NVRenderContext &theContext = m_qt3dsContext.GetRenderContext();

        if (!m_WidgetTexture) {
            IResourceManager &theManager = m_qt3dsContext.GetResourceManager();
            m_WidgetTexture = theManager.AllocateTexture2D(m_BeginFrameViewport.m_Width,
                                                           m_BeginFrameViewport.m_Height,
                                                           NVRenderTextureFormats::RGBA8);
            m_WidgetFBO = theManager.AllocateFrameBuffer();
            m_WidgetFBO->Attach(NVRenderFrameBufferAttachments::Color0,
                                NVRenderTextureOrRenderBuffer(*m_WidgetTexture));
            theContext.SetRenderTarget(m_WidgetFBO);

            // NVRenderRect theScissorRect( 0, 0, m_BeginFrameViewport.m_Width,
            // m_BeginFrameViewport.m_Height );
            // NVRenderContextScopedProperty<NVRenderRect> __scissorRect( theContext,
            // &NVRenderContext::GetScissorRect, &NVRenderContext::SetScissorRect, theScissorRect );
            qt3ds::render::NVRenderContextScopedProperty<bool> __scissorEnabled(
                theContext, &NVRenderContext::IsScissorTestEnabled,
                &NVRenderContext::SetScissorTestEnabled, false);
            m_Context->SetClearColor(QT3DSVec4(0, 0, 0, 0));
            m_Context->Clear(NVRenderClearValues::Color);

        } else
            theContext.SetRenderTarget(m_WidgetFBO);
    }

    void Qt3DSRendererImpl::BeginFrame()
    {
        for (QT3DSU32 idx = 0, end = m_LastFrameLayers.size(); idx < end; ++idx)
            m_LastFrameLayers[idx]->ResetForFrame();
        m_LastFrameLayers.clear();
        m_initialPrepareData.clear();
        for (auto *obj : qAsConst(m_materialClearDirty)) {
            if (obj->m_Type == GraphObjectTypes::DefaultMaterial)
                static_cast<SDefaultMaterial *>(obj)->m_Dirty.UpdateDirtyForFrame();
            else if (obj->m_Type == GraphObjectTypes::CustomMaterial)
                static_cast<SCustomMaterial *>(obj)->UpdateDirtyForFrame();
        }
        m_materialClearDirty.clear();
        m_BeginFrameViewport = m_qt3dsContext.GetRenderList().GetViewport();
    }
    void Qt3DSRendererImpl::EndFrame()
    {
        if (m_WidgetTexture) {
            using qt3ds::render::NVRenderContextScopedProperty;
            // Releasing the widget FBO can set it as the active frame buffer.
            NVRenderContextScopedProperty<NVRenderFrameBuffer *> __fbo(
                *m_Context, &NVRenderContext::GetRenderTarget, &NVRenderContext::SetRenderTarget);
            STextureDetails theDetails = m_WidgetTexture->GetTextureDetails();
            m_Context->SetBlendingEnabled(true);
            // Colors are expected to be non-premultiplied, so we premultiply alpha into them at
            // this point.
            m_Context->SetBlendFunction(qt3ds::render::NVRenderBlendFunctionArgument(
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha,
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha));
            m_Context->SetBlendEquation(qt3ds::render::NVRenderBlendEquationArgument(
                NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));

            m_Context->SetDepthTestEnabled(false);
            m_Context->SetScissorTestEnabled(false);
            m_Context->SetViewport(m_BeginFrameViewport);
            SCamera theCamera;
            theCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
            theCamera.m_Flags.SetOrthographic(true);
            QT3DSVec2 theTextureDims((QT3DSF32)theDetails.m_Width, (QT3DSF32)theDetails.m_Height);
            theCamera.CalculateGlobalVariables(
                NVRenderRect(0, 0, theDetails.m_Width, theDetails.m_Height), theTextureDims);
            QT3DSMat44 theViewProj;
            theCamera.CalculateViewProjectionMatrix(theViewProj);
            RenderQuad(theTextureDims, theViewProj, *m_WidgetTexture);

            IResourceManager &theManager(m_qt3dsContext.GetResourceManager());
            theManager.Release(*m_WidgetFBO);
            theManager.Release(*m_WidgetTexture);
            m_WidgetTexture = nullptr;
            m_WidgetFBO = nullptr;
        }
    }

    inline bool PickResultLessThan(const Qt3DSRenderPickResult &lhs, const Qt3DSRenderPickResult &rhs)
    {
        return FloatLessThan(lhs.m_CameraDistanceSq, rhs.m_CameraDistanceSq);
    }

    inline QT3DSF32 ClampUVCoord(QT3DSF32 inUVCoord, NVRenderTextureCoordOp::Enum inCoordOp)
    {
        if (inUVCoord > 1.0f || inUVCoord < 0.0f) {
            switch (inCoordOp) {
            default:
                QT3DS_ASSERT(false);
                break;
            case NVRenderTextureCoordOp::ClampToEdge:
                inUVCoord = NVMin(inUVCoord, 1.0f);
                inUVCoord = NVMax(inUVCoord, 0.0f);
                break;
            case NVRenderTextureCoordOp::Repeat: {
                QT3DSF32 multiplier = inUVCoord > 0.0f ? 1.0f : -1.0f;
                QT3DSF32 clamp = fabs(inUVCoord);
                clamp = clamp - floor(clamp);
                if (multiplier < 0)
                    inUVCoord = 1.0f - clamp;
                else
                    inUVCoord = clamp;
            } break;
            case NVRenderTextureCoordOp::MirroredRepeat: {
                QT3DSF32 multiplier = inUVCoord > 0.0f ? 1.0f : -1.0f;
                QT3DSF32 clamp = fabs(inUVCoord);
                if (multiplier > 0.0f)
                    clamp -= 1.0f;
                QT3DSU32 isMirrored = ((QT3DSU32)clamp) % 2 == 0;
                QT3DSF32 remainder = clamp - floor(clamp);
                inUVCoord = remainder;
                if (isMirrored) {
                    if (multiplier > 0.0f)
                        inUVCoord = 1.0f - inUVCoord;
                } else {
                    if (multiplier < 0.0f)
                        inUVCoord = 1.0f - remainder;
                }
            } break;
            }
        }
        return inUVCoord;
    }

    static eastl::pair<QT3DSVec2, QT3DSVec2>
    GetMouseCoordsAndViewportFromSubObject(QT3DSVec2 inLocalHitUVSpace,
                                           Qt3DSRenderPickSubResult &inSubResult)
    {
        QT3DSMat44 theTextureMatrix(inSubResult.m_TextureMatrix);
        QT3DSVec3 theNewUVCoords(
            theTextureMatrix.transform(QT3DSVec3(inLocalHitUVSpace.x, inLocalHitUVSpace.y, 0)));
        theNewUVCoords.x = ClampUVCoord(theNewUVCoords.x, inSubResult.m_HorizontalTilingMode);
        theNewUVCoords.y = ClampUVCoord(theNewUVCoords.y, inSubResult.m_VerticalTilingMode);
        QT3DSVec2 theViewportDimensions =
            QT3DSVec2((QT3DSF32)inSubResult.m_ViewportWidth, (QT3DSF32)inSubResult.m_ViewportHeight);
        QT3DSVec2 theMouseCoords(theNewUVCoords.x * theViewportDimensions.x,
                              (1.0f - theNewUVCoords.y) * theViewportDimensions.y);

        return eastl::make_pair(theMouseCoords, theViewportDimensions);
    }

    SPickResultProcessResult Qt3DSRendererImpl::ProcessPickResultList(bool inPickEverything)
    {
        if (m_LastPickResults.empty())
            return SPickResultProcessResult();
        // Things are rendered in a particular order and we need to respect that ordering.
        eastl::stable_sort(m_LastPickResults.begin(), m_LastPickResults.end(), PickResultLessThan);

        // We need to pick against sub objects basically somewhat recursively
        // but if we don't hit any sub objects and the parent isn't pickable then
        // we need to move onto the next item in the list.
        // We need to keep in mind that theQuery->Pick will enter this method in a later
        // stack frame so *if* we get to sub objects we need to pick against them but if the pick
        // completely misses *and* the parent object locally pickable is false then we need to move
        // onto the next object.

        QT3DSU32 maxPerFrameAllocationPickResultCount =
            SFastAllocator<>::SlabSize / sizeof(Qt3DSRenderPickResult);
        QT3DSU32 numToCopy =
            NVMin(maxPerFrameAllocationPickResultCount, (QT3DSU32)m_LastPickResults.size());
        QT3DSU32 numCopyBytes = numToCopy * sizeof(Qt3DSRenderPickResult);
        Qt3DSRenderPickResult *thePickResults = reinterpret_cast<Qt3DSRenderPickResult *>(
            GetPerFrameAllocator().allocate(numCopyBytes, "tempPickData", __FILE__, __LINE__));
        memCopy(thePickResults, m_LastPickResults.data(), numCopyBytes);
        m_LastPickResults.clear();
        bool foundValidResult = false;
        SPickResultProcessResult thePickResult(thePickResults[0]);
        for (size_t idx = 0; idx < numToCopy && foundValidResult == false; ++idx) {
            thePickResult = thePickResults[idx];
            // Here we do a hierarchy.  Picking against sub objects takes precedence.
            // If picking against the sub object doesn't return a valid result *and*
            // the current object isn't globally pickable then we move onto the next object returned
            // by the pick query.
            if (thePickResult.m_HitObject != nullptr && thePickResult.m_FirstSubObject != nullptr
                && m_PickRenderPlugins) {
                QT3DSVec2 theUVCoords(thePickResult.m_LocalUVCoords.x,
                                   thePickResult.m_LocalUVCoords.y);
                IOffscreenRenderer *theSubRenderer(thePickResult.m_FirstSubObject->m_SubRenderer);
                eastl::pair<QT3DSVec2, QT3DSVec2> mouseAndViewport =
                    GetMouseCoordsAndViewportFromSubObject(theUVCoords,
                                                           *thePickResult.m_FirstSubObject);
                QT3DSVec2 theMouseCoords = mouseAndViewport.first;
                QT3DSVec2 theViewportDimensions = mouseAndViewport.second;
                IGraphObjectPickQuery *theQuery = theSubRenderer->GetGraphObjectPickQuery(this);
                if (theQuery) {
                    Qt3DSRenderPickResult theInnerPickResult =
                        theQuery->Pick(theMouseCoords, theViewportDimensions, inPickEverything);
                    if (theInnerPickResult.m_HitObject) {
                        thePickResult = theInnerPickResult;
                        thePickResult.m_OffscreenRenderer = theSubRenderer;
                        foundValidResult = true;
                        thePickResult.m_WasPickConsumed = true;
                    } else if (GraphObjectTypes::IsNodeType(thePickResult.m_HitObject->m_Type)) {
                        const SNode *theNode =
                            static_cast<const SNode *>(thePickResult.m_HitObject);
                        if (theNode->m_Flags.IsGloballyPickable() == true) {
                            foundValidResult = true;
                            thePickResult.m_WasPickConsumed = true;
                        }
                    }
                } else {
                    // If the sub renderer doesn't consume the pick then we return the picked object
                    // itself.  So no matter what, if we get to here the pick was consumed.
                    thePickResult.m_WasPickConsumed = true;
                    bool wasPickConsumed =
                        theSubRenderer->Pick(theMouseCoords, theViewportDimensions, this);
                    if (wasPickConsumed) {
                        thePickResult.m_HitObject = nullptr;
                        foundValidResult = true;
                    }
                }
            } else {
                foundValidResult = true;
                thePickResult.m_WasPickConsumed = true;
            }
        }
        return thePickResult;
    }

    Qt3DSRenderPickResult Qt3DSRendererImpl::Pick(SLayer &inLayer, const QT3DSVec2 &inViewportDimensions,
                                                const QT3DSVec2 &inMouseCoords, bool inPickSiblings,
                                                bool inPickEverything, const SRenderInstanceId id)
    {
        m_LastPickResults.clear();

        SLayer *theLayer = &inLayer;
        // Stepping through how the original runtime did picking it picked layers in order
        // stopping at the first hit.  So objects on the top layer had first crack at the pick
        // vector itself.
        do {
            if (theLayer->m_Flags.IsActive()) {
                TInstanceRenderMap::iterator theIter
                        = m_InstanceRenderMap.find(combineLayerAndId(theLayer, id));
                if (theIter != m_InstanceRenderMap.end()) {
                    m_LastPickResults.clear();
                    GetLayerHitObjectList(*theIter->second, inViewportDimensions, inMouseCoords,
                                          inPickEverything, m_LastPickResults,
                                          GetPerFrameAllocator());
                    SPickResultProcessResult retval(ProcessPickResultList(inPickEverything));
                    if (retval.m_WasPickConsumed)
                        return retval;
                } else {
                    // QT3DS_ASSERT( false );
                }
            }

            if (inPickSiblings)
                theLayer = GetNextLayer(*theLayer);
            else
                theLayer = nullptr;
        } while (theLayer != nullptr);

        return Qt3DSRenderPickResult();
    }

    static inline Option<QT3DSVec2> IntersectRayWithNode(const SNode &inNode,
                                                      SRenderableObject &inRenderableObject,
                                                      const SRay &inPickRay)
    {
        if (inRenderableObject.m_RenderableFlags.IsText()) {
            STextRenderable &theRenderable = static_cast<STextRenderable &>(inRenderableObject);
            if (&theRenderable.m_Text == &inNode) {
                return inPickRay.GetRelativeXY(inRenderableObject.m_GlobalTransform,
                                               inRenderableObject.m_Bounds);
            }
#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
        } else if (inRenderableObject.m_RenderableFlags.isDistanceField()) {
            SDistanceFieldRenderable &theRenderable = static_cast<SDistanceFieldRenderable &>(
                        inRenderableObject);
            if (&theRenderable.m_text == &inNode) {
                return inPickRay.GetRelativeXY(inRenderableObject.m_GlobalTransform,
                                               inRenderableObject.m_Bounds);
            }
#endif
        } else if (inRenderableObject.m_RenderableFlags.IsDefaultMaterialMeshSubset()) {
            SSubsetRenderable &theRenderable = static_cast<SSubsetRenderable &>(inRenderableObject);
            if (&theRenderable.m_ModelContext.m_Model == &inNode)
                return inPickRay.GetRelativeXY(inRenderableObject.m_GlobalTransform,
                                               inRenderableObject.m_Bounds);
        } else if (inRenderableObject.m_RenderableFlags.IsCustomMaterialMeshSubset()) {
            SCustomMaterialRenderable &theRenderable =
                static_cast<SCustomMaterialRenderable &>(inRenderableObject);
            if (&theRenderable.m_ModelContext.m_Model == &inNode)
                return inPickRay.GetRelativeXY(inRenderableObject.m_GlobalTransform,
                                               inRenderableObject.m_Bounds);
        } else {
            QT3DS_ASSERT(false);
        }
        return Empty();
    }

    static inline Qt3DSRenderPickSubResult ConstructSubResult(SImage &inImage)
    {
        STextureDetails theDetails = inImage.m_TextureData.m_Texture->GetTextureDetails();
        return Qt3DSRenderPickSubResult(*inImage.m_LastFrameOffscreenRenderer,
                                       inImage.m_TextureTransform, inImage.m_HorizontalTilingMode,
                                       inImage.m_VerticalTilingMode, theDetails.m_Width,
                                       theDetails.m_Height);
    }

    Option<QT3DSVec2> Qt3DSRendererImpl::FacePosition(SNode &inNode, NVBounds3 inBounds,
                                                  const QT3DSMat44 &inGlobalTransform,
                                                  const QT3DSVec2 &inViewportDimensions,
                                                  const QT3DSVec2 &inMouseCoords,
                                                  NVDataRef<SGraphObject *> inMapperObjects,
                                                  SBasisPlanes::Enum inPlane)
    {
        SLayerRenderData *theLayerData = GetOrCreateLayerRenderDataForNode(inNode);
        if (theLayerData == nullptr)
            return Empty();
        // This function assumes the layer was rendered to the scene itself.  There is another
        // function
        // for completely offscreen layers that don't get rendered to the scene.
        bool wasRenderToTarget(theLayerData->m_Layer.m_Flags.IsLayerRenderToTarget());
        if (wasRenderToTarget == false || theLayerData->m_Camera == nullptr
            || theLayerData->m_LayerPrepResult.hasValue() == false
            || theLayerData->m_LastFrameOffscreenRenderer.mPtr != nullptr)
            return Empty();

        QT3DSVec2 theMouseCoords(inMouseCoords);
        QT3DSVec2 theViewportDimensions(inViewportDimensions);

        for (QT3DSU32 idx = 0, end = inMapperObjects.size(); idx < end; ++idx) {
            SGraphObject &currentObject = *inMapperObjects[idx];
            if (currentObject.m_Type == GraphObjectTypes::Layer) {
                // The layer knows its viewport so it can take the information directly.
                // This is extremely counter intuitive but a good sign.
            } else if (currentObject.m_Type == GraphObjectTypes::Image) {
                SImage &theImage = static_cast<SImage &>(currentObject);
                SModel *theParentModel = nullptr;
                if (theImage.m_Parent
                    && theImage.m_Parent->m_Type == GraphObjectTypes::DefaultMaterial) {
                    SDefaultMaterial *theMaterial =
                        static_cast<SDefaultMaterial *>(theImage.m_Parent);
                    if (theMaterial) {
                        theParentModel = theMaterial->m_Parent;
                    }
                }
                if (theParentModel == nullptr) {
                    QT3DS_ASSERT(false);
                    return Empty();
                }
                NVBounds3 theModelBounds = theParentModel->GetBounds(
                    GetQt3DSContext().GetBufferManager(), GetQt3DSContext().GetPathManager(), false);

                if (theModelBounds.isEmpty()) {
                    QT3DS_ASSERT(false);
                    return Empty();
                }
                Option<QT3DSVec2> relativeHit =
                    FacePosition(*theParentModel, theModelBounds, theParentModel->m_GlobalTransform,
                                 theViewportDimensions, theMouseCoords, NVDataRef<SGraphObject *>(),
                                 SBasisPlanes::XY);
                if (relativeHit.isEmpty()) {
                    return Empty();
                }
                Qt3DSRenderPickSubResult theResult = ConstructSubResult(theImage);
                QT3DSVec2 hitInUVSpace = (*relativeHit) + QT3DSVec2(.5f, .5f);
                eastl::pair<QT3DSVec2, QT3DSVec2> mouseAndViewport =
                    GetMouseCoordsAndViewportFromSubObject(hitInUVSpace, theResult);
                theMouseCoords = mouseAndViewport.first;
                theViewportDimensions = mouseAndViewport.second;
            }
        }

        Option<SRay> theHitRay = theLayerData->m_LayerPrepResult->GetPickRay(
            theMouseCoords, theViewportDimensions, false);
        if (theHitRay.hasValue() == false)
            return Empty();

        // Scale the mouse coords to change them into the camera's numerical space.
        SRay thePickRay = *theHitRay;
        Option<QT3DSVec2> newValue = thePickRay.GetRelative(inGlobalTransform, inBounds, inPlane);
        return newValue;
    }

    Qt3DSRenderPickResult
    Qt3DSRendererImpl::PickOffscreenLayer(SLayer &/*inLayer*/, const QT3DSVec2 & /*inViewportDimensions*/
                                         ,
                                         const QT3DSVec2 & /*inMouseCoords*/
                                         ,
                                         bool /*inPickEverything*/)
    {
        return Qt3DSRenderPickResult();
    }

    QT3DSVec3 Qt3DSRendererImpl::UnprojectToPosition(SNode &inNode, QT3DSVec3 &inPosition,
                                                 const QT3DSVec2 &inMouseVec) const
    {
        // Translate mouse into layer's coordinates
        SLayerRenderData *theData =
            const_cast<Qt3DSRendererImpl &>(*this).GetOrCreateLayerRenderDataForNode(inNode);
        if (theData == nullptr || theData->m_Camera == nullptr) {
            return QT3DSVec3(0, 0, 0);
        } // QT3DS_ASSERT( false ); return QT3DSVec3(0,0,0); }

        QSize theWindow = m_qt3dsContext.GetWindowDimensions();
        QT3DSVec2 theDims((QT3DSF32)theWindow.width(), (QT3DSF32)theWindow.height());

        SLayerRenderPreparationResult &thePrepResult(*theData->m_LayerPrepResult);
        SRay theRay = thePrepResult.GetPickRay(inMouseVec, theDims, true);

        return theData->m_Camera->UnprojectToPosition(inPosition, theRay);
    }

    QT3DSVec3 Qt3DSRendererImpl::UnprojectWithDepth(SNode &inNode, QT3DSVec3 &,
                                                const QT3DSVec3 &inMouseVec) const
    {
        // Translate mouse into layer's coordinates
        SLayerRenderData *theData =
            const_cast<Qt3DSRendererImpl &>(*this).GetOrCreateLayerRenderDataForNode(inNode);
        if (theData == nullptr || theData->m_Camera == nullptr) {
            return QT3DSVec3(0, 0, 0);
        } // QT3DS_ASSERT( false ); return QT3DSVec3(0,0,0); }

        // Flip the y into gl coordinates from window coordinates.
        QT3DSVec2 theMouse(inMouseVec.x, inMouseVec.y);
        NVReal theDepth = inMouseVec.z;

        SLayerRenderPreparationResult &thePrepResult(*theData->m_LayerPrepResult);
        QSize theWindow = m_qt3dsContext.GetWindowDimensions();
        SRay theRay = thePrepResult.GetPickRay(
            theMouse, QT3DSVec2((QT3DSF32)theWindow.width(), (QT3DSF32)theWindow.height()), true);
        QT3DSVec3 theTargetPosition = theRay.m_Origin + theRay.m_Direction * theDepth;
        if (inNode.m_Parent != nullptr && inNode.m_Parent->m_Type != GraphObjectTypes::Layer)
            theTargetPosition =
                inNode.m_Parent->m_GlobalTransform.getInverse().transform(theTargetPosition);
        // Our default global space is right handed, so if you are left handed z means something
        // opposite.
        if (inNode.m_Flags.IsLeftHanded())
            theTargetPosition.z *= -1;
        return theTargetPosition;
    }

    QT3DSVec3 Qt3DSRendererImpl::ProjectPosition(SNode &inNode, const QT3DSVec3 &inPosition) const
    {
        // Translate mouse into layer's coordinates
        SLayerRenderData *theData =
            const_cast<Qt3DSRendererImpl &>(*this).GetOrCreateLayerRenderDataForNode(inNode);
        if (theData == nullptr || theData->m_Camera == nullptr) {
            return QT3DSVec3(0, 0, 0);
        }

        QT3DSMat44 viewProj;
        theData->m_Camera->CalculateViewProjectionMatrix(viewProj);
        QT3DSVec4 projPos = viewProj.transform(QT3DSVec4(inPosition, 1.0f));
        projPos.x /= projPos.w;
        projPos.y /= projPos.w;

        NVRenderRectF theViewport = theData->m_LayerPrepResult->GetLayerToPresentationViewport();
        QT3DSVec2 theDims((QT3DSF32)theViewport.m_Width, (QT3DSF32)theViewport.m_Height);
        projPos.x += 1.0;
        projPos.y += 1.0;
        projPos.x *= 0.5;
        projPos.y *= 0.5;
        QT3DSVec3 cameraToObject = theData->m_Camera->GetGlobalPos() - inPosition;
        projPos.z = sqrtf(cameraToObject.dot(cameraToObject));
        QT3DSVec3 mouseVec = QT3DSVec3(projPos.x, projPos.y, projPos.z);
        mouseVec.x *= theDims.x;
        mouseVec.y *= theDims.y;

        mouseVec.x += theViewport.m_X;
        mouseVec.y += theViewport.m_Y;

        // Flip the y into window coordinates so it matches the mouse.
        QSize theWindow = m_qt3dsContext.GetWindowDimensions();
        mouseVec.y = theWindow.height() - mouseVec.y;

        return mouseVec;
    }

    uint Qt3DSRendererImpl::getLayerTextureId(SLayer &layer)
    {
        SLayerRenderData *data = GetOrCreateLayerRenderDataForNode(layer);
        if (data->m_LayerCachedTexture) {
            return static_cast<uint>(reinterpret_cast<size_t>(
                                         data->m_LayerCachedTexture->GetTextureObjectHandle()));
        }
        if (data->m_LayerTexture) {
            return static_cast<uint>(reinterpret_cast<size_t>(
                                         data->m_LayerTexture->GetTextureObjectHandle()));
        }
        return 0;
    }

    GLenum Qt3DSRendererImpl::getTextureGlFormat(NVRenderTextureFormats::Enum internalFormat)
    {
        auto ctxType = m_Context->GetRenderContextType();
        GLConversion conversion;
        GLenum glInternalFormat, glformat, gltype;

        if (NVRenderTextureFormats::isUncompressedTextureFormat(internalFormat)) {
            conversion.fromUncompressedTextureFormatToGL(ctxType, internalFormat,
                                                         glformat, gltype, glInternalFormat);
            return glInternalFormat;
        } else if (NVRenderTextureFormats::isCompressedTextureFormat(internalFormat)) {
            return conversion.fromCompressedTextureFormatToGL(internalFormat);
        } else if (NVRenderTextureFormats::isDepthTextureFormat(internalFormat)) {
            conversion.fromDepthTextureFormatToGL(ctxType, internalFormat, glformat,
                                                  gltype, glInternalFormat);
            return glInternalFormat;
        } else {
            return GL_INVALID_ENUM;
        }
    }

    STextureDetails Qt3DSRendererImpl::getLayerTextureDetails(SLayer &inLayer)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inLayer);

        if (theData->m_LayerCachedTexture)
            return theData->m_LayerCachedTexture->GetTextureDetails();
        if (theData->m_LayerTexture)
            return theData->m_LayerTexture->GetTextureDetails();

        return {};
    }

    Option<SLayerPickSetup> Qt3DSRendererImpl::GetLayerPickSetup(SLayer &inLayer,
                                                                const QT3DSVec2 &inMouseCoords,
                                                                const QSize &inPickDims)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inLayer);
        if (theData == nullptr || theData->m_Camera == nullptr) {
            QT3DS_ASSERT(false);
            return Empty();
        }
        QSize theWindow = m_qt3dsContext.GetWindowDimensions();
        QT3DSVec2 theDims((QT3DSF32)theWindow.width(), (QT3DSF32)theWindow.height());
        // The mouse is relative to the layer
        Option<QT3DSVec2> theLocalMouse = GetLayerMouseCoords(*theData, inMouseCoords, theDims, false);
        if (theLocalMouse.hasValue() == false) {
            return Empty();
        }

        SLayerRenderPreparationResult &thePrepResult(*theData->m_LayerPrepResult);
        if (thePrepResult.GetCamera() == nullptr) {
            return Empty();
        }
        // Perform gluPickMatrix and pre-multiply it into the view projection
        QT3DSMat44 theTransScale(QT3DSMat44::createIdentity());
        SCamera &theCamera(*thePrepResult.GetCamera());

        NVRenderRectF layerToPresentation = thePrepResult.GetLayerToPresentationViewport();
        // Offsetting is already taken care of in the camera's projection.
        // All we need to do is to scale and translate the image.
        layerToPresentation.m_X = 0;
        layerToPresentation.m_Y = 0;
        QT3DSVec2 theMouse(*theLocalMouse);
        // The viewport will need to center at this location
        QT3DSVec2 viewportDims((QT3DSF32)inPickDims.width(), (QT3DSF32)inPickDims.height());
        QT3DSVec2 bottomLeft =
            QT3DSVec2(theMouse.x - viewportDims.x / 2.0f, theMouse.y - viewportDims.y / 2.0f);
        // For some reason, and I haven't figured out why yet, the offsets need to be backwards for
        // this to work.
        // bottomLeft.x = layerToPresentation.m_Width - bottomLeft.x;
        // bottomLeft.y = layerToPresentation.m_Height - bottomLeft.y;
        // Virtual rect is relative to the layer.
        NVRenderRectF thePickRect(bottomLeft.x, bottomLeft.y, viewportDims.x, viewportDims.y);
        QT3DSMat44 projectionPremult(QT3DSMat44::createIdentity());
        projectionPremult = render::NVRenderContext::ApplyVirtualViewportToProjectionMatrix(
            projectionPremult, layerToPresentation, thePickRect);
        projectionPremult = projectionPremult.getInverse();

        QT3DSMat44 globalInverse = theCamera.m_GlobalTransform.getInverse();
        QT3DSMat44 theVP = theCamera.m_Projection * globalInverse;
        // For now we won't setup the scissor, so we may be off by inPickDims at most because
        // GetLayerMouseCoords will return
        // false if the mouse is too far off the layer.
        return SLayerPickSetup(projectionPremult, theVP,
                               NVRenderRect(0, 0, (QT3DSU32)layerToPresentation.m_Width,
                                            (QT3DSU32)layerToPresentation.m_Height));
    }

    Option<NVRenderRectF> Qt3DSRendererImpl::GetLayerRect(SLayer &inLayer)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inLayer);
        if (theData == nullptr || theData->m_Camera == nullptr) {
            QT3DS_ASSERT(false);
            return Empty();
        }
        SLayerRenderPreparationResult &thePrepResult(*theData->m_LayerPrepResult);
        return thePrepResult.GetLayerToPresentationViewport();
    }

    // This doesn't have to be cheap.
    void Qt3DSRendererImpl::RunLayerRender(SLayer &inLayer, const QT3DSMat44 &inViewProjection)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inLayer);
        if (theData == nullptr || theData->m_Camera == nullptr) {
            QT3DS_ASSERT(false);
            return;
        }
        theData->PrepareAndRender(inViewProjection);
    }

    void Qt3DSRendererImpl::AddRenderWidget(IRenderWidget &inWidget)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inWidget.GetNode());
        if (theData)
            theData->AddRenderWidget(inWidget);
    }

    void Qt3DSRendererImpl::RenderLayerRect(SLayer &inLayer, const QT3DSVec3 &inColor)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inLayer);
        if (theData)
            theData->m_BoundingRectColor = inColor;
    }

    SScaleAndPosition Qt3DSRendererImpl::GetWorldToPixelScaleFactor(const SCamera &inCamera,
                                                                   const QT3DSVec3 &inWorldPoint,
                                                                   SLayerRenderData &inRenderData)
    {
        if (inCamera.m_Flags.IsOrthographic() == true) {
            // There are situations where the camera can scale.
            return SScaleAndPosition(
                inWorldPoint,
                inCamera.GetOrthographicScaleFactor(
                    inRenderData.m_LayerPrepResult->GetLayerToPresentationViewport(),
                    inRenderData.m_LayerPrepResult->GetPresentationDesignDimensions()));
        } else {
            QT3DSVec3 theCameraPos(0, 0, 0);
            QT3DSVec3 theCameraDir(0, 0, -1);
            SRay theRay(theCameraPos, inWorldPoint - theCameraPos);
            NVPlane thePlane(theCameraDir, -600);
            QT3DSVec3 theItemPosition(inWorldPoint);
            Option<QT3DSVec3> theIntersection = theRay.Intersect(thePlane);
            if (theIntersection.hasValue())
                theItemPosition = *theIntersection;
            // The special number comes in from physically measuring how off we are on the screen.
            QT3DSF32 theScaleFactor = (1.0f / inCamera.m_Projection.column1[1]);
            SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inCamera);
            QT3DSU32 theHeight = theData->m_LayerPrepResult->GetTextureDimensions().height();
            QT3DSF32 theScaleMultiplier = 600.0f / ((QT3DSF32)theHeight / 2.0f);
            theScaleFactor *= theScaleMultiplier;

            return SScaleAndPosition(theItemPosition, theScaleFactor);
        }
    }

    SScaleAndPosition Qt3DSRendererImpl::GetWorldToPixelScaleFactor(SLayer &inLayer,
                                                                   const QT3DSVec3 &inWorldPoint)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inLayer);
        if (theData == nullptr || theData->m_Camera == nullptr) {
            QT3DS_ASSERT(false);
            return SScaleAndPosition();
        }
        return GetWorldToPixelScaleFactor(*theData->m_Camera, inWorldPoint, *theData);
    }

    void Qt3DSRendererImpl::ReleaseLayerRenderResources(SLayer &inLayer, const SRenderInstanceId id)
    {
        TInstanceRenderMap::iterator theIter
                = m_InstanceRenderMap.find(combineLayerAndId(&inLayer, id));
        if (theIter != m_InstanceRenderMap.end()) {
            TLayerRenderList::iterator theLastFrm = eastl::find(
                m_LastFrameLayers.begin(), m_LastFrameLayers.end(), theIter->second.mPtr);
            if (theLastFrm != m_LastFrameLayers.end()) {
                theIter->second->ResetForFrame();
                m_LastFrameLayers.erase(theLastFrm);
            }
            m_InstanceRenderMap.erase(theIter);
        }
    }

    void Qt3DSRendererImpl::FillQuad(const QT3DSVec4 &color)
    {
        m_Context->SetCullingEnabled(false);
        SFillRectShader *theShader = GetFillRectShader();
        NVRenderContext &theContext(*m_Context);
        theContext.SetActiveShader(&theShader->m_Shader);
        NVRenderBlendEquationArgument equ(NVRenderBlendEquation::Add, NVRenderBlendEquation::Add);
        NVRenderBlendFunctionArgument func(NVRenderSrcBlendFunc::One,
                                           NVRenderDstBlendFunc::OneMinusSrcAlpha,
                                           NVRenderSrcBlendFunc::One,
                                           NVRenderDstBlendFunc::OneMinusSrcAlpha);
        theContext.SetBlendEquation(equ);
        theContext.SetBlendFunction(func);
        theContext.SetBlendingEnabled(true);
        theShader->m_color.Set(color);
        GenerateXYQuad();
        m_Context->SetInputAssembler(m_QuadInputAssembler);
        m_Context->Draw(NVRenderDrawMode::Triangles, m_QuadIndexBuffer->GetNumIndices(), 0);
    }

    void Qt3DSRendererImpl::RenderQuad(const QT3DSVec2 inDimensions, const QT3DSMat44 &inMVP,
                                      NVRenderTexture2D &inQuadTexture)
    {
        m_Context->SetCullingEnabled(false);
        SLayerSceneShader *theShader = GetSceneLayerShader();
        NVRenderContext &theContext(*m_Context);
        theContext.SetActiveShader(&theShader->m_Shader);
        theShader->m_MVP.Set(inMVP);
        theShader->m_Dimensions.Set(inDimensions);
        theShader->m_Sampler.Set(&inQuadTexture);

        // Set anaglyph color multiplier based on mode & current eye
        // Default non-anaglyph modes use just vec4(1.0,1.0,1.0,1.0)
        auto stereoMode = m_qt3dsContext.GetStereoMode();
        auto stereoView = m_qt3dsContext.GetStereoView();
        QT3DSVec4 anaglyphColor(1.0);
        if (stereoMode == StereoModes::AnaglyphRedCyan) {
            if (stereoView == StereoViews::Left)
                anaglyphColor = QT3DSVec4(1.0, 0.0, 0.0, 0.0);
            else
                anaglyphColor = QT3DSVec4(0.0, 1.0, 1.0, 0.0);
        } else if (stereoMode == StereoModes::AnaglyphGreenMagenta) {
            if (stereoView == StereoViews::Left)
                anaglyphColor = QT3DSVec4(0.0, 1.0, 0.0, 0.0);
            else
                anaglyphColor = QT3DSVec4(1.0, 0.0, 1.0, 0.0);
        }
        theShader->m_anaglyphColor.Set(anaglyphColor);

        GenerateXYQuad();
        theContext.SetInputAssembler(m_QuadInputAssembler);
        theContext.Draw(NVRenderDrawMode::Triangles, m_QuadIndexBuffer->GetNumIndices(), 0);
    }

    void Qt3DSRendererImpl::RenderQuad()
    {
        m_Context->SetCullingEnabled(false);
        GenerateXYQuad();
        m_Context->SetInputAssembler(m_QuadInputAssembler);
        m_Context->Draw(NVRenderDrawMode::Triangles, m_QuadIndexBuffer->GetNumIndices(), 0);
    }

    void Qt3DSRendererImpl::RenderPointsIndirect()
    {
        m_Context->SetCullingEnabled(false);
        GenerateXYZPoint();
        m_Context->SetInputAssembler(m_PointInputAssembler);
        m_Context->DrawIndirect(NVRenderDrawMode::Points, 0);
    }

    void Qt3DSRendererImpl::LayerNeedsFrameClear(SLayerRenderData &inLayer)
    {
        m_LastFrameLayers.push_back(&inLayer);
    }

    void Qt3DSRendererImpl::BeginLayerDepthPassRender(SLayerRenderData &inLayer)
    {
        m_CurrentLayer = &inLayer;
    }

    void Qt3DSRendererImpl::EndLayerDepthPassRender() { m_CurrentLayer = nullptr; }

    void Qt3DSRendererImpl::BeginLayerRender(SLayerRenderData &inLayer)
    {
        m_CurrentLayer = &inLayer;
        // Remove all of the shaders from the layer shader set
        // so that we can only apply the camera and lighting properties to
        // shaders that are in the layer.
        m_LayerShaders.clear();
    }
    void Qt3DSRendererImpl::EndLayerRender() { m_CurrentLayer = nullptr; }

// Allocate an object that lasts only this frame.
#define RENDER_FRAME_NEW(type)                                                                     \
    new (m_PerFrameAllocator.m_FastAllocator.allocate(sizeof(type), __FILE__, __LINE__)) type

    void Qt3DSRendererImpl::PrepareImageForIbl(SImage &inImage)
    {
        if (inImage.m_TextureData.m_Texture && inImage.m_TextureData.m_Texture->GetNumMipmaps() < 1)
            inImage.m_TextureData.m_Texture->GenerateMipmaps();
    }

    void Qt3DSRendererImpl::addMaterialDirtyClear(SGraphObject *obj)
    {
        m_materialClearDirty.insert(obj);
    }

    bool NodeContainsBoneRoot(SNode &childNode, QT3DSI32 rootID)
    {
        for (SNode *childChild = childNode.m_FirstChild; childChild != nullptr;
             childChild = childChild->m_NextSibling) {
            if (childChild->m_SkeletonId == rootID)
                return true;
        }

        return false;
    }

    void FillBoneIdNodeMap(SNode &childNode, nvhash_map<long, SNode *> &ioMap)
    {
        if (childNode.m_SkeletonId >= 0)
            ioMap[childNode.m_SkeletonId] = &childNode;
        for (SNode *childChild = childNode.m_FirstChild; childChild != nullptr;
             childChild = childChild->m_NextSibling)
            FillBoneIdNodeMap(*childChild, ioMap);
    }

    bool Qt3DSRendererImpl::PrepareTextureAtlasForRender()
    {
        ITextTextureAtlas *theTextureAtlas = m_qt3dsContext.GetTextureAtlas();
        if (theTextureAtlas == nullptr)
            return false;

        // this is a one time creation
        if (!theTextureAtlas->IsInitialized()) {
            NVRenderContext &theContext(*m_Context);
            NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
            NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
            NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
            // temporay FB
            using qt3ds::render::NVRenderContextScopedProperty;
            NVRenderContextScopedProperty<NVRenderFrameBuffer *> __fbo(
                *m_Context, &NVRenderContext::GetRenderTarget, &NVRenderContext::SetRenderTarget);

            ITextRenderer &theTextRenderer(*m_qt3dsContext.GetOnscreenTextRenderer());
            TTextTextureAtlasDetailsAndTexture theResult = theTextureAtlas->PrepareTextureAtlas();
            if (!theResult.first.m_EntryCount) {
                QT3DS_ASSERT(theResult.first.m_EntryCount);
                return false;
            }

            // generate the index buffer we need
            GenerateXYQuad();

            qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
                qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                      qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
                qt3ds::render::NVRenderVertexBufferEntry(
                    "attr_uv", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2, 12),
            };

            // create our attribute layout
            mAttribLayout = m_Context->CreateAttributeLayout(toConstDataRef(theEntries, 2));

            NVRenderFrameBuffer *theAtlasFB(
                m_qt3dsContext.GetResourceManager().AllocateFrameBuffer());
            theAtlasFB->Attach(NVRenderFrameBufferAttachments::Color0, *theResult.second);
            m_qt3dsContext.GetRenderContext().SetRenderTarget(theAtlasFB);

            // this texture contains our single entries
            NVRenderTexture2D *theTexture = nullptr;
            if (m_Context->GetRenderContextType() == NVRenderContextValues::GLES2) {
                theTexture = m_qt3dsContext.GetResourceManager()
                        .AllocateTexture2D(32, 32, NVRenderTextureFormats::RGBA8);
            } else {
                theTexture = m_qt3dsContext.GetResourceManager()
                        .AllocateTexture2D(32, 32, NVRenderTextureFormats::Alpha8);
            }
            m_Context->SetClearColor(QT3DSVec4(0, 0, 0, 0));
            m_Context->Clear(NVRenderClearValues::Color);
            m_Context->SetDepthTestEnabled(false);
            m_Context->SetScissorTestEnabled(false);
            m_Context->SetCullingEnabled(false);
            m_Context->SetBlendingEnabled(false);
            m_Context->SetViewport(
                NVRenderRect(0, 0, theResult.first.m_TextWidth, theResult.first.m_TextHeight));

            SCamera theCamera;
            theCamera.m_ClipNear = -1.0;
            theCamera.m_ClipFar = 1.0;
            theCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
            theCamera.m_Flags.SetOrthographic(true);
            QT3DSVec2 theTextureDims((QT3DSF32)theResult.first.m_TextWidth,
                                  (QT3DSF32)theResult.first.m_TextHeight);
            theCamera.CalculateGlobalVariables(
                NVRenderRect(0, 0, theResult.first.m_TextWidth, theResult.first.m_TextHeight),
                theTextureDims);
            // We want a 2D lower left projection
            QT3DSF32 *writePtr(theCamera.m_Projection.front());
            writePtr[12] = -1;
            writePtr[13] = -1;

            // generate render stuff
            // We dynamicall update the vertex buffer
            QT3DSF32 tempBuf[20];
            QT3DSF32 *bufPtr = tempBuf;
            QT3DSU32 bufSize = 20 * sizeof(QT3DSF32); // 4 vertices  3 pos 2 tex
            NVDataRef<QT3DSU8> vertData((QT3DSU8 *)bufPtr, bufSize);
            mVertexBuffer = theContext.CreateVertexBuffer(
                qt3ds::render::NVRenderBufferUsageType::Dynamic, 20 * sizeof(QT3DSF32),
                3 * sizeof(QT3DSF32) + 2 * sizeof(QT3DSF32), vertData);
            QT3DSU32 strides = mVertexBuffer->GetStride();
            QT3DSU32 offsets = 0;
            mInputAssembler = theContext.CreateInputAssembler(
                mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), m_QuadIndexBuffer.mPtr,
                toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));

            NVRenderShaderProgram *theShader = GetTextAtlasEntryShader();
            STextShader theTextShader(*theShader);

            if (theShader) {
                theContext.SetActiveShader(theShader);
                theTextShader.m_MVP.Set(theCamera.m_Projection);

                // we are going through all entries and render to the FBO
                for (QT3DSU32 i = 0; i < theResult.first.m_EntryCount; i++) {
                    STextTextureAtlasEntryDetails theDetails =
                        theTextRenderer.RenderAtlasEntry(i, *theTexture);
                    // update vbo
                    // we need to mirror coordinates
                    QT3DSF32 x1 = (QT3DSF32)theDetails.m_X;
                    QT3DSF32 x2 = (QT3DSF32)theDetails.m_X + theDetails.m_TextWidth;
                    QT3DSF32 y1 = (QT3DSF32)theDetails.m_Y;
                    QT3DSF32 y2 = (QT3DSF32)theDetails.m_Y + theDetails.m_TextHeight;

                    QT3DSF32 box[4][5] = {
                        { x1, y1, 0, 0, 1 },
                        { x1, y2, 0, 0, 0 },
                        { x2, y2, 0, 1, 0 },
                        { x2, y1, 0, 1, 1 },
                    };

                    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)box, bufSize);
                    mVertexBuffer->UpdateBuffer(vertData, false);

                    theTextShader.m_Sampler.Set(theTexture);

                    theContext.SetInputAssembler(mInputAssembler);
                    theContext.Draw(NVRenderDrawMode::Triangles, m_QuadIndexBuffer->GetNumIndices(),
                                    0);
                }
            }

            m_qt3dsContext.GetResourceManager().Release(*theTexture);
            m_qt3dsContext.GetResourceManager().Release(*theAtlasFB);

            return true;
        }

        return theTextureAtlas->IsInitialized();
    }

    Option<QT3DSVec2> Qt3DSRendererImpl::GetLayerMouseCoords(SLayerRenderData &inLayerRenderData,
                                                         const QT3DSVec2 &inMouseCoords,
                                                         const QT3DSVec2 &inViewportDimensions,
                                                         bool forceImageIntersect) const
    {
        if (inLayerRenderData.m_LayerPrepResult.hasValue())
            return inLayerRenderData.m_LayerPrepResult->GetLayerMouseCoords(
                inMouseCoords, inViewportDimensions, forceImageIntersect);
        return Empty();
    }

    void Qt3DSRendererImpl::GetLayerHitObjectList(SLayerRenderData &inLayerRenderData,
                                                  const QT3DSVec2 &inViewportDimensions,
                                                  const QT3DSVec2 &inPresCoords,
                                                  bool inPickEverything,
                                                  TPickResultArray &outIntersectionResult,
                                                  NVAllocatorCallback &inTempAllocator)
    {
        // This function assumes the layer was rendered to the scene itself.  There is another
        // function for completely offscreen layers that don't get rendered to the scene.
        bool wasRenderToTarget(inLayerRenderData.m_Layer.m_Flags.IsLayerRenderToTarget());
        if (!wasRenderToTarget || !inLayerRenderData.m_Camera)
            return;

        Option<SRay> theHitRay;
        if (inLayerRenderData.m_LayerPrepResult.hasValue()) {
            theHitRay = inLayerRenderData.m_LayerPrepResult->GetPickRay(
                inPresCoords, inViewportDimensions, false, m_Context->isSceneCameraView());
        }
        if (inLayerRenderData.m_LastFrameOffscreenRenderer.mPtr) {
            IGraphObjectPickQuery *theQuery =
                inLayerRenderData.m_LastFrameOffscreenRenderer->GetGraphObjectPickQuery(this);
            if (theQuery) {
                Qt3DSRenderPickResult theResult =
                    theQuery->Pick(inPresCoords, inViewportDimensions, inPickEverything);
                if (theResult.m_HitObject) {
                    theResult.m_OffscreenRenderer =
                        inLayerRenderData.m_LastFrameOffscreenRenderer;
                    outIntersectionResult.push_back(theResult);
                }
            } else {
                inLayerRenderData.m_LastFrameOffscreenRenderer->Pick(inPresCoords,
                                                                     inViewportDimensions,
                                                                     this);
            }
            return;
        }
        if (!theHitRay.hasValue())
            return;
        // Scale the mouse coords to change them into the camera's coordinate space.
        SRay thePickRay = *theHitRay;
        for (QT3DSU32 idx = inLayerRenderData.m_OpaqueObjects.size(), end = 0; idx > end; --idx) {
            SRenderableObject *theRenderableObject = inLayerRenderData.m_OpaqueObjects[idx - 1];
            if (inPickEverything || theRenderableObject->m_RenderableFlags.GetPickable()) {
                IntersectRayWithSubsetRenderable(thePickRay, *theRenderableObject,
                                                 outIntersectionResult,
                                                 inTempAllocator);
            }
        }
        for (QT3DSU32 idx = inLayerRenderData.m_GroupObjects.size(), end = 0; idx > end; --idx) {
            SRenderableObject *object = inLayerRenderData.m_GroupObjects[idx - 1];
            SOrderedGroupRenderable &group(static_cast<SOrderedGroupRenderable &>(*object));
            Q_ASSERT(object->m_RenderableFlags.isOrderedGroup());
            for (int i = 0; i < group.m_renderables.size(); ++i) {
                if (inPickEverything || group.m_renderables[i]->m_RenderableFlags.GetPickable()) {
                    IntersectRayWithSubsetRenderable(thePickRay, *group.m_renderables[i],
                                                     outIntersectionResult,
                                                     inTempAllocator);
                }
            }
        }
        for (QT3DSU32 idx = inLayerRenderData.m_TransparentObjects.size(), end = 0;
             idx > end; --idx) {
            SRenderableObject *renderableObject = inLayerRenderData.m_TransparentObjects[idx - 1];
            if (inPickEverything || renderableObject->m_RenderableFlags.GetPickable()) {
                IntersectRayWithSubsetRenderable(thePickRay, *renderableObject,
                                                 outIntersectionResult,
                                                 inTempAllocator);
            }
        }
    }

    static inline Qt3DSRenderPickSubResult ConstructSubResult(SRenderableImage &inImage)
    {
        return ConstructSubResult(inImage.m_Image);
    }

    void Qt3DSRendererImpl::IntersectRayWithSubsetRenderable(
        const SRay &inRay, SRenderableObject &inRenderableObject,
        TPickResultArray &outIntersectionResultList, NVAllocatorCallback &inTempAllocator)
    {
        Option<SRayIntersectionResult> theIntersectionResultOpt(inRay.IntersectWithAABB(
            inRenderableObject.m_GlobalTransform, inRenderableObject.m_Bounds));
        if (theIntersectionResultOpt.hasValue() == false)
            return;
        SRayIntersectionResult &theResult(*theIntersectionResultOpt);

        // Leave the coordinates relative for right now.
        const SGraphObject *thePickObject = nullptr;
        if (inRenderableObject.m_RenderableFlags.IsDefaultMaterialMeshSubset())
            thePickObject =
                &static_cast<SSubsetRenderable *>(&inRenderableObject)->m_ModelContext.m_Model;
        else if (inRenderableObject.m_RenderableFlags.IsText())
            thePickObject = &static_cast<STextRenderable *>(&inRenderableObject)->m_Text;
#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
        else if (inRenderableObject.m_RenderableFlags.isDistanceField())
            thePickObject = &static_cast<SDistanceFieldRenderable *>(&inRenderableObject)->m_text;
#endif
        else if (inRenderableObject.m_RenderableFlags.IsCustomMaterialMeshSubset())
            thePickObject = &static_cast<SCustomMaterialRenderable *>(&inRenderableObject)
                                 ->m_ModelContext.m_Model;
        else if (inRenderableObject.m_RenderableFlags.IsPath())
            thePickObject = &static_cast<SPathRenderable *>(&inRenderableObject)->m_Path;

        if (thePickObject != nullptr) {
            outIntersectionResultList.push_back(Qt3DSRenderPickResult(
                *thePickObject, theResult.m_RayLengthSquared, theResult.m_RelXY));

            // For subsets, we know we can find images on them which may have been the result
            // of rendering a sub-presentation.
            if (inRenderableObject.m_RenderableFlags.IsDefaultMaterialMeshSubset()) {
                Qt3DSRenderPickSubResult *theLastResult = nullptr;
                for (SRenderableImage *theImage =
                         static_cast<SSubsetRenderable *>(&inRenderableObject)->m_FirstImage;
                     theImage != nullptr; theImage = theImage->m_NextImage) {
                    if (theImage->m_Image.m_LastFrameOffscreenRenderer != nullptr
                        && theImage->m_Image.m_TextureData.m_Texture != nullptr) {
                        Qt3DSRenderPickSubResult *theSubResult =
                            (Qt3DSRenderPickSubResult *)inTempAllocator.allocate(
                                sizeof(Qt3DSRenderPickSubResult), "Qt3DSRenderPickSubResult",
                                __FILE__, __LINE__);

                        new (theSubResult) Qt3DSRenderPickSubResult(ConstructSubResult(*theImage));
                        if (theLastResult == nullptr)
                            outIntersectionResultList.back().m_FirstSubObject = theSubResult;
                        else
                            theLastResult->m_NextSibling = theSubResult;
                        theLastResult = theSubResult;
                    }
                }
            }
        }
    }

#ifndef EA_PLATFORM_WINDOWS
#define _snprintf snprintf
#endif

    NVRenderShaderProgram *Qt3DSRendererImpl::CompileShader(CRegisteredString inName,
                                                           const char8_t *inVert,
                                                           const char8_t *inFrag)
    {
        GetProgramGenerator().BeginProgram();
        GetProgramGenerator().GetStage(ShaderGeneratorStages::Vertex)->Append(inVert);
        GetProgramGenerator().GetStage(ShaderGeneratorStages::Fragment)->Append(inFrag);
        return GetProgramGenerator().CompileGeneratedShader(inName);
    }

    const QT3DSF32 MINATTENUATION = 0;
    const QT3DSF32 MAXATTENUATION = 1000;

    QT3DSF32 ClampFloat(QT3DSF32 value, QT3DSF32 min, QT3DSF32 max)
    {
        return value < min ? min : ((value > max) ? max : value);
    }

    QT3DSF32 TranslateConstantAttenuation(QT3DSF32 attenuation) { return attenuation * .01f; }

    QT3DSF32 TranslateLinearAttenuation(QT3DSF32 attenuation)
    {
        attenuation = ClampFloat(attenuation, MINATTENUATION, MAXATTENUATION);
        return attenuation * 0.0001f;
    }

    QT3DSF32 TranslateQuadraticAttenuation(QT3DSF32 attenuation)
    {
        attenuation = ClampFloat(attenuation, MINATTENUATION, MAXATTENUATION);
        return attenuation * 0.0000001f;
    }

    SShaderGeneratorGeneratedShader *Qt3DSRendererImpl::GetShader(SSubsetRenderable &inRenderable,
                                                                  TShaderFeatureSet inFeatureSet,
                                                                  bool depth)
    {
        if (m_CurrentLayer == nullptr) {
            QT3DS_ASSERT(false);
            return nullptr;
        }

        SShaderGeneratorGeneratedShader *retval = nullptr;
        TShaderMap &map = depth ? m_DepthShaders : m_Shaders;
        TShaderMap::iterator theFind = map.find(inRenderable.m_ShaderDescription);
        if (theFind == map.end()) {
            // Generate the shader.
            NVRenderShaderProgram *theShader(GenerateShader(inRenderable, inFeatureSet, depth));
            if (theShader) {
                SShaderGeneratorGeneratedShader *theGeneratedShader =
                    (SShaderGeneratorGeneratedShader *)m_Context->GetAllocator().allocate(
                        sizeof(SShaderGeneratorGeneratedShader), "SShaderGeneratorGeneratedShader",
                        __FILE__, __LINE__);
                new (theGeneratedShader) SShaderGeneratorGeneratedShader(
                    m_StringTable->RegisterStr(m_GeneratedShaderString.c_str()), *theShader);
                map.insert(make_pair(inRenderable.m_ShaderDescription, theGeneratedShader));
                retval = theGeneratedShader;
            }
            // We still insert something because we don't want to attempt to generate the same
            // bad shader twice.
            else
                map.insert(make_pair(inRenderable.m_ShaderDescription,
                                           (SShaderGeneratorGeneratedShader *)nullptr));
        } else
            retval = theFind->second;

        if (retval != nullptr && !depth) {
            if (!m_LayerShaders.contains(*retval)) {
                m_LayerShaders.insert(*retval);
            }
            if (m_CurrentLayer && m_CurrentLayer->m_Camera) {
                SCamera &theCamera(*m_CurrentLayer->m_Camera);
                if (m_CurrentLayer->m_CameraDirection.hasValue() == false)
                    m_CurrentLayer->m_CameraDirection = theCamera.GetScalingCorrectDirection();
            }
        }
        return retval;
    }

    SRenderableDepthPrepassShader *Qt3DSRendererImpl::GetShadowShader(
            SSubsetRenderable &inRenderable, TShaderFeatureSet inFeatureSet,
            RenderLightTypes::Enum lightType)
    {
        if (m_CurrentLayer == nullptr) {
            QT3DS_ASSERT(false);
            return nullptr;
        }

        SRenderableDepthPrepassShader *retval = nullptr;
        TShadowShaderMap &map = lightType == RenderLightTypes::Point ? m_ShadowCubeShaders
                                                               : m_ShadowMapShaders;
        TShadowShaderMap::iterator theFind = map.find(inRenderable.m_ShaderDescription);
        if (theFind == map.end()) {
            // Generate the shader.
            NVRenderShaderProgram *theShader(GenerateShadowShader(inRenderable, inFeatureSet,
                                                                  lightType));
            if (theShader) {
                SRenderableDepthPrepassShader *theGeneratedShader =
                    (SRenderableDepthPrepassShader *)m_Context->GetAllocator().allocate(
                        sizeof(SRenderableDepthPrepassShader), "SRenderableDepthPrepassShader",
                        __FILE__, __LINE__);
                new (theGeneratedShader) SRenderableDepthPrepassShader(*theShader, GetContext());
                map.insert(make_pair(inRenderable.m_ShaderDescription, theGeneratedShader));
                retval = theGeneratedShader;
            }
            // We still insert something because we don't want to attempt to generate the same
            // bad shader twice.
            else {
                map.insert(make_pair(inRenderable.m_ShaderDescription, nullptr));
            }
        } else {
            retval = theFind->second;
        }

        return retval;
    }

    static QT3DSVec3 g_fullScreenRectFace[] = {
        QT3DSVec3(-1, -1, 0), QT3DSVec3(-1, 1, 0), QT3DSVec3(1, 1, 0), QT3DSVec3(1, -1, 0),
    };

    static QT3DSVec2 g_fullScreenRectUVs[] = { QT3DSVec2(0, 0), QT3DSVec2(0, 1), QT3DSVec2(1, 1),
                                            QT3DSVec2(1, 0) };

    void Qt3DSRendererImpl::GenerateXYQuad()
    {
        if (m_QuadInputAssembler)
            return;

        qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
            qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
            qt3ds::render::NVRenderVertexBufferEntry("attr_uv",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2, 12),
        };

        QT3DSF32 tempBuf[20];
        QT3DSF32 *bufPtr = tempBuf;
        QT3DSVec3 *facePtr(g_fullScreenRectFace);
        QT3DSVec2 *uvPtr(g_fullScreenRectUVs);
        for (int j = 0; j < 4; j++, ++facePtr, ++uvPtr, bufPtr += 5) {
            bufPtr[0] = facePtr->x;
            bufPtr[1] = facePtr->y;
            bufPtr[2] = facePtr->z;
            bufPtr[3] = uvPtr->x;
            bufPtr[4] = uvPtr->y;
        }
        m_QuadVertexBuffer = m_Context->CreateVertexBuffer(
            qt3ds::render::NVRenderBufferUsageType::Static, 20 * sizeof(QT3DSF32),
            3 * sizeof(QT3DSF32) + 2 * sizeof(QT3DSF32), toU8DataRef(tempBuf, 20));

        QT3DSU8 indexData[] = {
            0, 1, 2, 0, 2, 3,
        };
        m_QuadIndexBuffer = m_Context->CreateIndexBuffer(
            qt3ds::render::NVRenderBufferUsageType::Static, qt3ds::render::NVRenderComponentTypes::QT3DSU8,
            sizeof(indexData), toU8DataRef(indexData, sizeof(indexData)));

        // create our attribute layout
        m_QuadAttribLayout = m_Context->CreateAttributeLayout(toConstDataRef(theEntries, 2));

        // create input assembler object
        QT3DSU32 strides = m_QuadVertexBuffer->GetStride();
        QT3DSU32 offsets = 0;
        m_QuadInputAssembler = m_Context->CreateInputAssembler(
            m_QuadAttribLayout, toConstDataRef(&m_QuadVertexBuffer.mPtr, 1), m_QuadIndexBuffer,
            toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    }

    void Qt3DSRendererImpl::GenerateXYZPoint()
    {
        if (m_PointInputAssembler)
            return;

        qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
            qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
            qt3ds::render::NVRenderVertexBufferEntry("attr_uv",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2, 12),
        };

        QT3DSF32 tempBuf[5];
        tempBuf[0] = tempBuf[1] = tempBuf[2] = 0.0;
        tempBuf[3] = tempBuf[4] = 0.0;

        m_PointVertexBuffer = m_Context->CreateVertexBuffer(
            qt3ds::render::NVRenderBufferUsageType::Static, 5 * sizeof(QT3DSF32),
            3 * sizeof(QT3DSF32) + 2 * sizeof(QT3DSF32), toU8DataRef(tempBuf, 5));

        // create our attribute layout
        m_PointAttribLayout = m_Context->CreateAttributeLayout(toConstDataRef(theEntries, 2));

        // create input assembler object
        QT3DSU32 strides = m_PointVertexBuffer->GetStride();
        QT3DSU32 offsets = 0;
        m_PointInputAssembler = m_Context->CreateInputAssembler(
            m_PointAttribLayout, toConstDataRef(&m_PointVertexBuffer.mPtr, 1), nullptr,
            toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    }

    eastl::pair<NVRenderVertexBuffer *, NVRenderIndexBuffer *> Qt3DSRendererImpl::GetXYQuad()
    {
        if (!m_QuadInputAssembler)
            GenerateXYQuad();

        return eastl::make_pair(m_QuadVertexBuffer.mPtr, m_QuadIndexBuffer.mPtr);
    }

    SLayerGlobalRenderProperties Qt3DSRendererImpl::GetLayerGlobalRenderProperties()
    {
        SLayerRenderData &theData = *m_CurrentLayer;
        SLayer &theLayer = theData.m_Layer;
        if (theData.m_CameraDirection.hasValue() == false)
            theData.m_CameraDirection = theData.m_Camera->GetScalingCorrectDirection();

        return SLayerGlobalRenderProperties(
            theLayer, *theData.m_Camera, *theData.m_CameraDirection, theData.m_Lights,
            theData.m_LightDirections, theData.m_ShadowMapManager.mPtr, theData.m_LayerDepthTexture,
            theData.m_LayerSsaoTexture, theLayer.m_LightProbe, theLayer.m_LightProbe2,
            theLayer.m_ProbeHorizon, theLayer.m_ProbeBright, theLayer.m_Probe2Window,
            theLayer.m_Probe2Pos, theLayer.m_Probe2Fade, theLayer.m_ProbeFov);
    }

    void Qt3DSRendererImpl::GenerateXYQuadStrip()
    {
        if (m_QuadStripInputAssembler)
            return;

        qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
            qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
            qt3ds::render::NVRenderVertexBufferEntry("attr_uv",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2, 12),
        };

        // this buffer is filled dynmically
        m_QuadStripVertexBuffer =
            m_Context->CreateVertexBuffer(qt3ds::render::NVRenderBufferUsageType::Dynamic, 0,
                                          3 * sizeof(QT3DSF32) + 2 * sizeof(QT3DSF32) // stride
                                          ,
                                          NVDataRef<QT3DSU8>());

        // create our attribute layout
        m_QuadStripAttribLayout = m_Context->CreateAttributeLayout(toConstDataRef(theEntries, 2));

        // create input assembler object
        QT3DSU32 strides = m_QuadStripVertexBuffer->GetStride();
        QT3DSU32 offsets = 0;
        m_QuadStripInputAssembler = m_Context->CreateInputAssembler(
            m_QuadStripAttribLayout, toConstDataRef(&m_QuadStripVertexBuffer.mPtr, 1), nullptr,
            toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    }

    void Qt3DSRendererImpl::UpdateCbAoShadow(const SLayer *pLayer, const SCamera *pCamera,
                                            CResourceTexture2D &inDepthTexture)
    {
        if (m_Context->GetConstantBufferSupport()) {
            CRegisteredString theName = m_Context->GetStringTable().RegisterStr("cbAoShadow");
            NVRenderConstantBuffer *pCB = m_Context->GetConstantBuffer(theName);

            if (!pCB) {
                // the  size is determined automatically later on
                pCB = m_Context->CreateConstantBuffer(
                    theName, qt3ds::render::NVRenderBufferUsageType::Static, 0, NVDataRef<QT3DSU8>());
                if (!pCB) {
                    QT3DS_ASSERT(false);
                    return;
                }
                m_ConstantBuffers.insert(eastl::make_pair(theName, pCB));

                // Add paramters. Note should match the appearance in the shader program
                pCB->AddParam(m_Context->GetStringTable().RegisterStr("ao_properties"),
                              qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4, 1);
                pCB->AddParam(m_Context->GetStringTable().RegisterStr("ao_properties2"),
                              qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4, 1);
                pCB->AddParam(m_Context->GetStringTable().RegisterStr("shadow_properties"),
                              qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4, 1);
                pCB->AddParam(m_Context->GetStringTable().RegisterStr("aoScreenConst"),
                              qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4, 1);
                pCB->AddParam(m_Context->GetStringTable().RegisterStr("UvToEyeConst"),
                              qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4, 1);
            }

            // update values
            QT3DSVec4 aoProps(pLayer->m_AoStrength * 0.01f, pLayer->m_AoDistance * 0.4f,
                           pLayer->m_AoSoftness * 0.02f, pLayer->m_AoBias);
            pCB->UpdateParam("ao_properties", NVDataRef<QT3DSU8>((QT3DSU8 *)&aoProps, 1));
            QT3DSVec4 aoProps2((QT3DSF32)pLayer->m_AoSamplerate, (pLayer->m_AoDither) ? 1.0f : 0.0f, 0.0f,
                            0.0f);
            pCB->UpdateParam("ao_properties2", NVDataRef<QT3DSU8>((QT3DSU8 *)&aoProps2, 1));
            QT3DSVec4 shadowProps(pLayer->m_ShadowStrength * 0.01f, pLayer->m_ShadowDist,
                               pLayer->m_ShadowSoftness * 0.01f, pLayer->m_ShadowBias);
            pCB->UpdateParam("shadow_properties", NVDataRef<QT3DSU8>((QT3DSU8 *)&shadowProps, 1));

            QT3DSF32 R2 = pLayer->m_AoDistance * pLayer->m_AoDistance * 0.16f;
            QT3DSF32 rw = 100, rh = 100;

            if (inDepthTexture && inDepthTexture.GetTexture()) {
                rw = (QT3DSF32)inDepthTexture.GetTexture()->GetTextureDetails().m_Width;
                rh = (QT3DSF32)inDepthTexture.GetTexture()->GetTextureDetails().m_Height;
            }
            QT3DSF32 fov = (pCamera) ? pCamera->verticalFov(rw / rh) : 1.0f;
            QT3DSF32 tanHalfFovY = tanf(0.5f * fov * (rh / rw));
            QT3DSF32 invFocalLenX = tanHalfFovY * (rw / rh);

            QT3DSVec4 aoScreenConst(1.0f / R2, rh / (2.0f * tanHalfFovY), 1.0f / rw, 1.0f / rh);
            pCB->UpdateParam("aoScreenConst", NVDataRef<QT3DSU8>((QT3DSU8 *)&aoScreenConst, 1));
            QT3DSVec4 UvToEyeConst(2.0f * invFocalLenX, -2.0f * tanHalfFovY, -invFocalLenX,
                                tanHalfFovY);
            pCB->UpdateParam("UvToEyeConst", NVDataRef<QT3DSU8>((QT3DSU8 *)&UvToEyeConst, 1));

            // update buffer to hardware
            pCB->Update();
        }
    }

    // widget context implementation

    NVRenderVertexBuffer &Qt3DSRendererImpl::GetOrCreateVertexBuffer(CRegisteredString &inStr,
                                                                    QT3DSU32 stride,
                                                                    NVConstDataRef<QT3DSU8> bufferData)
    {
        NVRenderVertexBuffer *retval = GetVertexBuffer(inStr);
        if (retval) {
            // we update the buffer
            retval->UpdateBuffer(bufferData, false);
            return *retval;
        }
        retval = m_Context->CreateVertexBuffer(qt3ds::render::NVRenderBufferUsageType::Dynamic,
                                               bufferData.size(), stride, bufferData);
        m_WidgetVertexBuffers.insert(eastl::make_pair(inStr, retval));
        return *retval;
    }
    NVRenderIndexBuffer &
    Qt3DSRendererImpl::GetOrCreateIndexBuffer(CRegisteredString &inStr,
                                             qt3ds::render::NVRenderComponentTypes::Enum componentType,
                                             size_t size, NVConstDataRef<QT3DSU8> bufferData)
    {
        NVRenderIndexBuffer *retval = GetIndexBuffer(inStr);
        if (retval) {
            // we update the buffer
            retval->UpdateBuffer(bufferData, false);
            return *retval;
        }

        retval = m_Context->CreateIndexBuffer(qt3ds::render::NVRenderBufferUsageType::Dynamic,
                                              componentType, size, bufferData);
        m_WidgetIndexBuffers.insert(eastl::make_pair(inStr, retval));
        return *retval;
    }

    NVRenderAttribLayout &Qt3DSRendererImpl::CreateAttributeLayout(
        NVConstDataRef<qt3ds::render::NVRenderVertexBufferEntry> attribs)
    {
        // create our attribute layout
        NVRenderAttribLayout *theAttribLAyout = m_Context->CreateAttributeLayout(attribs);
        return *theAttribLAyout;
    }

    NVRenderInputAssembler &Qt3DSRendererImpl::GetOrCreateInputAssembler(
        CRegisteredString &inStr, NVRenderAttribLayout *attribLayout,
        NVConstDataRef<NVRenderVertexBuffer *> buffers, const NVRenderIndexBuffer *indexBuffer,
        NVConstDataRef<QT3DSU32> strides, NVConstDataRef<QT3DSU32> offsets)
    {
        NVRenderInputAssembler *retval = GetInputAssembler(inStr);
        if (retval)
            return *retval;

        retval =
            m_Context->CreateInputAssembler(attribLayout, buffers, indexBuffer, strides, offsets);
        m_WidgetInputAssembler.insert(eastl::make_pair(inStr, retval));
        return *retval;
    }

    NVRenderVertexBuffer *Qt3DSRendererImpl::GetVertexBuffer(CRegisteredString &inStr)
    {
        TStrVertBufMap::iterator theIter = m_WidgetVertexBuffers.find(inStr);
        if (theIter != m_WidgetVertexBuffers.end())
            return theIter->second;
        return nullptr;
    }

    NVRenderIndexBuffer *Qt3DSRendererImpl::GetIndexBuffer(CRegisteredString &inStr)
    {
        TStrIndexBufMap::iterator theIter = m_WidgetIndexBuffers.find(inStr);
        if (theIter != m_WidgetIndexBuffers.end())
            return theIter->second;
        return nullptr;
    }

    NVRenderInputAssembler *Qt3DSRendererImpl::GetInputAssembler(CRegisteredString &inStr)
    {
        TStrIAMap::iterator theIter = m_WidgetInputAssembler.find(inStr);
        if (theIter != m_WidgetInputAssembler.end())
            return theIter->second;
        return nullptr;
    }

    NVRenderShaderProgram *Qt3DSRendererImpl::GetShader(CRegisteredString inStr)
    {
        TStrShaderMap::iterator theIter = m_WidgetShaders.find(inStr);
        if (theIter != m_WidgetShaders.end())
            return theIter->second;
        return nullptr;
    }

    NVRenderShaderProgram *Qt3DSRendererImpl::CompileAndStoreShader(CRegisteredString inStr)
    {
        NVRenderShaderProgram *newProgram = GetProgramGenerator().CompileGeneratedShader(inStr);
        if (newProgram)
            m_WidgetShaders.insert(eastl::make_pair(inStr, newProgram));
        return newProgram;
    }

    IShaderProgramGenerator &Qt3DSRendererImpl::GetProgramGenerator()
    {
        return m_qt3dsContext.GetShaderProgramGenerator();
    }

    STextDimensions Qt3DSRendererImpl::MeasureText(const STextRenderInfo &inText)
    {
        if (m_qt3dsContext.GetTextRenderer() != nullptr)
            return m_qt3dsContext.GetTextRenderer()->MeasureText(inText, 0);
        return STextDimensions();
    }

    void Qt3DSRendererImpl::RenderText(const STextRenderInfo &inText, const QT3DSVec3 &inTextColor,
                                      const QT3DSVec3 &inBackgroundColor, const QT3DSMat44 &inMVP)
    {
        if (m_qt3dsContext.GetTextRenderer() != nullptr) {
            ITextRenderer &theTextRenderer(*m_qt3dsContext.GetTextRenderer());
            NVRenderTexture2D *theTexture = m_qt3dsContext.GetResourceManager().AllocateTexture2D(
                32, 32, NVRenderTextureFormats::RGBA8);
            STextTextureDetails theTextTextureDetails =
                theTextRenderer.RenderText(inText, *theTexture);
            STextRenderHelper theTextHelper(GetTextWidgetShader());
            if (theTextHelper.m_Shader != nullptr) {
                m_qt3dsContext.GetRenderContext().SetBlendingEnabled(false);
                STextScaleAndOffset theScaleAndOffset(*theTexture, theTextTextureDetails, inText);
                theTextHelper.m_Shader->Render(*theTexture, theScaleAndOffset,
                                               QT3DSVec4(inTextColor, 1.0f), inMVP, QT3DSVec2(0, 0),
                                               GetContext(), theTextHelper.m_QuadInputAssembler,
                                               theTextHelper.m_QuadInputAssembler.GetIndexCount(),
                                               theTextTextureDetails, inBackgroundColor);
            }
            m_qt3dsContext.GetResourceManager().Release(*theTexture);
        }
    }

    void Qt3DSRendererImpl::RenderText2D(QT3DSF32 x, QT3DSF32 y,
                                        qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor,
                                        const char *text)
    {
        if (m_qt3dsContext.GetOnscreenTextRenderer() != nullptr) {
            GenerateXYQuadStrip();

            if (PrepareTextureAtlasForRender()) {
                TTextRenderAtlasDetailsAndTexture theRenderTextDetails;
                ITextTextureAtlas *theTextureAtlas = m_qt3dsContext.GetTextureAtlas();
                QSize theWindow = m_qt3dsContext.GetWindowDimensions();

                // Use dynamic strings to avoid memory leakage
                QByteArray theText(text);
                CStringHandle textHandle = m_StringTable->getDynamicHandle(theText);
                STextRenderInfo theInfo;
                theInfo.m_Text = m_StringTable->HandleToStr(textHandle);
                theInfo.m_FontSize = 20;
                // text scale 2% of screen we don't scale Y though because it becomes unreadable
                theInfo.m_ScaleX = (theWindow.width() / 100.0f) * 1.5f / (theInfo.m_FontSize);
                theInfo.m_ScaleY = 1.0f;

                theRenderTextDetails = theTextureAtlas->RenderText(theInfo);

                if (theRenderTextDetails.first.m_Vertices.size()) {
                    STextRenderHelper theTextHelper(GetOnscreenTextShader());
                    if (theTextHelper.m_Shader != nullptr) {
                        // setup 2D projection
                        SCamera theCamera;
                        theCamera.m_ClipNear = -1.0;
                        theCamera.m_ClipFar = 1.0;

                        theCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
                        theCamera.m_Flags.SetOrthographic(true);
                        QT3DSVec2 theWindowDim((QT3DSF32)theWindow.width(),
                                               (QT3DSF32)theWindow.height());
                        theCamera.CalculateGlobalVariables(
                            NVRenderRect(0, 0, theWindow.width(), theWindow.height()),
                            theWindowDim);
                        // We want a 2D lower left projection
                        QT3DSF32 *writePtr(theCamera.m_Projection.front());
                        writePtr[12] = -1;
                        writePtr[13] = -1;

                        // upload vertices
                        m_QuadStripVertexBuffer->UpdateBuffer(theRenderTextDetails.first.m_Vertices,
                                                              false);

                        theTextHelper.m_Shader->Render2D(
                            *theRenderTextDetails.second, QT3DSVec4(inColor, 1.0f),
                            theCamera.m_Projection, GetContext(),
                            theTextHelper.m_QuadInputAssembler,
                            theRenderTextDetails.first.m_VertexCount, QT3DSVec2(x, y));
                    }
                    // we release the memory here
                    QT3DS_FREE(m_Context->GetAllocator(),
                            theRenderTextDetails.first.m_Vertices.begin());
                }
                m_StringTable->releaseDynamicHandle(textHandle);
            }
        }
    }

    void Qt3DSRendererImpl::RenderGpuProfilerStats(QT3DSF32 x, QT3DSF32 y,
                                                  qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor)
    {
        if (!IsLayerGpuProfilingEnabled())
            return;

        char messageLine[1024];
        TInstanceRenderMap::const_iterator theIter;

        QT3DSF32 startY = y;

        for (theIter = m_InstanceRenderMap.begin(); theIter != m_InstanceRenderMap.end(); theIter++) {
            QT3DSF32 startX = x;
            const SLayerRenderData *theLayerRenderData = theIter->second;
            const SLayer *theLayer = &theLayerRenderData->m_Layer;

            if (theLayer->m_Flags.IsActive() && theLayerRenderData->m_LayerProfilerGpu.mPtr) {
                const IRenderProfiler::TStrIDVec &idList =
                    theLayerRenderData->m_LayerProfilerGpu->GetTimerIDs();
                if (!idList.empty()) {
                    startY -= 22;
                    startX += 20;
                    RenderText2D(startX, startY, inColor, theLayer->m_Id);
                    IRenderProfiler::TStrIDVec::const_iterator theIdIter = idList.begin();
                    for (theIdIter = idList.begin(); theIdIter != idList.end(); theIdIter++) {
                        startY -= 22;
                        sprintf(messageLine, "%s: %.3f ms", theIdIter->c_str(),
                                theLayerRenderData->m_LayerProfilerGpu->GetElapsedTime(*theIdIter));
                        RenderText2D(startX + 20, startY, inColor, messageLine);
                    }
                }
            }
        }
    }

    // Given a node and a point in the node's local space (most likely its pivot point), we return
    // a normal matrix so you can get the axis out, a transformation from node to camera
    // a new position and a floating point scale factor so you can render in 1/2 perspective mode
    // or orthographic mode if you would like to.
    SWidgetRenderInformation
    Qt3DSRendererImpl::GetWidgetRenderInformation(SNode &inNode, const QT3DSVec3 &inPos,
                                                 RenderWidgetModes::Enum inWidgetMode)
    {
        SLayerRenderData *theData = GetOrCreateLayerRenderDataForNode(inNode);
        SCamera *theCamera = theData->m_Camera;
        if (theCamera == nullptr || theData->m_LayerPrepResult.hasValue() == false) {
            QT3DS_ASSERT(false);
            return SWidgetRenderInformation();
        }
        QT3DSMat44 theGlobalTransform(QT3DSMat44::createIdentity());
        if (inNode.m_Parent != nullptr && inNode.m_Parent->m_Type != GraphObjectTypes::Layer
            && !inNode.m_Flags.IsIgnoreParentTransform())
            theGlobalTransform = inNode.m_Parent->m_GlobalTransform;
        QT3DSMat44 theCameraInverse(theCamera->m_GlobalTransform.getInverse());
        QT3DSMat44 theNodeParentToCamera;
        if (inWidgetMode == RenderWidgetModes::Local)
            theNodeParentToCamera = theCameraInverse * theGlobalTransform;
        else
            theNodeParentToCamera = theCameraInverse;

        QT3DSMat33 theNormalMat(theNodeParentToCamera.column0.getXYZ(),
                             theNodeParentToCamera.column1.getXYZ(),
                             theNodeParentToCamera.column2.getXYZ());
        theNormalMat = theNormalMat.getInverse().getTranspose();
        theNormalMat.column0.normalize();
        theNormalMat.column1.normalize();
        theNormalMat.column2.normalize();

        QT3DSMat44 theTranslation(QT3DSMat44::createIdentity());
        theTranslation.column3.x = inNode.m_Position.x;
        theTranslation.column3.y = inNode.m_Position.y;
        theTranslation.column3.z = inNode.m_Position.z;
        theTranslation.column3.z *= -1.0f;

        theGlobalTransform = theGlobalTransform * theTranslation;

        QT3DSMat44 theNodeToParentPlusTranslation = theCameraInverse * theGlobalTransform;
        QT3DSVec3 thePos = theNodeToParentPlusTranslation.transform(inPos);
        SScaleAndPosition theScaleAndPos = GetWorldToPixelScaleFactor(*theCamera, thePos, *theData);
        QT3DSMat33 theLookAtMatrix(QT3DSMat33::createIdentity());
        if (theCamera->m_Flags.IsOrthographic() == false) {
            QT3DSVec3 theNodeToCamera = theScaleAndPos.m_Position;
            theNodeToCamera.normalize();
            QT3DSVec3 theOriginalAxis = QT3DSVec3(0, 0, -1);
            QT3DSVec3 theRotAxis = theOriginalAxis.cross(theNodeToCamera);
            QT3DSF32 theAxisLen = theRotAxis.normalize();
            if (theAxisLen > .05f) {
                QT3DSF32 theRotAmount = acos(theOriginalAxis.dot(theNodeToCamera));
                QT3DSQuat theQuat(theRotAmount, theRotAxis);
                theLookAtMatrix = QT3DSMat33(theQuat);
            }
        }
        QT3DSVec3 thePosInWorldSpace = theGlobalTransform.transform(inPos);
        QT3DSVec3 theCameraPosInWorldSpace = theCamera->GetGlobalPos();
        QT3DSVec3 theCameraOffset = thePosInWorldSpace - theCameraPosInWorldSpace;
        QT3DSVec3 theDir = theCameraOffset;
        theDir.normalize();
        // Things should be 600 units from the camera, as that is how all of our math is setup.
        theCameraOffset = 600.0f * theDir;
        return SWidgetRenderInformation(
            theNormalMat, theNodeParentToCamera, theCamera->m_Projection, theCamera->m_Projection,
            theLookAtMatrix, theCameraInverse, theCameraOffset, theScaleAndPos.m_Position,
            theScaleAndPos.m_Scale, *theCamera);
    }

    Option<QT3DSVec2> Qt3DSRendererImpl::GetLayerMouseCoords(SLayer &inLayer,
                                                         const QT3DSVec2 &inMouseCoords,
                                                         const QT3DSVec2 &inViewportDimensions,
                                                         bool forceImageIntersect) const
    {
        SLayerRenderData *theData =
            const_cast<Qt3DSRendererImpl &>(*this).GetOrCreateLayerRenderDataForNode(inLayer);
        return GetLayerMouseCoords(*theData, inMouseCoords, inViewportDimensions,
                                   forceImageIntersect);
    }

    bool IQt3DSRenderer::IsGlEsContext(qt3ds::render::NVRenderContextType inContextType)
    {
        qt3ds::render::NVRenderContextType esContextTypes(NVRenderContextValues::GLES2
                                                       | NVRenderContextValues::GLES3
                                                       | NVRenderContextValues::GLES3PLUS);

        if ((inContextType & esContextTypes))
            return true;

        return false;
    }

    bool IQt3DSRenderer::IsGlEs3Context(qt3ds::render::NVRenderContextType inContextType)
    {
        if (inContextType == NVRenderContextValues::GLES3
            || inContextType == NVRenderContextValues::GLES3PLUS)
            return true;

        return false;
    }

    bool IQt3DSRenderer::IsGl2Context(qt3ds::render::NVRenderContextType inContextType)
    {
        if (inContextType == NVRenderContextValues::GL2)
            return true;

        return false;
    }

    IQt3DSRenderer &IQt3DSRenderer::CreateRenderer(IQt3DSRenderContext &inContext)
    {
        return *QT3DS_NEW(inContext.GetAllocator(), Qt3DSRendererImpl)(inContext);
    }
}
}
