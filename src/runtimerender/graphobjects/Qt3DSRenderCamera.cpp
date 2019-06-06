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
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderPresentation.h"
#include "foundation/Qt3DSVec2.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSTextRenderer.h"

#include <qmath.h>

using namespace qt3ds::render;

namespace {

QT3DSF32 GetAspectRatio(const NVRenderRectF &inViewport)
{
    return inViewport.m_Height != 0 ? inViewport.m_Width / inViewport.m_Height : 0.0f;
}

QT3DSF32 GetAspectRatio(const QT3DSVec2 &inDimensions)
{
    return inDimensions.y != 0 ? inDimensions.x / inDimensions.y : 0.0f;
}

bool IsCameraVerticalAdjust(CameraScaleModes::Enum inMode, QT3DSF32 inDesignAspect,
                            QT3DSF32 inActualAspect)
{
    return (inMode == CameraScaleModes::Fit && inActualAspect >= inDesignAspect)
        || inMode == CameraScaleModes::FitVertical;
}

bool IsCameraHorizontalAdjust(CameraScaleModes::Enum inMode, QT3DSF32 inDesignAspect,
                              QT3DSF32 inActualAspect)
{
    return (inMode == CameraScaleModes::Fit && inActualAspect < inDesignAspect)
        || inMode == CameraScaleModes::FitHorizontal;
}

bool IsFitTypeScaleMode(CameraScaleModes::Enum inMode)
{
    return inMode == CameraScaleModes::Fit || inMode == CameraScaleModes::FitHorizontal
        || inMode == CameraScaleModes::FitVertical;
}

struct SPinCameraResult
{
    NVRenderRectF m_Viewport;
    NVRenderRectF m_VirtualViewport;
    SPinCameraResult(NVRenderRectF v, NVRenderRectF vv)
        : m_Viewport(v)
        , m_VirtualViewport(vv)
    {
    }
};
// Scale and transform the projection matrix to respect the camera anchor attribute
// and the scale mode.
SPinCameraResult PinCamera(const NVRenderRectF &inViewport, QT3DSVec2 inDesignDims,
                           QT3DSMat44 &ioPerspectiveMatrix, CameraScaleModes::Enum inScaleMode,
                           CameraScaleAnchors::Enum inPinLocation)
{
    NVRenderRectF viewport(inViewport);
    NVRenderRectF idealViewport(inViewport.m_X, inViewport.m_Y, inDesignDims.x, inDesignDims.y);
    QT3DSF32 designAspect = GetAspectRatio(inDesignDims);
    QT3DSF32 actualAspect = GetAspectRatio(inViewport);
    if (IsFitTypeScaleMode(inScaleMode)) {
        idealViewport.m_Width = viewport.m_Width;
        idealViewport.m_Height = viewport.m_Height;
    }
    // We move the viewport such that the left, top of the presentation sits against the left top
    // edge
    // We only need to translate in X *if* our actual aspect > design aspect
    // And then we only need to account for whatever centering would happen.

    bool pinLeft = inPinLocation == CameraScaleAnchors::SouthWest
        || inPinLocation == CameraScaleAnchors::West
        || inPinLocation == CameraScaleAnchors::NorthWest;
    bool pinRight = inPinLocation == CameraScaleAnchors::SouthEast
        || inPinLocation == CameraScaleAnchors::East
        || inPinLocation == CameraScaleAnchors::NorthEast;
    bool pinTop = inPinLocation == CameraScaleAnchors::NorthWest
        || inPinLocation == CameraScaleAnchors::North
        || inPinLocation == CameraScaleAnchors::NorthEast;
    bool pinBottom = inPinLocation == CameraScaleAnchors::SouthWest
        || inPinLocation == CameraScaleAnchors::South
        || inPinLocation == CameraScaleAnchors::SouthEast;

    if (inScaleMode == CameraScaleModes::SameSize) {
        // In this case the perspective transform does not center the view,
        // it places it in the lower-left of the viewport.
        QT3DSF32 idealWidth = inDesignDims.x;
        QT3DSF32 idealHeight = inDesignDims.y;
        if (pinRight)
            idealViewport.m_X -= ((idealWidth - inViewport.m_Width));
        else if (!pinLeft)
            idealViewport.m_X -= ((idealWidth - inViewport.m_Width) / 2.0f);

        if (pinTop)
            idealViewport.m_Y -= ((idealHeight - inViewport.m_Height));
        else if (!pinBottom)
            idealViewport.m_Y -= ((idealHeight - inViewport.m_Height) / 2.0f);
    } else {
        // In this case our perspective matrix will center the view and we need to decenter
        // it as necessary
        // if we are wider than we are high
        if (IsCameraVerticalAdjust(inScaleMode, designAspect, actualAspect)) {
            if (pinLeft || pinRight) {
                QT3DSF32 idealWidth = inViewport.m_Height * designAspect;
                QT3DSI32 halfOffset = (QT3DSI32)((idealWidth - inViewport.m_Width) / 2.0f);
                halfOffset = pinLeft ? halfOffset : -1 * halfOffset;
                idealViewport.m_X += halfOffset;
            }
        } else {
            if (pinTop || pinBottom) {
                QT3DSF32 idealHeight = inViewport.m_Width / designAspect;
                QT3DSI32 halfOffset = (QT3DSI32)((idealHeight - inViewport.m_Height) / 2.0f);
                halfOffset = pinBottom ? halfOffset : -1 * halfOffset;
                idealViewport.m_Y += halfOffset;
            }
        }
    }

    ioPerspectiveMatrix = NVRenderContext::ApplyVirtualViewportToProjectionMatrix(
        ioPerspectiveMatrix, viewport, idealViewport);
    return SPinCameraResult(viewport, idealViewport);
}
}

SCamera::SCamera()
    : SNode(GraphObjectTypes::Camera)
    , m_ClipNear(10)
    , m_ClipFar(10000)
    , m_FOV(60)
    , m_FOVHorizontal(false)
    , m_ScaleMode(CameraScaleModes::Fit)
    , m_ScaleAnchor(CameraScaleAnchors::Center)
{
    TORAD(m_FOV);
    m_Projection = QT3DSMat44::createIdentity();
    m_Position = QT3DSVec3(0, 0, -600);
}

// Code for testing
SCameraGlobalCalculationResult SCamera::CalculateGlobalVariables(const NVRenderRectF &inViewport,
                                                                 const QT3DSVec2 &inDesignDimensions)
{
    bool wasDirty = SNode::CalculateGlobalVariables();
    return SCameraGlobalCalculationResult(wasDirty,
                                          CalculateProjection(inViewport, inDesignDimensions));
}

bool SCamera::CalculateProjection(const NVRenderRectF &inViewport, const QT3DSVec2 &inDesignDimensions)
{
    bool retval = false;
    if (m_Flags.IsOrthographic())
        retval = ComputeFrustumOrtho(inViewport, inDesignDimensions);
    else
        retval = ComputeFrustumPerspective(inViewport, inDesignDimensions);
    if (retval) {
        QT3DSF32 *writePtr(m_Projection.front());
        m_FrustumScale.x = writePtr[0];
        m_FrustumScale.y = writePtr[5];
        PinCamera(inViewport, inDesignDimensions, m_Projection, m_ScaleMode, m_ScaleAnchor);
    }
    return retval;
}

//==============================================================================
/**
 *	Compute the projection matrix for a perspective camera
 *	@return true if the computed projection matrix is valid
 */
bool SCamera::ComputeFrustumPerspective(const NVRenderRectF &inViewport,
                                        const QT3DSVec2 &inDesignDimensions)
{
    m_Projection = QT3DSMat44::createIdentity();
    QT3DSF32 theAngleInRadians = verticalFov(inViewport) / 2.0f;
    QT3DSF32 theDeltaZ = m_ClipFar - m_ClipNear;
    QT3DSF32 theSine = sinf(theAngleInRadians);
    QT3DSF32 designAspect = GetAspectRatio(inDesignDimensions);
    QT3DSF32 theAspectRatio = designAspect;
    if (IsFitTypeScaleMode(m_ScaleMode))
        theAspectRatio = GetAspectRatio(inViewport);

    if ((theDeltaZ != 0) && (theSine != 0) && (theAspectRatio != 0)) {
        QT3DSF32 *writePtr(m_Projection.front());
        writePtr[10] = -(m_ClipFar + m_ClipNear) / theDeltaZ;
        writePtr[11] = -1;
        writePtr[14] = -2 * m_ClipNear * m_ClipFar / theDeltaZ;
        writePtr[15] = 0;

        if (IsCameraVerticalAdjust(m_ScaleMode, designAspect, theAspectRatio)) {
            QT3DSF32 theCotangent = cosf(theAngleInRadians) / theSine;
            writePtr[0] = theCotangent / theAspectRatio;
            writePtr[5] = theCotangent;
        } else {
            QT3DSF32 theCotangent = cosf(theAngleInRadians) / theSine;
            writePtr[0] = theCotangent / designAspect;
            writePtr[5] = theCotangent * (theAspectRatio / designAspect);
        }
        return true;
    } else {
        QT3DS_ASSERT(false);
        return false;
    }
}

//==============================================================================
/**
 *	Compute the projection matrix for a orthographic camera
 *	@return true if the computed projection matrix is valid
 */
bool SCamera::ComputeFrustumOrtho(const NVRenderRectF &inViewport, const QT3DSVec2 &inDesignDimensions)
{
    m_Projection = QT3DSMat44::createIdentity();

    QT3DSF32 theDeltaZ = m_ClipFar - m_ClipNear;
    QT3DSF32 halfWidth = inDesignDimensions.x / 2.0f;
    QT3DSF32 halfHeight = inDesignDimensions.y / 2.0f;
    QT3DSF32 designAspect = GetAspectRatio(inDesignDimensions);
    QT3DSF32 theAspectRatio = designAspect;
    if (IsFitTypeScaleMode(m_ScaleMode))
        theAspectRatio = GetAspectRatio(inViewport);
    if (theDeltaZ != 0) {
        QT3DSF32 *writePtr(m_Projection.front());
        writePtr[10] = -2.0f / theDeltaZ;
        writePtr[11] = 0.0f;
        writePtr[14] = -(m_ClipNear + m_ClipFar) / theDeltaZ;
        writePtr[15] = 1.0f;
        if (IsCameraVerticalAdjust(m_ScaleMode, designAspect, theAspectRatio)) {
            writePtr[0] = 1.0f / (halfHeight * theAspectRatio);
            writePtr[5] = 1.0f / halfHeight;
        } else {
            writePtr[0] = 1.0f / halfWidth;
            writePtr[5] = 1.0f / (halfWidth / theAspectRatio);
        }
        return true;
    } else {
        QT3DS_ASSERT(false);
        return false;
    }
}

QT3DSF32 SCamera::GetOrthographicScaleFactor(const NVRenderRectF &inViewport,
                                          const QT3DSVec2 &inDesignDimensions) const
{
    if (m_ScaleMode == CameraScaleModes::SameSize)
        return 1.0f;
    QT3DSMat44 temp(QT3DSMat44::createIdentity());
    QT3DSF32 designAspect = GetAspectRatio(inDesignDimensions);
    QT3DSF32 theAspectRatio = GetAspectRatio(inViewport);
    if (m_ScaleMode == CameraScaleModes::Fit) {
        if (theAspectRatio >= designAspect) {
            return inViewport.m_Width < inDesignDimensions.x ? theAspectRatio / designAspect : 1.0f;

        } else {
            return inViewport.m_Height < inDesignDimensions.y ? designAspect / theAspectRatio
                                                              : 1.0f;
        }
    } else if (m_ScaleMode == CameraScaleModes::FitVertical) {
        return (QT3DSF32)inDesignDimensions.y / (QT3DSF32)inViewport.m_Height;
    } else {
        return (QT3DSF32)inDesignDimensions.x / (QT3DSF32)inViewport.m_Width;
    }
}

QT3DSF32 SCamera::GetTextScaleFactor(const NVRenderRectF &inViewport,
                                  const QT3DSVec2 &inDesignDimensions) const
{
    return NVMax(1.0f, 1.0f / GetOrthographicScaleFactor(inViewport, inDesignDimensions));
}

QT3DSMat33 SCamera::GetLookAtMatrix(const QT3DSVec3 &inUpDir, const QT3DSVec3 &inDirection) const
{
    QT3DSVec3 theDirection(inDirection);

    theDirection.normalize();

    const QT3DSVec3 &theUpDir(inUpDir);

    // gram-shmidt orthogonalization
    QT3DSVec3 theCrossDir(theDirection.cross(theUpDir));
    theCrossDir.normalize();
    QT3DSVec3 theFinalDir(theCrossDir.cross(theDirection));
    theFinalDir.normalize();
    QT3DSF32 multiplier = 1.0f;
    if (m_Flags.IsLeftHanded())
        multiplier = -1.0f;

    QT3DSMat33 theResultMatrix(theCrossDir, theFinalDir, multiplier * theDirection);
    return theResultMatrix;
}

void SCamera::LookAt(const QT3DSVec3 &inCameraPos, const QT3DSVec3 &inUpDir, const QT3DSVec3 &inTargetPos)
{
    QT3DSVec3 theDirection = inTargetPos - inCameraPos;
    if (m_Flags.IsLeftHanded())
        theDirection.z *= -1.0f;
    m_Rotation = GetRotationVectorFromRotationMatrix(GetLookAtMatrix(inUpDir, theDirection));
    m_Position = inCameraPos;
    MarkDirty(qt3ds::render::NodeTransformDirtyFlag::TransformIsDirty);
}

void SCamera::CalculateViewProjectionMatrix(QT3DSMat44 &outMatrix) const
{
    QT3DSMat44 globalInverse = m_GlobalTransform.getInverse();
    outMatrix = m_Projection * globalInverse;
}

SCuboidRect SCamera::GetCameraBounds(const NVRenderRectF &inViewport,
                                     const QT3DSVec2 &inDesignDimensions) const
{
    QT3DSMat44 unused(QT3DSMat44::createIdentity());
    SPinCameraResult theResult =
        PinCamera(inViewport, inDesignDimensions, unused, m_ScaleMode, m_ScaleAnchor);
    // find the normalized edges of the view frustum given the renormalization that happens when
    // pinning the camera.
    SCuboidRect normalizedCuboid(-1, 1, 1, -1);
    QT3DSVec2 translation(theResult.m_Viewport.m_X - theResult.m_VirtualViewport.m_X,
                       theResult.m_Viewport.m_Y - theResult.m_VirtualViewport.m_Y);
    if (m_ScaleMode == CameraScaleModes::SameSize) {
        // the cuboid ranges are the actual divided by the ideal in this case
        QT3DSF32 xRange = 2.0f * (theResult.m_Viewport.m_Width / theResult.m_VirtualViewport.m_Width);
        QT3DSF32 yRange =
            2.0f * (theResult.m_Viewport.m_Height / theResult.m_VirtualViewport.m_Height);
        normalizedCuboid = SCuboidRect(-1, -1 + yRange, -1 + xRange, -1);
        translation.x /= (theResult.m_VirtualViewport.m_Width / 2.0f);
        translation.y /= (theResult.m_VirtualViewport.m_Height / 2.0f);
        normalizedCuboid.Translate(translation);
    }
    // fit.  This means that two parameters of the normalized cuboid will be -1, 1.
    else {
        // In this case our perspective matrix will center the view and we need to decenter
        // it as necessary
        QT3DSF32 actualAspect = GetAspectRatio(inViewport);
        QT3DSF32 designAspect = GetAspectRatio(inDesignDimensions);
        // if we are wider than we are high
        QT3DSF32 idealWidth = inViewport.m_Width;
        QT3DSF32 idealHeight = inViewport.m_Height;

        if (IsCameraVerticalAdjust(m_ScaleMode, designAspect, actualAspect)) {
            // then we just need to setup the left, right parameters of the cuboid because we know
            // the top
            // bottom are -1,1 due to how fit works.
            idealWidth = (QT3DSF32)ITextRenderer::NextMultipleOf4(
                (QT3DSU32)(inViewport.m_Height * designAspect + .5f));
            // halfRange should always be greater than 1.0f.
            QT3DSF32 halfRange = inViewport.m_Width / idealWidth;
            normalizedCuboid.m_Left = -halfRange;
            normalizedCuboid.m_Right = halfRange;
            translation.x = translation.x / (idealWidth / 2.0f);
        } else {
            idealHeight = (QT3DSF32)ITextRenderer::NextMultipleOf4(
                (QT3DSU32)(inViewport.m_Width / designAspect + .5f));
            QT3DSF32 halfRange = inViewport.m_Height / idealHeight;
            normalizedCuboid.m_Bottom = -halfRange;
            normalizedCuboid.m_Top = halfRange;
            translation.y = translation.y / (idealHeight / 2.0f);
        }
        normalizedCuboid.Translate(translation);
    }
    // Given no adjustment in the virtual rect, then this is what we would have.

    return normalizedCuboid;
}

void SCamera::SetupOrthographicCameraForOffscreenRender(NVRenderTexture2D &inTexture,
                                                        QT3DSMat44 &outVP)
{
    STextureDetails theDetails(inTexture.GetTextureDetails());
    SCamera theTempCamera;
    SetupOrthographicCameraForOffscreenRender(inTexture, outVP, theTempCamera);
}

void SCamera::SetupOrthographicCameraForOffscreenRender(NVRenderTexture2D &inTexture,
                                                        QT3DSMat44 &outVP, SCamera &outCamera)
{
    STextureDetails theDetails(inTexture.GetTextureDetails());
    SCamera theTempCamera;
    theTempCamera.m_Flags.SetOrthographic(true);
    theTempCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
    QT3DSVec2 theDimensions((QT3DSF32)theDetails.m_Width, (QT3DSF32)theDetails.m_Height);
    theTempCamera.CalculateGlobalVariables(
        NVRenderRect(0, 0, theDetails.m_Width, theDetails.m_Height), theDimensions);
    theTempCamera.CalculateViewProjectionMatrix(outVP);
    outCamera = theTempCamera;
}

SRay SCamera::Unproject(const QT3DSVec2 &inViewportRelativeCoords, const NVRenderRectF &inViewport,
                        const QT3DSVec2 &inDesignDimensions, bool sceneCameraView) const
{
    SRay theRay;
    QT3DSMat44 tempVal(QT3DSMat44::createIdentity());
    SPinCameraResult result =
        PinCamera(inViewport, inDesignDimensions, tempVal, m_ScaleMode, m_ScaleAnchor);
    QT3DSVec2 globalCoords = inViewport.ToAbsoluteCoords(inViewportRelativeCoords);
    QT3DSVec2 normalizedCoords =
        result.m_VirtualViewport.AbsoluteToNormalizedCoordinates(globalCoords);
    QT3DSVec3 &outOrigin(theRay.m_Origin);
    QT3DSVec3 &outDir(theRay.m_Direction);
    QT3DSVec2 inverseFrustumScale(1.0f / m_FrustumScale.x, 1.0f / m_FrustumScale.y);
    QT3DSVec2 scaledCoords(inverseFrustumScale.x * normalizedCoords.x,
                        inverseFrustumScale.y * normalizedCoords.y);

    if (m_Flags.IsOrthographic()) {
        outOrigin.x = scaledCoords.x;
        outOrigin.y = scaledCoords.y;
        outOrigin.z = 0.0f;

        outDir.x = 0.0f;
        outDir.y = 0.0f;
        outDir.z = -1.0f;
    } else {
        outOrigin.x = 0.0f;
        outOrigin.y = 0.0f;
        outOrigin.z = 0.0f;

        outDir.x = scaledCoords.x;
        outDir.y = scaledCoords.y;
        outDir.z = -1.0f;
    }

    outOrigin = m_GlobalTransform.transform(outOrigin);

    // CalculateNormalMatrix(), but 4x4 matrix to have scale() method
    QT3DSMat44 theNormalMatrix = m_GlobalTransform.getInverse().getTranspose();
    if (sceneCameraView) {
        // When in scene camera view mode, camera scale needs to be inverted.
        // See QT3DS-3393.
        const float scaleX = m_GlobalTransform[0][0] / theNormalMatrix[0][0];
        const float scaleY = m_GlobalTransform[1][1] / theNormalMatrix[1][1];
        const float scaleZ = m_GlobalTransform[2][2] / theNormalMatrix[2][2];
        QT3DSVec4 scaleVector(scaleX, scaleY, scaleZ, 1.0);
        theNormalMatrix.scale(scaleVector);
    }

    outDir = theNormalMatrix.transform(outDir);
    outDir.normalize();
    /*
    char printBuf[2000];
    sprintf_s( printBuf, "normCoords %f %f outDir %f %f %f\n"
            , normalizedCoords.x, normalizedCoords.y, outDir.x, outDir.y, outDir.z );
    OutputDebugStringA( printBuf );
    */

    return theRay;
}

QT3DSVec3 SCamera::UnprojectToPosition(const QT3DSVec3 &inGlobalPos, const SRay &inRay) const
{
    QT3DSVec3 theCameraDir = GetDirection();
    QT3DSVec3 theObjGlobalPos = inGlobalPos;
    QT3DSF32 theDistance = -1.0f * theObjGlobalPos.dot(theCameraDir);
    NVPlane theCameraPlane(theCameraDir, theDistance);
    return inRay.Intersect(theCameraPlane);
}

QT3DSF32 SCamera::verticalFov(QT3DSF32 aspectRatio) const
{
    if (m_FOVHorizontal)
        return 2.0f * qAtan(qTan(qreal(m_FOV) / 2.0) / qreal(aspectRatio));
    else
        return m_FOV;
}

QT3DSF32 SCamera::verticalFov(const NVRenderRectF &inViewport) const
{
    return verticalFov(GetAspectRatio(inViewport));
}
