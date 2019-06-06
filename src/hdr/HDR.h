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
#ifndef HDR_H
#define HDR_H

#include "foundation/Qt3DSVec3.h"
//#include "HDR.h"

namespace qt3ds {
class QT3DSVec3;

namespace HDR {

    template <int N>
    class HDRConfiguration;

    template <int N>
    class Histogram
    {
    public:
        /**
         * @brief build the histogram based on 2^10 binning.
         *
         * @param[in]	inImage			Pointer to image
         * @param[in] 	inWidth			Width of image
         * @param[in] 	inHeight		Height of image
         * @param[out]	outHistogram
         *
         * @return No return
         */
        static void Build(QT3DSVec3 *inImage, int inWidth, int inHeight, QT3DSVec3 *outHistogram)
        {
            long noPixels = inWidth * inHeight;

            for (int i = 0; i < noPixels; ++i) {
                outHistogram[(int)inImage[i].x].x++;
                outHistogram[(int)inImage[i].y].y++;
                outHistogram[(int)inImage[i].z].z++;
            }
        }
    };

    template <int N>
    class HDR
    {

    public:
        HDR(HDRConfiguration<N> *inConfiguration) { mHDRConfiguration = inConfiguration; }

        void Build(QT3DSVec3 **inImages, int inNoImages, int inWidth, int inHeight, float *inExposures,
                   QT3DSVec3 *outRadiance)
        {
            for (int x = 0; x < inWidth; ++x) {
                for (int y = 0; y < inHeight; ++y) {
                    QT3DSVec3 divisor(0.0f);
                    QT3DSVec3 dividend(0.0f);
                    for (int i = 0; i < inNoImages; ++i) {
                        QT3DSVec3 pixel = inImages[i][x + y * inWidth];

                        dividend += (LUT(mHDRConfiguration->weights, pixel) * inExposures[i])
                                        .multiply(LUT(mHDRConfiguration->CRF, pixel));
                        divisor += LUT(mHDRConfiguration->weights, pixel) * inExposures[i]
                            * inExposures[i];
                    }
                    divisor.x = 1.0f / divisor.x;
                    divisor.y = 1.0f / divisor.y;
                    divisor.z = 1.0f / divisor.z;

                    outRadiance[x + y * inWidth] = dividend.multiply(divisor);
                }
            }
        }

    private:
        /**
         * @brief Return the value based off from array as a LUT
         *
         * @param[in]	inLUT			The LUT table of interest
         * @param[in]	inValue			The value that you have
         * @param[out]	QT3DSVec3			The corresponding value of array based on input
         * tuple
         *
         * @return No return
         */
        QT3DSVec3 LUT(float *inLUT, QT3DSVec3 inValue)
        {
            return QT3DSVec3(inLUT[(int)inValue.x], inLUT[(int)inValue.y], inLUT[(int)inValue.z]);
        }

        QT3DSVec3 LUT(QT3DSVec3 *inLUT, QT3DSVec3 inValue)
        {
            return QT3DSVec3(inLUT[(int)inValue.x].x, inLUT[(int)inValue.y].y,
                          inLUT[(int)inValue.z].z);
        }

        HDRConfiguration<N> *mHDRConfiguration;
    };

    template <int N>
    class HDRConfiguration
    {
    public:
        HDRConfiguration()
        {
            threshold = 0.1f;
            maxIterations = 30;
            GenerateRobertsonWeighting();
        }

        void SetCRF(QT3DSVec3 *inCRF) { memcpy(CRF, inCRF, sizeof(QT3DSVec3) * N); }

        /**
         * @brief build the camera response function
         *
         * @param[in]	inImages		Pointer to images
         * @param[in] 	inNoImages		Number of images
         * @param[in] 	inWidth			Width of image
         * @param[in] 	inHeight		Height of image
         * @param[out]	outHistogram
         *
         * @return No return
         */
        void BuildCRF(QT3DSVec3 **inImages, int inNoImages, int inWidth, int inHeight,
                      float *inExposures)
        {
            QT3DSVec3 histogram[N];
            QT3DSVec3 newCRF[N];

            HDR<N> hdr(this);

            memset(histogram, 0, sizeof(QT3DSVec3) * N);

            for (int i = 0; i < inNoImages; ++i) {
                Histogram<N>::Build(inImages[i], inWidth, inHeight, histogram);
            }

            for (int i = 0; i < N; ++i) {
                histogram[i].x = histogram[i].x > 0 ? 1 / histogram[i].x : 0;
                histogram[i].y = histogram[i].y > 0 ? 1 / histogram[i].y : 0;
                histogram[i].z = histogram[i].z > 0 ? 1 / histogram[i].z : 0;
            }

            QT3DSVec3 *radiance = new QT3DSVec3[inWidth * inHeight * sizeof(QT3DSVec3)];

            // iteration 0, linearize CRF
            for (int i = 0; i < N; ++i) {
                CRF[i] = QT3DSVec3((float)i) * 2.0f / N;
            }

            for (int iteration = 0; iteration < maxIterations; ++iteration) {
                hdr.Build(inImages, inNoImages, inWidth, inHeight, inExposures, radiance);

                memset(newCRF, 0, sizeof(QT3DSVec3) * N);

                for (int i = 0; i < inNoImages; ++i) {
                    for (int x = 0; x < inWidth; ++x) {
                        for (int y = 0; y < inHeight; ++y) {
                            long offset = x + y * inWidth;
                            QT3DSVec3 pixel = inImages[i][offset];
                            newCRF[(int)pixel.x].x += (inExposures[i] * radiance[offset].x);
                            newCRF[(int)pixel.y].y += (inExposures[i] * radiance[offset].y);
                            newCRF[(int)pixel.z].z += (inExposures[i] * radiance[offset].z);
                        }
                    }
                }

                float difference = 0;
                for (int i = 0; i < N; ++i) {
                    newCRF[i] = newCRF[i].multiply(histogram[i]);
                }

                QT3DSVec3 middle = newCRF[N / 2];
                for (int i = 0; i < N; ++i) {
                    newCRF[i].x = newCRF[i].x / middle.x;
                    newCRF[i].y = newCRF[i].y / middle.y;
                    newCRF[i].z = newCRF[i].z / middle.z;
                    difference += (CRF[i] - newCRF[i]).magnitude();
                }
                for (int i = 0; i < N; ++i) {
                    CRF[i] = newCRF[i];
                }
                if (difference < threshold) {
                    break;
                }
            }
            delete[] radiance;
        }

        float weights[N];
        QT3DSVec3 CRF[N];

    private:
        void GenerateRobertsonWeighting()
        {
            // Dynamic Range Improvement Through Multiple Exposures (5)
            // gaussian random weighting
            float divisor = (N - 1) * (N - 1) / 4.0;
            for (int i = 0; i < N; ++i) {
                float dividend = (i - (N - 1) / 2.0f);
                dividend *= dividend;
                weights[i] = exp(-4.0f * dividend / divisor);
            }
        }

        int maxIterations;
        float threshold;
    };
}
}
#endif
