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
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSRenderContextCore.h"
#include "render/Qt3DSRenderShaderConstant.h"
#include "Qt3DSRenderDynamicObject.h"
#include "foundation/SerializationTypes.h"
#include "foundation/FileTools.h"
#include "foundation/PreAllocatedAllocator.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "StringTools.h"
#include "Qt3DSRenderShaderCache.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSRenderer.h"
#include "Qt3DSRenderDynamicObjectSystemCommands.h"
#include "Qt3DSRenderDynamicObjectSystemUtil.h"
#include "Qt3DSRenderShaderCodeGenerator.h"
#include "foundation/Qt3DSMutex.h"

using namespace qt3ds;
using namespace qt3ds::render;
using namespace qt3ds::render::dynamic;

namespace {
typedef eastl::pair<CRegisteredString, CRegisteredString> TStrStrPair;
}

namespace eastl {
template <>
struct hash<TStrStrPair>
{
    size_t operator()(const TStrStrPair &item) const
    {
        return hash<CRegisteredString>()(item.first) ^ hash<CRegisteredString>()(item.second);
    }
};
}

namespace qt3ds {
namespace render {
    namespace dynamic {

        QT3DSU32 SCommand::GetSizeofCommand(const SCommand &inCommand)
        {
            switch (inCommand.m_Type) {
            case CommandTypes::AllocateBuffer:
                return sizeof(SAllocateBuffer);
            case CommandTypes::BindBuffer:
                return sizeof(SBindBuffer);
            case CommandTypes::BindTarget:
                return sizeof(SBindTarget);
            case CommandTypes::BindShader:
                return sizeof(SBindShader);
            case CommandTypes::Render:
                return sizeof(SRender);
            case CommandTypes::ApplyBufferValue:
                return sizeof(SApplyBufferValue);
            case CommandTypes::ApplyDepthValue:
                return sizeof(SApplyDepthValue);
            case CommandTypes::ApplyInstanceValue:
                return sizeof(SApplyInstanceValue);
            case CommandTypes::ApplyBlending:
                return sizeof(SApplyBlending);
            case CommandTypes::ApplyRenderState:
                return sizeof(SApplyRenderState);
            case CommandTypes::ApplyBlitFramebuffer:
                return sizeof(SApplyBlitFramebuffer);
            case CommandTypes::ApplyValue:
                return sizeof(SApplyValue)
                    + static_cast<const SApplyValue &>(inCommand).m_Value.mSize;
            case CommandTypes::DepthStencil:
                return sizeof(SDepthStencil);
            case CommandTypes::AllocateImage:
                return sizeof(SAllocateImage);
            case CommandTypes::ApplyImageValue:
                return sizeof(SApplyImageValue);
            case CommandTypes::AllocateDataBuffer:
                return sizeof(SAllocateDataBuffer);
            case CommandTypes::ApplyDataBufferValue:
                return sizeof(SApplyDataBufferValue);
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        template <typename TCommandType>
        inline void CopyConstructCommandT(QT3DSU8 *inDataBuffer, const SCommand &inCommand,
                                          IStringTable &inStrTable)
        {
            TCommandType *theCommand = (TCommandType *)inDataBuffer;
            theCommand = new (theCommand)
                TCommandType(static_cast<const TCommandType &>(inCommand), inStrTable);
        }

        void SCommand::CopyConstructCommand(QT3DSU8 *inDataBuffer, const SCommand &inCommand,
                                            IStringTable &inStrTable)
        {
            switch (inCommand.m_Type) {
            case CommandTypes::AllocateBuffer:
                CopyConstructCommandT<SAllocateBuffer>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::BindBuffer:
                CopyConstructCommandT<SBindBuffer>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::BindTarget:
                CopyConstructCommandT<SBindTarget>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::BindShader:
                CopyConstructCommandT<SBindShader>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::Render:
                CopyConstructCommandT<SRender>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyBufferValue:
                CopyConstructCommandT<SApplyBufferValue>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyDepthValue:
                CopyConstructCommandT<SApplyDepthValue>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyInstanceValue:
                CopyConstructCommandT<SApplyInstanceValue>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyBlending:
                CopyConstructCommandT<SApplyBlending>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyRenderState:
                CopyConstructCommandT<SApplyRenderState>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyBlitFramebuffer:
                CopyConstructCommandT<SApplyBlitFramebuffer>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyValue: {
                CopyConstructCommandT<SApplyValue>(inDataBuffer, inCommand, inStrTable);
                SApplyValue &dest = *reinterpret_cast<SApplyValue *>(inDataBuffer);
                QT3DSU8 *destMem = inDataBuffer + sizeof(SApplyValue);
                const SApplyValue &src = static_cast<const SApplyValue &>(inCommand);
                memcpy(destMem, src.m_Value.mData, src.m_Value.mSize);
                dest.m_Value.mData = destMem;
                break;
            }
            case CommandTypes::DepthStencil:
                CopyConstructCommandT<SDepthStencil>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::AllocateImage:
                CopyConstructCommandT<SAllocateImage>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyImageValue:
                CopyConstructCommandT<SApplyImageValue>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::AllocateDataBuffer:
                CopyConstructCommandT<SAllocateDataBuffer>(inDataBuffer, inCommand, inStrTable);
                break;
            case CommandTypes::ApplyDataBufferValue:
                CopyConstructCommandT<SApplyDataBufferValue>(inDataBuffer, inCommand, inStrTable);
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
    }
}
}

namespace {

template <typename TCommandType>
struct SCommandRemapping
{
    template <typename TRemapper>
    static void RemapCommandData(TCommandType &, TRemapper &)
    {
    }
};

template <>
struct SCommandRemapping<SAllocateBuffer>
{
    template <typename TRemapper>
    static void RemapCommandData(SAllocateBuffer &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_Name);
    }
};

template <>
struct SCommandRemapping<SAllocateImage>
{
    template <typename TRemapper>
    static void RemapCommandData(SAllocateImage &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_Name);
    }
};

template <>
struct SCommandRemapping<SAllocateDataBuffer>
{
    template <typename TRemapper>
    static void RemapCommandData(SAllocateDataBuffer &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_Name);
        if (cmd.m_WrapName)
            remapper.Remap(cmd.m_WrapName);
    }
};

template <>
struct SCommandRemapping<SBindBuffer>
{
    template <typename TRemapper>
    static void RemapCommandData(SBindBuffer &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_BufferName);
    }
};
template <>
struct SCommandRemapping<SBindShader>
{
    template <typename TRemapper>
    static void RemapCommandData(SBindShader &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_ShaderPath);
        remapper.Remap(cmd.m_ShaderDefine);
    }
};
template <>
struct SCommandRemapping<SApplyInstanceValue>
{
    template <typename TRemapper>
    static void RemapCommandData(SApplyInstanceValue &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_PropertyName);
    }
};
template <>
struct SCommandRemapping<SApplyBufferValue>
{
    template <typename TRemapper>
    static void RemapCommandData(SApplyBufferValue &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_BufferName);
        remapper.Remap(cmd.m_ParamName);
    }
};

template <>
struct SCommandRemapping<SApplyDepthValue>
{
    template <typename TRemapper>
    static void RemapCommandData(SApplyDepthValue &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_ParamName);
    }
};

template <>
struct SCommandRemapping<SApplyBlitFramebuffer>
{
    template <typename TRemapper>
    static void RemapCommandData(SApplyBlitFramebuffer &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_SourceBufferName);
        remapper.Remap(cmd.m_DestBufferName);
    }
};

template <>
struct SCommandRemapping<SApplyValue>
{
    template <typename TRemapper>
    static void RemapCommandData(SApplyValue &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_PropertyName);
    }
};

template <>
struct SCommandRemapping<SApplyDataBufferValue>
{
    template <typename TRemapper>
    static void RemapCommandData(SApplyDataBufferValue &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_ParamName);
    }
};

template <>
struct SCommandRemapping<SDepthStencil>
{
    template <typename TRemapper>
    static void RemapCommandData(SDepthStencil &cmd, TRemapper &remapper)
    {
        remapper.Remap(cmd.m_BufferName);
    }
};

QT3DSU32 Align(QT3DSU32 inValue)
{
    if (inValue % 4)
        return inValue + (4 - (inValue % 4));
    return inValue;
}

QT3DSU32 Align8(QT3DSU32 inValue)
{
    if (inValue % 8)
        return inValue + (8 - (inValue % 8));
    return inValue;
}

inline const char *GetShaderDatatypeName(NVRenderShaderDataTypes::Enum inValue)
{
    switch (inValue) {
#define HANDLE_QT3DS_SHADER_DATA_TYPE(type)                                                           \
    case NVRenderShaderDataTypes::type:                                                            \
        return #type;
        ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE
    default:
        break;
    }
    QT3DS_ASSERT(false);
    return "";
}

inline qt3ds::QT3DSU32 getSizeofShaderDataType(NVRenderShaderDataTypes::Enum value)
{
    using namespace qt3ds;
    using namespace qt3ds::render;
    switch (value) {
#define HANDLE_QT3DS_SHADER_DATA_TYPE(x)                                                              \
    case NVRenderShaderDataTypes::x:                                                               \
        return sizeof(x);
        ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE
    default:
        break;
    }
    QT3DS_ASSERT(false);
    return 0;
}

struct SDynamicObjectShaderInfo
{
    CRegisteredString m_Type; ///< shader type (GLSL or HLSL)
    CRegisteredString m_Version; ///< shader version (e.g. 330 vor GLSL)
    bool m_HasGeomShader;
    bool m_IsComputeShader;

    SDynamicObjectShaderInfo()
        : m_HasGeomShader(false)
        , m_IsComputeShader(false)
    {
    }
    SDynamicObjectShaderInfo(CRegisteredString inType, CRegisteredString inVersion,
                             bool inHasGeomShader, bool inIsComputeShader)
        : m_Type(inType)
        , m_Version(inVersion)
        , m_HasGeomShader(inHasGeomShader)
        , m_IsComputeShader(inIsComputeShader)
    {
    }
};

struct SDynamicObjClassImpl : public IDynamicObjectClass
{
    NVAllocatorCallback *m_Allocator;
    CRegisteredString m_Id;
    NVConstDataRef<SPropertyDefinition> m_PropertyDefinitions;
    QT3DSU32 m_PropertySectionByteSize;
    QT3DSU32 m_BaseObjectSize;
    GraphObjectTypes::Enum m_GraphObjectType;
    QT3DSU8 *m_PropertyDefaultData;
    NVConstDataRef<SCommand *> m_RenderCommands;
    volatile QT3DSI32 mRefCount;
    bool m_RequiresDepthTexture;
    bool m_RequiresCompilation;
    NVRenderTextureFormats::Enum m_OutputFormat;

    SDynamicObjClassImpl(
        NVAllocatorCallback &alloc, CRegisteredString id,
        NVConstDataRef<SPropertyDefinition> definitions, QT3DSU32 propertySectionByteSize,
        QT3DSU32 baseObjectSize, GraphObjectTypes::Enum objectType, QT3DSU8 *propDefaultData,
        bool inRequiresDepthTexture = false,
        NVRenderTextureFormats::Enum inOutputFormat = NVRenderTextureFormats::RGBA8)
        : m_Allocator(&alloc)
        , m_Id(id)
        , m_PropertyDefinitions(definitions)
        , m_PropertySectionByteSize(propertySectionByteSize)
        , m_BaseObjectSize(baseObjectSize)
        , m_GraphObjectType(objectType)
        , m_PropertyDefaultData(propDefaultData)
        , mRefCount(0)
        , m_RequiresDepthTexture(inRequiresDepthTexture)
        , m_RequiresCompilation(false)
        , m_OutputFormat(inOutputFormat)
    {
    }

    ~SDynamicObjClassImpl()
    {
        if (m_PropertyDefinitions.size()) {
            for (QT3DSU32 idx = 0, end = m_PropertyDefinitions.size(); idx < end; ++idx) {
                if (m_PropertyDefinitions[idx].m_EnumValueNames.size())
                    m_Allocator->deallocate(
                        (void *)m_PropertyDefinitions[idx].m_EnumValueNames.begin());
            }
        }
        ReleaseCommands();
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(*m_Allocator)

    template <typename TRemapperType>
    static void RemapCommand(SCommand &inCommand, TRemapperType &inRemapper)
    {
        switch (inCommand.m_Type) {
#define QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(type)                                              \
    case CommandTypes::type:                                                                       \
        SCommandRemapping<S##type>::RemapCommandData(static_cast<S##type &>(inCommand),            \
                                                     inRemapper);                                  \
        break;
            QT3DS_RENDER_EFFECTS_ITERATE_COMMAND_TYPES
#undef QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES
        default:
            QT3DS_ASSERT(false);
            break;
        }
    }
    template <typename TRemapper>
    void SetupThisObjectFromMemory(NVAllocatorCallback &inAlloc, TRemapper &inRemapper,
                                   QT3DSU8 *inCommandStart, QT3DSU32 numEffectCommands)
    {
        m_Allocator = &inAlloc;
        mRefCount = 0;
        QT3DSU8 *theCommandPtrBegin = inCommandStart;
        QT3DSU32 theCommandOffset = 0;
        for (QT3DSU32 idx = 0; idx < numEffectCommands; ++idx) {
            SCommand *theCommand = reinterpret_cast<SCommand *>(inCommandStart + theCommandOffset);
            theCommandOffset += SCommand::GetSizeofCommand(*theCommand);
        }
        SCommand **theCommandPtrStart =
            reinterpret_cast<SCommand **>(theCommandPtrBegin + theCommandOffset);
        m_RenderCommands = NVConstDataRef<SCommand *>(theCommandPtrStart, numEffectCommands);
        // Now run through the commands, fixup strings and setup the command ptrs
        theCommandOffset = 0;
        for (QT3DSU32 idx = 0; idx < numEffectCommands; ++idx) {
            SCommand *theCommand =
                reinterpret_cast<SCommand *>(theCommandPtrBegin + theCommandOffset);
            theCommandPtrStart[idx] = theCommand;
            RemapCommand(*theCommand, inRemapper);
            theCommandOffset += SCommand::GetSizeofCommand(*theCommand);
        }
    }

    void ReleaseCommands()
    {
        if (m_RenderCommands.size()) {
            m_Allocator->deallocate(const_cast<SCommand *>(*m_RenderCommands.begin()));
            m_RenderCommands = NVConstDataRef<SCommand *>();
        }
    }

    CRegisteredString GetId() const override { return m_Id; }
    NVConstDataRef<SPropertyDefinition> GetProperties() const override
    {
        return m_PropertyDefinitions;
    }
    QT3DSU32 GetPropertySectionByteSize() const override { return m_PropertySectionByteSize; }
    const QT3DSU8 *GetDefaultValueBuffer() const override { return m_PropertyDefaultData; }
    QT3DSU32 GetBaseObjectSize() const override { return m_BaseObjectSize; }
    GraphObjectTypes::Enum GraphObjectType() const override { return m_GraphObjectType; }
    const SPropertyDefinition *FindDefinition(CRegisteredString &str) const
    {
        for (QT3DSU32 idx = 0, end = m_PropertyDefinitions.size(); idx < end; ++idx) {
            const SPropertyDefinition &def(m_PropertyDefinitions[idx]);
            if (def.m_Name == str)
                return &def;
        }
        return NULL;
    }
    const SPropertyDefinition *FindPropertyByName(CRegisteredString inName) const override
    {
        return FindDefinition(inName);
    }
    NVConstDataRef<dynamic::SCommand *> GetRenderCommands() const override
    {
        return m_RenderCommands;
    }
    bool RequiresDepthTexture() const override { return m_RequiresDepthTexture; }
    void SetRequiresDepthTexture(bool inVal) override { m_RequiresDepthTexture = inVal; }
    virtual bool RequiresCompilation() const override { return m_RequiresCompilation; }
    virtual void SetRequiresCompilation(bool inVal) override { m_RequiresCompilation = inVal; }
    NVRenderTextureFormats::Enum GetOutputTextureFormat() const override { return m_OutputFormat; }
};

struct SDataRemapper
{
    template <typename TRemapper>
    void Remap(QT3DSU8 *inData, SPropertyDefinition &item, TRemapper &remapper)
    {
        switch (item.m_DataType) {
        default:
            break; // no remapping necessary
        case NVRenderShaderDataTypes::NVRenderTexture2DPtr:
            CRegisteredString *realData = reinterpret_cast<CRegisteredString *>(inData);
            remapper.Remap(*realData);
            break;
        }
    }
};

struct SShaderMapKey
{
    TStrStrPair m_Name;
    eastl::vector<SShaderPreprocessorFeature> m_Features;
    TessModeValues::Enum m_TessMode;
    bool m_WireframeMode;
    size_t m_HashCode;
    SShaderMapKey(TStrStrPair inName, TShaderFeatureSet inFeatures, TessModeValues::Enum inTessMode,
                  bool inWireframeMode)
        : m_Name(inName)
        , m_Features(inFeatures.begin(), inFeatures.end())
        , m_TessMode(inTessMode)
        , m_WireframeMode(inWireframeMode)
    {
        m_HashCode = eastl::hash<TStrStrPair>()(m_Name)
            ^ HashShaderFeatureSet(toDataRef(m_Features.data(), (QT3DSU32)m_Features.size()))
            ^ eastl::hash<QT3DSU32>()(m_TessMode) ^ eastl::hash<bool>()(m_WireframeMode);
    }
    bool operator==(const SShaderMapKey &inKey) const
    {
        return m_Name == inKey.m_Name && m_Features == inKey.m_Features
            && m_TessMode == inKey.m_TessMode && m_WireframeMode == inKey.m_WireframeMode;
    }
};
}

namespace eastl {
template <>
struct hash<SShaderMapKey>
{
    size_t operator()(const SShaderMapKey &inKey) const { return inKey.m_HashCode; }
};
}

namespace {

typedef nvhash_map<CRegisteredString, NVScopedRefCounted<SDynamicObjClassImpl>> TStringClassMap;
typedef nvhash_map<CRegisteredString, char8_t *> TPathDataMap;
typedef nvhash_map<CRegisteredString, SDynamicObjectShaderInfo> TShaderInfoMap;
typedef nvhash_set<CRegisteredString> TPathSet;
typedef nvhash_map<SShaderMapKey, TShaderAndFlags> TShaderMap;

struct SDynamicObjectSystemCoreImpl : public IDynamicObjectSystem
{
};

struct SDynamicObjectSystemImpl : public IDynamicObjectSystem
{
    NVFoundationBase &m_Foundation;
    mutable qt3ds::render::SPreAllocatedAllocator m_Allocator;
    IQt3DSRenderContextCore &m_CoreContext;
    IQt3DSRenderContext *m_Context;
    TStringClassMap m_Classes;
    TPathDataMap m_ExpandedFiles;
    Qt3DSString m_ShaderKeyBuilder;
    TShaderMap m_ShaderMap;
    TShaderInfoMap m_ShaderInfoMap;
    Qt3DSString m_IncludePath;
    Qt3DSString m_IncludeSearch;
    Qt3DSString m_VertShader;
    Qt3DSString m_FragShader;
    Qt3DSString m_GeometryShader;
    QString m_shaderLibraryVersion;
    QString m_shaderLibraryPlatformDirectory;
    mutable Mutex m_PropertyLoadMutex;
    QT3DSI32 mRefCount;

    SDynamicObjectSystemImpl(IQt3DSRenderContextCore &inCore)
        : m_Foundation(inCore.GetFoundation())
        , m_Allocator(inCore.GetAllocator())
        , m_CoreContext(inCore)
        , m_Context(NULL)
        , m_Classes(inCore.GetAllocator(), "Classes")
        , m_ExpandedFiles(inCore.GetAllocator(), "ExpandedShaderFiles")
        , m_ShaderMap(inCore.GetAllocator(), "ShaderMap")
        , m_ShaderInfoMap(inCore.GetAllocator(), "ShaderInfoMap")
        , m_PropertyLoadMutex(inCore.GetAllocator())
        , mRefCount(0)
    {
        m_IncludeSearch = "#include \"";
    }

    virtual ~SDynamicObjectSystemImpl()
    {
        for (TPathDataMap::iterator iter = m_ExpandedFiles.begin(), end = m_ExpandedFiles.end();
             iter != end; ++iter)
            m_Allocator.deallocate(iter->second);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    bool IsRegistered(CRegisteredString inStr) override
    {
        return m_Classes.find(inStr) != m_Classes.end();
    }

    bool Register(CRegisteredString inName,
                          NVConstDataRef<SPropertyDeclaration> inProperties, QT3DSU32 inBaseObjectSize,
                          GraphObjectTypes::Enum inGraphObjectType) override
    {
        if (IsRegistered(inName)) {
            QT3DS_ASSERT(false);
            return false;
        }
        nvvector<SPropertyDefinition> definitions(m_Foundation.getAllocator(),
                                                  "PropertyDefinitions");
        QT3DSU32 theCurrentOffset = 0;
        for (QT3DSU32 idx = 0, end = inProperties.size(); idx < end; ++idx) {
            const SPropertyDeclaration &thePropDec = inProperties[idx];
            CRegisteredString thePropName(
                m_CoreContext.GetStringTable().RegisterStr(thePropDec.m_Name));
            QT3DSU32 propSize = getSizeofShaderDataType(thePropDec.m_DataType);
            definitions.push_back(SPropertyDefinition(thePropName, thePropDec.m_DataType,
                                                      theCurrentOffset, propSize));
            theCurrentOffset += propSize;
            theCurrentOffset = Align(theCurrentOffset);
        }
        QT3DSU32 dataSectionSize = theCurrentOffset;
        QT3DSU32 clsSize = Align(sizeof(SDynamicObjClassImpl));
        QT3DSU32 defSize = Align(sizeof(SPropertyDefinition) * inProperties.size());
        QT3DSU32 defaultSize = dataSectionSize;
        QT3DSU32 allocSize = clsSize + defSize + defaultSize;
        QT3DSU8 *allocData = reinterpret_cast<QT3DSU8 *>(
            m_Allocator.allocate(allocSize, "SDynamicObjClassImpl", __FILE__, __LINE__));
        QT3DSU8 *defData = allocData + clsSize;
        QT3DSU8 *defaultData = defData + defSize;
        SPropertyDefinition *defPtr = reinterpret_cast<SPropertyDefinition *>(defData);
        if (defSize)
            memCopy(defPtr, definitions.data(), defSize);
        if (defaultSize)
            memZero(defaultData, defaultSize);
        SDynamicObjClassImpl *theClass = new (allocData)
            SDynamicObjClassImpl(m_Allocator, inName, toDataRef(defPtr, inProperties.size()),
                                 dataSectionSize, inBaseObjectSize, inGraphObjectType, defaultData);
        m_Classes.insert(eastl::make_pair(inName, theClass));
        return true;
    }

    bool Unregister(CRegisteredString inName) override {
        if (!IsRegistered(inName)) {
            QT3DS_ASSERT(false);
            return false;
        }
        TStringClassMap::iterator iter = m_Classes.find(inName);
        if (iter != m_Classes.end())
            m_Classes.erase(iter);
        return true;
    }

    SDynamicObjClassImpl *FindClass(CRegisteredString inName)
    {
        TStringClassMap::iterator iter = m_Classes.find(inName);
        if (iter != m_Classes.end())
            return iter->second.mPtr;
        return NULL;
    }

    eastl::pair<const SPropertyDefinition *, SDynamicObjClassImpl *>
    FindProperty(CRegisteredString inName, CRegisteredString inPropName)
    {
        SDynamicObjClassImpl *cls = FindClass(inName);
        if (cls) {
            const SPropertyDefinition *def = cls->FindDefinition(inPropName);
            if (def)
                return eastl::make_pair(def, cls);
        }
        return eastl::pair<const SPropertyDefinition *, SDynamicObjClassImpl *>(NULL, NULL);
    }

    void SetPropertyDefaultValue(CRegisteredString inName, CRegisteredString inPropName,
                                         NVConstDataRef<QT3DSU8> inDefaultData) override
    {
        eastl::pair<const SPropertyDefinition *, SDynamicObjClassImpl *> def =
            FindProperty(inName, inPropName);
        if (def.first && inDefaultData.size() >= def.first->m_ByteSize) {
            memCopy(def.second->m_PropertyDefaultData + def.first->m_Offset, inDefaultData.begin(),
                    def.first->m_ByteSize);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    void SetPropertyEnumNames(CRegisteredString inName, CRegisteredString inPropName,
                                      NVConstDataRef<CRegisteredString> inNames) override
    {

        eastl::pair<const SPropertyDefinition *, SDynamicObjClassImpl *> def =
            FindProperty(inName, inPropName);
        SPropertyDefinition *theDefinitionPtr = const_cast<SPropertyDefinition *>(def.first);
        if (theDefinitionPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        if (theDefinitionPtr->m_EnumValueNames.size()) {
            m_Foundation.getAllocator().deallocate(
                (void *)theDefinitionPtr->m_EnumValueNames.begin());
            theDefinitionPtr->m_EnumValueNames = NVConstDataRef<CRegisteredString>();
        }
        theDefinitionPtr->m_IsEnumProperty = true;
        if (inNames.size()) {
            CRegisteredString *theNameValues = (CRegisteredString *)m_Allocator.allocate(
                inNames.size() * sizeof(CRegisteredString), "PropertyEnumNames", __FILE__,
                __LINE__);

            memCopy(theNameValues, inNames.begin(), inNames.size() * sizeof(CRegisteredString));
            theDefinitionPtr->m_EnumValueNames =
                NVConstDataRef<CRegisteredString>(theNameValues, inNames.size());
        }
    }

    virtual NVConstDataRef<CRegisteredString>
    GetPropertyEnumNames(CRegisteredString inName, CRegisteredString inPropName) const override
    {
        eastl::pair<const SPropertyDefinition *, SDynamicObjClassImpl *> def =
            const_cast<SDynamicObjectSystemImpl &>(*this).FindProperty(inName, inPropName);
        if (def.first)
            return def.first->m_EnumValueNames;
        return NVConstDataRef<CRegisteredString>();
    }

    // Called during loading which is pretty heavily multithreaded.
    virtual NVConstDataRef<dynamic::SPropertyDefinition>
    GetProperties(CRegisteredString inName) const override
    {
        Mutex::ScopedLock __locker(m_PropertyLoadMutex);
        SDynamicObjClassImpl *cls = const_cast<SDynamicObjectSystemImpl &>(*this).FindClass(inName);
        if (cls)
            return cls->m_PropertyDefinitions;
        return NVConstDataRef<dynamic::SPropertyDefinition>();
    }

    void SetPropertyTextureSettings(CRegisteredString inName, CRegisteredString inPropName,
                                            CRegisteredString inPropPath,
                                            NVRenderTextureTypeValue::Enum inTexType,
                                            NVRenderTextureCoordOp::Enum inCoordOp,
                                            NVRenderTextureMagnifyingOp::Enum inMagFilterOp,
                                            NVRenderTextureMinifyingOp::Enum inMinFilterOp) override
    {
        eastl::pair<const SPropertyDefinition *, SDynamicObjClassImpl *> def =
            FindProperty(inName, inPropName);
        SPropertyDefinition *theDefinitionPtr = const_cast<SPropertyDefinition *>(def.first);
        if (theDefinitionPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        theDefinitionPtr->m_ImagePath = inPropPath;
        theDefinitionPtr->m_TexUsageType = inTexType;
        theDefinitionPtr->m_CoordOp = inCoordOp;
        theDefinitionPtr->m_MagFilterOp = inMagFilterOp;
        theDefinitionPtr->m_MinFilterOp = inMinFilterOp;
    }

    IDynamicObjectClass *GetDynamicObjectClass(CRegisteredString inName) override
    {
        return FindClass(inName);
    }

    void SetRenderCommands(CRegisteredString inClassName,
                                   NVConstDataRef<dynamic::SCommand *> inCommands) override
    {
        SDynamicObjClassImpl *theClass =
            const_cast<SDynamicObjectSystemImpl &>(*this).FindClass(inClassName);
        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        theClass->ReleaseCommands();
        QT3DSU32 commandAllocationSize = 0;
        for (QT3DSU32 idx = 0, end = inCommands.size(); idx < end; ++idx) {
            QT3DSU32 commandSize = Align(SCommand::GetSizeofCommand(*inCommands[idx]));
            commandAllocationSize += commandSize;
        }
        QT3DSU32 commandPtrSize = inCommands.size() * sizeof(SCommand *);
        QT3DSU32 totalAllocationSize = Align8(commandAllocationSize) + commandPtrSize;
        QT3DSU8 *theCommandDataBegin = (QT3DSU8 *)m_Allocator.allocate(
            totalAllocationSize, "dynamic::SCommand", __FILE__, __LINE__);
        QT3DSU8 *theCurrentCommandData(theCommandDataBegin);
        SCommand **theCommandPtrBegin =
            reinterpret_cast<SCommand **>(theCommandDataBegin + Align8(commandAllocationSize));
        SCommand **theCurrentCommandPtr = theCommandPtrBegin;
        memZero(theCommandDataBegin, totalAllocationSize);

        theClass->m_RequiresDepthTexture = false;
        for (QT3DSU32 idx = 0, end = inCommands.size(); idx < end; ++idx) {
            SCommand &theCommand(*inCommands[idx]);
            QT3DSU32 theCommandSize = SCommand::GetSizeofCommand(theCommand);
            SCommand::CopyConstructCommand(theCurrentCommandData, theCommand,
                                           m_CoreContext.GetStringTable());
            if (theCommand.m_Type == CommandTypes::ApplyDepthValue)
                theClass->m_RequiresDepthTexture = true;
            if (theCommand.m_Type == CommandTypes::BindTarget) {
                SBindTarget *bt = reinterpret_cast<SBindTarget *>(&theCommand);
                theClass->m_OutputFormat = bt->m_OutputFormat;
            }

            *theCurrentCommandPtr = reinterpret_cast<SCommand *>(theCurrentCommandData);
            ++theCurrentCommandPtr;
            theCurrentCommandData += Align(theCommandSize);
        }
        QT3DS_ASSERT(theCurrentCommandData - theCommandDataBegin == (int)commandAllocationSize);
        QT3DS_ASSERT((QT3DSU8 *)theCurrentCommandPtr - theCommandDataBegin == (int)totalAllocationSize);
        theClass->m_RenderCommands =
            NVConstDataRef<SCommand *>(theCommandPtrBegin, inCommands.size());
    }

    virtual NVConstDataRef<dynamic::SCommand *>
    GetRenderCommands(CRegisteredString inClassName) const override
    {
        SDynamicObjClassImpl *cls =
            const_cast<SDynamicObjectSystemImpl &>(*this).FindClass(inClassName);
        if (cls)
            return cls->m_RenderCommands;
        return NVConstDataRef<dynamic::SCommand *>();
    }

    SDynamicObject *CreateInstance(CRegisteredString inClassName,
                                           NVAllocatorCallback &inSceneGraphAllocator) override
    {
        SDynamicObjClassImpl *theClass = FindClass(inClassName);
        if (!theClass) {
            QT3DS_ASSERT(false);
            return NULL;
        }
        QT3DSU32 totalObjectSize = theClass->m_BaseObjectSize + theClass->m_PropertySectionByteSize;
        SDynamicObject *retval = reinterpret_cast<SDynamicObject *>(inSceneGraphAllocator.allocate(
            totalObjectSize, inClassName.c_str(), __FILE__, __LINE__));
        new (retval)
            SDynamicObject(theClass->m_GraphObjectType, inClassName,
                           theClass->m_PropertySectionByteSize, theClass->m_BaseObjectSize);
        memCopy(retval->GetDataSectionBegin(), theClass->m_PropertyDefaultData,
                theClass->m_PropertySectionByteSize);
        return retval;
    }

    void SetShaderData(CRegisteredString inPath, const char8_t *inData,
                               const char8_t *inShaderType, const char8_t *inShaderVersion,
                               bool inHasGeomShader, bool inIsComputeShader) override
    {
        inData = inData ? inData : "";
        eastl::pair<TPathDataMap::iterator, bool> theInserter =
            m_ExpandedFiles.insert(eastl::make_pair(inPath, (char8_t *)""));
        if (theInserter.second == false) {
            // Delete the existing entry.
            m_Allocator.deallocate(theInserter.first->second);
        }
        QT3DSU32 theLen = (QT3DSU32)strlen(inData) + 1;
        char8_t *newData = (char8_t *)m_Allocator.allocate(
            theLen, "SDynamicObjectSystem::SetShaderData", __FILE__, __LINE__);
        memCopy(newData, inData, theLen);
        theInserter.first->second = newData;

        // set shader type and version if available
        if (inShaderType || inShaderVersion || inHasGeomShader || inIsComputeShader) {
            // UdoL TODO: Add this to the load / save setction
            // In addition we should merge the source code into SDynamicObjectShaderInfo as well
            SDynamicObjectShaderInfo &theShaderInfo =
                m_ShaderInfoMap.insert(eastl::make_pair(inPath, SDynamicObjectShaderInfo()))
                    .first->second;
            IStringTable &theStringTable(m_CoreContext.GetStringTable());
            theShaderInfo.m_Type = theStringTable.RegisterStr(nonNull(inShaderType));
            theShaderInfo.m_Version = theStringTable.RegisterStr(nonNull(inShaderVersion));
            theShaderInfo.m_HasGeomShader = inHasGeomShader;
            theShaderInfo.m_IsComputeShader = inIsComputeShader;
        }

        return;
    }

    CRegisteredString GetShaderCacheKey(const char8_t *inId, const char8_t *inProgramMacro,
                                        const dynamic::SDynamicShaderProgramFlags &inFlags)
    {
        m_ShaderKeyBuilder.assign(inId);
        if (inProgramMacro && *inProgramMacro) {
            m_ShaderKeyBuilder.append("#");
            m_ShaderKeyBuilder.append(inProgramMacro);
        }
        if (inFlags.IsTessellationEnabled()) {
            m_ShaderKeyBuilder.append("#");
            m_ShaderKeyBuilder.append(TessModeValues::toString(inFlags.m_TessMode));
        }
        if (inFlags.IsGeometryShaderEnabled() && inFlags.m_WireframeMode) {
            m_ShaderKeyBuilder.append("#");
            m_ShaderKeyBuilder.append(inFlags.wireframeToString(inFlags.m_WireframeMode));
        }

        return m_CoreContext.GetStringTable().RegisterStr(m_ShaderKeyBuilder.c_str());
    }

    void InsertShaderHeaderInformation(Qt3DSString &theReadBuffer,
                                               const char8_t *inPathToEffect) override
    {
        DoInsertShaderHeaderInformation(theReadBuffer, inPathToEffect);
    }

    void DoInsertShaderHeaderInformation(Qt3DSString &theReadBuffer,
                                         const char8_t *inPathToEffect)
    {
        // Now do search and replace for the headers
        for (Qt3DSString::size_type thePos = theReadBuffer.indexOf(m_IncludeSearch);
             thePos != Qt3DSString::npos;
             thePos = theReadBuffer.indexOf(m_IncludeSearch, thePos + 1)) {
            Qt3DSString::size_type theEndQuote =
                theReadBuffer.indexOf(QLatin1Char('\"'), thePos + m_IncludeSearch.size() + 1);

            // Indicates an unterminated include file.
            if (theEndQuote == Qt3DSString::npos) {
                qCCritical(INVALID_OPERATION, "Unterminated include in file: %s", inPathToEffect);
                theReadBuffer.clear();
                break;
            }
            Qt3DSString::size_type theActualBegin = thePos + m_IncludeSearch.size();
            Qt3DSString::iterator theIncludeBegin = theReadBuffer.begin() + theActualBegin;
            Qt3DSString::iterator theIncludeEnd = theReadBuffer.begin() + theEndQuote;
            m_IncludePath.clear();
            m_IncludePath.append(theIncludeBegin, theIncludeEnd);
            // If we haven't included the file yet this round
            Qt3DSString theIncludeBuffer;
            DoLoadShader(m_IncludePath.c_str(), theIncludeBuffer);
            theReadBuffer =
                theReadBuffer.replace(theReadBuffer.begin() + thePos,
                                      theReadBuffer.begin() + theEndQuote + 1,
                                      theIncludeBuffer);
        }
    }

    void DoLoadShader(const char8_t *inPathToEffect, Qt3DSString &outShaderData)
    {
        eastl::pair<TPathDataMap::iterator, bool> theInsert =
            m_ExpandedFiles.insert(eastl::make_pair(
                m_CoreContext.GetStringTable().RegisterStr(inPathToEffect), (char8_t *)""));

        if (theInsert.second) {

            const QString defaultDir = m_Context->GetDynamicObjectSystem()
                    .GetShaderCodeLibraryDirectory();
            const QString platformDir = m_Context->GetDynamicObjectSystem()
                    .shaderCodeLibraryPlatformDirectory();

            QString fullPath;
            NVScopedRefCounted<IRefCountedInputStream> theStream;
            if (!platformDir.isEmpty()) {
                QTextStream stream(&fullPath);
                stream << platformDir << QLatin1Char('/') << inPathToEffect;
                theStream = m_CoreContext.GetInputStreamFactory()
                        .GetStreamForFile(fullPath.toLatin1().data());
            }

            if (theStream.mPtr == NULL) {
                fullPath.clear();
                QTextStream stream(&fullPath);
                stream << defaultDir << QLatin1Char('/') << inPathToEffect;
                theStream = m_CoreContext.GetInputStreamFactory()
                        .GetStreamForFile(fullPath);
                if (theStream.mPtr == NULL) {
                    fullPath.clear();
                    QTextStream stream(&fullPath);
                    stream << defaultDir << QLatin1Char('/') << inPathToEffect;
                    theStream = m_CoreContext.GetInputStreamFactory()
                            .GetStreamForFile(fullPath);
                }
            }
            if (theStream.mPtr != NULL) {
                QT3DSU8 readBuf[1024];
                QT3DSU32 amountRead = 0;
                do {
                    amountRead = theStream->Read(NVDataRef<QT3DSU8>(readBuf, 1024));
                    if (amountRead)
                        outShaderData.append((const char8_t *)readBuf, amountRead);
                } while (amountRead);
            } else {
                qCCritical(INVALID_OPERATION, "Failed to find include file %s", inPathToEffect);
                QT3DS_ASSERT(false);
            }
            theInsert.first->second = (char8_t *)m_Allocator.allocate(
                outShaderData.size() + 1, "SDynamicObjectSystem::DoLoadShader", __FILE__, __LINE__);
            memCopy(theInsert.first->second, outShaderData.c_str(),
                    QT3DSU32(outShaderData.size()) + 1);
        } else {
            outShaderData.assign(theInsert.first->second);
        }

        DoInsertShaderHeaderInformation(outShaderData, inPathToEffect);
    }

    void Save(qt3ds::render::SWriteBuffer &ioBuffer,
                      const qt3ds::render::SStrRemapMap &inRemapMap, const char8_t *inProjectDir) const override
    {
        QT3DSU32 startOffset = ioBuffer.size();
        ioBuffer.write((QT3DSU32)m_ExpandedFiles.size());
        for (TPathDataMap::const_iterator theIter = m_ExpandedFiles.begin();
             theIter != m_ExpandedFiles.end(); ++theIter) {
            CRegisteredString thePath(theIter->first);
            char8_t *theData = theIter->second;
            theData = theData ? theData : (char8_t *)"";
            QT3DSU32 theLen = (QT3DSU32)strlen(theData);
            thePath.Remap(inRemapMap);
            ioBuffer.write(thePath);
            ioBuffer.write(theLen + 1);
            ioBuffer.write(theData, theLen + 1);
            ioBuffer.align(sizeof(void *));
        }
        ioBuffer.write((QT3DSU32)m_Classes.size());
        SStringSaveRemapper theRemapper(m_Allocator, inRemapMap, inProjectDir,
                                        m_CoreContext.GetStringTable());
        for (TStringClassMap::const_iterator iter = m_Classes.begin(), end = m_Classes.end();
             iter != end; ++iter) {
            const SDynamicObjClassImpl *theClass = iter->second;
            ioBuffer.align(4);
            QT3DSU32 objOffset = ioBuffer.size();
            QT3DSU32 classOffset = objOffset - startOffset;
            (void)classOffset;
            QT3DSU32 defOffset = objOffset + sizeof(SDynamicObjClassImpl);
            QT3DSU32 dataOffset =
                defOffset + theClass->m_PropertyDefinitions.size() * sizeof(SPropertyDefinition);
            QT3DSU32 dataEnd = dataOffset + theClass->m_PropertySectionByteSize;
            QT3DSU32 writeAmount = dataEnd - objOffset;
            ioBuffer.write((const QT3DSU8 *)theClass, writeAmount);
            ioBuffer.align(4);
            QT3DSU32 cmdOffset = 0;
            QT3DSU8 *writeCommandStart = NULL;
            if (theClass->m_RenderCommands.size()) {
                // We know commands are allocated in a block.
                const SCommand &firstCommand = *theClass->m_RenderCommands[0];
                const QT3DSU8 *commandStart = reinterpret_cast<const QT3DSU8 *>(&firstCommand);
                const SCommand &lastCommand(
                    *theClass->m_RenderCommands[theClass->m_RenderCommands.size() - 1]);
                const QT3DSU8 *commandEnd = reinterpret_cast<const QT3DSU8 *>(&lastCommand)
                    + SCommand::GetSizeofCommand(lastCommand);
                cmdOffset = ioBuffer.size();
                ioBuffer.write(commandStart, (QT3DSU32)(commandEnd - commandStart));
                // Write location of the actual storage for the command ptr array.
                ioBuffer.writeZeros(theClass->m_RenderCommands.size() * sizeof(SCommand **));
            }
            ioBuffer.align(4);
            if (cmdOffset)
                writeCommandStart = ioBuffer.begin() + cmdOffset;

            SDynamicObjClassImpl *writeClass =
                (SDynamicObjClassImpl *)(ioBuffer.begin() + objOffset);
            writeClass->m_Id.Remap(inRemapMap);
            writeClass->SetupThisObjectFromMemory(m_Allocator, theRemapper, writeCommandStart,
                                                  theClass->m_RenderCommands.size());
            for (QT3DSU32 idx = 0, end = theClass->m_PropertyDefinitions.size(); idx < end; ++idx) {
                // Moved into the loop because writing the enumerations may resize the data buffer.
                SPropertyDefinition *theDefinitions =
                    (SPropertyDefinition *)(ioBuffer.begin() + defOffset);
                theDefinitions[idx].m_Name.Remap(inRemapMap);
                const SPropertyDefinition &theDefinition(theClass->m_PropertyDefinitions[idx]);
                if (theDefinitions[idx].m_EnumValueNames.size()) {
                    QT3DSU32 enumOffset = ioBuffer.size();
                    ioBuffer.write(theDefinition.m_EnumValueNames.begin(),
                                   theDefinition.m_EnumValueNames.size()
                                       * sizeof(CRegisteredString));
                    CRegisteredString *strPtr =
                        (CRegisteredString *)(ioBuffer.begin() + enumOffset);
                    for (QT3DSU32 enumIdx = 0, enumEnd = theDefinition.m_EnumValueNames.size();
                         enumIdx < enumEnd; ++enumIdx)
                        strPtr[enumIdx].Remap(inRemapMap);
                }
                if (theDefinition.m_DataType == NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                    QT3DSU8 *theDataPtr = ioBuffer.begin() + dataOffset;
                    CRegisteredString *realData =
                        reinterpret_cast<CRegisteredString *>(theDataPtr + theDefinition.m_Offset);
                    realData->Remap(inRemapMap);
                }
            }
        }

        // Write out meta information about the shader system
        QT3DSU32 numShaderInfos = (QT3DSU32)m_ShaderInfoMap.size();
        ioBuffer.write(numShaderInfos);
        for (TShaderInfoMap::const_iterator iter = m_ShaderInfoMap.begin(),
                                            end = m_ShaderInfoMap.end();
             iter != end; ++iter) {
            CRegisteredString infoName = iter->first;
            infoName.Remap(inRemapMap);
            ioBuffer.write(infoName);
            const SDynamicObjectShaderInfo &theInfo = iter->second;
            CRegisteredString infoType(theInfo.m_Type);
            CRegisteredString infoVersion(theInfo.m_Version);
            infoType.Remap(inRemapMap);
            infoVersion.Remap(inRemapMap);
            ioBuffer.write(infoType);
            ioBuffer.write(infoVersion);
        }
    }

    void Load(NVDataRef<QT3DSU8> inData, CStrTableOrDataRef inStrDataBlock,
                      const char8_t *inProjectDir) override
    {
        m_Allocator.m_PreAllocatedBlock = inData;
        m_Allocator.m_OwnsMemory = false;
        TStr workspaceStr(ForwardingAllocator(m_Foundation.getAllocator(), "ProjPath"));
        qt3ds::render::SDataReader theReader(inData.begin(), inData.end());
        QT3DSU32 numMappedPaths = theReader.LoadRef<QT3DSU32>();
        for (QT3DSU32 idx = 0, end = numMappedPaths; idx < end; ++idx) {
            CRegisteredString theStr(theReader.LoadRef<CRegisteredString>());
            QT3DSU32 theCharLen = theReader.LoadRef<QT3DSU32>();
            char8_t *thePathData = reinterpret_cast<char8_t *>(theReader.m_CurrentPtr);
            theReader.m_CurrentPtr += theCharLen;
            theReader.Align();
            theStr.Remap(inStrDataBlock);
            m_ExpandedFiles.insert(eastl::make_pair(theStr, thePathData));
        }
        SStringLoadRemapper theRemapper(m_Allocator, inStrDataBlock, inProjectDir,
                                        m_CoreContext.GetStringTable());
        QT3DSU32 numClasses = theReader.LoadRef<QT3DSU32>();
        for (QT3DSU32 idx = 0, end = numClasses; idx < end; ++idx) {
            theReader.Align(4);
            size_t classOffset = static_cast<QT3DSU32>(theReader.m_CurrentPtr - inData.mData);
            (void)classOffset;
            SDynamicObjClassImpl *theClass = (SDynamicObjClassImpl *)theReader.m_CurrentPtr;
            theClass->m_Allocator = &m_Allocator;
            theReader.m_CurrentPtr += sizeof(SDynamicObjClassImpl);
            SPropertyDefinition *theDefinitions = (SPropertyDefinition *)theReader.m_CurrentPtr;
            theReader.m_CurrentPtr +=
                theClass->m_PropertyDefinitions.size() * sizeof(SPropertyDefinition);
            QT3DSU8 *theDataBuffer = theReader.m_CurrentPtr;
            theReader.m_CurrentPtr += theClass->m_PropertySectionByteSize;
            theClass->m_Id.Remap(inStrDataBlock);
            theClass->m_PropertyDefinitions = NVConstDataRef<SPropertyDefinition>(
                theDefinitions, theClass->m_PropertyDefinitions.size());
            theClass->m_PropertyDefaultData = theDataBuffer;
            theReader.Align(4);
            QT3DSU8 *theCommandStart = theReader.m_CurrentPtr;

            QT3DSU32 numRenderCommands = theClass->m_RenderCommands.size();
            new (theClass) SDynamicObjClassImpl(
                m_Allocator, theClass->m_Id, theClass->m_PropertyDefinitions,
                theClass->m_PropertySectionByteSize, theClass->m_BaseObjectSize,
                theClass->m_GraphObjectType, theClass->m_PropertyDefaultData,
                theClass->m_RequiresDepthTexture, theClass->m_OutputFormat);

            theClass->SetupThisObjectFromMemory(m_Allocator, theRemapper, theCommandStart,
                                                numRenderCommands);

            if (theClass->m_RenderCommands.size()) {
                const SCommand &theLastCommand =
                    *theClass->m_RenderCommands[theClass->m_RenderCommands.size() - 1];
                const QT3DSU8 *theCommandEnd = reinterpret_cast<const QT3DSU8 *>(&theLastCommand)
                    + SCommand::GetSizeofCommand(theLastCommand);
                theReader.m_CurrentPtr = const_cast<QT3DSU8 *>(theCommandEnd);
                theReader.m_CurrentPtr += theClass->m_RenderCommands.size() * sizeof(SCommand **);
            }
            theReader.Align(4);

            for (QT3DSU32 defIdx = 0, defEnd = theClass->m_PropertyDefinitions.size(); defIdx < defEnd;
                 ++defIdx) {
                SPropertyDefinition &theDef(theDefinitions[defIdx]);
                theDef.m_Name.Remap(inStrDataBlock);
                if (theDef.m_EnumValueNames.size()) {
                    CRegisteredString *theNames = (CRegisteredString *)theReader.m_CurrentPtr;
                    theReader.m_CurrentPtr +=
                        theDef.m_EnumValueNames.size() * sizeof(CRegisteredString);
                    theDef.m_EnumValueNames =
                        NVDataRef<CRegisteredString>(theNames, theDef.m_EnumValueNames.size());
                    for (QT3DSU32 enumIdx = 0, enumEnd = theDef.m_EnumValueNames.size();
                         enumIdx < enumEnd; ++enumIdx)
                        theNames[enumIdx].Remap(inStrDataBlock);
                }
                if (theDef.m_DataType == NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                    CRegisteredString *realData =
                        reinterpret_cast<CRegisteredString *>(theDataBuffer + theDef.m_Offset);
                    realData->Remap(inStrDataBlock);
                }
            }
            m_Classes.insert(eastl::make_pair(theClass->m_Id, theClass));
        }
        QT3DSU32 theNumShaderInfos = theReader.LoadRef<QT3DSU32>();
        for (QT3DSU32 idx = 0, end = theNumShaderInfos; idx < end; ++idx) {
            CRegisteredString name, type, version;
            name = theReader.LoadRef<CRegisteredString>();
            type = theReader.LoadRef<CRegisteredString>();
            version = theReader.LoadRef<CRegisteredString>();
            name.Remap(inStrDataBlock);
            type.Remap(inStrDataBlock);
            version.Remap(inStrDataBlock);
            SDynamicObjectShaderInfo &theInfo =
                m_ShaderInfoMap.insert(eastl::make_pair(name, SDynamicObjectShaderInfo()))
                    .first->second;
            theInfo.m_Type = type;
            theInfo.m_Version = version;
        }
    }

    IDynamicObjectSystem &CreateDynamicSystem(IQt3DSRenderContext &rc) override
    {
        m_Context = &rc;
        return *this;
    }

    QStringList getParameters(const QString &str, int begin, int end)
    {
        const QString s = str.mid(begin, end - begin + 1);
        return s.split(",");
    }

    void insertSnapperDirectives(QString &str)
    {
        int beginIndex = 0;
        // Snapper macros:
        //  #define SNAPPER_SAMPLER2D(propName, propNiceName, texFilter, texWrap, showUI ) \
        //      uniform sampler2D propName;                                     \
        //      uniform int flag##propName;                                     \
        //      uniform vec4 propName##Info;                                    \
        //      vec4 texture2D_##propName(vec2 uv)                              \
        //      {                                                               \
        //          return GetTextureValue( propName, uv, propName##Info.z );   \
        //      }
        //
        //  #define SNAPPER_SAMPLER2DWITHDEFAULT(propName, propNiceName, texFilter, texWrap, defaultPath, showUI ) \
        //      SNAPPER_SAMPLER2D( propName, propNiceName, texFilter, texWrap, showUI )
        //
        //  #define SNAPPER_SAMPLERCUBE(propName, propNiceName, texFilter, texWrap ) \
        //      uniform samplerCube propName;   \
        //      uniform vec2 propName##UVRange; \
        //      uniform int flag##propName;     \
        //      uniform vec2 propName##Size;
        QString snapperSampler = QStringLiteral("SNAPPER_SAMPLER2D(");
        QString snapperSamplerDefault = QStringLiteral("SNAPPER_SAMPLER2DWITHDEFAULT(");
        QString snapperSamplerCube = QStringLiteral("SNAPPER_SAMPLERCUBE(");
        QString endingBracket = QStringLiteral(")");

        while ((beginIndex = str.indexOf(snapperSampler, beginIndex)) >= 0) {
            int endIndex = str.indexOf(endingBracket, beginIndex);
            const QStringList list = getParameters(str, beginIndex + snapperSampler.length(),
                                                   endIndex);
            str.remove(beginIndex, endIndex - beginIndex + 1);
            if (list.size() == 5) {
                QString insertStr;
                QTextStream stream(&insertStr);
                stream << "uniform sampler2D " << list[0] << ";\n";
                stream << "uniform int flag" << list[0] << ";\n";
                stream << "vec4 " << list[0] << "Info;\n";
                stream << "vec4 texture2D_" << list[0] << "(vec2 uv) "
                       << "{ return GetTextureValue( " << list[0] << ", uv, "
                       << list[0] <<"Info.z ); }\n";
                str.insert(beginIndex, insertStr);
            }
        }
        beginIndex = 0;
        while ((beginIndex = str.indexOf(snapperSamplerDefault, beginIndex)) >= 0) {
            int endIndex = str.indexOf(endingBracket, beginIndex);
            const QStringList list = getParameters(str, beginIndex + snapperSamplerDefault.length(),
                                                   endIndex);
            str.remove(beginIndex, endIndex - beginIndex + 1);
            if (list.size() == 5) {
                QString insertStr;
                QTextStream stream(&insertStr);
                stream << "uniform sampler2D " << list[0] << ";\n";
                stream << "uniform int flag" << list[0] << ";\n";
                stream << "vec4 " << list[0] << "Info;\n";
                stream << "vec4 texture2D_" << list[0] << "(vec2 uv) "
                       << "{ return GetTextureValue( " << list[0] << ", uv, "
                       << list[0] <<"Info.z ); }\n";
                str.insert(beginIndex, insertStr);
            }
        }
        beginIndex = 0;
        while ((beginIndex = str.indexOf(snapperSamplerCube, beginIndex)) >= 0) {
            int endIndex = str.indexOf(endingBracket, beginIndex);
            const QStringList list = getParameters(str, beginIndex + snapperSamplerCube.length(),
                                                   endIndex);
            str.remove(beginIndex, endIndex - beginIndex + 1);
            if (list.size() == 4) {
                QString insertStr;
                QTextStream stream(&insertStr);
                stream << "uniform samplerCube " << list[0] << ";\n";
                stream << "uniform vec2 "<< list[0] << "UVRange;\n";
                stream << "uniform int flag" << list[0] << ";\n";
                stream << "uniform vec2 "<< list[0] << "Size;\n";
                str.insert(beginIndex, insertStr);
            }
        }
    }

    NVRenderShaderProgram *CompileShader(CRegisteredString inId, const char8_t *inProgramSource,
                                         const char8_t *inGeomSource,
                                         CRegisteredString inProgramMacroName,
                                         TShaderFeatureSet inFeatureSet,
                                         const dynamic::SDynamicShaderProgramFlags &inFlags,
                                         bool inForceCompilation = false)
    {
        m_VertShader.clear();
        m_FragShader.clear();
        m_GeometryShader.clear();
        SShaderCacheProgramFlags theFlags;

        m_VertShader.append("#define VERTEX_SHADER\n");
        m_FragShader.append("#define FRAGMENT_SHADER\n");

        if (inProgramMacroName.IsValid()) {
            m_VertShader.append("#define ");
            m_VertShader.append(inProgramMacroName.c_str());
            m_VertShader.append("\n");

            m_FragShader.append("#define ");
            m_FragShader.append(inProgramMacroName.c_str());
            m_FragShader.append("\n");
        }

        if (inGeomSource && inFlags.IsGeometryShaderEnabled()) {
            theFlags.SetGeometryShaderEnabled(true);

            m_GeometryShader.append("#define GEOMETRY_SHADER 1\n");
            m_GeometryShader.append(inGeomSource);

            m_VertShader.append("#define GEOMETRY_SHADER 1\n");
        } else if (inFlags.IsGeometryShaderEnabled()) {
            theFlags.SetGeometryShaderEnabled(true);
            m_GeometryShader.append("#define USER_GEOMETRY_SHADER 1\n");
            m_GeometryShader.append(inProgramSource);
            m_VertShader.append("#define GEOMETRY_SHADER 0\n");
            m_FragShader.append("#define GEOMETRY_WIREFRAME_SHADER 0\n");
        } else {
            m_VertShader.append("#define GEOMETRY_SHADER 0\n");
            m_FragShader.append("#define GEOMETRY_WIREFRAME_SHADER 0\n");
        }

        if (strstr(inProgramSource, "SNAPPER_SAMPLER")) {
            QString programSource(inProgramSource);
            insertSnapperDirectives(programSource);
            QByteArray data = programSource.toLatin1();
            const char *source = data.constData();

            m_VertShader.append(source);
            m_FragShader.append(source);
        } else {
            m_VertShader.append(inProgramSource);
            m_FragShader.append(inProgramSource);
        }

        IShaderCache &theShaderCache = m_Context->GetShaderCache();

        CRegisteredString theKey = m_Context->GetStringTable().RegisterStr(
            GetShaderCacheKey(inId, inProgramMacroName, inFlags));

        if (inForceCompilation) {
            return theShaderCache.ForceCompileProgram(theKey, m_VertShader.c_str(),
                                                      m_FragShader.c_str(), NULL, NULL,
                                                      m_GeometryShader.c_str(), theFlags,
                                                      inFeatureSet, false);
        }
        return theShaderCache.CompileProgram(theKey, m_VertShader.c_str(), m_FragShader.c_str(),
                                             NULL, NULL, m_GeometryShader.c_str(), theFlags,
                                             inFeatureSet);
    }

    // This just returns the custom material shader source without compiling
    void GetShaderSource(CRegisteredString inPath, Qt3DSString &outBuffer) override
    {
        outBuffer.clear();
        outBuffer.append("#define FRAGMENT_SHADER\n");

        Qt3DSString source;
        DoLoadShader(inPath, outBuffer);
    }

    TShaderAndFlags GetShaderProgram(CRegisteredString inPath,
                                             CRegisteredString inProgramMacro,
                                             TShaderFeatureSet inFeatureSet,
                                             const SDynamicShaderProgramFlags &inFlags,
                                             bool inForceCompilation) override
    {
        eastl::pair<const SShaderMapKey, TShaderAndFlags> theInserter(
            SShaderMapKey(TStrStrPair(inPath, inProgramMacro), inFeatureSet, inFlags.m_TessMode,
                          inFlags.m_WireframeMode),
            TShaderAndFlags());
        eastl::pair<TShaderMap::iterator, bool> theInsertResult(m_ShaderMap.insert(theInserter));
        if (theInsertResult.second || inForceCompilation) {
            NVRenderShaderProgram *theProgram = m_Context->GetShaderCache().GetProgram(
                GetShaderCacheKey(inPath, inProgramMacro, inFlags), inFeatureSet);
            SDynamicShaderProgramFlags theFlags(inFlags);
            if (!theProgram || inForceCompilation) {
                SDynamicObjectShaderInfo &theShaderInfo =
                    m_ShaderInfoMap.insert(eastl::make_pair(inPath, SDynamicObjectShaderInfo()))
                        .first->second;
                if (theShaderInfo.m_IsComputeShader == false) {
                    Qt3DSString theShaderBuffer;
                    DoLoadShader(inPath, theShaderBuffer);
                    if (theShaderInfo.m_HasGeomShader)
                        theFlags.SetGeometryShaderEnabled(true);
                    theProgram = CompileShader(inPath, theShaderBuffer.c_str(), NULL, inProgramMacro,
                                               inFeatureSet, theFlags, inForceCompilation);
                } else {
                    Qt3DSString theShaderBuffer;
                    const char8_t *shaderVersionStr = "#version 430\n";
                    if ((QT3DSU32)m_Context->GetRenderContext().GetRenderContextType()
                        == NVRenderContextValues::GLES3PLUS)
                        shaderVersionStr = "#version 310 es\n";
                    DoLoadShader(inPath, theShaderBuffer);
                    theShaderBuffer.insert(0, shaderVersionStr);
                    const char8_t *programSource = theShaderBuffer.c_str();
                    QT3DSU32 len = (QT3DSU32)strlen(nonNull(programSource)) + 1;
                    theProgram =
                        m_Context->GetRenderContext()
                            .CompileComputeSource(inPath.c_str(),
                                                  NVConstDataRef<QT3DSI8>((QT3DSI8 *)programSource, len))
                            .mShader;
                }
            }
            theInsertResult.first->second = TShaderAndFlags(theProgram, theFlags);
        }
        return theInsertResult.first->second;
    }

    TShaderAndFlags GetDepthPrepassShader(CRegisteredString inPath,
                                                  CRegisteredString inPMacro,
                                                  TShaderFeatureSet inFeatureSet) override
    {
        SDynamicObjectShaderInfo &theShaderInfo =
            m_ShaderInfoMap.insert(eastl::make_pair(inPath, SDynamicObjectShaderInfo()))
                .first->second;
        if (theShaderInfo.m_HasGeomShader == false)
            return TShaderAndFlags();
        // else, here we go...
        SDynamicShaderProgramFlags theFlags;
        m_ShaderKeyBuilder.assign(inPMacro.c_str());
        m_ShaderKeyBuilder.append("depthprepass");

        CRegisteredString theProgramMacro =
            m_Context->GetStringTable().RegisterStr(m_ShaderKeyBuilder.c_str());

        eastl::pair<const SShaderMapKey, TShaderAndFlags> theInserter(
            SShaderMapKey(TStrStrPair(inPath, theProgramMacro), inFeatureSet, theFlags.m_TessMode,
                          theFlags.m_WireframeMode),
            TShaderAndFlags());
        eastl::pair<TShaderMap::iterator, bool> theInsertResult(m_ShaderMap.insert(theInserter));
        if (theInsertResult.second) {
            NVRenderShaderProgram *theProgram = m_Context->GetShaderCache().GetProgram(
                GetShaderCacheKey(inPath, theProgramMacro, theFlags), inFeatureSet);
            SDynamicShaderProgramFlags flags(theFlags);
            if (!theProgram) {
                Qt3DSString theShaderBuffer;
                DoLoadShader(inPath, theShaderBuffer);
                SShaderVertexCodeGenerator vertexShader(
                    m_Context->GetStringTable(), m_Allocator,
                    m_Context->GetRenderContext().GetRenderContextType());
                SShaderFragmentCodeGenerator fragmentShader(
                    vertexShader, m_Allocator,
                    m_Context->GetRenderContext().GetRenderContextType());

                vertexShader.AddAttribute("attr_pos", "vec3");
                vertexShader.AddUniform("model_view_projection", "mat4");
                vertexShader.Append("void main() {");
                vertexShader.Append("\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
                vertexShader.Append("}");
                fragmentShader.Append("void main() {");
                fragmentShader.Append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
                fragmentShader.Append("}");
                const char8_t *vertexSource = vertexShader.BuildShaderSource();
                const char8_t *fragmentSource = fragmentShader.BuildShaderSource();

                Qt3DSString programBuffer;
                programBuffer.assign("#ifdef VERTEX_SHADER\n");
                programBuffer.append(vertexSource);
                programBuffer.append("\n#endif\n");
                programBuffer.append("\n#ifdef FRAGMENT_SHADER\n");
                programBuffer.append(fragmentSource);
                programBuffer.append("\n#endif");
                flags.SetGeometryShaderEnabled(true);
                theProgram = CompileShader(inPath, programBuffer.c_str(), theShaderBuffer.c_str(),
                                           theProgramMacro, inFeatureSet, flags);
            }
            theInsertResult.first->second = TShaderAndFlags(theProgram, flags);
        }
        return theInsertResult.first->second;
    }

    virtual void setShaderCodeLibraryPlatformDirectory(const QString &directory) override
    {
        m_shaderLibraryPlatformDirectory = directory;
    }

    virtual QString shaderCodeLibraryPlatformDirectory() override
    {
        return m_shaderLibraryPlatformDirectory;
    }
};
}

IDynamicObjectSystemCore &
IDynamicObjectSystemCore::CreateDynamicSystemCore(IQt3DSRenderContextCore &rc)
{
    return *QT3DS_NEW(rc.GetAllocator(), SDynamicObjectSystemImpl)(rc);
}
