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

#include "q3dsqmlstream.h"

/*!
    \qmltype QmlStream
    \instantiates Q3DSQmlStream
    \inqmlmodule Qt3DStudio
    \ingroup OpenGLRuntime
    \brief Allows streaming of QML as subpresentation.

    \sa Studio3D, Presentation, SubPresentation
*/
/*!
    \class Q3DSQmlStream
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.0
    \brief Allows streaming of QML as subpresentation.
    \param parent
 */

/*!
 * \brief Q3DSQmlStream::Q3DSQmlStream Constructor
 * \param parent Optional parent object.
 */
Q3DSQmlStream::Q3DSQmlStream(QObject *parent)
    : QObject(parent)
    , m_item(nullptr)
{
}

/*!
 * \brief Q3DSQmlStream::~Q3DSQmlStream Destructor.
 */
Q3DSQmlStream::~Q3DSQmlStream()
{
}

/*!
 * \qmlproperty string QmlStream::presentationId
 * Contains the presentation id of this subpresentation.
 */
/*!
 * \property Q3DSQmlStream::presentationId
 * Contains the presentation id of this subpresentation.
 */
QString Q3DSQmlStream::presentationId() const
{
    return m_presentationId;
}

/*!
 * \qmlproperty Item QmlStream::item
 * Contains the Item to be streamed as subpresentation.
 */
/*!
 * \property Q3DSQmlStream::item
 * Contains the QQuickItem to be streamed as subpresentation.
 */
QQuickItem *Q3DSQmlStream::item() const
{
    return m_item;
}

void Q3DSQmlStream::setPresentationId(const QString presentationId)
{
    if (m_presentationId != presentationId) {
        m_presentationId = presentationId;
        emit presentationIdChanged(presentationId);
    }
}

void Q3DSQmlStream::setItem(QQuickItem *item)
{
    if (m_item != item) {
        m_item = item;
        emit itemChanged(item);
    }
}
