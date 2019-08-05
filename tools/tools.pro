TEMPLATE = subdirs
CONFIG += ordered

!integrity:!qnx {
    SUBDIRS += viewer
}

win32 {
    SUBDIRS += attributehashes
}
