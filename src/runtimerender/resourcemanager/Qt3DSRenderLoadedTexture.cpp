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
#include "Qt3DSRenderLoadedTexture.h"
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSDMWindowsCompatibility.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSRenderImageScaler.h"
#include "Qt3DSTextRenderer.h"
#include <QImage>

using namespace qt3ds::render;

SLoadedTexture *SLoadedTexture::LoadQImage(const QString &inPath, QT3DSI32 flipVertical,
                                           NVFoundationBase &fnd,
                                           NVRenderContextType renderContextType)
{
    Q_UNUSED(flipVertical)
    Q_UNUSED(renderContextType)
    SLoadedTexture *retval(NULL);
    NVAllocatorCallback &alloc(fnd.getAllocator());
    QImage image(inPath);
    const QImage::Format format = image.format();
    switch (format) {
    case QImage::Format_RGBA64:
        image = image.convertToFormat(QImage::Format_RGBA8888);
        break;
    case QImage::Format_RGBX64:
        image = image.convertToFormat(QImage::Format_RGBX8888);
        break;
    default:
        break;
    }
    image = image.mirrored();
    image = image.rgbSwapped();
    retval = QT3DS_NEW(alloc, SLoadedTexture)(alloc);
    retval->width = image.width();
    retval->height = image.height();
    retval->components = image.pixelFormat().channelCount();
    retval->image = image;
    retval->data = (void*)retval->image.bits();
    retval->dataSizeInBytes = image.byteCount();
    retval->setFormatFromComponents();
    return retval;
}


namespace {

/**
        !!Large section of code ripped from FreeImage!!

*/
// ----------------------------------------------------------
//   Structures used by DXT textures
// ----------------------------------------------------------
typedef QT3DSU8 BYTE;
typedef QT3DSU16 WORD;

typedef struct tagColor8888
{
    BYTE b;
    BYTE g;
    BYTE r;
    BYTE a;
} Color8888;

typedef struct tagColor565
{
    WORD b : 5;
    WORD g : 6;
    WORD r : 5;
} Color565;

typedef struct tagDXTColBlock
{
    Color565 colors[2];
    BYTE row[4];
} DXTColBlock;

typedef struct tagDXTAlphaBlockExplicit
{
    WORD row[4];
} DXTAlphaBlockExplicit;

typedef struct tagDXTAlphaBlock3BitLinear
{
    BYTE alpha[2];
    BYTE data[6];
} DXTAlphaBlock3BitLinear;

typedef struct tagDXT1Block
{
    DXTColBlock color;
} DXT1Block;

typedef struct tagDXT3Block
{ // also used by dxt2
    DXTAlphaBlockExplicit alpha;
    DXTColBlock color;
} DXT3Block;

typedef struct tagDXT5Block
{ // also used by dxt4
    DXTAlphaBlock3BitLinear alpha;
    DXTColBlock color;
} DXT5Block;

static void GetBlockColors(const DXTColBlock &block, Color8888 colors[4], bool isDXT1)
{
    int i;
    for (i = 0; i < 2; i++) {
        colors[i].a = 0xff;
        colors[i].r = (BYTE)(block.colors[i].r * 0xff / 0x1f);
        colors[i].g = (BYTE)(block.colors[i].g * 0xff / 0x3f);
        colors[i].b = (BYTE)(block.colors[i].b * 0xff / 0x1f);
    }

    WORD *wCol = (WORD *)block.colors;
    if (wCol[0] > wCol[1] || !isDXT1) {
        // 4 color block
        for (i = 0; i < 2; i++) {
            colors[i + 2].a = 0xff;
            colors[i + 2].r =
                (BYTE)((WORD(colors[0].r) * (2 - i) + WORD(colors[1].r) * (1 + i)) / 3);
            colors[i + 2].g =
                (BYTE)((WORD(colors[0].g) * (2 - i) + WORD(colors[1].g) * (1 + i)) / 3);
            colors[i + 2].b =
                (BYTE)((WORD(colors[0].b) * (2 - i) + WORD(colors[1].b) * (1 + i)) / 3);
        }
    } else {
        // 3 color block, number 4 is transparent
        colors[2].a = 0xff;
        colors[2].r = (BYTE)((WORD(colors[0].r) + WORD(colors[1].r)) / 2);
        colors[2].g = (BYTE)((WORD(colors[0].g) + WORD(colors[1].g)) / 2);
        colors[2].b = (BYTE)((WORD(colors[0].b) + WORD(colors[1].b)) / 2);

        colors[3].a = 0x00;
        colors[3].g = 0x00;
        colors[3].b = 0x00;
        colors[3].r = 0x00;
    }
}

struct DXT_INFO_1
{
    typedef DXT1Block Block;
    enum { isDXT1 = 1, bytesPerBlock = 8 };
};

struct DXT_INFO_3
{
    typedef DXT3Block Block;
    enum { isDXT1 = 1, bytesPerBlock = 16 };
};

struct DXT_INFO_5
{
    typedef DXT5Block Block;
    enum { isDXT1 = 1, bytesPerBlock = 16 };
};

template <class INFO>
class DXT_BLOCKDECODER_BASE
{
protected:
    Color8888 m_colors[4];
    const typename INFO::Block *m_pBlock;
    unsigned m_colorRow;

public:
    void Setup(const BYTE *pBlock)
    {
        m_pBlock = (const typename INFO::Block *)pBlock;
        GetBlockColors(m_pBlock->color, m_colors, INFO::isDXT1);
    }

    void SetY(int y) { m_colorRow = m_pBlock->color.row[y]; }

    void GetColor(int x, int y, Color8888 &color)
    {
        Q_UNUSED(y)
        unsigned bits = (m_colorRow >> (x * 2)) & 3;
        color = m_colors[bits];
        std::swap(color.r, color.b);
    }
};

class DXT_BLOCKDECODER_1 : public DXT_BLOCKDECODER_BASE<DXT_INFO_1>
{
public:
    typedef DXT_INFO_1 INFO;
};

class DXT_BLOCKDECODER_3 : public DXT_BLOCKDECODER_BASE<DXT_INFO_3>
{
public:
    typedef DXT_BLOCKDECODER_BASE<DXT_INFO_3> base;
    typedef DXT_INFO_3 INFO;

protected:
    unsigned m_alphaRow;

public:
    void SetY(int y)
    {
        base::SetY(y);
        m_alphaRow = m_pBlock->alpha.row[y];
    }

    void GetColor(int x, int y, Color8888 &color)
    {
        base::GetColor(x, y, color);
        const unsigned bits = (m_alphaRow >> (x * 4)) & 0xF;
        color.a = (BYTE)((bits * 0xFF) / 0xF);
    }
};

class DXT_BLOCKDECODER_5 : public DXT_BLOCKDECODER_BASE<DXT_INFO_5>
{
public:
    typedef DXT_BLOCKDECODER_BASE<DXT_INFO_5> base;
    typedef DXT_INFO_5 INFO;

protected:
    unsigned m_alphas[8];
    unsigned m_alphaBits;
    int m_offset;

public:
    void Setup(const BYTE *pBlock)
    {
        base::Setup(pBlock);

        const DXTAlphaBlock3BitLinear &block = m_pBlock->alpha;
        m_alphas[0] = block.alpha[0];
        m_alphas[1] = block.alpha[1];
        if (m_alphas[0] > m_alphas[1]) {
            // 8 alpha block
            for (int i = 0; i < 6; i++) {
                m_alphas[i + 2] = ((6 - i) * m_alphas[0] + (1 + i) * m_alphas[1] + 3) / 7;
            }
        } else {
            // 6 alpha block
            for (int i = 0; i < 4; i++) {
                m_alphas[i + 2] = ((4 - i) * m_alphas[0] + (1 + i) * m_alphas[1] + 2) / 5;
            }
            m_alphas[6] = 0;
            m_alphas[7] = 0xFF;
        }
    }

    void SetY(int y)
    {
        base::SetY(y);
        int i = y / 2;
        const DXTAlphaBlock3BitLinear &block = m_pBlock->alpha;
        m_alphaBits = unsigned(block.data[0 + i * 3]) | (unsigned(block.data[1 + i * 3]) << 8)
            | (unsigned(block.data[2 + i * 3]) << 16);
        m_offset = (y & 1) * 12;
    }

    void GetColor(int x, int y, Color8888 &color)
    {
        base::GetColor(x, y, color);
        unsigned bits = (m_alphaBits >> (x * 3 + m_offset)) & 7;
        color.a = (BYTE)m_alphas[bits];
        std::swap(color.r, color.b);
    }
};

template <class DECODER>
void DecodeDXTBlock(BYTE *dstData, const BYTE *srcBlock, long dstPitch, int bw, int bh)
{
    DECODER decoder;
    decoder.Setup(srcBlock);
    for (int y = 0; y < bh; y++) {
        // Note that this assumes the pointer is pointing to the *last* valid start
        // row.
        BYTE *dst = dstData - y * dstPitch;
        decoder.SetY(y);
        for (int x = 0; x < bw; x++) {
            decoder.GetColor(x, y, (Color8888 &)*dst);
            dst += 4;
        }
    }
}

struct STextureDataWriter
{
    QT3DSU32 m_Width;
    QT3DSU32 m_Height;
    QT3DSU32 m_Stride;
    QT3DSU32 m_NumComponents;
    STextureData &m_TextureData;
    STextureDataWriter(QT3DSU32 w, QT3DSU32 h, bool hasA, STextureData &inTd, NVAllocatorCallback &alloc)
        : m_Width(w)
        , m_Height(h)
        , m_Stride(hasA ? m_Width * 4 : m_Width * 3)
        , m_NumComponents(hasA ? 4 : 3)
        , m_TextureData(inTd)
    {
        QT3DSU32 dataSize = m_Stride * m_Height;
        if (dataSize > m_TextureData.dataSizeInBytes) {
            alloc.deallocate(m_TextureData.data);
            m_TextureData.data =
                alloc.allocate(dataSize, "SLoadedTexture::DecompressDXTImage", __FILE__, __LINE__);
            m_TextureData.dataSizeInBytes = dataSize;
        }
        memZero(m_TextureData.data, m_TextureData.dataSizeInBytes);
        m_TextureData.format = hasA ? NVRenderTextureFormats::RGBA8 : NVRenderTextureFormats::RGB8;
    }

    void WritePixel(QT3DSU32 X, QT3DSU32 Y, QT3DSU8 *pixelData)
    {
        if (X < m_Width && Y < m_Height) {
            char *textureData = reinterpret_cast<char *>(m_TextureData.data);
            QT3DSU32 offset = Y * m_Stride + X * m_NumComponents;

            for (QT3DSU32 idx = 0; idx < m_NumComponents; ++idx)
                QT3DS_ASSERT(textureData[offset + idx] == 0);

            memCopy(textureData + offset, pixelData, m_NumComponents);
        }
    }

    // Incoming pixels are assumed to be RGBA or RGBX, 32 bit in any case
    void WriteBlock(QT3DSU32 X, QT3DSU32 Y, QT3DSU32 width, QT3DSU32 height, QT3DSU8 *pixelData)
    {
        QT3DSU32 offset = 0;
        for (QT3DSU32 yidx = 0; yidx < height; ++yidx) {
            for (QT3DSU32 xidx = 0; xidx < width; ++xidx, offset += 4) {
                WritePixel(X + xidx, Y + (height - yidx - 1), pixelData + offset);
            }
        }
    }
    bool Finished() { return false; }
};

struct STextureAlphaScanner
{
    bool &m_Alpha;
    bool *m_alsoOpaque = nullptr;

    STextureAlphaScanner(bool &inAlpha, bool *alsoOpaque)
        : m_Alpha(inAlpha), m_alsoOpaque(alsoOpaque)
    {
    }

    void WriteBlock(QT3DSU32 X, QT3DSU32 Y, QT3DSU32 width, QT3DSU32 height, QT3DSU8 *pixelData)
    {
        Q_UNUSED(X)
        Q_UNUSED(Y)
        QT3DSU32 offset = 0;
        bool opq = false;
        bool exitLoop = false;
        for (QT3DSU32 yidx = 0; yidx < height && !exitLoop; ++yidx) {
            for (QT3DSU32 xidx = 0; xidx < width && !exitLoop; ++xidx, offset += 4) {
                if (pixelData[offset + 3] < 255)
                    m_Alpha = true;
                else
                    opq = true;
                exitLoop = m_Alpha && (opq || !m_alsoOpaque);
            }
        }
        if (m_alsoOpaque)
            *m_alsoOpaque = opq;
    }

    // If we detect alpha we can stop right there.
    bool Finished() { return m_Alpha; }
};
// Scan the dds image's mipmap 0 level for alpha.
template <class DECODER, class TWriterType>
static void DecompressDDS(void *inSrc, QT3DSU32 inDataSize, QT3DSU32 inWidth, QT3DSU32 inHeight,
                          TWriterType ioWriter)
{
    typedef typename DECODER::INFO INFO;
    typedef typename INFO::Block Block;
    (void)inDataSize;

    const QT3DSU8 *pbSrc = (const QT3DSU8 *)inSrc;
    // Each DX block is composed of 16 pixels.  Free image decodes those
    // pixels into a 4x4 block of data.
    QT3DSU8 pbDstData[4 * 4 * 4];
    // The decoder decodes backwards
    // So we need to point to the last line.
    QT3DSU8 *pbDst = pbDstData + 48;

    int width = (int)inWidth;
    int height = (int)inHeight;
    int lineStride = 16;
    for (int y = 0; y < height && ioWriter.Finished() == false; y += 4) {
        int yPixels = NVMin(height - y, 4);
        for (int x = 0; x < width && ioWriter.Finished() == false; x += 4) {
            int xPixels = NVMin(width - x, 4);
            DecodeDXTBlock<DECODER>(pbDst, pbSrc, lineStride, xPixels, yPixels);
            pbSrc += INFO::bytesPerBlock;
            ioWriter.WriteBlock(x, y, xPixels, yPixels, pbDstData);
        }
    }
}

bool ScanDDSForAlpha(Qt3DSDDSImage *dds, bool *alsoOpaquePixels = nullptr)
{
    bool hasAlpha = false;
    switch (dds->format) {
    case qt3ds::render::NVRenderTextureFormats::RGBA_DXT1:
        DecompressDDS<DXT_BLOCKDECODER_1>(dds->data[0], dds->size[0], dds->mipwidth[0],
                                          dds->mipheight[0], STextureAlphaScanner(hasAlpha,
                                                                                  alsoOpaquePixels)
                );
        break;
    case qt3ds::render::NVRenderTextureFormats::RGBA_DXT3:
        DecompressDDS<DXT_BLOCKDECODER_3>(dds->data[0], dds->size[0], dds->mipwidth[0],
                                          dds->mipheight[0], STextureAlphaScanner(hasAlpha,
                                                                                  alsoOpaquePixels)
                );
        break;
    case qt3ds::render::NVRenderTextureFormats::RGBA_DXT5:
        DecompressDDS<DXT_BLOCKDECODER_5>(dds->data[0], dds->size[0], dds->mipwidth[0],
                                          dds->mipheight[0], STextureAlphaScanner(hasAlpha,
                                                                                  alsoOpaquePixels)
                );
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }
    return hasAlpha;
}

bool ScanImageForAlpha(const void *inData, QT3DSU32 inWidth, QT3DSU32 inHeight, QT3DSU32 inPixelSizeInBytes,
                       QT3DSU8 inAlphaSizeInBits, bool *alsoOpaque = nullptr)
{
    const QT3DSU8 *rowPtr = reinterpret_cast<const QT3DSU8 *>(inData);
    bool hasAlpha = false;
    bool hasOpaque = false;
    if (inAlphaSizeInBits == 0)
        return hasAlpha;
    if (inPixelSizeInBytes != 2 && inPixelSizeInBytes != 4) {
        QT3DS_ASSERT(false);
        return false;
    }
    if (inAlphaSizeInBits > 8) {
        QT3DS_ASSERT(false);
        return false;
    }

    QT3DSU32 alphaRightShift = inPixelSizeInBytes * 8 - inAlphaSizeInBits;
    QT3DSU32 maxAlphaValue = (1 << inAlphaSizeInBits) - 1;

    for (QT3DSU32 rowIdx = 0; rowIdx < inHeight && hasAlpha == false; ++rowIdx) {
        for (QT3DSU32 idx = 0; idx < inWidth && hasAlpha == false;
             ++idx, rowPtr += inPixelSizeInBytes) {
            QT3DSU32 pixelValue = 0;
            if (inPixelSizeInBytes == 2)
                pixelValue = *(reinterpret_cast<const QT3DSU16 *>(rowPtr));
            else
                pixelValue = *(reinterpret_cast<const QT3DSU32 *>(rowPtr));
            pixelValue = pixelValue >> alphaRightShift;
            if (pixelValue < maxAlphaValue)
                hasAlpha = true;
            else
                hasOpaque = true;
        }
    }
    if (alsoOpaque)
        *alsoOpaque = hasOpaque;
    return hasAlpha;
}
}

SLoadedTexture::~SLoadedTexture()
{
    if (dds) {
        if (dds->dataBlock)
            QT3DS_FREE(m_Allocator, dds->dataBlock);

        QT3DS_FREE(m_Allocator, dds);
    } else if (data && image.byteCount() <= 0) {
        m_Allocator.deallocate(data);
    }
    if (m_Palette)
        m_Allocator.deallocate(m_Palette);
    if (m_TransparencyTable)
        m_Allocator.deallocate(m_TransparencyTable);
}

void SLoadedTexture::release()
{
    NVAllocatorCallback *theAllocator(&m_Allocator);
    this->~SLoadedTexture();
    theAllocator->deallocate(this);
}

bool SLoadedTexture::ScanForTransparency(bool &alsoOpaquePixels)
{
    switch (format) {
    case NVRenderTextureFormats::SRGB8A8:
    case NVRenderTextureFormats::RGBA8:
        if (!data) { // dds
            return true;
        } else {
            return ScanImageForAlpha(data, width, height, 4, 8, &alsoOpaquePixels);
        }
        break;
    // Scan the image.
    case NVRenderTextureFormats::SRGB8:
    case NVRenderTextureFormats::RGB8:
        return false;
        break;
    case NVRenderTextureFormats::RGB565:
        return false;
        break;
    case NVRenderTextureFormats::RGBA5551:
        if (!data) { // dds
            return true;
        } else {
            return ScanImageForAlpha(data, width, height, 2, 1, &alsoOpaquePixels);
        }
        break;
    case NVRenderTextureFormats::Alpha8:
        return true;
        break;
    case NVRenderTextureFormats::Luminance8:
        return false;
        break;
    case NVRenderTextureFormats::LuminanceAlpha8:
        if (!data) { // dds
            return true;
        } else {
            return ScanImageForAlpha(data, width, height, 2, 8);
        }
        break;
    case NVRenderTextureFormats::RGB_DXT1:
        return false;
        break;
    case NVRenderTextureFormats::RGBA_DXT3:
    case NVRenderTextureFormats::RGBA_DXT1:
    case NVRenderTextureFormats::RGBA_DXT5:
        if (dds) {
            return ScanDDSForAlpha(dds, &alsoOpaquePixels);
        } else {
            QT3DS_ASSERT(false);
            return false;
        }
        break;
    case NVRenderTextureFormats::RGB9E5:
        return false;
        break;
    case NVRenderTextureFormats::RG32F:
    case NVRenderTextureFormats::RGB32F:
    case NVRenderTextureFormats::RGBA16F:
    case NVRenderTextureFormats::RGBA32F:
        // PKC TODO : For now, since IBL will be the main consumer, we'll just pretend there's no
        // alpha.
        // Need to do a proper scan down the line, but doing it for floats is a little different
        // from
        // integer scans.
        return false;
        break;
    default:
        break;
    }
    QT3DS_ASSERT(false);
    return false;
}

void SLoadedTexture::EnsureMultiplerOfFour(NVFoundationBase &inFoundation, const char *inPath)
{
    if (width % 4 || height % 4) {
        qCWarning(PERF_WARNING,
            "Image %s has non multiple of four width or height; perf hit for scaling", inPath);
        if (data) {
            QT3DSU32 newWidth = ITextRenderer::NextMultipleOf4(width);
            QT3DSU32 newHeight = ITextRenderer::NextMultipleOf4(height);
            QT3DSU32 newDataSize = newWidth * newHeight * components;
            NVAllocatorCallback &theAllocator(inFoundation.getAllocator());
            QT3DSU8 *newData = (QT3DSU8 *)(theAllocator.allocate(newDataSize, "Scaled Image Data",
                                                           __FILE__, __LINE__));
            CImageScaler theScaler(theAllocator);
            if (components == 4) {
                theScaler.FastExpandRowsAndColumns((unsigned char *)data, width, height, newData,
                                                   newWidth, newHeight);
            } else
                theScaler.ExpandRowsAndColumns((unsigned char *)data, width, height, newData,
                                               newWidth, newHeight, components);

            theAllocator.deallocate(data);
            data = newData;
            width = newWidth;
            height = newHeight;
            dataSizeInBytes = newDataSize;
        }
    }
}

STextureData SLoadedTexture::DecompressDXTImage(int inMipMapIdx, STextureData *inOptLastImage)
{
    STextureData retval;
    if (inOptLastImage)
        retval = *inOptLastImage;

    if (dds == NULL || inMipMapIdx >= dds->numMipmaps) {
        QT3DS_ASSERT(false);
        ReleaseDecompressedTexture(retval);
        return STextureData();
    }
    char *srcData = (char *)dds->data[inMipMapIdx];
    int srcDataSize = dds->size[inMipMapIdx];
    QT3DSU32 imgWidth = (QT3DSU32)dds->mipwidth[inMipMapIdx];
    QT3DSU32 imgHeight = (QT3DSU32)dds->mipheight[inMipMapIdx];

    switch (format) {
    case NVRenderTextureFormats::RGB_DXT1:
        DecompressDDS<DXT_BLOCKDECODER_1>(
            srcData, srcDataSize, imgWidth, imgHeight,
            STextureDataWriter(imgWidth, imgHeight, false, retval, m_Allocator));
        break;
    case NVRenderTextureFormats::RGBA_DXT1:
        DecompressDDS<DXT_BLOCKDECODER_1>(
            srcData, srcDataSize, imgWidth, imgHeight,
            STextureDataWriter(imgWidth, imgHeight, true, retval, m_Allocator));
        break;
    case NVRenderTextureFormats::RGBA_DXT3:
        DecompressDDS<DXT_BLOCKDECODER_3>(
            srcData, srcDataSize, imgWidth, imgHeight,
            STextureDataWriter(imgWidth, imgHeight, true, retval, m_Allocator));
        break;
    case NVRenderTextureFormats::RGBA_DXT5:
        DecompressDDS<DXT_BLOCKDECODER_5>(
            srcData, srcDataSize, imgWidth, imgHeight,
            STextureDataWriter(imgWidth, imgHeight, true, retval, m_Allocator));
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }
    return retval;
}

void SLoadedTexture::ReleaseDecompressedTexture(STextureData inImage)
{
    if (inImage.data)
        m_Allocator.deallocate(inImage.data);
}

#ifndef EA_PLATFORM_WINDOWS
#define stricmp strcasecmp
#endif

SLoadedTexture *SLoadedTexture::Load(const QString &inPath, NVFoundationBase &inFoundation,
                                     IInputStreamFactory &inFactory, bool inFlipY,
                                     NVRenderContextType renderContextType, bool preferKTX)
{
    if (inPath.isEmpty())
        return nullptr;

    // Check KTX path first
    QString path = inPath;
    QString ktxSource = inPath;
    if (preferKTX) {
        ktxSource = ktxSource.left(ktxSource.lastIndexOf(QLatin1Char('.')));
        ktxSource.append(QLatin1String(".ktx"));
    }

    SLoadedTexture *theLoadedImage = nullptr;
    // We will get invalid error logs of files not found if we don't force quiet mode
    // If the file is actually missing, it will be logged later (loaded image is null)
    NVScopedRefCounted<IRefCountedInputStream> theStream(
                inFactory.GetStreamForFile(preferKTX ? ktxSource : inPath, true));
    if (!theStream.mPtr) {
        if (preferKTX)
            theStream = inFactory.GetStreamForFile(inPath, true);
        else
            return nullptr;
    } else {
        path = ktxSource;
    }
    QString fileName;
    inFactory.GetPathForFile(path, fileName, true);
    if (theStream.mPtr && path.size() > 3) {
        if (path.endsWith(QLatin1String("png"), Qt::CaseInsensitive)
                || path.endsWith(QLatin1String("jpg"), Qt::CaseInsensitive)
                || path.endsWith(QLatin1String("peg"), Qt::CaseInsensitive)) {
            theLoadedImage = LoadQImage(fileName, inFlipY, inFoundation, renderContextType);
        } else if (path.endsWith(QLatin1String("dds"), Qt::CaseInsensitive)) {
            theLoadedImage = LoadDDS(*theStream, inFlipY, inFoundation, renderContextType);
        } else if (path.endsWith(QLatin1String("gif"), Qt::CaseInsensitive)) {
            theLoadedImage = LoadGIF(*theStream, !inFlipY, inFoundation, renderContextType);
        } else if (path.endsWith(QLatin1String("bmp"), Qt::CaseInsensitive)) {
            theLoadedImage = LoadBMP(*theStream, !inFlipY, inFoundation, renderContextType);
        } else if (path.endsWith(QLatin1String("hdr"), Qt::CaseInsensitive)) {
            theLoadedImage = LoadHDR(*theStream, inFoundation, renderContextType);
        } else if (path.endsWith(QLatin1String("ktx"), Qt::CaseInsensitive)) {
            theLoadedImage = LoadKTX(*theStream, inFlipY, inFoundation, renderContextType);
        } else {
            qCWarning(INTERNAL_ERROR, "Unrecognized image extension: %s", qPrintable(inPath));
        }
    }

    return theLoadedImage;
}
