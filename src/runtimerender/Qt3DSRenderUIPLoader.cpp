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
#ifdef QT3DS_RENDER_ENABLE_LOAD_UIP

#include "Qt3DSRenderUIPLoader.h"
#include "Qt3DSRenderPresentation.h"
#include "Qt3DSRenderNode.h"
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderLayer.h"
#include "Qt3DSRenderModel.h"
#include "Qt3DSRenderDefaultMaterial.h"
#include "Qt3DSRenderImage.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSRenderUIPSharedTranslation.h"
#include <vector>
#include <map>
#include <set>
#ifdef EA_PLATFORM_WINDOWS
#pragma warning(disable : 4201)
#endif
#include "Qt3DSDMXML.h"
#include "Qt3DSTypes.h"
#include "Qt3DSVector3.h"
#include "Qt3DSMetadata.h"
#include "Qt3DSDMWStrOps.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "EASTL/string.h"
#include "foundation/StrConvertUTF.h"
#include "Qt3DSRenderEffectSystem.h"
#include "StringTools.h"
#include "foundation/FileTools.h"
#include "Qt3DSRenderDynamicObjectSystemCommands.h"
#include "EASTL/map.h"
#include "Qt3DSRenderEffect.h"
#include "Qt3DSDMMetaDataTypes.h"
#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderPlugin.h"
#include "Qt3DSRenderPluginGraphObject.h"
#include "Qt3DSRenderPluginPropertyValue.h"
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderMaterialHelpers.h"
#include "Qt3DSRenderPath.h"
#include "Qt3DSRenderPathSubPath.h"
#include "Qt3DSRenderPathManager.h"
#include "q3dsvariantconfig_p.h"

using qt3ds::foundation::Option;
using qt3ds::foundation::Empty;
using qt3ds::QT3DSF32;
using qt3ds::QT3DSVec3;
using qt3ds::foundation::nvvector;
using qt3ds::QT3DSU32;
using qt3ds::render::RenderLightTypes;
using qt3ds::render::DefaultMaterialLighting;
using qt3ds::render::ImageMappingModes;
using qt3ds::render::DefaultMaterialBlendMode;
using qt3ds::render::NVRenderTextureCoordOp;
using qt3ds::foundation::IStringTable;
using qt3ds::NVFoundationBase;
using namespace qt3ds;
using namespace qt3ds::foundation;
using qt3ds::render::TIdObjectMap;
using qt3ds::render::IBufferManager;
using qt3ds::render::IEffectSystem;
using qt3ds::render::SPresentation;
using qt3ds::render::SScene;
using qt3ds::render::SLayer;
using qt3ds::render::SNode;
using qt3ds::render::SLight;
using qt3ds::render::SCamera;
using qt3ds::render::SModel;
using qt3ds::render::SText;
using qt3ds::render::SDefaultMaterial;
using qt3ds::render::SImage;
using qt3ds::render::SGraphObject;
using qt3ds::render::SDynamicObject;
using qt3ds::render::SEffect;
using qt3ds::render::SCustomMaterial;
using qt3ds::render::GraphObjectTypes;
using qt3ds::render::NodeFlags;
using qt3ds::foundation::CRegisteredString;
using qt3ds::foundation::Qt3DSString;
using qt3ds::foundation::CFileTools;
using qt3ds::render::SReferencedMaterial;
using qt3ds::render::IUIPReferenceResolver;
using qt3ds::render::SPath;
using qt3ds::render::SPathSubPath;
using qt3ds::render::SLightmaps;

namespace qt3dsdm {
template <>
struct WStrOps<SFloat2>
{
    void StrTo(const char8_t *buffer, SFloat2 &item, nvvector<char8_t> &ioTempBuf)
    {
        QT3DSU32 len = (QT3DSU32)strlen(buffer);
        ioTempBuf.resize(len + 1);
        memCopy(ioTempBuf.data(), buffer, (len + 1) * sizeof(char8_t));
        MemoryBuffer<RawAllocator> unused;
        qt3dsdm::IStringTable *theTable(NULL);
        WCharTReader reader(ioTempBuf.begin(), unused, *theTable);
        reader.ReadRef(NVDataRef<QT3DSF32>(item.m_Floats, 2));
    }
};

template <>
struct WStrOps<SFloat3>
{
    void StrTo(const char8_t *buffer, SFloat3 &item, nvvector<char8_t> &ioTempBuf)
    {
        QT3DSU32 len = (QT3DSU32)strlen(buffer);
        ioTempBuf.resize(len + 1);
        memCopy(ioTempBuf.data(), buffer, (len + 1) * sizeof(char8_t));
        MemoryBuffer<RawAllocator> unused;
        qt3dsdm::IStringTable *theTable(NULL);
        WCharTReader reader(ioTempBuf.begin(), unused, *theTable);
        reader.ReadRef(NVDataRef<QT3DSF32>(item.m_Floats, 3));
    }
};

template <>
struct WStrOps<SFloat4>
{
    void StrTo(const char8_t *buffer, SFloat4 &item, nvvector<char8_t> &ioTempBuf)
    {
        QT3DSU32 len = (QT3DSU32)strlen(buffer);
        ioTempBuf.resize(len + 1);
        memCopy(ioTempBuf.data(), buffer, (len + 1) * sizeof(char8_t));
        MemoryBuffer<RawAllocator> unused;
        qt3dsdm::IStringTable *theTable(NULL);
        WCharTReader reader(ioTempBuf.begin(), unused, *theTable);
        reader.ReadRef(NVDataRef<QT3DSF32>(item.m_Floats, 4));
    }
};
}

namespace {

typedef eastl::basic_string<char8_t> TStrType;
struct IPropertyParser
{
    virtual ~IPropertyParser() {}
    virtual Option<TStrType> ParseStr(const char8_t *inName) = 0;
    virtual Option<QT3DSF32> ParseFloat(const char8_t *inName) = 0;
    virtual Option<QT3DSVec2> ParseVec2(const char8_t *inName) = 0;
    virtual Option<QT3DSVec3> ParseVec3(const char8_t *inName) = 0;
    virtual Option<QT3DSVec4> ParseVec4(const char8_t *inName) = 0;
    virtual Option<bool> ParseBool(const char8_t *inName) = 0;
    virtual Option<QT3DSU32> ParseU32(const char8_t *inName) = 0;
    virtual Option<QT3DSI32> ParseI32(const char8_t *inName) = 0;
    virtual Option<SGraphObject *> ParseGraphObject(const char8_t *inName) = 0;
    virtual Option<SNode *> ParseNode(const char8_t *inName) = 0;
};
struct SMetaPropertyParser : public IPropertyParser
{
    Q3DStudio::IRuntimeMetaData &m_MetaData;
    TStrType m_TempStr;
    qt3ds::foundation::CRegisteredString m_Type;
    qt3ds::foundation::CRegisteredString m_ClassId;

    SMetaPropertyParser(const char8_t *inType, const char8_t *inClass,
                        Q3DStudio::IRuntimeMetaData &inMeta)
        : m_MetaData(inMeta)
        , m_Type(inMeta.GetStringTable()->GetRenderStringTable().RegisterStr(inType))
        , m_ClassId(inMeta.GetStringTable()->GetRenderStringTable().RegisterStr(inClass))
    {
    }

    qt3ds::foundation::CRegisteredString Register(const char8_t *inName)
    {
        return m_MetaData.GetStringTable()->GetRenderStringTable().RegisterStr(inName);
    }

    Option<TStrType> ParseStr(const char8_t *inName) override
    {
        qt3ds::foundation::CRegisteredString theName(Register(inName));
        Q3DStudio::ERuntimeDataModelDataType theType(
            m_MetaData.GetPropertyType(m_Type, theName, m_ClassId));
        if (theType != Q3DStudio::ERuntimeDataModelDataTypeObjectRef
            && theType != Q3DStudio::ERuntimeDataModelDataTypeLong4) {
            return m_MetaData.GetPropertyValueString(m_Type, theName, m_ClassId);
        }
        return Empty();
    }
    Option<QT3DSF32> ParseFloat(const char8_t *inName) override
    {
        return m_MetaData.GetPropertyValueFloat(m_Type, Register(inName), m_ClassId);
    }
    Option<QT3DSVec2> ParseVec2(const char8_t *inName) override
    {
        Option<qt3ds::QT3DSVec2> theProperty =
            m_MetaData.GetPropertyValueVector2(m_Type, Register(inName), m_ClassId);
        if (theProperty.hasValue())
            return *theProperty;

        return Empty();
    }
    Option<QT3DSVec3> ParseVec3(const char8_t *inName) override
    {
        Option<qt3ds::QT3DSVec3> theProperty =
            m_MetaData.GetPropertyValueVector3(m_Type, Register(inName), m_ClassId);
        if (theProperty.hasValue()) {
            return *theProperty;
        }
        return Empty();
    }
    Option<QT3DSVec4> ParseVec4(const char8_t *inName) override
    {
        Option<qt3ds::QT3DSVec4> theProperty =
            m_MetaData.GetPropertyValueVector4(m_Type, Register(inName), m_ClassId);
        if (theProperty.hasValue()) {
            return *theProperty;
        }
        return Empty();
    }
    Option<bool> ParseBool(const char8_t *inName) override
    {
        return m_MetaData.GetPropertyValueBool(m_Type, Register(inName), m_ClassId);
    }

    Option<QT3DSU32> ParseU32(const char8_t *inName) override
    {
        Option<QT3DSI32> retval = m_MetaData.GetPropertyValueLong(m_Type, Register(inName), m_ClassId);
        if (retval.hasValue())
            return (QT3DSU32)retval.getValue();
        return Empty();
    }

    Option<QT3DSI32> ParseI32(const char8_t *inName) override
    {
        Option<QT3DSI32> retval = m_MetaData.GetPropertyValueLong(m_Type, Register(inName), m_ClassId);
        if (retval.hasValue())
            return (QT3DSI32)retval.getValue();
        return Empty();
    }

    Option<SGraphObject *> ParseGraphObject(const char8_t *) override { return Empty(); }
    Option<SNode *> ParseNode(const char8_t *) override { return Empty(); }
};

class IDOMReferenceResolver
{
protected:
    virtual ~IDOMReferenceResolver() {}
public:
    virtual SGraphObject *ResolveReference(SGraphObject &inRootObject, const char *path) = 0;
};

struct SDomReaderPropertyParser : public IPropertyParser
{
    qt3dsdm::IDOMReader &m_Reader;
    nvvector<char8_t> &m_TempBuf;
    IDOMReferenceResolver &m_Resolver;
    SGraphObject &m_Object;

    SDomReaderPropertyParser(qt3dsdm::IDOMReader &reader, nvvector<char8_t> &inTempBuf,
                             IDOMReferenceResolver &inResolver, SGraphObject &inObject)
        : m_Reader(reader)
        , m_TempBuf(inTempBuf)
        , m_Resolver(inResolver)
        , m_Object(inObject)
    {
    }
    Option<TStrType> ParseStr(const char8_t *inName) override
    {
        const char8_t *retval;
        if (m_Reader.Att(inName, retval))
            return TStrType(retval);
        return Empty();
    }
    Option<QT3DSF32> ParseFloat(const char8_t *inName) override
    {
        QT3DSF32 retval;
        if (m_Reader.Att(inName, retval))
            return retval;
        return Empty();
    }
    Option<QT3DSVec2> ParseVec2(const char8_t *inName) override
    {
        qt3dsdm::SFloat2 retval;
        const char8_t *tempData;
        if (m_Reader.UnregisteredAtt(inName, tempData)) {
            qt3dsdm::WStrOps<qt3dsdm::SFloat2>().StrTo(tempData, retval, m_TempBuf);
            return QT3DSVec2(retval.m_Floats[0], retval.m_Floats[1]);
        }
        return Empty();
    }
    Option<QT3DSVec3> ParseVec3(const char8_t *inName) override
    {
        qt3dsdm::SFloat3 retval;
        const char8_t *tempData;
        if (m_Reader.UnregisteredAtt(inName, tempData)) {
            qt3dsdm::WStrOps<qt3dsdm::SFloat3>().StrTo(tempData, retval, m_TempBuf);
            return QT3DSVec3(retval.m_Floats[0], retval.m_Floats[1], retval.m_Floats[2]);
        }
        return Empty();
    }
    Option<QT3DSVec4> ParseVec4(const char8_t *inName) override
    {
        qt3dsdm::SFloat4 retval;
        const char8_t *tempData;
        if (m_Reader.UnregisteredAtt(inName, tempData)) {
            qt3dsdm::WStrOps<qt3dsdm::SFloat4>().StrTo(tempData, retval, m_TempBuf);
            return QT3DSVec4(retval.m_Floats[0], retval.m_Floats[1], retval.m_Floats[2],
                             retval.m_Floats[3]);
        }
        return Empty();
    }
    Option<bool> ParseBool(const char8_t *inName) override
    {
        bool retval;
        if (m_Reader.Att(inName, retval))
            return retval;
        return Empty();
    }

    Option<QT3DSU32> ParseU32(const char8_t *inName) override
    {
        QT3DSU32 retval;
        if (m_Reader.Att(inName, retval))
            return retval;
        return Empty();
    }

    Option<QT3DSI32> ParseI32(const char8_t *inName) override
    {
        QT3DSI32 retval;
        if (m_Reader.Att(inName, retval))
            return retval;
        return Empty();
    }

    Option<SGraphObject *> ParseGraphObject(const char8_t *inName) override
    {
        const char *temp;
        if (m_Reader.UnregisteredAtt(inName, temp)) {
            // Now we need to figure out if this is an element reference or if it is a relative path
            // from the current element.
            SGraphObject *retval = m_Resolver.ResolveReference(m_Object, temp);
            if (retval)
                return retval;
        }
        return Empty();
    }

    Option<SNode *> ParseNode(const char8_t *inName) override
    {
        Option<SGraphObject *> obj = ParseGraphObject(inName);
        if (obj.hasValue()) {
            if (GraphObjectTypes::IsNodeType((*obj)->m_Type))
                return static_cast<SNode *>((*obj));
        }
        return Empty();
    }
};

template <typename TDataType>
struct SParserHelper
{
};
template <>
struct SParserHelper<TStrType>
{
    static Option<TStrType> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseStr(inName);
    }
};
template <>
struct SParserHelper<QT3DSF32>
{
    static Option<QT3DSF32> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseFloat(inName);
    }
};
template <>
struct SParserHelper<QT3DSVec2>
{
    static Option<QT3DSVec2> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseVec2(inName);
    }
};
template <>
struct SParserHelper<QT3DSVec3>
{
    static Option<QT3DSVec3> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseVec3(inName);
    }
};
template <>
struct SParserHelper<QT3DSVec4>
{
    static Option<QT3DSVec4> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseVec4(inName);
    }
};
template <>
struct SParserHelper<bool>
{
    static Option<bool> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseBool(inName);
    }
};
template <>
struct SParserHelper<QT3DSU32>
{
    static Option<QT3DSU32> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseU32(inName);
    }
};
template <>
struct SParserHelper<QT3DSI32>
{
    static Option<QT3DSI32> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseI32(inName);
    }
};
template <>
struct SParserHelper<SGraphObject *>
{
    static Option<SGraphObject *> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseGraphObject(inName);
    }
};
template <>
struct SParserHelper<SNode *>
{
    static Option<SNode *> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseNode(inName);
    }
};

struct SPathAndAnchorIndex
{
    SPathSubPath *m_Segment;
    QT3DSU32 m_AnchorIndex;
    SPathAndAnchorIndex(SPathSubPath *inSegment, QT3DSU32 inAnchorIndex)
        : m_Segment(inSegment)
        , m_AnchorIndex(inAnchorIndex)
    {
    }
    SPathAndAnchorIndex()
        : m_Segment(NULL)
        , m_AnchorIndex(0)
    {
    }
};

struct SRenderUIPLoader : public IDOMReferenceResolver
{
    typedef qt3dsdm::IDOMReader::Scope TScope;
    typedef eastl::map<CRegisteredString, eastl::string> TIdStringMap;
    typedef eastl::hash_map<CRegisteredString, SPathAndAnchorIndex> TIdPathAnchorIndexMap;
    qt3dsdm::IDOMReader &m_Reader;
    Q3DStudio::IRuntimeMetaData &m_MetaData;
    IStringTable &m_StrTable;
    NVFoundationBase &m_Foundation;
    NVAllocatorCallback &m_PresentationAllocator;
    qt3ds::render::TIdObjectMap &m_ObjectMap;
    IBufferManager &m_BufferManager;
    SPresentation *m_Presentation;
    nvvector<char8_t> m_TempBuf;
    TStrType m_TempParseString;
    IEffectSystem &m_EffectSystem;
    const char8_t *m_PresentationDir;
    Qt3DSString m_PathString;
    qt3ds::render::IRenderPluginManager &m_RenderPluginManager;
    qt3ds::render::ICustomMaterialSystem &m_CustomMaterialSystem;
    qt3ds::render::IDynamicObjectSystem &m_DynamicObjectSystem;
    qt3ds::render::IPathManager &m_PathManager;
    TIdStringMap m_RenderPluginSourcePaths;
    IUIPReferenceResolver *m_ReferenceResolver;
    MemoryBuffer<RawAllocator> m_TempBuffer;
    MemoryBuffer<RawAllocator> m_ValueBuffer;
    TIdPathAnchorIndexMap m_AnchorIdToPathAndAnchorIndexMap;
    const Q3DSVariantConfig &m_variantConfig;

    SRenderUIPLoader(qt3dsdm::IDOMReader &inReader, const char8_t *inFullPathToPresentationFile,
                     Q3DStudio::IRuntimeMetaData &inMetaData, IStringTable &inStrTable
                     // Allocator for datastructures we need to parse the file.
                     ,
                     NVFoundationBase &inFoundation
                     // Allocator used for the presentation objects themselves
                     ,
                     NVAllocatorCallback &inPresentationAllocator
                     // Map of string ids to objects
                     ,
                     TIdObjectMap &ioObjectMap, IBufferManager &inBufferManager,
                     IEffectSystem &inEffectSystem, const char8_t *inPresentationDir,
                     qt3ds::render::IRenderPluginManager &inRPM,
                     qt3ds::render::ICustomMaterialSystem &inCMS,
                     qt3ds::render::IDynamicObjectSystem &inDynamicSystem,
                     qt3ds::render::IPathManager &inPathManager, IUIPReferenceResolver *inResolver,
                     const Q3DSVariantConfig &variantConfig)
        : m_Reader(inReader)
        , m_MetaData(inMetaData)
        , m_StrTable(inStrTable)
        , m_Foundation(inFoundation)
        , m_PresentationAllocator(inPresentationAllocator)
        , m_ObjectMap(ioObjectMap)
        , m_BufferManager(inBufferManager)
        , m_Presentation(QT3DS_NEW(inPresentationAllocator, SPresentation)())
        , m_TempBuf(inFoundation.getAllocator(), "SRenderUIPLoader::m_TempBuf")
        , m_EffectSystem(inEffectSystem)
        , m_PresentationDir(inPresentationDir)
        , m_RenderPluginManager(inRPM)
        , m_CustomMaterialSystem(inCMS)
        , m_DynamicObjectSystem(inDynamicSystem)
        , m_PathManager(inPathManager)
        , m_ReferenceResolver(inResolver)
        , m_variantConfig(variantConfig)
    {
        std::string presentationFile = inFullPathToPresentationFile;
        std::string::size_type pos = presentationFile.find_last_of("\\/");
        if (pos != std::string::npos) {
            std::string path = presentationFile.substr(0, pos);
            m_Presentation->m_PresentationDirectory = inStrTable.RegisterStr(path.c_str());
        }
    }

    SGraphObject *ResolveReference(SGraphObject &inRoot, const char *path) override
    {
        if (m_ReferenceResolver) {
            CRegisteredString resolvedReference =
                m_ReferenceResolver->ResolveReference(inRoot.m_Id, path);
            if (resolvedReference.IsValid()) {
                qt3ds::render::TIdObjectMap::iterator iter = m_ObjectMap.find(resolvedReference);
                if (iter != m_ObjectMap.end())
                    return iter->second;
            }
        }
        return NULL;
    }

    static bool IsNode(GraphObjectTypes::Enum inType)
    {
        return GraphObjectTypes::IsNodeType(inType);
    }
    template <typename TDataType>
    bool ParseProperty(IPropertyParser &inParser, const char8_t *inName, TDataType &outData)
    {
        Option<TDataType> theValue(SParserHelper<TDataType>::Parse(inName, inParser));
        if (theValue.hasValue()) {
            outData = theValue;
            return true;
        }
        return false;
    }
    bool ParseOpacityProperty(IPropertyParser &inParser, const char8_t *inName, QT3DSF32 &outOpacity)
    {
        if (ParseProperty(inParser, inName, outOpacity)) {
            outOpacity /= 100.0f;
            return true;
        }
        return false;
    }

    bool ParseRadianProperty(IPropertyParser &inParser, const char8_t *inName, QT3DSVec3 &ioRotation)
    {
        if (ParseProperty(inParser, inName, ioRotation)) {
            TORAD(ioRotation.x);
            TORAD(ioRotation.y);
            TORAD(ioRotation.z);
            return true;
        }
        return false;
    }
    bool ParseRadianProperty(IPropertyParser &inParser, const char8_t *inName, QT3DSF32 &ioRotation)
    {
        if (ParseProperty(inParser, inName, ioRotation)) {
            TORAD(ioRotation);
            return true;
        }
        return false;
    }

    void ParseRotationOrder(IPropertyParser &inParser, const char8_t *inName,
                            QT3DSU32 &ioRotationOrder)
    {
        if (ParseProperty(inParser, inName, m_TempParseString))
            ioRotationOrder = qt3ds::render::MapRotationOrder(m_TempParseString.c_str());
    }
    void ParseOrientation(IPropertyParser &inParser, const char8_t *inName, NodeFlags &ioFlags)
    {
        if (ParseProperty(inParser, inName, m_TempParseString)) {
            if (m_TempParseString == "Left Handed")
                ioFlags.SetLeftHanded(true);
            else
                ioFlags.SetLeftHanded(false);
        }
    }
    void ParseOrthographicProperty(IPropertyParser &inParser, const char8_t *inName,
                                   NodeFlags &ioFlags)
    {
        bool isOrthographic;
        if (ParseProperty(inParser, inName, isOrthographic))
            ioFlags.SetOrthographic(isOrthographic);
    }
    template <typename TEnumType>
    static bool ConvertEnumFromStr(const char8_t *inStr, TEnumType &ioEnum)
    {
        qt3ds::render::SEnumNameMap *theMap = qt3ds::render::SEnumParseMap<TEnumType>::GetMap();
        for (qt3ds::render::SEnumNameMap *item = theMap; item->m_Name; ++item) {
            // hack to match advanced overlay types, whose name start with a '*'
            const char8_t *p = inStr;
            if (*p == '*')
                ++p;
            if (qt3dsdm::AreEqual(p, item->m_Name)) {
                ioEnum = static_cast<TEnumType>(item->m_Enum);
                return true;
            }
        }
        return false;
    }

    template <typename TEnumType>
    void ParseEnumProperty(IPropertyParser &inParser, const char8_t *inName, TEnumType &ioEnum)
    {
        if (ParseProperty(inParser, inName, m_TempParseString)) {
            ConvertEnumFromStr(m_TempParseString.c_str(), ioEnum);
        }
    }
    void ParseAndResolveSourcePath(IPropertyParser &inParser, const char8_t *inName,
                                   CRegisteredString &ioString)
    {
        if (ParseProperty(inParser, inName, m_TempParseString))
            ioString = m_StrTable.RegisterStr(m_TempParseString.c_str());
    }
    void ParseProperty(IPropertyParser &inParser, const char8_t *inName, SImage *&ioImage)
    {
        if (ParseProperty(inParser, inName, m_TempParseString)) {
            TIdObjectMap::iterator theIter =
                m_ObjectMap.find(m_StrTable.RegisterStr(m_TempParseString.c_str() + 1));
            if (theIter != m_ObjectMap.end()
                && theIter->second->m_Type == GraphObjectTypes::Image) {
                ioImage = static_cast<SImage *>(theIter->second);
            } else {
                QT3DS_ASSERT(false);
            }
        }
    }
    void ParseProperty(IPropertyParser &inParser, const char8_t *inName, CRegisteredString &ioStr)
    {
        if (ParseProperty(inParser, inName, m_TempParseString))
            ioStr = m_StrTable.RegisterStr(m_TempParseString.c_str());
    }

    void ParseNodeFlagsProperty(IPropertyParser &inParser, const char8_t *inName,
                                qt3ds::render::NodeFlags &ioFlags,
                                qt3ds::render::NodeFlagValues::Enum prop)
    {
        bool temp;
        if (ParseProperty(inParser, inName, temp))
            ioFlags.ClearOrSet(temp, prop);
    }

    void ParseNodeFlagsInverseProperty(IPropertyParser &inParser, const char8_t *inName,
                                       qt3ds::render::NodeFlags &ioFlags,
                                       qt3ds::render::NodeFlagValues::Enum prop)
    {
        bool temp;
        if (ParseProperty(inParser, inName, temp))
            ioFlags.ClearOrSet(!temp, prop);
    }

// Create a mapping from UICRenderPropertyNames to the string in the UIP file.
#define Scene_ClearColor "backgroundcolor"
#define Scene_UseClearColor "bgcolorenable"
#define Node_Rotation "rotation"
#define Node_Position "position"
#define Node_Scale "scale"
#define Node_Pivot "pivot"
#define Node_LocalOpacity "opacity"
#define Node_RotationOrder "rotationorder"
#define Node_LeftHanded "orientation"
#define Layer_Variants "variants"
#define Layer_TemporalAAEnabled "temporalaa"
#define Layer_LayerEnableDepthTest "disabledepthtest"
#define Layer_LayerEnableDepthPrePass "disabledepthprepass"
#define Layer_ClearColor "backgroundcolor"
#define Layer_Background "background"
#define Layer_BlendType "blendtype"
#define Layer_Size "size"
#define Layer_Location "location"
#define Layer_TexturePath "sourcepath"
#define Layer_HorizontalFieldValues "horzfields"
#define Layer_Left "left"
#define Layer_LeftUnits "leftunits"
#define Layer_Width "width"
#define Layer_WidthUnits "widthunits"
#define Layer_Right "right"
#define Layer_RightUnits "rightunits"
#define Layer_VerticalFieldValues "vertfields"
#define Layer_Top "top"
#define Layer_TopUnits "topunits"
#define Layer_Height "height"
#define Layer_HeightUnits "heightunits"
#define Layer_Bottom "bottom"
#define Layer_BottomUnits "bottomunits"
#define Layer_AoStrength "aostrength"
#define Layer_AoDistance "aodistance"
#define Layer_AoSoftness "aosoftness"
#define Layer_AoBias "aobias"
#define Layer_AoSamplerate "aosamplerate"
#define Layer_AoDither "aodither"
#define Layer_ShadowStrength "shadowstrength"
#define Layer_ShadowDist "shadowdist"
#define Layer_ShadowSoftness "shadowsoftness"
#define Layer_ShadowBias "shadowbias"
#define Layer_LightProbe "lightprobe"
#define Layer_ProbeBright "probebright"
#define Layer_FastIbl "fastibl"
#define Layer_ProbeHorizon "probehorizon"
#define Layer_ProbeFov "probefov"
#define Layer_LightProbe2 "lightprobe2"
#define Layer_Probe2Fade "probe2fade"
#define Layer_Probe2Window "probe2window"
#define Layer_Probe2Pos "probe2pos"
#define Camera_ClipNear "clipnear"
#define Camera_ClipFar "clipfar"
#define Camera_FOV "fov"
#define Camera_FOVHorizontal "fovhorizontal"
#define Camera_Orthographic "orthographic"
#define Camera_EnableFrustumCulling "enablefrustumculling"
#define Camera_ScaleMode "scalemode"
#define Camera_ScaleAnchor "scaleanchor"
#define Light_LightType "lighttype"
#define Light_DiffuseColor "lightdiffuse"
#define Light_SpecularColor "lightspecular"
#define Light_AmbientColor "lightambient"
#define Light_Brightness "brightness"
#define Light_LinearFade "linearfade"
#define Light_ExponentialFade "expfade"
#define Light_AreaWidth "areawidth"
#define Light_AreaHeight "areaheight"
#define Light_CastShadow "castshadow"
#define Light_ShadowBias "shdwbias"
#define Light_ShadowFactor "shdwfactor"
#define Light_ShadowMapRes "shdwmapres"
#define Light_ShadowMapFar "shdwmapfar"
#define Light_ShadowMapFov "shdwmapfov"
#define Light_ShadowFilter "shdwfilter"
#define Model_MeshPath "sourcepath"
#define Model_ShadowCaster "shadowcaster"
#define Model_TessellationMode "tessellation"
#define Model_EdgeTess "edgetess"
#define Model_InnerTess "innertess"
#define Lightmaps_LightmapIndirect "lightmapindirect"
#define Lightmaps_LightmapRadiosity "lightmapradiosity"
#define Lightmaps_LightmapShadow "lightmapshadow"
#define Material_Lighting "shaderlighting"
#define Material_BlendMode "blendmode"
#define MaterialBase_IblProbe "iblprobe"
#define Material_DiffuseColor "diffuse"
#define Material_DiffuseMaps_0 "diffusemap"
#define Material_DiffuseMaps_1 "diffusemap2"
#define Material_DiffuseMaps_2 "diffusemap3"
#define Material_EmissivePower "emissivepower"
#define Material_EmissiveColor "emissivecolor"
#define Material_EmissiveMap "emissivemap"
#define Material_EmissiveMap2 "emissivemap2"
#define Material_SpecularReflection "specularreflection"
#define Material_SpecularMap "specularmap"
#define Material_SpecularModel "specularmodel"
#define Material_SpecularTint "speculartint"
#define Material_IOR "ior"
#define Material_FresnelPower "fresnelPower"
#define Material_SpecularAmount "specularamount"
#define Material_SpecularRoughness "specularroughness"
#define Material_RoughnessMap "roughnessmap"
#define Material_Opacity "opacity"
#define Material_OpacityMap "opacitymap"
#define Material_BumpMap "bumpmap"
#define Material_BumpAmount "bumpamount"
#define Material_NormalMap "normalmap"
#define Material_DisplacementMap "displacementmap"
#define Material_DisplaceAmount "displaceamount"
#define Material_TranslucencyMap "translucencymap"
#define Material_TranslucentFalloff "translucentfalloff"
#define Material_DiffuseLightWrap "diffuselightwrap"
#define Material_ReferencedMaterial "referencedmaterial"
#define Material_VertexColors "vertexcolors"
#define Material_TransparencyMode "transparencymode"
#define Image_ImagePath "sourcepath"
#define Image_OffscreenRendererId "subpresentation"
#define Image_Scale_X "scaleu"
#define Image_Scale_Y "scalev"
#define Image_Pivot_X "pivotu"
#define Image_Pivot_Y "pivotv"
#define Image_Rotation "rotationuv"
#define Image_Position_X "positionu"
#define Image_Position_Y "positionv"
#define Image_MappingMode "mappingmode"
#define Image_HorizontalTilingMode "tilingmodehorz"
#define Image_VerticalTilingMode "tilingmodevert"
#define Text_Text "textstring"
#define Text_Font "font"
#define Text_FontSize "size"
#define Text_HorizontalAlignment "horzalign"
#define Text_VerticalAlignment "vertalign"
#define Text_Leading "leading"
#define Text_Tracking "tracking"
#define Text_DropShadow "dropshadow"
#define Text_DropShadowStrength "dropshadowstrength"
#define Text_DropShadowOffsetX "dropshadowoffsetx"
#define Text_DropShadowOffsetY "dropshadowoffsety"
#define Text_WordWrap "wordwrap"
#define Text_BoundingBox "boundingbox"
#define Text_Elide "elide"
#define Text_TextColor "textcolor"
#define Text_BackColor "backcolor"
#define Text_EnableAcceleratedFont "enableacceleratedfont"
#define Layer_ProgressiveAAMode "progressiveaa"
#define Layer_MultisampleAAMode "multisampleaa"
#define Light_Scope "scope"
#define Path_PathType "pathtype"
#define Path_PaintStyle "paintstyle"
#define Path_Width "width"
#define Path_Opacity "opacity"
#define Path_LinearError "linearerror"
#define Path_EdgeTessAmount "edgetessamount"
#define Path_InnerTessAmount "innertessamount"
#define Path_BeginCapping "begincap"
#define Path_BeginCapOffset "begincapoffset"
#define Path_BeginCapOpacity "begincapopacity"
#define Path_BeginCapWidth "begincapwidth"
#define Path_EndCapping "endcap"
#define Path_EndCapOffset "endcapoffset"
#define Path_EndCapOpacity "endcapopacity"
#define Path_EndCapWidth "endcapwidth"
#define Path_PathBuffer "sourcepath"
#define SubPath_Closed "closed"

// Fill in implementations for the actual parse tables.
#define HANDLE_QT3DS_RENDER_PROPERTY(type, name, dirty)                                              \
    ParseProperty(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_RENDER_REAL_VEC2_PROPERTY(type, name, dirty)                                    \
    ParseProperty(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_RENDER_VEC3_PROPERTY(type, name, dirty)                                         \
    ParseProperty(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_RENDER_COLOR_PROPERTY(type, name, dirty)                                        \
    ParseProperty(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_RENDER_RADIAN_PROPERTY(type, name, dirty)                                       \
    ParseRadianProperty(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_RENDER_VEC3_RADIAN_PROPERTY(type, name, dirty)                                  \
    ParseRadianProperty(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_RENDER_OPACITY_PROPERTY(type, name, dirty)                                      \
    ParseOpacityProperty(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_ROTATION_ORDER_PROPERTY(type, name, dirty)                                      \
    ParseRotationOrder(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_NODE_ORIENTATION_PROPERTY(type, name, dirty)                                    \
    ParseOrientation(inParser, type##_##name, inItem.m_Flags);
#define HANDLE_QT3DS_RENDER_DEPTH_TEST_PROPERTY(type, name, dirty)                                   \
    if (ParseProperty(inParser, type##_##name, inItem.m_##name))                                   \
        inItem.m_##name = !inItem.m_##name;
#define HANDLE_QT3DS_NODE_FLAGS_PROPERTY(type, name, dirty)                                          \
    ParseNodeFlagsProperty(inParser, type##_##name, inItem.m_Flags,                                \
                           qt3ds::render::NodeFlagValues::name);
#define HANDLE_QT3DS_NODE_FLAGS_INVERSE_PROPERTY(type, name, dirty)                                  \
    ParseNodeFlagsInverseProperty(inParser, type##_##name, inItem.m_Flags,                         \
                                  qt3ds::render::NodeFlagValues::name);
#define HANDLE_QT3DS_RENDER_ENUM_PROPERTY(type, name, dirty)                                         \
    ParseEnumProperty(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_RENDER_SOURCEPATH_PROPERTY(type, name, dirty)                                   \
    ParseAndResolveSourcePath(inParser, type##_##name, inItem.m_##name);
#define HANDLE_QT3DS_RENDER_ARRAY_PROPERTY(type, name, index, dirty)                                 \
    ParseProperty(inParser, type##_##name##_##index, inItem.m_##name[index]);
#define HANDLE_QT3DS_RENDER_VEC2_PROPERTY(type, name, dirty)                                         \
    ParseProperty(inParser, type##_##name##_##X, inItem.m_##name.x);                               \
    ParseProperty(inParser, type##_##name##_##Y, inItem.m_##name.y);
#define HANDLE_QT3DS_RENDER_COLOR_VEC3_PROPERTY(                                                     \
    type, name, dirty) // noop by intention already handled by HANDLE_QT3DS_RENDER_COLOR_PROPERTY
#define HANDLE_QT3DS_RENDER_TRANSFORM_VEC3_PROPERTY(                                                 \
    type, name, dirty) // noop by intention already handled by HANDLE_QT3DS_RENDER_VEC3_PROPERTY

    // Call the correct parser functions.
    void ParseProperties(SScene &inItem, IPropertyParser &inParser)
    {
        ITERATE_QT3DS_RENDER_SCENE_PROPERTIES
    }
    void ParseProperties(SNode &inItem, IPropertyParser &inParser)
    {
        bool eyeball;
        if (ParseProperty(inParser, "eyeball", eyeball))
            inItem.m_Flags.SetActive(eyeball);
        ITERATE_QT3DS_RENDER_NODE_PROPERTIES
        ParseProperty(inParser, "boneid", inItem.m_SkeletonId);
        bool ignoreParent = false;
        if (ParseProperty(inParser, "ignoresparent", ignoreParent))
            inItem.m_Flags.SetIgnoreParentTransform(ignoreParent);
    }
    void ParseProperties(SLayer &inItem, IPropertyParser &inParser)
    {
        ParseProperties(static_cast<SNode &>(inItem), inParser);
        ITERATE_QT3DS_RENDER_LAYER_PROPERTIES
        ParseProperty(inParser, "aosamplerate", inItem.m_AoSamplerate);
    }
    void ParseProperties(SCamera &inItem, IPropertyParser &inParser)
    {
        ParseProperties(static_cast<SNode &>(inItem), inParser);
        ITERATE_QT3DS_RENDER_CAMERA_PROPERTIES
    }
    void ParseProperties(SLight &inItem, IPropertyParser &inParser)
    {
        ParseProperties(static_cast<SNode &>(inItem), inParser);
        ITERATE_QT3DS_RENDER_LIGHT_PROPERTIES
        ParseProperty(inParser, "shdwmapres", inItem.m_ShadowMapRes);
    }
    void ParseProperties(SModel &inItem, IPropertyParser &inParser)
    {
        ParseProperties(static_cast<SNode &>(inItem), inParser);
        ITERATE_QT3DS_RENDER_MODEL_PROPERTIES
        ParseProperty(inParser, "poseroot", inItem.m_SkeletonRoot);
    }

    void ParseProperties(SText &inItem, IPropertyParser &inParser)
    {
        ParseProperties(static_cast<SNode &>(inItem), inParser);
        ITERATE_QT3DS_RENDER_TEXT_PROPERTIES
        // QT3DS-3993: store line feeds as replacement chars in UIP
        auto text = QString::fromUtf8(inItem.m_Text);
        text.replace(LINE_BREAK_SUBSTITUTE, "\n");
        inItem.m_Text = m_StrTable.RegisterStr(text);
    }
    void ParseProperties(SLightmaps &inItem, IPropertyParser &inParser)
    {
        ITERATE_QT3DS_RENDER_LIGHTMAP_PROPERTIES
    }
    void ParseProperties(SDefaultMaterial &inItem, IPropertyParser &inParser)
    {
        ITERATE_QT3DS_RENDER_MATERIAL_PROPERTIES
        ParseProperties(inItem.m_Lightmaps, inParser);
    }
    void ParseProperties(SReferencedMaterial &inItem, IPropertyParser &inParser)
    {
        ITERATE_QT3DS_RENDER_REFERENCED_MATERIAL_PROPERTIES
        // Propagate lightmaps
        if (inItem.m_ReferencedMaterial
            && inItem.m_ReferencedMaterial->m_Type == GraphObjectTypes::DefaultMaterial)
            ParseProperties(
                static_cast<SDefaultMaterial *>(inItem.m_ReferencedMaterial)->m_Lightmaps,
                inParser);
        else if (inItem.m_ReferencedMaterial
                 && inItem.m_ReferencedMaterial->m_Type == GraphObjectTypes::CustomMaterial)
            ParseProperties(
                static_cast<SCustomMaterial *>(inItem.m_ReferencedMaterial)->m_Lightmaps, inParser);
    }
    void ParseProperties(SImage &inItem, IPropertyParser &inParser)
    {
        ITERATE_QT3DS_RENDER_IMAGE_PROPERTIES
    }
    template <typename TDataType>
    void SetDynamicObjectProperty(SDynamicObject &inEffect,
                                  const qt3ds::render::dynamic::SPropertyDefinition &inPropDesc,
                                  const TDataType &inProp)
    {
        memCopy(inEffect.GetDataSectionBegin() + inPropDesc.m_Offset, &inProp, sizeof(TDataType));
    }
    template <typename TDataType>
    void SetDynamicObjectProperty(SDynamicObject &inEffect,
                                  const qt3ds::render::dynamic::SPropertyDefinition &inPropDesc,
                                  Option<TDataType> inProp)
    {
        if (inProp.hasValue()) {
            SetDynamicObjectProperty(inEffect, inPropDesc, *inProp);
        }
    }
    void ParseProperties(SCustomMaterial &inItem, IPropertyParser &inParser)
    {
        ParseProperties(static_cast<SDynamicObject &>(inItem), inParser);
        ParseProperties(inItem.m_Lightmaps, inParser);
        ITERATE_QT3DS_RENDER_CUSTOM_MATERIAL_PROPERTIES
    }
    void ParseProperties(SDynamicObject &inDynamicObject, IPropertyParser &inParser)
    {
        NVConstDataRef<qt3ds::render::dynamic::SPropertyDefinition> theProperties =
            m_DynamicObjectSystem.GetProperties(inDynamicObject.m_ClassName);

        for (QT3DSU32 idx = 0, end = theProperties.size(); idx < end; ++idx) {
            const qt3ds::render::dynamic::SPropertyDefinition &theDefinition(theProperties[idx]);
            switch (theDefinition.m_DataType) {
            case qt3ds::render::NVRenderShaderDataTypes::QT3DSRenderBool:
                SetDynamicObjectProperty(inDynamicObject, theDefinition,
                                         inParser.ParseBool(theDefinition.m_Name));
                break;
            case qt3ds::render::NVRenderShaderDataTypes::QT3DSF32:
                SetDynamicObjectProperty(inDynamicObject, theDefinition,
                                         inParser.ParseFloat(theDefinition.m_Name));
                break;
            case qt3ds::render::NVRenderShaderDataTypes::QT3DSI32:
                if (theDefinition.m_IsEnumProperty == false)
                    SetDynamicObjectProperty(inDynamicObject, theDefinition,
                                             inParser.ParseU32(theDefinition.m_Name));
                else {
                    Option<eastl::string> theEnum = inParser.ParseStr(theDefinition.m_Name);
                    if (theEnum.hasValue()) {
                        NVConstDataRef<CRegisteredString> theEnumNames =
                            theDefinition.m_EnumValueNames;
                        for (QT3DSU32 idx = 0, end = theEnumNames.size(); idx < end; ++idx) {
                            if (theEnum->compare(theEnumNames[idx].c_str()) == 0) {
                                SetDynamicObjectProperty(inDynamicObject, theDefinition, idx);
                                break;
                            }
                        }
                    }
                }
                break;
            case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4:
                SetDynamicObjectProperty(inDynamicObject, theDefinition,
                                         inParser.ParseVec4(theDefinition.m_Name));
                break;
            case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec3:
                SetDynamicObjectProperty(inDynamicObject, theDefinition,
                                         inParser.ParseVec3(theDefinition.m_Name));
                break;
            case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec2:
                SetDynamicObjectProperty(inDynamicObject, theDefinition,
                                         inParser.ParseVec2(theDefinition.m_Name));
                break;
            case qt3ds::render::NVRenderShaderDataTypes::NVRenderTexture2DPtr:
            case qt3ds::render::NVRenderShaderDataTypes::NVRenderImage2DPtr: {
                Option<eastl::string> theTexture = inParser.ParseStr(theDefinition.m_Name);
                if (theTexture.hasValue()) {
                    CRegisteredString theStr;
                    if (theTexture->size())
                        theStr = m_StrTable.RegisterStr(theTexture->c_str());

                    SetDynamicObjectProperty(inDynamicObject, theDefinition, theStr);
                }
            } break;
            case qt3ds::render::NVRenderShaderDataTypes::NVRenderDataBufferPtr:
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
    }
    void ParseProperties(SPath &inItem, IPropertyParser &inParser)
    {
        ParseProperties(static_cast<SNode &>(inItem), inParser);
        ITERATE_QT3DS_RENDER_PATH_PROPERTIES
    }
    void ParseProperties(SPathSubPath &inItem, IPropertyParser &inParser)
    {
        ITERATE_QT3DS_RENDER_PATH_SUBPATH_PROPERTIES
    }

    void AddPluginPropertyUpdate(eastl::vector<qt3ds::render::SRenderPropertyValueUpdate> &ioUpdates,
                                 qt3ds::render::IRenderPluginClass &,
                                 const qt3ds::render::SRenderPluginPropertyDeclaration &inDeclaration,
                                 Option<float> data)
    {
        if (data.hasValue()) {
            ioUpdates.push_back(
                qt3ds::render::SRenderPropertyValueUpdate(inDeclaration.m_Name, *data));
        }
    }
    void AddPluginPropertyUpdate(eastl::vector<qt3ds::render::SRenderPropertyValueUpdate> &ioUpdates,
                                 qt3ds::render::IRenderPluginClass &inClass,
                                 const qt3ds::render::SRenderPluginPropertyDeclaration &inDeclaration,
                                 Option<QT3DSVec2> data)
    {
        if (data.hasValue()) {
            ioUpdates.push_back(qt3ds::render::SRenderPropertyValueUpdate(
                inClass.GetPropertyValueInfo(inDeclaration.m_StartOffset).first, data->x));
            ioUpdates.push_back(qt3ds::render::SRenderPropertyValueUpdate(
                inClass.GetPropertyValueInfo(inDeclaration.m_StartOffset + 1).first, data->y));
        }
    }
    void AddPluginPropertyUpdate(eastl::vector<qt3ds::render::SRenderPropertyValueUpdate> &ioUpdates,
                                 qt3ds::render::IRenderPluginClass &inClass,
                                 const qt3ds::render::SRenderPluginPropertyDeclaration &inDeclaration,
                                 Option<QT3DSVec3> data)
    {
        if (data.hasValue()) {
            ioUpdates.push_back(qt3ds::render::SRenderPropertyValueUpdate(
                inClass.GetPropertyValueInfo(inDeclaration.m_StartOffset).first, data->x));
            ioUpdates.push_back(qt3ds::render::SRenderPropertyValueUpdate(
                inClass.GetPropertyValueInfo(inDeclaration.m_StartOffset + 1).first, data->y));
            ioUpdates.push_back(qt3ds::render::SRenderPropertyValueUpdate(
                inClass.GetPropertyValueInfo(inDeclaration.m_StartOffset + 2).first, data->z));
        }
    }
    void AddPluginPropertyUpdate(eastl::vector<qt3ds::render::SRenderPropertyValueUpdate> &ioUpdates,
                                 qt3ds::render::IRenderPluginClass &,
                                 const qt3ds::render::SRenderPluginPropertyDeclaration &inDeclaration,
                                 Option<qt3ds::QT3DSI32> dataOpt)
    {
        if (dataOpt.hasValue()) {
            long data = static_cast<long>(*dataOpt);
            ioUpdates.push_back(
                qt3ds::render::SRenderPropertyValueUpdate(inDeclaration.m_Name, (QT3DSI32)data));
        }
    }
    void AddPluginPropertyUpdate(eastl::vector<qt3ds::render::SRenderPropertyValueUpdate> &ioUpdates,
                                 qt3ds::render::IRenderPluginClass &,
                                 const qt3ds::render::SRenderPluginPropertyDeclaration &inDeclaration,
                                 Option<eastl::string> dataOpt)
    {
        if (dataOpt.hasValue()) {
            eastl::string &data = dataOpt.getValue();
            ioUpdates.push_back(qt3ds::render::SRenderPropertyValueUpdate(
                inDeclaration.m_Name, m_StrTable.RegisterStr(data.c_str())));
        }
    }
    void AddPluginPropertyUpdate(eastl::vector<qt3ds::render::SRenderPropertyValueUpdate> &ioUpdates,
                                 qt3ds::render::IRenderPluginClass &,
                                 const qt3ds::render::SRenderPluginPropertyDeclaration &inDeclaration,
                                 Option<bool> dataOpt)
    {
        if (dataOpt.hasValue()) {
            bool &data = dataOpt.getValue();
            ioUpdates.push_back(
                qt3ds::render::SRenderPropertyValueUpdate(inDeclaration.m_Name, data));
        }
    }
    void ParseProperties(qt3ds::render::SRenderPlugin &inRenderPlugin, IPropertyParser &inParser)
    {
        qt3ds::render::IRenderPluginClass *theClass =
            m_RenderPluginManager.GetRenderPlugin(inRenderPlugin.m_PluginPath);
        if (theClass) {
            qt3ds::foundation::NVConstDataRef<qt3ds::render::SRenderPluginPropertyDeclaration>
                theClassProps = theClass->GetRegisteredProperties();
            if (theClassProps.size()) {
                qt3ds::render::IRenderPluginInstance *theInstance =
                    m_RenderPluginManager.GetOrCreateRenderPluginInstance(
                        inRenderPlugin.m_PluginPath, &inRenderPlugin);
                if (theInstance) {
                    eastl::vector<qt3ds::render::SRenderPropertyValueUpdate> theUpdates;
                    for (QT3DSU32 idx = 0, end = theClassProps.size(); idx < end; ++idx) {
                        const qt3ds::render::SRenderPluginPropertyDeclaration &theDec(
                            theClassProps[idx]);
                        eastl::string tempStr;
                        switch (theDec.m_Type) {
                        case qt3ds::render::SRenderPluginPropertyTypes::Float:
                            AddPluginPropertyUpdate(theUpdates, *theClass, theDec,
                                                    inParser.ParseFloat(theDec.m_Name.c_str()));
                            break;
                        case qt3ds::render::SRenderPluginPropertyTypes::Vector2:
                            AddPluginPropertyUpdate(theUpdates, *theClass, theDec,
                                                    inParser.ParseVec2(theDec.m_Name.c_str()));
                            break;
                        case qt3ds::render::SRenderPluginPropertyTypes::Color:
                        case qt3ds::render::SRenderPluginPropertyTypes::Vector3:
                            AddPluginPropertyUpdate(theUpdates, *theClass, theDec,
                                                    inParser.ParseVec3(theDec.m_Name.c_str()));
                            break;
                        case qt3ds::render::SRenderPluginPropertyTypes::Long:
                            AddPluginPropertyUpdate(theUpdates, *theClass, theDec,
                                                    inParser.ParseI32(theDec.m_Name.c_str()));
                            break;
                        case qt3ds::render::SRenderPluginPropertyTypes::String:
                            AddPluginPropertyUpdate(theUpdates, *theClass, theDec,
                                                    inParser.ParseStr(theDec.m_Name.c_str()));
                            break;
                        case qt3ds::render::SRenderPluginPropertyTypes::Boolean:
                            AddPluginPropertyUpdate(theUpdates, *theClass, theDec,
                                                    inParser.ParseBool(theDec.m_Name.c_str()));
                            break;
                        default:
                            QT3DS_ASSERT(false);
                        }
                    }
                    theInstance->Update(
                        qt3ds::foundation::toConstDataRef(theUpdates.data(), theUpdates.size()));
                }
            }
        }
    }

#undef HANDLE_QT3DS_RENDER_PROPERTY
#undef HANDLE_QT3DS_RENDER_ENUM_PROPERTY
#undef HANDLE_QT3DS_RENDER_RADIAN_PROPERTY
#undef HANDLE_QT3DS_RENDER_SOURCEPATH_PROPERTY
#undef HANDLE_QT3DS_RENDER_ARRAY_PROPERTY
#undef HANDLE_QT3DS_NODE_FLAGS_PROPERTY
#undef HANDLE_QT3DS_ROTATION_ORDER_PROPERTY
#undef HANDLE_QT3DS_RENDER_OPACITY_PROPERTY
#undef HANDLE_QT3DS_NODE_ORIENTATION_PROPERTY
#undef HANDLE_QT3DS_RENDER_DEPTH_TEST_PROPERTY
#undef HANDLE_QT3DS_RENDER_VEC2_PROPERTY

    void ParseGraphPass1(SGraphObject *inParent)
    {
        TScope __elemScope(m_Reader);
        qt3dsdm::ComposerObjectTypes::Enum theObjType =
            qt3dsdm::ComposerObjectTypes::Convert(m_Reader.GetElementName());
        SGraphObject *theNewObject(NULL);
        const char8_t *theId;
        const char8_t *theVariants;
        m_Reader.Att("id", theId);
        m_Reader.Att("variants", theVariants);

        QString theString(theVariants);
        QStringRef theStringRef(&theString);
        bool isPartOfConfig = m_variantConfig.isPartOfConfig(theStringRef);
        if (isPartOfConfig) {
            switch (theObjType) {
            case qt3dsdm::ComposerObjectTypes::Scene: {
                SScene *theScene = QT3DS_NEW(m_PresentationAllocator, SScene)();
                theNewObject = theScene;
                m_Presentation->m_Scene = theScene;
                theScene->m_Presentation = m_Presentation;
            } break;
            case qt3dsdm::ComposerObjectTypes::Layer:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SLayer)();
                break;
            case qt3dsdm::ComposerObjectTypes::Group:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SNode)();
                break;
            case qt3dsdm::ComposerObjectTypes::Component:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SNode)();
                break;
            case qt3dsdm::ComposerObjectTypes::Camera:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SCamera)();
                break;
            case qt3dsdm::ComposerObjectTypes::Light:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SLight)();
                break;
            case qt3dsdm::ComposerObjectTypes::Model:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SModel)();
                break;
            case qt3dsdm::ComposerObjectTypes::Material:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SDefaultMaterial)();
                break;
            case qt3dsdm::ComposerObjectTypes::ReferencedMaterial:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SReferencedMaterial)();
                break;
            case qt3dsdm::ComposerObjectTypes::Image:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SImage)();
                break;
            case qt3dsdm::ComposerObjectTypes::Text:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SText)();
                break;
            case qt3dsdm::ComposerObjectTypes::Path:
                theNewObject = QT3DS_NEW(m_PresentationAllocator, SPath)();
                break;
            case qt3dsdm::ComposerObjectTypes::SubPath: {
                SPathSubPath *thePath = QT3DS_NEW(m_PresentationAllocator, SPathSubPath)();
                theNewObject = thePath;
                QT3DSU32 anchorCount = 0;
                TScope _childScope(m_Reader);
                for (bool success = m_Reader.MoveToFirstChild("PathAnchorPoint"); success;
                     success = m_Reader.MoveToNextSibling("PathAnchorPoint")) {
                    const char8_t *theId;
                    m_Reader.Att("id", theId);
                    CRegisteredString theIdStr = m_StrTable.RegisterStr(theId);
                    m_AnchorIdToPathAndAnchorIndexMap.insert(
                        eastl::make_pair(theIdStr, SPathAndAnchorIndex(thePath, anchorCount)));
                    ++anchorCount;
                }
                m_PathManager.ResizePathSubPathBuffer(*thePath, anchorCount);
            } break;
            case qt3dsdm::ComposerObjectTypes::Effect: {
                const char8_t *effectClassId;
                m_Reader.Att("class", effectClassId);
                CRegisteredString theStr = m_StrTable.RegisterStr(effectClassId + 1);
                if (m_EffectSystem.IsEffectRegistered(theStr))
                    theNewObject = m_EffectSystem.CreateEffectInstance(theStr, m_PresentationAllocator);
            } break;
            case qt3dsdm::ComposerObjectTypes::RenderPlugin: {
                const char8_t *classId;
                m_Reader.Att("class", classId);
                if (!qt3ds::foundation::isTrivial(classId)) {
                    ++classId;
                    TIdStringMap::iterator iter =
                        m_RenderPluginSourcePaths.find(m_StrTable.RegisterStr(classId));
                    if (iter != m_RenderPluginSourcePaths.end()) {
                        CRegisteredString thePluginPath = m_StrTable.RegisterStr(iter->second.c_str());
                        qt3ds::render::IRenderPluginClass *theClass =
                            m_RenderPluginManager.GetRenderPlugin(thePluginPath);
                        if (theClass) {
                            qt3ds::render::SRenderPlugin *thePlugin =
                                QT3DS_NEW(m_PresentationAllocator, qt3ds::render::SRenderPlugin)();
                            thePlugin->m_PluginPath = thePluginPath;
                            thePlugin->m_Flags.SetActive(true);
                            theNewObject = thePlugin;
                        }
                    }
                }
            } break;
            case qt3dsdm::ComposerObjectTypes::CustomMaterial: {
                const char8_t *materialClassId;
                m_Reader.Att("class", materialClassId);
                CRegisteredString theStr = m_StrTable.RegisterStr(materialClassId + 1);
                if (m_CustomMaterialSystem.IsMaterialRegistered(theStr)) {
                    theNewObject =
                        m_CustomMaterialSystem.CreateCustomMaterial(theStr, m_PresentationAllocator);
                }
            } break;
            default:
                // Ignoring unknown objects entirely at this point
                break;
            }
        }
        if (theNewObject) {
            CRegisteredString theObjectId(m_StrTable.RegisterStr(theId));
            m_ObjectMap.insert(eastl::make_pair(theObjectId, theNewObject));
            theNewObject->m_Id = theObjectId;
            // setup hierarchy
            bool isParentNode;
            bool isChildNode;
            if (inParent) {
                switch (inParent->m_Type) {
                case GraphObjectTypes::Scene:
                    if (theNewObject->m_Type == GraphObjectTypes::Layer) {
                        static_cast<SScene *>(inParent)->AddChild(
                            *static_cast<SLayer *>(theNewObject));
                    }
                    break;

                case GraphObjectTypes::DefaultMaterial:
                    if (theNewObject->m_Type == GraphObjectTypes::Image) {
                        static_cast<SImage *>(theNewObject)->m_Parent =
                            static_cast<SDefaultMaterial *>(inParent);
                        eastl::string thePath = eastl::string(theNewObject->m_Id.c_str());
                        if (thePath.find("probe") != eastl::string::npos)
                            static_cast<SImage *>(theNewObject)->m_MappingMode =
                                ImageMappingModes::LightProbe;
                    }
                    break;

                case GraphObjectTypes::CustomMaterial:
                    if (theNewObject->m_Type == GraphObjectTypes::Image) {
                        static_cast<SImage *>(theNewObject)->m_Parent =
                            static_cast<SCustomMaterial *>(inParent);
                        eastl::string thePath = eastl::string(theNewObject->m_Id.c_str());
                        if (thePath.find("probe") != eastl::string::npos) {
                            static_cast<SImage *>(theNewObject)->m_MappingMode =
                                ImageMappingModes::LightProbe;
                        }
                    } else {
                        QT3DS_ASSERT(false);
                    }
                    break;
                case GraphObjectTypes::ReferencedMaterial:
                    if (theNewObject->m_Type == GraphObjectTypes::Image) {
                        // nothing to do yet
                    } else {
                        QT3DS_ASSERT(false);
                    }
                    break;
                case GraphObjectTypes::Path:

                    if (GraphObjectTypes::IsMaterialType(theNewObject->m_Type))
                        static_cast<SPath *>(inParent)->AddMaterial(theNewObject);

                    else if (theNewObject->m_Type == GraphObjectTypes::PathSubPath)
                        static_cast<SPath *>(inParent)->AddSubPath(
                            *static_cast<SPathSubPath *>(theNewObject));

                    break;

                default:
                    isParentNode = IsNode(inParent->m_Type);
                    isChildNode = IsNode(theNewObject->m_Type);
                    if (isParentNode && isChildNode) {
                        static_cast<SNode *>(inParent)->AddChild(
                            *static_cast<SNode *>(theNewObject));
                    } else if (isParentNode) {
                        if (inParent->m_Type == GraphObjectTypes::Model
                            && IsMaterial(theNewObject)) {
                            static_cast<SModel *>(inParent)->AddMaterial(*theNewObject);
                        } else {
                            if (inParent->m_Type == GraphObjectTypes::Layer
                                && theNewObject->m_Type == GraphObjectTypes::Effect) {
                                static_cast<SLayer *>(inParent)->AddEffect(
                                    *static_cast<SEffect *>(theNewObject));
                            } else if (inParent->m_Type == GraphObjectTypes::Layer
                                       && theNewObject->m_Type == GraphObjectTypes::Image) {
                                eastl::string thePath = eastl::string(theNewObject->m_Id.c_str());
                                if (thePath.find("probe2") != eastl::string::npos) {
                                    static_cast<SLayer *>(inParent)->m_LightProbe2 =
                                        static_cast<SImage *>(theNewObject);
                                } else {
                                    static_cast<SLayer *>(inParent)->m_LightProbe =
                                        static_cast<SImage *>(theNewObject);
                                }
                            } else {
                                if (theNewObject->m_Type == GraphObjectTypes::RenderPlugin) {
                                    qt3ds::render::SRenderPlugin *childObj =
                                        static_cast<qt3ds::render::SRenderPlugin *>(theNewObject);
                                    if (inParent->m_Type == GraphObjectTypes::Layer) {
                                        static_cast<SLayer *>(inParent)->m_RenderPlugin = childObj;
                                    } else {
                                        QT3DS_ASSERT(false);
                                    }
                                } else {
                                    QT3DS_ASSERT(false);
                                }
                            }
                        }
                    } else {
                        if (inParent->m_Type == GraphObjectTypes::Image
                            && theNewObject->m_Type == GraphObjectTypes::RenderPlugin) {
                            static_cast<SImage *>(inParent)->m_RenderPlugin =
                                static_cast<qt3ds::render::SRenderPlugin *>(theNewObject);
                        } else {
                            QT3DS_ASSERT(false);
                        }
                    }
                }
            }
            for (bool valid = m_Reader.MoveToFirstChild(); valid;
                 valid = m_Reader.MoveToNextSibling())
                ParseGraphPass1(theNewObject);
        } else {
            if (isPartOfConfig) {
                // Object was of unknown type -> parse children with NULL parent
                for (bool valid = m_Reader.MoveToFirstChild(); valid;
                     valid = m_Reader.MoveToNextSibling()) {
                    ParseGraphPass1(NULL);
                }
            }
            // If object wasn't part of variant config -> skip children.
            // Continue parsing from next sibling with same parent.
        }
    }

    template <typename TObjType>
    void ParsePass2Properties(TObjType &inObject, const char8_t *inClassId)
    {
        const char8_t *theTypeName = m_Reader.GetNarrowElementName();
        SMetaPropertyParser theMetaParser(theTypeName, inClassId, m_MetaData);
        // Set default values
        ParseProperties(inObject, theMetaParser);

        // Now setup property values from the element itself.
        SDomReaderPropertyParser theReaderParser(m_Reader, m_TempBuf, *this, inObject);
        ParseProperties(inObject, theReaderParser);
    }

    // Parse the instance properties from the graph.
    void ParseGraphPass2()
    {
        TScope __instanceScope(m_Reader);
        const char8_t *theId;
        m_Reader.Att("id", theId);
        const char8_t *theClass = "";
        const char8_t *theVariants = "";
        m_Reader.Att("class", theClass);
        m_Reader.Att("variants", theVariants);

        QString theString(theVariants);
        QStringRef theStringRef(&theString);
        bool isPartOfConfig = m_variantConfig.isPartOfConfig(theStringRef);
        if (isPartOfConfig) {
            TIdObjectMap::iterator theObject = m_ObjectMap.find(m_StrTable.RegisterStr(theId));
            if (theObject != m_ObjectMap.end()) {
                switch (theObject->second->m_Type) {
                case GraphObjectTypes::Scene:
                    ParsePass2Properties(*static_cast<SScene *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::Node:
                    ParsePass2Properties(*static_cast<SNode *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::Layer:
                    ParsePass2Properties(*static_cast<SLayer *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::Camera:
                    ParsePass2Properties(*static_cast<SCamera *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::Light:
                    ParsePass2Properties(*static_cast<SLight *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::Model:
                    ParsePass2Properties(*static_cast<SModel *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::DefaultMaterial:
                    ParsePass2Properties(*static_cast<SDefaultMaterial *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::ReferencedMaterial:
                    ParsePass2Properties(*static_cast<SReferencedMaterial *>(theObject->second),
                                         theClass);
                    break;
                case GraphObjectTypes::Image:
                    ParsePass2Properties(*static_cast<SImage *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::Text:
                    ParsePass2Properties(*static_cast<SText *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::Effect:
                    ParsePass2Properties(*static_cast<SEffect *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::RenderPlugin:
                    ParsePass2Properties(*static_cast<qt3ds::render::SRenderPlugin *>(theObject->second),
                                         theClass);
                    break;
                case GraphObjectTypes::CustomMaterial:
                    ParsePass2Properties(*static_cast<SCustomMaterial *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::Path:
                    ParsePass2Properties(*static_cast<SPath *>(theObject->second), theClass);
                    break;
                case GraphObjectTypes::PathSubPath:
                    ParsePass2Properties(*static_cast<SPathSubPath *>(theObject->second), theClass);
                    break;
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }
        }

        // If not part of variant config -> ignore children
        if (isPartOfConfig) {
            for (bool valid = m_Reader.MoveToFirstChild(); valid; valid = m_Reader.MoveToNextSibling())
                ParseGraphPass2();
        }
    }

    static bool ParseVec2(SDomReaderPropertyParser &inParser, const char *inName, QT3DSVec2 &outValue)
    {
        Option<QT3DSVec2> result = inParser.ParseVec2(inName);

        if (result.hasValue())
            outValue = *result;

        return result.hasValue();
    }

    static bool ParseFloat(SDomReaderPropertyParser &inParser, const char *inName, QT3DSF32 &outValue)
    {
        Option<QT3DSF32> result = inParser.ParseFloat(inName);
        if (result.hasValue())
            outValue = *result;
        return result.hasValue();
    }

    void ParseState(bool inSetSetValues)
    {
        TScope __slideScope(m_Reader);
        for (bool valid = m_Reader.MoveToFirstChild(); valid;
             valid = m_Reader.MoveToNextSibling()) {
            if (strcmp(m_Reader.GetNarrowElementName(), "Add") == 0
                || (inSetSetValues && strcmp(m_Reader.GetNarrowElementName(), "Set") == 0)) {
                const char8_t *theId;
                m_Reader.Att("ref", theId);
                CRegisteredString theIdStr(m_StrTable.RegisterStr(theId + 1));
                if (m_ObjectMap.contains(theIdStr)) {
                    TIdObjectMap::iterator theObject = m_ObjectMap.find(theIdStr);
                    if (theObject != m_ObjectMap.end()) {
                        SDomReaderPropertyParser parser(m_Reader, m_TempBuf, *this, *theObject->second);
                        switch (theObject->second->m_Type) {
                        case GraphObjectTypes::Scene:
                            ParseProperties(*reinterpret_cast<SScene *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::Node:
                            ParseProperties(*reinterpret_cast<SNode *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::Layer:
                            ParseProperties(*reinterpret_cast<SLayer *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::Camera:
                            ParseProperties(*reinterpret_cast<SCamera *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::Light:
                            ParseProperties(*reinterpret_cast<SLight *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::Model:
                            ParseProperties(*reinterpret_cast<SModel *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::DefaultMaterial:
                            ParseProperties(*reinterpret_cast<SDefaultMaterial *>(theObject->second),
                                            parser);
                            break;
                        case GraphObjectTypes::ReferencedMaterial:
                            ParseProperties(*static_cast<SReferencedMaterial *>(theObject->second),
                                            parser);
                            break;
                        case GraphObjectTypes::Image:
                            ParseProperties(*reinterpret_cast<SImage *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::Text:
                            ParseProperties(*static_cast<SText *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::Effect:
                            ParseProperties(*static_cast<SEffect *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::RenderPlugin:
                            ParseProperties(
                                *static_cast<qt3ds::render::SRenderPlugin *>(theObject->second), parser);
                            break;
                        case GraphObjectTypes::CustomMaterial:
                            ParseProperties(
                                *static_cast<qt3ds::render::SCustomMaterial *>(theObject->second),
                                parser);
                            break;
                        case GraphObjectTypes::Path:
                            ParseProperties(*static_cast<qt3ds::render::SPath *>(theObject->second),
                                            parser);
                            break;
                        case GraphObjectTypes::PathSubPath:
                            ParseProperties(
                                *static_cast<qt3ds::render::SPathSubPath *>(theObject->second), parser);
                            break;
                        default:
                            QT3DS_ASSERT(false);
                            break;
                        }
                    } else {
                        TIdPathAnchorIndexMap::iterator iter =
                            m_AnchorIdToPathAndAnchorIndexMap.find(theIdStr);
                        if (iter != m_AnchorIdToPathAndAnchorIndexMap.end()) {
                            SDomReaderPropertyParser parser(m_Reader, m_TempBuf, *this,
                                                            *iter->second.m_Segment);
                            NVDataRef<qt3ds::render::SPathAnchorPoint> thePathBuffer =
                                m_PathManager.GetPathSubPathBuffer(*iter->second.m_Segment);
                            QT3DSU32 anchorIndex = iter->second.m_AnchorIndex;
                            QT3DSU32 numAnchors = thePathBuffer.size();
                            if (anchorIndex < numAnchors) {
                                qt3ds::render::SPathAnchorPoint &thePoint(thePathBuffer[anchorIndex]);
                                ParseVec2(parser, "position", thePoint.m_Position);
                                ParseFloat(parser, "incomingangle", thePoint.m_IncomingAngle);
                                thePoint.m_OutgoingAngle = thePoint.m_IncomingAngle + 180.0f;
                                ParseFloat(parser, "incomingdistance", thePoint.m_IncomingDistance);
                                ParseFloat(parser, "outgoingdistance", thePoint.m_OutgoingDistance);
                            }
                        }
                    }
                }
            }
        }
    }

    void AddPluginProperty(qt3ds::render::IRenderPluginClass &pluginClass,
                           qt3ds::render::SRenderPluginPropertyTypes::Enum inPropType,
                           eastl::string &tempStr, const char *propName)
    {
        tempStr.assign(propName);
        qt3ds::render::SRenderPluginPropertyDeclaration theDec(
            m_StrTable.RegisterStr(tempStr.c_str()), inPropType);
        pluginClass.RegisterProperty(theDec);
    }

    SPresentation *Load(bool inSetValuesFromSlides)
    {
        {
            TScope __outerScope(m_Reader);
            if (m_Reader.MoveToFirstChild("ProjectSettings")) {
                m_Reader.Att("presentationWidth", m_Presentation->m_PresentationDimensions.x);
                m_Reader.Att("presentationHeight", m_Presentation->m_PresentationDimensions.y);
                // Upsize them to a multiple of four.
                m_Presentation->m_PresentationDimensions.x =
                    (QT3DSF32)qt3ds::render::ITextRenderer::NextMultipleOf4(
                        (QT3DSU32)m_Presentation->m_PresentationDimensions.x);
                m_Presentation->m_PresentationDimensions.y =
                    (QT3DSF32)qt3ds::render::ITextRenderer::NextMultipleOf4(
                        (QT3DSU32)m_Presentation->m_PresentationDimensions.y);
                const char8_t *thePresentationRotation = "";
                if (m_Reader.Att("presentationRotation", thePresentationRotation)) {
                    bool success = SRenderUIPLoader::ConvertEnumFromStr(
                        thePresentationRotation, m_Presentation->m_PresentationRotation);
                    (void)success;
                    QT3DS_ASSERT(success);
                }
                m_Reader.Att("preferKTX", m_Presentation->m_preferKTX);
            }
        }
        {
            TScope __outerScope(m_Reader);
            if (m_Reader.MoveToFirstChild("Classes")) {
                for (bool valid = m_Reader.MoveToFirstChild(); valid;
                     valid = m_Reader.MoveToNextSibling()) {
                    const char8_t *idStr = "", *name = "", *sourcepath = "";
                    m_Reader.Att("id", idStr);
                    m_Reader.Att("name", name);
                    m_Reader.Att("sourcepath", sourcepath);
                    if (AreEqual(m_Reader.GetNarrowElementName(), "Effect")) {
                        CRegisteredString theId(m_StrTable.RegisterStr(idStr));
                        if (m_EffectSystem.IsEffectRegistered(theId) == false) {
                            // File should already be loaded.
                            Option<qt3dsdm::SMetaDataEffect> theEffectMetaData =
                                m_MetaData.GetEffectMetaDataBySourcePath(sourcepath);
                            if (theEffectMetaData.hasValue()) {
                                qt3ds::render::IUIPLoader::CreateEffectClassFromMetaEffect(
                                    theId, m_Foundation, m_EffectSystem, *theEffectMetaData,
                                    m_StrTable);
                            } else {
                                QT3DS_ASSERT(false);
                            }
                        }
                    } else if (AreEqual(m_Reader.GetNarrowElementName(), "CustomMaterial")) {
                        CRegisteredString theId(m_StrTable.RegisterStr(idStr));
                        if (m_CustomMaterialSystem.IsMaterialRegistered(theId) == false) {
                            // File should already be loaded.
                            Option<qt3dsdm::SMetaDataCustomMaterial> theMetaData =
                                m_MetaData.GetMaterialMetaDataBySourcePath(sourcepath);
                            if (theMetaData.hasValue()) {
                                qt3ds::render::IUIPLoader::CreateMaterialClassFromMetaMaterial(
                                    theId, m_Foundation, m_CustomMaterialSystem, *theMetaData,
                                    m_StrTable);
                            } else {
                                QT3DS_ASSERT(false);
                            }
                        }
                    } else if (AreEqual(m_Reader.GetNarrowElementName(), "RenderPlugin")) {
                        CRegisteredString theId(m_StrTable.RegisterStr(idStr));
                        m_MetaData.LoadPluginXMLFile(m_Reader.GetNarrowElementName(), idStr, name,
                                                     sourcepath);
                        eastl::vector<Q3DStudio::TRuntimeMetaDataStrType> theProperties;
                        qt3ds::render::IRenderPluginClass *thePluginClass =
                            m_RenderPluginManager.GetOrCreateRenderPlugin(
                                m_StrTable.RegisterStr(sourcepath));
                        if (thePluginClass) {
                            m_RenderPluginSourcePaths.insert(
                                eastl::make_pair(m_StrTable.RegisterStr(idStr), sourcepath));
                            m_MetaData.GetInstanceProperties(m_Reader.GetNarrowElementName(), idStr,
                                                             theProperties, false);
                            eastl::string thePropertyStr;
                            CRegisteredString metaType =
                                m_MetaData.GetStringTable()->GetRenderStringTable().RegisterStr(
                                    m_Reader.GetNarrowElementName());
                            CRegisteredString metaId =
                                m_MetaData.GetStringTable()->GetRenderStringTable().RegisterStr(
                                    idStr);
                            for (QT3DSU32 idx = 0, end = theProperties.size(); idx < end; ++idx) {
                                using namespace Q3DStudio;
                                CRegisteredString metaProp =
                                    m_MetaData.GetStringTable()->GetRenderStringTable().RegisterStr(
                                        theProperties[idx].c_str());
                                Q3DStudio::ERuntimeDataModelDataType thePropType =
                                    m_MetaData.GetPropertyType(metaType, metaProp, metaId);
                                switch (thePropType) {
                                case ERuntimeDataModelDataTypeFloat:
                                    AddPluginProperty(
                                        *thePluginClass,
                                        qt3ds::render::SRenderPluginPropertyTypes::Float,
                                        thePropertyStr, metaProp.c_str());
                                    break;
                                case ERuntimeDataModelDataTypeFloat2:
                                    AddPluginProperty(
                                        *thePluginClass,
                                        qt3ds::render::SRenderPluginPropertyTypes::Vector2,
                                        thePropertyStr, metaProp.c_str());
                                    break;
                                case ERuntimeDataModelDataTypeFloat3:
                                    if (m_MetaData.GetAdditionalType(metaType, metaProp, metaId)
                                        != ERuntimeAdditionalMetaDataTypeColor)
                                        AddPluginProperty(
                                            *thePluginClass,
                                            qt3ds::render::SRenderPluginPropertyTypes::Vector3,
                                            thePropertyStr, metaProp.c_str());
                                    else
                                        AddPluginProperty(
                                            *thePluginClass,
                                            qt3ds::render::SRenderPluginPropertyTypes::Color,
                                            thePropertyStr, metaProp.c_str());
                                    break;
                                case ERuntimeDataModelDataTypeLong:
                                    AddPluginProperty(*thePluginClass,
                                                      qt3ds::render::SRenderPluginPropertyTypes::Long,
                                                      thePropertyStr, metaProp.c_str());
                                    break;
                                case ERuntimeDataModelDataTypeString:
                                case ERuntimeDataModelDataTypeStringRef:
                                    AddPluginProperty(
                                        *thePluginClass,
                                        qt3ds::render::SRenderPluginPropertyTypes::String,
                                        thePropertyStr, metaProp.c_str());
                                    break;
                                case ERuntimeDataModelDataTypeBool:
                                    AddPluginProperty(
                                        *thePluginClass,
                                        qt3ds::render::SRenderPluginPropertyTypes::Boolean,
                                        thePropertyStr, metaProp.c_str());
                                    break;
                                default:
                                    QT3DS_ASSERT(false);
                                }
                            }
                        }
                    }
                }
            }
        }
        {
            TScope __outerScope(m_Reader);
            if (m_Reader.MoveToFirstChild("BufferData")) {
                {
                    TScope __imageScope(m_Reader);
                    for (bool valid = m_Reader.MoveToFirstChild("ImageBuffer"); valid;
                         valid = m_Reader.MoveToNextSibling()) {
                        const char8_t *srcPath;
                        m_Reader.UnregisteredAtt("sourcepath", srcPath);
                        CRegisteredString imgPath = m_StrTable.RegisterStr(srcPath);
                        bool hasTransparency = false;
                        m_Reader.Att("hasTransparency", hasTransparency);
                        bool hasOpaque = false;
                        m_Reader.Att("hasOpaquePixels", hasOpaque);
                        m_BufferManager.SetImageHasTransparency(imgPath, hasTransparency,
                                                                hasOpaque);
                    }
                }
            }
        }
        {
            TScope __outerScope(m_Reader);
            {
                if (m_Reader.MoveToFirstChild("Graph")) {
                    {
                        TScope __graphScope(m_Reader);
                        for (bool valid = m_Reader.MoveToFirstChild(); valid;
                             valid = m_Reader.MoveToNextSibling())
                            ParseGraphPass1(NULL);
                    }
                    {
                        TScope __graphScope(m_Reader);
                        for (bool valid = m_Reader.MoveToFirstChild(); valid;
                             valid = m_Reader.MoveToNextSibling())
                            ParseGraphPass2();
                    }
                }
            }
        }
        TScope __outerScope(m_Reader);
        if (m_Reader.MoveToFirstChild("Logic")) {
            for (bool valid = m_Reader.MoveToFirstChild("State"); valid;
                 valid = m_Reader.MoveToNextSibling()) {
                {
                    TScope __slideScope(m_Reader);
                    ParseState(true); // parse master
                    for (bool subSlide = m_Reader.MoveToFirstChild("State"); subSlide;
                         subSlide = m_Reader.MoveToNextSibling("State")) {
                        TScope __subSlideScope(m_Reader);
                        ParseState(false); // parse slide setting only *add* values
                    }
                }
                {
                    TScope __slideScope(m_Reader);
                    if (inSetValuesFromSlides && m_Reader.MoveToFirstChild("State"))
                        ParseState(true); // parse slide setting only *set* values
                }
            }
        }

        return m_Presentation;
    }
};
}

SPresentation *qt3ds::render::IUIPLoader::LoadUIPFile(
    qt3dsdm::IDOMReader &inReader, const char8_t *inFullPathToPresentationFile,
    Q3DStudio::IRuntimeMetaData &inMetaData, IStringTable &inStrTable,
    NVFoundationBase &inFoundation
    // Allocator used for the presentation objects themselves
    // this allows clients to pre-allocate a block of memory just for
    // the scene graph
    ,
    NVAllocatorCallback &inPresentationAllocator
    // Map of string ids to objects
    ,
    TIdObjectMap &ioObjectMap, IBufferManager &inBufferManager, IEffectSystem &inEffectSystem,
    const char8_t *inPresentationDir, IRenderPluginManager &inPluginManager,
    ICustomMaterialSystem &inCMS, IDynamicObjectSystem &inDynamicSystem,
    qt3ds::render::IPathManager &inPathManager, IUIPReferenceResolver *inResolver,
    const Q3DSVariantConfig &variantConfig, bool inSetValuesFromSlides)
{
    SRenderUIPLoader theLoader(inReader, inFullPathToPresentationFile, inMetaData, inStrTable,
                               inFoundation, inPresentationAllocator, ioObjectMap, inBufferManager,
                               inEffectSystem, inPresentationDir, inPluginManager, inCMS,
                               inDynamicSystem, inPathManager, inResolver, variantConfig);
    return theLoader.Load(inSetValuesFromSlides);
}
using namespace qt3dsdm;

inline qt3ds::render::NVRenderTextureFormats::Enum
ConvertTypeAndFormatToTextureFormat(const char8_t *inType, const char8_t *inFormat,
                                    NVFoundationBase &inFoundation)
{
    qt3ds::render::NVRenderTextureFormats::Enum retval = qt3ds::render::NVRenderTextureFormats::RGBA8;
    if (AreEqual(inType, "ubyte")) {
        if (AreEqual(inFormat, "rgb"))
            retval = qt3ds::render::NVRenderTextureFormats::RGB8;
        else if (AreEqual(inFormat, "rgba"))
            retval = qt3ds::render::NVRenderTextureFormats::RGBA8;
        else if (AreEqual(inFormat, "alpha"))
            retval = qt3ds::render::NVRenderTextureFormats::Alpha8;
        else if (AreEqual(inFormat, "lum"))
            retval = qt3ds::render::NVRenderTextureFormats::Luminance8;
        else if (AreEqual(inFormat, "lum_alpha"))
            retval = qt3ds::render::NVRenderTextureFormats::LuminanceAlpha8;
    } else if (AreEqual(inType, "ushort")) {
        if (AreEqual(inFormat, "rgb"))
            retval = qt3ds::render::NVRenderTextureFormats::RGB565;
        else if (AreEqual(inFormat, "rgba"))
            retval = qt3ds::render::NVRenderTextureFormats::RGBA5551;
    } else {
        qCCritical(INVALID_PARAMETER, "Unsupported texture type %s, defaulting to RGBA8",
            inType);
    }
    return retval;
}

inline qt3ds::render::NVRenderTextureMagnifyingOp::Enum
ConvertFilterToMagOp(const char8_t *inFilter, NVFoundationBase &inFoundation)
{
    if (AreEqual(inFilter, "linear"))
        return qt3ds::render::NVRenderTextureMagnifyingOp::Linear;
    if (AreEqual(inFilter, "nearest"))
        return qt3ds::render::NVRenderTextureMagnifyingOp::Nearest;
    else {
        qCCritical(INVALID_PARAMETER, "Unsupported filter type %s, defaulting to linear",
            inFilter);
        return qt3ds::render::NVRenderTextureMagnifyingOp::Linear;
    }
}

inline qt3ds::render::NVRenderTextureCoordOp::Enum
ConvertTextureCoordOp(const char8_t *inWrap, NVFoundationBase &inFoundation)
{
    if (AreEqual(inWrap, "clamp"))
        return qt3ds::render::NVRenderTextureCoordOp::ClampToEdge;
    if (AreEqual(inWrap, "repeat"))
        return qt3ds::render::NVRenderTextureCoordOp::Repeat;
    else {
        qCCritical(INVALID_PARAMETER, "Unsupported wrap type %s, defaulting to clamp",
            inWrap);
        return qt3ds::render::NVRenderTextureCoordOp::ClampToEdge;
    }
}

// Re-register all strings because we can't be sure that the meta data system and the effect
// system are sharing the same string table.
void qt3ds::render::IUIPLoader::CreateEffectClassFromMetaEffect(
    CRegisteredString inEffectName, NVFoundationBase &inFoundation, IEffectSystem &inEffectSystem,
    const qt3dsdm::SMetaDataEffect &inMetaDataEffect, IStringTable &inStrTable)
{
    using namespace qt3ds::render::dynamic;
    if (inEffectSystem.IsEffectRegistered(inEffectName)) {
        qCCritical(INVALID_OPERATION, "Effect %s is already registered",
            inEffectName.c_str());
        QT3DS_ASSERT(false);
        return;
    }
    nvvector<SPropertyDeclaration> thePropertyDeclarations(
        inFoundation.getAllocator(), "qt3ds::render::IUIPLoader::CreateEffectClassFromMetaEffect");
    nvvector<CRegisteredString> theEnumNames(
        inFoundation.getAllocator(), "qt3ds::render::IUIPLoader::CreateEffectClassFromMetaEffect");
    Qt3DSString theConvertStr;
    Qt3DSString theConvertShaderTypeStr;
    Qt3DSString theConvertShaderVersionStr;

    for (QT3DSU32 idx = 0, end = inMetaDataEffect.m_Properties.size(); idx < end; ++idx)
        thePropertyDeclarations.push_back(
            SPropertyDeclaration(inMetaDataEffect.m_Properties[idx].m_Name.c_str(),
                                 inMetaDataEffect.m_Properties[idx].m_DataType));
    inEffectSystem.RegisterEffect(inEffectName, thePropertyDeclarations);
    for (QT3DSU32 idx = 0, end = inMetaDataEffect.m_Properties.size(); idx < end; ++idx) {
        const SPropertyDefinition &theDefinition(inMetaDataEffect.m_Properties[idx]);
        if (theDefinition.m_EnumValueNames.size()) {
            theEnumNames.clear();
            for (QT3DSU32 enumIdx = 0, enumEnd = theDefinition.m_EnumValueNames.size();
                 enumIdx < enumEnd; ++enumIdx)
                theEnumNames.push_back(
                    inStrTable.RegisterStr(theDefinition.m_EnumValueNames[enumIdx]));
            inEffectSystem.SetEffectPropertyEnumNames(
                inEffectName, inStrTable.RegisterStr(theDefinition.m_Name), theEnumNames);
        }
        if (theDefinition.m_DataType == qt3ds::render::NVRenderShaderDataTypes::NVRenderTexture2DPtr)
            inEffectSystem.SetEffectPropertyTextureSettings(
                inEffectName, inStrTable.RegisterStr(theDefinition.m_Name),
                inStrTable.RegisterStr(theDefinition.m_ImagePath), theDefinition.m_TexUsageType,
                theDefinition.m_CoordOp, theDefinition.m_MagFilterOp, theDefinition.m_MinFilterOp);
    }
    for (QT3DSU32 idx = 0, end = inMetaDataEffect.m_Shaders.size(); idx < end; ++idx) {
        const qt3dsdm::SMetaDataShader &theShader = inMetaDataEffect.m_Shaders[idx];
        theConvertStr.clear();
        theConvertStr = Qt3DSStringUtils::ConvertUTFtoQString(
            theShader.m_Code.c_str());
        theConvertShaderTypeStr = Qt3DSStringUtils::ConvertUTFtoQString(
            theShader.m_Type.c_str());
        theConvertShaderVersionStr = Qt3DSStringUtils::ConvertUTFtoQString(
            theShader.m_Version.c_str());

        inEffectSystem.SetShaderData(inStrTable.RegisterStr(theShader.m_Name.c_str()),
                                     theConvertStr.c_str(), theConvertShaderVersionStr.c_str(),
                                     theConvertStr.c_str(), theShader.m_HasGeomShader,
                                     theShader.m_IsComputeShader);
    }

    inEffectSystem.SetEffectCommands(inEffectName, inMetaDataEffect.m_EffectCommands);
}

void qt3ds::render::IUIPLoader::CreateMaterialClassFromMetaMaterial(
    CRegisteredString inClassName, NVFoundationBase &inFoundation,
    ICustomMaterialSystem &inMaterialSystem,
    const qt3dsdm::SMetaDataCustomMaterial &inMetaDataMaterial, IStringTable &inStrTable)
{
    using namespace qt3ds::render::dynamic;
    if (inMaterialSystem.IsMaterialRegistered(inClassName)) {
        qCCritical(INVALID_OPERATION, "Effect %s is already registered",
            inClassName.c_str());
        QT3DS_ASSERT(false);
        return;
    }
    nvvector<SPropertyDeclaration> thePropertyDeclarations(
        inFoundation.getAllocator(),
        "qt3ds::render::IUIPLoader::CreateMaterialClassFromMetaMaterial");
    nvvector<CRegisteredString> theEnumNames(
        inFoundation.getAllocator(),
        "qt3ds::render::IUIPLoader::CreateMaterialClassFromMetaMaterial");
    Qt3DSString theConvertStr;
    Qt3DSString theConvertShaderTypeStr;
    Qt3DSString theConvertShaderVersionStr;
    for (QT3DSU32 idx = 0, end = inMetaDataMaterial.m_Properties.size(); idx < end; ++idx)
        thePropertyDeclarations.push_back(
            SPropertyDeclaration(inMetaDataMaterial.m_Properties[idx].m_Name.c_str(),
                                 inMetaDataMaterial.m_Properties[idx].m_DataType));
    inMaterialSystem.RegisterMaterialClass(inClassName, thePropertyDeclarations);
    for (QT3DSU32 idx = 0, end = inMetaDataMaterial.m_Properties.size(); idx < end; ++idx) {
        const SPropertyDefinition &theDefinition(inMetaDataMaterial.m_Properties[idx]);
        if (theDefinition.m_EnumValueNames.size()) {
            theEnumNames.clear();
            for (QT3DSU32 enumIdx = 0, enumEnd = theDefinition.m_EnumValueNames.size();
                 enumIdx < enumEnd; ++enumIdx)
                theEnumNames.push_back(
                    inStrTable.RegisterStr(theDefinition.m_EnumValueNames[enumIdx]));
            inMaterialSystem.SetPropertyEnumNames(
                inClassName, inStrTable.RegisterStr(theDefinition.m_Name), theEnumNames);
        }
        if (theDefinition.m_DataType == qt3ds::render::NVRenderShaderDataTypes::NVRenderTexture2DPtr)
            inMaterialSystem.SetPropertyTextureSettings(
                inClassName, inStrTable.RegisterStr(theDefinition.m_Name),
                inStrTable.RegisterStr(theDefinition.m_ImagePath), theDefinition.m_TexUsageType,
                theDefinition.m_CoordOp, theDefinition.m_MagFilterOp, theDefinition.m_MinFilterOp);
    }
    if (inMetaDataMaterial.m_Shaders.size()) {
        for (QT3DSU32 idx = 0, end = (QT3DSU32)inMetaDataMaterial.m_Shaders.size(); idx < end; ++idx) {
            const qt3dsdm::SMetaDataShader &theShader = inMetaDataMaterial.m_Shaders[idx];
            theConvertStr = Qt3DSStringUtils::ConvertUTFtoQString(
                theShader.m_Code.c_str());
            theConvertShaderTypeStr = Qt3DSStringUtils::ConvertUTFtoQString(
                theShader.m_Type.c_str());
            theConvertShaderVersionStr = Qt3DSStringUtils::ConvertUTFtoQString(
                theShader.m_Version.c_str());
            inMaterialSystem.SetMaterialClassShader(
                inStrTable.RegisterStr(theShader.m_Name.c_str()), theConvertShaderTypeStr.c_str(),
                theConvertShaderVersionStr.c_str(), theConvertStr.c_str(),
                theShader.m_HasGeomShader, theShader.m_IsComputeShader);
        }
    }

    inMaterialSystem.SetCustomMaterialCommands(inClassName,
                                               inMetaDataMaterial.m_CustomMaterialCommands);
    inMaterialSystem.SetCustomMaterialTransparency(inClassName,
                                                   inMetaDataMaterial.m_HasTransparency);
    inMaterialSystem.SetCustomMaterialRefraction(inClassName, inMetaDataMaterial.m_HasRefraction);
    inMaterialSystem.SetCustomMaterialAlwaysDirty(inClassName, inMetaDataMaterial.m_AlwaysDirty);
    inMaterialSystem.SetCustomMaterialShaderKey(inClassName, inMetaDataMaterial.m_ShaderKey);
    inMaterialSystem.SetCustomMaterialLayerCount(inClassName, inMetaDataMaterial.m_LayerCount);
}

#endif
