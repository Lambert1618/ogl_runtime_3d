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
#ifndef QT3DS_RENDER_QT3DS_RENDER_FRAME_BUFFER_H
#define QT3DS_RENDER_QT3DS_RENDER_FRAME_BUFFER_H
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
namespace render {

    class NVRenderContextImpl;
    class NVRenderTexture2D;
    class NVRenderRenderBuffer;
    class NVRenderTexture2DArray;
    class NVRenderTextureCube;

    class NVRenderTextureOrRenderBuffer
    {
        NVRenderTexture2D *m_Texture2D;
        NVRenderTexture2DArray *m_Texture2DArray;
        NVRenderTextureCube *m_TextureCube;
        NVRenderRenderBuffer *m_RenderBuffer;

    public:
        NVRenderTextureOrRenderBuffer(NVRenderTexture2D &texture)
            : m_Texture2D(&texture)
            , m_Texture2DArray(NULL)
            , m_TextureCube(NULL)
            , m_RenderBuffer(NULL)
        {
        }
        NVRenderTextureOrRenderBuffer(NVRenderRenderBuffer &render)
            : m_Texture2D(NULL)
            , m_Texture2DArray(NULL)
            , m_TextureCube(NULL)
            , m_RenderBuffer(&render)
        {
        }
        NVRenderTextureOrRenderBuffer(NVRenderTexture2DArray &textureArray)
            : m_Texture2D(NULL)
            , m_Texture2DArray(&textureArray)
            , m_TextureCube(NULL)
            , m_RenderBuffer(NULL)
        {
        }
        NVRenderTextureOrRenderBuffer(NVRenderTextureCube &textureCube)
            : m_Texture2D(NULL)
            , m_Texture2DArray(NULL)
            , m_TextureCube(&textureCube)
            , m_RenderBuffer(NULL)
        {
        }
        NVRenderTextureOrRenderBuffer()
            : m_Texture2D(NULL)
            , m_Texture2DArray(NULL)
            , m_TextureCube(NULL)
            , m_RenderBuffer(NULL)
        {
        }
        NVRenderTextureOrRenderBuffer(const NVRenderTextureOrRenderBuffer &other)
            : m_Texture2D(other.m_Texture2D)
            , m_Texture2DArray(other.m_Texture2DArray)
            , m_TextureCube(other.m_TextureCube)
            , m_RenderBuffer(other.m_RenderBuffer)
        {
        }
        NVRenderTextureOrRenderBuffer &operator=(const NVRenderTextureOrRenderBuffer &other)
        {
            if (this != &other) {
                m_Texture2D = const_cast<NVRenderTexture2D *>(other.m_Texture2D);
                m_Texture2DArray = const_cast<NVRenderTexture2DArray *>(other.m_Texture2DArray);
                m_RenderBuffer = const_cast<NVRenderRenderBuffer *>(other.m_RenderBuffer);
                m_TextureCube = const_cast<NVRenderTextureCube *>(other.m_TextureCube);
            }
            return *this;
        }

        bool HasTexture2D() const { return m_Texture2D != NULL; }
        bool HasTexture2DArray() const { return m_Texture2DArray != NULL; }
        bool HasTextureCube() const { return m_TextureCube != NULL; }
        bool HasRenderBuffer() const { return m_RenderBuffer != NULL; }

        NVRenderTexture2D *GetTexture2D() const
        {
            QT3DS_ASSERT(HasTexture2D());
            return m_Texture2D;
        }
        NVRenderTexture2DArray *GetTexture2DArray() const
        {
            QT3DS_ASSERT(HasTexture2DArray());
            return m_Texture2DArray;
        }
        NVRenderTextureCube *GetTextureCube() const
        {
            QT3DS_ASSERT(HasTextureCube());
            return m_TextureCube;
        }
        NVRenderRenderBuffer *GetRenderBuffer() const
        {
            QT3DS_ASSERT(HasRenderBuffer());
            return m_RenderBuffer;
        }
    };

    class NVRenderFrameBuffer : public NVRefCounted, public NVRenderImplemented
    {
    private:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend

        NVRenderTextureOrRenderBuffer
            m_Attachments[NVRenderFrameBufferAttachments::LastAttachment]; ///< attachments array
        NVRenderBackend::NVRenderBackendRenderTargetObject
            m_BufferHandle; ///< opaque backend handle

    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] fnd			Pointer to foundation
         *
         * @return No return.
         */
        NVRenderFrameBuffer(NVRenderContextImpl &context, NVFoundationBase &fnd);

        /// destructor
        virtual ~NVRenderFrameBuffer();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

        /**
         * @brief query attachment
         *
         *
         * @return buffer format
         */
        virtual NVRenderTextureOrRenderBuffer
        GetAttachment(NVRenderFrameBufferAttachments::Enum attachment);

        /**
         * @brief Attach a render or texture buffer to a render target
         *		  For texture attachments we use always level 0
         *
         * @param[in] attachment		Attachment point (e.g. COLOR0, DEPTH...)
         * @param[in] buffer			Contains a pointer to the attachment
         * @param[in] target			Attachment texture target
         *
         * @return no return
         */
        virtual void
        Attach(NVRenderFrameBufferAttachments::Enum attachment,
               NVRenderTextureOrRenderBuffer buffer,
               NVRenderTextureTargetType::Enum target = NVRenderTextureTargetType::Texture2D);

        /**
         * @brief Attach a particular layer of the texture 2D array to a render target
         *
         * @param[in] attachment		Attachment point (e.g. COLOR0, DEPTH...)
         * @param[in] buffer			Pointer to the Texture Array which contains the
         * layers
         * @param[in] layer				The index to the layer that will be attached to the
         * target
         * @param[in] level				Mip level of the texture that will be attached
         * (default 0)
         *
         * @return no return
         */
        virtual void AttachLayer(NVRenderFrameBufferAttachments::Enum attachment,
                                 NVRenderTextureOrRenderBuffer buffer, QT3DSI32 layer,
                                 QT3DSI32 level = 0);

        /**
         * @brief Attach a particular face of the texture cubemap to a render target
         *
         * @param[in] attachment		Attachment point (e.g. COLOR0, DEPTH...)
         * @param[in] buffer			Pointer to the Texture Array which contains the
         * layers
         * @param[in] face				The face of the cubemap that will be attached to the
         * target
         * @param[in] level				Mip level of the texture that will be attached
         * (default 0)
         *
         * @return no return
         */
        virtual void AttachFace(NVRenderFrameBufferAttachments::Enum attachment,
                                NVRenderTextureOrRenderBuffer buffer,
                                NVRenderTextureCubeFaces::Enum face);

        /**
         * @brief Check that this framebuffer is complete and can be rendered to.
         *
         *
         * @return true if complete
         */
        virtual bool IsComplete();

        /**
         * @brief query if framebuffer has any attachment
         *
         * @return true if any attachment
         */
        virtual bool HasAnyAttachment() { return (m_AttachmentBits != 0); }

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        virtual NVRenderBackend::NVRenderBackendRenderTargetObject GetFrameBuffertHandle()
        {
            return m_BufferHandle;
        }

        // this will be obsolete
        const void *GetImplementationHandle() const override
        {
            return reinterpret_cast<const void *>(m_BufferHandle);
        }

        /**
         * @brief static creator function
         *
         * @param[in] context		Pointer to context
         *
         * @return a pointer to framebuffer object.
         */
        static NVRenderFrameBuffer *Create(NVRenderContextImpl &context);

    private:
        /**
         * @brief releaes an attached object
         *
         * @return which target we released
         */
        NVRenderTextureTargetType::Enum releaseAttachment(NVRenderFrameBufferAttachments::Enum idx);

        QT3DSU32 m_AttachmentBits; ///< holds flags for current attached buffers
    };
}
}

#endif
