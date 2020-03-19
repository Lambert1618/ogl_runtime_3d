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
#include "Qt3DSRendererImplLayerRenderHelper.h"
#include "Qt3DSRenderLayer.h"
#include "Qt3DSTextRenderer.h"

using namespace qt3ds::render;

namespace {
// left/top
QT3DSF32 GetMinValue(QT3DSF32 start, QT3DSF32 width, QT3DSF32 value, LayerUnitTypes::Enum units)
{
    if (units == LayerUnitTypes::Pixels)
        return start + value;

    return start + (value * width / 100.0f);
}

// width/height
QT3DSF32 GetValueLen(QT3DSF32 width, QT3DSF32 value, LayerUnitTypes::Enum units)
{
    if (units == LayerUnitTypes::Pixels)
        return value;

    return width * value / 100.0f;
}

// right/bottom
QT3DSF32 GetMaxValue(QT3DSF32 start, QT3DSF32 width, QT3DSF32 value, LayerUnitTypes::Enum units)
{
    if (units == LayerUnitTypes::Pixels)
        return start + width - value;

    return start + width - (value * width / 100.0f);
}

QT3DSVec2 ToRectRelativeCoords(const QT3DSVec2 &inCoords, const NVRenderRectF &inRect)
{
    return QT3DSVec2(inCoords.x - inRect.m_X, inCoords.y - inRect.m_Y);
}
}

SLayerRenderHelper::SLayerRenderHelper()
    : m_Layer(nullptr)
    , m_Camera(nullptr)
    , m_CameraLeftEye(nullptr)
    , m_CameraRightEye(nullptr)
    , m_Offscreen(false)
{
}

SLayerRenderHelper::SLayerRenderHelper(const NVRenderRectF &inPresentationViewport,
                                       const NVRenderRectF &inPresentationScissor,
                                       const QT3DSVec2 &inPresentationDesignDimensions,
                                       SLayer &inLayer, bool inOffscreen,
                                       qt3ds::render::ScaleModes::Enum inScaleMode,
                                       qt3ds::render::StereoModes::Enum inStereoMode,
                                       qt3ds::render::StereoViews::Enum inStereoView,
                                       double inStereoEyeSeparation,
                                       qt3ds::QT3DSVec2 inScaleFactor)
    : m_PresentationViewport(inPresentationViewport)
    , m_PresentationScissor(inPresentationScissor)
    , m_PresentationDesignDimensions(inPresentationDesignDimensions)
    , m_Layer(&inLayer)
    , m_Camera(nullptr)
    , m_CameraLeftEye(nullptr)
    , m_CameraRightEye(nullptr)
    , m_Offscreen(inOffscreen)
    , m_ScaleMode(inScaleMode)
    , m_StereoMode(inStereoMode)
    , m_StereoView(inStereoView)
    , m_StereoEyeSeparation(QT3DSF32(inStereoEyeSeparation))
    , m_ScaleFactor(inScaleFactor)
{
    {
        QT3DSF32 left = m_Layer->m_Left;
        QT3DSF32 right = m_Layer->m_Right;
        QT3DSF32 width = m_Layer->m_Width;

        if (m_ScaleMode == qt3ds::render::ScaleModes::FitSelected) {
            if (m_Layer->m_LeftUnits == LayerUnitTypes::Pixels)
                left *= m_ScaleFactor.x;

            if (m_Layer->m_RightUnits == LayerUnitTypes::Pixels)
                right *= m_ScaleFactor.x;

            if (m_Layer->m_WidthUnits == LayerUnitTypes::Pixels)
                width *= m_ScaleFactor.x;
        }

        QT3DSF32 horzMin = GetMinValue(inPresentationViewport.m_X, inPresentationViewport.m_Width,
                                    left, m_Layer->m_LeftUnits);
        QT3DSF32 horzWidth = GetValueLen(inPresentationViewport.m_Width, width, m_Layer->m_WidthUnits);
        QT3DSF32 horzMax = GetMaxValue(inPresentationViewport.m_X, inPresentationViewport.m_Width,
                                    right, m_Layer->m_RightUnits);

        switch (inLayer.m_HorizontalFieldValues) {
        case HorizontalFieldValues::LeftWidth:
            m_Viewport.m_X = horzMin;
            m_Viewport.m_Width = horzWidth;
            break;
        case HorizontalFieldValues::LeftRight:
            m_Viewport.m_X = horzMin;
            m_Viewport.m_Width = horzMax - horzMin;
            break;
        case HorizontalFieldValues::WidthRight:
            m_Viewport.m_Width = horzWidth;
            m_Viewport.m_X = horzMax - horzWidth;
            break;
        }
    }
    {
        QT3DSF32 top = m_Layer->m_Top;
        QT3DSF32 bottom = m_Layer->m_Bottom;
        QT3DSF32 height = m_Layer->m_Height;

        if (m_ScaleMode == qt3ds::render::ScaleModes::FitSelected) {

            if (m_Layer->m_TopUnits == LayerUnitTypes::Pixels)
                top *= m_ScaleFactor.y;

            if (m_Layer->m_BottomUnits == LayerUnitTypes::Pixels)
                bottom *= m_ScaleFactor.y;

            if (m_Layer->m_HeightUnits == LayerUnitTypes::Pixels)
                height *= m_ScaleFactor.y;
        }

        QT3DSF32 vertMin = GetMinValue(inPresentationViewport.m_Y, inPresentationViewport.m_Height,
                                    bottom, m_Layer->m_BottomUnits);
        QT3DSF32 vertWidth =
            GetValueLen(inPresentationViewport.m_Height, height, m_Layer->m_HeightUnits);
        QT3DSF32 vertMax = GetMaxValue(inPresentationViewport.m_Y, inPresentationViewport.m_Height,
                                    top, m_Layer->m_TopUnits);

        switch (inLayer.m_VerticalFieldValues) {
        case VerticalFieldValues::HeightBottom:
            m_Viewport.m_Y = vertMin;
            m_Viewport.m_Height = vertWidth;
            break;
        case VerticalFieldValues::TopBottom:
            m_Viewport.m_Y = vertMin;
            m_Viewport.m_Height = vertMax - vertMin;
            break;
        case VerticalFieldValues::TopHeight:
            m_Viewport.m_Height = vertWidth;
            m_Viewport.m_Y = vertMax - vertWidth;
            break;
        }
    }

    m_Viewport.m_Width = NVMax(1.0f, m_Viewport.m_Width);
    m_Viewport.m_Height = NVMax(1.0f, m_Viewport.m_Height);
    // Now force the viewport to be a multiple of four in width and height. This is because
    // when rendering to a texture we have to respect this and not forcing it causes scaling issues
    // that are noticeable especially in situations where customers are using text.
    QT3DSF32 originalWidth = m_Viewport.m_Width;
    QT3DSF32 originalHeight = m_Viewport.m_Height;

    m_Viewport.m_Width = (QT3DSF32)ITextRenderer::NextMultipleOf4((QT3DSU32)m_Viewport.m_Width);
    m_Viewport.m_Height = (QT3DSF32)ITextRenderer::NextMultipleOf4((QT3DSU32)m_Viewport.m_Height);

    // Now fudge the offsets to account for this slight difference
    m_Viewport.m_X += (originalWidth - m_Viewport.m_Width) / 2.0f;
    m_Viewport.m_Y += (originalHeight - m_Viewport.m_Height) / 2.0f;

    m_originalViewport = m_Viewport;
    m_Scissor = m_Viewport;
    m_Scissor.EnsureInBounds(inPresentationScissor);
    QT3DS_ASSERT(m_Scissor.m_Width >= 0.0f);
    QT3DS_ASSERT(m_Scissor.m_Height >= 0.0f);
}

// This is the viewport the camera will use to setup the projection.
NVRenderRectF SLayerRenderHelper::GetLayerRenderViewport() const
{
    if (m_Offscreen)
        return NVRenderRectF(0, 0, m_Viewport.m_Width, (QT3DSF32)m_Viewport.m_Height);
    else
        return m_Viewport;
}

NVRenderRectF SLayerRenderHelper::GetLayerToPresentationViewport() const
{
    if (m_StereoMode == StereoModes::LeftRight) {
        if (m_StereoView == StereoViews::Left) {
            return NVRenderRectF(m_Viewport.m_X, m_Viewport.m_Y, m_Viewport.m_Width/2,
                                 m_Viewport.m_Height);
        }
        if (m_StereoView == StereoViews::Right) {
            return NVRenderRectF(m_Viewport.m_X + m_Viewport.m_Width/2, m_Viewport.m_Y,
                                 m_Viewport.m_Width/2, m_Viewport.m_Height);
        }
    } else if (m_StereoMode == StereoModes::TopBottom) {
        if (m_StereoView == StereoViews::Left) {
            return NVRenderRectF(m_Viewport.m_X, m_Viewport.m_Y + m_Viewport.m_Height/2,
                                 m_Viewport.m_Width, m_Viewport.m_Height/2);
        }
        if (m_StereoView == StereoViews::Right) {
            return NVRenderRectF(m_Viewport.m_X, m_Viewport.m_Y,
                                 m_Viewport.m_Width, m_Viewport.m_Height/2);
        }
    }
    return m_Viewport;
}

NVRenderRectF SLayerRenderHelper::GetLayerToPresentationScissorRect() const
{
    if (m_StereoMode == StereoModes::LeftRight) {
        if (m_StereoView == StereoViews::Left) {
            return NVRenderRectF(m_Scissor.m_X, m_Scissor.m_Y,
                                 m_Scissor.m_Width/2, m_Scissor.m_Height);
        }
        if (m_StereoView == StereoViews::Right) {
            return NVRenderRectF(m_Scissor.m_X + m_Scissor.m_Width/2, m_Scissor.m_Y,
                                 m_Scissor.m_Width/2, m_Scissor.m_Height);
        }
    } else if (m_StereoMode == StereoModes::TopBottom) {
        if (m_StereoView == StereoViews::Left) {
            return NVRenderRectF(m_Scissor.m_X, m_Scissor.m_Y + m_Scissor.m_Height/2,
                                 m_Scissor.m_Width, m_Scissor.m_Height/2);
        }
        if (m_StereoView == StereoViews::Right) {
            return NVRenderRectF(m_Scissor.m_X, m_Scissor.m_Y,
                                 m_Scissor.m_Width, m_Scissor.m_Height/2);
        }
    }

    return m_Scissor;
}

QSize SLayerRenderHelper::GetTextureDimensions() const
{
    QT3DSU32 width = (QT3DSU32)m_Viewport.m_Width;
    QT3DSU32 height = (QT3DSU32)m_Viewport.m_Height;
    return QSize(ITextRenderer::NextMultipleOf4(width),
                             ITextRenderer::NextMultipleOf4(height));
}

SCamera *SLayerRenderHelper::GetCamera()  {
    if (m_StereoView == StereoViews::Left)
        return m_CameraLeftEye;
    if (m_StereoView == StereoViews::Right)
        return m_CameraRightEye;
    return m_Camera;
}

SCameraGlobalCalculationResult SLayerRenderHelper::SetupCameraForRender(SCamera &inCamera)
{
    m_Camera = &inCamera;

    if (isStereoscopic())
        adjustCameraStereoSeparation();

    NVRenderRectF rect = GetLayerRenderViewport();
    if (m_ScaleMode == ScaleModes::FitSelected) {
        rect.m_Width =
            (QT3DSF32)(ITextRenderer::NextMultipleOf4((QT3DSU32)(rect.m_Width / m_ScaleFactor.x)));
        rect.m_Height =
            (QT3DSF32)(ITextRenderer::NextMultipleOf4((QT3DSU32)(rect.m_Height / m_ScaleFactor.y)));
    }
    // Always calculate main camera variables
    if (isStereoscopic())
        m_Camera->CalculateGlobalVariables(rect, m_PresentationDesignDimensions);
    // Return current camera variables
    return GetCamera()->CalculateGlobalVariables(rect, m_PresentationDesignDimensions);
}

Option<QT3DSVec2> SLayerRenderHelper::GetLayerMouseCoords(const QT3DSVec2 &inMouseCoords,
                                                       const QT3DSVec2 &inWindowDimensions,
                                                       bool inForceIntersect) const
{
    // First invert the y so we are dealing with numbers in a normal coordinate space.
    // Second, move into our layer's coordinate space
    QT3DSVec2 correctCoords(inMouseCoords.x, inWindowDimensions.y - inMouseCoords.y);
    QT3DSVec2 theLocalMouse;

    if (m_Layer->m_DynamicResize) {
        float widthRatio = m_Viewport.m_Width / m_originalViewport.m_Width;
        float heightRatio = m_Viewport.m_Height / m_originalViewport.m_Height;
        theLocalMouse = m_originalViewport.ToRectRelative(correctCoords);
        theLocalMouse = QT3DSVec2(theLocalMouse.x * widthRatio, theLocalMouse.y * heightRatio);
    } else {
        theLocalMouse = m_Viewport.ToRectRelative(correctCoords);
    }

    QT3DSF32 theRenderRectWidth = m_Viewport.m_Width;
    QT3DSF32 theRenderRectHeight = m_Viewport.m_Height;
    // Crop the mouse to the rect.  Apply no further translations.
    if (inForceIntersect == false
        && (theLocalMouse.x < 0.0f || theLocalMouse.x >= theRenderRectWidth
            || theLocalMouse.y < 0.0f || theLocalMouse.y >= theRenderRectHeight)) {
        return Empty();
    }
    return theLocalMouse;
}

Option<SRay> SLayerRenderHelper::GetPickRay(const QT3DSVec2 &inMouseCoords,
                                            const QT3DSVec2 &inWindowDimensions,
                                            bool inForceIntersect,
                                            bool sceneCameraView) const
{
    if (!m_Camera)
        return Empty();
    Option<QT3DSVec2> theCoords(
        GetLayerMouseCoords(inMouseCoords, inWindowDimensions, inForceIntersect));
    if (theCoords.hasValue()) {
        // The cameras projection is different if we are onscreen vs. offscreen.
        // When offscreen, we need to move the mouse coordinates into a local space
        // to the layer.
        return m_Camera->Unproject(*theCoords, m_Viewport, m_PresentationDesignDimensions,
                                   sceneCameraView);
    }
    return Empty();
}

bool SLayerRenderHelper::IsLayerVisible() const
{
    return m_Scissor.m_Height >= 2.0f && m_Scissor.m_Width >= 2.0f;
}

bool SLayerRenderHelper::isStereoscopic() const
{
    return m_StereoMode != StereoModes::Mono;
}

void SLayerRenderHelper::setViewport(const NVRenderRectF &viewport)
{
    m_Viewport = viewport;
    m_Viewport.m_Width = NVMax(1.0f, m_Viewport.m_Width);
    m_Viewport.m_Height = NVMax(1.0f, m_Viewport.m_Height);
    // Now force the viewport to be a multiple of four in width and height. This is because
    // when rendering to a texture we have to respect this and not forcing it causes scaling issues
    // that are noticeable especially in situations where customers are using text.
    QT3DSF32 originalWidth = m_Viewport.m_Width;
    QT3DSF32 originalHeight = m_Viewport.m_Height;

    m_Viewport.m_Width = (QT3DSF32)ITextRenderer::NextMultipleOf4((QT3DSU32)m_Viewport.m_Width);
    m_Viewport.m_Height = (QT3DSF32)ITextRenderer::NextMultipleOf4((QT3DSU32)m_Viewport.m_Height);

    // Now fudge the offsets to account for this slight difference
    m_Viewport.m_X += (originalWidth - m_Viewport.m_Width) / 2.0f;
    m_Viewport.m_Y += (originalHeight - m_Viewport.m_Height) / 2.0f;
}

NVRenderRectF SLayerRenderHelper::getOriginalLayerToPresentationViewport() const
{
    return m_originalViewport;
}

void SLayerRenderHelper::copyCameraProperties(SCamera *sourceCamera,
                                              SCamera *destinationCamera)
{
    if (!sourceCamera || !destinationCamera)
        return;

    destinationCamera->m_FOV = sourceCamera->m_FOV;
    destinationCamera->m_ClipFar = sourceCamera->m_ClipFar;
    destinationCamera->m_ClipNear = sourceCamera->m_ClipNear;
    destinationCamera->m_ScaleMode = sourceCamera->m_ScaleMode;
    destinationCamera->m_Projection = sourceCamera->m_Projection;
    destinationCamera->m_ScaleAnchor = sourceCamera->m_ScaleAnchor;
    destinationCamera->m_FrustumScale = sourceCamera->m_FrustumScale;
    destinationCamera->m_FOVHorizontal = sourceCamera->m_FOVHorizontal;
    destinationCamera->m_EnableFrustumCulling = sourceCamera->m_EnableFrustumCulling;
    destinationCamera->m_Flags = sourceCamera->m_Flags;
    destinationCamera->m_Pivot = sourceCamera->m_Pivot;
    destinationCamera->m_Scale = sourceCamera->m_Scale;
    destinationCamera->m_DFSIndex = sourceCamera->m_DFSIndex;
    destinationCamera->m_Position = sourceCamera->m_Position;
    destinationCamera->m_Rotation = sourceCamera->m_Rotation;
    destinationCamera->m_UserData = sourceCamera->m_UserData;
    destinationCamera->m_LocalOpacity = sourceCamera->m_LocalOpacity;
    destinationCamera->m_GlobalOpacity = sourceCamera->m_GlobalOpacity;
    destinationCamera->m_RotationOrder = sourceCamera->m_RotationOrder;
    destinationCamera->m_LocalTransform = sourceCamera->m_LocalTransform;
    destinationCamera->m_GlobalTransform = sourceCamera->m_GlobalTransform;
}

void SLayerRenderHelper::adjustCameraStereoSeparation()
{
    if (!m_CameraLeftEye)
        m_CameraLeftEye = new SCamera();
    if (!m_CameraRightEye)
        m_CameraRightEye = new SCamera();

    // Copy m_Camera properties into left & right cameras
    copyCameraProperties(m_Camera, m_CameraLeftEye);
    copyCameraProperties(m_Camera, m_CameraRightEye);

    // Adjust left & right camera positions by eye separation
    QT3DSVec3 eyeMove(m_StereoEyeSeparation, 0, 0);
    QT3DSVec3 camMove = m_Camera->m_GlobalTransform.transform(eyeMove)
            - m_Camera->m_GlobalTransform.getPosition();
    camMove.z *= -1; // Inverse z
    m_CameraLeftEye->m_Position -= camMove;
    m_CameraLeftEye->m_Flags.SetTransformDirty(true);
    m_CameraRightEye->m_Position += camMove;
    m_CameraRightEye->m_Flags.SetTransformDirty(true);

    m_CameraLeftEye->MarkDirty();
    m_CameraRightEye->MarkDirty();
}
