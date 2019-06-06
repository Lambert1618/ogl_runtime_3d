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

#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "render/Qt3DSRenderRenderBuffer.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderContext.h"

namespace qt3ds {
namespace render {

    NVRenderFrameBuffer::NVRenderFrameBuffer(NVRenderContextImpl &context, NVFoundationBase &fnd)
        : m_Context(context)
        , m_Foundation(fnd)
        , mRefCount(0)
        , m_Backend(context.GetBackend())
        , m_BufferHandle(NULL)
        , m_AttachmentBits(0)
    {
        m_BufferHandle = m_Backend->CreateRenderTarget();
        QT3DS_ASSERT(m_BufferHandle);
    }

    NVRenderFrameBuffer::~NVRenderFrameBuffer()
    {
        m_Context.FrameBufferDestroyed(*this);
        m_Backend->ReleaseRenderTarget(m_BufferHandle);
        m_BufferHandle = 0;
        m_AttachmentBits = 0;

        // release attachments
        QT3DS_FOREACH(idx, (QT3DSU32)NVRenderFrameBufferAttachments::LastAttachment)
        {
            if ((NVRenderFrameBufferAttachments::Enum)idx
                    != NVRenderFrameBufferAttachments::DepthStencil
                || m_Context.IsDepthStencilSupported())
                releaseAttachment((NVRenderFrameBufferAttachments::Enum)idx);
        }
    }

    inline void CheckAttachment(NVRenderContext &ctx,
                                NVRenderFrameBufferAttachments::Enum attachment)
    {
#ifdef _DEBUG
        QT3DS_ASSERT(attachment != NVRenderFrameBufferAttachments::DepthStencil
                  || ctx.IsDepthStencilSupported());
#endif
        (void)ctx;
        (void)attachment;
    }

    NVRenderTextureTargetType::Enum
    NVRenderFrameBuffer::releaseAttachment(NVRenderFrameBufferAttachments::Enum idx)
    {
        NVRenderTextureTargetType::Enum target = NVRenderTextureTargetType::Unknown;

        NVRenderTextureOrRenderBuffer Attach = m_Attachments[idx];
        if (Attach.HasTexture2D()) {
            target = (Attach.GetTexture2D()->IsMultisampleTexture())
                ? NVRenderTextureTargetType::Texture2D_MS
                : NVRenderTextureTargetType::Texture2D;
            Attach.GetTexture2D()->release();
        } else if (Attach.HasTexture2DArray()) {
            target = (Attach.GetTexture2DArray()->IsMultisampleTexture())
                ? NVRenderTextureTargetType::Texture2D_MS
                : NVRenderTextureTargetType::Texture2D_Array;
            Attach.GetTexture2DArray()->release();
        } else if (Attach.HasTextureCube()) {
            target = (Attach.GetTextureCube()->IsMultisampleTexture())
                ? NVRenderTextureTargetType::Texture2D_MS
                : NVRenderTextureTargetType::TextureCube;
            Attach.GetTextureCube()->release();
        } else if (Attach.HasRenderBuffer())
            Attach.GetRenderBuffer()->release();

        CheckAttachment(m_Context, idx);
        m_Attachments[idx] = NVRenderTextureOrRenderBuffer();

        m_AttachmentBits &= ~(1 << idx);

        return target;
    }

    NVRenderTextureOrRenderBuffer
    NVRenderFrameBuffer::GetAttachment(NVRenderFrameBufferAttachments::Enum attachment)
    {
        if (attachment == NVRenderFrameBufferAttachments::Unknown
            || attachment > NVRenderFrameBufferAttachments::LastAttachment) {
            qCCritical(INVALID_PARAMETER, "Attachment out of range");
            return NVRenderTextureOrRenderBuffer();
        }
        CheckAttachment(m_Context, attachment);
        return m_Attachments[attachment];
    }

    void NVRenderFrameBuffer::Attach(NVRenderFrameBufferAttachments::Enum attachment,
                                     NVRenderTextureOrRenderBuffer buffer,
                                     NVRenderTextureTargetType::Enum target)
    {
        if (attachment == NVRenderFrameBufferAttachments::Unknown
            || attachment > NVRenderFrameBufferAttachments::LastAttachment) {
            qCCritical(INVALID_PARAMETER, "Attachment out of range");
            return;
        }

        // early out
        // if there is nothing to detach
        if (!buffer.HasTexture2D() && !buffer.HasRenderBuffer() && !buffer.HasTexture2DArray()
            && !(m_AttachmentBits & (1 << attachment)))
            return;

        CheckAttachment(m_Context, attachment);
        // Ensure we are the bound framebuffer
        m_Context.SetRenderTarget(this);

        // release previous attachments
        NVRenderTextureTargetType::Enum theRelTarget = releaseAttachment(attachment);

        if (buffer.HasTexture2D()) {
            // On the same attachment point there could be a something attached with a different
            // target MSAA <--> NoMSAA
            if (theRelTarget != NVRenderTextureTargetType::Unknown && theRelTarget != target)
                m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                              NVRenderBackend::NVRenderBackendTextureObject(NULL),
                                              theRelTarget);

            m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                          buffer.GetTexture2D()->GetTextureObjectHandle(), target);
            buffer.GetTexture2D()->addRef();
            m_AttachmentBits |= (1 << attachment);
        } else if (buffer.HasTexture2DArray()) {
            // On the same attachment point there could be a something attached with a different
            // target MSAA <--> NoMSAA
            if (theRelTarget != NVRenderTextureTargetType::Unknown && theRelTarget != target)
                m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                              NVRenderBackend::NVRenderBackendTextureObject(NULL),
                                              theRelTarget);

            m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                          buffer.GetTexture2D()->GetTextureObjectHandle(), target);
            buffer.GetTexture2DArray()->addRef();
            m_AttachmentBits |= (1 << attachment);
        } else if (buffer.HasRenderBuffer()) {
            m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                          buffer.GetRenderBuffer()->GetRenderBuffertHandle());
            buffer.GetRenderBuffer()->addRef();
            m_AttachmentBits |= (1 << attachment);
        } else if (theRelTarget == NVRenderTextureTargetType::Unknown) {
            // detach renderbuffer
            m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                          NVRenderBackend::NVRenderBackendRenderbufferObject(NULL));
        } else {
            // detach texture
            m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                          NVRenderBackend::NVRenderBackendTextureObject(NULL),
                                          theRelTarget);
        }
        m_Attachments[attachment] = buffer;
    }

    void NVRenderFrameBuffer::AttachLayer(NVRenderFrameBufferAttachments::Enum attachment,
                                          NVRenderTextureOrRenderBuffer buffer, QT3DSI32 layer,
                                          QT3DSI32 level)
    {
        if (attachment == NVRenderFrameBufferAttachments::Unknown
            || attachment > NVRenderFrameBufferAttachments::LastAttachment) {
            qCCritical(INVALID_PARAMETER, "Attachment out of range");
            return;
        }

        // This function is only used for attaching a layer
        // If texture exists probably something is wrong
        if (!buffer.HasTexture2DArray()) {
            QT3DS_ASSERT(false);
            return;
        }

        CheckAttachment(m_Context, attachment);
        // Ensure we are the bound framebuffer
        m_Context.SetRenderTarget(this);

        // release previous attachments
        NVRenderTextureTargetType::Enum theRelTarget = releaseAttachment(attachment);

        // On the same attachment point there could be a something attached with a different target
        // MSAA <--> NoMSAA
        if (theRelTarget != NVRenderTextureTargetType::Unknown
            && theRelTarget != NVRenderTextureTargetType::Texture2D_Array)
            m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                          NVRenderBackend::NVRenderBackendTextureObject(NULL),
                                          theRelTarget);

        m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                      buffer.GetTexture2DArray()->GetTextureObjectHandle(), level,
                                      layer);
        buffer.GetTexture2DArray()->addRef();
        m_AttachmentBits |= (1 << attachment);

        m_Attachments[attachment] = buffer;
    }

    void NVRenderFrameBuffer::AttachFace(NVRenderFrameBufferAttachments::Enum attachment,
                                         NVRenderTextureOrRenderBuffer buffer,
                                         NVRenderTextureCubeFaces::Enum face)
    {
        if (attachment == NVRenderFrameBufferAttachments::Unknown
            || attachment > NVRenderFrameBufferAttachments::LastAttachment) {
            qCCritical(INVALID_PARAMETER, "Attachment out of range");
            return;
        }

        if (face == NVRenderTextureCubeFaces::InvalidFace) {
            QT3DS_ASSERT(false);
            return;
        }

        CheckAttachment(m_Context, attachment);
        // Ensure we are the bound framebuffer
        m_Context.SetRenderTarget(this);

        // release previous attachments
        NVRenderTextureTargetType::Enum attachTarget = static_cast<NVRenderTextureTargetType::Enum>(
            (int)NVRenderTextureTargetType::TextureCube + (int)face);
        NVRenderTextureTargetType::Enum theRelTarget = releaseAttachment(attachment);

        // If buffer has no texture cube, this call is used to detach faces.
        // If release target is not cube, there is something else attached to that
        // attachment point, so we want to release that first. E.g (MSAA <--> NoMSAA)
        if (theRelTarget == NVRenderTextureTargetType::TextureCube && !buffer.HasTextureCube()) {
            theRelTarget = attachTarget;
            attachTarget = NVRenderTextureTargetType::Unknown;
        } else if (theRelTarget == NVRenderTextureTargetType::TextureCube) {
            theRelTarget = NVRenderTextureTargetType::Unknown;
        }
        if (theRelTarget != NVRenderTextureTargetType::Unknown) {
            m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                          NVRenderBackend::NVRenderBackendTextureObject(NULL),
                                          theRelTarget);
        }

        if (attachTarget != NVRenderTextureTargetType::Unknown) {
            m_Backend->RenderTargetAttach(m_BufferHandle, attachment,
                                          buffer.GetTextureCube()->GetTextureObjectHandle(),
                                          attachTarget);
            buffer.GetTextureCube()->addRef();
            m_AttachmentBits |= (1 << attachment);
        }

        m_Attachments[attachment] = buffer;
    }

    bool NVRenderFrameBuffer::IsComplete()
    {
        // Ensure we are the bound framebuffer
        m_Context.SetRenderTarget(this);

        return m_Backend->RenderTargetIsValid(m_BufferHandle);
    }
}
}

qt3ds::render::NVRenderFrameBuffer *
qt3ds::render::NVRenderFrameBuffer::Create(NVRenderContextImpl &context)
{
    return QT3DS_NEW(context.GetFoundation().getAllocator(),
                  NVRenderFrameBuffer)(context, context.GetFoundation());
}
