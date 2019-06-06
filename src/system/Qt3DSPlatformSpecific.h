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

#if defined(_PCPLATFORM)

#define Q3DStudio_memcpy(inDest, inSource, inCount)                                                \
    memcpy(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memmove(inDest, inSource, inCount)                                               \
    memmove(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memset(inDest, inChar, inCount)                                                  \
    memset(inDest, inChar, static_cast<size_t>(inCount))
#define Q3DStudio_memcmp(inDest, inSource, inCount)                                                \
    memcmp(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_sprintf _snprintf
#define Q3DStudio_stricmp(inStr1, inStr2) _stricmp(inStr1, inStr2)
#define Q3DStudio_strnicmp(inStr1, inStr2, inCount) _strnicmp(inStr1, inStr2, inCount)
#define Q3DStudio_tolower tolower
#define Q3DStudio_restrict /*__restrict*/
#define Q3DStudio_dcbt(inOffset, inAddress)
#define Q3DStudio_strcpy(inDest, inSize, inSource) strcpy_s(inDest, inSize, inSource)
#define Q3DStudio_strcat(inDest, inSize, inSource) strcat_s(inDest, inSize, inSource)
#define Q3DStudio_fopen fopen_s
#define Q3DStudio_sleepmillisec(inMillisec) Sleep(inMillisec)
#define Q3DStudio_getpid GetCurrentProcessId

#elif defined(_TEGRAPLATFORM)

#define Q3DStudio_memcpy(inDest, inSource, inCount)                                                \
    memcpy(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memmove(inDest, inSource, inCount)                                               \
    memmove(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memset(inDest, inChar, inCount)                                                  \
    memset(inDest, inChar, static_cast<size_t>(inCount))
#define Q3DStudio_memcmp(inDest, inSource, inCount)                                                \
    memcmp(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_sprintf _snprintf
#define Q3DStudio_stricmp(inStr1, inStr2) _stricmp(inStr1, inStr2)
#define Q3DStudio_strnicmp(inStr1, inStr2, inCount) _strnicmp(inStr1, inStr2, inCount)
#define Q3DStudio_tolower tolower
#define Q3DStudio_restrict /*__restrict*/
#define Q3DStudio_dcbt(inOffset, inAddress)
#define Q3DStudio_strcpy(inDest, inSize, inSource) strcpy_s(inDest, inSize, inSource)
#define Q3DStudio_strcat(inDest, inSize, inSource) strcat_s(inDest, inSize, inSource)
#define Q3DStudio_fopen(inFilePtr, inFilename, inMode) fopen_s(inFilePtr, inFilename, inMode)
#define Q3DStudio_sleepmillisec(inMillisec) Sleep(inMillisec)
#define Q3DStudio_getpid GetCurrentProcessId

#elif defined(_XENONPLATFORM)

#include <xtl.h>
#define Q3DStudio_memcpy(inDest, inSource, inCount)                                                \
    memcpy(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memmove(inDest, inSource, inCount)                                               \
    memmove(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memset(inDest, inChar, inCount)                                                  \
    memset(inDest, inChar, static_cast<size_t>(inCount))
#define Q3DStudio_memcmp(inDest, inSource, inCount)                                                \
    memcmp(inDest, inSource, static_cast<size_t>(inCount))
//#define Q3DStudio_memcpy( inDest, inSource, inCount )		XMemCpy( inDest, inSource,
//static_cast<SIZE_T>( inCount ) )
//#define Q3DStudio_memset( inDest, inChar, inCount )		XMemSet( inDest, inChar,
//static_cast<SIZE_T>( inCount ) )
#define Q3DStudio_sprintf _snprintf
#define Q3DStudio_tolower tolower
#define Q3DStudio_restrict /*__restrict*/
#define Q3DStudio_dcbt(inOffset, inAddress) __dcbt(inOffset, inAddress)
#define Q3DStudio_strcpy(inDest, inSize, inSource) strcpy_s(inDest, inSize, inSource)
#define Q3DStudio_strcat(inDest, inSize, inSource) strcat_s(inDest, inSize, inSource)
#define Q3DStudio_fopen(inFilePtr, inFilename, inMode) fopen_s(inFilePtr, inFilename, inMode)
#define Q3DStudio_sleepmillisec(inMillisec) Sleep(inMillisec)
#define Q3DStudio_getpid GetCurrentProcessId
#define Q3DStudio_strnicmp(inStr1, inStr2, inCount) _strnicmp(inStr1, inStr2, inCount)
#define Q3DStudio_stricmp(inStr1, inStr2) _stricmp(inStr1, inStr2)

#elif defined(_PS3PLATFORM)

#define Q3DStudio_memcpy(inDest, inSource, inCount)                                                \
    memcpy(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memmove(inDest, inSource, inCount)                                               \
    memmove(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memset(inDest, inChar, inCount)                                                  \
    memset(inDest, inChar, static_cast<size_t>(inCount))
#define Q3DStudio_memcmp(inDest, inSource, inCount)                                                \
    memcmp(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_sprintf snprintf
#define Q3DStudio_tolower std::tolower
#define Q3DStudio_restrict
#define Q3DStudio_dcbt(inOffset, inAddress)
#define Q3DStudio_strcpy(inDest, inSize, inSource) strcpy(inDest, inSource)
#define Q3DStudio_strcat(inDest, inSize, inSource) strcat(inDest, inSource)
#define Q3DStudio_fopen(inFilePtr, inFilename, inMode) *(inFilePtr) = fopen(inFilename, inMode)
#define Q3DStudio_sleepmillisec(inMillisec)                                                        \
    {                                                                                              \
        struct timespec theSleepTime = { inMillisec / 1000, (inMillisec % 1000) * 1000000 };       \
        ::nanosleep(&theSleepTime, NULL);                                                          \
    }
#define Q3DStudio_getpid getpid
#define Q3DStudio_stricmp(inStr1, inStr2) strcasecmp(inStr1, inStr2)
#define Q3DStudio_strnicmp(inStr1, inStr2, inCount) strncasecmp(inStr1, inStr2, inCount)

#elif defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
#include <time.h>
#define Q3DStudio_memcpy(inDest, inSource, inCount)                                                \
    memcpy(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memmove(inDest, inSource, inCount)                                               \
    memmove(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_memset(inDest, inChar, inCount)                                                  \
    memset(inDest, inChar, static_cast<size_t>(inCount))
#define Q3DStudio_memcmp(inDest, inSource, inCount)                                                \
    memcmp(inDest, inSource, static_cast<size_t>(inCount))
#define Q3DStudio_sprintf snprintf
#define Q3DStudio_stricmp(inStr1, inStr2) strcasecmp(inStr1, inStr2)
#define Q3DStudio_strnicmp(inStr1, inStr2, inCount) strncasecmp(inStr1, inStr2, inCount)
#define Q3DStudio_tolower tolower
#define Q3DStudio_restrict /*__restrict*/
#define Q3DStudio_dcbt(inOffset, inAddress)
#define Q3DStudio_strcpy(inDest, inSize, inSource) strcpy(inDest, inSource)
#define Q3DStudio_strcat(inDest, inSize, inSource) strcat(inDest, inSource)
#define Q3DStudio_fopen(inFilePtr, inFilename, inMode) *(inFilePtr) = fopen(inFilename, inMode)
#define Q3DStudio_sleepmillisec(inMillisec)                                                        \
    {                                                                                              \
        struct timespec theSleepTime = { inMillisec / 1000, (inMillisec % 1000) * 1000000 };       \
        ::nanosleep(&theSleepTime, NULL);                                                          \
    }
#define Q3DStudio_getpid getpid

#else
#error "A platform must be defined"
#endif

//==============================================================================
/** @def	Q3DStudio_dcbt( inOffset, inAddress )
 *	@brief	Data Cache Block Touch - Loads the block of memory containing
 *			the specified address into the data cache.
 *
 *	A program uses the dcbt instruction to request a cache block fetch before
 *	it is actually needed by the program. This prevents stalls in the pipeline.
 */

//==============================================================================
/** @def	Q3DStudio_restrict
 *	@brief	Cross platform macro for pointer aliasing.
 *
 *	Performance Implications of Pointer Aliasing
 *
 *	Doug Cook
 *	Silicon Graphics, Inc.
 *	August, 1997
 *
 *	Pointer aliasing can have a severe impact on program performance.
 *	Understanding its implications is critical to writing  high-performance code.
 *	This document provides a brief introduction to the problem, and suggests
 *	several approaches to solving it through source-code restructuring, compiler
 *	options, and C or C++ language extensions.
 *
 *	*Aliasing*
 *
 *	Here's a brief overview of aliasing. Consider the following function:
 *
 *	void process_data(float *in, float *out, float gain, int nsamps)
 *	{
 *		int i;
 *	    for (i = 0; i < nsamps; i++) {
 *		    out[i] = in[i] * gain;
 *		}
 *	}
 *
 *	In C or C++, it is legal for the parameters in and out to point to
 *	overlapping regions in memory. When this happens, in and out are said to be
 *	aliases. When the compiler optimizes the function, it does not in general
 *	know whether in and out are aliases. It must therefore assume that any
 *	store through out can affect the memory pointed to by in, which severely
 *	limits its ability to reorder or parallelize the code (For some simple
 *	cases, the compiler could analyze the entire program to determine that two
 *	pointers cannot be aliases. But in general, it is impossible for the
 *	compiler to determine whether or not two pointers are aliases, so to be
 *	safe, it must assume that they are).
 */
