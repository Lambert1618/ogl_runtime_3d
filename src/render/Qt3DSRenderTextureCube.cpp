/****************************************************************************
**
** Copyright (C) 2014 NVIDIA Corporation.
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
#include "render/Qt3DSRenderSampler.h"
#include "render/Qt3DSRenderTextureCube.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace render {

    NVRenderTextureCube::NVRenderTextureCube(NVRenderContextImpl &context, NVFoundationBase &fnd,
                                             NVRenderTextureTargetType::Enum texTarget)
        : NVRenderTextureBase(context, fnd, texTarget)
        , m_Width(0)
        , m_Height(0)
    {
    }

    NVRenderTextureCube::~NVRenderTextureCube() { m_Context.TextureDestroyed(*this); }

    void NVRenderTextureCube::SetTextureData(NVDataRef<QT3DSU8> newBuffer, QT3DSU8 inMipLevel,
                                             NVRenderTextureCubeFaces::Enum inFace, QT3DSU32 width,
                                             QT3DSU32 height, NVRenderTextureFormats::Enum format)
    {
        QT3DS_ASSERT(m_TextureHandle);
        QT3DS_ASSERT(inFace != NVRenderTextureCubeFaces::InvalidFace);

        if (inMipLevel == 0) {
            m_Width = width;
            m_Height = height;
            m_Format = format;
            m_MaxMipLevel = inMipLevel;
        }

        if (m_MaxMipLevel < inMipLevel) {
            m_MaxMipLevel = inMipLevel;
        }

        // get max size and check value
        QT3DSI32 theMaxSize;
        m_Backend->GetRenderBackendValue(NVRenderBackend::NVRenderBackendQuery::MaxTextureSize,
                                         &theMaxSize);
        if (width > (QT3DSU32)theMaxSize || height > (QT3DSU32)theMaxSize) {
            qCCritical(INVALID_OPERATION, "Width or height is greater than max texture size (%d, %d)",
                theMaxSize, theMaxSize);
        }

        NVRenderTextureTargetType::Enum outTarget =
            static_cast<NVRenderTextureTargetType::Enum>((int)m_TexTarget + (int)inFace);
        if (NVRenderTextureFormats::isUncompressedTextureFormat(format)
            || NVRenderTextureFormats::isDepthTextureFormat(format)) {
            m_Backend->SetTextureDataCubeFace(m_TextureHandle, outTarget, inMipLevel, format, width,
                                              height, 0, format, newBuffer.begin());
        } else if (NVRenderTextureFormats::isCompressedTextureFormat(format)) {
            m_Backend->SetCompressedTextureDataCubeFace(m_TextureHandle, outTarget, inMipLevel,
                                                        format, width, height, 0, newBuffer.size(),
                                                        newBuffer.begin());
        }

        // Set our texture parameters to a default that will look the best
        if (inMipLevel > 0)
            SetMinFilter(NVRenderTextureMinifyingOp::LinearMipmapLinear);
    }

    STextureDetails NVRenderTextureCube::GetTextureDetails() const
    {
        return STextureDetails(m_Width, m_Height, 6, m_SampleCount, m_Format);
    }

    void NVRenderTextureCube::Bind()
    {
        m_TextureUnit = m_Context.GetNextTextureUnit();

        m_Backend->BindTexture(m_TextureHandle, m_TexTarget, m_TextureUnit);

        applyTexParams();
        applyTexSwizzle();
    }

    NVRenderTextureCube *NVRenderTextureCube::Create(NVRenderContextImpl &context)
    {
        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderTextureCube)(context, context.GetFoundation());
    }
}
}
