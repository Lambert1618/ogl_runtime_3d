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

#ifndef QT3DS_FOUNDATION_QT3DS_VEC2_H
#define QT3DS_FOUNDATION_QT3DS_VEC2_H

/** \addtogroup foundation
@{
*/

#include "foundation/Qt3DSMath.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

/**
\brief 2 Element vector class.

This is a vector class with public data members.
This is not nice but it has become such a standard that hiding the xy data members
makes it difficult to reuse external code that assumes that these are public in the library.
The vector class can be made to use float or double precision by appropriately defining NVReal.
This has been chosen as a cleaner alternative to a template class.
*/
class QT3DSVec2
{
public:
    /**
    \brief default constructor leaves data uninitialized.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2() {}

    /**
    \brief Assigns scalar parameter to all elements.

    Useful to initialize to zero or one.

    \param[in] a Value to assign to elements.
    */
    explicit QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2(NVReal a)
        : x(a)
        , y(a)
    {
    }

    /**
    \brief Initializes from 2 scalar parameters.

    \param[in] nx Value to initialize X component.
    \param[in] ny Value to initialize Y component.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2(NVReal nx, NVReal ny)
        : x(nx)
        , y(ny)
    {
    }

    /**
    \brief Copy ctor.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2(const QT3DSVec2 &v)
        : x(v.x)
        , y(v.y)
    {
    }

    // Operators

    /**
    \brief Assignment operator
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 &operator=(const QT3DSVec2 &p)
    {
        x = p.x;
        y = p.y;
        return *this;
    }

    /**
    \brief element access
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal &operator[](int index)
    {
        QT3DS_ASSERT(index >= 0 && index <= 1);
        return (&x)[index];
    }

    /**
    \brief element access
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE const NVReal &operator[](int index) const
    {
        QT3DS_ASSERT(index >= 0 && index <= 1);
        return (&x)[index];
    }

    /**
    \brief returns true if the two vectors are exactly equal.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool operator==(const QT3DSVec2 &v) const
    {
        return x == v.x && y == v.y;
    }

    /**
    \brief returns true if the two vectors are not exactly equal.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool operator!=(const QT3DSVec2 &v) const
    {
        return x != v.x || y != v.y;
    }

    /**
    \brief tests for exact zero vector
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isZero() const { return x == 0.0f && y == 0.0f; }

    /**
    \brief returns true if all 2 elems of the vector are finite (not NAN or INF, etc.)
    */
    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool isFinite() const { return NVIsFinite(x) && NVIsFinite(y); }

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
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal magnitudeSquared() const { return x * x + y * y; }

    /**
    \brief returns the magnitude
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal magnitude() const { return NVSqrt(magnitudeSquared()); }

    /**
    \brief negation
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 operator-() const { return QT3DSVec2(-x, -y); }

    /**
    \brief vector addition
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 operator+(const QT3DSVec2 &v) const
    {
        return QT3DSVec2(x + v.x, y + v.y);
    }

    /**
    \brief vector difference
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 operator-(const QT3DSVec2 &v) const
    {
        return QT3DSVec2(x - v.x, y - v.y);
    }

    /**
    \brief scalar post-multiplication
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 operator*(NVReal f) const
    {
        return QT3DSVec2(x * f, y * f);
    }

    /**
    \brief scalar division
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 operator/(NVReal f) const
    {
        f = NVReal(1) / f; // PT: inconsistent notation with operator /=
        return QT3DSVec2(x * f, y * f);
    }

    /**
    \brief vector addition
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 &operator+=(const QT3DSVec2 &v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    /**
    \brief vector difference
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 &operator-=(const QT3DSVec2 &v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    /**
    \brief scalar multiplication
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 &operator*=(NVReal f)
    {
        x *= f;
        y *= f;
        return *this;
    }
    /**
    \brief scalar division
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 &operator/=(NVReal f)
    {
        f = 1.0f / f; // PT: inconsistent notation with operator /
        x *= f;
        y *= f;
        return *this;
    }

    /**
    \brief returns the scalar product of this and other.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal dot(const QT3DSVec2 &v) const { return x * v.x + y * v.y; }

    /** return a unit vector */

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 getNormalized() const
    {
        const NVReal m = magnitudeSquared();
        return m > 0 ? *this * NVRecipSqrt(m) : QT3DSVec2(0, 0);
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
    \brief a[i] * b[i], for all i.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 multiply(const QT3DSVec2 &a) const
    {
        return QT3DSVec2(x * a.x, y * a.y);
    }

    /**
    \brief element-wise minimum
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 minimum(const QT3DSVec2 &v) const
    {
        return QT3DSVec2(NVMin(x, v.x), NVMin(y, v.y));
    }

    /**
    \brief returns MIN(x, y);
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float minElement() const { return NVMin(x, y); }

    /**
    \brief element-wise maximum
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec2 maximum(const QT3DSVec2 &v) const
    {
        return QT3DSVec2(NVMax(x, v.x), NVMax(y, v.y));
    }

    /**
    \brief returns MAX(x, y);
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float maxElement() const { return NVMax(x, y); }

    NVReal x, y;
};

QT3DS_CUDA_CALLABLE static QT3DS_FORCE_INLINE QT3DSVec2 operator*(NVReal f, const QT3DSVec2 &v)
{
    return QT3DSVec2(f * v.x, f * v.y);
}

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_VEC2_H
