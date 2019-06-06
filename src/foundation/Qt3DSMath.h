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

#ifndef QT3DS_FOUNDATION_QT3DS_MATH_H
#define QT3DS_FOUNDATION_QT3DS_MATH_H

/** \addtogroup foundation
@{
*/

#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSAssert.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

// constants
static const NVReal NVPi = NVReal(3.141592653589793);
static const NVReal NVHalfPi = NVReal(1.57079632679489661923);
static const NVReal NVTwoPi = NVReal(6.28318530717958647692);
static const NVReal NVInvPi = NVReal(0.31830988618379067154);

/**
\brief The return value is the greater of the two specified values.
*/
template <class T>
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE T NVMax(T a, T b)
{
    return a < b ? b : a;
}

//! overload for float to use fsel on xbox
template <>
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float NVMax(float a, float b)
{
    return intrinsics::selectMax(a, b);
}

/**
\brief The return value is the lesser of the two specified values.
*/
template <class T>
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE T NVMin(T a, T b)
{
    return a < b ? a : b;
}

template <>
//! overload for float to use fsel on xbox
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float NVMin(float a, float b)
{
    return intrinsics::selectMin(a, b);
}

/*
Many of these are just implemented as QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE calls to the C lib right now,
but later we could replace some of them with some approximations or more
clever stuff.
*/

/**
\brief abs returns the absolute value of its argument.
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVAbs(QT3DSF32 a)
{
    return intrinsics::abs(a);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVEquals(QT3DSF32 a, QT3DSF32 b, QT3DSF32 epsilon)
{
    return (NVAbs(a - b) < epsilon);
}

/**
\brief abs returns the absolute value of its argument.
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVAbs(QT3DSF64 a)
{
    return ::fabs(a);
}

/**
\brief abs returns the absolute value of its argument.
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSI32 NVAbs(QT3DSI32 a)
{
    return ::abs(a);
}

/**
\brief Clamps v to the range [hi,lo]
*/
template <class T>
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE T NVClamp(T v, T lo, T hi)
{
    QT3DS_ASSERT(lo <= hi);
    return NVMin(hi, NVMax(lo, v));
}

//!	\brief Square root.
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVSqrt(QT3DSF32 a)
{
    return intrinsics::sqrt(a);
}

//!	\brief Square root.
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVSqrt(QT3DSF64 a)
{
    return ::sqrt(a);
}

//!	\brief reciprocal square root.
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVRecipSqrt(QT3DSF32 a)
{
    return intrinsics::recipSqrt(a);
}

//!	\brief reciprocal square root.
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVRecipSqrt(QT3DSF64 a)
{
    return 1 / ::sqrt(a);
}

//!trigonometry -- all angles are in radians.

//!	\brief Sine of an angle ( <b>Unit:</b> Radians )
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVSin(QT3DSF32 a)
{
    return intrinsics::sin(a);
}

//!	\brief Sine of an angle ( <b>Unit:</b> Radians )
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVSin(QT3DSF64 a)
{
    return ::sin(a);
}

//!	\brief Cosine of an angle (<b>Unit:</b> Radians)
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVCos(QT3DSF32 a)
{
    return intrinsics::cos(a);
}

//!	\brief Cosine of an angle (<b>Unit:</b> Radians)
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVCos(QT3DSF64 a)
{
    return ::cos(a);
}

/**
\brief Tangent of an angle.
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVTan(QT3DSF32 a)
{
    return QT3DSF32(::tan(a));
}

/**
\brief Tangent of an angle.
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVTan(QT3DSF64 a)
{
    return ::tan(a);
}

/**
\brief Arcsine.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVAsin(QT3DSF32 f)
{
    return QT3DSF32(::asin(NVClamp(f, -1.0f, 1.0f)));
}

/**
\brief Arcsine.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVAsin(QT3DSF64 f)
{
    return ::asin(NVClamp(f, -1.0, 1.0));
}

/**
\brief Arccosine.
Returns angle between 0 and PI in radians
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVAcos(QT3DSF32 f)
{
    return QT3DSF32(::acos(NVClamp(f, -1.0f, 1.0f)));
}

/**
\brief Arccosine.
Returns angle between 0 and PI in radians
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVAcos(QT3DSF64 f)
{
    return ::acos(NVClamp(f, -1.0, 1.0));
}

/**
\brief ArcTangent.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVAtan(QT3DSF32 a)
{
    return QT3DSF32(::atan(a));
}

/**
\brief ArcTangent.
Returns angle between -PI/2 and PI/2 in radians
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVAtan(QT3DSF64 a)
{
    return ::atan(a);
}

/**
\brief Arctangent of (x/y) with correct sign.
Returns angle between -PI and PI in radians
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVAtan2(QT3DSF32 x, QT3DSF32 y)
{
    return QT3DSF32(::atan2(x, y));
}

/**
\brief Arctangent of (x/y) with correct sign.
Returns angle between -PI and PI in radians
<b>Unit:</b> Radians
*/
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 NVAtan2(QT3DSF64 x, QT3DSF64 y)
{
    return ::atan2(x, y);
}

//!	\brief returns true if the passed number is a finite floating point number as opposed to INF, NAN, etc.
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVIsFinite(QT3DSF32 f)
{
    return intrinsics::isFinite(f);
}

//!	\brief returns true if the passed number is a finite floating point number as opposed to INF, NAN, etc.
QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool NVIsFinite(QT3DSF64 f)
{
    return intrinsics::isFinite(f);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVFloor(QT3DSF32 a)
{
    return ::floorf(a);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVExp(QT3DSF32 a)
{
    return ::expf(a);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVCeil(QT3DSF32 a)
{
    return ::ceilf(a);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVSign(QT3DSF32 a)
{
    return qt3ds::intrinsics::sign(a);
}

QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 NVPow(QT3DSF32 x, QT3DSF32 y)
{
    return ::powf(x, y);
};

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_MATH_H
