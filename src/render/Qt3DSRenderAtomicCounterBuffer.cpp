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

#include "render/Qt3DSRenderAtomicCounterBuffer.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderShaderProgram.h"

namespace qt3ds {
namespace render {

    ///< struct handling a constant buffer entry
    class AtomicCounterBufferEntry
    {
    public:
        CRegisteredString m_Name; ///< parameter Name
        QT3DSI32 m_Offset; ///< offset into the memory buffer

        AtomicCounterBufferEntry(CRegisteredString name, QT3DSI32 offset)
            : m_Name(name)
            , m_Offset(offset)
        {
        }
    };

    NVRenderAtomicCounterBuffer::NVRenderAtomicCounterBuffer(
        NVRenderContextImpl &context, CRegisteredString bufferName, size_t size,
        NVRenderBufferUsageType::Enum usageType, NVDataRef<QT3DSU8> data)
        : NVRenderDataBuffer(context, context.GetFoundation(), size,
                             NVRenderBufferBindValues::Storage, usageType, data)
        , m_Name(bufferName)
        , m_AtomicCounterBufferEntryMap(
              m_Foundation.getAllocator(),
              "NVRenderAtomicCounterBuffer::m_AtomicCounterBufferEntryMap")
        , m_Dirty(true)
    {
        QT3DS_ASSERT(context.IsStorageBufferSupported());
    }

    NVRenderAtomicCounterBuffer::~NVRenderAtomicCounterBuffer()
    {
        for (TRenderAtomiCounterBufferEntryMap::iterator
                 iter = m_AtomicCounterBufferEntryMap.begin(),
                 end = m_AtomicCounterBufferEntryMap.end();
             iter != end; ++iter) {
            NVDelete(m_Foundation.getAllocator(), iter->second);
        }

        m_AtomicCounterBufferEntryMap.clear();

        m_Context.BufferDestroyed(*this);
    }

    void NVRenderAtomicCounterBuffer::Bind()
    {
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to Bind a locked buffer");
            QT3DS_ASSERT(false);
        }

        m_Backend->BindBuffer(m_BufferHandle, m_BindFlags);
    }

    void NVRenderAtomicCounterBuffer::BindToShaderProgram(QT3DSU32 index)
    {
        m_Backend->ProgramSetAtomicCounterBuffer(index, m_BufferHandle);
    }

    void NVRenderAtomicCounterBuffer::Update()
    {
        // we only update the buffer if it is dirty and we actually have some data
        if (m_Dirty && m_BufferData.size()) {
            m_Backend->UpdateBuffer(m_BufferHandle, m_BindFlags, m_BufferData.size(), m_UsageType,
                                    m_BufferData.begin());
            m_Dirty = false;
        }
    }

    void NVRenderAtomicCounterBuffer::UpdateData(QT3DSI32 offset, NVDataRef<QT3DSU8> data)
    {
        // we only update the buffer if we something
        if (data.size())
            m_Backend->UpdateBuffer(m_BufferHandle, m_BindFlags, data.size(), m_UsageType,
                                    data.begin() + offset);
    }

    void NVRenderAtomicCounterBuffer::AddParam(CRegisteredString name, QT3DSU32 offset)
    {
        if (m_AtomicCounterBufferEntryMap.find(name) == m_AtomicCounterBufferEntryMap.end()) {
            AtomicCounterBufferEntry *newEntry =
                QT3DS_NEW(m_Foundation.getAllocator(), AtomicCounterBufferEntry)(name, offset);

            if (newEntry)
                m_AtomicCounterBufferEntryMap.insert(eastl::make_pair(name, newEntry));
        } else {
            // no duplicated entries
            return;
        }
    }

    bool NVRenderAtomicCounterBuffer::ContainsParam(CRegisteredString name)
    {
        if (m_AtomicCounterBufferEntryMap.find(name) != m_AtomicCounterBufferEntryMap.end())
            return true;
        else
            return false;
    }

    NVRenderAtomicCounterBuffer *
    NVRenderAtomicCounterBuffer::Create(NVRenderContextImpl &context, const char *bufferName,
                                        NVRenderBufferUsageType::Enum usageType, size_t size,
                                        NVConstDataRef<QT3DSU8> bufferData)
    {
        NVFoundationBase &fnd(context.GetFoundation());
        NVRenderAtomicCounterBuffer *retval = NULL;

        if (context.IsAtomicCounterBufferSupported()) {
            CRegisteredString theBufferName(context.GetStringTable().RegisterStr(bufferName));
            QT3DSU32 bufSize = sizeof(NVRenderAtomicCounterBuffer);
            QT3DSU8 *newMem = (QT3DSU8 *)QT3DS_ALLOC(fnd.getAllocator(), bufSize, "AtomicCounterBuffer");
            retval = new (newMem) NVRenderAtomicCounterBuffer(
                context, theBufferName, size, usageType,
                toDataRef(const_cast<QT3DSU8 *>(bufferData.begin()), bufferData.size()));
        } else {
            QT3DS_ASSERT(false);
        }
        return retval;
    }
}
}
