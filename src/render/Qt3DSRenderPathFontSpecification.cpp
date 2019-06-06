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

#include "render/Qt3DSRenderPathFontSpecification.h"
#include "render/Qt3DSRenderPathFontText.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderContext.h"

namespace qt3ds {
namespace render {

    NVRenderPathFontSpecification::NVRenderPathFontSpecification(NVRenderContextImpl &context,
                                                                 NVFoundationBase &fnd,
                                                                 CRegisteredString fontName)
        : m_Context(context)
        , m_Foundation(fnd)
        , m_Backend(context.GetBackend())
        , mRefCount(0)
        , m_NumFontGlyphs(0)
        , m_EmScale(2048) // 2048 is default true type scale
        , m_Type(NVRenderPathFormatType::UByte)
        , m_TransformType(NVRenderPathTransformType::Translate2D)
        , m_FontName(fontName)
    {
    }

    NVRenderPathFontSpecification::~NVRenderPathFontSpecification()
    {
        m_Context.ReleasePathFontSpecification(*this);
    }

    void NVRenderPathFontSpecification::LoadPathGlyphs(const char *fontName,
                                                       NVRenderPathFormatType::Enum type)
    {
        // check if we already created it
        if (m_NumFontGlyphs)
            return;

        m_Type = type;

        // create fonts based on the input
        m_PathRenderHandle = m_Backend->LoadPathGlyphsIndexedRange(
            NVRenderPathFontTarget::FileFont, fontName, NVRenderPathFontStyleFlags(), 0, m_EmScale,
            &m_NumFontGlyphs);

        // Fallback in case the previuos call fails
        // This is a no-op if the previous call succeeds
        // Note that sans is an inbuild driver font
        if (!m_PathRenderHandle) {
            m_PathRenderHandle = m_Backend->LoadPathGlyphsIndexedRange(
                NVRenderPathFontTarget::SystemFont, "Arial", NVRenderPathFontStyleFlags(), 0,
                m_EmScale, &m_NumFontGlyphs);
        }

        // we should have some glyphs
        QT3DS_ASSERT(m_NumFontGlyphs);
    }

    void
    NVRenderPathFontSpecification::StencilFillPathInstanced(NVRenderPathFontItem &inPathFontItem)
    {
        const void *glyphIDs = inPathFontItem.GetGlyphIDs();
        const QT3DSF32 *spacing = inPathFontItem.GetSpacing();
        if (!glyphIDs || !spacing || !inPathFontItem.GetGlyphsCount()) {
            QT3DS_ASSERT(false || !inPathFontItem.GetGlyphsCount());
            return;
        }

        m_Backend->StencilFillPathInstanced(m_PathRenderHandle, inPathFontItem.GetGlyphsCount(),
                                            m_Type, glyphIDs, NVRenderPathFillMode::Fill, 0xFF,
                                            m_TransformType, spacing);
    }

    void NVRenderPathFontSpecification::CoverFillPathInstanced(NVRenderPathFontItem &inPathFontItem)
    {
        const void *glyphIDs = inPathFontItem.GetGlyphIDs();
        const QT3DSF32 *spacing = inPathFontItem.GetSpacing();
        if (!glyphIDs || !spacing || !inPathFontItem.GetGlyphsCount()) {
            QT3DS_ASSERT(false || !inPathFontItem.GetGlyphsCount());
            return;
        }

        m_Backend->CoverFillPathInstanced(
            m_PathRenderHandle, inPathFontItem.GetGlyphsCount(), m_Type, glyphIDs,
            NVRenderPathCoverMode::BoundingBoxOfBoundingBox, m_TransformType, spacing);
    }

    QT3DSU32
    NVRenderPathFontSpecification::getSizeofType(NVRenderPathFormatType::Enum type)
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

    NVRenderPathFontSpecification *
    NVRenderPathFontSpecification::CreatePathFontSpecification(NVRenderContextImpl &context,
                                                               CRegisteredString fontName)
    {
        QT3DS_ASSERT(context.IsPathRenderingSupported());

        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderPathFontSpecification)(context, context.GetFoundation(), fontName);
    }
}
}
