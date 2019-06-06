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

#ifndef Q3DSQMLBEHAVIOR_H
#define Q3DSQMLBEHAVIOR_H

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtQml/qjsvalue.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qmatrix4x4.h>

namespace Q3DStudio {

class Q3DSQmlScript;

class Q3DSQmlBehavior : public QObject
{
    Q_OBJECT
public:
    Q3DSQmlBehavior(QObject *parent = nullptr);

    void setScript(Q3DSQmlScript *script);

    Q_INVOKABLE float getDeltaTime();
    Q_INVOKABLE float getAttribute(const QString &attribute);
    Q_INVOKABLE void setAttribute(const QString &attribute, const QVariant &value);
    Q_INVOKABLE void setAttribute(const QString &handle, const QString &attribute,
                                  const QVariant &value);
    Q_INVOKABLE void fireEvent(const QString &event);
    Q_INVOKABLE void registerForEvent(const QString &event, const QJSValue &function);
    Q_INVOKABLE void registerForEvent(const QString &handle, const QString &event,
                                      const QJSValue &function);
    Q_INVOKABLE void unregisterForEvent(const QString &event);
    Q_INVOKABLE void unregisterForEvent(const QString &handle, const QString &event);
    Q_INVOKABLE QVector2D getMousePosition();
    Q_INVOKABLE QMatrix4x4 calculateGlobalTransform(const QString &handle = QString());
    Q_INVOKABLE QVector3D lookAt(const QVector3D &target);
    Q_INVOKABLE QVector3D matrixToEuler(const QMatrix4x4 &matrix);
    Q_INVOKABLE QString getParent(const QString &handle = QString());
    Q_REVISION(1) Q_INVOKABLE void setDataInputValue(const QString &name, const QVariant &value);


Q_SIGNALS:
    void initialize();
    void activate();
    void update();
    void deactivate();

private:
    Q3DSQmlScript *m_script;
};

}

#endif
