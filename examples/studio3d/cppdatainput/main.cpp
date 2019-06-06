/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qstudio3dglobal.h>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <Q3DSSurfaceViewer>
#include <QWindow>
#include <QSurface>
#include <Q3DSPresentation>
#include <QOpenGLContext>
#include <Q3DSDataInput>
#include <QVector3D>
#include <QTimer>
#include <QDebug>
// Required for Ubuntu build
#include <cmath>

int main(int argc, char *argv[])
{
    float colorRed = 0.0f;
    int range = 0;

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    // Use the ideal format (i.e. OpenGL version and profile) recommended by
    // the Qt 3D Studio runtime. Without this the format set on the QQuickView
    // would be used instead.
    QSurfaceFormat::setDefaultFormat(Q3DS::surfaceFormat());

    QOpenGLContext context;
    if (!context.create())
        qFatal("Failed to create OpenGL context");

    QWindow window;
    window.setSurfaceType(QSurface::OpenGLSurface);
    window.setFormat(context.format());
    window.create();

    Q3DSSurfaceViewer viewer;
    QObject::connect(&viewer, &Q3DSSurfaceViewer::presentationLoaded, &viewer, [&] {
        qDebug() << "Presentation loaded";
    });

    QObject::connect(&viewer, &Q3DSSurfaceViewer::presentationReady, &viewer, [&] {
        qDebug() << "Presentation ready";
        const auto diList = viewer.presentation()->dataInputs();
        for (const auto &it : diList) {
            Q3DSDataInput *dynDi = it;
            qDebug() << "Processing data input " << dynDi->name();
            if (dynDi->name().contains(QLatin1String("color"))) {
                if (dynDi->isValid()) {
                    // Access metadata.
                    qDebug() << "Metadata: " << dynDi->metadata("metadata1");

                    QObject::connect(&viewer, &Q3DSSurfaceViewer::frameUpdate,
                                     [&colorRed, dynDi] {
                        dynDi->setValue(QVector3D(colorRed, 0.5, 0.5));
                        colorRed = std::fmod(colorRed + 0.001f, 1.0f);
                    });
                }
            } else if (dynDi->name().contains(QLatin1String("range"))) {
                qDebug() << "Found dynamic data input for range " << dynDi;
                if (dynDi->isValid()) {
                    QObject::connect(&viewer, &Q3DSSurfaceViewer::frameUpdate, [&range, dynDi] {
                        dynDi->setValue((float)range);
                        range = (range + 1) % 360;
                    });
                }
            }
        }
        auto diWithMetadata = viewer.presentation()->dataInputs("metadata1");
        qDebug() << "Datainputs with metadatakey metadata1: ";
        for (auto &it : diWithMetadata)
            qDebug() << it->name();
    });

    viewer.presentation()->setSource(QUrl(QStringLiteral("qrc:presentation/datainput.uia")));
    viewer.setUpdateInterval(0); // enable automatic updates

    viewer.create(&window, &context);

    window.setTitle(QStringLiteral("Qt 3D Studio Example"));
    window.resize(800, 480);
    window.show();

    return app.exec();
}
