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

#ifndef QT3DS_WINDOWS_TRIG_CONSTANTS_H
#define QT3DS_WINDOWS_TRIG_CONSTANTS_H

#define QT3DS_GLOBALCONST extern const __declspec(selectany)

__declspec(align(16)) struct QT3DS_VECTORF32
{
    float f[4];
};

#define QT3DS_PI 3.141592654f
#define QT3DS_2PI 6.283185307f
#define QT3DS_1DIVPI 0.318309886f
#define QT3DS_1DIV2PI 0.159154943f
#define QT3DS_PIDIV2 1.570796327f
#define QT3DS_PIDIV4 0.785398163f

QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXSinCoefficients0 = { 1.0f, -0.166666667f, 8.333333333e-3f,
                                                     -1.984126984e-4f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXSinCoefficients1 = { 2.755731922e-6f, -2.505210839e-8f,
                                                     1.605904384e-10f, -7.647163732e-13f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXSinCoefficients2 = { 2.811457254e-15f, -8.220635247e-18f,
                                                     1.957294106e-20f, -3.868170171e-23f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXCosCoefficients0 = { 1.0f, -0.5f, 4.166666667e-2f,
                                                     -1.388888889e-3f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXCosCoefficients1 = { 2.480158730e-5f, -2.755731922e-7f,
                                                     2.087675699e-9f, -1.147074560e-11f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXCosCoefficients2 = { 4.779477332e-14f, -1.561920697e-16f,
                                                     4.110317623e-19f, -8.896791392e-22f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXTanCoefficients0 = { 1.0f, 0.333333333f, 0.133333333f,
                                                     5.396825397e-2f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXTanCoefficients1 = { 2.186948854e-2f, 8.863235530e-3f,
                                                     3.592128167e-3f, 1.455834485e-3f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXTanCoefficients2 = { 5.900274264e-4f, 2.391290764e-4f,
                                                     9.691537707e-5f, 3.927832950e-5f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXASinCoefficients0 = { -0.05806367563904f, -0.41861972469416f,
                                                      0.22480114791621f, 2.17337241360606f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXASinCoefficients1 = { 0.61657275907170f, 4.29696498283455f,
                                                      -1.18942822255452f, -6.53784832094831f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXASinCoefficients2 = { -1.36926553863413f, -4.48179294237210f,
                                                      1.41810672941833f, 5.48179257935713f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXATanCoefficients0 = { 1.0f, 0.333333334f, 0.2f, 0.142857143f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXATanCoefficients1 = { 1.111111111e-1f, 9.090909091e-2f,
                                                      7.692307692e-2f, 6.666666667e-2f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXATanCoefficients2 = { 5.882352941e-2f, 5.263157895e-2f,
                                                      4.761904762e-2f, 4.347826087e-2f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXSinEstCoefficients = { 1.0f, -1.66521856991541e-1f,
                                                       8.199913018755e-3f, -1.61475937228e-4f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXCosEstCoefficients = { 1.0f, -4.95348008918096e-1f,
                                                       3.878259962881e-2f, -9.24587976263e-4f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXTanEstCoefficients = { 2.484f, -1.954923183e-1f, 2.467401101f,
                                                       QT3DS_1DIVPI };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXATanEstCoefficients = { 7.689891418951e-1f, 1.104742493348f,
                                                        8.661844266006e-1f, QT3DS_PIDIV2 };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXASinEstCoefficients = { -1.36178272886711f, 2.37949493464538f,
                                                        -8.08228565650486e-1f,
                                                        2.78440142746736e-1f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXASinEstConstants = { 1.00000011921f, QT3DS_PIDIV2, 0.0f, 0.0f };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXPiConstants0 = { QT3DS_PI, QT3DS_2PI, QT3DS_1DIVPI, QT3DS_1DIV2PI };
QT3DS_GLOBALCONST QT3DS_VECTORF32 g_PXReciprocalTwoPi = { QT3DS_1DIV2PI, QT3DS_1DIV2PI, QT3DS_1DIV2PI,
                                                    QT3DS_1DIV2PI };

#endif
