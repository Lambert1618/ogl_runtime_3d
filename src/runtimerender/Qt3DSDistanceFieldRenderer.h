/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#ifndef QT3DSDISTANCEFIELDRENDERER_H
#define QT3DSDISTANCEFIELDRENDERER_H

#include "Qt3DSFontDatabase_p.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)

#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderText.h"
#include "render/Qt3DSRenderShaderProgram.h"

#include "Qt3DSDistanceFieldGlyphCacheManager_p.h"
#include "Qt3DSDistanceFieldGlyphCache_p.h"

namespace qt3ds {
namespace render {

struct GlyphInfo {
    QVector<float> vertexes;
    QVector<quint32> glyphIndexes;
    Q3DSDistanceFieldGlyphCache *cache;
    float fontScale;
    float shadowOffsetX;
    float shadowOffsetY;
    NVBounds3 bounds;
};

struct Q3DSDistanceFieldShader {
    NVRenderShaderProgram *program = nullptr;
    NVRenderCachedShaderProperty<QT3DSMat44> mvp;
    NVRenderCachedShaderProperty<QT3DSI32> textureWidth;
    NVRenderCachedShaderProperty<QT3DSI32> textureHeight;
    NVRenderCachedShaderProperty<QT3DSF32> fontScale;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> texture;
    NVRenderCachedShaderProperty<QT3DSVec4> color;
};

struct Q3DSDistanceFieldDropShadowShader {
    NVRenderShaderProgram *program = nullptr;
    NVRenderCachedShaderProperty<QT3DSMat44> mvp;
    NVRenderCachedShaderProperty<QT3DSI32> textureWidth;
    NVRenderCachedShaderProperty<QT3DSI32> textureHeight;
    NVRenderCachedShaderProperty<QT3DSF32> fontScale;
    NVRenderCachedShaderProperty<QT3DSVec2> shadowOffset;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> texture;
    NVRenderCachedShaderProperty<QT3DSVec4> color;
    NVRenderCachedShaderProperty<QT3DSVec4> shadowColor;
};

struct Q3DSDistanceFieldMesh
{
    NVRenderAttribLayout *attribLayout = nullptr;
    NVRenderVertexBuffer *vertexBuffer = nullptr;
    NVRenderIndexBuffer *indexBuffer = nullptr;
    NVRenderInputAssembler *inputAssembler = nullptr;
};

class Q3DSDistanceFieldRenderer : public ITextRenderer
{
public:
    Q3DSDistanceFieldRenderer(NVFoundationBase &foundation);
    ~Q3DSDistanceFieldRenderer() override;
    QHash<Q3DSDistanceFieldGlyphCache::TextureInfo *, GlyphInfo> buildGlyphsPerTexture(
            const SText &textInfo);
    void buildShaders();
    Q3DSDistanceFieldMesh buildMesh(const GlyphInfo &glyphInfo, bool shadow);
    void renderMesh(NVRenderInputAssembler *inputAssembler, NVRenderTexture2D *texture,
                    const QT3DSMat44 &mvp, QT3DSI32 textureWidth, QT3DSI32 textureHeight,
                    QT3DSF32 fontScale, QT3DSVec4 color);
    void renderMeshWithDropShadow(NVRenderInputAssembler *inputAssembler,
                                  NVRenderTexture2D *texture, const QT3DSMat44 &mvp,
                                  QT3DSI32 textureWidth, QT3DSI32 textureHeight,
                                  QT3DSF32 fontScale, QT3DSVec2 shadowOffset,
                                  QT3DSVec4 color, QT3DSVec4 shadowColor);
    void renderText(SText &text, const QT3DSMat44 &mvp);
    void renderTextDepth(SText &text, const QT3DSMat44 &mvp);
    void setContext(IQt3DSRenderContext &context);

    bool checkAndBuildGlyphs(SText &text);

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_foundation.getAllocator())

    void AddSystemFontDirectory(const char8_t *dir) override;
    void AddProjectFontDirectory(const char8_t *dir) override;
    void ClearProjectFontDirectories() override;
    ITextRenderer &GetTextRenderer(NVRenderContext &) override;
    void EndFrame() override;

    // Unused methods:
    void PreloadFonts() override;
    void BeginPreloadFonts(IThreadPool &, IPerfTimer &) override;
    void EndPreloadFonts() override;
    void ReloadFonts() override;

    NVConstDataRef<SRendererFontEntry> GetProjectFontList() override;

    Option<CRegisteredString> GetFontNameForFont(CRegisteredString) override;
    Option<CRegisteredString> GetFontNameForFont(const char8_t *) override;

    STextDimensions MeasureText(const STextRenderInfo &, QT3DSF32,
                                const char8_t *) override;

    STextTextureDetails RenderText(const STextRenderInfo &,
                                   NVRenderTexture2D &) override;

    STextTextureDetails RenderText(const STextRenderInfo &, NVRenderPathFontItem &,
                                   NVRenderPathFontSpecification &) override;

    SRenderTextureAtlasDetails RenderText(const STextRenderInfo &) override;

    void BeginFrame() override;

    QT3DSI32 CreateTextureAtlas() override;
    STextTextureAtlasEntryDetails RenderAtlasEntry(QT3DSU32, NVRenderTexture2D &) override;

private:
    IQt3DSRenderContext *m_context = nullptr;
    QHash<size_t, QHash<Q3DSDistanceFieldGlyphCache::TextureInfo *, GlyphInfo>> m_glyphCache;
    QHash<size_t, Q3DSDistanceFieldMesh> m_meshCache;

    Q3DSFontDatabase m_fontDatabase;
    Q3DSDistanceFieldGlyphCacheManager m_glyphCacheManager;

    Q3DSDistanceFieldShader m_shader;
    Q3DSDistanceFieldDropShadowShader m_dropShadowShader;
    QVector<size_t> m_renderedGlyphs;

    QStringList m_systemDirs;
    QStringList m_projectDirs;

    qreal m_pixelRatio = 0.0;

    NVFoundationBase &m_foundation;
    volatile QT3DSI32 mRefCount = 0;
};

}
}

#endif // Qt version check

#endif // QT3DSDISTANCEFIELDRENDERER_H
