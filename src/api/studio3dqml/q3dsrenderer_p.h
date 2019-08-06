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

#ifndef Q3DS_RENDERER_H
#define Q3DS_RENDERER_H

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

#include <QtGui/qopenglframebufferobject.h>
#include <QtQuick/qquickframebufferobject.h>

#include "Qt3DSViewerApp.h"
#include "q3dsstudio3d_p.h"

QT_BEGIN_NAMESPACE

class Q3DSViewerSettings;
class Q3DSPresentation;

class Q3DSRenderer : public QObject,
                     public QQuickFramebufferObject::Renderer
{
    Q_OBJECT

public:
    Q3DSRenderer(bool visibleFlag, qt3ds::Qt3DSAssetVisitor *assetVisitor,
                 QElapsedTimer *startupTimer);
    ~Q3DSRenderer();

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

    void setInitElements(bool inFlag) { m_initElements = inFlag; }
    void processCommands();

Q_SIGNALS:
    void enterSlide(const QString &elementPath, unsigned int slide, const QString &slideName);
    void exitSlide(const QString &elementPath, unsigned int slide, const QString &slideName);
    void requestResponse(const QString &elementPath, CommandType commandType, void *requestData);
    void presentationReady();
    void presentationLoaded();
    void customSignalEmitted(const QString &elNmentPath, const QString &name);
    void elementsCreated(const QStringList &elementPaths, const QString &error);
    void materialsCreated(const QStringList &materialNames, const QString &error);
    void meshesCreated(const QStringList &meshNames, const QString &error);
    void dataOutputValueUpdated(const QString &name, const QVariant &newValue);

protected:
    static void onInitHandler(void *userData);
    static void onUpdateHandler(void *userData);
    bool initializeRuntime(QOpenGLFramebufferObject *inFbo);
    void draw();
    void render() override;
    void synchronize(QQuickFramebufferObject *inView) override;
    void releaseRuntime();

protected:
    bool m_visibleFlag; // Is the plugin visible? Prevents rendering hidden content.
    CommandQueue m_commands; // A list of commands retrieved by the plugin to be applied.
    bool m_initElements; // Flag set when the runtime is first ready to render.
    Q3DSViewer::Q3DSViewerApp *m_runtime; // The Qt3DS viewer that renders all content.
    QQuickWindow *m_window; // Window associated with the plugin; needed to reset OpenGL state.

    bool m_initialized; // Has the runtime and OpenGL state been initialized?
    bool m_initializationFailure; // Initialization failed, no point in trying to init again
    qt3ds::Qt3DSAssetVisitor *m_visitor;

    Q3DSViewerSettings *m_settings;
    Q3DSPresentation *m_presentation;
    QString m_error;
    QElapsedTimer *m_startupTimer;

    friend class RuntimeInitializer;
};

QT_END_NAMESPACE

#endif // Q3DS_RENDERER_H
