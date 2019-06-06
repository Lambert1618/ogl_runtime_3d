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

#ifndef QT3DS_FOUNDATION_QT3DS_TRANSFORM_H
#define QT3DS_FOUNDATION_QT3DS_TRANSFORM_H
/** \addtogroup foundation
  @{
*/

#include "foundation/Qt3DSQuat.h"
#include "foundation/Qt3DSPlane.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

/*!
\brief class representing a rigid euclidean transform as a quaternion and a vector
*/

class NVTransform
{
public:
    QT3DSQuat q;
    QT3DSVec3 p;

    //#define PXTRANSFORM_DEFAULT_CONSTRUCT_NAN

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVTransform()
#ifdef PXTRANSFORM_DEFAULT_CONSTRUCT_IDENTITY
        : q(0, 0, 0, 1)
        , p(0, 0, 0)
#elif defined(PXTRANSFORM_DEFAULT_CONSTRUCT_NAN)
#define invalid NVSqrt(-1.0f)
        : q(invalid, invalid, invalid, invalid)
        , p(invalid, invalid, invalid)
#undef invalid
#endif
    {
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE explicit NVTransform(const QT3DSVec3 &position)
        : q(0, 0, 0, 1)
        , p(position)
    {
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE explicit NVTransform(const QT3DSQuat &orientation)
        : q(orientation)
        , p(0, 0, 0)
    {
        QT3DS_ASSERT(orientation.isSane());
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVTransform(const QT3DSVec3 &p0, const QT3DSQuat &q0)
        : q(q0)
        , p(p0)
    {
        QT3DS_ASSERT(q0.isSane());
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE explicit NVTransform(const QT3DSMat44 &m); // defined in Qt3DSMat44.h

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVTransform operator*(const NVTransform &x) const
    {
        QT3DS_ASSERT(x.isSane());
        return transform(x);
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVTransform getInverse() const
    {
        QT3DS_ASSERT(isFinite());
        return NVTransform(q.rotateInv(-p), q.getConjugate());
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 transform(const QT3DSVec3 &input) const
    {
        QT3DS_ASSERT(isFinite());
        return q.rotate(input) + p;
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 transformInv(const QT3DSVec3 &input) const
    {
        QT3DS_ASSERT(isFinite());
        return q.rotateInv(input - p);
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 rotate(const QT3DSVec3 &input) const
    {
        QT3DS_ASSERT(isFinite());
        return q.rotate(input);
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 rotateInv(const QT3DSVec3 &input) const
    {
        QT3DS_ASSERT(isFinite());
        return q.rotateInv(input);
    }

    //! Transform transform to parent (returns compound transform: first src, then *this)
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVTransform transform(const NVTransform &src) const
    {
        QT3DS_ASSERT(src.isSane());
        QT3DS_ASSERT(isSane());
        // src = [srct, srcr] -> [r*srct + t, r*srcr]
        return NVTransform(q.rotate(src.p) + p, q * src.q);
    }

    /**
    \brief returns true if finite and q is a unit quaternion
    */

    QT3DS_CUDA_CALLABLE bool isValid() const { return p.isFinite() && q.isFinite() && q.isUnit(); }

    /**
    \brief returns true if finite and quat magnitude is reasonably close to unit to allow for some
    accumulation of error vs isValid
    */

    QT3DS_CUDA_CALLABLE bool isSane() const { return isFinite() && q.isSane(); }

    /**
    \brief returns true if all elems are finite (not NAN or INF, etc.)
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isFinite() const { return p.isFinite() && q.isFinite(); }

    //! Transform transform from parent (returns compound transform: first src, then this->inverse)
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVTransform transformInv(const NVTransform &src) const
    {
        QT3DS_ASSERT(src.isSane());
        QT3DS_ASSERT(isFinite());
        // src = [srct, srcr] -> [r^-1*(srct-t), r^-1*srcr]
        QT3DSQuat qinv = q.getConjugate();
        return NVTransform(qinv.rotate(src.p - p), qinv * src.q);
    }

    QT3DS_CUDA_CALLABLE static QT3DS_FORCE_INLINE NVTransform createIdentity()
    {
        return NVTransform(QT3DSVec3(0));
    }

    /**
    \brief transform plane
    */

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVPlane transform(const NVPlane &plane) const
    {
        QT3DSVec3 transformedNormal = rotate(plane.n);
        return NVPlane(transformedNormal, plane.d - p.dot(transformedNormal));
    }

    /**
    \brief inverse-transform plane
    */

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVPlane inverseTransform(const NVPlane &plane) const
    {
        QT3DSVec3 transformedNormal = rotateInv(plane.n);
        return NVPlane(transformedNormal, plane.d + p.dot(plane.n));
    }
};

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_TRANSFORM_H
