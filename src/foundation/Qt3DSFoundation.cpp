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

#include "foundation/Qt3DSFoundation.h"

#include "foundation/Qt3DSQuat.h"
#include "foundation/Qt3DSThread.h"
#include "foundation/Qt3DSUtilities.h"
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSLogging.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "EASTL/hash_map.h"

#include <stdio.h>
#ifdef _WIN32
#pragma warning(disable : 4996) // intentionally suppressing this warning message
#endif
namespace qt3ds {
namespace foundation {
    using namespace intrinsics;
    union TempAllocatorChunk;

    class NVAllocatorListenerManager;

    class QT3DS_FOUNDATION_API Foundation : public NVFoundation
    {

        Foundation(NVAllocatorCallback &alloc);
        ~Foundation();

    public:
        void addRef() override;
        void release() override;

        // factory
        static Foundation *createInstance(QT3DSU32 version, NVAllocatorCallback &alloc);

        NVBroadcastingAllocator &getAllocator() const override { return mAllocator; }
        NVAllocatorCallback &getAllocatorCallback() const override;
        NVAllocatorCallback &getCheckedAllocator() { return mAllocator; }

    private:
        class AlignCheckAllocator : public NVBroadcastingAllocator
        {
            static const QT3DSU32 MaxListenerCount = 5;

        public:
            AlignCheckAllocator(NVAllocatorCallback &originalAllocator)
                : mAllocator(originalAllocator)
                , mListenerCount(0)
            {
            }

            void deallocate(void *ptr) override
            {
                // So here, for performance reasons I don't grab the mutex.
                // The listener array is very rarely changing; for most situations
                // only at startup.  So it is unlikely that using the mutex
                // will help a lot but it could have serious perf implications.
                QT3DSU32 theCount = mListenerCount;
                for (QT3DSU32 idx = 0; idx < theCount; ++idx)
                    mListeners[idx]->onDeallocation(ptr);
                mAllocator.deallocate(ptr);
            }
            void *allocate(size_t size, const char *typeName, const char *filename, int line,
                           int flags) override;
            void *allocate(size_t size, const char *typeName, const char *filename, int line,
                           size_t alignment, size_t alignmentOffset) override;
            NVAllocatorCallback &getBaseAllocator() const { return mAllocator; }
            void registerAllocationListener(NVAllocationListener &inListener) override
            {
                QT3DS_ASSERT(mListenerCount < MaxListenerCount);
                if (mListenerCount < MaxListenerCount) {
                    mListeners[mListenerCount] = &inListener;
                    ++mListenerCount;
                }
            }
            void deregisterAllocationListener(NVAllocationListener &inListener) override
            {
                for (QT3DSU32 idx = 0; idx < mListenerCount; ++idx) {
                    if (mListeners[idx] == &inListener) {
                        mListeners[idx] = mListeners[mListenerCount - 1];
                        --mListenerCount;
                        break;
                    }
                }
            }

        private:
            NVAllocatorCallback &mAllocator;
            // I am not sure about using a NVArray here.
            // For now, this is fine.
            NVAllocationListener *mListeners[MaxListenerCount];
            volatile QT3DSU32 mListenerCount;
        };

        mutable AlignCheckAllocator mAllocator;
        QT3DSU32 mRefCount;
        Mutex mRefCountMutex;
    };

    Foundation::Foundation(NVAllocatorCallback &alloc)
        : mAllocator(alloc)
        , mRefCount(0)
        , mRefCountMutex(alloc)

    {
    }

    Foundation::~Foundation() {}

    NVAllocatorCallback &Foundation::getAllocatorCallback() const
    {
        return mAllocator.getBaseAllocator();
    }

    Foundation *Foundation::createInstance(QT3DSU32 version, NVAllocatorCallback &alloc)
    {
        if (version != QT3DS_FOUNDATION_VERSION) {
            qCCritical(INVALID_PARAMETER, "Wrong version: foundation version is %d, tried to create %d",
                      QT3DS_FOUNDATION_VERSION, version);
            return 0;
        }
        Foundation *mInstance = NULL;

        if (!mInstance) {
            // if we don't assign this here, the Foundation object can't create member
            // subobjects which require the allocator

            mInstance = reinterpret_cast<Foundation *>(
                alloc.allocate(sizeof(Foundation), "Foundation", __FILE__, __LINE__));

            if (mInstance) {
                QT3DS_PLACEMENT_NEW(mInstance, Foundation)(alloc);

                QT3DS_ASSERT(mInstance->mRefCount == 0);

                return mInstance;
            } else {
                qCCritical(INTERNAL_ERROR, "Memory allocation for foundation object failed.");
            }
        } else {
            qCCritical(
                INVALID_OPERATION,
                "Foundation object exists already. Only one instance per process can be created.");
        }

        return 0;
    }

    void Foundation::addRef()
    {
        mRefCountMutex.lock();
        ++mRefCount;
        mRefCountMutex.unlock();
    }

    void Foundation::release()
    {
        mRefCountMutex.lock();
        if (mRefCount)
            --mRefCount;
        QT3DSU32 refCount = mRefCount;
        mRefCountMutex.unlock();
        if (!refCount) {
            NVAllocatorCallback &alloc = mAllocator.getBaseAllocator();
            this->~Foundation();
            alloc.deallocate(this);
        }
    }

    void *Foundation::AlignCheckAllocator::allocate(size_t size, const char *typeName,
                                                    const char *filename, int line, int)
    {
        void *addr = mAllocator.allocate(size, typeName, filename, line);

        if (!addr)
            qFatal("User allocator returned NULL.");

        if (!(reinterpret_cast<size_t>(addr) & 15)) {
            // Same comment as before in the allocation system.
            // We don't lock the listener array mutex because of an assumption
            // where the listener array is rarely changing.
            QT3DSU32 theCount = mListenerCount;
            for (QT3DSU32 idx = 0; idx < theCount; ++idx)
                mListeners[idx]->onAllocation(size, typeName, filename, line, addr);
            return addr;
        }

        qFatal("Allocations for qt3ds::foundation must be 16-byte aligned.");
        return 0;
    }

    void *Foundation::AlignCheckAllocator::allocate(size_t size, const char *typeName,
                                                    const char *filename, int line,
                                                    size_t /*alignment*/,
                                                    size_t /*alignmentOffset*/)
    {
        return allocate(size, typeName, filename, line, 0);
    }

} // namespace foundation
} // namespace qt3ds

qt3ds::NVFoundation *NVCreateFoundation(qt3ds::QT3DSU32 version, qt3ds::NVAllocatorCallback &allocator)
{
    return qt3ds::foundation::Foundation::createInstance(version, allocator);
}
