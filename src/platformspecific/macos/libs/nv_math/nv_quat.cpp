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

#include "nv_quat.h"
#include <stdlib.h>
#include <memory.h>

void NvQuatCopy(GLfloat r[4], const GLfloat q[4])
{
    memcpy(r, q, 4 * sizeof(GLfloat));
}

void NvQuatConvertTo3x3Mat(GLfloat r[3][3], const GLfloat q[4])
{
    // Assumes that the quaternion is normalized!
    GLfloat x2 = q[0] * 2.0f;
    GLfloat y2 = q[1] * 2.0f;
    GLfloat z2 = q[2] * 2.0f;
    GLfloat xSq2 = x2 * q[0];
    GLfloat ySq2 = y2 * q[1];
    GLfloat zSq2 = z2 * q[2];
    GLfloat xy2 = x2 * q[1];
    GLfloat xz2 = x2 * q[2];
    GLfloat xw2 = x2 * q[3];
    GLfloat yz2 = y2 * q[2];
    GLfloat yw2 = y2 * q[3];
    GLfloat zw2 = z2 * q[3];

    /* Matrix is
     * |  1 - 2y^2 - 2z^2   2xy - 2zw         2xz + 2yw        |
     * |  2xy + 2zw         1 - 2x^2 - 2z^2   2yz - 2xw        |
     * |  2xz - 2yw         2yz + 2xw         1 - 2x^2 - 2y^2  |
     */
    r[0][0] = 1.0f - ySq2 - zSq2;
    r[0][1] = xy2 - zw2;
    r[0][2] = xz2 + yw2;

    r[1][0] = xy2 + zw2;
    r[1][1] = 1.0f - xSq2 - zSq2;
    r[1][2] = yz2 - xw2;

    r[2][0] = xz2 - yw2;
    r[2][1] = yz2 + xw2;
    r[2][2] = 1.0f - xSq2 - ySq2;
}

void NvQuatIdentity(GLfloat r[4])
{
    r[0] = 0.0f;
    r[1] = 0.0f;
    r[2] = 0.0f;
    r[3] = 1.0f;
}

void NvQuatFromAngleAxis(GLfloat r[4], GLfloat radians, const GLfloat axis[3])
{
    GLfloat cosa = cosf(radians * 0.5f);
    GLfloat sina = sinf(radians * 0.5f);

    r[0] = axis[0] * sina;
    r[1] = axis[1] * sina;
    r[2] = axis[2] * sina;
    r[3] = cosa;
}

void NvQuatX(GLfloat r[4], GLfloat radians)
{
    GLfloat cosa = cosf(radians * 0.5f);
    GLfloat sina = sinf(radians * 0.5f);

    r[0] = sina;
    r[1] = 0.0f;
    r[2] = 0.0f;
    r[3] = cosa;
}

void NvQuatY(GLfloat r[4], GLfloat radians)
{
    GLfloat cosa = cosf(radians * 0.5f);
    GLfloat sina = sinf(radians * 0.5f);

    r[0] = 0.0f;
    r[1] = sina;
    r[2] = 0.0f;
    r[3] = cosa;
}

void NvQuatZ(GLfloat r[4], GLfloat radians)
{
    GLfloat cosa = cosf(radians * 0.5f);
    GLfloat sina = sinf(radians * 0.5f);

    r[0] = 0.0f;
    r[1] = 0.0f;
    r[2] = sina;
    r[3] = cosa;
}

void NvQuatFromEuler(GLfloat r[4], GLfloat heading, GLfloat pitch, GLfloat roll)
{
    GLfloat h[4], p[4], ro[4];

    NvQuatY(h, heading);
    NvQuatX(p, pitch);
    NvQuatZ(ro, roll);

    NvQuatMult(r, h, p);
    NvQuatMult(r, ro, r);
}

void NvQuatFromEulerReverse(GLfloat r[4], GLfloat heading, GLfloat pitch, GLfloat roll)
{
    GLfloat h[4], p[4], ro[4];

    NvQuatZ(ro, roll);
    NvQuatX(p, pitch);
    NvQuatY(h, heading);

    NvQuatMult(r, p, h);
    NvQuatMult(r, r, ro);
}

GLfloat NvQuatDot(const GLfloat q1[4], const GLfloat q2[4])
{
    return q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];
}

void NvQuatMult(GLfloat r[4], const GLfloat q1[4], const GLfloat q2[4])
{
    const GLfloat q1x = q1[0];
    const GLfloat q1y = q1[1];
    const GLfloat q1z = q1[2];
    const GLfloat q1w = q1[3];
    const GLfloat q2x = q2[0];
    const GLfloat q2y = q2[1];
    const GLfloat q2z = q2[2];
    const GLfloat q2w = q2[3];

    r[0] = q1y * q2z - q2y * q1z + q1w * q2x + q2w * q1x;
    r[1] = q1z * q2x - q2z * q1x + q1w * q2y + q2w * q1y;
    r[2] = q1x * q2y - q2x * q1y + q1w * q2z + q2w * q1z;
    r[3] = q1w * q2w - q1x * q2x - q1y * q2y - q1z * q2z;
}

void NvQuatNLerp(GLfloat r[4], const GLfloat q1[4], const GLfloat q2[4], GLfloat t)
{
    GLfloat omt = 1.0f - t;

    if (NvQuatDot(q1, q2) < 0.0f) {
        r[0] = -q1[0] * omt + q2[0] * t;
        r[1] = -q1[1] * omt + q2[1] * t;
        r[2] = -q1[2] * omt + q2[2] * t;
        r[3] = -q1[3] * omt + q2[3] * t;
    } else {
        r[0] = q1[0] * omt + q2[0] * t;
        r[1] = q1[1] * omt + q2[1] * t;
        r[2] = q1[2] * omt + q2[2] * t;
        r[3] = q1[3] * omt + q2[3] * t;
    }

    NvQuatNormalize(r, r);
}

void NvQuatNormalize(GLfloat r[4], const GLfloat q[4])
{
    GLfloat invLength = 1.0f / sqrtf(NvQuatDot(q, q));

    r[0] = invLength * q[0];
    r[1] = invLength * q[1];
    r[2] = invLength * q[2];
    r[3] = invLength * q[3];
}
