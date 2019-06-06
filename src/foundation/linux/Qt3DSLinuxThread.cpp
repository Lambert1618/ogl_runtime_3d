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
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSThread.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#if !defined(QT3DS_APPLE) && !defined(ANDROID) && !defined(__CYGWIN__) && !defined(__QNX__) && !defined(__INTEGRITY)
#include <bits/local_lim.h> // PTHREAD_STACK_MIN
#endif
#include <stdio.h>
#include <pthread.h>
#if !defined(__QNX__) && !defined(__INTEGRITY)
#include <unistd.h>
#include <sys/syscall.h>
#if !defined(QT3DS_APPLE)
#include <asm/unistd.h>
#include <sys/resource.h>
#endif
#endif

#define NVSpinLockPause() asm("nop")

namespace qt3ds {
namespace foundation {
    using namespace intrinsics;

    typedef enum { _NVThreadNotStarted, _NVThreadStarted, _NVThreadStopped } NVThreadState;

    class ThreadImpl
    {
    public:
        ThreadImpl(NVFoundationBase &foundation)
            : mFoundation(foundation)
        {
        }
        NVFoundationBase &mFoundation;
        Thread::ExecuteFn fn;
        void *arg;
        volatile QT3DSI32 quitNow;
        volatile QT3DSI32 threadStarted;
        NVThreadState state;

        pthread_t thread;
        pid_t tid;
    };

    void *NVThreadStart(void *arg);

    Thread::Id Thread::getId() { return Id(pthread_self()); }

    Thread::Thread(NVFoundationBase &foundation)
    {
        mImpl = QT3DS_NEW(foundation.getAllocator(), ThreadImpl)(foundation);
        mImpl->thread = 0;
        mImpl->tid = 0;
        mImpl->state = _NVThreadNotStarted;
        mImpl->quitNow = 0;
        mImpl->threadStarted = 0;
        mImpl->fn = NULL;
        mImpl->arg = NULL;
    }

    Thread::Thread(NVFoundationBase &foundation, Thread::ExecuteFn fn, void *arg)
    {
        mImpl = (ThreadImpl *)QT3DS_NEW(foundation.getAllocator(), ThreadImpl)(foundation);
        mImpl->thread = 0;
        mImpl->tid = 0;
        mImpl->state = _NVThreadNotStarted;
        mImpl->quitNow = 0;
        mImpl->threadStarted = 0;
        mImpl->fn = fn;
        mImpl->arg = arg;

        start(0);
    }

    Thread::~Thread()
    {
        if (mImpl->state == _NVThreadStarted)
            kill();
        QT3DS_FREE(mImpl->mFoundation.getAllocator(), mImpl);
    }

    void Thread::start(QT3DSU32 stackSize)
    {
        if (mImpl->state != _NVThreadNotStarted)
            return;

        if (stackSize == 0)
            stackSize = DEFAULT_STACK_SIZE;

#if defined(PTHREAD_STACK_MIN) && !defined(ANDROID)
        if (stackSize < PTHREAD_STACK_MIN) {
            qCWarning(WARNING, "Thread::start(): stack size was set below PTHREAD_STACK_MIN");
            stackSize = PTHREAD_STACK_MIN;
        }
#endif

        pthread_attr_t attr;
        int status = pthread_attr_init(&attr);
        QT3DS_ASSERT(!status);

        status = pthread_attr_setstacksize(&attr, stackSize);
        QT3DS_ASSERT(!status);
        status = pthread_create(&mImpl->thread, &attr, NVThreadStart, this);
        QT3DS_ASSERT(!status);

#ifndef __QNX__
        // wait for thread to startup and write out TID
        // otherwise TID dependent calls like setAffinity will fail.
        while (atomicCompareExchange(&(mImpl->threadStarted), 1, 1) == 0)
            yield();
#endif

        mImpl->state = _NVThreadStarted;

        status = pthread_attr_destroy(&attr);
        QT3DS_ASSERT(!status);
    }

    static void setTid(ThreadImpl &threadImpl)
    {
#ifndef __QNX__
// query TID
#ifdef QT3DS_APPLE
        threadImpl.tid = syscall(SYS_gettid);
#elif defined(__INTEGRITY)
        pthread_t ptid = pthread_self();
        uint64_t threadId = 0;
        memcpy(&threadId, &ptid, std::min(sizeof(threadId), sizeof(ptid)));
        threadImpl.tid = threadId;
#else
        threadImpl.tid = syscall(__NR_gettid);
#endif

        // notify/unblock parent thread
        atomicCompareExchange(&(threadImpl.threadStarted), 1, 0);
#else
        QT3DS_ASSERT(false);
#endif
    }

    void *NVThreadStart(void *arg)
    {
        // run execute from base class to run gettid in the new thread's context
        ((Thread *)arg)->Thread::execute();
        return 0;
    }

    void Thread::execute(void)
    {
        // run setTid in thread's context
        setTid(*mImpl);

        // then run either the passed in function or execute from the derived class.
        if (mImpl->fn)
            (*mImpl->fn)(mImpl->arg);
        else
            this->execute();
    }

    void Thread::signalQuit() { atomicIncrement(&(mImpl->quitNow)); }

    bool Thread::waitForQuit()
    {
        if (mImpl->state == _NVThreadNotStarted)
            return false;

        pthread_join(mImpl->thread, NULL);
        return true;
    }

    bool Thread::quitIsSignalled()
    {
#ifndef __QNX__
        return atomicCompareExchange(&(mImpl->quitNow), 0, 0) != 0;
#else
        // Hope for memory locking on the arm.
        return mImpl->quitNow != 0;
#endif
    }

    void Thread::quit()
    {
        mImpl->state = _NVThreadStopped;
        pthread_exit(0);
    }

    void Thread::kill()
    {
#ifndef ANDROID
        if (mImpl->state == _NVThreadStarted)
            pthread_cancel(mImpl->thread);
        mImpl->state = _NVThreadStopped;
#else
        qCWarning(WARNING, "Thread::kill() called, but is not implemented");
#endif
    }

    void Thread::sleep(QT3DSU32 ms)
    {
        timespec sleepTime;
        QT3DSU32 remainder = ms % 1000;
        sleepTime.tv_sec = ms - remainder;
        sleepTime.tv_nsec = remainder * 1000000L;

        while (nanosleep(&sleepTime, &sleepTime) == -1)
            continue;
    }

    void Thread::yield() { sched_yield(); }

    QT3DSU32 Thread::setAffinityMask(QT3DSU32 mask)
    {
        // Same as windows impl if mask is zero
        if (!mask)
            return 0;

#ifndef __QNX__
        QT3DSU64 prevMask = 0;

// Apple doesn't support syscall with getaffinity and setaffinity
#if !defined(QT3DS_APPLE) && !defined(__INTEGRITY)
        QT3DSI32 errGet = syscall(__NR_sched_getaffinity, mImpl->tid, sizeof(prevMask), &prevMask);
        if (errGet < 0)
            return 0;

        QT3DSI32 errSet = syscall(__NR_sched_setaffinity, mImpl->tid, sizeof(mask), &mask);
        if (errSet != 0)
            return 0;
#endif

        return QT3DSU32(prevMask);
#else
        QT3DS_ASSERT(false);
        return 0;
#endif
    }

    void Thread::setName(const char *name)
    {
#if (defined(ANDROID) && (__ANDROID_API__ > 8))
        pthread_setname_np(mImpl->thread, name);
#else
// not implemented because most unix APIs expect setName()
// to be called from the thread's context. Example see next comment:

// this works only with the current thread and can rename
// the main process if used in the wrong context:
// prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name) ,0,0,0);
#endif
    }

#if !defined(QT3DS_APPLE) && !defined(__INTEGRITY)
    static ThreadPriority::Enum convertPriorityFromLinux(QT3DSU32 inPrio, int policy)
    {
        QT3DS_COMPILE_TIME_ASSERT(ThreadPriority::eLOW > ThreadPriority::eHIGH);
        QT3DS_COMPILE_TIME_ASSERT(ThreadPriority::eHIGH == 0);

        int maxL = sched_get_priority_max(policy);
        int minL = sched_get_priority_min(policy);
        int rangeL = maxL - minL;
        int rangeNV = ThreadPriority::eLOW - ThreadPriority::eHIGH;

        // case for default scheduler policy
        if (rangeL == 0)
            return ThreadPriority::eNORMAL;

        float floatPrio = (float(maxL - inPrio) * float(rangeNV)) / float(rangeL);

        return ThreadPriority::Enum(int(roundf(floatPrio)));
    }

    static int convertPriorityToLinux(ThreadPriority::Enum inPrio, int policy)
    {
        int maxL = sched_get_priority_max(policy);
        int minL = sched_get_priority_min(policy);
        int rangeL = maxL - minL;
        int rangeNV = ThreadPriority::eLOW - ThreadPriority::eHIGH;

        // case for default scheduler policy
        if (rangeL == 0)
            return 0;

        float floatPrio = (float(ThreadPriority::eLOW - inPrio) * float(rangeL)) / float(rangeNV);

        return minL + int(roundf(floatPrio));
    }
#endif

    void Thread::setPriority(ThreadPriority::Enum val)
    {
#if !defined(QT3DS_APPLE) && !defined(__INTEGRITY)
        int policy;
        sched_param s_param;
        pthread_getschedparam(mImpl->thread, &policy, &s_param);
        s_param.sched_priority = convertPriorityToLinux(val, policy);
        pthread_setschedparam(mImpl->thread, policy, &s_param);
#endif
    }

    ThreadPriority::Enum Thread::getPriority(Id pthread)
    {
#if !defined(QT3DS_APPLE) && !defined(__INTEGRITY)
        int policy;
        sched_param s_param;
        int ret = pthread_getschedparam(pthread_t(pthread), &policy, &s_param);
        if (ret == 0)
            return convertPriorityFromLinux(s_param.sched_priority, policy);
        else
            return ThreadPriority::eNORMAL;
#else
        return ThreadPriority::eNORMAL;
#endif
    }

    QT3DSU32 TlsAlloc()
    {
#if !defined(__INTEGRITY)
        pthread_key_t key;
        int status = pthread_key_create(&key, NULL);
        QT3DS_ASSERT(!status);
        (void)status;
        return (QT3DSU32)key;
#else
        QT3DS_ASSERT(false);
        return 0;
#endif
    }

    void TlsFree(QT3DSU32 index)
    {
        int status = pthread_key_delete((pthread_key_t)index);
        QT3DS_ASSERT(!status);
        (void)status;
    }

    void *TlsGet(QT3DSU32 index) { return (void *)pthread_getspecific((pthread_key_t)index); }

    QT3DSU32 TlsSet(QT3DSU32 index, void *value)
    {
        int status = pthread_setspecific((pthread_key_t)index, value);
        QT3DS_ASSERT(!status);
        return !status;
    }

    // DM: On Linux x86-32, without implementation-specific restrictions
    // the default stack size for a new thread should be 2 megabytes (kernel.org).
    // NOTE: take care of this value on other architecutres!
    const QT3DSU32 Thread::DEFAULT_STACK_SIZE = 1 << 21;

} // namespace foundation
} // namespace qt3ds
