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

#ifndef QT3DS_FOUNDATION_QT3DS_WINDOWS_STRING_H
#define QT3DS_FOUNDATION_QT3DS_WINDOWS_STRING_H

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
    ::strcpy_s(dest, size, src);
}
QT3DS_INLINE void NVStrcat(char *dest, size_t size, const char *src)
{
    ::strcat_s(dest, size, src);
}
QT3DS_INLINE QT3DSI32 NVVsprintf(char *dest, size_t size, const char *src, va_list arg)
{
    QT3DSI32 r = ::vsprintf_s(dest, size, src, arg);

    return r;
}
QT3DS_INLINE QT3DSI32 NVStricmp(const char *str, const char *str1)
{
    return (::_stricmp(str, str1));
}

namespace string {
    QT3DS_INLINE QT3DSI32 stricmp(const char *str, const char *str1) { return (::_stricmp(str, str1)); }
    QT3DS_INLINE QT3DSI32 strnicmp(const char *str, const char *str1, size_t len)
    {
        return (::_strnicmp(str, str1, len));
    }
    QT3DS_INLINE QT3DSI32 strncat_s(char *a, QT3DSI32 b, const char *c, size_t d)
    {
        return (::strncat_s(a, b, c, d));
    }
    QT3DS_INLINE QT3DSI32 strncpy_s(char *strDest, size_t sizeInBytes, const char *strSource,
                              size_t count)
    {
        return (::strncpy_s(strDest, sizeInBytes, strSource, count));
    }
    QT3DS_INLINE void strcpy_s(char *dest, size_t size, const char *src)
    {
        ::strcpy_s(dest, size, src);
    }
    QT3DS_INLINE void strcat_s(char *dest, size_t size, const char *src)
    {
        ::strcat_s(dest, size, src);
    }
    QT3DS_INLINE QT3DSI32 _vsnprintf(char *dest, size_t size, const char *src, va_list arg)
    {
        QT3DSI32 r = ::_vsnprintf(dest, size, src, arg);

        return r;
    }
    QT3DS_INLINE QT3DSI32 vsprintf_s(char *dest, size_t size, const char *src, va_list arg)
    {
        QT3DSI32 r = ::vsprintf_s(dest, size, src, arg);

        return r;
    }

    QT3DS_INLINE QT3DSI32 sprintf_s(char *_DstBuf, size_t _DstSize, const char *_Format, ...)
    {
        va_list arg;
        va_start(arg, _Format);
        QT3DSI32 r = ::vsprintf_s(_DstBuf, _DstSize, _Format, arg);
        va_end(arg);

        return r;
    }
    QT3DS_INLINE QT3DSI32 sscanf_s(const char *buffer, const char *format, ...)
    {
        va_list arg;
        va_start(arg, format);
        QT3DSI32 r = ::sscanf_s(buffer, format, arg);
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
