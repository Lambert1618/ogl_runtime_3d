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
#ifndef QT3DS_RENDER_BACKEND_INPUT_ASSEMBLER_GL_H
#define QT3DS_RENDER_BACKEND_INPUT_ASSEMBLER_GL_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/StringTable.h"
#include "foundation/Utils.h"
#include "render/Qt3DSRenderBaseTypes.h"

namespace qt3ds {
namespace render {

    struct NVRenderBackendLayoutEntryGL
    {
        CRegisteredString m_AttribName; ///< must be the same name as used in the vertex shader
        QT3DSU8 m_Normalize; ///< normalize parameter
        QT3DSU32 m_AttribIndex; ///< attribute index
        QT3DSU32 m_Type; ///< GL vertex format type @sa GL_FLOAT, GL_INT
        QT3DSU32 m_NumComponents; ///< component count. max 4
        QT3DSU32 m_InputSlot; ///< Input slot where to fetch the data from
        QT3DSU32 m_Offset; ///< offset in byte
    };

    ///< this class handles the vertex attribute layout setup
    class NVRenderBackendAttributeLayoutGL
    {
    public:
        ///< constructor
        NVRenderBackendAttributeLayoutGL(NVDataRef<NVRenderBackendLayoutEntryGL> entries,
                                         QT3DSU32 maxInputSlot)
            : m_LayoutAttribEntries(entries)
            , m_MaxInputSlot(maxInputSlot)
        {
        }
        ///< destructor
        ~NVRenderBackendAttributeLayoutGL(){}

        NVRenderBackendLayoutEntryGL *getEntryByName(CRegisteredString entryName) const
        {
            QT3DS_FOREACH(idx, m_LayoutAttribEntries.size())
            {
                if (m_LayoutAttribEntries[idx].m_AttribName == entryName)
                    return &m_LayoutAttribEntries.mData[idx];
            }
            return NULL;
        }

        Option<NVRenderBackendLayoutEntryGL> getEntryByAttribIndex(QT3DSU32 attribIndex) const
        {
            QT3DS_FOREACH(idx, m_LayoutAttribEntries.size())
            {
                if (m_LayoutAttribEntries[idx].m_AttribIndex == attribIndex)
                    return m_LayoutAttribEntries[idx];
            }
            return Empty();
        }

        NVDataRef<NVRenderBackendLayoutEntryGL>
            m_LayoutAttribEntries; ///< vertex attribute layout entries
        QT3DSU32 m_MaxInputSlot; ///< max used input slot
    };

    ///< this class handles the input assembler setup
    class NVRenderBackendInputAssemblerGL
    {
    public:
        ///< constructor
        NVRenderBackendInputAssemblerGL(
            NVFoundationBase &fnd, NVRenderBackendAttributeLayoutGL *attribLayout,
            NVConstDataRef<NVRenderBackend::NVRenderBackendBufferObject> buffers,
            const NVRenderBackend::NVRenderBackendBufferObject indexBuffer,
            NVConstDataRef<QT3DSU32> strides, NVConstDataRef<QT3DSU32> offsets, QT3DSU32 patchVertexCount)
            : m_Foundation(fnd)
            , m_attribLayout(attribLayout)
            , m_VertexbufferHandles(buffers)
            , m_IndexbufferHandle(indexBuffer)
            , m_VaoID(0)
            , m_cachedShaderHandle(0)
            , m_PatchVertexCount(patchVertexCount)
        {
            QT3DSU32 *strideMem = (QT3DSU32 *)QT3DS_ALLOC(fnd.getAllocator(), strides.mSize * sizeof(QT3DSU32),
                                                 "BackendAttributeLayoutGL:m_strides");
            QT3DSU32 *offsetMem = (QT3DSU32 *)QT3DS_ALLOC(fnd.getAllocator(), strides.mSize * sizeof(QT3DSU32),
                                                 "BackendAttributeLayoutGL:m_strides");
            // copy offsets and strides
            QT3DS_FOREACH(idx, strides.size())
            {
                strideMem[idx] = strides.mData[idx];
                offsetMem[idx] = offsets.mData[idx];
            }

            m_strides = toDataRef(strideMem, strides.size());
            m_offsets = toDataRef(offsetMem, offsets.size());
        }
        ///< destructor
        ~NVRenderBackendInputAssemblerGL()
        {
            QT3DS_FREE(m_Foundation.getAllocator(), m_strides.mData);
            QT3DS_FREE(m_Foundation.getAllocator(), m_offsets.mData);
        };

        NVFoundationBase &m_Foundation; ///< pointer to foundation
        NVRenderBackendAttributeLayoutGL *m_attribLayout; ///< pointer to attribute layout
        NVConstDataRef<NVRenderBackend::NVRenderBackendBufferObject>
            m_VertexbufferHandles; ///< opaque vertex buffer backend handles
        NVRenderBackend::NVRenderBackendBufferObject
            m_IndexbufferHandle; ///< opaque index buffer backend handles
        QT3DSU32 m_VaoID; ///< this is only used if GL version is greater or equal 3
        QT3DSU32 m_cachedShaderHandle; ///< this is the shader id which was last used with this object
        QT3DSU32 m_PatchVertexCount; ///< vertex count for a single patch primitive
        NVDataRef<QT3DSU32> m_strides; ///< buffer strides
        NVDataRef<QT3DSU32> m_offsets; ///< buffer offsets
    };
}
}

#endif
