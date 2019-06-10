/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "tst_qt3dsviewer.h"

#include <QtQuick/QQuickItem>
#include <QtGui/QSurfaceFormat>
#include <QtStudio3D/Q3DSPresentation>
#include <QtStudio3D/Q3DSElement>
#include <QtStudio3D/Q3DSViewerSettings>
#include <QtStudio3D/Q3DSGeometry>
#include <QtCore/QRandomGenerator>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QRegularExpression>

void messageOutput(QtMsgType type, const QMessageLogContext &context,
    const QString &msg)
{
    Q_UNUSED(context);
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        break; // swallow
    case QtFatalMsg:
        QFAIL(msg.toLocal8Bit().constData());
    }
}

void tst_qt3dsviewer::initTestCase()
{
    qInstallMessageHandler(messageOutput);
}

void tst_qt3dsviewer::cleanupTestCase()
{
}

void tst_qt3dsviewer::init()
{
#if defined(Q_OS_ANDROID)
    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
#else
    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    m_viewer = new QQuickView;
    m_viewer->setTitle(QStringLiteral("tst_qt3dsviewer"));
    m_viewer->setSource(QUrl("qrc:/tst_qt3dsviewer.qml"));
    m_studio3DItem = m_viewer->rootObject();
    m_presentation = nullptr;
    m_settings = nullptr;
    m_ignoreError = false;

    QVERIFY(m_studio3DItem);

    const auto children = m_studio3DItem->children();
    for (auto &child : children) {
        if (!m_presentation)
            m_presentation = qobject_cast<Q3DSPresentation *>(child);
        if (!m_settings)
            m_settings = qobject_cast<Q3DSViewerSettings *>(child);
    }

    QVERIFY(m_presentation);
    QVERIFY(m_settings);
}

void tst_qt3dsviewer::cleanup()
{
    deleteCreated();
    if (!m_ignoreError)
        QCOMPARE(m_studio3DItem->property("error").toString(), {});
    m_studio3DItem = nullptr;
    m_viewer->hide();
    m_viewer->deleteLater();
    m_viewer = nullptr;
}

void tst_qt3dsviewer::testEmpty()
{
    m_presentation->setProperty("source", QUrl());
    m_viewer->show();
    QTest::qWait(1000);
    QCOMPARE(m_studio3DItem->property("running").toBool(), false);
    QVERIFY(!m_studio3DItem->property("error").toString().isEmpty());
    m_ignoreError = true; // To avoid triggering cleanup() fail as we are expecting an error
}

void tst_qt3dsviewer::testLoading()
{
    QCOMPARE(m_studio3DItem->property("running").toBool(), false);
    m_viewer->show();
    QTest::qWait(1000);
    QCOMPARE(m_studio3DItem->property("running").toBool(), true);
}


void tst_qt3dsviewer::testSlides()
{
    QSignalSpy spyEntered(m_presentation,
                          SIGNAL(slideEntered(const QString &, unsigned int, const QString &)));
    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));
    QCOMPARE(spyEntered.count(), 0);
    QCOMPARE(spyExited.count(), 0);

    m_viewer->show();
    QTest::qWait(1000);

    QCOMPARE(spyEntered.count(), 1);
    QCOMPARE(spyExited.count(), 0);

    QVERIFY(spyExited.wait(12000));
    QCOMPARE(spyEntered.count(), 2);
    QCOMPARE(spyExited.count(), 1);
}

void tst_qt3dsviewer::testFrameUpdates()
{
    QSignalSpy spyFrames(m_studio3DItem, SIGNAL(frameUpdate()));
    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));
    m_viewer->show();
    QVERIFY(spyExited.wait(12000));
    // Just ensure we get some frames, exact count will vary a lot due to external factors
    QVERIFY(spyFrames.count() > 10);
}

void tst_qt3dsviewer::testSettings()
{
    m_viewer->show();
    m_settings->setMatteColor(QColor("#0000ff"));
    QVERIFY(m_settings->matteColor() == QColor("#0000ff"));

    // Save and change matte color
    m_settings->save("", "tst_qt3dsviewer", "tst_qt3dsviewer");
    m_settings->setMatteColor(QColor("#00ff00"));
    QVERIFY(m_settings->matteColor() == QColor("#00ff00"));
    // Load and previous matte color should be back
    m_settings->load("", "tst_qt3dsviewer", "tst_qt3dsviewer");
    QVERIFY(m_settings->matteColor() == QColor("#0000ff"));
}

void tst_qt3dsviewer::testCreateElement()
{
    m_viewer->show();

    m_settings->setShowRenderStats(true);
    m_settings->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));
    QSignalSpy spyElemCreated(m_presentation, SIGNAL(elementsCreated(const QStringList &,
                                                                     const QString &)));

    QObject::connect(m_presentation, &Q3DSPresentation::elementsCreated,
                     [this](const QStringList &elementNames, const QString &error) {
        QCOMPARE(error, QString());
        for (const auto &elementName : elementNames)
            QVERIFY(m_createdElements.contains(elementName));
    });

    auto loadMatDefFile = [&](const QString &fileName) -> QString {
        QFile matDefFile(fileName);
        if (!matDefFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};

        QTextStream in(&matDefFile);
        return in.readAll();
    };

    int animValue = 0;

    QString md = loadMatDefFile(QStringLiteral(
                            ":/scenes/simple_cube_animation/materials/Basic Red.materialdef"));
    m_presentation->createMaterial(md);
    m_createdMaterials << QStringLiteral("materials/Basic Red");
    md = loadMatDefFile(QStringLiteral(
                            ":/scenes/simple_cube_animation/materials/Basic Green.materialdef"));
    m_presentation->createMaterial(md);
    m_createdMaterials << QStringLiteral("materials/Basic Green");


    QHash<QString, QVariant> data;
    data.insert(QStringLiteral("name"), QStringLiteral("New Cylinder"));
    data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
    data.insert(QStringLiteral("material"), QString());
    data.insert(QStringLiteral("starttime"), 0);
    data.insert(QStringLiteral("endtime"), 4500);
    data.insert(QStringLiteral("position"),
                QVariant::fromValue<QVector3D>(QVector3D(200, 300, 200)));
    data.insert(QStringLiteral("opacity"), 20.0);
    data.insert(QStringLiteral("controlledproperty"), QStringLiteral("@newDataInput opacity"));

    createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);

    // Elements can be registered before they are created
    Q3DSElement newCylinder(m_presentation, QStringLiteral("Scene.Layer.New Cylinder"));
    Q3DSElement newCylinder2(m_presentation,
                             QStringLiteral("Scene.Layer.New Cylinder.New Cylinder 2"));
    Q3DSElement newGroup(m_presentation, QStringLiteral("Scene.Layer.New Group"));
    Q3DSElement newSphere(m_presentation, QStringLiteral("Scene.Layer.Cube2.New Sphere"));

    QTimer animationTimer;
    animationTimer.setInterval(10);
    int animDir = 1;
    QObject::connect(&animationTimer, &QTimer::timeout, [&]() {
        if (qAbs(animValue) > 100)
            animDir = -animDir;
        animValue += animDir;
        newCylinder.setAttribute(QStringLiteral("rotation.x"), animValue * 4);
        newCylinder2.setAttribute(QStringLiteral("position.y"), animValue * 3);
        newSphere.setAttribute(QStringLiteral("position.x"), 50 + animValue * 2);
        newGroup.setAttribute(QStringLiteral("opacity"), qAbs(animValue));
        m_presentation->setDataInputValue(QStringLiteral("newDataInput"), qAbs(animValue / 2));
    });

    // Create objects to slides 1 & 2 while slide 1 is executing
    QTimer::singleShot(1000, [&]() {
        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Cylinder 2"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Red"));
        data.insert(QStringLiteral("starttime"), 500);
        data.insert(QStringLiteral("endtime"), 5000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(50, animValue, 50)));

        createElement(QStringLiteral("Scene.Layer.New Cylinder"),
                      QStringLiteral("Slide1"), data);

        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Sphere"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Sphere"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Green"));
        data.insert(QStringLiteral("starttime"), 1000);
        data.insert(QStringLiteral("endtime"), 4000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(animValue, 75, 0)));

        createElement(QStringLiteral("Scene.Layer.Cube2"), QStringLiteral("Slide2"), data);

        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("Sphere To Delete"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Sphere"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Red"));
        data.insert(QStringLiteral("starttime"), 0);
        data.insert(QStringLiteral("endtime"), 10000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(-100, -100, 0)));

        createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide2"), data);

        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Group"));
        data.insert(QStringLiteral("type"), QStringLiteral("group"));
        data.insert(QStringLiteral("starttime"), 0);
        data.insert(QStringLiteral("endtime"), 10000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(50, -100, 0)));

        createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);

        QVector<QHash<QString, QVariant>> groupElemProps;
        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("Child 1 of Group"));
        data.insert(QStringLiteral("type"), QStringLiteral("model"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Green"));
        data.insert(QStringLiteral("starttime"), 1000);
        data.insert(QStringLiteral("endtime"), 4000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(0, 0, 0)));
        groupElemProps << data;
        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("Child 2 of Group"));
        data.insert(QStringLiteral("type"), QStringLiteral("model"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Green"));
        data.insert(QStringLiteral("starttime"), 2000);
        data.insert(QStringLiteral("endtime"), 4000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(100, 0, 0)));
        groupElemProps << data;

        m_createdElements << QStringLiteral("Scene.Layer.New Group.Child 1 of Group")
                          << QStringLiteral("Scene.Layer.New Group.Child 2 of Group");

        m_presentation->createElements(QStringLiteral("Scene.Layer.New Group"),
                                       QStringLiteral("Slide1"), groupElemProps);

        animationTimer.start();
    });

    // Switch to slide 2
    QVERIFY(spyExited.wait(20000));

    // Remove dynamically added object
    QTimer::singleShot(3000, [&]() {
        m_presentation->deleteElement(QStringLiteral("Scene.Layer.Sphere To Delete"));
        m_createdElements.removeOne(QStringLiteral("Scene.Layer.Sphere To Delete"));
    });

    // Create objects to slides 1 and 2 while slide 2 is executing
    QTimer::singleShot(2000, [&]() {
        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Cylinder 3"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Green"));
        data.insert(QStringLiteral("starttime"), 0);
        data.insert(QStringLiteral("endtime"), 3000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(-100, -100, 0)));

        createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);

        data.clear();
        data.insert(QStringLiteral("name"), QStringLiteral("New Sphere 2"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Sphere"));
        data.insert(QStringLiteral("material"), QStringLiteral("Basic Green"));
        data.insert(QStringLiteral("starttime"), 0);
        data.insert(QStringLiteral("endtime"), 5000);
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(-100, 100, 0)));

        createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide2"), data);
    });

    // Switch to slide 1
    QVERIFY(spyExited.wait(20000));

    QRandomGenerator rnd;
    QVector<QHash<QString, QVariant>> massProps;
    for (int i = 0; i < 1000; ++i) {
        data.clear();
        QString elementName = QStringLiteral("MassElement_%1").arg(i);
        data.insert(QStringLiteral("name"), elementName);
        data.insert(QStringLiteral("sourcepath"),
                    i % 2 ? QStringLiteral("#Cube") : QStringLiteral("#Cone"));
        data.insert(QStringLiteral("material"),
                    i % 2 ? QStringLiteral("Basic Green") : QStringLiteral("Basic Red"));
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(rnd.bounded(-600, 600),
                                                             rnd.bounded(-600, 600),
                                                             rnd.bounded(800, 1200))));
        massProps << data;
        m_createdElements << QStringLiteral("Scene.Layer.") + elementName;
    }
    m_presentation->createElements(QStringLiteral("Scene.Layer"), QStringLiteral("Slide2"),
                                   massProps);

    // Switch to slide 2
    QVERIFY(spyExited.wait(20000));

    QTest::qWait(500);
    QCOMPARE(spyElemCreated.count(), 9);
    const QStringList createdElements = m_presentation->createdElements();
    QCOMPARE(createdElements.size(), m_createdElements.size());
    for (const auto &elementName : createdElements)
        QVERIFY(m_createdElements.contains(elementName));
    deleteCreated();

    // Switch to slide 1
    QVERIFY(spyExited.wait(20000));
    QTest::qWait(1000);
}

void tst_qt3dsviewer::testCreateMaterial()
{
    m_viewer->show();

    m_settings->setShowRenderStats(true);
    m_settings->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));
    QSignalSpy spyMatCreated(m_presentation, SIGNAL(materialsCreated(const QStringList &,
                                                                     const QString &)));
    QSignalSpy spyElemCreated(m_presentation, SIGNAL(elementsCreated(const QStringList &,
                                                                     const QString &)));

    QStringList materialDefinitions;
    // Create material via .materialdef file in resources
    materialDefinitions
            << QStringLiteral(":/scenes/simple_cube_animation/materials/Basic Blue.materialdef")
            << QStringLiteral(":/scenes/simple_cube_animation/materials/Basic Texture.materialdef");

    // Create material directly from materialdef content
    auto loadMatDefFile = [&](const QString &fileName) -> QString {
        QFile matDefFile(fileName);
        if (!matDefFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};

        QTextStream in(&matDefFile);
        return in.readAll();
    };
    QString matDef = loadMatDefFile(
                QStringLiteral(":/scenes/simple_cube_animation/materials/Copper.materialdef"));
    QVERIFY(!matDef.isEmpty());
    materialDefinitions << matDef;

    m_presentation->createMaterials(materialDefinitions);
    m_createdMaterials << QStringLiteral("materials/Basic Blue")
                       << QStringLiteral("materials/Basic Texture")
                       << QStringLiteral("materials/Copper");

    QObject::connect(m_presentation, &Q3DSPresentation::materialsCreated,
                     [this](const QStringList &materialNames, const QString &error) {
        QCOMPARE(error, QString());
        for (auto &name : materialNames) {
            QVERIFY(m_createdMaterials.contains(name));
            QHash<QString, QVariant> data;
            if (name == QLatin1String("materials/Basic Blue")) {
                data.insert(QStringLiteral("name"), QStringLiteral("Blue Cylinder"));
                data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cylinder"));
                data.insert(QStringLiteral("material"), name);
                data.insert(QStringLiteral("position"),
                            QVariant::fromValue<QVector3D>(QVector3D(200, 300, 200)));
                createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
            } else if (name == QLatin1String("materials/Basic Texture")) {
                data.insert(QStringLiteral("name"), QStringLiteral("Textured Cone"));
                data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cone"));
                data.insert(QStringLiteral("material"), name);
                data.insert(QStringLiteral("position"),
                            QVariant::fromValue<QVector3D>(QVector3D(-200, -300, 200)));
                createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
            } else if (name == QLatin1String("materials/Copper")) {
                data.insert(QStringLiteral("name"), QStringLiteral("Copper Sphere"));
                data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Sphere"));
                data.insert(QStringLiteral("material"), name);
                data.insert(QStringLiteral("position"),
                            QVariant::fromValue<QVector3D>(QVector3D(-200, 300, 200)));
                createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
            } else if (name == QLatin1String("materials/Just Yellow")) {
                QHash<QString, QVariant> data;
                data.insert(QStringLiteral("name"), QStringLiteral("Yellow Cube"));
                data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cube"));
                data.insert(QStringLiteral("material"), name);
                data.insert(QStringLiteral("position"),
                            QVariant::fromValue<QVector3D>(QVector3D(200, -300, 200)));
                createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
            } else {
                QVERIFY(false);
            }
        }
    });

    // Create material after start
    QTimer::singleShot(1000, [&]() {
        QString md = loadMatDefFile(QStringLiteral(
                        ":/scenes/simple_cube_animation/materials/Basic Blue.materialdef"));
        // Modify the diffuse color and material name so that we can be sure it is a new one
        md.replace(QStringLiteral("Basic Blue"), QStringLiteral("Just Yellow"));
        md.replace(QRegularExpression(QStringLiteral("\"diffuse\">.*<")),
                   QStringLiteral("\"diffuse\">1 1 0 1<"));
        m_presentation->createMaterial(md);
        m_createdMaterials << QStringLiteral("materials/Just Yellow");
    });

    // Delete material
    QTimer::singleShot(2500, [&]() {
        m_presentation->deleteElement(QStringLiteral("Scene.Layer.Textured Cone"));
        m_presentation->deleteMaterial("materials/Basic Texture");
        m_createdMaterials.removeOne(QStringLiteral("materials/Basic Texture"));

        // Try to use the deleted material - should find a fallback material
        QHash<QString, QVariant> data;
        data.insert(QStringLiteral("name"), QStringLiteral("Textured Cone 2"));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("#Cone"));
        data.insert(QStringLiteral("material"), QStringLiteral("materials/Basic Texture"));
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(QVector3D(-100, -300, 200)));
        createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
    });

    QVERIFY(spyExited.wait(20000));
    QCOMPARE(spyMatCreated.count(), 2);
    QCOMPARE(spyElemCreated.count(), 5);
    const QStringList createdMaterials = m_presentation->createdMaterials();
    QCOMPARE(createdMaterials.size(), m_createdMaterials.size());
    for (const auto &name : createdMaterials)
        QVERIFY(m_createdMaterials.contains(name));
    deleteCreated();
    QTest::qWait(200); // Extra wait to verify slide change visually
}

void tst_qt3dsviewer::testCreateMesh()
{
    m_viewer->show();

    m_settings->setShowRenderStats(true);
    m_settings->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));
    QSignalSpy spyMeshCreated(m_presentation, SIGNAL(meshesCreated(const QStringList &,
                                                                   const QString &)));
    QSignalSpy spyElemCreated(m_presentation, SIGNAL(elementsCreated(const QStringList &,
                                                                     const QString &)));
    Q3DSGeometry pyramid;
    Q3DSGeometry star;
    createGeometries(pyramid, star);

    Q3DSElement pyramidElem(m_presentation, QStringLiteral("Scene.Layer.Pyramid"));
    Q3DSElement starElem(m_presentation, QStringLiteral("Scene.Layer.Star"));

    int animValue = 0;
    QTimer animationTimer;
    animationTimer.setInterval(10);
    QObject::connect(&animationTimer, &QTimer::timeout, [&]() {
        animValue++;
        pyramidElem.setAttribute(QStringLiteral("rotation.x"), animValue * 2);
        pyramidElem.setAttribute(QStringLiteral("rotation.y"), animValue);
        starElem.setAttribute(QStringLiteral("rotation.x"), -animValue * 2);
        starElem.setAttribute(QStringLiteral("rotation.y"), -animValue);
    });

    m_presentation->createMaterial(
            QStringLiteral(":/scenes/simple_cube_animation/materials/Basic Texture.materialdef"));
    m_createdMaterials << QStringLiteral("materials/Basic Texture");
    m_presentation->createMesh(QStringLiteral("Pyramid"), pyramid);
    m_createdMeshes << QStringLiteral("Pyramid");

    QObject::connect(m_presentation, &Q3DSPresentation::meshesCreated,
                     [&](const QStringList &meshNames, const QString &error) {
        QCOMPARE(error, QString());
        for (auto &name : meshNames) {
            QVERIFY(m_createdMeshes.contains(name));
            QHash<QString, QVariant> data;
            if (name == QLatin1String("Pyramid")) {
                data.insert(QStringLiteral("name"), QStringLiteral("Pyramid"));
                data.insert(QStringLiteral("sourcepath"), QStringLiteral("Pyramid"));
                data.insert(QStringLiteral("material"), QStringLiteral("Basic Texture"));
                data.insert(QStringLiteral("position"),
                            QVariant::fromValue<QVector3D>(QVector3D(100, 150, 500)));
                createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
                animationTimer.start();
            } else if (name == QLatin1String("Star")) {
                data.insert(QStringLiteral("name"), QStringLiteral("Star"));
                data.insert(QStringLiteral("sourcepath"), QStringLiteral("Star"));
                data.insert(QStringLiteral("material"), QStringLiteral("Basic Texture"));
                data.insert(QStringLiteral("position"),
                            QVariant::fromValue<QVector3D>(QVector3D(100, -150, 500)));
                createElement(QStringLiteral("Scene.Layer"), QStringLiteral("Slide1"), data);
            } else {
                QVERIFY(false);
            }
        }
    });

    // Create mesh after start
    QTimer::singleShot(1000, [&]() {
        m_presentation->createMesh(QStringLiteral("Star"), star);
        m_createdMeshes << QStringLiteral("Star");
    });

    QTimer::singleShot(3000, [&]() {
        m_presentation->deleteElement(QStringLiteral("Scene.Layer.Star"));
        m_presentation->deleteMesh(QStringLiteral("Star"));
        m_createdMeshes.removeOne(QStringLiteral("Star"));
    });

    QVERIFY(spyExited.wait(20000));
    QCOMPARE(spyMeshCreated.count(), 2);
    QCOMPARE(spyElemCreated.count(), 2);
    const QStringList createdMeshes = m_presentation->createdMeshes();
    QCOMPARE(createdMeshes.size(), m_createdMeshes.size());
    for (const auto &name : createdMeshes)
        QVERIFY(m_createdMeshes.contains(name));
    deleteCreated();
    QTest::qWait(200); // Extra wait to verify slide change visually
}

void tst_qt3dsviewer::testMouseEvents()
{
    m_viewer->show();
    QTest::qWait(1000);

    QSignalSpy spyEvents(m_studio3DItem,
                         SIGNAL(ignoredEventsChanged()));
    QSignalSpy spyExited(m_presentation,
                         SIGNAL(slideExited(const QString &, unsigned int, const QString &)));

    QCOMPARE(spyEvents.count(), 0);
    QCOMPARE(spyExited.count(), 0);

    // Ignore mouse, so slide doesn't change
    m_studio3DItem->setProperty("ignoredEvents", 1);
    QTest::mousePress(m_viewer, Qt::LeftButton);
    QTest::qWait(1000);
    QTest::mouseRelease(m_viewer, Qt::LeftButton);
    QCOMPARE(spyEvents.count(), 1);
    QCOMPARE(spyExited.count(), 0);

    // Enable mouse, clicking switches slide
    m_studio3DItem->setProperty("ignoredEvents", 0);
    QTest::mousePress(m_viewer, Qt::LeftButton);
    QTest::qWait(1000);
    QTest::mouseRelease(m_viewer, Qt::LeftButton);
    QCOMPARE(spyEvents.count(), 2);
    QCOMPARE(spyExited.count(), 1);
}

void tst_qt3dsviewer::deleteCreated()
{
    m_presentation->deleteElements(m_createdElements);
    m_presentation->deleteMaterials(m_createdMaterials);
    m_presentation->deleteMeshes(m_createdMeshes);
    m_createdElements.clear();
    m_createdMaterials.clear();
    m_createdMeshes.clear();
    QVERIFY(m_presentation->createdElements().size() == 0);
    QVERIFY(m_presentation->createdMaterials().size() == 0);
    QVERIFY(m_presentation->createdMeshes().size() == 0);
}

void tst_qt3dsviewer::createElement(const QString &parentElementPath, const QString &slideName,
                                    const QHash<QString, QVariant> &properties)
{
    m_createdElements << parentElementPath + QLatin1Char('.')
                         + properties[QStringLiteral("name")].toString();
    m_presentation->createElement(parentElementPath, slideName, properties);
}

void tst_qt3dsviewer::createGeometries(Q3DSGeometry &pyramid, Q3DSGeometry &star)
{
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D uv;
    };

    QVector<Vertex> vertices;

    auto createVertex = [&](const QVector3D &xyz, const QVector3D &n, const QVector2D &uv) {
        Vertex newVertex;
        newVertex.position = xyz;
        if (n.isNull())
            newVertex.normal = xyz; // This is almost never the correct normal
        else
            newVertex.normal = n.normalized();
        newVertex.uv = uv;
        vertices.append(newVertex);
    };

    auto createTriangle = [&](const QVector3D &xyz1, const QVector2D &uv1,
                              const QVector3D &xyz2, const QVector2D &uv2,
                              const QVector3D &xyz3, const QVector2D &uv3) {
        QVector3D n;
        n = QVector3D::crossProduct(xyz2 - xyz1, xyz3 - xyz1).normalized();

        createVertex(xyz1, n, uv1);
        createVertex(xyz2, n, uv2);
        createVertex(xyz3, n, uv3);
    };

    // Pyramid (no index buffer)
    {
        QVector3D xyz[5] = {{0, 0, 50}, {50, 50, -50}, {50, -50, -50}, {-50, -50, -50},
                            {-50, 50, -50}};
        QVector2D uv[4] = {{1, 1}, {1, 0}, {0, 0}, {0, 1}};
        createTriangle(xyz[0], uv[0], xyz[1], uv[1], xyz[2], uv[2]);
        createTriangle(xyz[0], uv[0], xyz[2], uv[1], xyz[3], uv[2]);
        createTriangle(xyz[0], uv[0], xyz[3], uv[1], xyz[4], uv[2]);
        createTriangle(xyz[0], uv[0], xyz[4], uv[1], xyz[1], uv[2]);
        createTriangle(xyz[1], uv[0], xyz[4], uv[2], xyz[3], uv[1]);
        createTriangle(xyz[1], uv[0], xyz[3], uv[3], xyz[2], uv[2]);

        QByteArray vertexBuffer(reinterpret_cast<const char *>(vertices.constData()),
                                vertices.size() * int(sizeof(Vertex)));

        pyramid.clear();
        pyramid.setVertexData(vertexBuffer);
        pyramid.addAttribute(Q3DSGeometry::Attribute::PositionSemantic);
        pyramid.addAttribute(Q3DSGeometry::Attribute::NormalSemantic);
        pyramid.addAttribute(Q3DSGeometry::Attribute::TexCoordSemantic);
    }

    vertices.clear();

    // Star (using index buffer)
    {
        // Note: Since faces share vertices, the normals on the vertices are not correct
        // for any face, leading to weird lighting behavior
        createVertex({0, 150, 0},     {}, {0.5f, 1});
        createVertex({50, 50, -50},   {}, {0.66f, 0.66f});
        createVertex({150, 0, 0},     {}, {1, 0.5f});
        createVertex({50, -50, -50},  {}, {0.66f, 0.33f});
        createVertex({0, -150, 0},    {}, {0.5f, 0});
        createVertex({-50, -50, -50}, {}, {0.33f, 0.33f});
        createVertex({-150, 0, 0},    {}, {0, 0.5f});
        createVertex({-50, 50, -50},  {}, {0.33f, 0.66f});
        createVertex({50, 50, 50},    {}, {0.66f, 0.66f});
        createVertex({50, -50, 50},   {}, {0.66f, 0.33f});
        createVertex({-50, -50, 50},  {}, {0.33f, 0.33f});
        createVertex({-50, 50, 50},   {}, {0.33f, 0.66f});

        QVector<quint16> indices = {
            0, 1, 8, 0, 7, 1, 0, 11, 7, 0, 8, 11,   // Top pyramid
            2, 1, 3, 2, 3, 9, 2, 9, 8, 2, 8, 1,     // Right pyramid
            4, 3, 5, 4, 5, 10, 4, 10, 9, 4, 9, 3,   // Bottom pyramid
            6, 5, 7, 6, 7, 11, 6, 11, 10, 6, 10, 5, // Left pyramid
            1, 7, 5, 1, 5, 3,                       // Front center rect
            8, 10, 11, 8, 9, 10                     // Back center rect
        };

        QByteArray vertexBuffer(reinterpret_cast<const char *>(vertices.constData()),
                                vertices.size() * int(sizeof(Vertex)));
        QByteArray indexBuffer(reinterpret_cast<const char *>(indices.constData()),
                               indices.size() * int(sizeof(quint16)));

        Q3DSGeometry::Attribute indexAtt;
        indexAtt.semantic = Q3DSGeometry::Attribute::IndexSemantic;
        indexAtt.componentType = Q3DSGeometry::Attribute::ComponentType::U16Type;

        star.clear();
        star.setVertexData(vertexBuffer);
        star.setIndexData(indexBuffer);
        star.addAttribute(Q3DSGeometry::Attribute::PositionSemantic);
        star.addAttribute(Q3DSGeometry::Attribute::NormalSemantic);
        star.addAttribute(Q3DSGeometry::Attribute::TexCoordSemantic);
        star.addAttribute(indexAtt);
    }
}

QTEST_MAIN(tst_qt3dsviewer)
