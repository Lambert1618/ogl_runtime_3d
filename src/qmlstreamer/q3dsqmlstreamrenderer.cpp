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

#include "q3dsqmlstreamrenderer.h"

#include <QCoreApplication>
#include <qdebug.h>
#include <QOpenGLContext>
#include <QSurface>
#include <QPainter>
#include <QOpenGLFunctions>
#include <QWindow>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QThread>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

Q_GLOBAL_STATIC(QThread, renderThread)
Q_GLOBAL_STATIC(QAtomicInt, renderThreadClientCount)

#define RenderEvent(name, value)                            \
    class name : public QEvent                              \
    {                                                       \
    public:                                                 \
        name() : QEvent(_type) {}                           \
        static const QEvent::Type _type;                    \
    };                                                      \
    const QEvent::Type name::_type = QEvent::Type(value);

RenderEvent(InitializeRender, QEvent::User + 1)
RenderEvent(RequestUpdate, QEvent::User + 2)
RenderEvent(InitializeRenderThread, QEvent::User + 3)
RenderEvent(PrepareRender, QEvent::User + 4)
RenderEvent(Cleanup, QEvent::User + 5)

#define EventType(e) (e::_type)

class RenderControl : public QQuickRenderControl
{
public:
    RenderControl(QWindow *w)
        : m_window(w)
    {
    }

    QWindow *renderWindow(QPoint *offset) Q_DECL_OVERRIDE
    {
        if (offset)
            *offset = QPoint(0, 0);
        return m_window;
    }

private:
    QWindow *m_window;
};

Q3DSQmlStreamEventHandler::Q3DSQmlStreamEventHandler(Q3DSQmlStreamRenderer *renderer)
    : m_renderer(renderer)
{

}

bool Q3DSQmlStreamEventHandler::event(QEvent *e)
{
    switch (e->type()) {

    case EventType(RequestUpdate): {
        m_renderer->renderTexture();
        return true;
    }

    case EventType(InitializeRenderThread): {
        m_renderer->initializeRender();
        return true;
    }

    case EventType(Cleanup): {
        m_renderer->cleanup();
        return true;
    }

    default:
        break;
    }
    return QObject::event(e);
}

Q3DSQmlStreamRenderer::Q3DSQmlStreamRenderer()
    : m_renderControl(nullptr)
    , m_quickWindow(nullptr)
    , m_rootItem(nullptr)
    , m_frameBuffer(nullptr)
    , m_program(nullptr)
    , m_vao(nullptr)
    , m_vertices(nullptr)
    , m_context(nullptr)
    , m_offscreenSurface(nullptr)
    , m_renderObject(nullptr)
    , m_renderThread(nullptr)
    , m_requestUpdate(false)
    , m_initialized(false)
    , m_prepared(false)
    , m_update(false)
    , m_delayedUpdateRequest(false)
{
    renderThreadClientCount->fetchAndAddAcquire(1);

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    if (QOpenGLContext::currentContext())
        format = QOpenGLContext::currentContext()->format();
    else {
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
    }

    m_offscreenSurface = new QOffscreenSurface;
    m_offscreenSurface->setFormat(format);
    m_offscreenSurface->create();

    m_renderControl = new RenderControl(nullptr);

    connect(m_renderControl, &QQuickRenderControl::renderRequested,
            this, &Q3DSQmlStreamRenderer::requestUpdate);
    connect(m_renderControl, &QQuickRenderControl::sceneChanged,
            this, &Q3DSQmlStreamRenderer::requestUpdate);

    m_quickWindow = new QQuickWindow(m_renderControl);
    m_quickWindow->setClearBeforeRendering(true);
    m_quickWindow->setColor(Qt::transparent);

    m_renderObject = new Q3DSQmlStreamEventHandler(this);
    renderThread->setObjectName(QStringLiteral("Qt3DSQmlStreamRenderer::renderThread"));
    m_renderThread = renderThread;

    m_renderObject->moveToThread(m_renderThread);
    if (!m_renderThread->isRunning())
        m_renderThread->start();
}

Q3DSQmlStreamRenderer::~Q3DSQmlStreamRenderer()
{
    QMutexLocker lock(&m_mutex);
    QCoreApplication::postEvent(m_renderObject, new Cleanup());
    m_waitCondition.wait(&m_mutex);

    delete m_offscreenSurface;
    delete m_renderControl;
    delete m_quickWindow;
    delete m_renderObject;

    renderThreadClientCount->fetchAndSubAcquire(1);
    if (renderThreadClientCount->load() == 0)
        renderThread->quit();
}

void Q3DSQmlStreamRenderer::cleanup()
{
    if (m_context)
        m_context->makeCurrent(m_offscreenSurface);
    if (m_renderControl)
        m_renderControl->invalidate();

    delete m_frameBuffer;
    delete m_program;
    delete m_vertices;

    if (m_context)
        m_context->doneCurrent();
    delete m_context;

    if (m_renderObject)
        m_renderObject->moveToThread(QCoreApplication::instance()->thread());
    m_waitCondition.wakeOne();
}

bool Q3DSQmlStreamRenderer::initialize(QOpenGLContext *context, QSurface *surface)
{
    Q_UNUSED(surface);
    QMutexLocker lock(&m_renderMutex);
    if (!m_context) {
        m_context = new QOpenGLContext();
        m_context->setShareContext(context);
        m_context->setFormat(context->format());
        m_context->create();
        m_context->moveToThread(m_renderThread);
    }

    if (!m_rootItem) {
        return true;
    }

    if (m_initialized) {
        Q_ASSERT(QOpenGLContext::areSharing(context, m_context));
        return true;
    }

    m_rootItem->setParentItem(m_quickWindow->contentItem());

    updateSizes();

    m_initialized = true;
    // Initialize render thread
    QCoreApplication::postEvent(m_renderObject,
                                new InitializeRenderThread());
    return true;
}
void Q3DSQmlStreamRenderer::initializeFboCopy()
{
    m_program = new QOpenGLShaderProgram;
    if (m_context->format().renderableType() == QSurfaceFormat::OpenGLES) {
        static const char *vsSource =
            "attribute highp vec4 pos;\n"
            "attribute highp vec2 tc;\n"
            "varying lowp vec2 texcoord;\n"
            "void main() {\n"
            "   texcoord = tc;\n"
            "   gl_Position = pos;\n"
            "}\n";
        static const char *fsSource =
            "varying highp vec2 texcoord;\n"
            "uniform sampler2D sampler;\n"
            "void main() {\n"
            "   gl_FragColor = texture2D(sampler, texcoord);\n"
            "}\n";
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsSource);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsSource);
    } else {
        static const char *vsSource =
            "#version 150 core\n"
            "in vec4 pos;\n"
            "in vec2 tc;\n"
            "out vec2 texcoord;\n"
            "void main() {\n"
            "   texcoord = tc;\n"
            "   gl_Position = pos;\n"
            "}\n";
        static const char *fsSource =
            "#version 150 core\n"
            "in vec2 texcoord;\n"
            "out vec4 fragColor;\n"
            "uniform sampler2D sampler;\n"
            "void main() {\n"
            "   fragColor = texture(sampler, texcoord);\n"
            "}\n";
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsSource);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsSource);
    }

    m_program->bindAttributeLocation("pos", 0);
    m_program->bindAttributeLocation("tc", 1);

    if (!m_program->link()) {
        QByteArray logData(m_program->log().toLocal8Bit());
        const char *log = logData.data();
        qFatal("Failed to create shader program: %s", log);
    }

    m_vertices = new QOpenGLBuffer;
    m_vertices->create();
    m_vertices->bind();

    static const float vertices[] =
    {
        -1, -1, 0, 1, 0, 0,
        1, -1, 0, 1, 1, 0,
        1, 1, 0, 1, 1, 1,
        -1, -1, 0, 1, 0, 0,
        1, 1, 0, 1, 1, 1,
        -1, 1, 0, 1, 0, 1,
    };

    m_vertices->allocate(vertices, sizeof(vertices));
    m_vertices->release();
}

void Q3DSQmlStreamRenderer::initializeRender()
{
    QMutexLocker lock(&m_renderMutex);
    m_context->makeCurrent(m_offscreenSurface);
    m_renderControl->initialize(m_context);
    m_context->doneCurrent();
    QCoreApplication::postEvent(this, new PrepareRender());
}

QSize Q3DSQmlStreamRenderer::getDesiredSize()
{
    return m_size;
}

E_TEXTURE_FORMAT Q3DSQmlStreamRenderer::getDesiredFormat()
{
    return E_TEXTURE_RGBA8;
}

bool Q3DSQmlStreamRenderer::isUpdateRequested()
{
    return m_update;
}

// Called by stream renderer client when it stops using the renderer
// TODO: We are sharing the context so stop using it.
void Q3DSQmlStreamRenderer::uninitialize()
{
}

void Q3DSQmlStreamRenderer::setItem(QQuickItem *item)
{
    if (item && m_rootItem && m_initialized) {
        QMutexLocker lock(&m_renderMutex);
        m_rootItem->setParentItem(nullptr);
        m_rootItem = item;
        m_rootItem->setParentItem(m_quickWindow->contentItem());
        updateSizes();
    } else {
        if (item && m_rootItem != item) {
            m_rootItem = item;

            if (m_context)
                initialize(m_context, nullptr);
        }
    }
}

bool Q3DSQmlStreamRenderer::event(QEvent *event)
{
    switch (event->type()) {

    case EventType(PrepareRender): {

        m_renderControl->prepareThread(m_renderThread);
        m_prepared = true;

        if (m_delayedUpdateRequest) {
            QCoreApplication::postEvent(this, new RequestUpdate());
            m_delayedUpdateRequest = false;
        }

        return true;
    }

    case EventType(RequestUpdate): {

        m_renderControl->polishItems();
        QMutexLocker lock(&m_mutex);
        QCoreApplication::postEvent(m_renderObject, new RequestUpdate());
        m_waitCondition.wait(&m_mutex);
        return true;
    }

    default:
        break;
    }

    return QObject::event(event);
}

void Q3DSQmlStreamRenderer::updateSizes()
{
    if (m_rootItem->width() > 0 && m_rootItem->height() > 0) {
        m_size = QSize(m_rootItem->width(), m_rootItem->height());
        // Make the dimensions divisible by four; this prevents rendering artefacts
        // In worst case scenario this increases the size by 3 pixels in each dimension
        // TODO: Would it be possible to add transparent pixels around the original stream
        // to meet the divisible by four -requirement, but render the original amount of pixels?
        // That way the end result would still be pixel perfect, unlike with this implementation.
        const int w = m_size.width();
        const int h = m_size.height();
        if (w % 4 != 0)
            m_size.setWidth(w + 4 - w % 4);
        if (h % 4 != 0)
            m_size.setHeight(h + 4 - h % 4);
    } else {
        m_rootItem->setWidth(256);
        m_rootItem->setHeight(256);
    }

    m_quickWindow->setGeometry(0, 0, m_size.width(), m_size.height());
}

void Q3DSQmlStreamRenderer::requestUpdate()
{
    if (!m_requestUpdate) {
        m_requestUpdate = true;
        if (m_initialized)
            QCoreApplication::postEvent(this, new RequestUpdate());
        else
            m_delayedUpdateRequest = true;
    }
}

void Q3DSQmlStreamRenderer::renderTexture()
{
    QMutexLocker lock(&m_renderMutex);
    m_context->makeCurrent(m_offscreenSurface);

    if (!m_frameBuffer || m_frameBuffer->size() != m_size) {
        if (m_frameBuffer)
            delete m_frameBuffer;

        m_frameBuffer = new QOpenGLFramebufferObject(m_size);
        m_frameBuffer->setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        m_quickWindow->setRenderTarget(m_frameBuffer);
    }

    {
        QMutexLocker lock(&m_mutex);
        if (m_requestUpdate) {
            m_requestUpdate = false;
            m_renderControl->sync();
            m_renderControl->render();
            m_waitCondition.wakeOne();
        }
    }

    m_context->functions()->glFlush();
    m_quickWindow->resetOpenGLState();
    m_context->doneCurrent();
    m_update = true;
}

void Q3DSQmlStreamRenderer::render()
{
    QMutexLocker lock(&m_renderMutex);
    if (m_update && m_initialized) {
        QOpenGLContext *context = QOpenGLContext::currentContext();
        QOpenGLFunctions *func = context->functions();
        GLuint texture = m_frameBuffer->texture();
        func->glDisable(GL_DEPTH_TEST);
        func->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        if (!m_program)
            initializeFboCopy();

        m_program->bind();

        if (!m_vao) {
            m_vao = new QOpenGLVertexArrayObject;
            m_vao->create();
            m_vao->bind();
            m_vertices->bind();

            m_program->enableAttributeArray(0);
            m_program->enableAttributeArray(1);
            func->glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
            func->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                                        (const void *)(4 * sizeof(GLfloat)));
            m_vertices->release();
        } else {
            m_vao->bind();
        }

        func->glActiveTexture(GL_TEXTURE0);
        func->glBindTexture(GL_TEXTURE_2D, texture);
        func->glDrawArrays(GL_TRIANGLES, 0, 6);
        func->glEnable(GL_DEPTH_TEST);

        m_program->release();
        m_vao->release();
    }
}
