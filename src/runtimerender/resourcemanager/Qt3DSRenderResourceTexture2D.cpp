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
#include "Qt3DSRenderResourceTexture2D.h"

using namespace qt3ds::render;

CResourceTexture2D::CResourceTexture2D(IResourceManager &mgr, NVRenderTexture2D *inTexture)
    : m_ResourceManager(mgr)
    , m_Texture(inTexture)
{
    if (inTexture)
        m_TextureDetails = inTexture->GetTextureDetails();
}

CResourceTexture2D::CResourceTexture2D(IResourceManager &mgr, QT3DSU32 width, QT3DSU32 height,
                                       NVRenderTextureFormats::Enum inFormat, QT3DSU32 inSamples)
    : m_ResourceManager(mgr)
    , m_Texture(NULL)
{
    EnsureTexture(width, height, inFormat, inSamples);
}

CResourceTexture2D::~CResourceTexture2D()
{
    ReleaseTexture();
}

// Returns true if the texture was allocated, false if nothing changed (no allocation).
bool CResourceTexture2D::TextureMatches(QT3DSU32 width, QT3DSU32 height,
                                        NVRenderTextureFormats::Enum inFormat, QT3DSU32 inSamples)
{
    return m_Texture && m_TextureDetails.m_Width == width && m_TextureDetails.m_Height == height
        && m_TextureDetails.m_Format == inFormat && m_TextureDetails.m_SampleCount == inSamples;
}

bool CResourceTexture2D::EnsureTexture(QT3DSU32 width, QT3DSU32 height,
                                       NVRenderTextureFormats::Enum inFormat, QT3DSU32 inSamples)
{
    if (TextureMatches(width, height, inFormat, inSamples))
        return false;

    if (m_Texture && inSamples > 1) {
        // we cannot resize MSAA textures though release first
        ReleaseTexture();
    }

    if (!m_Texture)
        m_Texture = m_ResourceManager.AllocateTexture2D(width, height, inFormat, inSamples);
    else {
        // multisampled textures are immuteable
        QT3DS_ASSERT(inSamples == 1);
        m_Texture->SetTextureData(NVDataRef<QT3DSU8>(), 0, width, height, inFormat);
    }

    m_TextureDetails = m_Texture->GetTextureDetails();
    return true;
}

void CResourceTexture2D::ReleaseTexture()
{
    if (m_Texture) {
        m_ResourceManager.Release(*m_Texture);
        ForgetTexture();
    }
}

void CResourceTexture2D::ForgetTexture()
{
    m_Texture = NULL;
}

void CResourceTexture2D::StealTexture(CResourceTexture2D &inOther)
{
    ReleaseTexture();
    m_Texture = inOther.m_Texture;
    m_TextureDetails = inOther.m_TextureDetails;
    inOther.m_Texture = NULL;
}

CResourceTexture2DArray::CResourceTexture2DArray(IResourceManager &mgr)
    : m_ResourceManager(mgr)
    , m_Texture(NULL)
{
}

CResourceTexture2DArray::CResourceTexture2DArray(IResourceManager &mgr, QT3DSU32 width, QT3DSU32 height,
                                                 QT3DSU32 slices,
                                                 NVRenderTextureFormats::Enum inFormat,
                                                 QT3DSU32 inSamples)
    : m_ResourceManager(mgr)
    , m_Texture(NULL)
{
    EnsureTexture(width, height, slices, inFormat, inSamples);
}

CResourceTexture2DArray::~CResourceTexture2DArray()
{
    ReleaseTexture();
}

bool CResourceTexture2DArray::TextureMatches(QT3DSU32 width, QT3DSU32 height, QT3DSU32 slices,
                                             NVRenderTextureFormats::Enum inFormat, QT3DSU32 inSamples)
{
    return m_Texture && m_TextureDetails.m_Depth == slices && m_TextureDetails.m_Width == width
        && m_TextureDetails.m_Height == height && m_TextureDetails.m_Format == inFormat
        && m_TextureDetails.m_SampleCount == inSamples;
}

bool CResourceTexture2DArray::EnsureTexture(QT3DSU32 width, QT3DSU32 height, QT3DSU32 slices,
                                            NVRenderTextureFormats::Enum inFormat, QT3DSU32 inSamples)
{
    if (TextureMatches(width, height, slices, inFormat, inSamples))
        return false;

    if (m_Texture && inSamples > 1) {
        // we cannot resize MSAA textures though release first
        ReleaseTexture();
    }

    if (!m_Texture)
        m_Texture =
            m_ResourceManager.AllocateTexture2DArray(width, height, slices, inFormat, inSamples);
    else {
        // multisampled textures are immuteable
        QT3DS_ASSERT(inSamples == 1);
        m_Texture->SetTextureData(NVDataRef<QT3DSU8>(), 0, width, height, slices, inFormat);
    }

    m_TextureDetails = m_Texture->GetTextureDetails();
    return true;
}

void CResourceTexture2DArray::ReleaseTexture()
{
    if (m_Texture) {
        m_ResourceManager.Release(*m_Texture);
        m_Texture = NULL;
    }
}

void CResourceTexture2DArray::StealTexture(CResourceTexture2DArray &inOther)
{
    ReleaseTexture();
    m_Texture = inOther.m_Texture;
    m_TextureDetails = inOther.m_TextureDetails;
    inOther.m_Texture = NULL;
}
