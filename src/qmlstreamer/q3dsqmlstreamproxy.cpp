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

#include "q3dsqmlstreamservice.h"
#include "q3dsqmlstreamproxy.h"
#include "q3dsqmlstreamproducer.h"
#include "q3dsincubationcontroller.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlIncubationController>
#include <QScreen>
#include <QFileInfo>

class Qt3DSQmlStreamEvent : public QEvent
{
public:
    Qt3DSQmlStreamEvent(const QString &id, const QString &args)
        : QEvent(QEvent::User), presentationId(id), presentationArgs(args), unregister(false)
    {

    }

    Qt3DSQmlStreamEvent(const QString &id)
        : QEvent(QEvent::User), presentationId(id), unregister(true)
    {

    }

    QString presentationId;
    QString presentationArgs;
    bool unregister;
};

Q3DSQmlStreamProxy::renderitem::~renderitem()
{
    delete item;
    delete component;
}

Q3DSQmlStreamProxy::Q3DSQmlStreamProxy(QObject *parent)
    : QObject(parent), m_engine(nullptr), m_settings(nullptr)
{
}

Q3DSQmlStreamProxy::~Q3DSQmlStreamProxy()
{
    for (Q3DSQmlStreamProducer *p : m_streamProducers) {
        IQ3DSQmlStreamService::getQmlStreamService()->unregisterProducer(p);
        delete p;
    }
}

void Q3DSQmlStreamProxy::loadPresentationItem(const QString& presentationId,
                                               const QString& presentationArgs)
{
    if (!m_engine) {
        m_engine = new QQmlEngine;

        // Add extra import path for binary installations
        QString extraImportPath(QStringLiteral("%1/qml"));
        m_engine->addImportPath(extraImportPath.arg(QGuiApplication::applicationDirPath()));

        m_engine->setIncubationController(new Q3DSIncubationController(this));
    }

    QString itemPath = presentationArgs;
    itemPath.replace("\\", "/");

    QUrl purl = QUrl(itemPath);
    if (purl.isRelative()) {
        purl.setPath(m_path.path() + QStringLiteral("/") + purl.path());
        purl.setScheme(m_path.scheme());
    } else {
        purl = pathToUrl(itemPath);
    }

    QQmlComponent *component = new QQmlComponent(m_engine, purl);
    if (!component)
        return;
    renderitem *item = new renderitem;
    item->args = itemPath;
    item->component = component;
    item->presentationId = presentationId;
    m_loadedItems.push_back(item);

    if (component->isLoading())
        connect(component, &QQmlComponent::statusChanged, this, &Q3DSQmlStreamProxy::itemLoaded);
    else
        itemLoaded();
}

void Q3DSQmlStreamProxy::itemLoaded()
{
    for (renderitem *item : m_loadedItems) {
        if (item->item == nullptr && item->component->isLoading() == false) {
            disconnect(item->component, &QQmlComponent::statusChanged,
                       this, &Q3DSQmlStreamProxy::itemLoaded);
            if (!item->component->errors().isEmpty()) {
                QList<QQmlError> errorList = item->component->errors();
                for (const QQmlError &error: errorList)
                    qWarning() << error.url() << error.line() << error;
                m_loadedItems.removeAll(item);
                delete item;
            } else {
                QObject *root = item->component->create();
                if (!root) {
                    QList<QQmlError> errorList = item->component->errors();
                    for (const QQmlError &error: errorList)
                        qWarning() << error.url() << error.line() << error;
                    m_loadedItems.removeAll(item);
                    delete item;
                } else {
                    QQuickItem *qi = qobject_cast<QQuickItem *>(root);
                    if (!qi) {
                        m_loadedItems.removeAll(item);
                        delete item;
                    } else {
                        item->item = qi;
                        m_streamProducers[item->presentationId]->setPresentationItem(qi);
                    }
                }
            }
        }
    }
}

QUrl Q3DSQmlStreamProxy::pathToUrl(const QString &path)
{
    QUrl retval;
    if (path.startsWith(QStringLiteral(":"))) {
        retval.setScheme(QStringLiteral("qrc"));
        retval.setPath(path.mid(1));
    } else if (path.startsWith(QStringLiteral("qrc:"), Qt::CaseInsensitive)) {
        retval.setScheme(QStringLiteral("qrc"));
        retval.setPath(path.mid(4));
    } else {
        retval.setScheme(QStringLiteral("file"));
        retval.setPath(path);
    }
    return retval;
}

bool Q3DSQmlStreamProxy::event(QEvent *event)
{
    if (event->type() == QEvent::User) {
        Qt3DSQmlStreamEvent *e = static_cast<Qt3DSQmlStreamEvent *>(event);
        Q3DSQmlStreamProducer *producer = nullptr;
        if (e->unregister && m_streamProducers.contains(e->presentationId)) {
            for (int i = 0; i < m_loadedItems.size(); i++) {
                if (m_loadedItems[i]->presentationId == e->presentationId) {
                    delete m_loadedItems[i];
                    m_loadedItems.remove(i);
                    break;
                }
            }
            producer = static_cast<Q3DSQmlStreamProducer *>(
                        m_streamProducers.take(e->presentationId));
            IQ3DSQmlStreamService::getQmlStreamService()->unregisterProducer(producer);
            delete producer;
            return true;
        }
        if (!m_streamProducers.contains(e->presentationId)) {
            producer = new Q3DSQmlStreamProducer(e->presentationId, e->presentationArgs, this);
            m_streamProducers[e->presentationId] = producer;
            IQ3DSQmlStreamService::getQmlStreamService()
                    ->registerProducer(producer, e->presentationId.toLatin1().data());
        } else {
            producer = static_cast<Q3DSQmlStreamProducer *>(m_streamProducers[e->presentationId]);
        }

        if (m_settings) {
            bool streamFound = false;
            for (Q3DSQmlStream *s : m_settings->qmlStreamsList()) {
                if (s->presentationId() == e->presentationId) {
                    producer->setPresentationItem(s->item());
                    streamFound = true;
                    break;
                }
            }
            if (!streamFound && !e->presentationArgs.isNull())
                loadPresentationItem(e->presentationId, e->presentationArgs);
        } else if (!e->presentationArgs.isNull()){
            loadPresentationItem(e->presentationId, e->presentationArgs);
        }
        return true;
    }
    return false;
}

void Q3DSQmlStreamProxy::registerPresentation(const QString &presentationId, const QString &args)
{
    QCoreApplication::postEvent(this,
                                new Qt3DSQmlStreamEvent(QString(presentationId), QString(args)));
}

void Q3DSQmlStreamProxy::unregisterPresentation(const QString &presentationId)
{
    QCoreApplication::postEvent(this, new Qt3DSQmlStreamEvent(QString(presentationId)));
}

void Q3DSQmlStreamProxy::setSettings(Q3DSSubPresentationSettings *settings)
{
    m_settings = settings;
}

void Q3DSQmlStreamProxy::setPath(const QString& path)
{
    QString modPath = QFileInfo(path).path(); // path() strips filename out
    m_path = pathToUrl(modPath);
}

void Q3DSQmlStreamProxy::setEngine(QQmlEngine *engine)
{
    if (m_engine)
        delete m_engine;

    m_engine = engine;

    // Add extra import path for binary installations
    QString extraImportPath(QStringLiteral("%1/qml"));
    m_engine->addImportPath(extraImportPath.arg(QGuiApplication::applicationDirPath()));

    m_engine->setIncubationController(new Q3DSIncubationController(this));
}
