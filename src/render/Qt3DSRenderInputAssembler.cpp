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

#include "render/Qt3DSRenderInputAssembler.h"
#include "render/Qt3DSRenderAttribLayout.h"
#include "render/Qt3DSRenderContext.h"

namespace qt3ds {
namespace render {

    ///< constructor
    NVRenderInputAssembler::NVRenderInputAssembler(
        NVRenderContextImpl &context, NVRenderAttribLayout *attribLayout,
        NVConstDataRef<NVRenderVertexBuffer *> buffers, const NVRenderIndexBuffer *indexBuffer,
        NVConstDataRef<QT3DSU32> strides, NVConstDataRef<QT3DSU32> offsets,
        NVRenderDrawMode::Enum primType, QT3DSU32 patchVertexCount)
        : m_Context(context)
        , m_Foundation(context.GetFoundation())
        , mRefCount(0)
        , m_Backend(context.GetBackend())
        , m_AttribLayout(attribLayout)
        , m_VertexBuffers(context.GetAllocator(), "m_VertexBuffers")
        , m_IndexBuffer(indexBuffer)
        , m_PrimitiveType(primType)
        , m_PatchVertexCount(patchVertexCount)
    {
        // we cannot currently attach more than 16  vertex buffers
        QT3DS_ASSERT(buffers.size() < 16);
        // if primitive is "Patch" we need a patch per vertex count > 0
        QT3DS_ASSERT(m_PrimitiveType != NVRenderDrawMode::Patches || m_PatchVertexCount > 1);

        QT3DSU32 entrySize = sizeof(NVRenderBackend::NVRenderBackendBufferObject) * buffers.size();
        NVRenderBackend::NVRenderBackendBufferObject *bufferHandle =
            (NVRenderBackend::NVRenderBackendBufferObject *)QT3DS_ALLOC(
                m_Foundation.getAllocator(), entrySize, "NVRenderInputAssembler");
        // setup vertex buffer backend handle array
        QT3DS_FOREACH(idx, buffers.size())
        {
            m_VertexBuffers.push_back(buffers.mData[idx]);
            bufferHandle[idx] = buffers.mData[idx]->GetBuffertHandle();
        };

        m_VertexbufferHandles = toConstDataRef(bufferHandle, buffers.size());

        m_InputAssemblertHandle = m_Backend->CreateInputAssembler(
            m_AttribLayout->GetAttribLayoutHandle(), m_VertexbufferHandles,
            (m_IndexBuffer) ? m_IndexBuffer->GetBuffertHandle() : NULL, strides, offsets,
            patchVertexCount);

        attribLayout->addRef();
    }

    ///< destructor
    NVRenderInputAssembler::~NVRenderInputAssembler()
    {
        m_AttribLayout->release();

        if (m_InputAssemblertHandle) {
            m_Backend->ReleaseInputAssembler(m_InputAssemblertHandle);
        }

        QT3DS_FREE(m_Foundation.getAllocator(), (void *)m_VertexbufferHandles.mData);
    }

    QT3DSU32 NVRenderInputAssembler::GetIndexCount() const
    {
        return (m_IndexBuffer) ? m_IndexBuffer->GetNumIndices() : 0;
    }

    QT3DSU32 NVRenderInputAssembler::GetVertexCount() const
    {
        // makes only sense if we have a single vertex buffer
        QT3DS_ASSERT(m_VertexBuffers.size() == 1);

        return m_VertexBuffers[0]->GetNumVertexes();
    }
}
}
