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

#include "q3dsqmlsubpresentationsettings.h"
#include "q3dsqmlstream.h"

/*!
    \qmltype SubPresentationSettings
    \instantiates Q3DSSubPresentationSettings
    \inqmlmodule Qt3DStudio
    \ingroup OpenGLRuntime
    \brief

    \sa Studio3D, Presentation, QmlStream
*/
/*!
    \class Q3DSSubPresentationSettings
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.0
    \brief Settings for subpresentations.
    \param parent

    \sa Q3DSPresentation, Q3DSQmlStream
 */
Q3DSSubPresentationSettings::Q3DSSubPresentationSettings(QObject *parent)
    : QObject(parent)
{
}

Q3DSSubPresentationSettings::~Q3DSSubPresentationSettings()
{
    qDeleteAll(m_list);
}

/*!
 * \qmlproperty variant SubPresentationSettings::qmlStream
 * Contains the QML streams to be used as subpresentations.
 */
/*!
 * \property Q3DSSubPresentationSettings::qmlStreams
 * Contains the QML streams to be used as subpresentations.
 */
QQmlListProperty<Q3DSQmlStream> Q3DSSubPresentationSettings::qmlStreams()
{
    return QQmlListProperty<Q3DSQmlStream>(this, m_list);
}

QList<Q3DSQmlStream *> Q3DSSubPresentationSettings::qmlStreamsList()
{
    return m_list;
}

