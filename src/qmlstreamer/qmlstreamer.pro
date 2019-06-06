TEMPLATE = lib
TARGET = qt3dsqmlstreamer
VERSION = $$MODULE_VERSION
include($$PWD/../../commonplatform.pri)

CONFIG += installed
load(qt_helper_lib)

# qt_helper_lib assumes non-qt lib, so it clears some qt variables, reset those
CONFIG += qt
QT += core qml quick opengl

DEFINES += QMLSTREAMER_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    q3dsqmlstream.cpp \
    q3dsqmlstreamerserviceimpl.cpp \
    q3dsqmlstreamproducer.cpp \
    q3dsqmlstreamproxy.cpp \
    q3dsqmlstreamrenderer.cpp \
    q3dsqmlsubpresentationsettings.cpp

HEADERS += \
    q3dsincubationcontroller.h \
    q3dsqmlstream.h \
    q3dsqmlstreamer_global.h \
    q3dsqmlstreamerserviceimpl.h \
    q3dsqmlstreamproducer.h \
    q3dsqmlstreamproxy.h \
    q3dsqmlstreamrenderer.h \
    q3dsqmlstreamservice.h \
    q3dsqmlsubpresentationsettings.h

INCLUDEPATH += \
    $$PWD/../Runtime/ogl-runtime/src/engine \
    $$PWD/../Runtime/ogl-runtime/src/system \
    $$PWD/../QtExtras/qmlstreamer

macos:QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
