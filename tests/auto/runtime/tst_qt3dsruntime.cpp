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
#include "tst_qt3dsruntime.h"

#include "render/Qt3DSRenderContext.h"
#include "foundation/TrackingAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSMat44.h"

#include "base/Qt3DSRenderTestClear.h"
#include "base/Qt3DSRenderTestPrimitives.h"
#include "base/Qt3DSRenderTestConstantBuffer.h"
#include "base/Qt3DSRenderTestBackendQuery.h"
#include "base/Qt3DSRenderTestTimerQuery.h"
#include "base/Qt3DSRenderTestTexture2D.h"
#include "base/Qt3DSRenderTestAtomicCounterBuffer.h"
#include "base/Qt3DSRenderTestDrawIndirectBuffer.h"
#include "base/Qt3DSRenderTestAttribBuffers.h"
#include "base/Qt3DSRenderTestProgramPipeline.h"
#include "fbo/Qt3DSRenderTestFboMsaa.h"
#include "geometry/Qt3DSRenderTestTessellation.h"
#include "geometry/Qt3DSRenderTestGeometryShader.h"
#include "geometry/Qt3DSRenderTestOcclusionQuery.h"
#include "compute/Qt3DSRenderTestComputeShader.h"
#include "shadergenerator/Qt3DSRenderTestDefaultMaterialGenerator.h"
#include "shadergenerator/Qt3DSRenderTestCustomMaterialGenerator.h"
#include "shadergenerator/Qt3DSRenderTestEffectGenerator.h"

#include <QImage>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>

using namespace std;
using namespace qt3ds;
using namespace qt3ds::render;
using namespace qt3ds::foundation;

extern "C" {
bool InitializeGL();
}

// Enable this to dump the test output into a log.txt file
//#define DUMP_LOGFILE

#ifndef EA_PLATFORM_WINDOWS

#ifndef EASTL_DEBUG_BREAK
void EASTL_DEBUG_BREAK()
{
    return;
}
#endif

namespace qt3ds {
void NVAssert(const char *exp, const char *file, int line, bool *ignore)
{
    *ignore = true;
    QString message = QString("failed: %1, file %2, line %3\n")
        .arg(exp).arg(file).arg(line);
    QFAIL(message.toLatin1().constData());
}
}
#endif

void messageOutput(QtMsgType type, const QMessageLogContext &context,
    const QString &msg)
{
    Q_UNUSED(context);
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg: {
#ifdef DUMP_LOGFILE
        QFile file("log.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream stream(&file);
            stream << msg;
        }
        file.close();
#endif
    } break; // swallow
    case QtFatalMsg:
        QFAIL(msg.toLocal8Bit().constData());
    }
}

void tst_qt3dsruntime::initTestCase()
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
}

QSurfaceFormat makeFormat(int major, int minor, bool gles = false, bool coreProfile = true)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setVersion(major, minor);
    if (coreProfile)
        format.setProfile(QSurfaceFormat::CoreProfile);
    else
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
    if (gles)
        format.setRenderableType(QSurfaceFormat::OpenGLES);
    return format;
}

bool tst_qt3dsruntime::init(QSurfaceFormat format)
{
    m_glContext = new QT_PREPEND_NAMESPACE(QOpenGLContext)(this);
    m_glContext->setFormat(format);
    bool success = m_glContext->create();
    if (!success)
        return false;

    m_glSurface = new QOffscreenSurface;
    m_glSurface->setFormat(format);
    m_glSurface->create();
    m_glContext->makeCurrent(m_glSurface);

    m_allocator = new CAllocator;
    m_foundation = NVCreateFoundation(QT3DS_FOUNDATION_VERSION, *m_allocator);
    m_stringTable = &IStringTable::CreateStringTable(*m_allocator);
    m_renderContext = &NVRenderContext::CreateGL(*m_foundation, *m_stringTable, format);
    return true;
}

bool tst_qt3dsruntime::init()
{
#if defined(QT_OPENGL_ES_2)
    return init(makeFormat(2, 0, true, false));
#elif defined(Q_OS_ANDROID) || defined(QT_OPENGL_ES_3)
    return init(makeFormat(3, 2, true, false));
#else
    return init(makeFormat(4, 3));
#endif
}

bool tst_qt3dsruntime::executeTest(NVRenderTestBase *curTest,
    const QString &testName,
    bool performPixelTest)
{
    bool success = true;
    int width = 640;
    int height = 480;
    userContextData userData = { (unsigned int)width, (unsigned int)height };

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    QOpenGLFramebufferObject *fbo = new QOpenGLFramebufferObject(QSize(width, height), fboFormat);

    m_renderContext->SetDefaultRenderTarget(fbo->handle());
    m_renderContext->SetDefaultDepthBufferBitCount(m_glContext->format().depthBufferSize());
    m_renderContext->SetViewport(NVRenderRect(0, 0, userData.winWidth, userData.winHeight));

    success = curTest->run(m_renderContext, &userData);

    if (performPixelTest) {
        QImage image = fbo->toImage();
        QImage refImage(QString(":/images/%1.png").arg(testName));
        refImage = refImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        if (!refImage.isNull()) {
            bool pixelTest = image == refImage;
            success &= pixelTest;
            if (!pixelTest)
                image.save(QString("%1_failed.png").arg(testName));
        }
    }

    curTest->cleanup(m_renderContext, &userData);

    return success;
}

void tst_qt3dsruntime::testNVRenderTestClear()
{
    init();
    NVRenderTestClear *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestClear);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    bool success = executeTest(test, "NVRenderTestClear");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestPrimitives()
{
    init();
    NVRenderTestPrimitives *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestPrimitives);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    bool success = executeTest(test, "NVRenderTestPrimitives");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestConstantBuffer()
{
    init();
    NVRenderTestConstantBuffer *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestConstantBuffer);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    bool success = executeTest(test, "NVRenderTestConstantBuffer");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestBackendQuery()
{
    init();
    NVRenderTestBackendQuery *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestBackendQuery);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    // TODO: Fix BOUL-332 to re-enable this pixel test
    bool success = executeTest(test, "NVRenderTestBackendQuery", false);
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestTimerQuery()
{
    init();
    NVRenderTestTimerQuery *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestTimerQuery);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    bool success = executeTest(test, "NVRenderTestTimerQuery");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestFboMsaa()
{
    init();
    NVRenderTestFboMsaa *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestFboMsaa);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    // TODO: Fix BOUL-332 to re-enable this pixel test
    bool success = executeTest(test, "NVRenderTestFboMsaa", false);
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestTessellation()
{
    init();
    NVRenderTestTessellation *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestTessellation);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    bool success = executeTest(test, "NVRenderTestTessellation");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestGeometryShader()
{
    init();
    NVRenderTestGeometryShader *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestGeometryShader);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    bool success = executeTest(test, "NVRenderTestGeometryShader");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestComputeShader()
{
    init();
    NVRenderTestComputeShader *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestComputeShader);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    // TODO: Fix BOUL-332 to re-enable this pixel test
    bool success = executeTest(test, "NVRenderTestComputeShader", false);
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestOcclusionQuery()
{
    init();
    NVRenderTestOcclusionQuery *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestOcclusionQuery);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    // TODO: Fix BOUL-332 to re-enable this pixel test
    bool success = executeTest(test, "NVRenderTestOcclusionQuery", false);
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestTexture2D()
{
    init();
    NVRenderTestTexture2D *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestTexture2D);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    // TODO: Fix BOUL-332 to re-enable this pixel test
    bool success = executeTest(test, "NVRenderTestTexture2D", false);
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestAtomicCounterBuffer()
{
    init();
    NVRenderTestAtomicCounterBuffer *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestAtomicCounterBuffer);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    bool success = executeTest(test, "NVRenderTestAtomicCounterBuffer");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestDrawIndirectBuffer()
{
    init();
    NVRenderTestDrawIndirectBuffer *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestDrawIndirectBuffer);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    // TODO: Fix BOUL-332 to re-enable this pixel test
    bool success = executeTest(test, "NVRenderTestDrawIndirectBuffer", false);
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestAttribBuffers()
{
    init();
    NVRenderTestAttribBuffers *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestAttribBuffers);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    // TODO: Fix BOUL-332 to re-enable this pixel test
    bool success = executeTest(test, "NVRenderTestAttribBuffers", false);
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testNVRenderTestProgramPipeline()
{
    init();
    NVRenderTestProgramPipeline *test =
        QT3DS_NEW(m_foundation->getAllocator(), NVRenderTestProgramPipeline);
    if (!test->isSupported(m_renderContext)) {
        QT3DS_FREE(m_foundation->getAllocator(), test);
        QSKIP("not supported");
    }
    // TODO: Fix BOUL-332 to re-enable this pixel test
    bool success = executeTest(test, "NVRenderTestProgramPipeline", false);
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

#if defined(QT_OPENGL_ES_2)
void tst_qt3dsruntime::testRenderDefaultShaderGenerator_200es()
{
    if (init(makeFormat(2, 0, true, false))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}
void tst_qt3dsruntime::testRenderCustomShaderGenerator_200es()
{
    runCustomShaderGeneratorTest(makeFormat(2, 0, true, false));
    cleanup();
}
#endif

#if defined(QT_OPENGL_ES_3)
void tst_qt3dsruntime::testRenderDefaultShaderGenerator_300es()
{
    if (init(makeFormat(3, 0, true, false))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}
void tst_qt3dsruntime::testRenderCustomShaderGenerator_300es()
{
    runCustomShaderGeneratorTest(makeFormat(3, 0, true, false));
    cleanup();
}

#if defined(QT_FEATURE_opengles31)
void tst_qt3dsruntime::testRenderDefaultShaderGenerator_310es()
{
    if (init(makeFormat(3, 1, true, false))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}
void tst_qt3dsruntime::testRenderCustomShaderGenerator_310es()
{
    runCustomShaderGeneratorTest(makeFormat(3, 1, true, false));
    cleanup();
}
#endif
#if defined(QT_FEATURE_opengles32)
void tst_qt3dsruntime::testRenderDefaultShaderGenerator_320es()
{
    if (init(makeFormat(3, 1, true, false))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}
void tst_qt3dsruntime::testRenderCustomShaderGenerator_320es()
{
    runCustomShaderGeneratorTest(makeFormat(3, 1, true, false));
    cleanup();
}

#endif
#endif

#if defined(QT_OPENGL_DYNAMIC)
void tst_qt3dsruntime::testRenderDefaultShaderGenerator_300()
{
    QSKIP("OpenGL 3.0 is not supported");
    if (init(makeFormat(3, 0))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}

void tst_qt3dsruntime::testRenderDefaultShaderGenerator_310()
{
    if (init(makeFormat(3, 1))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}

void tst_qt3dsruntime::testRenderDefaultShaderGenerator_320()
{
    if (init(makeFormat(3, 2))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}

void tst_qt3dsruntime::testRenderDefaultShaderGenerator_330()
{
    if (init(makeFormat(3, 3))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}

void tst_qt3dsruntime::testRenderDefaultShaderGenerator_400()
{
    if (init(makeFormat(4, 0))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}

void tst_qt3dsruntime::testRenderDefaultShaderGenerator_410()
{
    if (init(makeFormat(4, 1))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}

void tst_qt3dsruntime::testRenderDefaultShaderGenerator_420()
{
    if (init(makeFormat(4, 2))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}

void tst_qt3dsruntime::testRenderDefaultShaderGenerator_430()
{
    if (init(makeFormat(4, 3))) {
        runDefaultShaderGeneratorTest();
        cleanup();
    }
}

void tst_qt3dsruntime::testRenderCustomShaderGenerator_300()
{
    QSKIP("OpenGL 3.0 is not supported");
    runCustomShaderGeneratorTest(makeFormat(3, 0));
}

void tst_qt3dsruntime::testRenderCustomShaderGenerator_310()
{
    runCustomShaderGeneratorTest(makeFormat(3, 1));
}

void tst_qt3dsruntime::testRenderCustomShaderGenerator_320()
{
    runCustomShaderGeneratorTest(makeFormat(3, 2));
}

void tst_qt3dsruntime::testRenderCustomShaderGenerator_330()
{
    runCustomShaderGeneratorTest(makeFormat(3, 3));
}

void tst_qt3dsruntime::testRenderCustomShaderGenerator_400()
{
    runCustomShaderGeneratorTest(makeFormat(4, 0));
}

void tst_qt3dsruntime::testRenderCustomShaderGenerator_410()
{
    runCustomShaderGeneratorTest(makeFormat(4, 1));
}

void tst_qt3dsruntime::testRenderCustomShaderGenerator_420()
{
    runCustomShaderGeneratorTest(makeFormat(4, 2));
}

void tst_qt3dsruntime::testRenderCustomShaderGenerator_430()
{
    runCustomShaderGeneratorTest(makeFormat(4, 3));
}

#endif

void tst_qt3dsruntime::runDefaultShaderGeneratorTest()
{
    Qt3DSRenderTestDefaultMaterialGenerator *test =
        QT3DS_NEW(m_foundation->getAllocator(), Qt3DSRenderTestDefaultMaterialGenerator);
    if (!test->isSupported(m_renderContext))
        QSKIP("not supported");
    bool success = executeTest(test, "Qt3DSRenderTestDefaultMaterialGenerator");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
}

void tst_qt3dsruntime::runCustomShaderGeneratorTest(QSurfaceFormat format)
{
    m_glContext = new QT_PREPEND_NAMESPACE(QOpenGLContext)(this);
    m_glContext->setFormat(format);
    bool success = m_glContext->create();
    if (!success)
        return;

    m_glSurface = new QOffscreenSurface;
    m_glSurface->setFormat(format);
    m_glSurface->create();
    m_glContext->makeCurrent(m_glSurface);

    m_allocator = new CAllocator;
    m_foundation = NVCreateFoundation(QT3DS_FOUNDATION_VERSION, *m_allocator);

    Qt3DSRenderTestCustomMaterialGenerator *test =
        QT3DS_NEW(m_foundation->getAllocator(), Qt3DSRenderTestCustomMaterialGenerator);

    test->initializeQt3DSRenderer(format);
    m_renderContext = &NVRenderContext::CreateGL(*m_foundation, test->qt3dsRenderer()->GetContext()
                                                 .GetStringTable(), format);

    if (!test->isSupported(m_renderContext))
        QSKIP("not supported");
    success = executeTest(test, "Qt3DSRenderTestCusromMaterialGenerator");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::testRenderEffectGenerator()
{
    QSurfaceFormat format = makeFormat(4, 3);
    m_glContext = new QT_PREPEND_NAMESPACE(QOpenGLContext)(this);
    m_glContext->setFormat(format);
    bool success = m_glContext->create();
    if (!success)
        return;

    m_glSurface = new QOffscreenSurface;
    m_glSurface->setFormat(format);
    m_glSurface->create();
    m_glContext->makeCurrent(m_glSurface);

    m_allocator = new CAllocator;
    m_foundation = NVCreateFoundation(QT3DS_FOUNDATION_VERSION, *m_allocator);

    Qt3DSRenderTestEffectGenerator *test =
        QT3DS_NEW(m_foundation->getAllocator(), Qt3DSRenderTestEffectGenerator);

    test->initializeQt3DSRenderer(format);
    m_renderContext = &NVRenderContext::CreateGL(*m_foundation, test->qt3dsRenderer()->GetContext()
                                                 .GetStringTable(), format);

    if (!test->isSupported(m_renderContext))
        QSKIP("not supported");
    success = executeTest(test, "Qt3DSRenderTestEffectGenerator");
    QT3DS_FREE(m_foundation->getAllocator(), test);
    test = 0;
    QVERIFY(success);
    cleanup();
}

void tst_qt3dsruntime::cleanup()
{
    if (m_renderContext)
        m_renderContext->release();
    if (m_foundation)
        m_foundation->release();

    m_renderContext = 0;
    m_stringTable = 0;
    m_foundation = 0;

    delete m_allocator;
    m_allocator = 0;

    m_glSurface->destroy();
    delete m_glSurface;
    m_glSurface = 0;

    delete m_glContext;
    m_glContext = 0;
}

QTEST_MAIN(tst_qt3dsruntime)
