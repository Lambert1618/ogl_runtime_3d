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
#ifndef QT3DS_FOUNDATION_POOLING_ALLOCATOR_H
#define QT3DS_FOUNDATION_POOLING_ALLOCATOR_H
#pragma once

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "foundation/Qt3DSMutex.h"

// Pooling allocator.  Not designed for small allocations
// starting at 64 bytes and up to 4 K, allocator uses pools.
// Above that, uses default allocator.  THis object is absolutely not threadsafe.
// This addes 8 bytes to each allocation in order to safely track the allocation size.
// There is no strict requirement to deallocate; this allocator automatically deallocates
// anything allocated through it.
namespace qt3ds {
namespace foundation {
    struct SPoolingAllocator : public NVAllocatorCallback
    {
        typedef Mutex TMutexType;
        typedef Mutex::ScopedLock TLockType;

        NVAllocatorCallback &m_Allocator;
        TMutexType m_Mutex;

        struct SAllocationTag
        {
            QT3DSU32 m_Tag;
            size_t m_Size;
            SAllocationTag(size_t inSize = 0)
                : m_Tag(0xAB5534CD)
                , m_Size(inSize)
            {
            }
        };

        template <size_t TObjSize>
        struct SPoolObj
        {
            QT3DSU32 m_Buffer[TObjSize / 4];
            SPoolObj() {}
        };
#define ITERATE_POOLING_ALLOCATOR_POOL_SIZES                                                       \
    HANDLE_POOLING_ALLOCATOR_POOL_SIZE(64)                                                         \
    HANDLE_POOLING_ALLOCATOR_POOL_SIZE(128)                                                        \
    HANDLE_POOLING_ALLOCATOR_POOL_SIZE(256)                                                        \
    HANDLE_POOLING_ALLOCATOR_POOL_SIZE(512)                                                        \
    HANDLE_POOLING_ALLOCATOR_POOL_SIZE(1024)

#define HANDLE_POOLING_ALLOCATOR_POOL_SIZE(sz)                                                     \
    Pool<SPoolObj<sz>, ForwardingAllocator, 4> m_Pool##sz;
        ITERATE_POOLING_ALLOCATOR_POOL_SIZES
#undef HANDLE_POOLING_ALLOCATOR_POOL_SIZE

        SSAutoDeallocatorAllocator m_LargeAllocator;

#define HANDLE_POOLING_ALLOCATOR_POOL_SIZE(sz)                                                     \
    , m_Pool##sz(ForwardingAllocator(inAllocator, "PoolingAllocatorPool"))

        SPoolingAllocator(NVAllocatorCallback &inAllocator)
            : m_Allocator(inAllocator)
            , m_Mutex(inAllocator) ITERATE_POOLING_ALLOCATOR_POOL_SIZES
            , m_LargeAllocator(inAllocator)
        {
        }
#undef HANDLE_POOLING_ALLOCATOR_POOL_SIZE

        void *doAllocateFromPool(size_t size)
        {
            TLockType locker(m_Mutex);

#define HANDLE_POOLING_ALLOCATOR_POOL_SIZE(sz)                                                     \
    if (size <= sz)                                                                                \
        return m_Pool##sz.allocate(__FILE__, __LINE__);
            ITERATE_POOLING_ALLOCATOR_POOL_SIZES
#undef HANDLE_POOLING_ALLOCATOR_POOL_SIZE

            return m_LargeAllocator.allocate(size, "largetype", __FILE__, __LINE__, 0);
        }

        void *doAllocate(size_t size)
        {
            size += sizeof(SAllocationTag);
            SAllocationTag *tag = reinterpret_cast<SAllocationTag *>(doAllocateFromPool(size));
            new (tag) SAllocationTag(size);
            QT3DSU8 *data = reinterpret_cast<QT3DSU8 *>(tag);
            return data + sizeof(SAllocationTag);
        }

        void *allocate(size_t size, const char * /*typeName*/, const char * /*filename*/,
                               int /*line*/, int /*flags*/ = 0) override
        {
            return doAllocate(size);
        }

        void *allocate(size_t size, const char * /*typeName*/, const char * /*filename*/,
                               int /*line*/, size_t /*alignment*/, size_t /*alignmentOffset*/) override
        {
            return doAllocate(size);
        }

        /**
        \brief Frees memory previously allocated by allocate().

        <b>Threading:</b> This function should be thread safe as it can be called in the context of
        the user thread
        and physics processing thread(s).

        \param ptr Memory to free.
        */
        void deallocate(void *ptr) override
        {
            TLockType locker(m_Mutex);

            // Deallocate on null is fine.
            if (ptr == NULL)
                return;

            SAllocationTag tempTag;
            QT3DSU8 *dataPtr = reinterpret_cast<QT3DSU8 *>(ptr);
            SAllocationTag *theTag =
                reinterpret_cast<SAllocationTag *>(dataPtr - sizeof(SAllocationTag));
            if (theTag->m_Tag != tempTag.m_Tag) {
                QT3DS_ASSERT(false);
                return;
            }

            size_t size = theTag->m_Size;

            // We add this offset at allocation time
            ptr = dataPtr - sizeof(SAllocationTag);

#define HANDLE_POOLING_ALLOCATOR_POOL_SIZE(sz)                                                     \
    if (size <= sz) {                                                                              \
        m_Pool##sz.deallocate(ptr);                                                                \
        return;                                                                                    \
    }
            ITERATE_POOLING_ALLOCATOR_POOL_SIZES
#undef HANDLE_POOLING_ALLOCATOR_POOL_SIZE
            m_LargeAllocator.deallocate(ptr);
        }
    };
}
}

#endif
