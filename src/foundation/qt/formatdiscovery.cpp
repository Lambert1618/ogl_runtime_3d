/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include <QtGui/qopenglcontext.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qopengltexture.h>

#include "qstudio3dglobal.h"
#include "Qt3DSFoundation.h"

QT_BEGIN_NAMESPACE

namespace Q3DS {

static QSurfaceFormat findIdealGLVersion()
{
    QSurfaceFormat fmt;
    fmt.setProfile(QSurfaceFormat::CoreProfile);

    // Advanced: Try 4.3 core (so we get compute shaders for instance)
    fmt.setVersion(4, 3);
    QOpenGLContext ctx;
    ctx.setFormat(fmt);
    if (ctx.create() && ctx.format().version() >= qMakePair(4, 3)) {
        qCInfo(qt3ds::TRACE_INFO) << "Requesting OpenGL 4.3 core context succeeded";
        return ctx.format();
    }

    // Basic: Stick with 3.3 for now to keep less fortunate, Mesa-based systems happy
    fmt.setVersion(3, 3);
    ctx.setFormat(fmt);
    if (ctx.create() && ctx.format().version() >= qMakePair(3, 3)) {
        qCInfo(qt3ds::TRACE_INFO) << "Requesting OpenGL 3.3 core context succeeded";
        return ctx.format();
    }

    qWarning("Failed to get OpenGL 3.3 or OpenGL 4.3 context!");
    return fmt;
}

static QSurfaceFormat findIdealGLESVersion()
{
    QSurfaceFormat fmt;

    // Advanced: Try 3.1 (so we get compute shaders for instance)
    fmt.setVersion(3, 1);
    QOpenGLContext ctx;
    ctx.setFormat(fmt);

    // Now, it's important to check the format with the actual version (parsed
    // back from GL_VERSION) since some implementations, ANGLE for instance,
    // are broken and succeed the 3.1 context request even though they only
    // support and return a 3.0 context. This is against the spec since 3.0 is
    // obviously not backwards compatible with 3.1, but hey...
    if (ctx.create() && ctx.format().version() >= qMakePair(3, 1)) {
        qCInfo(qt3ds::TRACE_INFO) << "Requesting OpenGL ES 3.1 context succeeded";
        return ctx.format();
    }

    // Basic: OpenGL ES 3.0 is a hard requirement at the moment since we can
    // only generate 300 ES shaders, uniform buffers are mandatory.
    fmt.setVersion(3, 0);
    ctx.setFormat(fmt);
    if (ctx.create() && ctx.format().version() >= qMakePair(3, 0)) {
        qCInfo(qt3ds::TRACE_INFO) << "Requesting OpenGL ES 3.0 context succeeded";
        return ctx.format();
    }

    fmt.setVersion(2, 0);
    ctx.setFormat(fmt);
    if (ctx.create()) {
        qCInfo(qt3ds::TRACE_INFO) << "Requesting OpenGL ES 2.0 context succeeded";
        return fmt;
    }

    qWarning("Failed to get OpenGL ES 2.0, 3.0 or 3.1 context");
    return fmt;
}

static QSurfaceFormat s_f;

QSurfaceFormat surfaceFormat()
{
    if (s_f.renderableType() == QSurfaceFormat::DefaultRenderableType) {
        // works in dynamic gl builds too because there's a qguiapp already
        // this requirement is also a problem, see QT3DS-3603
        if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
            s_f = findIdealGLVersion();
        else
            s_f = findIdealGLESVersion();
        s_f.setDepthBufferSize(24);
        s_f.setStencilBufferSize(8);
        // Ignore MSAA here as that is a per-layer setting.
    }
    return s_f;
}
} // End namespace Q3DS

QT_END_NAMESPACE
