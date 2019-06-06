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

#include "render/Qt3DSRenderStorageBuffer.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderShaderProgram.h"

namespace qt3ds {
namespace render {

    NVRenderStorageBuffer::NVRenderStorageBuffer(NVRenderContextImpl &context,
                                                 CRegisteredString bufferName, size_t size,
                                                 NVRenderBufferUsageType::Enum usageType,
                                                 NVDataRef<QT3DSU8> data, NVRenderDataBuffer *pBuffer)
        : NVRenderDataBuffer(context, context.GetFoundation(), size,
                             NVRenderBufferBindValues::Storage, usageType, data)
        , m_Name(bufferName)
        , m_WrappedBuffer(pBuffer)
        , m_Dirty(true)
    {
        QT3DS_ASSERT(context.IsStorageBufferSupported());

        if (pBuffer)
            pBuffer->addRef();
    }

    NVRenderStorageBuffer::~NVRenderStorageBuffer()
    {
        if (m_WrappedBuffer)
            m_WrappedBuffer->release();

        m_Context.BufferDestroyed(*this);
    }

    void NVRenderStorageBuffer::Bind()
    {
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to Bind a locked buffer");
            QT3DS_ASSERT(false);
        }

        if (m_WrappedBuffer)
            m_WrappedBuffer->Bind();
        else
            m_Backend->BindBuffer(m_BufferHandle, m_BindFlags);
    }

    void NVRenderStorageBuffer::BindToShaderProgram(QT3DSU32 index)
    {
        m_Backend->ProgramSetStorageBuffer(
            index, (m_WrappedBuffer) ? m_WrappedBuffer->GetBuffertHandle() : m_BufferHandle);
    }

    void NVRenderStorageBuffer::Update()
    {
        // we only update the buffer if it is dirty and we actually have some data
        if (m_Dirty && m_BufferData.size()) {
            m_Backend->UpdateBuffer(m_BufferHandle, m_BindFlags, m_BufferData.size(), m_UsageType,
                                    m_BufferData.begin());
            m_Dirty = false;
        }
    }

    void NVRenderStorageBuffer::UpdateData(QT3DSI32 offset, NVDataRef<QT3DSU8> data)
    {
        // we only update the buffer if it is not just a wrapper
        if (!m_WrappedBuffer)
            m_Backend->UpdateBuffer(m_BufferHandle, m_BindFlags, data.size(), m_UsageType,
                                    data.begin() + offset);
    }

    NVRenderStorageBuffer *
    NVRenderStorageBuffer::Create(NVRenderContextImpl &context, const char *bufferName,
                                  NVRenderBufferUsageType::Enum usageType, size_t size,
                                  NVConstDataRef<QT3DSU8> bufferData, NVRenderDataBuffer *pBuffer)
    {
        NVFoundationBase &fnd(context.GetFoundation());
        NVRenderStorageBuffer *retval = NULL;

        if (context.IsStorageBufferSupported()) {
            CRegisteredString theBufferName(context.GetStringTable().RegisterStr(bufferName));
            QT3DSU32 cbufSize = sizeof(NVRenderStorageBuffer);
            QT3DSU8 *newMem = (QT3DSU8 *)QT3DS_ALLOC(fnd.getAllocator(), cbufSize, "StorageBuffer");
            retval = new (newMem) NVRenderStorageBuffer(
                context, theBufferName, size, usageType,
                toDataRef(const_cast<QT3DSU8 *>(bufferData.begin()), bufferData.size()), pBuffer);
        } else {
            QString errorMsg = QObject::tr("Shader storage buffers are not supported: %1")
                    .arg(bufferName);
            qCCritical(INVALID_OPERATION) << errorMsg;
            QT3DS_ALWAYS_ASSERT_MESSAGE(errorMsg.toUtf8());
        }
        return retval;
    }
}
}
