contains(CONFIG, VIEWER_BUILD) {
    android: {
        CONFIG(debug, debug|release) {
            DEFINES += _DEBUG
        } else {
            DEFINES += NDEBUG
        }
        DEFINES += _LINUX QT3DS_OS_LINUX _LINUXPLATFORM
    } else {
        include($$PWD/commonplatform.pri)
    }
} else {
    include($$PWD/commonplatform.pri)
}

contains(TEMPLATE, lib) {
    load(qt_helper_lib)
    # qt_helper_lib assumes non-qt lib, so it clears qt from config - reset that
    CONFIG += qt exceptions
}
QT += core gui openglextensions

DEFINES += COMPILED_FROM_DSP QT3DSDM_USE_NVLOG QT3DSDM_META_DATA_NO_SIGNALS

INCLUDEPATH += \
    $$PWD/src \
    $$PWD/src/datamodel \
    $$PWD/src/runtime \
    $$PWD/src/system \
    $$PWD/src/engine \
    $$PWD/src/foundation \
    $$PWD/src/render \
    $$PWD/src/render/gl2 \
    $$PWD/src/render/gl3 \
    $$PWD/src/render/glg \
    $$PWD/src/uipparser \
    $$PWD/src/state \
    $$PWD/src/event \
    $$PWD/src/viewer \
    $$PWD/src/viewer/perflog \
    $$PWD/src/importlib \
    $$PWD/src/dm \
    $$PWD/src/dm/systems \
    $$PWD/src/dm/systems/cores \
    $$PWD/src/3rdparty/EASTL/UnknownVersion/include \
    $$PWD/src/3rdparty/platformspecific/$$PlatformSpecificDir/PlatformLibs \
    $$PWD/src/qmlstreamer \
    $$PWD/src/runtimerender \
    $$PWD/src/runtimerender/graphobjects \
    $$PWD/src/runtimerender/resourcemanager

# TODO: Investigate whether these can be moved to commonplatform
win32-msvc {
    CONFIG += nostrictstrings
    QMAKE_CXXFLAGS += /EHsc /GA
    QMAKE_CFLAGS += /EHsc /GA
}

win32 {
INCLUDEPATH += \
    $$PWD/src/platformspecific/windows/libs \
    $$PWD/src/3rdparty/platformspecific/Windows/Qt3DSLibs
}

linux|qnx {
QMAKE_CXXFLAGS += -fpermissive
QMAKE_CFLAGS += -fpermissive
INCLUDEPATH += \
    $$PWD/src/foundation/linux \
    $$PWD/src/platformspecific/linux/libs \
    $$PWD/src/3rdparty/platformspecific/Linux/Qt3DSLibs
}

integrity {
INCLUDEPATH += \
    $$PWD/src/foundation/linux \
    $$PWD/src/platformspecific/linux/libs \
    $$PWD/src/3rdparty/platformspecific/Linux/Qt3DSLibs
}

macos {
INCLUDEPATH += \
  $$PWD/src/3rdparty/platformspecific/Macos/Qt3DSLibs \
  $$PWD/src/platformspecific/macos/libs
}
