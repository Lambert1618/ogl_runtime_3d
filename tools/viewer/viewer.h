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

#ifndef VIEWER_H
#define VIEWER_H

#include "remotedeploymentreceiver.h"

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>
#include <QtGui/qwindow.h>
#include "q3dsstudio3d_p.h"

class Viewer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ContentView contentView READ contentView WRITE setContentView NOTIFY contentViewChanged)
    Q_PROPERTY(QUrl openFolder READ openFolder WRITE setOpenFolder NOTIFY openFolderChanged)
    Q_PROPERTY(int connectPort READ connectPort WRITE setConnectPort NOTIFY connectPortChanged)
    Q_PROPERTY(QString connectText READ connectText NOTIFY connectTextChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QStringList variantList READ variantList WRITE setVariantList NOTIFY variantListChanged)

public:
    enum ContentView {
        DefaultView,
        StudioView,
        ConnectView,
        SequenceView
    };

    Q_ENUM(ContentView)

    explicit Viewer(bool generatorMode, QObject *parent = nullptr);
    ~Viewer();

    Q_INVOKABLE void connectRemote();
    Q_INVOKABLE void disconnectRemote();
    Q_INVOKABLE void loadFile(const QString &filename);
    Q_INVOKABLE QString convertUrlListToFilename(const QList<QUrl> &list);
    Q_INVOKABLE void restoreWindowState(QWindow *window);
    Q_INVOKABLE void storeWindowState(QWindow *window);

    Q_INVOKABLE void handleMousePress(int x, int y, int button, int buttons, int modifiers);
    Q_INVOKABLE void handleMouseRelease(int x, int y, int button, int buttons, int modifiers);
    Q_INVOKABLE void handleMouseMove(int x, int y, int button, int buttons, int modifiers);

    void setVariantList(const QStringList &variantList);
    QStringList variantList() const;
    void setContentView(ContentView view);
    ContentView contentView() const;
    void setOpenFolder(const QUrl &folder);
    QUrl openFolder(); // not const since it potentially updates from settings
    void setConnectPort(int port);
    int connectPort(); // not const since it potentially updates from settings
    QString connectText() const;
    bool isConnected() const;

    void setQmlRootObject(QObject *obj);

    void loadProject(const QByteArray &data);
    void updateProgress(int percent);
    void setGeneratorDetails(const QString &filename);

public Q_SLOTS:
    void generatorProgress(int totalFrames, int frameCount);
    void generatorFinished(bool success, const QString &details);

private Q_SLOTS:
    void loadRemoteDeploymentReceiver();
    void remoteProjectChanging();
    void remoteConnected();
    void remoteDisconnected();
    void resetConnectionInfoText();

Q_SIGNALS:
    void contentViewChanged();
    void openFolderChanged();
    void connectPortChanged();
    void connectTextChanged();
    void connectedChanged();
    void variantListChanged();
    void showInfoOverlay(const QString &infoStr);

private:
    Q3DSStudio3D *qmlStudio();

    QString m_openFileDir;
    QStringList m_variantList;
    RemoteDeploymentReceiver *m_remoteDeploymentReceiver = nullptr;
    bool m_generatorMode = false;
    ContentView m_contentView = DefaultView;
    QObject *m_qmlRootObject = nullptr;
    int m_connectPort = -1;
    QString m_connectText;
    Q3DSStudio3D *m_qmlStudio = nullptr;
    QTimer m_connectTextResetTimer;
};

#endif // VIEWER_H
