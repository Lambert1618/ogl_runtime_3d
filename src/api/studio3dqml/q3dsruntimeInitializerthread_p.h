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

#ifndef Q3DS_RUNTIME_INITIALIZER_THEAD_H
#define Q3DS_RUNTIME_INITIALIZER_THEAD_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtStudio3D API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "Qt3DSViewerApp.h"
#include "q3dsstudio3d_p.h"

#include <QtCore/qthread.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qsurface.h>
#include <QtQuick/qquickframebufferobject.h>

QT_BEGIN_NAMESPACE

class Q3DSRenderer;

class Q3DSRuntimeInitializerThread : public QThread
{
    Q_OBJECT
public:
    Q3DSRuntimeInitializerThread(Q3DSViewer::Q3DSViewerApp *runtime,
                                 int width, int height, const QSurfaceFormat &format,
                                 int offscreenID, const QString &source,
                                 const QStringList &variantList, bool delayedLoading,
                                 qt3ds::Qt3DSAssetVisitor *assetVisitor, QOpenGLContext *context,
                                 QSurface *surface, const QByteArray &shaderCache);

    void run() override;

    bool wasSuccess() const { return m_success; }

Q_SIGNALS:
    void initDone();

private:
    Q3DSViewer::Q3DSViewerApp *m_runtime = nullptr;
    Q3DSRenderer *m_renderer = nullptr;
    int m_width = 0;
    int m_height = 0;
    QSurfaceFormat m_format;
    int m_offscreenId = 0;
    QString m_source;
    QStringList m_variantList;
    bool m_delayedLoading = false;
    qt3ds::Qt3DSAssetVisitor *m_assetVisitor;
    QOpenGLContext *m_context;
    QSurface *m_surface;
    QByteArray m_shaderCache;
    bool m_success = false;
};

QT_END_NAMESPACE

#endif // Q3DS_RUNTIME_INITIALIZER_THEAD_H
