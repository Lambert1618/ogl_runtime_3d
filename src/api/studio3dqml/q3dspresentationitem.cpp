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

#include "q3dspresentationitem_p.h"

#include <QtStudio3D/q3dssceneelement.h>
#include <QtStudio3D/q3dsdatainput.h>
#include <QtStudio3D/private/q3dspresentation_p.h>
#include <QtStudio3D/private/viewerqmlstreamproxy_p.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Presentation
    \instantiates Q3DSPresentationItem
    \inqmlmodule QtStudio3D
    \ingroup OpenGLRuntime
    \inherits Q3DSPresentation
    \keyword Studio3D

    \brief Represents a Qt 3D Studio presentation.

    This item provides properties and methods for controlling a
    presentation.

    Qt 3D Studio supports multiple presentations in one project. There
    is always a main presentation and zero or more
    sub-presentations. The sub-presentations are composed into the
    main presentations either as contents of Qt 3D Studio layers or as
    texture maps.

    In the filesystem each presentation corresponds to one \c{.uip}
    presentation file. When present, the \c{.uia} project file ties
    these together by specifying a name for each of the
    (sub-)presentations and specifies which one is the main one.

    The \c{.uia} project also defines \l{DataInput}s and
    \l{DataOutput}s that are exported by the presentations.
    \l{DataInput}s provide a way to provide input to the presentation
    to e.g. control a timeline of a subpresentation from code.
    \l{DataOutput}s provide a way to get notified when an attribute
    is changed in the presentation by animation timeline,
    by behavior scripts or by a \l{DataInput}.

    The Presentation type handles child objects of the types \l Element, \l
    SceneElement, \l DataInput, and \l SubPresentationSettings specially. These
    will get automatically associated with the presentation and can control
    certain aspects of it from that point on.

    From the API point of view Presentation corresponds to the
    main presentation. The source property can refer either to a
    \c{.uia} or \c{.uip} file. When specifying a file with \c{.uip}
    extension and a \c{.uia} is present with the same name, the
    \c{.uia} is loaded automatically and thus sub-presentation
    information is available regardless.
 */
Q3DSPresentationItem::Q3DSPresentationItem(QObject *parent)
    : Q3DSPresentation(parent)
    , m_subPresentationSettings(nullptr)
{
}

Q3DSPresentationItem::~Q3DSPresentationItem()
{
}

/*!
    Returns the \l SubPresentationSettings associated with this presentation.
*/
Q3DSSubPresentationSettings *Q3DSPresentationItem::subPresentationSettings() const
{
    return m_subPresentationSettings;
}

QQmlListProperty<QObject> Q3DSPresentationItem::qmlChildren()
{
    return QQmlListProperty<QObject>(this, nullptr, &appendQmlChildren, nullptr, nullptr, nullptr);
}

void Q3DSPresentationItem::appendQmlChildren(QQmlListProperty<QObject> *list, QObject *obj)
{
    auto item = qobject_cast<Q3DSPresentationItem *>(list->object);
    if (item) {
        auto scene = qobject_cast<Q3DSSceneElement *>(obj);
        if (scene) {
            if (item->registeredElement(scene->elementPath()))
                qWarning() << __FUNCTION__ << "A duplicate SceneElement defined for Presentation.";
            else
                item->registerElement(scene);
        } else {
            auto studioElement = qobject_cast<Q3DSElement *>(obj);
            if (studioElement) {
                if (item->registeredElement(studioElement->elementPath()))
                    qWarning() << __FUNCTION__ << "A duplicate Element defined for Presentation.";
                else
                    item->registerElement(studioElement);
            } else {
                auto subPresSettings = qobject_cast<Q3DSSubPresentationSettings *>(obj);
                if (subPresSettings) {
                    if (item->m_subPresentationSettings) {
                        qWarning() << __FUNCTION__
                                   << "Duplicate SubPresentationSettings defined for Presentation.";
                    } else {
                        item->m_subPresentationSettings = subPresSettings;
                        item->d_ptr->streamProxy()->setSettings(subPresSettings);
                    }
                } else {
                    auto dataInput = qobject_cast<Q3DSDataInput *>(obj);
                    if (dataInput) {
                        if (item->registeredDataInput(dataInput->name())) {
                            qWarning() << __FUNCTION__
                                       << "Duplicate DataInput defined for Presentation.";
                        } else {
                            item->registerDataInput(dataInput);
                        }
                    } else {
                        auto dataOutput = qobject_cast<Q3DSDataOutput *>(obj);
                        if (dataOutput) {
                            if (item->registeredDataOutput(dataOutput->name())) {
                                qWarning() << __FUNCTION__
                                           << "Duplicate DataOutput defined for Presentation.";
                            } else {
                                item->registerDataOutput(dataOutput);
                            }
                        }
                    }
                }
            }
        }
    }
}

QT_END_NAMESPACE
