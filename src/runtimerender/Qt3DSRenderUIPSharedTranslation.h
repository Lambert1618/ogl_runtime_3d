/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DS_RENDER_UIP_SHARED_TRANSLATION_H
#define QT3DS_RENDER_UIP_SHARED_TRANSLATION_H
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderDefaultMaterial.h"
#include "Qt3DSRenderImage.h"
#include "Qt3DSRenderText.h"
#include "Qt3DSDMWindowsCompatibility.h"
#include "Qt3DSRenderLayer.h"
#include "Qt3DSRenderModel.h"
#include "Qt3DSRenderPath.h"
#include "Qt3DSRenderPresentation.h"

// map from qt3dsdm to qt3ds::render
namespace qt3ds {
namespace render {

    template <typename TEnumType>
    struct SEnumParseMap
    {
    };

    struct SEnumNameMap
    {
        QT3DSU32 m_Enum;
        const wchar_t *m_WideName;
        const char8_t *m_Name;
    };

    template <>
    struct SEnumParseMap<RenderLightTypes::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<DefaultMaterialLighting::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<DefaultMaterialBlendMode::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<ImageMappingModes::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<NVRenderTextureCoordOp::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<TextHorizontalAlignment::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<TextVerticalAlignment::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<TextWordWrap::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<TextElide::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<AAModeValues::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<LayerBlendTypes::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<RenderRotationValues::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<CameraScaleModes::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<CameraScaleAnchors::Enum>
    {
        static SEnumNameMap *GetMap();
    };
    template <>
    struct SEnumParseMap<HorizontalFieldValues::Enum>
    {
        static SEnumNameMap *GetMap();
    };
    template <>
    struct SEnumParseMap<VerticalFieldValues::Enum>
    {
        static SEnumNameMap *GetMap();
    };
    template <>
    struct SEnumParseMap<LayerUnitTypes::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<LayerBackground::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<DefaultMaterialSpecularModel::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<TessModeValues::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<PathCapping::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<PathTypes::Enum>
    {
        static SEnumNameMap *GetMap();
    };

    template <>
    struct SEnumParseMap<PathPaintStyles::Enum>
    {
        static SEnumNameMap *GetMap();
    };

#define QT3DS_RENDER_WCHAR_T_XYZs L"XYZ"
#define QT3DS_RENDER_WCHAR_T_YZXs L"YZX"
#define QT3DS_RENDER_WCHAR_T_ZXYs L"ZXY"
#define QT3DS_RENDER_WCHAR_T_XZYs L"XZY"
#define QT3DS_RENDER_WCHAR_T_YXZs L"YXZ"
#define QT3DS_RENDER_WCHAR_T_ZYXs L"ZYX"

#define QT3DS_RENDER_WCHAR_T_XYZr L"XYZr"
#define QT3DS_RENDER_WCHAR_T_YZXr L"YZXr"
#define QT3DS_RENDER_WCHAR_T_ZXYr L"ZXYr"
#define QT3DS_RENDER_WCHAR_T_XZYr L"XZYr"
#define QT3DS_RENDER_WCHAR_T_YXZr L"YXZr"
#define QT3DS_RENDER_WCHAR_T_ZYXr L"ZYXr"

#define QT3DS_RENDER_CHAR_T_XYZs "XYZ"
#define QT3DS_RENDER_CHAR_T_YZXs "YZX"
#define QT3DS_RENDER_CHAR_T_ZXYs "ZXY"
#define QT3DS_RENDER_CHAR_T_XZYs "XZY"
#define QT3DS_RENDER_CHAR_T_YXZs "YXZ"
#define QT3DS_RENDER_CHAR_T_ZYXs "ZYX"

#define QT3DS_RENDER_CHAR_T_XYZr "XYZr"
#define QT3DS_RENDER_CHAR_T_YZXr "YZXr"
#define QT3DS_RENDER_CHAR_T_ZXYr "ZXYr"
#define QT3DS_RENDER_CHAR_T_XZYr "XZYr"
#define QT3DS_RENDER_CHAR_T_YXZr "YXZr"
#define QT3DS_RENDER_CHAR_T_ZYXr "ZYXr"

    inline QT3DSU32 MapRotationOrder(const wchar_t *inOrderStr)
    {
#define MAP_ROTATION_ORDER(name, postfix)                                                          \
    if (wcscmp(inOrderStr, QT3DS_RENDER_WCHAR_T_##name##postfix) == 0) {                             \
        return EulOrd##name##postfix;                                                              \
    }
        MAP_ROTATION_ORDER(XYZ, s);
        MAP_ROTATION_ORDER(YZX, s);
        MAP_ROTATION_ORDER(ZXY, s);
        MAP_ROTATION_ORDER(XZY, s);
        MAP_ROTATION_ORDER(YXZ, s);
        MAP_ROTATION_ORDER(ZYX, s);
        MAP_ROTATION_ORDER(XYZ, r);
        MAP_ROTATION_ORDER(YZX, r);
        MAP_ROTATION_ORDER(ZXY, r);
        MAP_ROTATION_ORDER(XZY, r);
        MAP_ROTATION_ORDER(YXZ, r);
        MAP_ROTATION_ORDER(ZYX, r);
#undef MAP_ROTATION_ORDER
        return EulOrdYXZs;
    }

    inline QT3DSU32 MapRotationOrder(const char8_t *inOrderStr)
    {
#define MAP_ROTATION_ORDER(name, postfix)                                                          \
    if (strcmp(inOrderStr, QT3DS_RENDER_CHAR_T_##name##postfix) == 0) {                              \
        return EulOrd##name##postfix;                                                              \
    }
        MAP_ROTATION_ORDER(XYZ, s);
        MAP_ROTATION_ORDER(YZX, s);
        MAP_ROTATION_ORDER(ZXY, s);
        MAP_ROTATION_ORDER(XZY, s);
        MAP_ROTATION_ORDER(YXZ, s);
        MAP_ROTATION_ORDER(ZYX, s);
        MAP_ROTATION_ORDER(XYZ, r);
        MAP_ROTATION_ORDER(YZX, r);
        MAP_ROTATION_ORDER(ZXY, r);
        MAP_ROTATION_ORDER(XZY, r);
        MAP_ROTATION_ORDER(YXZ, r);
        MAP_ROTATION_ORDER(ZYX, r);
#undef MAP_ROTATION_ORDER
        return EulOrdYXZs;
    }

    // the goal is to unify the systems that transfer information into the UICRender library.
    // There are currently three such systems; the runtime, studio, and the uip loader that loads
    // uip files
    // directly into the render library.
    // To do this, we need to have a mapping between a generic key and a given property on every
    // object
    // along with some information about what portion of the object model this property affects.

    struct Qt3DSRenderDirtyFlags
    {
        enum Enum {
            Unknown = 0,
            Dirty = 1 << 0,
            TransformDirty = 1 << 1,
            TextDirty = 1 << 2,
        };
    };

// Now we build out generic macros with no implementation that list all of the properties
// on each struct that we care about.  We will fill in these macros with implementation later.
// Each macro will list the property name along with what dirty operation should get marked
// Global parse tables that list every property used by the system.

#define ITERATE_QT3DS_RENDER_SCENE_PROPERTIES                                                        \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Scene, ClearColor, Dirty)                                     \
    HANDLE_QT3DS_RENDER_PROPERTY(Scene, UseClearColor, Dirty)

#define ITERATE_QT3DS_RENDER_NODE_PROPERTIES                                                         \
    HANDLE_QT3DS_RENDER_TRANSFORM_VEC3_PROPERTY(Node, Rotation, TransformDirty)                      \
    HANDLE_QT3DS_RENDER_VEC3_RADIAN_PROPERTY(Node, Rotation, TransformDirty)                         \
    HANDLE_QT3DS_RENDER_TRANSFORM_VEC3_PROPERTY(Node, Position, TransformDirty)                      \
    HANDLE_QT3DS_RENDER_VEC3_PROPERTY(Node, Position, TransformDirty)                                \
    HANDLE_QT3DS_RENDER_TRANSFORM_VEC3_PROPERTY(Node, Scale, TransformDirty)                         \
    HANDLE_QT3DS_RENDER_VEC3_PROPERTY(Node, Scale, TransformDirty)                                   \
    HANDLE_QT3DS_RENDER_TRANSFORM_VEC3_PROPERTY(Node, Pivot, TransformDirty)                         \
    HANDLE_QT3DS_RENDER_VEC3_PROPERTY(Node, Pivot, TransformDirty)                                   \
    HANDLE_QT3DS_RENDER_OPACITY_PROPERTY(Node, LocalOpacity, TransformDirty)                         \
    HANDLE_QT3DS_ROTATION_ORDER_PROPERTY(Node, RotationOrder, TransformDirty)                        \
    HANDLE_QT3DS_NODE_ORIENTATION_PROPERTY(Node, LeftHanded, TransformDirty)

#define ITERATE_QT3DS_RENDER_LAYER_PROPERTIES                                                        \
    HANDLE_QT3DS_NODE_FLAGS_INVERSE_PROPERTY(Layer, LayerEnableDepthTest, Dirty)                     \
    HANDLE_QT3DS_NODE_FLAGS_INVERSE_PROPERTY(Layer, LayerEnableDepthPrePass, Dirty)                  \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, ProgressiveAAMode, Dirty)                               \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, MultisampleAAMode, Dirty)                               \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, TemporalAAEnabled, Dirty)                                    \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Layer, ClearColor, Dirty)                                     \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, BlendType, Dirty)                                       \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, Background, Dirty)                                      \
    HANDLE_QT3DS_RENDER_SOURCEPATH_PROPERTY(Layer, TexturePath, Dirty)                               \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, HorizontalFieldValues, Dirty)                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Left, Dirty)                                                 \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, LeftUnits, Dirty)                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Width, Dirty)                                                \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, WidthUnits, Dirty)                                      \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Right, Dirty)                                                \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, RightUnits, Dirty)                                      \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, VerticalFieldValues, Dirty)                             \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Top, Dirty)                                                  \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, TopUnits, Dirty)                                        \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Height, Dirty)                                               \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, HeightUnits, Dirty)                                     \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Bottom, Dirty)                                               \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Layer, BottomUnits, Dirty)                                     \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, AoStrength, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, AoDistance, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, AoSoftness, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, AoBias, Dirty)                                               \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, AoDither, Dirty)                                             \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, ShadowStrength, Dirty)                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, ShadowDist, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, ShadowSoftness, Dirty)                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, ShadowBias, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, LightProbe, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, ProbeBright, Dirty)                                          \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, FastIbl, Dirty)                                              \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, ProbeHorizon, Dirty)                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, ProbeFov, Dirty)                                             \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, LightProbe2, Dirty)                                          \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Probe2Fade, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Probe2Window, Dirty)                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Layer, Probe2Pos, Dirty)

#define ITERATE_QT3DS_RENDER_CAMERA_PROPERTIES                                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Camera, ClipNear, Dirty)                                            \
    HANDLE_QT3DS_RENDER_PROPERTY(Camera, ClipFar, Dirty)                                             \
    HANDLE_QT3DS_RENDER_RADIAN_PROPERTY(Camera, FOV, Dirty)                                          \
    HANDLE_QT3DS_RENDER_PROPERTY(Camera, FOVHorizontal, Dirty)                                          \
    HANDLE_QT3DS_RENDER_PROPERTY(Camera, EnableFrustumCulling, Dirty)                                \
    HANDLE_QT3DS_NODE_FLAGS_PROPERTY(Camera, Orthographic, Dirty)                                    \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Camera, ScaleMode, Dirty)                                      \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Camera, ScaleAnchor, Dirty)

#define ITERATE_QT3DS_RENDER_LIGHT_PROPERTIES                                                        \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Light, LightType, Dirty)                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, Scope, Dirty)                                                \
    HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(Light, DiffuseColor, Dirty)                              \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Light, DiffuseColor, Dirty)                                   \
    HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(Light, SpecularColor, Dirty)                             \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Light, SpecularColor, Dirty)                                  \
    HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(Light, AmbientColor, Dirty)                              \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Light, AmbientColor, Dirty)                                   \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, Brightness, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, LinearFade, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, ExponentialFade, Dirty)                                      \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, AreaWidth, Dirty)                                            \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, AreaHeight, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, CastShadow, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, ShadowBias, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, ShadowFactor, Dirty)                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, ShadowMapFar, Dirty)                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, ShadowMapFov, Dirty)                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Light, ShadowFilter, Dirty)

#define ITERATE_QT3DS_RENDER_MODEL_PROPERTIES                                                        \
    HANDLE_QT3DS_RENDER_SOURCEPATH_PROPERTY(Model, MeshPath, Dirty)                                  \
    HANDLE_QT3DS_RENDER_PROPERTY(Model, ShadowCaster, Dirty)                                         \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Model, TessellationMode, Dirty)                                \
    HANDLE_QT3DS_RENDER_PROPERTY(Model, EdgeTess, Dirty)                                             \
    HANDLE_QT3DS_RENDER_PROPERTY(Model, InnerTess, Dirty)

#define ITERATE_QT3DS_RENDER_CUSTOM_MATERIAL_PROPERTIES                                              \
    HANDLE_QT3DS_RENDER_PROPERTY(MaterialBase, IblProbe, Dirty)

#define ITERATE_QT3DS_RENDER_LIGHTMAP_PROPERTIES                                                     \
    HANDLE_QT3DS_RENDER_PROPERTY(Lightmaps, LightmapIndirect, Dirty)                                 \
    HANDLE_QT3DS_RENDER_PROPERTY(Lightmaps, LightmapRadiosity, Dirty)                                \
    HANDLE_QT3DS_RENDER_PROPERTY(Lightmaps, LightmapShadow, Dirty)

#define ITERATE_QT3DS_RENDER_MATERIAL_PROPERTIES                                                     \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Material, Lighting, Dirty)                                     \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Material, BlendMode, Dirty)                                    \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, VertexColors, Dirty)                                      \
    HANDLE_QT3DS_RENDER_PROPERTY(MaterialBase, IblProbe, Dirty)                                      \
    HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(Material, DiffuseColor, Dirty)                           \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Material, DiffuseColor, Dirty)                                \
    HANDLE_QT3DS_RENDER_ARRAY_PROPERTY(Material, DiffuseMaps, 0, Dirty)                              \
    HANDLE_QT3DS_RENDER_ARRAY_PROPERTY(Material, DiffuseMaps, 1, Dirty)                              \
    HANDLE_QT3DS_RENDER_ARRAY_PROPERTY(Material, DiffuseMaps, 2, Dirty)                              \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, EmissivePower, Dirty)                                     \
    HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(Material, EmissiveColor, Dirty)                          \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Material, EmissiveColor, Dirty)                               \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, EmissiveMap, Dirty)                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, EmissiveMap2, Dirty)                                      \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, SpecularReflection, Dirty)                                \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, SpecularMap, Dirty)                                       \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Material, SpecularModel, Dirty)                                \
    HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(Material, SpecularTint, Dirty)                           \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Material, SpecularTint, Dirty)                                \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, FresnelPower, Dirty)                                      \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, IOR, Dirty)                                               \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, SpecularAmount, Dirty)                                    \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, SpecularRoughness, Dirty)                                 \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, RoughnessMap, Dirty)                                      \
    HANDLE_QT3DS_RENDER_OPACITY_PROPERTY(Material, Opacity, Dirty)                                   \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, OpacityMap, Dirty)                                        \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, BumpMap, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, BumpAmount, Dirty)                                        \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, NormalMap, Dirty)                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, DisplacementMap, Dirty)                                   \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, DisplaceAmount, Dirty)                                    \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, TranslucencyMap, Dirty)                                   \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, TranslucentFalloff, Dirty)                                \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, DiffuseLightWrap, Dirty)

#define ITERATE_QT3DS_RENDER_REFERENCED_MATERIAL_PROPERTIES                                          \
    HANDLE_QT3DS_RENDER_PROPERTY(Material, ReferencedMaterial, Dirty)

#define ITERATE_QT3DS_RENDER_IMAGE_PROPERTIES                                                        \
    HANDLE_QT3DS_RENDER_SOURCEPATH_PROPERTY(Image, ImagePath, Dirty)                                 \
    HANDLE_QT3DS_RENDER_PROPERTY(Image, OffscreenRendererId, Dirty)                                  \
    HANDLE_QT3DS_RENDER_VEC2_PROPERTY(Image, Scale, TransformDirty)                                  \
    HANDLE_QT3DS_RENDER_VEC2_PROPERTY(Image, Pivot, TransformDirty)                                  \
    HANDLE_QT3DS_RENDER_RADIAN_PROPERTY(Image, Rotation, TransformDirty)                             \
    HANDLE_QT3DS_RENDER_VEC2_PROPERTY(Image, Position, TransformDirty)                               \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Image, MappingMode, Dirty)                                     \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Image, HorizontalTilingMode, Dirty)                            \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Image, VerticalTilingMode, Dirty)

#define ITERATE_QT3DS_RENDER_TEXT_PROPERTIES                                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, Text, TextDirty)                                              \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, Font, TextDirty)                                              \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, FontSize, TextDirty)                                          \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Text, HorizontalAlignment, TextDirty)                          \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Text, VerticalAlignment, TextDirty)                            \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, Leading, TextDirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, Tracking, TextDirty)                                          \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, DropShadow, TextDirty)                                        \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, DropShadowStrength, TextDirty)                                \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, DropShadowOffsetX, TextDirty)                                 \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, DropShadowOffsetY, TextDirty)                                 \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Text, WordWrap, TextDirty)                                     \
    HANDLE_QT3DS_RENDER_REAL_VEC2_PROPERTY(Text, BoundingBox, TextDirty)                             \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Text, Elide, TextDirty)                                        \
    HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(Text, TextColor, Dirty)                                  \
    HANDLE_QT3DS_RENDER_COLOR_PROPERTY(Text, TextColor, Dirty)                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Text, EnableAcceleratedFont, Dirty)

#define ITERATE_QT3DS_RENDER_PATH_PROPERTIES                                                         \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Path, PathType, Dirty)                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, Width, Dirty)                                                 \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, LinearError, Dirty)                                           \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, EdgeTessAmount, Dirty)                                        \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, InnerTessAmount, Dirty)                                       \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Path, BeginCapping, Dirty)                                     \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, BeginCapOffset, Dirty)                                        \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, BeginCapOpacity, Dirty)                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, BeginCapWidth, Dirty)                                         \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Path, EndCapping, Dirty)                                       \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, EndCapOffset, Dirty)                                          \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, EndCapOpacity, Dirty)                                         \
    HANDLE_QT3DS_RENDER_PROPERTY(Path, EndCapWidth, Dirty)                                           \
    HANDLE_QT3DS_RENDER_ENUM_PROPERTY(Path, PaintStyle, Dirty)                                       \
    HANDLE_QT3DS_RENDER_SOURCEPATH_PROPERTY(Path, PathBuffer, Dirty)

#define ITERATE_QT3DS_RENDER_PATH_SUBPATH_PROPERTIES                                                 \
    HANDLE_QT3DS_RENDER_PROPERTY(SubPath, Closed, Dirty)
}
}
#endif
