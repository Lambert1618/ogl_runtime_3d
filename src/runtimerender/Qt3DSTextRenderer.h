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
#ifndef QT3DS_TEXT_RENDERER_H
#define QT3DS_TEXT_RENDERER_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/StringTable.h"
#include "Qt3DSRenderTextTypes.h"

namespace qt3ds {
namespace render {

    struct SRendererFontEntry
    {
        QString m_FontName;
        QString m_FontFile;
        SRendererFontEntry() {}
        SRendererFontEntry(QString name, QString file)
            : m_FontName(name)
            , m_FontFile(file)
        {
        }
    };

    class ITextRendererCore : public NVRefCounted
    {
    public:
        // You can have several standard font directories and these will be persistent
        virtual void AddSystemFontDirectory(const char8_t *inDirectory) = 0;
        // Should be called to clear the current context.
        virtual void AddProjectFontDirectory(const char8_t *inProjectDirectory) = 0;
        virtual void ClearProjectFontDirectories() = 0;
        // Force font loading *right now*
        virtual void PreloadFonts() = 0;
        // Do not access object in between begin/end preload pairs.
        virtual void BeginPreloadFonts(IThreadPool &inThreadPool, IPerfTimer &inTimer) = 0;
        virtual void EndPreloadFonts() = 0;
        // Force a clear and reload of all of the fonts.
        virtual void ReloadFonts() = 0;
        // Get the list of project fonts.  These are the only fonts that can be displayed.
        virtual NVConstDataRef<SRendererFontEntry> GetProjectFontList() = 0;
        // The name stored in the ttf file isn't the actual name we use; we use the file stems.
        // But we used to use the name.  So this provides a sort of first-come-first-serve remapping
        // from ttf-name to file-stem.
        virtual Option<CRegisteredString> GetFontNameForFont(CRegisteredString inFontname) = 0;
        virtual Option<CRegisteredString> GetFontNameForFont(const char8_t *inFontname) = 0;

        virtual ITextRenderer &GetTextRenderer(NVRenderContext &inContext) = 0;

        static ITextRendererCore &CreateQtTextRenderer(NVFoundationBase &inFoundation,
                                                       IStringTable &inStrTable);

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
        static ITextRendererCore &createDistanceFieldRenderer(NVFoundationBase &inFnd);
#endif

        // call this to create onscreen text renderer
        // it needs true type fonts
        static ITextRendererCore &CreateOnscreenTextRenderer(NVFoundationBase &inFoundation);
    };
    /**
     *	Opaque text rendering system.  Must be able to render text to an opengl texture object.
     */
    class ITextRenderer : public ITextRendererCore
    {
    protected:
        virtual ~ITextRenderer() {}

    public:
        // Measure text will inText if it isn't null or the text on the info if inText is null
        virtual STextDimensions MeasureText(const STextRenderInfo &inText, QT3DSF32 inTextScaleFactor,
                                            const char8_t *inTextOverride = NULL) = 0;
        // The system will use the 'r' channel as an alpha mask in order to render the
        // text.  You can assume GetTextDimensions was called *just* prior to this.
        // It is a good idea to ensure the texture is a power of two as not all rendering systems
        // support nonpot textures.  Our text rendering algorithms will render a sub-rect of the
        // image
        // assuming it is located toward the upper-left of the image and we are also capable of
        // flipping
        // the image.
        virtual STextTextureDetails RenderText(const STextRenderInfo &inText,
                                               NVRenderTexture2D &inTexture) = 0;
        // this is for rendering text with NV path rendering
        virtual STextTextureDetails
        RenderText(const STextRenderInfo &inText, NVRenderPathFontItem &inPathFontItem,
                   NVRenderPathFontSpecification &inPathFontSpecicification) = 0;
        // this is for rednering text using a texture atlas
        virtual SRenderTextureAtlasDetails RenderText(const STextRenderInfo &inText) = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        // these two function are for texture atlas usage only
        // returns the atlas entries count
        virtual QT3DSI32 CreateTextureAtlas() = 0;
        virtual STextTextureAtlasEntryDetails RenderAtlasEntry(QT3DSU32 index,
                                                               NVRenderTexture2D &inTexture) = 0;

        // Helper function to upload the texture data to the texture
        // Will resize texture as necessary and upload using texSubImage for
        // quickest upload times
        // This function expects that the dataWidth to be divisible by four and
        // that the total data height is larger then inTextHeight *and* divisible by four.
        // and that textWidth and textHeight are less than or equal to dataWidth,dataHeight
        //,can be zero, and don't need to be divisible by four (or 2).
        static STextTextureDetails
        UploadData(NVDataRef<QT3DSU8> inTextureData, NVRenderTexture2D &inTexture, QT3DSU32 inDataWidth,
                   QT3DSU32 inDataHeight, QT3DSU32 inTextWidth, QT3DSU32 inTextHeight,
                   NVRenderTextureFormats::Enum inFormat, bool inFlipYAxis);

        // Helper function to return the next power of two.
        // Fails for values of 0 or QT3DS_MAX_U32
        static QT3DSU32 NextPowerOf2(QT3DSU32 inValue);
        // If inValue is divisible by four, then return inValue
        // else next largest number that is divisible by four.
        static QT3DSU32 NextMultipleOf4(QT3DSU32 inValue);
    };
}
}

#endif
