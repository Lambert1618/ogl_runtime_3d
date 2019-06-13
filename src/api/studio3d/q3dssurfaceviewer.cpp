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

#include "q3dssurfaceviewer_p.h"
#include "Qt3DSAudioPlayerImpl.h"
#include "viewerqmlstreamproxy_p.h"
#include "q3dsviewersettings_p.h"
#include "q3dspresentation_p.h"
#include "studioutils_p.h"

#include <QtCore/qdebug.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qwindow.h>
#include <QtGui/QPlatformSurfaceEvent>

#include <QtCore/QFileInfo>

using namespace Q3DSViewer;

QT_BEGIN_NAMESPACE

/*!
    \class Q3DSSurfaceViewer
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.0

    \brief Renders a Qt 3D Studio presentation on a QWindow or an offscreen
    render target using OpenGL.

    Q3DSSurfaceViewer is used to render Qt 3D Studio presentations onto a
    QSurface. In practice this means two types of uses: rendering to an
    on-screen QWindow, or rendering to an offscreen render target (typically an
    OpenGL texture via a framebuffer object and a QOffscreenSurface).

    \section2 Example Usage

    \code
    int main(int argc, char *argv[])
    {
        QGuiApplication app(argc, argv);

        QOpenGLContext context;
        context.create();

        QWindow window;
        window.setSurfaceType(QSurface::OpenGLSurface);
        window.setFormat(context.format());
        window.create();

        Q3DSSurfaceViewer viewer;
        viewer.presentation()->setSource(QUrl(QStringLiteral("qrc:/my_presentation.uip")));
        viewer.setUpdateInterval(0); // enable automatic updates

        // Register a scene object for slide management (optional)
        Q3DSSceneElement scene(viewer.presentation(), QStringLiteral("Scene"));

        // Register an element object for attribute setting (optional)
        Q3DSElement element(viewer.presentation(), QStringLiteral("Scene.Layer.myCarModel"));

        viewer.create(&window, &context);

        w.resize(1024, 768);
        w.show();

        return app.exec();
    }
    \endcode

 */

/*!
 * \brief Q3DSSurfaceViewer::Q3DSSurfaceViewer Constructor.
 * \param parent Optional parent of the object.
 */
Q3DSSurfaceViewer::Q3DSSurfaceViewer(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSSurfaceViewerPrivate(this))
{
}

/*!
 * \brief Q3DSSurfaceViewer::~Q3DSSurfaceViewer Destructor
 */
Q3DSSurfaceViewer::~Q3DSSurfaceViewer()
{
    delete d_ptr;
}

/*!
    Initializes Q3DSSurfaceViewer to render the presentation to the given
    \a surface using the \a context.

    The source property of the attached presentation must be set before the
    viewer can be initialized.

    Returns whether the initialization succeeded.

    \sa running, Q3DSPresentation::source, presentation()
*/
bool Q3DSSurfaceViewer::create(QSurface *surface, QOpenGLContext *context)
{
    return d_ptr->initialize(surface, context, 0, false);
}


/*!
    Initializes Q3DSSurfaceViewer to render the presentation to the given
    \a surface using the \a context and optional framebuffer id (\a fboId). If
    \a fboId is omitted, it defaults to zero.

    The source property of the attached presentation must be set before the
    viewer can be initialized.

    Returns whether the initialization succeeded.

    \sa running, Q3DSPresentation::source, presentation()
*/
bool Q3DSSurfaceViewer::create(QSurface *surface, QOpenGLContext *context, GLuint fboId)
{
    return d_ptr->initialize(surface, context, fboId, true);
}

/*!
    Releases the presentation and all related resources.
    The Q3DSSurfaceViewer instance can be reused by calling create() again.
 */
void Q3DSSurfaceViewer::destroy()
{
    d_ptr->destroy();
}

/*!
    Updates the surface viewer with a new frame.
*/
void Q3DSSurfaceViewer::update()
{
    d_ptr->update();
}

/*!
    Grabs the data rendered to the framebuffer into an image using the given \a
    rect. The \a rect parameter is optional. If it is omitted, the whole
    framebuffer is captured.

    \note This is a potentially expensive operation.
*/
QImage Q3DSSurfaceViewer::grab(const QRect &rect)
{
    return d_ptr->grab(rect);
}

/*!
    \property Q3DSSurfaceViewer::size

    Holds the desired size of the presentation. Relevant only when
    autoSize is set to \c false.

    \sa autoSize
*/
QSize Q3DSSurfaceViewer::size() const
{
    return d_ptr->m_size;
}

void Q3DSSurfaceViewer::setSize(const QSize &size)
{
    d_ptr->setSize(size);
}

/*!
    \property Q3DSSurfaceViewer::autoSize

    Specifies whether the viewer should change the size of the presentation
    automatically to match the surface size when surface size changes. The
    \l{Q3DSSurfaceViewer::size}{size} property is updated automatically
    whenever the viewer is \l{Q3DSSurfaceViewer::update()}{updated} if this
    property value is \c{true}.

    When rendering offscreen, via a QOffscreenSurface, this property must be
    set to \c{false} by the application since it is then up to the application
    to provide a QOpenGLFramebufferObject with the desired size. The size of
    the Q3DSSurfaceViewer must be set to the same value.

    The default value is \c{true}.
*/
bool Q3DSSurfaceViewer::autoSize() const
{
    return d_ptr->m_autoSize;
}

void Q3DSSurfaceViewer::setAutoSize(bool autoSize)
{
    if (d_ptr->m_autoSize != autoSize) {
        d_ptr->m_autoSize = autoSize;
        Q_EMIT autoSizeChanged(autoSize);
    }
}

/*!
    \property Q3DSSurfaceViewer::updateInterval

    Holds the viewer update interval in milliseconds. If the value is negative,
    the viewer doesn't update the presentation automatically.

    The default value is -1, meaning there are no automatic updates and
    update() must be called manually.

    \sa update()
*/
int Q3DSSurfaceViewer::updateInterval() const
{
    return d_ptr->m_updateInterval;
}

void Q3DSSurfaceViewer::setUpdateInterval(int interval)
{
    d_ptr->setUpdateInterval(interval);
}

/*!
    \property Q3DSSurfaceViewer::running

    The value of this property is \c true when the presentation has been loaded
    and is ready to be shown.

    This property is read-only.
*/
bool Q3DSSurfaceViewer::isRunning() const
{
    return d_ptr->m_viewerApp != nullptr;
}

//     #TODO QT3DS-3534

/*!
    \property Q3DSSurfaceViewer::presentationId
 */
QString Q3DSSurfaceViewer::presentationId() const
{
    return d_ptr->m_id;
}

/*!
    Returns the framebuffer id given in initialization.

    \sa create()
*/
int Q3DSSurfaceViewer::fboId() const
{
    return d_ptr->m_fboId;
}

/*!
    Returns the error string.
*/
QString Q3DSSurfaceViewer::error() const
{
    return d_ptr->m_error;
}

/*!
    Returns the surface given in initialization.

    \sa create()
*/
QSurface *Q3DSSurfaceViewer::surface() const
{
    return d_ptr->m_surface;
}

/*!
    Returns the context given in initialization.

    \sa create()
*/
QOpenGLContext *Q3DSSurfaceViewer::context() const
{
    return d_ptr->m_context;
}

/*!
    Returns the settings object used by the Q3DSSurfaceViewer.
*/
Q3DSViewerSettings *Q3DSSurfaceViewer::settings() const
{
    return d_ptr->m_settings;
}

/*!
    Returns the presentation object used by the Q3DSSurfaceViewer.
*/
Q3DSPresentation *Q3DSSurfaceViewer::presentation() const
{
    return d_ptr->m_presentation;
}

/*!
 * \internal
 */
void Q3DSSurfaceViewer::setPresentationId(const QString &id)
{
    if (d_ptr->m_id != id) {
        d_ptr->m_id = id;
        Q_EMIT presentationIdChanged(id);
        if (d_ptr->m_viewerApp)
            d_ptr->m_viewerApp->setPresentationId(id);
    }
}

// TODO: QT3DS-3563

/*!
 * \brief Q3DSSurfaceViewer::qmlEngine
 * \return
 */
QQmlEngine *Q3DSSurfaceViewer::qmlEngine() const
{
    Q_D(const Q3DSSurfaceViewer);
    return d->qmlEngine;
}

/*!
 * \brief Q3DSSurfaceViewer::setQmlEngine
 * \param qmlEngine
 */
void Q3DSSurfaceViewer::setQmlEngine(QQmlEngine *qmlEngine)
{
    Q_D(Q3DSSurfaceViewer);
    d->qmlEngine = qmlEngine;
}


/*!
    \fn Q3DSSurfaceViewer::frameUpdate()

    Emitted each time a frame has been rendered.
*/

/*!
    \fn Q3DSSurfaceViewer::presentationLoaded()

    Emitted when the presentation has been loaded and is ready
    to be shown.
*/

/*!
    \fn Q3DSSurfaceViewer::presentationReady()
    Emitted when first frame is about to be shown. DataInputs and setAttribute
    are ready to be used after this signal.
 */

/*!
 * \internal
 */
Q3DSSurfaceViewerPrivate::Q3DSSurfaceViewerPrivate(Q3DSSurfaceViewer *q)
    : QObject(q)
    , q_ptr(q)
    , m_viewerApp(nullptr)
    , m_timer(nullptr)
    , m_updateInterval(-1)
    , m_pixelRatio(1.0)
    , m_fboId(0)
    , m_surface(nullptr)
    , m_context(nullptr)
    , m_autoSize(true)
    , m_settings(new Q3DSViewerSettings(this))
    , m_presentation(new Q3DSPresentation(this))
{
    m_startupTimer.start();
    connect(m_presentation, &Q3DSPresentation::sourceChanged,
            this, &Q3DSSurfaceViewerPrivate::reset);
}

/*!
 * \internal
 */
Q3DSSurfaceViewerPrivate::~Q3DSSurfaceViewerPrivate()
{
    releaseRuntime();

    delete m_timer;
}

/*!
 * \internal
 */
void Q3DSSurfaceViewerPrivate::setSize(const QSize &size)
{
    if (m_size != size) {
        m_size = size;

        if (m_viewerApp) {
            m_context->makeCurrent(m_surface);
            m_viewerApp->Resize(int(m_size.width() * m_pixelRatio),
                                int(m_size.height() * m_pixelRatio));
        }

        Q_EMIT q_ptr->sizeChanged(m_size);
    }
}

/*!
 * \internal
 */
void Q3DSSurfaceViewerPrivate::setUpdateInterval(int interval)
{
    if (m_updateInterval != interval) {
        m_updateInterval = interval;
        resetUpdateTimer();
        Q_EMIT q_ptr->updateIntervalChanged(m_updateInterval);
    }
}

/*!
 * \internal
 */
bool Q3DSSurfaceViewerPrivate::initialize(QSurface *surface, QOpenGLContext *context, GLuint fboId,
                                          bool idValid)
{
    Q_ASSERT(context);
    Q_ASSERT(surface);

    if (m_presentation->source().isEmpty()) {
        setError(QStringLiteral("Failed to initialize Q3DSSurfaceViewer,"
                                 " presentation source must be set before calling initialize()"));
        return false;
    }

    QFileInfo info(Q3DSUtils::urlToLocalFileOrQrc(m_presentation->source()));
    if (!info.exists()) {
        setError(QStringLiteral("Failed to initialize Q3DSSurfaceViewer, the presentation doesn't exist: %1")
                .arg(m_presentation->source().toString()));
        return false;
    }

    destroy();

    m_surface = surface;
    m_context = context;
    m_fboId = idValid ? fboId : m_context->defaultFramebufferObject();
    if (m_surface->surfaceClass() == QSurface::Window)
        m_pixelRatio = static_cast<QWindow *>(m_surface)->devicePixelRatio();

    surfaceObject()->installEventFilter(this);

    connect(context, &QOpenGLContext::aboutToBeDestroyed, this, &Q3DSSurfaceViewerPrivate::destroy);

    bool success = initializeRuntime();
    resetUpdateTimer();

    if (success)
        Q_EMIT q_ptr->runningChanged(true);

    return success;
}

/*!
 * \internal
 */
void Q3DSSurfaceViewerPrivate::destroy()
{
    bool oldInitialized = (m_viewerApp != nullptr);

    if (m_context) {
        disconnect(m_context, &QOpenGLContext::aboutToBeDestroyed,
                   this, &Q3DSSurfaceViewerPrivate::destroy);
    }

    if (m_surface)
        surfaceObject()->removeEventFilter(this);

    releaseRuntime();

    m_surface = nullptr;
    m_context = nullptr;
    m_fboId = 0;

    if (oldInitialized)
        Q_EMIT q_ptr->runningChanged(false);
}

/*!
 * \internal
 */
void Q3DSSurfaceViewerPrivate::update()
{
    if (m_viewerApp) {
        if (m_surface->surfaceClass() != QSurface::Window
                || static_cast<QWindow *>(m_surface)->isExposed()) {
            if (!m_context->makeCurrent(m_surface)) {
                qWarning () << "Q3DSSurfaceViewer: Unable to make context current";
                return;
            }

            if (!m_viewerApp->IsInitialised() && !initializeRuntime())
                return;

            if (m_autoSize)
                setSize(m_surface->size());
            m_viewerApp->Render();

            const uint defaultFbo = m_context->defaultFramebufferObject();

            if (m_surface->surfaceClass() == QSurface::Window && m_fboId == defaultFbo)
                m_context->swapBuffers(m_surface);

            Q_EMIT q_ptr->frameUpdate();
        }
    }
}

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format,
                                                  bool include_alpha);

/*!
 * \internal
 */
QImage Q3DSSurfaceViewerPrivate::grab(const QRect &rect)
{
    QRect captureRect;
    QSize fullSize = m_size * m_pixelRatio;
    if (rect.isValid()) {
        captureRect = QRect(rect.x() * m_pixelRatio, rect.y() * m_pixelRatio,
                            rect.width() * m_pixelRatio, rect.height() * m_pixelRatio);
    } else {
        captureRect = QRect(0, 0, fullSize.width(), fullSize.height());
    }
    QImage image(captureRect.size(), QImage::Format_ARGB32);

    if (m_surface && m_context && m_viewerApp && m_viewerApp->IsInitialised()
            && (m_surface->surfaceClass() != QSurface::Window
                || static_cast<QWindow *>(m_surface)->isExposed())) {
        m_context->makeCurrent(m_surface);

        // Render now to ensure the image is up to date and will actually exist in case
        // the surface has a non-preserved swap buffer
        if (m_autoSize)
            setSize(m_surface->size());
        m_viewerApp->Render();

        m_context->functions()->glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
        QImage fullGrab = qt_gl_read_framebuffer(fullSize, false, false);

        // Also update the screen to match the grab, since we just rendered
        if (m_surface->surfaceClass() == QSurface::Window
                && m_fboId == m_context->defaultFramebufferObject()) {
            m_context->swapBuffers(m_surface);
        }

        if (captureRect.size() == fullSize)
            image = fullGrab;
        else
            image = fullGrab.copy(captureRect);
    }

    return image;
}

/*!
 * \internal
 */
bool Q3DSSurfaceViewerPrivate::eventFilter(QObject *obj, QEvent *e)
{
    if (m_surface && e->type() == QEvent::PlatformSurface) {
        if (surfaceObject() == obj) {
            QPlatformSurfaceEvent *ev = static_cast<QPlatformSurfaceEvent *>(e);
            if (ev->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
                destroy();
        }
    }
    return QObject::eventFilter(obj, e);
}

/*!
 * \internal
 */
void Q3DSSurfaceViewerPrivate::reset()
{
    if (m_viewerApp) {
        releaseRuntime();
        initializeRuntime();
        resetUpdateTimer();
    }
}

/*!
 * \internal
 */
void Q3DSSurfaceViewerPrivate::setError(const QString &error)
{
    if (m_error != error) {
        m_error = error;
        qWarning() << error;
        Q_EMIT q_ptr->errorChanged();
    }
}

/*!
 * \internal
 */
bool Q3DSSurfaceViewerPrivate::initializeRuntime()
{
    if (!m_viewerApp) {
        m_viewerApp = &Q3DSViewerApp::Create(m_context, new Qt3DSAudioPlayerImpl(), &m_startupTimer);
        connect(m_viewerApp, &Q3DSViewerApp::SigPresentationReady,
                this->q_ptr, &Q3DSSurfaceViewer::presentationReady);
        connect(m_viewerApp, &Q3DSViewerApp::SigPresentationLoaded,
                this->q_ptr, &Q3DSSurfaceViewer::presentationLoaded);
        Q_ASSERT(m_viewerApp);
    }
    if (!m_context->makeCurrent(m_surface)) {
        qWarning () << "Q3DSSurfaceViewer: Unable to make context current";
        return false;
    }


    const QString localSource = Q3DSUtils::urlToLocalFileOrQrc(m_presentation->source());

    if (m_autoSize)
        m_size = m_surface->size();

    if (nullptr != qmlEngine)
        m_presentation->d_ptr->streamProxy()->setEngine(qmlEngine);

    if (!m_viewerApp->InitializeApp(int(m_size.width() * m_pixelRatio),
                                    int(m_size.height() * m_pixelRatio),
                                    m_context->format(), m_fboId, localSource,
                                    m_presentation->variantList(),
                                    m_presentation->delayedLoading(),
                                    m_presentation->d_ptr->streamProxy())) {
        setError(m_viewerApp->error());
        releaseRuntime();
        qWarning("Failed to initialize runtime");
        return false;
    }

    if (!m_id.isEmpty())
        m_viewerApp->setPresentationId(m_id);
    m_settings->d_ptr->setViewerApp(m_viewerApp);
    m_presentation->d_ptr->setViewerApp(m_viewerApp);
    return true;
}

/*!
 * \internal
 */
void Q3DSSurfaceViewerPrivate::releaseRuntime()
{
    m_settings->d_ptr->setViewerApp(nullptr);
    m_presentation->d_ptr->setViewerApp(nullptr);

    if (m_context && m_surface)
        m_context->makeCurrent(m_surface);

    if (m_viewerApp) {
        m_viewerApp->Release();
        m_viewerApp = nullptr;
    }

    resetUpdateTimer();
}

/*!
 * \internal
 */
void Q3DSSurfaceViewerPrivate::resetUpdateTimer()
{
    if (m_viewerApp && m_updateInterval >= 0) {
        if (!m_timer) {
            m_timer = new QTimer();
            connect(m_timer, &QTimer::timeout, this, &Q3DSSurfaceViewerPrivate::update);
        }
        m_timer->start(m_updateInterval);
    } else if (m_timer) {
        m_timer->stop();
    }
}

/*!
 * \internal
 */
QObject *Q3DSSurfaceViewerPrivate::surfaceObject()
{
    if (m_surface) {
        if (m_surface->surfaceClass() == QSurface::Window)
            return static_cast<QWindow *>(m_surface);
        else
            return static_cast<QOffscreenSurface *>(m_surface);
    }
    return nullptr;
}

QT_END_NAMESPACE
