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

#include "render/Qt3DSRenderVertexBuffer.h"
#include "render/Qt3DSRenderContext.h"

namespace qt3ds {
namespace render {

    NVRenderVertexBuffer::NVRenderVertexBuffer(NVRenderContextImpl &context, size_t size,
                                               QT3DSU32 stride, NVRenderBufferBindFlags bindFlags,
                                               NVRenderBufferUsageType::Enum usageType,
                                               NVDataRef<QT3DSU8> data)
        : NVRenderDataBuffer(context, context.GetFoundation(), size, bindFlags, usageType, data)
        , m_Stride(stride)
    {
        QT3DS_ASSERT(m_Stride);
    }

    NVRenderVertexBuffer::~NVRenderVertexBuffer() { m_Context.BufferDestroyed(*this); }

    void NVRenderVertexBuffer::Bind()
    {
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to Bind a locked buffer");
            QT3DS_ASSERT(false);
        }

        m_Backend->BindBuffer(m_BufferHandle, m_BindFlags);
    }

    NVRenderVertexBuffer *NVRenderVertexBuffer::Create(NVRenderContextImpl &context,
                                                       NVRenderBufferUsageType::Enum usageType,
                                                       size_t size, QT3DSU32 stride,
                                                       NVConstDataRef<QT3DSU8> bufferData)
    {
        NVFoundationBase &fnd(context.GetFoundation());

        QT3DSU32 vbufSize = sizeof(NVRenderVertexBuffer);
        QT3DSU8 *newMem = (QT3DSU8 *)QT3DS_ALLOC(fnd.getAllocator(), vbufSize, "VertexBuffer");
        NVRenderVertexBuffer *retval = new (newMem) NVRenderVertexBuffer(
            context, size, stride, NVRenderBufferBindValues::Vertex, usageType,
            toDataRef(const_cast<QT3DSU8 *>(bufferData.begin()), bufferData.size()));
        return retval;
    }
}
}
