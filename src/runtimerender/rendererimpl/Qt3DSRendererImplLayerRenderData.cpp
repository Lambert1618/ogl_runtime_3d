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
#include "Qt3DSRenderLayer.h"
#include "Qt3DSRenderEffect.h"
#include "EASTL/sort.h"
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderScene.h"
#include "Qt3DSRenderPresentation.h"
#include "foundation/Qt3DSFoundation.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderResourceManager.h"
#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderEffectSystem.h"
#include "Qt3DSRenderContextCore.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "render/Qt3DSRenderRenderBuffer.h"
#include "Qt3DSOffscreenRenderKey.h"
#include "Qt3DSRenderPlugin.h"
#include "Qt3DSRenderPluginGraphObject.h"
#include "Qt3DSRenderResourceBufferObjects.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "Qt3DSRenderMaterialHelpers.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderTextTextureCache.h"
#include "Qt3DSRenderTextTextureAtlas.h"
#include "Qt3DSRenderRenderList.h"
#include "Qt3DSRendererUtil.h"
#include "Qt3DSRenderSubPresentationHelper.h"

#ifdef WIN32
#pragma warning(disable : 4355)
#endif

#define QT3DS_CACHED_POST_EFFECT
const float QT3DS_DEGREES_TO_RADIANS = 0.0174532925199f;

namespace qt3ds {
namespace render {
    using eastl::reverse;
    using eastl::stable_sort;
    using qt3ds::render::NVRenderContextScopedProperty;
    using qt3ds::QT3DSVec2;

    SLayerRenderData::SLayerRenderData(SLayer &inLayer, Qt3DSRendererImpl &inRenderer)
        : SLayerRenderPreparationData(inLayer, inRenderer)
        , m_LayerTexture(inRenderer.GetQt3DSContext().GetResourceManager())
        , m_TemporalAATexture{inRenderer.GetQt3DSContext().GetResourceManager(),
                              inRenderer.GetQt3DSContext().GetResourceManager()}
        , m_LayerDepthTexture(inRenderer.GetQt3DSContext().GetResourceManager())
        , m_LayerPrepassDepthTexture(inRenderer.GetQt3DSContext().GetResourceManager())
        , m_LayerWidgetTexture(inRenderer.GetQt3DSContext().GetResourceManager())
        , m_LayerSsaoTexture(inRenderer.GetQt3DSContext().GetResourceManager())
        , m_LayerMultisampleTexture(inRenderer.GetQt3DSContext().GetResourceManager())
        , m_LayerMultisamplePrepassDepthTexture(inRenderer.GetQt3DSContext().GetResourceManager())
        , m_LayerMultisampleWidgetTexture(inRenderer.GetQt3DSContext().GetResourceManager())
        , m_LayerCachedTexture(NULL)
        , m_AdvancedBlendDrawTexture(NULL)
        , m_AdvancedBlendBlendTexture(NULL)
        , m_AdvancedModeDrawFB(NULL)
        , m_AdvancedModeBlendFB(NULL)
        , m_ProgressiveAAPassIndex(0)
        , m_TemporalAAPassIndex(0)
        , m_NonDirtyTemporalAAPassIndex(0)
        , m_TextScale(1.0f)
        , mRefCount(0)
        , m_DepthBufferFormat(NVRenderTextureFormats::Unknown)
    {

    }

    SLayerRenderData::~SLayerRenderData()
    {
        IResourceManager &theResourceManager(m_Renderer.GetQt3DSContext().GetResourceManager());
        if (m_LayerCachedTexture && m_LayerCachedTexture != m_LayerTexture)
            theResourceManager.Release(*m_LayerCachedTexture);
        if (m_AdvancedModeDrawFB) {
            m_AdvancedModeDrawFB->release();
            m_AdvancedModeDrawFB = NULL;
        }
        if (m_AdvancedModeBlendFB) {
            m_AdvancedModeBlendFB->release();
            m_AdvancedModeBlendFB = NULL;
        }
        if (m_AdvancedBlendBlendTexture)
            m_AdvancedBlendBlendTexture = NULL;
        if (m_AdvancedBlendDrawTexture)
            m_AdvancedBlendDrawTexture = NULL;
    }
    void SLayerRenderData::PrepareForRender(const QSize &inViewportDimensions)
    {
        SLayerRenderPreparationData::PrepareForRender(inViewportDimensions);
        SLayerRenderPreparationResult &thePrepResult(*m_LayerPrepResult);
        IResourceManager &theResourceManager(m_Renderer.GetQt3DSContext().GetResourceManager());

        // Generate all necessary lighting keys

        if (thePrepResult.m_Flags.WasLayerDataDirty()) {
            m_ProgressiveAAPassIndex = 0;
        }

        // Get rid of the layer texture if we aren't rendering to texture this frame.
        if (m_LayerTexture && !thePrepResult.m_Flags.ShouldRenderToTexture()) {
            if (m_LayerCachedTexture && m_LayerCachedTexture != m_LayerTexture) {
                theResourceManager.Release(*m_LayerCachedTexture);
                m_LayerCachedTexture = NULL;
            }

            m_LayerTexture.ReleaseTexture();
            m_LayerDepthTexture.ReleaseTexture();
            m_LayerWidgetTexture.ReleaseTexture();
            m_LayerSsaoTexture.ReleaseTexture();
            m_LayerMultisampleTexture.ReleaseTexture();
            m_LayerMultisamplePrepassDepthTexture.ReleaseTexture();
            m_LayerMultisampleWidgetTexture.ReleaseTexture();
        }

        if (NeedsWidgetTexture() == false)
            m_LayerWidgetTexture.ReleaseTexture();

        if (m_LayerDepthTexture && !thePrepResult.m_Flags.RequiresDepthTexture())
            m_LayerDepthTexture.ReleaseTexture();

        if (m_LayerSsaoTexture && !thePrepResult.m_Flags.RequiresSsaoPass())
            m_LayerSsaoTexture.ReleaseTexture();

        m_Renderer.LayerNeedsFrameClear(*this);

        // Clean up the texture cache if layer dimensions changed
        if (inViewportDimensions.width() != m_previousDimensions.width()
                || inViewportDimensions.height() != m_previousDimensions.height()) {
            m_LayerTexture.ReleaseTexture();
            m_LayerDepthTexture.ReleaseTexture();
            m_LayerSsaoTexture.ReleaseTexture();
            m_LayerWidgetTexture.ReleaseTexture();
            m_LayerPrepassDepthTexture.ReleaseTexture();
            m_TemporalAATexture[0].ReleaseTexture();
            m_TemporalAATexture[1].ReleaseTexture();
            m_LayerMultisampleTexture.ReleaseTexture();
            m_LayerMultisamplePrepassDepthTexture.ReleaseTexture();
            m_LayerMultisampleWidgetTexture.ReleaseTexture();

            m_previousDimensions.setWidth(inViewportDimensions.width());
            m_previousDimensions.setHeight(inViewportDimensions.height());

            theResourceManager.DestroyFreeSizedResources();

            // Effect system uses different resource manager, so clean that up too
            m_Renderer.GetQt3DSContext().GetEffectSystem().GetResourceManager()
                    .DestroyFreeSizedResources();
        }
    }

    NVRenderTextureFormats::Enum SLayerRenderData::GetDepthBufferFormat()
    {
        if (m_DepthBufferFormat == NVRenderTextureFormats::Unknown) {
            QT3DSU32 theExistingDepthBits = m_Renderer.GetContext().GetDepthBits();
            QT3DSU32 theExistingStencilBits = m_Renderer.GetContext().GetStencilBits();
            switch (theExistingDepthBits) {
            case 32:
                m_DepthBufferFormat = NVRenderTextureFormats::Depth32;
                break;
            case 24:
                //  check if we have stencil bits
                if (theExistingStencilBits > 0)
                    m_DepthBufferFormat =
                        NVRenderTextureFormats::Depth24Stencil8; // currently no stencil usage
                                                                 // should be Depth24Stencil8 in
                                                                 // this case
                else
                    m_DepthBufferFormat = NVRenderTextureFormats::Depth24;
                break;
            case 16:
                m_DepthBufferFormat = NVRenderTextureFormats::Depth16;
                break;
            default:
                QT3DS_ASSERT(false);
                m_DepthBufferFormat = NVRenderTextureFormats::Depth16;
                break;
            }
        }
        return m_DepthBufferFormat;
    }

    NVRenderFrameBufferAttachments::Enum
    SLayerRenderData::GetFramebufferDepthAttachmentFormat(NVRenderTextureFormats::Enum depthFormat)
    {
        NVRenderFrameBufferAttachments::Enum fmt = NVRenderFrameBufferAttachments::Depth;

        switch (depthFormat) {
        case NVRenderTextureFormats::Depth16:
        case NVRenderTextureFormats::Depth24:
        case NVRenderTextureFormats::Depth32:
            fmt = NVRenderFrameBufferAttachments::Depth;
            break;
        case NVRenderTextureFormats::Depth24Stencil8:
            fmt = NVRenderFrameBufferAttachments::DepthStencil;
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }

        return fmt;
    }

    void SLayerRenderData::RenderAoPass()
    {
        m_Renderer.BeginLayerDepthPassRender(*this);
        m_Renderer.UpdateCbAoShadow(&m_Layer, m_Camera, m_LayerDepthTexture);

        NVRenderContext &theContext(m_Renderer.GetContext());
        SDefaultAoPassShader *shader = m_Renderer.GetDefaultAoPassShader(GetShaderFeatureSet());
        if (shader == NULL)
            return;

        // Set initial state
        theContext.SetBlendingEnabled(false);
        theContext.SetDepthWriteEnabled(false);
        theContext.SetDepthTestEnabled(false);
        theContext.SetActiveShader(&(shader->m_Shader));

        // Setup constants
        shader->m_CameraDirection.Set(m_CameraDirection);
        shader->m_ViewMatrix.Set(m_Camera->m_GlobalTransform);

        shader->m_DepthTexture.Set(m_LayerDepthTexture);
        shader->m_DepthSamplerSize.Set(QT3DSVec2(m_LayerDepthTexture->GetTextureDetails().m_Width,
                                                 m_LayerDepthTexture->GetTextureDetails().m_Height));

        // Important uniforms for AO calculations
        QT3DSVec2 theCameraProps = QT3DSVec2(m_Camera->m_ClipNear, m_Camera->m_ClipFar);
        shader->m_CameraProperties.Set(theCameraProps);
        shader->m_AoShadowParams.Set();

        // Draw a fullscreen quad
        m_Renderer.RenderQuad();

        m_Renderer.EndLayerDepthPassRender();
    }

    void SLayerRenderData::RenderFakeDepthMapPass(NVRenderTexture2D *theDepthTex,
                                                  NVRenderTextureCube *theDepthCube)
    {
        m_Renderer.BeginLayerDepthPassRender(*this);

        NVRenderContext &theContext(m_Renderer.GetContext());
        SDefaultAoPassShader *shader = theDepthTex
            ? m_Renderer.GetFakeDepthShader(GetShaderFeatureSet())
            : m_Renderer.GetFakeCubeDepthShader(GetShaderFeatureSet());
        if (shader == NULL)
            return;

        // Set initial state
        theContext.SetBlendingEnabled(false);
        theContext.SetDepthWriteEnabled(false);
        theContext.SetDepthTestEnabled(false);
        theContext.SetActiveShader(&(shader->m_Shader));

        // Setup constants
        shader->m_CameraDirection.Set(m_CameraDirection);
        shader->m_ViewMatrix.Set(m_Camera->m_GlobalTransform);

        shader->m_DepthTexture.Set(theDepthTex);
        shader->m_CubeTexture.Set(theDepthCube);
        shader->m_DepthSamplerSize.Set(QT3DSVec2(theDepthTex->GetTextureDetails().m_Width,
                                                 theDepthTex->GetTextureDetails().m_Height));

        // Important uniforms for AO calculations
        QT3DSVec2 theCameraProps = QT3DSVec2(m_Camera->m_ClipNear, m_Camera->m_ClipFar);
        shader->m_CameraProperties.Set(theCameraProps);
        shader->m_AoShadowParams.Set();

        // Draw a fullscreen quad
        m_Renderer.RenderQuad();
    }

    namespace {

        void computeFrustumBounds(const SCamera &inCamera, const NVRenderRectF &inViewPort,
                                  QT3DSVec3 &ctrBound, QT3DSVec3 camVerts[8])
        {
            QT3DSVec3 camEdges[4];

            const QT3DSF32 *dataPtr(inCamera.m_GlobalTransform.front());
            QT3DSVec3 camX(dataPtr[0], dataPtr[1], dataPtr[2]);
            QT3DSVec3 camY(dataPtr[4], dataPtr[5], dataPtr[6]);
            QT3DSVec3 camZ(dataPtr[8], dataPtr[9], dataPtr[10]);

            float tanFOV = tanf(inCamera.verticalFov(inViewPort) * 0.5f);
            float asTanFOV = tanFOV * inViewPort.m_Width / inViewPort.m_Height;
            camEdges[0] = -asTanFOV * camX + tanFOV * camY + camZ;
            camEdges[1] = asTanFOV * camX + tanFOV * camY + camZ;
            camEdges[2] = asTanFOV * camX - tanFOV * camY + camZ;
            camEdges[3] = -asTanFOV * camX - tanFOV * camY + camZ;

            for (int i = 0; i < 4; ++i) {
                camEdges[i].x = -camEdges[i].x;
                camEdges[i].y = -camEdges[i].y;
            }

            camVerts[0] = inCamera.m_Position + camEdges[0] * inCamera.m_ClipNear;
            camVerts[1] = inCamera.m_Position + camEdges[0] * inCamera.m_ClipFar;
            camVerts[2] = inCamera.m_Position + camEdges[1] * inCamera.m_ClipNear;
            camVerts[3] = inCamera.m_Position + camEdges[1] * inCamera.m_ClipFar;
            camVerts[4] = inCamera.m_Position + camEdges[2] * inCamera.m_ClipNear;
            camVerts[5] = inCamera.m_Position + camEdges[2] * inCamera.m_ClipFar;
            camVerts[6] = inCamera.m_Position + camEdges[3] * inCamera.m_ClipNear;
            camVerts[7] = inCamera.m_Position + camEdges[3] * inCamera.m_ClipFar;

            ctrBound = camVerts[0];
            for (int i = 1; i < 8; ++i) {
                ctrBound += camVerts[i];
            }
            ctrBound *= 0.125f;
        }

        void SetupCameraForShadowMap(const QT3DSVec2 &inCameraVec, NVRenderContext & /*inContext*/,
                                     const NVRenderRectF &inViewport, const SCamera &inCamera,
                                     const SLight *inLight, SCamera &theCamera)
        {
            // setup light matrix
            QT3DSU32 mapRes = 1 << inLight->m_ShadowMapRes;
            NVRenderRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
            theCamera.m_ClipNear = 1.0f;
            theCamera.m_ClipFar = inLight->m_ShadowMapFar;
            // Setup camera projection
            QT3DSVec3 inLightPos = inLight->GetGlobalPos();
            QT3DSVec3 inLightDir = inLight->GetDirection();

            if (inLight->m_Flags.IsLeftHanded())
                inLightPos.z = -inLightPos.z;

            inLightPos -= inLightDir * inCamera.m_ClipNear;
            theCamera.m_FOV = inLight->m_ShadowMapFov * QT3DS_DEGREES_TO_RADIANS;

            if (inLight->m_LightType == RenderLightTypes::Directional) {
                QT3DSVec3 frustBounds[8], boundCtr;
                computeFrustumBounds(inCamera, inViewport, boundCtr, frustBounds);

                QT3DSVec3 forward = inLightDir;
                forward.normalize();
                QT3DSVec3 right = forward.cross(QT3DSVec3(0, 1, 0));
                right.normalize();
                QT3DSVec3 up = right.cross(forward);
                up.normalize();

                // Calculate bounding box of the scene camera frustum
                float minDistanceZ = std::numeric_limits<float>::max();
                float maxDistanceZ = -std::numeric_limits<float>::max();
                float minDistanceY = std::numeric_limits<float>::max();
                float maxDistanceY = -std::numeric_limits<float>::max();
                float minDistanceX = std::numeric_limits<float>::max();
                float maxDistanceX = -std::numeric_limits<float>::max();
                for (int i = 0; i < 8; ++i) {
                    float distanceZ = frustBounds[i].dot(forward);
                    if (distanceZ < minDistanceZ)
                        minDistanceZ = distanceZ;
                    if (distanceZ > maxDistanceZ)
                        maxDistanceZ = distanceZ;
                    float distanceY = frustBounds[i].dot(up);
                    if (distanceY < minDistanceY)
                        minDistanceY = distanceY;
                    if (distanceY > maxDistanceY)
                        maxDistanceY = distanceY;
                    float distanceX = frustBounds[i].dot(right);
                    if (distanceX < minDistanceX)
                        minDistanceX = distanceX;
                    if (distanceX > maxDistanceX)
                        maxDistanceX = distanceX;
                }

                // Apply bounding box parameters to shadow map camera projection matrix
                // so that the whole scene is fit inside the shadow map
                inLightPos = boundCtr;
                theViewport.m_Height = abs(maxDistanceY - minDistanceY);
                theViewport.m_Width = abs(maxDistanceX - minDistanceX);
                theCamera.m_ClipNear = -abs(maxDistanceZ - minDistanceZ);
                theCamera.m_ClipFar = abs(maxDistanceZ - minDistanceZ);
            }

            theCamera.m_Flags.SetLeftHanded(false);

            theCamera.m_Flags.ClearOrSet(inLight->m_LightType == RenderLightTypes::Directional,
                                         NodeFlagValues::Orthographic);
            theCamera.m_Parent = NULL;
            theCamera.m_Pivot = inLight->m_Pivot;

            if (inLight->m_LightType != RenderLightTypes::Point) {
                theCamera.LookAt(inLightPos, QT3DSVec3(0, 1.0, 0), inLightPos + inLightDir);
            } else {
                theCamera.LookAt(inLightPos, QT3DSVec3(0, 1.0, 0), QT3DSVec3(0, 0, 0));
            }

            theCamera.CalculateGlobalVariables(theViewport,
                                               QT3DSVec2(theViewport.m_Width, theViewport.m_Height));
        }
    }

    void SetupCubeShadowCameras(const SLight *inLight, SCamera inCameras[6])
    {
        // setup light matrix
        QT3DSU32 mapRes = 1 << inLight->m_ShadowMapRes;
        NVRenderRectF theViewport(0.0f, 0.0f, (float)mapRes, (float)mapRes);
        QT3DSVec3 rotOfs[6];

        QT3DS_ASSERT(inLight != NULL);
        QT3DS_ASSERT(inLight->m_LightType != RenderLightTypes::Directional);

        QT3DSVec3 inLightPos = inLight->GetGlobalPos();
        if (inLight->m_Flags.IsLeftHanded())
            inLightPos.z = -inLightPos.z;

        rotOfs[0] = QT3DSVec3(0.f, -NVHalfPi, NVPi);
        rotOfs[1] = QT3DSVec3(0.f, NVHalfPi, NVPi);
        rotOfs[2] = QT3DSVec3(NVHalfPi, 0.f, 0.f);
        rotOfs[3] = QT3DSVec3(-NVHalfPi, 0.f, 0.f);
        rotOfs[4] = QT3DSVec3(0.f, NVPi, -NVPi);
        rotOfs[5] = QT3DSVec3(0.f, 0.f, NVPi);

        for (int i = 0; i < 6; ++i) {
            inCameras[i].m_Flags.SetLeftHanded(false);

            inCameras[i].m_Flags.ClearOrSet(false, NodeFlagValues::Orthographic);
            inCameras[i].m_Parent = NULL;
            inCameras[i].m_Pivot = inLight->m_Pivot;
            inCameras[i].m_ClipNear = 1.0f;
            inCameras[i].m_ClipFar = NVMax<QT3DSF32>(2.0f, inLight->m_ShadowMapFar);
            inCameras[i].m_FOV = inLight->m_ShadowMapFov * QT3DS_DEGREES_TO_RADIANS;

            inCameras[i].m_Position = inLightPos;
            inCameras[i].m_Rotation = rotOfs[i];
            inCameras[i].CalculateGlobalVariables(
                theViewport, QT3DSVec2(theViewport.m_Width, theViewport.m_Height));
        }

        /*
        if ( inLight->m_LightType == RenderLightTypes::Point ) return;

        QT3DSVec3 viewDirs[6];
        QT3DSVec3 viewUp[6];
        QT3DSMat33 theDirMatrix( inLight->m_GlobalTransform.getUpper3x3() );

        viewDirs[0] = theDirMatrix.transform( QT3DSVec3( 1.f, 0.f, 0.f ) );
        viewDirs[2] = theDirMatrix.transform( QT3DSVec3( 0.f, -1.f, 0.f ) );
        viewDirs[4] = theDirMatrix.transform( QT3DSVec3( 0.f, 0.f, 1.f ) );
        viewDirs[0].normalize();  viewDirs[2].normalize();  viewDirs[4].normalize();
        viewDirs[1] = -viewDirs[0];
        viewDirs[3] = -viewDirs[2];
        viewDirs[5] = -viewDirs[4];

        viewUp[0] = viewDirs[2];
        viewUp[1] = viewDirs[2];
        viewUp[2] = viewDirs[5];
        viewUp[3] = viewDirs[4];
        viewUp[4] = viewDirs[2];
        viewUp[5] = viewDirs[2];

        for (int i = 0; i < 6; ++i)
        {
                inCameras[i].LookAt( inLightPos, viewUp[i], inLightPos + viewDirs[i] );
                inCameras[i].CalculateGlobalVariables( theViewport, QT3DSVec2( theViewport.m_Width,
        theViewport.m_Height ) );
        }
        */
    }

    inline void RenderRenderableShadowMapPass(SLayerRenderData &inData, SRenderableObject &inObject,
                                              const QT3DSVec2 &inCameraProps, TShaderFeatureSet set,
                                              QT3DSU32 lightIndex, const SCamera &inCamera)
    {
        if (!inObject.m_RenderableFlags.IsShadowCaster())
            return;

        SShadowMapEntry *pEntry = inData.m_ShadowMapManager->GetShadowMapEntry(lightIndex);

        if (inObject.m_RenderableFlags.IsDefaultMaterialMeshSubset()) {
            auto &sub = static_cast<SSubsetRenderable &>(inObject);
            if (sub.m_Generator.alphaTestEnabled()) {
                sub.RenderShadow(inCameraProps, set, inData.m_Lights[lightIndex], inCamera, pEntry);
            } else {
                sub.RenderShadowMapPass(inCameraProps, inData.m_Lights[lightIndex], inCamera,
                                        pEntry);
            }
        } else if (inObject.m_RenderableFlags.IsCustomMaterialMeshSubset()) {
            static_cast<SSubsetRenderableBase &>(inObject).RenderShadowMapPass(
                inCameraProps, inData.m_Lights[lightIndex], inCamera, pEntry);
        } else if (inObject.m_RenderableFlags.IsPath()) {
            static_cast<SPathRenderable &>(inObject).RenderShadowMapPass(
                inCameraProps, inData.m_Lights[lightIndex], inCamera, pEntry);
        }
    }

    void SLayerRenderData::RenderShadowCubeBlurPass(CResourceFrameBuffer *theFB,
                                                    NVRenderTextureCube *target0,
                                                    NVRenderTextureCube *target1, QT3DSF32 filterSz,
                                                    QT3DSF32 clipFar)
    {
        NVRenderContext &theContext(m_Renderer.GetContext());

        SShadowmapPreblurShader *shaderX = m_Renderer.GetCubeShadowBlurXShader();
        SShadowmapPreblurShader *shaderY = m_Renderer.GetCubeShadowBlurYShader();

        if (shaderX == NULL)
            return;
        if (shaderY == NULL)
            return;
        // if ( theShader == NULL ) return;

        // Enable drawing to 6 color attachment buffers for cubemap passes
        qt3ds::QT3DSI32 buffers[6] = { 0, 1, 2, 3, 4, 5 };
        qt3ds::foundation::NVConstDataRef<qt3ds::QT3DSI32> bufferList(buffers, 6);
        theContext.SetDrawBuffers(bufferList);

        // Attach framebuffer targets
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color0, *target1,
                             NVRenderTextureCubeFaces::CubePosX);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color1, *target1,
                             NVRenderTextureCubeFaces::CubeNegX);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color2, *target1,
                             NVRenderTextureCubeFaces::CubePosY);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color3, *target1,
                             NVRenderTextureCubeFaces::CubeNegY);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color4, *target1,
                             NVRenderTextureCubeFaces::CubePosZ);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color5, *target1,
                             NVRenderTextureCubeFaces::CubeNegZ);

        // Set initial state
        theContext.SetBlendingEnabled(false);
        theContext.SetDepthWriteEnabled(false);
        theContext.SetDepthTestEnabled(false);
        // theContext.SetColorWritesEnabled(true);
        theContext.SetActiveShader(&(shaderX->m_Shader));

        shaderX->m_CameraProperties.Set(QT3DSVec2(filterSz, clipFar));
        shaderX->m_DepthCube.Set(target0);

        // Draw a fullscreen quad
        m_Renderer.RenderQuad();

        theContext.SetActiveShader(&(shaderY->m_Shader));

        // Lather, Rinse, and Repeat for the Y-blur pass
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color0, *target0,
                             NVRenderTextureCubeFaces::CubePosX);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color1, *target0,
                             NVRenderTextureCubeFaces::CubeNegX);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color2, *target0,
                             NVRenderTextureCubeFaces::CubePosY);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color3, *target0,
                             NVRenderTextureCubeFaces::CubeNegY);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color4, *target0,
                             NVRenderTextureCubeFaces::CubePosZ);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color5, *target0,
                             NVRenderTextureCubeFaces::CubeNegZ);

        shaderY->m_CameraProperties.Set(QT3DSVec2(filterSz, clipFar));
        shaderY->m_DepthCube.Set(target1);

        // Draw a fullscreen quad
        m_Renderer.RenderQuad();

        theContext.SetDepthWriteEnabled(true);
        theContext.SetDepthTestEnabled(true);
        // theContext.SetColorWritesEnabled(false);

        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color0, NVRenderTextureOrRenderBuffer(),
                             NVRenderTextureCubeFaces::CubePosX);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color1, NVRenderTextureOrRenderBuffer(),
                             NVRenderTextureCubeFaces::CubeNegX);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color2, NVRenderTextureOrRenderBuffer(),
                             NVRenderTextureCubeFaces::CubePosY);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color3, NVRenderTextureOrRenderBuffer(),
                             NVRenderTextureCubeFaces::CubeNegY);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color4, NVRenderTextureOrRenderBuffer(),
                             NVRenderTextureCubeFaces::CubePosZ);
        (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color5, NVRenderTextureOrRenderBuffer(),
                             NVRenderTextureCubeFaces::CubeNegZ);

        theContext.SetDrawBuffers(qt3ds::foundation::toConstDataRef((qt3ds::QT3DSI32)0));
    }

    void SLayerRenderData::RenderShadowMapBlurPass(CResourceFrameBuffer *theFB,
                                                   NVRenderTexture2D *target0,
                                                   NVRenderTexture2D *target1, QT3DSF32 filterSz,
                                                   QT3DSF32 clipFar)
    {
        NVRenderContext &theContext(m_Renderer.GetContext());

        SShadowmapPreblurShader *shaderX = m_Renderer.GetOrthoShadowBlurXShader();
        SShadowmapPreblurShader *shaderY = m_Renderer.GetOrthoShadowBlurYShader();

        if (shaderX == NULL)
            return;
        if (shaderY == NULL)
            return;

        // Attach framebuffer target
        (*theFB)->Attach(NVRenderFrameBufferAttachments::Color0, *target1);
        //(*theFB)->Attach( NVRenderFrameBufferAttachments::DepthStencil, *target1 );

        // Set initial state
        theContext.SetBlendingEnabled(false);
        theContext.SetDepthWriteEnabled(false);
        theContext.SetDepthTestEnabled(false);
        theContext.SetColorWritesEnabled(true);
        theContext.SetActiveShader(&(shaderX->m_Shader));

        shaderX->m_CameraProperties.Set(QT3DSVec2(filterSz, clipFar));
        shaderX->m_DepthMap.Set(target0);

        // Draw a fullscreen quad
        m_Renderer.RenderQuad();

        (*theFB)->Attach(NVRenderFrameBufferAttachments::Color0, *target0);
        //(*theFB)->Attach( NVRenderFrameBufferAttachments::DepthStencil, *target0 );
        theContext.SetActiveShader(&(shaderY->m_Shader));

        shaderY->m_CameraProperties.Set(QT3DSVec2(filterSz, clipFar));
        shaderY->m_DepthMap.Set(target1);

        // Draw a fullscreen quad
        m_Renderer.RenderQuad();

        theContext.SetDepthWriteEnabled(true);
        theContext.SetDepthTestEnabled(true);
        theContext.SetColorWritesEnabled(false);

        //(*theFB)->Attach( NVRenderFrameBufferAttachments::DepthStencil,
        //NVRenderTextureOrRenderBuffer() );
        (*theFB)->Attach(NVRenderFrameBufferAttachments::Color0, NVRenderTextureOrRenderBuffer());
    }

    void SLayerRenderData::RenderShadowMapPass(CResourceFrameBuffer *theFB)
    {
        QT3DS_PERF_SCOPED_TIMER(m_Renderer.GetQt3DSContext().GetPerfTimer(),
                                "SLayerRenderData::RenderShadowMapPass")

        if (m_Camera == NULL || !GetShadowMapManager())
            return;

        // Check if we have anything to render
        if ((m_OpaqueObjects.size() == 0 && GetTransparentRenderableObjects().size() == 0)
                || m_Lights.size() == 0) {
            return;
        }

        m_Renderer.BeginLayerDepthPassRender(*this);

        NVRenderContext &theRenderContext(m_Renderer.GetContext());

        // we may change the viewport
        NVRenderContextScopedProperty<NVRenderRect> __viewport(
            theRenderContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport);

        // disable color writes
        // theRenderContext.SetColorWritesEnabled( false );
        theRenderContext.SetColorWritesEnabled(true);
        theRenderContext.SetDepthWriteEnabled(true);
        theRenderContext.SetCullingEnabled(false);
        theRenderContext.SetClearColor(QT3DSVec4(1.0f));

        // we render the shadow map with a slight offset to prevent shadow acne and cull the front
        // faces
        NVScopedRefCounted<qt3ds::render::NVRenderRasterizerState> rsdefaultstate =
            theRenderContext.CreateRasterizerState(0.0, 0.0, qt3ds::render::NVRenderFaces::Back);
        NVScopedRefCounted<qt3ds::render::NVRenderRasterizerState> rsstate =
            theRenderContext.CreateRasterizerState(1.5, 2.0, qt3ds::render::NVRenderFaces::Front);
        theRenderContext.SetRasterizerState(rsstate);

        qt3ds::render::NVRenderClearFlags clearFlags(qt3ds::render::NVRenderClearValues::Depth
                                                  | qt3ds::render::NVRenderClearValues::Stencil
                                                  | qt3ds::render::NVRenderClearValues::Color);

        for (QT3DSU32 i = 0; i < m_Lights.size(); i++) {
            // don't render shadows when not casting
            if (m_Lights[i]->m_CastShadow == false)
                continue;
            SShadowMapEntry *pEntry = m_ShadowMapManager->GetShadowMapEntry(i);
            if (pEntry && pEntry->m_DepthMap && pEntry->m_DepthCopy && pEntry->m_DepthRender) {
                SCamera theCamera;

                QT3DSVec2 theCameraProps = QT3DSVec2(m_Camera->m_ClipNear, m_Camera->m_ClipFar);
                SetupCameraForShadowMap(theCameraProps, m_Renderer.GetContext(),
                                        __viewport.m_InitialValue, *m_Camera,
                                        m_Lights[i], theCamera);
                // we need this matrix for the final rendering
                theCamera.CalculateViewProjectionMatrix(pEntry->m_LightVP);
                pEntry->m_LightView = theCamera.m_GlobalTransform.getInverse();

                STextureDetails theDetails(pEntry->m_DepthMap->GetTextureDetails());
                theRenderContext.SetViewport(
                    NVRenderRect(0, 0, (QT3DSU32)theDetails.m_Width, (QT3DSU32)theDetails.m_Height));

                (*theFB)->Attach(NVRenderFrameBufferAttachments::Color0, *pEntry->m_DepthMap);
                (*theFB)->Attach(NVRenderFrameBufferAttachments::DepthStencil,
                                 *pEntry->m_DepthRender);
                theRenderContext.Clear(clearFlags);

                RunRenderPass(RenderRenderableShadowMapPass, false, true, true, i, theCamera);
                RenderShadowMapBlurPass(theFB, pEntry->m_DepthMap, pEntry->m_DepthCopy,
                                        m_Lights[i]->m_ShadowFilter, m_Lights[i]->m_ShadowMapFar);
            } else if (pEntry && pEntry->m_DepthCube && pEntry->m_CubeCopy
                       && pEntry->m_DepthRender) {
                SCamera theCameras[6];

                SetupCubeShadowCameras(m_Lights[i], theCameras);

                // pEntry->m_LightView = m_Lights[i]->m_LightType == RenderLightTypes::Point ?
                // QT3DSMat44::createIdentity()
                //	: m_Lights[i]->m_GlobalTransform;
                pEntry->m_LightView = QT3DSMat44::createIdentity();

                STextureDetails theDetails(pEntry->m_DepthCube->GetTextureDetails());
                theRenderContext.SetViewport(
                    NVRenderRect(0, 0, (QT3DSU32)theDetails.m_Width, (QT3DSU32)theDetails.m_Height));

                // int passes = m_Lights[i]->m_LightType == RenderLightTypes::Point ? 6 : 5;
                int passes = 6;
                for (int k = 0; k < passes; ++k) {
                    // theCameras[k].CalculateViewProjectionMatrix( pEntry->m_LightCubeVP[k] );
                    pEntry->m_LightCubeView[k] = theCameras[k].m_GlobalTransform.getInverse();
                    theCameras[k].CalculateViewProjectionMatrix(pEntry->m_LightVP);

                    // Geometry shader multiplication really doesn't work unless you have a
                    // 6-layered 3D depth texture...
                    // Otherwise, you have no way to depth test while rendering...
                    // which more or less completely defeats the purpose of having a cubemap render
                    // target.
                    NVRenderTextureCubeFaces::Enum curFace =
                        (NVRenderTextureCubeFaces::Enum)(k + 1);
                    //(*theFB)->AttachFace( NVRenderFrameBufferAttachments::DepthStencil,
                    //*pEntry->m_DepthCube, curFace );
                    (*theFB)->Attach(NVRenderFrameBufferAttachments::DepthStencil,
                                     *pEntry->m_DepthRender);
                    (*theFB)->AttachFace(NVRenderFrameBufferAttachments::Color0,
                                         *pEntry->m_DepthCube, curFace);
                    (*theFB)->IsComplete();
                    theRenderContext.Clear(clearFlags);

                    RunRenderPass(RenderRenderableShadowMapPass, false, true, true, i,
                                  theCameras[k]);
                }

                RenderShadowCubeBlurPass(theFB, pEntry->m_DepthCube, pEntry->m_CubeCopy,
                                         m_Lights[i]->m_ShadowFilter, m_Lights[i]->m_ShadowMapFar);
            }
        }

        (*theFB)->Attach(NVRenderFrameBufferAttachments::Depth, NVRenderTextureOrRenderBuffer());
        (*theFB)->Attach(NVRenderFrameBufferAttachments::Color0, NVRenderTextureOrRenderBuffer());
        (*theFB)->Attach(NVRenderFrameBufferAttachments::DepthStencil,
                         NVRenderTextureOrRenderBuffer());

        // enable color writes
        theRenderContext.SetColorWritesEnabled(true);
        theRenderContext.SetCullingEnabled(true);
        theRenderContext.SetClearColor(QT3DSVec4(0.0f));
        // reset rasterizer state
        theRenderContext.SetRasterizerState(rsdefaultstate);

        m_Renderer.EndLayerDepthPassRender();
    }

    inline void RenderRenderableDepthPass(SLayerRenderData &inData, SRenderableObject &inObject,
                                          const QT3DSVec2 &inCameraProps, TShaderFeatureSet set,
                                          QT3DSU32, const SCamera &inCamera)
    {
        if (inObject.m_RenderableFlags.IsDefaultMaterialMeshSubset()) {
            SSubsetRenderable &sub = static_cast<SSubsetRenderable &>(inObject);
            if (sub.m_Generator.alphaTestEnabled())
                sub.Render(inCameraProps, set, true);
            else
                sub.RenderDepthPass(inCameraProps);
        } else if (inObject.m_RenderableFlags.IsText()) {
            static_cast<STextRenderable &>(inObject).RenderDepthPass(inCameraProps);
#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
        } else if (inObject.m_RenderableFlags.isDistanceField()) {
            static_cast<SDistanceFieldRenderable &>(inObject).RenderDepthPass(inCameraProps);
#endif
        } else if (inObject.m_RenderableFlags.IsCustomMaterialMeshSubset()) {
            static_cast<SCustomMaterialRenderable &>(inObject).RenderDepthPass(
                inCameraProps, inData.m_Layer, inData.m_Lights, inCamera, NULL);
        } else if (inObject.m_RenderableFlags.IsPath()) {
            static_cast<SPathRenderable &>(inObject).RenderDepthPass(
                inCameraProps, inData.m_Layer, inData.m_Lights, inCamera, NULL);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    void SLayerRenderData::RenderDepthPass(bool inEnableTransparentDepthWrite)
    {
        QT3DS_PERF_SCOPED_TIMER(m_Renderer.GetQt3DSContext().GetPerfTimer(),
                                "SLayerRenderData::RenderDepthPass")
        if (m_Camera == NULL)
            return;

        // Avoid running this method if possible.
        if ((inEnableTransparentDepthWrite == false
             && ((m_OpaqueObjects.size() == 0 && m_TransparentObjects.size() == 0)
                 || m_Layer.m_Flags.IsLayerEnableDepthPrepass() == false))
            || m_Layer.m_Flags.IsLayerEnableDepthTest() == false)
            return;

        m_Renderer.BeginLayerDepthPassRender(*this);

        NVRenderContext &theRenderContext(m_Renderer.GetContext());

        // disable color writes
        theRenderContext.SetColorWritesEnabled(false);
        theRenderContext.SetDepthWriteEnabled(true);

        qt3ds::render::NVRenderClearFlags clearFlags(qt3ds::render::NVRenderClearValues::Stencil
                                                  | qt3ds::render::NVRenderClearValues::Depth);
        theRenderContext.Clear(clearFlags);

        RunRenderPass(RenderRenderableDepthPass, false, true, inEnableTransparentDepthWrite, 0,
                      *m_Camera);

        // enable color writes
        theRenderContext.SetColorWritesEnabled(true);

        m_Renderer.EndLayerDepthPassRender();
    }

    inline void RenderRenderable(SLayerRenderData &inData, SRenderableObject &inObject,
                                 const QT3DSVec2 &inCameraProps, TShaderFeatureSet inFeatureSet,
                                 QT3DSU32, const SCamera &inCamera)
    {
        if (inObject.m_RenderableFlags.IsDefaultMaterialMeshSubset())
            static_cast<SSubsetRenderable &>(inObject).Render(inCameraProps, inFeatureSet, false);
        else if (inObject.m_RenderableFlags.IsText())
            static_cast<STextRenderable &>(inObject).Render(inCameraProps);
#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
        else if (inObject.m_RenderableFlags.isDistanceField())
            static_cast<SDistanceFieldRenderable &>(inObject).Render(inCameraProps);
#endif
        else if (inObject.m_RenderableFlags.IsCustomMaterialMeshSubset()) {
            // PKC : Need a better place to do this.
            SCustomMaterialRenderable &theObject =
                static_cast<SCustomMaterialRenderable &>(inObject);
            if (!inData.m_Layer.m_LightProbe && theObject.m_Material.m_IblProbe)
                inData.SetShaderFeature("QT3DS_ENABLE_LIGHT_PROBE",
                                        theObject.m_Material.m_IblProbe->m_TextureData.m_Texture
                                            != NULL);
            else if (inData.m_Layer.m_LightProbe)
                inData.SetShaderFeature("QT3DS_ENABLE_LIGHT_PROBE",
                                        inData.m_Layer.m_LightProbe->m_TextureData.m_Texture
                                            != NULL);

            static_cast<SCustomMaterialRenderable &>(inObject).Render(
                inCameraProps, inData, inData.m_Layer, inData.m_Lights, inCamera,
                inData.m_LayerDepthTexture, inData.m_LayerSsaoTexture, inFeatureSet);
        } else if (inObject.m_RenderableFlags.IsPath()) {
            static_cast<SPathRenderable &>(inObject).Render(
                inCameraProps, inData.m_Layer, inData.m_Lights, inCamera,
                inData.m_LayerDepthTexture, inData.m_LayerSsaoTexture, inFeatureSet);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    void SLayerRenderData::renderTransparentObjectsPass(
            TRenderRenderableFunction inRenderFn, bool inEnableBlending,
            bool inEnableTransparentDepthWrite, QT3DSU32 indexLight,
            const SCamera &inCamera, CResourceFrameBuffer *theFB)
    {
        NVDataRef<SRenderableObject *> theTransparentObjects = GetTransparentRenderableObjects();
        NVRenderContext &theRenderContext(m_Renderer.GetContext());
        QT3DSVec2 theCameraProps = QT3DSVec2(m_Camera->m_ClipNear, m_Camera->m_ClipFar);
        if (inEnableBlending || m_Layer.m_Flags.IsLayerEnableDepthTest() == false) {
            theRenderContext.SetBlendingEnabled(true && inEnableBlending);
            theRenderContext.SetDepthWriteEnabled(inEnableTransparentDepthWrite);

            // Assume all objects have transparency if the layer's depth test enabled flag is true.
            if (m_Layer.m_Flags.IsLayerEnableDepthTest() == true) {
                for (QT3DSU32 idx = 0, end = theTransparentObjects.size(); idx < end; ++idx) {
                    SRenderableObject &theObject(*theTransparentObjects[idx]);
                    if (!(theObject.m_RenderableFlags.IsCompletelyTransparent())) {
#ifdef ADVANCED_BLEND_SW_FALLBACK
                        // SW fallback for advanced blend modes.
                        // Renders transparent objects to a separate FBO and blends them in shader
                        // with the opaque items and background.
                        DefaultMaterialBlendMode::Enum blendMode
                                = DefaultMaterialBlendMode::Enum::Normal;
                        if (theObject.m_RenderableFlags.IsDefaultMaterialMeshSubset())
                            blendMode = static_cast<SSubsetRenderable &>(theObject).getBlendingMode();
                        bool useBlendFallback = (blendMode == DefaultMaterialBlendMode::Overlay ||
                                                 blendMode == DefaultMaterialBlendMode::ColorBurn ||
                                                 blendMode == DefaultMaterialBlendMode::ColorDodge) &&
                                                !theRenderContext.IsAdvancedBlendHwSupported() &&
                                                !theRenderContext.IsAdvancedBlendHwSupportedKHR() &&
                                                m_LayerPrepassDepthTexture;
                        if (useBlendFallback)
                            SetupDrawFB(true);
#endif
                        SScopedLightsListScope lightsScope(m_Lights, m_LightDirections,
                                                           m_SourceLightDirections,
                                                           theObject.m_ScopedLights);
                        SetShaderFeature(m_CGLightingFeatureName, m_Lights.empty() == false);

                        inRenderFn(*this, theObject, theCameraProps, GetShaderFeatureSet(),
                                   indexLight, inCamera);
#ifdef ADVANCED_BLEND_SW_FALLBACK
                        // SW fallback for advanced blend modes.
                        // Continue blending after transparent objects have been rendered to a FBO
                        if (useBlendFallback) {
                            BlendAdvancedToFB(blendMode, true, theFB);
                            // restore blending status
                            theRenderContext.SetBlendingEnabled(inEnableBlending);
                            // restore depth test status
                            theRenderContext.SetDepthTestEnabled(
                                        m_Layer.m_Flags.IsLayerEnableDepthTest());
                            theRenderContext.SetDepthWriteEnabled(inEnableTransparentDepthWrite);
                        }
#endif
                    }
                }
            }
            // If the layer doesn't have depth enabled then we have to render via an alternate route
            // where the transparent objects vector could have both opaque and transparent objects.
            else {
                for (QT3DSU32 idx = 0, end = theTransparentObjects.size(); idx < end; ++idx) {
                    SRenderableObject &theObject(*theTransparentObjects[idx]);
                    if (!(theObject.m_RenderableFlags.IsCompletelyTransparent())) {
#ifdef ADVANCED_BLEND_SW_FALLBACK
                        DefaultMaterialBlendMode::Enum blendMode
                                = DefaultMaterialBlendMode::Enum::Normal;
                        if (theObject.m_RenderableFlags.IsDefaultMaterialMeshSubset())
                            blendMode = static_cast<SSubsetRenderable &>(theObject).getBlendingMode();
                        bool useBlendFallback = (blendMode == DefaultMaterialBlendMode::Overlay ||
                                                 blendMode == DefaultMaterialBlendMode::ColorBurn ||
                                                 blendMode == DefaultMaterialBlendMode::ColorDodge) &&
                                                !theRenderContext.IsAdvancedBlendHwSupported() &&
                                                !theRenderContext.IsAdvancedBlendHwSupportedKHR();

                        if (theObject.m_RenderableFlags.HasTransparency()) {
                            theRenderContext.SetBlendingEnabled(true && inEnableBlending);
                            // If we have SW fallback for blend mode, render to a FBO and blend back.
                            // Slow as this must be done per-object (transparent and opaque items
                            // are mixed, not batched)
                            if (useBlendFallback)
                                SetupDrawFB(false);
                        }
#endif
                        SScopedLightsListScope lightsScope(m_Lights, m_LightDirections,
                                                           m_SourceLightDirections,
                                                           theObject.m_ScopedLights);
                        SetShaderFeature(m_CGLightingFeatureName, m_Lights.empty() == false);
                        inRenderFn(*this, theObject, theCameraProps, GetShaderFeatureSet(),
                                   indexLight, inCamera);
#ifdef ADVANCED_BLEND_SW_FALLBACK
                        if (useBlendFallback) {
                            BlendAdvancedToFB(blendMode, false, theFB);
                            // restore blending status
                            theRenderContext.SetBlendingEnabled(inEnableBlending);

                        }
#endif
                    }
                }
            }
        }
    };

    void SLayerRenderData::RunRenderPass(TRenderRenderableFunction inRenderFn,
                                         bool inEnableBlending, bool inEnableDepthWrite,
                                         bool inEnableTransparentDepthWrite, QT3DSU32 indexLight,
                                         const SCamera &inCamera, CResourceFrameBuffer *theFB)
    {
        NVRenderContext &theRenderContext(m_Renderer.GetContext());
        theRenderContext.SetDepthFunction(qt3ds::render::NVRenderBoolOp::LessThanOrEqual);
        theRenderContext.SetBlendingEnabled(false);
        QT3DSVec2 theCameraProps = QT3DSVec2(m_Camera->m_ClipNear, m_Camera->m_ClipFar);
        NVDataRef<SRenderableObject *> theOpaqueObjects = GetOpaqueRenderableObjects();

        if (m_Layer.m_Flags.IsLayerEnableDepthTest()) {
            theRenderContext.SetDepthTestEnabled(true);
            theRenderContext.SetDepthWriteEnabled(inEnableDepthWrite);
        } else {
            theRenderContext.SetDepthWriteEnabled(false);
            theRenderContext.SetDepthTestEnabled(false);
        }

        for (QT3DSU32 idx = 0, end = theOpaqueObjects.size(); idx < end; ++idx) {
            SRenderableObject &theObject(*theOpaqueObjects[idx]);
            SScopedLightsListScope lightsScope(m_Lights, m_LightDirections, m_SourceLightDirections,
                                               theObject.m_ScopedLights);
            SetShaderFeature(m_CGLightingFeatureName, m_Lights.empty() == false);
            inRenderFn(*this, theObject, theCameraProps, GetShaderFeatureSet(), indexLight,
                       inCamera);
        }

        NVDataRef<SRenderableObject *> theTransparentObjects = GetTransparentRenderableObjects();
        // Also draw opaque parts of transparent objects
        m_Renderer.setAlphaTest(true, 1.0f, -1.0f + (1.0f / 255.0f));
        for (QT3DSU32 idx = 0, end = theTransparentObjects.size(); idx < end; ++idx) {
            SRenderableObject &theObject(*theTransparentObjects[idx]);
            SScopedLightsListScope lightsScope(m_Lights, m_LightDirections, m_SourceLightDirections,
                                               theObject.m_ScopedLights);
            SetShaderFeature(m_CGLightingFeatureName, m_Lights.empty() == false);
            inRenderFn(*this, theObject, theCameraProps, GetShaderFeatureSet(), indexLight,
                       inCamera);
        }

        m_Renderer.setAlphaTest(true, -1.0f, 1.0f);
        // transparent parts of transparent objects
        // does not render objects without alpha test enabled so
        // we need another pass without alpha test
        renderTransparentObjectsPass(inRenderFn, inEnableBlending, inEnableTransparentDepthWrite,
                                     indexLight, inCamera, theFB);

        m_Renderer.setAlphaTest(false, 1.0, 1.0);
        // transparent objects without alpha test
        renderTransparentObjectsPass(inRenderFn, inEnableBlending, inEnableTransparentDepthWrite,
                                     indexLight, inCamera, theFB);
    }

    void SLayerRenderData::Render(CResourceFrameBuffer *theFB)
    {
        QT3DS_PERF_SCOPED_TIMER(m_Renderer.GetQt3DSContext().GetPerfTimer(),
                                "SLayerRenderData::Render")
        if (m_Camera == NULL)
            return;

        m_Renderer.BeginLayerRender(*this);
        RunRenderPass(RenderRenderable, true, !m_Layer.m_Flags.IsLayerEnableDepthPrepass(), false,
                      0, *m_Camera, theFB);
        m_Renderer.EndLayerRender();
    }

    void SLayerRenderData::CreateGpuProfiler()
    {
        if (m_Renderer.GetContext().IsTimerQuerySupported()) {
            m_LayerProfilerGpu = IRenderProfiler::CreateGpuProfiler(
                m_Renderer.GetContext().GetFoundation(), m_Renderer.GetQt3DSContext(),
                m_Renderer.GetContext());
        }
    }

    void SLayerRenderData::StartProfiling(CRegisteredString &nameID, bool sync)
    {
        if (m_LayerProfilerGpu.mPtr) {
            m_LayerProfilerGpu->StartTimer(nameID, false, sync);
        }
    }

    void SLayerRenderData::EndProfiling(CRegisteredString &nameID)
    {
        if (m_LayerProfilerGpu.mPtr) {
            m_LayerProfilerGpu->EndTimer(nameID);
        }
    }

    void SLayerRenderData::StartProfiling(const char *nameID, bool sync)
    {
        if (m_LayerProfilerGpu.mPtr) {
            CRegisteredString theStr(
                m_Renderer.GetQt3DSContext().GetStringTable().RegisterStr(nameID));
            m_LayerProfilerGpu->StartTimer(theStr, false, sync);
        }
    }

    void SLayerRenderData::EndProfiling(const char *nameID)
    {
        if (m_LayerProfilerGpu.mPtr) {
            CRegisteredString theStr(
                m_Renderer.GetQt3DSContext().GetStringTable().RegisterStr(nameID));
            m_LayerProfilerGpu->EndTimer(theStr);
        }
    }

    void SLayerRenderData::AddVertexCount(QT3DSU32 count)
    {
        if (m_LayerProfilerGpu.mPtr) {
            m_LayerProfilerGpu->AddVertexCount(count);
        }
    }

    // Assumes the viewport is setup appropriately to render the widget.
    void SLayerRenderData::RenderRenderWidgets()
    {
        if (m_Camera) {
            NVRenderContext &theContext(m_Renderer.GetContext());
            for (QT3DSU32 idx = 0, end = m_IRenderWidgets.size(); idx < end; ++idx) {
                IRenderWidget &theWidget = *m_IRenderWidgets[idx];
                theWidget.Render(m_Renderer, theContext);
            }
        }
    }

#ifdef ADVANCED_BLEND_SW_FALLBACK
    void SLayerRenderData::BlendAdvancedEquationSwFallback(NVRenderTexture2D *drawTexture,
                                                           NVRenderTexture2D *layerTexture,
                                                           AdvancedBlendModes::Enum blendMode)
    {
        NVRenderContext &theContext(m_Renderer.GetContext());
        SAdvancedModeBlendShader *shader = m_Renderer.GetAdvancedBlendModeShader(blendMode);
        if (shader == NULL)
            return;

        theContext.SetActiveShader(&(shader->m_Shader));

        shader->m_baseLayer.Set(layerTexture);
        shader->m_blendLayer.Set(drawTexture);
        // Draw a fullscreen quad
        m_Renderer.RenderQuad();
    }

    void SLayerRenderData::SetupDrawFB(bool depthEnabled)
    {
        NVRenderContext &theRenderContext(m_Renderer.GetContext());
        // create drawing FBO and texture, if not existing
        if (!m_AdvancedModeDrawFB)
            m_AdvancedModeDrawFB = theRenderContext.CreateFrameBuffer();
        if (!m_AdvancedBlendDrawTexture) {
            m_AdvancedBlendDrawTexture = theRenderContext.CreateTexture2D();
            NVRenderRect theViewport = m_Renderer.GetQt3DSContext().GetRenderList().GetViewport();
            m_AdvancedBlendDrawTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0,
                                                       theViewport.m_Width,
                                                       theViewport.m_Height,
                                                       NVRenderTextureFormats::RGBA8);
            m_AdvancedModeDrawFB->Attach(NVRenderFrameBufferAttachments::Color0,
                                         *m_AdvancedBlendDrawTexture);
            // Use existing depth prepass information when rendering transparent objects to a FBO
            if (depthEnabled)
                m_AdvancedModeDrawFB->Attach(NVRenderFrameBufferAttachments::Depth,
                                             *m_LayerPrepassDepthTexture);
        }
        theRenderContext.SetRenderTarget(m_AdvancedModeDrawFB);
        // make sure that depth testing is on in order to render just the
        // depth-passed objects (=transparent objects) and leave background intact
        if (depthEnabled)
            theRenderContext.SetDepthTestEnabled(true);
        theRenderContext.SetBlendingEnabled(false);
        // clear color commonly is the layer background, make sure that it is all-zero here
        QT3DSVec4 originalClrColor = theRenderContext.GetClearColor();
        theRenderContext.SetClearColor(QT3DSVec4(0.0));
        theRenderContext.Clear(NVRenderClearValues::Color);
        theRenderContext.SetClearColor(originalClrColor);

    }
    void SLayerRenderData::BlendAdvancedToFB(DefaultMaterialBlendMode::Enum blendMode,
                                             bool depthEnabled, CResourceFrameBuffer *theFB)
    {
        NVRenderContext &theRenderContext(m_Renderer.GetContext());
        NVRenderRect theViewport = m_Renderer.GetQt3DSContext().GetRenderList().GetViewport();
        AdvancedBlendModes::Enum advancedMode;

        switch (blendMode) {
        case DefaultMaterialBlendMode::Overlay:
            advancedMode = AdvancedBlendModes::Overlay;
            break;
        case DefaultMaterialBlendMode::ColorBurn:
            advancedMode = AdvancedBlendModes::ColorBurn;
            break;
        case DefaultMaterialBlendMode::ColorDodge:
            advancedMode = AdvancedBlendModes::ColorDodge;
            break;
        default:
            Q_UNREACHABLE();
        }
        // create blending FBO and texture if not existing
        if (!m_AdvancedModeBlendFB)
            m_AdvancedModeBlendFB = theRenderContext.CreateFrameBuffer();
        if (!m_AdvancedBlendBlendTexture) {
            m_AdvancedBlendBlendTexture = theRenderContext.CreateTexture2D();
            m_AdvancedBlendBlendTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0,
                                                        theViewport.m_Width,
                                                        theViewport.m_Height,
                                                        NVRenderTextureFormats::RGBA8);
            m_AdvancedModeBlendFB->Attach(NVRenderFrameBufferAttachments::Color0,
                                          *m_AdvancedBlendBlendTexture);
        }
        theRenderContext.SetRenderTarget(m_AdvancedModeBlendFB);

        // Blend transparent objects with SW fallback shaders.
        // Disable depth testing as transparent objects have already been
        // depth-checked; here we want to run shader for all layer pixels
        if (depthEnabled)
        {
            theRenderContext.SetDepthTestEnabled(false);
            theRenderContext.SetDepthWriteEnabled(false);
        }
        BlendAdvancedEquationSwFallback(m_AdvancedBlendDrawTexture, m_LayerTexture, advancedMode);
        theRenderContext.SetRenderTarget(*theFB);
        // setup read target
        theRenderContext.SetReadTarget(m_AdvancedModeBlendFB);
        theRenderContext.SetReadBuffer(NVReadFaces::Color0);
        theRenderContext.BlitFramebuffer(0, 0, theViewport.m_Width, theViewport.m_Height,
                                         0, 0, theViewport.m_Width, theViewport.m_Height,
                                         NVRenderClearValues::Color,
                                         NVRenderTextureMagnifyingOp::Nearest);
    }
#endif

    void SLayerRenderData::RenderToViewport()
    {
        if (m_LayerPrepResult->IsLayerVisible()) {
            if (GetOffscreenRenderer()) {
                auto rect = m_Renderer.GetQt3DSContext().GetRenderList().GetViewport();
                SSubPresentationHelper helper(m_Renderer.GetQt3DSContext(),
                                              QSize(rect.m_Width, rect.m_Height));
                if (m_Layer.m_Background == LayerBackground::Color) {
                    m_LastFrameOffscreenRenderer->RenderWithClear(
                        CreateOffscreenRenderEnvironment(), m_Renderer.GetContext(),
                        m_Renderer.GetQt3DSContext().GetPresentationScaleFactor(),
                        SScene::AlwaysClear, m_Layer.m_ClearColor, &m_Layer);
                } else {
                    m_LastFrameOffscreenRenderer->Render(
                        CreateOffscreenRenderEnvironment(), m_Renderer.GetContext(),
                        m_Renderer.GetQt3DSContext().GetPresentationScaleFactor(),
                        SScene::ClearIsOptional, &m_Layer);
                }
            } else {
                RenderDepthPass(false);
                Render();
                RenderRenderWidgets();
            }
        }
    }
    // These are meant to be pixel offsets, so you need to divide them by the width/height
    // of the layer respectively.
    const QT3DSVec2 s_VertexOffsets[SLayerRenderPreparationData::MAX_AA_LEVELS] = {
        QT3DSVec2(-0.170840f, -0.553840f), // 1x
        QT3DSVec2(0.162960f, -0.319340f), // 2x
        QT3DSVec2(0.360260f, -0.245840f), // 3x
        QT3DSVec2(-0.561340f, -0.149540f), // 4x
        QT3DSVec2(0.249460f, 0.453460f), // 5x
        QT3DSVec2(-0.336340f, 0.378260f), // 6x
        QT3DSVec2(0.340000f, 0.166260f), // 7x
        QT3DSVec2(0.235760f, 0.527760f), // 8x
    };

    // Blend factors are in the form of (frame blend factor, accumulator blend factor)
    const QT3DSVec2 s_BlendFactors[SLayerRenderPreparationData::MAX_AA_LEVELS] = {
        QT3DSVec2(0.500000f, 0.500000f), // 1x
        QT3DSVec2(0.333333f, 0.666667f), // 2x
        QT3DSVec2(0.250000f, 0.750000f), // 3x
        QT3DSVec2(0.200000f, 0.800000f), // 4x
        QT3DSVec2(0.166667f, 0.833333f), // 5x
        QT3DSVec2(0.142857f, 0.857143f), // 6x
        QT3DSVec2(0.125000f, 0.875000f), // 7x
        QT3DSVec2(0.111111f, 0.888889f), // 8x
    };

    const QT3DSVec2 s_TemporalVertexOffsets[SLayerRenderPreparationData::MAX_TEMPORAL_AA_LEVELS] = {
        QT3DSVec2(.3f, .3f), QT3DSVec2(-.3f, -.3f)
    };

    static inline void OffsetProjectionMatrix(QT3DSMat44 &inProjectionMatrix, QT3DSVec2 inVertexOffsets)
    {
        inProjectionMatrix.column3.x =
            inProjectionMatrix.column3.x + inProjectionMatrix.column3.w * inVertexOffsets.x;
        inProjectionMatrix.column3.y =
            inProjectionMatrix.column3.y + inProjectionMatrix.column3.w * inVertexOffsets.y;
    }

    CRegisteredString depthPassStr;

    // Render this layer's data to a texture.  Required if we have any effects,
    // prog AA, or if forced.
    void SLayerRenderData::RenderToTexture()
    {
        QT3DS_ASSERT(m_LayerPrepResult->m_Flags.ShouldRenderToTexture());
        SLayerRenderPreparationResult &thePrepResult(*m_LayerPrepResult);
        NVRenderContext &theRenderContext(m_Renderer.GetContext());
        QSize theLayerTextureDimensions = thePrepResult.GetTextureDimensions();
        QSize theLayerOriginalTextureDimensions = theLayerTextureDimensions;
        NVRenderTextureFormats::Enum DepthTextureFormat = NVRenderTextureFormats::Depth24Stencil8;
        NVRenderTextureFormats::Enum ColorTextureFormat = NVRenderTextureFormats::RGBA8;
        if (thePrepResult.m_LastEffect
                && theRenderContext.GetRenderContextType() != NVRenderContextValues::GLES2) {
            if (m_Layer.m_Background != LayerBackground::Transparent)
                ColorTextureFormat = NVRenderTextureFormats::R11G11B10;
            else
                ColorTextureFormat = NVRenderTextureFormats::RGBA16F;
        }
        NVRenderTextureFormats::Enum ColorSSAOTextureFormat = NVRenderTextureFormats::RGBA8;

        bool needsRender = false;
        QT3DSU32 sampleCount = 1;
        // check multsample mode and MSAA texture support
        if (m_Layer.m_MultisampleAAMode != AAModeValues::NoAA
            && theRenderContext.AreMultisampleTexturesSupported())
            sampleCount = (QT3DSU32)m_Layer.m_MultisampleAAMode;

        bool isMultisamplePass = false;
        if (theRenderContext.GetRenderContextType() != NVRenderContextValues::GLES2)
            isMultisamplePass =
                    (sampleCount > 1) || (m_Layer.m_MultisampleAAMode == AAModeValues::SSAA);

        qt3ds::render::NVRenderTextureTargetType::Enum thFboAttachTarget =
            qt3ds::render::NVRenderTextureTargetType::Texture2D;

        // If the user has disabled all layer caching this has the side effect of disabling the
        // progressive AA algorithm.
        if (thePrepResult.m_Flags.WasLayerDataDirty()
                || thePrepResult.m_Flags.WasDirty()
                || m_Renderer.IsLayerCachingEnabled() == false) {
            m_ProgressiveAAPassIndex = 0;
            m_NonDirtyTemporalAAPassIndex = 0;
            needsRender = true;
        }

        CResourceTexture2D *renderColorTexture = &m_LayerTexture;
        CResourceTexture2D *renderPrepassDepthTexture = &m_LayerPrepassDepthTexture;
        CResourceTexture2D *renderWidgetTexture = &m_LayerWidgetTexture;
        NVRenderContextScopedProperty<bool> __multisampleEnabled(
            theRenderContext, &NVRenderContext::IsMultisampleEnabled,
            &NVRenderContext::SetMultisampleEnabled);
        theRenderContext.SetMultisampleEnabled(false);
        if (isMultisamplePass) {
            renderColorTexture = &m_LayerMultisampleTexture;
            renderPrepassDepthTexture = &m_LayerMultisamplePrepassDepthTexture;
            renderWidgetTexture = &m_LayerMultisampleWidgetTexture;
            // for SSAA we don't use MS textures
            if (m_Layer.m_MultisampleAAMode != AAModeValues::SSAA)
                thFboAttachTarget = qt3ds::render::NVRenderTextureTargetType::Texture2D_MS;
        }
        QT3DSU32 maxTemporalPassIndex = m_Layer.m_TemporalAAEnabled ? 2 : 0;

        // If all the dimensions match then we do not have to re-render the layer.
        if (m_LayerTexture.TextureMatches(theLayerTextureDimensions.width(),
                                          theLayerTextureDimensions.height(), ColorTextureFormat)
            && (!thePrepResult.m_Flags.RequiresDepthTexture()
                || m_LayerDepthTexture.TextureMatches(theLayerTextureDimensions.width(),
                                                      theLayerTextureDimensions.height(),
                                                      DepthTextureFormat))
            && m_ProgressiveAAPassIndex >= thePrepResult.m_MaxAAPassIndex
            && m_NonDirtyTemporalAAPassIndex >= maxTemporalPassIndex && needsRender == false) {
            return;
        }

        // adjust render size for SSAA
        if (m_Layer.m_MultisampleAAMode == AAModeValues::SSAA) {
            QT3DSU32 ow, oh;
            CRendererUtil::GetSSAARenderSize(theLayerOriginalTextureDimensions.width(),
                                             theLayerOriginalTextureDimensions.height(),
                                             ow, oh);
            theLayerTextureDimensions = QSize(ow, oh);
        }

        // If our pass index == thePreResult.m_MaxAAPassIndex then
        // we shouldn't get into here.

        IResourceManager &theResourceManager = m_Renderer.GetQt3DSContext().GetResourceManager();
        bool hadLayerTexture = true;

        if (renderColorTexture->EnsureTexture(theLayerTextureDimensions.width(),
                                              theLayerTextureDimensions.height(),
                                              ColorTextureFormat, sampleCount)) {
            m_ProgressiveAAPassIndex = 0;
            m_NonDirtyTemporalAAPassIndex = 0;
            hadLayerTexture = false;
        }

        if (thePrepResult.m_Flags.RequiresDepthTexture()) {
            // The depth texture doesn't need to be multisample, the prepass depth does.
            if (m_LayerDepthTexture.EnsureTexture(theLayerTextureDimensions.width(),
                                                  theLayerTextureDimensions.height(),
                                                  DepthTextureFormat)) {
                // Depth textures are generally not bilinear filtered.
                m_LayerDepthTexture->SetMinFilter(NVRenderTextureMinifyingOp::Nearest);
                m_LayerDepthTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Nearest);
                m_ProgressiveAAPassIndex = 0;
                m_NonDirtyTemporalAAPassIndex = 0;
            }
        }

        if (thePrepResult.m_Flags.RequiresSsaoPass()) {
            if (m_LayerSsaoTexture.EnsureTexture(theLayerTextureDimensions.width(),
                                                 theLayerTextureDimensions.height(),
                                                 ColorSSAOTextureFormat)) {
                m_LayerSsaoTexture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
                m_LayerSsaoTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
                m_ProgressiveAAPassIndex = 0;
                m_NonDirtyTemporalAAPassIndex = 0;
            }
        }

        QT3DS_ASSERT(!thePrepResult.m_Flags.RequiresDepthTexture() || m_LayerDepthTexture);
        QT3DS_ASSERT(!thePrepResult.m_Flags.RequiresSsaoPass() || m_LayerSsaoTexture);

        CResourceTexture2D theLastLayerTexture(theResourceManager);
        SLayerProgAABlendShader *theBlendShader = NULL;
        QT3DSU32 aaFactorIndex = 0;
        bool isProgressiveAABlendPass =
            m_ProgressiveAAPassIndex && m_ProgressiveAAPassIndex < thePrepResult.m_MaxAAPassIndex;
        bool isTemporalAABlendPass = m_Layer.m_TemporalAAEnabled && m_ProgressiveAAPassIndex == 0;

        // Select correct temporal AA texture
        StereoViews::Enum stereoView = m_Renderer.GetQt3DSContext().GetStereoView();
        int temporalIndex = (stereoView == StereoViews::Right) ? 1 : 0;
        CResourceTexture2D &temporalAATexture = m_TemporalAATexture[temporalIndex];

        if (isProgressiveAABlendPass || isTemporalAABlendPass) {
            theBlendShader = m_Renderer.GetLayerProgAABlendShader();
            if (theBlendShader) {
                m_LayerTexture.EnsureTexture(theLayerOriginalTextureDimensions.width(),
                                             theLayerOriginalTextureDimensions.height(),
                                             ColorTextureFormat);
                QT3DSVec2 theVertexOffsets;
                if (isProgressiveAABlendPass) {
                    theLastLayerTexture.StealTexture(m_LayerTexture);
                    aaFactorIndex = (m_ProgressiveAAPassIndex - 1);
                    theVertexOffsets = s_VertexOffsets[aaFactorIndex];
                } else {
                    if (temporalAATexture.GetTexture())
                        theLastLayerTexture.StealTexture(temporalAATexture);
                    else if (hadLayerTexture)
                        theLastLayerTexture.StealTexture(m_LayerTexture);

                    theVertexOffsets = s_TemporalVertexOffsets[m_TemporalAAPassIndex];
                    ++m_TemporalAAPassIndex;
                    ++m_NonDirtyTemporalAAPassIndex;
                    m_TemporalAAPassIndex = m_TemporalAAPassIndex % MAX_TEMPORAL_AA_LEVELS;
                }
                if (theLastLayerTexture.GetTexture()) {
                    theVertexOffsets.x =
                        theVertexOffsets.x / (theLayerOriginalTextureDimensions.width() / 2.0f);
                    theVertexOffsets.y =
                        theVertexOffsets.y / (theLayerOriginalTextureDimensions.height() / 2.0f);
                    // Run through all models and update MVP.
                    // run through all texts and update MVP.
                    // run through all path and update MVP.

                    // TODO - optimize this exact matrix operation.
                    for (QT3DSU32 idx = 0, end = m_ModelContexts.size(); idx < end; ++idx) {
                        QT3DSMat44 &originalProjection(m_ModelContexts[idx]->m_ModelViewProjection);
                        OffsetProjectionMatrix(originalProjection, theVertexOffsets);
                    }
                    for (QT3DSU32 idx = 0, end = m_OpaqueObjects.size(); idx < end; ++idx) {
                        if (m_OpaqueObjects[idx]->m_RenderableFlags.IsPath()) {
                            SPathRenderable &theRenderable =
                                static_cast<SPathRenderable &>(*m_OpaqueObjects[idx]);
                            OffsetProjectionMatrix(theRenderable.m_ModelViewProjection,
                                                   theVertexOffsets);
                        }
                    }
                    for (QT3DSU32 idx = 0, end = m_TransparentObjects.size(); idx < end; ++idx) {
                        if (m_TransparentObjects[idx]->m_RenderableFlags.IsText()) {
                            STextRenderable &theRenderable =
                                static_cast<STextRenderable &>(*m_TransparentObjects[idx]);
                            OffsetProjectionMatrix(theRenderable.m_ModelViewProjection,
                                                   theVertexOffsets);
#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
                        } else if (m_TransparentObjects[idx]->m_RenderableFlags
                                   .isDistanceField()) {
                            SDistanceFieldRenderable &theRenderable
                                    = static_cast<SDistanceFieldRenderable &>(
                                        *m_TransparentObjects[idx]);
                            OffsetProjectionMatrix(theRenderable.m_mvp,
                                                   theVertexOffsets);
#endif
                        } else if (m_TransparentObjects[idx]->m_RenderableFlags.IsPath()) {
                            SPathRenderable &theRenderable =
                                static_cast<SPathRenderable &>(*m_TransparentObjects[idx]);
                            OffsetProjectionMatrix(theRenderable.m_ModelViewProjection,
                                                   theVertexOffsets);
                        }
                    }
                }
            }
        }
        if (theLastLayerTexture.GetTexture() == NULL) {
            isProgressiveAABlendPass = false;
            isTemporalAABlendPass = false;
        }
        // Sometimes we will have stolen the render texture.
        renderColorTexture->EnsureTexture(theLayerTextureDimensions.width(),
                                          theLayerTextureDimensions.height(), ColorTextureFormat,
                                          sampleCount);

        if (!isTemporalAABlendPass)
            temporalAATexture.ReleaseTexture();

        // Allocating a frame buffer can cause it to be bound, so we need to save state before this
        // happens.
        NVRenderContextScopedProperty<NVRenderFrameBuffer *> __framebuf(
            theRenderContext, &NVRenderContext::GetRenderTarget, &NVRenderContext::SetRenderTarget);
        // Match the bit depth of the current render target to avoid popping when we switch from aa
        // to non aa layers
        // We have to all this here in because once we change the FB by allocating an FB we are
        // screwed.
        NVRenderTextureFormats::Enum theDepthFormat(GetDepthBufferFormat());
        NVRenderFrameBufferAttachments::Enum theDepthAttachmentFormat(
            GetFramebufferDepthAttachmentFormat(theDepthFormat));

        // Definitely disable the scissor rect if it is running right now.
        NVRenderContextScopedProperty<bool> __scissorEnabled(
            theRenderContext, &NVRenderContext::IsScissorTestEnabled,
            &NVRenderContext::SetScissorTestEnabled, false);
        CResourceFrameBuffer theFB(theResourceManager);
        // Allocates the frame buffer which has the side effect of setting the current render target
        // to that frame buffer.
        theFB.EnsureFrameBuffer();

        bool hasDepthObjects = m_OpaqueObjects.size() > 0 || m_TransparentObjects.size() > 0;
        bool requiresDepthStencilBuffer =
            hasDepthObjects || thePrepResult.m_Flags.RequiresStencilBuffer();
        NVRenderRect theNewViewport(0, 0, theLayerTextureDimensions.width(),
                                    theLayerTextureDimensions.height());
        {
            theRenderContext.SetRenderTarget(theFB);
            NVRenderContextScopedProperty<NVRenderRect> __viewport(
                theRenderContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport,
                theNewViewport);
            QT3DSVec4 clearColor(0.0f);
            if (m_Layer.m_Background == LayerBackground::Color) {
                clearColor = m_Layer.m_ClearColor;
                // Framebuffer holds premultiplied colors so the clearcolor must be premultiplied
                if (clearColor.w < 1.0f) {
                    clearColor.x *= clearColor.w;
                    clearColor.y *= clearColor.w;
                    clearColor.z *= clearColor.w;
                }
            }

            NVRenderContextScopedProperty<QT3DSVec4> __clearColor(
                theRenderContext, &NVRenderContext::GetClearColor, &NVRenderContext::SetClearColor,
                clearColor);
            if (requiresDepthStencilBuffer) {
                if (renderPrepassDepthTexture->EnsureTexture(theLayerTextureDimensions.width(),
                                                             theLayerTextureDimensions.height(),
                                                             theDepthFormat, sampleCount)) {
                    (*renderPrepassDepthTexture)->SetMinFilter(NVRenderTextureMinifyingOp::Nearest);
                    (*renderPrepassDepthTexture)
                        ->SetMagFilter(NVRenderTextureMagnifyingOp::Nearest);
                }
            }

            if (thePrepResult.m_Flags.RequiresDepthTexture() && m_ProgressiveAAPassIndex == 0) {
                // Setup FBO with single depth buffer target.
                // Note this does not use multisample.
                NVRenderFrameBufferAttachments::Enum theAttachment =
                    GetFramebufferDepthAttachmentFormat(DepthTextureFormat);
                theFB->Attach(theAttachment, *m_LayerDepthTexture);

                // In this case transparent objects also may write their depth.
                RenderDepthPass(true);
                theFB->Attach(theAttachment, NVRenderTextureOrRenderBuffer());
            }

            if (thePrepResult.m_Flags.RequiresSsaoPass() && m_ProgressiveAAPassIndex == 0
                    && m_Camera != nullptr) {
                StartProfiling("AO pass", false);
                // Setup FBO with single color buffer target
                theFB->Attach(NVRenderFrameBufferAttachments::Color0, *m_LayerSsaoTexture);
                theRenderContext.Clear(qt3ds::render::NVRenderClearValues::Color);
                RenderAoPass();
                theFB->Attach(NVRenderFrameBufferAttachments::Color0,
                              NVRenderTextureOrRenderBuffer());
                EndProfiling("AO pass");
            }

            if (thePrepResult.m_Flags.RequiresShadowMapPass() && m_ProgressiveAAPassIndex == 0) {
                // shadow map path
                StartProfiling("Shadow pass", false);
                RenderShadowMapPass(&theFB);
                EndProfiling("Shadow pass");
            }

            if (sampleCount > 1) {
                theRenderContext.SetMultisampleEnabled(true);
            }

            qt3ds::render::NVRenderClearFlags clearFlags = qt3ds::render::NVRenderClearValues::Color;

            // render depth prepass
            if ((*renderPrepassDepthTexture)) {
                theFB->Attach(theDepthAttachmentFormat, **renderPrepassDepthTexture,
                              thFboAttachTarget);

                if (m_Layer.m_Flags.IsLayerEnableDepthPrepass()) {
                    StartProfiling("Depth pass", false);
                    RenderDepthPass(false);
                    EndProfiling("Depth pass");
                } else {
                    clearFlags |= (qt3ds::render::NVRenderClearValues::Depth);
                    clearFlags |= (qt3ds::render::NVRenderClearValues::Stencil);
                    // enable depth write for the clear below
                    theRenderContext.SetDepthWriteEnabled(true);
                }
            }

            theFB->Attach(NVRenderFrameBufferAttachments::Color0, **renderColorTexture,
                          thFboAttachTarget);
            if (m_Layer.m_Background != LayerBackground::Unspecified) {
                theRenderContext.SetClearColor(clearColor);
                theRenderContext.Clear(clearFlags);
            }

            // We don't clear the depth buffer because the layer render code we are about to call
            // will do this.
            StartProfiling("Render pass", false);
            Render(&theFB);
            // Debug measure to view the depth map to ensure we're rendering it correctly.
            //if (m_Layer.m_TemporalAAEnabled) {
            //    RenderFakeDepthMapPass(m_ShadowMapManager->GetShadowMapEntry(0)->m_DepthMap,
            //                           m_ShadowMapManager->GetShadowMapEntry(0)->m_DepthCube);
            //}
            EndProfiling("Render pass");

            // Now before going further, we downsample and resolve the multisample information.
            // This allows all algorithms running after
            // this point to run unchanged.
            if (isMultisamplePass) {
                if (m_Layer.m_MultisampleAAMode != AAModeValues::SSAA) {
                    // Resolve the FBO to the layer texture
                    CRendererUtil::ResolveMutisampleFBOColorOnly(
                        theResourceManager, m_LayerTexture, theRenderContext,
                        theLayerTextureDimensions.width(), theLayerTextureDimensions.height(),
                        ColorTextureFormat, *theFB);

                    theRenderContext.SetMultisampleEnabled(false);
                } else {
                    // Resolve the FBO to the layer texture
                    CRendererUtil::ResolveSSAAFBOColorOnly(
                        theResourceManager, m_LayerTexture,
                        theLayerOriginalTextureDimensions.width(),
                        theLayerOriginalTextureDimensions.height(), theRenderContext,
                        theLayerTextureDimensions.width(), theLayerTextureDimensions.height(),
                        ColorTextureFormat, *theFB);
                }
            }

            // CN - when I tried to get anti-aliased widgets I lost all transparency on the widget
            // layer which made it overwrite the object you were
            // manipulating.  When I tried to use parallel nsight on it the entire studio
            // application crashed on startup.
            if (NeedsWidgetTexture()) {
                m_LayerWidgetTexture.EnsureTexture(theLayerTextureDimensions.width(),
                                                   theLayerTextureDimensions.height(),
                                                   NVRenderTextureFormats::RGBA8);
                theRenderContext.SetRenderTarget(theFB);
                theFB->Attach(NVRenderFrameBufferAttachments::Color0, *m_LayerWidgetTexture);
                theFB->Attach(GetFramebufferDepthAttachmentFormat(DepthTextureFormat),
                              *m_LayerDepthTexture);
                theRenderContext.SetClearColor(QT3DSVec4(0.0f));
                theRenderContext.Clear(qt3ds::render::NVRenderClearValues::Color);
                // We should already have the viewport and everything setup for this.
                RenderRenderWidgets();
            }

            if (theLastLayerTexture.GetTexture() != NULL
                && (isProgressiveAABlendPass || isTemporalAABlendPass)) {
                theRenderContext.SetViewport(
                    NVRenderRect(0, 0, theLayerOriginalTextureDimensions.width(),
                                 theLayerOriginalTextureDimensions.height()));
                CResourceTexture2D targetTexture(
                    theResourceManager, theLayerOriginalTextureDimensions.width(),
                    theLayerOriginalTextureDimensions.height(), ColorTextureFormat);
                theFB->Attach(theDepthAttachmentFormat,
                              NVRenderTextureOrRenderBuffer());
                theFB->Attach(NVRenderFrameBufferAttachments::Color0, *targetTexture);
                QT3DSVec2 theBlendFactors;
                if (isProgressiveAABlendPass)
                    theBlendFactors = s_BlendFactors[aaFactorIndex];
                else
                    theBlendFactors = QT3DSVec2(.5f, .5f);

                theRenderContext.SetDepthTestEnabled(false);
                theRenderContext.SetBlendingEnabled(false);
                theRenderContext.SetCullingEnabled(false);
                theRenderContext.SetActiveShader(theBlendShader->m_Shader);
                theBlendShader->m_AccumSampler.Set(theLastLayerTexture);
                theBlendShader->m_LastFrame.Set(m_LayerTexture);
                theBlendShader->m_BlendFactors.Set(theBlendFactors);
                m_Renderer.RenderQuad();
                theFB->Attach(NVRenderFrameBufferAttachments::Color0,
                              qt3ds::render::NVRenderTextureOrRenderBuffer());
                if (isTemporalAABlendPass)
                    temporalAATexture.StealTexture(m_LayerTexture);
                m_LayerTexture.StealTexture(targetTexture);
            }

            m_LayerTexture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
            m_LayerTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);

            // Don't remember why needs widget texture is false here.
            // Should have commented why progAA plus widgets is a fail.
            if (m_ProgressiveAAPassIndex < thePrepResult.m_MaxAAPassIndex
                && NeedsWidgetTexture() == false)
                ++m_ProgressiveAAPassIndex;

// now we render all post effects
#ifdef QT3DS_CACHED_POST_EFFECT
            ApplyLayerPostEffects();
#endif

            if (m_LayerPrepassDepthTexture) {
                // Detach any depth buffers.
                theFB->Attach(theDepthAttachmentFormat, qt3ds::render::NVRenderTextureOrRenderBuffer(),
                              thFboAttachTarget);
            }

            theFB->Attach(NVRenderFrameBufferAttachments::Color0,
                          qt3ds::render::NVRenderTextureOrRenderBuffer(), thFboAttachTarget);
            // Let natural scoping rules destroy the other stuff.
        }
    }

    void SLayerRenderData::ApplyLayerPostEffects()
    {
        bool effectsActive = false;
        SEffect *lastEffect = nullptr;
        // Check if effects are active and get last active effect
        for (SEffect *theEffect = m_Layer.m_FirstEffect; theEffect;
             theEffect = theEffect->m_NextEffect) {
            if (theEffect->m_Flags.IsActive()) {
                effectsActive = true;
                lastEffect = theEffect;
            }
        }
        if (!effectsActive || !m_Camera) {
            if (m_LayerCachedTexture) {
                IResourceManager &theResourceManager(m_Renderer.GetQt3DSContext().GetResourceManager());
                theResourceManager.Release(*m_LayerCachedTexture);
                m_LayerCachedTexture = NULL;
            }
            return;
        }

        IEffectSystem &theEffectSystem(m_Renderer.GetQt3DSContext().GetEffectSystem());
        IResourceManager &theResourceManager(m_Renderer.GetQt3DSContext().GetResourceManager());
        // we use the non MSAA buffer for the effect
        NVRenderTexture2D *theLayerColorTexture = m_LayerTexture;
        NVRenderTexture2D *theLayerDepthTexture = m_LayerDepthTexture;

        if (!m_LayerCachedTexture) {
            STextureDetails details(theLayerColorTexture->GetTextureDetails());
            QT3DSU32 finalWidth = ITextRenderer::NextMultipleOf4((QT3DSU32)(details.m_Width));
            QT3DSU32 finalHeight = ITextRenderer::NextMultipleOf4((QT3DSU32)(details.m_Height));
            m_LayerCachedTexture = theResourceManager.AllocateTexture2D(finalWidth, finalHeight,
                                                                        details.m_Format);
        }

        NVRenderTexture2D *theCurrentTexture = theLayerColorTexture;
        for (SEffect *theEffect = m_Layer.m_FirstEffect; theEffect;
             theEffect = theEffect->m_NextEffect) {
            if (theEffect->m_Flags.IsActive()) {
                NVRenderTexture2D *targetTexture = nullptr;
                if (theEffect == lastEffect)
                    targetTexture = m_LayerCachedTexture;

                StartProfiling(theEffect->m_ClassName, false);

                NVRenderTexture2D *theRenderedEffect = theEffectSystem.RenderEffect(
                    SEffectRenderArgument(*theEffect, *theCurrentTexture,
                                          QT3DSVec2(m_Camera->m_ClipNear, m_Camera->m_ClipFar),
                                          theLayerDepthTexture, m_LayerPrepassDepthTexture,
                                          targetTexture));

                EndProfiling(theEffect->m_ClassName);

                // If the texture came from rendering a chain of effects, then we don't need it
                // after this.
                if (theCurrentTexture != theLayerColorTexture)
                    theResourceManager.Release(*theCurrentTexture);

                theCurrentTexture = theRenderedEffect;

                if (!theRenderedEffect) {
                    QString errorMsg = QObject::tr("Failed to compile \"%1\" effect.\nConsider"
                                                   " removing it from the presentation.")
                            .arg(theEffect->m_ClassName.c_str());
                    QT3DS_ALWAYS_ASSERT_MESSAGE(errorMsg.toUtf8());
                    break;
                }
            }
        }
    }

    inline bool AnyCompletelyNonTransparentObjects(TRenderableObjectList &inObjects)
    {
        for (QT3DSU32 idx = 0, end = inObjects.size(); idx < end; ++idx) {
            if (inObjects[idx]->m_RenderableFlags.IsCompletelyTransparent() == false)
                return true;
        }
        return false;
    }

    void SLayerRenderData::RunnableRenderToViewport(qt3ds::render::NVRenderFrameBuffer *theFB)
    {
        // If we have an effect, an opaque object, or any transparent objects that aren't completely
        // transparent
        // or an offscreen renderer or a layer widget texture
        // Then we can't possible affect the resulting render target.
        bool needsToRender = m_Layer.m_FirstEffect != NULL || m_OpaqueObjects.empty() == false
            || AnyCompletelyNonTransparentObjects(m_TransparentObjects) || GetOffscreenRenderer()
            || m_LayerWidgetTexture || m_BoundingRectColor.hasValue()
            || m_Layer.m_Background == LayerBackground::Color;

        if (needsToRender == false)
            return;

        NVRenderContext &theContext(m_Renderer.GetContext());
        theContext.resetStates();

        NVRenderContextScopedProperty<NVRenderFrameBuffer *> __fbo(
            theContext, &NVRenderContext::GetRenderTarget, &NVRenderContext::SetRenderTarget);
        qt3ds::render::NVRenderRect theCurrentViewport = theContext.GetViewport();
        NVRenderContextScopedProperty<NVRenderRect> __viewport(
            theContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport);
        NVRenderContextScopedProperty<bool> theScissorEnabled(
            theContext, &NVRenderContext::IsScissorTestEnabled,
            &NVRenderContext::SetScissorTestEnabled);
        NVRenderContextScopedProperty<qt3ds::render::NVRenderRect> theScissorRect(
            theContext, &NVRenderContext::GetScissorRect, &NVRenderContext::SetScissorRect);
        SLayerRenderPreparationResult &thePrepResult(*m_LayerPrepResult);
        NVRenderRectF theScreenRect(thePrepResult.GetLayerToPresentationViewport());

        bool blendingEnabled = m_Layer.m_Background != LayerBackground::Unspecified;
        if (!thePrepResult.m_Flags.ShouldRenderToTexture()) {
            theContext.SetViewport(
                m_LayerPrepResult->GetLayerToPresentationViewport().ToIntegerRect());
            theContext.SetScissorTestEnabled(true);
            theContext.SetScissorRect(
                m_LayerPrepResult->GetLayerToPresentationScissorRect().ToIntegerRect());
            if (m_Layer.m_Background == LayerBackground::Color) {
                QT3DSVec4 clearColor = m_Layer.m_ClearColor;
                // Framebuffer holds premultiplied colors so the clearcolor must be premultiplied
                if (clearColor.w < 1.0f) {
                    clearColor.x *= clearColor.w;
                    clearColor.y *= clearColor.w;
                    clearColor.z *= clearColor.w;
                }
                NVRenderContextScopedProperty<QT3DSVec4> __clearColor(
                    theContext, &NVRenderContext::GetClearColor, &NVRenderContext::SetClearColor,
                    clearColor);
                theContext.Clear(NVRenderClearValues::Color);
            }
            RenderToViewport();
        } else {
// First, render the layer along with whatever progressive AA is appropriate.
// The render graph should have taken care of the render to texture step.
#ifdef QT3DS_CACHED_POST_EFFECT
            NVRenderTexture2D *theLayerColorTexture =
                (m_LayerCachedTexture) ? m_LayerCachedTexture : m_LayerTexture;
#else
            // Then render all but the last effect
            IEffectSystem &theEffectSystem(m_Renderer.GetQt3DSContext().GetEffectSystem());
            IResourceManager &theResourceManager(m_Renderer.GetQt3DSContext().GetResourceManager());
            // we use the non MSAA buffer for the effect
            NVRenderTexture2D *theLayerColorTexture = m_LayerTexture;
            NVRenderTexture2D *theLayerDepthTexture = m_LayerDepthTexture;

            NVRenderTexture2D *theCurrentTexture = theLayerColorTexture;
            for (SEffect *theEffect = m_Layer.m_FirstEffect;
                 theEffect && theEffect != thePrepResult.m_LastEffect;
                 theEffect = theEffect->m_NextEffect) {
                if (theEffect->m_Flags.IsActive() && m_Camera) {
                    StartProfiling(theEffect->m_ClassName, false);

                    NVRenderTexture2D *theRenderedEffect = theEffectSystem.RenderEffect(
                        SEffectRenderArgument(*theEffect, *theCurrentTexture,
                                              QT3DSVec2(m_Camera->m_ClipNear, m_Camera->m_ClipFar),
                                              theLayerDepthTexture, m_LayerPrepassDepthTexture));

                    EndProfiling(theEffect->m_ClassName);

                    // If the texture came from rendering a chain of effects, then we don't need it
                    // after this.
                    if (theCurrentTexture != theLayerColorTexture)
                        theResourceManager.Release(*theCurrentTexture);

                    theCurrentTexture = theRenderedEffect;
                }
            }
#endif
            // Now the last effect or straight to the scene if we have no last effect
            // There are two cases we need to consider here.  The first is when we shouldn't
            // transform
            // the result and thus we need to setup an MVP that just maps to the viewport width and
            // height.
            // The second is when we are expected to render to the scene using some global
            // transform.
            QT3DSMat44 theFinalMVP(QT3DSMat44::createIdentity());
            SCamera theTempCamera;
            NVRenderRect theLayerViewport(
                thePrepResult.GetLayerToPresentationViewport().ToIntegerRect());
            NVRenderRect theLayerClip(
                thePrepResult.GetLayerToPresentationScissorRect().ToIntegerRect());

            {
                QT3DSMat33 ignored;
                QT3DSMat44 theViewProjection;
                // We could cache these variables
                theTempCamera.m_Flags.SetOrthographic(true);
                theTempCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
                // Move the camera back far enough that we can see everything
                QT3DSF32 theCameraSetback(10);
                // Attempt to ensure the layer can never be clipped.
                theTempCamera.m_Position.z = -theCameraSetback;
                theTempCamera.m_ClipFar = 2.0f * theCameraSetback;
                // Render the layer texture to the entire viewport.
                SCameraGlobalCalculationResult theResult = theTempCamera.CalculateGlobalVariables(
                    theLayerViewport,
                    QT3DSVec2((QT3DSF32)theLayerViewport.m_Width, (QT3DSF32)theLayerViewport.m_Height));
                theTempCamera.CalculateViewProjectionMatrix(theViewProjection);
                SNode theTempNode;
                theFinalMVP = theViewProjection;
                qt3ds::render::NVRenderBlendFunctionArgument blendFunc;
                qt3ds::render::NVRenderBlendEquationArgument blendEqu;

                switch (m_Layer.m_BlendType) {
                case LayerBlendTypes::Screen:
                    blendFunc = qt3ds::render::NVRenderBlendFunctionArgument(
                        NVRenderSrcBlendFunc::SrcAlpha, NVRenderDstBlendFunc::One,
                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::One);
                    blendEqu = qt3ds::render::NVRenderBlendEquationArgument(
                        NVRenderBlendEquation::Add, NVRenderBlendEquation::Add);
                    break;
                case LayerBlendTypes::Multiply:
                    blendFunc = qt3ds::render::NVRenderBlendFunctionArgument(
                        NVRenderSrcBlendFunc::DstColor, NVRenderDstBlendFunc::Zero,
                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::One);
                    blendEqu = qt3ds::render::NVRenderBlendEquationArgument(
                        NVRenderBlendEquation::Add, NVRenderBlendEquation::Add);
                    break;
                case LayerBlendTypes::Add:
                    blendFunc = qt3ds::render::NVRenderBlendFunctionArgument(
                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::One,
                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::One);
                    blendEqu = qt3ds::render::NVRenderBlendEquationArgument(
                        NVRenderBlendEquation::Add, NVRenderBlendEquation::Add);
                    break;
                case LayerBlendTypes::Subtract:
                    blendFunc = qt3ds::render::NVRenderBlendFunctionArgument(
                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::One,
                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::One);
                    blendEqu = qt3ds::render::NVRenderBlendEquationArgument(
                        NVRenderBlendEquation::ReverseSubtract,
                        NVRenderBlendEquation::ReverseSubtract);
                    break;
                case LayerBlendTypes::Overlay:
                    // SW fallback doesn't use blend equation
                    // note blend func is not used here anymore
                    if (theContext.IsAdvancedBlendHwSupported() ||
                            theContext.IsAdvancedBlendHwSupportedKHR())
                        blendEqu = qt3ds::render::NVRenderBlendEquationArgument(
                            NVRenderBlendEquation::Overlay, NVRenderBlendEquation::Overlay);
                    break;
                case LayerBlendTypes::ColorBurn:
                    // SW fallback doesn't use blend equation
                    // note blend func is not used here anymore
                    if (theContext.IsAdvancedBlendHwSupported() ||
                            theContext.IsAdvancedBlendHwSupportedKHR())
                        blendEqu = qt3ds::render::NVRenderBlendEquationArgument(
                            NVRenderBlendEquation::ColorBurn, NVRenderBlendEquation::ColorBurn);
                    break;
                case LayerBlendTypes::ColorDodge:
                    // SW fallback doesn't use blend equation
                    // note blend func is not used here anymore
                    if (theContext.IsAdvancedBlendHwSupported() ||
                            theContext.IsAdvancedBlendHwSupportedKHR())
                        blendEqu = qt3ds::render::NVRenderBlendEquationArgument(
                            NVRenderBlendEquation::ColorDodge, NVRenderBlendEquation::ColorDodge);
                    break;
                default:
                    blendFunc = qt3ds::render::NVRenderBlendFunctionArgument(
                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha,
                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha);
                    blendEqu = qt3ds::render::NVRenderBlendEquationArgument(
                        NVRenderBlendEquation::Add, NVRenderBlendEquation::Add);
                    break;
                }
                theContext.SetBlendFunction(blendFunc);
                theContext.SetBlendEquation(blendEqu);
                theContext.SetBlendingEnabled(blendingEnabled);
                theContext.SetDepthTestEnabled(false);
            }

            {
                theContext.SetScissorTestEnabled(true);
                theContext.SetViewport(theLayerViewport);
                theContext.SetScissorRect(theLayerClip);

                // Remember the camera we used so we can get a valid pick ray
                m_SceneCamera = theTempCamera;
                theContext.SetDepthTestEnabled(false);
#ifndef QT3DS_CACHED_POST_EFFECT
                if (thePrepResult.m_LastEffect && m_Camera) {
                    StartProfiling(thePrepResult.m_LastEffect->m_ClassName, false);
                    // inUseLayerMPV is true then we are rendering directly to the scene and thus we
                    // should enable blending
                    // for the final render pass.  Else we should leave it.
                    theEffectSystem.RenderEffect(
                        SEffectRenderArgument(*thePrepResult.m_LastEffect, *theCurrentTexture,
                                              QT3DSVec2(m_Camera->m_ClipNear, m_Camera->m_ClipFar),
                                              theLayerDepthTexture, m_LayerPrepassDepthTexture),
                        theFinalMVP, blendingEnabled);
                    EndProfiling(thePrepResult.m_LastEffect->m_ClassName);
                    // If the texture came from rendering a chain of effects, then we don't need it
                    // after this.
                    if (theCurrentTexture != theLayerColorTexture)
                        theResourceManager.Release(*theCurrentTexture);
                } else
#endif
                {
                    theContext.SetCullingEnabled(false);
                    theContext.SetBlendingEnabled(blendingEnabled);
                    theContext.SetDepthTestEnabled(false);
#ifdef ADVANCED_BLEND_SW_FALLBACK
                    NVScopedRefCounted<NVRenderTexture2D> screenTexture =
                            m_Renderer.GetLayerBlendTexture();
                    NVScopedRefCounted<NVRenderFrameBuffer> blendFB = m_Renderer.GetBlendFB();

                    // Layer blending for advanced blending modes if SW fallback is needed
                    // rendering to FBO and blending with separate shader
                    if (screenTexture) {
                        // Blending is enabled only if layer background has been chosen transparent
                        // Layers with advanced blending modes
                        if (blendingEnabled && (m_Layer.m_BlendType == LayerBlendTypes::Overlay ||
                                                m_Layer.m_BlendType == LayerBlendTypes::ColorBurn ||
                                                m_Layer.m_BlendType == LayerBlendTypes::ColorDodge)) {
                            theContext.SetScissorTestEnabled(false);
                            theContext.SetBlendingEnabled(false);

                            // Get part matching to layer from screen texture and
                            // use that for blending
                            qt3ds::render::NVRenderTexture2D *blendBlitTexture;
                            blendBlitTexture = theContext.CreateTexture2D();
                            blendBlitTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0,
                                                             theLayerViewport.m_Width,
                                                             theLayerViewport.m_Height,
                                                             NVRenderTextureFormats::RGBA8);
                            qt3ds::render::NVRenderFrameBuffer *blitFB;
                            blitFB = theContext.CreateFrameBuffer();
                            blitFB->Attach(NVRenderFrameBufferAttachments::Color0,
                                           NVRenderTextureOrRenderBuffer(*blendBlitTexture));
                            blendFB->Attach(NVRenderFrameBufferAttachments::Color0,
                                            NVRenderTextureOrRenderBuffer(*screenTexture));
                            theContext.SetRenderTarget(blitFB);
                            theContext.SetReadTarget(blendFB);
                            theContext.SetReadBuffer(NVReadFaces::Color0);
                            theContext.BlitFramebuffer(theLayerViewport.m_X, theLayerViewport.m_Y,
                                                       theLayerViewport.m_Width +
                                                       theLayerViewport.m_X,
                                                       theLayerViewport.m_Height +
                                                       theLayerViewport.m_Y,
                                                       0, 0,
                                                       theLayerViewport.m_Width,
                                                       theLayerViewport.m_Height,
                                                       NVRenderClearValues::Color,
                                                       NVRenderTextureMagnifyingOp::Nearest);

                            qt3ds::render::NVRenderTexture2D *blendResultTexture;
                            blendResultTexture = theContext.CreateTexture2D();
                            blendResultTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0,
                                                               theLayerViewport.m_Width,
                                                               theLayerViewport.m_Height,
                                                               NVRenderTextureFormats::RGBA8);
                            qt3ds::render::NVRenderFrameBuffer *resultFB;
                            resultFB = theContext.CreateFrameBuffer();
                            resultFB->Attach(NVRenderFrameBufferAttachments::Color0,
                                             NVRenderTextureOrRenderBuffer(*blendResultTexture));
                            theContext.SetRenderTarget(resultFB);

                            AdvancedBlendModes::Enum advancedMode;
                            switch (m_Layer.m_BlendType) {
                            case LayerBlendTypes::Overlay:
                                advancedMode = AdvancedBlendModes::Overlay;
                                break;
                            case LayerBlendTypes::ColorBurn:
                                advancedMode = AdvancedBlendModes::ColorBurn;
                                break;
                            case LayerBlendTypes::ColorDodge:
                                advancedMode = AdvancedBlendModes::ColorDodge;
                                break;
                            default:
                                advancedMode = AdvancedBlendModes::None;
                                break;
                            }

                            theContext.SetViewport(NVRenderRect(0, 0, theLayerViewport.m_Width,
                                                                theLayerViewport.m_Height));
                            BlendAdvancedEquationSwFallback(theLayerColorTexture, blendBlitTexture,
                                                            advancedMode);
                            blitFB->release();
                            // save blending result to screen texture for use with other layers
                            theContext.SetViewport(theLayerViewport);
                            theContext.SetRenderTarget(blendFB);
                            m_Renderer.RenderQuad(QT3DSVec2((QT3DSF32)theLayerViewport.m_Width,
                                                            (QT3DSF32)theLayerViewport.m_Height),
                                                  theFinalMVP, *blendResultTexture);
                            // render the blended result
                            theContext.SetRenderTarget(theFB);
                            theContext.SetScissorTestEnabled(true);
                            m_Renderer.RenderQuad(QT3DSVec2((QT3DSF32)theLayerViewport.m_Width,
                                                            (QT3DSF32)theLayerViewport.m_Height),
                                                  theFinalMVP, *blendResultTexture);
                            resultFB->release();
                        } else {
                            // Layers with normal blending modes
                            // save result for future use
                            theContext.SetViewport(theLayerViewport);
                            theContext.SetScissorTestEnabled(false);
                            theContext.SetBlendingEnabled(true);
                            theContext.SetRenderTarget(blendFB);
                            m_Renderer.RenderQuad(QT3DSVec2((QT3DSF32)theLayerViewport.m_Width,
                                                            (QT3DSF32)theLayerViewport.m_Height),
                                                  theFinalMVP, *theLayerColorTexture);
                            theContext.SetRenderTarget(theFB);
                            theContext.SetScissorTestEnabled(true);
                            theContext.SetViewport(theCurrentViewport);
                            m_Renderer.RenderQuad(QT3DSVec2((QT3DSF32)theLayerViewport.m_Width,
                                                            (QT3DSF32)theLayerViewport.m_Height),
                                                  theFinalMVP, *theLayerColorTexture);
                        }
                    } else {
                        // No advanced blending SW fallback needed
                        m_Renderer.RenderQuad(QT3DSVec2((QT3DSF32)theLayerViewport.m_Width,
                                                        (QT3DSF32)theLayerViewport.m_Height),
                                              theFinalMVP, *theLayerColorTexture);
                    }
#else
                    m_Renderer.RenderQuad(QT3DSVec2((QT3DSF32)theLayerViewport.m_Width,
                                                    (QT3DSF32)theLayerViewport.m_Height),
                                          theFinalMVP, *theLayerColorTexture);
#endif
                }
                if (m_LayerWidgetTexture) {
                    theContext.SetBlendingEnabled(false);
                    m_Renderer.SetupWidgetLayer();
                    SLayerRenderPreparationResult &thePrepResult(*m_LayerPrepResult);
                    NVRenderRectF thePresRect(thePrepResult.GetPresentationViewport());
                    NVRenderRectF theLayerRect(thePrepResult.GetLayerToPresentationViewport());

                    // Ensure we remove any offsetting in the layer rect that was caused simply by
                    // the
                    // presentation rect offsetting but then use a new rect.
                    NVRenderRectF theWidgetLayerRect(theLayerRect.m_X - thePresRect.m_X,
                                                     theLayerRect.m_Y - thePresRect.m_Y,
                                                     theLayerRect.m_Width, theLayerRect.m_Height);
                    theContext.SetScissorTestEnabled(false);
                    theContext.SetViewport(theWidgetLayerRect.ToIntegerRect());
                    m_Renderer.RenderQuad(
                        QT3DSVec2((QT3DSF32)theLayerViewport.m_Width, (QT3DSF32)theLayerViewport.m_Height),
                        theFinalMVP, *m_LayerWidgetTexture);
                }
            }
        } // End offscreen render code.

        if (m_BoundingRectColor.hasValue()) {
            NVRenderContextScopedProperty<NVRenderRect> __viewport(
                theContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport);
            NVRenderContextScopedProperty<bool> theScissorEnabled(
                theContext, &NVRenderContext::IsScissorTestEnabled,
                &NVRenderContext::SetScissorTestEnabled);
            NVRenderContextScopedProperty<qt3ds::render::NVRenderRect> theScissorRect(
                theContext, &NVRenderContext::GetScissorRect, &NVRenderContext::SetScissorRect);
            m_Renderer.SetupWidgetLayer();
            // Setup a simple viewport to render to the entire presentation viewport.
            theContext.SetViewport(
                NVRenderRect(0, 0, (QT3DSU32)thePrepResult.GetPresentationViewport().m_Width,
                             (QT3DSU32)thePrepResult.GetPresentationViewport().m_Height));

            NVRenderRectF thePresRect(thePrepResult.GetPresentationViewport());

            // Remove any offsetting from the presentation rect since the widget layer is a
            // stand-alone fbo.
            NVRenderRectF theWidgetScreenRect(theScreenRect.m_X - thePresRect.m_X,
                                              theScreenRect.m_Y - thePresRect.m_Y,
                                              theScreenRect.m_Width, theScreenRect.m_Height);
            theContext.SetScissorTestEnabled(false);
            m_Renderer.DrawScreenRect(theWidgetScreenRect, *m_BoundingRectColor);
        }
        theContext.SetBlendFunction(qt3ds::render::NVRenderBlendFunctionArgument(
                                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha,
                                        NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha));
        theContext.SetBlendEquation(qt3ds::render::NVRenderBlendEquationArgument(
                                        NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));
    }

#define RENDER_FRAME_NEW(type) QT3DS_NEW(m_Renderer.GetPerFrameAllocator(), type)

    void SLayerRenderData::AddLayerRenderStep()
    {
        QT3DS_PERF_SCOPED_TIMER(m_Renderer.GetQt3DSContext().GetPerfTimer(),
                                "SLayerRenderData::AddLayerRenderStep")
        QT3DS_ASSERT(m_Camera);
        if (!m_Camera)
            return;

        IRenderList &theGraph(m_Renderer.GetQt3DSContext().GetRenderList());

        qt3ds::render::NVRenderRect theCurrentViewport = theGraph.GetViewport();
        if (!m_LayerPrepResult.hasValue())
            PrepareForRender(
                QSize(theCurrentViewport.m_Width, theCurrentViewport.m_Height));
    }

    void SLayerRenderData::PrepareForRender()
    {
        // When we render to the scene itself (as opposed to an offscreen buffer somewhere)
        // then we use the MVP of the layer somewhat.
        NVRenderRect theViewport = m_Renderer.GetQt3DSContext().GetRenderList().GetViewport();
        PrepareForRender(
            QSize((QT3DSU32)theViewport.m_Width, (QT3DSU32)theViewport.m_Height));
    }

    void SLayerRenderData::ResetForFrame()
    {
        SLayerRenderPreparationData::ResetForFrame();
        m_BoundingRectColor.setEmpty();
    }

    void SLayerRenderData::PrepareAndRender(const QT3DSMat44 &inViewProjection)
    {
        TRenderableObjectList theTransparentObjects(m_TransparentObjects);
        TRenderableObjectList theOpaqueObjects(m_OpaqueObjects);
        theTransparentObjects.clear();
        theOpaqueObjects.clear();
        m_ModelContexts.clear();
        SLayerRenderPreparationResultFlags theFlags;
        PrepareRenderablesForRender(inViewProjection, Empty(), 1.0, theFlags);
        RenderDepthPass(false);
        Render();
    }

    struct SLayerRenderToTextureRunnable : public IRenderTask
    {
        SLayerRenderData &m_Data;
        SLayerRenderToTextureRunnable(SLayerRenderData &d)
            : m_Data(d)
        {
        }

        void Run() override { m_Data.RenderToTexture(); }
    };

    static inline OffscreenRendererDepthValues::Enum
    GetOffscreenRendererDepthValue(NVRenderTextureFormats::Enum inBufferFormat)
    {
        switch (inBufferFormat) {
        case NVRenderTextureFormats::Depth32:
            return OffscreenRendererDepthValues::Depth32;
        case NVRenderTextureFormats::Depth24:
            return OffscreenRendererDepthValues::Depth24;
        case NVRenderTextureFormats::Depth24Stencil8:
            return OffscreenRendererDepthValues::Depth24;
        default:
            QT3DS_ASSERT(false); // fallthrough intentional
        case NVRenderTextureFormats::Depth16:
            return OffscreenRendererDepthValues::Depth16;
        }
    }

    SOffscreenRendererEnvironment SLayerRenderData::CreateOffscreenRenderEnvironment()
    {
        OffscreenRendererDepthValues::Enum theOffscreenDepth(
            GetOffscreenRendererDepthValue(GetDepthBufferFormat()));
        NVRenderRect theViewport = m_Renderer.GetQt3DSContext().GetRenderList().GetViewport();
        return SOffscreenRendererEnvironment(theViewport.m_Width, theViewport.m_Height,
                                             NVRenderTextureFormats::RGBA8, theOffscreenDepth,
                                             false, AAModeValues::NoAA);
    }

    IRenderTask &SLayerRenderData::CreateRenderToTextureRunnable()
    {
        return *RENDER_FRAME_NEW(SLayerRenderToTextureRunnable)(*this);
    }

    void SLayerRenderData::addRef() { atomicIncrement(&mRefCount); }

    void SLayerRenderData::release() { QT3DS_IMPLEMENT_REF_COUNT_RELEASE(m_Allocator); }
}
}
