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
#include "foundation/StringTable.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Utils.h"
#include "EASTL/string.h"
#include "EASTL/sort.h"
#include "foundation/StrConvertUTF.h"
#include "foundation/PreAllocatedAllocator.h"
#include "foundation/SerializationTypes.h"
#include "foundation/Qt3DSMutex.h"

#include <QtCore/qhash.h>

using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace eastl;

namespace eastl {
struct SCharAndHash
{
    const char8_t *m_Data;
    size_t m_Hash;
    // Cache the generated hash code.
    SCharAndHash(const char8_t *inData)
        : m_Data(inData)
        , m_Hash(eastl::hash<const char8_t *>()(inData))
    {
    }
    SCharAndHash()
        : m_Data(nullptr)
        , m_Hash(0)
    {
    }
    operator const char *() const { return m_Data; }
};
template <>
struct hash<SCharAndHash>
{
    size_t operator()(const SCharAndHash &inItem) const { return inItem.m_Hash; }
};

template <>
struct equal_to<SCharAndHash>
{
    bool operator()(const SCharAndHash &inLhs, const SCharAndHash &inRhs) const
    {
        return char_equal_to()(inLhs.m_Data, inRhs.m_Data);
    }
};
}

void CRegisteredString::Remap(const IStringTable &inTable)
{
    Remap(const_cast<IStringTable &>(inTable).GetRemapMap());
}

void CRegisteredString::Remap(const SStrRemapMap &inMap)
{
    if (IsValid()) {
        SStrRemapMap::const_iterator theIter = inMap.find((char8_t *)m_String);
        if (theIter != inMap.end())
            m_String = reinterpret_cast<const char8_t *>(theIter->second);
        else {
            QT3DS_ASSERT(false);
            // Ensure a failure here doesn't *guarantee* a crash somewhere else.
            m_String = nullptr;
        }
    } else {
        // Indicates an invalid string.
        m_String = reinterpret_cast<const char8_t *>(QT3DS_MAX_U32);
    }
}

void CRegisteredString::Remap(NVDataRef<QT3DSU8> inDataPtr)
{
    size_t theOffset = reinterpret_cast<size_t>(m_String);
    if (theOffset >= QT3DS_MAX_U32)
        m_String = "";
    else {
        if (theOffset < inDataPtr.size())
            m_String = reinterpret_cast<char8_t *>(inDataPtr.begin() + theOffset);
        else {
            QT3DS_ASSERT(false);
            m_String = "";
        }
    }
}

namespace {

const char16_t g_char16EmptyStr[] = { 0, 0, 0, 0 };
const char32_t g_char32EmptyStr[] = { 0, 0, 0, 0 };

typedef eastl::basic_string<char8_t, ForwardingAllocator> TNarrowStr;
typedef eastl::basic_string<TWCharEASTLConverter::TCharType, ForwardingAllocator> TWideStr;

inline bool isTrivialWide(const wchar_t *str)
{
    return str == nullptr || *str == 0;
}

QT3DSU8 *AlignPointer(QT3DSU8 *inStart, QT3DSU8 *inPtr)
{
    QT3DSU32 alignment = sizeof(void *);
    size_t numBytes = (size_t)(inPtr - inStart);
    QT3DS_ASSERT(inStart < inPtr);
    if (numBytes % alignment)
        inPtr += alignment - (numBytes % alignment);
    return inPtr;
}

QT3DSU32 Align(QT3DSU32 inValue)
{
    QT3DSU32 alignment = sizeof(void *);
    QT3DSU32 leftover = inValue % alignment;
    if (leftover)
        inValue += alignment - leftover;
    return inValue;
}

// Structure that is written out to the file.
// Directly after this is the string character data.
// TWideStr& inConvertStr
struct SStringFileData
{
    QT3DSU32 m_StartOffset;
    QT3DSU32 m_StrLen;
    QT3DSU32 m_Handle;
    char8_t *m_Str;
    SStringFileData(QT3DSU32 len, QT3DSU32 off, QT3DSU32 handle, char8_t *data)
        : m_StartOffset(off)
        , m_StrLen(len)
        , m_Handle(handle)
        , m_Str(data)
    {
    }
    SStringFileData()
        : m_StartOffset(0)
        , m_StrLen(0)
        , m_Handle(0)
        , m_Str(nullptr)
    {
    }
    void Deallocate(NVAllocatorCallback &inAlloc)
    {
        inAlloc.deallocate(m_Str);
        m_Str = nullptr;
    }
    const char8_t *GetNarrow() { return m_Str; }
    operator CRegisteredString() const
    {
        return CRegisteredString::ISwearThisHasBeenRegistered(m_Str);
    }
    QT3DSU32 EndOffset() const { return m_StartOffset + m_StrLen; }

    // Used only for the lower bound operation to find a given object by offset.
    bool operator<(const SStringFileData &inOther) const
    {
        return m_StartOffset < inOther.m_StartOffset;
    }
};

struct SCharAndHandle
{
    const char8_t *m_Data;
    QT3DSU32 m_Handle;
    SCharAndHandle()
        : m_Data(nullptr)
        , m_Handle(0)
    {
    }
    SCharAndHandle(const char8_t *data, QT3DSU32 hdl)
        : m_Data(data)
        , m_Handle(hdl)
    {
    }
    operator const char *() const { return m_Data; }
};

struct SStringFileHeader
{
    QT3DSU32 m_NumStrings;
    QT3DSU32 m_NextHandleValue;
    // offset to the items of fixed size.  Variable sized data is stored
    // up front.
    QT3DSU32 m_FixedBufferOffset;
    SStringFileHeader()
        : m_NumStrings(0)
        , m_NextHandleValue(0)
        , m_FixedBufferOffset(0)
    {
    }

    SStringFileHeader(QT3DSU32 ns, QT3DSU32 hv, QT3DSU32 fbo)
        : m_NumStrings(ns)
        , m_NextHandleValue(hv)
        , m_FixedBufferOffset(fbo)
    {
    }
};

// This is the core of the string table.
struct SStringFileDataList
{
    typedef nvhash_map<SCharAndHash, SCharAndHandle> TMapType;
    typedef nvhash_map<QT3DSU32, const char8_t *> THandleMapType;

    mutable SPreAllocatedAllocator m_Allocator;

    // When we load from a file, we get a these items
    NVConstDataRef<SStringFileData> m_DataBlock;
    NVConstDataRef<eastl::pair<size_t, QT3DSU32>> m_HashDataBlockOffset;
    NVConstDataRef<eastl::pair<QT3DSU32, QT3DSU32>> m_HandleDataBlockOffset;

    // When we are running normally, this items get mangled
    nvvector<SStringFileData> m_AppendedStrings;

protected:
    // Built roughly on demand
    SStrRemapMap m_StrRemapMap;
    TMapType m_HashToStrMap;
    THandleMapType m_HandleToStrMap;
    QT3DSU32 m_NextHandleValue;

public:
    SStringFileDataList(NVAllocatorCallback &inAlloc)
        : m_Allocator(inAlloc)
        , m_AppendedStrings(inAlloc, "StringTable::m_AppendedStrings")
        , m_StrRemapMap(inAlloc, "StringTable::m_StrRemapMap")
        , m_HashToStrMap(inAlloc, "StringTable::m_HashToStrMap")
        , m_HandleToStrMap(inAlloc, "StringTable::m_HashToStrMap")
        , m_NextHandleValue(1)
    {
    }
    ~SStringFileDataList()
    {
        for (QT3DSU32 idx = 0, end = m_AppendedStrings.size(); idx < end; ++idx)
            m_AppendedStrings[idx].Deallocate(m_Allocator);
    }

    SStrRemapMap &GetRemapMap()
    {
        if (m_StrRemapMap.empty()) {
            for (QT3DSU32 idx = 0, end = m_DataBlock.size(); idx < end; ++idx)
                m_StrRemapMap.insert(
                    eastl::make_pair(m_DataBlock[idx].m_Str, m_DataBlock[idx].m_StartOffset));
            for (QT3DSU32 idx = 0, end = m_AppendedStrings.size(); idx < end; ++idx)
                m_StrRemapMap.insert(eastl::make_pair(m_AppendedStrings[idx].m_Str,
                                                      m_AppendedStrings[idx].m_StartOffset));
        }
        return m_StrRemapMap;
    }

    struct SHashPairComparator
    {
        bool operator()(const eastl::pair<size_t, QT3DSU32> &lhs,
                        const eastl::pair<size_t, QT3DSU32> &rhs) const
        {
            return lhs.first < rhs.first;
        }
    };

    struct SHandlePairComparator
    {
        bool operator()(const eastl::pair<QT3DSU32, QT3DSU32> &lhs,
                        const eastl::pair<QT3DSU32, QT3DSU32> &rhs) const
        {
            return lhs.first < rhs.first;
        }
    };

    SCharAndHandle DoFindStr(SCharAndHash hashCode)
    {
        if (isTrivial(hashCode.m_Data))
            return SCharAndHandle();
        TMapType::iterator theFind = m_HashToStrMap.find(hashCode);
        if (theFind == m_HashToStrMap.end()) {
            SCharAndHandle newStr = FindStrByHash(hashCode);
            if (!newStr.m_Data)
                newStr = Append(hashCode);

            // It may not be obvious why we have to reset the data member.
            // we have to do this so the hashtable keys don't change if the user isn't
            // passing in a persistent string.
            hashCode.m_Data = newStr;

            theFind = m_HashToStrMap.insert(eastl::make_pair(hashCode, newStr)).first;
            m_HandleToStrMap.insert(eastl::make_pair(theFind->second.m_Handle, newStr));
        }
        return theFind->second;
    }

    const CRegisteredString FindStr(SCharAndHash hashCode)
    {
        SCharAndHandle result = DoFindStr(hashCode);
        if (result.m_Data)
            return CRegisteredString::ISwearThisHasBeenRegistered(result.m_Data);
        return CRegisteredString();
    }

    const CStringHandle FindStrHandle(SCharAndHash hashCode)
    {
        SCharAndHandle result = DoFindStr(hashCode);
        return CStringHandle::ISwearThisHasBeenRegistered(result.m_Handle);
    }

    const CRegisteredString FindStrByHandle(QT3DSU32 handle)
    {
        if (handle == 0)
            return CRegisteredString();

        THandleMapType::iterator theFind = m_HandleToStrMap.find(handle);
        if (theFind == m_HandleToStrMap.end()) {
            const char8_t *newStr = FindStrByHandleData(handle);
            if (!newStr)
                return CRegisteredString();
            theFind = m_HandleToStrMap.insert(eastl::make_pair(handle, newStr)).first;
        }
        return CRegisteredString::ISwearThisHasBeenRegistered(theFind->second);
    }

    void Save(SWriteBuffer &ioBuffer) const
    {
        // Buffer should be aligned before we get to it.
        QT3DS_ASSERT(ioBuffer.size() % 4 == 0);

        QT3DSU32 numStrs = Size();
        size_t bufferBegin = ioBuffer.size();
        SStringFileHeader theHeader;
        ioBuffer.write(theHeader);
        QT3DSU32 startOffset = ioBuffer.size();

        eastl::vector<eastl::pair<size_t, QT3DSU32>> hashToStringIndex;
        eastl::vector<eastl::pair<QT3DSU32, QT3DSU32>> handleToStringIndex;
        hashToStringIndex.reserve(numStrs);
        handleToStringIndex.reserve(numStrs);
        WriteStringBlockData(m_DataBlock.begin(), m_DataBlock.end(), ioBuffer, hashToStringIndex,
                             handleToStringIndex);
        WriteStringBlockData(m_AppendedStrings.data(),
                             m_AppendedStrings.data() + m_AppendedStrings.size(), ioBuffer,
                             hashToStringIndex, handleToStringIndex);
        ioBuffer.align(4);

        QT3DSU32 fixedOffset = ioBuffer.size() - startOffset;
        WriteStringBlock(m_DataBlock.begin(), m_DataBlock.end(), ioBuffer);
        WriteStringBlock(m_AppendedStrings.data(),
                         m_AppendedStrings.data() + m_AppendedStrings.size(), ioBuffer);

        // sort by hash code so we can do binary lookups later.
        eastl::sort(hashToStringIndex.begin(), hashToStringIndex.end(), SHashPairComparator());
        ioBuffer.write(hashToStringIndex.data(), (QT3DSU32)hashToStringIndex.size());

        eastl::sort(handleToStringIndex.begin(), handleToStringIndex.end(),
                    SHandlePairComparator());
        ioBuffer.write(handleToStringIndex.data(), (QT3DSU32)handleToStringIndex.size());

        SStringFileHeader *fixedOffsetPtr =
            reinterpret_cast<SStringFileHeader *>(ioBuffer.begin() + bufferBegin);
        *fixedOffsetPtr = SStringFileHeader(numStrs, m_NextHandleValue, fixedOffset);
    }

    void Load(NVDataRef<QT3DSU8> inMemory)
    {
        QT3DS_ASSERT(m_AppendedStrings.empty());
        m_Allocator.m_PreAllocatedBlock = inMemory;
        m_Allocator.m_OwnsMemory = false;
        SDataReader theReader(inMemory.begin(), inMemory.end());
        SStringFileHeader theHeader = theReader.LoadRef<SStringFileHeader>();
        QT3DSU32 numStrs = theHeader.m_NumStrings;
        m_NextHandleValue = theHeader.m_NextHandleValue;
        QT3DSU32 fixedOffset = theHeader.m_FixedBufferOffset;
        theReader.m_CurrentPtr += fixedOffset;
        m_DataBlock = NVConstDataRef<SStringFileData>(
            reinterpret_cast<const SStringFileData *>(theReader.m_CurrentPtr), numStrs);
        theReader.m_CurrentPtr += sizeof(SStringFileData) * numStrs;

        m_HashDataBlockOffset = NVConstDataRef<eastl::pair<size_t, QT3DSU32>>(
            reinterpret_cast<const eastl::pair<size_t, QT3DSU32> *>(theReader.m_CurrentPtr), numStrs);
        theReader.m_CurrentPtr += sizeof(eastl::pair<size_t, QT3DSU32>) * numStrs;

        m_HandleDataBlockOffset = NVConstDataRef<eastl::pair<QT3DSU32, QT3DSU32>>(
            reinterpret_cast<const eastl::pair<QT3DSU32, QT3DSU32> *>(theReader.m_CurrentPtr), numStrs);

        for (QT3DSU32 idx = 0, end = m_DataBlock.size(); idx < end; ++idx) {
            SStringFileData &theData = const_cast<SStringFileData &>(m_DataBlock[idx]);
            theData.m_Str = reinterpret_cast<char8_t *>(inMemory.mData + theData.m_StartOffset);
        }
    }

protected:
    // When we load from a file, we do not put every string into the hash because building
    // hashtables takes
    // time.  We only put them into the hash on demand; this avoids building a giant hashtable upon
    // load.
    // and since in general we do not need to lookup all hash values again it avoids building the
    // hashtable
    // in general.
    SCharAndHandle FindStrByHash(SCharAndHash hashCode) const
    {
        const eastl::pair<size_t, QT3DSU32> *iter =
            eastl::lower_bound(m_HashDataBlockOffset.begin(), m_HashDataBlockOffset.end(),
                               eastl::make_pair(hashCode.m_Hash, (QT3DSU32)0), SHashPairComparator());

        for (; iter != m_HashDataBlockOffset.end() && iter->first == hashCode.m_Hash; ++iter) {
            if (AreEqual(m_DataBlock[iter->second].m_Str, hashCode.m_Data))
                return SCharAndHandle(m_DataBlock[iter->second].m_Str,
                                      m_DataBlock[iter->second].m_Handle);
        }
        return SCharAndHandle();
    }

    const char8_t *FindStrByHandleData(QT3DSU32 handle) const
    {
        const eastl::pair<QT3DSU32, QT3DSU32> *iter =
            eastl::lower_bound(m_HandleDataBlockOffset.begin(), m_HandleDataBlockOffset.end(),
                               eastl::make_pair(handle, (QT3DSU32)0), SHandlePairComparator());

        if (iter != m_HandleDataBlockOffset.end() && iter->first == handle
            && m_DataBlock[iter->second].m_Handle == handle)
            return m_DataBlock[iter->second].m_Str;
        return nullptr;
    }

    void WriteStringBlockData(const SStringFileData *begin, const SStringFileData *end,
                              SWriteBuffer &ioBuffer,
                              eastl::vector<eastl::pair<size_t, QT3DSU32>> &ioHashToStringIndex,
                              eastl::vector<eastl::pair<QT3DSU32, QT3DSU32>> &ioHandleToStringIndex) const
    {
        for (const SStringFileData *iter = begin; iter != end; ++iter) {
            size_t idx = ioHashToStringIndex.size();
            SCharAndHash theHash(iter->m_Str);
            ioHashToStringIndex.push_back(eastl::make_pair(theHash.m_Hash, (QT3DSU32)idx));
            ioHandleToStringIndex.push_back(eastl::make_pair(iter->m_Handle, (QT3DSU32)idx));
            ioBuffer.write(iter->m_Str, iter->m_StrLen);
        }
    }

    void WriteStringBlock(const SStringFileData *begin, const SStringFileData *end,
                          SWriteBuffer &ioBuffer) const
    {
        for (const SStringFileData *iter = begin; iter != end; ++iter)
            ioBuffer.write(*iter);
    }

    SCharAndHandle Append(SCharAndHash inStrHash)
    {
        if (isTrivial(inStrHash.m_Data))
            return SCharAndHandle();

        const char8_t *inStr(inStrHash.m_Data);
        QT3DSU32 len = (QT3DSU32)strlen(inStr) + 1;
        char8_t *newStr = (char8_t *)m_Allocator.allocate(len, "StringData", __FILE__, __LINE__);
        memCopy(newStr, inStr, len);

        // We write the number of strings to the file just after the header.
        QT3DSU32 theOffset = sizeof(SStringFileHeader);
        if (Size())
            theOffset = Back().EndOffset();

        QT3DSU32 handleValue = m_NextHandleValue;
        ++m_NextHandleValue;
        m_AppendedStrings.push_back(SStringFileData(len, theOffset, handleValue, newStr));

        // Only add to the str map if necessary because building it could be expensive.
        if (m_StrRemapMap.empty() == false)
            m_StrRemapMap.insert(eastl::make_pair(newStr, theOffset));

        return SCharAndHandle(newStr, handleValue);
    }
    // precondition is that size  != 0
    const SStringFileData &Back() const
    {
        QT3DS_ASSERT(Size());

        if (m_AppendedStrings.size()) {
            return m_AppendedStrings.back();
        } else {
            return m_DataBlock[m_DataBlock.size() - 1];
        }
    }

    QT3DSU32 Size() const { return static_cast<QT3DSU32>(m_AppendedStrings.size() + m_DataBlock.size()); }
};

struct SStringTableMutexScope
{
    Mutex *m_Mutex;
    SStringTableMutexScope(Mutex *inM)
        : m_Mutex(inM)
    {
        if (m_Mutex)
            m_Mutex->lock();
    }
    ~SStringTableMutexScope()
    {
        if (m_Mutex)
            m_Mutex->unlock();
    }
};

#define STRING_TABLE_MULTITHREADED_METHOD SStringTableMutexScope __locker(m_MultithreadMutex)

static const QT3DSU32 dynHandleBase = 0x80000000;

class StringTable : public IStringTable
{
    typedef nvhash_map<SCharAndHash, QT3DSU32> TMapType;
    typedef nvhash_map<CRegisteredString, TWideStr> TNarrowToWideMapType;
    SStringFileDataList m_FileData;
    NVAllocatorCallback &m_Allocator;
    TNarrowToWideMapType m_StrWideMap;
    volatile QT3DSI32 mRefCount; // fnd's naming convention
    eastl::basic_string<char8_t, ForwardingAllocator> m_ConvertBuffer;
    TWideStr m_WideConvertBuffer;
    Mutex m_MultithreadMutexBacker;
    Mutex *m_MultithreadMutex;

    struct DynamicString
    {
        QByteArray str;
        int refCount = 0;
    };

    QHash<QT3DSU32, DynamicString *> m_dynamicFreeHandlesMap;
    QHash<QT3DSU32, DynamicString *> m_dynamicUsedHandlesMap;
    QHash<QByteArray, QT3DSU32> m_dynamicStringToHandleMap;
    QT3DSU32 m_nextFreeDynamicHandle = dynHandleBase;

    // Data that will be written out to the file.

public:
    StringTable(NVAllocatorCallback &alloc)
        : m_FileData(alloc)
        , m_Allocator(m_FileData.m_Allocator)
        , m_StrWideMap(alloc, "StringTable::m_StrWideMap")
        , mRefCount(0)
        , m_ConvertBuffer(ForwardingAllocator(alloc, "StringTable::m_ConvertBuffer"))
        , m_WideConvertBuffer(ForwardingAllocator(alloc, "StringTable::m_WideConvertBuffer"))
        , m_MultithreadMutexBacker(alloc)
        , m_MultithreadMutex(nullptr)
    {
    }

    virtual ~StringTable() override
    {
        qDeleteAll(m_dynamicFreeHandlesMap);
        qDeleteAll(m_dynamicUsedHandlesMap);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_FileData.m_Allocator.m_Allocator)

    void EnableMultithreadedAccess() override
    {
        m_MultithreadMutex = &m_MultithreadMutexBacker;
    }

    void DisableMultithreadedAccess() override
    {
        m_MultithreadMutex = nullptr;
    }

    CRegisteredString RegisterStr(const QString &str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        if (str.isEmpty())
            return CRegisteredString();
        return RegisterStr(str.toUtf8());
    }

    CRegisteredString RegisterStr(const QByteArray &str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        if (str.isEmpty())
            return CRegisteredString();
        return RegisterStr(str.constData());
    }

    CRegisteredString RegisterStr(const eastl::string &string) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        return RegisterStr(string.c_str());
    }

    CRegisteredString RegisterStr(Qt3DSBCharPtr str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        if (isTrivial(str))
            return CRegisteredString();
        return m_FileData.FindStr(str);
    }

    // utf-16->utf-8
    CRegisteredString RegisterStr(const char16_t *str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        qt3ds::foundation::ConvertUTF(str, 0, m_ConvertBuffer);
        return RegisterStr(m_ConvertBuffer.c_str());
    }
    // utf-32->utf-8
    CRegisteredString RegisterStr(const char32_t *str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        qt3ds::foundation::ConvertUTF(str, 0, m_ConvertBuffer);
        return RegisterStr(m_ConvertBuffer.c_str());
    }

    CRegisteredString RegisterStr(const wchar_t *str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        if (isTrivialWide(str))
            return CRegisteredString();
        qt3ds::foundation::ConvertUTF(
                    reinterpret_cast<const TWCharEASTLConverter::TCharType *>(str), 0,
                    m_ConvertBuffer);
        return RegisterStr(m_ConvertBuffer.c_str());
    }

    CStringHandle GetHandle(Qt3DSBCharPtr str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        return m_FileData.FindStrHandle(str);
    }

    CStringHandle getDynamicHandle(const QByteArray &str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        QT3DSU32 handle = m_dynamicStringToHandleMap.value(str, 0);
        DynamicString *theStr = nullptr;

        if (!handle) {
            if (m_dynamicFreeHandlesMap.isEmpty()) {
                handle = ++m_nextFreeDynamicHandle;
                theStr = new DynamicString;
            } else {
                auto first = m_dynamicFreeHandlesMap.begin();
                handle = first.key();
                theStr = first.value();
                m_dynamicFreeHandlesMap.erase(first);
            }
            theStr->str = str;
            m_dynamicStringToHandleMap.insert(str, handle);
            m_dynamicUsedHandlesMap.insert(handle, theStr);
        } else {
            theStr = m_dynamicUsedHandlesMap.value(handle);
        }
        ++theStr->refCount;
        return CStringHandle::ISwearThisHasBeenRegistered(handle);
    }

    void releaseDynamicHandle(QT3DSU32 strHandle) override
    {
        DynamicString *str = m_dynamicUsedHandlesMap.value(strHandle);
        if (str) {
            --str->refCount;
            if (str->refCount == 0) {
                m_dynamicUsedHandlesMap.remove(strHandle);
                m_dynamicFreeHandlesMap.insert(strHandle, str);
                m_dynamicStringToHandleMap.remove(str->str);
            }
        }
    }

    CRegisteredString HandleToStr(QT3DSU32 strHandle) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        if (strHandle >= dynHandleBase) {
            DynamicString *str = m_dynamicUsedHandlesMap.value(strHandle);
            return CRegisteredString::ISwearThisHasBeenRegistered(str->str.constData());
        } else {
            return m_FileData.FindStrByHandle(strHandle);
        }
    }

    const wchar_t *GetWideStr(CRegisteredString theStr)
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        eastl::pair<TNarrowToWideMapType::iterator, bool> pair_iter = m_StrWideMap.insert(
            eastl::make_pair(theStr, TWideStr(ForwardingAllocator(m_Allocator, "WideString"))));
        if (pair_iter.second)
            qt3ds::foundation::ConvertUTF(theStr.c_str(), 0, pair_iter.first->second);
        return reinterpret_cast<const wchar_t *>(pair_iter.first->second.c_str());
    }

    const wchar_t *GetWideStr(Qt3DSBCharPtr src) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        if (isTrivial(src))
            return L"";
        return GetWideStr(RegisterStr(src));
    }

    const wchar_t *GetWideStr(const wchar_t *str) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        if (isTrivialWide(str))
            return L"";
        qt3ds::foundation::ConvertUTF(
                    reinterpret_cast<const TWCharEASTLConverter::TCharType *>(str), 0,
                    m_ConvertBuffer);
        return GetWideStr(RegisterStr(m_ConvertBuffer.c_str()));
    }

    const SStrRemapMap &GetRemapMap() override { return m_FileData.GetRemapMap(); }

    NVAllocatorCallback &GetAllocator() override { return m_FileData.m_Allocator.m_Allocator; }

    // Save to a block of memory.  It is up the callers to deallocate using allocator
    // from GetAllocator().  Returns a remap map that takes existing strings and changes their
    // address such that they are offsets into the block of memory where this object
    // started saving.  Returns a map that converts registered strings into offsets of the
    // written buffer.
    void Save(SWriteBuffer &ioBuffer) const override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        m_FileData.Save(ioBuffer);
    }

    // Load all the strings from inMemory.  inMemory is not freed by this object
    // Finally, you will need to take inMemory and call remap on all strings
    // from offsets back into their correct address into inMemory.
    void Load(qt3ds::foundation::NVDataRef<QT3DSU8> inMemory) override
    {
        STRING_TABLE_MULTITHREADED_METHOD;
        m_FileData.Load(inMemory);
    }
};
}

const char16_t *char16EmptyStr = g_char16EmptyStr;
const char32_t *char32EmptyStr = g_char32EmptyStr;

IStringTable &IStringTable::CreateStringTable(NVAllocatorCallback &alloc)
{
    return *QT3DS_NEW(alloc, StringTable)(alloc);
}
