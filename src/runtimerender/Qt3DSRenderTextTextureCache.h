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
#ifndef QT3DS_RENDER_TEXT_TEXTURE_CACHE_H
#define QT3DS_RENDER_TEXT_TEXTURE_CACHE_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "Qt3DSRenderText.h"
#include "EASTL/algorithm.h"

namespace qt3ds {
namespace render {

    class ITextRenderer;

    typedef eastl::pair<NVScopedRefCounted<NVRenderPathFontSpecification>,
                        NVScopedRefCounted<NVRenderPathFontItem>>
        TPathFontSpecAndPathObject;
    typedef eastl::pair<STextTextureDetails, NVScopedRefCounted<NVRenderTexture2D>>
        TTextTextureDetailsAndTexture;
    typedef eastl::pair<TPathFontSpecAndPathObject, TTextTextureDetailsAndTexture>
        TTPathObjectAndTexture;

    class ITextTextureCache : public NVRefCounted
    {
    protected:
        virtual ~ITextTextureCache() {}
    public:
        virtual TTPathObjectAndTexture RenderText(const STextRenderInfo &inText,
                                                  QT3DSF32 inScaleFactor) = 0;
        // We may have one more texture in cache than this byte count, but this will be the limiting
        // factor.
        virtual QT3DSU32 GetCacheHighWaterBytes() const = 0;
        virtual void SetCacheHighWaterBytes(QT3DSU32 inNumBytes) = 0;

        virtual void BeginFrame() = 0;
        // We need to know the frame rhythm because we can't release anything that was touched this
        // frame.
        virtual void EndFrame() = 0;

        static ITextTextureCache &CreateTextureCache(NVFoundationBase &inFnd,
                                                     ITextRenderer &inTextRenderer,
                                                     NVRenderContext &inRenderContext);
    };
}
}
#endif
