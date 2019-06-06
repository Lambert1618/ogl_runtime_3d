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
#include "Qt3DSDMMetaDataValue.h"

namespace qt3ds {
namespace render {
    namespace dynamic {
        struct SCommand;
        struct SPropertyDeclaration;
        struct SPropertyDefinition;
    }
}
}

namespace qt3dsdm {

struct HandlerArgumentType {

  enum Value {
    None,
    Property, // Property reference. Load the Properties of the Target Object.
    Dependent, // Property type depends on another property.
    Slide, // Slide reference. Load the list of slides of the Target Object if
    // applicable.
    Event, // Event reference. Load the applicable Events of the Target Object.
    Object, // Object reference. Used for dynamic actions with object referenced
    // property.
    Signal, // Signal reference. Used for emiting signals fired from the trigger
    // object.
  };

  Q_ENUM(Value)
  Q_GADGET
};

struct CompleteMetaDataType
{
    enum Enum {
        Unknown = 0,
        StringList,
        FloatRange,
        LongRange,
        Float,
        Long,
        Float2,
        Vector,
        Scale,
        Rotation,
        Color,
        Boolean,
        Slide,
        Font,
        FontSize,
        String,
        MultiLineString,
        ObjectRef,
        Image,
        Mesh,
        Import,
        Texture,
        Guid,
        StringListOrInt,
        Renderable,
        Image2D,
        Buffer,
        PathBuffer,
        ShadowMapResolution,
    };

    // Functions defined in UICDMMetaData.cpp
    static DataModelDataType::Value ToDataType(Enum inCompleteType);
    static AdditionalMetaDataType::Value ToAdditionalType(Enum inCompleteType);
    static CompleteMetaDataType::Enum ToCompleteType(DataModelDataType::Value inDataType,
                                                     AdditionalMetaDataType::Value inAdditionalType);
    Q_ENUM(Enum)
    Q_GADGET
};

typedef std::pair<DataModelDataType::Value, AdditionalMetaDataType::Value> TDataTypePair;

inline bool Equals(const TMetaDataData &lhs, const TMetaDataData &rhs)
{
    return rhs == lhs;
}

inline AdditionalMetaDataType::Value GetMetaDataValueType(const TMetaDataData &inValue)
{
    return inValue.getType();
}

// Base class shared between property info
// and handler arguments.
struct SMetaPropertyBase
{
    TCharStr m_Name;
    TCharStr m_FormalName;
    TCharStr m_Usage;
    TCharStr m_Description;
    CompleteMetaDataType::Enum m_CompleteType;
    TMetaDataData m_MetaDataData;
    SValue m_DefaultValue;

    SMetaPropertyBase()
        : m_CompleteType(CompleteMetaDataType::Float)
    {
    }
    bool operator==(const SMetaPropertyBase &inOther) const
    {
        return m_Name == inOther.m_Name && m_FormalName == inOther.m_FormalName
            && m_Usage == inOther.m_Usage && m_Description == inOther.m_Description
            && m_CompleteType == inOther.m_CompleteType
            && Equals(m_DefaultValue.toOldSkool(), inOther.m_DefaultValue.toOldSkool())
            && Equals(m_MetaDataData, inOther.m_MetaDataData);
    }
    DataModelDataType::Value GetDataType() const
    {
        return CompleteMetaDataType::ToDataType(m_CompleteType);
    }
    AdditionalMetaDataType::Value GetAdditionalType() const
    {
        return CompleteMetaDataType::ToAdditionalType(m_CompleteType);
    }
};

struct SMetaDataPropertyInfo : SMetaPropertyBase
{
    Qt3DSDMInstanceHandle m_Instance;
    Qt3DSDMPropertyHandle m_Property;
    bool m_Controllable = false; // Can this property be controlled via data input (default no)
    bool m_IsHidden = false;     // Is this property hidden in the inspector (default no)
    bool m_Animatable = true;    // Is this property animatable (default yes)
    // Note: all animatables are controllable

    TCharStr m_GroupName; // Name of the group this property belongs to or "default"

    SMetaDataPropertyInfo(Qt3DSDMInstanceHandle inInstance)
        : m_Instance(inInstance)
    {
    }
    SMetaDataPropertyInfo() {}

    bool operator==(const SMetaDataPropertyInfo &inOther) const
    {
        return m_Instance == inOther.m_Instance && m_Property == inOther.m_Property
            && m_IsHidden == inOther.m_IsHidden && m_Animatable == inOther.m_Animatable
            && m_GroupName == inOther.m_GroupName
            && m_Controllable == inOther.m_Controllable
            && SMetaPropertyBase::operator==(inOther);
    }

    bool operator!=(const SMetaDataPropertyInfo &inOther) const { return !(*this == inOther); }
};

struct SMetaDataHandlerArgumentInfo : SMetaPropertyBase
{
    Qt3DSDMHandlerHandle m_Handler;
    HandlerArgumentType::Value m_ArgType;
    SMetaDataHandlerArgumentInfo(Qt3DSDMHandlerHandle inHandler = Qt3DSDMHandlerHandle())
        : m_Handler(inHandler)
        , m_ArgType(HandlerArgumentType::None)
    {
    }
};

struct SCategoryInfo
{
    TCharStr m_Name;
    TCharStr m_Description;
    TCharStr m_Icon;
    TCharStr m_HighlightIcon;
    bool m_Canonical;

    SCategoryInfo()
        : m_Canonical(false)
    {
    }
    SCategoryInfo(TCharStr inName)
        : m_Name(inName)
        , m_Canonical(false)
    {
    }
};

struct SEventInfo
{
    bool operator!=(const SEventInfo &inEvent) const
    {
        return (m_Name != inEvent.m_Name || m_FormalName != inEvent.m_FormalName
                || m_Category != inEvent.m_Category || m_Description != inEvent.m_Description);
    }

    TCharStr m_Name;
    TCharStr m_FormalName;
    TCharStr m_Category;
    TCharStr m_Description;
};

struct SHandlerInfo
{
    TCharStr m_Name;
    TCharStr m_FormalName;
    TCharStr m_Category;
    TCharStr m_Description;

    bool operator!=(const SHandlerInfo &inHandler) const
    {
        return (m_Name != inHandler.m_Name || m_FormalName != inHandler.m_FormalName
                || m_Category != inHandler.m_Category || m_Description != inHandler.m_Description);
    }
};

struct PropertyFilterTypes
{
    enum Enum {
        Unknown,
        ShowIfEqual,
        HideIfEqual,
    };
};

struct SPropertyFilterInfo
{
    PropertyFilterTypes::Enum m_FilterType;
    Qt3DSDMPropertyHandle m_FilterProperty;
    SValue m_Value;
    SPropertyFilterInfo()
        : m_FilterType(PropertyFilterTypes::Unknown)
    {
    }
    SPropertyFilterInfo(PropertyFilterTypes::Enum inFilterType, Qt3DSDMPropertyHandle inProp,
                        const SValue &inValue)
        : m_FilterType(inFilterType)
        , m_FilterProperty(inProp)
        , m_Value(inValue)
    {
    }
};
struct SMetaDataShader
{
    TCharStr m_Name;
    TCharStr m_Type; ///< shader type (GLSL or HLSL)
    TCharStr m_Version; ///< shader version (e.g. 330 vor GLSL)
    // Code contains both the vertex and fragment portions separated by #define's.
    //#define VERTEX_SHADER, #define FRAGMENT_SHADER
    TCharStr m_Code;
    bool m_HasGeomShader;
    bool m_IsComputeShader;
    SMetaDataShader()
        : m_HasGeomShader(false)
        , m_IsComputeShader(false)
    {
    }
    SMetaDataShader(const TCharStr &inName, const TCharStr &inType, const TCharStr &inVersion,
                    const TCharStr &inCode, bool hasGeom, bool isCompute)
        : m_Name(inName)
        , m_Type(inType)
        , m_Version(inVersion)
        , m_Code(inCode)
        , m_HasGeomShader(hasGeom)
        , m_IsComputeShader(isCompute)
    {
    }
};

struct SMetaDataDynamicObject
{
    TCharStr m_Name;
    qt3ds::foundation::NVConstDataRef<SMetaDataShader> m_Shaders;
    qt3ds::foundation::NVConstDataRef<qt3ds::render::dynamic::SPropertyDefinition> m_Properties;
    SMetaDataDynamicObject() {}
    SMetaDataDynamicObject(
        const TCharStr &inName, qt3ds::foundation::NVConstDataRef<SMetaDataShader> inShaders,
        qt3ds::foundation::NVConstDataRef<qt3ds::render::dynamic::SPropertyDefinition> inProperties)
        : m_Name(inName)
        , m_Shaders(inShaders)
        , m_Properties(inProperties)
    {
    }
};

struct SMetaDataEffect : public SMetaDataDynamicObject
{
    qt3ds::foundation::NVConstDataRef<qt3ds::render::dynamic::SCommand *> m_EffectCommands;
    SMetaDataEffect() {}
    SMetaDataEffect(
        const TCharStr &inName, qt3ds::foundation::NVConstDataRef<SMetaDataShader> inShaders,
        qt3ds::foundation::NVConstDataRef<qt3ds::render::dynamic::SPropertyDefinition> inProperties,
        qt3ds::foundation::NVConstDataRef<qt3ds::render::dynamic::SCommand *> inEffectCommands)
        : SMetaDataDynamicObject(inName, inShaders, inProperties)
        , m_EffectCommands(inEffectCommands)
    {
    }
};

struct SMetaDataCustomMaterial : public SMetaDataDynamicObject
{
    qt3ds::foundation::NVConstDataRef<qt3ds::render::dynamic::SCommand *> m_CustomMaterialCommands;
    bool m_HasTransparency;
    bool m_HasRefraction;
    bool m_AlwaysDirty;
    unsigned int m_ShaderKey;
    unsigned int m_LayerCount;
    SMetaDataCustomMaterial() {}
    SMetaDataCustomMaterial(
        const TCharStr &inName, qt3ds::foundation::NVConstDataRef<SMetaDataShader> inShaders,
        qt3ds::foundation::NVConstDataRef<qt3ds::render::dynamic::SPropertyDefinition> inProperties,
        qt3ds::foundation::NVConstDataRef<qt3ds::render::dynamic::SCommand *> inCustomMaterialCommands,
        bool inHasTransparency, bool inHasRefraction, bool inIsAlwaysDirty,
        unsigned int inShaderKey, unsigned int inLayerCount)
        : SMetaDataDynamicObject(inName, inShaders, inProperties)
        , m_CustomMaterialCommands(inCustomMaterialCommands)
        , m_HasTransparency(inHasTransparency)
        , m_HasRefraction(inHasRefraction)
        , m_AlwaysDirty(inIsAlwaysDirty)
        , m_ShaderKey(inShaderKey)
        , m_LayerCount(inLayerCount)
    {
    }
};
}

Q_DECLARE_METATYPE(qt3dsdm::HandlerArgumentType::Value)

