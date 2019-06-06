/****************************************************************************
**
** Copyright (C) 2015 NVIDIA Corporation.
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

#include "render/Qt3DSRenderPathFontText.h"
#include "render/Qt3DSRenderPathFontSpecification.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderPathRender.h"

namespace qt3ds {
namespace render {

    // see NVprSDK for explanation
    // Math from page 54-56 of "Digital Image Warping" by George Wolberg,
    // though credited to Paul Heckert's "Fundamentals of Texture
    // Mapping and Image Warping" 1989 Master's thesis.
    static QT3DSMat33 mapSquareToQuad(QT3DSVec2 inSquare[4])
    {
        QT3DSMat33 ret;

        QT3DSVec2 d1(inSquare[1] - inSquare[2]);
        QT3DSVec2 d2(inSquare[3] - inSquare[2]);
        QT3DSVec2 d3(inSquare[0] - inSquare[1] + inSquare[2] - inSquare[3]);

        QT3DSF32 denom = d1.x * d2.y - d2.x * d1.y;
        if (denom == 0.0) {
            return QT3DSMat33::createIdentity();
        }

        ret[2][0] = (d3.x * d2.y - d2.x * d3.y) / denom;
        ret[2][1] = (d1.x * d3.y - d3.x * d1.y) / denom;
        ret[2][2] = 1.0;
        ret[0][0] = inSquare[1].x - inSquare[0].x + ret[2][0] * inSquare[1].x;
        ret[1][0] = inSquare[1].y - inSquare[0].y + ret[2][0] * inSquare[1].y;
        ret[0][1] = inSquare[3].x - inSquare[0].x + ret[2][1] * inSquare[3].x;
        ret[1][1] = inSquare[3].y - inSquare[0].y + ret[2][1] * inSquare[3].y;
        ret[0][2] = inSquare[0].x;
        ret[1][2] = inSquare[0].y;

        return ret;
    }

    static QT3DSMat33 mapQuadToSquare(QT3DSVec2 inSquare[4])
    {
        return mapSquareToQuad(inSquare).getInverse();
    }

    static QT3DSMat33 mapQuadToQuad(QT3DSVec2 fromSquare[4], QT3DSVec2 toSquare[4])
    {
        return (mapSquareToQuad(toSquare) * mapQuadToSquare(fromSquare));
    }

    static QT3DSMat44 mapBoxToQuad(QT3DSVec4 inBox, QT3DSVec2 inSquare[4])
    {
        QT3DSVec2 fromSquare[4] = { QT3DSVec2(inBox.x, inBox.y), QT3DSVec2(inBox.z, inBox.y),
                                 QT3DSVec2(inBox.z, inBox.w), QT3DSVec2(inBox.x, inBox.w) };

        QT3DSMat33 ret = mapQuadToQuad(fromSquare, inSquare);

        return QT3DSMat44(ret.column0, ret.column1, ret.column2, QT3DSVec3(0.0, 0.0, 0.0));
    }

    NVRenderPathFontItem::NVRenderPathFontItem(NVFoundationBase &fnd)
        : m_Foundation(fnd)
        , mRefCount(0)
        , m_NumGlyphs(0)
        , m_GlyphIDs(NULL)
        , m_TranslateXY(NULL)
    {
    }

    NVRenderPathFontItem::~NVRenderPathFontItem()
    {
        if (m_TranslateXY)
            QT3DS_FREE(m_Foundation.getAllocator(), m_TranslateXY);
        if (m_GlyphIDs)
            QT3DS_FREE(m_Foundation.getAllocator(), m_GlyphIDs);
    }

    void NVRenderPathFontItem::InitTextItem(size_t glyphCount, const QT3DSU32 *glyphIDs,
                                            NVRenderPathFormatType::Enum type, QT3DSF32 *posArray,
                                            QT3DSVec2 pixelBound, QT3DSVec2 logicalBound, QT3DSF32 emScale)
    {
        m_NumGlyphs = glyphCount;

        // allocate glyphs array
        if (m_GlyphIDs)
            QT3DS_FREE(m_Foundation.getAllocator(), m_GlyphIDs);

        // allocate position array
        if (m_TranslateXY)
            QT3DS_FREE(m_Foundation.getAllocator(), m_TranslateXY);

        m_GlyphIDs = (QT3DSU32 *)QT3DS_ALLOC(m_Foundation.getAllocator(),
                                       glyphCount * getSizeofType(type), "NVRenderPathFontItem");
        m_TranslateXY =
            (QT3DSF32 *)QT3DS_ALLOC(m_Foundation.getAllocator(), 2 * (glyphCount + 1) * sizeof(QT3DSF32),
                              "NVRenderPathFontItem");

        if (!m_GlyphIDs || !m_TranslateXY)
            return;

        QT3DSU32 *pTheGlyphIDs = (QT3DSU32 *)m_GlyphIDs;
        QT3DSU32 *pInGlyphs = (QT3DSU32 *)glyphIDs;

        /// copy glyphs array
        for (size_t i = 0; i < glyphCount; i++) {
            pTheGlyphIDs[i] = pInGlyphs[i];
        }

        // copy position array
        // we copy what we got from our layout system
        if (posArray != NULL) {
            for (size_t i = 0, k = 0; i < glyphCount * 2; i += 2, k++) {
                m_TranslateXY[i] = posArray[i] * emScale;
                m_TranslateXY[i + 1] = posArray[i + 1] * emScale;
            }
        }

        // setup transform
        QT3DSVec2 square[4] = { QT3DSVec2(0.0, 0.0), QT3DSVec2(pixelBound.x, 0.0),
                             QT3DSVec2(pixelBound.x, pixelBound.y), QT3DSVec2(0.0, pixelBound.y) };
        QT3DSVec4 box(0.0, 0.0, logicalBound.x * emScale, logicalBound.y * emScale);

        m_ModelMatrix = mapBoxToQuad(box, square);
    }

    const QT3DSMat44 NVRenderPathFontItem::GetTransform()
    {
        return QT3DSMat44(QT3DSVec4(m_ModelMatrix[0][0], m_ModelMatrix[1][0], 0.0, m_ModelMatrix[2][0]),
                       QT3DSVec4(m_ModelMatrix[0][1], m_ModelMatrix[1][1], 0.0, m_ModelMatrix[2][1]),
                       QT3DSVec4(0.0, 0.0, 1.0, 0.0),
                       QT3DSVec4(m_ModelMatrix[0][2], m_ModelMatrix[1][2], 0.0, m_ModelMatrix[2][2]));
    }

    QT3DSU32
    NVRenderPathFontItem::getSizeofType(NVRenderPathFormatType::Enum type)
    {
        switch (type) {
        case NVRenderPathFormatType::Byte:
            return sizeof(QT3DSI8);
        case NVRenderPathFormatType::UByte:
            return sizeof(QT3DSU8);
        case NVRenderPathFormatType::Bytes2:
            return sizeof(QT3DSU16);
        case NVRenderPathFormatType::Uint:
            return sizeof(QT3DSU32);
        case NVRenderPathFormatType::Utf8:
            return sizeof(QT3DSU32);
        default:
            QT3DS_ASSERT(false);
            return 1;
        }
    }

    NVRenderPathFontItem *NVRenderPathFontItem::CreatePathFontItem(NVRenderContextImpl &context)
    {
        QT3DS_ASSERT(context.IsPathRenderingSupported());

        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderPathFontItem)(context.GetFoundation());
    }
}
}
