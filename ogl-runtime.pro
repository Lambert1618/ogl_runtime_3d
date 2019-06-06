requires(!ios)
requires(!integrity)
requires(!qnx)
requires(!tvos)
requires(!watchos)
requires(!winrt)
requires(!wasm)

ios|integrity|qnx|tvos|watchos|winrt|wasm|*-icc*: {
    message("WARNING, target excluded from ogl-runtime")
    #Exclude non-working cross-compile targets, see:
    # QT3DS-3645 ogl-runtime doesn't compile on INTEGRITY_11_04 in CI
    # QT3DS-3647 ogl-runtime doesn't compile on TvOS_ANY in CI
    # QT3DS-3648 ogl-runtime doesn't compile on WatchOS_ANY in CI
    # QT3DS-3646 ogl-runtime doesn't compile on IOS_ANY in CI
    # QT3DS-3649 ogl-runtime doesn't compile on WinRT in CI
    # QT3DS-3650 ogl-runtime doesn't compile on WebAssembly in CI
    # QT3DS-3652 ogl-runtime doesn't compile on QNX in CI
    TEMPLATE = subdirs
    CONFIG += ordered
    SUBDIRS += src_dummy
} else {
    contains(QMAKE_TARGET.os, WinRT)|contains(QMAKE_TARGET.os, TvOS)|contains(QMAKE_TARGET.os, IOS)|contains(QMAKE_TARGET.os, WatchOS)|contains(QMAKE_TARGET.os, INTEGRITY)|contains(QMAKE_TARGET.os, QNX) {
        message("WARNING, target OS excluded from ogl-runtime")
        TEMPLATE = subdirs
        CONFIG += ordered
        SUBDIRS += src_dummy
    } else {
        load(qt_parts)
        requires(qtHaveModule(opengl))
    }
}
