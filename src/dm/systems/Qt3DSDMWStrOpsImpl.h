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
#ifndef QT3DS_IMPORT_WSTR_OPS_IMPL_H
#define QT3DS_IMPORT_WSTR_OPS_IMPL_H
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMWStrOps.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMAnimation.h"
#include "Qt3DSDMMetaDataTypes.h"
#include "Qt3DSDMXML.h"
#include "EABase/eabase.h"
#include "Qt3DSDMStringTable.h"

namespace qt3dsdm {
using qt3ds::QT3DSI8;
using qt3ds::QT3DSU8;
using qt3ds::QT3DSI16;
using qt3ds::QT3DSU16;
using qt3ds::QT3DSU32;
using qt3ds::QT3DSI32;
using qt3ds::QT3DSU64;
using qt3ds::QT3DSI64;
using qt3ds::QT3DSF32;
using qt3ds::QT3DSF64;
template <>
struct WStrOps<bool>
{
    QT3DSU32 ToStr(bool item, NVDataRef<wchar_t> buffer)
    {
        return static_cast<QT3DSU32>(
            swprintf(buffer.begin(), buffer.size(), L"%s", item ? L"True" : L"False"));
    }
    bool StrTo(const wchar_t *buffer, bool &item)
    {
        if (AreEqual(buffer, L"True"))
            item = true;
        else
            item = false;
        return true;
    }
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
struct WStrOps<QT3DSU8>
{
    QT3DSU32 ToStr(QT3DSU8 item, NVDataRef<wchar_t> buffer)
    {
        return static_cast<QT3DSU32>(
            swprintf(buffer.begin(), buffer.size(), L"%hu", static_cast<QT3DSU16>(item)));
    }
    bool StrTo(const char8_t *buffer, QT3DSU8 &item)
    {
        item = static_cast<QT3DSU8>(strtoul(buffer, NULL, 10));
        return true;
    }
};
template <>
struct WStrOps<QT3DSI8>
{
    QT3DSU32 ToStr(QT3DSI8 item, NVDataRef<wchar_t> buffer)
    {
        return static_cast<QT3DSU32>(
            swprintf(buffer.begin(), buffer.size(), L"%hd", static_cast<QT3DSI16>(item)));
    }
    bool StrTo(const char8_t *buffer, QT3DSI8 &item)
    {
        item = static_cast<QT3DSI8>(strtol(buffer, NULL, 10));
        return true;
    }
};
template <>
struct WStrOps<QT3DSU16>
{
    QT3DSU32 ToStr(QT3DSU16 item, NVDataRef<wchar_t> buffer)
    {
        return static_cast<QT3DSU32>(swprintf(buffer.begin(), buffer.size(), L"%hu", item));
    }
    bool StrTo(const char8_t *buffer, QT3DSU16 &item)
    {
        item = static_cast<QT3DSU16>(strtoul(buffer, NULL, 10));
        return true;
    }
};
template <>
struct WStrOps<QT3DSI16>
{
    QT3DSU32 ToStr(QT3DSI16 item, NVDataRef<wchar_t> buffer)
    {
        return static_cast<QT3DSU32>(swprintf(buffer.begin(), buffer.size(), L"%hd", item));
    }
    QT3DSU32 ToStr(QT3DSI16 item, NVDataRef<char8_t> buffer)
    {
        return static_cast<QT3DSU32>(sprintf(buffer.begin(), "%hd", item));
    }
    bool StrTo(const char8_t *buffer, QT3DSI16 &item)
    {
        item = static_cast<QT3DSI16>(strtol(buffer, NULL, 10));
        return true;
    }
};

template <>
struct WStrOps<QT3DSU32>
{
    QT3DSU32 ToStr(QT3DSU32 item, NVDataRef<wchar_t> buffer)
    {
        return static_cast<QT3DSU32>(swprintf(buffer.begin(), buffer.size(), L"%lu", item));
    }
    QT3DSU32 ToStr(QT3DSU32 item, NVDataRef<char8_t> buffer)
    {
        // hope the buffer is big enough...
        return static_cast<QT3DSU32>(sprintf(buffer.begin(), "%u", item));
    }
    bool StrTo(const char8_t *buffer, QT3DSU32 &item)
    {
        item = strtoul(buffer, NULL, 10);
        return true;
    }
};

template <>
struct WStrOps<QT3DSI32>
{
    QT3DSU32 ToStr(QT3DSI32 item, NVDataRef<wchar_t> buffer)
    {
        return static_cast<QT3DSU32>(swprintf(buffer.begin(), buffer.size(), L"%ld", item));
    }
    bool StrTo(const char8_t *buffer, QT3DSI32 &item)
    {
        item = strtol(buffer, NULL, 10);
        return true;
    }
};
template <>
struct WStrOps<QT3DSF32>
{
    QT3DSU32 ToStr(QT3DSF32 item, NVDataRef<wchar_t> buffer)
    {
        QString s = QLocale::c().toString(item);
        wcsncpy(buffer.begin(), s.toStdWString().c_str(), buffer.size());
        return s.length();
    }
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
struct WStrOps<QT3DSF64>
{
    QT3DSU32 ToStr(QT3DSF64 item, NVDataRef<wchar_t> buffer)
    {
        QString s = QLocale::c().toString(item);
        wcsncpy(buffer.begin(), s.toStdWString().c_str(), buffer.size());
        return s.length();
    }
    bool StrTo(const char8_t *buffer, QT3DSF64 &item)
    {
        bool ok;
        item = QLocale::c().toDouble(buffer, &ok);
        return ok;
    }
};
#define QT3DS_WCHAR_T_None L"None"
#define QT3DS_WCHAR_T_Float L"Float"
#define QT3DS_WCHAR_T_Float2 L"Float2"
#define QT3DS_WCHAR_T_Float3 L"Float3"
#define QT3DS_WCHAR_T_Float4 L"Float4"
#define QT3DS_WCHAR_T_Long L"Long"
#define QT3DS_WCHAR_T_String L"String"
#define QT3DS_WCHAR_T_Bool L"Bool"
#define QT3DS_WCHAR_T_Long4 L"Long4"
#define QT3DS_WCHAR_T_StringRef L"StringRef"
#define QT3DS_WCHAR_T_ObjectRef L"ObjectRef"
#define QT3DS_WCHAR_T_StringOrInt L"StringOrInt"
#define QT3DS_WCHAR_T_FloatList L"FloatList"

#define QT3DS_IMPORT_ITERATE_DMTYPE                                                                \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::None, None, QT3DSF32)                            \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::Float, Float, QT3DSF32)                          \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::Float2, Float2, SFloat2)                         \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::Float3, Float3, SFloat3)                         \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::Float4, Float4, SFloat4)                         \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::Long, Long, QT3DSI32)                            \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::String, String, TDataStrPtr)                     \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::Bool, Bool, bool)                                \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::Long4, Long4, SLong4)                            \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::StringRef, StringRef, SStringRef)                \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::ObjectRef, ObjectRef, SObjectRefType)            \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::StringOrInt, StringOrInt, SStringOrInt)          \
    QT3DS_IMPORT_HANDLE_DMTYPE(DataModelDataType::FloatList, FloatList, TFloatList)

template <>
struct WStrOps<DataModelDataType::Value>
{
    QT3DSU32 ToStr(DataModelDataType::Value item, NVDataRef<wchar_t> buffer)
    {
        const wchar_t *data = NULL;
        switch (item) {
#define QT3DS_IMPORT_HANDLE_DMTYPE(x, y, z)                                                        \
    case x:                                                                                        \
        data = QT3DS_WCHAR_T_##y;                                                                  \
        break;
            QT3DS_IMPORT_ITERATE_DMTYPE
#undef QT3DS_IMPORT_HANDLE_DMTYPE
    default:
        break;
        }
        if (data == NULL) {
            QT3DS_ASSERT(false);
            data = L"Unknown";
        }
        return (QT3DSU32)swprintf(buffer.begin(), buffer.size(), L"%ls", data);
    }
    bool StrTo(const wchar_t *buffer, DataModelDataType::Value &item)
    {
#define QT3DS_IMPORT_HANDLE_DMTYPE(x, y, z)                                                        \
    if (AreEqual(buffer, QT3DS_WCHAR_T_##y)) {                                                     \
        item = x;                                                                                  \
        return true;                                                                               \
    }
        QT3DS_IMPORT_ITERATE_DMTYPE
#undef QT3DS_IMPORT_HANDLE_DMTYPE
        item = DataModelDataType::None;
        return false;
    }
};
template <typename TBufferType>
struct WStrOpsDMWriter
{
    TBufferType &buf;
    WStrOpsDMWriter(TBufferType &_buf)
        : buf(_buf)
    {
    }

    void operator()(float val) { buf.Write(val); }
    void operator()(const SFloat2 &val) { buf.Write(NVConstDataRef<QT3DSF32>(&val[0], 2)); }
    void operator()(const SFloat3 &val) { buf.Write(NVConstDataRef<QT3DSF32>(&val[0], 3)); }
    void operator()(const SFloat4 &val) { buf.Write(NVConstDataRef<QT3DSF32>(&val[0], 4)); }
    void operator()(QT3DSI32 val) { buf.Write(val); }
    void operator()(bool val) { buf.Write(val); }
    void operator()(const TDataStrPtr &val)
    {
        if (val != NULL)
            buf.Write(NVConstDataRef<wchar_t>(val->GetData(), (QT3DSU32)val->GetLength()));
    }
    void operator()(const SLong4 &val)
    {
        buf.Write(NVConstDataRef<QT3DSU32>(&val.m_Longs[0], 4), 4);
    }
    void operator()(const SStringRef &val) { buf.Write(val.m_Id, (QT3DSU32)wcslen(val.m_Id) + 1); }
    void operator()(const SObjectRefType &val) { val.m_Value.visit<void>(*this); }
    void operator()(const SStringOrInt &val) { val.m_Value.visit<void>(*this); }
    void operator()(const TFloatList &val)
    {
        buf.Write(NVConstDataRef<float>(val.data(), (QT3DSU32)val.size()));
    }
    void operator()() { QT3DS_ASSERT(false); }
};

template <>
struct WStrOps<SValue>
{
    template <typename TBufferType>
    void ToBuf(const SValue &item, TBufferType &outBuffer)
    {
        WStrOpsDMWriter<TBufferType> writer(outBuffer);
        item.visit<void>(writer);
    }
    template <typename TBufferType>
    SValue BufTo(DataModelDataType::Value type, TBufferType &inReader)
    {
        switch (type) {
#define QT3DS_IMPORT_HANDLE_DMTYPE(x, y, z)                                                        \
    case x: {                                                                                      \
        z retval;                                                                                  \
        Read(inReader, retval);                                                                    \
        return retval;                                                                             \
    }
            QT3DS_IMPORT_ITERATE_DMTYPE
#undef QT3DS_IMPORT_HANDLE_DMTYPE
    default:
        break;
        }
        QT3DS_ASSERT(false);
        return SValue();
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, float &val)
    {
        reader.Read(val);
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, SFloat2 &val)
    {
        reader.ReadRef(NVDataRef<QT3DSF32>(&val[0], 2));
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, SFloat3 &val)
    {
        reader.ReadRef(NVDataRef<QT3DSF32>(&val[0], 3));
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, SFloat4 &val)
    {
        reader.ReadRef(NVDataRef<QT3DSF32>(&val[0], 4));
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, QT3DSI32 &val)
    {
        reader.Read(val);
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, bool &val)
    {
        reader.Read(val);
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, TDataStrPtr &val)
    {
        NVConstDataRef<wchar_t> buffer;
        reader.ReadBuffer(buffer);
        val = std::make_shared<CDataStr>(buffer.begin(), buffer.size());
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, SLong4 &val)
    {
        reader.ReadRef(NVDataRef<QT3DSU32>(&val.m_Longs[0], 4));
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, SStringRef &val)
    {
        NVConstDataRef<wchar_t> buffer;
        reader.ReadBuffer(buffer);
        val = SStringRef(buffer.begin());
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, TFloatList &val)
    {
        NVConstDataRef<QT3DSF32> buffer;
        reader.ReadBuffer(buffer);
        val.assign(buffer.begin(), buffer.end());
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, SObjectRefType &val)
    {
        // Force the read to be as string; callers can always convert to a different
        // format later if required.
        TDataStrPtr theValue;
        Read(reader, theValue);
        val.m_Value = theValue;
    }
    template <typename TBufferType>
    void Read(TBufferType &reader, SStringOrInt &val)
    {
        NVConstDataRef<char8_t> buffer;
        reader.ReadBuffer(buffer);
        if (buffer.size() == 0)
            return;

        if ((buffer[0] >= '0' && buffer[0] <= '9') || buffer[0] == '-') {
            QT3DSI32 theVal;
            WStrOps<QT3DSI32>().StrTo(buffer.begin(), theVal);
            val = SStringOrInt(theVal);
        } else {
            NVConstDataRef<wchar_t> wideBuffer;
            reader.ReadBuffer(wideBuffer);
            val = SStringOrInt(std::make_shared<CDataStr>(wideBuffer.begin()));
        }
    }
};

// Write the wchar_t but exlude the null terminator
// Meant to write data model values.
// Memory buffer contains a non-null-terminated
// wchar_t string
struct WCharTWriter
{
    MemoryBuffer<RawAllocator> &m_Buffer;
    WCharTWriter(MemoryBuffer<RawAllocator> &buf)
        : m_Buffer(buf)
    {
    }
    void Write(const wchar_t *value, QT3DSU32 len = 0)
    {
        if (IsTrivial(value))
            return;
        if (len == 0)
            len = (QT3DSU32)wcslen(value);
        m_Buffer.write(value, len);
    }
    void Write(wchar_t value) { m_Buffer.write(value); }
    void Write(bool value) { Write(value ? L"True" : L"False"); }

    // Takes care of long and float
    template <typename TDataType>
    void Write(TDataType value)
    {
        wchar_t buf[256];
        QT3DSU32 numWritten = WStrOps<TDataType>().ToStr(value, NVDataRef<wchar_t>(buf, 256));
        if (numWritten)
            Write((const wchar_t *)buf);
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
                    Write((wchar_t)'\n');
                    for (QT3DSU32 tabIdx = 0; tabIdx < tabCount; ++tabIdx)
                        Write((wchar_t)'\t');
                } else
                    Write((wchar_t)' ');
            }
            Write(values[idx]);
        }
    }

    void Write(NVConstDataRef<wchar_t> values, QT3DSU32 ignored = 6)
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

inline bool IsWhite(wchar_t value)
{
    return value == '\n' || value == '\r' || value == ' ' || value == '\t';
}

// skip until we find whitespace.
inline wchar_t *FindNextWhitespace(wchar_t *input)
{
    if (input == NULL)
        return input;
    wchar_t *marker = input;
    // Empty loop intentional
    for (; *marker && !IsWhite(*marker); ++marker)
        ;
    return marker;
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
inline wchar_t *FindNextNonWhitespace(wchar_t *input)
{
    if (input == NULL)
        return input;
    wchar_t *marker = input;
    // Empty loop intentional
    for (; *marker && IsWhite(*marker); ++marker)
        ;
    return marker;
}

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
struct WCharTReader
{
    char8_t *m_StartPtr;
    // Buffer used for temp storage
    MemoryBuffer<RawAllocator> &m_Buffer;
    IStringTable &m_StringTable;
    WCharTReader(char8_t *sp, MemoryBuffer<RawAllocator> &buf, IStringTable &inStringTable)
        : m_StartPtr(sp)
        , m_Buffer(buf)
        , m_StringTable(inStringTable)
    {
    }
    void Read(const char8_t *&outPtr) { outPtr = m_StartPtr; }
    void Read(const wchar_t *&outPtr) { outPtr = m_StringTable.GetWideStr(m_StartPtr); }
    template <typename TDataType>
    void Read(TDataType &data)
    {
        bool success = WStrOps<TDataType>().StrTo(m_StartPtr, data);
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
            WStrOps<TDataType>().StrTo(m_StartPtr, data[idx]);
            m_StartPtr = nextPtr;
            if (m_StartPtr)
                m_StartPtr = FindNextNonWhitespace(m_StartPtr + 1);
        }
    }

    void ReadBuffer(NVConstDataRef<char8_t> &outBuffer)
    {
        if (m_StartPtr && *m_StartPtr) {
            QT3DSU32 len = (QT3DSU32)strlen(m_StartPtr);
            outBuffer = NVConstDataRef<char8_t>(m_StartPtr, len + 1);
        }
    }
    void ReadBuffer(NVConstDataRef<wchar_t> &outBuffer)
    {
        if (m_StartPtr && *m_StartPtr) {
            QT3DSU32 len = (QT3DSU32)strlen(m_StartPtr);
            outBuffer = NVConstDataRef<wchar_t>(m_StringTable.GetWideStr(m_StartPtr), len + 1);
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
            WStrOps<TDataType>().StrTo(m_StartPtr, temp);
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

template <>
struct WStrOps<CDataModelHandle>
{
    template <typename THandleType>
    QT3DSU32 ToStr(THandleType item, NVDataRef<wchar_t> buffer)
    {
        int theValue(item);
        return WStrOps<int>().ToStr(theValue, buffer);
    }
    template <typename THandleType>
    bool StrTo(const char8_t *buffer, THandleType &item)
    {
        int theValue;
        bool retval = WStrOps<int>().StrTo(buffer, theValue);
        item = theValue;
        return retval;
    }
};

#define IMPLEMENT_HANDLE_WSTROPS(HandleType)                                                       \
    template <>                                                                                    \
    struct WStrOps<HandleType> : public WStrOps<CDataModelHandle>                                  \
    {                                                                                              \
    };
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMInstanceHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMPropertyHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMSlideHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMSlideGraphHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMAnimationHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMKeyframeHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMActionHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMHandlerArgHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMEventHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMHandlerHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMHandlerParamHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMMetaDataPropertyHandle);
IMPLEMENT_HANDLE_WSTROPS(Qt3DSDMCategoryHandle);
#undef IMPLEMENT_HANDLE_WSTROPS

template <>
struct WStrOps<EAnimationType>
{
    QT3DSU32 ToStr(EAnimationType value, NVDataRef<wchar_t> buffer)
    {
        const wchar_t *animType = NULL;
        switch (value) {
        case EAnimationTypeLinear:
            animType = L"Linear";
            break;
        case EAnimationTypeBezier:
            animType = L"Bezier";
            break;
        case EAnimationTypeEaseInOut:
            animType = L"EaseInOut";
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
        if (animType != NULL)
            return swprintf(buffer.begin(), buffer.size(), L"%ls", animType);
        else {
            QT3DS_ASSERT(false);
            return 0;
        }
    }
    bool StrTo(const wchar_t *buffer, EAnimationType &item)
    {
        if (AreEqual(L"Linear", buffer)) {
            item = EAnimationTypeLinear;
            return true;
        }
        if (AreEqual(L"Bezier", buffer)) {
            item = EAnimationTypeBezier;
            return true;
        }
        if (AreEqual(L"EaseInOut", buffer)) {
            item = EAnimationTypeEaseInOut;
            return true;
        }
        return false;
    }
};

// Implemented in UICDMMetaData.h
template <>
struct WStrOps<CompleteMetaDataType::Enum>
{
    QT3DSU32 ToStr(CompleteMetaDataType::Enum item, NVDataRef<wchar_t> buffer);
    bool StrTo(const wchar_t *buffer, CompleteMetaDataType::Enum &item);
};

template <>
struct WStrOps<HandlerArgumentType::Value>
{
    QT3DSU32 ToStr(HandlerArgumentType::Value item, NVDataRef<wchar_t> buffer);
    bool StrTo(const wchar_t *buffer, HandlerArgumentType::Value &item);
};

#ifndef __clang__
#ifndef __INTEGRITY
// IDOMReader implementations
template <typename TDataType>
bool IDOMReader::ValueList(NVDataRef<TDataType> data)
{
    const wchar_t *value;
    if (Value(value)) {
        WCharTReader reader(const_cast<wchar_t *>(value), m_TempBuf);
        reader.ReadRef(data);
    }
    return true;
}

// Destructive operation because we can't trust
// strtod to do the right thing.  On windows, for long strings,
// it calls strlen every operation thus leading to basically N^2
// behavior
template <typename TDataType>
NVConstDataRef<TDataType> IDOMReader::ChildValueList(TWideXMLCharPtr listName)
{
    NVConstDataRef<TDataType> retval;
    TWideXMLCharPtr childValue = NULL;
    if (ChildValue(listName, childValue)) {
        WCharTReader reader(const_cast<wchar_t *>(childValue), m_TempBuf);
        reader.ReadBuffer(retval);
    }
    return retval;
}
#endif
#endif
}

#endif
