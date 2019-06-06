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

#ifndef QT3DS_LINUX_AOS_H
#define QT3DS_LINUX_AOS_H

// no includes here! this file should be included from NVcVecMath.h only!!!

#if !COMPILE_VECTOR_INTRINSICS
#error Vector intrinsics should not be included when using scalar implementation.
#endif

// only SSE compatible platforms should reach this
// likely to be split up when we add NEON support.
#include <xmmintrin.h>

typedef union UnionM128 {
    UnionM128() {}
    UnionM128(__m128 in) { m128 = in; }

    UnionM128(__m128i in) { m128i = in; }

    operator __m128() { return m128; }

    operator const __m128() const { return m128; }

    float m128_f32[4];
    __int8_t m128_i8[16];
    __int16_t m128_i16[8];
    __int32_t m128_i32[4];
    __int64_t m128_i64[2];
    __uint16_t m128_u16[8];
    __uint32_t m128_u32[4];
    __uint64_t m128_u64[2];
    __m128 m128;
    __m128i m128i;
} UnionM128;

typedef __m128 FloatV;
typedef __m128 Vec3V;
typedef __m128 Vec4V;
typedef __m128 BoolV;
typedef __m128 QuatV;
// typedef __m128 VecU32V;
// typedef __m128 VecI32V;
// typedef __m128 VecU16V;
// typedef __m128 VecI16V;
// typedef __m128 VecU8V;
typedef UnionM128 VecU32V;
typedef UnionM128 VecI32V;
typedef UnionM128 VecU16V;
typedef UnionM128 VecI16V;
typedef UnionM128 VecU8V;

#define FloatVArg FloatV &
#define Vec3VArg Vec3V &
#define Vec4VArg Vec4V &
#define BoolVArg BoolV &
#define VecU32VArg VecU32V &
#define VecI32VArg VecI32V &
#define VecU16VArg VecU16V &
#define VecI16VArg VecI16V &
#define VecU8VArg VecU8V &
#define QuatVArg QuatV &

QT3DS_ALIGN_PREFIX(16)
struct Mat33V
{
    Mat33V() {}
    Mat33V(const Vec3V &c0, const Vec3V &c1, const Vec3V &c2)
        : col0(c0)
        , col1(c1)
        , col2(c2)
    {
    }
    Vec3V QT3DS_ALIGN(16, col0);
    Vec3V QT3DS_ALIGN(16, col1);
    Vec3V QT3DS_ALIGN(16, col2);
} QT3DS_ALIGN_SUFFIX(16);

QT3DS_ALIGN_PREFIX(16)
struct Mat34V
{
    Mat34V() {}
    Mat34V(const Vec3V &c0, const Vec3V &c1, const Vec3V &c2, const Vec3V &c3)
        : col0(c0)
        , col1(c1)
        , col2(c2)
        , col3(c3)
    {
    }
    Vec3V QT3DS_ALIGN(16, col0);
    Vec3V QT3DS_ALIGN(16, col1);
    Vec3V QT3DS_ALIGN(16, col2);
    Vec3V QT3DS_ALIGN(16, col3);
} QT3DS_ALIGN_SUFFIX(16);

QT3DS_ALIGN_PREFIX(16)
struct Mat43V
{
    Mat43V() {}
    Mat43V(const Vec4V &c0, const Vec4V &c1, const Vec4V &c2)
        : col0(c0)
        , col1(c1)
        , col2(c2)
    {
    }
    Vec4V QT3DS_ALIGN(16, col0);
    Vec4V QT3DS_ALIGN(16, col1);
    Vec4V QT3DS_ALIGN(16, col2);
} QT3DS_ALIGN_SUFFIX(16);

QT3DS_ALIGN_PREFIX(16)
struct Mat44V
{
    Mat44V() {}
    Mat44V(const Vec4V &c0, const Vec4V &c1, const Vec4V &c2, const Vec4V &c3)
        : col0(c0)
        , col1(c1)
        , col2(c2)
        , col3(c3)
    {
    }
    Vec4V QT3DS_ALIGN(16, col0);
    Vec4V QT3DS_ALIGN(16, col1);
    Vec4V QT3DS_ALIGN(16, col2);
    Vec4V QT3DS_ALIGN(16, col3);
} QT3DS_ALIGN_SUFFIX(16);

#endif // QT3DS_LINUX_AOS_H
