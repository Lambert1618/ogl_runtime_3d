/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include <memory>
#include "Qt3DSDMDataCore.h"
#include "SimpleDataCore.h"
#include "DataCoreProducer.h"
#include "Qt3DSDMSlideCore.h"

#include <QtGlobal>

using namespace qt3dsdm;
using std::make_shared;
using std::shared_ptr;

#ifdef _WIN32
#pragma warning(disable : 4100)
#endif

// SComposerTypePropertyDefinition constructors

namespace {

template <typename TDataType>
struct DataConstructor
{
    template <typename TInputType>
    TDataType Construct(const TInputType &inInput)
    {
        return TDataType(inInput);
    }
};

template <>
struct DataConstructor<SLong4>
{
    SLong4 Construct(int inValue)
    {
        unsigned long theValue((unsigned long)inValue);
        return SLong4(theValue, theValue, theValue, theValue);
    }
};

template <>
struct DataConstructor<TDataStrPtr>
{
    TDataStrPtr Construct(const wchar_t *inData)
    {
        if (IsTrivial(inData))
            return make_shared<CDataStr>();
        return make_shared<CDataStr>(inData);
    }
};
template <>
struct DataConstructor<SStringOrInt>
{
    SStringOrInt Construct(const wchar_t *inData)
    {
        if (IsTrivial(inData))
            return make_shared<CDataStr>();
        return make_shared<CDataStr>(inData);
    }
};
template <>
struct DataConstructor<SObjectRefType>
{
    SObjectRefType Construct(const wchar_t *) { return SObjectRefType(); }
};
}

#define QT3DS_WCHAR_T_Typed L"Typed"
#define QT3DS_WCHAR_T_Guided L"Guided"
#define QT3DS_WCHAR_T_Named L"Named"
#define QT3DS_WCHAR_T_SlideOwner L"SlideOwner"
#define QT3DS_WCHAR_T_Slide L"Slide"
#define QT3DS_WCHAR_T_Action L"Action"
#define QT3DS_WCHAR_T_Asset L"Asset"
#define QT3DS_WCHAR_T_Scene L"Scene"
#define QT3DS_WCHAR_T_Image L"Image"
#define QT3DS_WCHAR_T_Material L"Material"
#define QT3DS_WCHAR_T_Behavior L"Behavior"
#define QT3DS_WCHAR_T_Node L"Node"
#define QT3DS_WCHAR_T_Layer L"Layer"
#define QT3DS_WCHAR_T_Group L"Group"
#define QT3DS_WCHAR_T_Model L"Model"
#define QT3DS_WCHAR_T_Light L"Light"
#define QT3DS_WCHAR_T_Camera L"Camera"
#define QT3DS_WCHAR_T_Component L"Component"
#define QT3DS_WCHAR_T_Text L"Text"
#define QT3DS_WCHAR_T_Effect L"Effect"
#define QT3DS_WCHAR_T_RenderPlugin L"RenderPlugin"
#define QT3DS_WCHAR_T_MaterialBase L"MaterialBase"
#define QT3DS_WCHAR_T_CustomMaterial L"CustomMaterial"
#define QT3DS_WCHAR_T_ReferencedMaterial L"ReferencedMaterial"
#define QT3DS_WCHAR_T_Alias L"Alias"
#define QT3DS_WCHAR_T_Lightmaps L"Lightmaps"

#define QT3DS_WCHAR_T_type L"type"
#define QT3DS_WCHAR_T_id L"id"
#define QT3DS_WCHAR_T_name L"name"
#define QT3DS_WCHAR_T_componentid L"componentid"
#define QT3DS_WCHAR_T_playmode L"playmode"
#define QT3DS_WCHAR_T_playthroughto L"playthroughto"
#define QT3DS_WCHAR_T_initialplaystate L"initialplaystate"
#define QT3DS_WCHAR_T_actioneyeball L"actioneyeball"
#define QT3DS_WCHAR_T_sourcepath L"sourcepath"
#define QT3DS_WCHAR_T_importid L"importid"
#define QT3DS_WCHAR_T_starttime L"starttime"
#define QT3DS_WCHAR_T_endtime L"endtime"
#define QT3DS_WCHAR_T_eyeball L"eyeball"
#define QT3DS_WCHAR_T_shy L"shy"
#define QT3DS_WCHAR_T_locked L"locked"
#define QT3DS_WCHAR_T_timebarcolor L"timebarcolor"
#define QT3DS_WCHAR_T_timebartext L"timebartext"
#define QT3DS_WCHAR_T_bgcolorenable L"bgcolorenable"
#define QT3DS_WCHAR_T_background L"background"
#define QT3DS_WCHAR_T_backgroundcolor L"backgroundcolor"
#define QT3DS_WCHAR_T_blendtype L"blendtype"
#define QT3DS_WCHAR_T_scaleu L"scaleu"
#define QT3DS_WCHAR_T_scalev L"scalev"
#define QT3DS_WCHAR_T_mappingmode L"mappingmode"
#define QT3DS_WCHAR_T_tilingmodehorz L"tilingmodehorz"
#define QT3DS_WCHAR_T_tilingmodevert L"tilingmodevert"
#define QT3DS_WCHAR_T_rotationuv L"rotationuv"
#define QT3DS_WCHAR_T_positionu L"positionu"
#define QT3DS_WCHAR_T_positionv L"positionv"
#define QT3DS_WCHAR_T_pivotu L"pivotu"
#define QT3DS_WCHAR_T_pivotv L"pivotv"
#define QT3DS_WCHAR_T_subpresentation L"subpresentation"
#define QT3DS_WCHAR_T_iblprobe L"iblprobe"
#define QT3DS_WCHAR_T_shaderlighting L"shaderlighting"
#define QT3DS_WCHAR_T_blendmode L"blendmode"
#define QT3DS_WCHAR_T_vertexcolors L"vertexcolors"
#define QT3DS_WCHAR_T_diffuse L"diffuse"
#define QT3DS_WCHAR_T_diffusemap L"diffusemap"
#define QT3DS_WCHAR_T_diffusemap2 L"diffusemap2"
#define QT3DS_WCHAR_T_diffusemap3 L"diffusemap3"
#define QT3DS_WCHAR_T_specularreflection L"specularreflection"
#define QT3DS_WCHAR_T_specularmap L"specularmap"
#define QT3DS_WCHAR_T_specularmodel L"specularmodel"
#define QT3DS_WCHAR_T_speculartint L"speculartint"
#define QT3DS_WCHAR_T_ior L"ior"
#define QT3DS_WCHAR_T_specularamount L"specularamount"
#define QT3DS_WCHAR_T_specularroughness L"specularroughness"
#define QT3DS_WCHAR_T_roughnessmap L"roughnessmap"
#define QT3DS_WCHAR_T_opacitymap L"opacitymap"
#define QT3DS_WCHAR_T_emissivepower L"emissivepower"
#define QT3DS_WCHAR_T_emissivecolor L"emissivecolor"
#define QT3DS_WCHAR_T_emissivemap L"emissivemap"
#define QT3DS_WCHAR_T_emissivemap2 L"emissivemap2"
#define QT3DS_WCHAR_T_normalmap L"normalmap"
#define QT3DS_WCHAR_T_normalstrength L"normalstrength"
#define QT3DS_WCHAR_T_position L"position"
#define QT3DS_WCHAR_T_rotation L"rotation"
#define QT3DS_WCHAR_T_scale L"scale"
#define QT3DS_WCHAR_T_pivot L"pivot"
#define QT3DS_WCHAR_T_opacity L"opacity"
#define QT3DS_WCHAR_T_rotationorder L"rotationorder"
#define QT3DS_WCHAR_T_orientation L"orientation"
#define QT3DS_WCHAR_T_progressiveaa L"progressiveaa"
#define QT3DS_WCHAR_T_multisampleaa L"multisampleaa"
#define QT3DS_WCHAR_T_disabledepthtest L"disabledepthtest"
#define QT3DS_WCHAR_T_disabledepthprepass L"disabledepthprepass"
#define QT3DS_WCHAR_T_layerwidth L"layerwidth"
#define QT3DS_WCHAR_T_layerheight L"layerheight"
#define QT3DS_WCHAR_T_lighttype L"lighttype"
#define QT3DS_WCHAR_T_lightdiffuse L"lightdiffuse"
#define QT3DS_WCHAR_T_lightspecular L"lightspecular"
#define QT3DS_WCHAR_T_lightambient L"lightambient"
#define QT3DS_WCHAR_T_brightness L"brightness"
#define QT3DS_WCHAR_T_linearfade L"linearfade"
#define QT3DS_WCHAR_T_expfade L"expfade"
#define QT3DS_WCHAR_T_areawidth L"areawidth"
#define QT3DS_WCHAR_T_areaheight L"areaheight"
#define QT3DS_WCHAR_T_castshadow L"castshadow"
#define QT3DS_WCHAR_T_shdwbias L"shdwbias"
#define QT3DS_WCHAR_T_shdwfactor L"shdwfactor"
#define QT3DS_WCHAR_T_shdwmapres L"shdwmapres"
#define QT3DS_WCHAR_T_shdwmapfar L"shdwmapfar"
#define QT3DS_WCHAR_T_shdwmapfov L"shdwmapfov"
#define QT3DS_WCHAR_T_shdwfilter L"shdwfilter"
#define QT3DS_WCHAR_T_orthographic L"orthographic"
#define QT3DS_WCHAR_T_enablefrustumculling L"enablefrustumculling"
#define QT3DS_WCHAR_T_fov L"fov"
#define QT3DS_WCHAR_T_fovhorizontal L"fovhorizontal"
#define QT3DS_WCHAR_T_clipnear L"clipnear"
#define QT3DS_WCHAR_T_clipfar L"clipfar"
#define QT3DS_WCHAR_T_lookatlock L"lookatlock"
#define QT3DS_WCHAR_T_lookatpoint L"lookatpoint"
#define QT3DS_WCHAR_T_textstring L"textstring"
#define QT3DS_WCHAR_T_textcolor L"textcolor"
#define QT3DS_WCHAR_T_font L"font"
#define QT3DS_WCHAR_T_size L"size"
#define QT3DS_WCHAR_T_horzalign L"horzalign"
#define QT3DS_WCHAR_T_vertalign L"vertalign"
#define QT3DS_WCHAR_T_leading L"leading"
#define QT3DS_WCHAR_T_tracking L"tracking"
#define QT3DS_WCHAR_T_dropshadow L"dropshadow"
#define QT3DS_WCHAR_T_dropshadowstrength L"dropshadowstrength"
#define QT3DS_WCHAR_T_dropshadowoffsetx L"dropshadowoffsetx"
#define QT3DS_WCHAR_T_dropshadowoffsety L"dropshadowoffsety"
#define QT3DS_WCHAR_T_wordwrap L"wordwrap"
#define QT3DS_WCHAR_T_boundingbox L"boundingbox"
#define QT3DS_WCHAR_T_elide L"elide"
#define QT3DS_WCHAR_T_enableacceleratedfont L"enableacceleratedfont"
#define QT3DS_WCHAR_T_importfile L"importfile"
#define QT3DS_WCHAR_T_fileid L"fileid"
#define QT3DS_WCHAR_T_size L"size"
#define QT3DS_WCHAR_T_location L"location"
#define QT3DS_WCHAR_T_boneid L"boneid"
#define QT3DS_WCHAR_T_poseroot L"poseroot"
#define QT3DS_WCHAR_T_ignoresparent L"ignoresparent"
#define QT3DS_WCHAR_T_shadowcaster L"shadowcaster"
#define QT3DS_WCHAR_T_tessellation L"tessellation"
#define QT3DS_WCHAR_T_edgetess L"edgetess"
#define QT3DS_WCHAR_T_innertess L"innertess"
#define QT3DS_WCHAR_T_scalemode L"scalemode"
#define QT3DS_WCHAR_T_scaleanchor L"scaleanchor"
#define QT3DS_WCHAR_T_horzfields L"horzfields"
#define QT3DS_WCHAR_T_left L"left"
#define QT3DS_WCHAR_T_leftunits L"leftunits"
#define QT3DS_WCHAR_T_width L"width"
#define QT3DS_WCHAR_T_widthunits L"widthunits"
#define QT3DS_WCHAR_T_right L"right"
#define QT3DS_WCHAR_T_rightunits L"rightunits"
#define QT3DS_WCHAR_T_vertfields L"vertfields"
#define QT3DS_WCHAR_T_top L"top"
#define QT3DS_WCHAR_T_topunits L"topunits"
#define QT3DS_WCHAR_T_height L"height"
#define QT3DS_WCHAR_T_heightunits L"heightunits"
#define QT3DS_WCHAR_T_bottom L"bottom"
#define QT3DS_WCHAR_T_bottomunits L"bottomunits"
#define QT3DS_WCHAR_T_aostrength L"aostrength"
#define QT3DS_WCHAR_T_aodistance L"aodistance"
#define QT3DS_WCHAR_T_aosoftness L"aosoftness"
#define QT3DS_WCHAR_T_aobias L"aobias"
#define QT3DS_WCHAR_T_aosamplerate L"aosamplerate"
#define QT3DS_WCHAR_T_aodither L"aodither"
#define QT3DS_WCHAR_T_shadowstrength L"shadowstrength"
#define QT3DS_WCHAR_T_shadowdist L"shadowdist"
#define QT3DS_WCHAR_T_shadowsoftness L"shadowsoftness"
#define QT3DS_WCHAR_T_shadowbias L"shadowbias"
#define QT3DS_WCHAR_T_source L"source"
#define QT3DS_WCHAR_T_referencedmaterial L"referencedmaterial"
#define QT3DS_WCHAR_T_lightprobe L"lightprobe"
#define QT3DS_WCHAR_T_probebright L"probebright"
#define QT3DS_WCHAR_T_fastibl L"fastibl"
#define QT3DS_WCHAR_T_probehorizon L"probehorizon"
#define QT3DS_WCHAR_T_probefov L"probefov"
#define QT3DS_WCHAR_T_lightprobe2 L"lightprobe2"
#define QT3DS_WCHAR_T_probe2fade L"probe2fade"
#define QT3DS_WCHAR_T_probe2window L"probe2window"
#define QT3DS_WCHAR_T_probe2pos L"probe2pos"
#define QT3DS_WCHAR_T_bumpmap L"bumpmap"
#define QT3DS_WCHAR_T_bumpamount L"bumpamount"
#define QT3DS_WCHAR_T_normalmap L"normalmap"
#define QT3DS_WCHAR_T_displacementmap L"displacementmap"
#define QT3DS_WCHAR_T_displaceamount L"displaceamount"
#define QT3DS_WCHAR_T_translucencymap L"translucencymap"
#define QT3DS_WCHAR_T_translucentfalloff L"translucentfalloff"
#define QT3DS_WCHAR_T_diffuselightwrap L"diffuselightwrap"
#define QT3DS_WCHAR_T_fresnelPower L"fresnelPower"
#define QT3DS_WCHAR_T_referencednode L"referencednode"
#define QT3DS_WCHAR_T_temporalaa L"temporalaa"
#define QT3DS_WCHAR_T_scope L"scope"
#define QT3DS_WCHAR_T_Path L"Path"
#define QT3DS_WCHAR_T_pathdata L"pathdata"
#define QT3DS_WCHAR_T_PathAnchorPoint L"PathAnchorPoint"
#define QT3DS_WCHAR_T_incomingangle L"incomingangle"
#define QT3DS_WCHAR_T_incomingdistance L"incomingdistance"
#define QT3DS_WCHAR_T_outgoingdistance L"outgoingdistance"
#define QT3DS_WCHAR_T_linearerror L"linearerror"
#define QT3DS_WCHAR_T_edgetessamount L"edgetessamount"
#define QT3DS_WCHAR_T_innertessamount L"innertessamount"
#define QT3DS_WCHAR_T_begincap L"begincap"
#define QT3DS_WCHAR_T_begincapoffset L"begincapoffset"
#define QT3DS_WCHAR_T_begincapopacity L"begincapopacity"
#define QT3DS_WCHAR_T_begincapwidth L"begincapwidth"
#define QT3DS_WCHAR_T_endcap L"endcap"
#define QT3DS_WCHAR_T_endcapoffset L"endcapoffset"
#define QT3DS_WCHAR_T_endcapopacity L"endcapopacity"
#define QT3DS_WCHAR_T_endcapwidth L"endcapwidth"
#define QT3DS_WCHAR_T_pathtype L"pathtype"
#define QT3DS_WCHAR_T_closed L"closed"
#define QT3DS_WCHAR_T_paintstyle L"paintstyle"
#define QT3DS_WCHAR_T_SubPath L"SubPath"
#define QT3DS_WCHAR_T_lightmapindirect L"lightmapindirect"
#define QT3DS_WCHAR_T_lightmapradiosity L"lightmapradiosity"
#define QT3DS_WCHAR_T_lightmapshadow L"lightmapshadow"
#define QT3DS_WCHAR_T_controlledproperty L"controlledproperty"
#define QT3DS_WCHAR_T_variants L"variants"

const wchar_t *ComposerObjectTypes::Convert(ComposerObjectTypes::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    case name:                                                                                     \
        return QT3DS_WCHAR_T_##name;
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
    default:
        break;
    }

    QT3DS_ASSERT(false);
    return L"Unknown";
}

ComposerObjectTypes::Enum ComposerObjectTypes::Convert(const wchar_t *inType)
{

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    if (AreEqual(QT3DS_WCHAR_T_##name, inType))                                                    \
        return ComposerObjectTypes::name;
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE

    QT3DS_ASSERT(false);
    return ComposerObjectTypes::Unknown;
}

ComposerObjectTypes::Enum ComposerObjectTypes::Convert(const char8_t *inType)
{

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    if (AreEqual(#name, inType))                                                                   \
        return ComposerObjectTypes::name;
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE

    QT3DS_ASSERT(false);
    return ComposerObjectTypes::Unknown;
}

const wchar_t *ComposerPropertyNames::Convert(ComposerPropertyNames::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    case name:                                                                                     \
        return QT3DS_WCHAR_T_##name;
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue)                             \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    default:
        break;
    }
    QT3DS_ASSERT(false);
    return L"Unknown";
}

ComposerPropertyNames::Enum ComposerPropertyNames::Convert(const wchar_t *inType)
{
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    if (AreEqual(inType, QT3DS_WCHAR_T_##name))                                                    \
        return name;
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue)                             \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    QT3DS_ASSERT(false);
    return ComposerPropertyNames::Unknown;
}

const char8_t *ComposerPropertyNames::ConvertNarrow(ComposerPropertyNames::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    case name:                                                                                     \
        return #name;
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue)                             \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    default:
        break;
    }
    QT3DS_ASSERT(false);
    return "Unknown";
}

ComposerPropertyNames::Enum ComposerPropertyNames::Convert(const char8_t *inType)
{
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)                                \
    if (AreEqual(inType, #name))                                                                   \
        return name;
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue)                             \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type)
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    QT3DS_ASSERT(false);
    return ComposerPropertyNames::Unknown;
}

#define HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)                            \
    , memberName(inCore, inInstance, DataConstructor<dtype>().Construct(defaultValue))

#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)                               \
    , memberName(inCore, inInstance)

#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, dtype, defaultValue)                  \
    HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    SComposerTypePropertyDefinition<ComposerObjectTypes::name>::SComposerTypePropertyDefinition(   \
        IDataCore &inCore, Qt3DSDMInstanceHandle inInstance)                                       \
        : reserved(false) propmacro                                                                \
    {                                                                                              \
        Q_UNUSED(inCore);                                                                          \
        Q_UNUSED(inInstance);                                                                      \
    }
ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE

void ComposerTypeDefinitionsHelper::SetInstanceAsCanonical(IMetaData &inMetaData,
                                                           Qt3DSDMInstanceHandle inInstance,
                                                           ComposerObjectTypes::Enum inObjectType)
{
    inMetaData.SetInstanceAsCanonical(inInstance, ComposerObjectTypes::Convert(inObjectType));
}

void ComposerTypeDefinitionsHelper::SetInstancePropertyValue(IDataCore &inDataCore,
                                                             Qt3DSDMInstanceHandle inInstance,
                                                             Qt3DSDMPropertyHandle inProperty,
                                                             const wchar_t *inPropValue)
{
    inDataCore.SetInstancePropertyValue(inInstance, inProperty, make_shared<CDataStr>(inPropValue));
}

void ComposerTypeDefinitionsHelper::DeriveInstance(IDataCore &inDataCore,
                                                   Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMInstanceHandle inParent)
{
    inDataCore.DeriveInstance(inInstance, inParent);
}

// Container object for all of the object definitions
SComposerObjectDefinitions::SComposerObjectDefinitions(IDataCore &inCore, IMetaData &inMetaData)
    : m_Typed(inCore, inMetaData, inCore.CreateInstance())
    , m_Guided(inCore, inMetaData, inCore.CreateInstance())
    , m_Named(inCore, inMetaData, inCore.CreateInstance())
    , m_SlideOwner(inCore, inMetaData, inCore.CreateInstance())
    , m_Slide(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Named)
    , m_Action(inCore, inMetaData, inCore.CreateInstance(), m_Typed)
    , m_Asset(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Guided, m_Named)
    , m_Scene(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset, m_SlideOwner)
    , m_Image(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_MaterialBase(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Lightmaps(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_MaterialBase)
    , m_Material(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Lightmaps)
    , m_CustomMaterial(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Lightmaps)
    , m_ReferencedMaterial(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Lightmaps)
    , m_Behavior(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Effect(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Node(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Layer(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Group(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Model(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Light(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Camera(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Component(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node, m_SlideOwner)
    , m_Text(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_RenderPlugin(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_Alias(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_Path(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Node)
    , m_PathAnchorPoint(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_SubPath(inCore, inMetaData, inCore.CreateInstance(), m_Typed, m_Asset)
    , m_DataCore(inCore)
{
}

bool SComposerObjectDefinitions::IsA(Qt3DSDMInstanceHandle inInstance,
                                     ComposerObjectTypes::Enum inType)
{
    if (m_DataCore.IsInstance(inInstance) == false)
        return false;

    switch (inType) {
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    case ComposerObjectTypes::name:                                                                \
        return m_DataCore.IsInstanceOrDerivedFrom(inInstance, m_##name.m_Instance);
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
    default:
        break;
    }

    QT3DS_ASSERT(false);
    return false;
}

// Could easily return None, meaning we can't identify the object type.
// Checks the type of the first derivation parent, so this won't ever return
// SlideOwner, for instance.
ComposerObjectTypes::Enum SComposerObjectDefinitions::GetType(Qt3DSDMInstanceHandle inInstance)
{
    if (m_DataCore.IsInstance(inInstance) == false)
        return ComposerObjectTypes::Unknown;

    Qt3DSDMInstanceHandle theTargetInstance = inInstance;
    TInstanceHandleList theHandleList;
    m_DataCore.GetInstanceParents(inInstance, theHandleList);
    if (theHandleList.empty() == false)
        theTargetInstance = theHandleList[0];

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    if (m_##name.m_Instance == theTargetInstance)                                                  \
        return ComposerObjectTypes::name;
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE

    return ComposerObjectTypes::Unknown;
}

Qt3DSDMInstanceHandle
SComposerObjectDefinitions::GetInstanceForType(ComposerObjectTypes::Enum inType)
{
    switch (inType) {
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    case ComposerObjectTypes::name:                                                                \
        return m_##name.m_Instance;
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
    default:
        break;
    }

    QT3DS_ASSERT(false);
    return 0;
}
