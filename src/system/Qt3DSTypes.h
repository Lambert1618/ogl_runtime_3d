/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

#include <QtCore/qglobal.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Typedefs
//==============================================================================
typedef bool BOOL; ///< true or false, usually 8 bits
typedef qint8 INT8; ///< A signed 8-bit integer, not a character
typedef quint8 UINT8; ///< An unsigned 8-bit integer 0-255, not a character
typedef qint16 INT16; ///< A signed 16-bit integer
typedef quint16 UINT16; ///< An unsigned 16-bit integer
typedef qint32 INT32; ///< A signed 32-bit integer
typedef quint32 UINT32; ///< An unsigned 32-bit integer
typedef float FLOAT; ///< A 32-bit floating point number
typedef char CHAR; ///< String character, not a number
typedef qint64 INT64;
typedef quint64 UINT64;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

//==============================================================================
//	Max constants and computation
//==============================================================================
INT32 Q3DStudio_maxbits(const INT32 inBitCount, const BOOL inUnsigned);

#define AKMAX_INT8 static_cast<INT8>(0x7F)
#define AKMAX_UINT8 static_cast<UINT8>(0xFF)
#define AKMAX_INT16 static_cast<INT16>(0x7FFF)
#define AKMAX_UINT16 static_cast<UINT16>(0xFFFF)
#define AKMAX_INT32 0x7FFFFFFFL
#define AKMAX_UINT32 0xFFFFFFFFUL

#define Q3DStudio_INT64_C(x) x
#define Q3DStudio_UINT64_C(x) x

typedef FILE TFile;
typedef size_t TFileSize;

struct VECTOR4
{
    union {
        FLOAT v[4];
        UINT32 u[4];
    };

    FLOAT &x() { return v[0]; }
    FLOAT &y() { return v[1]; }
    FLOAT &z() { return v[2]; }
    FLOAT &w() { return v[3]; }
};

struct MATRIX16
{
    union {
        VECTOR4 v[4];
        FLOAT m[4][4];
        FLOAT f[16];
    };

    FLOAT &_11() { return f[0]; }
    FLOAT &_12() { return f[1]; }
    FLOAT &_13() { return f[2]; }
    FLOAT &_14() { return f[3]; }
    FLOAT &_21() { return f[4]; }
    FLOAT &_22() { return f[5]; }
    FLOAT &_23() { return f[6]; }
    FLOAT &_24() { return f[7]; }
    FLOAT &_31() { return f[8]; }
    FLOAT &_32() { return f[9]; }
    FLOAT &_33() { return f[10]; }
    FLOAT &_34() { return f[11]; }
    FLOAT &_41() { return f[12]; }
    FLOAT &_42() { return f[13]; }
    FLOAT &_43() { return f[14]; }
    FLOAT &_44() { return f[15]; }
};

typedef INT64 TMicroSeconds; ///< Time in microseconds
typedef INT64 TTimeUnit; ///< Time in milliseconds
} // namespace Q3DStudio
