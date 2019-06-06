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
#include "render/Qt3DSRenderDataBuffer.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSFoundation.h"

namespace qt3ds {
namespace render {

    NVRenderDataBuffer::NVRenderDataBuffer(NVRenderContextImpl &context, NVFoundationBase &fnd,
                                           size_t size, NVRenderBufferBindFlags bindFlags,
                                           NVRenderBufferUsageType::Enum usageType,
                                           NVDataRef<QT3DSU8> data)
        : m_Context(context)
        , m_Foundation(fnd)
        , mRefCount(0)
        , m_Backend(context.GetBackend())
        , m_UsageType(usageType)
        , m_BindFlags(bindFlags)
        , m_BufferData(data)
        , m_BufferCapacity(data.size())
        , m_BufferSize(size)
        , m_OwnsData(false)
        , m_Mapped(false)
    {
        m_BufferHandle =
            m_Backend->CreateBuffer(size, bindFlags, usageType, (const void *)m_BufferData.begin());
    }

    NVRenderDataBuffer::~NVRenderDataBuffer()
    {
        if (m_BufferHandle) {
            m_Backend->ReleaseBuffer(m_BufferHandle);
        }
        m_BufferHandle = 0;

        releaseMemory();
    }

    void NVRenderDataBuffer::releaseMemory()
    {
        // chekc if we should release memory
        if (m_BufferData.size() && m_OwnsData) {
            m_Foundation.getAllocator().deallocate(m_BufferData.begin());
        }

        m_BufferData = NVDataRef<QT3DSU8>();
        m_OwnsData = false;
    }

    NVDataRef<QT3DSU8> NVRenderDataBuffer::MapBuffer()
    {
        // don't map twice
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to map a mapped buffer");
            QT3DS_ASSERT(false);
        }

        QT3DSU8 *pData = (QT3DSU8 *)m_Backend->MapBuffer(
            m_BufferHandle, m_BindFlags, 0, m_BufferSize,
            NVRenderBufferAccessFlags(NVRenderBufferAccessTypeValues::Read
                                      | NVRenderBufferAccessTypeValues::Write));

        releaseMemory();
        m_BufferData = toDataRef(const_cast<QT3DSU8 *>(pData), (QT3DSU32)m_BufferSize);
        m_BufferCapacity = (QT3DSU32)m_BufferSize;
        m_OwnsData = false;

        // currently we return a reference to the system memory
        m_Mapped = true;
        return m_BufferData;
    }

    NVDataRef<QT3DSU8> NVRenderDataBuffer::MapBufferRange(size_t offset, size_t size,
                                                       NVRenderBufferAccessFlags flags)
    {
        // don't map twice
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to map a mapped buffer");
            QT3DS_ASSERT(false);
        }
        // don't map out of range
        if ((m_BufferSize < (offset + size)) || (size == 0)) {
            qCCritical(INVALID_OPERATION, "Attempting to map out of buffer range");
            QT3DS_ASSERT(false);
        }

        QT3DSU8 *pData =
            (QT3DSU8 *)m_Backend->MapBuffer(m_BufferHandle, m_BindFlags, offset, size, flags);

        releaseMemory();
        m_BufferData = toDataRef(const_cast<QT3DSU8 *>(pData), (QT3DSU32)size);
        m_BufferCapacity = (QT3DSU32)size;
        m_OwnsData = false;

        // currently we return a reference to the system memory
        m_Mapped = true;
        return m_BufferData;
    }

    void NVRenderDataBuffer::UnmapBuffer()
    {
        if (m_Mapped) {
            // update hardware
            m_Backend->UnmapBuffer(m_BufferHandle, m_BindFlags);
            m_Mapped = false;
            releaseMemory();
        }
    }

    void NVRenderDataBuffer::UpdateBuffer(NVConstDataRef<QT3DSU8> data, bool ownsMemory)
    {
        // don't update a mapped buffer
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to update a mapped buffer");
            QT3DS_ASSERT(false);
        }

        releaseMemory();

        m_BufferData = toDataRef(const_cast<QT3DSU8 *>(data.begin()), data.size());
        m_BufferCapacity = data.mSize;
        m_OwnsData = ownsMemory;
        // update hardware
        m_Backend->UpdateBuffer(m_BufferHandle, m_BindFlags, m_BufferCapacity, m_UsageType,
                                (const void *)m_BufferData.begin());
    }
}
}
