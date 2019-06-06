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

#include "render/Qt3DSRenderIndexBuffer.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/StringTable.h"

namespace qt3ds {
namespace render {

    NVRenderIndexBuffer::NVRenderIndexBuffer(NVRenderContextImpl &context, size_t size,
                                             NVRenderComponentTypes::Enum componentType,
                                             NVRenderBufferUsageType::Enum usageType,
                                             NVDataRef<QT3DSU8> data)
        : NVRenderDataBuffer(context, context.GetFoundation(), size,
                             NVRenderBufferBindValues::Index, usageType, data)
        , m_ComponentType(componentType)
    {
    }

    NVRenderIndexBuffer::~NVRenderIndexBuffer() { m_Context.BufferDestroyed(*this); }

    QT3DSU32 NVRenderIndexBuffer::GetNumIndices() const
    {
        QT3DSU32 dtypeSize = NVRenderComponentTypes::getSizeofType(m_ComponentType);
        return m_BufferCapacity / dtypeSize;
    }

    void NVRenderIndexBuffer::Draw(NVRenderDrawMode::Enum drawMode, QT3DSU32 count, QT3DSU32 offset)
    {
        m_Backend->DrawIndexed(
            drawMode, count, m_ComponentType,
            (const void *)(offset * NVRenderComponentTypes::getSizeofType(m_ComponentType)));
    }

    void NVRenderIndexBuffer::DrawIndirect(NVRenderDrawMode::Enum drawMode, QT3DSU32 offset)
    {
        m_Backend->DrawIndexedIndirect(drawMode, m_ComponentType, (const void *)offset);
    }

    void NVRenderIndexBuffer::Bind()
    {
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to Bind a locked buffer");
            QT3DS_ASSERT(false);
        }

        m_Backend->BindBuffer(m_BufferHandle, m_BindFlags);
    }

    NVRenderIndexBuffer *NVRenderIndexBuffer::Create(NVRenderContextImpl &context,
                                                     NVRenderBufferUsageType::Enum usageType,
                                                     NVRenderComponentTypes::Enum componentType,
                                                     size_t size, NVConstDataRef<QT3DSU8> bufferData)
    {
        NVFoundationBase &fnd(context.GetFoundation());
        if (componentType != NVRenderComponentTypes::QT3DSU32
            && componentType != NVRenderComponentTypes::QT3DSU16
            && componentType != NVRenderComponentTypes::QT3DSU8) {
            qCCritical(INVALID_PARAMETER, "Invalid component type for index buffer");
            QT3DS_ASSERT(false);
            return NULL;
        }

        QT3DSU32 ibufSize = sizeof(NVRenderIndexBuffer);
        QT3DSU8 *baseMem = (QT3DSU8 *)QT3DS_ALLOC(fnd.getAllocator(), ibufSize, "IndexBuffer");
        NVRenderIndexBuffer *retval = new (baseMem) NVRenderIndexBuffer(
            context, size, componentType, usageType,
            toDataRef(const_cast<QT3DSU8 *>(bufferData.begin()), bufferData.size()));
        return retval;
    }
}
}
