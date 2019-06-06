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
#ifndef QT3DS_RENDER_INPUT_ASSEMBLER_H
#define QT3DS_RENDER_INPUT_ASSEMBLER_H

#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Utils.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
namespace render {

    // forward declarations
    class NVRenderContextImpl;
    class NVRenderBackend;
    class NVRenderAttribLayout;

    ///< this class handles the vertex attribute layout setup
    class NVRenderInputAssembler : public NVRefCounted
    {
    public:
        /**
         * @brief constructor
         *
         *	NOTE: The limit for buffers count is currently 16
         *
         * @param[in] context			Pointer to context
         * @param[in] attribLayout		Pointer to NVRenderAttribLayout object
         * @param[in] buffers			list of vertex buffers
         * @param[in] indexBuffer		pointer to index buffer. Can be NULL
         * @param[in] strides			list of strides of the buffer
         * @param[in] offsets			list of offsets into the buffer
         * @param[in] primType			primitive type used for drawing
         * @param[in] patchVertexCount	if primitive is "Patch" this is the vertex count for a
         *single patch
         *
         * @return No return.
         */
        NVRenderInputAssembler(NVRenderContextImpl &context, NVRenderAttribLayout *attribLayout,
                               NVConstDataRef<NVRenderVertexBuffer *> buffers,
                               const NVRenderIndexBuffer *indexBuffer,
                               NVConstDataRef<QT3DSU32> strides, NVConstDataRef<QT3DSU32> offsets,
                               NVRenderDrawMode::Enum primType = NVRenderDrawMode::Triangles,
                               QT3DSU32 patchVertexCount = 1);
        ///< destructor
        ~NVRenderInputAssembler();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        NVRenderBackend::NVRenderBackendInputAssemblerObject GetInputAssemblerHandle() const
        {
            return m_InputAssemblertHandle;
        }

        /**
         * @brief get the attached index buffer
         *
         * @return the index buffer
         */
        const NVRenderIndexBuffer *GetIndexBuffer() { return m_IndexBuffer; }

        /**
         * @brief get the index count of the attached index buffer (if any)
         *
         * @return the index buffer count
         */
        QT3DSU32 GetIndexCount() const;

        /**
         * @brief get the vertex count of the buffer
         *		  Note this makes only sense if we have a single
         *		  interleaved buffer
         *
         * @return the vertex buffer count
         */
        QT3DSU32 GetVertexCount() const;

        /**
         * @brief get the primitive type used for drawing
         *
         * @return primitive type
         */
        NVRenderDrawMode::Enum GetPrimitiveType() const { return m_PrimitiveType; }

        /**
         * @brief set the per vertex patch count
         *
         * @return none
         */
        void SetPatchVertexCount(QT3DSU32 count)
        {
            if (count != m_PatchVertexCount) {
                // clamp to 1;
                m_PatchVertexCount = (count == 0) ? 1 : count;
                ;
                m_Backend->SetPatchVertexCount(m_InputAssemblertHandle, m_PatchVertexCount);
            }
        }

    private:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend

        NVRenderAttribLayout *m_AttribLayout; ///< pointer to attribute layout
        nvvector<NVRenderVertexBuffer *> m_VertexBuffers; ///< vertex buffers
        const NVRenderIndexBuffer *m_IndexBuffer; ///< index buffer
        NVConstDataRef<NVRenderBackend::NVRenderBackendBufferObject>
            m_VertexbufferHandles; ///< opaque vertex buffer backend handles

        NVRenderBackend::NVRenderBackendInputAssemblerObject
            m_InputAssemblertHandle; ///< opaque backend handle
        NVRenderDrawMode::Enum m_PrimitiveType; ///< primitive type used for drawing
        QT3DSU32 m_PatchVertexCount; ///< vertex count if primitive type is patch
    };
}
}

#endif
