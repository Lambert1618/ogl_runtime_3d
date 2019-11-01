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

#include "Qt3DSDistanceFieldGlyphCache_p.h"

#include <QtQuick/private/qsgareaallocator_p.h>

#include <QtCore/qmath.h>
#include <QtCore/qendian.h>
#include <QtGui/qimage.h>

#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "Qt3DSRenderResourceManager.h"
#include "foundation/Qt3DSAllocator.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)

QT_BEGIN_NAMESPACE

// Should work on most hardware. Used as stop gap until Qt 3D provides a
// way to retrieve the system value
#ifndef Q3DSDISTANCEFIELDGLYPHCACHE_MAXIMUM_TEXURE_SIZE
#  define Q3DSDISTANCEFIELDGLYPHCACHE_MAXIMUM_TEXURE_SIZE 2048
#endif

#if !defined(Q3DSDISTANCEFIELDGLYPHCACHE_PADDING)
#  define Q3DSDISTANCEFIELDGLYPHCACHE_PADDING 2
#endif

Q3DSDistanceFieldGlyphCache::Q3DSDistanceFieldGlyphCache(
        const QRawFont &font, qt3ds::render::IQt3DSRenderContext &context)
    : QSGDistanceFieldGlyphCache(font)
    , m_context(context)
{
    m_maxTextureSize = Q3DSDISTANCEFIELDGLYPHCACHE_MAXIMUM_TEXURE_SIZE;

    loadPregeneratedCache(font);
}

Q3DSDistanceFieldGlyphCache::~Q3DSDistanceFieldGlyphCache()
{
    for (auto &texture : m_textures)
        qt3ds::foundation::NVDelete(m_context.GetAllocator(), texture.texture);
    delete m_areaAllocator;
}

int Q3DSDistanceFieldGlyphCache::maxTextureSize() const
{
    return m_maxTextureSize;
}

Q3DSDistanceFieldGlyphCache::TextureInfo *Q3DSDistanceFieldGlyphCache::textureInfo(int index) const
{
    while (index >= m_textures.size())
        m_textures.append(TextureInfo());
    return &m_textures[index];
}

void Q3DSDistanceFieldGlyphCache::referenceGlyphs(const QSet<glyph_t> &glyphs)
{
    m_unusedGlyphs -= glyphs;
}

void Q3DSDistanceFieldGlyphCache::releaseGlyphs(const QSet<glyph_t> &glyphs)
{
    m_unusedGlyphs += glyphs;
}

void Q3DSDistanceFieldGlyphCache::setTextureData(qt3ds::render::NVRenderTexture2D *texture,
                                                 QImage &image)
{
    bool isGLES2 = m_context.GetRenderContext().GetRenderContextType()
            == qt3ds::render::NVRenderContextValues::GLES2;
    texture->SetTextureData(qt3ds::render::toU8DataRef(image.bits(), image.byteCount()),
                            0, image.width(), image.height(),
                            isGLES2 ? qt3ds::render::NVRenderTextureFormats::Alpha8
                                    : qt3ds::render::NVRenderTextureFormats::R8);
}

void Q3DSDistanceFieldGlyphCache::resizeTexture(TextureInfo *info, int width, int height)
{
    QImage &image = info->copy;
    if (info->texture == nullptr) {
        info->texture = m_context.GetRenderContext().CreateTexture2D();
        info->texture->SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Enum::Linear);
        info->texture->SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Enum::Linear);
        setTextureData(info->texture, image);
    }

    qt3ds::render::STextureDetails textureDetails = info->texture->GetTextureDetails();
    if (int(textureDetails.m_Width) != width || int(textureDetails.m_Height) != height)
        setTextureData(info->texture, image);

    if (info->copy.width() != width || info->copy.height() != height) {
        QImage newImage(width, height, QImage::Format_Alpha8);

        for (int y = 0; y < info->copy.height(); ++y) {
            uchar *dest = newImage.scanLine(y);
            const uchar *src = info->copy.scanLine(y);
            ::memcpy(dest, src, size_t(info->copy.width()));
        }

        info->copy = newImage;

        setTextureData(info->texture, image);
    }
}

void Q3DSDistanceFieldGlyphCache::storeGlyphs(const QList<QDistanceField> &glyphs)
{
    using GlyphTextureHash = QHash<TextureInfo *, QVector<glyph_t> >;
    using GlyphTextureHashConstIt = GlyphTextureHash::const_iterator;

    GlyphTextureHash glyphTextures;
    for (int i = 0; i < glyphs.size(); ++i) {
        QDistanceField glyph = glyphs.at(i);
        glyph_t glyphIndex = glyph.glyph();
        TexCoord c = glyphTexCoord(glyphIndex);
        TextureInfo *texInfo = m_glyphsTexture.value(glyphIndex);

        resizeTexture(texInfo, maxTextureSize(), texInfo->allocatedArea.height());

        Q_ASSERT(!glyphTextures[texInfo].contains(glyphIndex));
        glyphTextures[texInfo].append(glyphIndex);

        int padding = texInfo->padding;
        int expectedWidth = qCeil(c.width + c.xMargin * 2);
        glyph = glyph.copy(-padding, -padding,
                           expectedWidth + padding * 2, glyph.height() + padding * 2);

        for (int y = 0; y < glyph.height(); ++y) {
            const uchar *src = glyph.scanLine(y);
            uchar *dest = texInfo->copy.scanLine(int(c.y) + y - padding) + (int(c.x) - padding);
            ::memcpy(dest, src, size_t(glyph.width()));
        }
    }

    for (GlyphTextureHashConstIt i = glyphTextures.constBegin(),
         cend = glyphTextures.constEnd(); i != cend; ++i) {
        Texture t;
         // 0 == empty texture, (i - 1) == index into m_textures
        t.textureId = uint(i.key() - m_textures.constData()) + 1;
        qt3ds::render::STextureDetails textureDetails = i.key()->texture->GetTextureDetails();
        t.size = QSize(textureDetails.m_Width, textureDetails.m_Height);
        setGlyphsTexture(i.value(), t);

        QImage &image = i.key()->copy;

        setTextureData(i.key()->texture, image);
    }
}

void Q3DSDistanceFieldGlyphCache::requestGlyphs(const QSet<glyph_t> &glyphs)
{
    // Note: Most of this is copy-pasted from QSGDefaultDistanceFieldGlyphCache in Qt Quick.
    // All of this can probably be shared as a default implementation, since it does not
    // actually create any textures, but it might have to be either templated or based
    // on void*. For now we will just live with the duplication.

    QList<GlyphPosition> glyphPositions;
    QVector<glyph_t> glyphsToRender;

    if (m_areaAllocator == nullptr) {
        m_areaAllocator = new QSGAreaAllocator(QSize(maxTextureSize(),
                                                     m_maxTextureCount * maxTextureSize()));
    }

    for (QSet<glyph_t>::const_iterator it = glyphs.constBegin(); it != glyphs.constEnd() ; ++it) {
        glyph_t glyphIndex = *it;

        int padding = Q3DSDISTANCEFIELDGLYPHCACHE_PADDING;
        QRectF boundingRect = glyphData(glyphIndex).boundingRect;
        int glyphWidth = qCeil(boundingRect.width()) + distanceFieldRadius() * 2;
        int glyphHeight = qCeil(boundingRect.height()) + distanceFieldRadius() * 2;
        QSize glyphSize(glyphWidth + padding * 2, glyphHeight + padding * 2);
        QRect alloc = m_areaAllocator->allocate(glyphSize);

        if (alloc.isNull()) {
            // Unallocate unused glyphs until we can allocated the new glyph
            while (alloc.isNull() && !m_unusedGlyphs.isEmpty()) {
                glyph_t unusedGlyph = *m_unusedGlyphs.constBegin();

                TexCoord unusedCoord = glyphTexCoord(unusedGlyph);
                QRectF unusedGlyphBoundingRect = glyphData(unusedGlyph).boundingRect;
                int unusedGlyphWidth = qCeil(unusedGlyphBoundingRect.width())
                        + distanceFieldRadius() * 2;
                int unusedGlyphHeight = qCeil(unusedGlyphBoundingRect.height())
                        + distanceFieldRadius() * 2;
                m_areaAllocator->deallocate(QRect(int(unusedCoord.x) - padding,
                                                  int(unusedCoord.y) - padding,
                                                  padding * 2 + unusedGlyphWidth,
                                                  padding * 2 + unusedGlyphHeight));

                m_unusedGlyphs.remove(unusedGlyph);
                m_glyphsTexture.remove(unusedGlyph);
                removeGlyph(unusedGlyph);

                alloc = m_areaAllocator->allocate(glyphSize);
            }

            // Not enough space left for this glyph... skip to the next one
            if (alloc.isNull())
                continue;
        }

        TextureInfo *tex = textureInfo(alloc.y() / maxTextureSize());
        alloc = QRect(alloc.x(), alloc.y() % maxTextureSize(), alloc.width(), alloc.height());

        tex->allocatedArea |= alloc;
        Q_ASSERT(tex->padding == padding || tex->padding < 0);
        tex->padding = padding;

        GlyphPosition p;
        p.glyph = glyphIndex;
        p.position = alloc.topLeft() + QPoint(padding, padding);

        glyphPositions.append(p);
        glyphsToRender.append(glyphIndex);
        m_glyphsTexture.insert(glyphIndex, tex);
    }

    setGlyphsPosition(glyphPositions);
    markGlyphsToRender(glyphsToRender);
}

void Q3DSDistanceFieldGlyphCache::processPendingGlyphs()
{
    update();
}

Q3DSDistanceFieldGlyphCache::TextureInfo *Q3DSDistanceFieldGlyphCache::textureInfoById(
        uint id) const
{
    Q_ASSERT(id > 0);
    return textureInfo(id - 1);
}

// This is all copy-pasted from Qt Quick, as sharing it would require some refactoring, and we
// need to work with Qt 5.12.2 at the moment.
namespace {
    struct Qtdf {
        // We need these structs to be tightly packed, but some compilers we use do not
        // support #pragma pack(1), so we need to hardcode the offsets/sizes in the
        // file format
        enum TableSize {
            HeaderSize        = 14,
            GlyphRecordSize   = 46,
            TextureRecordSize = 17
        };

        enum Offset {
            // Header
            majorVersion       = 0,
            minorVersion       = 1,
            pixelSize          = 2,
            textureSize        = 4,
            flags              = 8,
            headerPadding      = 9,
            numGlyphs          = 10,

            // Glyph record
            glyphIndex         = 0,
            textureOffsetX     = 4,
            textureOffsetY     = 8,
            textureWidth       = 12,
            textureHeight      = 16,
            xMargin            = 20,
            yMargin            = 24,
            boundingRectX      = 28,
            boundingRectY      = 32,
            boundingRectWidth  = 36,
            boundingRectHeight = 40,
            textureIndex       = 44,

            // Texture record
            allocatedX         = 0,
            allocatedY         = 4,
            allocatedWidth     = 8,
            allocatedHeight    = 12,
            texturePadding     = 16

        };

        template <typename T>
        static inline T fetch(const char *data, Offset offset)
        {
            return qFromBigEndian<T>(data + int(offset));
        }
    };
}

qreal Q3DSDistanceFieldGlyphCache::fontSize() const
{
    return QT_DISTANCEFIELD_BASEFONTSIZE(m_doubleGlyphResolution);
}

bool Q3DSDistanceFieldGlyphCache::loadPregeneratedCache(const QRawFont &font)
{
    // The pregenerated data must be loaded first, otherwise the area allocator
    // will be wrong
    if (m_areaAllocator != nullptr) {
        qWarning("Font cache must be loaded before cache is used");
        return false;
    }

    QByteArray qtdfTable = font.fontTable("qtdf");
    if (qtdfTable.isEmpty())
        return false;

    using GlyphTextureHash = QHash<TextureInfo *, QVector<glyph_t> >;

    GlyphTextureHash glyphTextures;

    if (uint(qtdfTable.size()) < Qtdf::HeaderSize) {
        qWarning("Invalid qtdf table in font '%s'",
                 qPrintable(font.familyName()));
        return false;
    }

    const char *qtdfTableStart = qtdfTable.constData();
    const char *qtdfTableEnd = qtdfTableStart + qtdfTable.size();

    int padding = 0;
    int textureCount = 0;
    {
        quint8 majorVersion = Qtdf::fetch<quint8>(qtdfTableStart, Qtdf::majorVersion);
        quint8 minorVersion = Qtdf::fetch<quint8>(qtdfTableStart, Qtdf::minorVersion);
        if (majorVersion != 5 || minorVersion != 12) {
            qWarning("Invalid version of qtdf table %d.%d in font '%s'",
                     majorVersion,
                     minorVersion,
                     qPrintable(font.familyName()));
            return false;
        }

        qreal pixelSize = qreal(Qtdf::fetch<quint16>(qtdfTableStart, Qtdf::pixelSize));
        m_maxTextureSize = int(Qtdf::fetch<quint32>(qtdfTableStart, Qtdf::textureSize));
        m_doubleGlyphResolution = Qtdf::fetch<quint8>(qtdfTableStart, Qtdf::flags) == 1;
        padding = Qtdf::fetch<quint8>(qtdfTableStart, Qtdf::headerPadding);

        if (pixelSize <= 0.0) {
            qWarning("Invalid pixel size in '%s'", qPrintable(font.familyName()));
            return false;
        }

        if (m_maxTextureSize <= 0) {
            qWarning("Invalid texture size in '%s'", qPrintable(font.familyName()));
            return false;
        }

        if (padding != Q3DSDISTANCEFIELDGLYPHCACHE_PADDING) {
            qWarning("Padding mismatch in '%s'. Font requires %d, but Qt is compiled with %d.",
                     qPrintable(font.familyName()),
                     padding,
                     Q3DSDISTANCEFIELDGLYPHCACHE_PADDING);
        }

        m_referenceFont.setPixelSize(pixelSize);

        quint32 glyphCount = Qtdf::fetch<quint32>(qtdfTableStart, Qtdf::numGlyphs);
        m_unusedGlyphs.reserve(int(glyphCount));

        const char *allocatorData = qtdfTableStart + Qtdf::HeaderSize;
        {
            m_areaAllocator = new QSGAreaAllocator(QSize(0, 0));
            allocatorData = m_areaAllocator->deserialize(allocatorData,
                                                         qtdfTableEnd - allocatorData);
            if (allocatorData == nullptr)
                return false;
        }

        if (m_areaAllocator->size().height() % m_maxTextureSize != 0) {
            qWarning("Area allocator size mismatch in '%s'", qPrintable(font.familyName()));
            return false;
        }

        textureCount = m_areaAllocator->size().height() / m_maxTextureSize;
        m_maxTextureCount = qMax(m_maxTextureCount, textureCount);

        const char *textureRecord = allocatorData;
        for (int i = 0; i < textureCount; ++i, textureRecord += Qtdf::TextureRecordSize) {
            if (textureRecord + Qtdf::TextureRecordSize > qtdfTableEnd) {
                qWarning("qtdf table too small in font '%s'.",
                         qPrintable(font.familyName()));
                return false;
            }

            TextureInfo *tex = textureInfo(i);
            tex->allocatedArea.setX(int(Qtdf::fetch<quint32>(textureRecord, Qtdf::allocatedX)));
            tex->allocatedArea.setY(int(Qtdf::fetch<quint32>(textureRecord, Qtdf::allocatedY)));
            tex->allocatedArea.setWidth(int(Qtdf::fetch<quint32>(textureRecord,
                                                                 Qtdf::allocatedWidth)));
            tex->allocatedArea.setHeight(int(Qtdf::fetch<quint32>(textureRecord,
                                                                  Qtdf::allocatedHeight)));
            tex->padding = Qtdf::fetch<quint8>(textureRecord, Qtdf::texturePadding);
        }

        const char *glyphRecord = textureRecord;
        for (quint32 i = 0; i < glyphCount; ++i, glyphRecord += Qtdf::GlyphRecordSize) {
            if (glyphRecord + Qtdf::GlyphRecordSize > qtdfTableEnd) {
                qWarning("qtdf table too small in font '%s'.",
                         qPrintable(font.familyName()));
                return false;
            }

            glyph_t glyph = Qtdf::fetch<quint32>(glyphRecord, Qtdf::glyphIndex);
            m_unusedGlyphs.insert(glyph);

            GlyphData &glyphData = emptyData(glyph);

#define FROM_FIXED_POINT(value) (qreal(value)/qreal(65536))

            glyphData.texCoord.x
                    = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::textureOffsetX));
            glyphData.texCoord.y
                    = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::textureOffsetY));
            glyphData.texCoord.width
                    = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::textureWidth));
            glyphData.texCoord.height
                    = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::textureHeight));
            glyphData.texCoord.xMargin
                    = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::xMargin));
            glyphData.texCoord.yMargin
                    = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::yMargin));
            glyphData.boundingRect.setX(
                        FROM_FIXED_POINT(Qtdf::fetch<qint32>(glyphRecord, Qtdf::boundingRectX)));
            glyphData.boundingRect.setY(
                        FROM_FIXED_POINT(Qtdf::fetch<qint32>(glyphRecord, Qtdf::boundingRectY)));
            glyphData.boundingRect.setWidth(
                        FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord,
                                                              Qtdf::boundingRectWidth)));
            glyphData.boundingRect.setHeight(
                        FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord,
                                                              Qtdf::boundingRectHeight)));

#undef FROM_FIXED_POINT

            int textureIndex = Qtdf::fetch<quint16>(glyphRecord, Qtdf::textureIndex);
            if (textureIndex < 0 || textureIndex >= textureCount) {
                qWarning("Invalid texture index %d (texture count == %d) in '%s'",
                         textureIndex,
                         textureCount,
                         qPrintable(font.familyName()));
                return false;
            }


            TextureInfo *texInfo = textureInfo(textureIndex);
            m_glyphsTexture.insert(glyph, texInfo);

            glyphTextures[texInfo].append(glyph);
        }

        const uchar *textureData = reinterpret_cast<const uchar *>(glyphRecord);
        for (int i = 0; i < textureCount; ++i) {

            TextureInfo *texInfo = textureInfo(i);

            int width = texInfo->allocatedArea.width();
            int height = texInfo->allocatedArea.height();
            qint64 size = width * height;
            if (reinterpret_cast<const char *>(textureData + size) > qtdfTableEnd) {
                qWarning("qtdf table too small in font '%s'.",
                         qPrintable(font.familyName()));
                return false;
            }

            resizeTexture(texInfo, width, height);

            for (int y = 0; y < height; ++y)
                memcpy(texInfo->copy.scanLine(y), textureData + y * width, width);
            textureData += size;

            QImage &image = texInfo->copy;
            setTextureData(texInfo->texture, image);

            QVector<glyph_t> glyphs = glyphTextures.value(texInfo);

            Texture t;
            t.textureId = uint(i + 1);
            t.size = texInfo->copy.size();

            setGlyphsTexture(glyphs, t);
        }
    }

    return true;
}

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
bool Q3DSDistanceFieldGlyphCache::eightBitFormatIsAlphaSwizzled() const
{
    return m_context.GetRenderContext().GetRenderContextType()
        == qt3ds::render::NVRenderContextValues::GLES2;
}
#endif // QT_VERSION >= QT_VERSION_CHECK(5,14,0)

QT_END_NAMESPACE

#endif // QT_VERSION >= QT_VERSION_CHECK(5,12,2)
