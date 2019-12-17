/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "EnginePrefix.h"
#include "Qt3DSRenderRuntimeBindingImpl.h"
#include "Qt3DSRenderUIPSharedTranslation.h"
#include "Qt3DSRenderBufferManager.h"
#include "foundation/SerializationTypes.h"
#include "StringTools.h"
#include "foundation/FileTools.h"
#include "Qt3DSHash.h"
#include "Qt3DSRenderPlugin.h"
#include "Qt3DSRenderPluginPropertyValue.h"
#include "Qt3DSElementHelper.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSApplication.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderPath.h"
#include "Qt3DSRenderPathSubPath.h"
#include "Qt3DSRenderPathManager.h"

using namespace qt3ds::foundation;

namespace Q3DStudio {
enum ExtendedAttributes {
    ATTRIBUTE_NONE = 0,
    ATTRIBUTE_NONE_R,
    ATTRIBUTE_NONE_G,
    ATTRIBUTE_NONE_B
};
}

namespace {
QT3DSU32 g_TranslatorTag;
QT3DSU32 g_PresentationTag;
}

// Add specializations for the tagged pointer operations to make casting
// a bit safer and more transparent.
namespace qt3ds {
namespace render {
    template <>
    struct SPointerTag<Qt3DSTranslator>
    {
        static QT3DSU32 GetTag() { return g_TranslatorTag; }
    };
    template <>
    struct SPointerTag<Q3DStudio::IPresentation>
    {
        static QT3DSU32 GetTag() { return g_PresentationTag; }
    };
}
}

namespace {

template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
struct SDirtySetter
{
};

template <>
struct SDirtySetter<Qt3DSRenderDirtyFlags::Dirty>
{
    static void Set(bool &dirty, bool & /*transformDirty*/, bool & /*textDirty*/) { dirty = true; }
};
template <>
struct SDirtySetter<Qt3DSRenderDirtyFlags::TransformDirty>
{
    static void Set(bool &dirty, bool &transformDirty, bool & /*textDirty*/)
    {
        dirty = true;
        transformDirty = true;
    }
};
template <>
struct SDirtySetter<Qt3DSRenderDirtyFlags::TextDirty>
{
    static void Set(bool &dirty, bool & /*transformDirty*/, bool &textDirty)
    {
        dirty = true;
        textDirty = true;
    }
};
template <>
struct SDirtySetter<Qt3DSRenderDirtyFlags::Unknown>
{
    static void Set(bool & /*dirty*/, bool & /*transformDirty*/, bool & /*textDirty*/) {}
};

// Translates individual property values from the runtime into the rendering system.
struct SRuntimePropertyParser
{
    QT3DSI32 m_PropertyName;
    Q3DStudio::UVariant m_Value;
    Q3DStudio::EAttributeType m_Type;
    Q3DStudio::TElement &m_Element;
    SPresentation &m_Presentation;
    IQt3DSRenderContext &m_RenderContext;
    bool m_Dirty;
    bool m_TransformDirty;
    bool m_TextDirty;

    SRuntimePropertyParser(SPresentation &inPresentation, IQt3DSRenderContext &inRenderContext,
                           Q3DStudio::TElement &inElement)
        : m_PropertyName(0)
        , m_Type((Q3DStudio::EAttributeType)0)
        , m_Element(inElement)
        , m_Presentation(inPresentation)
        , m_RenderContext(inRenderContext)
        , m_Dirty(false)
        , m_TransformDirty(false)
        , m_TextDirty(false)
    {
    }

    void Setup(QT3DSI32 inPropName, Q3DStudio::UVariant inValue, Q3DStudio::EAttributeType inType)
    {
        m_PropertyName = inPropName;
        m_Value = inValue;
        m_Type = inType;
    }
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    void SetDirty()
    {
        SDirtySetter<TDirtyType>::Set(m_Dirty, m_TransformDirty, m_TextDirty);
    }
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(QT3DSF32 &outValue)
    {
        if (m_Type == Q3DStudio::ATTRIBUTETYPE_FLOAT) {
            QT3DSF32 newValue = m_Value.m_FLOAT;
            if (outValue != newValue) {
                outValue = newValue;
                SetDirty<TDirtyType>();
                return true;
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return false;
    };
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(QT3DSU32 &outValue)
    {
        if (m_Type == Q3DStudio::ATTRIBUTETYPE_INT32) {
            QT3DSU32 newValue = (QT3DSU32)m_Value.m_INT32;
            if (outValue != newValue) {
                outValue = newValue;
                SetDirty<TDirtyType>();
                return true;
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return false;
    };
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(SGraphObject *&outValue)
    {
        Q3DStudio::TElement *theElem = NULL;
        if (m_Type == Q3DStudio::ATTRIBUTETYPE_ELEMENTREF) {
            theElem = m_Element.GetBelongedPresentation()->GetApplication().GetElementByHandle(
                m_Value.m_ElementHandle);
        } else if (m_Type == Q3DStudio::ATTRIBUTETYPE_STRING) {
            CRegisteredString theString =
                m_RenderContext.GetStringTable().HandleToStr(m_Value.m_StringHandle);
            theElem = Q3DStudio::CElementHelper::GetElement(
                m_Element.GetBelongedPresentation()->GetApplication(),
                m_Element.GetBelongedPresentation(), theString.c_str(), &m_Element);
        }

        SGraphObject *newValue = NULL;
        if (theElem) {
            Qt3DSTranslator *theTranslator =
                reinterpret_cast<Qt3DSTranslator *>(theElem->GetAssociation());
            if (theTranslator)
                newValue = theTranslator->m_RenderObject;
        }
        if (outValue != newValue) {
            outValue = newValue;
            SetDirty<TDirtyType>();
            return true;
        }
        return false;
    };
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseRadianProperty(QT3DSF32 &outValue)
    {
        if (ParseProperty<TDirtyType>(outValue)) {
            TORAD(outValue);
            return true;
        }
        return false;
    }
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseOpacityProperty(QT3DSF32 &outValue)
    {
        if (ParseProperty<TDirtyType>(outValue)) {
            outValue *= 1.0f / 100.0f;
            return true;
        }
        return false;
    }

    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseOrientationProperty(bool &ioCurrent)
    {
        if (m_Type == Q3DStudio::ATTRIBUTETYPE_STRING) {
            CRegisteredString strValue =
                m_RenderContext.GetStringTable().HandleToStr(m_Value.m_StringHandle);
            bool newValue = AreEqual(strValue.c_str(), "Left Handed") ? true : false;
            if (ioCurrent != newValue) {
                ioCurrent = newValue;
                SetDirty<TDirtyType>();
                return true;
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return false;
    }

    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(bool &ioCurrent)
    {
        if (m_Type == Q3DStudio::ATTRIBUTETYPE_BOOL || m_Type == Q3DStudio::ATTRIBUTETYPE_INT32) {
            bool newValue = m_Value.m_INT32 ? true : false;
            if (ioCurrent != newValue) {
                ioCurrent = newValue;
                SetDirty<TDirtyType>();
                return true;
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return false;
    }
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseInverseBoolean(bool &ioCurrent)
    {
        bool temp = !ioCurrent;
        if (ParseProperty<TDirtyType>(temp)) {
            ioCurrent = temp;
            return true;
        }
        return false;
    }
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(QT3DSVec3 &outValue)
    {
        if (m_Type == Q3DStudio::ATTRIBUTETYPE_FLOAT3) {
            QT3DSVec3 newValue(m_Value.m_FLOAT3[0], m_Value.m_FLOAT3[1], m_Value.m_FLOAT3[2]);
            if (outValue != newValue) {
                outValue = newValue;
                SetDirty<TDirtyType>();
                return true;
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return false;
    }
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(QT3DSVec4 &outValue)
    {
        if (m_Type == Q3DStudio::ATTRIBUTETYPE_FLOAT4) {
            QT3DSVec4 newValue(m_Value.m_FLOAT4[0], m_Value.m_FLOAT4[1], m_Value.m_FLOAT4[2],
                               m_Value.m_FLOAT4[3]);
            if (outValue != newValue) {
                outValue = newValue;
                SetDirty<TDirtyType>();
                return true;
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return false;
    }


    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(CRegisteredString &outValue)
    {
        if (m_Type == Q3DStudio::ATTRIBUTETYPE_STRING) {
            CRegisteredString newValue =
                m_RenderContext.GetStringTable().HandleToStr(m_Value.m_StringHandle);
            if (newValue.c_str() != outValue.c_str()) {
                outValue = newValue;
                SetDirty<TDirtyType>();
                return true;
            }
        } else {
            QT3DS_ASSERT(false);
        }
        return false;
    }

    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseRotationOrder(QT3DSU32 &outValue)
    {
        CRegisteredString theString;
        ParseProperty<Qt3DSRenderDirtyFlags::Unknown>(theString);
        if (theString.IsValid()) {
            QT3DSU32 newRotationOrder = MapRotationOrder(theString);
            if (newRotationOrder != outValue) {
                outValue = newRotationOrder;
                SetDirty<TDirtyType>();
                return true;
            }
        }
        return false;
    }

    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseOrientation(NodeFlags &outValue)
    {
        bool temp = false;
        ParseOrientationProperty<TDirtyType>(temp);
        bool wasLeftHanded = outValue.IsLeftHanded();
        if (wasLeftHanded != temp) {
            outValue.SetLeftHanded(temp);
            SetDirty<TDirtyType>();
            return true;
        }
        return false;
    }
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseNodeFlagsProperty(NodeFlags &outValue, NodeFlagValues::Enum inFlag)
    {
        bool temp = false;
        ParseProperty<Qt3DSRenderDirtyFlags::Unknown>(temp);
        bool wasSet = outValue & inFlag;
        if (temp != wasSet) {
            outValue.ClearOrSet(temp, inFlag);
            SetDirty<TDirtyType>();
            return true;
        }
        return false;
    }

    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseNodeFlagsInverseProperty(NodeFlags &outValue, NodeFlagValues::Enum inFlag)
    {
        bool temp = false;
        ParseProperty<Qt3DSRenderDirtyFlags::Unknown>(temp);
        temp = !temp;
        bool wasSet = outValue & inFlag;
        if (temp != wasSet) {
            outValue.ClearOrSet(temp, inFlag);
            SetDirty<TDirtyType>();
            return true;
        }
        return false;
    }

    template <Qt3DSRenderDirtyFlags::Enum TDirtyType, typename TEnumType>
    bool ParseEnumProperty(TEnumType &outValue)
    {
        CRegisteredString theString;
        ParseProperty<Qt3DSRenderDirtyFlags::Unknown>(theString);
        if (theString.IsValid()) {
            SEnumNameMap *theMap = SEnumParseMap<TEnumType>::GetMap();
            for (SEnumNameMap *theItem = theMap; theItem->m_Name && *theItem->m_Name; ++theItem) {
                // hack to match advanced overlay types, whose name start with a '*'
                const char8_t *p = theString.c_str();
                if (*p == '*')
                    ++p;
                if (strcmp(p, theItem->m_Name) == 0) {
                    TEnumType theNewValue = static_cast<TEnumType>(theItem->m_Enum);
                    if (outValue != theNewValue) {
                        outValue = theNewValue;
                        SetDirty<TDirtyType>();
                        return true;
                    }
                }
            }
        }
        return false;
    }
    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseAndResolveSourcePath(CRegisteredString &outValue)
    {
        CRegisteredString theTemp;
        ParseProperty<Qt3DSRenderDirtyFlags::Unknown>(theTemp);
        CRegisteredString theNewStr = theTemp;
        if (outValue.c_str() != theNewStr.c_str()) {
            SetDirty<TDirtyType>();
            outValue = theNewStr;
            return true;
        }
        return false;
    }

    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(SImage *&ioImagePtr)
    {
        Q3DStudio::TElement *theElem =
            m_Element.GetBelongedPresentation()->GetApplication().GetElementByHandle(
                m_Value.m_ElementHandle);
        // Try to be very careful in here as casting things around like crazy is a sure
        // way to crash the runtime at a bad point.
        if (theElem != NULL) {
            Qt3DSTranslator *theTranslator =
                reinterpret_cast<Qt3DSTranslator *>(theElem->GetAssociation());
            if (theTranslator != NULL && theTranslator->GetUIPType() == GraphObjectTypes::Image) {
                SImage *theImage = static_cast<SImage *>(theTranslator->m_RenderObject);
                if (ioImagePtr != theImage) {
                    ioImagePtr = theImage;
                    SetDirty<TDirtyType>();
                    return true;
                }
            }
        }
        return false;
    }

    template <Qt3DSRenderDirtyFlags::Enum TDirtyType>
    bool ParseProperty(SNode *&ioNodePtr)
    {
        Q3DStudio::TElement *theElem =
            m_Element.GetBelongedPresentation()->GetApplication().GetElementByHandle(
                m_Value.m_ElementHandle);
        // Try to be very careful in here as casting things around like crazy is a sure
        // way to crash the runtime at a bad point.
        SNode *theNode = NULL;
        if (theElem != NULL
            && theElem->GetBelongedPresentation() == m_Element.GetBelongedPresentation()) {
            Qt3DSTranslator *theTranslator =
                reinterpret_cast<Qt3DSTranslator *>(theElem->GetAssociation());
            if (theTranslator != NULL
                && GraphObjectTypes::IsNodeType(theTranslator->GetUIPType())) {
                theNode = static_cast<SNode *>(theTranslator->m_RenderObject);
            }
        }
        if (ioNodePtr != theNode) {
            ioNodePtr = theNode;
            SetDirty<TDirtyType>();
            return true;
        }
        return false;
    }
};

// Fill out parse name table.
#define Scene_ClearColor_R ATTRIBUTE_BACKGROUNDCOLOR_R
#define Scene_ClearColor_G ATTRIBUTE_BACKGROUNDCOLOR_G
#define Scene_ClearColor_B ATTRIBUTE_BACKGROUNDCOLOR_B
#define Scene_ClearColor_A ATTRIBUTE_BACKGROUNDCOLOR_A
#define Scene_UseClearColor ATTRIBUTE_BGCOLORENABLE
#define Node_Rotation ATTRIBUTE_ROTATION
#define Node_Rotation_X ATTRIBUTE_ROTATION_X
#define Node_Rotation_Y ATTRIBUTE_ROTATION_Y
#define Node_Rotation_Z ATTRIBUTE_ROTATION_Z
#define Node_Position ATTRIBUTE_POSITION
#define Node_Position_X ATTRIBUTE_POSITION_X
#define Node_Position_Y ATTRIBUTE_POSITION_Y
#define Node_Position_Z ATTRIBUTE_POSITION_Z
#define Node_Scale ATTRIBUTE_SCALE
#define Node_Scale_X ATTRIBUTE_SCALE_X
#define Node_Scale_Y ATTRIBUTE_SCALE_Y
#define Node_Scale_Z ATTRIBUTE_SCALE_Z
#define Node_Pivot ATTRIBUTE_PIVOT
#define Node_Pivot_X ATTRIBUTE_PIVOT_X
#define Node_Pivot_Y ATTRIBUTE_PIVOT_Y
#define Node_Pivot_Z ATTRIBUTE_PIVOT_Z
#define Node_LocalOpacity ATTRIBUTE_OPACITY
#define Node_RotationOrder ATTRIBUTE_ROTATIONORDER
#define Node_LeftHanded ATTRIBUTE_ORIENTATION
#define Group_ordered ATTRIBUTE_ORDERED
#define Layer_TemporalAAEnabled ATTRIBUTE_TEMPORALAA
#define Layer_LayerEnableDepthTest ATTRIBUTE_DISABLEDEPTHTEST
#define Layer_LayerEnableDepthPrePass ATTRIBUTE_DISABLEDEPTHPREPASS
#define Layer_ClearColor_R ATTRIBUTE_BACKGROUNDCOLOR_R
#define Layer_ClearColor_G ATTRIBUTE_BACKGROUNDCOLOR_G
#define Layer_ClearColor_B ATTRIBUTE_BACKGROUNDCOLOR_B
#define Layer_ClearColor_A ATTRIBUTE_BACKGROUNDCOLOR_A
#define Layer_Background ATTRIBUTE_BACKGROUND
#define Layer_BlendType ATTRIBUTE_BLENDTYPE
#define Layer_ProgressiveAAMode ATTRIBUTE_PROGRESSIVEAA
#define Layer_MultisampleAAMode ATTRIBUTE_MULTISAMPLEAA
#define Layer_HorizontalFieldValues ATTRIBUTE_HORZFIELDS
#define Layer_Left ATTRIBUTE_LEFT
#define Layer_LeftUnits ATTRIBUTE_LEFTUNITS
#define Layer_Width ATTRIBUTE_WIDTH
#define Layer_WidthUnits ATTRIBUTE_WIDTHUNITS
#define Layer_Right ATTRIBUTE_RIGHT
#define Layer_RightUnits ATTRIBUTE_RIGHTUNITS
#define Layer_VerticalFieldValues ATTRIBUTE_VERTFIELDS
#define Layer_Top ATTRIBUTE_TOP
#define Layer_TopUnits ATTRIBUTE_TOPUNITS
#define Layer_Height ATTRIBUTE_HEIGHT
#define Layer_HeightUnits ATTRIBUTE_HEIGHTUNITS
#define Layer_Bottom ATTRIBUTE_BOTTOM
#define Layer_BottomUnits ATTRIBUTE_BOTTOMUNITS
#define Layer_AoStrength ATTRIBUTE_AOSTRENGTH
#define Layer_AoDistance ATTRIBUTE_AODISTANCE
#define Layer_AoSoftness ATTRIBUTE_AOSOFTNESS
#define Layer_AoBias ATTRIBUTE_AOBIAS
#define Layer_AoSamplerate ATTRIBUTE_AOSAMPLERATE
#define Layer_AoDither ATTRIBUTE_AODITHER
#define Layer_ShadowStrength ATTRIBUTE_SHADOWSTRENGTH
#define Layer_ShadowDist ATTRIBUTE_SHADOWDIST
#define Layer_ShadowSoftness ATTRIBUTE_SHADOWSOFTNESS
#define Layer_ShadowBias ATTRIBUTE_SHADOWBIAS
#define Layer_LightProbe ATTRIBUTE_LIGHTPROBE
#define Layer_ProbeBright ATTRIBUTE_PROBEBRIGHT
#define Layer_FastIbl ATTRIBUTE_FASTIBL
#define Layer_ProbeTheta ATTRIBUTE_PROBETHETA
#define Layer_ProbePhi ATTRIBUTE_PROBEPHI
#define Layer_ProbeHorizon ATTRIBUTE_PROBEHORIZON
#define Layer_LightProbe2 ATTRIBUTE_LIGHTPROBE2
#define Layer_Probe2Fade ATTRIBUTE_PROBE2FADE
#define Layer_Probe2Window ATTRIBUTE_PROBE2WINDOW
#define Layer_Probe2Pos ATTRIBUTE_PROBE2POS
#define Layer_ProbeFov ATTRIBUTE_PROBEFOV
#define Layer_TexturePath ATTRIBUTE_SOURCEPATH
#define Camera_ClipNear ATTRIBUTE_CLIPNEAR
#define Camera_ClipFar ATTRIBUTE_CLIPFAR
#define Camera_FOV ATTRIBUTE_FOV
#define Camera_FOVHorizontal ATTRIBUTE_FOVHORIZONTAL
#define Camera_EnableFrustumCulling ATTRIBUTE_ENABLEFRUSTUMCULLING
#define Camera_Orthographic ATTRIBUTE_ORTHOGRAPHIC
#define Camera_ScaleMode ATTRIBUTE_SCALEMODE
#define Camera_ScaleAnchor ATTRIBUTE_SCALEANCHOR
#define Light_ImageSource ATTRIBUTE_IMAGESOURCE
#define Light_Scope ATTRIBUTE_SCOPE
#define Light_ImageSetsColor ATTRIBUTE_IMAGESETSCOLOR
#define Light_ImageSetsRotation ATTRIBUTE_IMAGESETSROTATION
#define Light_ImageHFov ATTRIBUTE_IMAGEHFOV
#define Light_ImageIsFisheye ATTRIBUTE_IMAGEISFISHEYE
#define Light_LightType ATTRIBUTE_LIGHTTYPE
#define Light_DiffuseColor ATTRIBUTE_LIGHTDIFFUSE
#define Light_DiffuseColor_R ATTRIBUTE_LIGHTDIFFUSE_R
#define Light_DiffuseColor_G ATTRIBUTE_LIGHTDIFFUSE_G
#define Light_DiffuseColor_B ATTRIBUTE_LIGHTDIFFUSE_B
#define Light_DiffuseColor_A ATTRIBUTE_LIGHTDIFFUSE_A
#define Light_SpecularColor ATTRIBUTE_LIGHTSPECULAR
#define Light_SpecularColor_R ATTRIBUTE_LIGHTSPECULAR_R
#define Light_SpecularColor_G ATTRIBUTE_LIGHTSPECULAR_G
#define Light_SpecularColor_B ATTRIBUTE_LIGHTSPECULAR_B
#define Light_SpecularColor_A ATTRIBUTE_LIGHTSPECULAR_A
#define Light_AmbientColor ATTRIBUTE_LIGHTAMBIENT
#define Light_AmbientColor_R ATTRIBUTE_LIGHTAMBIENT_R
#define Light_AmbientColor_G ATTRIBUTE_LIGHTAMBIENT_G
#define Light_AmbientColor_B ATTRIBUTE_LIGHTAMBIENT_B
#define Light_AmbientColor_A ATTRIBUTE_LIGHTAMBIENT_A
#define Light_Brightness ATTRIBUTE_BRIGHTNESS
#define Light_LinearFade ATTRIBUTE_LINEARFADE
#define Light_ExponentialFade ATTRIBUTE_EXPFADE
#define Light_AreaWidth ATTRIBUTE_AREAWIDTH
#define Light_AreaHeight ATTRIBUTE_AREAHEIGHT
#define Light_CastShadow ATTRIBUTE_CASTSHADOW
#define Light_ShadowBias ATTRIBUTE_SHDWBIAS
#define Light_ShadowFactor ATTRIBUTE_SHDWFACTOR
#define Light_ShadowMapRes ATTRIBUTE_SHDWMAPRES
#define Light_ShadowMapFar ATTRIBUTE_SHDWMAPFAR
#define Light_ShadowMapFov ATTRIBUTE_SHDWMAPFOV
#define Light_ShadowFilter ATTRIBUTE_SHDWFILTER
#define Model_MeshPath ATTRIBUTE_SOURCEPATH
#define Model_ShadowCaster ATTRIBUTE_SHADOWCASTER
#define Model_TessellationMode ATTRIBUTE_TESSELLATION
#define Model_EdgeTess ATTRIBUTE_EDGETESS
#define Model_InnerTess ATTRIBUTE_INNERTESS
#define Lightmaps_LightmapIndirect ATTRIBUTE_LIGHTMAPINDIRECT
#define Lightmaps_LightmapRadiosity ATTRIBUTE_LIGHTMAPRADIOSITY
#define Lightmaps_LightmapShadow ATTRIBUTE_LIGHTMAPSHADOW
#define Material_Lighting ATTRIBUTE_SHADERLIGHTING
#define Material_BlendMode ATTRIBUTE_BLENDMODE
#define MaterialBase_IblProbe ATTRIBUTE_IBLPROBE
#define Material_DiffuseColor ATTRIBUTE_DIFFUSE
#define Material_DiffuseColor_R ATTRIBUTE_DIFFUSE_R
#define Material_DiffuseColor_G ATTRIBUTE_DIFFUSE_G
#define Material_DiffuseColor_B ATTRIBUTE_DIFFUSE_B
#define Material_DiffuseColor_A ATTRIBUTE_DIFFUSE_A
#define Material_DiffuseMaps_0 ATTRIBUTE_DIFFUSEMAP
#define Material_DiffuseMaps_1 ATTRIBUTE_DIFFUSEMAP2
#define Material_DiffuseMaps_2 ATTRIBUTE_DIFFUSEMAP3
#define Material_EmissivePower ATTRIBUTE_EMISSIVEPOWER
#define Material_EmissiveColor ATTRIBUTE_EMISSIVECOLOR
#define Material_EmissiveColor_R ATTRIBUTE_EMISSIVECOLOR_R
#define Material_EmissiveColor_G ATTRIBUTE_EMISSIVECOLOR_G
#define Material_EmissiveColor_B ATTRIBUTE_EMISSIVECOLOR_B
#define Material_EmissiveColor_A ATTRIBUTE_EMISSIVECOLOR_A
#define Material_EmissiveMap ATTRIBUTE_EMISSIVEMAP
#define Material_EmissiveMap2 ATTRIBUTE_EMISSIVEMAP2
#define Material_SpecularReflection ATTRIBUTE_SPECULARREFLECTION
#define Material_SpecularMap ATTRIBUTE_SPECULARMAP
#define Material_SpecularModel ATTRIBUTE_SPECULARMODEL
#define Material_SpecularTint ATTRIBUTE_SPECULARTINT
#define Material_SpecularTint_R ATTRIBUTE_SPECULARTINT_R
#define Material_SpecularTint_G ATTRIBUTE_SPECULARTINT_G
#define Material_SpecularTint_B ATTRIBUTE_SPECULARTINT_B
#define Material_SpecularTint_A ATTRIBUTE_SPECULARTINT_A
#define Material_IOR ATTRIBUTE_IOR
#define Material_FresnelPower ATTRIBUTE_FRESNELPOWER
#define Material_SpecularAmount ATTRIBUTE_SPECULARAMOUNT
#define Material_SpecularRoughness ATTRIBUTE_SPECULARROUGHNESS
#define Material_RoughnessMap ATTRIBUTE_ROUGHNESSMAP
#define Material_Opacity ATTRIBUTE_OPACITY
#define Material_OpacityMap ATTRIBUTE_OPACITYMAP
#define Material_BumpAmount ATTRIBUTE_BUMPAMOUNT
#define Material_BumpMap ATTRIBUTE_BUMPMAP
#define Material_NormalMap ATTRIBUTE_NORMALMAP
#define Material_DisplaceAmount ATTRIBUTE_DISPLACEAMOUNT
#define Material_DisplacementMap ATTRIBUTE_DISPLACEMENTMAP
#define Material_TranslucentFalloff ATTRIBUTE_TRANSLUCENTFALLOFF
#define Material_TranslucencyMap ATTRIBUTE_TRANSLUCENCYMAP
#define Material_DiffuseLightWrap ATTRIBUTE_DIFFUSELIGHTWRAP
#define Material_ReferencedMaterial ATTRIBUTE_REFERENCEDMATERIAL
#define Material_VertexColors ATTRIBUTE_VERTEXCOLORS
#define Material_TransparencyMode ATTRIBUTE_TRANSPARENCYMODE
#define Image_ImagePath ATTRIBUTE_SOURCEPATH
#define Image_OffscreenRendererId ATTRIBUTE_SUBPRESENTATION
#define Image_Scale_X ATTRIBUTE_SCALEU
#define Image_Scale_Y ATTRIBUTE_SCALEV
#define Image_Pivot_X ATTRIBUTE_PIVOTU
#define Image_Pivot_Y ATTRIBUTE_PIVOTV
#define Image_Rotation ATTRIBUTE_ROTATIONUV
#define Image_Position_X ATTRIBUTE_POSITIONU
#define Image_Position_Y ATTRIBUTE_POSITIONV
#define Image_MappingMode ATTRIBUTE_MAPPINGMODE
#define Image_HorizontalTilingMode ATTRIBUTE_TILINGMODEHORZ
#define Image_VerticalTilingMode ATTRIBUTE_TILINGMODEVERT
#define Text_Text ATTRIBUTE_TEXTSTRING
#define Text_Font ATTRIBUTE_FONT
#define Text_FontSize ATTRIBUTE_SIZE
#define Text_HorizontalAlignment ATTRIBUTE_HORZALIGN
#define Text_VerticalAlignment ATTRIBUTE_VERTALIGN
#define Text_Leading ATTRIBUTE_LEADING
#define Text_Tracking ATTRIBUTE_TRACKING
#define Text_DropShadow ATTRIBUTE_DROPSHADOW
#define Text_DropShadowStrength ATTRIBUTE_DROPSHADOWSTRENGTH
#define Text_DropShadowOffsetX ATTRIBUTE_DROPSHADOWOFFSETX
#define Text_DropShadowOffsetY ATTRIBUTE_DROPSHADOWOFFSETY
#define Text_WordWrap ATTRIBUTE_WORDWRAP
#define Text_BoundingBox ATTRIBUTE_BOUNDINGBOX
#define Text_BoundingBox_X ATTRIBUTE_BOUNDINGBOX_X
#define Text_BoundingBox_Y ATTRIBUTE_BOUNDINGBOX_Y
#define Text_Elide ATTRIBUTE_ELIDE
#define Text_TextColor ATTRIBUTE_TEXTCOLOR
#define Text_TextColor_R ATTRIBUTE_TEXTCOLOR_R
#define Text_TextColor_G ATTRIBUTE_TEXTCOLOR_G
#define Text_TextColor_B ATTRIBUTE_TEXTCOLOR_B
#define Text_TextColor_A ATTRIBUTE_TEXTCOLOR_A
#define Text_BackColor_R ATTRIBUTE_BACKCOLOR_R
#define Text_BackColor_G ATTRIBUTE_BACKCOLOR_G
#define Text_BackColor_B ATTRIBUTE_BACKCOLOR_B
#define Text_BackColor_A ATTRIBUTE_BACKCOLOR_A
#define Text_UseBackColor ATTRIBUTE_USEBACKCOLOR
#define Text_EnableAcceleratedFont ATTRIBUTE_ENABLEACCELERATEDFONT
#define Path_PathType ATTRIBUTE_PATHTYPE
#define Path_Width ATTRIBUTE_WIDTH
#define Path_LinearError ATTRIBUTE_LINEARERROR
#define Path_EdgeTessAmount ATTRIBUTE_EDGETESSAMOUNT
#define Path_InnerTessAmount ATTRIBUTE_INNERTESSAMOUNT
#define Path_BeginCapping ATTRIBUTE_BEGINCAP
#define Path_BeginCapOffset ATTRIBUTE_BEGINCAPOFFSET
#define Path_BeginCapOpacity ATTRIBUTE_BEGINCAPOPACITY
#define Path_BeginCapWidth ATTRIBUTE_BEGINCAPWIDTH
#define Path_EndCapping ATTRIBUTE_ENDCAP
#define Path_EndCapOffset ATTRIBUTE_ENDCAPOFFSET
#define Path_EndCapOpacity ATTRIBUTE_ENDCAPOPACITY
#define Path_EndCapWidth ATTRIBUTE_ENDCAPWIDTH
#define Path_PaintStyle ATTRIBUTE_PAINTSTYLE
#define Path_PathBuffer ATTRIBUTE_SOURCEPATH
#define SubPath_Closed ATTRIBUTE_CLOSED

// Fill in implementations for the actual parse tables.
#define HANDLE_QT3DS_RENDER_PROPERTY(type, name, dirty)                                            \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);                    \
        break;

#define HANDLE_QT3DS_RENDER_VEC3_PROPERTY(type, name, dirty)                                       \
    case Q3DStudio::type##_##name##_X:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.x);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_Y:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.y);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_Z:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.z);                  \
        break;

#define HANDLE_QT3DS_RENDER_REAL_VEC2_PROPERTY(type, name, dirty)                                  \
    case Q3DStudio::type##_##name##_X:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.x);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_Y:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.y);                  \
        break;

#define HANDLE_QT3DS_RENDER_COLOR_PROPERTY(type, name, dirty)                                      \
    case Q3DStudio::type##_##name##_R:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.x);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_G:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.y);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_B:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.z);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_A:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.w);                  \
        break;

#define HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(type, name, dirty)                                 \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);                    \
        break;

#define HANDLE_QT3DS_RENDER_TRANSFORM_VEC3_PROPERTY(type, name, dirty)                             \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);                    \
        break;

#define HANDLE_QT3DS_RENDER_RADIAN_PROPERTY(type, name, dirty)                                     \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseRadianProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);              \
        break;

// The runtime converts rotations for us.
#define HANDLE_QT3DS_RENDER_VEC3_RADIAN_PROPERTY(type, name, dirty)                                \
    case Q3DStudio::type##_##name##_X:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.x);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_Y:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.y);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_Z:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.z);                  \
        break;

#define HANDLE_QT3DS_RENDER_OPACITY_PROPERTY(type, name, dirty)                                    \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseOpacityProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);             \
        break;

#define HANDLE_QT3DS_ROTATION_ORDER_PROPERTY(type, name, dirty)                                    \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseRotationOrder<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);               \
        break;

#define HANDLE_QT3DS_NODE_ORIENTATION_PROPERTY(type, name, dirty)                                  \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseOrientation<Qt3DSRenderDirtyFlags::dirty>(theItem.m_Flags);                  \
        break;

#define HANDLE_QT3DS_RENDER_DEPTH_TEST_PROPERTY(type, name, dirty)                                 \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseInverseBoolean<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);              \
        break;

#define HANDLE_QT3DS_NODE_FLAGS_PROPERTY(type, name, dirty)                                        \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseNodeFlagsProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_Flags,             \
                                                                    NodeFlagValues::name);         \
        break;

#define HANDLE_QT3DS_NODE_FLAGS_INVERSE_PROPERTY(type, name, dirty)                                \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseNodeFlagsInverseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_Flags,      \
                                                                           NodeFlagValues::name);  \
        break;

#define HANDLE_QT3DS_RENDER_ENUM_PROPERTY(type, name, dirty)                                       \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseEnumProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);                \
        break;

#define HANDLE_QT3DS_RENDER_SOURCEPATH_PROPERTY(type, name, dirty)                                 \
    case Q3DStudio::type##_##name:                                                                 \
        inParser.ParseAndResolveSourcePath<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name);        \
        break;

#define HANDLE_QT3DS_RENDER_ARRAY_PROPERTY(type, name, index, dirty)                               \
    case Q3DStudio::type##_##name##_##index:                                                       \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name[index]);             \
        break;

#define HANDLE_QT3DS_RENDER_VEC2_PROPERTY(type, name, dirty)                                       \
    case Q3DStudio::type##_##name##_X:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.x);                  \
        break;                                                                                     \
    case Q3DStudio::type##_##name##_Y:                                                             \
        inParser.ParseProperty<Qt3DSRenderDirtyFlags::dirty>(theItem.m_##name.y);                  \
        break;

struct SSceneTranslator : public Qt3DSTranslator
{
    typedef SScene TNodeType;
    SSceneTranslator(Q3DStudio::TElement &inElement, SScene &inRenderObject)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
    }
    // Ignored, scenes are always active
    void SetActive(bool /*inElementActive*/) {}
    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SScene &theItem = *static_cast<SScene *>(m_RenderObject);

        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_SCENE_PROPERTIES
        // These are ignored by the renderer
        case Q3DStudio::ATTRIBUTE_NAME:
        case Q3DStudio::ATTRIBUTE_STARTTIME:
        case Q3DStudio::ATTRIBUTE_ENDTIME:
        case Q3DStudio::ATTRIBUTE_IMPORTID:
        case Q3DStudio::ATTRIBUTE_EYEBALL:
            break;
        default:
            // Unknown attribute
            // QT3DS_ASSERT( false );
            break;
        }
    }
    void PostPropertyChanged(const SRuntimePropertyParser &, Q3DStudio::IPresentation &) {}
};

struct SNodeTranslator : public Qt3DSTranslator
{
    typedef SNode TNodeType;
    SNodeTranslator(Q3DStudio::TElement &inElement, SNode &inRenderObject)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
        SNode &theItem = *static_cast<SNode *>(m_RenderObject);
        theItem.m_Flags.SetLocallyPickable(false);
    }
    void SetActive(bool inElementActive)
    {
        SNode &theItem = *static_cast<SNode *>(m_RenderObject);
        if (theItem.m_Flags.IsActive() != inElementActive) {
            theItem.m_Flags.SetActive(inElementActive);
            theItem.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
        }
    }

    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SNode &theItem = *static_cast<SNode *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_NODE_PROPERTIES
        case Q3DStudio::ATTRIBUTE_NAME:
        case Q3DStudio::ATTRIBUTE_STARTTIME:
        case Q3DStudio::ATTRIBUTE_ENDTIME:
        case Q3DStudio::ATTRIBUTE_IMPORTID:
        case Q3DStudio::ATTRIBUTE_EYEBALL:
        // Groups have a source path property on them that we like to ignore.
        case Q3DStudio::ATTRIBUTE_SOURCEPATH:
            break;
        default:
            // Unknown attribute
            // QT3DS_ASSERT( false );
            break;
        }
    }
    void PostPropertyChanged(const SRuntimePropertyParser &inParser,
                             Q3DStudio::IPresentation & /*inStudioPresentation*/)
    {
        SNode &theItem = *static_cast<SNode *>(m_RenderObject);
        if (inParser.m_TransformDirty)
            theItem.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
        else if (inParser.m_Dirty)
            theItem.MarkDirty(NodeTransformDirtyFlag::TransformNotDirty);
        if (inParser.m_TextDirty)
            theItem.m_Flags.SetTextDirty(true);
        SetActive(Element().GetActive());
        bool isNodePickable = m_Element->IsPickEnabled();
        if (theItem.m_Flags.IsLocallyPickable() != isNodePickable) {
            theItem.m_Flags.SetLocallyPickable(isNodePickable);
            theItem.MarkDirty(NodeTransformDirtyFlag::TransformNotDirty);
        }
    }
};

struct SLayerTranslator : public SNodeTranslator
{
    typedef SLayer TNodeType;
    SLayerTranslator(Q3DStudio::TElement &inElement, SLayer &inRenderObject)
        : SNodeTranslator(inElement, inRenderObject)
    {
    }
    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        const char *propName =
            Q3DStudio::GetAttributeString((Q3DStudio::EAttribute)inParser.m_PropertyName);
        (void)propName;
        SLayer &theItem = *static_cast<SLayer *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_LAYER_PROPERTIES
        // Ignored
        default:
            SNodeTranslator::OnSpecificPropertyChange(inParser);
        }
    }
};

struct SLightTranslator : public SNodeTranslator
{
    typedef SLight TNodeType;
    SLightTranslator(Q3DStudio::TElement &inElement, SLight &inRenderObject)
        : SNodeTranslator(inElement, inRenderObject)
    {
    }

    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SLight &theItem = *static_cast<SLight *>(m_RenderObject);
        // I guess there is no switching of light type in the runtime right now.
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_LIGHT_PROPERTIES
        default:
            SNodeTranslator::OnSpecificPropertyChange(inParser);
            break;
        }
    }
};
struct SCameraTranslator : public SNodeTranslator
{
    typedef SCamera TNodeType;
    SCameraTranslator(Q3DStudio::TElement &inElement, SCamera &inRenderObject)
        : SNodeTranslator(inElement, inRenderObject)
    {
    }
    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SCamera &theItem = *static_cast<SCamera *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_CAMERA_PROPERTIES
        default:
            SNodeTranslator::OnSpecificPropertyChange(inParser);
            break;
        }
    }
};

struct SModelTranslator : public SNodeTranslator
{
    typedef SModel TNodeType;
    SModelTranslator(Q3DStudio::TElement &inElement, SModel &inRenderObject)
        : SNodeTranslator(inElement, inRenderObject)
    {
    }
    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SModel &theItem = *static_cast<SModel *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_MODEL_PROPERTIES
        default:
            SNodeTranslator::OnSpecificPropertyChange(inParser);
            break;
        }
    }
};

struct SPathTranslator : public SNodeTranslator
{
    typedef SPath TNodeType;
    SPathTranslator(Q3DStudio::TElement &inElement, SPath &inRenderObject)
        : SNodeTranslator(inElement, inRenderObject)
    {
    }

    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SPath &theItem = *static_cast<SPath *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_PATH_PROPERTIES
        default:
            SNodeTranslator::OnSpecificPropertyChange(inParser);
            break;
        }
    }
};

struct SPathSubPathTranslator : public Qt3DSTranslator
{

    typedef SPathSubPath TNodeType;

    SPathSubPathTranslator(Q3DStudio::TElement &inElement, SPathSubPath &inRenderObject)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
    }

    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SPathSubPath &theItem = *static_cast<SPathSubPath *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_PATH_SUBPATH_PROPERTIES
        default:
            break;
        }
    }

    void PostPropertyChanged(const SRuntimePropertyParser &inParser,
                             Q3DStudio::IPresentation & /*inStudioPresentation*/)
    {
        SPathSubPath &theItem = *static_cast<SPathSubPath *>(m_RenderObject);
        qt3ds::render::IPathManager &theManager = inParser.m_RenderContext.GetPathManager();
        QT3DSU32 numAnchors = 0;
        bool updatePath = false;
        CRegisteredString theAnchorType =
            inParser.m_RenderContext.GetStringTable().RegisterStr("PathAnchorPoint");
        for (Q3DStudio::TElement *theChild = Element().GetChild(); theChild;
             theChild = theChild->GetSibling()) {
            if (theChild->GetType() == theAnchorType) {
                ++numAnchors;
                if (theChild->IsDirty())
                    updatePath = true;
            }
        }
        if (updatePath) {
            NVDataRef<qt3ds::render::SPathAnchorPoint> thePathBuffer =
                theManager.ResizePathSubPathBuffer(theItem, numAnchors);
            if (thePathBuffer.size()) {
                QT3DSU32 anchorIndex = 0;
                for (Q3DStudio::TElement *theChild = Element().GetChild(); theChild;
                     theChild = theChild->GetSibling()) {
                    if (theChild->GetType() == theAnchorType) {
                        if (theChild->IsDirty()) {
                            qt3ds::render::SPathAnchorPoint &thePoint(thePathBuffer[anchorIndex]);

                            for (QT3DSI32 idx = 0, end = theChild->GetAttributeCount(); idx < end;
                                 ++idx) {
                                qt3ds::runtime::element::TPropertyDescAndValuePtr thePropInfo =
                                    theChild->GetPropertyByIndex(idx);
                                switch (thePropInfo.first.GetNameHash()) {
                                case Q3DStudio::ATTRIBUTE_POSITION_X:
                                    thePoint.m_Position.x = thePropInfo.second->m_FLOAT;
                                    break;
                                case Q3DStudio::ATTRIBUTE_POSITION_Y:
                                    thePoint.m_Position.y = thePropInfo.second->m_FLOAT;
                                    break;
                                case Q3DStudio::ATTRIBUTE_INCOMINGANGLE:
                                    thePoint.m_IncomingAngle = thePropInfo.second->m_FLOAT;
                                    thePoint.m_OutgoingAngle = thePoint.m_IncomingAngle + 180.0f;
                                    break;
                                case Q3DStudio::ATTRIBUTE_INCOMINGDISTANCE:
                                    thePoint.m_IncomingDistance = thePropInfo.second->m_FLOAT;
                                    break;
                                case Q3DStudio::ATTRIBUTE_OUTGOINGDISTANCE:
                                    thePoint.m_OutgoingDistance = thePropInfo.second->m_FLOAT;
                                    break;
                                default: // ignored
                                    break;
                                }
                            }
                        }
                        ++anchorIndex;
                    }
                }
            }
        }
    }
};

struct SDefaultMaterialTranslator : public Qt3DSTranslator
{
    typedef SDefaultMaterial TNodeType;
    SDefaultMaterialTranslator(Q3DStudio::TElement &inElement, SDefaultMaterial &inRenderObject)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
    }
    // Right now materials do not respect the active flag
    void SetActive(bool) {}
    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SDefaultMaterial &theItem = *static_cast<SDefaultMaterial *>(m_RenderObject);
        // There is no render-time caching on a material, so the dirty flag doesn't do much.
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_MATERIAL_PROPERTIES

        case Q3DStudio::ATTRIBUTE_NAME:
        case Q3DStudio::ATTRIBUTE_STARTTIME:
        case Q3DStudio::ATTRIBUTE_ENDTIME:
        case Q3DStudio::ATTRIBUTE_IMPORTID:
        case Q3DStudio::ATTRIBUTE_EYEBALL:
        case Q3DStudio::ATTRIBUTE_SOURCEPATH:
            break;
        default:
            // Unknown attribute
            // QT3DS_ASSERT( false );
            break;
        }
    }

    void PostPropertyChanged(const SRuntimePropertyParser &, Q3DStudio::IPresentation &)
    {
        SDefaultMaterial &theItem = *static_cast<SDefaultMaterial *>(m_RenderObject);
        theItem.m_Dirty.SetDirty();
    }
};

struct SImageTranslator : public Qt3DSTranslator
{
    typedef SImage TNodeType;
    SImageTranslator(Q3DStudio::TElement &inElement, SImage &inRenderObject)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
    }

    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SImage &theItem = *static_cast<SImage *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_IMAGE_PROPERTIES
        case Q3DStudio::ATTRIBUTE_STARTTIME:
        case Q3DStudio::ATTRIBUTE_ENDTIME:
        case Q3DStudio::ATTRIBUTE_NAME:
        case Q3DStudio::ATTRIBUTE_EYEBALL:
            break;
        default:
            // Unknown attribute
            // QT3DS_ASSERT( false );
            break;
        }
    }
    void PostPropertyChanged(const SRuntimePropertyParser &inParser, Q3DStudio::IPresentation &)
    {
        SImage &theItem = *static_cast<SImage *>(m_RenderObject);
        if (inParser.m_Dirty)
            theItem.m_Flags.SetDirty(true);
        if (inParser.m_TransformDirty)
            theItem.m_Flags.SetTransformDirty(true);
    }
};

struct SReferencedMaterialTranslator : public Qt3DSTranslator
{
    typedef SReferencedMaterial TNodeType;
    SReferencedMaterialTranslator(Q3DStudio::TElement &inElement, TNodeType &inRenderObject)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
    }

    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        TNodeType &theItem = *static_cast<TNodeType *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_REFERENCED_MATERIAL_PROPERTIES
        case Q3DStudio::ATTRIBUTE_STARTTIME:
        case Q3DStudio::ATTRIBUTE_ENDTIME:
        case Q3DStudio::ATTRIBUTE_NAME:
        case Q3DStudio::ATTRIBUTE_EYEBALL:
            break;
        default:
            // Unknown attribute
            // QT3DS_ASSERT( false );
            break;
        }
    }
    void PostPropertyChanged(const SRuntimePropertyParser &inParser, Q3DStudio::IPresentation &)
    {
        TNodeType &theItem = *static_cast<TNodeType *>(m_RenderObject);
        if (inParser.m_Dirty)
            theItem.m_Dirty.SetDirty();
    }
};

struct STextTranslator : public SNodeTranslator
{
    typedef SText TNodeType;
    STextTranslator(Q3DStudio::TElement &inElement, SText &inRenderObject)
        : SNodeTranslator(inElement, inRenderObject)
    {
    }
    void OnSpecificPropertyChange(SRuntimePropertyParser &inParser)
    {
        SText &theItem = *static_cast<SText *>(m_RenderObject);
        switch (inParser.m_PropertyName) {
            ITERATE_QT3DS_RENDER_TEXT_PROPERTIES
        case Q3DStudio::ATTRIBUTE_TEXTTYPE:
        case Q3DStudio::ATTRIBUTE_RENDERSTYLE:
        case Q3DStudio::ATTRIBUTE_HORZSCROLL:
        case Q3DStudio::ATTRIBUTE_VERTSCROLL:
        case Q3DStudio::ATTRIBUTE_BOXHEIGHT:
        case Q3DStudio::ATTRIBUTE_BOXWIDTH:
        case Q3DStudio::ATTRIBUTE_REMOTESTRINGSOURCE:
        case Q3DStudio::ATTRIBUTE_CACHEDTEXTSTRING:
            // These text properties are ignored for now.
            break;
        default:
            SNodeTranslator::OnSpecificPropertyChange(inParser);
            break;
        }
    }
};

struct SEffectPropertyEntry
{
    Q3DStudio::EAttributeType m_AttributeType;
    QT3DSU32 m_PropertyOffset; // offset into the property array for the property def
    QT3DSU32 m_DataOffset;
    SEffectPropertyEntry(Q3DStudio::EAttributeType attType, QT3DSU32 poff, QT3DSU32 doff = 0)
        : m_AttributeType(attType)
        , m_PropertyOffset(poff)
        , m_DataOffset(doff)
    {
    }
};

struct SDynamicObjectTranslatorContext : public STranslatorContext
{
    typedef nvhash_map<Q3DStudio::INT32, SEffectPropertyEntry> THashToOffsetMap;
    THashToOffsetMap m_PropertyHashes;
    NVAllocatorCallback &m_Allocator;
    Qt3DSString m_Workspace;
    SDynamicObjectTranslatorContext(NVAllocatorCallback &inCallback)
        : m_PropertyHashes(inCallback, "SEffectTranslatorContext::PropertyHashes")
        , m_Allocator(inCallback)
    {
    }
    ~SDynamicObjectTranslatorContext() {}
    void AddEffectExtendedProperty(const QByteArray &baseName,
                                   const QByteArray &extension, Q3DStudio::EAttributeType inType,
                                   QByteArray &ioStringBuilder, QT3DSU32 inOffset,
                                   QT3DSU32 dataOffset)
    {
        ioStringBuilder = baseName;
        ioStringBuilder.append(extension);
        Q3DStudio::INT32 theHash = Q3DStudio::CHash::HashAttribute(ioStringBuilder.constData());
        m_PropertyHashes.insert(
            eastl::make_pair(theHash, SEffectPropertyEntry(inType, inOffset, dataOffset)));
    }
    void BuildPropertyHashes(NVConstDataRef<qt3ds::render::dynamic::SPropertyDefinition> inProperties)
    {
        if (m_PropertyHashes.size() == 0) {
            QByteArray nameBuilder;
            QByteArray baseName;
            for (QT3DSU32 idx = 0, end = inProperties.size(); idx < end; ++idx) {
                const qt3ds::render::dynamic::SPropertyDefinition &thePropDef = inProperties[idx];
                switch (thePropDef.m_DataType) {
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSF32:
                    m_PropertyHashes.insert(eastl::make_pair(
                        Q3DStudio::CHash::HashAttribute(thePropDef.m_Name.c_str()),
                        SEffectPropertyEntry(Q3DStudio::ATTRIBUTETYPE_FLOAT, idx)));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSRenderBool:
                    m_PropertyHashes.insert(
                        eastl::make_pair(Q3DStudio::CHash::HashAttribute(thePropDef.m_Name.c_str()),
                                         SEffectPropertyEntry(Q3DStudio::ATTRIBUTETYPE_BOOL, idx)));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSI32:
                    if (thePropDef.m_IsEnumProperty == false) {
                        m_PropertyHashes.insert(eastl::make_pair(
                            Q3DStudio::CHash::HashAttribute(thePropDef.m_Name.c_str()),
                            SEffectPropertyEntry(Q3DStudio::ATTRIBUTETYPE_INT32, idx)));
                    } else {
                        m_PropertyHashes.insert(eastl::make_pair(
                            Q3DStudio::CHash::HashAttribute(thePropDef.m_Name.c_str()),
                            SEffectPropertyEntry(Q3DStudio::ATTRIBUTETYPE_STRING, idx)));
                    }
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec2:
                    baseName = QString::fromUtf8(thePropDef.m_Name.c_str()).toUtf8();
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".x"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT, nameBuilder, idx, 0);
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".y"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT, nameBuilder, idx,
                                              sizeof(QT3DSF32));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec3:
                    baseName = QString::fromUtf8(thePropDef.m_Name.c_str()).toUtf8();
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".x"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 0);
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".y"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, sizeof(QT3DSF32));
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".z"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 2 * sizeof(QT3DSF32));

                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".r"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 0);
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".g"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, sizeof(QT3DSF32));
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".b"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 2 * sizeof(QT3DSF32));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4:
                    baseName = QString::fromUtf8(thePropDef.m_Name.c_str()).toUtf8();
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".x"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 0);
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".y"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, sizeof(QT3DSF32));
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".z"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 2 * sizeof(QT3DSF32));
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".w"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 3 * sizeof(QT3DSF32));

                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".r"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 0);
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".g"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, sizeof(QT3DSF32));
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".b"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 2 * sizeof(QT3DSF32));
                    AddEffectExtendedProperty(baseName, QByteArrayLiteral(".a"),
                                              Q3DStudio::ATTRIBUTETYPE_FLOAT,
                                              nameBuilder, idx, 3 * sizeof(QT3DSF32));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::NVRenderTexture2DPtr:
                case qt3ds::render::NVRenderShaderDataTypes::NVRenderImage2DPtr:
                    m_PropertyHashes.insert(eastl::make_pair(
                        Q3DStudio::CHash::HashAttribute(thePropDef.m_Name.c_str()),
                        SEffectPropertyEntry(Q3DStudio::ATTRIBUTETYPE_STRING, idx)));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::NVRenderDataBufferPtr:
                    break;
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }
        }
    }
    void ApplyChanges(SPresentation &inPresentation, IQt3DSRenderContext &inRenderContext,
                      SDynamicObject &inObject, Q3DStudio::TElement &element,
                      IDynamicObjectSystem &inSystem)
    {
        if (element.GetActive()) {
            NVConstDataRef<qt3ds::render::dynamic::SPropertyDefinition> theProperties =
                inSystem.GetProperties(inObject.m_ClassName);
            BuildPropertyHashes(theProperties);
            SDynamicObject &theItem(inObject);
            for (long idx = 0, end = element.GetAttributeCount(); idx < end; ++idx) {
                qt3ds::runtime::element::TPropertyDescAndValuePtr thePropInfo =
                    *element.GetPropertyByIndex(idx);
                THashToOffsetMap::iterator theFind =
                    m_PropertyHashes.find(thePropInfo.first.GetNameHash());
                if (theFind != m_PropertyHashes.end()) {
                    const SEffectPropertyEntry &theEntry(theFind->second);
                    const qt3ds::render::dynamic::SPropertyDefinition &theDefinition(
                        theProperties[theEntry.m_PropertyOffset]);
                    if (theEntry.m_AttributeType
                        == (Q3DStudio::EAttributeType)thePropInfo.first.m_Type) {
                        switch (theEntry.m_AttributeType) {
                        case Q3DStudio::ATTRIBUTETYPE_BOOL:
                            theItem.SetPropertyValue(theDefinition,
                                                     thePropInfo.second->m_INT32 ? true : false);
                            break;
                        case Q3DStudio::ATTRIBUTETYPE_FLOAT:
                            theItem.SetPropertyValue(theDefinition, thePropInfo.second->m_FLOAT,
                                                     theEntry.m_DataOffset);
                            break;
                        case Q3DStudio::ATTRIBUTETYPE_INT32:
                            theItem.SetPropertyValue(theDefinition,
                                                     (QT3DSI32)thePropInfo.second->m_INT32);
                            break;
                        case Q3DStudio::ATTRIBUTETYPE_STRING: {
                            CRegisteredString theStr =
                                element.GetBelongedPresentation()->GetStringTable().HandleToStr(
                                    thePropInfo.second->m_StringHandle);
                            theItem.SetPropertyValue(theDefinition, theStr.c_str(),
                                                     inPresentation.m_PresentationDirectory.c_str(),
                                                     m_Workspace, inRenderContext.GetStringTable());
                        } break;
                        default:
                            // QT3DS_ASSERT( false );
                            break;
                        }
                    } else {
                        // QT3DS_ASSERT( false );
                    }
                }
            }
            theItem.m_Flags.SetDirty(true);
        }
    }
};

struct SEffectTranslator : public Qt3DSTranslator
{
    typedef SEffect TNodeType;
    SEffectTranslator(Q3DStudio::TElement &inElement, SEffect &inRenderObject,
                      NVAllocatorCallback &inCallback)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
        m_TranslatorContext = QT3DS_NEW(inCallback, SDynamicObjectTranslatorContext)(inCallback);
    }

    void OnElementChanged(SPresentation &inPresentation, IQt3DSRenderContext &inRenderContext,
                          Q3DStudio::IPresentation &)
    {
        SRuntimePropertyParser theParser(inPresentation, inRenderContext, *m_Element);
        SEffect &theItem = *static_cast<SEffect *>(m_RenderObject);
        static_cast<SDynamicObjectTranslatorContext *>(m_TranslatorContext)
            ->ApplyChanges(inPresentation, inRenderContext, theItem, Element(),
                           inRenderContext.GetDynamicObjectSystem());
        theItem.SetActive(Element().GetActive(), inRenderContext.GetEffectSystem());
    }
};

struct SCustomMaterialTranslator : public Qt3DSTranslator
{
    typedef SCustomMaterial TNodeType;
    SCustomMaterialTranslator(Q3DStudio::TElement &inElement, SCustomMaterial &inRenderObject,
                              NVAllocatorCallback &inCallback)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
        m_TranslatorContext = QT3DS_NEW(inCallback, SDynamicObjectTranslatorContext)(inCallback);
    }

    void OnElementChanged(SPresentation &inPresentation, IQt3DSRenderContext &inRenderContext,
                          Q3DStudio::IPresentation &)
    {
        SRuntimePropertyParser theParser(inPresentation, inRenderContext, *m_Element);
        SCustomMaterial &theItem = *static_cast<SCustomMaterial *>(m_RenderObject);
        static_cast<SDynamicObjectTranslatorContext *>(m_TranslatorContext)
            ->ApplyChanges(inPresentation, inRenderContext, theItem, Element(),
                           inRenderContext.GetDynamicObjectSystem());
        bool active = m_Element->GetActive();
        if (active != theItem.m_Flags.IsActive()) {
            theItem.m_Flags.SetActive(active);
            ICustomMaterialSystem &theSystem(inRenderContext.GetCustomMaterialSystem());
            theSystem.OnMaterialActivationChange(theItem, active);
        }
    }
};

struct SRenderPluginTranslatorContext : public STranslatorContext
{
    NVAllocatorCallback &m_Allocator;
    nvhash_map<int, CRegisteredString> m_AttribHashIndexMap;
    nvvector<SRenderPropertyValueUpdate> m_PropertyUpdates;
    SRenderPluginTranslatorContext(NVAllocatorCallback &alloc)
        : m_Allocator(alloc)
        , m_AttribHashIndexMap(alloc, "SRenderPluginTranslatorContext::AttribIndexMap")
        , m_PropertyUpdates(alloc, "SRenderPluginTranslatorContext::m_PropertyUpdates")
    {
    }
    void ReverseMap(CRegisteredString str)
    {
        m_AttribHashIndexMap.insert(
            eastl::make_pair(Q3DStudio::CHash::HashAttribute(str.c_str()), str));
    }
};

struct SRenderPluginTranslator : public Qt3DSTranslator
{
    typedef SRenderPlugin TNodeType;
    SRenderPluginTranslator(Q3DStudio::TElement &inElement, SRenderPlugin &inRenderObject,
                            NVAllocatorCallback &inCallback)
        : Qt3DSTranslator(inElement, inRenderObject)
    {
        m_TranslatorContext = QT3DS_NEW(inCallback, SRenderPluginTranslatorContext)(inCallback);
    }

    void OnElementChanged(SPresentation & /*inPresentation*/, IQt3DSRenderContext &inRenderContext,
                          Q3DStudio::IPresentation &)
    {
        SRenderPlugin &theItem = *static_cast<SRenderPlugin *>(m_RenderObject);
        theItem.m_Flags.SetActive(Element().GetActive());
        IRenderPluginInstance *theInstance =
            inRenderContext.GetRenderPluginManager().GetOrCreateRenderPluginInstance(
                theItem.m_PluginPath, &theItem);
        if (theInstance != NULL) {
            IRenderPluginClass &theClass(theInstance->GetPluginClass());
            SRenderPluginTranslatorContext &theTransContext =
                static_cast<SRenderPluginTranslatorContext &>(*m_TranslatorContext);
            if (theTransContext.m_AttribHashIndexMap.empty()) {
                NVConstDataRef<SRenderPluginPropertyDeclaration> theProperties(
                    theClass.GetRegisteredProperties());
                for (QT3DSU32 idx = 0, end = theProperties.size(); idx < end; ++idx) {
                    const SRenderPluginPropertyDeclaration &theDec(theProperties[idx]);
                    switch (theDec.m_Type) {
                    case SRenderPluginPropertyTypes::Boolean:
                        theTransContext.ReverseMap(theDec.m_Name);
                        break;
                    case SRenderPluginPropertyTypes::Float:
                        theTransContext.ReverseMap(theDec.m_Name);
                        break;
                    case SRenderPluginPropertyTypes::Long:
                        theTransContext.ReverseMap(theDec.m_Name);
                        break;
                    case SRenderPluginPropertyTypes::String:
                        theTransContext.ReverseMap(theDec.m_Name);
                        break;
                    case SRenderPluginPropertyTypes::Vector2:
                        theTransContext.ReverseMap(
                            theClass.GetPropertyValueInfo(theDec.m_StartOffset).first);
                        theTransContext.ReverseMap(
                            theClass.GetPropertyValueInfo(theDec.m_StartOffset + 1).first);
                        break;
                    case SRenderPluginPropertyTypes::Vector3:
                    case SRenderPluginPropertyTypes::Color:
                        theTransContext.ReverseMap(
                            theClass.GetPropertyValueInfo(theDec.m_StartOffset).first);
                        theTransContext.ReverseMap(
                            theClass.GetPropertyValueInfo(theDec.m_StartOffset + 1).first);
                        theTransContext.ReverseMap(
                            theClass.GetPropertyValueInfo(theDec.m_StartOffset + 2).first);
                        break;
                    default:
                        // QT3DS_ASSERT( false );
                        break;
                    }
                }
            } // ok, now we have an efficient mapping from attribute to plugin value name.
            theTransContext.m_PropertyUpdates.clear();
            for (long idx = 0, end = Element().GetAttributeCount(); idx < end; ++idx) {
                qt3ds::runtime::element::TPropertyDescAndValuePtr thePropInfo =
                    *Element().GetPropertyByIndex(idx);
                nvhash_map<int, CRegisteredString>::iterator theFind =
                    theTransContext.m_AttribHashIndexMap.find(thePropInfo.first.GetNameHash());
                if (theFind != theTransContext.m_AttribHashIndexMap.end()) {
                    CRegisteredString thePropName(theFind->second);
                    Q3DStudio::EAttributeType theType =
                        (Q3DStudio::EAttributeType)thePropInfo.first.m_Type;
                    switch (theType) {
                    case Q3DStudio::ATTRIBUTETYPE_BOOL:
                        theTransContext.m_PropertyUpdates.push_back(SRenderPropertyValueUpdate(
                            thePropName, thePropInfo.second->m_INT32 ? true : false));
                        break;
                    case Q3DStudio::ATTRIBUTETYPE_FLOAT:
                        theTransContext.m_PropertyUpdates.push_back(
                            SRenderPropertyValueUpdate(thePropName, thePropInfo.second->m_FLOAT));
                        break;
                    case Q3DStudio::ATTRIBUTETYPE_INT32:
                        theTransContext.m_PropertyUpdates.push_back(SRenderPropertyValueUpdate(
                            thePropName, (QT3DSI32)thePropInfo.second->m_INT32));
                        break;
                    case Q3DStudio::ATTRIBUTETYPE_STRING: {
                        CRegisteredString theStr = inRenderContext.GetStringTable().HandleToStr(
                            thePropInfo.second->m_StringHandle);
                        theTransContext.m_PropertyUpdates.push_back(
                            SRenderPropertyValueUpdate(thePropName, theStr));
                    } break;
                    default:
                        // QT3DS_ASSERT( false );
                        break;
                    }
                }
            }
            if (theTransContext.m_PropertyUpdates.empty() == false) {
                theInstance->Update(toConstDataRef(theTransContext.m_PropertyUpdates.data(),
                                                   theTransContext.m_PropertyUpdates.size()));
            }
        }
    }
};
}

Qt3DSTranslator::Qt3DSTranslator(Q3DStudio::TElement &inElement, SGraphObject &inRenderObject)
    : m_DirtyIndex(QT3DS_MAX_U32)
    , m_Element(&inElement)
    , m_RenderObject(&inRenderObject)
    , m_TranslatorContext(NULL)
{
    Element().SetAssociation(reinterpret_cast<void *>(this));
    inRenderObject.m_UserData = STaggedPointer(this);
}

void Qt3DSTranslator::Save(SWriteBuffer &inWriteBuffer, QT3DSU32 inGraphObjectOffset)
{
    // We have to start on pointer aligned boundaries.
    QT3DS_ASSERT(inWriteBuffer.size() % 4 == 0);
    QT3DSU32 theOffset = inWriteBuffer.size();
    inWriteBuffer.write(*this);
    Qt3DSTranslator *theNewTranslator =
        reinterpret_cast<Qt3DSTranslator *>(inWriteBuffer.begin() + theOffset);
    theNewTranslator->m_DirtyIndex = QT3DS_MAX_U32;
    if (theNewTranslator->m_Element)
        theNewTranslator->m_Element = m_Element->GetBelongedPresentation()
                                          ->GetApplication()
                                          .GetElementAllocator()
                                          .GetRemappedElementAddress(m_Element);
    size_t *graphObjPtr = reinterpret_cast<size_t *>(&theNewTranslator->m_RenderObject);
    *graphObjPtr = inGraphObjectOffset;
}

Qt3DSTranslator *Qt3DSTranslator::GetTranslatorFromGraphNode(SGraphObject &inObject)
{
    return inObject.m_UserData.DynamicCast<Qt3DSTranslator>();
}

namespace {
struct SPresentationTranslator;
struct SEffectTranslator;
template <typename TTranslatorType>
struct STranslatorCreator
{
    static TTranslatorType *Create(Q3DStudio::TElement &inElement, SGraphObject &inObject,
                                   NVAllocatorCallback &inAllocator)
    {
        typedef typename TTranslatorType::TNodeType TNodeType;
        // This assert needs to be here because we serialize all translators generically without
        // regard for type.
        StaticAssert<sizeof(TTranslatorType) == sizeof(Qt3DSTranslator)>::valid_expression();
        return QT3DS_NEW(inAllocator, TTranslatorType)(inElement, static_cast<TNodeType &>(inObject));
    }
    static void InitializeTranslator(Qt3DSTranslator &inTranslator, NVAllocatorCallback &)
    {
        typedef typename TTranslatorType::TNodeType TNodeType;
        // Initialize the vtable.
        new (&inTranslator) TTranslatorType(inTranslator.Element(),
                                            static_cast<TNodeType &>(inTranslator.RenderObject()));
    }

    static void OnElementChanged(Qt3DSTranslator &inTranslator, SPresentation &inPresentation,
                                 IQt3DSRenderContext &inRenderContext,
                                 Q3DStudio::IPresentation &inStudioPresentation)
    {
        TTranslatorType &theTranslator(static_cast<TTranslatorType &>(inTranslator));
        SRuntimePropertyParser theParser(inPresentation, inRenderContext, *theTranslator.m_Element);
        const QString type(theTranslator.Element().GetTypeDescription().m_TypeName.c_str());
        bool force = type.contains(QLatin1String("Material"))
                        || type.contains(QLatin1String("Image"));
        if (theTranslator.Element().GetActive() || force) {
            // Don't push properties from inactive elements.
            // Exceptions to this rule are Materials since materials that reference them need
            // the values even when the original material is inactive
            for (long idx = 0, end = theTranslator.Element().GetAttributeCount(); idx < end;
                 ++idx) {
                qt3ds::runtime::element::TPropertyDescAndValuePtr thePropInfo =
                    *theTranslator.Element().GetPropertyByIndex(idx);
                theParser.Setup(thePropInfo.first.GetNameHash(), *thePropInfo.second,
                                (Q3DStudio::EAttributeType)thePropInfo.first.m_Type);
                // right now, this is the best we can do because the attribute's dirty system
                // is all jacked up.
                theTranslator.OnSpecificPropertyChange(theParser);
            }
            // same for dynamic properties
            for (long idx = 0, end = theTranslator.Element().GetDynamicAttributeCount(); idx < end;
                 ++idx) {
                qt3ds::runtime::element::TPropertyDescAndValuePtr thePropInfo =
                    *theTranslator.Element().GetDynamicPropertyByIndex(idx);
                theParser.Setup(thePropInfo.first.GetNameHash(), *thePropInfo.second,
                                (Q3DStudio::EAttributeType)thePropInfo.first.m_Type);
                // right now, this is the best we can do because the attribute's dirty system
                // is all jacked up.
                theTranslator.OnSpecificPropertyChange(theParser);
            }
            // Set appropriate dirty flags
        }
        theTranslator.PostPropertyChanged(theParser, inStudioPresentation);
    }
};

template <>
struct STranslatorCreator<SPresentationTranslator>
{
    static Qt3DSTranslator *Create(Q3DStudio::TElement &, SGraphObject &, NVAllocatorCallback &)
    {
        return NULL;
    }
    static void InitializeTranslator(Qt3DSTranslator &, NVAllocatorCallback &) {}
    static void OnElementChanged(Qt3DSTranslator & /*inTranslator*/,
                                 SPresentation & /*inPresentation*/
                                 ,
                                 IQt3DSRenderContext & /*inRenderContext*/,
                                 Q3DStudio::IPresentation & /*inStudioPresentation*/)
    {
        QT3DS_ASSERT(false);
    }
};

template <>
struct STranslatorCreator<SEffectTranslator>
{
    static SEffectTranslator *Create(Q3DStudio::TElement &inElement, SGraphObject &inObject,
                                     NVAllocatorCallback &inAllocator)
    {
        typedef SEffectTranslator::TNodeType TNodeType;
        // This assert needs to be here because we serialize all translators generically without
        // regard for type.
        StaticAssert<sizeof(SEffectTranslator) == sizeof(Qt3DSTranslator)>::valid_expression();
        return QT3DS_NEW(inAllocator, SEffectTranslator)(inElement, static_cast<TNodeType &>(inObject),
                                                      inAllocator);
    }

    static void InitializeTranslator(Qt3DSTranslator &inTranslator, NVAllocatorCallback &inAllocator)
    {
        typedef SEffectTranslator::TNodeType TNodeType;
        // Initialize the vtable.
        new (&inTranslator)
            SEffectTranslator(inTranslator.Element(),
                              static_cast<TNodeType &>(inTranslator.RenderObject()), inAllocator);
    }
    static void OnElementChanged(Qt3DSTranslator &inTranslator, SPresentation &inPresentation,
                                 IQt3DSRenderContext &inRenderContext,
                                 Q3DStudio::IPresentation &inStudioPresentation)
    {
        static_cast<SEffectTranslator &>(inTranslator)
            .OnElementChanged(inPresentation, inRenderContext, inStudioPresentation);
    }
};

template <>
struct STranslatorCreator<SCustomMaterialTranslator>
{
    static SCustomMaterialTranslator *Create(Q3DStudio::TElement &inElement, SGraphObject &inObject,
                                             NVAllocatorCallback &inAllocator)
    {
        typedef SCustomMaterialTranslator::TNodeType TNodeType;
        // This assert needs to be here because we serialize all translators generically without
        // regard for type.
        StaticAssert<sizeof(SCustomMaterialTranslator)
                     == sizeof(Qt3DSTranslator)>::valid_expression();
        return QT3DS_NEW(inAllocator, SCustomMaterialTranslator)(
            inElement, static_cast<TNodeType &>(inObject), inAllocator);
    }

    static void InitializeTranslator(Qt3DSTranslator &inTranslator, NVAllocatorCallback &inAllocator)
    {
        typedef SCustomMaterialTranslator::TNodeType TNodeType;
        // Initialize the vtable.
        new (&inTranslator) SCustomMaterialTranslator(
            inTranslator.Element(), static_cast<TNodeType &>(inTranslator.RenderObject()),
            inAllocator);
    }

    static void OnElementChanged(Qt3DSTranslator &inTranslator, SPresentation &inPresentation,
                                 IQt3DSRenderContext &inRenderContext,
                                 Q3DStudio::IPresentation &inStudioPresentation)
    {
        static_cast<SCustomMaterialTranslator &>(inTranslator)
            .OnElementChanged(inPresentation, inRenderContext, inStudioPresentation);
    }
};

template <>
struct STranslatorCreator<SRenderPluginTranslator>
{
    static SRenderPluginTranslator *Create(Q3DStudio::TElement &inElement, SGraphObject &inObject,
                                           NVAllocatorCallback &inAllocator)
    {
        typedef SRenderPluginTranslator::TNodeType TNodeType;
        // This assert needs to be here because we serialize all translators generically without
        // regard for type.
        StaticAssert<sizeof(SRenderPluginTranslator) == sizeof(Qt3DSTranslator)>::valid_expression();
        return QT3DS_NEW(inAllocator, SRenderPluginTranslator)(
            inElement, static_cast<TNodeType &>(inObject), inAllocator);
    }

    static void InitializeTranslator(Qt3DSTranslator &inTranslator, NVAllocatorCallback &inAllocator)
    {
        typedef SRenderPluginTranslator::TNodeType TNodeType;
        // Initialize the vtable.
        new (&inTranslator) SRenderPluginTranslator(
            inTranslator.Element(), static_cast<TNodeType &>(inTranslator.RenderObject()),
            inAllocator);
    }
    static void OnElementChanged(Qt3DSTranslator &inTranslator, SPresentation &inPresentation,
                                 IQt3DSRenderContext &inRenderContext,
                                 Q3DStudio::IPresentation &inStudioPresentation)
    {
        static_cast<SRenderPluginTranslator &>(inTranslator)
            .OnElementChanged(inPresentation, inRenderContext, inStudioPresentation);
    }
};
}
Qt3DSTranslator *Qt3DSTranslator::CreateTranslatorForElement(Q3DStudio::TElement &inElement,
                                                           SGraphObject &inObject,
                                                           NVAllocatorCallback &inAlloc)
{
    Qt3DSTranslator *theTranslator = NULL;
    switch (inObject.m_Type) {
#define QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(type)                                                   \
    case GraphObjectTypes::type:                                                                   \
        theTranslator =                                                                            \
            STranslatorCreator<S##type##Translator>::Create(inElement, inObject, inAlloc);         \
        break;
        QT3DS_RENDER_ITERATE_GRAPH_OBJECT_TYPES
#undef QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE
    default:
        QT3DS_ASSERT(false);
        break;
    }
    return theTranslator;
}

void Qt3DSTranslator::OnElementChanged(SPresentation &inPresentation,
                                      IQt3DSRenderContext &inRenderContext,
                                      Q3DStudio::IPresentation &inStudioPresentation)
{
    switch (m_RenderObject->m_Type) {
#define QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(type)                                                   \
    case GraphObjectTypes::type:                                                                   \
        STranslatorCreator<S##type##Translator>::OnElementChanged(                                 \
            *this, inPresentation, inRenderContext, inStudioPresentation);                         \
        break;
        QT3DS_RENDER_ITERATE_GRAPH_OBJECT_TYPES
#undef QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE
    default:
        QT3DS_ASSERT(false);
        break;
    }
}

Qt3DSTranslator *Qt3DSTranslator::LoadTranslator(SDataReader &inReader, size_t inElemOffset,
                                               NVDataRef<QT3DSU8> inSGSection,
                                               NVAllocatorCallback &inAllocator)
{
    // Reader points to a new translator but we don't know what type
    QT3DSU8 *theTranslatorStart = inReader.m_CurrentPtr;
    // Make sure things are aligned
    (void)theTranslatorStart;
    QT3DS_ASSERT((size_t)theTranslatorStart % 4 == 0);
    Qt3DSTranslator *theTranslator = inReader.Load<Qt3DSTranslator>();
    if (theTranslator) {
        size_t *elemPtr = reinterpret_cast<size_t *>(&theTranslator->m_Element);
        *elemPtr += inElemOffset;
        size_t *graphObjPtr = reinterpret_cast<size_t *>(&theTranslator->m_RenderObject);
        size_t sgSectionStart = reinterpret_cast<size_t>(inSGSection.begin());
        *graphObjPtr += sgSectionStart;
        // Call actual constructor to initialize vtable.
        switch (theTranslator->RenderObject().m_Type) {
#define QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE(type)                                                   \
    case GraphObjectTypes::type:                                                                   \
        STranslatorCreator<S##type##Translator>::InitializeTranslator(*theTranslator,              \
                                                                      inAllocator);                \
        break;
            QT3DS_RENDER_ITERATE_GRAPH_OBJECT_TYPES
#undef QT3DS_RENDER_HANDL_GRAPH_OBJECT_TYPE
        default:
            QT3DS_ASSERT(false);
            break;
        }
    }
    return theTranslator;
}
Q3DStudio::IPresentation *
Qt3DSTranslator::GetPresentationFromPresentation(SPresentation &inPresentation)
{
    return inPresentation.m_UserData.DynamicCast<Q3DStudio::IPresentation>();
}

void Qt3DSTranslator::InitializePointerTags(IStringTable &)
{
    g_TranslatorTag = 0x0044FEED;
    g_PresentationTag = 0x0022ABBA;
}

void Qt3DSTranslator::AssignUserData(Q3DStudio::IPresentation &inPresentation,
                                    SPresentation &inGraphPresentation)
{
    inGraphPresentation.m_UserData = STaggedPointer(&inPresentation);
}
