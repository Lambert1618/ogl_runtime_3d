/****************************************************************************
**
** Copyright (C) 2009-2011 NVIDIA Corporation.
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

#ifndef __INCLUDED_QT3DS_TYPES_H
#define __INCLUDED_QT3DS_TYPES_H

typedef unsigned char NvU8; // 0 to 255
typedef unsigned short NvU16; // 0 to 65535
typedef unsigned int NvU32; // 0 to 4294967295
typedef unsigned long long NvU64; // 0 to 18446744073709551615
typedef signed char NvS8; // -128 to 127
typedef signed short NvS16; // -32768 to 32767
typedef signed int NvS32; // -2147483648 to 2147483647
typedef signed long long NvS64; // 2^-63 to 2^63-1

// Explicitly sized floats
typedef float NvF32; // IEEE Single Precision (S1E8M23)
typedef double NvF64; // IEEE Double Precision (S1E11M52)

// Boolean type
#define QT3DS_FALSE 0
#define QT3DS_TRUE 1
typedef NvU8 NvBool;

// Result of sizeof operator
typedef unsigned long NvSize;

// Type large enough to hold a relative file offset
typedef long NvOffset;

// Base NULL type.
#define QT3DS_NULL 0

// Error related
typedef enum {
    NvError_Success = 0,
    NvError_NotSupported,
    NvError_NotInitialized,
    NvError_BadParameter,
    NvError_InsufficientMemory,
    NvError_NoEntries,
    NvError_UnknownError,
} NvError;

#if 1 // defined(_DEBUG) // !!!!TBD TODO
#define NvAssert(c)
#else
#define NvAssert(c) ((void)((c) ? 0 : (NvHandleAssertion(#c, __FILE__, __LINE__), 0)))
#endif

// Other standardized typedefs
typedef NvU64 NvUST; // U64 unadjusted system time value

#endif
