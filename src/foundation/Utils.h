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
#pragma once
#ifndef QT3DS_RENDER_UTILS_H
#define QT3DS_RENDER_UTILS_H
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/vector.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSMath.h"
#include <ctype.h>

namespace qt3ds {
namespace foundation {

    // Defined in IStringTable.cpp
    extern const char16_t *char16EmptyStr;
    extern const char32_t *char32EmptyStr;

    inline const char *nonNull(const char *src) { return src == NULL ? "" : src; }
    inline bool isTrivial(const char *str) { return str == NULL || *str == 0; }
    inline const char16_t *nonNull(const char16_t *src)
    {
        return src == NULL ? char16EmptyStr : src;
    }
    inline bool isTrivial(const char16_t *str) { return str == NULL || *str == 0; }
    inline const char32_t *nonNull(const char32_t *src)
    {
        return src == NULL ? char32EmptyStr : src;
    }
    inline bool isTrivial(const char32_t *str) { return str == NULL || *str == 0; }

    // Note this is safe to call on null strings.
    template <typename TCharType>
    inline QT3DSU32 StrLen(const TCharType *inType)
    {
        QT3DSU32 retval = 0;
        while (inType && *inType) {
            ++retval;
            ++inType;
        }
        return retval;
    }

    template <typename TCharType>
    inline QT3DSI32 Compare(const TCharType *inLhs, const TCharType *inRhs)
    {
        inLhs = nonNull(inLhs);
        inRhs = nonNull(inRhs);
        while (*inLhs && *inRhs) {
            QT3DSI32 diff = *inLhs - *inRhs;
            if (diff)
                return diff;
            ++inLhs;
            ++inRhs;
        }
        return *inLhs - *inRhs;
    }

    template <typename TCharType>
    inline QT3DSI32 CompareCaseless(const TCharType *inLhs, const TCharType *inRhs)
    {
        inLhs = nonNull(inLhs);
        inRhs = nonNull(inRhs);
        while (*inLhs && *inRhs) {
            QT3DSI32 diff = tolower(*inLhs) - tolower(*inRhs);
            if (diff)
                return diff;
            ++inLhs;
            ++inRhs;
        }
        return *inLhs - *inRhs;
    }

    template <typename TCharType>
    inline QT3DSI32 CompareNChars(const TCharType *inLhs, const TCharType *inRhs, QT3DSU32 inNumChars)
    {
        inLhs = nonNull(inLhs);
        inRhs = nonNull(inRhs);
        while (*inLhs && *inRhs && inNumChars) {
            QT3DSI32 diff = *inLhs - *inRhs;
            if (diff)
                return diff;
            ++inLhs;
            ++inRhs;
            --inNumChars;
        }
        if (inNumChars)
            return *inLhs - *inRhs;
        return 0;
    }

    template <typename TCharType>
    bool AreEqual(const TCharType *inLhs, const TCharType *inRhs)
    {
        return Compare(inLhs, inRhs) == 0;
    }

    template <typename TCharType>
    bool AreEqualCaseless(const TCharType *inLhs, const TCharType *inRhs)
    {
        return CompareCaseless(inLhs, inRhs) == 0;
    }

    inline QT3DSU32 IsBitSetInBitmap(NVConstDataRef<QT3DSU32> bitmap, QT3DSU32 entryIndex)
    {
        QT3DSU32 bitMapIndex = entryIndex / 32;
        QT3DSU32 bitIndex = entryIndex % 32;
        QT3DSU32 shouldApply = 0;
        if (bitMapIndex < bitmap.mSize)
            shouldApply = (1 << bitIndex) & bitmap[bitMapIndex];
        return shouldApply;
    }

    inline void SetBitInBitmap(nvvector<QT3DSU32> &bitmap, QT3DSU32 entryIndex)
    {
        QT3DSU32 bitMapIndex = entryIndex / 32;
        QT3DSU32 bitIndex = entryIndex % 32;
        while (bitmap.size() <= bitMapIndex)
            bitmap.push_back(0);
        bitmap[bitMapIndex] |= (QT3DSU32)(1 << bitIndex);
    }
    inline void UnsetBitInBitmap(nvvector<QT3DSU32> &bitmap, QT3DSU32 entryIndex)
    {
        QT3DSU32 bitMapIndex = entryIndex / 32;
        QT3DSU32 bitIndex = entryIndex % 32;
        if (bitMapIndex < bitmap.size())
            bitmap[bitMapIndex] &= ~((QT3DSU32)(1 << bitIndex));
    }

#define CHECK_CONDITION_AND_RETURN_FALSE(cond, errorType, message)                                 \
    if (cond) {                                                                                    \
        m_Foundation.error(errorType, message);                                                    \
        QT3DS_ASSERT(false);                                                                          \
        return false;                                                                              \
    }
// Returns false if the property type doesn't match the incoming type.
#define CHECK_CONDITION_AND_RETURN(cond, errorType, message)                                       \
    if (cond) {                                                                                    \
        m_Foundation.error(errorType, message);                                                    \
        QT3DS_ASSERT(false);                                                                          \
    }

#define QT3DS_FOREACH(varname, stop) for (QT3DSU32 varname = 0, end = stop; varname < end; ++varname)

    template <typename TKeyType, typename TValueType, typename THashType, typename TPredicate,
              typename TBufType, typename TOperator>
    QT3DSU32 GetMapKeysOp(const eastl::hash_map<TKeyType, TValueType, THashType, TPredicate,
                                             ForwardingAllocator> &map,
                       TBufType *buffer, QT3DSU32 bufSize, QT3DSU32 startIdx, TOperator op)
    {
        QT3DSU32 numItems = static_cast<QT3DSU32>(map.size());
        if (numItems == 0 || bufSize == 0)
            return 0;

        startIdx = NVMin(numItems - 1, startIdx);
        QT3DSU32 retval = 0;
        for (typename eastl::hash_map<TKeyType, TValueType, THashType, TPredicate,
                                      ForwardingAllocator>::const_iterator iter = map.begin(),
                                                                           end = map.end();
             iter != end && bufSize; ++iter) {
            if (startIdx)
                --startIdx;
            else {
                buffer[retval] = op(iter->first);
                --bufSize;
                ++retval;
            }
        }
        return retval;
    }

    struct IdOp
    {
        template <typename TDataType>
        TDataType operator()(const TDataType &item)
        {
            return item;
        }
    };

    template <typename TKeyType, typename TValueType, typename THashType, typename TPredicateType>
    QT3DSU32
    GetMapKeys(const eastl::hash_map<TKeyType, TValueType, THashType, ForwardingAllocator> &map,
               TKeyType *buffer, QT3DSU32 bufSize, QT3DSU32 startIdx)
    {
        return GetMapKeysOp(map, buffer, bufSize, startIdx, IdOp());
    }

    struct DerefOp
    {
        template <typename TDataType>
        TDataType operator()(const TDataType *item)
        {
            return *item;
        }
    };

    template <typename TKeyType, typename TValueType, typename THashType, typename TPredicateType,
              typename TAllocatorType, typename TBufType, typename TOp>
    QT3DSU32 GetMapValues(
        const eastl::hash_map<TKeyType, TValueType, THashType, TPredicateType, TAllocatorType> &map,
        TBufType *buffer, QT3DSU32 bufSize, QT3DSU32 startIdx, TOp op)
    {
        QT3DSU32 numItems = static_cast<QT3DSU32>(map.size());
        if (numItems == 0 || bufSize == 0)
            return 0;

        startIdx = NVMin(numItems - 1, startIdx);
        QT3DSU32 retval = 0;
        for (typename eastl::hash_map<TKeyType, TValueType, THashType, TPredicateType,
                                      TAllocatorType>::const_iterator iter = map.begin(),
                                                                      end = map.end();
             iter != end && bufSize; ++iter) {
            if (startIdx)
                --startIdx;
            else {
                buffer[retval] = op(iter->second);
                --bufSize;
                ++retval;
            }
        }
        return retval;
    }

    template <typename TValueType, typename TBufType>
    QT3DSU32 GetArrayEntries(const nvvector<TValueType> &data, TBufType *buffer, QT3DSU32 bufSize,
                          QT3DSU32 startIdx)
    {
        QT3DSU32 numItems = static_cast<QT3DSU32>(data.size());
        if (numItems == 0 || bufSize == 0)
            return 0;

        startIdx = NVMin(numItems - 1, startIdx);
        QT3DSU32 available = NVMin(numItems - startIdx, bufSize);
        QT3DS_FOREACH(idx, available)
        buffer[idx] = data[idx + startIdx];
        return available;
    }

    template <typename TValueType, typename TBufType>
    QT3DSU32 GetDataRefEntries(const NVDataRef<TValueType> &data, TBufType *buffer, QT3DSU32 bufSize,
                            QT3DSU32 startIdx)
    {
        QT3DSU32 numItems = static_cast<QT3DSU32>(data.size());
        if (numItems == 0 || bufSize == 0)
            return 0;

        startIdx = NVMin(numItems - 1, startIdx);
        QT3DSU32 available = NVMin(numItems - startIdx, bufSize);
        QT3DS_FOREACH(idx, available)
        buffer[idx] = data.mData[idx + startIdx];
        return available;
    }

    template <typename TValueType, typename TBufType>
    QT3DSU32 GetDataRefEntries(const NVConstDataRef<TValueType> &data, TBufType *buffer, QT3DSU32 bufSize,
                            QT3DSU32 startIdx)
    {
        QT3DSU32 numItems = static_cast<QT3DSU32>(data.size());
        if (numItems == 0 || bufSize == 0)
            return 0;

        startIdx = NVMin(numItems - 1, startIdx);
        QT3DSU32 available = NVMin(numItems - startIdx, bufSize);
        QT3DS_FOREACH(idx, available)
        buffer[idx] = data.mData[idx + startIdx];
        return available;
    }

    template <typename TKeyType, typename THashType, typename TPredicate, typename TAllocator>
    QT3DSU32 GetHashSetEntries(eastl::hash_set<TKeyType, THashType, TPredicate, TAllocator> &data,
                            TKeyType *buffer, QT3DSU32 bufSize, QT3DSU32 startIdx)
    {
        QT3DSU32 numItems = static_cast<QT3DSU32>(data.size());
        if (numItems == 0 || bufSize == 0)
            return 0;
        startIdx = NVMin(numItems - 1, startIdx);
        QT3DSU32 available = NVMin(numItems - startIdx, bufSize);
        QT3DSU32 idx = 0;
        for (typename eastl::hash_set<TKeyType, THashType, TPredicate, TAllocator>::const_iterator
                 iter = data.begin(),
                 end = data.end();
             iter != end && idx < available; ++iter, ++idx) {
            if (startIdx) {
                --startIdx;
                continue;
            }
            buffer[idx] = *iter;
        }
        return available;
    }

    template <typename TDataType>
    void memCopyT(TDataType *dest, const TDataType *src, QT3DSU32 size)
    {
        memCopy(dest, src, size * sizeof(TDataType));
    }

    template <typename TDataType>
    NVDataRef<TDataType> PtrAtOffset(QT3DSU8 *baseData, QT3DSU32 offset, QT3DSU32 byteSize)
    {
        return NVDataRef<TDataType>(byteSize ? (TDataType *)(baseData + offset) : NULL,
                                    byteSize / sizeof(TDataType));
    }

    struct char_hasher
    {
        size_t operator()(const char *item) const { return eastl::hash<const char8_t *>()(item); }
    };

    struct char_equal_to
    {
        bool operator()(const char *lhs, const char *rhs) const
        {
            if (lhs == NULL)
                lhs = "";
            if (rhs == NULL)
                rhs = "";
            return strcmp(lhs, rhs) == 0;
        }
    };

    struct wide_char_hasher
    {
        size_t operator()(const char16_t *item) const
        {
            return eastl::hash<const char16_t *>()(item);
        }
    };

    struct wide_char_equal_to
    {
        bool operator()(const char16_t *lhs, const char16_t *rhs) const
        {
            if (lhs == NULL)
                lhs = reinterpret_cast<const char16_t *>(L"");
            if (rhs == NULL)
                rhs = reinterpret_cast<const char16_t *>(L"");
            while (*lhs && *rhs) {
                int answer = *lhs - *rhs;
                if (answer)
                    return false;
                ++lhs;
                ++rhs;
            }
            return *lhs == *rhs;
        }
    };
}
}
#endif
