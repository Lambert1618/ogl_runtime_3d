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
#ifndef QT3DS_RENDER_QT3DS_RENDER_RENDER_BUFFER_H
#define QT3DS_RENDER_QT3DS_RENDER_RENDER_BUFFER_H
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
namespace render {
    class NVRenderContextImpl;

    struct NVRenderRenderBufferDimensions
    {
        QT3DSU32 m_Width; ///< buffer width
        QT3DSU32 m_Height; ///< buffer height

        NVRenderRenderBufferDimensions(QT3DSU32 w, QT3DSU32 h)
            : m_Width(w)
            , m_Height(h)
        {
        }
        NVRenderRenderBufferDimensions()
            : m_Width(0)
            , m_Height(0)
        {
        }
    };

    class NVRenderRenderBuffer : public NVRefCounted, public NVRenderImplemented
    {
    private:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend
        QT3DSU32 m_Width; ///< buffer width
        QT3DSU32 m_Height; ///< buffer height
        NVRenderRenderBufferFormats::Enum m_StorageFormat; ///< buffer storage format

        NVRenderBackend::NVRenderBackendRenderbufferObject
            m_BufferHandle; ///< opaque backend handle

    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] fnd			Pointer to foundation
         * @param[in] format		Renderbuffer format
         * @param[in] width			Renderbuffer width
         * @param[in] height		Renderbuffer height
         *
         * @return No return.
         */
        NVRenderRenderBuffer(NVRenderContextImpl &context, NVFoundationBase &fnd,
                             NVRenderRenderBufferFormats::Enum format, QT3DSU32 width, QT3DSU32 height);

        /// destructor
        virtual ~NVRenderRenderBuffer();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

        /**
         * @brief query buffer format
         *
         *
         * @return buffer format
         */
        virtual NVRenderRenderBufferFormats::Enum GetStorageFormat() const
        {
            return m_StorageFormat;
        }

        /**
         * @brief query buffer dimension
         *
         *
         * @return NVRenderRenderBufferDimensions object
         */
        virtual NVRenderRenderBufferDimensions GetDimensions() const
        {
            return NVRenderRenderBufferDimensions(m_Width, m_Height);
        }

        /**
         * @brief constructor
         *
         * @param[in] inDimensions		A dimension object
         *
         * @return buffer format
         */
        virtual void SetDimensions(const NVRenderRenderBufferDimensions &inDimensions);

        /**
         * @brief static creator function
         *
         * @param[in] context		Pointer to context
         * @param[in] format		Renderbuffer format
         * @param[in] width			Renderbuffer width
         * @param[in] height		Renderbuffer height
         *
         * @return No return.
         */
        static NVRenderRenderBuffer *Create(NVRenderContextImpl &context,
                                            NVRenderRenderBufferFormats::Enum format, QT3DSU32 width,
                                            QT3DSU32 height);

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        virtual NVRenderBackend::NVRenderBackendRenderbufferObject GetRenderBuffertHandle()
        {
            return m_BufferHandle;
        }

        // this will be obsolete
        const void *GetImplementationHandle() const override
        {
            return reinterpret_cast<void *>(m_BufferHandle);
        }
    };
}
}

#endif
