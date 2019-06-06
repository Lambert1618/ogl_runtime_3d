/****************************************************************************
**
** Copyright (C) 2008-2015 NVIDIA Corporation.
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
#pragma once
#ifndef QT3DS_RENDER_TEXT_TYPES_H
#define QT3DS_RENDER_TEXT_TYPES_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSVec2.h"

namespace qt3ds {
namespace render {

    struct TextHorizontalAlignment
    {
        enum Enum {
            Unknown = 0,
            Left,
            Center,
            Right,
        };
    };

    struct TextVerticalAlignment
    {
        enum Enum {
            Unknown = 0,
            Top,
            Middle,
            Bottom,
        };
    };

    struct TextWordWrap
    {
        enum Enum {
            Unknown = 0,
            Clip,
            WrapWord,
            WrapAnywhere,
        };
    };

    struct TextElide
    {
        enum Enum {
            ElideNone = 0,
            ElideLeft,
            ElideMiddle,
            ElideRight,
        };
    };

    struct STextDimensions
    {
        QT3DSU32 m_TextWidth;
        QT3DSU32 m_TextHeight;
        STextDimensions(QT3DSU32 w, QT3DSU32 h)
            : m_TextWidth(w)
            , m_TextHeight(h)
        {
        }
        STextDimensions()
            : m_TextWidth(0)
            , m_TextHeight(0)
        {
        }
    };

    struct STextTextureDetails : public STextDimensions
    {
        QT3DSVec2 m_ScaleFactor;
        bool m_FlipY;
        STextTextureDetails(QT3DSU32 w, QT3DSU32 h, bool inFlipY, QT3DSVec2 scaleF)
            : STextDimensions(w, h)
            , m_ScaleFactor(scaleF)
            , m_FlipY(inFlipY)
        {
        }
        STextTextureDetails()
            : m_ScaleFactor(1.0f)
            , m_FlipY(false)
        {
        }
    };

    struct STextTextureAtlasEntryDetails : public STextDimensions
    {
        QT3DSI32 m_X, m_Y;
        STextTextureAtlasEntryDetails(QT3DSU32 w, QT3DSU32 h, QT3DSI32 x, QT3DSI32 y)
            : STextDimensions(w, h)
            , m_X(x)
            , m_Y(y)
        {
        }
        STextTextureAtlasEntryDetails()
            : m_X(0)
            , m_Y(0)
        {
        }
    };

    struct SRenderTextureAtlasDetails
    {
        QT3DSU32 m_VertexCount;
        NVDataRef<QT3DSU8> m_Vertices;

        SRenderTextureAtlasDetails(QT3DSU32 count, NVDataRef<QT3DSU8> inVertices)
            : m_VertexCount(count)
            , m_Vertices(inVertices)
        {
        }
        SRenderTextureAtlasDetails()
            : m_VertexCount(0)
            , m_Vertices(NVDataRef<QT3DSU8>())
        {
        }
    };

    struct STextTextureAtlasDetails : public STextTextureDetails
    {
        QT3DSU32 m_EntryCount;
        STextTextureAtlasDetails(QT3DSU32 w, QT3DSU32 h, bool inFlipY, QT3DSU32 count)
            : STextTextureDetails(w, h, inFlipY, QT3DSVec2(1.0f))
            , m_EntryCount(count)
        {
        }
        STextTextureAtlasDetails()
            : m_EntryCount(0)
        {
        }
    };

    // Adding/removing a member to this object means you need to update the texture cache code
    // - UICRenderTextTextureCache.cpp

    struct STextRenderInfo
    {
        CRegisteredString m_Text;
        CRegisteredString m_Font;
        QT3DSF32 m_FontSize;
        TextHorizontalAlignment::Enum m_HorizontalAlignment;
        TextVerticalAlignment::Enum m_VerticalAlignment;
        QT3DSF32 m_Leading; // space between lines
        QT3DSF32 m_Tracking; // space between letters
        bool m_DropShadow;
        QT3DSF32 m_DropShadowStrength;
        QT3DSF32 m_DropShadowOffsetX;
        QT3DSF32 m_DropShadowOffsetY;
        TextWordWrap::Enum m_WordWrap;
        QT3DSVec2 m_BoundingBox;
        TextElide::Enum m_Elide;

        QT3DSF32 m_ScaleX; // Pixel scale in X
        QT3DSF32 m_ScaleY; // Pixel scale in Y

        bool m_EnableAcceleratedFont; // use NV path rendering

        STextRenderInfo();
        ~STextRenderInfo();
    };
}
}

#endif
