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
#include "Qt3DSRenderRay.h"
#include "foundation/Qt3DSPlane.h"

using namespace qt3ds::render;

// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rayplane_intersection.htm

Option<QT3DSVec3> SRay::Intersect(const NVPlane &inPlane) const
{
    QT3DSF32 Vd = inPlane.n.dot(m_Direction);
    if (fabs(Vd) < .0001f)
        return Empty();
    QT3DSF32 V0 = -1.0f * (inPlane.n.dot(m_Origin) + inPlane.d);
    QT3DSF32 t = V0 / Vd;
    return m_Origin + (m_Direction * t);
}

Option<SRayIntersectionResult> SRay::IntersectWithAABB(const QT3DSMat44 &inGlobalTransform,
                                                       const NVBounds3 &inBounds,
                                                       bool inForceIntersect) const
{
    // Intersect the origin with the AABB described by bounds.

    // Scan each axis separately.  This code basically finds the distance
    // distance from the origin to the near and far bbox planes for a given
    // axis.  It then divides this distance by the direction for that axis to
    // get a range of t [near,far] that the ray intersects assuming the ray is
    // described via origin + t*(direction).  Running through all three axis means
    // that you need to min/max those ranges together to find a global min/max
    // that the pick could possibly be in.

    // Transform pick origin and direction into the subset's space.
    QT3DSMat44 theOriginTransform = inGlobalTransform.getInverse();

    QT3DSVec3 theTransformedOrigin = theOriginTransform.transform(m_Origin);
    QT3DSF32 *outOriginTransformPtr(theOriginTransform.front());
    outOriginTransformPtr[12] = outOriginTransformPtr[13] = outOriginTransformPtr[14] = 0.0f;
    QT3DSVec3 theTransformedDirection = theOriginTransform.rotate(m_Direction);

    static const QT3DSF32 KD_FLT_MAX = 3.40282346638528860e+38;
    static const QT3DSF32 kEpsilon = 1e-5f;

    QT3DSF32 theMinWinner = -KD_FLT_MAX;
    QT3DSF32 theMaxWinner = KD_FLT_MAX;

    for (QT3DSU32 theAxis = 0; theAxis < 3; ++theAxis) {
        // Extract the ranges and direction for this axis
        QT3DSF32 theMinBox = inBounds.minimum[theAxis];
        QT3DSF32 theMaxBox = inBounds.maximum[theAxis];
        QT3DSF32 theDirectionAxis = theTransformedDirection[theAxis];
        QT3DSF32 theOriginAxis = theTransformedOrigin[theAxis];

        QT3DSF32 theMinAxis = -KD_FLT_MAX;
        QT3DSF32 theMaxAxis = KD_FLT_MAX;
        if (theDirectionAxis > kEpsilon) {
            theMinAxis = (theMinBox - theOriginAxis) / theDirectionAxis;
            theMaxAxis = (theMaxBox - theOriginAxis) / theDirectionAxis;
        } else if (theDirectionAxis < -kEpsilon) {
            theMinAxis = (theMaxBox - theOriginAxis) / theDirectionAxis;
            theMaxAxis = (theMinBox - theOriginAxis) / theDirectionAxis;
        } else if ((theOriginAxis < theMinBox || theOriginAxis > theMaxBox)
                   && inForceIntersect == false) {
            // Pickray is roughly parallel to the plane of the slab
            // so, if the origin is not in the range, we have no intersection
            return Empty();
        }

        // Shrink the intersections to find the closest hit
        theMinWinner = NVMax(theMinWinner, theMinAxis);
        theMaxWinner = NVMin(theMaxWinner, theMaxAxis);

        if ((theMinWinner > theMaxWinner || theMaxWinner < 0) && inForceIntersect == false)
            return Empty();
    }

    QT3DSVec3 scaledDir = theTransformedDirection * theMinWinner;
    QT3DSVec3 newPosInLocal = theTransformedOrigin + scaledDir;
    QT3DSVec3 newPosInGlobal = inGlobalTransform.transform(newPosInLocal);
    QT3DSVec3 cameraToLocal = m_Origin - newPosInGlobal;

    QT3DSF32 rayLengthSquared = cameraToLocal.magnitudeSquared();

    QT3DSF32 xRange = inBounds.maximum.x - inBounds.minimum.x;
    QT3DSF32 yRange = inBounds.maximum.y - inBounds.minimum.y;

    QT3DSVec2 relXY;
    relXY.x = (newPosInLocal[0] - inBounds.minimum.x) / xRange;
    relXY.y = (newPosInLocal[1] - inBounds.minimum.y) / yRange;

    return SRayIntersectionResult(rayLengthSquared, relXY);
}

Option<QT3DSVec2> SRay::GetRelative(const QT3DSMat44 &inGlobalTransform, const NVBounds3 &inBounds,
                                 SBasisPlanes::Enum inPlane) const
{
    QT3DSMat44 theOriginTransform = inGlobalTransform.getInverse();

    QT3DSVec3 theTransformedOrigin = theOriginTransform.transform(m_Origin);
    QT3DSF32 *outOriginTransformPtr(theOriginTransform.front());
    outOriginTransformPtr[12] = outOriginTransformPtr[13] = outOriginTransformPtr[14] = 0.0f;
    QT3DSVec3 theTransformedDirection = theOriginTransform.rotate(m_Direction);

    // The XY plane is going to be a plane with either positive or negative Z direction that runs
    // through
    QT3DSVec3 theDirection(0, 0, 1);
    QT3DSVec3 theRight(1, 0, 0);
    QT3DSVec3 theUp(0, 1, 0);
    switch (inPlane) {
    case SBasisPlanes::XY:
        break;
    case SBasisPlanes::XZ:
        theDirection = QT3DSVec3(0, 1, 0);
        theUp = QT3DSVec3(0, 0, 1);
        break;
    case SBasisPlanes::YZ:
        theDirection = QT3DSVec3(1, 0, 0);
        theRight = QT3DSVec3(0, 0, 1);
        break;
    }
    NVPlane thePlane(theDirection, theDirection.dot(theTransformedDirection) > 0.0f
                         ? theDirection.dot(inBounds.maximum)
                         : theDirection.dot(inBounds.minimum));

    SRay relativeRay(theTransformedOrigin, theTransformedDirection);
    Option<QT3DSVec3> localIsect = relativeRay.Intersect(thePlane);
    if (localIsect.hasValue()) {
        QT3DSF32 xRange = theRight.dot(inBounds.maximum) - theRight.dot(inBounds.minimum);
        QT3DSF32 yRange = theUp.dot(inBounds.maximum) - theUp.dot(inBounds.minimum);
        QT3DSF32 xOrigin = xRange / 2.0f + theRight.dot(inBounds.minimum);
        QT3DSF32 yOrigin = yRange / 2.0f + theUp.dot(inBounds.minimum);
        return QT3DSVec2((theRight.dot(*localIsect) - xOrigin) / xRange,
                      (theUp.dot(*localIsect) - yOrigin) / yRange);
    }
    return Empty();
}
