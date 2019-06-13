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

#include "q3dselement_p.h"
#include "q3dspresentation_p.h"
#include "q3dscommandqueue_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Element
    \instantiates Q3DSElement
    \inqmlmodule QtStudio3D.OpenGL
    \ingroup OpenGLRuntime
    \brief Control type for elements in a Qt 3D Studio presentation.

    This class is provided for backwards compatibility. We recommend using
    DataInput and DataOutput APIs for contractual and clean API between
    the design and the code.

    \sa DataInput, DataOutput

    This type is a convenience for controlling the properties of a scene object
    (such as, model, material, camera, layer) in a Qt 3D Studio presentation.

    \note The functionality of Element is equivalent to
    Presentation::setAttribute() and Presentation::fireEvent().

    \sa Studio3D, SceneElement, Presentation, DataInput, DataOutput
*/

/*!
    \class Q3DSElement
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.0

    \brief Controls a scene object (element) in a Qt 3D Studio presentation.

    This class is provided for backwards compatibility. We recommend using
    DataInput and DataOutput APIs for contractual and clean API between
    the design and the code.

    This class is a convenience class for controlling the properties of a scene
    object (such as, model, material, camera, layer) in a Qt 3D Studio
    presentation.

    \sa Q3DSSurfaceViewer, Q3DSSceneElement
 */

/*!
    \internal
 */
Q3DSElement::Q3DSElement(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSElementPrivate(this))
{
}

/*!
    \internal
 */
Q3DSElement::Q3DSElement(const QString &elementPath, QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSElementPrivate(this))
{
    d_ptr->m_elementPath = elementPath;
}

/*!
    Constructs a Q3DSElement instance controlling the scene object specified by
    \a elementPath. An optional \a parent object can be specified. The
    constructed instance is automatically associated with the specified \a
    presentation. An optional \a parent object can be specified.
 */
Q3DSElement::Q3DSElement(Q3DSPresentation *presentation, const QString &elementPath,
                         QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSElementPrivate(this))
{
    d_ptr->m_elementPath = elementPath;
    if (presentation)
        presentation->registerElement(this);
}

/*!
    \internal
 */
Q3DSElement::Q3DSElement(Q3DSElementPrivate *d, Q3DSPresentation *presentation,
                         const QString &elementPath, QObject *parent)
    : QObject(parent)
    , d_ptr(d)
{
    d_ptr->m_elementPath = elementPath;
    if (presentation)
        presentation->registerElement(this);
}

/*!
    Destructor.
 */
Q3DSElement::~Q3DSElement()
{
    // Private class isn't QObject, so we need to delete it explicitly
    delete d_ptr;
}

/*!
    \qmlproperty string Element::elementPath

    Holds the element path of the presentation element.

    An element path refers to an object in the scene by name, for example,
    \c{Scene.Layer.Camera}. Here the right camera object gets chosen even if
    the scene contains other layers with the default camera names (for instance
    \c{Scene.Layer2.Camera}).

    To reference an object stored in a property of another object, the dot
    syntax can be used. The most typical example of this is changing the source
    of a texture map by changing the \c sourcepath property on the object
    selected by \c{SomeMaterial.diffusemap}.

    To access an object in a sub-presentation, prepend the name of the
    sub-presentation followed by a colon, for example,
    \c{SubPresentationOne:Scene.Layer.Camera}.
 */

/*!
    \property Q3DSElement::elementPath

    Holds the element path of the presentation element.

    An element path refers to an object in the scene by name, for example,
    \c{Scene.Layer.Camera}. Here the right camera object gets chosen even if
    the scene contains other layers with the default camera names (for instance
    \c{Scene.Layer2.Camera}).

    To reference an object stored in a property of another object, the dot
    syntax can be used. The most typical example of this is changing the source
    of a texture map by changing the \c sourcepath property on the object
    selected by \c{SomeMaterial.diffusemap}.

    To access an object in a sub-presentation, prepend the name of the
    sub-presentation followed by a colon, for example,
    \c{SubPresentationOne:Scene.Layer.Camera}.
 */
QString Q3DSElement::elementPath() const
{
    return d_ptr->m_elementPath;
}

void Q3DSElement::setElementPath(const QString &elementPath)
{
    if (d_ptr->m_elementPath != elementPath) {
        d_ptr->setElementPath(elementPath);
        Q_EMIT elementPathChanged(d_ptr->m_elementPath);
    }
}

/*!
    \qmlmethod void Element::setAttribute(string attributeName, variant value)

    Sets the \a value of an attribute (property) of the scene object specified
    by this Element instance. The \a attributeName is the \l{Attribute
    Names}{scripting name} of the attribute.
*/

/*!
    Sets the \a value of an attribute (property) of the scene object
    specified by elementPath.

    The \a attributeName is the \l{Attribute Names}{scripting name} of the attribute.
 */
void Q3DSElement::setAttribute(const QString &attributeName, const QVariant &value)
{
    if (d_ptr->m_presentation)
        d_ptr->m_presentation->q_ptr->setAttribute(d_ptr->m_elementPath, attributeName, value);
    else
        qWarning() << __FUNCTION__ << "Element is not registered to any presentation!";
}

/*!
    \qmlmethod void Element::fireEvent(string eventName)

    Dispatches an event with \a eventName on the scene object
    specified by elementPath.

    Appropriate actions created in Qt 3D Studio or callbacks registered using
    the registerForEvent() method in attached \c{behavior scripts} will be
    executed in response to the event.
*/

/*!
    Dispatches an event with \a eventName on the scene object
    specified by elementPath.

    Appropriate actions created in Qt 3D Studio or callbacks registered using
    the registerForEvent() method in attached (behavior) scripts will be
    executed in response to the event.
 */
void Q3DSElement::fireEvent(const QString &eventName)
{
    if (d_ptr->m_presentation)
        d_ptr->m_presentation->q_ptr->fireEvent(d_ptr->m_elementPath, eventName);
    else
        qWarning() << __FUNCTION__ << "Element is not registered to any presentation!";
}

Q3DSElementPrivate::Q3DSElementPrivate(Q3DSElement *parent)
    : q_ptr(parent)
    , m_viewerApp(nullptr)
    , m_commandQueue(nullptr)
    , m_presentation(nullptr)
{
}

Q3DSElementPrivate::~Q3DSElementPrivate()
{
    if (m_presentation)
        m_presentation->unregisterElement(q_ptr);
}

void Q3DSElementPrivate::setElementPath(const QString &elementPath)
{
    m_elementPath = elementPath;

    if (m_presentation)
        m_presentation->registerElement(q_ptr);
}

void Q3DSElementPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    m_viewerApp = app;
}

void Q3DSElementPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;
}

void Q3DSElementPrivate::setPresentation(Q3DSPresentationPrivate *presentation)
{
    m_presentation = presentation;
}

void Q3DSElementPrivate::requestResponseHandler(CommandType commandType, void *requestData)
{
    Q_UNUSED(commandType)
    Q_UNUSED(requestData)

    // Base element doesn't handle any request command types yet
    qWarning() << __FUNCTION__ << "Unknown command type.";
}

QT_END_NAMESPACE
