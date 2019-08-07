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

#include "Qt3DSDistanceFieldRenderer.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)

#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderContext.h"
#include "qmath.h"
#include "foundation/Qt3DSAllocator.h"

using namespace qt3ds::render;

Q3DSDistanceFieldRenderer::Q3DSDistanceFieldRenderer(NVFoundationBase &foundation)
    : m_foundation(foundation)
{
    const QWindowList list = QGuiApplication::topLevelWindows();
    if (list.size() > 0)
        m_pixelRatio = list[0]->devicePixelRatio();
}

Q3DSDistanceFieldRenderer::~Q3DSDistanceFieldRenderer()
{
    NVAllocatorCallback &alloc = m_context->GetAllocator();

    QHash<size_t, Q3DSDistanceFieldMesh>::const_iterator it;
    for (it = m_meshCache.constBegin(); it != m_meshCache.constEnd(); ++it) {
        const Q3DSDistanceFieldMesh &mesh = it.value();
        NVDelete(alloc, mesh.vertexBuffer);
        NVDelete(alloc, mesh.indexBuffer);
        NVDelete(alloc, mesh.inputAssembler);
    }
}

void Q3DSDistanceFieldRenderer::AddSystemFontDirectory(const char8_t *dir)
{
    QString systemDir(dir);
    m_systemDirs += systemDir;
    m_fontDatabase.registerFonts({ systemDir });
}

void Q3DSDistanceFieldRenderer::AddProjectFontDirectory(const char8_t *dir)
{
    QString projectDir(dir);
    m_projectDirs += projectDir;
    m_fontDatabase.registerFonts({ projectDir });
}

void Q3DSDistanceFieldRenderer::ClearProjectFontDirectories()
{
    m_fontDatabase.unregisterFonts(m_projectDirs);
    m_projectDirs.clear();
}

void Q3DSDistanceFieldRenderer::ReloadFonts()
{
    m_fontDatabase.unregisterFonts(m_projectDirs);
    m_fontDatabase.registerFonts(m_projectDirs);
}

ITextRenderer &Q3DSDistanceFieldRenderer::GetTextRenderer(NVRenderContext &)
{
    return *this;
}

void Q3DSDistanceFieldRenderer::EndFrame()
{
    // Remove meshes for glyphs that weren't rendered last frame
    NVAllocatorCallback &alloc = m_context->GetAllocator();

    QHash<size_t, QHash<Q3DSDistanceFieldGlyphCache::TextureInfo *, GlyphInfo>>::const_iterator
            glyphIt = m_glyphCache.constBegin();
    while (glyphIt != m_glyphCache.constEnd()) {
        const size_t textHash = glyphIt.key();
        if (!m_renderedTexts.contains(textHash))
            m_glyphCache.erase(glyphIt++);
        else
            glyphIt++;
    }

    QHash<size_t, Q3DSDistanceFieldMesh>::const_iterator meshIt = m_meshCache.constBegin();
    while (meshIt != m_meshCache.constEnd()) {
        const size_t glyphHash = meshIt.key();
        const Q3DSDistanceFieldMesh &mesh = meshIt.value();
        if (!m_renderedGlyphs.contains(glyphHash)) {
            NVDelete(alloc, mesh.vertexBuffer);
            NVDelete(alloc, mesh.indexBuffer);
            NVDelete(alloc, mesh.inputAssembler);
            m_meshCache.erase(meshIt++);
        } else {
            meshIt++;
        }
    }

    m_renderedGlyphs.clear();
    m_renderedTexts.clear();
}

QHash<Q3DSDistanceFieldGlyphCache::TextureInfo *, GlyphInfo>
Q3DSDistanceFieldRenderer::buildGlyphsPerTexture(const SText &textInfo)
{
    if (textInfo.m_BoundingBox.x < 0 || textInfo.m_BoundingBox.y < 0)
        return QHash<Q3DSDistanceFieldGlyphCache::TextureInfo *, GlyphInfo>();

    QVector2D boundingBox = QVector2D(textInfo.m_BoundingBox.x, textInfo.m_BoundingBox.y);
    const float halfWidth = boundingBox.x() / 2.0f;
    const float halfHeight = boundingBox.y() / 2.0f;
    bool hasValidBoundingBox = boundingBox.x() > 0 || boundingBox.y() > 0;

    QRawFont font = m_fontDatabase.findFont(textInfo.m_Font.c_str());
    qreal scaleFactor = font.pixelSize() / qreal(textInfo.m_FontSize);

    const qreal maximumWidth = boundingBox.isNull() ? qreal(0x01000000)
                                                    : qreal(boundingBox.x()) * scaleFactor;
    const qreal maximumHeight = boundingBox.isNull() ? qreal(0x01000000)
                                                     : qreal(boundingBox.y()) * scaleFactor;

    QTextLayout layout;
    QTextOption option = layout.textOption();

    QTextOption::WrapMode wrapMode;
    switch (textInfo.m_WordWrap) {
    case TextWordWrap::Clip:
        wrapMode = QTextOption::ManualWrap;
        break;
    case TextWordWrap::WrapWord:
        wrapMode = QTextOption::WrapAtWordBoundaryOrAnywhere;
        break;
    case TextWordWrap::WrapAnywhere:
        wrapMode = QTextOption::WrapAnywhere;
        break;
    case TextWordWrap::Unknown:
        wrapMode = QTextOption::ManualWrap;
        Q_ASSERT(0);
    };
    option.setWrapMode(wrapMode);
    option.setUseDesignMetrics(true);

    layout.setTextOption(option);
    layout.setRawFont(font);

    QString text = textInfo.m_Text.c_str();
    text.replace(QLatin1Char('\n'), QChar::LineSeparator);

    qreal width;
    qreal height;
    bool needsElide;
    do {
        needsElide = false;

        layout.clearLayout();

        qreal leading = qreal(textInfo.m_Leading) * scaleFactor;
        width = 0.0;
        height = -leading;

        QVector<QTextLayout::FormatRange> formatRanges;

        QTextLayout::FormatRange formatRange;
        formatRange.start = 0;
        formatRange.length = text.length();
        formatRange.format.setFontLetterSpacingType(QFont::AbsoluteSpacing);
        formatRange.format.setFontLetterSpacing(qreal(textInfo.m_Tracking) * scaleFactor);
        formatRanges.append(formatRange);
        layout.setFormats(formatRanges);

        layout.setText(text);
        layout.beginLayout();

        QTextLine previousLine;
        forever {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;

            line.setLineWidth(maximumWidth);
            height += leading;
            height = qCeil(height);

            qreal textWidth = line.naturalTextWidth();
            line.setPosition(QPointF(0.0, height));

            width = qMin(maximumWidth, qMax(width, textWidth));
            height += layout.engine()->lines[line.lineNumber()].height().toReal();

            // Fast path for right elide
            if (textInfo.m_Elide == TextElide::ElideRight
                    && previousLine.isValid()
                    && height > maximumHeight) {
                break;
            }

            previousLine = line;
        }
        layout.endLayout();

        if (textInfo.m_Elide != TextElide::ElideNone && height > maximumHeight) {
            needsElide = true;

            QString elidedText;
            switch (textInfo.m_Elide) {
            case TextElide::ElideRight:
                if (previousLine.textStart() > 0)
                    elidedText = text.left(previousLine.textStart());

                elidedText += layout.engine()->elidedText(
                            Qt::ElideRight, QFixed::fromReal(width), 0, previousLine.textStart(),
                            text.length() - previousLine.textStart());
                break;
            case TextElide::ElideLeft:
            {
                height = 0.0;
                previousLine = QTextLine();
                for (int i = layout.lineCount() - 1; i >= 0; --i) {
                    qreal lineHeight = layout.lineAt(i).height();
                    if (i < layout.lineCount() - 1 && height + lineHeight > maximumHeight)
                        break;
                    height += lineHeight;
                    previousLine = layout.lineAt(i);
                }

                Q_ASSERT(previousLine.isValid());
                elidedText += layout.engine()->elidedText(
                            Qt::ElideLeft, QFixed::fromReal(width), 0, 0,
                            previousLine.textStart() + previousLine.textLength());

                int nextPosition = (previousLine.textStart() + previousLine.textLength());
                if (nextPosition < text.length())
                    elidedText += text.mid(nextPosition);
                break;
            }
            case TextElide::ElideMiddle:
            {
                height = 0.0;
                QTextLine lastLineBefore;
                QTextLine firstLineAfter;
                for (int i = 0; i < (layout.lineCount() / 2) + (layout.lineCount() % 2); ++i) {
                    qreal lineHeight = 3 * layout.lineAt(i).height();
                    if (height + lineHeight > maximumHeight)
                        break;
                    height += lineHeight;

                    lastLineBefore = layout.lineAt(i);
                    firstLineAfter = layout.lineAt(layout.lineCount() - i - 1);
                }

                int nextPosition = 0;
                if (lastLineBefore.isValid()) {
                    elidedText += text.left(lastLineBefore.textStart()
                                            + lastLineBefore.textLength());
                    nextPosition = lastLineBefore.textStart() + lastLineBefore.textLength();
                }

                QString suffix;
                int length = text.length() - nextPosition;
                if (firstLineAfter.isValid()) {
                    length = firstLineAfter.textStart() - nextPosition;
                    suffix = text.mid(firstLineAfter.textStart());
                }

                elidedText += layout.engine()->elidedText(
                            Qt::ElideMiddle, QFixed::fromReal(width), 0, nextPosition, length);

                elidedText += suffix;
                break;
            }
            case TextElide::ElideNone:
                Q_UNREACHABLE();
            };

            // Failsafe
            if (elidedText.isEmpty() || elidedText == text)
                needsElide = false;
            else
                text = elidedText;
        }
    } while (needsElide);

    QHash<Q3DSDistanceFieldGlyphCache::TextureInfo *, GlyphInfo> glyphsPerTexture;

    float originY = float(height) / 2.0f;
    if (textInfo.m_VerticalAlignment == TextVerticalAlignment::Bottom)
        originY = float(height);
    else if (textInfo.m_VerticalAlignment == TextVerticalAlignment::Top)
        originY = 0.0;

    float originX = float(width) / 2.0f;
    if (textInfo.m_HorizontalAlignment == TextHorizontalAlignment::Right)
        originX = float(width);
    else if (textInfo.m_HorizontalAlignment == TextHorizontalAlignment::Left)
        originX = 0.0;

    float offsetY = -originY;

    QT3DSVec3 minimum(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 0);
    QT3DSVec3 maximum(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), 0);

    // To match the original behavior of the sources, we don't actually align to
    // the bounding box. This is only used for word wrapping and elide. Keeping the
    // code here in case this was a mistake or we for some other reason want to change
    // it.
#if 0
   // If there is no bounding box, then alignmentHeight == height, so we skip it
   if (!boundingBox.isNull() && text3DS->verticalAlignment() == Q3DSTextNode::Bottom)
        offsetY += float(maximumHeight - height);
    else if (!boundingBox.isNull() && text3DS->verticalAlignment() == Q3DSTextNode::Middle)
        offsetY += float(maximumHeight / 2.0 - height / 2.0);
    float alignmentWidth = boundingBox.isNull() ? float(width) : float(maximumWidth);
#else
    float alignmentWidth = float(width);
#endif

    for (int j = 0; j < layout.lineCount(); ++j) {
        QTextLine line = layout.lineAt(j);

        float offsetX = -originX;
        if (textInfo.m_HorizontalAlignment == TextHorizontalAlignment::Right)
            offsetX += alignmentWidth - float(line.naturalTextWidth());
        else if (textInfo.m_HorizontalAlignment == TextHorizontalAlignment::Center)
            offsetX += alignmentWidth / 2.0f - float(line.naturalTextWidth()) / 2.0f;

        const QList<QGlyphRun> glyphRuns = line.glyphRuns();
        for (const QGlyphRun &glyphRun : glyphRuns) {
            const QVector<quint32> glyphIndexes = glyphRun.glyphIndexes();
            const QVector<QPointF> glyphPositions = glyphRun.positions();

            Q3DSDistanceFieldGlyphCache *cache = m_glyphCacheManager.glyphCache(
                        glyphRun.rawFont());
            cache->populate(glyphRun.glyphIndexes());
            cache->processPendingGlyphs();

            qreal fontPixelSize = glyphRun.rawFont().pixelSize();

            qreal shadowOffsetX = qreal(cache->fontSize())
                    * qreal(textInfo.m_DropShadowOffsetX) / 1000.0;
            qreal shadowOffsetY = qreal(cache->fontSize())
                    * qreal(textInfo.m_DropShadowOffsetY) / 1000.0;

            qreal maxTexMargin = cache->distanceFieldRadius();
            qreal fontScale = cache->fontScale(fontPixelSize);
            qreal margin = 2;
            qreal texMargin = margin / fontScale;
            if (texMargin > maxTexMargin) {
                texMargin = maxTexMargin;
                margin = maxTexMargin * fontScale;
            }

            for (int i = 0; i < glyphIndexes.size(); ++i) {
                quint32 glyphIndex = glyphIndexes.at(i);
                QPointF position = glyphPositions.at(i);

                QSGDistanceFieldGlyphCache::TexCoord c = cache->glyphTexCoord(glyphIndex);
                if (c.isNull())
                    continue;

                QSGDistanceFieldGlyphCache::Metrics metrics = cache->glyphMetrics(glyphIndex,
                                                                                  fontPixelSize);
                if (metrics.isNull())
                    continue;

                metrics.width += margin * 2;
                metrics.height += margin * 2;
                metrics.baselineX -= margin;
                metrics.baselineY += margin;
                c.xMargin -= texMargin;
                c.yMargin -= texMargin;
                c.width += texMargin * 2;
                c.height += texMargin * 2;

                float cx1 = float(position.x() + metrics.baselineX) + offsetX;
                float cx2 = cx1 + float(metrics.width);
                float cy1 = float(position.y() - metrics.baselineY) + offsetY;
                float cy2 = cy1 + float(metrics.height);

                cx1 /= float(scaleFactor);
                cy1 /= float(scaleFactor);
                cx2 /= float(scaleFactor);
                cy2 /= float(scaleFactor);

                if (textInfo.m_DropShadow) {
                    if (shadowOffsetX < 0.0)
                        cx1 += float(shadowOffsetX * fontScale);
                    else
                        cx2 += float(shadowOffsetX * fontScale);

                    if (shadowOffsetY < 0.0)
                        cy1 += float(shadowOffsetY * fontScale);
                    else
                        cy2 += float(shadowOffsetY * fontScale);
                }

                float x1Clip = 1.0f;
                float x2Clip = 1.0f;
                float y1Clip = 1.0f;
                float y2Clip = 1.0f;

                if (hasValidBoundingBox) {
                    if ((cx1 < -halfWidth && cx2 < -halfWidth)
                            || (cx1 > halfWidth && cx2 > halfWidth)
                            || (cy1 < -halfHeight && cy2 < -halfHeight)
                            || (cy1 > halfHeight && cy2 > halfHeight)) {
                        continue;
                    }

                    float xDiff = qAbs(cx1 - cx2);
                    float yDiff = qAbs(cy1 - cy2);

                    if (cx1 < -halfWidth) {
                        x1Clip = 1.0f - qAbs(cx1 - (-halfWidth)) / xDiff;
                        cx1 = -halfWidth;
                    }

                    if (cx2 > halfWidth) {
                        x2Clip = 1.0f - qAbs(cx2 - halfWidth) / xDiff;
                        cx2 = halfWidth;
                    }

                    if (cy1 < -halfHeight) {
                        y1Clip = 1.0f - qAbs(cy1 - (-halfHeight)) / yDiff;
                        cy1 = -halfHeight;
                    }

                    if (cy2 > halfHeight) {
                        y2Clip = 1.0f - qAbs(cy2 - halfHeight) / yDiff;
                        cy2 = halfHeight;
                    }
                }

                cy1 = -cy1;
                cy2 = -cy2;

                if (cx1 < minimum.x)
                    minimum.x = cx1;
                else if (cx1 > maximum.x)
                    maximum.x = cx1;

                if (cx2 < minimum.x)
                    minimum.x = cx2;
                else if (cx2 > maximum.x)
                    maximum.x = cx2;

                // Note: Handle cy2 before cy1 because it is smaller due to y-coordinate switch.
                // This way both minimum & maximum will get value even with single glyph text.
                if (cy2 < minimum.y)
                    minimum.y = cy2;
                else if (cy2 > maximum.y)
                    maximum.y = cy2;

                if (cy1 < minimum.y)
                    minimum.y = cy1;
                else if (cy1 > maximum.y)
                    maximum.y = cy1;

                if (hasValidBoundingBox) {
                    if (maximum.x < halfWidth)
                        maximum.x = halfWidth;
                    if (minimum.x > -halfWidth)
                        minimum.x = -halfWidth;
                    if (maximum.y < halfHeight)
                        maximum.y = halfHeight;
                    if (minimum.y > -halfHeight)
                        minimum.y = -halfHeight;
                }

                float tx1 = float(c.x + c.xMargin);
                float tx2 = tx1 + float(c.width);
                float ty1 = float(c.y + c.yMargin);
                float ty2 = ty1 + float(c.height);

                // Preserve original bounds of glyphs
                float ttx1 = tx1;
                float tty1 = ty1;
                float ttx2 = tx2;
                float tty2 = ty2;

                if (textInfo.m_DropShadow) {
                    if (shadowOffsetX < 0.0)
                        tx1 += float(c.width * shadowOffsetX * fontScale) / float(metrics.width);
                    else
                        tx2 += float(c.width * shadowOffsetX * fontScale) / float(metrics.width);

                    if (shadowOffsetY < 0.0)
                        ty1 += float(c.height * shadowOffsetY * fontScale) / float(metrics.height);
                    else
                        ty2 += float(c.height * shadowOffsetY * fontScale) / float(metrics.height);
                }

                if (hasValidBoundingBox) {
                    float tx1Orig = tx1;
                    float tx2Orig = tx2;
                    float ty1Orig = ty1;
                    float ty2Orig = ty2;

                    float xDiff = qAbs(tx1 - tx2);
                    tx1 = tx2Orig - xDiff * x1Clip;
                    tx2 = tx1Orig + xDiff * x2Clip;

                    float yDiff = qAbs(ty1 - ty2);
                    ty1 = ty2Orig - yDiff * y1Clip;
                    ty2 = ty1Orig + yDiff * y2Clip;
                }

                const QSGDistanceFieldGlyphCache::Texture *texture
                        = cache->glyphTexture(glyphIndex);
                if (texture->textureId == 0) {
                    qWarning() << "Empty texture for glyph" << glyphIndex;
                    continue;
                }

                Q3DSDistanceFieldGlyphCache::TextureInfo *textureInfo = cache->textureInfoById(
                            texture->textureId);

                GlyphInfo &glyphInfo = glyphsPerTexture[textureInfo];
                glyphInfo.fontScale = float(fontScale);
                glyphInfo.shadowOffsetX = float(shadowOffsetX);
                glyphInfo.shadowOffsetY = float(shadowOffsetY);
                glyphInfo.bounds = NVBounds3(minimum, maximum);

                QVector<float> &vertexes = glyphInfo.vertexes;
                vertexes.reserve(vertexes.size() + 20 + (textInfo.m_DropShadow ? 16 : 0));

                vertexes.append(cx1);
                vertexes.append(cy1);
                vertexes.append(0.0);
                vertexes.append(tx1);
                vertexes.append(ty1);

                if (textInfo.m_DropShadow) {
                    vertexes.append(ttx1);
                    vertexes.append(tty1);
                    vertexes.append(ttx2);
                    vertexes.append(tty2);
                }

                vertexes.append(cx2);
                vertexes.append(cy1);
                vertexes.append(0.0);
                vertexes.append(tx2);
                vertexes.append(ty1);

                if (textInfo.m_DropShadow) {
                    vertexes.append(ttx1);
                    vertexes.append(tty1);
                    vertexes.append(ttx2);
                    vertexes.append(tty2);
                }

                vertexes.append(cx2);
                vertexes.append(cy2);
                vertexes.append(0.0);
                vertexes.append(tx2);
                vertexes.append(ty2);

                if (textInfo.m_DropShadow) {
                    vertexes.append(ttx1);
                    vertexes.append(tty1);
                    vertexes.append(ttx2);
                    vertexes.append(tty2);
                }

                vertexes.append(cx1);
                vertexes.append(cy2);
                vertexes.append(0.0);
                vertexes.append(tx1);
                vertexes.append(ty2);

                if (textInfo.m_DropShadow) {
                    vertexes.append(ttx1);
                    vertexes.append(tty1);
                    vertexes.append(ttx2);
                    vertexes.append(tty2);
                }
            }
        }
    }

    return glyphsPerTexture;
}

template <typename T>
static QVector<T> fillIndexBuffer(uint quadCount)
{
    QVector<T> indexes;

    const uint triangleCount = 2 * quadCount;
    indexes.resize(3 * int(triangleCount));

    Q_ASSERT(indexes.size() % 6 == 0);

    for (uint i = 0; i < quadCount; i ++) {
        indexes[int(i * 6 + 0)] = T(i * 4 + 0);
        indexes[int(i * 6 + 1)] = T(i * 4 + 3);
        indexes[int(i * 6 + 2)] = T(i * 4 + 1);

        indexes[int(i * 6 + 3)] = T(i * 4 + 1);
        indexes[int(i * 6 + 4)] = T(i * 4 + 3);
        indexes[int(i * 6 + 5)] = T(i * 4 + 2);
    }

    return indexes;
}

void Q3DSDistanceFieldRenderer::buildShaders()
{
    if (m_shader.program)
        return;
    IShaderProgramGenerator &gen = m_context->GetShaderProgramGenerator();
    gen.BeginProgram();
    IShaderStageGenerator &vertexGenerator(*gen.GetStage(ShaderGeneratorStages::Vertex));
    IShaderStageGenerator &fragmentGenerator(*gen.GetStage(ShaderGeneratorStages::Fragment));

    if (m_context->GetRenderContext().GetRenderContextType() == NVRenderContextValues::GLES2) {
        vertexGenerator.AddInclude("distancefieldtext.vert");
        fragmentGenerator.AddInclude("distancefieldtext.frag");
    } else {
        vertexGenerator.AddInclude("distancefieldtext_core.vert");
        fragmentGenerator.AddInclude("distancefieldtext_core.frag");
    }

    m_shader.program = gen.CompileGeneratedShader("distancefieldtext",
                                                  SShaderCacheProgramFlags(),
                                                  TShaderFeatureSet(), false);

    if (m_shader.program) {
        m_shader.mvp = NVRenderCachedShaderProperty<QT3DSMat44>(
                    "mvp", *m_shader.program);
        m_shader.textureWidth = NVRenderCachedShaderProperty<QT3DSI32>(
                    "textureWidth", *m_shader.program);
        m_shader.textureHeight = NVRenderCachedShaderProperty<QT3DSI32>(
                    "textureHeight", *m_shader.program);
        m_shader.fontScale = NVRenderCachedShaderProperty<QT3DSF32>(
                    "fontScale", *m_shader.program);
        m_shader.texture = NVRenderCachedShaderProperty<NVRenderTexture2D *>(
                    "_qt_texture", *m_shader.program);
        m_shader.color = NVRenderCachedShaderProperty<QT3DSVec4>(
                    "color", *m_shader.program);
    }

    gen.BeginProgram();
    vertexGenerator = *gen.GetStage(ShaderGeneratorStages::Vertex);
    fragmentGenerator = *gen.GetStage(ShaderGeneratorStages::Fragment);

    if (m_context->GetRenderContext().GetRenderContextType() == NVRenderContextValues::GLES2) {
        vertexGenerator.AddInclude("distancefieldtext_dropshadow.vert");
        fragmentGenerator.AddInclude("distancefieldtext_dropshadow.frag");
    } else {
        vertexGenerator.AddInclude("distancefieldtext_dropshadow_core.vert");
        fragmentGenerator.AddInclude("distancefieldtext_dropshadow_core.frag");
    }

    m_dropShadowShader.program = gen.CompileGeneratedShader("distancefieldtext_dropshadow",
                                                            SShaderCacheProgramFlags(),
                                                            TShaderFeatureSet(), false);

    if (m_dropShadowShader.program) {
        m_dropShadowShader.mvp = NVRenderCachedShaderProperty<QT3DSMat44>(
                    "mvp", *m_dropShadowShader.program);
        m_dropShadowShader.textureWidth = NVRenderCachedShaderProperty<QT3DSI32>(
                    "textureWidth", *m_dropShadowShader.program);
        m_dropShadowShader.textureHeight = NVRenderCachedShaderProperty<QT3DSI32>(
                    "textureHeight", *m_dropShadowShader.program);
        m_dropShadowShader.fontScale = NVRenderCachedShaderProperty<QT3DSF32>(
                    "fontScale", *m_dropShadowShader.program);
        m_dropShadowShader.shadowOffset = NVRenderCachedShaderProperty<QT3DSVec2>(
                    "shadowOffset", *m_dropShadowShader.program);
        m_dropShadowShader.texture = NVRenderCachedShaderProperty<NVRenderTexture2D *>(
                    "_qt_texture", *m_dropShadowShader.program);
        m_dropShadowShader.color = NVRenderCachedShaderProperty<QT3DSVec4>(
                    "color", *m_dropShadowShader.program);
        m_dropShadowShader.shadowColor = NVRenderCachedShaderProperty<QT3DSVec4>(
                    "shadowColor", *m_dropShadowShader.program);
    }
}

Q3DSDistanceFieldMesh Q3DSDistanceFieldRenderer::buildMesh(const GlyphInfo &glyphInfo,
                                                           bool shadow)
{
    NVRenderVertexBufferEntry entries[] = {
        NVRenderVertexBufferEntry("vCoord", NVRenderComponentTypes::QT3DSF32, 3),
        NVRenderVertexBufferEntry("tCoord", NVRenderComponentTypes::QT3DSF32, 2, 3 * sizeof(float))
    };

    NVRenderVertexBufferEntry shadowEntries[] = {
        NVRenderVertexBufferEntry("vCoord", NVRenderComponentTypes::QT3DSF32, 3),
        NVRenderVertexBufferEntry("tCoord", NVRenderComponentTypes::QT3DSF32, 2,
        3 * sizeof(float)),
        NVRenderVertexBufferEntry("textureBounds", NVRenderComponentTypes::QT3DSF32, 4,
        5 * sizeof(float))
    };

    const uint floatsPerVertex = 3 + 2 + (shadow ? 4 : 0);
    const uint stride = floatsPerVertex * sizeof(float);
    const uint offset = 0;

    NVRenderContext &renderContext = m_context->GetRenderContext();
    QVector<float> vertexes = glyphInfo.vertexes;

    Q_ASSERT(uint(vertexes.size()) % floatsPerVertex == 0);
    const uint vertexCount = uint(vertexes.size()) / floatsPerVertex;

    Q_ASSERT(vertexCount % 4 == 0);
    const uint quadCount = vertexCount / 4;

    Q3DSDistanceFieldMesh mesh;

    mesh.attribLayout = renderContext.CreateAttributeLayout(
                toConstDataRef(shadow ? shadowEntries : entries, shadow ? 3 : 2));
    mesh.vertexBuffer = renderContext.CreateVertexBuffer(
                NVRenderBufferUsageType::Static, size_t(vertexes.size()) * sizeof(float), stride,
                toU8DataRef(vertexes.begin(), QT3DSU32(vertexes.size())));

    if (vertexCount <= 0xffff) {
        QVector<QT3DSU16> indexes = fillIndexBuffer<QT3DSU16>(quadCount);
        mesh.indexBuffer = renderContext.CreateIndexBuffer(
                    NVRenderBufferUsageType::Static, NVRenderComponentTypes::QT3DSU16,
                    size_t(indexes.size()) * sizeof(QT3DSU16),
                    toU8DataRef(indexes.begin(), QT3DSU32(indexes.size())));
    } else {
        QVector<QT3DSU32> indexes = fillIndexBuffer<QT3DSU32>(quadCount);
        mesh.indexBuffer = renderContext.CreateIndexBuffer(
                    NVRenderBufferUsageType::Static, NVRenderComponentTypes::QT3DSU32,
                    size_t(indexes.size()) * sizeof(QT3DSU32),
                    toU8DataRef(indexes.begin(), QT3DSU32(indexes.size())));
    }

    mesh.inputAssembler = renderContext.CreateInputAssembler(
                mesh.attribLayout, toConstDataRef(&mesh.vertexBuffer, 1), mesh.indexBuffer,
                toConstDataRef(&stride, 1), toConstDataRef(&offset, 1));

    return mesh;
}

void Q3DSDistanceFieldRenderer::renderMesh(
        NVRenderInputAssembler *inputAssembler, NVRenderTexture2D *texture, const QT3DSMat44 &mvp,
        QT3DSI32 textureWidth, QT3DSI32 textureHeight, QT3DSF32 fontScale, QT3DSVec4 color)
{
    NVRenderContext &renderContext = m_context->GetRenderContext();
    renderContext.SetCullingEnabled(false);

    renderContext.SetBlendFunction(NVRenderBlendFunctionArgument(
                                       NVRenderSrcBlendFunc::One,
                                       NVRenderDstBlendFunc::OneMinusSrcAlpha,
                                       NVRenderSrcBlendFunc::One,
                                       NVRenderDstBlendFunc::OneMinusSrcAlpha));
    renderContext.SetBlendEquation(NVRenderBlendEquationArgument(
                                       NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));

    renderContext.SetActiveShader(m_shader.program);
    m_shader.mvp.Set(mvp);
    m_shader.textureWidth.Set(textureWidth);
    m_shader.textureHeight.Set(textureHeight);
    m_shader.fontScale.Set(fontScale);
    m_shader.texture.Set(texture);
    m_shader.color.Set(color);

    renderContext.SetInputAssembler(inputAssembler);
    renderContext.Draw(NVRenderDrawMode::Triangles, inputAssembler->GetIndexCount(), 0);
}

void Q3DSDistanceFieldRenderer::renderMeshWithDropShadow(
        NVRenderInputAssembler *inputAssembler, NVRenderTexture2D *texture, const QT3DSMat44 &mvp,
        QT3DSI32 textureWidth, QT3DSI32 textureHeight, QT3DSF32 fontScale, QT3DSVec2 shadowOffset,
        QT3DSVec4 color, QT3DSVec4 shadowColor)
{
    NVRenderContext &renderContext = m_context->GetRenderContext();
    renderContext.SetCullingEnabled(false);

    renderContext.SetBlendFunction(NVRenderBlendFunctionArgument(
                                       NVRenderSrcBlendFunc::One,
                                       NVRenderDstBlendFunc::OneMinusSrcAlpha,
                                       NVRenderSrcBlendFunc::One,
                                       NVRenderDstBlendFunc::OneMinusSrcAlpha));
    renderContext.SetBlendEquation(NVRenderBlendEquationArgument(
                                       NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));

    renderContext.SetActiveShader(m_dropShadowShader.program);
    m_dropShadowShader.mvp.Set(mvp);
    m_dropShadowShader.textureWidth.Set(textureWidth);
    m_dropShadowShader.textureHeight.Set(textureHeight);
    m_dropShadowShader.fontScale.Set(fontScale);
    m_dropShadowShader.shadowOffset.Set(shadowOffset);
    m_dropShadowShader.texture.Set(texture);
    m_dropShadowShader.color.Set(color);
    m_dropShadowShader.shadowColor.Set(shadowColor);

    renderContext.SetInputAssembler(inputAssembler);
    renderContext.Draw(NVRenderDrawMode::Triangles, inputAssembler->GetIndexCount(), 0);
}

namespace std {
template<class T>
struct hash<QVector<T>>
{
    size_t operator()(const QVector<T> &s) const
    {
        return qHash(s);
    }
};

template<>
struct hash<TextHorizontalAlignment::Enum>
{
    size_t operator()(const TextHorizontalAlignment::Enum& s) const
    {
      return qHash(s);
    }
};

template<>
struct hash<TextVerticalAlignment::Enum>
{
    size_t operator()(const TextVerticalAlignment::Enum& s) const
    {
      return qHash(s);
    }
};

template<>
struct hash<TextElide::Enum>
{
    size_t operator()(const TextElide::Enum& s) const
    {
      return qHash(s);
    }
};

template<>
struct hash<TextWordWrap::Enum>
{
    size_t operator()(const TextWordWrap::Enum& s) const
    {
      return qHash(s);
    }
};
}

// Copied from boost
template <class T>
inline void hashCombine(std::size_t &seed, const T &v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

size_t getTextHashValue(const SText &text)
{
    size_t hashValue = 0;
    hashCombine(hashValue, text.m_TextColor.x);
    hashCombine(hashValue, text.m_TextColor.y);
    hashCombine(hashValue, text.m_TextColor.z);
    hashCombine(hashValue, text.m_TextColor.w);
    hashCombine(hashValue, std::string(text.m_Font.c_str()));
    hashCombine(hashValue, std::string(text.m_Text.c_str()));
    hashCombine(hashValue, text.m_Elide);
    hashCombine(hashValue, text.m_Leading);
    hashCombine(hashValue, text.m_FontSize);
    hashCombine(hashValue, text.m_Tracking);
    hashCombine(hashValue, text.m_WordWrap);
    hashCombine(hashValue, text.m_DropShadow);
    hashCombine(hashValue, text.m_BoundingBox.x);
    hashCombine(hashValue, text.m_BoundingBox.y);
    hashCombine(hashValue, text.m_DropShadowOffsetX);
    hashCombine(hashValue, text.m_DropShadowOffsetY);
    hashCombine(hashValue, text.m_DropShadowStrength);
    hashCombine(hashValue, text.m_VerticalAlignment);
    hashCombine(hashValue, text.m_HorizontalAlignment);
    return hashValue;
}

size_t getGlyphHashValue(const GlyphInfo &glyph)
{
    size_t hashValue = 0;
    hashCombine(hashValue, glyph.vertexes);
    hashCombine(hashValue, glyph.glyphIndexes);
    hashCombine(hashValue, glyph.fontScale);
    hashCombine(hashValue, glyph.shadowOffsetX);
    hashCombine(hashValue, glyph.shadowOffsetY);
    return hashValue;
}

void Q3DSDistanceFieldRenderer::renderText(SText &text, const QT3DSMat44 &mvp)
{
    if (!m_shader.program)
        buildShaders();

    float alpha = text.m_GlobalOpacity * text.m_TextColor.w;
    QT3DSVec4 textColor = QT3DSVec4(text.m_TextColor.getXYZ() * alpha, alpha);
    int shadowRgb = int(100 - int(text.m_DropShadowStrength));
    QT3DSVec4 shadowColor(shadowRgb * 0.01f * alpha, shadowRgb * 0.01f * alpha,
                          shadowRgb * 0.01f * alpha, alpha);

    size_t textHashValue = getTextHashValue(text);
    if (!m_glyphCache.contains(textHashValue))
        m_glyphCache[textHashValue] = buildGlyphsPerTexture(text);

    QHash<Q3DSDistanceFieldGlyphCache::TextureInfo *, GlyphInfo> &glyphsPerTexture
            = m_glyphCache[textHashValue];

    QT3DSVec3 minimum(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 0);
    QT3DSVec3 maximum(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), 0);

    QHash<Q3DSDistanceFieldGlyphCache::TextureInfo *, GlyphInfo>::const_iterator it;
    for (it = glyphsPerTexture.constBegin(); it != glyphsPerTexture.constEnd(); ++it) {
        const GlyphInfo &glyphInfo = it.value();

        if (glyphInfo.bounds.minimum.x < minimum.x)
            minimum.x = glyphInfo.bounds.minimum.x;
        if (glyphInfo.bounds.minimum.y < minimum.y)
            minimum.y = glyphInfo.bounds.minimum.y;
        if (glyphInfo.bounds.minimum.z < minimum.z)
            minimum.z = glyphInfo.bounds.minimum.z;

        if (glyphInfo.bounds.maximum.x > maximum.x)
            maximum.x = glyphInfo.bounds.maximum.x;
        if (glyphInfo.bounds.maximum.y > maximum.y)
            maximum.y = glyphInfo.bounds.maximum.y;
        if (glyphInfo.bounds.maximum.z > maximum.z)
            maximum.z = glyphInfo.bounds.maximum.z;

        size_t glyphHashValue = getGlyphHashValue(glyphInfo);

        if (!m_meshCache.contains(glyphHashValue))
            m_meshCache[glyphHashValue] = buildMesh(glyphInfo, text.m_DropShadow);

        Q3DSDistanceFieldMesh &mesh = m_meshCache[glyphHashValue];

        STextureDetails textureDetails = it.key()->texture->GetTextureDetails();

        if (text.m_DropShadow) {
            renderMeshWithDropShadow(mesh.inputAssembler, it.key()->texture, mvp,
                                     int(textureDetails.m_Width), int(textureDetails.m_Height),
                                     glyphInfo.fontScale * float(m_pixelRatio),
                                     QT3DSVec2(glyphInfo.shadowOffsetX, glyphInfo.shadowOffsetY),
                                     textColor, shadowColor);
        } else {
            renderMesh(mesh.inputAssembler, it.key()->texture, mvp,
                       int(textureDetails.m_Width), int(textureDetails.m_Height),
                       glyphInfo.fontScale * float(m_pixelRatio), textColor);
        }

        m_renderedGlyphs += glyphHashValue;
    }

    m_renderedTexts += textHashValue;

    text.m_Bounds = NVBounds3(minimum, maximum);
}

void Q3DSDistanceFieldRenderer::renderTextDepth(SText &text, const QT3DSMat44 &mvp)
{
    // TODO: Create a depth pass shader for distance field text
    renderText(text, mvp);
}

void Q3DSDistanceFieldRenderer::setContext(IQt3DSRenderContext &context)
{
    m_context = &context;
    m_glyphCacheManager.setContext(context);
}

ITextRendererCore &ITextRendererCore::createDistanceFieldRenderer(NVFoundationBase &fnd)
{
    return *QT3DS_NEW(fnd.getAllocator(), Q3DSDistanceFieldRenderer)(fnd);
}

bool Q3DSDistanceFieldRenderer::checkAndBuildGlyphs(SText &text)
{
    auto hashVal = getTextHashValue(text);
    m_renderedTexts += hashVal;
    if (!m_glyphCache.contains(hashVal)) {
        m_glyphCache[hashVal] = buildGlyphsPerTexture(text);
        return true;
    }

    return false;
}

// Unused methods:

void Q3DSDistanceFieldRenderer::PreloadFonts()
{
    Q_ASSERT(false);
}

void Q3DSDistanceFieldRenderer::BeginPreloadFonts(IThreadPool &, IPerfTimer &)
{
    Q_ASSERT(false);
}

void Q3DSDistanceFieldRenderer::EndPreloadFonts()
{
    Q_ASSERT(false);
}

NVConstDataRef<SRendererFontEntry> Q3DSDistanceFieldRenderer::GetProjectFontList()
{
    Q_ASSERT(false);
    return NVConstDataRef<SRendererFontEntry>();
}

Option<CRegisteredString> Q3DSDistanceFieldRenderer::GetFontNameForFont(CRegisteredString)
{
    Q_ASSERT(false);
    return Option<CRegisteredString>();
}

Option<CRegisteredString> Q3DSDistanceFieldRenderer::GetFontNameForFont(const char8_t *)
{
    Q_ASSERT(false);
    return Option<CRegisteredString>();
}

STextDimensions Q3DSDistanceFieldRenderer::MeasureText(const STextRenderInfo &, QT3DSF32,
                                                       const char8_t *)
{
    Q_ASSERT(false);
    return STextDimensions();
}

STextTextureDetails Q3DSDistanceFieldRenderer::RenderText(const STextRenderInfo &,
                                                          NVRenderTexture2D &)
{
    Q_ASSERT(false);
    return STextTextureDetails();
}

STextTextureDetails Q3DSDistanceFieldRenderer::RenderText(
        const STextRenderInfo &, NVRenderPathFontItem &, NVRenderPathFontSpecification &)
{
    Q_ASSERT(false);
    return STextTextureDetails();
}

SRenderTextureAtlasDetails Q3DSDistanceFieldRenderer::RenderText(const STextRenderInfo &)
{
    Q_ASSERT(false);
    return SRenderTextureAtlasDetails();
}

void Q3DSDistanceFieldRenderer::BeginFrame()
{
    Q_ASSERT(false);
}

QT3DSI32 Q3DSDistanceFieldRenderer::CreateTextureAtlas()
{
    Q_ASSERT(false);
    return 0;
}

STextTextureAtlasEntryDetails Q3DSDistanceFieldRenderer::RenderAtlasEntry(QT3DSU32,
                                                                          NVRenderTexture2D &)
{
    Q_ASSERT(false);
    return STextTextureAtlasEntryDetails();
}

#endif
