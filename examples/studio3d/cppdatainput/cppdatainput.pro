CONFIG += c++11
QT += widgets qml quick studio3d

target.path = $$[QT_INSTALL_EXAMPLES]/studio3d/$$TARGET
INSTALLS += target

SOURCES += \
        main.cpp

RESOURCES += \
    res.qrc
