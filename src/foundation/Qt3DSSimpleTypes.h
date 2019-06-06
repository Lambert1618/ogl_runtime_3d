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
#pragma once
#ifndef QT3DS_FOUNDATION_QT3DS_SIMPLE_TYPES_H
#define QT3DS_FOUNDATION_QT3DS_SIMPLE_TYPES_H

/** \addtogroup foundation
  @{
*/

// Platform specific types:
// Design note: Its OK to use int for general loop variables and temps.

#include <QtCore/qglobal.h>
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSPreprocessor.h"
#include "EABase/eabase.h"
#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif //#ifndef QT3DS_DOXYGEN

typedef quint8 QT3DSU8;
typedef qint8 QT3DSI8;
typedef quint16 QT3DSU16;
typedef qint16 QT3DSI16;
typedef quint32 QT3DSU32;
typedef qint32 QT3DSI32;

// Android's definition of GLuint64 as unsigned long (64-bits) requires this workaround
#if Q_PROCESSOR_WORDSIZE == 8 && (defined(Q_OS_ANDROID) || defined(Q_OS_INTEGRITY))
typedef unsigned long QT3DSU64;
#else
typedef quint64 QT3DSU64;
#endif

typedef qint64 QT3DSI64;
typedef float QT3DSF32;
typedef double QT3DSF64;
typedef QT3DSI32 IntBool;

struct QT3DSF16
{
    QT3DSU16 mData;
};

QT3DS_COMPILE_TIME_ASSERT(sizeof(QT3DSI8) == 1);
QT3DS_COMPILE_TIME_ASSERT(sizeof(QT3DSU8) == 1);
QT3DS_COMPILE_TIME_ASSERT(sizeof(QT3DSI16) == 2);
QT3DS_COMPILE_TIME_ASSERT(sizeof(QT3DSU16) == 2);
QT3DS_COMPILE_TIME_ASSERT(sizeof(QT3DSI32) == 4);
QT3DS_COMPILE_TIME_ASSERT(sizeof(QT3DSU32) == 4);
QT3DS_COMPILE_TIME_ASSERT(sizeof(QT3DSI64) == 8);
QT3DS_COMPILE_TIME_ASSERT(sizeof(QT3DSU64) == 8);

// Type ranges
#define QT3DS_MAX_I8 127 // maximum possible sbyte value, 0x7f
#define QT3DS_MIN_I8 (-128) // minimum possible sbyte value, 0x80
#define QT3DS_MAX_U8 255U // maximum possible ubyte value, 0xff
#define QT3DS_MIN_U8 0 // minimum possible ubyte value, 0x00
#define QT3DS_MAX_I16 32767 // maximum possible sword value, 0x7fff
#define QT3DS_MIN_I16 (-32768) // minimum possible sword value, 0x8000
#define QT3DS_MAX_U16 65535U // maximum possible uword value, 0xffff
#define QT3DS_MIN_U16 0 // minimum possible uword value, 0x0000
#define QT3DS_MAX_I32 2147483647 // maximum possible sdword value, 0x7fffffff
#define QT3DS_MIN_I32 (-2147483647 - 1) // minimum possible sdword value, 0x80000000
#define QT3DS_MAX_U32 4294967295U // maximum possible udword value, 0xffffffff
#define QT3DS_MIN_U32 0 // minimum possible udword value, 0x00000000
#define QT3DS_MAX_F32 3.4028234663852885981170418348452e+38F
// maximum possible float value
#define QT3DS_MAX_F64 DBL_MAX // maximum possible double value

#define QT3DS_ENV_F32 FLT_EPSILON // maximum relative error of float rounding
#define QT3DS_ENV_F64 DBL_EPSILON // maximum relative error of double rounding

#ifndef QT3DS_FOUNDATION_USE_F64

typedef QT3DSF32 NVReal;

#define QT3DS_MAX_REAL QT3DS_MAX_F32
#define QT3DS_ENV_REAL QT3DS_ENV_F32
#define QT3DS_NORMALIZATION_EPSILON NVReal(1e-20f)

#else

typedef QT3DSF64 NVReal;

#define QT3DS_MAX_REAL QT3DS_MAX_F64
#define QT3DS_ENV_REAL QT3DS_ENV_F64
#define QT3DS_NORMALIZATION_EPSILON NVReal(1e-180)

#endif

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_SIMPLE_TYPES_H
