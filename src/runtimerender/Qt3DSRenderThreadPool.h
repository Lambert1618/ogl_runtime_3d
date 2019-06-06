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
#pragma once
#ifndef QT3DS_RENDER_THREAD_POOL_H
#define QT3DS_RENDER_THREAD_POOL_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"

namespace qt3ds {
namespace render {

    typedef void (*TTaskFunction)(void *inUserData);

struct TaskStates
{
    enum Enum {
        UnknownTask = 0,
        Queued,
        Running,
    };
};


struct STask
{
    void *m_UserData;
    TTaskFunction m_Function;
    TTaskFunction m_CancelFunction;
    QT3DSU64 m_Id;
    TaskStates::Enum m_TaskState;
    STask *m_NextTask;
    STask *m_PreviousTask;

    STask(void *ud, TTaskFunction func, TTaskFunction cancelFunc, QT3DSU64 inId)
        : m_UserData(ud)
        , m_Function(func)
        , m_CancelFunction(cancelFunc)
        , m_Id(inId)
        , m_TaskState(TaskStates::Queued)
        , m_NextTask(NULL)
        , m_PreviousTask(NULL)
    {
    }
    STask()
        : m_UserData(NULL)
        , m_Function(NULL)
        , m_CancelFunction(NULL)
        , m_Id(0)
        , m_TaskState(TaskStates::UnknownTask)
        , m_NextTask(NULL)
        , m_PreviousTask(NULL)
    {
    }
    void CallFunction()
    {
        if (m_Function)
            m_Function(m_UserData);
    }
    void Cancel()
    {
        if (m_CancelFunction)
            m_CancelFunction(m_UserData);
    }
};


    struct CancelReturnValues
    {
        enum Enum {
            TaskCanceled = 0,
            TaskRunning,
            TaskNotFound,
        };
    };

    class IThreadPool : public NVRefCounted
    {
    protected:
        virtual ~IThreadPool() {}
    public:
        // Add a task to be run at some point in the future.
        // Tasks will be run roughly in order they are given.
        // The returned value is a handle that can be used to query
        // details about the task
        // Cancel function will be called if the thread pool is destroyed or
        // of the task gets canceled.
        virtual QT3DSU64 AddTask(void *inUserData, TTaskFunction inFunction,
                              TTaskFunction inCancelFunction) = 0;
        virtual TaskStates::Enum GetTaskState(QT3DSU64 inTaskId) = 0;
        virtual CancelReturnValues::Enum CancelTask(QT3DSU64 inTaskId) = 0;

        virtual STask GetNextTask() = 0;
        virtual void TaskFinished(QT3DSU64 inId) = 0;

        static IThreadPool &CreateThreadPool(NVFoundationBase &inFoundation,
                                             QT3DSU32 inNumThreads = 4);
    };
}
}
#endif
