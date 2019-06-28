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

#ifndef TST_QT3DSRUNTIME
#define TST_QT3DSRUNTIME

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>

namespace qt3ds {
class NVFoundation;
namespace foundation {
class CAllocator;
class IStringTable;
}
namespace render {
class NVRenderContext;
class NVRenderTestBase;
}
}

QT_BEGIN_NAMESPACE
class QOpenGLContext;
class QOffscreenSurface;
QT_END_NAMESPACE

class tst_qt3dsruntime : public QObject
{
    Q_OBJECT
public:
    tst_qt3dsruntime()
        : m_allocator(0)
        , m_foundation(0)
        , m_stringTable(0)
        , m_renderContext(0)
        , m_glContext(0)
        , m_glSurface(0)
    {
    }

private Q_SLOTS:
    void initTestCase();

    void testNVRenderTestClear();
    void testNVRenderTestPrimitives();
    void testNVRenderTestConstantBuffer();
    void testNVRenderTestBackendQuery();
    void testNVRenderTestTimerQuery();
    void testNVRenderTestFboMsaa();
    void testNVRenderTestTessellation();
    void testNVRenderTestGeometryShader();
    void testNVRenderTestComputeShader();
    void testNVRenderTestOcclusionQuery();
    void testNVRenderTestTexture2D();
    void testNVRenderTestAtomicCounterBuffer();
    void testNVRenderTestDrawIndirectBuffer();
    void testNVRenderTestAttribBuffers();
    void testNVRenderTestProgramPipeline();

    void testRenderEffectGenerator();

#if defined(QT_OPENGL_ES_2)
    void testRenderDefaultShaderGenerator_200es();
    void testRenderCustomShaderGenerator_200es();
#endif
#if defined(QT_OPENGL_ES_3)
    void testRenderDefaultShaderGenerator_300es();
    void testRenderCustomShaderGenerator_300es();
#if defined(QT_FEATURE_opengles31)
    void testRenderDefaultShaderGenerator_310es();
    void testRenderCustomShaderGenerator_310es();
#endif
#if defined(QT_FEATURE_opengles32)
    void testRenderDefaultShaderGenerator_320es();
    void testRenderCustomShaderGenerator_320es();
#endif
#endif

#if defined(QT_OPENGL_DYNAMIC)
    void testRenderDefaultShaderGenerator_300();
    void testRenderDefaultShaderGenerator_310();
    void testRenderDefaultShaderGenerator_320();
    void testRenderDefaultShaderGenerator_330();
    void testRenderDefaultShaderGenerator_400();
    void testRenderDefaultShaderGenerator_410();
    void testRenderDefaultShaderGenerator_420();
    void testRenderDefaultShaderGenerator_430();

    void testRenderCustomShaderGenerator_300();
    void testRenderCustomShaderGenerator_310();
    void testRenderCustomShaderGenerator_320();
    void testRenderCustomShaderGenerator_330();
    void testRenderCustomShaderGenerator_400();
    void testRenderCustomShaderGenerator_410();
    void testRenderCustomShaderGenerator_420();
    void testRenderCustomShaderGenerator_430();
#endif



private:
    bool executeTest(qt3ds::render::NVRenderTestBase *curTest,
        const QString &testName, bool peformPixelTest = true);
    bool init(QSurfaceFormat format);
    bool init();
    void runDefaultShaderGeneratorTest();
    void runCustomShaderGeneratorTest(QSurfaceFormat format);
    void cleanup();

    qt3ds::foundation::CAllocator *m_allocator;
    qt3ds::NVFoundation *m_foundation;
    qt3ds::foundation::IStringTable *m_stringTable;
    qt3ds::render::NVRenderContext *m_renderContext;
    QOpenGLContext *m_glContext;
    QOffscreenSurface *m_glSurface;
};

#endif // TST_QT3DSRUNTIME
