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
#ifndef QT3DS_RENDER_LOADED_TEXTURE_H
#define QT3DS_RENDER_LOADED_TEXTURE_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "Qt3DSRenderLoadedTextureDDS.h"
#include "foundation/Qt3DSRefCounted.h"
#include <QImage>

namespace qt3ds {
namespace foundation {
    class ISeekableIOStream;
    class IInStream;
}
}

namespace qt3ds {
namespace render {

    class IInputStreamFactory;

    struct STextureData
    {
        void *data;
        QT3DSU32 dataSizeInBytes;
        qt3ds::render::NVRenderTextureFormats::Enum format;
        STextureData()
            : data(NULL)
            , dataSizeInBytes(0)
            , format(qt3ds::render::NVRenderTextureFormats::Unknown)
        {
        }
    };
    struct ExtendedTextureFormats
    {
        enum Enum {
            NoExtendedFormat = 0,
            Palettized,
            CustomRGB,
        };
    };
    // Utility class used for loading image data from disk.
    // Supports jpg, png, and dds.
    struct SLoadedTexture : public NVReleasable
    {
    private:
        ~SLoadedTexture();

    public:
        NVAllocatorCallback &m_Allocator;
        QT3DSI32 width;
        QT3DSI32 height;
        QT3DSI32 components;
        void *data;
        QImage image;
        QT3DSU32 dataSizeInBytes;
        qt3ds::render::NVRenderTextureFormats::Enum format;
        Qt3DSDDSImage *dds;
        ExtendedTextureFormats::Enum m_ExtendedFormat;
        // Used for palettized images.
        void *m_Palette;
        QT3DSI32 m_CustomMasks[3];
        int m_BitCount;
        char8_t m_BackgroundColor[3];
        uint8_t *m_TransparencyTable;
        int32_t m_TransparentPaletteIndex;

        SLoadedTexture(NVAllocatorCallback &inAllocator)
            : m_Allocator(inAllocator)
            , width(0)
            , height(0)
            , components(0)
            , data(NULL)
            , image(0)
            , dataSizeInBytes(0)
            , format(qt3ds::render::NVRenderTextureFormats::RGBA8)
            , dds(NULL)
            , m_ExtendedFormat(ExtendedTextureFormats::NoExtendedFormat)
            , m_Palette(NULL)
            , m_BitCount(0)
            , m_TransparencyTable(NULL)
            , m_TransparentPaletteIndex(-1)
        {
            m_CustomMasks[0] = 0;
            m_CustomMasks[1] = 0;
            m_CustomMasks[2] = 0;
            m_BackgroundColor[0] = 0;
            m_BackgroundColor[1] = 0;
            m_BackgroundColor[2] = 0;
        }
        void setFormatFromComponents()
        {
            switch (components) {
            case 1: // undefined, but in this context probably luminance
                format = qt3ds::render::NVRenderTextureFormats::Luminance8;
                break;
            case 2:
                format = qt3ds::render::NVRenderTextureFormats::LuminanceAlpha8;
                break;
            case 3:
                format = qt3ds::render::NVRenderTextureFormats::RGB8;
                break;

            default:
            // fallthrough intentional
            case 4:
                format = qt3ds::render::NVRenderTextureFormats::RGBA8;
                break;
            }
        }

        void EnsureMultiplerOfFour(NVFoundationBase &inFoundation, const char *inPath);
        // Returns true if this image has a pixel less than 255.
        // If yes, then alsoOpaquePixels is true if some are not
        bool ScanForTransparency(bool &alsoOpaquePixels);

        // Be sure to call this or risk leaking an enormous amount of memory
        void release() override;

        // Not all video cards support dxt compression.  Giving the last image allows
        // this object to potentially reuse the memory
        STextureData DecompressDXTImage(int inMipMapIdx, STextureData *inOptLastImage = NULL);
        void ReleaseDecompressedTexture(STextureData inImage);

        static SLoadedTexture *Load(const QString &inPath, NVFoundationBase &inAllocator,
                                    IInputStreamFactory &inFactory, bool inFlipY = true,
                                    NVRenderContextType renderContextType
                                        = NVRenderContextValues::NullContext,
                                    bool preferKTX = false,
                                    IBufferManager *bufferManager = nullptr);
        static SLoadedTexture *LoadDDS(IInStream &inStream, QT3DSI32 flipVertical,
                                       NVFoundationBase &fnd,
                                       NVRenderContextType renderContextType);
        static SLoadedTexture *LoadKTX(IInStream &inStream, QT3DSI32 flipVertical,
                                       NVFoundationBase &fnd,
                                       NVRenderContextType renderContextType);
        static SLoadedTexture *LoadBMP(ISeekableIOStream &inStream, bool inFlipY,
                                       NVFoundationBase &inFnd,
                                       NVRenderContextType renderContextType);
        static SLoadedTexture *LoadGIF(ISeekableIOStream &inStream, bool inFlipY,
                                       NVFoundationBase &inFnd,
                                       NVRenderContextType renderContextType);
        static SLoadedTexture *LoadHDR(ISeekableIOStream &inStream, NVFoundationBase &inFnd,
                                       NVRenderContextType renderContextType);

        static SLoadedTexture *LoadQImage(const QString &inPath, QT3DSI32 flipVertical,
                                          NVFoundationBase &fnd,
                                          NVRenderContextType renderContextType,
                                          IBufferManager *bufferManager = nullptr);

        static SLoadedTexture *LoadASTC(const QString &inPath, QT3DSI32 flipVertical,
                                        NVFoundationBase &fnd,
                                        NVRenderContextType renderContextType);

    private:
        // Implemented in the bmp loader.
        void FreeImagePostProcess(bool inFlipY);
    };
}
}

#endif
