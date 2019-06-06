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
#include "Qt3DSRenderThreadPool.h"
#include "foundation/Qt3DSThread.h"
#include "EASTL/utility.h"
#include "EASTL/list.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"

using namespace qt3ds::render;

namespace {

struct STaskHeadOp
{
    STask *get(STask &inTask) { return inTask.m_PreviousTask; }
    void set(STask &inTask, STask *inItem) { inTask.m_PreviousTask = inItem; }
};

struct STaskTailOp
{
    STask *get(STask &inTask) { return inTask.m_NextTask; }
    void set(STask &inTask, STask *inItem) { inTask.m_NextTask = inItem; }
};

typedef InvasiveLinkedList<STask, STaskHeadOp, STaskTailOp> TTaskList;

struct SThreadPoolThread : public Thread
{
    IThreadPool *m_Mgr;
    SThreadPoolThread(NVFoundationBase &foundation, IThreadPool *inMgr)
        : Thread(foundation)
        , m_Mgr(inMgr)
    {
    }
    void execute(void) override
    {
        setName("Qt3DSRender Thread manager thread");
        while (!quitIsSignalled()) {
            STask task = m_Mgr->GetNextTask();
            if (task.m_Function) {
                task.CallFunction();
                m_Mgr->TaskFinished(task.m_Id);
            }
        }
        quit();
    }
};

struct SThreadPool : public IThreadPool
{
    typedef nvhash_map<QT3DSU64, STask *> TIdTaskMap;
    typedef Mutex::ScopedLock TLockType;
    typedef Pool<STask, ForwardingAllocator> TTaskPool;

    NVFoundationBase &m_Foundation;
    volatile QT3DSI32 mRefCount;
    nvvector<SThreadPoolThread *> m_Threads;
    TIdTaskMap m_Tasks;
    Sync m_TaskListEvent;
    volatile bool m_Running;
    Mutex m_TaskListMutex;
    TTaskPool m_TaskPool;
    TTaskList m_TaskList;

    QT3DSU64 m_NextId;

    SThreadPool(NVFoundationBase &inBase, QT3DSU32 inMaxThreads)
        : m_Foundation(inBase)
        , mRefCount(0)
        , m_Threads(inBase.getAllocator(), "SThreadPool::m_Threads")
        , m_Tasks(inBase.getAllocator(), "SThreadPool::m_Tasks")
        , m_TaskListEvent(inBase.getAllocator())
        , m_Running(true)
        , m_TaskListMutex(m_Foundation.getAllocator())
        , m_TaskPool(ForwardingAllocator(m_Foundation.getAllocator(), "SThreadPool::m_TaskPool"))
        , m_NextId(1)
    {
        // Fire up our little pools of chaos.
        for (QT3DSU32 idx = 0; idx < inMaxThreads; ++idx) {
            m_Threads.push_back(
                QT3DS_NEW(m_Foundation.getAllocator(), SThreadPoolThread)(m_Foundation, this));
            m_Threads.back()->start(Thread::DEFAULT_STACK_SIZE);
        }
    }

    void MutexHeldRemoveTaskFromList(STask *theTask)
    {
        if (theTask)
            m_TaskList.remove(*theTask);
        QT3DS_ASSERT(theTask->m_NextTask == NULL);
        QT3DS_ASSERT(theTask->m_PreviousTask == NULL);
    }

    STask *MutexHeldNextTask()
    {
        STask *theTask = m_TaskList.front_ptr();
        if (theTask) {
            MutexHeldRemoveTaskFromList(theTask);
        }
        if (theTask) {
            QT3DS_ASSERT(m_TaskList.m_Head != theTask);
            QT3DS_ASSERT(m_TaskList.m_Tail != theTask);
        }
        return theTask;
    }

    virtual ~SThreadPool()
    {
        m_Running = false;

        m_TaskListEvent.set();

        for (QT3DSU32 idx = 0, end = m_Threads.size(); idx < end; ++idx)
            m_Threads[idx]->signalQuit();

        for (QT3DSU32 idx = 0, end = m_Threads.size(); idx < end; ++idx) {
            m_Threads[idx]->waitForQuit();
            NVDelete(m_Foundation.getAllocator(), m_Threads[idx]);
        }

        m_Threads.clear();

        TLockType __listMutexLocker(m_TaskListMutex);

        for (STask *theTask = MutexHeldNextTask(); theTask; theTask = MutexHeldNextTask()) {
            theTask->Cancel();
        }

        m_Tasks.clear();
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void VerifyTaskList()
    {
        STask *theLastTask = NULL;
        for (STask *theTask = m_TaskList.m_Head; theTask; theTask = theTask->m_NextTask) {
            QT3DS_ASSERT(theTask->m_PreviousTask == theLastTask);
            theLastTask = theTask;
        }
        theLastTask = NULL;
        for (STask *theTask = m_TaskList.m_Tail; theTask; theTask = theTask->m_PreviousTask) {
            QT3DS_ASSERT(theTask->m_NextTask == theLastTask);
            theLastTask = theTask;
        }
    }

    QT3DSU64 AddTask(void *inUserData, TTaskFunction inFunction,
                          TTaskFunction inCancelFunction) override
    {
        if (inFunction && m_Running) {
            TLockType __listMutexLocker(m_TaskListMutex);
            QT3DSU64 taskId = m_NextId;
            ++m_NextId;

            STask *theTask = (STask *)m_TaskPool.allocate(__FILE__, __LINE__);
            new (theTask) STask(inUserData, inFunction, inCancelFunction, taskId);
            TIdTaskMap::iterator theTaskIter =
                m_Tasks.insert(eastl::make_pair(taskId, theTask)).first;

            m_TaskList.push_back(*theTask);
            QT3DS_ASSERT(m_TaskList.m_Tail == theTask);

#ifdef _DEBUG
            VerifyTaskList();
#endif
            m_TaskListEvent.set();
            m_TaskListEvent.reset();
            return taskId;
        }
        QT3DS_ASSERT(false);
        return 0;
    }

    TaskStates::Enum GetTaskState(QT3DSU64 inTaskId) override
    {
        TLockType __listMutexLocker(m_TaskListMutex);
        TIdTaskMap::iterator theTaskIter = m_Tasks.find(inTaskId);
        if (theTaskIter != m_Tasks.end())
            return theTaskIter->second->m_TaskState;
        return TaskStates::UnknownTask;
    }

    CancelReturnValues::Enum CancelTask(QT3DSU64 inTaskId) override
    {
        TLockType __listMutexLocker(m_TaskListMutex);
        TIdTaskMap::iterator theTaskIter = m_Tasks.find(inTaskId);
        if (theTaskIter == m_Tasks.end())
            return CancelReturnValues::TaskCanceled;
        if (theTaskIter->second->m_TaskState == TaskStates::Running)
            return CancelReturnValues::TaskRunning;

        STask *theTask = theTaskIter->second;
        theTask->Cancel();
        MutexHeldRemoveTaskFromList(theTask);
        m_Tasks.erase(inTaskId);
        m_TaskPool.deallocate(theTask);

        return CancelReturnValues::TaskCanceled;
    }

    STask GetNextTask() override
    {

        if (m_Running) {
            {
                TLockType __listMutexLocker(m_TaskListMutex);
                STask *retval = MutexHeldNextTask();
                if (retval)
                    return *retval;
            }
            // If we couldn't get a task then wait.
            m_TaskListEvent.wait(1000);
        }
        return STask();
    }

    void TaskFinished(QT3DSU64 inId) override
    {
        TLockType __listMutexLocker(m_TaskListMutex);
        TIdTaskMap::iterator theTaskIter = m_Tasks.find(inId);
        if (theTaskIter == m_Tasks.end()) {
            QT3DS_ASSERT(false);
            return;
        }

        STask *theTask(theTaskIter->second);

#ifdef _DEBUG
        QT3DS_ASSERT(theTask->m_NextTask == NULL);
        QT3DS_ASSERT(theTask->m_PreviousTask == NULL);
#endif
        m_TaskPool.deallocate(theTask);
        m_Tasks.erase(inId);
        return;
    }
};
}

IThreadPool &IThreadPool::CreateThreadPool(NVFoundationBase &inFoundation, QT3DSU32 inNumThreads)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SThreadPool)(inFoundation, inNumThreads);
}
