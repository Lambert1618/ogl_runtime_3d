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

#include "q3dsdataoutput_p.h"
#include "q3dspresentation_p.h"
#include "q3dscommandqueue_p.h"

/*!
    \qmltype DataOutput
    \instantiates Q3DSDataOutput
    \inqmlmodule QtStudio3D.OpenGL
    \ingroup OpenGLRuntime
    \brief Provides notifications from data output entries in Qt 3D Studio presentation.

    This class is a convenience class for listening for changes in the Qt 3D Studio
    presentation attributes. DataOutput provides a clean contract between the presentation
    design and the code. It hides the presentation details from the code while providing a
    contractual access point to code to be notified when aspects of the presentation change
    (e.g. movement of an element in the presentation due to timeline animation).

    DataOutput can be attached to same attributes in the design as DataInput is, including
    presentation timeline. Only exception is slide changes. Slide changes are already notified
    through \c{Presentation::slideEntered} and \c{Presentation::slideExited} signals.

    \note There is a performance cost for each registered DataOutput, so try to avoid
    creating unnecessary DataOutputs.

    \sa Presentation, DataInput, Presentation::slideExited, Presentation::slideEntered
    \sa Presentation::customSignalEmitted
*/

/*!
    \class Q3DSDataOutput
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.4
    \brief Provides notifications from data output entries in Qt 3D Studio presentation.

    This class is a convenience class for listening for changes in the Qt 3D Studio
    presentation attributes. DataOutput provides a clean contract between the presentation
    design and the code. It hides the presentation details from the code while providing a
    contractual access point to code to be notified when aspects of the presentation change
    (e.g. movement of an element in the presentation due to timeline animation).

    DataOutput can be attached to same attributes in the design as DataInput is, including
    presentation timeline. Only exception is slide changes (slide changes are already notified
    through \c{Q3DSPresentation::slideEntered} and \c{Q3DSPresentation::slideExited} signals).

    \note There is a performance cost for each registered DataOutput, so try to avoid
    creating unnecessary DataOutputs.

    For other integration points between code and presentation see:
    \sa Q3DSPresentation::customSignalEmitted
    \sa Q3DSPresentation::slideEntered
    \sa Q3DSPresentation::slideExited
    \sa Q3DSDataInput

    \sa Q3DSPresentation
*/
Q3DSDataOutput::Q3DSDataOutput(QObject *parent)
  : QObject(parent)
  , d_ptr(new Q3DSDataOutputPrivate(this))
{

}

Q3DSDataOutput::Q3DSDataOutput(const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSDataOutputPrivate(this))
{
    d_ptr->m_name = name;
}

Q3DSDataOutput::~Q3DSDataOutput()
{
    delete d_ptr;
}

/*!
    \qmlproperty string DataOutput::name

    Specifies the name of the observed data output element in the
    presentation. The name must match a name of a data output defined
    in the presentation. This property must be set before setting the value
    property.
 */

/*!
    \property Q3DSDataOutput::name

    Specifies the name of the observed data output element in the
    presentation. The name must match a name of a data output defined
    in the presentation.

    This property must be set before setting the value property.
    The initial value is provided via the constructor, but the name
    can also be changed later on.
 */
QString Q3DSDataOutput::name() const
{
    return d_ptr->m_name;
}

void Q3DSDataOutput::setName(const QString &name)
{
    if (d_ptr->m_name != name) {
        d_ptr->m_name = name;
        if (d_ptr->m_presentation)
            d_ptr->m_presentation->registerDataOutput(this);
        Q_EMIT nameChanged(name);
    }
}

/*!
    \qmlproperty var DataOutput::value
    \readonly

    Contains the read-only value of the controlled data output element in the
    presentation.

    The value of this property accounts for actual value in the last processed
    frame of the presentation. This includes animation timeline driven changes,
    changes done via DataInput and changes done via Behavior scripts.
*/

/*!
    \property Q3DSDataOutput::value

    Contains the read-only value of the controlled data output element in the
    presentation.

    The value of this property accounts for actual value in the last processed
    frame of the presentation. This includes animation timeline driven changes,
    changes done via DataInput and changes done via Behavior scripts.
*/
QVariant Q3DSDataOutput::value() const
{
    return d_ptr->m_value;
}

/*!
    \qmlsignal DataOutput::valueChanged(var newValue)

    Emitted when the value of the observed DataOutput has changed to
    \a newValue in the presentation.
 */

/*!
    \fn Q3DSDataOutput::valueChanged(const QVariant &newValue)

    Emitted when the value of the observed DataOutput has changed to
    \a newValue in the presentation.
 */

/*!
 * \internal
 */
void Q3DSDataOutput::setValue(const QVariant &value)
{
    if (d_ptr->m_value == value)
        return;

    d_ptr->m_value = value;
    Q_EMIT valueChanged(value);
}

void Q3DSDataOutputPrivate::setPresentation(Q3DSPresentation *presentation)
{
    m_presentation = presentation;
}

Q3DSDataOutputPrivate::Q3DSDataOutputPrivate(Q3DSDataOutput *parent)
    : q_ptr(parent)
{
}

Q3DSDataOutputPrivate::~Q3DSDataOutputPrivate()
{
    if (m_presentation)
        m_presentation->unregisterDataOutput(q_ptr);
}

void Q3DSDataOutputPrivate::setValue(const QVariant &value)
{
    m_value = value;
    Q_EMIT q_ptr->valueChanged(value);
}

void Q3DSDataOutputPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    m_viewerApp = app;

    if (m_viewerApp && m_value.isValid())
        setValue(m_value);
}

void Q3DSDataOutputPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;

    if (m_commandQueue && m_value.isValid())
        setValue(m_value);
}
