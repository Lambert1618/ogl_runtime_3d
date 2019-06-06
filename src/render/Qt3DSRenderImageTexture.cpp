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
#include "EASTL/vector.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderImageTexture.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace render {

    NVRenderImage2D::NVRenderImage2D(NVRenderContextImpl &context, NVFoundationBase &fnd,
                                     NVRenderTexture2D *inTexture,
                                     NVRenderImageAccessType::Enum inAccess)
        : m_Context(context)
        , m_Foundation(fnd)
        , mRefCount(0)
        , m_Backend(context.GetBackend())
        , m_Texture2D(inTexture)
        , m_TextureUnit(QT3DS_MAX_U32)
        , m_AccessType(inAccess)
        , m_TextureLevel(0)
    {
        inTexture->addRef();
    }

    NVRenderImage2D::~NVRenderImage2D()
    {
        m_Context.ImageDestroyed(*this);
        m_Texture2D->release();
    }

    void NVRenderImage2D::SetTextureLevel(QT3DSI32 inLevel)
    {
        if (m_Texture2D && m_Texture2D->GetNumMipmaps() >= (QT3DSU32)inLevel) {
            m_TextureLevel = inLevel;
        }
    }

    void NVRenderImage2D::Bind(QT3DSU32 unit)
    {
        if (unit == -1)
            m_TextureUnit = m_Context.GetNextTextureUnit();
        else
            m_TextureUnit = unit;

        STextureDetails theDetails(m_Texture2D->GetTextureDetails());

        // note it is the callers responsibility that the texture format is supported by the compute
        // shader
        m_Backend->BindImageTexture(m_Texture2D->GetTextureObjectHandle(), m_TextureUnit,
                                    m_TextureLevel, false, 0, m_AccessType, theDetails.m_Format);
    }

    NVRenderBackend::NVRenderBackendTextureObject NVRenderImage2D::GetTextureObjectHandle()
    {
        return m_Texture2D->GetTextureObjectHandle();
    }

    NVRenderImage2D *NVRenderImage2D::Create(NVRenderContextImpl &context,
                                             NVRenderTexture2D *inTexture,
                                             NVRenderImageAccessType::Enum inAccess)
    {
        if (inTexture)
            return QT3DS_NEW(context.GetFoundation().getAllocator(),
                          NVRenderImage2D)(context, context.GetFoundation(), inTexture, inAccess);
        else
            return NULL;
    }
}
}
