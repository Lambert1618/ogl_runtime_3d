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

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSAtomic.h"

#include <pthread.h>

namespace qt3ds {
namespace foundation {

    namespace {
        pthread_mutex_t *getMutex(MutexImpl *impl)
        {
            return reinterpret_cast<pthread_mutex_t *>(impl);
        }
    }

    MutexImpl::MutexImpl()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(getMutex(this), &attr);
        pthread_mutexattr_destroy(&attr);
    }

    MutexImpl::~MutexImpl() { pthread_mutex_destroy(getMutex(this)); }

    bool MutexImpl::lock() { return !pthread_mutex_lock(getMutex(this)); }

    bool MutexImpl::trylock() { return !pthread_mutex_trylock(getMutex(this)); }

    bool MutexImpl::unlock() { return !pthread_mutex_unlock(getMutex(this)); }

    const QT3DSU32 MutexImpl::size = sizeof(pthread_mutex_t);

    class ReadWriteLockImpl
    {
    public:
        ReadWriteLockImpl(NVAllocatorCallback &alloc)
            : mAllocator(alloc)
        {
        }
        NVAllocatorCallback &mAllocator;
        pthread_mutex_t mutex;
        volatile int readerCounter;
    };

    ReadWriteLock::ReadWriteLock(NVAllocatorCallback &alloc)
    {
        mImpl = QT3DS_NEW(alloc, ReadWriteLockImpl)(alloc);

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mImpl->mutex, &attr);
        pthread_mutexattr_destroy(&attr);

        mImpl->readerCounter = 0;
    }

    ReadWriteLock::~ReadWriteLock()
    {
        pthread_mutex_destroy(&mImpl->mutex);
        QT3DS_FREE(mImpl->mAllocator, mImpl);
    }

    void ReadWriteLock::lockReader()
    {
        pthread_mutex_lock(&mImpl->mutex);

        atomicIncrement(&mImpl->readerCounter);

        pthread_mutex_unlock(&mImpl->mutex);
    }

    void ReadWriteLock::lockWriter()
    {
        pthread_mutex_lock(&mImpl->mutex);

        while (mImpl->readerCounter != 0)
            ;
    }

    void ReadWriteLock::unlockReader() { atomicDecrement(&mImpl->readerCounter); }

    void ReadWriteLock::unlockWriter() { pthread_mutex_unlock(&mImpl->mutex); }

} // namespace foundation
} // namespace qt3ds
