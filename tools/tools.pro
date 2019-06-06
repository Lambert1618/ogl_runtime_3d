TEMPLATE = subdirs
CONFIG += ordered

!boot2qt:!integrity:!qnx {
    SUBDIRS += viewer
}

win32 {
    SUBDIRS += attributehashes
}
