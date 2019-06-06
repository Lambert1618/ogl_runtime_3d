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

#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtCore/qfileinfo.h>
#include <QtStudio3D/private/q3dsviewersettings_p.h>
#include <QtStudio3D/private/q3dspresentation_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtQuick/qquickwindow.h>

#include "viewer.h"
#include "q3dspresentationitem_p.h"

Viewer::Viewer(bool generatorMode, QObject *parent)
    : QObject(parent)
    , m_generatorMode(generatorMode)
{
    if (m_generatorMode)
        setContentView(SequenceView);

    m_connectTextResetTimer.setInterval(5000);
    m_connectTextResetTimer.setSingleShot(true);
    connect(&m_connectTextResetTimer, &QTimer::timeout, this, &Viewer::resetConnectionInfoText);
}

Viewer::~Viewer()
{
}

void Viewer::connectRemote()
{
    if (m_remoteDeploymentReceiver) {
        delete m_remoteDeploymentReceiver;
        m_remoteDeploymentReceiver = 0;
        Q_EMIT connectedChanged();
    }

    m_remoteDeploymentReceiver = new RemoteDeploymentReceiver(m_connectPort, this);
    QString error = m_remoteDeploymentReceiver->startServer();
    if (!error.isEmpty()) {
        delete m_remoteDeploymentReceiver;
        m_remoteDeploymentReceiver = nullptr;
        setContentView(DefaultView);
        m_qmlRootObject->setProperty("error", QVariant(error));
        return;
    }

    resetConnectionInfoText();

    connect(m_remoteDeploymentReceiver, &RemoteDeploymentReceiver::remoteConnected,
            this, &Viewer::remoteConnected);

    connect(m_remoteDeploymentReceiver, &RemoteDeploymentReceiver::remoteDisconnected,
            this, &Viewer::remoteDisconnected);

    connect(m_remoteDeploymentReceiver, &RemoteDeploymentReceiver::projectChanging,
            this, &Viewer::remoteProjectChanging);

    connect(m_remoteDeploymentReceiver, &RemoteDeploymentReceiver::projectChanged,
            this, &Viewer::loadRemoteDeploymentReceiver);

    Q_EMIT connectedChanged();
}

void Viewer::disconnectRemote()
{
    m_remoteDeploymentReceiver->disconnectRemote();
}

void Viewer::setVariantList(const QStringList &variantList)
{
    if (m_variantList != variantList) {
        m_variantList = variantList;
        Q_EMIT variantListChanged();
    }
}

QStringList Viewer::variantList() const
{
    return m_variantList;
}

// Used to load files via command line and when using remote deployment
void Viewer::loadFile(const QString &filename)
{
    QString targetFilename = filename;
    // Try to find the application (*.uia) file for loading instead of the presentation (*.uip)
    // in case we are connected to remote sender.
    if (isConnected() && targetFilename.endsWith(QStringLiteral(".uip"))) {
        targetFilename.chop(4);
        targetFilename.append(QStringLiteral(".uia"));
        QFileInfo targetfileInfo(targetFilename);
        // uia not found, revert to given uip
        if (!targetfileInfo.exists())
            targetFilename = filename;
    }

    QFileInfo fileInfo(targetFilename);
    if (!fileInfo.exists()) {
        setContentView(DefaultView);
        m_qmlRootObject->setProperty(
                    "error", QVariant(tr("Tried to load nonexistent file %1").arg(targetFilename)));
        return;
    }

    QUrl sourceUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath());

    setContentView(StudioView);

    if (qmlStudio()) {
        qmlStudio()->presentation()->setVariantList(m_variantList);
        qmlStudio()->presentation()->setSource(sourceUrl);
    }
}

QString Viewer::convertUrlListToFilename(const QList<QUrl> &list)
{
    for (const QUrl &url : list) {
        QString str = url.toLocalFile();
        if (!str.isEmpty()) {
            if (QFileInfo(str).suffix() == QStringLiteral("uip")
                    || QFileInfo(str).suffix() == QStringLiteral("uia")) {
                return str;
            }
        }
    }
    return QString();
}

void Viewer::restoreWindowState(QWindow *window)
{
    QSettings settings;
    QRect geo = settings.value(QStringLiteral("WindowGeometry"), QRect()).toRect();
    int visibility = settings.value(QStringLiteral("WindowVisibility"),
                                    QWindow::Windowed).toInt();

    // Do not restore geometry if resulting geometry means the center of the window
    // would be offscreen on the virtual desktop
    QRect vgeo = window->screen()->availableVirtualGeometry();
    QPoint center(geo.x() + geo.width() / 2, geo.y() + geo.height() / 2);
    bool offscreen = center.x() > vgeo.width() || center.x() < 0
            || center.y() > vgeo.height() || center.y() < 0;

    if (!offscreen && !geo.isNull()) {
        // The first geometry set at startup may adjust the geometry according to pixel
        // ratio if mouse cursor is on different screen than where window goes and the
        // two screens have different pixel ratios. Setting geometry twice seems to
        // work around this.
        geo.adjust(0, 0, -1, 0);
        window->setGeometry(geo);
        geo.adjust(0, 0, 1, 0);
        window->setGeometry(geo);
    }

    window->setVisibility(QWindow::Visibility(visibility));
}

void Viewer::storeWindowState(QWindow *window)
{
    QSettings settings;
    settings.setValue(QStringLiteral("WindowGeometry"), window->geometry());
    settings.setValue(QStringLiteral("WindowState"), window->visibility());
}

// Since we need mouse events for handling the swipe gesture in viewer, we need to generate
// a fake event for the presentation.
void Viewer::handleMousePress(int x, int y, int button, int buttons, int modifiers)
{
    if (qmlStudio()) {
        QMouseEvent fakeEvent(QEvent::MouseButtonPress,
                              QPointF(x, y) * qmlStudio()->window()->devicePixelRatio(),
                              Qt::MouseButton(button),
                              Qt::MouseButtons(buttons),
                              Qt::KeyboardModifiers(modifiers));
        qmlStudio()->presentation()->mousePressEvent(&fakeEvent);
    }
}

void Viewer::handleMouseRelease(int x, int y, int button, int buttons, int modifiers)
{
    if (qmlStudio()) {
        QMouseEvent fakeEvent(QEvent::MouseButtonRelease,
                              QPointF(x, y) * qmlStudio()->window()->devicePixelRatio(),
                              Qt::MouseButton(button),
                              Qt::MouseButtons(buttons),
                              Qt::KeyboardModifiers(modifiers));
        qmlStudio()->presentation()->mouseReleaseEvent(&fakeEvent);
    }
}

void Viewer::handleMouseMove(int x, int y, int button, int buttons, int modifiers)
{
    if (qmlStudio()) {
        QMouseEvent fakeEvent(QEvent::MouseMove,
                              QPointF(x, y) * qmlStudio()->window()->devicePixelRatio(),
                              Qt::MouseButton(button),
                              Qt::MouseButtons(buttons),
                              Qt::KeyboardModifiers(modifiers));
        qmlStudio()->presentation()->mouseMoveEvent(&fakeEvent);
    }
}

void Viewer::setContentView(Viewer::ContentView view)
{
    if (view != m_contentView) {
        m_qmlStudio = nullptr;
        m_contentView = view;
        Q_EMIT contentViewChanged();
    }
}

Viewer::ContentView Viewer::contentView() const
{
    return m_contentView;
}

void Viewer::setOpenFolder(const QUrl &folder)
{
    QString localFolder = folder.toLocalFile();
    QFileInfo fi(localFolder);
    QString newDir;
    if (fi.isDir())
        newDir = fi.absoluteFilePath();
    else
        newDir = fi.absolutePath();
    if (newDir != m_openFileDir) {
        m_openFileDir = newDir;
        QSettings settings;
        settings.setValue(QStringLiteral("DirectoryOfLastOpen"), m_openFileDir);
        Q_EMIT openFolderChanged();
    }
}

QUrl Viewer::openFolder()
{
    if (m_openFileDir.size() == 0) {
        QSettings settings;
        m_openFileDir = settings.value(QStringLiteral("DirectoryOfLastOpen"),
                                       QString("")).toString();
#ifdef Q_OS_ANDROID
        if (m_openFileDir.isEmpty())
            m_openFileDir = QStringLiteral("/sdcard/qt3dviewer"); // Add default folder for Android
#endif
    }
    return QUrl::fromLocalFile(m_openFileDir);
}

void Viewer::setConnectPort(int port)
{
    if (m_connectPort != port) {
        QSettings settings;
        m_connectPort = port;
        settings.setValue(QStringLiteral("ConnectPort"), m_connectPort);
        Q_EMIT connectPortChanged();
    }
}

int Viewer::connectPort()
{
    if (m_connectPort < 0) {
        QSettings settings;
        m_connectPort = settings.value(QStringLiteral("ConnectPort"), 36000).toInt();
    }
    return m_connectPort;
}

QString Viewer::connectText() const
{
    return m_connectText;
}

bool Viewer::isConnected() const
{
    return m_remoteDeploymentReceiver ? m_remoteDeploymentReceiver->isConnected() : false;
}

void Viewer::setQmlRootObject(QObject *obj)
{
    m_qmlRootObject = obj;
}

void Viewer::loadRemoteDeploymentReceiver()
{
    Q_ASSERT(m_remoteDeploymentReceiver);
    const QString remote = m_remoteDeploymentReceiver->fileName();
    QMetaObject::invokeMethod(this, "loadFile", Qt::QueuedConnection, Q_ARG(QString, remote));
}

void Viewer::remoteProjectChanging()
{
    if (m_contentView != ConnectView)
        setContentView(ConnectView);
    m_connectText = tr("Loading remote project...");
    Q_EMIT connectTextChanged();
}

void Viewer::remoteConnected()
{
    m_connectText = tr("Remote Connected");
    Q_EMIT connectTextChanged();
    Q_EMIT connectedChanged();
    if (m_contentView != ConnectView)
        Q_EMIT showInfoOverlay(m_connectText);
}

void Viewer::remoteDisconnected()
{
    m_connectText = tr("Remote Disconnected");
    Q_EMIT connectTextChanged();
    Q_EMIT connectedChanged();
    if (m_contentView != ConnectView) {
        Q_EMIT showInfoOverlay(m_connectText);
    } else {
        // Start timer to reset connection info text
        m_connectTextResetTimer.start();
    }
}

void Viewer::resetConnectionInfoText()
{
    m_connectText.clear();
    QTextStream stream(&m_connectText);
    stream << tr("Use IP: %1 and Port: %2\n"
                 "in Qt 3D Studio Editor to connect to this viewer.\n\n"
                 "Use File/Open... to open a local presentation.")
              .arg(m_remoteDeploymentReceiver->hostAddress().toString())
              .arg(QString::number(m_connectPort));
    Q_EMIT connectTextChanged();
}

Q3DSStudio3D *Viewer::qmlStudio()
{
    if (m_contentView == StudioView) {
        if (!m_qmlStudio) {
            QObject *loadedContent = m_qmlRootObject->property("loadedContent").value<QObject *>();
            m_qmlStudio = static_cast<Q3DSStudio3D *>(loadedContent);
        }
    } else {
        m_qmlStudio = nullptr;
    }
    return m_qmlStudio;
}

void Viewer::generatorProgress(int totalFrames, int frameCount)
{
    QString progressString;
    if (frameCount >= totalFrames) {
        progressString =
                QCoreApplication::translate(
                    "main", "Image sequence generation done! (%2 frames generated)")
        .arg(totalFrames);
    } else {
        progressString =
                QCoreApplication::translate("main", "Image sequence generation progress: %1 / %2")
        .arg(frameCount).arg(totalFrames);
    }
    QObject *loadedContent = m_qmlRootObject->property("loadedContent").value<QObject *>();
    loadedContent->setProperty("mainText", progressString);
}

void Viewer::generatorFinished(bool success, const QString &details)
{
    QObject *loadedContent = m_qmlRootObject->property("loadedContent").value<QObject *>();
    if (success) {
        loadedContent->setProperty("detailsText", details);
    } else {
        QString mainString =
                QCoreApplication::translate("main", "Image sequence generation failed:");
        loadedContent->setProperty("mainText", mainString);
        loadedContent->setProperty("detailsText", details);
    }
}

void Viewer::updateProgress(int percent)
{
    // Don't wait for 100%, as it'll already start loading and text isn't updated anymore
    if (percent >= 99)
        m_connectText = tr("Loading remote project...");
    else
        m_connectText = QStringLiteral("Receiving (%1%)").arg(percent);
    Q_EMIT connectTextChanged();
}

void Viewer::setGeneratorDetails(const QString &filename)
{
    QObject *loadedContent = m_qmlRootObject->property("loadedContent").value<QObject *>();
    loadedContent->setProperty("detailsText", filename);
}
