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

//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSDMPrefix.h"
#include "Qt3DSMetadata.h"
#include "SimpleDataCore.h"
#include "Qt3DSDMXML.h"
#include "foundation/IOStreams.h"
#include "Qt3DSEulerAngles.h"
#include "Qt3DSDMWindowsCompatibility.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "DocumentResourceManagerScriptParser.h"
#include "foundation/StrConvertUTF.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "foundation/FileTools.h"
#include "foundation/TrackingAllocator.h"
#include "EASTL/hash_map.h"
#include "DocumentResourceManagerRenderPluginParser.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/StringTable.h"

using qt3ds::render::IInputStreamFactory;
using qt3ds::render::IRefCountedInputStream;

using namespace qt3dsdm;
using namespace Q3DStudio;

namespace {


struct SImportXmlErrorHandler : public CXmlErrorHandler
{
    void OnXmlError(const QString &errorName, int line, int) override
    {
        qCCritical(INTERNAL_ERROR) << "XML error:"
            << errorName << "on line" << line;
    }
};

struct SRuntimeMetaDataPropertyInfo
{
    ERuntimeDataModelDataType m_DataType;
    ERuntimeAdditionalMetaDataType m_AdditionalType;
    Option<SValue> m_Value;
    bool m_Exist;
    SRuntimeMetaDataPropertyInfo(ERuntimeDataModelDataType inDataType,
                                 ERuntimeAdditionalMetaDataType inAdditionalType,
                                 const Option<SValue> &inValue, bool inExist)
        : m_DataType(inDataType)
        , m_AdditionalType(inAdditionalType)
        , m_Value(inValue)
        , m_Exist(inExist)
    {
    }
    SRuntimeMetaDataPropertyInfo()
        : m_DataType(ERuntimeDataModelDataTypeNone)
        , m_AdditionalType(ERuntimeAdditionalMetaDataTypeNone)
        , m_Exist(false)
    {
    }
};

struct SPropertyKey
{
    const char8_t *m_TypeOrId;
    const char8_t *m_PropertyName;

    // The character pointers are expected to have come from the string table
    // thus we can safely use their exact pointer addresses for the hash code.
    SPropertyKey(const char8_t *typeOrId, const char8_t *inPname)
        : m_TypeOrId(typeOrId)
        , m_PropertyName(inPname)
    {
    }
    bool operator==(const SPropertyKey &inOther) const
    {
        // equality comparison safe since strings are assumed to come from string table.
        return m_TypeOrId == inOther.m_TypeOrId && m_PropertyName == inOther.m_PropertyName;
    }
};
}

namespace eastl {
template <>
struct hash<SPropertyKey>
{
    size_t operator()(const SPropertyKey &inKey) const
    {
        return reinterpret_cast<size_t>(inKey.m_TypeOrId)
            ^ reinterpret_cast<size_t>(inKey.m_PropertyName);
    }
};
}

namespace {

//===========================================================================
/**
 *	@class	SRuntimeMetaDataImpl
 *	@brief	implements the IRumtimeMetaData interface class
 */
struct SRuntimeMetaDataImpl : public IRuntimeMetaData
{

private:
    typedef std::map<TCharStr, int> TIdToHandleMap;
    typedef eastl::hash_map<SPropertyKey, SRuntimeMetaDataPropertyInfo> TPropertyLookupHash;

    //==============================================================================
    //	Fields
    //==============================================================================
    std::shared_ptr<qt3dsdm::IStringTable> m_StrTable;
    std::shared_ptr<IDataCore> m_DataCore;
    std::shared_ptr<IMetaData> m_NewMetaData;
    std::shared_ptr<SComposerObjectDefinitions> m_Objects;
    std::vector<TCharStr> m_TempReferences;

    TIdToHandleMap m_IdToHandleMap;
    // We cache properties by id or by instance so that the same query for the same value
    // will return an answer instantly instead of calling GetAggregateInstancePropertyByName which
    // is really
    // expensive.
    TPropertyLookupHash m_PropertyLookupHash;
    IInputStreamFactory &m_InputStreamFactory;

    // Helper to convert char to wchar_t
    eastl::basic_string<qt3ds::foundation::TWCharEASTLConverter::TCharType> m_Buf[4];

public:
    static void SimpleDataModelLogger(const char *inMessage)
    {
        qCInfo(TRACE_INFO) << SRuntimeMetaDataImpl::SimpleDataModelLogger
                               << inMessage;
    }
    //==============================================================================
    /**
     *	Constructor
     */
    SRuntimeMetaDataImpl(IInputStreamFactory &inFactory)
        : m_InputStreamFactory(inFactory)
    {
        // Use this to enable some simple uicdm debug logging.  Beware of a *lot* of log output.
        // g_DataModelDebugLogger = SimpleDataModelLogger;
        try {
            // need to pause here to hook up the debugger
            m_StrTable = qt3dsdm::IStringTable::CreateStringTable();
            m_DataCore = std::make_shared<CSimpleDataCore>(m_StrTable);
            m_NewMetaData = IMetaData::CreateNewMetaData(m_DataCore);

            m_Objects = std::make_shared<SComposerObjectDefinitions>(std::ref(*m_DataCore),
                                                                       std::ref(*m_NewMetaData));
        } catch (qt3dsdm::Qt3DSDMError &error) {
            qCCritical(INTERNAL_ERROR) << "SRuntimeMetaDataImpl Qt3DSDMError: "
                                       << m_StrTable->GetNarrowStr(error.m_Message);
        } catch (std::runtime_error &exc) {
            if (exc.what()) {
                qCCritical(INTERNAL_ERROR) << "SRuntimeMetaDataImpl std::runtime_error: "
                                           << exc.what();
            } else {
                qCCritical(INTERNAL_ERROR) << "SRuntimeMetaDataImpl std::runtime_error";
            }
        } catch (std::exception &exc) {
            if (exc.what()) {
                qCCritical(INTERNAL_ERROR) << "SRuntimeMetaDataImpl std::exception: "
                                           << exc.what();
            } else {
                qCCritical(INTERNAL_ERROR) << "SRuntimeMetaDataImpl std::exception";
            }
        }
    }

    void Load()
    {
        // Create the base types.
        std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(m_StrTable));
        const char8_t *theMetaDataPath = GetMetaDataDirectory();
        NVScopedRefCounted<IRefCountedInputStream> theInStream(
            m_InputStreamFactory.GetStreamForFile(theMetaDataPath));
        SDOMElement *topElement = NULL;
        if (theInStream)
            topElement = CDOMSerializer::Read(*theFactory, *theInStream);
        if (topElement) {
            shared_ptr<IDOMReader> theReader =
                IDOMReader::CreateDOMReader(*topElement, m_StrTable, theFactory);
            m_NewMetaData->Load(*theReader);

        } else {
            qCCritical(INVALID_OPERATION) << "Metadata loading failed", "Failed to find meta data file";
            // Q3DStudio_ASSERT requires static linkage to UICSystem, which is only used
            // by UICEngine and UICRuntime
            // QT3DS_ASSERT requires the qt3ds foundation libraries which are used by all products.
            QT3DS_ASSERT(false);
            return;
        }
    }

    // Helper to convert wchar_t to eastl::string
    static inline void ConvertWide(const wchar_t *inType, eastl::string &outStr)
    {
        if (inType && *inType)
            qt3ds::foundation::ConvertUTF(
                reinterpret_cast<const qt3ds::foundation::TWCharEASTLConverter::TCharType *>(inType),
                0, outStr);
        else
            outStr.clear();
    }

    // Helper to convert char to wchar_t
    const wchar_t *ConvertChar(const char *inName, int inBufferIndex)
    {
        if (inName && *inName) {
            qt3ds::foundation::ConvertUTF(inName, 0, m_Buf[inBufferIndex]);
            return reinterpret_cast<const wchar_t *>(m_Buf[inBufferIndex].c_str());
        }
        return NULL;
    }
    inline const wchar_t *Convert0(const char *inName) { return ConvertChar(inName, 0); }
    inline const wchar_t *Convert1(const char *inName) { return ConvertChar(inName, 1); }
    inline const wchar_t *Convert2(const char *inName) { return ConvertChar(inName, 2); }
    inline const wchar_t *Convert3(const char *inName) { return ConvertChar(inName, 3); }

    inline Qt3DSDMInstanceHandle GetInstanceForType(const wchar_t *inType)
    {
        return m_Objects->GetInstanceForType(ComposerObjectTypes::Convert(inType));
    }

    Qt3DSDMInstanceHandle GetInstanceById(const wchar_t *inId)
    {
        if (IsTrivial(inId))
            return 0;
        if (inId[0] == '#')
            ++inId;

        TIdToHandleMap::iterator find = m_IdToHandleMap.find(inId);
        if (find != m_IdToHandleMap.end())
            return find->second;
        return 0;
    }

    virtual bool IsPropertyExist(const wchar_t *inType, const wchar_t *inProperty,
                                 const wchar_t *inId)
    {
        Qt3DSDMInstanceHandle theInstance =
            IsTrivial(inId) ? GetInstanceForType(inType) : GetInstanceById(inId);
        if (theInstance.Valid()) {
            Qt3DSDMPropertyHandle theProperty =
                m_DataCore->GetAggregateInstancePropertyByName(theInstance, inProperty);

            return theProperty.Valid();
        }
        return false;
    }

    ERuntimeDataModelDataType Convert(DataModelDataType::Value inType)
    {
        switch (inType) {
        case DataModelDataType::None:
            return ERuntimeDataModelDataTypeNone;
        case DataModelDataType::Float:
            return ERuntimeDataModelDataTypeFloat;
        case DataModelDataType::Float2:
            return ERuntimeDataModelDataTypeFloat2;
        case DataModelDataType::Float3:
            return ERuntimeDataModelDataTypeFloat3;
        case DataModelDataType::Float4:
            return ERuntimeDataModelDataTypeFloat4;
        case DataModelDataType::Long:
            return ERuntimeDataModelDataTypeLong;
        case DataModelDataType::String:
            return ERuntimeDataModelDataTypeString;
        case DataModelDataType::Bool:
            return ERuntimeDataModelDataTypeBool;
        case DataModelDataType::Long4:
            return ERuntimeDataModelDataTypeLong4;
        case DataModelDataType::StringRef:
            return ERuntimeDataModelDataTypeStringRef;
        case DataModelDataType::ObjectRef:
            return ERuntimeDataModelDataTypeObjectRef;
        case DataModelDataType::StringOrInt:
            return ERuntimeDataModelDataTypeStringOrInt;
        default:
            QT3DS_ASSERT(false);
            return ERuntimeDataModelDataTypeNone;
        }
    }

    virtual ERuntimeDataModelDataType
    GetPropertyType(const wchar_t *inType, const wchar_t *inProperty, const wchar_t *inId)
    {
        Qt3DSDMInstanceHandle theInstance =
            IsTrivial(inId) ? GetInstanceForType(inType) : GetInstanceById(inId);
        Qt3DSDMPropertyHandle theProperty =
            m_DataCore->GetAggregateInstancePropertyByName(theInstance, inProperty);
        return Convert(m_DataCore->GetProperty(theProperty).m_Type);
    }

    ERuntimeAdditionalMetaDataType Convert(AdditionalMetaDataType::Value inType)
    {
        switch (inType) {
        case AdditionalMetaDataType::None:
            return ERuntimeAdditionalMetaDataTypeNone;
        case AdditionalMetaDataType::StringList:
            return ERuntimeAdditionalMetaDataTypeStringList;
        case AdditionalMetaDataType::Range:
            return ERuntimeAdditionalMetaDataTypeRange;
        case AdditionalMetaDataType::Image:
            return ERuntimeAdditionalMetaDataTypeImage;
        case AdditionalMetaDataType::Color:
            return ERuntimeAdditionalMetaDataTypeColor;
        case AdditionalMetaDataType::Rotation:
            return ERuntimeAdditionalMetaDataTypeRotation;
        case AdditionalMetaDataType::Font:
            return ERuntimeAdditionalMetaDataTypeFont;
        case AdditionalMetaDataType::FontSize:
            return ERuntimeAdditionalMetaDataTypeFontSize;
        case AdditionalMetaDataType::MultiLine:
            return ERuntimeAdditionalMetaDataTypeMultiLine;
        case AdditionalMetaDataType::ObjectRef:
            return ERuntimeAdditionalMetaDataTypeObjectRef;
        case AdditionalMetaDataType::Mesh:
            return ERuntimeAdditionalMetaDataTypeMesh;
        case AdditionalMetaDataType::Import:
            return ERuntimeAdditionalMetaDataTypeImport;
        case AdditionalMetaDataType::Texture:
            return ERuntimeAdditionalMetaDataTypeTexture;
        case AdditionalMetaDataType::Renderable:
            return ERuntimeAdditionalMetaDataTypeRenderable;
        case AdditionalMetaDataType::PathBuffer:
            return ERuntimeAdditionalMetaDataTypePathBuffer;
        case AdditionalMetaDataType::ShadowMapResolution:
            return ERuntimeAdditionalMetaDataTypeShadowMapResolution;
        case AdditionalMetaDataType::String:
            return ERuntimeAdditionalMetaDataTypeString;
        default:
            QT3DS_ASSERT(false);
            return ERuntimeAdditionalMetaDataTypeNone;
        }
    }

    virtual ERuntimeAdditionalMetaDataType
    GetAdditionalType(const wchar_t *inType, const wchar_t *inProperty, const wchar_t *inId)
    {
        Qt3DSDMInstanceHandle theInstance =
            IsTrivial(inId) ? GetInstanceForType(inType) : GetInstanceById(inId);
        Qt3DSDMPropertyHandle theProperty =
            m_DataCore->GetAggregateInstancePropertyByName(theInstance, inProperty);
        return Convert(m_NewMetaData->GetAdditionalMetaDataType(theInstance, theProperty));
    }

    bool GetPropertyValue(const wchar_t *inType, const wchar_t *inProperty, const wchar_t *inId,
                          SValue &outValue)
    {
        Qt3DSDMInstanceHandle theInstance =
            IsTrivial(inId) ? GetInstanceForType(inType) : GetInstanceById(inId);
        if (theInstance.Valid()) {
            Qt3DSDMPropertyHandle theProperty =
                m_DataCore->GetAggregateInstancePropertyByName(theInstance, inProperty);
            return m_DataCore->GetInstancePropertyValue(theInstance, theProperty, outValue);
        } else {
            eastl::string typeStr;
            eastl::string propertyStr;
            eastl::string idStr;
            ConvertWide(inType, typeStr);
            ConvertWide(inProperty, propertyStr);
            ConvertWide(inId, idStr);
            qCCritical(INVALID_OPERATION) << "GetPropertyValueString: "
                                          << typeStr.c_str() << " "
                                          << propertyStr.c_str() << " "
                                          << idStr.c_str();
            QT3DS_ASSERT(false);
        }
        return false;
    }

    SRuntimeMetaDataPropertyInfo &FindProperty(TStrTableStr inType, TStrTableStr inProperty,
                                               TStrTableStr inId)
    {
        TStrTableStr theTypeOrId = inId.IsValid() ? inId : inType;
        SPropertyKey theKey(theTypeOrId, inProperty);
        eastl::pair<TPropertyLookupHash::iterator, bool> theLookup =
            m_PropertyLookupHash.insert(eastl::make_pair(theKey, SRuntimeMetaDataPropertyInfo()));
        if (theLookup.second) {
            const wchar_t *theType(Convert0(inType));
            const wchar_t *theProperty(Convert1(inProperty));
            const wchar_t *theId(Convert2(inId));
            if (IsPropertyExist(theType, theProperty, theId)) {
                ERuntimeDataModelDataType theDataType(GetPropertyType(theType, theProperty, theId));
                ERuntimeAdditionalMetaDataType theAdditionalType(
                    GetAdditionalType(theType, theProperty, theId));
                Option<SValue> theValue;
                SValue theValueData;
                if (GetPropertyValue(theType, theProperty, theId, theValueData))
                    theValue = theValueData.toOldSkool();
                theLookup.first->second =
                    SRuntimeMetaDataPropertyInfo(theDataType, theAdditionalType, theValue, true);
            }
        }
        return theLookup.first->second;
    }

    bool IsPropertyExist(TStrTableStr inType, TStrTableStr inProperty, TStrTableStr inId) override
    {
        return FindProperty(inType, inProperty, inId).m_Exist;
    }

    ERuntimeDataModelDataType GetPropertyType(TStrTableStr inType, TStrTableStr inProperty,
                                                      TStrTableStr inId) override
    {
        return FindProperty(inType, inProperty, inId).m_DataType;
    }

    virtual ERuntimeAdditionalMetaDataType
    GetAdditionalType(TStrTableStr inType, TStrTableStr inProperty, TStrTableStr inId) override
    {
        return FindProperty(inType, inProperty, inId).m_AdditionalType;
    }

    Option<float> GetPropertyValueFloat(TStrTableStr inType, TStrTableStr inProperty,
                                                TStrTableStr inId) override
    {
        SRuntimeMetaDataPropertyInfo &theInfo(FindProperty(inType, inProperty, inId));
        if (theInfo.m_Value.hasValue())
            return qt3dsdm::get<float>(*theInfo.m_Value);
        return Empty();
    }

    Option<qt3ds::QT3DSVec2> GetPropertyValueVector2(TStrTableStr inType, TStrTableStr inProperty,
                                                       TStrTableStr inId) override
    {
        SRuntimeMetaDataPropertyInfo &theInfo(FindProperty(inType, inProperty, inId));
        if (theInfo.m_Value.hasValue()) {
            SFloat2 theFloat2 = qt3dsdm::get<SFloat2>(*theInfo.m_Value);
            return qt3ds::QT3DSVec2(theFloat2[0], theFloat2[1]);
        }
        return Empty();
    }

    Option<qt3ds::QT3DSVec3> GetPropertyValueVector3(TStrTableStr inType, TStrTableStr inProperty,
                                                       TStrTableStr inId) override
    {
        SRuntimeMetaDataPropertyInfo &theInfo(FindProperty(inType, inProperty, inId));
        if (theInfo.m_Value.hasValue()) {
            SFloat3 theFloat3 = qt3dsdm::get<SFloat3>(*theInfo.m_Value);
            return qt3ds::QT3DSVec3(theFloat3[0], theFloat3[1], theFloat3[2]);
        }
        return Empty();
    }

    Option<qt3ds::QT3DSVec4> GetPropertyValueVector4(TStrTableStr inType, TStrTableStr inProperty,
                                                       TStrTableStr inId) override
    {
        SRuntimeMetaDataPropertyInfo &theInfo(FindProperty(inType, inProperty, inId));
        if (theInfo.m_Value.hasValue()) {
            SFloat4 theFloat4 = qt3dsdm::get<SFloat4>(*theInfo.m_Value);
            return qt3ds::QT3DSVec4(theFloat4[0], theFloat4[1], theFloat4[2], theFloat4[3]);
        }
        return Empty();
    }

    virtual Option<qt3ds::QT3DSI32> GetPropertyValueLong(TStrTableStr inType, TStrTableStr inProperty,
                                              TStrTableStr inId) override
    {
        SRuntimeMetaDataPropertyInfo &theInfo(FindProperty(inType, inProperty, inId));
        if (theInfo.m_Value.hasValue())
            return qt3dsdm::get<qt3ds::QT3DSI32>(*theInfo.m_Value);
        return Empty();
    }

    virtual Option<TRuntimeMetaDataStrType>
    GetPropertyValueString(TStrTableStr inType, TStrTableStr inProperty, TStrTableStr inId) override
    {
        SRuntimeMetaDataPropertyInfo &theInfo(FindProperty(inType, inProperty, inId));
        if (theInfo.m_Value.hasValue()) {
            TDataStrPtr theStr = qt3dsdm::get<TDataStrPtr>(*theInfo.m_Value);
            TRuntimeMetaDataStrType retval;
            qt3ds::foundation::ConvertUTF(
                reinterpret_cast<const qt3ds::foundation::TWCharEASTLConverter::TCharType *>(
                    theStr->GetData()),
                0, retval);
            return retval;
        }
        return Empty();
    }

    virtual Option<TRuntimeMetaDataStrType>
    GetPropertyValueObjectRef(TStrTableStr inType, TStrTableStr inProperty, TStrTableStr inId) override
    {
        SRuntimeMetaDataPropertyInfo &theInfo(FindProperty(inType, inProperty, inId));
        if (theInfo.m_Value.hasValue()) {
            SObjectRefType theObjectRef = ConvertToObjectRef(*theInfo.m_Value);
            if (theObjectRef.GetReferenceType() == ObjectReferenceType::Relative) {
                TDataStrPtr theStr = qt3dsdm::get<TDataStrPtr>(theObjectRef.m_Value);
                TRuntimeMetaDataStrType retval;
                qt3ds::foundation::ConvertUTF(
                    reinterpret_cast<const qt3ds::foundation::TWCharEASTLConverter::TCharType *>(
                        theStr->GetData()),
                    0, retval);
                return retval;
            } else // we don't know how to solve absolute reference type
            {
                // QT3DS_ASSERT( false );
                return TRuntimeMetaDataStrType("");
            }
        }
        return Empty();
    }

    Option<bool> GetPropertyValueBool(TStrTableStr inType, TStrTableStr inProperty,
                                              TStrTableStr inId) override
    {
        SRuntimeMetaDataPropertyInfo &theInfo(FindProperty(inType, inProperty, inId));
        if (theInfo.m_Value.hasValue())
            return qt3dsdm::get<bool>(*theInfo.m_Value);
        return Empty();
    }

    virtual void GetReferences(const wchar_t *inType,
                               eastl::vector<TRuntimeMetaDataStrType> &outReferences,
                               const wchar_t *inId)
    {
        Qt3DSDMInstanceHandle theInstance =
            IsTrivial(inId) ? GetInstanceForType(inType) : GetInstanceById(inId);

        if (theInstance.Valid()) {
            m_TempReferences.clear();
            m_NewMetaData->GetReferences(theInstance, m_TempReferences);
            outReferences.resize((uint32_t)m_TempReferences.size());
            for (uint32_t idx = 0, end = (uint32_t)m_TempReferences.size(); idx < end; ++idx) {
                qt3ds::foundation::ConvertUTF(m_TempReferences[idx].c_str(),
                                           m_TempReferences[idx].size(), outReferences[idx]);
            }
        }
    }

    void GetReferences(const char *inType,
                               eastl::vector<TRuntimeMetaDataStrType> &outReferences,
                               const char *inId) override
    {
        GetReferences(Convert0(inType), outReferences, Convert2(inId));
    }

    virtual bool IsCustomProperty(const wchar_t *inId, const wchar_t *inProperty)
    {
        Qt3DSDMInstanceHandle theInstance = GetInstanceById(inId);
        Qt3DSDMPropertyHandle theProperty =
            m_DataCore->GetAggregateInstancePropertyByName(theInstance, inProperty);
        return m_NewMetaData->IsCustomProperty(theInstance, theProperty);
    }

    bool IsCustomProperty(const char *inId, const char *inProperty) override
    {
        return IsCustomProperty(Convert2(inId), Convert1(inProperty));
    }

    virtual bool IsCustomHandler(const wchar_t *inId, const wchar_t *inHandlerName)
    {
        Qt3DSDMInstanceHandle theInstance = GetInstanceById(inId);
        Qt3DSDMHandlerHandle theHandle =
            m_NewMetaData->FindHandlerByName(theInstance, inHandlerName);
        return m_NewMetaData->IsCustomHandler(theHandle);
    }

    inline void SetDataType(SAttOrArg &arg, const SMetaPropertyBase &argInfo)
    {
        arg.m_DataType = Convert(argInfo.GetDataType());
        arg.m_MetaType = Convert(argInfo.GetAdditionalType());
        qt3ds::foundation::ConvertUTF(argInfo.m_Name.c_str(), argInfo.m_Name.size(), arg.m_Name);
        qt3ds::foundation::ConvertUTF(argInfo.m_FormalName.c_str(), argInfo.m_FormalName.size(),
                                   arg.m_FormalName);
        switch (argInfo.GetAdditionalType()) {
        case qt3dsdm::AdditionalMetaDataType::StringList: {
            const eastl::vector<TCharStr> &theData =
                argInfo.m_MetaDataData.getData<eastl::vector<TCharStr>>();
            arg.m_MetaDataList.resize(theData.size());
            for (uint32_t metaIdx = 0, metaEnd = theData.size(); metaIdx < metaEnd; ++metaIdx)
                qt3ds::foundation::ConvertUTF(theData[metaIdx].c_str(), theData[metaIdx].size(),
                                           arg.m_MetaDataList[metaIdx]);
        } break;
        case qt3dsdm::AdditionalMetaDataType::Range: {
            const SMetaDataRange &range = argInfo.m_MetaDataData.getData<SMetaDataRange>();
            arg.m_MetaDataRange = eastl::make_pair(range.m_min, range.m_max);
        } break;
        default:
            break;
        }
    }

    THandlerList GetCustomHandlers(const char *inId) override
    {
        THandlerList retval;
        Qt3DSDMInstanceHandle theInstance = GetInstanceById(Convert0(inId));
        THandlerHandleList handlerList;
        m_NewMetaData->GetHandlers(theInstance, handlerList);
        retval.reserve((uint32_t)handlerList.size());
        for (size_t idx = 0, end = handlerList.size(); idx < end; ++idx) {
            if (m_NewMetaData->IsCustomHandler(handlerList[idx])) {
                SHandlerInfo theInfo(m_NewMetaData->GetHandlerInfo(handlerList[idx]));
                retval.push_back(SHandler());
                SHandler &handler(retval.back());
                qt3ds::foundation::ConvertUTF(theInfo.m_Name.c_str(), theInfo.m_Name.size(),
                                           handler.m_Name);
                qt3ds::foundation::ConvertUTF(theInfo.m_FormalName.c_str(),
                                           theInfo.m_FormalName.size(), handler.m_FormalName);
                vector<SMetaDataHandlerArgumentInfo> theArgInfo;
                m_NewMetaData->GetHandlerArguments(handlerList[idx], theArgInfo);
                handler.m_Arguments.resize((uint32_t)theArgInfo.size());
                for (uint32_t argIdx = 0, argEnd = (uint32_t)theArgInfo.size(); argIdx < argEnd;
                     ++argIdx) {
                    const SMetaDataHandlerArgumentInfo &argInfo(theArgInfo[argIdx]);
                    SAttOrArg &arg = handler.m_Arguments[argIdx];
                    SetDataType(arg, argInfo);
                }
            }
        }
        return retval;
    }

    TVisualEventList GetVisualEvents(const char *inId) override
    {
        TVisualEventList theRetval;
        Qt3DSDMInstanceHandle theInstance = GetInstanceById(Convert0(inId));
        TEventHandleList theEventList;
        m_NewMetaData->GetEvents(theInstance, theEventList);
        theRetval.reserve((uint32_t)theEventList.size());
        for (uint32_t idx = 0, end = (uint32_t)theEventList.size(); idx < end; ++idx) {
            SEventInfo theInfo(m_NewMetaData->GetEventInfo(theEventList[idx]));
            theRetval.push_back(SVisualEvent());
            SVisualEvent &theEvent(theRetval.back());
            qt3ds::foundation::ConvertUTF(theInfo.m_Name.c_str(), theInfo.m_Name.size(),
                                       theEvent.m_Name);
            qt3ds::foundation::ConvertUTF(theInfo.m_FormalName.c_str(), theInfo.m_FormalName.size(),
                                       theEvent.m_FormalName);
        }
        return theRetval;
    }

    void MetaPropertiesToAttList(vector<Qt3DSDMMetaDataPropertyHandle> &theProperties,
                                 TAttOrArgList &outAtts)
    {
        outAtts.reserve((uint32_t)theProperties.size());
        for (uint32_t idx = 0, end = (uint32_t)theProperties.size(); idx < end; ++idx) {
            SMetaDataPropertyInfo thePropInfo =
                m_NewMetaData->GetMetaDataPropertyInfo(theProperties[idx]);
            outAtts.push_back(SAttOrArg());
            SAttOrArg &arg(outAtts.back());
            arg.m_DataType = Convert(thePropInfo.GetDataType());
            arg.m_MetaType = Convert(thePropInfo.GetAdditionalType());
            qt3ds::foundation::ConvertUTF(thePropInfo.m_Name.c_str(), thePropInfo.m_Name.size(),
                                       arg.m_Name);
            qt3ds::foundation::ConvertUTF(thePropInfo.m_FormalName.c_str(),
                                       thePropInfo.m_FormalName.size(), arg.m_FormalName);
            SetDataType(arg, thePropInfo);
        }
    }

    SElementInfo LoadElement(const char *clsName, const char *clsRef, const char *inId) override
    {
        SElementInfo retval;
        Qt3DSDMInstanceHandle theInstance;
        Qt3DSDMInstanceHandle parentInstance;
        if (!qt3ds::foundation::isTrivial(clsRef)) {
            if (clsRef[0] == '#')
                ++clsRef;
            parentInstance = GetInstanceById(Convert0(clsRef));
        } else {
            parentInstance = m_NewMetaData->GetCanonicalInstanceForType(Convert0(clsName));
        }
        if (parentInstance.Valid() == false) {
            QT3DS_ASSERT(false);
            return retval;
        }
        theInstance = m_DataCore->CreateInstance();
        m_DataCore->DeriveInstance(theInstance, parentInstance);
        m_IdToHandleMap.insert(std::make_pair(TCharStr(Convert0(inId)), theInstance));
        // setup isComponent
        Qt3DSDMInstanceHandle slideOwner = m_NewMetaData->GetCanonicalInstanceForType(L"SlideOwner");
        retval.m_IsComponent = m_DataCore->IsInstanceOrDerivedFrom(theInstance, slideOwner);
        TCharStr type = m_NewMetaData->GetTypeForInstance(theInstance);
        qt3ds::foundation::ConvertUTF(type.c_str(), type.size(), retval.m_ClassName);
        vector<Qt3DSDMMetaDataPropertyHandle> theProperties;
        m_NewMetaData->GetMetaDataProperties(theInstance, theProperties);
        MetaPropertiesToAttList(theProperties, retval.m_Attributes);
        return retval;
    }

    TAttOrArgList GetSlideAttributes() override
    {
        TAttOrArgList retval;
        Qt3DSDMInstanceHandle slideOwner = m_NewMetaData->GetCanonicalInstanceForType(L"Slide");
        vector<Qt3DSDMMetaDataPropertyHandle> theProperties;
        m_NewMetaData->GetSpecificMetaDataProperties(slideOwner, theProperties);
        MetaPropertiesToAttList(theProperties, retval);
        return retval;
    }

    bool IsCustomHandler(const char *inId, const char *inHandlerName) override
    {
        return IsCustomHandler(Convert0(inId), Convert1(inHandlerName));
    }

    virtual void GetHandlerArgumentType(const wchar_t *inId, const wchar_t *inHandlerName,
                                        const wchar_t *inArgumentName,
                                        ERuntimeDataModelDataType &outType,
                                        ERuntimeAdditionalMetaDataType &outAdditionalType)
    {
        outType = ERuntimeDataModelDataTypeNone;
        outAdditionalType = ERuntimeAdditionalMetaDataTypeNone;

        Qt3DSDMInstanceHandle theInstance = GetInstanceById(inId);
        Qt3DSDMHandlerHandle theHandle =
            m_NewMetaData->FindHandlerByName(theInstance, inHandlerName);
        Option<SMetaDataHandlerArgumentInfo> theArgMetaData(
            m_NewMetaData->FindHandlerArgumentByName(theHandle, inArgumentName));
        if (theArgMetaData.hasValue()) {
            outType = Convert(theArgMetaData->GetDataType());
            outAdditionalType = Convert(theArgMetaData->GetAdditionalType());
        }

        // Check if the type is something that we support
        QT3DS_ASSERT(outType != ERuntimeDataModelDataTypeNone);
    }

    void GetHandlerArgumentType(const char *inId, const char *inHandlerName,
                                        const char *inArgumentName,
                                        ERuntimeDataModelDataType &outType,
                                        ERuntimeAdditionalMetaDataType &outAdditionalType) override
    {
        GetHandlerArgumentType(Convert0(inId), Convert1(inHandlerName), Convert2(inArgumentName),
                               outType, outAdditionalType);
    }

    std::shared_ptr<IDOMReader> ParseScriptFile(const wchar_t *inFullPathToDocument,
                                                  std::shared_ptr<qt3dsdm::IStringTable> inStringTable)
    {
        using namespace ScriptParser;
        std::shared_ptr<qt3dsdm::IStringTable> theStringTable(inStringTable);
        std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
        SImportXmlErrorHandler theXmlErrorHandler;
        const QString path = QString::fromWCharArray(inFullPathToDocument);
        std::shared_ptr<IDOMReader> theReaderPtr(
            SScriptParser::ParseScriptFile(theFactory, inStringTable,
                                           path,
                                           theXmlErrorHandler, m_InputStreamFactory));
        if (!theReaderPtr)
            qCCritical(INVALID_OPERATION) << "Failed to open script file: " << path;
        return theReaderPtr;
    }

    Qt3DSDMInstanceHandle CreateAndDeriveInstance(const wchar_t *inType, const wchar_t *inId)
    {
        // Make sure that the same id has not been created before
        TCharStr theId(inId);
        QT3DS_ASSERT(m_IdToHandleMap.find(theId) == m_IdToHandleMap.end());

        Qt3DSDMInstanceHandle theCanonicalType(m_NewMetaData->GetCanonicalInstanceForType(inType));
        if (theCanonicalType.Valid() == false) {
            QT3DS_ASSERT(false);
            return 0;
        }

        Qt3DSDMInstanceHandle theMaster = m_DataCore->CreateInstance();
        m_DataCore->DeriveInstance(theMaster, theCanonicalType);
        m_IdToHandleMap.insert(std::make_pair(theId, theMaster));
        return theMaster;
    }

    virtual bool LoadScriptFile(const wchar_t *inType, const wchar_t *inId, const wchar_t *inName,
                                const wchar_t *inSourcePath)
    {
        TCharStr theId(inId);
        if (m_IdToHandleMap.find(theId) != m_IdToHandleMap.end())
            return true;

        Qt3DSDMInstanceHandle theMaster = CreateAndDeriveInstance(inType, inId);
        if (!theMaster.Valid())
            return false;

        std::shared_ptr<IDOMReader> theScriptPtr =
            ParseScriptFile(inSourcePath, m_DataCore->GetStringTablePtr());
        if (theScriptPtr) {
            std::vector<SMetaDataLoadWarning> warnings;
            // Now the magic section
            m_NewMetaData->LoadInstance(*theScriptPtr, theMaster, inName, warnings);

            // Set the name
            Qt3DSDMPropertyHandle theProperty =
                m_DataCore->GetAggregateInstancePropertyByName(theMaster, L"name");
            m_DataCore->SetInstancePropertyValue(theMaster, theProperty,
                                                 std::make_shared<CDataStr>(inName));
            return true;
        }

        return false;
    }

    bool LoadScriptFile(const char *inType, const char *inId, const char *inName,
                        const char *inSourcePath) override
    {
        return LoadScriptFile(Convert0(inType), Convert1(inId), Convert2(inName),
                              Convert3(inSourcePath));
    }

    bool LoadEffectXMLFile(const char *inType, const char *inId, const char *inName,
                                   const char *inSourcePath) override
    {
        const wchar_t *theType(Convert0(inType));
        const wchar_t *theId(Convert1(inId));
        const wchar_t *theName(Convert2(inName));
        if (m_IdToHandleMap.find(theId) != m_IdToHandleMap.end())
            return true;

        Qt3DSDMInstanceHandle theMaster = CreateAndDeriveInstance(theType, theId);
        if (!theMaster.Valid())
            return false;
        NVScopedRefCounted<IRefCountedInputStream> theStream(
            m_InputStreamFactory.GetStreamForFile(inSourcePath));
        if (theStream) {
            std::vector<SMetaDataLoadWarning> warnings;
            bool success = m_NewMetaData->LoadEffectXMLFromSourcePath(
                inSourcePath, theMaster, theName, warnings, *theStream);
            (void)success;
            QT3DS_ASSERT(success);
            return success;
        }
        return false;
    }

    bool LoadMaterialXMLFile(const char *inType, const char *inId, const char *inName,
                                     const char *inSourcePath) override
    {
        const wchar_t *theType(Convert0(inType));
        const wchar_t *theId(Convert1(inId));
        const wchar_t *theName(Convert2(inName));
        (void)theName;
        if (m_IdToHandleMap.find(theId) != m_IdToHandleMap.end())
            return true;

        Qt3DSDMInstanceHandle theMaster = CreateAndDeriveInstance(theType, theId);
        if (!theMaster.Valid())
            return false;

        NVScopedRefCounted<IRefCountedInputStream> theStream(
            m_InputStreamFactory.GetStreamForFile(inSourcePath));
        if (theStream) {
            std::vector<SMetaDataLoadWarning> warnings;
            bool success = m_NewMetaData->LoadMaterialClassFromSourcePath(
                inSourcePath, theMaster, theId, warnings, *theStream);
            (void)success;
            QT3DS_ASSERT(success);
            return success;
        }
        return false;
    }

    struct PluginErrorHandler : public qt3dsdm::CXmlErrorHandler
    {
        void OnXmlError(const QString &, int, int) override {}
    };

    bool LoadPluginXMLFile(const char *inType, const char *inId, const char *inName,
                                   const char *inSourcePath) override
    {
        const wchar_t *theType(Convert0(inType));
        const wchar_t *theId(Convert1(inId));
        const wchar_t *theName(Convert2(inName));
        if (m_IdToHandleMap.find(theId) != m_IdToHandleMap.end())
            return true;

        Qt3DSDMInstanceHandle theMaster = CreateAndDeriveInstance(theType, theId);
        if (!theMaster.Valid())
            return false;

        std::shared_ptr<qt3dsdm::IDOMFactory> theFactory =
            qt3dsdm::IDOMFactory::CreateDOMFactory(m_StrTable);

        PluginErrorHandler dummyHandler;
        std::shared_ptr<qt3dsdm::IDOMReader> theReader = Q3DStudio::CRenderPluginParser::ParseFile(
            theFactory, m_StrTable, inSourcePath, dummyHandler, m_InputStreamFactory);
        if (theReader) {
            Q3DStudio::CRenderPluginParser::NavigateToMetadata(theReader);
            std::vector<SMetaDataLoadWarning> theWarnings;
            m_NewMetaData->LoadInstance(*theReader, theMaster, theName, theWarnings);
            Qt3DSDMPropertyHandle theProperty =
                m_DataCore->GetAggregateInstancePropertyByName(theMaster, L"name");
            m_DataCore->SetInstancePropertyValue(theMaster, theProperty,
                                                 std::make_shared<CDataStr>(theName));
            return true;
        }
        return false;
    }

    Option<qt3dsdm::SMetaDataEffect> GetEffectMetaDataBySourcePath(const char *inName) override
    {
        return m_NewMetaData->GetEffectBySourcePath(inName);
    }

    virtual Option<qt3dsdm::SMetaDataCustomMaterial>
    GetMaterialMetaDataBySourcePath(const char *inName) override
    {
        return m_NewMetaData->GetMaterialBySourcePath(inName);
    }

    virtual void GetInstanceProperties(const wchar_t *inType, const wchar_t *inId,
                                       eastl::vector<TRuntimeMetaDataStrType> &outProperties,
                                       bool inSearchParent)
    {
        // Get the instance handle given the type or id
        Qt3DSDMInstanceHandle theInstance =
            IsTrivial(inId) ? GetInstanceForType(inType) : GetInstanceById(inId);
        if (!theInstance.Valid()) {
            QT3DS_ASSERT(false);
            return;
        }

        vector<Qt3DSDMMetaDataPropertyHandle> theProperties;
        if (inSearchParent)
            // Get the meta data properties defined on this object or its derivation parents
            m_NewMetaData->GetMetaDataProperties(theInstance, theProperties);
        else
            // Get the meta data properties defined on *only* this object, don't search parents
            m_NewMetaData->GetSpecificMetaDataProperties(theInstance, theProperties);

        outProperties.clear();
        // Iterate each property and fill outProperties
        for (size_t propIdx = 0, propEnd = theProperties.size(); propIdx < propEnd; ++propIdx) {
            SMetaDataPropertyInfo theInfo(
                m_NewMetaData->GetMetaDataPropertyInfo(theProperties[propIdx]));
            outProperties.push_back(TRuntimeMetaDataStrType());
            qt3ds::foundation::ConvertUTF(theInfo.m_Name.c_str(), 0, outProperties.back());
        }
    }

    void GetInstanceProperties(const char *inType, const char *inId,
                                       eastl::vector<TRuntimeMetaDataStrType> &outProperties,
                                       bool inSearchParent) override
    {
        GetInstanceProperties(Convert0(inType), Convert2(inId), outProperties, inSearchParent);
    }

    std::shared_ptr<qt3dsdm::IStringTable> GetStringTable() override { return m_StrTable; }

    TStrTableStr Register(const char *inStr) override
    {
        return m_StrTable->GetRenderStringTable().RegisterStr(inStr);
    }

    //==============================================================================
    /**
     *	@Self release
     */
    void release() override { delete this; }

    void ClearPerProjectData() override
    {

        m_IdToHandleMap.clear();
        m_PropertyLookupHash.clear();
    }

}; // end of struct SRuntimeMetaDataImpl
}

IRuntimeMetaData &IRuntimeMetaData::Create(IInputStreamFactory &inInputStreamFactory)
{
    SRuntimeMetaDataImpl &retval = *new SRuntimeMetaDataImpl(inInputStreamFactory);
    retval.Load();
    return retval;
}
