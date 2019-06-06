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

#ifndef QT3DS_FOUNDATION_PSMUTEX_H
#define QT3DS_FOUNDATION_PSMUTEX_H

#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSNoCopy.h"

/*
 * This <new> inclusion is a best known fix for gcc 4.4.1 error:
 * Creating object file for apex/src/NVAllocator.cpp ...
 * In file included from apex/include/Qt3DSFoundation.h:30,
 *                from apex/src/NVAllocator.cpp:26:
 * apex/include/Qt3DSMutex.h: In constructor  'nv::foundation::MutexT<Alloc>::MutexT(const Alloc&)':
 * apex/include/Qt3DSMutex.h:92: error: no matching function for call to 'operator new(unsigned int,
 * qt3ds::foundation::MutexImpl*&)'
 * <built-in>:0: note: candidates are: void* operator new(unsigned int)
 */
#include <new>

namespace qt3ds {
namespace foundation {
#ifdef QT3DS_FOUNDATION_NO_EXPORTS
    class QT3DS_AUTOTEST_EXPORT MutexImpl
#else
    class QT3DS_FOUNDATION_API MutexImpl
#endif
    {
    public:
        /**
        The constructor for Mutex creates a mutex. It is initially unlocked.
        */
        MutexImpl();

        /**
        The destructor for Mutex deletes the mutex.
        */
        ~MutexImpl();

        /**
        Acquire (lock) the mutex. If the mutex is already locked
        by another thread, this method blocks until the mutex is
        unlocked.
        */
        bool lock();

        /**
        Acquire (lock) the mutex. If the mutex is already locked
        by another thread, this method returns false without blocking.
        */
        bool trylock();

        /**
        Release (unlock) the mutex.
        */
        bool unlock();

        /**
        Size of this class.
        */
        static const QT3DSU32 size;
    };

    class Mutex
    {
    public:
        class ScopedLock : private NoCopy
        {
            Mutex &mMutex;

        public:
            QT3DS_INLINE ScopedLock(Mutex &mutex)
                : mMutex(mutex)
            {
                mMutex.lock();
            }
            QT3DS_INLINE ~ScopedLock() { mMutex.unlock(); }
        };

        /**
        The constructor for Mutex creates a mutex. It is initially unlocked.
        */
        Mutex(NVAllocatorCallback &alloc)
            : mAllocator(alloc)
        {
            mImpl = (MutexImpl *)QT3DS_ALLOC(alloc, MutexImpl::size, "MutexImpl");
            QT3DS_PLACEMENT_NEW(mImpl, MutexImpl)();
        }

        /**
        The destructor for Mutex deletes the mutex.
        */
        ~Mutex()
        {
            mImpl->~MutexImpl();
            QT3DS_FREE(mAllocator, mImpl);
        }

        /**
        Acquire (lock) the mutex. If the mutex is already locked
        by another thread, this method blocks until the mutex is
        unlocked.
        */
        bool lock() const { return mImpl->lock(); }

        /**
        Acquire (lock) the mutex. If the mutex is already locked
        by another thread, this method returns false without blocking.
        */
        bool trylock() const { return mImpl->trylock(); }

        /**
        Release (unlock) the mutex.
        */
        bool unlock() const { return mImpl->unlock(); }

    private:
        NVAllocatorCallback &mAllocator;
        MutexImpl *mImpl;
    };

    class QT3DS_FOUNDATION_API ReadWriteLock : private NoCopy
    {
    public:
        ReadWriteLock(NVAllocatorCallback &alloc);
        ~ReadWriteLock();

        void lockReader();
        void lockWriter();

        void unlockReader();
        void unlockWriter();

    private:
        class ReadWriteLockImpl *mImpl;
    };

    class ScopedReadLock : private NoCopy
    {
    public:
        QT3DS_INLINE ScopedReadLock(ReadWriteLock &lock)
            : mLock(lock)
        {
            mLock.lockReader();
        }
        QT3DS_INLINE ~ScopedReadLock() { mLock.unlockReader(); }

    private:
        ReadWriteLock &mLock;
    };

    class ScopedWriteLock : private NoCopy
    {
    public:
        QT3DS_INLINE ScopedWriteLock(ReadWriteLock &lock)
            : mLock(lock)
        {
            mLock.lockWriter();
        }
        QT3DS_INLINE ~ScopedWriteLock() { mLock.unlockWriter(); }

    private:
        ReadWriteLock &mLock;
    };

/*
 * Use this type of lock for mutex behaviour that must operate on SPU and PPU
 * On non-PS3 platforms, it is implemented using Mutex
 */
#ifndef QT3DS_PS3

    class AtomicLock : private NoCopy
    {
        Mutex mMutex;

    public:
        AtomicLock(NVAllocatorCallback &alloc)
            : mMutex(alloc)
        {
        }

        bool lock() { return mMutex.lock(); }

        bool trylock() { return mMutex.trylock(); }

        bool unlock() { return mMutex.unlock(); }
    };

    class AtomicLockCopy
    {
        AtomicLock *pLock;

    public:
        AtomicLockCopy()
            : pLock(NULL)
        {
        }

        AtomicLockCopy &operator=(AtomicLock &lock)
        {
            pLock = &lock;
            return *this;
        }

        bool lock() { return pLock->lock(); }

        bool trylock() { return pLock->trylock(); }

        bool unlock() { return pLock->unlock(); }
    };
#else
    struct AtomicLockImpl
    {
        QT3DS_ALIGN(128, QT3DSU32 m_Lock);
        QT3DSI32 m_LockId;
        QT3DSU32 m_LockCount;

        AtomicLockImpl();
    };
    class AtomicLock //: private NoCopy
    {
        friend class AtomicLockCopy;
        AtomicLockImpl *m_pImpl;

    public:
        AtomicLock();

        ~AtomicLock();

        bool lock();

        bool trylock();

        bool unlock();
    };

    // if an AtomicLock is copied and then the copy goes out of scope, it'll delete the atomic
    // primitive
    // (just a 128-byte aligned int) and cause a crash when it tries to delete it again
    // This class just uses the atomic primitive without releasing it in the end.

    class AtomicLockCopy
    {
        AtomicLockImpl *m_pImpl;

    public:
        AtomicLockCopy()
            : m_pImpl(NULL)
        {
        }

        AtomicLockCopy(const AtomicLock &lock)
            : m_pImpl(lock.m_pImpl)
        {
        }

        ~AtomicLockCopy() {}

        AtomicLockCopy &operator=(const AtomicLock &lock)
        {
            m_pImpl = lock.m_pImpl;
            return *this;
        }

        bool lock();

        bool trylock();

        bool unlock();
    };
#endif

#ifndef QT3DS_PS3

    class AtomicRwLock : private NoCopy
    {
        ReadWriteLock m_Lock;

    public:
        AtomicRwLock(NVAllocatorCallback &alloc)
            : m_Lock(alloc)
        {
        }

        void lockReader() { m_Lock.lockReader(); }
        void lockWriter() { m_Lock.lockWriter(); }

        bool tryLockReader()
        {
            // Todo - implement this
            m_Lock.lockReader();
            return true;
        }

        void unlockReader() { m_Lock.unlockReader(); }
        void unlockWriter() { m_Lock.unlockWriter(); }
    };
#else

    struct AtomicRwLockImpl
    {
        QT3DS_ALIGN(128, volatile QT3DSU32 m_Lock);
        QT3DS_ALIGN(128, volatile QT3DSU32 m_ReadCounter);
        QT3DSI32 m_LockId;
        QT3DSU32 m_LockCount;

        AtomicRwLockImpl();
    };

    class AtomicRwLock : private NoCopy
    {
        AtomicRwLockImpl *m_pImpl;

    public:
        AtomicRwLock();

        ~AtomicRwLock();

        void lockReader();

        bool tryLockReader();

        void lockWriter();

        void unlockReader();

        void unlockWriter();
    };

#endif

    class ScopedAtomicLock : private NoCopy
    {
        QT3DS_INLINE ScopedAtomicLock(AtomicLock &lock)
            : mLock(lock)
        {
            mLock.lock();
        }
        QT3DS_INLINE ~ScopedAtomicLock() { mLock.unlock(); }

    private:
        AtomicLock &mLock;
    };

} // namespace foundation
} // namespace qt3ds

#endif
