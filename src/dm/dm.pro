TEMPLATE = lib
TARGET = QT3DSDM
CONFIG += staticlib
include(../commoninclude.pri)

DEFINES += QT3DS_AUTHORING _AFXDLL PCRE_STATIC _UNICODE

linux {
    DEFINES += QT3DSDM_META_DATA_NO_SIGNALS
}

INCLUDEPATH += \
    \
    systems \
    systems/cores \
    ../system \
    ../foundation \
    ../runtimerender \
    ../datamodel \
    ../render \
    ../3rdparty/EASTL/UnknownVersion/include \
    ../3rdparty/color \

PRECOMPILED_HEADER = Qt3DSDMPrefix.h

SOURCES += \
    EASTL_new.cpp \
    systems/ActionSystem.cpp \
    systems/SignalsImpl.cpp \
    systems/SlideSystem.cpp \
    systems/StudioAnimationSystem.cpp \
    systems/StudioCoreSystem.cpp \
    systems/StudioFullSystem.cpp \
    systems/StudioPropertySystem.cpp \
    systems/Qt3DSDMComposerTypeDefinitions.cpp \
    systems/Qt3DSDMGuides.cpp \
    systems/Qt3DSDMMetaData.cpp \
    systems/Qt3DSDMSignalSystem.cpp \
    systems/Qt3DSDMStringTable.cpp \
    systems/Qt3DSDMValue.cpp \
    systems/Qt3DSDMXML.cpp \
    systems/cores/ActionCoreProducer.cpp \
    systems/cores/AnimationCoreProducer.cpp \
    systems/cores/DataCoreProducer.cpp \
    systems/cores/SimpleActionCore.cpp \
    systems/cores/SimpleAnimationCore.cpp \
    systems/cores/SimpleDataCore.cpp \
    systems/cores/SimpleSlideCore.cpp \
    systems/cores/SimpleSlideGraphCore.cpp \
    systems/cores/SlideCoreProducer.cpp \
    systems/cores/SlideGraphCoreProducer.cpp \

HEADERS += \
    systems/Qt3DSDMDataTypes.h \
    systems/Qt3DSDMMetaDataTypes.h \
    systems/Qt3DSDMMetaDataValue.h \
    systems/Qt3DSDMMetaData.h \
    systems/Qt3DSDMWStrOpsImpl.h
