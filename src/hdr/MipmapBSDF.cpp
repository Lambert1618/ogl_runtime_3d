/****************************************************************************
**
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

#include "MipmapBSDF.h"
#include "GLComputeMipMap.h"

#ifdef PLATFORM_HAS_CUDA
#include "cuda.h"
#include "cuda_runtime.h"
#include "CUDABSDFMipmap.h"
#endif

#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "foundation/Qt3DSRefCounted.h"
#include "nv_log.h"

using namespace qt3ds;
using namespace qt3ds::render;
using namespace qt3ds::foundation;

BSDFMipMap::BSDFMipMap(NVRenderContext *inNVRenderContext, int inWidth, int inHeight,
                       NVRenderTexture2D &inTexture2D, NVRenderTextureFormats::Enum inDestFormat,
                       NVFoundationBase &inFnd)
    : m_Foundation(inFnd)
    , m_Texture2D(inTexture2D)
    , m_Width(inWidth)
    , m_Height(inHeight)
    , m_DestinationFormat(inDestFormat)
    , m_NVRenderContext(inNVRenderContext)
{
    // Calculate mip level
    int maxDim = inWidth >= inHeight ? inWidth : inHeight;

    m_MaxMipMapLevel = static_cast<int>(logf((float)maxDim) / logf(2.0f));
    // no concept of sizeOfFormat just does'nt make sense
    m_SizeOfFormat = NVRenderTextureFormats::getSizeofFormat(m_DestinationFormat);
    m_NoOfComponent = NVRenderTextureFormats::getNumberOfComponent(m_DestinationFormat);
}

BSDFMipMap *BSDFMipMap::Create(NVRenderContext *inNVRenderContext, int inWidth, int inHeight,
                               NVRenderTexture2D &inTexture2D,
                               NVRenderTextureFormats::Enum inDestFormat,
                               qt3ds::NVFoundationBase &inFnd)
{
    BSDFMipMap *theBSDFMipMap = NULL;
#ifdef PLATFORM_HAS_CUDA
    int deviceCount;
    cudaError_t e = cudaGetDeviceCount(&deviceCount);
#endif

    if (inNVRenderContext->IsComputeSupported()) {
        theBSDFMipMap = QT3DS_NEW(inFnd.getAllocator(), GLComputeMipMap)(
            inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat, inFnd);
    } else
#ifdef PLATFORM_HAS_CUDA
        if (e == cudaSuccess && deviceCount > 0) {
        theBSDFMipMap = QT3DS_NEW(inFnd.getAllocator(), CUDABSDFMipMap)(
            inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat, inFnd);
    } else
#endif
        if (!theBSDFMipMap) {
        theBSDFMipMap = QT3DS_NEW(inFnd.getAllocator(), BasicBSDFMipMap)(
            inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat, inFnd);
    }

    return theBSDFMipMap;
}

BasicBSDFMipMap::BasicBSDFMipMap(NVRenderContext *inNVRenderContext, int inWidth, int inHeight,
                                 NVRenderTexture2D &inTexture2D,
                                 NVRenderTextureFormats::Enum inDestFormat, NVFoundationBase &inFnd)
    : BSDFMipMap(inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat, inFnd)
{
}

BSDFMipMap::~BSDFMipMap()
{
}

void BasicBSDFMipMap::Build(void *inTextureData, int inTextureDataSize,
                            NVRenderBackend::NVRenderBackendTextureObject,
                            NVRenderTextureFormats::Enum inFormat)
{

    m_InternalFormat = inFormat;
    m_SizeOfInternalFormat = NVRenderTextureFormats::getSizeofFormat(m_InternalFormat);
    m_InternalNoOfComponent = NVRenderTextureFormats::getNumberOfComponent(m_InternalFormat);

    m_Texture2D.SetTextureData(NVDataRef<QT3DSU8>((QT3DSU8 *)inTextureData, inTextureDataSize), 0,
                               m_Width, m_Height, inFormat, m_DestinationFormat);

    STextureData theMipImage;
    STextureData prevImage;
    prevImage.data = inTextureData;
    prevImage.dataSizeInBytes = inTextureDataSize;
    prevImage.format = inFormat;
    int curWidth = m_Width;
    int curHeight = m_Height;
    int size = NVRenderTextureFormats::getSizeofFormat(m_InternalFormat);
    for (int idx = 1; idx <= m_MaxMipMapLevel; ++idx) {
        theMipImage =
            CreateBsdfMipLevel(theMipImage, prevImage, curWidth, curHeight); //, m_PerfTimer );
        curWidth = curWidth >> 1;
        curHeight = curHeight >> 1;
        curWidth = curWidth >= 1 ? curWidth : 1;
        curHeight = curHeight >= 1 ? curHeight : 1;
        inTextureDataSize = curWidth * curHeight * size;

        m_Texture2D.SetTextureData(toU8DataRef((char *)theMipImage.data, (QT3DSU32)inTextureDataSize),
                                   (QT3DSU8)idx, (QT3DSU32)curWidth, (QT3DSU32)curHeight, theMipImage.format,
                                   m_DestinationFormat);

        if (prevImage.data == inTextureData)
            prevImage = STextureData();

        STextureData temp = prevImage;
        prevImage = theMipImage;
        theMipImage = temp;
    }
    QT3DS_FREE(m_Foundation.getAllocator(), theMipImage.data);
    QT3DS_FREE(m_Foundation.getAllocator(), prevImage.data);
}

inline int BasicBSDFMipMap::wrapMod(int a, int base)
{
    return (a >= 0) ? a % base : (a % base) + base;
}

inline void BasicBSDFMipMap::getWrappedCoords(int &sX, int &sY, int width, int height)
{
    if (sY < 0) {
        sX -= width >> 1;
        sY = -sY;
    }
    if (sY >= height) {
        sX += width >> 1;
        sY = height - sY;
    }
    sX = wrapMod(sX, width);
}

STextureData BasicBSDFMipMap::CreateBsdfMipLevel(STextureData &inCurMipLevel,
                                                 STextureData &inPrevMipLevel, int width,
                                                 int height) //, IPerfTimer& inPerfTimer )
{
    //	SStackPerfTimer __timer( inPerfTimer, "Image BSDF Mip Level" );
    STextureData retval;
    int newWidth = width >> 1;
    int newHeight = height >> 1;
    newWidth = newWidth >= 1 ? newWidth : 1;
    newHeight = newHeight >= 1 ? newHeight : 1;

    if (inCurMipLevel.data) {
        retval = inCurMipLevel;
        retval.dataSizeInBytes =
            newWidth * newHeight * NVRenderTextureFormats::getSizeofFormat(inPrevMipLevel.format);
    } else {
        retval.dataSizeInBytes =
            newWidth * newHeight * NVRenderTextureFormats::getSizeofFormat(inPrevMipLevel.format);
        retval.format = inPrevMipLevel.format; // inLoadedImage.format;
        retval.data = m_Foundation.getAllocator().allocate(
            retval.dataSizeInBytes, "Bsdf Scaled Image Data", __FILE__, __LINE__);
    }

    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            float accumVal[4];
            accumVal[0] = 0;
            accumVal[1] = 0;
            accumVal[2] = 0;
            accumVal[3] = 0;
            for (int sy = -2; sy <= 2; ++sy) {
                for (int sx = -2; sx <= 2; ++sx) {
                    int sampleX = sx + (x << 1);
                    int sampleY = sy + (y << 1);
                    getWrappedCoords(sampleX, sampleY, width, height);

                    // Cauchy filter (this is simply because it's the easiest to evaluate, and
                    // requires no complex
                    // functions).
                    float filterPdf = 1.f / (1.f + float(sx * sx + sy * sy) * 2.f);
                    // With FP HDR formats, we're not worried about intensity loss so much as
                    // unnecessary energy gain,
                    // whereas with LDR formats, the fear with a continuous normalization factor is
                    // that we'd lose
                    // intensity and saturation as well.
                    filterPdf /= (NVRenderTextureFormats::getSizeofFormat(retval.format) >= 8)
                        ? 4.71238898f
                        : 4.5403446f;
                    // filterPdf /= 4.5403446f;		// Discrete normalization factor
                    // filterPdf /= 4.71238898f;		// Continuous normalization factor
                    float curPix[4];
                    QT3DSI32 byteOffset = (sampleY * width + sampleX)
                        * NVRenderTextureFormats::getSizeofFormat(retval.format);
                    if (byteOffset < 0) {
                        sampleY = height + sampleY;
                        byteOffset = (sampleY * width + sampleX)
                            * NVRenderTextureFormats::getSizeofFormat(retval.format);
                    }

                    NVRenderTextureFormats::decodeToFloat(inPrevMipLevel.data, byteOffset, curPix,
                                                          retval.format);

                    accumVal[0] += filterPdf * curPix[0];
                    accumVal[1] += filterPdf * curPix[1];
                    accumVal[2] += filterPdf * curPix[2];
                    accumVal[3] += filterPdf * curPix[3];
                }
            }

            /*
            // Re-adjustment after the fact for the RGBD hack.
            if (retval.format == NVRenderTextureFormats::RGBA8 || retval.format ==
            NVRenderTextureFormats::SRGB8A8)
            {
                    float divVal = (accumVal[0] > accumVal[1]) ? accumVal[0] : accumVal[1];
                    divVal = (divVal > accumVal[2]) ? divVal : accumVal[2];
                    if (divVal > 1.0)
                    {
                            divVal = 1.0f / divVal;
                            accumVal[0] *= divVal;
                            accumVal[1] *= divVal;
                            accumVal[2] *= divVal;
                            accumVal[3] = divVal;
                    }
                    else
                            accumVal[3] = 1.0f;
            }
            */
            QT3DSU32 newIdx =
                (y * newWidth + x) * NVRenderTextureFormats::getSizeofFormat(retval.format);

            NVRenderTextureFormats::encodeToPixel(accumVal, retval.data, newIdx, retval.format);
        }
    }

    return retval;
}
