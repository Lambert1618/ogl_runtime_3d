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
#include "render/Qt3DSRenderRenderBuffer.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSFoundation.h"

namespace qt3ds {
namespace render {

    NVRenderRenderBuffer::NVRenderRenderBuffer(NVRenderContextImpl &context, NVFoundationBase &fnd,
                                               NVRenderRenderBufferFormats::Enum format,
                                               QT3DSU32 width, QT3DSU32 height)
        : m_Context(context)
        , m_Foundation(fnd)
        , mRefCount(0)
        , m_Backend(context.GetBackend())
        , m_Width(width)
        , m_Height(height)
        , m_StorageFormat(format)
        , m_BufferHandle(NULL)
    {
        SetDimensions(NVRenderRenderBufferDimensions(width, height));
    }

    NVRenderRenderBuffer::~NVRenderRenderBuffer()
    {
        m_Context.RenderBufferDestroyed(*this);
        m_Backend->ReleaseRenderbuffer(m_BufferHandle);
        m_BufferHandle = 0;
    }

    void NVRenderRenderBuffer::SetDimensions(const NVRenderRenderBufferDimensions &inDimensions)
    {
        QT3DSU32 maxWidth, maxHeight;
        m_Width = inDimensions.m_Width;
        m_Height = inDimensions.m_Height;

        // get max size and clamp to max value
        m_Context.getMaxTextureSize(maxWidth, maxHeight);
        if (m_Width > maxWidth || m_Height > maxHeight) {
            qCCritical(INVALID_OPERATION, "Width or height is greater than max texture size (%d, %d)",
                maxWidth, maxHeight);
            m_Width = NVMin(m_Width, maxWidth);
            m_Height = NVMin(m_Height, maxHeight);
        }

        bool success = true;

        if (m_BufferHandle == NULL)
            m_BufferHandle = m_Backend->CreateRenderbuffer(m_StorageFormat, m_Width, m_Height);
        else
            success =
                m_Backend->ResizeRenderbuffer(m_BufferHandle, m_StorageFormat, m_Width, m_Height);

        if (m_BufferHandle == NULL || !success) {
            // We could try smaller sizes
            QT3DS_ASSERT(false);
            qCCritical(INTERNAL_ERROR, "Unable to create render buffer %s, %dx%d",
                               NVRenderRenderBufferFormats::toString(m_StorageFormat), m_Width,
                               m_Height);
        }
    }
}
}

qt3ds::render::NVRenderRenderBuffer *
qt3ds::render::NVRenderRenderBuffer::Create(NVRenderContextImpl &context,
                                         NVRenderRenderBufferFormats::Enum format, QT3DSU32 width,
                                         QT3DSU32 height)
{
    NVRenderRenderBuffer *retval = NULL;
    if (width == 0 || height == 0) {
        qCCritical(INVALID_PARAMETER, "Invalid renderbuffer width or height");
        return retval;
    }

    retval = QT3DS_NEW(context.GetFoundation().getAllocator(),
                    NVRenderRenderBuffer)(context, context.GetFoundation(), format, width, height);

    return retval;
}
