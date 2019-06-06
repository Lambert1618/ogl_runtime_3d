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
#ifndef QT3DS_EGL_WINDOW_SYSTEM_H
#define QT3DS_EGL_WINDOW_SYSTEM_H
#include "Qt3DSEGLInfo.h"
#include "Qt3DSWindowSystem.h"
#include "nv_main/nv_main.h"

namespace Q3DStudio {
struct SEGLWindowSystemImpl : public IWindowSystem
{
    SEGLInfo m_EGLInfo;

    virtual QSize GetWindowDimensions()
    {
        int w, h;

        eglQuerySurface(gEGLHandles[0].display, gEGLHandles[0].surface, EGL_WIDTH, &w);
        eglQuerySurface(gEGLHandles[0].display, gEGLHandles[0].surface, EGL_HEIGHT, &h);

        return QSize(w, h);
    }
    virtual void SetWindowDimensions(const QSize &) {}
    // For platforms that support it, we get the egl info for render plugins
    // Feel free to return NULL.
    virtual SEGLInfo *GetEGLInfo()
    {
        m_EGLInfo.display = gEGLHandles[0].display;
        m_EGLInfo.surface = gEGLHandles[0].surface;
        m_EGLInfo.context = gEGLHandles[0].context;
        m_EGLInfo.config = gEGLHandles[0].config;
        m_EGLInfo.nativewin = gEGLHandles[0].nativewin;
        return &m_EGLInfo;
    }
    // on some systems we allow our default render target to be a offscreen buffer
    // otherwise return 0;
    virtual int GetDefaultRenderTargetID() { return 0; }
    // returns the depth buffer bit count for the render window
    // overwrite this one for your own needs
    virtual int GetDepthBitCount()
    {
        EGLint depth = 16;
#ifdef _PLATFORM_USE_EGL
        if (gEGLHandles[0].display && gEGLHandles[0].config)
            eglGetConfigAttrib(gEGLHandles[0].display, gEGLHandles[0].config, EGL_DEPTH_SIZE,
                               &depth);
#endif

        return depth;
    }
};
};

#endif
