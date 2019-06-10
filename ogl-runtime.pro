requires(!ios)
requires(!qnx)
requires(!tvos)
requires(!watchos)
requires(!winrt)
requires(!wasm)

ios|qnx|tvos|watchos|winrt|wasm|*-icc*|!contains(INTEGRITY_DIR, *int1144$) {
    message("WARNING, target not supported by ogl-runtime")
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
    load(qt_parts)
    requires(qtHaveModule(opengl))
}
