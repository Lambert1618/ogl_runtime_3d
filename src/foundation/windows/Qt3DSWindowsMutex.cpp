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

#include "foundation/windows/Qt3DSWindowsInclude.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSAssert.h"

namespace qt3ds {
namespace foundation {

    namespace {
        CRITICAL_SECTION *getMutex(MutexImpl *impl)
        {
            return reinterpret_cast<CRITICAL_SECTION *>(impl);
        }
    }

    MutexImpl::MutexImpl() { InitializeCriticalSection(getMutex(this)); }

    MutexImpl::~MutexImpl() { DeleteCriticalSection(getMutex(this)); }

    bool MutexImpl::lock()
    {
        EnterCriticalSection(getMutex(this));
        return true;
    }

    bool MutexImpl::trylock() { return TryEnterCriticalSection(getMutex(this)) != 0; }

    bool MutexImpl::unlock()
    {
        LeaveCriticalSection(getMutex(this));
        return true;
    }

    const QT3DSU32 MutexImpl::size = sizeof(CRITICAL_SECTION);

    class ReadWriteLockImpl
    {
    public:
        ReadWriteLockImpl(NVAllocatorCallback &alloc)
            : mAllocator(alloc)
        {
        }
        NVAllocatorCallback &mAllocator;
        HANDLE hReaderEvent;
        HANDLE hMutex;
        CRITICAL_SECTION writerMutex;
        LONG counter; // count the number of readers in the lock.
        LONG recursionCounter; // handle recursive writer locking
    };

    ReadWriteLock::ReadWriteLock(NVAllocatorCallback &alloc)
    {
        mImpl = QT3DS_NEW(alloc, ReadWriteLockImpl)(alloc);

        mImpl->hReaderEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        QT3DS_ASSERT(mImpl->hReaderEvent != NULL);

        mImpl->hMutex = CreateEvent(NULL, FALSE, TRUE, NULL);
        QT3DS_ASSERT(mImpl->hMutex != NULL);

        InitializeCriticalSection(&mImpl->writerMutex);
        mImpl->counter = -1;
        mImpl->recursionCounter = 0;
    }

    ReadWriteLock::~ReadWriteLock()
    {
        if (mImpl->hReaderEvent != NULL) {
            CloseHandle(mImpl->hReaderEvent);
        }

        if (mImpl->hMutex != NULL) {
            CloseHandle(mImpl->hMutex);
        }

        DeleteCriticalSection(&mImpl->writerMutex);

        QT3DS_FREE(mImpl->mAllocator, mImpl);
    }

    void ReadWriteLock::lockReader()
    {
        if (InterlockedIncrement(&mImpl->counter) == 0) {
            WaitForSingleObject(mImpl->hMutex, INFINITE);
            SetEvent(mImpl->hReaderEvent);
        }

        WaitForSingleObject(mImpl->hReaderEvent, INFINITE);
    }

    void ReadWriteLock::lockWriter()
    {
        EnterCriticalSection(&mImpl->writerMutex);

        // we may already have the global mutex(really an event so we have to handle recursion
        // ourselves)
        if (++mImpl->recursionCounter == 1) {
            WaitForSingleObject(mImpl->hMutex, INFINITE);
        }
    }

    void ReadWriteLock::unlockReader()
    {
        if (InterlockedDecrement(&mImpl->counter) < 0) {
            ResetEvent(mImpl->hReaderEvent);
            SetEvent(mImpl->hMutex);
        }
    }

    void ReadWriteLock::unlockWriter()
    {
        if (--mImpl->recursionCounter == 0) {
            SetEvent(mImpl->hMutex);
        }

        LeaveCriticalSection(&mImpl->writerMutex);
    }

} // namespace foundation
} // namespace qt3ds
