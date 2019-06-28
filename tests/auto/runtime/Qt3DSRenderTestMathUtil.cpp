/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "Qt3DSRenderTestMathUtil.h"
#include <math.h>

using namespace qt3ds;
using namespace qt3ds::render;

#define QT3DS_RENDER_TEST_PI 3.14159265358979323846f

inline float degreeToRad(float degree)
{
    return (degree * QT3DS_RENDER_TEST_PI) / 180.0f;
}

void qt3ds::render::NvRenderTestMatrixFrustum(QT3DSMat44 *m, float l, float r, float b, float t, float n,
                                           float f)
{
    float rightMinusLeftInv, topMinusBottomInv, farMinusNearInv, twoNear;

    rightMinusLeftInv = 1.0f / (r - l);
    topMinusBottomInv = 1.0f / (t - b);
    farMinusNearInv = 1.0f / (f - n);
    twoNear = 2.0f * n;

    m->column0 = QT3DSVec4(twoNear * rightMinusLeftInv, 0, 0, 0);
    m->column1 = QT3DSVec4(0, twoNear * topMinusBottomInv, 0, 0);
    m->column2 = QT3DSVec4((r + l) * rightMinusLeftInv, (t + b) * topMinusBottomInv,
                        -(f + n) * farMinusNearInv, -1.0f);
    m->column3 = QT3DSVec4(0, 0, -(twoNear * f) * farMinusNearInv, 0.0f);
}

void qt3ds::render::NvGl2DemoMatrixOrtho(QT3DSMat44 *m, float l, float r, float b, float t, float n,
                                      float f)
{
    float rightMinusLeftInv, topMinusBottomInv, farMinusNearInv;

    rightMinusLeftInv = 1.0f / (r - l);
    topMinusBottomInv = 1.0f / (t - b);
    farMinusNearInv = 1.0f / (f - n);

    m->column0 = QT3DSVec4(2.0f * rightMinusLeftInv, 0, 0, 0);
    m->column1 = QT3DSVec4(0, 2.0f * topMinusBottomInv, 0, 0);
    m->column2 = QT3DSVec4(0, 0, -2.0f * farMinusNearInv, 0);
    m->column3 = QT3DSVec4(-(r + l) * rightMinusLeftInv, -(t + b) * topMinusBottomInv,
                        -(f + n) * farMinusNearInv, 1.0f);
}

void qt3ds::render::NvRenderTestMatrixRotX(QT3DSMat44 *m, float angle)
{
    float rad = degreeToRad(angle);
    float cosPhi = cos(rad);
    float sinPhi = sin(rad);

    m->column0 = QT3DSVec4(1.0, 0.0, 0.0, 0.0);
    m->column1 = QT3DSVec4(0.0, cosPhi, -sinPhi, 0.0);
    m->column2 = QT3DSVec4(0.0, sinPhi, cosPhi, 0.0);
    m->column3 = QT3DSVec4(0.0, 0.0, 0.0, 1.0);
}

void qt3ds::render::NvRenderTestMatrixRotY(QT3DSMat44 *m, float angle)
{
    float rad = degreeToRad(angle);
    float cosPhi = cos(rad);
    float sinPhi = sin(rad);

    m->column0 = QT3DSVec4(cosPhi, 0.0, sinPhi, 0.0);
    m->column1 = QT3DSVec4(0.0, 1.0, 0.0, 0.0);
    m->column2 = QT3DSVec4(-sinPhi, 0.0, cosPhi, 0.0);
    m->column3 = QT3DSVec4(0.0, 0.0, 0.0, 1.0);
}

void qt3ds::render::NvRenderTestMatrixRotZ(QT3DSMat44 *m, float angle)
{
    float rad = degreeToRad(angle);
    float cosPhi = cos(rad);
    float sinPhi = sin(rad);

    m->column0 = QT3DSVec4(cosPhi, -sinPhi, 0.0, 0.0);
    m->column1 = QT3DSVec4(sinPhi, cosPhi, 0.0, 0.0);
    m->column2 = QT3DSVec4(0.0, 0.0, 1.0, 0.0);
    m->column3 = QT3DSVec4(0.0, 0.0, 0.0, 1.0);
}

void qt3ds::render::NvRenderTestMatrixTranslation(QT3DSMat44 *m, float x, float y, float z)
{
    m->column0 = QT3DSVec4(1.0, 0.0, 0.0, 0.0);
    m->column1 = QT3DSVec4(0.0, 1.0, 0.0, 0.0);
    m->column2 = QT3DSVec4(0.0, 0.0, 1.0, 0.0);
    m->column3 = QT3DSVec4(x, y, z, 1.0);
}
