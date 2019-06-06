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

#ifndef Q3DSQMLSCRIPT_H
#define Q3DSQMLSCRIPT_H

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtQml/qjsvalue.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qmatrix4x4.h>

#include "Qt3DSTypes.h"
#include "Qt3DSKernelTypes.h"
#include "Qt3DSEvent.h"
#include "q3dsqmlbehavior.h"
#include "Qt3DSApplication.h"

namespace Q3DStudio {
class CQmlEngine;

class Q3DSQmlScript : public QObject
{
    Q_OBJECT
public:
    Q3DSQmlScript(CQmlEngine &api, Q3DSQmlBehavior &object, TElement &element, TElement &parent);
    ~Q3DSQmlScript();

    void update();
    void call(const QString &function);
    void updateProperties();
    bool hasBehavior(const TElement *behavior);

    float getDeltaTime();
    float getAttribute(const QString &attribute);
    void setAttribute(const QString &attribute, const QVariant &value);
    void setAttribute(const QString &handle, const QString &attribute,
                      const QVariant &value);
    void fireEvent(const QString &event);
    void registerForEvent(const QString &event, const QJSValue &function);
    void registerForEvent(const QString &handle, const QString &event,
                          const QJSValue &function);
    void unregisterForEvent(const QString &event);
    void unregisterForEvent(const QString &handle, const QString &event);
    QVector2D getMousePosition();
    QMatrix4x4 calculateGlobalTransform(const QString &handle);
    QVector3D lookAt(const QVector3D &target);
    QVector3D matrixToEuler(const QMatrix4x4 &matrix);
    QString getParent(const QString &handle);
    void setDataInputValue(const QString &name, const QVariant &value,
                           qt3ds::runtime::DataInputValueRole valueRole
                           = qt3ds::runtime::DataInputValueRole::Value);

    struct EventData {
        QJSValue function;
    };

private:
    TElement *getElementByPath(const QString &path);

    CQmlEngine &m_api;
    Q3DSQmlBehavior &m_object;
    TElement &m_behavior;
    TElement &m_owner;

    bool m_initialized;
    bool m_lastActivationState;

    float m_deltaTime;
    TTimeUnit m_lastTime;
    struct EventCallbackInfo {
        TElement *element;
        TEventCommandHash eventHash;
        EventData *data;
    };

    QVector<std::function<void()>> m_mappedProperties;
    QVector<EventCallbackInfo> m_eventCallbacks;
};
}

#endif // Q3DSQMLSCRIPT_H
