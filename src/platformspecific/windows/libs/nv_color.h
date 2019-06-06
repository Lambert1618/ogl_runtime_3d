/****************************************************************************
**
** Copyright (C) 2007-2008 NVIDIA Corporation.
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

#ifndef _QT3DS_COLOR_H
#define _QT3DS_COLOR_H

/** @file nv_color.h
    Simple abstraction for RGBA colors as parameters to various libraries/functions.
*/

/** Base type for a single color channel (8-bit unsigned). */
typedef unsigned char NvPCColor8;

//#define NV_COLOR_BREAKOLD
#ifdef NV_COLOR_BREAKOLD // uses a struct to break backwards-compat and accidental uses

typedef struct
{
    NvPCColor8 r;
    NvPCColor8 g;
    NvPCColor8 b;
    NvPCColor8 a;
} NvPackedColor;

static const NvPackedColor gnvpcwhite = { 255, 255, 255, 255 };
static const NvPackedColor gnvpcblack = { 0, 0, 0, 255 };

#define NV_PACKED_COLOR(r, g, b, a)                                                                \
    {                                                                                              \
        (NvPCColor8)(r), (NvPCColor8)(g), (NvPCColor8)(b), (NvPCColor8)(a)                         \
    }

#define NV_PC_PREDEF_WHITE gnvpcwhite
#define NV_PC_PREDEF_BLACK gnvpcblack

#define NV_PC_RED(c) (c.r)
#define NV_PC_GREEN(c) (c.g)
#define NV_PC_BLUE(c) (c.b)
#define NV_PC_ALPHA(c) (c.a)

#define NV_PC_PACK_UINT(c) (*(unsigned int *)(&(c)))

#define NV_PC_EQUAL(x, y) ((x).r == (y).r && (x).g == (y).g && (x).b == (y).b && (x).a == (y).a)

#else /* code that doesn't break old pass-as-uint stuff */

/** Main type declaration for a packed 4-color construct. */
typedef unsigned int NvPackedColor;

/** A macro to build a packed color, passing in RGBA as four 8-bit integer values. */
#define NV_PACKED_COLOR(r, g, b, a)                                                                \
    ((NvPackedColor)(((((int)(a)) & 0xFF) << 24) | ((((int)(b)) & 0xFF) << 16)                     \
                     | ((((int)(g)) & 0xFF) << 8) | ((((int)(r)) & 0xFF))))

/** A predefined constant for WHITE. */
#define NV_PC_PREDEF_WHITE NV_PACKED_COLOR(0xFF, 0xFF, 0xFF, 0xFF)
/** A predefined constant for BLACK. */
#define NV_PC_PREDEF_BLACK NV_PACKED_COLOR(0x00, 0x00, 0x00, 0xFF)

/** A macro for 'extracting' the red value from an NvPackedColor. */
#define NV_PC_RED(c) ((c >> 0) & 0xff)
/** A macro for 'extracting' the green value from an NvPackedColor. */
#define NV_PC_GREEN(c) ((c >> 8) & 0xff)
/** A macro for 'extracting' the blue value from an NvPackedColor. */
#define NV_PC_BLUE(c) ((c >> 16) & 0xff)
/** A macro for 'extracting' the alpha value from an NvPackedColor. */
#define NV_PC_ALPHA(c) ((c >> 24) & 0xff)

/** A macro requesting the packed color repacked into a 32-bit unsigned int.
    This is a no-op for the color-as-uint approach.
*/
#define NV_PC_PACK_UINT(c) (c)
/** A macro for testing the equality of two NvPackedColors. */
#define NV_PC_EQUAL(x, y) (x == y)
#endif

/** A macro for mapping a single packed color channel into its floating point [0,1] rep. */
#define NV_PC_TO_FLOAT(c) (c / 255.0f)

#endif /*_QT3DS_COLOR_H*/
