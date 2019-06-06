# Add our static libs to target dependencies so we detect changes to them
defineReplace(fixLibPredeps) {
    # TODO Needs something more clever once debug libs are correctly suffixed
    for(lib, $${2}) {
        PREDEPS *= $${1}/$${QMAKE_PREFIX_STATICLIB}$${lib}$$qtPlatformTargetSuffix().$${QMAKE_EXTENSION_STATICLIB}
    }
    return($$PREDEPS)
}
