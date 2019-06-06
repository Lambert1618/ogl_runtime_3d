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

#ifndef QT3DS_FOUNDATION_QT3DS_QUAT_H
#define QT3DS_FOUNDATION_QT3DS_QUAT_H

/** \addtogroup foundation
@{
*/

#include "foundation/Qt3DSVec3.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

/**
\brief This is a quaternion class. For more information on quaternion mathematics
consult a mathematics source on complex numbers.

*/

class QT3DSQuat
{
public:
    /**
    \brief Default constructor, does not do any initialization.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat() {}

    /**
    \brief Constructor.  Take note of the order of the elements!
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat(NVReal nx, NVReal ny, NVReal nz, NVReal nw)
        : x(nx)
        , y(ny)
        , z(nz)
        , w(nw)
    {
    }

    /**
    \brief Creates from angle-axis representation.

    Axis must be normalized!

    Angle is in radians!

    <b>Unit:</b> Radians
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSQuat(NVReal angleRadians, const QT3DSVec3 &unitAxis)
    {
        QT3DS_ASSERT(NVAbs(1.0f - unitAxis.magnitude()) < 1e-3f);
        const NVReal a = angleRadians * 0.5f;
        const NVReal s = NVSin(a);
        w = NVCos(a);
        x = unitAxis.x * s;
        y = unitAxis.y * s;
        z = unitAxis.z * s;
    }

    /**
    \brief Copy ctor.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat(const QT3DSQuat &v)
        : x(v.x)
        , y(v.y)
        , z(v.z)
        , w(v.w)
    {
    }

    /**
    \brief Creates from orientation matrix.

    \param[in] m Rotation matrix to extract quaternion from.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE explicit QT3DSQuat(const QT3DSMat33 &m); /* defined in Qt3DSMat33.h */

    /**
    \brief returns true if all elements are finite (not NAN or INF, etc.)
    */
    QT3DS_CUDA_CALLABLE bool isFinite() const
    {
        return NVIsFinite(x) && NVIsFinite(y) && NVIsFinite(z) && NVIsFinite(w);
    }

    /**
    \brief returns true if finite and magnitude is close to unit
    */

    QT3DS_CUDA_CALLABLE bool isUnit() const
    {
        const NVReal unitTolerance = NVReal(1e-4);
        return isFinite() && NVAbs(magnitude() - 1) < unitTolerance;
    }

    /**
    \brief returns true if finite and magnitude is reasonably close to unit to allow for some
    accumulation of error vs isValid
    */

    QT3DS_CUDA_CALLABLE bool isSane() const
    {
        const NVReal unitTolerance = NVReal(1e-2);
        return isFinite() && NVAbs(magnitude() - 1) < unitTolerance;
    }

    /**
    \brief converts this quaternion to angle-axis representation
    */

    QT3DS_CUDA_CALLABLE QT3DS_INLINE void toRadiansAndUnitAxis(NVReal &angle, QT3DSVec3 &axis) const
    {
        const NVReal quatEpsilon = (NVReal(1.0e-8f));
        const NVReal s2 = x * x + y * y + z * z;
        if (s2 < quatEpsilon * quatEpsilon) // can't extract a sensible axis
        {
            angle = 0;
            axis = QT3DSVec3(1, 0, 0);
        } else {
            const NVReal s = NVRecipSqrt(s2);
            axis = QT3DSVec3(x, y, z) * s;
            angle = w < quatEpsilon ? NVPi : NVAtan2(s2 * s, w) * 2;
        }
    }

    /**
    \brief Gets the angle between this quat and the identity quaternion.

    <b>Unit:</b> Radians
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal getAngle() const { return NVAcos(w) * NVReal(2); }

    /**
    \brief Gets the angle between this quat and the argument

    <b>Unit:</b> Radians
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal getAngle(const QT3DSQuat &q) const
    {
        return NVAcos(dot(q)) * NVReal(2);
    }

    /**
    \brief This is the squared 4D vector length, should be 1 for unit quaternions.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal magnitudeSquared() const
    {
        return x * x + y * y + z * z + w * w;
    }

    /**
    \brief returns the scalar product of this and other.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal dot(const QT3DSQuat &v) const
    {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSQuat getNormalized() const
    {
        const NVReal s = 1.0f / magnitude();
        return QT3DSQuat(x * s, y * s, z * s, w * s);
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE float magnitude() const { return NVSqrt(magnitudeSquared()); }

    // modifiers:
    /**
    \brief maps to the closest unit quaternion.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal normalize() // convert this QT3DSQuat to a unit quaternion
    {
        const NVReal mag = magnitude();
        if (mag) {
            const NVReal imag = NVReal(1) / mag;

            x *= imag;
            y *= imag;
            z *= imag;
            w *= imag;
        }
        return mag;
    }

    /*
    \brief returns the conjugate.

    \note for unit quaternions, this is the inverse.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSQuat getConjugate() const { return QT3DSQuat(-x, -y, -z, w); }

    /*
    \brief returns imaginary part.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 getImaginaryPart() const { return QT3DSVec3(x, y, z); }

    /** brief computes rotation of x-axis */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 getBasisVector0() const
    {
        //		return rotate(QT3DSVec3(1,0,0));
        const QT3DSF32 x2 = x * 2.0f;
        const QT3DSF32 w2 = w * 2.0f;
        return QT3DSVec3((w * w2) - 1.0f + x * x2, (z * w2) + y * x2, (-y * w2) + z * x2);
    }

    /** brief computes rotation of y-axis */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 getBasisVector1() const
    {
        //		return rotate(QT3DSVec3(0,1,0));
        const QT3DSF32 y2 = y * 2.0f;
        const QT3DSF32 w2 = w * 2.0f;
        return QT3DSVec3((-z * w2) + x * y2, (w * w2) - 1.0f + y * y2, (x * w2) + z * y2);
    }

    /** brief computes rotation of z-axis */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 getBasisVector2() const
    {
        //		return rotate(QT3DSVec3(0,0,1));
        const QT3DSF32 z2 = z * 2.0f;
        const QT3DSF32 w2 = w * 2.0f;
        return QT3DSVec3((y * w2) + x * z2, (-x * w2) + y * z2, (w * w2) - 1.0f + z * z2);
    }

    /**
    rotates passed vec by this (assumed unitary)
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE const QT3DSVec3 rotate(const QT3DSVec3 &v) const
    //	QT3DS_CUDA_CALLABLE QT3DS_INLINE const QT3DSVec3 rotate(const QT3DSVec3& v) const
    {
        const QT3DSF32 vx = 2.0f * v.x;
        const QT3DSF32 vy = 2.0f * v.y;
        const QT3DSF32 vz = 2.0f * v.z;
        const QT3DSF32 w2 = w * w - 0.5f;
        const QT3DSF32 dot2 = (x * vx + y * vy + z * vz);
        return QT3DSVec3((vx * w2 + (y * vz - z * vy) * w + x * dot2),
                      (vy * w2 + (z * vx - x * vz) * w + y * dot2),
                      (vz * w2 + (x * vy - y * vx) * w + z * dot2));
        /*
        const QT3DSVec3 qv(x,y,z);
        return (v*(w*w-0.5f) + (qv.cross(v))*w + qv*(qv.dot(v)))*2;
        */
    }

    /**
    inverse rotates passed vec by this (assumed unitary)
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE const QT3DSVec3 rotateInv(const QT3DSVec3 &v) const
    //	QT3DS_CUDA_CALLABLE QT3DS_INLINE const QT3DSVec3 rotateInv(const QT3DSVec3& v) const
    {
        const QT3DSF32 vx = 2.0f * v.x;
        const QT3DSF32 vy = 2.0f * v.y;
        const QT3DSF32 vz = 2.0f * v.z;
        const QT3DSF32 w2 = w * w - 0.5f;
        const QT3DSF32 dot2 = (x * vx + y * vy + z * vz);
        return QT3DSVec3((vx * w2 - (y * vz - z * vy) * w + x * dot2),
                      (vy * w2 - (z * vx - x * vz) * w + y * dot2),
                      (vz * w2 - (x * vy - y * vx) * w + z * dot2));
        //		const QT3DSVec3 qv(x,y,z);
        //		return (v*(w*w-0.5f) - (qv.cross(v))*w + qv*(qv.dot(v)))*2;
    }

    /**
    \brief Assignment operator
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat &operator=(const QT3DSQuat &p)
    {
        x = p.x;
        y = p.y;
        z = p.z;
        w = p.w;
        return *this;
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat &operator*=(const QT3DSQuat &q)
    {
        const NVReal tx = w * q.x + q.w * x + y * q.z - q.y * z;
        const NVReal ty = w * q.y + q.w * y + z * q.x - q.z * x;
        const NVReal tz = w * q.z + q.w * z + x * q.y - q.x * y;

        w = w * q.w - q.x * x - y * q.y - q.z * z;
        x = tx;
        y = ty;
        z = tz;

        return *this;
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat &operator+=(const QT3DSQuat &q)
    {
        x += q.x;
        y += q.y;
        z += q.z;
        w += q.w;
        return *this;
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat &operator-=(const QT3DSQuat &q)
    {
        x -= q.x;
        y -= q.y;
        z -= q.z;
        w -= q.w;
        return *this;
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat &operator*=(const NVReal s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    /** quaternion multiplication */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSQuat operator*(const QT3DSQuat &q) const
    {
        return QT3DSQuat(w * q.x + q.w * x + y * q.z - q.y * z, w * q.y + q.w * y + z * q.x - q.z * x,
                      w * q.z + q.w * z + x * q.y - q.x * y, w * q.w - x * q.x - y * q.y - z * q.z);
    }

    /** quaternion addition */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat operator+(const QT3DSQuat &q) const
    {
        return QT3DSQuat(x + q.x, y + q.y, z + q.z, w + q.w);
    }

    /** quaternion subtraction */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat operator-() const { return QT3DSQuat(-x, -y, -z, -w); }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat operator-(const QT3DSQuat &q) const
    {
        return QT3DSQuat(x - q.x, y - q.y, z - q.z, w - q.w);
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSQuat operator*(NVReal r) const
    {
        return QT3DSQuat(x * r, y * r, z * r, w * r);
    }

    static QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSQuat createIdentity() { return QT3DSQuat(0, 0, 0, 1); }

    /** the quaternion elements */
    NVReal x, y, z, w;
};

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_QUAT_H
