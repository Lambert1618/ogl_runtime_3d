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
//	DISABLED WARNINGS
//
//  Note that most of these warnings are tuned off by default by the compiler
//	even at warning level 4.  Using option /Wall turns on all warnings and makes
//	even standard Microsoft include files cry.  We had to turn off these or
//	turn off warnings individually for each standard include file which was
//	too much work.  Point is that /Wall is like Warning Level 5 and this brings
//	it down to about Warning level 4.5, still way above /W4.
//#pragma warning( disable : 4189 )	// local variable is initialized but not referenced
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4511) // copy constructor could not be generated
#pragma warning(disable : 4512) // assignment operator could not be generated
#pragma warning(disable : 4514) // unreferenced inline function has been removed
#pragma warning(disable : 4619) // #pragma warning : there is no warning number '4619' (in string.h)
#pragma warning(disable : 4625) // copy constructor could not be generated because a base class copy
                                // constructor is inaccessible
#pragma warning(disable : 4626) // assignment operator could not be generated because a base class
                                // assignment operator is inaccessible
#pragma warning(                                                                                   \
    disable : 4668) // not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(disable : 4826) // Conversion from 'const void *' to 'void *' is sign-extended
#pragma warning(disable : 4996) // _snprintf' was declared deprecated
#pragma warning(disable : 4711) // function selected for automatic inline expansion
#pragma warning(disable : 4710) // function not inlined
#pragma warning(                                                                                   \
    disable : 4738) // storing 32-bit float result in memory, possible loss of performance

#pragma warning(disable : 4640) // construction of local static object is not thread-safe
#pragma warning(disable : 4127) // conditional expression is constant

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif //_WIN32

//==============================================================================
//	STD INCLUDES - Standard includes MUST come first
#include <stdlib.h> // Standard functions
#include <stdio.h> // File and IO
#ifndef _INTEGRITYPLATFORM
#include <memory.h> // memset, memcpy
#endif
#include <string.h> // strlen

#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
#include <stdint.h>

#define UNREFERENCED_PARAMETER(theParam) theParam;
#endif

//==============================================================================
//	ROOT INCLUDES FOR FRAMEWORK
//==============================================================================
#include "Qt3DSTypes.h"
#include "Qt3DSPlatformSpecific.h"
#include "Qt3DSAssert.h"
#include "Qt3DSMacros.h"
#include "Qt3DSConfig.h"
#include "Qt3DSMemorySettings.h"
#include "Qt3DSMemory.h"
#include "Qt3DSFrameworkTypes.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSTimer.h"
#include "Qt3DSIScene.h"
#include "foundation/Qt3DSRefCounted.h" //scoped releasable auto ptr.
#include "EASTL/hash_map.h"
#include "EASTL/string.h"
