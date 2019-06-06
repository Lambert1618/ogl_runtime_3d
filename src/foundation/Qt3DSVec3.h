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

#ifndef QT3DS_FOUNDATION_QT3DS_VEC3_H
#define QT3DS_FOUNDATION_QT3DS_VEC3_H

/** \addtogroup foundation
@{
*/

#include "foundation/Qt3DSMath.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

/**
\brief 3 Element vector class.

This is a vector class with public data members.
This is not nice but it has become such a standard that hiding the xyz data members
makes it difficult to reuse external code that assumes that these are public in the library.
The vector class can be made to use float or double precision by appropriately defining NVReal.
This has been chosen as a cleaner alternative to a template class.
*/
class QT3DSVec3
{
public:
    /**
    \brief default constructor leaves data uninitialized.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3() {}

    /**
    \brief Assigns scalar parameter to all elements.

    Useful to initialize to zero or one.

    \param[in] a Value to assign to elements.
    */
    explicit QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3(NVReal a)
        : x(a)
        , y(a)
        , z(a)
    {
    }

    /**
    \brief Initializes from 3 scalar parameters.

    \param[in] nx Value to initialize X component.
    \param[in] ny Value to initialize Y component.
    \param[in] nz Value to initialize Z component.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3(NVReal nx, NVReal ny, NVReal nz)
        : x(nx)
        , y(ny)
        , z(nz)
    {
    }

    /**
    \brief Copy ctor.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3(const QT3DSVec3 &v)
        : x(v.x)
        , y(v.y)
        , z(v.z)
    {
    }

    // Operators

    /**
    \brief Assignment operator
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 &operator=(const QT3DSVec3 &p)
    {
        x = p.x;
        y = p.y;
        z = p.z;
        return *this;
    }

    /**
    \brief element access
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal &operator[](int index)
    {
        QT3DS_ASSERT(index >= 0 && index <= 2);
        return (&x)[index];
    }

    /**
    \brief element access
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE const NVReal &operator[](int index) const
    {
        QT3DS_ASSERT(index >= 0 && index <= 2);
        return (&x)[index];
    }

    /**
    \brief returns true if the two vectors are exactly equal.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool operator==(const QT3DSVec3 &v) const
    {
        return x == v.x && y == v.y && z == v.z;
    }

    /**
    \brief returns true if the two vectors are not exactly equal.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool operator!=(const QT3DSVec3 &v) const
    {
        return x != v.x || y != v.y || z != v.z;
    }

    /**
    \brief tests for exact zero vector
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isZero() const
    {
        return x == 0.0f && y == 0.0f && z == 0.0f;
    }

    /**
    \brief returns true if all 3 elems of the vector are finite (not NAN or INF, etc.)
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool isFinite() const
    {
        return NVIsFinite(x) && NVIsFinite(y) && NVIsFinite(z);
    }

    /**
    \brief is normalized - used by API parameter validation
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isNormalized() const
    {
        const float unitTolerance = NVReal(1e-4);
        return isFinite() && NVAbs(magnitude() - 1) < unitTolerance;
    }

    /**
    \brief returns the squared magnitude

    Avoids calling NVSqrt()!
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal magnitudeSquared() const
    {
        return x * x + y * y + z * z;
    }

    /**
    \brief returns the magnitude
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal magnitude() const { return NVSqrt(magnitudeSquared()); }

    /**
    \brief negation
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 operator-() const { return QT3DSVec3(-x, -y, -z); }

    /**
    \brief vector addition
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 operator+(const QT3DSVec3 &v) const
    {
        return QT3DSVec3(x + v.x, y + v.y, z + v.z);
    }

    /**
    \brief vector difference
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 operator-(const QT3DSVec3 &v) const
    {
        return QT3DSVec3(x - v.x, y - v.y, z - v.z);
    }

    /**
    \brief scalar post-multiplication
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 operator*(NVReal f) const
    {
        return QT3DSVec3(x * f, y * f, z * f);
    }

    /**
    \brief scalar division
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 operator/(NVReal f) const
    {
        f = NVReal(1) / f; // PT: inconsistent notation with operator /=
        return QT3DSVec3(x * f, y * f, z * f);
    }

    /**
    \brief vector addition
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 &operator+=(const QT3DSVec3 &v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    /**
    \brief vector difference
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 &operator-=(const QT3DSVec3 &v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    /**
    \brief scalar multiplication
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 &operator*=(NVReal f)
    {
        x *= f;
        y *= f;
        z *= f;
        return *this;
    }
    /**
    \brief scalar division
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 &operator/=(NVReal f)
    {
        f = 1.0f / f; // PT: inconsistent notation with operator /
        x *= f;
        y *= f;
        z *= f;
        return *this;
    }

    /**
    \brief returns the scalar product of this and other.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal dot(const QT3DSVec3 &v) const
    {
        return x * v.x + y * v.y + z * v.z;
    }

    /**
    \brief cross product
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 cross(const QT3DSVec3 &v) const
    {
        return QT3DSVec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }

    /** return a unit vector */

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 getNormalized() const
    {
        const NVReal m = magnitudeSquared();
        return m > 0 ? *this * NVRecipSqrt(m) : QT3DSVec3(0, 0, 0);
    }

    /**
    \brief normalizes the vector in place
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal normalize()
    {
        const NVReal m = magnitude();
        if (m > 0)
            *this /= m;
        return m;
    }

    /**
    \brief normalizes the vector in place. Does nothing if vector magnitude is under
    QT3DS_NORMALIZATION_EPSILON.
    Returns vector magnitude if >= QT3DS_NORMALIZATION_EPSILON and 0.0f otherwise.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal normalizeSafe()
    {
        const NVReal mag = magnitude();
        if (mag < QT3DS_NORMALIZATION_EPSILON)
            return 0.0f;
        *this *= NVReal(1) / mag;
        return mag;
    }

    /**
    \brief normalizes the vector in place. Asserts if vector magnitude is under
    QT3DS_NORMALIZATION_EPSILON.
    returns vector magnitude.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal normalizeFast()
    {
        const NVReal mag = magnitude();
        QT3DS_ASSERT(mag >= QT3DS_NORMALIZATION_EPSILON);
        *this *= NVReal(1) / mag;
        return mag;
    }

    /**
    \brief a[i] * b[i], for all i.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 multiply(const QT3DSVec3 &a) const
    {
        return QT3DSVec3(x * a.x, y * a.y, z * a.z);
    }

    /**
    \brief element-wise minimum
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 minimum(const QT3DSVec3 &v) const
    {
        return QT3DSVec3(NVMin(x, v.x), NVMin(y, v.y), NVMin(z, v.z));
    }

    /**
    \brief returns MIN(x, y, z);
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float minElement() const { return NVMin(x, NVMin(y, z)); }

    /**
    \brief element-wise maximum
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 maximum(const QT3DSVec3 &v) const
    {
        return QT3DSVec3(NVMax(x, v.x), NVMax(y, v.y), NVMax(z, v.z));
    }

    /**
    \brief returns MAX(x, y, z);
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float maxElement() const { return NVMax(x, NVMax(y, z)); }

    NVReal x, y, z;
};

QT3DS_CUDA_CALLABLE static QT3DS_FORCE_INLINE QT3DSVec3 operator*(NVReal f, const QT3DSVec3 &v)
{
    return QT3DSVec3(f * v.x, f * v.y, f * v.z);
}

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_VEC3_H
