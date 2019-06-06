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

#ifndef Q3DS_QMLSTREAM_H
#define Q3DS_QMLSTREAM_H

#include <QObject>

#include "q3dsqmlstreamer_global.h"

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

class Q3DS_QMLSTREAMERSHARED_EXPORT Q3DSQmlStream : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString presentationId READ presentationId WRITE setPresentationId NOTIFY presentationIdChanged)
    Q_PROPERTY(QQuickItem *item READ item WRITE setItem NOTIFY itemChanged)

    Q_CLASSINFO("DefaultProperty", "item")
public:
    Q3DSQmlStream(QObject *parent = nullptr);
    ~Q3DSQmlStream();

    QString presentationId() const;
    QQuickItem *item() const;

public Q_SLOTS:
    void setPresentationId(const QString presentationId);
    void setItem(QQuickItem *item);

Q_SIGNALS:
    void presentationIdChanged(const QString presentationId);
    void itemChanged(QQuickItem *item);

private:
    QString m_presentationId;
    QQuickItem *m_item;
};

#endif // Q3DSQMLSTREAM_H
