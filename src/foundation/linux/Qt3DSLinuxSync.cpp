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
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSAllocatorCallback.h"

#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

namespace qt3ds {
namespace foundation {

    class SyncImpl
    {
    public:
        SyncImpl(NVAllocatorCallback &alloc)
            : mAllocator(alloc)
        {
        }
        NVAllocatorCallback &mAllocator;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        volatile bool is_set;
    };

    struct NVLinuxScopeLock
    {
        NVLinuxScopeLock(pthread_mutex_t &m)
            : mMutex(m)
        {
            pthread_mutex_lock(&mMutex);
        }

        ~NVLinuxScopeLock() { pthread_mutex_unlock(&mMutex); }
    private:
        pthread_mutex_t &mMutex;
    };

    Sync::Sync(NVAllocatorCallback &alloc)
    {
        mImpl = QT3DS_NEW(alloc, SyncImpl)(alloc);
        int status = pthread_mutex_init(&mImpl->mutex, 0);
        QT3DS_ASSERT(!status);
        status = pthread_cond_init(&mImpl->cond, 0);
        QT3DS_ASSERT(!status);
        mImpl->is_set = false;
    }

    Sync::~Sync()
    {
        pthread_cond_destroy(&mImpl->cond);
        pthread_mutex_destroy(&mImpl->mutex);
        QT3DS_FREE(mImpl->mAllocator, mImpl);
    }

    void Sync::reset()
    {
        NVLinuxScopeLock lock(mImpl->mutex);
        mImpl->is_set = false;
    }

    void Sync::set()
    {
        NVLinuxScopeLock lock(mImpl->mutex);
        if (!mImpl->is_set) {
            mImpl->is_set = true;
            pthread_cond_broadcast(&mImpl->cond);
        }
    }

    bool Sync::wait(QT3DSU32 ms)
    {
        NVLinuxScopeLock lock(mImpl->mutex);
        if (!mImpl->is_set) {
            if (ms == QT3DSU32(-1)) {
                int status = pthread_cond_wait(&mImpl->cond, &mImpl->mutex);
                QT3DS_ASSERT(!status);
                (void)status;
            } else {
                timespec ts;
                timeval tp;
                gettimeofday(&tp, NULL);
                QT3DSU32 sec = ms / 1000;
                QT3DSU32 usec = (ms - 1000 * sec) * 1000;

                // sschirm: taking into account that us might accumulate to a second
                // otherwise the pthread_cond_timedwait complains on osx.
                usec = tp.tv_usec + usec;
                QT3DSU32 div_sec = usec / 1000000;
                QT3DSU32 rem_usec = usec - div_sec * 1000000;

                ts.tv_sec = tp.tv_sec + sec + div_sec;
                ts.tv_nsec = rem_usec * 1000;

                int ierr = pthread_cond_timedwait(&mImpl->cond, &mImpl->mutex, &ts);
                QT3DS_ASSERT((ierr == 0) || (ierr == ETIMEDOUT));
                (void)ierr;
            }
        }
        return mImpl->is_set;
    }

} // namespace foundation
} // namespace qt3ds
