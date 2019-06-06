/****************************************************************************
**
** Copyright (C) 2007-2008 NVIDIA Corporation.
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
// this is set from higher level.  think of it as the upper model matrix
uniform mat4 pixelToClipMat;

uniform vec4 col_uni;

attribute vec2 pos_attr;
attribute vec2 tex_attr;
attribute vec4 col_attr;

varying vec4 col_var;
varying vec2 tex_var;

void main()
{
	// account for translation and rotation of the primitive into [-1,1] spatial default.
    gl_Position = pixelToClipMat * vec4(pos_attr.x, pos_attr.y, 0, 1);

    col_var = col_attr * col_uni;
    tex_var = tex_attr;
}
