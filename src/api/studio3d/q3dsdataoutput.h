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

#ifndef Q3DSDATAOUTPUT_H
#define Q3DSDATAOUTPUT_H

#include <QtStudio3D/qstudio3dglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class Q3DSDataOutputPrivate;
class Q3DSPresentation;

class Q_STUDIO3D_EXPORT Q3DSDataOutput : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DSDataOutput)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QVariant value READ value NOTIFY valueChanged)

public:

    explicit Q3DSDataOutput(QObject *parent = nullptr);
    explicit Q3DSDataOutput(const QString &name, QObject *parent = nullptr);
    explicit Q3DSDataOutput(Q3DSPresentation *presentation, const QString &name,
                            QObject *parent = nullptr);
    virtual ~Q3DSDataOutput();

    QString name() const;
    QVariant value() const;

public Q_SLOTS:
    void setName(const QString &name);

Q_SIGNALS:
    void nameChanged(const QString &newName);
    void valueChanged(const QVariant &newValue);

protected:
    explicit Q3DSDataOutput(Q3DSDataOutputPrivate *d, Q3DSPresentation *presentation,
                            const QString &name, QObject *parent = nullptr);
    Q3DSDataOutputPrivate *d_ptr;

private:
    Q_DISABLE_COPY(Q3DSDataOutput)
    void setValue(const QVariant &newValue);
    friend class Q3DSPresentationPrivate;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(Q3DSDataOutput*)

#endif // Q3DSDATAOUTPUT_H
