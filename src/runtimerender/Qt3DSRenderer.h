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
#ifndef QT3DS_RENDERER_H
#define QT3DS_RENDERER_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSFlags.h"
#include "EASTL/algorithm.h" //pair
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSVec2.h"
#include "Qt3DSRenderGraphObjectPickQuery.h"
#include "Qt3DSRenderCamera.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "Qt3DSRenderRay.h"

namespace qt3ds {
namespace render {

    class IRenderableObject;
    struct SModel;
    struct SText;
    struct SCamera;
    struct SLight;
    struct SLayer;
    class IBufferManager;
    typedef void *SRenderInstanceId;

    using qt3ds::foundation::NVConstDataRef;

    class IQt3DSRenderNodeFilter
    {
    protected:
        virtual ~IQt3DSRenderNodeFilter() {}
    public:
        virtual bool IncludeNode(const SNode &inNode) = 0;
    };
    struct SLayerPickSetup
    {
        QT3DSMat44 m_ProjectionPreMultiply;
        QT3DSMat44 m_ViewProjection;
        NVRenderRect m_ScissorRect;
        SLayerPickSetup(const QT3DSMat44 &inProjPreMult, const QT3DSMat44 &inVP,
                        const NVRenderRect &inScissor)
            : m_ProjectionPreMultiply(inProjPreMult)
            , m_ViewProjection(inVP)
            , m_ScissorRect(inScissor)
        {
        }
        SLayerPickSetup() {}
    };

    struct SScaleAndPosition
    {
        QT3DSVec3 m_Position;
        QT3DSF32 m_Scale;
        SScaleAndPosition(const QT3DSVec3 &inPos, QT3DSF32 inScale)
            : m_Position(inPos)
            , m_Scale(inScale)
        {
        }
        SScaleAndPosition() {}
    };

    class IQt3DSRenderer : public NVRefCounted
    {
    protected:
        virtual ~IQt3DSRenderer() {}

    public:
        virtual void EnableLayerCaching(bool inEnabled) = 0;
        virtual bool IsLayerCachingEnabled() const = 0;
        virtual void EnableLayerGpuProfiling(bool inEnabled) = 0;
        virtual bool IsLayerGpuProfilingEnabled() const = 0;

        virtual void setAlphaTest(bool enable, float op, float ref) = 0;

        // Get the camera that rendered this node last render
        virtual SCamera *GetCameraForNode(const SNode &inNode) const = 0;
        virtual Option<SCuboidRect> GetCameraBounds(const SGraphObject &inObject) = 0;
        // Called when you have changed the number or order of children of a given node.
        virtual void ChildrenUpdated(SNode &inParent) = 0;
        virtual QT3DSF32 GetTextScale(const SText &inText) = 0;

        // The IQt3DSRenderContext calls these, clients should not.
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        // Setup the vertex and index buffers (but not shader state)
        // and render the quad.  The quad is setup so that its edges
        // go from -1,1 in x,y and its UV coordinates will map naturally
        // to an image.
        virtual void RenderQuad() = 0;

        // Render a given texture to the scene using a given transform.
        virtual void RenderQuad(const QT3DSVec2 inDimensions, const QT3DSMat44 &inMVP,
                                NVRenderTexture2D &inQuadTexture) = 0;

        virtual void FillQuad(const QT3DSVec4 &color) = 0;

        // This point rendering works uisng indirect array drawing
        // This means you need to setup a GPU buffer
        // which contains the drawing information
        virtual void RenderPointsIndirect() = 0;

        // Returns true if this layer or a sibling was dirty.
        virtual bool PrepareLayerForRender(SLayer &inLayer, const QT3DSVec2 &inViewportDimensions,
                                           bool inRenderSiblings = true,
                                           const SRenderInstanceId id = nullptr) = 0;
        virtual void RenderLayer(SLayer &inLayer, const QT3DSVec2 &inViewportDimensions, bool clear,
                                 QT3DSVec4 clearColor, bool inRenderSiblings = true,
                                 const SRenderInstanceId id = nullptr) = 0;

        // Studio option to disable picking against sub renderers.  This allows better interaction
        // in studio.
        // In pick siblings measn pick the layer siblings; this is the normal behavior.
        // InPickEverything means ignore the node's pick flags; this allows us to only pick things
        // that have handlers
        // in some cases and just pick everything in other things.
        virtual void PickRenderPlugins(bool inPick) = 0;
        virtual Qt3DSRenderPickResult Pick(SLayer &inLayer, const QT3DSVec2 &inViewportDimensions,
                                           const QT3DSVec2 &inMouseCoords, bool inPickSiblings = true,
                                           bool inPickEverything = false,
                                           const SRenderInstanceId id = nullptr) = 0;

        // Return the relative hit position, in UV space, of a mouse pick against this object.
        // We need the node in order to figure out which layer rendered this object.
        // We need mapper objects if this is a in a subpresentation because we have to know how
        // to map the mouse coordinates into the subpresentation.  So for instance if inNode is in
        // a subpres then we need to know which image is displaying the subpres in order to map
        // the mouse coordinates into the subpres's render space.
        virtual Option<QT3DSVec2> FacePosition(SNode &inNode, NVBounds3 inBounds,
                                            const QT3DSMat44 &inGlobalTransform,
                                            const QT3DSVec2 &inViewportDimensions,
                                            const QT3DSVec2 &inMouseCoords,
                                            NVDataRef<SGraphObject *> inMapperObjects,
                                            SBasisPlanes::Enum inIsectPlane) = 0;

        virtual QT3DSVec3 UnprojectToPosition(SNode &inNode, QT3DSVec3 &inPosition,
                                           const QT3DSVec2 &inMouseVec) const = 0;
        virtual QT3DSVec3 UnprojectWithDepth(SNode &inNode, QT3DSVec3 &inPosition,
                                          const QT3DSVec3 &inMouseVec) const = 0;
        virtual QT3DSVec3 ProjectPosition(SNode &inNode, const QT3DSVec3 &inPosition) const = 0;

        virtual uint getLayerTextureId(SLayer &layer) = 0;

        // Roughly equivalent of gluPickMatrix, allows users to setup a perspective transform that
        // will draw some sub component
        // of the layer.  Used in combination with an expected viewport of 0,0,width,height the
        // viewproj matrix returned will center
        // around the center of the viewport and render just the part of the layer around this area.
        // The return value is optional because if the mouse point is completely outside the layer
        // obviously this method is irrelevant.
        virtual Option<SLayerPickSetup> GetLayerPickSetup(SLayer &inLayer,
                                                          const QT3DSVec2 &inMouseCoords,
                                                          const QSize &inPickDims) = 0;

        // Return the layer's viewport rect after the layer's member variables have been applied.
        // Uses the last rendered viewport rect.
        virtual Option<NVRenderRectF> GetLayerRect(SLayer &inLayer) = 0;
        // Testing function to allow clients to render a layer using a custom view project instead
        // of the one that would be setup
        // using the layer's camera in conjunction with the layer's position,scale.
        virtual void RunLayerRender(SLayer &inLayer, const QT3DSMat44 &inViewProjection) = 0;

        // This allocator is cleared every frame on BeginFrame.  Objects constructed using this
        // allocator
        // Must not need their destructors called.  Objects are allocate on 4 byte boundaries using
        // this allocator
        // regardless
        virtual NVAllocatorCallback &GetPerFrameAllocator() = 0;

        // Render the layer's rect onscreen.  Will only render one frame, you need to call this
        // every frame
        // for this to work and be persistent.
        virtual void RenderLayerRect(SLayer &inLayer, const QT3DSVec3 &inColor) = 0;
        // Render widgets are things that are draw on the layer's widget texture which is then
        // rendered to the
        // scene's widget texture.  You must add them every frame you wish them to be rendered; the
        // list of
        // widgets is cleared every frame.
        virtual void AddRenderWidget(IRenderWidget &inWidget) = 0;

        // Get a scale factor so you can have objects precisely 50 pixels.  Note that this scale
        // factor
        // only applies to things drawn parallel to the camera plane; If you aren't parallel then
        // there isn't
        // a single scale factor that will work.
        // For perspective-rendered objects, we shift the object forward or backwards along the
        // vector from the camera
        // to the object so that we are working in a consistent mathematical space.  So if the
        // camera is orthographic,
        // you are done.
        // If the camera is perspective, then this method will tell you want you need to scale
        // things by to account for
        // the FOV and also where the origin of the object needs to be to ensure the scale factor is
        // relevant.
        virtual SScaleAndPosition GetWorldToPixelScaleFactor(SLayer &inLayer,
                                                             const QT3DSVec3 &inWorldPoint) = 0;
        // Called before a layer goes completely out of scope to release any rendering resources
        // related to the layer.
        virtual void ReleaseLayerRenderResources(SLayer &inLayer, const SRenderInstanceId id) = 0;

        // render a screen aligned 2D text
        virtual void RenderText2D(QT3DSF32 x, QT3DSF32 y, qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor,
                                  const char *text) = 0;
        // render Gpu profiler values
        virtual void RenderGpuProfilerStats(QT3DSF32 x, QT3DSF32 y,
                                            qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor) = 0;

        // Get the mouse coordinates as they relate to a given layer
        virtual Option<QT3DSVec2> GetLayerMouseCoords(SLayer &inLayer, const QT3DSVec2 &inMouseCoords,
                                                   const QT3DSVec2 &inViewportDimensions,
                                                   bool forceImageIntersect = false) const = 0;

        virtual IRenderWidgetContext &GetRenderWidgetContext() = 0;

        static bool IsGlEsContext(qt3ds::render::NVRenderContextType inContextType);
        static bool IsGlEs3Context(qt3ds::render::NVRenderContextType inContextType);
        static bool IsGl2Context(qt3ds::render::NVRenderContextType inContextType);
        static const char *GetGlslVesionString(qt3ds::render::NVRenderContextType inContextType);

        static IQt3DSRenderer &CreateRenderer(IQt3DSRenderContext &inContext);
    };
}
}

#endif
