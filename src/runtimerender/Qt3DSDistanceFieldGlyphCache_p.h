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

#ifndef Q3DSDISTANCEFIELDGLYPHCACHE_P_H
#define Q3DSDISTANCEFIELDGLYPHCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/private/qsgadaptationlayer_p.h>
#include "render/Qt3DSRenderTexture2D.h"
#include "Qt3DSRenderContextCore.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)

QT_BEGIN_NAMESPACE

class QSGAreaAllocator;
class Q3DSDistanceFieldGlyphCache : public QSGDistanceFieldGlyphCache
{
public:
    struct TextureInfo {
        qt3ds::render::NVRenderTexture2D *texture;
        int padding = -1;

        QRect allocatedArea;
        QImage copy;
    };

    Q3DSDistanceFieldGlyphCache(const QRawFont &font,
                                qt3ds::render::IQt3DSRenderContext &context);
    ~Q3DSDistanceFieldGlyphCache() override;

    void requestGlyphs(const QSet<glyph_t> &glyphs) override;
    void storeGlyphs(const QList<QDistanceField> &glyphs) override;
    void referenceGlyphs(const QSet<glyph_t> &glyphs) override;
    void releaseGlyphs(const QSet<glyph_t> &glyphs) override;

    void processPendingGlyphs() override;

    TextureInfo *textureInfoById(uint textureId) const;

    qreal fontSize() const;

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    bool eightBitFormatIsAlphaSwizzled() const override;
#endif

private:
    bool loadPregeneratedCache(const QRawFont &font);
    TextureInfo *textureInfo(int index) const;

    int maxTextureSize() const;
    void resizeTexture(TextureInfo *info, int width, int height);
    void setTextureData(qt3ds::render::NVRenderTexture2D *texture, QImage &image);

    QSGAreaAllocator *m_areaAllocator = nullptr;
    int m_maxTextureSize              = 0;
    int m_maxTextureCount             = 3;

    mutable QVector<TextureInfo> m_textures;
    QHash<glyph_t, TextureInfo *> m_glyphsTexture;
    QSet<glyph_t> m_unusedGlyphs;
    qt3ds::render::IQt3DSRenderContext &m_context;
};

QT_END_NAMESPACE

#endif // Q3DSDISTANCEFIELDGLYPHCACHE_P_H

#endif // QT_VERSION >= QT_VERSION_CHECK(5,12,2)
