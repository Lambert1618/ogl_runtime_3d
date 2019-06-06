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

#include "render/Qt3DSRenderConstantBuffer.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderShaderProgram.h"

namespace qt3ds {
namespace render {

    ///< struct handling a constant buffer entry
    class ConstantBufferParamEntry
    {
    public:
        CRegisteredString m_Name; ///< parameter Name
        NVRenderShaderDataTypes::Enum m_Type; ///< parameter type
        QT3DSI32 m_Count; ///< one or array size
        QT3DSI32 m_Offset; ///< offset into the memory buffer

        ConstantBufferParamEntry(CRegisteredString name, NVRenderShaderDataTypes::Enum type,
                                 QT3DSI32 count, QT3DSI32 offset)
            : m_Name(name)
            , m_Type(type)
            , m_Count(count)
            , m_Offset(offset)
        {
        }
    };

    NVRenderConstantBuffer::NVRenderConstantBuffer(NVRenderContextImpl &context,
                                                   CRegisteredString bufferName, size_t size,
                                                   NVRenderBufferUsageType::Enum usageType,
                                                   NVDataRef<QT3DSU8> data)
        : NVRenderDataBuffer(context, context.GetFoundation(), size,
                             NVRenderBufferBindValues::Constant, usageType, NVDataRef<QT3DSU8>())
        , m_Name(bufferName)
        , m_ConstantBufferEntryMap(m_Foundation.getAllocator(),
                                   "NVRenderConstantBuffer::m_ConstantBufferEntryMap")
        , m_CurrentOffset(0)
        , m_CurrentSize(0)
        , m_HWBufferInitialized(false)
        , m_Dirty(true)
        , m_RangeStart(0)
        , m_RangeEnd(0)
        , m_MaxBlockSize(0)
    {
        QT3DS_ASSERT(context.GetConstantBufferSupport());

        m_Backend->GetRenderBackendValue(
            NVRenderBackend::NVRenderBackendQuery::MaxConstantBufferBlockSize, &m_MaxBlockSize);

        if (size && data.size() && size == data.size()) {
            QT3DS_ASSERT(size < (QT3DSU32)m_MaxBlockSize);
            if (allocateShadowBuffer(data.size())) {
                memcpy(m_ShadowCopy.begin(), data.begin(), data.size());
            }
        }
    }

    NVRenderConstantBuffer::~NVRenderConstantBuffer()
    {
        // check if we should release memory
        if (m_ShadowCopy.size()) {
            m_Foundation.getAllocator().deallocate(m_ShadowCopy.begin());
        }

        m_ShadowCopy = NVDataRef<QT3DSU8>();

        for (TRenderConstantBufferEntryMap::iterator iter = m_ConstantBufferEntryMap.begin(),
                                                     end = m_ConstantBufferEntryMap.end();
             iter != end; ++iter) {
            NVDelete(m_Foundation.getAllocator(), iter->second);
        }

        m_ConstantBufferEntryMap.clear();

        m_Context.BufferDestroyed(*this);
    }

    void NVRenderConstantBuffer::Bind()
    {
        if (m_Mapped) {
            qCCritical(INVALID_OPERATION, "Attempting to Bind a locked buffer");
            QT3DS_ASSERT(false);
        }

        m_Backend->BindBuffer(m_BufferHandle, m_BindFlags);
    }

    void NVRenderConstantBuffer::BindToShaderProgram(NVRenderShaderProgram *inShader,
                                                     QT3DSU32 blockIndex, QT3DSU32 binding)
    {
        if ((QT3DSI32)binding == -1) {
            binding = m_Context.GetNextConstantBufferUnit();
            m_Backend->ProgramSetConstantBlock(inShader->GetShaderProgramHandle(), blockIndex,
                                               binding);
        }

        m_Backend->ProgramSetConstantBuffer(binding, m_BufferHandle);
    }

    bool NVRenderConstantBuffer::SetupBuffer(NVRenderShaderProgram *pProgram, QT3DSI32 index,
                                             QT3DSI32 bufSize, QT3DSI32 paramCount)
    {
        bool bSuccess = false;

        if (!m_HWBufferInitialized) {
            // allocate shadow buffer
            QT3DSU8 *newMem = (QT3DSU8 *)m_Foundation.getAllocator().allocate(
                bufSize, "NVRenderConstantBuffer", __FILE__, __LINE__);
            if (!newMem)
                return false;

            // allocate temp buffers to hold constant buffer information
            QT3DSI32 *theIndices = NULL, *theTypes = NULL, *theSizes = NULL, *theOffsets = NULL;
            theIndices = (QT3DSI32 *)QT3DS_ALLOC(m_Foundation.getAllocator(), paramCount * sizeof(QT3DSI32),
                                           "NVRenderConstantBuffer");
            if (!theIndices)
                goto fail;
            theTypes = (QT3DSI32 *)QT3DS_ALLOC(m_Foundation.getAllocator(), paramCount * sizeof(QT3DSI32),
                                         "NVRenderConstantBuffer");
            if (!theTypes)
                goto fail;
            theSizes = (QT3DSI32 *)QT3DS_ALLOC(m_Foundation.getAllocator(), paramCount * sizeof(QT3DSI32),
                                         "NVRenderConstantBuffer");
            if (!theSizes)
                goto fail;
            theOffsets = (QT3DSI32 *)QT3DS_ALLOC(m_Foundation.getAllocator(), paramCount * sizeof(QT3DSI32),
                                           "NVRenderConstantBuffer");
            if (!theOffsets)
                goto fail;

            bSuccess = true;

            // get indices for the individal constant buffer entries
            m_Backend->GetConstantBufferParamIndices(pProgram->GetShaderProgramHandle(), index,
                                                     theIndices);

            // get constant buffer uniform information
            m_Backend->GetConstantBufferParamInfoByIndices(pProgram->GetShaderProgramHandle(),
                                                           paramCount, (QT3DSU32 *)theIndices,
                                                           theTypes, theSizes, theOffsets);

            // get the names of the uniforms
            char nameBuf[512];
            QT3DSI32 elementCount, binding;
            NVRenderShaderDataTypes::Enum type;

            QT3DS_FOREACH(idx, paramCount)
            {
                m_Backend->GetConstantInfoByID(pProgram->GetShaderProgramHandle(), theIndices[idx],
                                               512, &elementCount, &type, &binding, nameBuf);
                // check if we already have this entry
                CRegisteredString theName(m_Context.GetStringTable().RegisterStr(nameBuf));
                TRenderConstantBufferEntryMap::iterator entry =
                    m_ConstantBufferEntryMap.find(theName);
                if (entry != m_ConstantBufferEntryMap.end()) {
                    ConstantBufferParamEntry *pParam = entry->second;
                    // copy content
                    if (m_ShadowCopy.size())
                        memcpy(newMem + theOffsets[idx],
                               m_ShadowCopy.begin() + entry->second->m_Offset,
                               entry->second->m_Count * getUniformTypeSize(pParam->m_Type));

                    pParam->m_Offset = theOffsets[idx];
                    QT3DS_ASSERT(type == pParam->m_Type);
                    QT3DS_ASSERT(elementCount == pParam->m_Count);
                } else {
                    // create one
                    m_ConstantBufferEntryMap.insert(eastl::make_pair(
                        theName,
                        createParamEntry(theName, (NVRenderShaderDataTypes::Enum)theTypes[idx],
                                         theSizes[idx], theOffsets[idx])));
                }
            }

            // release previous one
            if (m_ShadowCopy.size()) {
                m_Foundation.getAllocator().deallocate(m_ShadowCopy.begin());
            }
            // set new one
            m_ShadowCopy = NVDataRef<QT3DSU8>(newMem, bufSize);

            m_HWBufferInitialized = true;

        fail:
            if (theIndices)
                QT3DS_FREE(m_Foundation.getAllocator(), theIndices);
            if (theTypes)
                QT3DS_FREE(m_Foundation.getAllocator(), theTypes);
            if (theSizes)
                QT3DS_FREE(m_Foundation.getAllocator(), theSizes);
            if (theOffsets)
                QT3DS_FREE(m_Foundation.getAllocator(), theOffsets);

        } else {
            // some sanity checks
            bSuccess = true;
            bSuccess &= (m_ShadowCopy.size() <= (QT3DSU32)bufSize);
        }

        return bSuccess;
    }

    void NVRenderConstantBuffer::Update()
    {
        // we only update the buffer if the buffer is already on hardware
        // and if it is dirty
        if (m_Dirty && m_HWBufferInitialized) {
            if (m_RangeEnd == 0)
                m_Backend->UpdateBuffer(m_BufferHandle, m_BindFlags, m_ShadowCopy.size(),
                                        m_UsageType, m_ShadowCopy.begin());
            else
                UpdateRange();

            m_Dirty = false;
            m_RangeStart = m_RangeEnd = 0;
        }
    }

    void NVRenderConstantBuffer::UpdateRange()
    {
        if ((m_RangeStart + m_RangeEnd) > m_ShadowCopy.size()) {
            QT3DS_ASSERT(false);
            return;
        }

        m_Backend->UpdateBufferRange(m_BufferHandle, m_BindFlags, m_RangeStart,
                                     m_RangeEnd - m_RangeStart,
                                     m_ShadowCopy.begin() + m_RangeStart);
    }

    void NVRenderConstantBuffer::AddParam(CRegisteredString name,
                                          NVRenderShaderDataTypes::Enum type, QT3DSI32 count)
    {
        if (m_ConstantBufferEntryMap.find(name) == m_ConstantBufferEntryMap.end()) {
            ConstantBufferParamEntry *newEntry =
                QT3DS_NEW(m_Foundation.getAllocator(), ConstantBufferParamEntry)(name, type, count,
                                                                              m_CurrentOffset);

            if (newEntry)
                m_ConstantBufferEntryMap.insert(eastl::make_pair(name, newEntry));
        } else {
            // no duplicated entries
            return;
        }

        // compute new current buffer size and offset
        QT3DSI32 constantSize = getUniformTypeSize(type) * count;
        m_CurrentSize += constantSize;
        m_CurrentOffset += constantSize;
    }

    void NVRenderConstantBuffer::UpdateParam(const char *inName, NVDataRef<QT3DSU8> value)
    {
        // allocate space if not done yet
        // NOTE this gets reallocated once we get the real constant buffer size from a program
        if (!m_ShadowCopy.size()) {
            // allocate shadow buffer
            if (!allocateShadowBuffer(m_CurrentSize))
                return;
        }

        CRegisteredString theName(m_Context.GetStringTable().RegisterStr(inName));
        TRenderConstantBufferEntryMap::iterator entry = m_ConstantBufferEntryMap.find(theName);
        if (entry != m_ConstantBufferEntryMap.end()) {
            if (!memcmp(m_ShadowCopy.begin() + entry->second->m_Offset, value.begin(),
                        entry->second->m_Count * getUniformTypeSize(entry->second->m_Type))) {
                return;
            }
            memcpy(m_ShadowCopy.begin() + entry->second->m_Offset, value.begin(),
                   entry->second->m_Count * getUniformTypeSize(entry->second->m_Type));
            m_Dirty = true;
        }
    }

    void NVRenderConstantBuffer::UpdateRaw(QT3DSI32 offset, NVDataRef<QT3DSU8> data)
    {
        // allocate space if yet done
        if (!m_ShadowCopy.size()) {
            // allocate shadow buffer
            if (!allocateShadowBuffer(data.size()))
                return;
        }

        QT3DS_ASSERT((offset + data.size()) < (QT3DSU32)m_MaxBlockSize);

        // we do not initialize anything when this is used
        m_HWBufferInitialized = true;

        // we do not allow resize once allocated
        if ((offset + data.size()) > m_ShadowCopy.size())
            return;

        // copy data
        if (!memcmp(m_ShadowCopy.begin() + offset, data.begin(), data.size())) {
            return;
        }
        memcpy(m_ShadowCopy.begin() + offset, data.begin(), data.size());

        // update start
        m_RangeStart = (m_Dirty) ? (m_RangeStart > (QT3DSU32)offset) ? offset : m_RangeStart : offset;
        m_RangeEnd = (offset + data.size() > m_RangeEnd) ? offset + data.size() : m_RangeEnd;

        m_Dirty = true;
    }

    ConstantBufferParamEntry *NVRenderConstantBuffer::createParamEntry(
        CRegisteredString name, NVRenderShaderDataTypes::Enum type, QT3DSI32 count, QT3DSI32 offset)
    {
        ConstantBufferParamEntry *newEntry = QT3DS_NEW(
            m_Foundation.getAllocator(), ConstantBufferParamEntry)(name, type, count, offset);

        return newEntry;
    }

    QT3DSI32
    NVRenderConstantBuffer::getUniformTypeSize(NVRenderShaderDataTypes::Enum type)
    {
        switch (type) {
        case NVRenderShaderDataTypes::QT3DSF32:
            return sizeof(QT3DSF32);
        case NVRenderShaderDataTypes::QT3DSI32:
            return sizeof(QT3DSI32);
        case NVRenderShaderDataTypes::QT3DSI32_2:
            return sizeof(QT3DSI32) * 2;
        case NVRenderShaderDataTypes::QT3DSI32_3:
            return sizeof(QT3DSI32) * 3;
        case NVRenderShaderDataTypes::QT3DSI32_4:
            return sizeof(QT3DSI32) * 4;
        case NVRenderShaderDataTypes::QT3DSU32:
            return sizeof(QT3DSU32);
        case NVRenderShaderDataTypes::QT3DSU32_2:
            return sizeof(QT3DSU32) * 2;
        case NVRenderShaderDataTypes::QT3DSU32_3:
            return sizeof(QT3DSU32) * 3;
        case NVRenderShaderDataTypes::QT3DSU32_4:
            return sizeof(QT3DSU32) * 4;
        case NVRenderShaderDataTypes::QT3DSVec2:
            return sizeof(QT3DSF32) * 2;
        case NVRenderShaderDataTypes::QT3DSVec3:
            return sizeof(QT3DSF32) * 3;
        case NVRenderShaderDataTypes::QT3DSVec4:
            return sizeof(QT3DSF32) * 4;
        case NVRenderShaderDataTypes::QT3DSMat33:
            return sizeof(QT3DSF32) * 9;
        case NVRenderShaderDataTypes::QT3DSMat44:
            return sizeof(QT3DSF32) * 16;
        default:
            QT3DS_ASSERT(!"Unhandled type in NVRenderConstantBuffer::getUniformTypeSize");
            break;
        }

        return 0;
    }

    bool NVRenderConstantBuffer::allocateShadowBuffer(QT3DSU32 size)
    {
        // allocate shadow buffer
        QT3DSU8 *newMem = (QT3DSU8 *)m_Foundation.getAllocator().allocate(size, "NVRenderConstantBuffer",
                                                                    __FILE__, __LINE__);
        if (!newMem)
            return false;

        m_ShadowCopy = NVDataRef<QT3DSU8>(newMem, size);

        m_BufferCapacity = size;

        return true;
    }

    NVRenderConstantBuffer *NVRenderConstantBuffer::Create(NVRenderContextImpl &context,
                                                           const char *bufferName,
                                                           NVRenderBufferUsageType::Enum usageType,
                                                           size_t size,
                                                           NVConstDataRef<QT3DSU8> bufferData)
    {
        NVFoundationBase &fnd(context.GetFoundation());
        NVRenderConstantBuffer *retval = NULL;

        if (context.GetConstantBufferSupport()) {
            CRegisteredString theBufferName(context.GetStringTable().RegisterStr(bufferName));
            QT3DSU32 cbufSize = sizeof(NVRenderConstantBuffer);
            QT3DSU8 *newMem = (QT3DSU8 *)QT3DS_ALLOC(fnd.getAllocator(), cbufSize, "ConstantBuffer");
            retval = new (newMem) NVRenderConstantBuffer(
                context, theBufferName, size, usageType,
                toDataRef(const_cast<QT3DSU8 *>(bufferData.begin()), bufferData.size()));
        } else {
            QT3DS_ASSERT(false);
        }
        return retval;
    }
}
}
