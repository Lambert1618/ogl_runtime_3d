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
#ifndef QT3DS_FOUNDATION_STRING_CONVERSION_IMPL_H
#define QT3DS_FOUNDATION_STRING_CONVERSION_IMPL_H
#include "foundation/StringConversion.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "EABase/eabase.h"
#include "foundation/StringTable.h"
#include "foundation/Utils.h"
#include "stdlib.h"
#include "stdio.h" //snprintf
#include <QLocale>

#if !defined EA_PLATFORM_WINDOWS
#define _snprintf snprintf
#endif

namespace qt3ds {
namespace foundation {

    template <>
    struct StringConversion<bool>
    {
        QT3DSU32 ToStr(bool item, NVDataRef<char8_t> buffer)
        {
            return static_cast<QT3DSU32>(
                _snprintf(buffer.begin(), buffer.size(), "%s", item ? "True" : "False"));
        }
        bool StrTo(const char8_t *buffer, bool &item)
        {
            if (AreEqualCaseless(buffer, "True"))
                item = true;
            else
                item = false;
            return true;
        }
    };

    template <>
    struct StringConversion<QT3DSU8>
    {
        QT3DSU32 ToStr(QT3DSU8 item, NVDataRef<char8_t> buffer)
        {
            return static_cast<QT3DSU32>(
                _snprintf(buffer.begin(), buffer.size(), "%hu", static_cast<QT3DSU16>(item)));
        }
        bool StrTo(const char8_t *buffer, QT3DSU8 &item)
        {
            item = static_cast<QT3DSU8>(strtoul(buffer, NULL, 10));
            return true;
        }
    };
    template <>
    struct StringConversion<QT3DSI8>
    {
        QT3DSU32 ToStr(QT3DSI8 item, NVDataRef<char8_t> buffer)
        {
            return static_cast<QT3DSU32>(
                _snprintf(buffer.begin(), buffer.size(), "%hd", static_cast<QT3DSI16>(item)));
        }
        bool StrTo(const char8_t *buffer, QT3DSI8 &item)
        {
            item = static_cast<QT3DSI8>(strtol(buffer, NULL, 10));
            return true;
        }
    };
    template <>
    struct StringConversion<QT3DSU16>
    {
        QT3DSU32 ToStr(QT3DSU16 item, NVDataRef<char8_t> buffer)
        {
            return static_cast<QT3DSU32>(_snprintf(buffer.begin(), buffer.size(), "%hu", item));
        }
        bool StrTo(const char8_t *buffer, QT3DSU16 &item)
        {
            item = static_cast<QT3DSU16>(strtoul(buffer, NULL, 10));
            return true;
        }
    };
    template <>
    struct StringConversion<QT3DSI16>
    {
        QT3DSU32 ToStr(QT3DSI16 item, NVDataRef<char8_t> buffer)
        {
            return static_cast<QT3DSU32>(_snprintf(buffer.begin(), buffer.size(), "%hd", item));
        }
        bool StrTo(const char8_t *buffer, QT3DSI16 &item)
        {
            item = static_cast<QT3DSI16>(strtol(buffer, NULL, 10));
            return true;
        }
    };

    template <>
    struct StringConversion<QT3DSU32>
    {
        QT3DSU32 ToStr(QT3DSU32 item, NVDataRef<char8_t> buffer)
        {
            // hope the buffer is big enough...
            return static_cast<QT3DSU32>(_snprintf(buffer.begin(), buffer.size(), "%u", item));
        }
        bool StrTo(const char8_t *buffer, QT3DSU32 &item)
        {
            item = strtoul(buffer, NULL, 10);
            return true;
        }
    };
    template <>
    struct StringConversion<QT3DSI32>
    {
        QT3DSU32 ToStr(QT3DSI32 item, NVDataRef<char8_t> buffer)
        {
            return static_cast<QT3DSU32>(_snprintf(buffer.begin(), buffer.size(), "%d", (int)item));
        }
        bool StrTo(const char8_t *buffer, QT3DSI32 &item)
        {
            item = strtol(buffer, NULL, 10);
            return true;
        }
    };
    template <>
    struct StringConversion<QT3DSU64>
    {
        QT3DSU32 ToStr(QT3DSU64 item, NVDataRef<char8_t> buffer)
        {
#ifdef _WIN32
            return static_cast<QT3DSU32>(_snprintf(buffer.begin(), buffer.size(), "%I64u", item));
#else
            return static_cast<QT3DSU32>(_snprintf(buffer.begin(), buffer.size(), "%lu", item));
#endif
        }
        bool StrTo(const char8_t *buffer, QT3DSU64 &item)
        {
            item = strtoul(buffer, NULL, 10);
            return true;
        }
    };
    template <>
    struct StringConversion<QT3DSI64>
    {
        QT3DSU32 ToStr(QT3DSI64 item, NVDataRef<char8_t> buffer)
        {
#ifdef _WIN32
            return static_cast<QT3DSU32>(_snprintf(buffer.begin(), buffer.size(), "%I64d", item));
#else
            return static_cast<QT3DSU32>(_snprintf(buffer.begin(), buffer.size(), "%ld", item));
#endif
        }
        bool StrTo(const char8_t *buffer, QT3DSI64 &item)
        {
            item = strtol(buffer, NULL, 10);
            return true;
        }
    };
    template <>
    struct StringConversion<QT3DSF32>
    {
        QT3DSU32 ToStr(QT3DSF32 item, NVDataRef<char8_t> buffer)
        {
            QString s = QLocale::c().toString(item);
            strncpy(buffer.begin(), s.toStdString().c_str(), buffer.size());
            return s.length();
        }
        bool StrTo(const char8_t *buffer, QT3DSF32 &item)
        {
            bool ok;
            item = QLocale::c().toFloat(buffer, &ok);
            return ok;
        }
    };

    template <>
    struct StringConversion<QT3DSF64>
    {
        QT3DSU32 ToStr(QT3DSF64 item, NVDataRef<char8_t> buffer)
        {
            QString s = QLocale::c().toString(item);
            strncpy(buffer.begin(), s.toStdString().c_str(), buffer.size());
            return s.length();
        }
        bool StrTo(const char8_t *buffer, QT3DSF64 &item)
        {
            bool ok;
            item = QLocale::c().toDouble(buffer, &ok);
            return ok;
        }
    };
}
}

#endif
