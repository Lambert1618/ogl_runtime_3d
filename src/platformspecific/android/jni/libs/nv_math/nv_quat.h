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

#ifndef INCLUDED_QUAT_H
#define INCLUDED_QUAT_H

#include <GLES2/gl2.h>
#include <math.h>
#include <assert.h>

void NvQuatCopy(GLfloat r[4], const GLfloat q[4]);
void NvQuatConvertTo3x3Mat(GLfloat r[3][3], const GLfloat q[4]);

void NvQuatIdentity(GLfloat r[4]);

void NvQuatFromAngleAxis(GLfloat r[4], GLfloat radians, const GLfloat axis[3]);

void NvQuatX(GLfloat r[4], GLfloat radians);

void NvQuatY(GLfloat r[4], GLfloat radians);

void NvQuatZ(GLfloat r[4], GLfloat radians);

void NvQuatFromEuler(GLfloat r[4], GLfloat heading, GLfloat pitch, GLfloat roll);

void NvQuatFromEulerReverse(GLfloat r[4], GLfloat heading, GLfloat pitch, GLfloat roll);

GLfloat NvQuatDot(const GLfloat q1[4], const GLfloat q2[4]);

void NvQuatMult(GLfloat r[4], const GLfloat q1[4], const GLfloat q2[4]);

void NvQuatNLerp(GLfloat r[4], const GLfloat q1[4], const GLfloat q2[4], GLfloat t);

void NvQuatNormalize(GLfloat r[4], const GLfloat q[4]);

#endif
