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

#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderSync.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSFoundation.h"

namespace qt3ds {
namespace render {

    NVRenderSync::NVRenderSync(NVRenderContextImpl &context, NVFoundationBase &fnd)
        : m_Context(context)
        , m_Foundation(fnd)
        , mRefCount(0)
        , m_Backend(context.GetBackend())
        , m_SyncHandle(NULL)
    {
    }

    NVRenderSync::~NVRenderSync()
    {
        if (m_SyncHandle)
            m_Backend->ReleaseSync(m_SyncHandle);
    }

    void NVRenderSync::Sync()
    {
        // On every sync call we need to create a new sync object
        // A sync object can only be used once

        // First delete the old object
        // We can safely do this because it is actually not deleted until
        // it is unused
        if (m_SyncHandle)
            m_Backend->ReleaseSync(m_SyncHandle);

        m_SyncHandle =
            m_Backend->CreateSync(NVRenderSyncType::GpuCommandsComplete, NVRenderSyncFlags());
    }

    void NVRenderSync::Wait()
    {
        // wait until the sync object is signaled or a timeout happens
        if (m_SyncHandle)
            m_Backend->WaitSync(m_SyncHandle, NVRenderCommandFlushFlags(), 0);
    }

    NVRenderSync *NVRenderSync::Create(NVRenderContextImpl &context)
    {
        if (!context.IsCommandSyncSupported())
            return NULL;

        NVRenderSync *retval = QT3DS_NEW(context.GetFoundation().getAllocator(),
                                      NVRenderSync)(context, context.GetFoundation());

        return retval;
    }
}
}
