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

#ifndef QT3DS_FOUNDATION_QT3DS_LINUX_STRING_H
#define QT3DS_FOUNDATION_QT3DS_LINUX_STRING_H

#include "foundation/Qt3DS.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#pragma warning(push)
#pragma warning(disable : 4995 4996)

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

QT3DS_INLINE void NVStrcpy(char *dest, size_t size, const char *src)
{
    ::strncpy(dest, src, size);
}

QT3DS_INLINE int NVStrcat(char *dest, size_t size, const char *src)
{
    ::strcat(dest, src);
    return 0;
}
QT3DS_INLINE int NVVsprintf(char *dest, size_t size, const char *src, va_list arg)
{
    int r = ::vsprintf(dest, src, arg);

    return r;
}
QT3DS_INLINE int NVStricmp(const char *str, const char *str1)
{
    return (::strcasecmp(str, str1));
}

namespace string {

    QT3DS_INLINE void strcpy(char *dest, size_t size, const char *src) { ::strcpy(dest, src); }
    QT3DS_INLINE void strcat(char *dest, size_t size, const char *src) { ::strcat(dest, src); }
    // QT3DS_INLINE int strcasecmp(const char *str, const char *str1) {return(::strcasecmp(str,
    // str1));}
    QT3DS_INLINE QT3DSI32 stricmp(const char *str, const char *str1) { return (::strcasecmp(str, str1)); }
    QT3DS_INLINE QT3DSI32 strnicmp(const char *str, const char *str1, size_t len)
    {
        return (::strncasecmp(str, str1, len));
    }

    QT3DS_INLINE QT3DSI32 strncat_s(char *dstBfr, size_t dstSize, const char *srcBfr, size_t numCpy)
    {
        if (!dstBfr || !srcBfr || !dstSize)
            return -1;

        size_t len = strlen(dstBfr);

        if (len >= dstSize)
            return -1;

        size_t remain = dstSize - len - 1;
        size_t transfer = remain > numCpy ? numCpy : remain;
        ::memmove(dstBfr + len, srcBfr, transfer);
        dstBfr[len + transfer] = '\0';
        return numCpy <= remain ? 0 : -1;
    }

    QT3DS_INLINE QT3DSI32 _vsnprintf(char *dest, size_t size, const char *src, va_list arg)
    {
        QT3DSI32 r = ::vsnprintf(dest, size, src, arg);

        return r;
    }
    QT3DS_INLINE QT3DSI32 vsprintf(char *dest, size_t size, const char *src, va_list arg)
    {
        QT3DSI32 r = ::vsprintf(dest, src, arg);

        return r;
    }
    QT3DS_INLINE int vsprintf_s(char *dest, size_t size, const char *src, va_list arg)
    {
        int r = ::vsprintf(dest, src, arg);

        return r;
    }

    QT3DS_INLINE int sprintf_s(char *_DstBuf, size_t _DstSize, const char *_Format, ...)
    {
        if (_DstBuf == NULL || _Format == NULL) {
            return -1;
        }

        va_list arg;
        va_start(arg, _Format);
        int r = ::vsprintf(_DstBuf, _Format, arg);
        va_end(arg);

        return r;
    }

    QT3DS_INLINE QT3DSI32 sprintf(char *_DstBuf, size_t _DstSize, const char *_Format, ...)
    {
        va_list arg;
        va_start(arg, _Format);
        QT3DSI32 r = ::vsprintf(_DstBuf, _Format, arg);
        va_end(arg);

        return r;
    }

    QT3DS_INLINE int strncpy_s(char *strDest, size_t sizeInBytes, const char *strSource, size_t count)
    {
        if (strDest == NULL || strSource == NULL || sizeInBytes == 0) {
            return -1;
        }

        if (sizeInBytes < count) {
            strDest[0] = 0;
            return -1;
        }

        ::strncpy(strDest, strSource, count);
        return 0;
    }

    QT3DS_INLINE void strcpy_s(char *dest, size_t size, const char *src)
    {
        ::strncpy(dest, src, size);
    }

    QT3DS_INLINE int strcat_s(char *dest, size_t size, const char *src)
    {
        ::strcat(dest, src);
        return 0;
    }

    QT3DS_INLINE QT3DSI32 sscanf(const char *buffer, const char *format, ...)
    {
        va_list arg;
        va_start(arg, format);
        QT3DSI32 r = ::sscanf(buffer, format, arg);
        va_end(arg);

        return r;
    };

    QT3DS_INLINE void strlwr(char *str)
    {
        while (*str) {
            if (*str >= 'A' && *str <= 'Z')
                *str += 32;
            str++;
        }
    }

    QT3DS_INLINE void strupr(char *str)
    {
        while (*str) {
            if (*str >= 'a' && *str <= 'z')
                *str -= 32;
            str++;
        }
    }

} // namespace string

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

#pragma warning(pop)

#endif
