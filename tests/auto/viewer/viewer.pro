TEMPLATE = app
CONFIG += testcase
include($$PWD/../../../commoninclude.pri)

TARGET = tst_qt3dsviewer
QT += testlib gui quick studio3d
RESOURCES += viewer.qrc

HEADERS += \
    tst_qt3dsviewer.h

SOURCES += \
    tst_qt3dsviewer.cpp

LIBS += \
    -lqt3dsopengl$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

ANDROID_EXTRA_LIBS = \
  libqt3dsopengl.so \
  libqt3dsqmlstreamer.so
