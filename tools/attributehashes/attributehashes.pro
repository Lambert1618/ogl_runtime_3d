TEMPLATE = app
TARGET = AttributeHashes
CONFIG += console

win32 {
LIBS += \
    -lws2_32
}

include(../../commoninclude.pri)

SOURCES += AttributeHashes.cpp

load(qt_tool)

INSTALLS -= target
