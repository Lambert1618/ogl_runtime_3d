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

#ifndef Q3DS_QMLSTREAM_PROXY_H
#define Q3DS_QMLSTREAM_PROXY_H

#include <QtCore/QObject>
#include <QtCore/QEvent>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtQml/QQmlEngine>

#include "q3dsqmlstreamproducer.h"
#include "q3dsqmlsubpresentationsettings.h"

class Q3DS_QMLSTREAMERSHARED_EXPORT Q3DSQmlStreamProxy : public QObject
{
    Q_OBJECT

public:
    Q3DSQmlStreamProxy(QObject *parent = nullptr);
    ~Q3DSQmlStreamProxy();

    bool event(QEvent *event) Q_DECL_OVERRIDE;
    void registerPresentation(const QString &presentationId, const QString &args);
    void unregisterPresentation(const QString &presentationId);
    void setSettings(Q3DSSubPresentationSettings *settings);
    void setPath(const QString& path);
    void setEngine(QQmlEngine *engine);

private:
    void loadPresentationItem(const QString& presentationId, const QString& presentationArgs);
    void itemLoaded();
    QUrl pathToUrl(const QString &path);

    QQmlEngine *m_engine;
    Q3DSSubPresentationSettings *m_settings;
    QMap<QString, Q3DSQmlStreamProducer *> m_streamProducers;
    struct renderitem {
        QString args;
        QString presentationId;
        QQmlComponent *component;
        QQuickItem *item;
        renderitem()
            : component(nullptr), item(nullptr) {}
        ~renderitem();
    };

    QUrl m_path;
    QVector<renderitem *> m_loadedItems;
};

#endif // Q3DS_QMLSTREAM_PROXY_H
