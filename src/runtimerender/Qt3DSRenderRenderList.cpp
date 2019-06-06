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
#include "Qt3DSRenderRenderList.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSContainers.h"
#include "render/Qt3DSRenderBaseTypes.h"

using namespace qt3ds::render;

namespace {

struct SRenderList : public IRenderList
{
    typedef eastl::pair<QT3DSU32, IRenderTask *> TTaskIdTaskPair;
    typedef nvvector<TTaskIdTaskPair> TTaskList;

    NVFoundationBase &m_Foundation;
    TTaskList m_Tasks;
    TTaskList m_PersistentTasks;
    QT3DSU32 m_NextTaskId;
    QT3DSI32 mRefCount;
    bool m_ScissorEnabled;
    NVRenderRect m_ScissorRect;
    NVRenderRect m_Viewport;

    SRenderList(NVFoundationBase &fnd)
        : m_Foundation(fnd)
        , m_Tasks(fnd.getAllocator(), "m_Tasks")
        , m_PersistentTasks(fnd.getAllocator(), "m_PersistentTasks")
        , m_NextTaskId(1)
        , mRefCount(0)
        , m_ScissorEnabled(false)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void BeginFrame() override
    {
        m_NextTaskId = 1;
        m_Tasks.clear();
    }

    QT3DSU32 AddRenderTask(IRenderTask &inTask) override
    {
        QT3DSU32 taskId = m_NextTaskId;
        if (inTask.persistent()) {
            m_PersistentTasks.push_back(eastl::make_pair(0, &inTask));
            taskId = 0;
        } else {
            QT3DSU32 taskId = m_NextTaskId;
            ++m_NextTaskId;
            m_Tasks.push_back(eastl::make_pair(taskId, &inTask));
        }
        return taskId;
    }

    void DiscardRenderTask(QT3DSU32 inTaskId) override
    {
        TTaskList::iterator iter, end;
        for (iter = m_Tasks.begin(), end = m_Tasks.end(); iter != end && iter->first != inTaskId;
             ++iter) {
        }
        if (iter != end)
            m_Tasks.erase(iter);
    }
    // This runs through the added tasks in reverse order.  This is used to render dependencies
    // before rendering to the main render target.
    void RunRenderTasks() override
    {
        for (TTaskList::reverse_iterator iter = m_PersistentTasks.rbegin(),
             end = m_PersistentTasks.rend(); iter != end;
             ++iter) {
            iter->second->Run();
        }
        for (TTaskList::reverse_iterator iter = m_Tasks.rbegin(), end = m_Tasks.rend(); iter != end;
             ++iter) {
            iter->second->Run();
        }
        BeginFrame();
    }

    void SetScissorTestEnabled(bool enabled) override { m_ScissorEnabled = enabled; }
    void SetScissorRect(NVRenderRect rect) override { m_ScissorRect = rect; }
    void SetViewport(NVRenderRect rect) override { m_Viewport = rect; }
    bool IsScissorTestEnabled() const override { return m_ScissorEnabled; }
    NVRenderRect GetScissor() const override { return m_ScissorRect; }
    NVRenderRect GetViewport() const override { return m_Viewport; }
};
}

IRenderList &IRenderList::CreateRenderList(NVFoundationBase &fnd)
{
    return *QT3DS_NEW(fnd.getAllocator(), SRenderList)(fnd);
}
