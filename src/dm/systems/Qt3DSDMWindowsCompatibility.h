/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DSDM_WINDOWS_COMPATIBILITY_H
#define QT3DSDM_WINDOWS_COMPATIBILITY_H
#include "EABase/config/eaplatform.h"
#include "EABase/eabase.h"
#include <string>
#ifndef __INTEGRITY
#include <memory.h>
#endif
#include <string.h>
#include <stdio.h>

#if defined QT3DS_PLATFORM_NEEDS_WCHAR_T

inline size_t wcslen(const wchar_t *inStr)
{
    size_t retval = 0;
    while (inStr && *inStr) {
        ++retval;
        ++inStr;
    }
    return retval;
}

inline int wmemcmp(const wchar_t *lhs, const wchar_t *rhs, size_t count)
{
    return memcmp(lhs, rhs, count * sizeof(wchar_t));
}

inline wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t count)
{
    memcpy(dest, src, count * sizeof(wchar_t));
    return dest;
}

inline const wchar_t *wmemchr(const wchar_t *inFirst, size_t count, const wchar_t val)
{
    size_t idx;
    // empty loop intentional
    for (idx = 0; idx < count && inFirst[idx] != val; ++idx) {
    }
    if (idx < count)
        return inFirst + idx;
    return NULL;
}

inline wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t count)
{
    memmove(dest, src, count * sizeof(wchar_t));
    return dest;
}

inline wchar_t *wmemset(wchar_t *dest, size_t count, wchar_t val)
{
    for (size_t idx = 0; idx < count; ++idx)
        dest[idx] = val;
    return dest;
}

#endif

inline size_t WideToNarrow(char *inDest, size_t inDestLen, const wchar_t *inWideStr);
template <unsigned int N>
inline size_t WideToNarrow(char (&inDest)[N], const wchar_t *inWideStr);

#if !defined EA_PLATFORM_WINDOWS

inline void wcscpy_s(wchar_t *inDest, size_t destLen, const wchar_t *inMessage)
{
    if (destLen) {
        size_t idx = 0;
        while (inMessage && *inMessage && idx < destLen) {
            inDest[idx] = *inMessage;
            ++inMessage;
            ++idx;
        }

        idx = idx < destLen ? idx : destLen - 1;
        inDest[idx] = 0;
    }
}

template <unsigned int N>
inline void wcscpy_s(wchar_t (&inDest)[N], const wchar_t *inMessage)
{
    wcscpy_s(inDest, N, inMessage);
}

inline FILE *_wfopen(const wchar_t *inFname, const wchar_t *inFlags)
{
    char name[1024] = { 0 };
    WideToNarrow(name, inFname);
    char flags[24] = { 0 };
    WideToNarrow(flags, inFlags);
    return fopen(name, flags);
}

inline int _fseeki64(FILE *inFile, int64_t pos, int seekFlags)
{
    return fseek(inFile, (int32_t)pos, seekFlags);
}

inline int64_t _ftelli64(FILE *inFile)
{
    return ftell(inFile);
}

#define _snprintf snprintf

#endif

template <typename TLHS, typename TRHS>
inline size_t ConvertStr(TLHS *inDest, size_t inDestLen, const TRHS *inWideStr)
{
    if (inDestLen == 0)
        return 0;

    size_t nameLen = 0;

    // empty loop intentional.
    for (const TRHS *tempPtr = inWideStr; tempPtr && *tempPtr; ++tempPtr, ++nameLen) {
    }

    if (nameLen > inDestLen - 1)
        nameLen = inDestLen - 1;
    for (size_t idx = 0; idx < nameLen; ++idx)
        inDest[idx] = (char)inWideStr[idx];
    inDest[nameLen] = 0;
    return nameLen;
}

inline size_t WideToNarrow(char *inDest, size_t inDestLen, const wchar_t *inWideStr)
{
    return ConvertStr(inDest, inDestLen, inWideStr);
}

template <unsigned int N>
inline size_t WideToNarrow(char (&inDest)[N], const wchar_t *inWideStr)
{
    return WideToNarrow(inDest, N, inWideStr);
}

inline size_t WideToNarrow(char *inDest, size_t inDestLen, const char16_t *inWideStr)
{
    return ConvertStr(inDest, inDestLen, inWideStr);
}

template <unsigned int N>
inline size_t WideToNarrow(char (&inDest)[N], const char16_t *inWideStr)
{
    return WideToNarrow(inDest, N, inWideStr);
}

#endif