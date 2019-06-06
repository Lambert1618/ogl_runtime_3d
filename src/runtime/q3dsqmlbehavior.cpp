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

#include "q3dsqmlbehavior.h"
#include "q3dsqmlscript.h"

namespace Q3DStudio {

Q3DSQmlBehavior::Q3DSQmlBehavior(QObject *parent)
    : QObject(parent), m_script(nullptr)
{

}

void Q3DSQmlBehavior::setScript(Q3DSQmlScript *script)
{
    m_script = script;
}

float Q3DSQmlBehavior::getDeltaTime()
{
    return m_script->getDeltaTime();
}

float Q3DSQmlBehavior::getAttribute(const QString &attribute)
{
    return m_script->getAttribute(attribute);
}

void Q3DSQmlBehavior::setAttribute(const QString &attribute, const QVariant &value)
{
    m_script->setAttribute(attribute, value);
}

void Q3DSQmlBehavior::setAttribute(const QString &handle, const QString &attribute,
                  const QVariant &value)
{
    m_script->setAttribute(handle, attribute, value);
}

void Q3DSQmlBehavior::fireEvent(const QString &event)
{
    m_script->fireEvent(event);
}

void Q3DSQmlBehavior::registerForEvent(const QString &event, const QJSValue &function)
{
    m_script->registerForEvent(event, function);
}

void Q3DSQmlBehavior::registerForEvent(const QString &handle, const QString &event,
                      const QJSValue &function)
{
    m_script->registerForEvent(handle, event, function);
}

void Q3DSQmlBehavior::unregisterForEvent(const QString &event)
{
    m_script->unregisterForEvent(event);
}

void Q3DSQmlBehavior::unregisterForEvent(const QString &handle, const QString &event)
{
    m_script->unregisterForEvent(handle, event);
}

QVector2D Q3DSQmlBehavior::getMousePosition()
{
    return m_script->getMousePosition();
}

QMatrix4x4 Q3DSQmlBehavior::calculateGlobalTransform(const QString &handle)
{
    return m_script->calculateGlobalTransform(handle);
}

QVector3D Q3DSQmlBehavior::lookAt(const QVector3D &target)
{
    return m_script->lookAt(target);
}

QVector3D Q3DSQmlBehavior::matrixToEuler(const QMatrix4x4 &matrix)
{
    return m_script->matrixToEuler(matrix);
}

QString Q3DSQmlBehavior::getParent(const QString &handle)
{
    return m_script->getParent(handle);
}

void Q3DSQmlBehavior::setDataInputValue(const QString &name, const QVariant &value)
{
    return m_script->setDataInputValue(name, value);
}

}
