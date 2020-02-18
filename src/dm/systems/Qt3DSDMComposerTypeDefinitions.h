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
#pragma once
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMDataTypes.h"
#include "foundation/Qt3DSOption.h"
#include "Qt3DSDMDataCore.h"
#include <utility>

// Defines the minimal property model that UIComposer needs to
// run off of.
namespace qt3dsdm {
class IDataCore;
class IMetaData;
class ISlideCore;
class IPropertySystem;

// Extensible macro definitions
// Enumerations *have* to be in derivation order
// else initialization of the SComposerObjectDefinitions class
// will fail catastrophically
// Specific type documentation is below
#define ITERATE_COMPOSER_OBJECT_TYPES                                                              \
    HANDLE_COMPOSER_OBJECT_TYPE(Typed, ITERATE_COMPOSER_TYPED_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Guided, ITERATE_COMPOSER_GUIDED_PROPERTIES)                        \
    HANDLE_COMPOSER_OBJECT_TYPE(Named, ITERATE_COMPOSER_NAMED_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(SlideOwner, ITERATE_COMPOSER_NO_ADDITIONAL_PROPERTIES)             \
    HANDLE_COMPOSER_OBJECT_TYPE(Slide, ITERATE_COMPOSER_SLIDE_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Action, ITERATE_COMPOSER_ACTION_PROPERTIES)                        \
    HANDLE_COMPOSER_OBJECT_TYPE(Asset, ITERATE_COMPOSER_ASSET_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Scene, ITERATE_COMPOSER_SCENE_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Image, ITERATE_COMPOSER_IMAGE_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(MaterialBase, ITERATE_COMPOSER_MATERIAL_BASE_PROPERTIES)           \
    HANDLE_COMPOSER_OBJECT_TYPE(Lightmaps, ITERATE_COMPOSER_LIGHTMAP_PROPERTIES)                   \
    HANDLE_COMPOSER_OBJECT_TYPE(Material, ITERATE_COMPOSER_MATERIAL_PROPERTIES)                    \
    HANDLE_COMPOSER_OBJECT_TYPE(CustomMaterial, ITERATE_COMPOSER_NO_ADDITIONAL_PROPERTIES)         \
    HANDLE_COMPOSER_OBJECT_TYPE(ReferencedMaterial,                                                \
                                ITERATE_COMPOSER_REFERENCED_MATERIAL_PROPERTIES)                   \
    HANDLE_COMPOSER_OBJECT_TYPE(Behavior, ITERATE_COMPOSER_NO_ADDITIONAL_PROPERTIES)               \
    HANDLE_COMPOSER_OBJECT_TYPE(Effect, ITERATE_COMPOSER_NO_ADDITIONAL_PROPERTIES)                 \
    HANDLE_COMPOSER_OBJECT_TYPE(Node, ITERATE_COMPOSER_NODE_PROPERTIES)                            \
    HANDLE_COMPOSER_OBJECT_TYPE(Layer, ITERATE_COMPOSER_LAYER_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Group, ITERATE_COMPOSER_GROUP_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Model, ITERATE_COMPOSER_MODEL_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Light, ITERATE_COMPOSER_LIGHT_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Camera, ITERATE_COMPOSER_CAMERA_PROPERTIES)                        \
    HANDLE_COMPOSER_OBJECT_TYPE(Component, ITERATE_COMPOSER_COMPONENT_PROPERTIES)                  \
    HANDLE_COMPOSER_OBJECT_TYPE(Text, ITERATE_COMPOSER_TEXT_PROPERTIES)                            \
    HANDLE_COMPOSER_OBJECT_TYPE(RenderPlugin, ITERATE_COMPOSER_NO_ADDITIONAL_PROPERTIES)           \
    HANDLE_COMPOSER_OBJECT_TYPE(Alias, ITERATE_COMPOSER_ALIAS_PROPERTIES)                          \
    HANDLE_COMPOSER_OBJECT_TYPE(Path, ITERATE_COMPOSER_PATH_PROPERTIES)                            \
    HANDLE_COMPOSER_OBJECT_TYPE(PathAnchorPoint, ITERATE_COMPOSER_PATH_ANCHOR_POINT_PROPERTIES)    \
    HANDLE_COMPOSER_OBJECT_TYPE(SubPath, ITERATE_COMPOSER_PATH_SUBPATH_PROPERTIES)

#define ITERATE_COMPOSER_NO_ADDITIONAL_PROPERTIES

#define ITERATE_COMPOSER_TYPED_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(type, m_TypeProp, TDataStrPtr, L"Typed")

#define ITERATE_COMPOSER_GUIDED_PROPERTIES                                                         \
    HANDLE_COMPOSER_PROPERTY(id, m_GuidProp, SLong4, 0)

#define ITERATE_COMPOSER_NAMED_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(name, m_NameProp, TDataStrPtr, L"Unnamed")

#define ITERATE_COMPOSER_ASSET_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(sourcepath, m_SourcePath, TDataStrPtr, L"")                           \
    HANDLE_COMPOSER_PROPERTY(importid, m_ImportId, TDataStrPtr, L"")                               \
    HANDLE_COMPOSER_PROPERTY(importfile, m_ImportFile, TDataStrPtr, L"")                           \
    HANDLE_COMPOSER_PROPERTY(fileid, m_FileId, TDataStrPtr, L"")                                   \
    HANDLE_COMPOSER_PROPERTY(starttime, m_StartTime, qt3ds::QT3DSI32, 0)                           \
    HANDLE_COMPOSER_PROPERTY(endtime, m_EndTime, qt3ds::QT3DSI32, 10000)                           \
    HANDLE_COMPOSER_PROPERTY(eyeball, m_Eyeball, bool, true)                                       \
    HANDLE_COMPOSER_PROPERTY(shy, m_Shy, bool, false)                                              \
    HANDLE_COMPOSER_PROPERTY(locked, m_Locked, bool, false)                                        \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(timebarcolor, m_TimebarColor, SFloat4)                     \
    HANDLE_COMPOSER_PROPERTY(timebartext, m_TimebarText, TDataStrPtr, L"")                         \
    HANDLE_COMPOSER_PROPERTY(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_SCENE_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(bgcolorenable, m_BgColorEnable, bool, true)                           \
    HANDLE_COMPOSER_PROPERTY(backgroundcolor, m_BackgroundColor, SFloat4, SFloat4(0, 0, 0, 1))     \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_COMPONENT_PROPERTIES                                                      \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(variants, m_variants, TDataStrPtr, L"")                     \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_NODE_PROPERTIES                                                           \
    HANDLE_COMPOSER_PROPERTY(position, m_Position, SFloat3, SFloat3(0, 0, 0))                      \
    HANDLE_COMPOSER_PROPERTY(rotation, m_Rotation, SFloat3, SFloat3(0, 0, 0))                      \
    HANDLE_COMPOSER_PROPERTY(scale, m_Scale, SFloat3, SFloat3(1, 1, 1))                            \
    HANDLE_COMPOSER_PROPERTY(pivot, m_Pivot, SFloat3, SFloat3(0, 0, 0))                            \
    HANDLE_COMPOSER_PROPERTY(opacity, m_Opacity, float, 100.f)                                     \
    HANDLE_COMPOSER_PROPERTY(rotationorder, m_RotationOrder, TDataStrPtr, L"YXZ")                  \
    HANDLE_COMPOSER_PROPERTY(orientation, m_Orientation, TDataStrPtr, L"Left Handed")              \
    HANDLE_COMPOSER_PROPERTY(boneid, m_BoneId, qt3ds::QT3DSI32, 0)                                 \
    HANDLE_COMPOSER_PROPERTY(ignoresparent, m_IgnoresParent, bool, false)                          \
    HANDLE_COMPOSER_PROPERTY(ordered, m_ordered, bool, false)                                      \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_MODEL_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(shadowcaster, m_ShadowCaster, bool, true)                             \
    HANDLE_COMPOSER_PROPERTY(poseroot, m_PoseRoot, qt3ds::QT3DSI32, -1)                            \
    HANDLE_COMPOSER_PROPERTY(tessellation, m_Tessellation, TDataStrPtr, L"None")                   \
    HANDLE_COMPOSER_PROPERTY(edgetess, m_EdgeTess, float, 1.0)                                     \
    HANDLE_COMPOSER_PROPERTY(innertess, m_InnerTess, float, 1.0)                                   \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(variants, m_variants, TDataStrPtr, L"")                     \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_IMAGE_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(scaleu, m_RepeatU, float, 1.0f)                                       \
    HANDLE_COMPOSER_PROPERTY(scalev, m_RepeatV, float, 1.0f)                                       \
    HANDLE_COMPOSER_PROPERTY(mappingmode, m_TextureMapping, TDataStrPtr, L"UV Mapping")            \
    HANDLE_COMPOSER_PROPERTY(tilingmodehorz, m_TilingU, TDataStrPtr, L"No Tiling")                 \
    HANDLE_COMPOSER_PROPERTY(tilingmodevert, m_TilingV, TDataStrPtr, L"No Tiling")                 \
    HANDLE_COMPOSER_PROPERTY(rotationuv, m_RotationUV, float, 0.f)                                 \
    HANDLE_COMPOSER_PROPERTY(positionu, m_PositionU, float, 0.f)                                   \
    HANDLE_COMPOSER_PROPERTY(positionv, m_PositionV, float, 0.f)                                   \
    HANDLE_COMPOSER_PROPERTY(pivotu, m_PivotU, float, 0.f)                                         \
    HANDLE_COMPOSER_PROPERTY(pivotv, m_PivotV, float, 0.f)                                         \
    HANDLE_COMPOSER_PROPERTY(subpresentation, m_SubPresentation, TDataStrPtr, L"")                 \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_LIGHTMAP_PROPERTIES                                                       \
    HANDLE_COMPOSER_PROPERTY(lightmapindirect, m_LightmapIndirect, SLong4, 0)                      \
    HANDLE_COMPOSER_PROPERTY(lightmapradiosity, m_LightmapRadiosity, SLong4, 0)                    \
    HANDLE_COMPOSER_PROPERTY(lightmapshadow, m_LightmapShadow, SLong4, 0)                          \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_CUSTOM_MATERIAL_PROPERTIES                                                \
    HANDLE_QT3DS_RENDER_PROPERTY(iblprobe, m_IblProbe, SLong4, 0)

#define ITERATE_COMPOSER_MATERIAL_BASE_PROPERTIES                                                  \
    HANDLE_COMPOSER_PROPERTY(iblprobe, m_IblProbe, SLong4, 0)

#define ITERATE_COMPOSER_MATERIAL_PROPERTIES                                                       \
    HANDLE_COMPOSER_PROPERTY(shaderlighting, m_ShaderLighting, TDataStrPtr, L"Vertex")             \
    HANDLE_COMPOSER_PROPERTY(blendmode, m_BlendMode, TDataStrPtr, L"Normal")                       \
    HANDLE_COMPOSER_PROPERTY(diffuse, m_DiffuseColor, SFloat4, SFloat4(1, 1, 1, 1))                \
    HANDLE_COMPOSER_PROPERTY(diffusemap, m_DiffuseMap1, SLong4, 0)                                 \
    HANDLE_COMPOSER_PROPERTY(diffusemap2, m_DiffuseMap2, SLong4, 0)                                \
    HANDLE_COMPOSER_PROPERTY(diffusemap3, m_DiffuseMap3, SLong4, 0)                                \
    HANDLE_COMPOSER_PROPERTY(specularreflection, m_SpecularReflection, SLong4, 0)                  \
    HANDLE_COMPOSER_PROPERTY(specularamount, m_SpecularAmount, float, 0.f)                         \
    HANDLE_COMPOSER_PROPERTY(specularroughness, m_SpecularRoughness, float, 50.f)                  \
    HANDLE_COMPOSER_PROPERTY(roughnessmap, m_RoughnessMap, SLong4, 0)                              \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(opacity, m_Opacity, float, 100.f)                           \
    HANDLE_COMPOSER_PROPERTY(opacitymap, m_OpacityMap, SLong4, 0)                                  \
    HANDLE_COMPOSER_PROPERTY(emissivecolor, m_EmissiveColor, SFloat4, SFloat4(1, 1, 1, 1))         \
    HANDLE_COMPOSER_PROPERTY(emissivepower, m_EmissivePower, float, 0.f)                           \
    HANDLE_COMPOSER_PROPERTY(emissivemap, m_EmissiveMap, SLong4, 0)                                \
    HANDLE_COMPOSER_PROPERTY(emissivemap2, m_EmissiveMap2, SLong4, 0)                              \
    HANDLE_COMPOSER_PROPERTY(bumpmap, m_BumpMap, SLong4, 0)                                        \
    HANDLE_COMPOSER_PROPERTY(bumpamount, m_BumpAmount, float, 9.0f)                                \
    HANDLE_COMPOSER_PROPERTY(normalmap, m_NormalMap, SLong4, 0)                                    \
    HANDLE_COMPOSER_PROPERTY(displacementmap, m_DisplacementMap, SLong4, 0)                        \
    HANDLE_COMPOSER_PROPERTY(displaceamount, m_DisplaceAmount, float, 0.5f)                        \
    HANDLE_COMPOSER_PROPERTY(translucencymap, m_TranslucencyMap, SLong4, 0)                        \
    HANDLE_COMPOSER_PROPERTY(translucentfalloff, m_TranslucentFalloff, float, 1.f)                 \
    HANDLE_COMPOSER_PROPERTY(diffuselightwrap, m_DiffuseLightWrap, float, 0.f)                     \
    HANDLE_COMPOSER_PROPERTY(specularmap, m_SpecularMap, SLong4, 0)                                \
    HANDLE_COMPOSER_PROPERTY(specularmodel, m_SpecularModel, TDataStrPtr, L"Default")              \
    HANDLE_COMPOSER_PROPERTY(speculartint, m_SpecularTint, SFloat4, SFloat4(1, 1, 1, 1))           \
    HANDLE_COMPOSER_PROPERTY(ior, m_IOR, float, 0)                                                 \
    HANDLE_COMPOSER_PROPERTY(fresnelPower, m_FresnelPower, float, 0)                               \
    HANDLE_COMPOSER_PROPERTY(vertexcolors, m_VertexColors, bool, false)                            \
    HANDLE_COMPOSER_PROPERTY(transparencymode, m_TransparencyMode, TDataStrPtr, L"Default")        \
    HANDLE_COMPOSER_PROPERTY(cullmode, m_CullMode, TDataStrPtr, L"Back")                           \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_REFERENCED_MATERIAL_PROPERTIES                                            \
    HANDLE_COMPOSER_PROPERTY(referencedmaterial, m_ReferencedMaterial, SObjectRefType, L"")

#define ITERATE_COMPOSER_LAYER_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(progressiveaa, m_ProgressiveAA, TDataStrPtr, L"None")                 \
    HANDLE_COMPOSER_PROPERTY(multisampleaa, m_MultisampleAA, TDataStrPtr, L"None")                 \
    HANDLE_COMPOSER_PROPERTY(temporalaa, m_TemporalAA, bool, false)                                \
    HANDLE_COMPOSER_PROPERTY(disabledepthtest, m_DisableDepthTest, bool, false)                    \
    HANDLE_COMPOSER_PROPERTY(disabledepthprepass, m_DisableDepthPrepass, bool, true)              \
    HANDLE_COMPOSER_PROPERTY(background, m_Background, TDataStrPtr, L"Transparent")                \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(backgroundcolor, m_BackgroundColor, SFloat4,                \
                                       SFloat4(0, 0, 0, 1))                                        \
    HANDLE_COMPOSER_PROPERTY(blendtype, m_BlendType, TDataStrPtr, L"Normal")                       \
    HANDLE_COMPOSER_PROPERTY(dynamicresize, m_DynamicResize, bool, false)                          \
    HANDLE_COMPOSER_PROPERTY(dynamicpaddingunits, m_DynamicPaddingUnits, TDataStrPtr, L"percent")  \
    HANDLE_COMPOSER_PROPERTY(dynamicpadding, m_DynamicPadding, float, 0)                           \
    HANDLE_COMPOSER_PROPERTY(dynamiccombine, m_DynamicCombine, bool, false)                        \
    HANDLE_COMPOSER_PROPERTY(horzfields, m_HorizontalFieldValues, TDataStrPtr, L"Left/Width")      \
    HANDLE_COMPOSER_PROPERTY(left, m_Left, float, 0)                                               \
    HANDLE_COMPOSER_PROPERTY(leftunits, m_LeftUnits, TDataStrPtr, L"percent")                      \
    HANDLE_COMPOSER_PROPERTY(width, m_Width, float, 100.0f)                                        \
    HANDLE_COMPOSER_PROPERTY(widthunits, m_WidthUnits, TDataStrPtr, L"percent")                    \
    HANDLE_COMPOSER_PROPERTY(right, m_Right, float, 0)                                             \
    HANDLE_COMPOSER_PROPERTY(rightunits, m_RightUnits, TDataStrPtr, L"percent")                    \
    HANDLE_COMPOSER_PROPERTY(vertfields, m_VerticalFieldValues, TDataStrPtr, L"Top/Height")        \
    HANDLE_COMPOSER_PROPERTY(top, m_Top, float, 0)                                                 \
    HANDLE_COMPOSER_PROPERTY(topunits, m_TopUnits, TDataStrPtr, L"percent")                        \
    HANDLE_COMPOSER_PROPERTY(height, m_Height, float, 100.0f)                                      \
    HANDLE_COMPOSER_PROPERTY(heightunits, m_HeightUnits, TDataStrPtr, L"percent")                  \
    HANDLE_COMPOSER_PROPERTY(bottom, m_Bottom, float, 0)                                           \
    HANDLE_COMPOSER_PROPERTY(bottomunits, m_BottomUnits, TDataStrPtr, L"percent")                  \
    HANDLE_COMPOSER_PROPERTY(aoenabled, m_AoEnabled, bool, false)                                  \
    HANDLE_COMPOSER_PROPERTY(aostrength, m_AoStrength, float, 0)                                   \
    HANDLE_COMPOSER_PROPERTY(aodistance, m_AoDistance, float, 0)                                   \
    HANDLE_COMPOSER_PROPERTY(aosoftness, m_AoSoftness, float, 0)                                   \
    HANDLE_COMPOSER_PROPERTY(aobias, m_AoBias, float, 0)                                           \
    HANDLE_COMPOSER_PROPERTY(aosamplerate, m_AoSamplerate, qt3ds::QT3DSI32, 1)                     \
    HANDLE_COMPOSER_PROPERTY(aodither, m_AoDither, bool, false)                                    \
    HANDLE_COMPOSER_PROPERTY(shadowstrength, m_ShadowStrength, float, 0)                           \
    HANDLE_COMPOSER_PROPERTY(shadowdist, m_ShadowDist, float, 0)                                   \
    HANDLE_COMPOSER_PROPERTY(shadowsoftness, m_ShadowSoftness, float, 0)                           \
    HANDLE_COMPOSER_PROPERTY(shadowbias, m_ShadowBias, float, 0)                                   \
    HANDLE_COMPOSER_PROPERTY(lightprobe, m_LightProbe, SLong4, 0)                                  \
    HANDLE_COMPOSER_PROPERTY(probebright, m_ProbeBright, float, 100.0f)                            \
    HANDLE_COMPOSER_PROPERTY(fastibl, m_FastIbl, bool, true)                                       \
    HANDLE_COMPOSER_PROPERTY(probehorizon, m_ProbeHorizon, float, -1)                              \
    HANDLE_COMPOSER_PROPERTY(probefov, m_ProbeFov, float, 180.0f)                                  \
    HANDLE_COMPOSER_PROPERTY(lightprobe2, m_LightProbe2, SLong4, 0)                                \
    HANDLE_COMPOSER_PROPERTY(probe2fade, m_Probe2Fade, float, 1)                                   \
    HANDLE_COMPOSER_PROPERTY(probe2window, m_Probe2Window, float, 1)                               \
    HANDLE_COMPOSER_PROPERTY(probe2pos, m_Probe2Pos, float, 0.5f)                                  \
    HANDLE_COMPOSER_PROPERTY(variants, m_variants, TDataStrPtr, L"")                               \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_GROUP_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(ordered, m_ordered, bool, false)                            \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(variants, m_variants, TDataStrPtr, L"")                     \

#define ITERATE_COMPOSER_LIGHT_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(lighttype, m_LightType, TDataStrPtr, L"Directional")                  \
    HANDLE_COMPOSER_PROPERTY(scope, m_Scope, SObjectRefType, L"")                                  \
    HANDLE_COMPOSER_PROPERTY(lightdiffuse, m_LightColor, SFloat4, SFloat4(1, 1, 1, 1))             \
    HANDLE_COMPOSER_PROPERTY(lightspecular, m_SpecularColor, SFloat4, SFloat4(1, 1, 1, 1))         \
    HANDLE_COMPOSER_PROPERTY(lightambient, m_AmbientColor, SFloat4, SFloat4(0, 0, 0, 1))           \
    HANDLE_COMPOSER_PROPERTY(brightness, m_Brightness, float, 100.0f)                              \
    HANDLE_COMPOSER_PROPERTY(linearfade, m_LinearFade, float, 0.0f)                                \
    HANDLE_COMPOSER_PROPERTY(expfade, m_ExpFade, float, 0.0f)                                      \
    HANDLE_COMPOSER_PROPERTY(areawidth, m_AreaWidth, float, 0.0f)                                  \
    HANDLE_COMPOSER_PROPERTY(areaheight, m_AreaHeight, float, 0.0f)                                \
    HANDLE_COMPOSER_PROPERTY(castshadow, m_CastShadow, bool, false)                                \
    HANDLE_COMPOSER_PROPERTY(shdwbias, m_ShadowBias, float, 0.0f)                                  \
    HANDLE_COMPOSER_PROPERTY(shdwfactor, m_ShadowFactor, float, 5.0f)                              \
    HANDLE_COMPOSER_PROPERTY(shdwmapres, m_ShadowMapRes, qt3ds::QT3DSI32, 9)                       \
    HANDLE_COMPOSER_PROPERTY(shdwmapfar, m_ShadowMapFar, float, 5000.0f)                           \
    HANDLE_COMPOSER_PROPERTY(shdwmapfov, m_ShadowMapFov, float, 90.0f)                             \
    HANDLE_COMPOSER_PROPERTY(shdwfilter, m_ShadowFilter, float, 35.0f)                             \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_CAMERA_PROPERTIES                                                         \
    HANDLE_COMPOSER_PROPERTY(orthographic, m_Orthographic, bool, false)                       \
    HANDLE_COMPOSER_PROPERTY(enablefrustumculling, m_EnableFrustumCulling, bool, false)             \
    HANDLE_COMPOSER_PROPERTY(fov, m_Fov, float, 60.f)                                              \
    HANDLE_COMPOSER_PROPERTY(fovhorizontal, m_FovHorizontal, bool, false)                          \
    HANDLE_COMPOSER_PROPERTY(clipnear, m_ClipNear, float, 10.f)                                    \
    HANDLE_COMPOSER_PROPERTY(clipfar, m_ClipFar, float, 100000.f)                                  \
    HANDLE_COMPOSER_PROPERTY(scalemode, m_ScaleMode, TDataStrPtr, L"Fit")                          \
    HANDLE_COMPOSER_PROPERTY(scaleanchor, m_ScaleAnchor, TDataStrPtr, L"Center")                   \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_EDIT_CAMERA_PROPERTIES HANDLE_COMPOSER_PROPERTY(is3d, m_Is3d, bool, false)

#define ITERATE_COMPOSER_TEXT_PROPERTIES                                                           \
    HANDLE_COMPOSER_PROPERTY(textstring, m_TextString, TDataStrPtr, L"Text")                       \
    HANDLE_COMPOSER_PROPERTY(textcolor, m_TextColor, SFloat4, SFloat4(1, 1, 1, 1))                 \
    HANDLE_COMPOSER_PROPERTY(font, m_Font, TDataStrPtr, L"arial")                                  \
    HANDLE_COMPOSER_PROPERTY(size, m_Size, float, 24.f)                                            \
    HANDLE_COMPOSER_PROPERTY(horzalign, m_HorzAlign, TDataStrPtr, L"Center")                       \
    HANDLE_COMPOSER_PROPERTY(vertalign, m_VertAlign, TDataStrPtr, L"Middle")                       \
    HANDLE_COMPOSER_PROPERTY(leading, m_Leading, float, 0.f)                                       \
    HANDLE_COMPOSER_PROPERTY(tracking, m_Tracking, float, 0.f)                                     \
    HANDLE_COMPOSER_PROPERTY(dropshadow, m_DropShadow, bool, false)                                \
    HANDLE_COMPOSER_PROPERTY(dropshadowstrength, m_DropShadowStrength, float, 80.f)                \
    HANDLE_COMPOSER_PROPERTY(dropshadowoffsetx, m_DropShadowOffsetX, float, 0.f)                   \
    HANDLE_COMPOSER_PROPERTY(dropshadowoffsety, m_DropShadowOffsetY, float, 0.f)                   \
    HANDLE_COMPOSER_PROPERTY(wordwrap, m_WordWrap, TDataStrPtr, L"WrapWord")                       \
    HANDLE_COMPOSER_PROPERTY(boundingbox, m_BoundingBox, SFloat2, SFloat2(0, 0))                   \
    HANDLE_COMPOSER_PROPERTY(elide, m_Elide, TDataStrPtr, L"ElideNone")                            \
    HANDLE_COMPOSER_PROPERTY(enableacceleratedfont, m_EnableAcceleratedFont, bool, false)          \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(variants, m_variants, TDataStrPtr, L"")                     \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_SLIDE_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(componentid, m_ComponentId, SLong4, 0)                                \
    HANDLE_COMPOSER_PROPERTY(playmode, m_PlayMode, TDataStrPtr, L"Looping")                        \
    HANDLE_COMPOSER_PROPERTY(playthroughto, m_PlaythroughTo, SStringOrInt, L"Next")                \
    HANDLE_COMPOSER_PROPERTY(initialplaystate, m_InitialPlayState, TDataStrPtr, L"Play")           \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(fileid, m_FileId, TDataStrPtr, L"")                         \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_ACTION_PROPERTIES                                                         \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(fileid, m_FileId, TDataStrPtr, L"")                         \
    HANDLE_COMPOSER_PROPERTY(actioneyeball, m_ActionEyeball, bool, true)

#define ITERATE_COMPOSER_ALIAS_PROPERTIES                                                          \
    HANDLE_COMPOSER_PROPERTY(referencednode, m_ReferencedNode, SObjectRefType, L"")                \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_PATH_PROPERTIES                                                           \
    HANDLE_COMPOSER_PROPERTY(pathtype, m_PathType, TDataStrPtr, L"Geometry")                       \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(width, m_Width, float, 5.0f)                                \
    HANDLE_COMPOSER_PROPERTY(linearerror, m_LinearError, float, 10)                                \
    HANDLE_COMPOSER_PROPERTY(edgetessamount, m_EdgeTessAmount, float, 8)                           \
    HANDLE_COMPOSER_PROPERTY(innertessamount, m_InnerTessAmount, float, 8)                         \
    HANDLE_COMPOSER_PROPERTY(begincap, m_BeginCap, TDataStrPtr, L"None")                           \
    HANDLE_COMPOSER_PROPERTY(begincapoffset, m_BeginCapOffset, float, 0.0f)                        \
    HANDLE_COMPOSER_PROPERTY(begincapopacity, m_BeginCapOpacity, float, 0.0f)                      \
    HANDLE_COMPOSER_PROPERTY(begincapwidth, m_BeginCapWidth, float, 0.0f)                          \
    HANDLE_COMPOSER_PROPERTY(endcap, m_EndCap, TDataStrPtr, L"None")                               \
    HANDLE_COMPOSER_PROPERTY(endcapoffset, m_EndCapOffset, float, 0.0f)                            \
    HANDLE_COMPOSER_PROPERTY(endcapopacity, m_EndCapOpacity, float, 0.0f)                          \
    HANDLE_COMPOSER_PROPERTY(endcapwidth, m_EndCapWidth, float, 0.0f)                              \
    HANDLE_COMPOSER_PROPERTY(paintstyle, m_PaintStyle, TDataStrPtr, L"Stroked")                    \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(controlledproperty, m_ControlledProperty, TDataStrPtr, L"")

#define ITERATE_COMPOSER_PATH_ANCHOR_POINT_PROPERTIES                                              \
    HANDLE_COMPOSER_PROPERTY_DUPLICATE(position, m_Position, SFloat2, SFloat2(0.0f, 0.0f))         \
    HANDLE_COMPOSER_PROPERTY(incomingangle, m_IncomingAngle, float, 0.0f)                          \
    HANDLE_COMPOSER_PROPERTY(incomingdistance, m_IncomingDistance, float, 0.0f)                    \
    HANDLE_COMPOSER_PROPERTY(outgoingdistance, m_OutgoingDistance, float, 0.0f)

#define ITERATE_COMPOSER_PATH_SUBPATH_PROPERTIES                                                   \
    HANDLE_COMPOSER_PROPERTY(closed, m_Closed, bool, true)

struct ComposerObjectTypes
{
    enum Enum {
        Unknown = 0,
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) name,
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
        ControllableObject
    };
    static const wchar_t *Convert(ComposerObjectTypes::Enum inType);
    static ComposerObjectTypes::Enum Convert(const wchar_t *inType);
    static ComposerObjectTypes::Enum Convert(const char8_t *inType);
};

struct ComposerPropertyNames
{
    enum Enum {
        Unknown = 0,
#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, type) name,
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, type, defaultValue)
#define HANDLE_COMPOSER_PROPERTY(name, memberName, type, defaultValue) name,
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro) propmacro
        ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE
    };

    static const wchar_t *Convert(ComposerPropertyNames::Enum inType);
    static ComposerPropertyNames::Enum Convert(const wchar_t *inType);
    static const char8_t *ConvertNarrow(ComposerPropertyNames::Enum inType);
    static ComposerPropertyNames::Enum Convert(const char8_t *inType);
};

template <typename TDataType>
struct TypeToDataTypeMap
{
    bool force_compile_error;
};
template <DataModelDataType::Value TEnum>
struct DataTypeToTypeMap
{
    bool force_compile_error;
};

#define QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(enumName, type)                                           \
    template <>                                                                                    \
    struct TypeToDataTypeMap<type>                                                                 \
    {                                                                                              \
        static DataModelDataType::Value GetDataType() { return enumName; }                         \
    };                                                                                             \
    template <>                                                                                    \
    struct DataTypeToTypeMap<enumName>                                                             \
    {                                                                                              \
        typedef type TDataType;                                                                    \
    };

QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::Float, float)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::Float2, SFloat2)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::Float3, SFloat3)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::Float4, SFloat4)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::Long, qt3ds::QT3DSI32)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::String, TDataStrPtr)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::Bool, bool)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::Long4, SLong4)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::StringRef, SStringRef)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::StringOrInt, SStringOrInt)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::ObjectRef, SObjectRefType)
QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE(DataModelDataType::FloatList, TFloatList)

#undef QT3DSDM_DEFINE_TYPE_TO_DATA_TYPE

template <typename TDataType>
inline DataModelDataType::Value TypeToDataType()
{
    return TypeToDataTypeMap<TDataType>::GetDataType();
}

template <ComposerPropertyNames::Enum TPropName, typename TDataType>
struct SComposerPropertyDefinition
{
    qt3ds::foundation::Option<TDataType> m_DefaultValue;
    Qt3DSDMPropertyHandle m_Property;
    SComposerPropertyDefinition(IDataCore &inDataCore, Qt3DSDMInstanceHandle inInstance,
                                const TDataType &inDefault)
        : m_DefaultValue(inDefault)
    {
        QT3DSDM_LOG_FUNCTION("SComposerPropertyDefinition-1");
        m_Property = inDataCore.AddProperty(inInstance, ComposerPropertyNames::Convert(TPropName),
                                            TypeToDataType<TDataType>());
        if (m_DefaultValue.hasValue())
            inDataCore.SetInstancePropertyValue(inInstance, m_Property, m_DefaultValue.getValue());
    }
    SComposerPropertyDefinition(IDataCore &inDataCore, Qt3DSDMInstanceHandle inInstance)
    {
        QT3DSDM_LOG_FUNCTION("SComposerPropertyDefinition-2");
        m_Property = inDataCore.AddProperty(inInstance, ComposerPropertyNames::Convert(TPropName),
                                            TypeToDataType<TDataType>());
    }
    operator Qt3DSDMPropertyHandle() const { return m_Property; }
};

// Define all the objects with their properties

template <ComposerObjectTypes::Enum>
struct SComposerTypePropertyDefinition
{
    bool force_compile_error;
};

#define HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)                               \
    SComposerPropertyDefinition<ComposerPropertyNames::name, dtype> memberName;

#define HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)                            \
    HANDLE_COMPOSER_PROPERTY_NO_DEFAULT(name, memberName, dtype)
#define HANDLE_COMPOSER_PROPERTY_DUPLICATE(name, memberName, dtype, defaultValue)                  \
    HANDLE_COMPOSER_PROPERTY(name, memberName, dtype, defaultValue)

#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    template <>                                                                                    \
    struct SComposerTypePropertyDefinition<ComposerObjectTypes::name>                              \
    {                                                                                              \
        bool reserved;                                                                             \
        propmacro SComposerTypePropertyDefinition(IDataCore &inCore,                               \
                                                  Qt3DSDMInstanceHandle inInstance);               \
    };

ITERATE_COMPOSER_OBJECT_TYPES

#undef HANDLE_COMPOSER_OBJECT_TYPE
#undef HANDLE_COMPOSER_PROPERTY
#undef HANDLE_COMPOSER_PROPERTY_NO_DEFAULT
#undef HANDLE_COMPOSER_PROPERTY_DUPLICATE

struct ComposerTypeDefinitionsHelper
{
    // Functions here so we don't have to include UICDMDataCore.h or UICDMMetaData.h
    static void SetInstanceAsCanonical(IMetaData &inMetaData, Qt3DSDMInstanceHandle inInstance,
                                       ComposerObjectTypes::Enum inObjectType);
    static void SetInstancePropertyValue(IDataCore &inDataCore, Qt3DSDMInstanceHandle inInstance,
                                         Qt3DSDMPropertyHandle inProperty,
                                         const wchar_t *inPropValue);
    static void DeriveInstance(IDataCore &inDataCore, Qt3DSDMInstanceHandle inInstance,
                               Qt3DSDMInstanceHandle inParent);
};

template <ComposerObjectTypes::Enum TEnumType>
struct SComposerBaseObjectDefinition : public SComposerTypePropertyDefinition<TEnumType>
{
    Qt3DSDMInstanceHandle m_Instance;
    SComposerBaseObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                                  Qt3DSDMInstanceHandle inInstance)
        : SComposerTypePropertyDefinition<TEnumType>(inCore, inInstance)
        , m_Instance(inInstance)
    {
        QT3DSDM_LOG_FUNCTION("SComposerBaseObjectDefinition");
        ComposerTypeDefinitionsHelper::SetInstanceAsCanonical(inMetaData, inInstance, TEnumType);
    }

    template <ComposerObjectTypes::Enum TBaseType>
    void Derive(IDataCore &inCore, const SComposerBaseObjectDefinition<TBaseType> &inParent)
    {
        QT3DSDM_LOG_FUNCTION("SComposerBaseObjectDefinition::Derive");
        ComposerTypeDefinitionsHelper::DeriveInstance(inCore, m_Instance, inParent.m_Instance);
    }
    void SetType(IDataCore &inCore,
                 const SComposerBaseObjectDefinition<ComposerObjectTypes::Typed> &inParent)
    {
        QT3DSDM_LOG_FUNCTION("SComposerBaseObjectDefinition::SetInstancePropertyValue");
        ComposerTypeDefinitionsHelper::SetInstancePropertyValue(
            inCore, m_Instance, inParent.m_TypeProp, ComposerObjectTypes::Convert(TEnumType));
    }
    void TypedDerive(IDataCore &inCore,
                     const SComposerBaseObjectDefinition<ComposerObjectTypes::Typed> &inParent)
    {
        QT3DSDM_LOG_FUNCTION("SComposerBaseObjectDefinition::TypedDerive");
        Derive(inCore, inParent);
        SetType(inCore, inParent);
    }
};

template <ComposerObjectTypes::Enum TEnumType>
struct SComposerObjectDefinition : public SComposerBaseObjectDefinition<TEnumType>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance)
        : SComposerBaseObjectDefinition<TEnumType>(inCore, inMetaData, inInstance)
    {
    }
};

// Base class of slides
template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Slide>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Slide>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Named> &inNamed)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Slide>(inCore, inMetaData, inInstance)
    {
        TypedDerive(inCore, inTyped);
        Derive(inCore, inNamed);
    }
};

// Base class of actions
template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Action>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Action>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Action>(inCore, inMetaData, inInstance)
    {
        TypedDerive(inCore, inTyped);
    }
};

// Base class of assets
template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Asset>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Asset>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Guided> &inGuided,
                              SComposerObjectDefinition<ComposerObjectTypes::Named> &inNamed)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Asset>(inCore, inMetaData, inInstance)
    {
        TypedDerive(inCore, inTyped);
        Derive(inCore, inGuided);
        Derive(inCore, inNamed);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Scene>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Scene>
{
    SComposerObjectDefinition(
        IDataCore &inCore, IMetaData &inMetaData, Qt3DSDMInstanceHandle inInstance,
        SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
        SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset,
        SComposerObjectDefinition<ComposerObjectTypes::SlideOwner> &inSlideOwner)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Scene>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inAsset);
        Derive(inCore, inSlideOwner);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Image>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Image>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Image>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inAsset);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::MaterialBase>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::MaterialBase>

{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::MaterialBase>(inCore, inMetaData,
                                                                           inInstance)
    {
        Derive(inCore, inAsset);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Lightmaps>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Lightmaps>

{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::MaterialBase> &inBase)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Lightmaps>(inCore, inMetaData,
                                                                        inInstance)
    {
        Derive(inCore, inBase);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Material>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Material>

{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Lightmaps> &inBase)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Material>(inCore, inMetaData,
                                                                       inInstance)
    {
        Derive(inCore, inBase);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::CustomMaterial>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::CustomMaterial>

{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Lightmaps> &inBase)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::CustomMaterial>(inCore, inMetaData,
                                                                             inInstance)
    {
        Derive(inCore, inBase);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::ReferencedMaterial>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::ReferencedMaterial>

{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Lightmaps> &inBase)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::ReferencedMaterial>(inCore, inMetaData,
                                                                                 inInstance)
    {
        Derive(inCore, inBase);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Behavior>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Behavior>

{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Behavior>(inCore, inMetaData,
                                                                       inInstance)
    {
        Derive(inCore, inAsset);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Effect>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Effect>

{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Effect>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inAsset);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Node>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Node>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Node>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inAsset);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Layer>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Layer>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Layer>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inNode);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Model>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Model>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Model>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inNode);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Group>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Group>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Group>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inNode);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Light>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Light>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Light>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inNode);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Camera>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Camera>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Camera>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inNode);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Text>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Text>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Text>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inNode);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Component>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Component>
{
    SComposerObjectDefinition(
        IDataCore &inCore, IMetaData &inMetaData, Qt3DSDMInstanceHandle inInstance,
        SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
        SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode,
        SComposerObjectDefinition<ComposerObjectTypes::SlideOwner> &inSlideOwner)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Component>(inCore, inMetaData,
                                                                        inInstance)
    {
        Derive(inCore, inNode);
        Derive(inCore, inSlideOwner);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::RenderPlugin>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::RenderPlugin>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::RenderPlugin>(inCore, inMetaData,
                                                                           inInstance)
    {
        Derive(inCore, inAsset);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Alias>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Alias>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Alias>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inNode);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::Path>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::Path>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Node> &inNode)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::Path>(inCore, inMetaData, inInstance)
    {
        Derive(inCore, inNode);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::PathAnchorPoint>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::PathAnchorPoint>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::PathAnchorPoint>(inCore, inMetaData,
                                                                              inInstance)
    {
        Derive(inCore, inAsset);
        SetType(inCore, inTyped);
    }
};

template <>
struct SComposerObjectDefinition<ComposerObjectTypes::SubPath>
    : public SComposerBaseObjectDefinition<ComposerObjectTypes::SubPath>
{
    SComposerObjectDefinition(IDataCore &inCore, IMetaData &inMetaData,
                              Qt3DSDMInstanceHandle inInstance,
                              SComposerObjectDefinition<ComposerObjectTypes::Typed> &inTyped,
                              SComposerObjectDefinition<ComposerObjectTypes::Asset> &inAsset)
        : SComposerBaseObjectDefinition<ComposerObjectTypes::SubPath>(inCore, inMetaData,
                                                                      inInstance)
    {
        Derive(inCore, inAsset);
        SetType(inCore, inTyped);
    }
};

// Container object for all of the object definitions
class SComposerObjectDefinitions
{
public:
#define HANDLE_COMPOSER_OBJECT_TYPE(name, propmacro)                                               \
    SComposerObjectDefinition<ComposerObjectTypes::name> m_##name;
    ITERATE_COMPOSER_OBJECT_TYPES
#undef HANDLE_COMPOSER_OBJECT_TYPE

    SComposerObjectDefinitions(IDataCore &inDataCore, IMetaData &inMetaData);
    SComposerObjectDefinitions() = default;
    ~SComposerObjectDefinitions() = default;

    // RTTI API
    bool IsA(Qt3DSDMInstanceHandle inInstance, ComposerObjectTypes::Enum inType);
    // Could easily return None, meaning we can't identify the object type.
    // Checks the type of the first derivation parent, so this won't ever return
    // SlideOwner, for instance.
    ComposerObjectTypes::Enum GetType(Qt3DSDMInstanceHandle inInstance);

    Qt3DSDMInstanceHandle GetInstanceForType(ComposerObjectTypes::Enum inType);
private:
    IDataCore &m_DataCore;
    SComposerObjectDefinitions(const SComposerObjectDefinitions&) = delete;
    SComposerObjectDefinitions& operator=(const SComposerObjectDefinitions&) = delete;
};
}
