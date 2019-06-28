TEMPLATE = app
CONFIG += testcase
include($$PWD/../../../../commonplatform.pri)

TARGET = tst_qt3dsqmlstream

QT += testlib quick

SOURCES += tst_qt3dsqmlstream.cpp

INCLUDEPATH += \
    $$PWD/../../../../src/qmlstreamer

LIBS += \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

ANDROID_EXTRA_LIBS = \
  libqt3dsqmlstreamer.so
