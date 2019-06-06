/****************************************************************************
**
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

#ifndef Q3DSSURFACEVIEWER_P_H
#define Q3DSSURFACEVIEWER_P_H

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

#include "q3dssurfaceviewer.h"
#include "Qt3DSViewerApp.h"

#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

class QSurface;
class QOpenGLContext;
class Q3DSViewerSettings;
class Q3DSPresentation;
class QQmlEngine;

class Q_STUDIO3D_EXPORT Q3DSSurfaceViewerPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Q3DSSurfaceViewer)
public:
    explicit Q3DSSurfaceViewerPrivate(Q3DSSurfaceViewer *parent = nullptr);
    ~Q3DSSurfaceViewerPrivate();

    void setSize(const QSize &size);
    void setUpdateInterval(int interval);
    bool initialize(QSurface *surface, QOpenGLContext *context, GLuint fboId, bool idValid);
    void update();

    QImage grab(const QRect &rect);

private Q_SLOTS:
    void destroy();

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    void reset();
    void setError(const QString &error);
    bool initializeRuntime();
    void releaseRuntime();
    void resetUpdateTimer();
    QObject *surfaceObject();

    Q3DSSurfaceViewer *q_ptr;

    Q3DSViewer::Q3DSViewerApp *m_viewerApp;
    QSize m_size;
    QTimer *m_timer;
    int m_updateInterval;
    qreal m_pixelRatio;
    GLuint m_fboId;
    QSurface *m_surface; // Not owned
    QOpenGLContext *m_context; // Not owned
    bool m_autoSize;
    Q3DSViewerSettings *m_settings;
    Q3DSPresentation *m_presentation;
    QString m_id;
    QElapsedTimer m_startupTimer;
    QQmlEngine *qmlEngine = nullptr;
    QString m_error;
};

QT_END_NAMESPACE

#endif // Q3DSSURFACEVIEWER_P_H
