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

#ifndef INCLUDED_MATRIX_H
#define INCLUDED_MATRIX_H

#include <GLES2/gl2.h>
#include <math.h>
#include <assert.h>

#define KD_FLT_EPSILON 1.19209290E-07F
#define KD_DEG_TO_RAD_F 0.0174532924F
#define KD_RAD_TO_DEG_F 57.2957802F

void NvCopyMatf(GLfloat r[4][4], const GLfloat m[4][4]);
void NvExtract3x3Matf(GLfloat r[3][3], const GLfloat m[4][4]);

/* vector utilities */

GLfloat NvVecLengthf(const GLfloat v[3]);

void NvNormalizeVecf(GLfloat r[3], const GLfloat v[3]);

void NvAddVecf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3]);

void NvSubVecf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3]);

void NvCrossProductf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3]);

void NvTransformPointf(GLfloat r[3], const GLfloat m[4][4], const GLfloat a[3]);
void NvTransformHomPointf(GLfloat r[4], const GLfloat m[4][4], const GLfloat a[4]);
void NvTransformVecf(GLfloat r[3], const GLfloat m[4][4], const GLfloat a[3]);

/* matrix utilities */

void NvMultMatf(GLfloat r[4][4], const GLfloat a[4][4], const GLfloat b[4][4]);

void NvInvMatf(GLfloat r[4][4], const GLfloat m[4][4]);

/* matrix building utilities */

void NvBuildIdentityMatf(GLfloat r[4][4]);

void NvBuildTranslateMatf(GLfloat r[4][4], GLfloat x, GLfloat y, GLfloat z);

void NvBuildScaleMatf(GLfloat r[4][4], GLfloat x, GLfloat y, GLfloat z);

void NvBuildRotXDegMatf(GLfloat r[4][4], GLfloat degrees);
void NvBuildRotYDegMatf(GLfloat r[4][4], GLfloat degrees);
void NvBuildRotZDegMatf(GLfloat r[4][4], GLfloat degrees);

void NvBuildRotDegMatf(GLfloat r[4][4], const GLfloat axis[3], GLfloat degrees);

void NvBuildRotXRadMatf(GLfloat r[4][4], GLfloat radians);
void NvBuildRotYRadMatf(GLfloat r[4][4], GLfloat radians);
void NvBuildRotZRadMatf(GLfloat r[4][4], GLfloat radians);

void NvBuildRotRadMatf(GLfloat r[4][4], const GLfloat axis[3], GLfloat radians);

void NvBuildLookatMatf(GLfloat r[4][4], const GLfloat eye[3], const GLfloat obj[3],
                       const GLfloat up[3]);

void NvBuildFrustumMatf(GLfloat r[4][4], GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
                        GLfloat znear, GLfloat zfar);

void NvBuildOrtho2Matf(GLfloat r[4][4], GLfloat left, GLfloat right, GLfloat bottom, GLfloat top);

void NvBuildOrthoMatf(GLfloat r[4][4], GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
                      GLfloat znear, GLfloat zfar);

/* matrix concatenation utilities */

void NvMultTranslateMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat x, GLfloat y, GLfloat z);
void NvMultScaleMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat x, GLfloat y, GLfloat z);

void NvMultRotXDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees);
void NvMultRotYDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees);
void NvMultRotZDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees);

void NvMultRotDegMatf(GLfloat r[4][4], const GLfloat m[4][4], const GLfloat axis[3],
                      GLfloat degrees);

void NvMultRotXRadMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat radians);
void NvMultRotYRadMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat radians);
void NvMultRotZRadMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat radians);

void NvMultRotRadMatf(GLfloat r[4][4], const GLfloat m[4][4], const GLfloat axis[3],
                      GLfloat radians);

#endif
