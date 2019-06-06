/****************************************************************************
**
** Copyright (C) 2013 - 2016 NVIDIA Corporation.
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

#include "viewer.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qtouchdevice.h>
#include <QtGui/qscreen.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qfile.h>
#include <QtCore/qtimer.h>
#include <QtGui/qopenglcontext.h>
#include <QtStudio3D/q3dssurfaceviewer.h>
#include <QtStudio3D/private/q3dsimagesequencegenerator_p.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>

static QSurfaceFormat findIdealGLVersion()
{
    QSurfaceFormat fmt;
    fmt.setProfile(QSurfaceFormat::CoreProfile);

    // Advanced: Try 4.3 core (so we get compute shaders for instance)
    fmt.setVersion(4, 3);
    QOpenGLContext ctx;
    ctx.setFormat(fmt);
    if (ctx.create() && ctx.format().version() >= qMakePair(4, 3))
        return fmt;

    // Basic: Stick with 3.3 for now to keep less fortunate,
    // Mesa-based systems happy
    fmt.setVersion(3, 3);
    ctx.setFormat(fmt);
    if (ctx.create())
        return fmt;

    // We tried...
    return QSurfaceFormat::defaultFormat();
}

static QSurfaceFormat findIdealGLESVersion()
{
    QSurfaceFormat fmt;

    // Advanced: Try 3.1 (so we get compute shaders for instance)
    fmt.setVersion(3, 1);
    QOpenGLContext ctx;
    ctx.setFormat(fmt);
    if (ctx.create())
        return fmt;

    // Basic: OpenGL ES 3.0 is a hard requirement at the moment since we can
    // only generate 300 es shaders, uniform buffers are mandatory.
    fmt.setVersion(3, 0);
    ctx.setFormat(fmt);
    if (ctx.create())
        return fmt;

    // We tried...
    return QSurfaceFormat::defaultFormat();
}

int main(int argc, char *argv[])
{
#if defined(Q_OS_MACOS)
    QSurfaceFormat openGLFormat;
    openGLFormat.setRenderableType(QSurfaceFormat::OpenGL);
    openGLFormat.setProfile(QSurfaceFormat::CoreProfile);
    openGLFormat.setMajorVersion(4);
    openGLFormat.setMinorVersion(1);
    openGLFormat.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(openGLFormat);
#endif

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName("The Qt Company");
    QCoreApplication::setOrganizationDomain("qt.io");
    QCoreApplication::setApplicationName("Qt 3D Viewer");

    QGuiApplication a(argc, argv);

#if !defined(Q_OS_MACOS)
    QSurfaceFormat fmt;
    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
        fmt = findIdealGLVersion();
    else
        fmt = findIdealGLESVersion();
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);
#endif

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument(
                "file",
                QCoreApplication::translate("main", "The presentation file to open."),
                QCoreApplication::translate("main", "[file]"));

    parser.addOption({"sequence",
                      QCoreApplication::translate("main",
                      "Generates an image sequence.\n"
                      "The file argument must be specified.\n"""
                      "Specifying any of the seq-* arguments\n"
                      "implies setting this option.")});
    parser.addOption({"seq-start",
                      QCoreApplication::translate("main",
                      "Start time of the sequence in\n"
                      "milliseconds.\n"
                      "The default value is 0."),
                      QCoreApplication::translate("main", "ms"), QString::number(0)});
    parser.addOption({"seq-end",
                      QCoreApplication::translate("main",
                      "End time of the sequence in\n"
                      "milliseconds.\n"
                      "The default value is 1000."),
                      QCoreApplication::translate("main", "ms"), QString::number(1000)});
    parser.addOption({"seq-fps",
                      QCoreApplication::translate("main",
                      "Frames per second for the sequence.\n"
                      "The default value is 60."),
                      QCoreApplication::translate("main", "fps"), QString::number(60)});
    parser.addOption({"seq-interval",
                      QCoreApplication::translate("main",
                      "Time interval between frames in\n"
                      "the sequence in milliseconds. The seq-fps argument is ignored"
                      "if this argument is used."),
                      QCoreApplication::translate("main", "ms"), QString::number(0)});
    parser.addOption({"seq-width",
                      QCoreApplication::translate("main",
                      "Width of the image sequence.\n"
                      "The default value is 1920."),
                      QCoreApplication::translate("main", "pixels"), QString::number(1920)});
    parser.addOption({"seq-height",
                      QCoreApplication::translate("main",
                      "Height of the image sequence.\n"
                      "The default value is 1080."),
                      QCoreApplication::translate("main", "pixels"), QString::number(1080)});
    parser.addOption({"seq-outpath",
                      QCoreApplication::translate("main",
                      "Output path of the image sequence.\n"
                      "The default value is the current directory."),
                      QCoreApplication::translate("main", "path"), QStringLiteral(".")});
    parser.addOption({"seq-outfile",
                      QCoreApplication::translate("main",
                      "Output filename base for the image\n"
                      "sequence.\n"
                      "The default value is derived from the presentation filename."),
                      QCoreApplication::translate("main", "file"), QStringLiteral("")});
    parser.addOption({"connect",
                      QCoreApplication::translate("main",
                      "If this parameter is specified, the viewer\n"
                      "is started in connection mode.\n"
                      "The default value is 36000."),
                      QCoreApplication::translate("main", "port"), QString::number(36000)});
    parser.addOption({"fullscreen",
                      QCoreApplication::translate("main",
                      "Starts the viewer in fullscreen mode.\n")});
    parser.addOption({"maximized",
                      QCoreApplication::translate("main",
                      "Starts the viewer in maximized mode.")});
    parser.addOption({"windowgeometry",
                      QCoreApplication::translate("main",
                      "Specifies the initial\n"
                      "window geometry using the X11-syntax.\n"
                      "For example: 1000x800+50+50"),
                      QCoreApplication::translate("main", "geometry"), QStringLiteral("")});
    parser.addOption({"mattecolor",
                      QCoreApplication::translate("main",
                      "Specifies custom matte color\n"
                      "using #000000 syntax.\n"
                      "For example, white matte: #ffffff"),
                      QCoreApplication::translate("main", "color"), QStringLiteral("#333333")});
    parser.addOption({"showstats",
                      QCoreApplication::translate("main",
                      "Show render statistics on screen.")});
    parser.addOption({"scalemode",
                      QCoreApplication::translate("main",
                      "Specifies scaling mode.\n"
                      "The default value is 'center'."),
                      QCoreApplication::translate("main", "center|fit|fill"),
                      QStringLiteral("center")});
    QCommandLineOption variantListOption({QStringLiteral("v"),
                                          QStringLiteral("variants")},
                                          QObject::tr("Gives list of variant groups and variants\n"
                                                      "to be loaded from the presentation.\n"
                                                      "For example VarGroupA:var1,VarGroupB:var4"),
                                         QStringLiteral("variants"));
    parser.addOption(variantListOption);

    parser.process(a);

    const QStringList files = parser.positionalArguments();
    if (files.count() > 1) {
        qWarning() << "Only one presentation file can be given.";
        parser.showHelp(-1);
    }

    bool generateSequence = parser.isSet("sequence") || parser.isSet("seq-start")
            || parser.isSet("seq-end") || parser.isSet("seq-fps")
            || parser.isSet("seq-interval") || parser.isSet("seq-width")
            || parser.isSet("seq-height") || parser.isSet("seq-outpath")
            || parser.isSet("seq-outfile");

    QStringList variantList;
    if (parser.isSet(variantListOption)) {
        QString variantOption = parser.value(variantListOption);
        variantList = variantOption.split(QLatin1Char(','),
                                          QString::SkipEmptyParts);
    }

#ifndef Q_OS_ANDROID
    Q3DSImageSequenceGenerator *generator = nullptr;
#endif
    Viewer viewer(generateSequence);

    // Figure out control size multiplier for devices using touch screens to ensure all controls
    // have minimum usable size.
    qreal sizeMultiplier = 1.0;
    const auto touchDevices = QTouchDevice::devices();
    if (touchDevices.size() > 0) {
        // Find out the actual screen logical pixel size. Typically touch devices we care about
        // only have a single screen, so we just check primary screen.
        const auto screens = QGuiApplication::screens();
        if (screens.size() > 0) {
            QScreen *screen = screens.at(0);
            qreal dpi = screen->physicalDotsPerInch() / screen->devicePixelRatio();
            sizeMultiplier = dpi / 40.0; // divider chosen empirically
        }
    }

    QQmlApplicationEngine engine;
    // Set import paths so that standalone installation works
    QString extraImportPath1(QStringLiteral("%1/qml"));
    engine.addImportPath(extraImportPath1.arg(QGuiApplication::applicationDirPath()));
#ifdef Q_OS_MACOS
    QString extraImportPath2(QStringLiteral("%1/../../../../qml"));
    engine.addImportPath(extraImportPath2.arg(QGuiApplication::applicationDirPath()));
#endif

    QQmlContext *ctx = engine.rootContext();
    ctx->setContextProperty(QStringLiteral("_menuBackgroundColor"), QColor("#404244"));
    ctx->setContextProperty(QStringLiteral("_menuSelectionColor"), QColor("#46a2da"));
    ctx->setContextProperty(QStringLiteral("_menuBorderColor"), QColor("#727476"));
    ctx->setContextProperty(QStringLiteral("_dialogBorderColor"), QColor("#404244"));
    ctx->setContextProperty(QStringLiteral("_dialogBackgroundColor"), QColor("#2e2f30"));
    ctx->setContextProperty(QStringLiteral("_dialogFieldColor"), QColor("#404244"));
    ctx->setContextProperty(QStringLiteral("_dialogFieldBorderColor"), QColor("#262829"));
    ctx->setContextProperty(QStringLiteral("_textColor"), QColor("#ffffff"));
    ctx->setContextProperty(QStringLiteral("_disabledColor"), QColor("#727476"));
    ctx->setContextProperty(QStringLiteral("_fontSize"), int(12 * sizeMultiplier));
    ctx->setContextProperty(QStringLiteral("_controlBaseHeight"), int(24 * sizeMultiplier));
    ctx->setContextProperty(QStringLiteral("_controlBaseWidth"), int(80 * sizeMultiplier));
    ctx->setContextProperty(QStringLiteral("_controlPadding"), int(12 * sizeMultiplier));
    ctx->setContextProperty(QStringLiteral("_viewerHelper"), &viewer);
    qmlRegisterUncreatableType<Viewer>(
                "Qt3DStudioViewer", 1, 0, "ViewerHelper",
                QCoreApplication::translate("main",
                                            "Creation of ViewerHelper not allowed from QML"));
    engine.load(QUrl(QLatin1String("qrc:/qml/main.qml")));
    Q_ASSERT(engine.rootObjects().size() > 0);
    QWindow *appWindow = qobject_cast<QWindow *>(engine.rootObjects().at(0));
    Q_ASSERT(appWindow);
    viewer.setQmlRootObject(appWindow);

    if (parser.isSet(QStringLiteral("windowgeometry"))) {
        int width = 1280;
        int height = 768;
        int x = 50;
        int y = 50;
        QString geometryStr = parser.value(QStringLiteral("windowgeometry"));
        const QStringList splitPlus = geometryStr.split(QLatin1Char('+'));
        if (splitPlus.size() > 0) {
            const QStringList splitX = splitPlus[0].split(QLatin1Char('x'));
            if (splitX.size() >= 2) {
                width = splitX[0].toInt();
                height = splitX[1].toInt();
            }
            if (splitPlus.size() >= 3) {
                x = splitPlus[1].toInt();
                y = splitPlus[2].toInt();
            }
        }
        appWindow->setGeometry(x, y, width, height);
    }
    if (parser.isSet(QStringLiteral("fullscreen")))
        appWindow->setVisibility(QWindow::FullScreen);
    else if (parser.isSet(QStringLiteral("maximized")))
        appWindow->setVisibility(QWindow::Maximized);

    if (parser.isSet(QStringLiteral("mattecolor"))) {
        QColor matteColor(parser.value("mattecolor"));
        if (matteColor != Qt::black) {
            appWindow->setProperty("showMatteColor", QVariant::fromValue<QColor>(matteColor));
            appWindow->setProperty("matteColor", QVariant::fromValue<QColor>(matteColor));
        }
    }
    if (parser.isSet(QStringLiteral("showstats")))
        appWindow->setProperty("showRenderStats", true);
    if (parser.isSet(QStringLiteral("scalemode"))) {
        QString scaleStr(parser.value("scalemode"));
        if (scaleStr == QStringLiteral("fit"))
            appWindow->setProperty("scaleMode", Q3DSViewerSettings::ScaleModeFit);
        else if (scaleStr == QStringLiteral("fill"))
            appWindow->setProperty("scaleMode", Q3DSViewerSettings::ScaleModeFill);
        else
            appWindow->setProperty("scaleMode", Q3DSViewerSettings::ScaleModeCenter);
    }

    viewer.setVariantList(variantList);

#ifndef Q_OS_ANDROID
    if (generateSequence) {
        if (files.count() != 1) {
            qWarning() << "Presentation file is required for generating an image sequence.";
            parser.showHelp(-1);
        }
        generator = new Q3DSImageSequenceGenerator;
        QObject::connect(generator, &Q3DSImageSequenceGenerator::progress,
                         &viewer, &Viewer::generatorProgress);
        QObject::connect(generator, &Q3DSImageSequenceGenerator::finished,
                         &viewer, &Viewer::generatorFinished);
        viewer.setGeneratorDetails(files.first());
        generator->generateImageSequence(
                    files.first(),
                    parser.value("seq-start").toDouble(),
                    parser.value("seq-end").toDouble(),
                    parser.value("seq-fps").toDouble(),
                    parser.value("seq-interval").toDouble(),
                    parser.value("seq-width").toInt(),
                    parser.value("seq-height").toInt(),
                    parser.value("seq-outpath"),
                    parser.value("seq-outfile"));
    } else
#endif
    if (!files.isEmpty()) {
        // Load the presentation after window has been exposed to give QtQuick time to construct
        // the application window properly
        QTimer *exposeTimer = new QTimer(appWindow);
        QObject::connect(exposeTimer, &QTimer::timeout, [&](){
            if (appWindow->isExposed()) {
                exposeTimer->stop();
                exposeTimer->deleteLater();
                viewer.loadFile(files.first());
            }
        });
        exposeTimer->start(0);
    } else {
        viewer.setContentView(Viewer::ConnectView);
        if (parser.isSet(QStringLiteral("connect")))
            viewer.setConnectPort(parser.value(QStringLiteral("connect")).toInt());
        viewer.connectRemote();
    }

    return a.exec();
}
