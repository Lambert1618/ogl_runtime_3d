include($$PWD/../../commoninclude.pri)

QT += qml quick opengl studio3d-private
CONFIG += plugin

qtHaveModule(multimedia) {
DEFINES += PLATFORM_HAS_QT_MULTIMEDIA_LIB
QT += multimedia
}

TARGET = qtstudio3dopengl
TARGETPATH = QtStudio3D/OpenGL
IMPORT_VERSION = 2.4

SOURCES += \
    q3dsplugin.cpp \
    q3dsstudio3d.cpp \
    q3dsrenderer.cpp \
    q3dspresentationitem.cpp

HEADERS += \
    q3dsplugin.h \
    q3dsrenderer_p.h \
    q3dsstudio3d_p.h \
    q3dspresentationitem_p.h

LIBS += \
    -lqt3dsopengl$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

OTHER_FILES += \
  qmldir

load(qml_plugin)
