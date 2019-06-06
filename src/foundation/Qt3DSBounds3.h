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

#ifndef QT3DS_FOUNDATION_QT3DS_BOUNDS3_H
#define QT3DS_FOUNDATION_QT3DS_BOUNDS3_H

/** \addtogroup foundation
@{
*/

#include "foundation/Qt3DSTransform.h"
#include "foundation/Qt3DSMat33.h"
#include "foundation/Qt3DSMat44.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

typedef QT3DSVec3 TNVBounds2BoxPoints[8];

/**
\brief Class representing 3D range or axis aligned bounding box.

Stored as minimum and maximum extent corners. Alternate representation
would be center and dimensions.
May be empty or nonempty. If not empty, minimum <= maximum has to hold.
*/
class NVBounds3
{
public:
    /**
    \brief Default constructor, not performing any initialization for performance reason.
    \remark Use empty() function below to construct empty bounds.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3() {}

    /**
    \brief Construct from two bounding points
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3(const QT3DSVec3 &minimum, const QT3DSVec3 &maximum);

    /**
    \brief Return empty bounds.
    */
    static QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3 empty();

    /**
    \brief returns the AABB containing v0 and v1.
    \param v0 first point included in the AABB.
    \param v1 second point included in the AABB.
    */
    static QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3 boundsOfPoints(const QT3DSVec3 &v0,
                                                                           const QT3DSVec3 &v1);

    /**
    \brief returns the AABB from center and extents vectors.
    \param center Center vector
    \param extent Extents vector
    */
    static QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3 centerExtents(const QT3DSVec3 &center,
                                                                          const QT3DSVec3 &extent);

    /**
    \brief Construct from center, extent, and (not necessarily orthogonal) basis
    */
    static QT3DS_CUDA_CALLABLE QT3DS_INLINE NVBounds3 basisExtent(const QT3DSVec3 &center,
                                                                  const QT3DSMat33 &basis,
                                                                  const QT3DSVec3 &extent);

    /**
    \brief Construct from pose and extent
    */
    static QT3DS_CUDA_CALLABLE QT3DS_INLINE NVBounds3 poseExtent(const NVTransform &pose,
                                                                 const QT3DSVec3 &extent);

    /**
    \brief gets the transformed bounds of the passed AABB (resulting in a bigger AABB).
    \param[in] matrix Transform to apply, can contain scaling as well
    \param[in] bounds The bounds to transform.
    */
    static QT3DS_CUDA_CALLABLE QT3DS_INLINE NVBounds3 transform(const QT3DSMat33 &matrix,
                                                                const NVBounds3 &bounds);

    /**
    \brief gets the transformed bounds of the passed AABB (resulting in a bigger AABB).
    \param[in] transform Transform to apply, can contain scaling as well
    \param[in] bounds The bounds to transform.
    */
    static QT3DS_CUDA_CALLABLE QT3DS_INLINE NVBounds3 transform(const NVTransform &transform,
                                                                const NVBounds3 &bounds);

    /**
    \brief Sets empty to true
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void setEmpty();

    /**
    \brief Sets infinite bounds
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void setInfinite();

    /**
    \brief expands the volume to include v
    \param v Point to expand to.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void include(const QT3DSVec3 &v);

    /**
    \brief expands the volume to include b.
    \param b Bounds to perform union with.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void include(const NVBounds3 &b);

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isEmpty() const;

    /**
    \brief indicates whether the intersection of this and b is empty or not.
    \param b Bounds to test for intersection.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool intersects(const NVBounds3 &b) const;

    /**
     \brief computes the 1D-intersection between two AABBs, on a given axis.
     \param	a		the other AABB
     \param	axis	the axis (0, 1, 2)
     */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool intersects1D(const NVBounds3 &a, QT3DSU32 axis) const;

    /**
    \brief indicates if these bounds contain v.
    \param v Point to test against bounds.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool contains(const QT3DSVec3 &v) const;

    /**
     \brief	checks a box is inside another box.
     \param	box		the other AABB
     */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isInside(const NVBounds3 &box) const;

    /**
    \brief returns the center of this axis aligned box.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 getCenter() const;

    /**
    \brief get component of the box's center along a given axis
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float getCenter(QT3DSU32 axis) const;

    /**
    \brief get component of the box's extents along a given axis
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float getExtents(QT3DSU32 axis) const;

    /**
    \brief returns the dimensions (width/height/depth) of this axis aligned box.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 getDimensions() const;

    /**
    \brief returns the extents, which are half of the width/height/depth.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 getExtents() const;

    /**
    \brief scales the AABB.
    \param scale Factor to scale AABB by.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void scale(QT3DSF32 scale);

    /**
    fattens the AABB in all 3 dimensions by the given distance.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void fatten(NVReal distance);

    /**
    checks that the AABB values are not NaN
    */

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isFinite() const;

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void expand(TNVBounds2BoxPoints &outPoints) const;

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void transform(const QT3DSMat44 &inMatrix);

    QT3DSVec3 minimum, maximum;
};

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3::NVBounds3(const QT3DSVec3 &minimum, const QT3DSVec3 &maximum)
    : minimum(minimum)
    , maximum(maximum)
{
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3 NVBounds3::empty()
{
    return NVBounds3(QT3DSVec3(QT3DS_MAX_REAL), QT3DSVec3(-QT3DS_MAX_REAL));
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVBounds3::isFinite() const
{
    return minimum.isFinite() && maximum.isFinite();
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3 NVBounds3::boundsOfPoints(const QT3DSVec3 &v0,
                                                                           const QT3DSVec3 &v1)
{
    return NVBounds3(v0.minimum(v1), v0.maximum(v1));
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVBounds3 NVBounds3::centerExtents(const QT3DSVec3 &center,
                                                                          const QT3DSVec3 &extent)
{
    return NVBounds3(center - extent, center + extent);
}

QT3DS_CUDA_CALLABLE QT3DS_INLINE NVBounds3 NVBounds3::basisExtent(const QT3DSVec3 &center,
                                                                  const QT3DSMat33 &basis,
                                                                  const QT3DSVec3 &extent)
{
    // extended basis vectors
    QT3DSVec3 c0 = basis.column0 * extent.x;
    QT3DSVec3 c1 = basis.column1 * extent.y;
    QT3DSVec3 c2 = basis.column2 * extent.z;

    QT3DSVec3 w;
    // find combination of base vectors that produces max. distance for each component = sum of
    // abs()
    w.x = NVAbs(c0.x) + NVAbs(c1.x) + NVAbs(c2.x);
    w.y = NVAbs(c0.y) + NVAbs(c1.y) + NVAbs(c2.y);
    w.z = NVAbs(c0.z) + NVAbs(c1.z) + NVAbs(c2.z);

    return NVBounds3(center - w, center + w);
}

QT3DS_CUDA_CALLABLE QT3DS_INLINE NVBounds3 NVBounds3::poseExtent(const NVTransform &pose,
                                                                 const QT3DSVec3 &extent)
{
    return basisExtent(pose.p, QT3DSMat33(pose.q), extent);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void NVBounds3::setEmpty()
{
    minimum = QT3DSVec3(QT3DS_MAX_REAL);
    maximum = QT3DSVec3(-QT3DS_MAX_REAL);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void NVBounds3::setInfinite()
{
    minimum = QT3DSVec3(-QT3DS_MAX_REAL);
    maximum = QT3DSVec3(QT3DS_MAX_REAL);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void NVBounds3::include(const QT3DSVec3 &v)
{
    QT3DS_ASSERT(isFinite());
    minimum = minimum.minimum(v);
    maximum = maximum.maximum(v);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void NVBounds3::include(const NVBounds3 &b)
{
    QT3DS_ASSERT(isFinite());
    minimum = minimum.minimum(b.minimum);
    maximum = maximum.maximum(b.maximum);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVBounds3::isEmpty() const
{
    QT3DS_ASSERT(isFinite());
    // Consistency condition for (Min, Max) boxes: minimum < maximum
    return minimum.x > maximum.x || minimum.y > maximum.y || minimum.z > maximum.z;
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVBounds3::intersects(const NVBounds3 &b) const
{
    QT3DS_ASSERT(isFinite() && b.isFinite());
    return !(b.minimum.x > maximum.x || minimum.x > b.maximum.x || b.minimum.y > maximum.y
             || minimum.y > b.maximum.y || b.minimum.z > maximum.z || minimum.z > b.maximum.z);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVBounds3::intersects1D(const NVBounds3 &a, QT3DSU32 axis) const
{
    QT3DS_ASSERT(isFinite() && a.isFinite());
    return maximum[axis] >= a.minimum[axis] && a.maximum[axis] >= minimum[axis];
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVBounds3::contains(const QT3DSVec3 &v) const
{
    QT3DS_ASSERT(isFinite());

    return !(v.x < minimum.x || v.x > maximum.x || v.y < minimum.y || v.y > maximum.y
             || v.z < minimum.z || v.z > maximum.z);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVBounds3::isInside(const NVBounds3 &box) const
{
    QT3DS_ASSERT(isFinite() && box.isFinite());
    if (box.minimum.x > minimum.x)
        return false;
    if (box.minimum.y > minimum.y)
        return false;
    if (box.minimum.z > minimum.z)
        return false;
    if (box.maximum.x < maximum.x)
        return false;
    if (box.maximum.y < maximum.y)
        return false;
    if (box.maximum.z < maximum.z)
        return false;
    return true;
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 NVBounds3::getCenter() const
{
    QT3DS_ASSERT(isFinite());
    return (minimum + maximum) * NVReal(0.5);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float NVBounds3::getCenter(QT3DSU32 axis) const
{
    QT3DS_ASSERT(isFinite());
    return (minimum[axis] + maximum[axis]) * NVReal(0.5);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float NVBounds3::getExtents(QT3DSU32 axis) const
{
    QT3DS_ASSERT(isFinite());
    return (maximum[axis] - minimum[axis]) * NVReal(0.5);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 NVBounds3::getDimensions() const
{
    QT3DS_ASSERT(isFinite());
    return maximum - minimum;
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 NVBounds3::getExtents() const
{
    QT3DS_ASSERT(isFinite());
    return getDimensions() * NVReal(0.5);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void NVBounds3::scale(QT3DSF32 scale)
{
    QT3DS_ASSERT(isFinite());
    *this = centerExtents(getCenter(), getExtents() * scale);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void NVBounds3::fatten(NVReal distance)
{
    QT3DS_ASSERT(isFinite());
    minimum.x -= distance;
    minimum.y -= distance;
    minimum.z -= distance;

    maximum.x += distance;
    maximum.y += distance;
    maximum.z += distance;
}

QT3DS_CUDA_CALLABLE QT3DS_INLINE NVBounds3 NVBounds3::transform(const QT3DSMat33 &matrix,
                                                                const NVBounds3 &bounds)
{
    QT3DS_ASSERT(bounds.isFinite());
    return bounds.isEmpty() ? bounds : NVBounds3::basisExtent(matrix * bounds.getCenter(), matrix,
                                                              bounds.getExtents());
}

QT3DS_CUDA_CALLABLE QT3DS_INLINE NVBounds3 NVBounds3::transform(const NVTransform &transform,
                                                                const NVBounds3 &bounds)
{
    QT3DS_ASSERT(bounds.isFinite());
    return bounds.isEmpty() ? bounds
                            : NVBounds3::basisExtent(transform.transform(bounds.getCenter()),
                                                     QT3DSMat33(transform.q), bounds.getExtents());
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void NVBounds3::expand(TNVBounds2BoxPoints &outPoints) const
{
    if (isEmpty()) {
        for (QT3DSU32 idx = 0; idx < 8; ++idx)
            outPoints[idx] = QT3DSVec3(0, 0, 0);
    } else {
        // Min corner of box
        outPoints[0] = QT3DSVec3(minimum[0], minimum[1], minimum[2]);
        outPoints[1] = QT3DSVec3(maximum[0], minimum[1], minimum[2]);
        outPoints[2] = QT3DSVec3(minimum[0], maximum[1], minimum[2]);
        outPoints[3] = QT3DSVec3(minimum[0], minimum[1], maximum[2]);

        // Max corner of box
        outPoints[4] = QT3DSVec3(maximum[0], maximum[1], maximum[2]);
        outPoints[5] = QT3DSVec3(minimum[0], maximum[1], maximum[2]);
        outPoints[6] = QT3DSVec3(maximum[0], minimum[1], maximum[2]);
        outPoints[7] = QT3DSVec3(maximum[0], maximum[1], minimum[2]);
    }
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void NVBounds3::transform(const QT3DSMat44 &inMatrix)
{
    if (!isEmpty()) {
        TNVBounds2BoxPoints thePoints;
        expand(thePoints);
        setEmpty();
        for (QT3DSU32 idx = 0; idx < 8; ++idx)
            include(inMatrix.transform(thePoints[idx]));
    }
}

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_BOUNDS3_H
