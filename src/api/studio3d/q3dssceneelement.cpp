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

#include "q3dssceneelement_p.h"
#include "q3dspresentation_p.h"
#include "q3dscommandqueue_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SceneElement
    \instantiates Q3DSSceneElement
    \inqmlmodule Qt3DStudio
    \ingroup OpenGLRuntime
    \brief Controls the special Scene or Component scene objects in a Qt 3D
    Studio presentation.

    This class is a convenience class for controlling the properties of Scene
    and Component objects in the scene. These are special since they have a
    time context, meaning they control a timeline and a set of associated
    slides.

    \sa Studio3D, Element, Presentation
*/

/*!
    \class Q3DSSceneElement
    \inherits Q3DSElement
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.0

    \brief Controls the special Scene or Component scene objects in a Qt 3D
    Studio presentation.

    This class is a convenience class for controlling the properties of Scene
    and Component objects in the scene. These are special since they have a
    time context, meaning they control a timline and a set of associated
    slides.

    \note The functionality of Q3DSSceneElement is equivalent to
    Q3DSPresentation::goToTime() and Q3DSPresentation::goToSlide().

    \sa Q3DSPresentation, Q3DSWidget, Q3DSSurfaceViewer, Q3DSElement
 */

/*!
    \internal
 */
Q3DSSceneElement::Q3DSSceneElement(QObject *parent)
    : Q3DSElement(new Q3DSSceneElementPrivate(this), nullptr, QString(), parent)
{
}

/*!
    \internal
 */
Q3DSSceneElement::Q3DSSceneElement(const QString &elementPath, QObject *parent)
    : Q3DSElement(new Q3DSSceneElementPrivate(this), nullptr, elementPath, parent)
{
}

/*!
    Constructs a Q3DSSceneElement instance and associated it with the object
    specified by \a elementPath and the given \a presentation. An optional \a
    parent object can be specified.
 */
Q3DSSceneElement::Q3DSSceneElement(Q3DSPresentation *presentation, const QString &elementPath,
                                   QObject *parent)
    : Q3DSElement(new Q3DSSceneElementPrivate(this), presentation, elementPath, parent)
{

}

/*!
   Destructor.
 */
Q3DSSceneElement::~Q3DSSceneElement()
{
}

/*!
    \qmlproperty int SceneElement::currentSlideIndex

    Holds the index of the currently active slide of the tracked time context.

    \note If this property is set to something else than the default slide for
    the scene at the initial declaration of SceneElement, a changed signal for
    the default slide may still be emitted before the slide changes to the
    desired one. This happens in order to ensure we end up with the index of
    the slide that is actually shown even if the slide specified in the initial
    declaration is invalid.
*/

/*!
    \property Q3DSSceneElement::currentSlideIndex

    Holds the index of the currently active slide of the tracked time context.

    \note If this property is set to something else than the default slide for
    the scene at the initial declaration of SceneElement, a changed signal for
    the default slide may still be emitted before the slide changes to the
    desired one. This happens in order to ensure we end up with the index of
    the slide that is actually shown even if the slide specified in the initial
    declaration is invalid.
*/
int Q3DSSceneElement::currentSlideIndex() const
{
    Q_D(const Q3DSSceneElement);
    return d->m_currentSlideIndex;
}

void Q3DSSceneElement::setCurrentSlideIndex(int currentSlideIndex)
{
    Q_D(Q3DSSceneElement);
    if (d->m_viewerApp) {
        const QByteArray path(d->m_elementPath.toUtf8());
        d->m_viewerApp->GoToSlideByIndex(path, currentSlideIndex + 1);
    } else if (d->m_commandQueue) {
        d->m_commandQueue->queueCommand(d->m_elementPath, CommandType_GoToSlide,
                                            int(currentSlideIndex + 1));
    } else {
        // Store desired slide until we have either app or queue. Only name or index can be set.
        d->m_initialSlideIndex = currentSlideIndex + 1;
        d->m_initialSlideName.clear();
    }
}

/*!
    \property int SceneElement::previousSlideIndex

    Holds the index of the previously active slide of the tracked time context.

    Note: This property is read-only.
*/
/*!
    \property Q3DSSceneElement::previousSlideIndex

    Holds the index of the previously active slide of the tracked time context.

    This property is read-only.
*/
int Q3DSSceneElement::previousSlideIndex() const
{
    Q_D(const Q3DSSceneElement);
    return d->m_previousSlideIndex;
}

/*!
    \qmlproperty string SceneElement::currentSlideName

    Holds the name of the currently active slide of the tracked time context.

    \note If this property is set to something else than the default slide for
    the scene at the initial declaration of SceneElement, a changed signal for
    the default slide may still be emitted before the slide changes to the
    desired one. This happens in order to ensure we end up with the index of
    the slide that is actually shown even if the slide specified in the initial
    declaration is invalid.
*/

/*!
    \property Q3DSSceneElement::currentSlideName

    Holds the name of the currently active slide of the tracked time context.

    \note If this property is set to something else than the default slide for
    the scene at the initial declaration of SceneElement, a changed signal for
    the default slide may still be emitted before the slide changes to the
    desired one. This happens in order to ensure we end up with the index of
    the slide that is actually shown even if the slide specified in the initial
    declaration is invalid.
*/
QString Q3DSSceneElement::currentSlideName() const
{
    Q_D(const Q3DSSceneElement);
    return d->m_currentSlideName;
}

void Q3DSSceneElement::setCurrentSlideName(const QString &currentSlideName)
{
    Q_D(Q3DSSceneElement);
    if (d->m_viewerApp) {
        const QByteArray path(d->m_elementPath.toUtf8());
        const QByteArray name(currentSlideName.toUtf8());
        d->m_viewerApp->GoToSlideByName(path, name);
    } else if (d->m_commandQueue) {
        d->m_commandQueue->queueCommand(d->m_elementPath, CommandType_GoToSlideByName,
                                            currentSlideName);
    } else {
        // Store desired slide until we have either app or queue. Only name or index can be set.
        d->m_initialSlideName = currentSlideName;
        d->m_initialSlideIndex = 0;
    }
}

/*!
    \qmlmproperty string SceneElement::previousSlideName

    Holds the name of the previously active slide of the tracked time context.

    Note: This property is read-only.
*/
/*!
    \property Q3DSSceneElement::previousSlideName

    Holds the name of the previously active slide of the tracked time context.

    This property is read-only.
*/
QString Q3DSSceneElement::previousSlideName() const
{
    Q_D(const Q3DSSceneElement);
    return d->m_previousSlideName;
}

/*!
    Requests a time context (a Scene or a Component object) to change to the
    next or previous slide, depending on the value of \a next. If the context
    is already at the last or first slide, \a wrap defines if wrapping over to
    the first or last slide, respectively, occurs.
 */
void Q3DSSceneElement::goToSlide(bool next, bool wrap)
{
    Q_D(Q3DSSceneElement);
    d->goToSlide(next, wrap);
}

/*!
    Moves the timeline for a time context (a Scene or a Component element) to a
    specific position. The position is given in seconds in \a timeSeconds.
 */
void Q3DSSceneElement::goToTime(float time)
{
    Q_D(Q3DSSceneElement);
    d->goToTime(time);
}


/*!
    \internal
 */
Q3DSSceneElementPrivate::Q3DSSceneElementPrivate(Q3DSSceneElement *parent)
    : Q3DSElementPrivate(parent)
    , m_currentSlideIndex(0)
    , m_previousSlideIndex(0)
    , m_initialSlideIndex(0)
    , m_slideInfoRequestPending(false)
{
}

/*!
    \internal
 */
Q3DSSceneElementPrivate::~Q3DSSceneElementPrivate()
{
}

/*!
    \internal
 */
void Q3DSSceneElementPrivate::handleSlideEntered(int index, const QString &name)
{
    Q_Q(Q3DSSceneElement);

    // Initializing presentation will report slide entered for the scenes on the default slide
    // of the presentation even if user has specified different initial slides.
    // Since we don't have robust error reporting mechanism from the viewerapp,
    // we cannot simply ignore these initial enters, as there is no guarantee the slide
    // user wants even exists.

    // We ignore the slide exited signals and rely on stored previous slide data
    // to avoid excessive signaling on slide changes.
    bool notifyCurrent = m_currentSlideIndex != index;
    bool notifyPrevious = m_previousSlideIndex != m_currentSlideIndex;

    // Since child (i.e. component) slides always get enter event when parent slide is entered,
    // it is possible that current and previous slides are the same. This feels bit odd, but is
    // technically correct, as the last time we got enter, the same slide was made current.
    // It also matches the internal m_viewerApp logic for previous slides.
    m_previousSlideIndex = m_currentSlideIndex;
    m_previousSlideName = m_currentSlideName;
    m_currentSlideIndex = index;
    m_currentSlideName = name;

    if (notifyPrevious) {
        Q_EMIT q->previousSlideIndexChanged(m_previousSlideIndex);
        Q_EMIT q->previousSlideNameChanged(m_previousSlideName);
    }

    if (notifyCurrent) {
        Q_EMIT q->currentSlideIndexChanged(m_currentSlideIndex);
        Q_EMIT q->currentSlideNameChanged(m_currentSlideName);
    }
}

/*!
    \internal
 */
void Q3DSSceneElementPrivate::goToSlide(bool next, bool wrap)
{
    if (m_presentation)
        m_presentation->q_ptr->goToSlide(m_elementPath, next, wrap);
    else
        qWarning() << __FUNCTION__ << "Element is not registered to any presentation!";
}

/*!
    \internal
 */
void Q3DSSceneElementPrivate::goToTime(float time)
{
    if (m_presentation)
        m_presentation->q_ptr->goToTime(m_elementPath, time);
    else
        qWarning() << __FUNCTION__ << "Element is not registered to any presentation!";
}

/*!
    \internal
 */
void Q3DSSceneElementPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    Q_Q(Q3DSSceneElement);

    if (app || m_viewerApp) {
        m_currentSlideIndex = 0;
        m_currentSlideName.clear();
        m_previousSlideIndex = 0;
        m_previousSlideName.clear();
    }

    Q3DSElementPrivate::setViewerApp(app);

    if (m_viewerApp) {
        const QByteArray path(m_elementPath.toUtf8());
        m_viewerApp->GetSlideInfo(path, m_currentSlideIndex, m_previousSlideIndex,
                                  m_currentSlideName, m_previousSlideName);

        // If user has set current slide before viewer app has been set for the first time,
        // we will switch to the desired slide after we initialize.
        if (m_initialSlideIndex != 0)
            q->setCurrentSlideIndex(m_initialSlideIndex - 1);
        else if (!m_initialSlideName.isEmpty())
            q->setCurrentSlideName(m_initialSlideName);

        m_initialSlideIndex = 0;
        m_initialSlideName.clear();
    }
}

/*!
    \internal
 */
void Q3DSSceneElementPrivate::setCommandQueue(CommandQueue *queue)
{
    Q_Q(Q3DSSceneElement);

    if (queue || m_commandQueue) {
        m_currentSlideIndex = 0;
        m_currentSlideName.clear();
        m_previousSlideIndex = 0;
        m_previousSlideName.clear();
    }
    Q3DSElementPrivate::setCommandQueue(queue);

    if (m_commandQueue) {
        m_commandQueue->queueCommand(m_elementPath, CommandType_RequestSlideInfo);
        m_slideInfoRequestPending = true;
        // If user has set current slide before the queue has been set for the first time,
        // we will switch to the desired slide after we initialize.
        if (m_initialSlideIndex != 0)
            q->setCurrentSlideIndex(m_initialSlideIndex - 1);
        else if (!m_initialSlideName.isEmpty())
            q->setCurrentSlideName(m_initialSlideName);

        m_initialSlideIndex = 0;
        m_initialSlideName.clear();
    }
}

/*!
    \internal
 */
void Q3DSSceneElementPrivate::requestResponseHandler(CommandType commandType, void *requestData)
{
    switch (commandType) {
    case CommandType_RequestSlideInfo: {
        QVariantList *response = reinterpret_cast<QVariantList *>(requestData);
        if (m_slideInfoRequestPending) {
            m_slideInfoRequestPending = false;

            m_previousSlideIndex = response->at(1).toInt();
            m_previousSlideName = response->at(3).toString();

            handleSlideEntered(response->at(0).toInt(), response->at(2).toString());
        }
        delete response;
        break;
    }
    default:
        Q3DSElementPrivate::requestResponseHandler(commandType, requestData);
        break;
    }
}

QT_END_NAMESPACE
