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

#ifndef QT3DS_FOUNDATION_PSTHREAD_H
#define QT3DS_FOUNDATION_PSTHREAD_H

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSFoundation.h"

// dsequeira: according to existing comment here (David Black would be my guess)
// "This is useful to reduce bus contention on tight spin locks. And it needs
// to be a macro as the xenon compiler often ignores even __forceinline." What's not
// clear is why a pause function needs inlining...? (TODO: check with XBox team)

// todo: these need to go somewhere else

#if defined(QT3DS_WINDOWS)
#define NVSpinLockPause() __asm pause
#elif defined(QT3DS_X360)
#define NVSpinLockPause() __asm nop
#elif defined(QT3DS_LINUX) || defined(QT3DS_ANDROID) || defined(QT3DS_APPLE) || defined(QT3DS_QNX)
#define NVSpinLockPause() asm("nop")
#elif defined(QT3DS_PS3)
#define NVSpinLockPause() asm("nop") // don't know if it's correct yet...
#define QT3DS_TLS_MAX_SLOTS 64
#elif defined(QT3DS_WII)
#define NVSpinLockPause() asm { nop } // don't know if it's correct yet...
#endif

namespace qt3ds {
namespace foundation {
    struct ThreadPriority // todo: put in some other header file
    {
        enum Enum {
            /**
            \brief High priority
            */
            eHIGH = 0,

            /**
            \brief Above Normal priority
            */
            eABOVE_NORMAL = 1,

            /**
            \brief Normal/default priority
            */
            eNORMAL = 2,

            /**
            \brief Below Normal priority
            */
            eBELOW_NORMAL = 3,

            /**
            \brief Low priority.
            */
            eLOW = 4,

            eFORCE_DWORD = 0xffFFffFF
        };
    };

    /**
    Thread abstraction API
    */

    class QT3DS_FOUNDATION_API Thread
    {
    public:
        static const QT3DSU32 DEFAULT_STACK_SIZE;
        typedef size_t Id; // space for a pointer or an integer
        typedef void *(*ExecuteFn)(void *);

        static Id getId();

        /**
        Construct (but do not start) the thread object. Executes in the context
        of the spawning thread
        */

        Thread(NVFoundationBase &foundation);

        /**
        Construct and start the the thread, passing the given arg to the given fn. (pthread style)
        */

        Thread(NVFoundationBase &foundation, ExecuteFn fn, void *arg);

        /**
        Deallocate all resources associated with the thread. Should be called in the
        context of the spawning thread.
        */

        virtual ~Thread();

        /**
        start the thread running. Called in the context of the spawning thread.
        */

        void start(QT3DSU32 stackSize);

        /**
        Violently kill the current thread. Blunt instrument, not recommended since
        it can leave all kinds of things unreleased (stack, memory, mutexes...) Should
        be called in the context of the spawning thread.
        */

        void kill();

        /**
        The virtual execute() method is the user defined function that will
        run in the new thread. Called in the context of the spawned thread.
        */

        virtual void execute(void);

        /**
        stop the thread. Signals the spawned thread that it should stop, so the
        thread should check regularly
        */

        void signalQuit();

        /**
        Wait for a thread to stop. Should be called in the context of the spawning
        thread. Returns false if the thread has not been started.
        */

        bool waitForQuit();

        /**
        check whether the thread is signalled to quit. Called in the context of the
        spawned thread.
        */

        bool quitIsSignalled();

        /**
        Cleanly shut down this thread. Called in the context of the spawned thread.
        */
        void quit();

        /**
        Change the affinity mask for this thread.
        On Xbox360, sets the hardware thread to the first non-zero bit.

        Returns previous mask if successful, or zero on failure
        */
        virtual QT3DSU32 setAffinityMask(QT3DSU32 mask);

        static ThreadPriority::Enum getPriority(Id threadId);

        /** Set thread priority. */
        void setPriority(ThreadPriority::Enum prio);

        /** set the thread's name */
        void setName(const char *name);

        /** Put the current thread to sleep for the given number of milliseconds */
        static void sleep(QT3DSU32 ms);

        /** Yield the current thread's slot on the CPU */
        static void yield();

    private:
        class ThreadImpl *mImpl;
    };

    QT3DS_FOUNDATION_API QT3DSU32 TlsAlloc();
    QT3DS_FOUNDATION_API void TlsFree(QT3DSU32 index);
    QT3DS_FOUNDATION_API void *TlsGet(QT3DSU32 index);
    QT3DS_FOUNDATION_API QT3DSU32 TlsSet(QT3DSU32 index, void *value);

} // namespace foundation
} // namespace qt3ds

#endif
