/****************************************************************************
**
** Copyright (c) 2016 NVIDIA CORPORATION.
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

#include "q3dsstudio3d_p.h"
#include "q3dsrenderer_p.h"
#include "q3dspresentationitem_p.h"

#include <QtStudio3D/private/q3dsviewersettings_p.h>
#include <QtStudio3D/private/q3dspresentation_p.h>
#include <QtStudio3D/private/q3dssceneelement_p.h>
#include <QtStudio3D/private/viewerqmlstreamproxy_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfileinfo.h>
#include <QtQuick/qquickwindow.h>
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Studio3D
    \instantiates Q3DSStudio3D
    \inqmlmodule QtStudio3D.OpenGL
    \ingroup OpenGLRuntime
    \inherits Item
    \keyword Studio3D

    \brief Qt 3D Studio presentation viewer.

    This type enables developers to embed Qt 3D Studio presentations in Qt
    Quick.

    \section2 Example Usage

    \qml
    Studio3D {
        id: studio3D
        anchors.fill: parent

        Presentation {
            source: "qrc:///presentation.uia"
            SceneElement {
                id: scene
                elementPath: "Scene"
                currentSlideIndex: 2
            }
            Element {
                id: textLabel
                elementPath: "Scene.Layer.myLabel"
            }
        }
        ViewerSettings {
            showRenderStats: true
        }
        onRunningChanged: {
            console.log("Presentation ready!");
        }
    }
    \endqml

    \section2 Controlling the presentation

    Like the example above suggests, Studio3D and the other types under the
    QtStudio3D import offer more than simply rendering the animated Qt 3D
    Studio presentation. They also offer scene manipulation, including

    \list

    \li querying and changing scene object properties (for example, the
    transform of a model, colors and other settings of a material, etc.) via
    Presentation::getAttribute(), Presentation::setAttribute(), \l Element, and
    \l DataInput,

    \li changing slides (and thus starting the relevant animations and applying
    the scene object property changes associated with the new slide) via
    Presentation::goToSlide(), \l SceneElement, and \l DataInput,

    \li and controlling the timeline (the current playback position for the
    key-frame based animations) both on the main scene and on individual
    Component nodes via Presentation::goToTime(), \l SceneElement, and \l DataInput.

    \endlist

    \sa Presentation
*/

/*!
    \qmlsignal Studio3D::frameUpdate()

    This signal is emitted each time a frame has been rendered.
*/

/*!
    \qmlsignal Studio3D::presentationLoaded()

    This signal is emitted when the presentation has been loaded and is ready
    to be shown.
*/

/*!
    \qmlsignal Studio3D::presentationReady()

    This signal is emitted when the presentation has fully initialized its 3D
    scene for the first frame.

    The difference to presentationLoaded() is that this signal is emitted only
    when the asynchronous operations needed to build to 3D scene and the first
    frame have completed.

    When implementing splash screens via Loader items and the Item::visible
    property, this is the signal that should be used to trigger hiding the
    splash screen.
*/

Q3DSStudio3D::Q3DSStudio3D()
    : m_viewerSettings(nullptr)
    , m_presentation(nullptr)
    , m_emitRunningChange(false)
    , m_isRunning(false)
    , m_eventIgnoreFlags(EnableAllEvents)
    , m_pixelRatio(1.0)
{
    setMirrorVertically(true);
    connect(this, &Q3DSStudio3D::windowChanged, this, &Q3DSStudio3D::handleWindowChanged);
    connect(this, &Q3DSStudio3D::visibleChanged, this, &Q3DSStudio3D::handleVisibleChanged);

    updateEventMasks();
    m_startupTimer.reset(new QElapsedTimer());
    m_startupTimer->restart();
}

Q3DSStudio3D::~Q3DSStudio3D()
{
}

/*!
    \qmlproperty Presentation Studio3D::presentation

    Accessor for the presentation. Applications are expected to create a single
    Presentation child object for Studio3D. If this is omitted, a presentation
    is created automatically.

    This property is read-only.
*/
Q3DSPresentationItem *Q3DSStudio3D::presentation() const
{
    return m_presentation;
}

/*!
    \qmlproperty ViewerSettings Studio3D::viewerSettings

    Accessor for the viewerSettings. Applications are expected to create a single
    ViewerSettings child object for Studio3D. If this is omitted, ViewerSettings
    is created automatically.

    This property is read-only.
*/
Q3DSViewerSettings *Q3DSStudio3D::viewerSettings() const
{
    return m_viewerSettings;
}

/*!
    \qmlproperty string Studio3D::error

    Contains the text for the error message that was generated during the
    loading of the presentation. When no error occurred or there is no
    presentation loaded, the value is an empty string.

    This property is read-only.
*/
QString Q3DSStudio3D::error() const
{
    return m_error;
}

void Q3DSStudio3D::setError(const QString &error)
{
    if (error != m_error) {
        m_error = error;
        Q_EMIT errorChanged(m_error);
    }
}

/*!
    \qmlproperty bool Studio3D::asyncInit

    If set to \c{true}, indicates that renderer initialization should be done asynchronously
    in a helper thread. This improves UI responsiveness while initialization is happening,
    but can lead to slower initialization. Asynchronous initialization may not work properly
    on all platforms.

    Defaults to \c{false}.

    Changing this property after renderer is created doesn't do anything.
*/
bool Q3DSStudio3D::asyncInit() const
{
    return m_asyncInit;
}

void Q3DSStudio3D::setAsyncInit(bool enabled)
{
    if (enabled != m_asyncInit) {
        m_asyncInit = enabled;
        Q_EMIT asyncInitChanged(m_asyncInit);
    }
}

/*!
    \qmlproperty EventIgnoreFlags Studio3D::ignoredEvents

    This property can be used to ignore mouse/wheel/keyboard events.
    By default all events are enabled.
*/

Q3DSStudio3D::EventIgnoreFlags Q3DSStudio3D::ignoredEvents() const
{
    return m_eventIgnoreFlags;
}

void Q3DSStudio3D::setIgnoredEvents(EventIgnoreFlags flags)
{
    if (m_eventIgnoreFlags == flags)
        return;

    m_eventIgnoreFlags = flags;
    updateEventMasks();
    Q_EMIT ignoredEventsChanged();
}

/*!
    \internal
 */
void Q3DSStudio3D::updateEventMasks()
{
    if (m_eventIgnoreFlags.testFlag(IgnoreMouseEvents)) {
        setAcceptedMouseButtons(Qt::NoButton);
        setAcceptHoverEvents(false);
    } else {
        setAcceptedMouseButtons(Qt::MouseButtonMask);
        setAcceptHoverEvents(true);
    }
}

/*!
    \internal
 */
void Q3DSStudio3D::componentComplete()
{
    const auto childObjs = children();
    for (QObject *child : childObjs) {
        auto settings = qobject_cast<Q3DSViewerSettings *>(child);
        if (settings) {
            if (m_viewerSettings)
                qWarning("Duplicate ViewerSettings defined for Studio3D.");
            else
                m_viewerSettings = settings;
        }
        auto presentation = qobject_cast<Q3DSPresentationItem *>(child);
        if (presentation) {
            if (m_presentation)
                qWarning("Duplicate Presentation defined for Studio3D.");
            else
                m_presentation = presentation;
        }
    }
    if (!m_viewerSettings)
        m_viewerSettings = new Q3DSViewerSettings(this);
    if (!m_presentation)
        m_presentation = new Q3DSPresentationItem(this);

    m_viewerSettings->d_ptr->setCommandQueue(&m_pendingCommands);
    m_presentation->d_ptr->setCommandQueue(&m_pendingCommands);

    // Ensure qml stream proxy gets created on main thread
    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    m_presentation->d_ptr->streamProxy()->setEngine(ctx->engine());

    QObject::connect(m_presentation, &Q3DSPresentationItem::sourceChanged, [this]() {
        m_startupTimer->restart();
    });

    QQuickFramebufferObject::componentComplete();
}

/*!
    \internal
 */
void Q3DSStudio3D::handleWindowChanged(QQuickWindow *window)
{
    if (!window)
        return;

    // We need to enable clearing until the presentation starts rendering and takes care of that.
    // Note that having this flag as false assumes presentation is always full screen.
    window->setClearBeforeRendering(m_asyncInit);

    m_pixelRatio = window->devicePixelRatio();

    // Call tick every frame of the GUI thread to notify QML about new frame via frameUpdate signal
    connect(window, &QQuickWindow::afterAnimating, this, &Q3DSStudio3D::tick);
    // Call update after the frame is handled to queue another frame
    connect(window, &QQuickWindow::afterSynchronizing, this, &Q3DSStudio3D::update);

    reset();
}

/*!
    \internal
    Queue up a command to inform the renderer of a newly-changed visible/hidden status.
 */
void Q3DSStudio3D::handleVisibleChanged()
{
    m_pendingCommands.m_visibleChanged = true;
    m_pendingCommands.m_visible = isVisible();
}

/*!
    \brief internal
 */
void Q3DSStudio3D::reset()
{
    // Fake a source change to trigger a reloading of the presentation
    m_pendingCommands.m_sourceChanged = true;
    m_pendingCommands.m_source = m_presentation ? m_presentation->source() : QString();
    m_pendingCommands.m_variantListChanged = true;
    m_pendingCommands.m_variantList = m_presentation ? m_presentation->variantList() : QStringList();
}

/*!
    \internal
 */
void Q3DSStudio3D::requestResponseHandler(const QString &elementPath, CommandType commandType,
                                      void *requestData)
{
    switch (commandType) {
    case CommandType_RequestSlideInfo: {
        Q3DSSceneElement *handler = qobject_cast<Q3DSSceneElement *>(
                    m_presentation->registeredElement(elementPath));
        if (handler)
            handler->d_ptr->requestResponseHandler(commandType, requestData);
        else
            qWarning() << __FUNCTION__ << "RequestSlideInfo response got for unregistered scene.";
        break;
    }
    case CommandType_RequestDataInputs: {
        Q3DSPresentation *handler = qobject_cast<Q3DSPresentation *>(m_presentation);
        if (handler) {
            handler->d_ptr->requestResponseHandler(commandType, requestData);
        } else {
            qWarning() << __FUNCTION__
                       << "RequestDataInputs response got for invalid presentation.";
        }
        break;
    }
    case CommandType_RequestDataOutputs: {
        Q3DSPresentation *handler = qobject_cast<Q3DSPresentation *>(m_presentation);
        if (handler) {
            handler->d_ptr->requestResponseHandler(commandType, requestData);
        } else {
            qWarning() << __FUNCTION__
                       << "RequestDataOutputs response got for invalid presentation.";
        }
        break;
    }
    default:
        qWarning() << __FUNCTION__ << "Unknown command type.";
        break;
    }
}

/*!
    \internal
    Create the Q3DSRenderer. Invoked automatically by the QML scene graph.
 */
QQuickFramebufferObject::Renderer *Q3DSStudio3D::createRenderer() const
{
    // It is "illegal" to create a connection between the renderer
    // and the plugin, and vice-versa. The only valid time the two
    // may communicate is during Q3DSRenderer::synchronize().
    Q3DSRenderer *renderer = new Q3DSRenderer(isVisible(), m_presentation->d_ptr->streamProxy(),
                                              m_startupTimer.get(), m_asyncInit);
    connect(renderer, &Q3DSRenderer::enterSlide,
            m_presentation->d_ptr, &Q3DSPresentationPrivate::handleSlideEntered);
    connect(renderer, &Q3DSRenderer::dataOutputValueUpdated,
            m_presentation->d_ptr, &Q3DSPresentationPrivate::handleDataOutputValueUpdate);
    connect(renderer, &Q3DSRenderer::elementsCreated,
            m_presentation->d_ptr, &Q3DSPresentationPrivate::handleElementsCreated);
    connect(renderer, &Q3DSRenderer::materialsCreated,
            m_presentation->d_ptr, &Q3DSPresentationPrivate::handleMaterialsCreated);
    connect(renderer, &Q3DSRenderer::meshesCreated,
            m_presentation->d_ptr, &Q3DSPresentationPrivate::handleMeshesCreated);
    connect(renderer, &Q3DSRenderer::exitSlide,
            m_presentation, &Q3DSPresentation::slideExited);
    connect(renderer, &Q3DSRenderer::customSignalEmitted,
            m_presentation, &Q3DSPresentation::customSignalEmitted);
    connect(renderer, &Q3DSRenderer::requestResponse,
            this, &Q3DSStudio3D::requestResponseHandler);
    connect(renderer, &Q3DSRenderer::presentationLoaded,
            this, &Q3DSStudio3D::presentationLoaded);
    connect(renderer, &Q3DSRenderer::presentationReady,
            this, &Q3DSStudio3D::presentationReady);
    return renderer;
}

/*!
    \qmlproperty bool Studio3D::running

    The value of this property is \c true when the presentation has been loaded
    and is ready to be shown.

    This property is read-only.
*/
bool Q3DSStudio3D::isRunning() const
{
    return m_isRunning;
}

/*!
    \internal
    Emit QML `runningChanged` and `frameUpdate` and signals.
    This method is called every frame, and emits the `frameUpdate` signal every frame,
    regardless of plugin visibility. This allows a hidden Qt3DSView to still process
    information every frame, even though the Renderer is not rendering.

    To prevent expensive onFrameUpdate handlers from being processed when hidden,
    add an early return to the top like:

        onFrameUpdate: {
            if (!visible) return;
            ...
        }
 */
void Q3DSStudio3D::tick()
{
    if (m_emitRunningChange) {
        m_isRunning = true;
        Q_EMIT runningChanged(true);
        m_emitRunningChange = false;
    }

    // Don't call onFrameUpdate until after onInitialize has been called
    if (m_isRunning) {
        // Give QML an opportunity to change Qt3DS values every frame
        Q_EMIT frameUpdate();
    }
}

/*!
    \internal
    Copies the list of commands previously queued up. Called by Q3DSRenderer::synchronize().
  */
void Q3DSStudio3D::getCommands(bool emitInitialize, CommandQueue &renderQueue)
{
    if (m_presentation->d_ptr->m_dataInputsChanged) {
        m_presentation->d_ptr->setDataInputValueBatch();
        m_presentation->d_ptr->m_dataInputsChanged = false;
    }

    if (emitInitialize)
        m_emitRunningChange = true;

    renderQueue.copyCommands(m_pendingCommands);
    m_pendingCommands.clear(false);
}

/*!
    \internal
 */
void Q3DSStudio3D::mousePressEvent(QMouseEvent *event)
{
    if (m_eventIgnoreFlags.testFlag(IgnoreMouseEvents))
        return;

    if (m_pixelRatio != 1.0) {
        QMouseEvent scaledEvent(event->type(), event->pos() * m_pixelRatio,
                                event->button(), event->buttons(), event->modifiers());
        m_presentation->mousePressEvent(&scaledEvent);
    } else {
        m_presentation->mousePressEvent(event);
    }
}

/*!
    \internal
 */
void Q3DSStudio3D::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_eventIgnoreFlags.testFlag(IgnoreMouseEvents))
        return;

    if (m_pixelRatio != 1.0) {
        QMouseEvent scaledEvent(event->type(), event->pos() * m_pixelRatio,
                                event->button(), event->buttons(), event->modifiers());
        m_presentation->mouseReleaseEvent(&scaledEvent);
    } else {
        m_presentation->mouseReleaseEvent(event);
    }
}

/*!
    \internal
 */
void Q3DSStudio3D::mouseMoveEvent(QMouseEvent *event)
{
    if (m_eventIgnoreFlags.testFlag(IgnoreMouseEvents))
        return;

    if (m_pixelRatio != 1.0) {
        QMouseEvent scaledEvent(event->type(), event->pos() * m_pixelRatio,
                                event->button(), event->buttons(), event->modifiers());
        m_presentation->mouseMoveEvent(&scaledEvent);
    } else {
        m_presentation->mouseMoveEvent(event);
    }
}

/*!
    \internal
 */
void Q3DSStudio3D::wheelEvent(QWheelEvent *event)
{
    if (m_eventIgnoreFlags.testFlag(IgnoreWheelEvents))
        return;

    m_presentation->wheelEvent(event);
}

/*!
    \internal
 */
void Q3DSStudio3D::keyPressEvent(QKeyEvent *event)
{
    if (m_eventIgnoreFlags.testFlag(IgnoreKeyboardEvents))
        return;

    m_presentation->keyPressEvent(event);
}

/*!
    \internal
 */
void Q3DSStudio3D::keyReleaseEvent(QKeyEvent *event)
{
    if (m_eventIgnoreFlags.testFlag(IgnoreKeyboardEvents))
        return;

    if (!event->isAutoRepeat())
        m_presentation->keyReleaseEvent(event);
}

QT_END_NAMESPACE
