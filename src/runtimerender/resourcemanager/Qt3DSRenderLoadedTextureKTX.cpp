/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "foundation/Qt3DSFoundation.h"
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSRenderLoadedTexture.h"
#include "Qt3DSRenderLoadedTextureKTX.h"
#include "Qt3DSRenderLoadedTextureDDS.h"

#include <qendian.h>
#include <qopengltexture.h>

using namespace qt3ds::render;
using namespace qt3ds::foundation;

namespace qt3ds {
namespace render {

static inline int blockSizeForTextureFormat(int format)
{
    switch (format) {
    case QOpenGLTexture::RGB8_ETC1:
    case QOpenGLTexture::RGB8_ETC2:
    case QOpenGLTexture::SRGB8_ETC2:
    case QOpenGLTexture::RGB8_PunchThrough_Alpha1_ETC2:
    case QOpenGLTexture::SRGB8_PunchThrough_Alpha1_ETC2:
    case QOpenGLTexture::R11_EAC_UNorm:
    case QOpenGLTexture::R11_EAC_SNorm:
    case QOpenGLTexture::RGB_DXT1:
        return 8;

    default:
        return 16;
    }
}

static inline int runtimeFormat(quint32 internalFormat)
{
    switch (internalFormat) {
    case QOpenGLTexture::RGBA8_ETC2_EAC:
        return NVRenderTextureFormats::RGBA8_ETC2_EAC;
    case QOpenGLTexture::RGB8_ETC1:
        return NVRenderTextureFormats::RGB8_ETC1;
    case QOpenGLTexture::RGB8_ETC2:
        return NVRenderTextureFormats::RGB8_ETC2;
    case QOpenGLTexture::SRGB8_ETC2:
        return NVRenderTextureFormats::SRGB8_ETC2;
    case QOpenGLTexture::RGB8_PunchThrough_Alpha1_ETC2:
        return NVRenderTextureFormats::RGB8_PunchThrough_Alpha1_ETC2;
    case QOpenGLTexture::SRGB8_PunchThrough_Alpha1_ETC2:
        return NVRenderTextureFormats::SRGB8_PunchThrough_Alpha1_ETC2;
    case QOpenGLTexture::R11_EAC_UNorm:
        return NVRenderTextureFormats::R11_EAC_UNorm;
    case QOpenGLTexture::R11_EAC_SNorm:
        return NVRenderTextureFormats::R11_EAC_SNorm;
    case QOpenGLTexture::RGB_DXT1:
        return NVRenderTextureFormats::RGB_DXT1;
    case QOpenGLTexture::RGBA_DXT1:
        return NVRenderTextureFormats::RGBA_DXT3;
    case QOpenGLTexture::RGBA_DXT3:
        return NVRenderTextureFormats::RGBA_DXT3;
    case QOpenGLTexture::RGBA_DXT5:
        return NVRenderTextureFormats::RGBA_DXT5;
    case QOpenGLTexture::RGBA_ASTC_4x4:
        return NVRenderTextureFormats::RGBA_ASTC_4x4;
    case QOpenGLTexture::RGBA_ASTC_5x4:
        return NVRenderTextureFormats::RGBA_ASTC_5x4;
    case QOpenGLTexture::RGBA_ASTC_5x5:
        return NVRenderTextureFormats::RGBA_ASTC_5x5;
    case QOpenGLTexture::RGBA_ASTC_6x5:
        return NVRenderTextureFormats::RGBA_ASTC_6x5;
    case QOpenGLTexture::RGBA_ASTC_6x6:
        return NVRenderTextureFormats::RGBA_ASTC_6x6;
    case QOpenGLTexture::RGBA_ASTC_8x5:
        return NVRenderTextureFormats::RGBA_ASTC_8x5;
    case QOpenGLTexture::RGBA_ASTC_8x6:
        return NVRenderTextureFormats::RGBA_ASTC_8x6;
    case QOpenGLTexture::RGBA_ASTC_8x8:
        return NVRenderTextureFormats::RGBA_ASTC_8x8;
    case QOpenGLTexture::RGBA_ASTC_10x5:
        return NVRenderTextureFormats::RGBA_ASTC_10x5;
    case QOpenGLTexture::RGBA_ASTC_10x6:
        return NVRenderTextureFormats::RGBA_ASTC_10x6;
    case QOpenGLTexture::RGBA_ASTC_10x8:
        return NVRenderTextureFormats::RGBA_ASTC_10x8;
    case QOpenGLTexture::RGBA_ASTC_10x10:
        return NVRenderTextureFormats::RGBA_ASTC_10x10;
    case QOpenGLTexture::RGBA_ASTC_12x10:
        return NVRenderTextureFormats::RGBA_ASTC_12x10;
    case QOpenGLTexture::RGBA_ASTC_12x12:
        return NVRenderTextureFormats::RGBA_ASTC_12x12;
    default:
        break;
    }
    return NVRenderTextureFormats::Unknown;
}

static inline int imageSize(QT3DSI32 width, QT3DSI32 height, const Qt3DSDDSImage *image)
{
    return ((width + 3) / 4) * ((height + 3) / 4)
            * blockSizeForTextureFormat(image->internalFormat);
}

static inline quint32 totalImageDataSize(Qt3DSDDSImage *image)
{
    int i, j;
    int index = 0;
    quint32 size = 0;
    int w, h;
    int cubeCount = image->cubemap ? 6 : 1;

    for (j = 0; j < cubeCount; j++) {
        w = image->width;
        h = image->height;

        for (i = 0; i < image->numMipmaps; i++) // account for base plus each mip
        {
            size += 4; // image size is saved in the file
            image->size[index] = imageSize(w, h, image);
            image->mipwidth[index] = w;
            image->mipheight[index] = h;
            size += quint32(image->size[index]);
            if (w != 1)
                w >>= 1;
            if (h != 1)
                h >>= 1;

            index++;
        }
    }

    return (size);
}

static inline quint32 alignedOffset(quint32 offset, quint32 byteAlign)
{
    return (offset + byteAlign - 1) & ~(byteAlign - 1);
}

inline SLoadedTexture *loadKtx(NVAllocatorCallback &allocator, IInStream &inStream,
                               QT3DSI32 flipVertical)
{
    static const int KTX_IDENTIFIER_LENGTH = 12;
    static const char ktxIdentifier[KTX_IDENTIFIER_LENGTH] = {
        '\xAB', 'K', 'T', 'X', ' ', '1', '1', '\xBB', '\r', '\n', '\x1A', '\n'
    };
    static const quint32 platformEndianIdentifier = 0x04030201;
    static const quint32 inversePlatformEndianIdentifier = 0x01020304;

    struct KTXHeader {
        quint8 identifier[KTX_IDENTIFIER_LENGTH];
        quint32 endianness;
        quint32 glType;
        quint32 glTypeSize;
        quint32 glFormat;
        quint32 glInternalFormat;
        quint32 glBaseInternalFormat;
        quint32 pixelWidth;
        quint32 pixelHeight;
        quint32 pixelDepth;
        quint32 numberOfArrayElements;
        quint32 numberOfFaces;
        quint32 numberOfMipmapLevels;
        quint32 bytesOfKeyValueData;
    };

    KTXHeader header;
    if (inStream.Read(header) != sizeof(header)
            || qstrncmp(reinterpret_cast<char *>(header.identifier),
                        ktxIdentifier, KTX_IDENTIFIER_LENGTH) != 0
            || (header.endianness != platformEndianIdentifier
                && header.endianness != inversePlatformEndianIdentifier)) {
        return nullptr;
    }

    const bool isInverseEndian = (header.endianness == inversePlatformEndianIdentifier);
    auto decode = [isInverseEndian](quint32 val) {
        return isInverseEndian ? qbswap<quint32>(val) : val;
    };

    const bool isCompressed = decode(header.glType) == 0 && decode(header.glFormat) == 0
            && decode(header.glTypeSize) == 1;
    if (!isCompressed) {
        qWarning("Uncompressed ktx texture data is not supported");
        return nullptr;
    }

    if (decode(header.numberOfArrayElements) != 0) {
        qWarning("Array ktx textures not supported");
        return nullptr;
    }

    if (decode(header.pixelDepth) != 0) {
        qWarning("Only 2D and cube ktx textures are supported");
        return nullptr;
    }

    const int bytesToSkip = int(decode(header.bytesOfKeyValueData));
    QVector<uint8_t> skipData;
    skipData.resize(bytesToSkip);
    if (inStream.Read(NVDataRef<uint8_t>(skipData.data(), bytesToSkip)) != bytesToSkip) {
        qWarning("Unexpected end of ktx data");
        return nullptr;
    }

    // now for each mipmap level we have (arrays and 3d textures not supported here)
    // uint32 imageSize
    // for each array element
    //   for each face
    //     for each z slice
    //       compressed data
    //     padding so that each face data starts at an offset that is a multiple of 4
    // padding so that each imageSize starts at an offset that is a multiple of 4

    Qt3DSDDSImage *image = (Qt3DSDDSImage *)QT3DS_ALLOC(allocator, sizeof(Qt3DSDDSImage), "DoLoadDDS");

    const quint32 level0Width = decode(header.pixelWidth);
    const quint32 level0Height = decode(header.pixelHeight);
    quint32 faceCount = decode(header.numberOfFaces);
    const quint32 mipMapLevels = decode(header.numberOfMipmapLevels);
    const quint32 format = decode(header.glInternalFormat);
    image->numMipmaps = int(mipMapLevels);
    image->cubemap = faceCount == 6 ? 6 : 0;
    image->internalFormat = int(format);
    image->format = runtimeFormat(format);
    image->width = int(level0Width);
    image->height = int(level0Height);
    image->compressed = 1;
    quint32 totalSize = totalImageDataSize(image);
    image->dataBlock = QT3DS_ALLOC(allocator, totalSize, "Qt3DSDDSAllocDataBlock");
    if (inStream.Read(NVDataRef<uint8_t>(reinterpret_cast<uint8_t*>(image->dataBlock), totalSize))
            != totalSize) {
        QT3DS_FREE(allocator, image);
        return nullptr;
    }

    SLoadedTexture *result = QT3DS_NEW(allocator, SLoadedTexture)(allocator);
    result->dds = image;
    result->width = int(level0Width);
    result->height = int(level0Height);
    result->format = static_cast<NVRenderTextureFormats::Enum>(image->format);

    // TODO: Proper support for cubemaps should be implemented at some point.
    if (faceCount > 1) {
        qWarning("Multiple faces (cubemaps) not currently supported in ktx");
        faceCount = 1;
    }

    uint8_t *p = reinterpret_cast<uint8_t *>(image->dataBlock);
    uint8_t *basep = p;

    for (quint32 mip = 0; mip < mipMapLevels; ++mip) {
        if (p + 4 - basep > totalSize)
            break;
        const quint32 imageSize = *reinterpret_cast<const quint32 *>(p);
        p += 4;
        for (quint32 face = 0; face < faceCount; ++face) {
            const quint32 nextOffset = quint32(p + imageSize - basep);
            if (nextOffset > totalSize)
                break;
            image->data[mip] = reinterpret_cast<void *>(p);
            p = basep + alignedOffset(nextOffset, 4);
        }
    }

    return result;
}

SLoadedTexture *SLoadedTexture::LoadKTX(IInStream &inStream, QT3DSI32 flipVertical,
                                        NVFoundationBase &inFnd,
                                        qt3ds::render::NVRenderContextType renderContextType)
{
    Q_UNUSED(renderContextType)
    SLoadedTexture *retval = loadKtx(inFnd.getAllocator(), inStream, flipVertical);

    return retval;
}

}
}
