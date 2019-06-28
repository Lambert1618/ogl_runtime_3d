build_online_docs: \
    QMAKE_DOCS = $$PWD/online/qtstudio3d.qdocconf
else: \
    QMAKE_DOCS = $$PWD/qtstudio3d.qdocconf

OTHER_FILES += \
    $$PWD/src/*.qdoc \
    $$PWD/src/*.html \
    $$PWD/src/images/*.png \
