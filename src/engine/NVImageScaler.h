/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

namespace Q3DStudio {
namespace ImageScaler {

    //==============================================================================
    /**
     *
     */
    template <INT32 TNumChannels>
    inline void BilinearReduceRows(const CHAR *inSrcBuffer, INT32 inSrcWidth, INT32 inSrcHeight,
                                   CHAR *outDstBuffer, INT32 inDstHeight)
    {
        INT32 theDDAConst = static_cast<INT32>(1024.0 * inDstHeight / inSrcHeight);
        INT32 theDDAAccum = 0;
        INT32 thePixelCount;

        INT32 theSrcRow;
        INT32 theSrcCol;
        INT32 theDstRow;

        INT32 theChannelAccum[TNumChannels] = { 0 };

        CHAR *theDstPointer = outDstBuffer;
        const CHAR *theSrcPointer = inSrcBuffer;
        const CHAR *theSrcColPointer = NULL;
        CHAR *theDstColPointer = NULL;

        INT32 theStepSize = TNumChannels;
        INT32 theSrcStride = TNumChannels * inSrcWidth;
        INT32 theDstStride = TNumChannels * inSrcWidth;

        for (theSrcCol = 0; theSrcCol < inSrcWidth; ++theSrcCol) {
            theSrcColPointer = theSrcPointer + (theSrcCol * theStepSize);
            theDstColPointer = theDstPointer + (theSrcCol * theStepSize);

            theSrcRow = 0L;
            theDstRow = 0L;
            thePixelCount = 0L;

            for (INT32 idx = 0; idx < TNumChannels; ++idx)
                theChannelAccum[idx] = 0L;

            theDDAAccum = 0L;

            while (theSrcRow < inSrcHeight) {
                while ((theDDAAccum < 1024L) && (theSrcRow < inSrcHeight)) {
                    for (INT32 idx = 0; idx < TNumChannels; ++idx)
                        theChannelAccum[idx] +=
                            1024L * theSrcColPointer[(theSrcRow * theSrcStride) + idx];

                    theDDAAccum += theDDAConst;
                    thePixelCount += 1024L;
                    ++theSrcRow;
                }

                theDDAAccum = (theSrcRow < inSrcHeight) ? (theDDAAccum - 1024L) : (0L);
                thePixelCount -= theDDAAccum;

                for (INT32 idx = 0; idx < TNumChannels; ++idx)
                    theChannelAccum[idx] -= theDDAAccum
                        * (INT32)theSrcColPointer[((theSrcRow - 1) * theSrcStride) + idx];

                for (INT32 idx = 0; idx < TNumChannels; ++idx)
                    theDstColPointer[(theDstRow * theDstStride) + idx] =
                        (CHAR)(theChannelAccum[idx] / thePixelCount);

                thePixelCount = 1024L - theDDAAccum;
                ++theDstRow;

                if (theDstRow >= inDstHeight)
                    break;

                for (INT32 idx = 0; idx < TNumChannels; ++idx)
                    theChannelAccum[idx] = thePixelCount
                        * (INT32)theSrcColPointer[((theSrcRow - 1) * theSrcStride) + idx];
            }
        }
    }

    inline void BilinearReduceRows(const CHAR *inSrcBuffer, INT32 inSrcWidth, INT32 inSrcHeight,
                                   INT32 inChannels, CHAR *outDstBuffer, INT32 inDstHeight)
    {
        switch (inChannels) {
        case 4:
            BilinearReduceRows<4>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstHeight);
            break;
        case 3:
            BilinearReduceRows<3>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstHeight);
            break;
        case 2:
            BilinearReduceRows<2>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstHeight);
            break;
        case 1:
            BilinearReduceRows<1>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstHeight);
            break;
        }
    }

    template <INT32 TNumChannels>
    inline void BilinearReduceCols(const CHAR *inSrcBuffer, INT32 inSrcWidth, INT32 inSrcHeight,
                                   CHAR *outDstBuffer, INT32 inDstWidth)
    {
        INT32 theDDAConst = static_cast<INT32>(1024.0 * inDstWidth / inSrcWidth);
        INT32 theDDAAccum = 0L;
        INT32 thePixelCount;

        INT32 theSrcRow;
        INT32 theSrcCol;
        INT32 theDstCol;

        INT32 theChannelAccum[TNumChannels];

        CHAR *theDstPointer = outDstBuffer;
        const CHAR *theSrcPointer = inSrcBuffer;
        const CHAR *theSrcRowPointer;
        CHAR *theDstRowPointer;

        INT32 theSrcStepSize = TNumChannels;
        INT32 theDstStepSize = TNumChannels;

        for (theSrcRow = 0; theSrcRow < inSrcHeight; ++theSrcRow) {

            theSrcRowPointer = theSrcPointer + (theSrcRow * inSrcWidth * theSrcStepSize);
            theDstRowPointer = theDstPointer + (theSrcRow * inDstWidth * theDstStepSize);

            theSrcCol = 0L;
            theDstCol = 0L;
            for (INT32 idx = 0; idx < TNumChannels; ++idx)
                theChannelAccum[idx] = 0L;
            thePixelCount = 0L;
            theDDAAccum = 0L;

            while (theSrcCol < inSrcWidth) {
                while ((theDDAAccum < 1024L) && (theSrcCol < inSrcWidth)) {
                    for (INT32 idx = 0; idx < TNumChannels; ++idx)
                        theChannelAccum[idx] +=
                            1024L * theSrcRowPointer[(theSrcCol * theSrcStepSize) + idx];

                    theDDAAccum += theDDAConst;
                    thePixelCount += 1024L;
                    ++theSrcCol;
                }

                theDDAAccum = (theSrcCol < inSrcWidth) ? (theDDAAccum - 1024L) : (0L);
                thePixelCount -= theDDAAccum;

                for (INT32 idx = 0; idx < TNumChannels; ++idx)
                    theChannelAccum[idx] -= theDDAAccum
                        * (INT32)theSrcRowPointer[((theSrcCol - 1) * theSrcStepSize) + idx];

                for (INT32 idx = 0; idx < TNumChannels; ++idx)
                    theDstRowPointer[(theDstCol * theDstStepSize) + idx] =
                        (CHAR)(theChannelAccum[idx] / thePixelCount);

                thePixelCount = 1024L - theDDAAccum;
                ++theDstCol;

                if (theDstCol >= inDstWidth) {
                    break;
                }

                for (INT32 idx = 0; idx < TNumChannels; ++idx)
                    theChannelAccum[idx] = thePixelCount
                        * (INT32)theSrcRowPointer[((theSrcCol - 1) * theSrcStepSize) + idx];
            }
        }
    }

    inline void BilinearReduceCols(const CHAR *inSrcBuffer, INT32 inSrcWidth, INT32 inSrcHeight,
                                   INT32 inChannels, CHAR *&outDstBuffer, INT32 inDstWidth)
    {
        switch (inChannels) {
        case 4:
            BilinearReduceCols<4>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstWidth);
            break;
        case 3:
            BilinearReduceCols<3>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstWidth);
            break;
        case 2:
            BilinearReduceCols<2>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstWidth);
            break;
        case 1:
            BilinearReduceCols<1>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstWidth);
            break;
        }
    }

    template <INT32 TNumChannels>
    inline void BilinearReduceImage(const CHAR *inSrcBuffer, INT32 inSrcWidth, INT32 inSrcHeight,
                                    CHAR *outDstBuffer, INT32 inDstWidth, INT32 inDstHeight)
    {
        // Number larger than 1024
        // One pixel in the source image is equivalent to this much of a pixel
        // in terms of dest pixels.
        INT32 theXIncrement = static_cast<INT32>(1024.f * inSrcWidth / inDstWidth);
        // How many rows do we skip, in fixed point arithmetic, when we bump lines
        INT32 theYIncrement = static_cast<INT32>(1024.f * inSrcHeight / inDstHeight);
        // We are starting at position .5 in textel space
        INT32 theSrcYPtr = 0;
        CHAR *theDstPtr = outDstBuffer;
        for (INT32 theHeightIndex = 0; theHeightIndex < inDstHeight;
             ++theHeightIndex, theSrcYPtr += theYIncrement) {
            INT32 theSrcXPtr = 0;
            for (INT32 theWidthIndex = 0; theWidthIndex < inDstWidth;
                 ++theWidthIndex, theSrcXPtr += theXIncrement, theDstPtr += TNumChannels) {
                INT32 theAccum[TNumChannels] = { 0 };
                INT32 theNumPixels = 0;
                // Pull all reasonable pixels from the source image.
                INT32 theStartRow = (theSrcYPtr / 1024);
                INT32 theStopRow = (theSrcYPtr + theYIncrement) / 1024;
                INT32 theStartColumn = theSrcXPtr / 1024;
                INT32 theStopColumn = (theSrcXPtr + theXIncrement) / 1024;
                // Average everything between the columns
                for (INT32 theRow = theStartRow; theRow < theStopRow; ++theRow) {
                    INT32 theSrcAddr = (theRow * inSrcWidth + theStartColumn) * TNumChannels;
                    for (INT32 theCol = theStartColumn; theCol < theStopColumn;
                         ++theCol, theSrcAddr += TNumChannels) {
                        ++theNumPixels;
                        for (INT32 idx = 0; idx < TNumChannels; ++idx)
                            theAccum[idx] += inSrcBuffer[theSrcAddr + idx];
                    }
                }

                for (INT32 idx = 0; idx < TNumChannels; ++idx)
                    theDstPtr[idx] = theAccum[idx] / theNumPixels;
            }
        }
    }

    template <INT32 TNumChannels>
    inline void NearestReduceImage(const CHAR *inSrcBuffer, INT32 inSrcWidth, INT32 inSrcHeight,
                                   CHAR *outDstBuffer, INT32 inDstWidth, INT32 inDstHeight)
    {
        // Number larger than 1024
        // One pixel in the source image is equivalent to this much of a pixel
        // in terms of dest pixels.
        INT32 theXIncrement = static_cast<INT32>(1024.f * inSrcWidth / inDstWidth);
        // How many rows do we skip, in fixed point arithmetic, when we bump lines
        INT32 theYIncrement = static_cast<INT32>(1024.f * inSrcHeight / inDstHeight);
        // We are starting at position .5 in textel space
        INT32 theSrcYPtr = theYIncrement / 2;
        CHAR *theDstPtr = outDstBuffer;
        for (INT32 theHeightIndex = 0; theHeightIndex < inDstHeight;
             ++theHeightIndex, theSrcYPtr += theYIncrement) {
            INT32 theSrcRow = (theSrcYPtr / 1024) * inSrcWidth * TNumChannels;
            INT32 theSrcXPtr = theXIncrement / 2;
            for (INT32 theWidthIndex = 0; theWidthIndex < inDstWidth;
                 ++theWidthIndex, theSrcXPtr += theXIncrement, theDstPtr += TNumChannels) {
                INT32 theSrcPtr = theSrcRow + (theSrcXPtr >> 10) * TNumChannels;
                for (INT32 idx = 0; idx < TNumChannels; ++idx)
                    theDstPtr[idx] = inSrcBuffer[theSrcPtr + idx];
            }
        }
    }

    inline void NearestReduceImage(const CHAR *inSrcBuffer, INT32 inSrcWidth, INT32 inSrcHeight,
                                   INT32 inChannels, CHAR *outDstBuffer, INT32 inDstWidth,
                                   INT32 inDstHeight)
    {
        switch (inChannels) {
        case 4:
            NearestReduceImage<4>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstWidth,
                                  inDstHeight);
            break;
        case 3:
            NearestReduceImage<3>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstWidth,
                                  inDstHeight);
            break;
        case 2:
            NearestReduceImage<2>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstWidth,
                                  inDstHeight);
            break;
        case 1:
            NearestReduceImage<1>(inSrcBuffer, inSrcWidth, inSrcHeight, outDstBuffer, inDstWidth,
                                  inDstHeight);
            break;
        }
    }
}
}