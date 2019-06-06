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

#include "Qt3DSRenderTextTextureAtlas.h"
#include "Qt3DSTextRenderer.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "render/Qt3DSRenderContext.h"

using namespace qt3ds::render;

namespace {

struct STextTextureAtlas : public ITextTextureAtlas
{
    static const QT3DSI32 TEXTURE_ATLAS_DIM =
        256; // if you change this you need to adjust Qt3DSOnscreenTextRenderer size as well

    NVFoundationBase &m_Foundation;
    volatile QT3DSI32 mRefCount;
    NVScopedRefCounted<ITextRenderer> m_TextRenderer;
    NVScopedRefCounted<NVRenderContext> m_RenderContext;

    STextTextureAtlas(NVFoundationBase &inFnd, ITextRenderer &inRenderer,
                      NVRenderContext &inRenderContext)
        : m_Foundation(inFnd)
        , mRefCount(0)
        , m_TextRenderer(inRenderer)
        , m_RenderContext(inRenderContext)
        , m_TextureAtlasInitialized(false)
        , m_textureAtlas(NULL)
    {
    }

    virtual ~STextTextureAtlas()
    {
        if (m_textureAtlas) {
            m_textureAtlas->release();
        }
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    TTextRenderAtlasDetailsAndTexture RenderText(const STextRenderInfo &inText) override
    {
        SRenderTextureAtlasDetails theDetails = m_TextRenderer->RenderText(inText);

        return TTextRenderAtlasDetailsAndTexture(theDetails, m_textureAtlas);
    }

    bool IsInitialized() override { return m_TextureAtlasInitialized && m_textureAtlas; }

    TTextTextureAtlasDetailsAndTexture PrepareTextureAtlas() override
    {
        if (!m_TextureAtlasInitialized && !m_textureAtlas) {
            // create the texture atlas entries
            QT3DSI32 count = m_TextRenderer->CreateTextureAtlas();

            m_textureAtlas = m_RenderContext->CreateTexture2D();
            if (m_textureAtlas && count) {
                m_TextureAtlasInitialized = true;
                m_textureAtlas->addRef();
                // if you change the size you need to adjust Qt3DSOnscreenTextRenderer too
                if (m_RenderContext->GetRenderContextType() == NVRenderContextValues::GLES2) {
                    m_textureAtlas->SetTextureData(NVDataRef<QT3DSU8>(), 0, TEXTURE_ATLAS_DIM,
                                               TEXTURE_ATLAS_DIM, NVRenderTextureFormats::RGBA8);
                } else {
                    m_textureAtlas->SetTextureData(NVDataRef<QT3DSU8>(), 0, TEXTURE_ATLAS_DIM,
                                               TEXTURE_ATLAS_DIM, NVRenderTextureFormats::Alpha8);
                }
                m_textureAtlas->SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Linear);
                m_textureAtlas->SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Linear);
                m_textureAtlas->SetTextureWrapS(NVRenderTextureCoordOp::ClampToEdge);
                m_textureAtlas->SetTextureWrapT(NVRenderTextureCoordOp::ClampToEdge);
                qt3ds::render::STextureDetails texTexDetails = m_textureAtlas->GetTextureDetails();
                return TTextTextureAtlasDetailsAndTexture(
                    STextTextureAtlasDetails(texTexDetails.m_Height, texTexDetails.m_Height, false,
                                             count),
                    m_textureAtlas);
            }
        }

        return TTextTextureAtlasDetailsAndTexture(STextTextureAtlasDetails(), NULL);
    }

private:
    bool m_TextureAtlasInitialized;
    NVRenderTexture2D *m_textureAtlas; // this is the actual texture which has application lifetime
};

} // namespace

ITextTextureAtlas &ITextTextureAtlas::CreateTextureAtlas(NVFoundationBase &inFnd,
                                                         ITextRenderer &inTextRenderer,
                                                         NVRenderContext &inRenderContext)
{
    return *QT3DS_NEW(inFnd.getAllocator(), STextTextureAtlas)(inFnd, inTextRenderer, inRenderContext);
}
