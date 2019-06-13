build_online_docs: \
    QMAKE_DOCS = $$PWD/online/qt3dstudio-opengl-runtime.qdocconf
else: \
    QMAKE_DOCS = $$PWD/qt3dstudio-opengl-runtime.qdocconf

OTHER_FILES += \
    $$PWD/src/*.qdoc \
    $$PWD/src/*.html \
    $$PWD/src/images/*.png \
