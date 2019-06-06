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
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSThread.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"

// an exception for setting the thread name in microsoft debuggers
#define QT3DS_MS_VC_EXCEPTION 0x406D1388

namespace qt3ds {
namespace foundation {

// struct for naming a thread in the debugger
#pragma pack(push, 8)

    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;

#pragma pack(pop)

    namespace {

        DWORD WINAPI NVThreadStart(LPVOID arg)
        {
            ((Thread *)arg)->execute();
            return 0;
        }
    }

    class ThreadImpl
    {
    public:
        enum State { NotStarted, Started, Stopped };
        ThreadImpl(NVFoundationBase &foundation)
            : mFoundation(foundation)
        {
        }
        NVFoundationBase &mFoundation;
        HANDLE thread;
        LONG quitNow; // Should be 32bit aligned on SMP systems.
        State state;
        DWORD threadID;

        Thread::ExecuteFn fn;
        void *arg;
    };

    Thread::Id Thread::getId() { return static_cast<Id>(GetCurrentThreadId()); }

    Thread::Thread(NVFoundationBase &foundation)
    {
        mImpl = (ThreadImpl *)QT3DS_NEW(foundation.getAllocator(), ThreadImpl)(foundation);
        mImpl->thread = NULL;
        mImpl->state = ThreadImpl::NotStarted;
        mImpl->quitNow = 0;
    }

    Thread::Thread(NVFoundationBase &foundation, ExecuteFn fn, void *arg)
    {
        mImpl = (ThreadImpl *)QT3DS_NEW(foundation.getAllocator(), ThreadImpl)(foundation);
        mImpl->thread = NULL;
        mImpl->state = ThreadImpl::NotStarted;
        mImpl->quitNow = 0;
        mImpl->fn = fn;
        mImpl->arg = arg;

        start(0);
    }

    Thread::~Thread()
    {
        if (mImpl->state == ThreadImpl::Started)
            kill();
        CloseHandle(mImpl->thread);
        QT3DS_FREE(mImpl->mFoundation.getAllocator(), mImpl);
    }

    void Thread::start(QT3DSU32 stackSize)
    {
        if (mImpl->state != ThreadImpl::NotStarted)
            return;
        mImpl->state = ThreadImpl::Started;

        mImpl->thread =
            CreateThread(NULL, stackSize, NVThreadStart, (LPVOID)this, 0, &mImpl->threadID);
    }

    void Thread::signalQuit() { InterlockedIncrement(&(mImpl->quitNow)); }

    bool Thread::waitForQuit()
    {
        if (mImpl->state == ThreadImpl::NotStarted)
            return false;

        WaitForSingleObject(mImpl->thread, INFINITE);
        return true;
    }

    bool Thread::quitIsSignalled()
    {
        return InterlockedCompareExchange(&(mImpl->quitNow), 0, 0) != 0;
    }

    void Thread::quit()
    {
        mImpl->state = ThreadImpl::Stopped;
        ExitThread(0);
    }

    void Thread::kill()
    {
        if (mImpl->state == ThreadImpl::Started)
            TerminateThread(mImpl->thread, 0);
        mImpl->state = ThreadImpl::Stopped;
    }

    void Thread::sleep(QT3DSU32 ms) { Sleep(ms); }

    void Thread::yield() { SwitchToThread(); }

    void Thread::execute(void) { (*mImpl->fn)(mImpl->arg); }

    QT3DSU32 Thread::setAffinityMask(QT3DSU32 mask)
    {
        return mask ? (QT3DSU32)SetThreadAffinityMask(mImpl->thread, mask) : 0;
    }

    void Thread::setName(const char *name)
    {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = mImpl->threadID;
        info.dwFlags = 0;

#ifdef QT3DS_VC
        // C++ Exceptions are disabled for this project, but SEH is not (and cannot be)
        // http://stackoverflow.com/questions/943087/what-exactly-will-happen-if-i-disable-c-exceptions-in-a-project
        __try {
            RaiseException(QT3DS_MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR),
                           (ULONG_PTR *)&info);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // this runs if not attached to a debugger (thus not really naming the thread)
        }
#else
        RaiseException(QT3DS_MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR),
                       (ULONG_PTR *)&info);
#endif
    }

    void Thread::setPriority(ThreadPriority::Enum prio)
    {
        switch (prio) {
        case ThreadPriority::eHIGH:
            SetThreadPriority(mImpl->thread, THREAD_PRIORITY_HIGHEST);
            break;
        case ThreadPriority::eABOVE_NORMAL:
            SetThreadPriority(mImpl->thread, THREAD_PRIORITY_ABOVE_NORMAL);
            break;
        case ThreadPriority::eNORMAL:
            SetThreadPriority(mImpl->thread, THREAD_PRIORITY_NORMAL);
            break;
        case ThreadPriority::eBELOW_NORMAL:
            SetThreadPriority(mImpl->thread, THREAD_PRIORITY_BELOW_NORMAL);
            break;
        case ThreadPriority::eLOW:
            SetThreadPriority(mImpl->thread, THREAD_PRIORITY_LOWEST);
            break;
        default:
            break;
        }
    }

    ThreadPriority::Enum Thread::getPriority(Id threadId)
    {
        ThreadPriority::Enum retval = ThreadPriority::eLOW;
        int priority = GetThreadPriority((HANDLE)threadId);
        StaticAssert<(THREAD_PRIORITY_HIGHEST > THREAD_PRIORITY_ABOVE_NORMAL)>::valid_expression();
        if (priority >= THREAD_PRIORITY_HIGHEST)
            retval = ThreadPriority::eHIGH;
        else if (priority >= THREAD_PRIORITY_ABOVE_NORMAL)
            retval = ThreadPriority::eABOVE_NORMAL;
        else if (priority >= THREAD_PRIORITY_NORMAL)
            retval = ThreadPriority::eNORMAL;
        else if (priority >= THREAD_PRIORITY_BELOW_NORMAL)
            retval = ThreadPriority::eBELOW_NORMAL;
        return retval;
    }

    QT3DSU32 TlsAlloc()
    {
        DWORD rv = ::TlsAlloc();
        QT3DS_ASSERT(rv != TLS_OUT_OF_INDEXES);
        return (QT3DSU32)rv;
    }

    void TlsFree(QT3DSU32 index) { ::TlsFree(index); }

    void *TlsGet(QT3DSU32 index) { return ::TlsGetValue(index); }

    QT3DSU32 TlsSet(QT3DSU32 index, void *value) { return ::TlsSetValue(index, value); }

    const QT3DSU32 Thread::DEFAULT_STACK_SIZE = 1048576;

} // namespace foundation
} // namespace qt3ds
