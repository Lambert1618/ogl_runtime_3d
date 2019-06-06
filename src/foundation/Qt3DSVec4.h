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

#ifndef QT3DS_FOUNDATION_QT3DS_VEC4_H
#define QT3DS_FOUNDATION_QT3DS_VEC4_H
/** \addtogroup foundation
@{
*/
#include "foundation/Qt3DSMath.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSAssert.h"

/**
\brief 4 Element vector class.

This is a vector class with public data members.
This is not nice but it has become such a standard that hiding the xyz data members
makes it difficult to reuse external code that assumes that these are public in the library.
The vector class can be made to use float or double precision by appropriately defining NVReal.
This has been chosen as a cleaner alternative to a template class.
*/
#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

class QT3DSVec4
{
public:
    /**
    \brief default constructor leaves data uninitialized.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4() {}

    /**
    \brief Assigns scalar parameter to all elements.

    Useful to initialize to zero or one.

    \param[in] a Value to assign to elements.
    */
    explicit QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4(NVReal a)
        : x(a)
        , y(a)
        , z(a)
        , w(a)
    {
    }

    /**
    \brief Initializes from 3 scalar parameters.

    \param[in] nx Value to initialize X component.
    \param[in] ny Value to initialize Y component.
    \param[in] nz Value to initialize Z component.
    \param[in] nw Value to initialize W component.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4(NVReal nx, NVReal ny, NVReal nz, NVReal nw)
        : x(nx)
        , y(ny)
        , z(nz)
        , w(nw)
    {
    }

    /**
    \brief Initializes from 3 scalar parameters.

    \param[in] v Value to initialize the X, Y, and Z components.
    \param[in] nw Value to initialize W component.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4(const QT3DSVec3 &v, NVReal nw)
        : x(v.x)
        , y(v.y)
        , z(v.z)
        , w(nw)
    {
    }

    /**
    \brief Initializes from an array of scalar parameters.

    \param[in] v Value to initialize with.
    */
    explicit QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4(const NVReal v[])
        : x(v[0])
        , y(v[1])
        , z(v[2])
        , w(v[3])
    {
    }

    /**
    \brief Copy ctor.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4(const QT3DSVec4 &v)
        : x(v.x)
        , y(v.y)
        , z(v.z)
        , w(v.w)
    {
    }

    // Operators

    /**
    \brief Assignment operator
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 &operator=(const QT3DSVec4 &p)
    {
        x = p.x;
        y = p.y;
        z = p.z;
        w = p.w;
        return *this;
    }

    /**
    \brief element access
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal &operator[](int index)
    {
        QT3DS_ASSERT(index >= 0 && index <= 3);
        return (&x)[index];
    }

    /**
    \brief element access
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE const NVReal &operator[](int index) const
    {
        QT3DS_ASSERT(index >= 0 && index <= 3);
        return (&x)[index];
    }

    /**
    \brief returns true if the two vectors are exactly equal.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool operator==(const QT3DSVec4 &v) const
    {
        return x == v.x && y == v.y && z == v.z && w == v.w;
    }

    /**
    \brief returns true if the two vectors are not exactly equal.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool operator!=(const QT3DSVec4 &v) const
    {
        return x != v.x || y != v.y || z != v.z || w != v.w;
    }

    /**
    \brief tests for exact zero vector
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool isZero() const { return x == 0 && y == 0 && z == 0 && w == 0; }

    /**
    \brief returns true if all 3 elems of the vector are finite (not NAN or INF, etc.)
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool isFinite() const
    {
        return NVIsFinite(x) && NVIsFinite(y) && NVIsFinite(z) && NVIsFinite(w);
    }

    /**
    \brief is normalized - used by API parameter validation
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool isNormalized() const
    {
        const float unitTolerance = NVReal(1e-4);
        return isFinite() && NVAbs(magnitude() - 1) < unitTolerance;
    }

    /**
    \brief returns the squared magnitude

    Avoids calling NVSqrt()!
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal magnitudeSquared() const
    {
        return x * x + y * y + z * z + w * w;
    }

    /**
    \brief returns the magnitude
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal magnitude() const { return NVSqrt(magnitudeSquared()); }

    /**
    \brief negation
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 operator-() const { return QT3DSVec4(-x, -y, -z, -w); }

    /**
    \brief vector addition
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 operator+(const QT3DSVec4 &v) const
    {
        return QT3DSVec4(x + v.x, y + v.y, z + v.z, w + v.w);
    }

    /**
    \brief vector difference
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 operator-(const QT3DSVec4 &v) const
    {
        return QT3DSVec4(x - v.x, y - v.y, z - v.z, w - v.w);
    }

    /**
    \brief scalar post-multiplication
    */

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 operator*(NVReal f) const
    {
        return QT3DSVec4(x * f, y * f, z * f, w * f);
    }

    /**
    \brief scalar division
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 operator/(NVReal f) const
    {
        f = NVReal(1) / f;
        return QT3DSVec4(x * f, y * f, z * f, w * f);
    }

    /**
    \brief vector addition
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 &operator+=(const QT3DSVec4 &v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }

    /**
    \brief vector difference
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 &operator-=(const QT3DSVec4 &v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }

    /**
    \brief scalar multiplication
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 &operator*=(NVReal f)
    {
        x *= f;
        y *= f;
        z *= f;
        w *= f;
        return *this;
    }
    /**
    \brief scalar division
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 &operator/=(NVReal f)
    {
        f = 1.0f / f;
        x *= f;
        y *= f;
        z *= f;
        w *= f;
        return *this;
    }

    /**
    \brief returns the scalar product of this and other.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal dot(const QT3DSVec4 &v) const
    {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    /** return a unit vector */

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 getNormalized() const
    {
        NVReal m = magnitudeSquared();
        return m > 0 ? *this * NVRecipSqrt(m) : QT3DSVec4(0, 0, 0, 0);
    }

    /**
    \brief normalizes the vector in place
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal normalize()
    {
        NVReal m = magnitude();
        if (m > 0)
            *this /= m;
        return m;
    }

    /**
    \brief a[i] * b[i], for all i.
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 multiply(const QT3DSVec4 &a) const
    {
        return QT3DSVec4(x * a.x, y * a.y, z * a.z, w * a.w);
    }

    /**
    \brief element-wise minimum
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 minimum(const QT3DSVec4 &v) const
    {
        return QT3DSVec4(NVMin(x, v.x), NVMin(y, v.y), NVMin(z, v.z), NVMin(w, v.w));
    }

    /**
    \brief element-wise maximum
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 maximum(const QT3DSVec4 &v) const
    {
        return QT3DSVec4(NVMax(x, v.x), NVMax(y, v.y), NVMax(z, v.z), NVMax(w, v.w));
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 getXYZ() const { return QT3DSVec3(x, y, z); }

    /**
    \brief set vector elements to zero
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE void setZero() { x = y = z = w = NVReal(0); }

    NVReal x, y, z, w;
};

QT3DS_CUDA_CALLABLE static QT3DS_INLINE QT3DSVec4 operator*(NVReal f, const QT3DSVec4 &v)
{
    return QT3DSVec4(f * v.x, f * v.y, f * v.z, f * v.w);
}

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_VEC4_H
