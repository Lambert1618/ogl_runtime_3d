/****************************************************************************
**
** Copyright (C) 2009-2011 NVIDIA Corporation.
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

#ifndef INCLUDED_QT3DS_MATH_CPP_MISC_H
#define INCLUDED_QT3DS_MATH_CPP_MISC_H

inline bool nvIsPowerOfTwo(unsigned int i)
{
    return (i & (i - 1)) == 0;
}

inline float nvDegToRadf(float d)
{
    return d * 3.14159265358979323846f / 180.0f;
}

inline float nvRadToDegf(float r)
{
    return r * 180.0f / 3.14159265358979323846f;
}

/*
  'mod' differs from '%' in that it behaves correctly when either the
  numerator or denominator is negative.
*/

inline int nvMod(int n, int d)
{
    int m = n % d;
    return (((m < 0) && (d > 0)) || ((m > 0) && (d < 0))) ? (m + d) : m;
}

inline int nvAbs(int n)
{
    if (n < 0)
        return -n;
    return n;
}

inline int nvSign(int n)
{
    if (n > 0)
        return 1;
    if (n < 0)
        return -1;
    return 0;
}

/*
   This returns the smallest amplitude value x such that
   nvMod(b + x, m) == a
*/

inline int nvDifMod(int a, int b, int m)
{

    int x1 = a - b;
    int x2 = (x1 > 0) ? x1 - m : x1 + m;
    return (nvAbs(x1) < nvAbs(x2)) ? x1 : x2;
}

inline float nvWrapf(float a, float min, float max)
{

    assert(max > min);

    float d = max - min;
    float s = a - min;
    float q = s / d;
    float m = q - floorf(q);
    return m * d + min;
}

inline float nvClampf(float a, float min, float max)
{
    return (a < min) ? min : ((a > max) ? max : a);
}

inline int nvClampi(int i, int min, int max)
{
    return (i < min) ? min : ((i > max) ? max : i);
}

inline float nvGaussian(float x, float s)
{
    float c = s * sqrtf(2.0f * 3.14159265358979323846f);
    return expf(-(x * x) / (2.0f * s * s)) / c;
}

inline float nvLerpf(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

inline float nvEasef(float t)
{

    float t_2 = t * t;
    float t_3 = t_2 * t;
    return 3.0f * t_2 - 2.0f * t_3;
}

#endif
