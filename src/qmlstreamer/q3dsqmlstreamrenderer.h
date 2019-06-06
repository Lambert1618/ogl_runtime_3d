/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef Q3DS_QMLSTREAM_RENDERER_H
#define Q3DS_QMLSTREAM_RENDERER_H

#include "q3dsqmlstreamservice.h"

#include <QOpenGLFunctions>
#include <QSize>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

QT_BEGIN_NAMESPACE
class QQuickRenderControl;
class QQuickWindow;
class QQuickItem;
class QOpenGLContext;
class QOffscreenSurface;
class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
QT_END_NAMESPACE

class Q3DSQmlStreamRenderer;

class Q3DSQmlStreamEventHandler : public QObject
{
    Q_OBJECT
public:
    Q3DSQmlStreamEventHandler(Q3DSQmlStreamRenderer *renderer);
    bool event(QEvent *e) Q_DECL_OVERRIDE;

private:
    Q3DSQmlStreamRenderer *m_renderer;
};

class Q3DSQmlStreamRenderer : public QObject,
                               public IQ3DSQmlStreamRenderer
{
    Q_OBJECT
public:
    Q3DSQmlStreamRenderer();
    virtual ~Q3DSQmlStreamRenderer();

    bool initialize(QOpenGLContext *context, QSurface *surface) override;

    QSize getDesiredSize() override;
    E_TEXTURE_FORMAT getDesiredFormat() override;
    bool isUpdateRequested() override;

    void render() override;

    void uninitialize() override;

    void setItem(QQuickItem *item);

    bool event(QEvent *event) Q_DECL_OVERRIDE;

private:

    void updateSizes();
    void requestUpdate();
    void cleanup();
    void initializeRender();
    void renderTexture();
    void initializeFboCopy();

    QSize m_size;
    QMutex m_mutex, m_renderMutex;
    QWaitCondition m_waitCondition;

    QQuickRenderControl *m_renderControl;
    QQuickWindow *m_quickWindow;
    QQuickItem *m_rootItem;

    QOpenGLFramebufferObject *m_frameBuffer;
    QOpenGLShaderProgram *m_program;
    QOpenGLVertexArrayObject *m_vao;
    QOpenGLBuffer *m_vertices;
    QOpenGLContext *m_context;
    QOffscreenSurface *m_offscreenSurface;

    Q3DSQmlStreamEventHandler *m_renderObject;
    QThread *m_renderThread;

    bool m_requestUpdate;
    bool m_initialized;
    bool m_prepared;
    bool m_update;
    bool m_delayedUpdateRequest;

    friend class Q3DSQmlStreamEventHandler;
};

#endif // Q3DS_QMLSTREAM_RENDERER_H
