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

#ifndef QT3DS_FOUNDATION_QT3DS_PLANE_H
#define QT3DS_FOUNDATION_QT3DS_PLANE_H

/** \addtogroup foundation
@{
*/

#include "foundation/Qt3DSMath.h"
#include "foundation/Qt3DSVec3.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

/**
\brief Representation of a plane.

 Plane equation used: n.dot(v) + d = 0
*/
class NVPlane
{
public:
    /**
    \brief Constructor
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVPlane() {}

    /**
    \brief Constructor from a normal and a distance
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVPlane(NVReal nx, NVReal ny, NVReal nz, NVReal distance)
        : n(nx, ny, nz)
        , d(distance)
    {
    }

    /**
    \brief Constructor from a normal and a distance
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVPlane(const QT3DSVec3 &normal, NVReal distance)
        : n(normal)
        , d(distance)
    {
    }

    /**
    \brief Constructor from a point on the plane and a normal
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVPlane(const QT3DSVec3 &point, const QT3DSVec3 &normal)
        : n(normal)
        , d(-point.dot(n)) // p satisfies normal.dot(p) + d = 0
    {
    }

    /**
    \brief Constructor from three points
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVPlane(const QT3DSVec3 &p0, const QT3DSVec3 &p1, const QT3DSVec3 &p2)
    {
        n = (p1 - p0).cross(p2 - p0).getNormalized();
        d = -p0.dot(n);
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal distance(const QT3DSVec3 &p) const { return p.dot(n) + d; }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool contains(const QT3DSVec3 &p) const
    {
        return NVAbs(distance(p)) < (1.0e-7f);
    }

    /**
    \brief projects p into the plane
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 project(const QT3DSVec3 &p) const
    {
        return p - n * distance(p);
    }

    /**
    \brief find an arbitrary point in the plane
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 pointInPlane() const { return -n * d; }

    /**
    \brief equivalent plane with unit normal
    */

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void normalize()
    {
        NVReal denom = 1.0f / n.magnitude();
        n *= denom;
        d *= denom;
    }

    QT3DSVec3 n; //!< The normal to the plane
    NVReal d; //!< The distance from the origin
};

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif
