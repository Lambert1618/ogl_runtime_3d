requires(!ios)
requires(!tvos)
requires(!watchos)
requires(!winrt)
requires(!wasm)

integrity {
    CHECK_INTEGRITY_DIR = $$(INTEGRITY_DIR)
}

ios|tvos|watchos|winrt|wasm|*-icc*|contains(CHECK_INTEGRITY_DIR, .*int1144$)|contains(QMAKE_HOST.arch, mips64) {
    message("WARNING, target not supported by ogl-runtime")
    #Exclude non-working cross-compile targets, see:
    # QT3DS-3647 ogl-runtime doesn't compile on TvOS_ANY in CI
    # QT3DS-3648 ogl-runtime doesn't compile on WatchOS_ANY in CI
    # QT3DS-3646 ogl-runtime doesn't compile on IOS_ANY in CI
    # QT3DS-3649 ogl-runtime doesn't compile on WinRT in CI
    # QT3DS-3650 ogl-runtime doesn't compile on WebAssembly in CI
    # QT3DS-4129 ogl-runtime doesn't compile on Qemu mips64 in CI
    TEMPLATE = subdirs
    CONFIG += ordered
    SUBDIRS += src_dummy
} else {
    load(qt_parts)
    requires(qtHaveModule(opengl))
}
