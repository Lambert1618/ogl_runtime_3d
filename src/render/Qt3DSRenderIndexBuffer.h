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
#ifndef QT3DS_RENDER_INDEX_BUFFER_H
#define QT3DS_RENDER_INDEX_BUFFER_H
#include "render/Qt3DSRenderDataBuffer.h"
#include "render/Qt3DSRenderDrawable.h"

namespace qt3ds {
class NVFoundationBase;
}

namespace qt3ds {
namespace render {

    // forward declaration
    class NVRenderContextImpl;

    class NVRenderIndexBuffer : public NVRenderDataBuffer, public NVRenderDrawable
    {
    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] size			Size of the buffer
         * @param[in] componentType	Size of the buffer
         * @param[in] usage			Usage of the buffer (e.g. static, dynamic...)
         * @param[in] data			A pointer to the buffer data that is allocated by the
         * application.
         *
         * @return No return.
         */
        NVRenderIndexBuffer(NVRenderContextImpl &context, size_t size,
                            NVRenderComponentTypes::Enum componentType,
                            NVRenderBufferUsageType::Enum usageType, NVDataRef<QT3DSU8> data);

        ///< destruvtor
        ~NVRenderIndexBuffer();

        /**
         * @brief get the component type (QT3DSU8, QT3DSU16)
         *
         * @return the component type
         */
        virtual NVRenderComponentTypes::Enum GetComponentType() const { return m_ComponentType; }

        /**
         * @brief get the index count
         *
         * @return actual index count
         */
        virtual QT3DSU32 GetNumIndices() const;

        /**
         * @brief bind the buffer bypasses the context state
         *
         * @return no return.
         */
        void Bind() override;

        /**
         * @brief draw the buffer
         *
         * @param[in] drawMode		draw mode (e.g Triangles...)
         * @param[in] count			vertex count
         * @param[in] offset		start offset in byte
         *
         * @return no return.
         */
        void Draw(NVRenderDrawMode::Enum drawMode, QT3DSU32 count, QT3DSU32 offset) override;

        /**
         * @brief draw the buffer via indirec draw buffer setup
         *
         * @param[in] drawMode		draw mode (e.g Triangles...)
         * @param[in] offset		byte offset into the bound drawIndirectBuffer see
         * NVRenderDrawIndirectBuffer
         *
         * @return no return.
         */
        virtual void DrawIndirect(NVRenderDrawMode::Enum drawMode, QT3DSU32 offset);

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        NVRenderBackend::NVRenderBackendBufferObject GetBuffertHandle() const override
        {
            return m_BufferHandle;
        }

        // this will be obsolete
        const void *GetImplementationHandle() const override
        {
            return reinterpret_cast<void *>(m_BufferHandle);
        }

        static NVRenderIndexBuffer *Create(NVRenderContextImpl &context,
                                           NVRenderBufferUsageType::Enum usageType,
                                           NVRenderComponentTypes::Enum componentType, size_t size,
                                           NVConstDataRef<QT3DSU8> bufferData);

    private:
        NVRenderComponentTypes::Enum m_ComponentType; ///< component type (QT3DSU8, QT3DSU16)
    };
}
}

#endif
