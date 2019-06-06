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
#include "Qt3DSTextRenderer.h"
#include "render/Qt3DSRenderTexture2D.h"

using namespace qt3ds::render;

// http://acius2.blogspot.com/2007/11/calculating-next-power-of-2.html
QT3DSU32 ITextRenderer::NextPowerOf2(QT3DSU32 input)
{
    // Algorithm doesn't work for 0 or QT3DS_MAX_U32
    QT3DS_ASSERT(input > 0 && input < QT3DS_MAX_U32);
    input--;
    input = (input >> 1) | input;
    input = (input >> 2) | input;
    input = (input >> 4) | input;
    input = (input >> 8) | input;
    input = (input >> 16) | input;
    input++; // input is now the next highest power of 2.
    return input;
}

QT3DSU32 ITextRenderer::NextMultipleOf4(QT3DSU32 inValue)
{
    QT3DSU32 remainder(inValue % 4);
    if (remainder != 0)
        inValue = inValue + (4 - remainder);

    return inValue;
}

STextTextureDetails ITextRenderer::UploadData(NVDataRef<QT3DSU8> inTextureData,
                                              NVRenderTexture2D &inTexture, QT3DSU32 inDataWidth,
                                              QT3DSU32 inDataHeight, QT3DSU32 inTextWidth,
                                              QT3DSU32 inTextHeight,
                                              NVRenderTextureFormats::Enum inFormat,
                                              bool inFlipYAxis)
{
    if (inTextWidth == 0 || inTextHeight == 0) {
        QT3DSU32 black[] = { 0, 0, 0, 0 };
        inTexture.SetTextureData(toU8DataRef(black, 4), 0, 2, 2, NVRenderTextureFormats::RGBA8);
        return STextTextureDetails(2, 2, false, QT3DSVec2(1.0));
    }
    QT3DS_ASSERT(NextMultipleOf4(inDataWidth) == inDataWidth);
    QT3DSU32 theNecessaryHeight = NextMultipleOf4(inTextHeight);
    QT3DSU32 dataStride = inDataWidth * NVRenderTextureFormats::getSizeofFormat(inFormat);
    if (inTextureData.size() < dataStride * inDataHeight) {
        QT3DS_ASSERT(false);
        return STextTextureDetails();
    }

    STextureDetails theTextureDetails = inTexture.GetTextureDetails();
    QT3DSU32 theUploadSize = theNecessaryHeight * dataStride;

    NVDataRef<QT3DSU8> theUploadData = NVDataRef<QT3DSU8>(inTextureData.begin(), theUploadSize);
    inTexture.SetTextureData(theUploadData, 0, inDataWidth, theNecessaryHeight, inFormat);
    inTexture.SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Linear);
    inTexture.SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Linear);
    return STextTextureDetails(inTextWidth, inTextHeight, inFlipYAxis, QT3DSVec2(1.0f, 1.0f));
}
