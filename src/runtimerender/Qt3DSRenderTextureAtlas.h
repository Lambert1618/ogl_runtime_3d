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
#pragma once
#ifndef QT3DS_RENDER_TEXTURE_ATLAS_H
#define QT3DS_RENDER_TEXTURE_ATLAS_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "EASTL/algorithm.h"

namespace qt3ds {
namespace render {

    class ITextRenderer;

    struct STextureAtlasRect
    {

        STextureAtlasRect()
            : m_X(0)
            , m_Y(0)
            , m_Width(0)
            , m_Height(0)
        {
        }

        STextureAtlasRect(QT3DSI32 x, QT3DSI32 y, QT3DSI32 w, QT3DSI32 h)
            : m_X(x)
            , m_Y(y)
            , m_Width(w)
            , m_Height(h)
        {
        }

        QT3DSI32 m_X;
        QT3DSI32 m_Y;
        QT3DSI32 m_Width;
        QT3DSI32 m_Height;

        // normalized coordinates
        QT3DSF32 m_NormX;
        QT3DSF32 m_NormY;
        QT3DSF32 m_NormWidth;
        QT3DSF32 m_NormHeight;
    };

    typedef eastl::pair<STextureAtlasRect, NVDataRef<QT3DSU8>> TTextureAtlasEntryAndBuffer;

    /**
     *	Abstract class of a texture atlas representation
    */
    class ITextureAtlas : public NVRefCounted
    {
    protected:
        virtual ~ITextureAtlas() {}

    public:
        virtual QT3DSI32 GetWidth() const = 0;
        virtual QT3DSI32 GetHeight() const = 0;
        virtual QT3DSI32 GetAtlasEntryCount() const = 0;
        virtual TTextureAtlasEntryAndBuffer GetAtlasEntryByIndex(QT3DSU32 index) = 0;

        virtual STextureAtlasRect AddAtlasEntry(QT3DSI32 width, QT3DSI32 height, QT3DSI32 pitch,
                                                QT3DSI32 dataWidth,
                                                NVConstDataRef<QT3DSU8> bufferData) = 0;
        virtual void RelaseEntries() = 0;

        static ITextureAtlas &CreateTextureAtlas(NVFoundationBase &inFnd,
                                                 NVRenderContext &inRenderContext, QT3DSI32 width,
                                                 QT3DSI32 height);
    };
}
}

#endif
