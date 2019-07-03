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

#pragma once

#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSVec4.h"
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include "foundation/StringTable.h"
#include <memory>

namespace qt3ds {
namespace foundation {
}
}

namespace qt3dsdm {
class IStringTable;
struct SMetaDataEffect;
struct SMetaDataCustomMaterial;
}

namespace qt3ds {
namespace render {
    class IInputStreamFactory;
}
}

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

using namespace qt3ds::foundation;

//==============================================================================
// Redifinition of qt3dsdm::DataModelDataType::Value
//==============================================================================
enum ERuntimeDataModelDataType {
    ERuntimeDataModelDataTypeNone,
    ERuntimeDataModelDataTypeFloat,
    ERuntimeDataModelDataTypeFloat2,
    ERuntimeDataModelDataTypeFloat3,
    ERuntimeDataModelDataTypeFloat4,
    ERuntimeDataModelDataTypeLong,
    ERuntimeDataModelDataTypeString,
    ERuntimeDataModelDataTypeBool,
    ERuntimeDataModelDataTypeLong4,
    ERuntimeDataModelDataTypeStringRef,
    ERuntimeDataModelDataTypeObjectRef,
    ERuntimeDataModelDataTypeStringOrInt,
};

//==============================================================================
// Redifinition of qt3dsdm::AdditionalMetaDataType
//==============================================================================
enum ERuntimeAdditionalMetaDataType {
    ERuntimeAdditionalMetaDataTypeNone,
    ERuntimeAdditionalMetaDataTypeStringList,
    ERuntimeAdditionalMetaDataTypeRange,
    ERuntimeAdditionalMetaDataTypeImage,
    ERuntimeAdditionalMetaDataTypeColor,
    ERuntimeAdditionalMetaDataTypeRotation,
    ERuntimeAdditionalMetaDataTypeFont,
    ERuntimeAdditionalMetaDataTypeFontSize,
    ERuntimeAdditionalMetaDataTypeMultiLine,
    ERuntimeAdditionalMetaDataTypeObjectRef,
    ERuntimeAdditionalMetaDataTypeMesh,
    ERuntimeAdditionalMetaDataTypeImport,
    ERuntimeAdditionalMetaDataTypeTexture,
    ERuntimeAdditionalMetaDataTypeRenderable,
    ERuntimeAdditionalMetaDataTypePathBuffer,
    ERuntimeAdditionalMetaDataTypeShadowMapResolution,
    ERuntimeAdditionalMetaDataTypeString,
};

typedef eastl::basic_string<char8_t> TRuntimeMetaDataStrType;
typedef qt3ds::foundation::CRegisteredString TStrTableStr;

struct SAttOrArg
{
    TRuntimeMetaDataStrType m_Name;
    TRuntimeMetaDataStrType m_FormalName;
    ERuntimeDataModelDataType m_DataType;
    ERuntimeAdditionalMetaDataType m_MetaType;
    eastl::vector<TRuntimeMetaDataStrType> m_MetaDataList;
    eastl::pair<float, float> m_MetaDataRange;
};

typedef eastl::vector<SAttOrArg> TAttOrArgList;
struct SHandler
{
    TRuntimeMetaDataStrType m_Name;
    TRuntimeMetaDataStrType m_FormalName;
    TAttOrArgList m_Arguments;
};

typedef eastl::vector<SHandler> THandlerList;

struct SVisualEvent
{
    TRuntimeMetaDataStrType m_Name;
    TRuntimeMetaDataStrType m_FormalName;
};

typedef eastl::vector<SVisualEvent> TVisualEventList;

struct SElementInfo
{
    TRuntimeMetaDataStrType m_ClassName;
    bool m_IsComponent;
    TAttOrArgList m_Attributes;
};

//==============================================================================
/**
 *	@class	IRuntimeMetaData
 *	@brief	Declare interfaces for querying meta data values
 */
class QT3DS_AUTOTEST_EXPORT IRuntimeMetaData : public NVReleasable
{

public:
    static const char *GetMetaDataDirectory()
    {
        return ":/res/DataModelMetadata/en-us/MetaData.xml";
    }
    //==============================================================================
    /**
     *	Check if a property for the specified type or id exists
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value type
     */
    virtual bool IsPropertyExist(TStrTableStr inType, TStrTableStr inProperty,
                                 TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get property value type
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value type
     */
    virtual ERuntimeDataModelDataType GetPropertyType(TStrTableStr inType, TStrTableStr inProperty,
                                                      TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get additional metadata type
     *	@param inType          default object type
     *	@param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *	@param return property meta data type
     */
    virtual ERuntimeAdditionalMetaDataType
    GetAdditionalType(TStrTableStr inType, TStrTableStr inProperty,
                      TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get property value as float
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value
     */
    virtual Option<float> GetPropertyValueFloat(TStrTableStr inType, TStrTableStr inProperty,
                                                TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get property value as QT3DSVec2
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value
     */
    virtual Option<qt3ds::QT3DSVec2> GetPropertyValueVector2(TStrTableStr inType, TStrTableStr inProperty,
                                                       TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get property value as QT3DSVec3
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value
     */
    virtual Option<qt3ds::QT3DSVec3> GetPropertyValueVector3(TStrTableStr inType, TStrTableStr inProperty,
                                                       TStrTableStr inId = TStrTableStr()) = 0;

    virtual Option<qt3ds::QT3DSVec4> GetPropertyValueVector4(TStrTableStr inType,
                                TStrTableStr inProperty, TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get property value as long
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value
     */
    virtual Option<qt3ds::QT3DSI32> GetPropertyValueLong(TStrTableStr inType, TStrTableStr inProperty,
                                              TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get property value as string
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value
     */
    virtual Option<TRuntimeMetaDataStrType>
    GetPropertyValueString(TStrTableStr inType, TStrTableStr inProperty,
                           TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get object ref property value as string
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value
     */
    virtual Option<TRuntimeMetaDataStrType>
    GetPropertyValueObjectRef(TStrTableStr inType, TStrTableStr inProperty,
                              TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get property value as bool
     *	@param inType          default object type
     *  @param inProperty      the property to query
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param return property value
     */
    virtual Option<bool> GetPropertyValueBool(TStrTableStr inType, TStrTableStr inProperty,
                                              TStrTableStr inId = TStrTableStr()) = 0;

    //==============================================================================
    /**
     *	Get references
     *	@param inType          default object type
     *  @param outReferences   References of the object
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     */
    virtual void GetReferences(const char *inType,
                               eastl::vector<TRuntimeMetaDataStrType> &outReferences,
                               const char *inId = NULL) = 0;

    //==============================================================================
    /**
     *	Check if the property is custom or built in (canonical)
     *	@param inId            object id
     *  @param inProperty      the property to query
     */
    virtual bool IsCustomProperty(const char *inId, const char *inProperty) = 0;

    //==============================================================================
    /**
     *	Check if the handle is custom or built in (canonical)
     *	@param inId            object id
     *  @param inHandlerName   the handler name
     */
    virtual bool IsCustomHandler(const char *inId, const char *inHandlerName) = 0;

    virtual THandlerList GetCustomHandlers(const char *inId) = 0;

    virtual TVisualEventList GetVisualEvents(const char *inId) = 0;

    virtual SElementInfo LoadElement(const char *clsName, const char *clsRef, const char *inId) = 0;

    virtual TAttOrArgList GetSlideAttributes() = 0;

    //==============================================================================
    /**
     *	Get the data type and additional type of the custom handler
     *	@param inId                object id
     *  @param inHandlerName       the handler name
     *  @param inArgumentName      the handler argument name
     *  @param outType             the type
     *  @param outAdditionalType   the additional type
     */
    virtual void GetHandlerArgumentType(const char *inId, const char *inHandlerName,
                                        const char *inArgumentName,
                                        ERuntimeDataModelDataType &outType,
                                        ERuntimeAdditionalMetaDataType &outAdditionalType) = 0;

    //==============================================================================
    /**
     *	Get properties of the object with the given id, with the option of searching the parent or
     *only specific to this object.
     *	@param inType          default object type
     *	@param inId            object id. if this value is not trivial, the query is based on inId
     *instead of inType
     *  @param outProperties   the list of property names
     *  @param inSearchParent  if true, get the properties defined on the object or its derivation
     *parents
     *                         else, get only the properties defined on the object, not properties
     *derived from the parent.
     */
    virtual void GetInstanceProperties(const char *inType, const char *inId,
                                       eastl::vector<TRuntimeMetaDataStrType> &outProperties,
                                       bool inSearchParent) = 0;

    virtual bool LoadScriptFile(const char *inType, const char *inId, const char *inName,
                                const char *inSourcePath) = 0;

    virtual bool LoadEffectXMLFile(const char *inType, const char *inId, const char *inName,
                                   const char *inSourcePath) = 0;

    virtual bool LoadMaterialXMLFile(const char *inType, const char *inId, const char *inName,
                                     const char *inSourcePath) = 0;

    virtual bool LoadPluginXMLFile(const char *inType, const char *inId, const char *inName,
                                   const char *inSourcePath) = 0;

    virtual Option<qt3dsdm::SMetaDataEffect>
    GetEffectMetaDataBySourcePath(const char *sourcePath) = 0;
    virtual Option<qt3dsdm::SMetaDataEffect> GetEffectMetaDataByName(const char *name) = 0;

    virtual Option<qt3dsdm::SMetaDataCustomMaterial>
    GetMaterialMetaDataBySourcePath(const char *inSourcePath) = 0;
    virtual Option<qt3dsdm::SMetaDataCustomMaterial> GetMaterialMetaDataByName(const char *name) = 0;

    virtual std::shared_ptr<qt3dsdm::IStringTable> GetStringTable() = 0;

    virtual TStrTableStr Register(const char *inStr) = 0;

    // Clear data that affects project loading (mainly id->instance map)
    virtual void ClearPerProjectData() = 0;

public:
    //==============================================================================
    /**
     *	Creation function creates an object implements the IRuntimeMetaData interface
     *	This takes an input stream factory because on android the metadata is installed
     *	in the APK file, so we don't have exact access to it on the filesystem.
     */
    static IRuntimeMetaData &Create(qt3ds::render::IInputStreamFactory &inInputStreamFactory);
};

} // namespace Q3DStudio
