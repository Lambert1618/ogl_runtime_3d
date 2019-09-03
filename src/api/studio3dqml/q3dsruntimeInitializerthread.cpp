/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "q3dsruntimeInitializerthread_p.h"
#include "Qt3DSViewerApp.h"

#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

Q3DSRuntimeInitializerThread::Q3DSRuntimeInitializerThread(
        Q3DSViewer::Q3DSViewerApp *runtime,
        int width, int height, const QSurfaceFormat &format, int offscreenID, const QString &source,
        const QStringList &variantList, bool delayedLoading, qt3ds::Qt3DSAssetVisitor *assetVisitor,
        QOpenGLContext *context, QSurface *surface, const QByteArray &shaderCache)
    : m_runtime(runtime)
    , m_width(width)
    , m_height(height)
    , m_format(format)
    , m_offscreenId(offscreenID)
    , m_source(source)
    , m_variantList(variantList)
    , m_delayedLoading(delayedLoading)
    , m_assetVisitor(assetVisitor)
    , m_context(context)
    , m_surface(surface)
    , m_shaderCache(shaderCache)
{

}

void Q3DSRuntimeInitializerThread::run()
{
    m_context->makeCurrent(m_surface);
    m_success = m_runtime->InitializeApp(m_width, m_height, m_format, m_offscreenId,
                                         m_source, m_variantList, m_delayedLoading, false,
                                         m_shaderCache, m_assetVisitor);
    m_context->doneCurrent();
    delete m_context;

    Q_EMIT initDone();

    // Enter event loop to ensure thread is alive for long enough for initDone to be delivered
    exec();
}

QT_END_NAMESPACE
