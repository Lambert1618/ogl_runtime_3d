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

// we have some build dependencies which forces us to include
// this on linux to recognize malloc
#include <stdio.h>
#include <stdarg.h>

#if !defined(_MSC_VER)
#include <stdlib.h>
#endif

#include "EASTL/allocator.h"

void *operator new[](size_t size, const char *, int, unsigned, const char *, int)
{
    return malloc(size);
}

void *operator new[](size_t size, size_t, size_t, const char *, int, unsigned, const char *, int)
{
    return malloc(size);
}
// EASTL also wants us to define this (see string.h line 197)
int Vsnprintf8(char8_t *pDestination, size_t n, const char8_t *pFormat, va_list arguments)
{
#ifdef _MSC_VER
    return _vsnprintf(pDestination, n, pFormat, arguments);
#else
    return vsnprintf(pDestination, n, pFormat, arguments);
#endif
}
int Vsnprintf16(char8_t *pDestination, size_t n, const char16_t *pFormat, va_list arguments)
{
#ifdef _MSC_VER
    return _vsnprintf(pDestination, n, (char *)pFormat, arguments);
#else
    return vsnprintf(pDestination, n, (char *)pFormat, arguments);
#endif
}
int Vsnprintf32(char8_t *pDestination, size_t n, const char32_t *pFormat, va_list arguments)
{
#ifdef _MSC_VER
    return _vsnprintf(pDestination, n, (char *)pFormat, arguments);
#else
    return vsnprintf(pDestination, n, (char *)pFormat, arguments);
#endif
}
