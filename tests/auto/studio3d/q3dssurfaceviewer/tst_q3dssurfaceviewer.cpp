/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include <QtTest/QtTest>
#include <QtStudio3D/q3dssurfaceviewer.h>
#include <QtStudio3D/q3dsviewersettings.h>
#include <QtStudio3D/q3dspresentation.h>
#include <QtStudio3D/q3dssceneelement.h>
#include <QtStudio3D/q3dselement.h>
#include <QtStudio3D/q3dsdatainput.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qimage.h>
#include <QtGui/qscreen.h>
#include <QtGui/qopenglframebufferobject.h>
#include <QtGui/qevent.h>
#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include "../shared/shared_presentations.h"

class tst_Q3DSSurfaceViewer : public QObject
{
    Q_OBJECT
public:
    tst_Q3DSSurfaceViewer();
    ~tst_Q3DSSurfaceViewer() {}

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testBasics_data();
    void testBasics();
    void testSourceChange_data();
    void testSourceChange();
    void testSizeChange_data();
    void testSizeChange();
    void testUpdateInterval_data();
    void testUpdateInterval();
    void testMultiple_data();
    void testMultiple();
    void testGrab_data();
    void testGrab();
    void testReset_data();
    void testReset();
    void testSettings_data();
    void testSettings();
    void testPresentation_data();
    void testPresentation();
    void testPresentationActivation_data();
    void testPresentationActivation();
    void testSceneElement_data();
    void testSceneElement();
    void testElement_data();
    void testElement();
    void testMouseInput_data();
    void testMouseInput();
    void testDataInput_data();
    void testDataInput();

private:
    QWindow *createWindow(const QSize &size);
    QOffscreenSurface *createOffscreen();
    QOpenGLFramebufferObject *createFbo(const QSize &size);

    // Created viewers are returned via *&viewer parameter rather than return value, so that we can
    // use QCOMPARE and QVERIFY inside these functions (they require void return value).
    void createViewer(Q3DSSurfaceViewer *&viewer, QSurface *surface, const QUrl &url,
                      bool autoSize, const QSize &size, int updateInterval, int fboId);
    void createWindowAndViewer(Q3DSSurfaceViewer *&viewer, const QUrl &url = RED,
                               bool autoSize = true, const QSize &size = QSize(),
                               int updateInterval = 0);
    void createOffscreenAndViewer(Q3DSSurfaceViewer *&viewer, const QUrl &url = RED,
                                  const QSize &size = QSize(), int updateInterval = 0);
    void checkPixel(Q3DSSurfaceViewer *viewer, const QColor &color,
                    const QPoint &pixel = QPoint(50, 50));

    QWindow *m_window;
    QOffscreenSurface *m_surface;
    Q3DSSurfaceViewer *m_viewer;
    QSurfaceFormat m_format;
    QOpenGLContext *m_context;
    QOpenGLFramebufferObject *m_fbo;
};

tst_Q3DSSurfaceViewer::tst_Q3DSSurfaceViewer()
    : m_window(nullptr)
    , m_surface(nullptr)
    , m_viewer(nullptr)
    , m_context(nullptr)
    , m_fbo(nullptr)
{
}

//#define DUMP_LOGFILE  // Uncomment log Qt 3D Studio internal messages to log.txt file
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    switch (type) {
    //    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg: {
#ifdef DUMP_LOGFILE
        QFile file("log.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream stream(&file);
            stream << msg << "\n";
        }
        file.close();
#endif
    } break; // swallow
    case QtFatalMsg:
        QFAIL(msg.toLocal8Bit().constData());
    }
}

void tst_Q3DSSurfaceViewer::initTestCase()
{
    qInstallMessageHandler(messageOutput);
#ifdef DUMP_LOGFILE
    QFile file("log.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream stream(&file);
        stream << "Log file: " << QTime::currentTime().toString() << "\n";
    }
    file.close();
#endif

    QWindow *dummy = createWindow(QSize(100, 100));
    m_format = dummy->format();
    qDebug() << m_format;
    delete dummy;
}

void tst_Q3DSSurfaceViewer::init()
{
    m_context = new QOpenGLContext();
    m_context->setFormat(m_format);
    m_context->create();
}

void tst_Q3DSSurfaceViewer::cleanup()
{
    if (m_window)
        m_window->close();

    delete m_viewer;
    m_viewer = nullptr;

    delete m_window;
    m_window = nullptr;

    delete m_surface;
    m_surface = nullptr;

    delete m_fbo;
    m_fbo = nullptr;

    delete m_context;
    m_context = nullptr;
}

QWindow *tst_Q3DSSurfaceViewer::createWindow(const QSize &size)
{
    QWindow *window = new QWindow();

    window->resize(size);
    window->setSurfaceType(QSurface::OpenGLSurface);
    window->setFormat(m_format);
    window->setTitle(QStringLiteral("Q3DSSurfaceViewer test window"));
    window->create();

    return window;
}

QOffscreenSurface *tst_Q3DSSurfaceViewer::createOffscreen()
{
    QOffscreenSurface *surface = new QOffscreenSurface();
    surface->setFormat(m_format);
    surface->create();

    return surface;
}

QOpenGLFramebufferObject *tst_Q3DSSurfaceViewer::createFbo(const QSize &size)
{
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, fboFormat);
}

void tst_Q3DSSurfaceViewer::createViewer(Q3DSSurfaceViewer *&viewer, QSurface *surface,
                                         const QUrl &url, bool autoSize, const QSize &size,
                                         int updateInterval, int fboId)
{
    viewer = new Q3DSSurfaceViewer();
    QSignalSpy spy(viewer, &Q3DSSurfaceViewer::runningChanged);

    viewer->presentation()->setSource(url);
    QCOMPARE(viewer->presentation()->source(), url);

    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 0);

    viewer->setAutoSize(autoSize);
    if (!autoSize)
        viewer->setSize(size);
    viewer->setUpdateInterval(updateInterval);

    QVERIFY(viewer->create(surface, m_context, fboId));

    QCOMPARE(spy.count(), 1);
    QVERIFY(viewer->isRunning());

    QVERIFY(viewer->fboId() == fboId);
    QVERIFY(viewer->surface() == surface);
    QVERIFY(viewer->context() == m_context);
}

void tst_Q3DSSurfaceViewer::createWindowAndViewer(Q3DSSurfaceViewer *&viewer,
                                                  const QUrl &url, bool autoSize,
                                                  const QSize &size, int updateInterval)
{
    QSize actualSize = size;
    if (actualSize.isEmpty())
        actualSize = QSize(300, 200);

    m_window = createWindow(actualSize);

    createViewer(viewer, m_window, url, autoSize, actualSize, updateInterval, 0);

    m_window->show();
    QGuiApplication::processEvents();
}

void tst_Q3DSSurfaceViewer::createOffscreenAndViewer(Q3DSSurfaceViewer *&viewer,
                                                     const QUrl &url, const QSize &size,
                                                     int updateInterval)
{
    QSize actualSize = size;
    if (actualSize.isEmpty())
        actualSize = QSize(300, 200);

    m_surface = createOffscreen();
    m_context->makeCurrent(m_surface);
    m_fbo = createFbo(actualSize);

    createViewer(viewer, m_surface, url, false, actualSize, updateInterval, m_fbo->handle());
}

void tst_Q3DSSurfaceViewer::checkPixel(Q3DSSurfaceViewer *viewer, const QColor &color,
                                       const QPoint &pixel)
{
    // Grab operation is potentially costly, so retry only every second instead of using
    // QTRY_COMPARE which would try it every 50ms. We also want to wait first as it takes some time
    // for the presentation to be displayed.
    QColor grabColor;
    for (int i = 0; i < 20; i++) {
        // Note: If checkpixel is ever changed to not have this wait, some
        // QGuiApplication::processEvents() calls may be necessary in various test cases to
        // ensure expected signaling order.
        QTest::qWait(1000);
        QImage image = viewer->grab(QRect(pixel, QSize(1, 1)));
        grabColor = QColor(image.pixel(0, 0));
        if (grabColor == color)
            break;
    }
    QCOMPARE(grabColor, color);
}

void tst_Q3DSSurfaceViewer::testBasics_data()
{
    QTest::addColumn<bool>("isWindow");
    QTest::newRow("window") << true;
    QTest::newRow("offscreen") << false;
}

void tst_Q3DSSurfaceViewer::testBasics()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, RED);
    else
        createOffscreenAndViewer(m_viewer, RED);

    QSignalSpy spy(m_viewer, &Q3DSSurfaceViewer::runningChanged);
    QVERIFY(spy.isValid());

    checkPixel(m_viewer, Qt::red);

    m_viewer->destroy();

    QCOMPARE(spy.count(), 1);
    QVERIFY(!m_viewer->isRunning());
}

void tst_Q3DSSurfaceViewer::testSourceChange_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testSourceChange()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, RED);
    else
        createOffscreenAndViewer(m_viewer, RED);

    QSignalSpy spy(m_viewer->presentation(), &Q3DSPresentation::sourceChanged);
    QVERIFY(spy.isValid());
    QVERIFY(m_viewer->presentation()->source() == RED);

    checkPixel(m_viewer, Qt::red);

    // Different source
    m_viewer->presentation()->setSource(BLUE);
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_viewer->presentation()->source() == BLUE);

    checkPixel(m_viewer, Qt::blue);

    // Reset same source
    m_viewer->presentation()->setSource(BLUE);
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_viewer->presentation()->source() == BLUE);

    checkPixel(m_viewer, Qt::blue);

    // Different source again
    m_viewer->presentation()->setSource(RED);
    QCOMPARE(spy.count(), 2);
    QVERIFY(m_viewer->presentation()->source() == RED);

    checkPixel(m_viewer, Qt::red);
}

void tst_Q3DSSurfaceViewer::testSizeChange_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testSizeChange()
{
    QFETCH(bool, isWindow);

    if (isWindow) {
        createWindowAndViewer(m_viewer, MIXED, true, QSize(600, 600));
    } else {
        // Changing size for offscreen surface means recreating the fbo. There's no guarantee
        // we can get the same fbo id if we do that, so we would have to reinitialize anyway
        QSKIP("Skipping size change testing for offscreen surfaces");
    }

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QSignalSpy spy1(m_viewer, &Q3DSSurfaceViewer::sizeChanged);
    QVERIFY(spy1.isValid());
    QSignalSpy spy2(m_viewer, &Q3DSSurfaceViewer::autoSizeChanged);
    QVERIFY(spy2.isValid());

    QPoint leftPoint(m_viewer->size().width() * 11 / 24, m_viewer->size().height() / 2);
    QPoint rightPoint(m_viewer->size().width() * 13 / 24, m_viewer->size().height() / 2);

    // MIXED presentation has left side blue, right side red
    checkPixel(m_viewer, Qt::blue, leftPoint);
    checkPixel(m_viewer, Qt::red, rightPoint);

    m_window->resize(QSize(800, 800));
    QGuiApplication::processEvents();
    QCOMPARE(spy1.count(), 1);

    checkPixel(m_viewer, Qt::blue, leftPoint);
    checkPixel(m_viewer, Qt::blue, rightPoint);

    m_window->resize(QSize(400, 400));
    QGuiApplication::processEvents();
    QCOMPARE(spy1.count(), 2);

    checkPixel(m_viewer, Qt::red, leftPoint);
    checkPixel(m_viewer, Qt::red, rightPoint);

    m_viewer->setAutoSize(false);
    QCOMPARE(spy2.count(), 1);

    // Size should not change since autosize is no longer on
    m_window->resize(QSize(600, 600));
    QGuiApplication::processEvents();
    QCOMPARE(spy1.count(), 2);

    checkPixel(m_viewer, Qt::red, leftPoint);
    checkPixel(m_viewer, Qt::red, rightPoint);

    m_viewer->setSize(QSize(700, 700));
    QCOMPARE(spy1.count(), 3);

    checkPixel(m_viewer, Qt::blue, leftPoint);
    checkPixel(m_viewer, Qt::blue, rightPoint);
}

void tst_Q3DSSurfaceViewer::testUpdateInterval_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testUpdateInterval()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, ANIMATION);
    else
        createOffscreenAndViewer(m_viewer, ANIMATION);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QSignalSpy spy(m_viewer, &Q3DSSurfaceViewer::updateIntervalChanged);
    QVERIFY(spy.isValid());
    QVERIFY(m_viewer->updateInterval() == 0);

    checkPixel(m_viewer, Qt::black);
    {
        // Grab two images two seconds apart to verify animation is happening
        QImage image1 = m_viewer->grab();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grab();
        QVERIFY(image1 != image2);

    }
    {
        m_viewer->setUpdateInterval(100000);
        QVERIFY(m_viewer->updateInterval() == 100000);
        QCOMPARE(spy.count(), 1);
        // Can't test if animation actually stopped, as grabbing the viewer forces update on it
    }
    {
        m_viewer->setUpdateInterval(20);
        QCOMPARE(spy.count(), 2);
        QVERIFY(m_viewer->updateInterval() == 20);

        // Non-zero interval short enough to see animation
        QImage image1 = m_viewer->grab();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grab();
        QVERIFY(image1 != image2);
    }
    {
        m_viewer->setUpdateInterval(-1);
        QCOMPARE(spy.count(), 3);
        QVERIFY(m_viewer->updateInterval() == -1);
        // Can't test if animation actually stopped, as grabbing the viewer forces update on it
    }
}

void tst_Q3DSSurfaceViewer::testMultiple_data()
{
    QTest::addColumn<int>("windowCount");
    QTest::addColumn<int>("offscreenCount");
    QTest::newRow("windows") << 2 << 0;
    QTest::newRow("offscreens") << 0 << 2;
    QTest::newRow("mixed") << 1 << 1;
}

void tst_Q3DSSurfaceViewer::testMultiple()
{
    QFETCH(int, windowCount);
    QFETCH(int, offscreenCount);

    QVector<QWindow *> windows;
    QVector<QOffscreenSurface *> surfaces;
    QVector<Q3DSSurfaceViewer *> viewers;
    QVector<QOpenGLFramebufferObject *> fbos;

    int viewerCount = windowCount + offscreenCount;
    windows.resize(windowCount);
    surfaces.resize(offscreenCount);
    fbos.resize(offscreenCount);
    viewers.resize(viewerCount);

    QSize size(200, 150);
    QUrl url;
    for (int i = 0; i < windowCount; i++) {
        windows[i] = createWindow(size);
        if (i % 2)
            url = RED;
        else
            url = BLUE;
        createViewer(viewers[i], windows[i], url, true, size, 0, 0);
        windows[i]->setPosition(10 + i * 50, 10 + i * 50);
        windows[i]->show();
        QGuiApplication::processEvents();
    }
    for (int i = 0; i < offscreenCount; i++) {
        surfaces[i] = createOffscreen();
        m_context->makeCurrent(surfaces[i]);
        fbos[i] = createFbo(size);
        if ((i + windowCount) % 2)
            url = RED;
        else
            url = BLUE;
        createViewer(viewers[i + windowCount], surfaces[i], url, false, size, 0,
                fbos[i]->handle());
    }

    for (int i = 0; i < viewerCount; i++) {
        if (i % 2)
            checkPixel(viewers[i], Qt::red);
        else
            checkPixel(viewers[i], Qt::blue);
    }

    for (QWindow *w : windows) {
        w->close();
        delete w;
    }
    windows.clear();

    for (QOffscreenSurface *s : surfaces)
        delete s;
    surfaces.clear();

    for (Q3DSSurfaceViewer *v : viewers)
        delete v;
    viewers.clear();

    for (QOpenGLFramebufferObject *f : fbos)
        delete f;
    fbos.clear();
}

void tst_Q3DSSurfaceViewer::testGrab_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testGrab()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, MIXED_VERTICAL);
    else
        createOffscreenAndViewer(m_viewer, MIXED_VERTICAL);

    // Single pixels
    int w = m_viewer->size().width();
    int h = m_viewer->size().height();

    checkPixel(m_viewer, Qt::blue, QPoint(w / 2, h / 4));
    checkPixel(m_viewer, Qt::red, QPoint(w / 2, 3 * h / 4));

    checkPixel(m_viewer, Qt::blue, QPoint(0, 0));
    checkPixel(m_viewer, Qt::blue, QPoint(w - 1, 0));
    checkPixel(m_viewer, Qt::red, QPoint(0, h - 1));
    checkPixel(m_viewer, Qt::red, QPoint(w - 1, h - 1));

    // Full buffer
    QImage img = m_viewer->grab();
    QColor grabColor1 = img.pixel(w / 2, h / 4);
    QColor grabColor2 = img.pixel(w / 2, 3 * h / 4);
    QCOMPARE(grabColor1, QColor(Qt::blue));
    QCOMPARE(grabColor2, QColor(Qt::red));

    // Partial buffer
    img = m_viewer->grab(QRect(w / 3, h / 3, w / 2, h / 2));
    grabColor1 = img.pixel(0, 0);
    grabColor2 = img.pixel(w / 2 - 1, h / 2 - 1);
    QCOMPARE(grabColor1, QColor(Qt::blue));
    QCOMPARE(grabColor2, QColor(Qt::red));
}

void tst_Q3DSSurfaceViewer::testReset_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testReset()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, RED);
    else
        createOffscreenAndViewer(m_viewer, RED);

    checkPixel(m_viewer, Qt::red);

    m_viewer->presentation()->setAttribute(QStringLiteral("Scene.Layer.Rectangle.Material"),
                                           QStringLiteral("diffuse.r"), QVariant(0.0));
    m_viewer->presentation()->setAttribute(QStringLiteral("Scene.Layer.Rectangle.Material"),
                                           QStringLiteral("diffuse.b"), QVariant(1.0));
    checkPixel(m_viewer, Qt::blue);

    // Note: reset() is private method now, instead can reload presentation by switching sources
    //  m_viewer->reset();
    m_viewer->presentation()->setSource(ANIMATION);
    m_viewer->presentation()->setSource(RED);

    checkPixel(m_viewer, Qt::red);
}

void tst_Q3DSSurfaceViewer::testSettings_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testSettings()
{
    QFETCH(bool, isWindow);

    int width = 500;
    int height = 500;

    if (isWindow)
        createWindowAndViewer(m_viewer, SETTINGS, false, QSize(width, height));
    else
        createOffscreenAndViewer(m_viewer, SETTINGS, QSize(width, height));

    Q3DSViewerSettings *s = m_viewer->settings();

    QSignalSpy spy1(s, &Q3DSViewerSettings::matteColorChanged);
    QSignalSpy spy2(s, &Q3DSViewerSettings::showRenderStatsChanged);
    QSignalSpy spy4(s, &Q3DSViewerSettings::scaleModeChanged);
    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());
    QVERIFY(spy4.isValid());

    // Check defaults
    QCOMPARE(s->matteColor(), QColor(Qt::black));
    QCOMPARE(s->isShowRenderStats(), false);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeCenter);

    // Enable matte
    s->setMatteEnabled(true);

    // Matte
    checkPixel(m_viewer, Qt::black);

    s->setMatteColor(Qt::cyan);
    QCOMPARE(s->matteColor(), QColor(Qt::cyan));

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 0);
    QCOMPARE(spy4.count(), 0);

    checkPixel(m_viewer, Qt::cyan);

    // Render stats
    QImage image1 = m_viewer->grab();

    s->setShowRenderStats(true);
    QCOMPARE(s->isShowRenderStats(), true);

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy4.count(), 0);

    QImage image2 = m_viewer->grab();
    QVERIFY(image1 != image2);

    s->setShowRenderStats(false);
    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy4.count(), 0);

    // ScaleMode
    checkPixel(m_viewer, Qt::cyan);
    checkPixel(m_viewer, Qt::cyan, QPoint(50, height / 2));
    s->setScaleMode(Q3DSViewerSettings::ScaleModeFit);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeFit);

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy4.count(), 1);

    checkPixel(m_viewer, Qt::cyan);
    checkPixel(m_viewer, Qt::red, QPoint(50, height / 2));

    s->setScaleMode(Q3DSViewerSettings::ScaleModeFill);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeFill);

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy4.count(), 2);

    checkPixel(m_viewer, Qt::blue);
    checkPixel(m_viewer, Qt::red, QPoint(50, height / 2));

    // Saving & loading settings
    s->save(QStringLiteral("testViewer"), QStringLiteral("The Qt Company"),
            QStringLiteral("tst_q3dsurfaceviewer"));

    image1 = m_viewer->grab();

    s->setMatteColor(Qt::yellow);
    s->setShowRenderStats(true);
    s->setScaleMode(Q3DSViewerSettings::ScaleModeFit);

    QCOMPARE(s->matteColor(), QColor(Qt::yellow));
    QCOMPARE(s->isShowRenderStats(), true);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeFit);

    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy2.count(), 3);
    QCOMPARE(spy4.count(), 3);

    image2 = m_viewer->grab();

    s->load(QStringLiteral("testViewer"), QStringLiteral("The Qt Company"),
            QStringLiteral("tst_q3dsurfaceviewer"));

    QCOMPARE(s->matteColor(), QColor(Qt::cyan));
    QCOMPARE(s->isShowRenderStats(), false);
    QCOMPARE(s->scaleMode(), Q3DSViewerSettings::ScaleModeFill);

    QCOMPARE(spy1.count(), 3);
    QCOMPARE(spy2.count(), 4);
    QCOMPARE(spy4.count(), 4);

    QImage image3 = m_viewer->grab();
    QVERIFY(image1 != image2);
    QVERIFY(image3 != image2);
    QVERIFY(image1 == image3);

    // Clean up the settings so they don't pollute the system (and we have clean slate next time)
    QSettings(QStringLiteral("The Qt Company"), QStringLiteral("tst_q3dsurfaceviewer")).clear();
}

void tst_Q3DSSurfaceViewer::testPresentation_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testPresentation()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, MULTISLIDE);
    else
        createOffscreenAndViewer(m_viewer, MULTISLIDE);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QList<QVariant> args;
    Q3DSPresentation *p = m_viewer->presentation();
    QSignalSpy spy1(p, &Q3DSPresentation::slideEntered);
    QSignalSpy spy2(p, &Q3DSPresentation::slideExited);
    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());

    // There are three different "scenes":
    // The main Scene, three slides: S1, S2, S3
    // Two components on Scene.Layer:
    //   Scene.Layer.Component1, two slides: C1S1, C1S2
    //   Scene.Layer.Component2, two slides: C2S1, C2S2
    // The component slides also get enter when parent slide is entered, but they do not get
    // the corresponding exit if parent slide is exited.
    QString path = QStringLiteral("Scene");
    QString pathC1 = QStringLiteral("Scene.Layer.Component1");
    QString pathC2 = QStringLiteral("Scene.Layer.Component2");
    QPoint mainPoint(m_viewer->size().width() * 2 / 8, m_viewer->size().height() / 2);
    QPoint bgPoint(m_viewer->size().width() * 2 / 8, m_viewer->size().height() / 32);
    QPoint c1Point(m_viewer->size().width() * 5 / 8, m_viewer->size().height() / 2);
    QPoint c2Point(m_viewer->size().width() * 7 / 8, m_viewer->size().height() / 2);

    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    QCOMPARE(spy1.count(), 3);
    QCOMPARE(spy2.count(), 0);

    // String Attribute
    QImage image1 = m_viewer->grab();
    p->setAttribute(QStringLiteral("Scene.Layer.Text"),
                    QStringLiteral("textstring"), QStringLiteral("Test!"));
    QImage image2 = m_viewer->grab();
    QTRY_VERIFY(image1 != image2);

    // Float Attribute
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.r"), 0.0);
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.g"), 1.0);
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.b"), 1.0);

    checkPixel(m_viewer, Qt::cyan, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.r"), 1.0);
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.g"), 0.0);
    p->setAttribute(QStringLiteral("Scene.Layer.Rect.Material"),
                    QStringLiteral("diffuse.b"), 0.0);

    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    // Bool Attribute
    checkPixel(m_viewer, Qt::yellow, bgPoint);
    p->setAttribute(QStringLiteral("Scene"), QStringLiteral("bgcolorenable"), false);
    checkPixel(m_viewer, Qt::black, bgPoint);

    // Slide changes
    p->goToSlide(path, 2);
    checkPixel(m_viewer, Qt::blue, mainPoint);
    checkPixel(m_viewer, Qt::blue, c1Point);
    checkPixel(m_viewer, Qt::blue, c2Point);

    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 1);
    args = spy1.last();
    QCOMPARE(args.at(0).toString(), path);
    QCOMPARE(args.at(1).toInt(), 2);
    QCOMPARE(args.at(2).toString(), QStringLiteral("S2"));
    args = spy2.last();
    QCOMPARE(args.at(0).toString(), path);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toString(), QStringLiteral("S1"));

    // Time change
    p->goToTime(path, 7.0f);
    checkPixel(m_viewer, Qt::black, mainPoint);
    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 1);

    // More complex slide changes
    // Changing slide that is not visible should not trigger enter signals
    // The slides should still change, though, and become visible later when we switch back to S1
    p->goToSlide(pathC1, QStringLiteral("C1S2"));
    p->goToSlide(pathC2, QStringLiteral("C2S2"));
    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 1);

    p->goToSlide(path, QStringLiteral("S1"));
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::cyan, c1Point);
    checkPixel(m_viewer, Qt::magenta, c2Point);
    QCOMPARE(spy1.count(), 7);
    QCOMPARE(spy2.count(), 2);

    p->goToSlide(pathC1, QStringLiteral("C1S1"));
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::magenta, c2Point);
    QCOMPARE(spy1.count(), 8);
    QCOMPARE(spy2.count(), 3);

    args = spy1.last();
    QCOMPARE(args.at(0).toString(), pathC1);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toString(), QStringLiteral("C1S1"));
    args = spy2.last();
    QCOMPARE(args.at(0).toString(), pathC1);
    QCOMPARE(args.at(1).toInt(), 2);
    QCOMPARE(args.at(2).toString(), QStringLiteral("C1S2"));

    p->goToSlide(pathC2, QStringLiteral("C2S1"));
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy1.count(), 9);
    QCOMPARE(spy2.count(), 4);

    args = spy1.last();
    QCOMPARE(args.at(0).toString(), pathC2);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toString(), QStringLiteral("C2S1"));
    args = spy2.last();
    QCOMPARE(args.at(0).toString(), pathC2);
    QCOMPARE(args.at(1).toInt(), 2);
    QCOMPARE(args.at(2).toString(), QStringLiteral("C2S2"));

    p->goToSlide(path, true, true);
    checkPixel(m_viewer, Qt::blue, mainPoint);
    checkPixel(m_viewer, Qt::blue, c1Point);
    checkPixel(m_viewer, Qt::blue, c2Point);
    QCOMPARE(spy1.count(), 10);
    QCOMPARE(spy2.count(), 5);

    args = spy1.last();
    QCOMPARE(args.at(0).toString(), path);
    QCOMPARE(args.at(1).toInt(), 2);
    QCOMPARE(args.at(2).toString(), QStringLiteral("S2"));
    args = spy2.last();
    QCOMPARE(args.at(0).toString(), path);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toString(), QStringLiteral("S1"));

    p->goToSlide(path, false, true);
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy1.count(), 11);
    QCOMPARE(spy2.count(), 6);

    // No wrap, should not change
    p->goToSlide(path, false, false);
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy1.count(), 11);
    QCOMPARE(spy2.count(), 6);

    // Should wrap
    p->goToSlide(path, false, true);
    checkPixel(m_viewer, Qt::green, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::green, c2Point);
    QCOMPARE(spy1.count(), 12);
    QCOMPARE(spy2.count(), 7);

    // No wrap, should not change
    p->goToSlide(path, true, false);
    checkPixel(m_viewer, Qt::green, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::green, c2Point);
    QCOMPARE(spy1.count(), 12);
    QCOMPARE(spy2.count(), 7);

    // Should wrap
    p->goToSlide(path, true, true);
    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy1.count(), 13);
    QCOMPARE(spy2.count(), 8);
}

void tst_Q3DSSurfaceViewer::testPresentationActivation_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testPresentationActivation()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, ANIMATION);
    else
        createOffscreenAndViewer(m_viewer, ANIMATION);

    // Note: Presentation filename isn't default ID anymore, need to set manually.
    m_viewer->setPresentationId(QStringLiteral("animation"));
    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    {
        // Grab two images two seconds apart to verify animation is happening
        QImage image1 = m_viewer->grab();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grab();
        QVERIFY(image1 != image2);
    }

    m_viewer->presentation()->setPresentationActive(QStringLiteral("animation"), false);

    {
        // Grab two images two seconds apart to verify animation has stopped
        QImage image1 = m_viewer->grab();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grab();
        QVERIFY(image1 == image2);
    }

    m_viewer->presentation()->setPresentationActive(QStringLiteral("animation"), true);

    {
        // Grab two images two seconds apart to verify animation is happening
        QImage image1 = m_viewer->grab();
        QTest::qWait(2000);
        QImage image2 = m_viewer->grab();
        QVERIFY(image1 != image2);
    }
}

void tst_Q3DSSurfaceViewer::testSceneElement_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testSceneElement()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, MULTISLIDE);
    else
        createOffscreenAndViewer(m_viewer, MULTISLIDE);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QString path = QStringLiteral("Scene");
    QString pathC1 = QStringLiteral("Scene.Layer.Component1");
    QString pathC2 = QStringLiteral("Scene.Layer.Component2");

    Q3DSPresentation *p = m_viewer->presentation();
    Q3DSSceneElement *scene = new Q3DSSceneElement(path);
    Q3DSSceneElement *sceneC1 = new Q3DSSceneElement(pathC1);
    Q3DSSceneElement *sceneC2 = new Q3DSSceneElement(pathC2);
    QSignalSpy spy1(scene, &Q3DSSceneElement::currentSlideIndexChanged);
    QSignalSpy spy2(scene, &Q3DSSceneElement::previousSlideIndexChanged);
    QSignalSpy spy3(scene, &Q3DSSceneElement::currentSlideNameChanged);
    QSignalSpy spy4(scene, &Q3DSSceneElement::previousSlideNameChanged);
    QSignalSpy spy5(scene, &Q3DSSceneElement::elementPathChanged);
    QSignalSpy spy6(sceneC1, &Q3DSSceneElement::currentSlideIndexChanged);
    QSignalSpy spy7(sceneC1, &Q3DSSceneElement::previousSlideIndexChanged);
    QSignalSpy spy8(sceneC1, &Q3DSSceneElement::currentSlideNameChanged);
    QSignalSpy spy9(sceneC1, &Q3DSSceneElement::previousSlideNameChanged);
    QSignalSpy spy10(sceneC1, &Q3DSSceneElement::elementPathChanged);
    QSignalSpy spy11(sceneC2, &Q3DSSceneElement::currentSlideIndexChanged);
    QSignalSpy spy12(sceneC2, &Q3DSSceneElement::previousSlideIndexChanged);
    QSignalSpy spy13(sceneC2, &Q3DSSceneElement::currentSlideNameChanged);
    QSignalSpy spy14(sceneC2, &Q3DSSceneElement::previousSlideNameChanged);
    QSignalSpy spy15(sceneC2, &Q3DSSceneElement::elementPathChanged);
    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());
    QVERIFY(spy3.isValid());
    QVERIFY(spy4.isValid());
    QVERIFY(spy5.isValid());
    QVERIFY(spy6.isValid());
    QVERIFY(spy7.isValid());
    QVERIFY(spy8.isValid());
    QVERIFY(spy9.isValid());
    QVERIFY(spy10.isValid());
    QVERIFY(spy11.isValid());
    QVERIFY(spy12.isValid());
    QVERIFY(spy13.isValid());
    QVERIFY(spy14.isValid());
    QVERIFY(spy15.isValid());

    // Defaults
    QCOMPARE(scene->currentSlideIndex(), 0);
    QCOMPARE(scene->previousSlideIndex(), 0);
    QCOMPARE(scene->currentSlideName(), QStringLiteral(""));
    QCOMPARE(scene->previousSlideName(), QStringLiteral(""));
    QCOMPARE(scene->elementPath(), path);

    checkPixel(m_viewer, Qt::red);

    // Ensure we have no pending events to confuse spy counts
    QGuiApplication::processEvents();

    p->registerElement(scene);
    p->registerElement(sceneC1);
    p->registerElement(sceneC2);

    QCOMPARE(scene->currentSlideIndex(), 1);
    QCOMPARE(scene->previousSlideIndex(), 0);
    QCOMPARE(scene->currentSlideName(), QStringLiteral("S1"));
    QCOMPARE(scene->previousSlideName(), QStringLiteral(""));
    QCOMPARE(scene->elementPath(), path);

    p->goToSlide(path, QStringLiteral("S2"));
    checkPixel(m_viewer, Qt::blue);

    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy3.count(), 1);
    QCOMPARE(spy4.count(), 1);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 0);
    QCOMPARE(spy7.count(), 0);
    QCOMPARE(spy8.count(), 0);
    QCOMPARE(spy9.count(), 0);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 0);
    QCOMPARE(spy12.count(), 0);
    QCOMPARE(spy13.count(), 0);
    QCOMPARE(spy14.count(), 0);
    QCOMPARE(spy15.count(), 0);

    p->goToSlide(path, QStringLiteral("S1"));
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 0);
    QCOMPARE(spy7.count(), 0);
    QCOMPARE(spy8.count(), 0);
    QCOMPARE(spy9.count(), 0);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 0);
    QCOMPARE(spy12.count(), 0);
    QCOMPARE(spy13.count(), 0);
    QCOMPARE(spy14.count(), 0);
    QCOMPARE(spy15.count(), 0);

    p->goToSlide(pathC1, QStringLiteral("C1S2"));
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 1);
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 1);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 0);
    QCOMPARE(spy12.count(), 0);
    QCOMPARE(spy13.count(), 0);
    QCOMPARE(spy14.count(), 0);
    QCOMPARE(spy15.count(), 0);

    p->goToSlide(pathC2, QStringLiteral("C2S2"));
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 2);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 1);
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 1);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 1);
    QCOMPARE(spy12.count(), 1);
    QCOMPARE(spy13.count(), 1);
    QCOMPARE(spy14.count(), 1);
    QCOMPARE(spy15.count(), 0);

    // Subscenes revert to original slides when parent is re-entered
    p->goToSlide(path, QStringLiteral("S2"));
    checkPixel(m_viewer, Qt::blue);

    QCOMPARE(spy1.count(), 3);
    QCOMPARE(spy2.count(), 3);
    QCOMPARE(spy3.count(), 3);
    QCOMPARE(spy4.count(), 3);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 1);
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 1);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 1);
    QCOMPARE(spy12.count(), 1);
    QCOMPARE(spy13.count(), 1);
    QCOMPARE(spy14.count(), 1);
    QCOMPARE(spy15.count(), 0);

    p->goToSlide(path, QStringLiteral("S1"));
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 4);
    QCOMPARE(spy3.count(), 4);
    QCOMPARE(spy4.count(), 4);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 1);
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 1);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 1);
    QCOMPARE(spy12.count(), 1);
    QCOMPARE(spy13.count(), 1);
    QCOMPARE(spy14.count(), 1);
    QCOMPARE(spy15.count(), 0);

    p->unregisterElement(scene);
    p->unregisterElement(sceneC1);
    p->unregisterElement(sceneC2);

    // No more signals after unregistering
    p->goToSlide(path, QStringLiteral("S2"));
    checkPixel(m_viewer, Qt::blue);

    QCOMPARE(spy1.count(), 4);
    QCOMPARE(spy2.count(), 4);
    QCOMPARE(spy3.count(), 4);
    QCOMPARE(spy4.count(), 4);
    QCOMPARE(spy5.count(), 0);
    QCOMPARE(spy6.count(), 1);
    QCOMPARE(spy7.count(), 1);
    QCOMPARE(spy8.count(), 1);
    QCOMPARE(spy9.count(), 1);
    QCOMPARE(spy10.count(), 0);
    QCOMPARE(spy11.count(), 1);
    QCOMPARE(spy12.count(), 1);
    QCOMPARE(spy13.count(), 1);
    QCOMPARE(spy14.count(), 1);
    QCOMPARE(spy15.count(), 0);

    // Reregister
    p->registerElement(scene);
    p->goToSlide(path, QStringLiteral("S1"));
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 5);
    QCOMPARE(spy2.count(), 5);
    QCOMPARE(spy3.count(), 5);
    QCOMPARE(spy4.count(), 5);
    QCOMPARE(spy5.count(), 0);

    QCOMPARE(scene->currentSlideIndex(), 1);
    QCOMPARE(scene->previousSlideIndex(), 2);
    QCOMPARE(scene->currentSlideName(), QStringLiteral("S1"));
    QCOMPARE(scene->previousSlideName(), QStringLiteral("S2"));
    QCOMPARE(scene->elementPath(), path);

    scene->setCurrentSlideName(QStringLiteral("S2"));
    checkPixel(m_viewer, Qt::blue);

    QCOMPARE(spy1.count(), 6);
    QCOMPARE(spy2.count(), 6);

    scene->setCurrentSlideIndex(1);
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 7);
    QCOMPARE(spy2.count(), 7);

    // Go to next slide, wrap parameter doesn't matter
    scene->goToSlide(true, true);
    checkPixel(m_viewer, Qt::blue);

    QCOMPARE(spy1.count(), 8);
    QCOMPARE(spy2.count(), 8);

    // Go to next slide, wrap parameter doesn't matter
    scene->goToSlide(true, false);
    checkPixel(m_viewer, Qt::green);

    QCOMPARE(spy1.count(), 9);
    QCOMPARE(spy2.count(), 9);

    // No wrap, should not change
    scene->goToSlide(true, false);
    checkPixel(m_viewer, Qt::green);

    QCOMPARE(spy1.count(), 9);
    QCOMPARE(spy2.count(), 9);

    // Should wrap
    scene->goToSlide(true, true);
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 10);
    QCOMPARE(spy2.count(), 10);

    // No wrap, should not change
    scene->goToSlide(false, false);
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 10);
    QCOMPARE(spy2.count(), 10);

    // Should wrap
    scene->goToSlide(false, true);
    checkPixel(m_viewer, Qt::green);

    QCOMPARE(spy1.count(), 11);
    QCOMPARE(spy2.count(), 11);

    // Time change
    scene->goToTime(7.0f);
    checkPixel(m_viewer, Qt::yellow);

    QCOMPARE(spy1.count(), 11);
    QCOMPARE(spy2.count(), 11);

    // Back to first slide for further tests
    scene->setCurrentSlideIndex(1);
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 12);
    QCOMPARE(spy2.count(), 12);

    // Change element path
    scene->setElementPath(pathC1);
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 12);
    QCOMPARE(spy2.count(), 12);
    QCOMPARE(spy3.count(), 12);
    QCOMPARE(spy4.count(), 12);
    QCOMPARE(spy5.count(), 1);

    QCOMPARE(scene->currentSlideIndex(), 2);
    QCOMPARE(scene->previousSlideIndex(), 1);
    QCOMPARE(scene->currentSlideName(), QStringLiteral("C1S2"));
    QCOMPARE(scene->previousSlideName(), QStringLiteral("C1S1"));
    QCOMPARE(scene->elementPath(), pathC1);

    p->goToSlide(pathC1, QStringLiteral("C1S2"));
    checkPixel(m_viewer, Qt::red);

    QCOMPARE(spy1.count(), 12);
    QCOMPARE(spy2.count(), 13);
    QCOMPARE(spy3.count(), 12);
    QCOMPARE(spy4.count(), 13);
    QCOMPARE(spy5.count(), 1);
}

void tst_Q3DSSurfaceViewer::testElement_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testElement()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, MULTISLIDE);
    else
        createOffscreenAndViewer(m_viewer, MULTISLIDE);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QString path1 = QStringLiteral("Scene.Layer.Rect.Material"); // Red
    QString path2 = QStringLiteral("Scene.Layer.Component1.Rectangle4.Material"); // Green
    QString path3 = QStringLiteral("Scene.Layer.Component2.Rectangle6.Material"); // Yellow

    QPoint mainPoint(m_viewer->size().width() * 2 / 8, m_viewer->size().height() / 2);
    QPoint c1Point(m_viewer->size().width() * 5 / 8, m_viewer->size().height() / 2);
    QPoint c2Point(m_viewer->size().width() * 7 / 8, m_viewer->size().height() / 2);

    Q3DSPresentation *p = m_viewer->presentation();
    Q3DSElement *element1 = new Q3DSElement(p, path1);
    Q3DSElement *element2 = new Q3DSElement(p, path2);
    QSignalSpy spy2(element2, &Q3DSSceneElement::elementPathChanged);
    QVERIFY(spy2.isValid());

    checkPixel(m_viewer, Qt::red, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    element1->setAttribute(QStringLiteral("diffuse.r"), 0.0);
    element1->setAttribute(QStringLiteral("diffuse.g"), 0.0);
    element1->setAttribute(QStringLiteral("diffuse.b"), 1.0);
    checkPixel(m_viewer, Qt::blue, mainPoint);
    checkPixel(m_viewer, Qt::green, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    element2->setAttribute(QStringLiteral("diffuse.r"), 1.0);
    element2->setAttribute(QStringLiteral("diffuse.g"), 0.0);
    element2->setAttribute(QStringLiteral("diffuse.b"), 1.0);
    checkPixel(m_viewer, Qt::blue, mainPoint);
    checkPixel(m_viewer, Qt::magenta, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);

    // Change elementpath, nothing changes visually
    element2->setElementPath(path3);
    checkPixel(m_viewer, Qt::blue, mainPoint);
    checkPixel(m_viewer, Qt::magenta, c1Point);
    checkPixel(m_viewer, Qt::yellow, c2Point);
    QCOMPARE(spy2.count(), 1);

    element2->setAttribute(QStringLiteral("diffuse.r"), 0.0);
    element2->setAttribute(QStringLiteral("diffuse.g"), 1.0);
    element2->setAttribute(QStringLiteral("diffuse.b"), 1.0);
    checkPixel(m_viewer, Qt::blue, mainPoint);
    checkPixel(m_viewer, Qt::magenta, c1Point);
    checkPixel(m_viewer, Qt::cyan, c2Point);
}

void tst_Q3DSSurfaceViewer::testMouseInput_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testMouseInput()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, MOUSE);
    else
        createOffscreenAndViewer(m_viewer, MOUSE);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QPoint point1(m_viewer->size().width() * 1 / 4, m_viewer->size().height() / 2);
    QPoint point2(m_viewer->size().width() * 3 / 4, m_viewer->size().height() / 2);

    checkPixel(m_viewer, Qt::blue, point1);
    checkPixel(m_viewer, Qt::red, point2);

    QMouseEvent e1(QEvent::MouseButtonPress, point1, Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    m_viewer->presentation()->mousePressEvent(&e1);

    checkPixel(m_viewer, Qt::green, point1);
    checkPixel(m_viewer, Qt::red, point2);

    QMouseEvent e2(QEvent::MouseButtonRelease, point1, Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    m_viewer->presentation()->mouseReleaseEvent(&e2);

    checkPixel(m_viewer, Qt::blue, point1);
    checkPixel(m_viewer, Qt::red, point2);

    QMouseEvent e3(QEvent::MouseButtonPress, point2, Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    m_viewer->presentation()->mousePressEvent(&e3);

    checkPixel(m_viewer, Qt::blue, point1);
    checkPixel(m_viewer, Qt::blue, point2);

    QMouseEvent e4(QEvent::MouseButtonRelease, point2, Qt::LeftButton, Qt::LeftButton,
                   Qt::NoModifier);
    m_viewer->presentation()->mouseReleaseEvent(&e4);

    checkPixel(m_viewer, Qt::blue, point1);
    checkPixel(m_viewer, Qt::red, point2);

    // Note: No way yet to hook mouse move into anything in the presentation
}

void tst_Q3DSSurfaceViewer::testDataInput_data()
{
    testBasics_data();
}

void tst_Q3DSSurfaceViewer::testDataInput()
{
    QFETCH(bool, isWindow);

    if (isWindow)
        createWindowAndViewer(m_viewer, DATAINPUT);
    else
        createOffscreenAndViewer(m_viewer, DATAINPUT);

    m_viewer->settings()->setScaleMode(Q3DSViewerSettings::ScaleModeFill);

    QPoint point1(m_viewer->size().width() / 4, m_viewer->size().height() / 4);

    const QString animationName = QStringLiteral("animationInput");
    const QString slideName = QStringLiteral("slideInput");

    checkPixel(m_viewer, Qt::red, point1);
    m_viewer->presentation()->setDataInputValue(animationName, 90);
    checkPixel(m_viewer, Qt::blue, point1);
    m_viewer->presentation()->setDataInputValue(animationName, 10);
    checkPixel(m_viewer, Qt::red, point1);

    Q3DSDataInput *animationInput = new Q3DSDataInput();
    animationInput->setName(animationName);

    m_viewer->presentation()->registerDataInput(animationInput);
    QVERIFY(m_viewer->presentation()->registeredDataInput(animationInput->name()));

    Q3DSDataInput *slideInput = new Q3DSDataInput(m_viewer->presentation(), slideName);
    QVERIFY(m_viewer->presentation()->registeredDataInput(slideInput->name()));

    animationInput->setValue(90);
    checkPixel(m_viewer, Qt::blue, point1);
    animationInput->setValue(10);
    checkPixel(m_viewer, Qt::red, point1);

    slideInput->setValue(QStringLiteral("Slide2"));
    checkPixel(m_viewer, Qt::green, point1);
    slideInput->setValue(QStringLiteral("Slide1"));
    checkPixel(m_viewer, Qt::red, point1);

    m_viewer->presentation()->unregisterDataInput(animationInput);
    m_viewer->presentation()->unregisterDataInput(slideInput);
    QVERIFY(!m_viewer->presentation()->registeredDataInput(animationInput->name()));
    QVERIFY(!m_viewer->presentation()->registeredDataInput(slideInput->name()));
    delete animationInput;
    delete slideInput;
}


QTEST_MAIN(tst_Q3DSSurfaceViewer)

#include "tst_q3dssurfaceviewer.moc"
