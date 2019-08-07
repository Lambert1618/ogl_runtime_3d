CONFIG += VIEWER_BUILD

include($$PWD/../../commoninclude.pri)

TEMPLATE = app
TARGET = Qt3DViewer
QT += studio3d-private
!android: {
    QT += qml quickcontrols2
}

INCLUDEPATH += $$PWD/../../src/api/studio3dqml
INCLUDEPATH += $$PWD/../../src/api/studio3d

RESOURCES += Viewer.qrc
RC_ICONS = resources/images/3D-studio-viewer.ico

ICON = resources/images/viewer.icns

SOURCES += \
    $$PWD/../../src/api/studio3dqml/q3dsstudio3d.cpp \
    $$PWD/../../src/api/studio3dqml/q3dsrenderer.cpp \
    $$PWD/../../src/api/studio3dqml/q3dspresentationitem.cpp \
    $$PWD/../../src/api/studio3dqml/q3dsruntimeinitializerthread.cpp \
    main.cpp \
    viewer.cpp \
    remotedeploymentreceiver.cpp

HEADERS += \
    $$PWD/../../src/api/studio3dqml/q3dsstudio3d_p.h \
    $$PWD/../../src/api/studio3dqml/q3dsrenderer_p.h \
    $$PWD/../../src/api/studio3dqml/q3dspresentationitem_p.h \
    $$PWD/../../src/api/studio3dqml/q3dsruntimeinitializerthread_p.h \
    viewer.h \
    remotedeploymentreceiver.h

android: {
SOURCES += \
    $$PWD/../../src/api/studio3d/q3dsviewersettings.cpp \
    $$PWD/../../src/api/studio3d/q3dspresentation.cpp \
    $$PWD/../../src/api/studio3d/q3dsdatainput.cpp

HEADERS += \
    $$PWD/../../src/api/studio3d/q3dsviewersettings.h \
    $$PWD/../../src/api/studio3d/q3dspresentation.h \
    $$PWD/../../src/api/studio3d/q3dsdatainput.h
}

LIBS += \
    -lqt3dsopengl$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

macos:QMAKE_RPATHDIR += @executable_path/../../../../lib

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}

contains(ANDROID_TARGET_ARCH,arm64-v8a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}
