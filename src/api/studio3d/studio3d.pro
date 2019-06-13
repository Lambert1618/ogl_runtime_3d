TARGET = QtStudio3D

include($$PWD/../../commoninclude.pri)
QT += opengl widgets qml

include($$PWD/doc/doc.pri)

qtHaveModule(multimedia) {
DEFINES += PLATFORM_HAS_QT_MULTIMEDIA_LIB
QT += multimedia
}
CONFIG += console

LIBS += \
    -lqt3dsopengl$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

HEADERS += \
    q3dsdataoutput.h \
    q3dsdataoutput_p.h \
    q3dssurfaceviewer.h \
    q3dssurfaceviewer_p.h \
    qstudio3dglobal.h \
    viewerqmlstreamproxy_p.h \
    q3dsviewersettings.h \
    q3dsviewersettings_p.h \
    q3dspresentation.h \
    q3dspresentation_p.h \
    q3dssceneelement.h \
    q3dssceneelement_p.h \
    q3dselement.h \
    q3dselement_p.h \
    studioutils_p.h \
    q3dscommandqueue_p.h \
    q3dsimagesequencegenerator_p.h \
    q3dsimagesequencegeneratorthread_p.h \
    q3dsdatainput.h \
    q3dsdatainput_p.h \
    q3dsgeometry.h \
    q3dsgeometry_p.h

SOURCES += q3dsdataoutput.cpp \
           q3dssurfaceviewer.cpp \
           viewerqmlstreamproxy.cpp \
           q3dsviewersettings.cpp \
           q3dspresentation.cpp \
           q3dssceneelement.cpp \
           q3dselement.cpp \
           studioutils.cpp \
           q3dscommandqueue.cpp \
           q3dsimagesequencegenerator.cpp \
           q3dsimagesequencegeneratorthread.cpp \
           q3dsdatainput.cpp \
           q3dsgeometry.cpp

# Platform specific surface format discovery functions
#TODO: QT3DS-3608 Implement OpenGL version discovery for Windows
#TODO: QT3DS-3609 Implement OpenGL version discovery for Linux
#TODO: QT3DS-3610 Implement OpenGL version discovery for QNX
#TODO: QT3DS-3611 Implement OpenGL version discovery for Integrity
#TODO: QT3DS-3612 Implement OpenGL version discovery for macOS
#TODO: QT3DS-3613 Implement OpenGL version discovery for Android
SOURCES += \
    ../../foundation/qt/formatdiscovery.cpp

load(qt_module)
