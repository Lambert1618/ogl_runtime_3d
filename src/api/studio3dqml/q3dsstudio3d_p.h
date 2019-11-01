/****************************************************************************
**
** Copyright (c) 2016 NVIDIA CORPORATION.
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

#ifndef Q3DS_STUDIO3D_H
#define Q3DS_STUDIO3D_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtStudio3D API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtStudio3D/private/q3dscommandqueue_p.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtQuick/qquickframebufferobject.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class Q3DSRenderer;
class Q3DSViewerSettings;
class Q3DSPresentationItem;
class QOffscreenSurface;

class Q3DSStudio3D : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(Q3DSPresentationItem *presentation READ presentation CONSTANT)
    Q_PROPERTY(Q3DSViewerSettings *viewerSettings READ viewerSettings CONSTANT)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(EventIgnoreFlags ignoredEvents READ ignoredEvents WRITE setIgnoredEvents NOTIFY ignoredEventsChanged)
    Q_PROPERTY(bool asyncInit READ asyncInit WRITE setAsyncInit NOTIFY asyncInitChanged REVISION 1)

public:
    enum EventIgnoreFlag {
        EnableAllEvents = 0,
        IgnoreMouseEvents = 0x01,
        IgnoreWheelEvents = 0x02,
        IgnoreKeyboardEvents = 0x04,
        IgnoreAllInputEvents = IgnoreMouseEvents | IgnoreWheelEvents | IgnoreKeyboardEvents
    };
    Q_DECLARE_FLAGS(EventIgnoreFlags, EventIgnoreFlag)
    Q_FLAG(EventIgnoreFlags)

    Q3DSStudio3D();
    ~Q3DSStudio3D() override;

    QQuickFramebufferObject::Renderer *createRenderer() const override;

    bool isRunning() const;
    Q3DSPresentationItem *presentation() const;
    Q3DSViewerSettings *viewerSettings() const;
    QString error() const;
    void setError(const QString &error);
    bool asyncInit() const;
    void setAsyncInit(bool enabled);

    void getCommands(bool emitInitialize, CommandQueue &renderQueue);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    EventIgnoreFlags ignoredEvents() const;
    void setIgnoredEvents(EventIgnoreFlags flags);

    void componentComplete() override;

Q_SIGNALS:
    void frameUpdate();
    void runningChanged(bool initialized);
    void errorChanged(const QString &error);
    void ignoredEventsChanged();
    void presentationReady();
    void presentationLoaded();
    void asyncInitChanged(bool enabled);

public Q_SLOTS:
    void reset();

protected Q_SLOTS:
    void handleWindowChanged(QQuickWindow *window);
    void handleVisibleChanged();
    void tick();
    void requestResponseHandler(const QString &elementPath, CommandType commandType,
                                void *requestData);
private:
    void updateEventMasks();

protected:
    Q3DSViewerSettings *m_viewerSettings;
    Q3DSPresentationItem *m_presentation;

    bool m_emitRunningChange;
    bool m_isRunning;
    EventIgnoreFlags m_eventIgnoreFlags;

    CommandQueue m_pendingCommands;
    qreal m_pixelRatio;
    QString m_error;
    QScopedPointer<QElapsedTimer> m_startupTimer;
    bool m_asyncInit = false;
    QScopedPointer<QOffscreenSurface> m_asyncInitSurface;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Q3DSStudio3D::EventIgnoreFlags)

QT_END_NAMESPACE

#endif // Q3DS_STUDIO3D_H
