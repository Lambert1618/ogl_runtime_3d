TEMPLATE = app

QT += qml quick studio3d

SOURCES += \
        demo.cpp \
        main.cpp

HEADERS += \
    demo.h

RESOURCES += dynamicelement.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/studio3d/$$TARGET
INSTALLS += target
