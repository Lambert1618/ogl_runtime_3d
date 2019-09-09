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
#ifndef QT3DS_RENDERER_IMPL_LAYER_RENDER_DATA_H
#define QT3DS_RENDERER_IMPL_LAYER_RENDER_DATA_H
#include "Qt3DSRender.h"
#include "Qt3DSRendererImplLayerRenderPreparationData.h"
#include "Qt3DSRenderResourceBufferObjects.h"

namespace qt3ds {
namespace render {

struct AdvancedBlendModes
{
    enum Enum {
        None = 0,
        Overlay,
        ColorBurn,
        ColorDodge
    };
};
    struct QT3DS_AUTOTEST_EXPORT SLayerRenderData : public SLayerRenderPreparationData
    {

        // Layers can be rendered offscreen for many reasons; effects, progressive aa,
        // or just because a flag forces it.  If they are rendered offscreen we can then
        // cache the result so we don't render the layer again if it isn't dirty.
        CResourceTexture2D m_LayerTexture;
        // Multiple temporal AA textures for stereoscopic needs.
        // First used for mono/left view, second for right view.
        CResourceTexture2D m_TemporalAATexture[2];
        // Sometimes we need to render our depth buffer to a depth texture.
        CResourceTexture2D m_LayerDepthTexture;
        CResourceTexture2D m_LayerPrepassDepthTexture;
        CResourceTexture2D m_LayerWidgetTexture;
        CResourceTexture2D m_LayerSsaoTexture;
        // if we render multisampled we need resolve buffers
        CResourceTexture2D m_LayerMultisampleTexture;
        CResourceTexture2D m_LayerMultisamplePrepassDepthTexture;
        CResourceTexture2D m_LayerMultisampleWidgetTexture;
        // the texture contains the render result inclusive post effects
        NVRenderTexture2D *m_LayerCachedTexture;

        NVRenderTexture2D *m_AdvancedBlendDrawTexture;
        NVRenderTexture2D *m_AdvancedBlendBlendTexture;
        qt3ds::render::NVRenderFrameBuffer *m_AdvancedModeDrawFB;
        qt3ds::render::NVRenderFrameBuffer *m_AdvancedModeBlendFB;

        // True if this layer was rendered offscreen.
        // If this object has no value then this layer wasn't rendered at all.
        SOffscreenRendererEnvironment m_LastOffscreenRenderEnvironment;

        // GPU profiler per layer
        NVScopedRefCounted<IRenderProfiler> m_LayerProfilerGpu;

        SCamera m_SceneCamera;
        QT3DSVec2 m_SceneDimensions;

        // ProgressiveAA algorithm details.
        QT3DSU32 m_ProgressiveAAPassIndex;
        // Increments every frame regardless to provide appropriate jittering
        QT3DSU32 m_TemporalAAPassIndex;
        // Ensures we don't stop on an in-between frame; we will run two frames after the dirty flag
        // is clear.
        QT3DSU32 m_NonDirtyTemporalAAPassIndex;
        QT3DSF32 m_TextScale;

        volatile QT3DSI32 mRefCount;
        Option<QT3DSVec3> m_BoundingRectColor;
        NVRenderTextureFormats::Enum m_DepthBufferFormat;

        QSize m_previousDimensions;

        SLayerRenderData(SLayer &inLayer, Qt3DSRendererImpl &inRenderer);

        virtual ~SLayerRenderData();

        void PrepareForRender();

        // Internal Call
        void PrepareForRender(const QSize &inViewportDimensions) override;

        NVRenderTextureFormats::Enum GetDepthBufferFormat();
        NVRenderFrameBufferAttachments::Enum
        GetFramebufferDepthAttachmentFormat(NVRenderTextureFormats::Enum depthFormat);

        // Render this layer assuming viewport and RT are setup.  Just renders exactly this item
        // no effects.
        void RenderDepthPass(bool inEnableTransparentDepthWrite = false);
        void RenderAoPass();
        void RenderFakeDepthMapPass(NVRenderTexture2D *theDepthTex,
                                    NVRenderTextureCube *theDepthCube);
        void RenderShadowMapPass(CResourceFrameBuffer *theFB);
        void RenderShadowCubeBlurPass(CResourceFrameBuffer *theFB, NVRenderTextureCube *target0,
                                      NVRenderTextureCube *target1, QT3DSF32 filterSz, QT3DSF32 clipFar);
        void RenderShadowMapBlurPass(CResourceFrameBuffer *theFB, NVRenderTexture2D *target0,
                                     NVRenderTexture2D *target1, QT3DSF32 filterSz, QT3DSF32 clipFar);

        void Render(CResourceFrameBuffer *theFB = NULL);
        void ResetForFrame() override;

        void CreateGpuProfiler();
        void StartProfiling(CRegisteredString &nameID, bool sync);
        void EndProfiling(CRegisteredString &nameID);
        void StartProfiling(const char *nameID, bool sync);
        void EndProfiling(const char *nameID);
        void AddVertexCount(QT3DSU32 count);

        void RenderToViewport();
        // Render this layer's data to a texture.  Required if we have any effects,
        // prog AA, or if forced.
        void RenderToTexture();

        void ApplyLayerPostEffects();

        void RunnableRenderToViewport(qt3ds::render::NVRenderFrameBuffer *theFB);

        void AddLayerRenderStep();

        void RenderRenderWidgets();

#ifdef ADVANCED_BLEND_SW_FALLBACK
        void BlendAdvancedEquationSwFallback(NVRenderTexture2D *drawTexture,
                                             NVRenderTexture2D *m_LayerTexture,
                                             AdvancedBlendModes::Enum blendMode);
#endif
        // test method to render this layer to a given view projection without running the entire
        // layer setup system.  This assumes the client has setup the viewport, scissor, and render
        // target
        // the way they want them.
        void PrepareAndRender(const QT3DSMat44 &inViewProjection);

        SOffscreenRendererEnvironment CreateOffscreenRenderEnvironment() override;
        IRenderTask &CreateRenderToTextureRunnable() override;

        void addRef();
        void release();

    protected:
        // Used for both the normal passes and the depth pass.
        // When doing the depth pass, we disable blending completely because it does not really make
        // sense
        // to write blend equations into
        void RunRenderPass(TRenderRenderableFunction renderFn, bool inEnableBlending,
                           bool inEnableDepthWrite, bool inEnableTransparentDepthWrite,
                           QT3DSU32 indexLight, const SCamera &inCamera,
                           CResourceFrameBuffer *theFB = NULL);
#ifdef ADVANCED_BLEND_SW_FALLBACK
        //Functions for advanced blending mode fallback
        void SetupDrawFB(bool depthEnabled);
        void BlendAdvancedToFB(DefaultMaterialBlendMode::Enum blendMode, bool depthEnabled,
                               CResourceFrameBuffer *theFB);
#endif
    };
}
}
#endif
