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

#include "render/backends/gl/Qt3DSOpenGLPrefix.h"

#ifdef _WIN32
//==============================================================================
//	DEFINES
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

//==============================================================================
//	DISABLED WARNINGS
//
//  Note that most of these warnings are tuned off by default by the compiler
//	even at warning level 4.  Using option /Wall turns on all warnings and makes
//	even standard Microsoft include files cry.  We had to turn off these or
//	turn off warnings individually for each standard include file which was
//	too much work.  Point is that /Wall is like Warning Level 5 and this brings
//	it down to about Warning level 4.5, still way above /W4.
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
//#pragma warning( disable : 4355 )	// 'this' : used in base member initializer list
//#pragma warning( disable : 4511 )	// copy constructor could not be generated
#pragma warning(disable : 4512) // assignment operator could not be generated
#pragma warning(disable : 4514) // unreferenced inline function has been removed
//#pragma warning( disable : 4619 )	// #pragma warning : there is no warning number '4619' (in
//string.h)
#pragma warning(disable : 4625) // copy constructor could not be generated because a base class copy
                                // constructor is inaccessible
#pragma warning(disable : 4626) // assignment operator could not be generated because a base class
                                // assignment operator is inaccessible
//#pragma warning( disable : 4640 )	// TODO MF Remove - construction of local static object is
//not thread-safe
#pragma warning(disable : 4710) // function not inlined
#pragma warning(disable : 4711) // function selected for automatic inline expansion
#pragma warning(                                                                                   \
    disable : 4738) // storing 32-bit float result in memory, possible loss of performance
#pragma warning(disable : 4826) // Conversion from 'const void *' to 'void *' is sign-extended
//#pragma warning( disable : 4996 )	// _snprintf' was declared deprecated

#endif //_WIN32

//==============================================================================
//	Non-windows environments need to declare this
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

//==============================================================================
//	STD - Standard includes MUST come first
#ifdef _WIN32
#pragma warning(push, 3)
#pragma warning(                                                                                   \
    disable : 4548) // expression before comma has no effect; expected expression with side-effect
#endif

#include <stdio.h>
#include <iostream>

#if defined(_PCPLATFORM) || defined(_TEGRAPLATFORM)
#include <tchar.h>
#endif

#ifdef _WIN32
#pragma warning(pop)
#endif

//==============================================================================
//	Runtime
//==============================================================================
#include "Qt3DSAssert.h"
#include "Qt3DSMacros.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSQmlEngine.h"
#include "Qt3DSAttributeHashes.h"

//==============================================================================
//	Additional Linux only dependencies
//==============================================================================
#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)

#pragma pack(1)
struct BITMAPFILEHEADER
{
    Q3DStudio::UINT16 bfType;
    Q3DStudio::UINT32 bfSize;
    Q3DStudio::UINT16 bfReserved1;
    Q3DStudio::UINT16 bfReserved2;
    Q3DStudio::UINT32 bfOffBits;
};
#pragma pack()

#pragma pack(1)

typedef struct tagBITMAPINFOHEADER
{
    Q3DStudio::UINT32 biSize;
    Q3DStudio::INT32 biWidth;
    Q3DStudio::INT32 biHeight;
    Q3DStudio::UINT16 biPlanes;
    Q3DStudio::UINT16 biBitCount;
    Q3DStudio::UINT32 biCompression;
    Q3DStudio::UINT32 biSizeImage;
    Q3DStudio::INT32 biXPelsPerMeter;
    Q3DStudio::INT32 biYPelsPerMeter;
    Q3DStudio::UINT32 biClrUsed;
    Q3DStudio::UINT32 biClrImportant;
} BITMAPINFOHEADER;

#pragma pack()

#define UNREFERENCED_PARAMETER(theParam) theParam;

#endif
