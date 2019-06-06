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
#ifndef QT3DS_FOUNDATION_THREAD_SAFE_QUEUE_H
#define QT3DS_FOUNDATION_THREAD_SAFE_QUEUE_H
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSSemaphore.h"
#include "EASTL/vector.h"

namespace qt3ds {
namespace foundation {

    template <typename TObjType, typename TAllocator = EASTLAllocatorType>
    class ThreadSafeQueue
    {
    public:
        static const QT3DSU32 MaxQueue = 0xffffffff;

    protected:
        eastl::vector<TObjType, TAllocator> mQueue;
        Semaphore mGetSemaphore;
        Semaphore mPutSemaphore;
        Mutex mMutex;
        const QT3DSU32 mMaxCount;

    public:
        ThreadSafeQueue(NVAllocatorCallback &alloc, QT3DSU32 maxCount = MaxQueue)
            : mMutex(alloc)
            , mGetSemaphore(alloc, 0, maxCount)
            , mPutSemaphore(alloc, maxCount, maxCount)
            , mMaxCount(maxCount)
        {
            mQueue.clear();
        }

        bool push(const TObjType &obj, QT3DSU32 milliseconds = MaxQueue)
        {
            if (mPutSemaphore.wait(milliseconds)) {
                Mutex::ScopedLock __locker(mMutex);
                mQueue.push_back(obj);
                mGetSemaphore.post();
                return true;
            }
            return false;
        }

        bool pop(TObjType &obj, QT3DSU32 milliseconds = 0)
        {
            if (mGetSemaphore.wait(milliseconds)) {
                Mutex::ScopedLock __locker(mMutex);
                obj = mQueue.back();
                mQueue.pop_back();
                mPutSemaphore.post();
                return true;
            }
            return false;
        }

        bool isFull()
        {
            Mutex::ScopedLock __locker(mMutex);
            return mQueue.size() >= mMaxCount;
        }

        bool isEmpty()
        {
            Mutex::ScopedLock __locker(mMutex);
            return mQueue.size() == 0;
        }

        void clear()
        {
            Mutex::ScopedLock __locker(mMutex);
            while (!mQueue.empty()) {
                if (mGetSemaphore.wait(MaxQueue)) {
                    mQueue.pop_back();
                    mPutSemaphore.post();
                }
            }
        }
    };
}
}

#endif