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

#ifndef QT3DS_FOUNDATION_MEMORYBUFFER_H
#define QT3DS_FOUNDATION_MEMORYBUFFER_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSIntrinsics.h"

#ifdef __INTEGRITY
#define __restrict
#endif

namespace qt3ds {
namespace foundation {
    using namespace intrinsics;

    template <typename TAllocator = ForwardingAllocator>
    class MemoryBuffer : public TAllocator
    {
        QT3DSU8 *mBegin;
        QT3DSU8 *mEnd;
        QT3DSU8 *mCapacityEnd;

    public:
        MemoryBuffer(const TAllocator &inAlloc = TAllocator())
            : TAllocator(inAlloc)
            , mBegin(0)
            , mEnd(0)
            , mCapacityEnd(0)
        {
        }
        ~MemoryBuffer()
        {
            if (mBegin)
                TAllocator::deallocate(mBegin);
        }
        QT3DSU32 size() const { return static_cast<QT3DSU32>(mEnd - mBegin); }
        QT3DSU32 capacity() const { return static_cast<QT3DSU32>(mCapacityEnd - mBegin); }
        QT3DSU8 *begin() { return mBegin; }
        QT3DSU8 *end() { return mEnd; }
        const QT3DSU8 *begin() const { return mBegin; }
        const QT3DSU8 *end() const { return mEnd; }
        void clear() { mEnd = mBegin; }
        void write(QT3DSU8 inValue) { *growBuf(1) = inValue; }

        template <typename TDataType>
        void write(const TDataType &inValue)
        {
            const QT3DSU8 *__restrict readPtr = reinterpret_cast<const QT3DSU8 *>(&inValue);
            QT3DSU8 *__restrict writePtr = growBuf(sizeof(TDataType));
            for (QT3DSU32 idx = 0; idx < sizeof(TDataType); ++idx)
                writePtr[idx] = readPtr[idx];
        }

        template <typename TDataType>
        void write(const TDataType *inValue, QT3DSU32 inLength)
        {
            using namespace qt3ds::intrinsics;
            if (inValue && inLength) {
                QT3DSU32 writeSize = inLength * sizeof(TDataType);
                memCopy(growBuf(writeSize), inValue, writeSize);
            }
            if (inLength && !inValue) {
                QT3DS_ASSERT(false);
                // You can't not write something, because that will cause
                // the receiving end to crash.
                QT3DSU32 writeSize = inLength * sizeof(TDataType);
                for (QT3DSU32 idx = 0; idx < writeSize; ++idx)
                    write((QT3DSU8)0);
            }
        }

        void writeStrided(const QT3DSU8 *__restrict inData, QT3DSU32 inItemSize, QT3DSU32 inLength,
                          QT3DSU32 inStride)
        {
            if (inStride == 0 || inStride == inItemSize)
                write(inData, inLength * inItemSize);
            else if (inData && inLength) {
                QT3DSU32 writeSize = inLength * inItemSize;
                QT3DSU8 *__restrict writePtr = growBuf(writeSize);
                for (QT3DSU32 idx = 0; idx < inLength;
                     ++idx, writePtr += inItemSize, inData += inStride)
                    memCopy(writePtr, inData, inItemSize);
            }
        }
        QT3DSU8 *growBuf(QT3DSU32 inAmount)
        {
            QT3DSU32 offset = size();
            QT3DSU32 newSize = offset + inAmount;
            reserve(newSize);
            mEnd += inAmount;
            return mBegin + offset;
        }
        void writeZeros(QT3DSU32 inAmount)
        {
            QT3DSU32 offset = size();
            growBuf(inAmount);
            qt3ds::foundation::memZero(begin() + offset, inAmount);
        }
        void align(QT3DSU32 inAmount)
        {
            QT3DSU32 leftover = size() % inAmount;
            if (leftover)
                writeZeros(inAmount - leftover);
        }
        void reserve(QT3DSU32 newSize)
        {
            using namespace qt3ds::intrinsics;
            QT3DSU32 currentSize = size();
            if (newSize && newSize >= capacity()) {
                QT3DSU32 newDataSize = newSize * 2;
                if (newDataSize > 8192)
                    newDataSize = (QT3DSU32)((QT3DSU32)newSize * 1.2f);
                QT3DSU8 *newData =
                    static_cast<QT3DSU8 *>(TAllocator::allocate(newDataSize, __FILE__, __LINE__));
                if (mBegin) {
                    memCopy(newData, mBegin, currentSize);
                    TAllocator::deallocate(mBegin);
                }
                mBegin = newData;
                mEnd = mBegin + currentSize;
                mCapacityEnd = mBegin + newDataSize;
            }
        }
        operator NVDataRef<QT3DSU8>() { return NVDataRef<QT3DSU8>(begin(), size()); }
        operator NVConstDataRef<QT3DSU8>() const { return NVConstDataRef<QT3DSU8>(begin(), size()); }
    };
}
}

#endif