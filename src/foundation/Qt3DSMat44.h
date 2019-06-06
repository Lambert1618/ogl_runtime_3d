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

#ifndef QT3DS_FOUNDATION_QT3DS_MAT44_H
#define QT3DS_FOUNDATION_QT3DS_MAT44_H
/** \addtogroup foundation
@{
*/

#include "foundation/Qt3DSQuat.h"
#include "foundation/Qt3DSVec4.h"
#include "foundation/Qt3DSMat33.h"
#include "foundation/Qt3DSTransform.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

/*!
\brief 4x4 matrix class

This class is layout-compatible with D3D and OpenGL matrices. More notes on layout are given in the QT3DSMat33
Graphics matrices always (by which I mean DX and OpenGL) have the rotation basis vectors stored contiguously
in 0-2, 3-5, 6-8 and the the translation stored contiguously in 12-14. However, DX calls this row major and GL calls it column-major.
http://www.mindcontrol.org/~hplus/graphics/matrix-layout.html

you can blat these directly into GL (or DX)

--Dilip Sequeira

@see QT3DSMat33 NVTransform
*/

class QT3DSMat44
{
public:
    //! Default constructor
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44() {}

    //! Construct from four 4-vectors
    QT3DS_CUDA_CALLABLE QT3DSMat44(const QT3DSVec4 &col0, const QT3DSVec4 &col1, const QT3DSVec4 &col2,
                             const QT3DSVec4 &col3)
        : column0(col0)
        , column1(col1)
        , column2(col2)
        , column3(col3)
    {
    }

    //! Construct from three base vectors and a translation
    QT3DS_CUDA_CALLABLE QT3DSMat44(const QT3DSVec3 &column0, const QT3DSVec3 &column1, const QT3DSVec3 &column2,
                             const QT3DSVec3 &column3)
        : column0(column0, 0)
        , column1(column1, 0)
        , column2(column2, 0)
        , column3(column3, 1)
    {
    }

    //! Construct from float[16]
    explicit QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44(NVReal values[])
        : column0(values[0], values[1], values[2], values[3])
        , column1(values[4], values[5], values[6], values[7])
        , column2(values[8], values[9], values[10], values[11])
        , column3(values[12], values[13], values[14], values[15])
    {
    }

    //! Construct from a quaternion
    explicit QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44(const QT3DSQuat &q)
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

        column0 = QT3DSVec4(1.0f - yy - zz, xy + zw, xz - yw, 0.0f);
        column1 = QT3DSVec4(xy - zw, 1.0f - xx - zz, yz + xw, 0.0f);
        column2 = QT3DSVec4(xz + yw, yz - xw, 1.0f - xx - yy, 0.0f);
        column3 = QT3DSVec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    //! Construct from a diagonal vector
    explicit QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44(const QT3DSVec4 &diagonal)
        : column0(diagonal.x, 0.0f, 0.0f, 0.0f)
        , column1(0.0f, diagonal.y, 0.0f, 0.0f)
        , column2(0.0f, 0.0f, diagonal.z, 0.0f)
        , column3(0.0f, 0.0f, 0.0f, diagonal.w)
    {
    }

    QT3DS_CUDA_CALLABLE QT3DSMat44(const QT3DSMat33 &orientation, const QT3DSVec3 &position)
        : column0(orientation.column0, 0.0f)
        , column1(orientation.column1, 0.0f)
        , column2(orientation.column2, 0.0f)
        , column3(position, 1)
    {
    }

    QT3DS_CUDA_CALLABLE QT3DSMat44(const NVTransform &t) { *this = QT3DSMat44(QT3DSMat33(t.q), t.p); }

    //! Copy constructor
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44(const QT3DSMat44 &other)
        : column0(other.column0)
        , column1(other.column1)
        , column2(other.column2)
        , column3(other.column3)
    {
    }

    //! Assignment operator
    QT3DS_CUDA_CALLABLE QT3DS_INLINE const QT3DSMat44 &operator=(const QT3DSMat44 &other)
    {
        column0 = other.column0;
        column1 = other.column1;
        column2 = other.column2;
        column3 = other.column3;
        return *this;
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE static QT3DSMat44 createIdentity()
    {
        return QT3DSMat44(QT3DSVec4(1.0f, 0.0f, 0.0f, 0.0f), QT3DSVec4(0.0f, 1.0f, 0.0f, 0.0f),
                       QT3DSVec4(0.0f, 0.0f, 1.0f, 0.0f), QT3DSVec4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE static QT3DSMat44 createZero()
    {
        return QT3DSMat44(QT3DSVec4(0.0f), QT3DSVec4(0.0f), QT3DSVec4(0.0f), QT3DSVec4(0.0f));
    }

    //! Get transposed matrix
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 getTranspose() const
    {
        return QT3DSMat44(QT3DSVec4(column0.x, column1.x, column2.x, column3.x),
                       QT3DSVec4(column0.y, column1.y, column2.y, column3.y),
                       QT3DSVec4(column0.z, column1.z, column2.z, column3.z),
                       QT3DSVec4(column0.w, column1.w, column2.w, column3.w));
    }

    //! Unary minus
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 operator-() const
    {
        return QT3DSMat44(-column0, -column1, -column2, -column3);
    }

    //! Add
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 operator+(const QT3DSMat44 &other) const
    {
        return QT3DSMat44(column0 + other.column0, column1 + other.column1, column2 + other.column2,
                       column3 + other.column3);
    }

    //! Subtract
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 operator-(const QT3DSMat44 &other) const
    {
        return QT3DSMat44(column0 - other.column0, column1 - other.column1, column2 - other.column2,
                       column3 - other.column3);
    }

    //! Scalar multiplication
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 operator*(NVReal scalar) const
    {
        return QT3DSMat44(column0 * scalar, column1 * scalar, column2 * scalar, column3 * scalar);
    }

    friend QT3DSMat44 operator*(NVReal, const QT3DSMat44 &);

    //! Matrix multiplication
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 operator*(const QT3DSMat44 &other) const
    {
        // Rows from this <dot> columns from other
        // column0 = transform(other.column0) etc
        return QT3DSMat44(transform(other.column0), transform(other.column1), transform(other.column2),
                       transform(other.column3));
    }

    // a <op>= b operators

    //! Equals-add
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 &operator+=(const QT3DSMat44 &other)
    {
        column0 += other.column0;
        column1 += other.column1;
        column2 += other.column2;
        column3 += other.column3;
        return *this;
    }

    //! Equals-sub
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 &operator-=(const QT3DSMat44 &other)
    {
        column0 -= other.column0;
        column1 -= other.column1;
        column2 -= other.column2;
        column3 -= other.column3;
        return *this;
    }

    //! Equals scalar multiplication
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 &operator*=(NVReal scalar)
    {
        column0 *= scalar;
        column1 *= scalar;
        column2 *= scalar;
        column3 *= scalar;
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

    //! Transform vector by matrix, equal to v' = M*v
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 transform(const QT3DSVec4 &other) const
    {
        return column0 * other.x + column1 * other.y + column2 * other.z + column3 * other.w;
    }

    //! Transform vector by matrix, equal to v' = M*v
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 transform(const QT3DSVec3 &other) const
    {
        return transform(QT3DSVec4(other, 1)).getXYZ();
    }

    //! Rotate vector by matrix, equal to v' = M*v
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec4 rotate(const QT3DSVec4 &other) const
    {
        return column0 * other.x + column1 * other.y + column2 * other.z; // + column3*0;
    }

    //! Rotate vector by matrix, equal to v' = M*v
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 rotate(const QT3DSVec3 &other) const
    {
        return rotate(QT3DSVec4(other, 1)).getXYZ();
    }

    /**
*	Shamelessly pulled from gl-matrix.js
    *	Rotates a matrix by the given angle around the specified axis
*
* @param angle Angle (in radians) to rotate
* @param axis vec3 representing the axis to rotate around
*/
    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool rotate(QT3DSF32 angleRadians, QT3DSVec3 axis)
    {
        QT3DSF32 x = axis[0], y = axis[1], z = axis[2], len = x * x + y * y + z * z, s, c, t, a00, a01,
              a02, a03, a10, a11, a12, a13, a20, a21, a22, a23, b00, b01, b02, b10, b11, b12, b20,
              b21, b22;

        if (!len) {
            return false;
        }
        if (len != 1.0f) {
            len = 1 / (NVSqrt(len));
            x *= len;
            y *= len;
            z *= len;
        }

        s = NVSin(angleRadians);
        c = NVCos(angleRadians);
        t = 1 - c;

        QT3DSF32 *dataPtr(front());

// inverse algorithm was written for row-major matrixes.
#define mat(idx) dataPtr[idx]

        a00 = mat(0);
        a01 = mat(1);
        a02 = mat(2);
        a03 = mat(3);
        a10 = mat(4);
        a11 = mat(5);
        a12 = mat(6);
        a13 = mat(7);
        a20 = mat(8);
        a21 = mat(9);
        a22 = mat(10);
        a23 = mat(11);
#undef mat

        // Construct the elements of the rotation matrix
        b00 = x * x * t + c;
        b01 = y * x * t + z * s;
        b02 = z * x * t - y * s;
        b10 = x * y * t - z * s;
        b11 = y * y * t + c;
        b12 = z * y * t + x * s;
        b20 = x * z * t + y * s;
        b21 = y * z * t - x * s;
        b22 = z * z * t + c;

// Perform rotation-specific matrix multiplication

#define dest(idx) dataPtr[idx]

        dest(0) = a00 * b00 + a10 * b01 + a20 * b02;
        dest(1) = a01 * b00 + a11 * b01 + a21 * b02;
        dest(2) = a02 * b00 + a12 * b01 + a22 * b02;
        dest(3) = a03 * b00 + a13 * b01 + a23 * b02;

        dest(4) = a00 * b10 + a10 * b11 + a20 * b12;
        dest(5) = a01 * b10 + a11 * b11 + a21 * b12;
        dest(6) = a02 * b10 + a12 * b11 + a22 * b12;
        dest(7) = a03 * b10 + a13 * b11 + a23 * b12;

        dest(8) = a00 * b20 + a10 * b21 + a20 * b22;
        dest(9) = a01 * b20 + a11 * b21 + a21 * b22;
        dest(10) = a02 * b20 + a12 * b21 + a22 * b22;
        dest(11) = a03 * b20 + a13 * b21 + a23 * b22;

#undef dest

        return true;
    };

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 getBasis(int num) const
    {
        QT3DS_ASSERT(num >= 0 && num < 3);
        return (&column0)[num].getXYZ();
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 getPosition() const { return column3.getXYZ(); }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE void setPosition(const QT3DSVec3 &position)
    {
        column3.x = position.x;
        column3.y = position.y;
        column3.z = position.z;
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVReal *front() { return &column0.x; }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE const NVReal *front() const { return &column0.x; }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec4 &operator[](int num) { return (&column0)[num]; }
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE const QT3DSVec4 &operator[](int num) const
    {
        return (&column0)[num];
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE void scale(const QT3DSVec4 &p)
    {
        column0 *= p.x;
        column1 *= p.y;
        column2 *= p.z;
        column3 *= p.w;
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 inverseRT(void) const
    {
        QT3DSVec3 r0(column0.x, column1.x, column2.x), r1(column0.y, column1.y, column2.y),
            r2(column0.z, column1.z, column2.z);

        return QT3DSMat44(r0, r1, r2, -(r0 * column3.x + r1 * column3.y + r2 * column3.z));
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat44 getInverse(void) const
    {
        // inverse algorithm was written for row-major matrixes.
        const QT3DSF32 *myPtr(front());
#define mat(idx) myPtr[idx]

        QT3DSF32 a00 = mat(0), a01 = mat(1), a02 = mat(2), a03 = mat(3), a10 = mat(4), a11 = mat(5),
              a12 = mat(6), a13 = mat(7), a20 = mat(8), a21 = mat(9), a22 = mat(10), a23 = mat(11),
              a30 = mat(12), a31 = mat(13), a32 = mat(14), a33 = mat(15),
#undef mat

              b00 = a00 * a11 - a01 * a10, b01 = a00 * a12 - a02 * a10, b02 = a00 * a13 - a03 * a10,
              b03 = a01 * a12 - a02 * a11, b04 = a01 * a13 - a03 * a11, b05 = a02 * a13 - a03 * a12,
              b06 = a20 * a31 - a21 * a30, b07 = a20 * a32 - a22 * a30, b08 = a20 * a33 - a23 * a30,
              b09 = a21 * a32 - a22 * a31, b10 = a21 * a33 - a23 * a31, b11 = a22 * a33 - a23 * a32,

              d = (b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06), invDet;

        // Calculate the determinant
        if (!d) {
            QT3DS_ASSERT(false);
            return QT3DSMat44::createIdentity();
        }
        invDet = 1 / d;

        QT3DSMat44 retval;
        QT3DSF32 *destPtr = retval.front();

#define dest(idx) destPtr[idx]
        dest(0) = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
        dest(1) = (-a01 * b11 + a02 * b10 - a03 * b09) * invDet;
        dest(2) = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
        dest(3) = (-a21 * b05 + a22 * b04 - a23 * b03) * invDet;
        dest(4) = (-a10 * b11 + a12 * b08 - a13 * b07) * invDet;
        dest(5) = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
        dest(6) = (-a30 * b05 + a32 * b02 - a33 * b01) * invDet;
        dest(7) = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;
        dest(8) = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
        dest(9) = (-a00 * b10 + a01 * b08 - a03 * b06) * invDet;
        dest(10) = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
        dest(11) = (-a20 * b04 + a21 * b02 - a23 * b00) * invDet;
        dest(12) = (-a10 * b09 + a11 * b07 - a12 * b06) * invDet;
        dest(13) = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
        dest(14) = (-a30 * b03 + a31 * b01 - a32 * b00) * invDet;
        dest(15) = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;
#undef dest

        return retval;
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE bool isFinite() const
    {
        return column0.isFinite() && column1.isFinite() && column2.isFinite() && column3.isFinite();
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 getUpper3x3() const
    {
        return QT3DSMat33(column0.getXYZ(), column1.getXYZ(), column2.getXYZ());
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSMat33 getUpper3x3InverseTranspose() const
    {
        return getUpper3x3().getInverse().getTranspose();
    }

    // Data, see above for format!

    QT3DSVec4 column0, column1, column2, column3; // the four base vectors
};

// implementation from Qt3DSTransform.h
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE NVTransform::NVTransform(const QT3DSMat44 &m)
{
    QT3DSVec3 column0 = QT3DSVec3(m.column0.x, m.column0.y, m.column0.z);
    QT3DSVec3 column1 = QT3DSVec3(m.column1.x, m.column1.y, m.column1.z);
    QT3DSVec3 column2 = QT3DSVec3(m.column2.x, m.column2.y, m.column2.z);

    q = QT3DSQuat(QT3DSMat33(column0, column1, column2));
    p = QT3DSVec3(m.column3.x, m.column3.y, m.column3.z);
}

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_MAT44_H
