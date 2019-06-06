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

#ifndef QT3DS_FOUNDATION_QT3DS_MAT33_H
#define QT3DS_FOUNDATION_QT3DS_MAT33_H
/** \addtogroup foundation
@{
*/

#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSQuat.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif
/*!
\brief 3x3 matrix class

Some clarifications, as there have been much confusion about matrix formats etc in the past.

Short:
- Matrix have base vectors in columns (vectors are column matrices, 3x1 matrices).
- Matrix is physically stored in column major format
- Matrices are concaternated from left

Long:
Given three base vectors a, b and c the matrix is stored as

|a.x b.x c.x|
|a.y b.y c.y|
|a.z b.z c.z|

Vectors are treated as columns, so the vector v is

|x|
|y|
|z|

And matrices are applied _before_ the vector (pre-multiplication)
v' = M*v

|x'|   |a.x b.x c.x|   |x|   |a.x*x + b.x*y + c.x*z|
|y'| = |a.y b.y c.y| * |y| = |a.y*x + b.y*y + c.y*z|
|z'|   |a.z b.z c.z|   |z|   |a.z*x + b.z*y + c.z*z|


Physical storage and indexing:
To be compatible with popular 3d rendering APIs (read D3d and OpenGL)
the physical indexing is

|0 3 6|
|1 4 7|
|2 5 8|

index = column*3 + row

which in C++ translates to M[column][row]

The mathematical indexing is M_row,column and this is what is used for _-notation
so _12 is 1st row, second column and operator(row, column)!

@see QT3DSMat44

*/
class QT3DSMat33
{
public:
    //! Default constructor
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33() {}

    //! Construct from three base vectors
    QT3DS_CUDA_CALLABLE QT3DSMat33(const QT3DSVec3 &col0, const QT3DSVec3 &col1, const QT3DSVec3 &col2)
        : column0(col0)
        , column1(col1)
        , column2(col2)
    {
    }

    //! Construct from float[9]
    QT3DS_CUDA_CALLABLE explicit QT3DS_INLINE QT3DSMat33(NVReal values[])
        : column0(values[0], values[1], values[2])
        , column1(values[3], values[4], values[5])
        , column2(values[6], values[7], values[8])
    {
    }

    //! Construct from a quaternion
    QT3DS_CUDA_CALLABLE explicit QT3DS_FORCE_INLINE QT3DSMat33(const QT3DSQuat &q)
    {
        const NVReal x = q.x;
        const NVReal y = q.y;
        const NVReal z = q.z;
        const NVReal w = q.w;

        const NVReal x2 = x + x;
        const NVReal y2 = y + y;
        const NVReal z2 = z + z;

        const NVReal xx = x2 * x;
        const NVReal yy = y2 * y;
        const NVReal zz = z2 * z;

        const NVReal xy = x2 * y;
        const NVReal xz = x2 * z;
        const NVReal xw = x2 * w;

        const NVReal yz = y2 * z;
        const NVReal yw = y2 * w;
        const NVReal zw = z2 * w;

        column0 = QT3DSVec3(1.0f - yy - zz, xy + zw, xz - yw);
        column1 = QT3DSVec3(xy - zw, 1.0f - xx - zz, yz + xw);
        column2 = QT3DSVec3(xz + yw, yz - xw, 1.0f - xx - yy);
    }

    //! Copy constructor
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33(const QT3DSMat33 &other)
        : column0(other.column0)
        , column1(other.column1)
        , column2(other.column2)
    {
    }

    //! Assignment operator
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSMat33 &operator=(const QT3DSMat33 &other)
    {
        column0 = other.column0;
        column1 = other.column1;
        column2 = other.column2;
        return *this;
    }

    //! Set to identity matrix
    QT3DS_CUDA_CALLABLE QT3DS_INLINE static QT3DSMat33 createIdentity()
    {
        return QT3DSMat33(QT3DSVec3(1, 0, 0), QT3DSVec3(0, 1, 0), QT3DSVec3(0, 0, 1));
    }

    //! Set to zero matrix
    QT3DS_CUDA_CALLABLE QT3DS_INLINE static QT3DSMat33 createZero()
    {
        return QT3DSMat33(QT3DSVec3(0.0f), QT3DSVec3(0.0f), QT3DSVec3(0.0f));
    }

    //! Construct from diagonal, off-diagonals are zero.
    QT3DS_CUDA_CALLABLE QT3DS_INLINE static QT3DSMat33 createDiagonal(const QT3DSVec3 &d)
    {
        return QT3DSMat33(QT3DSVec3(d.x, 0.0f, 0.0f), QT3DSVec3(0.0f, d.y, 0.0f), QT3DSVec3(0.0f, 0.0f, d.z));
    }

    //! Get transposed matrix
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSMat33 getTranspose() const
    {
        const QT3DSVec3 v0(column0.x, column1.x, column2.x);
        const QT3DSVec3 v1(column0.y, column1.y, column2.y);
        const QT3DSVec3 v2(column0.z, column1.z, column2.z);

        return QT3DSMat33(v0, v1, v2);
    }

    //! Get the real inverse
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 getInverse() const
    {
        const NVReal det = getDeterminant();
        QT3DSMat33 inverse;

        if (det != 0) {
            const NVReal invDet = 1.0f / det;

            inverse.column0[0] = invDet * (column1[1] * column2[2] - column2[1] * column1[2]);
            inverse.column0[1] = invDet * -(column0[1] * column2[2] - column2[1] * column0[2]);
            inverse.column0[2] = invDet * (column0[1] * column1[2] - column0[2] * column1[1]);

            inverse.column1[0] = invDet * -(column1[0] * column2[2] - column1[2] * column2[0]);
            inverse.column1[1] = invDet * (column0[0] * column2[2] - column0[2] * column2[0]);
            inverse.column1[2] = invDet * -(column0[0] * column1[2] - column0[2] * column1[0]);

            inverse.column2[0] = invDet * (column1[0] * column2[1] - column1[1] * column2[0]);
            inverse.column2[1] = invDet * -(column0[0] * column2[1] - column0[1] * column2[0]);
            inverse.column2[2] = invDet * (column0[0] * column1[1] - column1[0] * column0[1]);

            return inverse;
        } else {
            return createIdentity();
        }
    }

    //! Get determinant
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal getDeterminant() const
    {
        return column0.dot(column1.cross(column2));
    }

    //! Unary minus
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 operator-() const
    {
        return QT3DSMat33(-column0, -column1, -column2);
    }

    //! Add
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 operator+(const QT3DSMat33 &other) const
    {
        return QT3DSMat33(column0 + other.column0, column1 + other.column1, column2 + other.column2);
    }

    //! Subtract
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 operator-(const QT3DSMat33 &other) const
    {
        return QT3DSMat33(column0 - other.column0, column1 - other.column1, column2 - other.column2);
    }

    //! Scalar multiplication
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 operator*(NVReal scalar) const
    {
        return QT3DSMat33(column0 * scalar, column1 * scalar, column2 * scalar);
    }

    friend QT3DSMat33 operator*(NVReal, const QT3DSMat33 &);

    //! Matrix vector multiplication (returns 'this->transform(vec)')
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 operator*(const QT3DSVec3 &vec) const { return transform(vec); }

    //! Matrix multiplication
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSMat33 operator*(const QT3DSMat33 &other) const
    {
        // Rows from this <dot> columns from other
        // column0 = transform(other.column0) etc
        return QT3DSMat33(transform(other.column0), transform(other.column1),
                       transform(other.column2));
    }

    // a <op>= b operators

    //! Equals-add
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 &operator+=(const QT3DSMat33 &other)
    {
        column0 += other.column0;
        column1 += other.column1;
        column2 += other.column2;
        return *this;
    }

    //! Equals-sub
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 &operator-=(const QT3DSMat33 &other)
    {
        column0 -= other.column0;
        column1 -= other.column1;
        column2 -= other.column2;
        return *this;
    }

    //! Equals scalar multiplication
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 &operator*=(NVReal scalar)
    {
        column0 *= scalar;
        column1 *= scalar;
        column2 *= scalar;
        return *this;
    }

    //! Element access, mathematical way!
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal operator()(unsigned int row, unsigned int col) const
    {
        return (*this)[col][row];
    }

    //! Element access, mathematical way!
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal &operator()(unsigned int row, unsigned int col)
    {
        return (*this)[col][row];
    }

    // Transform etc

    //! Transform vector by matrix, equal to v' = M*v
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 transform(const QT3DSVec3 &other) const
    {
        return column0 * other.x + column1 * other.y + column2 * other.z;
    }

    //! Transform vector by matrix transpose, v' = M^t*v
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 transformTranspose(const QT3DSVec3 &other) const
    {
        return QT3DSVec3(column0.dot(other), column1.dot(other), column2.dot(other));
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE const NVReal *front() const { return &column0.x; }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 &operator[](int num) { return (&column0)[num]; }
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE const QT3DSVec3 &operator[](int num) const
    {
        return (&column0)[num];
    }

    // Data, see above for format!

    QT3DSVec3 column0, column1, column2; // the three base vectors
};

// implementation from Qt3DSQuat.h
QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSQuat::QT3DSQuat(const QT3DSMat33 &m)
{
    NVReal tr = m(0, 0) + m(1, 1) + m(2, 2), h;
    if (tr >= 0) {
        h = NVSqrt(tr + 1);
        w = NVReal(0.5) * h;
        h = NVReal(0.5) / h;

        x = (m(2, 1) - m(1, 2)) * h;
        y = (m(0, 2) - m(2, 0)) * h;
        z = (m(1, 0) - m(0, 1)) * h;
    } else {
        int i = 0;
        if (m(1, 1) > m(0, 0))
            i = 1;
        if (m(2, 2) > m(i, i))
            i = 2;
        switch (i) {
        case 0:
            h = NVSqrt((m(0, 0) - (m(1, 1) + m(2, 2))) + 1);
            x = NVReal(0.5) * h;
            h = NVReal(0.5) / h;

            y = (m(0, 1) + m(1, 0)) * h;
            z = (m(2, 0) + m(0, 2)) * h;
            w = (m(2, 1) - m(1, 2)) * h;
            break;
        case 1:
            h = NVSqrt((m(1, 1) - (m(2, 2) + m(0, 0))) + 1);
            y = NVReal(0.5) * h;
            h = NVReal(0.5) / h;

            z = (m(1, 2) + m(2, 1)) * h;
            x = (m(0, 1) + m(1, 0)) * h;
            w = (m(0, 2) - m(2, 0)) * h;
            break;
        case 2:
            h = NVSqrt((m(2, 2) - (m(0, 0) + m(1, 1))) + 1);
            z = NVReal(0.5) * h;
            h = NVReal(0.5) / h;

            x = (m(2, 0) + m(0, 2)) * h;
            y = (m(1, 2) + m(2, 1)) * h;
            w = (m(1, 0) - m(0, 1)) * h;
            break;
        default: // Make compiler happy
            x = y = z = w = 0;
            break;
        }
    }
}

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_MAT33_H
