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
#ifndef QT3DS_FOUNDATION_STRING_CONVERSION_H
#define QT3DS_FOUNDATION_STRING_CONVERSION_H
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Utils.h"

namespace qt3ds {
namespace foundation {

    // Template base class so that we can convert items to and from char* and wchar_t*
    // representations
    template <typename TDataType>
    struct StringConversion
    {
        bool force_compile_error;
    };

    // Write the char8_t but exlude the null terminator
    // Meant to write data model values.
    // Memory buffer contains a non-null-terminated
    // char8_t string
    struct Char8TWriter
    {
        MemoryBuffer<ForwardingAllocator> &m_Buffer;
        Char8TWriter(MemoryBuffer<ForwardingAllocator> &buf)
            : m_Buffer(buf)
        {
        }
        void Write(const char8_t *value, QT3DSU32 len = 0)
        {
            if (isTrivial(value))
                return;
            if (len == 0)
                len = (QT3DSU32)StrLen(value);
            m_Buffer.write(value, len);
        }
        void Write(char8_t value) { m_Buffer.write(value); }
        void Write(bool value) { Write(value ? "True" : "False"); }

        // Takes care of long and float
        template <typename TDataType>
        void Write(TDataType value)
        {
            char8_t buf[256];
            QT3DSU32 numWritten =
                StringConversion<TDataType>().ToStr(value, NVDataRef<char8_t>(buf, 256));
            if (numWritten)
                Write((const char8_t *)buf);
            else {
                QT3DS_ASSERT(false);
            }
        }

        template <typename TDataType>
        void Write(NVConstDataRef<TDataType> values, QT3DSU32 grouping = 6, QT3DSU32 tabCount = 0)
        {
            for (QT3DSU32 idx = 0; idx < values.size(); ++idx) {
                if (idx) {
                    if ((idx % grouping) == 0) {
                        Write((char8_t)'\n');
                        for (QT3DSU32 tabIdx = 0; tabIdx < tabCount; ++tabIdx)
                            Write((char8_t)'\t');
                    } else
                        Write((char8_t)' ');
                }
                Write(values[idx]);
            }
        }

        void Write(NVConstDataRef<char8_t> values, QT3DSU32 ignored = 6)
        {
            (void)ignored;
            if (values.size() && values[0] != 0) {
                QT3DSU32 lastItem = values.size() - 1;
                if (values[lastItem] == 0)
                    --lastItem;
                Write(values.begin(), lastItem + 1);
            }
        }
    };

    inline bool IsWhite(char8_t value)
    {
        return value == '\n' || value == '\r' || value == ' ' || value == '\t';
    }

    // skip until we find whitespace.
    inline char8_t *FindNextWhitespace(char8_t *input)
    {
        if (input == NULL)
            return input;
        char8_t *marker = input;
        // Empty loop intentional
        for (; *marker && !IsWhite(*marker); ++marker)
            ;
        return marker;
    }

    // skip until we find something that isn't whitespace.
    inline char8_t *FindNextNonWhitespace(char8_t *input)
    {
        if (input == NULL)
            return input;
        char8_t *marker = input;
        // Empty loop intentional
        for (; *marker && IsWhite(*marker); ++marker)
            ;
        return marker;
    }

    // Reading is destructive in the case of floating point lists, so we may
    // destroy the incoming string.
    // We are assuming the string is null-terminated at end ptr.
    struct Char8TReader
    {
        char8_t *m_StartPtr;
        // Buffer used for temp storage
        MemoryBuffer<ForwardingAllocator> &m_Buffer;
        Char8TReader(char8_t *sp, MemoryBuffer<ForwardingAllocator> &buf)
            : m_StartPtr(sp)
            , m_Buffer(buf)
        {
        }
        void Read(const char8_t *&outPtr) { outPtr = m_StartPtr; }

        template <typename TDataType>
        void Read(TDataType &data)
        {
            bool success = StringConversion<TDataType>().StrTo(m_StartPtr, data);
            QT3DS_ASSERT(success);
            (void)success;
        }
        // Destructive operation because we can't trust
        // strtod to do the right thing.  On windows, for long strings,
        // it calls strlen every operation thus leading to basically N^2
        // behavior
        template <typename TDataType>
        void ReadRef(NVDataRef<TDataType> data)
        {
            QT3DSU32 idx = 0;
            m_StartPtr = FindNextNonWhitespace(m_StartPtr);
            for (; idx < data.size() && m_StartPtr && *m_StartPtr; ++idx) {
                char8_t *nextPtr = FindNextWhitespace(m_StartPtr);
                if (nextPtr && *nextPtr)
                    *nextPtr = 0;
                else
                    nextPtr = NULL;
                StringConversion<TDataType>().StrTo(m_StartPtr, data[idx]);
                m_StartPtr = nextPtr;
                if (m_StartPtr)
                    m_StartPtr = FindNextNonWhitespace(m_StartPtr + 1);
            }
            QT3DS_ASSERT(idx == data.size());
        }

        void ReadBuffer(NVConstDataRef<char8_t> &outBuffer)
        {
            if (m_StartPtr && *m_StartPtr) {
                QT3DSU32 len = (QT3DSU32)strlen(m_StartPtr);
                outBuffer = NVConstDataRef<char8_t>(m_StartPtr, len + 1);
            }
        }

        // Destructive operation because we can't trust
        // strtod to do the right thing.  On windows, for long strings,
        // it calls strlen every operation thus leading to basically N^2
        // behavior
        template <typename TDataType>
        void ReadBuffer(NVConstDataRef<TDataType> &outBuffer)
        {
            m_Buffer.clear();
            m_StartPtr = FindNextNonWhitespace(m_StartPtr);
            while (m_StartPtr && *m_StartPtr) {
                char8_t *nextPtr = FindNextWhitespace(m_StartPtr);
                if (nextPtr && *nextPtr)
                    *nextPtr = 0;
                else
                    nextPtr = NULL;
                TDataType temp;
                StringConversion<TDataType>().StrTo(m_StartPtr, temp);
                m_Buffer.write(temp);
                m_StartPtr = nextPtr;
                if (m_StartPtr)
                    m_StartPtr = FindNextNonWhitespace(m_StartPtr + 1);
            }
            QT3DSU32 numItems = m_Buffer.size() / sizeof(TDataType);
            if (numItems)
                outBuffer = NVConstDataRef<TDataType>((TDataType *)m_Buffer.begin(), numItems);
            else
                outBuffer = NVConstDataRef<TDataType>();
        }
    };
}
}
#endif