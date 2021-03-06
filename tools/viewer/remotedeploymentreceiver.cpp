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

#include "remotedeploymentreceiver.h"
#include "viewer.h"

#include <QtNetwork>

RemoteDeploymentReceiver::RemoteDeploymentReceiver(int serverPort, QObject *parent)
    : QObject(parent)
    , m_tcpServer(0)
    , m_connection(0)
    , m_temporaryDir(0)
    , m_serverPort(serverPort)
    , m_projectDeployed(false)
{
    m_incoming.setVersion(QDataStream::Qt_5_8);
}

RemoteDeploymentReceiver::~RemoteDeploymentReceiver()
{
    delete m_temporaryDir;
    m_temporaryDir = 0;
}

void RemoteDeploymentReceiver::setPort(int value)
{
    m_serverPort = value;
}

QString RemoteDeploymentReceiver::startServer()
{
    if (m_tcpServer)
        return QString();

    m_tcpServer = new QTcpServer(this);
    if (!m_tcpServer->listen(QHostAddress::Any, m_serverPort)) {
        QString error = tr("Can't start the remote connection: '%1'")
                .arg(m_tcpServer->errorString());
        delete m_tcpServer;
        m_tcpServer = 0;
        return error;
    }

    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost
                && ipAddressesList.at(i).toIPv4Address()) {
            m_hostAddress = ipAddressesList.at(i);
            break;
        }
    }

    // if we did not find one, use IPv4 localhost
    if (m_hostAddress.isNull())
        m_hostAddress = QHostAddress(QHostAddress::LocalHost);

    m_serverPort = m_tcpServer->serverPort();
    connect(m_tcpServer, SIGNAL(newConnection()),
            this, SLOT(acceptRemoteConnection()));
    return QString();
}

void RemoteDeploymentReceiver::disconnectRemote()
{
    if (m_connection)
        m_connection->disconnectFromHost();
}

void RemoteDeploymentReceiver::acceptRemoteConnection()
{
    Q_ASSERT(m_tcpServer);
    Q_ASSERT(!m_connection);
    m_connection = m_tcpServer->nextPendingConnection();
    Q_ASSERT(m_connection);

    m_incoming.setDevice(m_connection);

    connect(m_connection, &QTcpSocket::disconnected,
        this, &RemoteDeploymentReceiver::acceptRemoteDisconnection);

    connect(m_connection, &QTcpSocket::readyRead,
        this, &RemoteDeploymentReceiver::readProject);

    Q_EMIT(remoteConnected());
}

void RemoteDeploymentReceiver::acceptRemoteDisconnection()
{
    Q_ASSERT(m_tcpServer);
    Q_ASSERT(m_connection);
    m_connection->deleteLater();
    m_connection = 0;

    m_incoming.setDevice(0);

    Q_EMIT(remoteDisconnected());
}

void RemoteDeploymentReceiver::readProject()
{
    m_projectDeployed = false;
    Q_EMIT(projectChanging());

    m_incoming.startTransaction();

    int totalBytes = 0;
    m_incoming >> totalBytes;

    Viewer *viewer = qobject_cast<Viewer *>(parent());
    if (viewer && totalBytes != 0) {
        viewer->updateProgress(
                    100 * ((double)m_connection->bytesAvailable() / (double)totalBytes));
    }

    if (m_connection->bytesAvailable() < totalBytes) {
        m_incoming.rollbackTransaction();
        return;
    }

    int numberOfFiles = 0;
    QString projectFile;
    m_incoming >> numberOfFiles;
    m_incoming >> projectFile;

    QVector<QPair<QString, QByteArray> > files;
    for (int i = 0; i < numberOfFiles; ++i) {
        QString fileName;
        QByteArray fileContents;
        m_incoming >> fileName;
        m_incoming >> fileContents;
        files.append(qMakePair(fileName, fileContents));
    }

    if (!m_incoming.commitTransaction()) {
        m_incoming.abortTransaction();
        qWarning() << "Error transferring remote project in one payload";
        return;
    }

    QFileInfo currentProject(m_projectFile);
    if (projectFile != currentProject.fileName()) {
        delete m_temporaryDir;
        m_temporaryDir = 0;
    }

    if (!m_temporaryDir)
        m_temporaryDir = new QTemporaryDir;

    Q_ASSERT(m_temporaryDir->isValid());

    for (const auto &file : qAsConst(files)) {
        QString filePath = m_temporaryDir->path() + QDir::separator() + file.first;
        QFile tmpFile(filePath);
        QDir tmpFileDir = QFileInfo(tmpFile).absoluteDir();
        if (!tmpFileDir.exists())
            tmpFileDir.mkpath(".");
        if (!tmpFile.open(QIODevice::WriteOnly)) {
            delete m_temporaryDir;
            m_temporaryDir = 0;
            qWarning() << "Error opening temporary file for remote project:"
                       << filePath;
            return;
        }

        if (file.first == projectFile)
            m_projectFile = filePath;

        tmpFile.write(file.second);
        tmpFile.close();
    }

    m_projectDeployed = true;
    Q_EMIT(projectChanged());
}
