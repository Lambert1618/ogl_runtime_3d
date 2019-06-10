TEMPLATE = subdirs
CONFIG += ordered

!macos:!win32: SUBDIRS += \
    qtextras

# TODO: Re-enable these tests after confirming CI can build & run them
#    runtime \
#    viewer \
#    studio3d
