TEMPLATE = app

QT += widgets qml quick studio3d

SOURCES += \
    main.cpp

RESOURCES += simpleqml.qrc

OTHER_FILES += \
    main.qml \
    doc/src/* \
    doc/images/*

target.path = $$[QT_INSTALL_EXAMPLES]/studio3d/$$TARGET
INSTALLS += target
