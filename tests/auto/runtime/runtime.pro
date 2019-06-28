TEMPLATE = app
CONFIG += testcase
include($$PWD/../../../commoninclude.pri)

TARGET = tst_qt3dsruntime
QT += testlib gui
QT += quick-private

RESOURCES += \
    runtime.qrc

INCLUDEPATH += \
    $$PWD/../../../src/Qt3DSRuntimeRender/RendererImpl

HEADERS += \
    base/Qt3DSRenderTestAtomicCounterBuffer.h \
    base/Qt3DSRenderTestAttribBuffers.h \
    base/Qt3DSRenderTestBackendQuery.h \
    base/Qt3DSRenderTestClear.h \
    base/Qt3DSRenderTestConstantBuffer.h \
    base/Qt3DSRenderTestDrawIndirectBuffer.h \
    base/Qt3DSRenderTestPrimitives.h \
    base/Qt3DSRenderTestProgramPipeline.h \
    base/Qt3DSRenderTestTexture2D.h \
    base/Qt3DSRenderTestTimerQuery.h \
    compute/Qt3DSRenderTestComputeShader.h \
    fbo/Qt3DSRenderTestFboMsaa.h \
    geometry/Qt3DSRenderTestGeometryShader.h \
    geometry/Qt3DSRenderTestOcclusionQuery.h \
    geometry/Qt3DSRenderTestTessellation.h \
    Qt3DSRenderTestBase.h \
    Qt3DSRenderTestMathUtil.h \
    tst_qt3dsruntime.h \
    shadergenerator/Qt3DSRenderTestDefaultMaterialGenerator.h \
    shadergenerator/Qt3DSRenderTestCustomMaterialGenerator.h \
    shadergenerator/Qt3DSRenderTestEffectGenerator.h

SOURCES += \
    base/Qt3DSRenderTestAtomicCounterBuffer.cpp \
    base/Qt3DSRenderTestAttribBuffers.cpp \
    base/Qt3DSRenderTestBackendQuery.cpp \
    base/Qt3DSRenderTestClear.cpp \
    base/Qt3DSRenderTestConstantBuffer.cpp \
    base/Qt3DSRenderTestDrawIndirectBuffer.cpp \
    base/Qt3DSRenderTestPrimitives.cpp \
    base/Qt3DSRenderTestProgramPipeline.cpp \
    base/Qt3DSRenderTestTexture2D.cpp \
    base/Qt3DSRenderTestTimerQuery.cpp \
    compute/Qt3DSRenderTestComputeShader.cpp \
    fbo/Qt3DSRenderTestFboMsaa.cpp \
    geometry/Qt3DSRenderTestGeometryShader.cpp \
    geometry/Qt3DSRenderTestOcclusionQuery.cpp \
    geometry/Qt3DSRenderTestTessellation.cpp \
    Qt3DSRenderTestMathUtil.cpp \
    tst_qt3dsruntime.cpp \
    Qt3DSRenderTestBase.cpp \
    shadergenerator/Qt3DSRenderTestDefaultMaterialGenerator.cpp \
    shadergenerator/Qt3DSRenderTestCustomMaterialGenerator.cpp \
    shadergenerator/Qt3DSRenderTestEffectGenerator.cpp

linux {
    BEGIN_ARCHIVE = -Wl,--whole-archive
    END_ARCHIVE = -Wl,--no-whole-archive
}

LIBS += \
    -lqt3dsopengl$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

ANDROID_EXTRA_LIBS = \
  libqt3dsqmlstreamer.so

win32 {
    LIBS += \
        -lws2_32
}

linux {
    LIBS += \
        -ldl \
        -lEGL
}
