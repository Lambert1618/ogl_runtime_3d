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

#include "render/Qt3DSRenderDrawIndirectBuffer.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderShaderProgram.h"

namespace qt3ds {
namespace render {

    NVRenderDrawIndirectBuffer::NVRenderDrawIndirectBuffer(NVRenderContextImpl &context,
                                                           size_t size,
                                                           NVRenderBufferUsageType::Enum usageType,
                                                           NVDataRef<QT3DSU8> data)
        : NVRenderDataBuffer(context, context.GetFoundation(), size,
                             NVRenderBufferBindValues::Draw_Indirect, usageType, data)
        , m_Dirty(true)
    {
    }

    NVRenderDrawIndirectBuffer::~NVRenderDrawIndirectBuffer() { m_Context.BufferDestroyed(*this); }

    void NVRenderDrawIndirectBuffer::Bind()
    {
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to Bind a locked buffer");
            QT3DS_ASSERT(false);
        }

        m_Backend->BindBuffer(m_BufferHandle, m_BindFlags);
    }

    void NVRenderDrawIndirectBuffer::Update()
    {
        // we only update the buffer if it is dirty and we actually have some data
        if (m_Dirty && m_BufferData.size()) {
            m_Backend->UpdateBuffer(m_BufferHandle, m_BindFlags, m_BufferData.size(), m_UsageType,
                                    m_BufferData.begin());
            m_Dirty = false;
        }
    }

    void NVRenderDrawIndirectBuffer::UpdateData(QT3DSI32 offset, NVDataRef<QT3DSU8> data)
    {
        // we only update the buffer if we something
        if (data.size())
            m_Backend->UpdateBuffer(m_BufferHandle, m_BindFlags, data.size(), m_UsageType,
                                    data.begin() + offset);
    }

    NVRenderDrawIndirectBuffer *
    NVRenderDrawIndirectBuffer::Create(NVRenderContextImpl &context,
                                       NVRenderBufferUsageType::Enum usageType, size_t size,
                                       NVConstDataRef<QT3DSU8> bufferData)
    {
        NVFoundationBase &fnd(context.GetFoundation());
        NVRenderDrawIndirectBuffer *retval = NULL;

        // these are the context flags which do not support this drawing mode
        NVRenderContextType noDrawIndirectSupported(
            NVRenderContextValues::GL2 | NVRenderContextValues::GLES2 | NVRenderContextValues::GL3
            | NVRenderContextValues::GLES3);
        NVRenderContextType ctxType = context.GetRenderContextType();

        if (!(ctxType & noDrawIndirectSupported)) {
            QT3DSU32 bufSize = sizeof(NVRenderDrawIndirectBuffer);
            QT3DSU8 *newMem = (QT3DSU8 *)QT3DS_ALLOC(fnd.getAllocator(), bufSize, "DrawIndirectBuffer");
            retval = new (newMem) NVRenderDrawIndirectBuffer(
                context, size, usageType,
                toDataRef(const_cast<QT3DSU8 *>(bufferData.begin()), bufferData.size()));
        } else {
            QT3DS_ASSERT(false);
        }
        return retval;
    }
}
}
