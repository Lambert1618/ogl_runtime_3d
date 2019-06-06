TEMPLATE = subdirs

SUBDIRS += \
    cppdatainput

qtHaveModule(quick) {
    SUBDIRS += simpleqml \
               qmldatainput
}
