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
#ifndef QT3DS_RENDER_SYNC_H
#define QT3DS_RENDER_SYNC_H
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
class NVFoundationBase;
}

namespace qt3ds {
namespace render {

    // forward declaration
    class NVRenderContextImpl;
    class NVRenderBackend;

    ///< Base class
    class NVRenderSync : public NVRefCounted
    {
    protected:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend
        NVRenderBackend::NVRenderBackendSyncObject m_SyncHandle; ///< opaque backend handle

    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] fnd			Pointer to foundation
         *
         * @return No return.
         */
        NVRenderSync(NVRenderContextImpl &context, NVFoundationBase &fnd);

        virtual ~NVRenderSync();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        /**
         * @brief Get sync type
         *
         * @return Return query type
         */
        virtual NVRenderSyncType::Enum GetSyncType() const
        {
            return NVRenderSyncType::GpuCommandsComplete;
        }

        /**
         * @brief Get a pointer to the foundation
         *
         * @return pointer to foundation
         */
        NVFoundationBase &GetFoundation() { return m_Foundation; }

        /**
         * @brief Create a sync object and place it in command stream.
         *		  Note every syncobject can only be used once.
         *		  This function creates a new sync object on ever call
         *		  and deletes the previous one
         *
         * @return no return.
         */
        virtual void Sync();

        /**
         * @brief Wait for a sync to be signaled
         *		  Note this blocks until the sync is signaled
         *
         * @return no return.
         */
        virtual void Wait();

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        virtual NVRenderBackend::NVRenderBackendSyncObject GetSyncHandle() const
        {
            return m_SyncHandle;
        }

        /*
         * @brief static creation function
         *
         * @return a sync object on success
         */
        static NVRenderSync *Create(NVRenderContextImpl &context);
    };
}
}

#endif
