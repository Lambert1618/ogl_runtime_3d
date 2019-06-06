/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef QT3DS_WINDOW_SYSTEM_H
#define QT3DS_WINDOW_SYSTEM_H

#include <QtCore/qsize.h>

namespace Q3DStudio {

struct SEGLInfo;

class IWindowSystem
{
protected:
    virtual ~IWindowSystem() {}
public:
    virtual QSize GetWindowDimensions() = 0;
    virtual void SetWindowDimensions(const QSize &inSize) = 0;
    // For platforms that support it, we get the egl info for render plugins
    // Feel free to return NULL.
    virtual SEGLInfo *GetEGLInfo() = 0;
    // on some systems we allow our default render target to be a offscreen buffer
    // otherwise return 0;
    virtual int GetDefaultRenderTargetID() = 0;
    // returns the depth buffer bit count for the render window
    virtual int GetDepthBitCount() = 0;
};
}

#endif
