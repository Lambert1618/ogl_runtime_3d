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
#ifdef _WIN32
#pragma warning(disable : 4103)
#endif
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMXML.h"
#include "foundation/Qt3DSAssert.h"
#include "StandardExtensions.h"
#include <unordered_map>
#include <memory>
#include <unordered_set>
#include "Qt3DSDMTransactions.h"
#include "VectorTransactions.h"
#include "Qt3DSDMSignals.h"
// Pull in the fancy str-type implementations
#include "Qt3DSDMWStrOpsImpl.h"
#include "Qt3DSDMDataCore.h"
#include "DataCoreProducer.h"
#include "Qt3DSDMWindowsCompatibility.h"
#include "Qt3DSRenderEffectSystem.h"
#include "Qt3DSRenderDynamicObjectSystemCommands.h"
#include "foundation/StringConversionImpl.h"

#include <QtCore/qdir.h>

using namespace qt3dsdm;
using std::shared_ptr;
using std::make_shared;
using std::static_pointer_cast;
using std::unordered_map;
using std::unordered_set;
using std::function;
using std::bind;
using std::ref;
using std::get;
using qt3ds::foundation::Empty;

typedef Qt3DSDMInstanceHandle TInstanceHandle;
typedef Qt3DSDMPropertyHandle TPropertyHandle;
typedef Qt3DSDMEventHandle TEventHandle;
typedef Qt3DSDMHandlerHandle THandlerHandle;
typedef Qt3DSDMHandlerArgHandle THandlerArgHandle;
typedef Qt3DSDMMetaDataPropertyHandle TMetaDataPropertyHandle;
typedef Qt3DSDMCategoryHandle TCategoryHandle;

namespace qt3dsdm {
#define QT3DS_WCHAR_T_None L"None"
#define QT3DS_WCHAR_T_StringList L"StringList"
#define QT3DS_WCHAR_T_Range L"Range"
#define QT3DS_WCHAR_T_Image L"Image"
#define QT3DS_WCHAR_T_Color L"Color"
#define QT3DS_WCHAR_T_Rotation L"Rotation"
#define QT3DS_WCHAR_T_Font L"Font"
#define QT3DS_WCHAR_T_FontSize L"FontSize"
#define QT3DS_WCHAR_T_MultiLine L"MultiLine"
#define QT3DS_WCHAR_T_ObjectRef L"ObjectRef"
#define QT3DS_WCHAR_T_Mesh L"Mesh"
#define QT3DS_WCHAR_T_Import L"Import"
#define QT3DS_WCHAR_T_Texture L"Texture"
#define QT3DS_WCHAR_T_Image2D L"Image2D"
#define QT3DS_WCHAR_T_Buffer L"Buffer"
#define QT3DS_WCHAR_T_Property L"Property"
#define QT3DS_WCHAR_T_Dependent L"Dependent"
#define QT3DS_WCHAR_T_Slide L"Slide"
#define QT3DS_WCHAR_T_Event L"Event"
#define QT3DS_WCHAR_T_Object L"Object"
#define QT3DS_WCHAR_T_Signal L"Signal"
#define QT3DS_WCHAR_T_Renderable L"Renderable"
#define QT3DS_WCHAR_T_PathBuffer L"PathBuffer"
#define QT3DS_WCHAR_T_ShadowMapResolution L"ShadowMapResolution"
#define QT3DS_WCHAR_T_String L"String"

#define ITERATE_ADDITIONAL_META_DATA_TYPES                                                         \
    HANDLE_ADDITIONAL_META_DATA_TYPE(None)                                                         \
    HANDLE_ADDITIONAL_META_DATA_TYPE(StringList)                                                   \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Range)                                                        \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Image)                                                        \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Color)                                                        \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Rotation)                                                     \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Font)                                                         \
    HANDLE_ADDITIONAL_META_DATA_TYPE(FontSize)                                                     \
    HANDLE_ADDITIONAL_META_DATA_TYPE(MultiLine)                                                    \
    HANDLE_ADDITIONAL_META_DATA_TYPE(ObjectRef)                                                    \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Mesh)                                                         \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Import)                                                       \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Texture)                                                      \
    HANDLE_ADDITIONAL_META_DATA_TYPE(Renderable)                                                   \
    HANDLE_ADDITIONAL_META_DATA_TYPE(PathBuffer)                                                   \
    HANDLE_ADDITIONAL_META_DATA_TYPE(ShadowMapResolution)                                          \
    HANDLE_ADDITIONAL_META_DATA_TYPE(String)
template <>
struct WStrOps<AdditionalMetaDataType::Value>
{
    QT3DSU32 ToStr(AdditionalMetaDataType::Value item, NVDataRef<wchar_t> buffer)
    {
        switch (item) {
#define HANDLE_ADDITIONAL_META_DATA_TYPE(name)                                                     \
        case AdditionalMetaDataType::name:                                                         \
    wcscpy_s(buffer.begin(), buffer.size(), QT3DS_WCHAR_T_##name);                                 \
    return 1;
        ITERATE_ADDITIONAL_META_DATA_TYPES
        #undef HANDLE_ADDITIONAL_META_DATA_TYPE
        }
        return 0;
    }
    bool StrTo(const wchar_t *buffer, AdditionalMetaDataType::Value &item)
    {
#define HANDLE_ADDITIONAL_META_DATA_TYPE(name)                                                     \
    if (AreEqual(buffer, QT3DS_WCHAR_T_##name)) {                                                  \
    item = AdditionalMetaDataType::name;                                                           \
    return true;                                                                                   \
    }
        ITERATE_ADDITIONAL_META_DATA_TYPES
        #undef HANDLE_ADDITIONAL_META_DATA_TYPE
                return false;
    }
};

#undef ITERATE_ADDITIONAL_META_DATA_TYPES

#define ITERATE_HANDLER_ARG_TYPES                                                                  \
    HANDLE_HANDLER_ARG_TYPE(None)                                                                  \
    HANDLE_HANDLER_ARG_TYPE(Property)                                                              \
    HANDLE_HANDLER_ARG_TYPE(Dependent)                                                             \
    HANDLE_HANDLER_ARG_TYPE(Slide)                                                                 \
    HANDLE_HANDLER_ARG_TYPE(Event)                                                                 \
    HANDLE_HANDLER_ARG_TYPE(Object)                                                                \
    HANDLE_HANDLER_ARG_TYPE(Signal)

QT3DSU32 WStrOps<HandlerArgumentType::Value>::ToStr(HandlerArgumentType::Value item, NVDataRef<wchar_t> buffer)
{
    switch (item) {
#define HANDLE_HANDLER_ARG_TYPE(name)                                                              \
    case HandlerArgumentType::name:                                                                \
    wcscpy_s(buffer.begin(), buffer.size(), QT3DS_WCHAR_T_##name);                                 \
    return 1;
    ITERATE_HANDLER_ARG_TYPES
        #undef HANDLE_HANDLER_ARG_TYPE
    }
    return 0;
}

bool WStrOps<HandlerArgumentType::Value>::StrTo(const wchar_t *buffer, HandlerArgumentType::Value &item)
{
#define HANDLE_HANDLER_ARG_TYPE(name)                                                              \
    if (AreEqual(buffer, QT3DS_WCHAR_T_##name)) {                                                  \
    item = HandlerArgumentType::name;                                                              \
    return true;                                                                                   \
}
    ITERATE_HANDLER_ARG_TYPES
        #undef HANDLE_HANDLER_ARG_TYPE
            return false;
}

#define QT3DS_WCHAR_T_FloatRange L"FloatRange"
#define QT3DS_WCHAR_T_LongRange L"LongRange"
#define QT3DS_WCHAR_T_Vector L"Vector"
#define QT3DS_WCHAR_T_MultiLineString L"MultiLineString"
#define QT3DS_WCHAR_T_Boolean L"Boolean"
#define QT3DS_WCHAR_T_Guid L"Guid"
#define QT3DS_WCHAR_T_StringListOrInt L"StringListOrInt"
#define QT3DS_WCHAR_T_Scale L"Scale"

#define ITERATE_QT3DSDM_COMPLETE_TYPES                                                             \
    HANDLE_QT3DSDM_COMPLETE_NONE_TYPE                                                              \
    HANDLE_QT3DSDM_COMPLETE_TYPE(StringList, StringList, DataModelDataType::String)                \
    HANDLE_QT3DSDM_COMPLETE_TYPE(FloatRange, Range, DataModelDataType::Float)                      \
    HANDLE_QT3DSDM_COMPLETE_TYPE(LongRange, Range, DataModelDataType::Long)                        \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Float, None, DataModelDataType::Float)                            \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Long, None, DataModelDataType::Long)                              \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Float2, None, DataModelDataType::Float2)                          \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Vector, None, DataModelDataType::Float3)                          \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Scale, None, DataModelDataType::Float3)                           \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Rotation, Rotation, DataModelDataType::Float3)                    \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Color, Color, DataModelDataType::Float4)                          \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Boolean, None, DataModelDataType::Bool)                           \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Slide, None, DataModelDataType::String)                           \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Font, Font, DataModelDataType::String)                            \
    HANDLE_QT3DSDM_COMPLETE_TYPE(FontSize, FontSize, DataModelDataType::Float)                     \
    HANDLE_QT3DSDM_COMPLETE_TYPE(String, String, DataModelDataType::String)                        \
    HANDLE_QT3DSDM_COMPLETE_TYPE(MultiLineString, MultiLine, DataModelDataType::String)            \
    HANDLE_QT3DSDM_COMPLETE_TYPE(ObjectRef, ObjectRef, DataModelDataType::ObjectRef)               \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Image, Image, DataModelDataType::Long4)                           \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Mesh, Mesh, DataModelDataType::String)                            \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Import, Import, DataModelDataType::String)                        \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Texture, Texture, DataModelDataType::String)                      \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Image2D, Texture, DataModelDataType::String)                      \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Buffer, Texture, DataModelDataType::String)                       \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Guid, None, DataModelDataType::Long4)                             \
    HANDLE_QT3DSDM_COMPLETE_TYPE(StringListOrInt, StringList, DataModelDataType::StringOrInt)      \
    HANDLE_QT3DSDM_COMPLETE_TYPE(Renderable, Renderable, DataModelDataType::String)                \
    HANDLE_QT3DSDM_COMPLETE_TYPE(PathBuffer, PathBuffer, DataModelDataType::String)                \
    HANDLE_QT3DSDM_COMPLETE_TYPE(ShadowMapResolution, ShadowMapResolution, DataModelDataType::Long)

DataModelDataType::Value CompleteMetaDataType::ToDataType(CompleteMetaDataType::Enum inCompleteType)
{
    switch (inCompleteType) {
#define HANDLE_QT3DSDM_COMPLETE_NONE_TYPE                                                             \
    case Unknown:                                                                                  \
    return DataModelDataType::None;
#define HANDLE_QT3DSDM_COMPLETE_TYPE(name, addtype, dtype)                                            \
    case name:                                                                                     \
    return dtype;
    ITERATE_QT3DSDM_COMPLETE_TYPES
        #undef HANDLE_QT3DSDM_COMPLETE_NONE_TYPE
        #undef HANDLE_QT3DSDM_COMPLETE_TYPE
    }
    QT3DS_ASSERT(false);
    return DataModelDataType::None;
}

AdditionalMetaDataType::Value
CompleteMetaDataType::ToAdditionalType(CompleteMetaDataType::Enum inCompleteType)
{
    switch (inCompleteType) {
#define HANDLE_QT3DSDM_COMPLETE_NONE_TYPE                                                             \
    case Unknown:                                                                                  \
    return AdditionalMetaDataType::None;
#define HANDLE_QT3DSDM_COMPLETE_TYPE(name, addtype, dtype)                                            \
    case name:                                                                                     \
    return AdditionalMetaDataType::addtype;
    ITERATE_QT3DSDM_COMPLETE_TYPES
        #undef HANDLE_QT3DSDM_COMPLETE_NONE_TYPE
        #undef HANDLE_QT3DSDM_COMPLETE_TYPE
    }
    QT3DS_ASSERT(false);
    return AdditionalMetaDataType::None;
}

CompleteMetaDataType::Enum
CompleteMetaDataType::ToCompleteType(DataModelDataType::Value inDataType,
                                     AdditionalMetaDataType::Value inAdditionalType)
{
#define HANDLE_QT3DSDM_COMPLETE_NONE_TYPE                                                             \
    if (inDataType == DataModelDataType::None)                                                      \
    return CompleteMetaDataType::Unknown;
#define HANDLE_QT3DSDM_COMPLETE_TYPE(name, addtype, dtype)                                            \
    if (inDataType == dtype                                                    \
    && inAdditionalType == AdditionalMetaDataType::addtype)                                    \
    return CompleteMetaDataType::name;

    ITERATE_QT3DSDM_COMPLETE_TYPES
        #undef HANDLE_QT3DSDM_COMPLETE_NONE_TYPE
        #undef HANDLE_QT3DSDM_COMPLETE_TYPE
            QT3DS_ASSERT(false);
    return CompleteMetaDataType::Unknown;
}

QT3DSU32 WStrOps<CompleteMetaDataType::Enum>::ToStr(CompleteMetaDataType::Enum item,
                                                    NVDataRef<wchar_t> buffer)
{
    switch (item) {
#define HANDLE_QT3DSDM_COMPLETE_NONE_TYPE                                                             \
    case CompleteMetaDataType::Unknown:                                                            \
    wcscpy_s(buffer.begin(), buffer.size(), L"None");                                          \
    return 1;
#define HANDLE_QT3DSDM_COMPLETE_TYPE(name, addtype, dtype)                                            \
    case CompleteMetaDataType::name:                                                               \
    wcscpy_s(buffer.begin(), buffer.size(), QT3DS_WCHAR_T_##name);                               \
    return 1;

    ITERATE_QT3DSDM_COMPLETE_TYPES
        #undef HANDLE_QT3DSDM_COMPLETE_NONE_TYPE
        #undef HANDLE_QT3DSDM_COMPLETE_TYPE
    }
    QT3DS_ASSERT(false);
    return 0;
}
bool WStrOps<CompleteMetaDataType::Enum>::StrTo(const wchar_t *buffer,
                                                CompleteMetaDataType::Enum &item)
{

#define HANDLE_QT3DSDM_COMPLETE_NONE_TYPE                                                             \
    if (AreEqual(buffer, L"None")) {                                                               \
    item = CompleteMetaDataType::Unknown;                                                      \
    return true;                                                                               \
}
#define HANDLE_QT3DSDM_COMPLETE_TYPE(name, addtype, dtype)                                            \
    if (AreEqual(buffer, QT3DS_WCHAR_T_##name)) {                                                    \
    item = CompleteMetaDataType::name;                                                         \
    return true;                                                                               \
}

    ITERATE_QT3DSDM_COMPLETE_TYPES
        #undef HANDLE_QT3DSDM_COMPLETE_NONE_TYPE
        #undef HANDLE_QT3DSDM_COMPLETE_TYPE
            return false;
}
}

namespace {

#ifndef QT3DSDM_META_DATA_NO_SIGNALS
#define CONNECT(x) std::make_shared<qt3dsdm::QtSignalConnection>(QObject::connect(this, x, inCallback))
#else
#define CONNECT(x) std::shared_ptr<qt3dsdm::ISignalConnection>()

struct SNullFunc
{
    template <typename TArgType>
    void operator()(TArgType)
    {
    }
    template <typename TA1, typename TA2>
    void operator()(TA1, TA2)
    {
    }
};

#endif

typedef TCharStr TStrType;
using std::hash;

struct InstanceHandleVecHash
{
    size_t operator()(const vector<TInstanceHandle> &inInstances) const
    {
        size_t retval = 0;
        for (size_t idx = 0, end = inInstances.size(); idx < end; ++idx)
            retval = retval ^ hash<int>()(inInstances[idx]);
        return retval;
    }
};

struct SEventAndHandlerBase
{
    Qt3DSDMInstanceHandle m_Instance;
    TStrType m_Name;
    TStrType m_FormalName;
    TStrType m_Category;
    TStrType m_Description;
    SEventAndHandlerBase() {}
    SEventAndHandlerBase(Qt3DSDMInstanceHandle hdl)
        : m_Instance(hdl)
    {
    }
};

struct SEvent : public SEventAndHandlerBase
{
    SEvent() {}
    SEvent(Qt3DSDMInstanceHandle hdl)
        : SEventAndHandlerBase(hdl)
    {
    }
};

struct SHandler : public SEventAndHandlerBase
{
    SHandler() {}
    SHandler(Qt3DSDMInstanceHandle hdl)
        : SEventAndHandlerBase(hdl)
    {
    }
    vector<SMetaDataHandlerArgumentInfo> m_Arguments;
};

// Note that this hash item only works for strings that are in the string table.
// These have the property that pointer comparison also indicates string equality.
struct SInstanceStrHash
{
    size_t operator()(const pair<TInstanceHandle, TCharPtr> &inPair) const
    {
        return hash<int>()(inPair.first) ^ hash<const void *>()(inPair.second);
    }
};

template <typename TDataType>
NVConstDataRef<TDataType> VecToCRef(const eastl::vector<TDataType> &inType)
{
    return NVConstDataRef<TDataType>(inType.data(), (QT3DSU32)inType.size());
}

struct SMetaDataDynamicObjectImpl
{
private:
    SMetaDataDynamicObjectImpl &operator=(const SMetaDataDynamicObjectImpl &inOther);

public:
    TCharStr m_Name;
    TCharStr m_SourcePath;
    eastl::vector<SMetaDataShader> m_Shaders;
    eastl::vector<qt3ds::render::dynamic::SPropertyDefinition> m_Properties;
    eastl::vector<eastl::vector<qt3ds::foundation::CRegisteredString> *> m_EnumValueNames;
    ~SMetaDataDynamicObjectImpl() { ClearEnumValueNames(); }

    void ClearEnumValueNames()
    {
        for (QT3DSU32 idx = 0, end = m_EnumValueNames.size(); idx < end; ++idx)
            delete (m_EnumValueNames[idx]);
        m_EnumValueNames.clear();
    }
};

struct SMetaDataEffectImpl : public SMetaDataDynamicObjectImpl
{
private:
    SMetaDataEffectImpl &operator=(const SMetaDataEffectImpl &inOther);

public:
    eastl::vector<qt3ds::render::dynamic::SCommand *> m_EffectCommands;

    void ClearEffectCommands()
    {
        for (QT3DSU32 idx = 0, end = m_EnumValueNames.size(); idx < end; ++idx)
            free(m_EffectCommands[idx]);
        m_EffectCommands.clear();
    }
    SMetaDataEffect ToEffect() const
    {
        return SMetaDataEffect(m_Name, VecToCRef(m_Shaders), VecToCRef(m_Properties),
                               VecToCRef(m_EffectCommands));
    }
};

struct SMetaDataCustomMaterialImpl : public SMetaDataDynamicObjectImpl
{
private:
    SMetaDataCustomMaterialImpl &operator=(const SMetaDataCustomMaterialImpl &);

public:
    eastl::vector<qt3ds::render::dynamic::SCommand *>
    m_CustomerMaterialCommands; ///< our command stream used for rendering
    bool m_HasTransparency; ///< this material is transparent
    bool m_HasRefraction; ///< this material is refractive (e.g glass)
    bool m_AlwaysDirty;
    QT3DSU32 m_ShaderKey; ///< What does this shader contain ( e.g. specular, diffuse, ...)
    QT3DSU32 m_LayerCount; ///< How much layers does this material have

    void ClearEffectCommands()
    {
        for (QT3DSU32 idx = 0, end = m_EnumValueNames.size(); idx < end; ++idx) {
            free(m_CustomerMaterialCommands[idx]);
        }

        m_CustomerMaterialCommands.clear();
    }

    SMetaDataCustomMaterial ToMaterial() const
    {
        return SMetaDataCustomMaterial(m_Name, VecToCRef(m_Shaders), VecToCRef(m_Properties),
                                       VecToCRef(m_CustomerMaterialCommands), m_HasTransparency,
                                       m_HasRefraction, m_AlwaysDirty, m_ShaderKey, m_LayerCount);
    }
};

#ifndef QT3DSDM_META_DATA_NO_SIGNALS
class SNewMetaDataImpl : public QObject, public IMetaData
{
    Q_OBJECT
#else
class SNewMetaDataImpl : public IMetaData
{
#endif
public:
    typedef unordered_map<TCharPtr, TInstanceHandle> TStrInstanceMap;
    typedef unordered_map<TInstanceHandle, TCharPtr, hash<int>> TInstanceStrMap;
    // Caching the derivation chain lookup so we can quickly lookup the entire list of
    // derived instances (and have it ordered, somewhat).
    typedef unordered_map<vector<TInstanceHandle>, vector<TInstanceHandle>, InstanceHandleVecHash>
    TDerivationMap;

    typedef unordered_map<TCategoryHandle, SCategoryInfo, hash<int>> TCategoryMap;
    typedef unordered_map<TCharPtr, TCategoryHandle> TNameCategoryMap;

    typedef unordered_map<TMetaDataPropertyHandle, SMetaDataPropertyInfo, hash<int>>
    TMetaDataPropertyMap;
    typedef unordered_map<TInstanceHandle, vector<TMetaDataPropertyHandle>, hash<int>>
    TInstancePropertyMap;
    typedef unordered_map<pair<TInstanceHandle, TCharPtr>, TMetaDataPropertyHandle,
    SInstanceStrHash>
    TInstancePropertyNamePropertyMap;
    typedef unordered_map<TMetaDataPropertyHandle, eastl::vector<SPropertyFilterInfo>, hash<int>>
    TMetaDataPropertyFilterMap;
    typedef unordered_map<TInstanceHandle, vector<TCharPtr>, hash<int>> TInstanceGroupMap;

    typedef unordered_map<TEventHandle, SEvent, hash<int>> TEventMap;
    typedef unordered_map<TInstanceHandle, vector<TEventHandle>, hash<int>> TInstanceEventMap;
    typedef unordered_map<pair<TInstanceHandle, TCharPtr>, TEventHandle, SInstanceStrHash>
    TInstanceEventNameEventMap;

    typedef unordered_map<THandlerHandle, SHandler, hash<int>> THandlerMap;
    typedef unordered_map<TInstanceHandle, vector<THandlerHandle>, hash<int>> TInstanceHandlerMap;
    typedef unordered_map<pair<TInstanceHandle, TCharPtr>, THandlerHandle, SInstanceStrHash>
    TInstanceHandlerNameHandlerMap;

    typedef unordered_map<TInstanceHandle, vector<TCharPtr>, hash<int>> TInstanceStringListMap;
    typedef unordered_map<TCharPtr, SMetaDataEffectImpl> TEffectMap;
    typedef unordered_map<TCharPtr, SMetaDataCustomMaterialImpl> TCustomMaterialMap;

    std::shared_ptr<IDataCore> m_DataCore;
    IStringTable &m_StringTable;
    TTransactionConsumerPtr m_Consumer;

    int m_NextId;

    // Helper objects to speed up queries
    TStrInstanceMap m_CanonicalTypeToInstances;
    TInstanceStrMap m_InstancesToCanonicalType;
    TDerivationMap m_DerivationMap;

    TCategoryMap m_Categories;
    TNameCategoryMap m_NameToCategories;

    TMetaDataPropertyMap m_Properties;
    TInstancePropertyMap m_InstanceToProperties;
    TInstancePropertyNamePropertyMap m_InstanceNameToProperties;
    TMetaDataPropertyFilterMap m_PropertyFilters;
    TInstanceGroupMap m_InstanceGroupMap;

    TEventMap m_Events;
    TInstanceEventMap m_InstanceToEvents;
    TInstanceEventNameEventMap m_InstanceNameToEvents;

    THandlerMap m_Handlers;
    TInstanceHandlerMap m_InstanceToHandlers;
    TInstanceHandlerNameHandlerMap m_InstanceNameToHandlers;

    TInstanceStringListMap m_InstanceToReferences;

    vector<TInstanceHandle> m_Parents;
    vector<TInstanceHandle> m_NextParents;
    vector<TInstanceHandle> m_DerivationChain;
    unordered_set<int> m_UniqueSet;
    unordered_set<size_t> m_SizeTSet;

    MemoryBuffer<RawAllocator> m_TempBuffer;
    MemoryBuffer<RawAllocator> m_ReadBuffer;

    eastl::string m_ConvertStr;

    TCharStr m_ObjectName;

    TEffectMap m_EffectMap;
    TCustomMaterialMap m_CustomMaterials;

#ifndef QT3DSDM_META_DATA_NO_SIGNALS
Q_SIGNALS:
    void internalCategoryDestroyed(Qt3DSDMCategoryHandle);
    void internalMetaDataPropertyDestroyed(Qt3DSDMMetaDataPropertyHandle);
    void internalEventDestroyed(Qt3DSDMEventHandle);
    void internalHandlerDestroyed(Qt3DSDMHandlerHandle);
    void internalHandlerArgDestroyed(Qt3DSDMHandlerHandle, QT3DSU32);
#else
    SNullFunc internalCategoryDestroyed;
    SNullFunc internalMetaDataPropertyDestroyed;
    SNullFunc internalEventDestroyed;
    SNullFunc internalHandlerDestroyed;
    SNullFunc internalHandlerArgDestroyed;
#endif
public:
    TSignalConnectionPtr m_PropertyDeletedConnection;
    bool m_IgnorePropertyDeleted;
    TSignalConnectionPtr m_InstanceDeletedConnection;

    SNewMetaDataImpl(std::shared_ptr<IDataCore> inDataCore)
        : m_DataCore(inDataCore)
        , m_StringTable(inDataCore->GetStringTable())
        , m_NextId(1)
        , m_IgnorePropertyDeleted(false)
    {
#ifndef QT3DSDM_META_DATA_NO_SIGNALS
        CDataCoreProducer *producer = dynamic_cast<CDataCoreProducer *>(inDataCore.get());
        if (producer) {
            m_PropertyDeletedConnection = producer->ConnectPropertyRemoved(
                        bind(&SNewMetaDataImpl::OnPropertyRemoved, this,
                             std::placeholders::_1, std::placeholders::_2));
            m_InstanceDeletedConnection = producer->ConnectBeforeInstanceDeleted(
                        bind(&SNewMetaDataImpl::OnInstanceRemoved, this,
                             std::placeholders::_1));
        }
#endif
    }

    ////////////////////////////////////////////////////////////////////////////
    // Helper Functions

    const wchar_t *Intern(TStrType inData) { return m_StringTable.RegisterStr(inData.wide_str()); }
    const wchar_t *Intern(const char *inData) { return m_StringTable.GetWideStr(inData); }

    inline int GetNextId()
    {
        int retval = m_NextId;
        ++m_NextId;
        return retval;
    }

    template <typename TMapType, typename THandleType>
    static void AddItemToInstanceList(TInstanceHandle inInstance, THandleType inHandle, QT3DSU32 inIdx,
                                      TMapType &ioMap)
    {
        pair<typename TMapType::iterator, bool> inserter =
                ioMap.insert(make_pair(inInstance, vector<THandleType>()));
        inserter.first->second.insert(inserter.first->second.begin() + inIdx, inHandle);
    }

    template <typename TMapType, typename THandleType>
    static QT3DSU32 AddItemToInstanceList(TInstanceHandle inInstance, THandleType inHandle,
                                          TMapType &ioMap)
    {
        pair<typename TMapType::iterator, bool> inserter =
                ioMap.insert(make_pair(inInstance, vector<THandleType>()));
        QT3DSU32 offset = (QT3DSU32)inserter.first->second.size();
        inserter.first->second.push_back(inHandle);
        return offset;
    }

    template <typename TItemType>
    struct VectorEqualPred
    {
        TItemType m_Item;
        VectorEqualPred(const TItemType &inItem)
            : m_Item(inItem)
        {
        }

        bool operator()(const TItemType &inOther) const { return m_Item == inOther; }
    };

    template <typename THandleType, typename TMapType>
    static QT3DSU32 DoRemoveItemFromInstanceList(TInstanceHandle inInstance, THandleType inHandle,
                                                 TMapType &ioMap)
    {
        typename TMapType::iterator find = ioMap.find(inInstance);
        if (find != ioMap.end()) {
            typename vector<THandleType>::iterator theVecFind =
                    std::find(find->second.begin(), find->second.end(), inHandle);
            if (theVecFind != find->second.end()) {
                QT3DSU32 retval = (QT3DSU32)(theVecFind - find->second.begin());
                find->second.erase(theVecFind);
                if (find->second.empty())
                    ioMap.erase(find);
                return retval;
            }
        }
        QT3DS_ASSERT(false);
        return QT3DS_MAX_U32;
    }

    template <typename TMapType, typename THandleType>
    struct InstanceListTransaction : ITransaction
    {
        TInstanceHandle m_Instance;
        THandleType m_Handle;
        TMapType &m_Map;
        QT3DSU32 m_Idx;
        bool m_InsertOnDo;

        InstanceListTransaction(const char *inFile, int inLine, TInstanceHandle inst,
                                THandleType handle, TMapType &map, QT3DSU32 inIdx, bool inInsertOnDo)
            : ITransaction(inFile, inLine)
            , m_Instance(inst)
            , m_Handle(handle)
            , m_Map(map)
            , m_Idx(inIdx)
            , m_InsertOnDo(inInsertOnDo)
        {
        }

        void insert()
        {
            SNewMetaDataImpl::AddItemToInstanceList(m_Instance, m_Handle, m_Idx, m_Map);
        }
        void remove()
        {
            SNewMetaDataImpl::DoRemoveItemFromInstanceList(m_Instance, m_Handle, m_Map);
        }

        void Do() override
        {
            if (m_InsertOnDo)
                insert();
            else
                remove();
        }
        void Undo() override
        {
            if (m_InsertOnDo)
                remove();
            else
                insert();
        }
    };

    template <typename THandleType, typename TMapType>
    void RemoveItemFromInstanceList(const char *inFile, int inLine, TInstanceHandle inInstance,
                                    THandleType inHandle, TMapType &ioMap)
    {
        typename TMapType::iterator find = ioMap.find(inInstance);
        if (find != ioMap.end()) {
            QT3DSU32 idx = DoRemoveItemFromInstanceList(inInstance, inHandle, ioMap);
            if (m_Consumer != NULL) {
                m_Consumer->OnTransaction(
                            std::make_shared<InstanceListTransaction<TMapType, THandleType>>(
                                inFile, inLine, inInstance, inHandle, std::ref(ioMap), idx, false));
            }
        }
    }

    template <typename THandleType, typename TValueType, typename TMapType,
              typename TInstanceListMapType>
    THandleType CreateItem(const char *inFile, int inLine, Qt3DSDMInstanceHandle inInstance,
                           TMapType &inMap, TInstanceListMapType &inInstanceListMap)
    {
        int retval = GetNextId();
        pair<THandleType, TValueType> thePair(make_pair(retval, TValueType(inInstance)));
        inMap.insert(thePair);
        QT3DSU32 idx = AddItemToInstanceList(inInstance, THandleType(retval), inInstanceListMap);
        if (m_Consumer) {
            CreateHashMapInsertTransaction(inFile, inLine, m_Consumer, thePair, inMap);
            m_Consumer->OnTransaction(
                        std::make_shared<InstanceListTransaction<TInstanceListMapType, THandleType>>(
                            inFile, inLine, inInstance, retval, std::ref(inInstanceListMap), idx, true));
        }
        return retval;
    }

    template <typename TKeyType, typename TValueType, typename THashType>
    TValueType *FindHashItem(TKeyType inHandle,
                             unordered_map<TKeyType, TValueType, THashType> &ioHash)
    {
        typename unordered_map<TKeyType, TValueType, THashType>::iterator find =
                ioHash.find(inHandle);
        if (find != ioHash.end())
            return &find->second;
        return NULL;
    }

    template <typename THandleType, typename TMapType>
    static void DoReplaceNamedItem(TInstanceHandle inInst, TCharPtr inOldName, TCharPtr inNewName,
                                   THandleType inNewHandle, TMapType &ioMap)
    {
        ioMap.erase(make_pair(inInst, inOldName));
        bool success = ioMap.insert(make_pair(make_pair(inInst, inNewName), inNewHandle)).second;
        (void)success;
        QT3DS_ASSERT(success);
    }

    template <typename TMapType, typename THandleType>
    struct ReplaceNamedItemTransaction : ITransaction
    {
        TInstanceHandle m_Instance;
        TCharPtr m_OldName;
        TCharPtr m_NewName;
        THandleType m_OldHandle;
        THandleType m_NewHandle;
        TMapType &m_Map;
        ReplaceNamedItemTransaction(const char *inFile, int inLine, TInstanceHandle inst,
                                    TCharPtr oldNm, TCharPtr newNm, THandleType oldHdl,
                                    THandleType newHdl, TMapType &map)
            : ITransaction(inFile, inLine)
            , m_Instance(inst)
            , m_OldName(oldNm)
            , m_NewName(newNm)
            , m_OldHandle(oldHdl)
            , m_NewHandle(newHdl)
            , m_Map(map)
        {
        }
        void Do() override
        {
            SNewMetaDataImpl::DoReplaceNamedItem(m_Instance, m_OldName, m_NewName, m_NewHandle,
                                                 m_Map);
        }
        void Undo() override
        {
            SNewMetaDataImpl::DoReplaceNamedItem(m_Instance, m_NewName, m_OldName, m_OldHandle,
                                                 m_Map);
        }
    };

    template <typename THandleType, typename TMapType>
    void ReplaceNamedItem(const char *inFile, int inLine, TInstanceHandle inInst,
                          TCharPtr inOldName, TCharPtr inNewName, THandleType inNewHandle,
                          TMapType &ioMap)
    {
        typename TMapType::iterator find = ioMap.find(std::make_pair(inInst, inOldName));
        THandleType oldHandle;
        if (find != ioMap.end())
            oldHandle = find->second;
        DoReplaceNamedItem(inInst, inOldName, inNewName, inNewHandle, ioMap);
        if (m_Consumer) {
            if (oldHandle.Valid()) {
                m_Consumer->OnTransaction(
                            std::make_shared<ReplaceNamedItemTransaction<TMapType, THandleType>>(
                                inFile, inLine, inInst, inOldName, inNewName, oldHandle, inNewHandle,
                                std::ref(ioMap)));
            } else
                CreateHashMapInsertTransaction(__FILE__, __LINE__, m_Consumer,
                                               make_pair(make_pair(inInst, inNewName), inNewHandle),
                                               ioMap);
        }
    }

    template <typename THandleType, typename TMapType>
    void DestroyNamedItem(const char *inFile, int inLine, TInstanceHandle inInst, TCharPtr inName,
                          TMapType &ioMap)
    {
        typename TMapType::iterator iter = ioMap.find(make_pair(inInst, inName));
        if (iter != ioMap.end()) {
            pair<pair<TInstanceHandle, TCharPtr>, THandleType> existing(*iter);
            ioMap.erase(iter);
            CreateHashMapEraseTransaction(inFile, inLine, m_Consumer, existing, ioMap);
        }
    }

    template <typename THandleType, typename TInfoType, typename TMapType, typename TNamedMapType>
    void SetItemInfo(const char *inFile, int inLine, THandleType inItem, const TInfoType &oldInfo,
                     const TInfoType &newInfo, TMapType &inMap, TNamedMapType &inNamedMap)
    {
        TCharPtr newName = Intern(newInfo.m_Name.wide_str());
        TCharPtr oldName = Intern(oldInfo.m_Name.wide_str());
        ReplaceNamedItem(inFile, inLine, newInfo.m_Instance, oldName, newName, inItem, inNamedMap);
        CreateHashMapSwapTransaction(inFile, inLine, m_Consumer, inItem, oldInfo, newInfo, inMap);
    }

    bool AddDerivationChainItem(TInstanceHandle inInst)
    {
        if (m_UniqueSet.find(inInst) == m_UniqueSet.end()) {
            m_DerivationChain.push_back(inInst);
            m_UniqueSet.insert(inInst);
            return true;
        }
        return false;
    }

    void GetDerivationChain(TInstanceHandle inInst)
    {
        m_Parents.clear();
        m_DerivationChain.clear();
        m_UniqueSet.clear();
        m_NextParents.clear();
        m_DataCore->GetInstanceParents(inInst, m_Parents);

        TDerivationMap::iterator mapIter = m_DerivationMap.find(m_Parents);
        if (mapIter != m_DerivationMap.end())
            m_DerivationChain = mapIter->second;
        else {
            while (m_Parents.empty() == false) {
                for (size_t idx = 0, end = m_Parents.size(); idx < end; ++idx) {
                    if (AddDerivationChainItem(m_Parents[idx]))
                        m_DataCore->GetInstanceParents(m_Parents[idx], m_NextParents);
                }
                m_Parents = m_NextParents;
                m_NextParents.clear();
            }
            m_NextParents.clear();

            m_DataCore->GetInstanceParents(inInst, m_NextParents);
            m_DerivationMap.insert(make_pair(m_NextParents, m_DerivationChain));
        }
    }

    template <typename THandleType, typename TMapType>
    THandleType FindItemByName(TInstanceHandle inInst, TCharPtr inName, TMapType &ioMap)
    {
        typename TMapType::iterator find(ioMap.find(make_pair(inInst, inName)));
        if (find != ioMap.end())
            return find->second;

        GetDerivationChain(inInst);
        for (size_t idx = 0, end = m_DerivationChain.size(); idx < end; ++idx) {
            find = ioMap.find(make_pair(m_DerivationChain[idx], inName));
            if (find != ioMap.end())
                return find->second;
        }
        return 0;
    }

    template <typename TItemType, typename TVectorType>
    void AddListMapItems(const std::vector<TItemType> &inMapEntry, TVectorType &outVector)
    {
        typedef typename std::vector<TItemType>::const_iterator TIterType;
        for (TIterType theIter = inMapEntry.begin(), theEnd = inMapEntry.end(); theIter != theEnd;
             ++theIter)
            outVector.push_back(*theIter);
    }

    template <typename TListMapType, typename TSizeTOpType, typename TVectorType>
    void DoGetHandleList(Qt3DSDMInstanceHandle inInstance, TListMapType &inMap,
                         TVectorType &outHandles, TSizeTOpType inOperator)
    {
        typename TListMapType::iterator find;
        GetDerivationChain(inInstance);
        for (size_t idx = 0, end = m_DerivationChain.size(); idx < end; ++idx) {
            // Add base classes to the list first
            find = inMap.find(m_DerivationChain[end - idx - 1]);
            if (find != inMap.end())
                AddListMapItems(find->second, outHandles);
        }
        find = inMap.find(inInstance);
        if (find != inMap.end())
            AddListMapItems(find->second, outHandles);
        m_SizeTSet.clear();
        for (size_t ridx = 0; ridx < outHandles.size(); ++ridx) {
            size_t idx = outHandles.size() - ridx - 1;
            // Run through the list backwards, making sure that items further in the list
            // completely overshadow items earlier in the list.

            // Create unique key from the item that we can check against
            size_t item = inOperator(outHandles[idx]);
            if (m_SizeTSet.insert(item).second == false) {
                outHandles.erase(outHandles.begin() + idx);
                --ridx;
            }
        }
    }

    template <typename THandleType, typename TMapType>
    struct NameSizeTOpType
    {
        SNewMetaDataImpl &m_Impl;
        TMapType &m_Map;
        NameSizeTOpType(SNewMetaDataImpl &inImpl, TMapType &inMap)
            : m_Impl(inImpl)
            , m_Map(inMap)
        {
        }

        size_t operator()(THandleType inHandle)
        {
            return reinterpret_cast<size_t>(m_Impl.Intern(m_Map[inHandle].m_Name));
        }
    };

    // Ensure we don't return two items of the same name.
    template <typename THandleType, typename TListMapType, typename TMapType>
    void GetHandleList(Qt3DSDMInstanceHandle inInstance, TListMapType &inMap, TMapType &inTypeName,
                       vector<THandleType> &outHandles)
    {
        DoGetHandleList(inInstance, inMap, outHandles,
                        NameSizeTOpType<THandleType, TMapType>(*this, inTypeName));
    }

    template <typename THandleType, typename TMapType, typename TNameMapType,
              typename TListMapType>
    bool DestroyItem(const char *inFile, int inLine, THandleType inItem,
                     TMapType &inMap, TNameMapType &inNameMap, TListMapType &inListMap)
    {
        typename TMapType::iterator find(inMap.find(inItem));
        if (find == inMap.end())
            return false;
        DestroyNamedItem<THandleType>(inFile, inLine, find->second.m_Instance,
                                      Intern(find->second.m_Name.wide_str()), inNameMap);
        RemoveItemFromInstanceList(inFile, inLine, find->second.m_Instance, inItem, inListMap);
        CreateHashMapEraseTransaction(inFile, inLine, m_Consumer,
                                      make_pair(find->first, find->second), inMap);
        inMap.erase(find);
        return true;
    }

    template <typename THandleType, typename TListMapType>
    void ForEachItem(Qt3DSDMInstanceHandle inInstance, TListMapType &ioMap,
                     function<void(THandleType)> inOperation)
    {
        typename TListMapType::iterator find = ioMap.find(inInstance);
        if (find != ioMap.end()) {
            vector<THandleType> itemData(find->second);
            for (size_t idx = 0, end = itemData.size(); idx < end; ++idx)
                inOperation(itemData[idx]);
        }
    }

    SCategoryInfo *FindCategory(Qt3DSDMCategoryHandle inCategory)
    {
        return FindHashItem(inCategory, m_Categories);
    }

    SMetaDataPropertyInfo *FindProperty(Qt3DSDMMetaDataPropertyHandle inPropertyHandle)
    {
        return FindHashItem(inPropertyHandle, m_Properties);
    }

    SEvent *FindEvent(Qt3DSDMEventHandle inEventHandle)
    {
        return FindHashItem(inEventHandle, m_Events);
    }

    SHandler *FindHandler(Qt3DSDMHandlerHandle inHandle)
    {
        return FindHashItem(inHandle, m_Handlers);
    }

    SMetaDataHandlerArgumentInfo *FindHandlerArg(Qt3DSDMHandlerHandle inHandler, QT3DSU32 inIdx)
    {
        SHandler *theHandler(FindHandler(inHandler));
        if (theHandler && theHandler->m_Arguments.size() > inIdx)
            return &theHandler->m_Arguments[inIdx];
        return NULL;
    }

    void OnPropertyRemoved(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty)
    {
        if (m_IgnorePropertyDeleted)
            return;

        vector<Qt3DSDMMetaDataPropertyHandle> propertiesToDestroy;
        for (TMetaDataPropertyMap::iterator iter = m_Properties.begin(), end = m_Properties.end();
             iter != end; ++iter) {
            if (iter->second.m_Property == inProperty)
                propertiesToDestroy.push_back(iter->first);
        }

        for (size_t idx = 0, end = propertiesToDestroy.size(); idx < end; ++idx)
            DestroyMetaDataProperty(propertiesToDestroy[idx]);
    }

    void OnInstanceRemoved(Qt3DSDMInstanceHandle inInstance) { DestroyMetaData(inInstance); }
    template <typename TEntryType, typename TMapType>
    void InsertWithTransaction(const char *inFile, int inLine, const TEntryType &inEntry,
                               TMapType &inMap)
    {
        inMap.insert(inEntry);
        CreateHashMapInsertTransaction(inFile, inLine, m_Consumer, inEntry, inMap);
    }

    template <typename TKeyType, typename TMapType>
    void EraseWithTransaction(const char *inFile, int inLine, TKeyType inKey, TMapType &inMap)
    {
        typename TMapType::iterator find(inMap.find(inKey));
        if (find != inMap.end()) {
            CreateHashMapEraseTransaction(inFile, inLine, m_Consumer,
                                          std::make_pair(find->first, find->second), inMap);
            inMap.erase(find);
        }
    }

    template <typename TEntryType, typename TMapType>
    void InsertOrUpdateWithTransaction(const TEntryType &inEntry, TMapType &inMap)
    {
        pair<typename TMapType::iterator, bool> inserter = inMap.insert(inEntry);
        if (inserter.second)
            CreateHashMapInsertTransaction(__FILE__, __LINE__, m_Consumer, inEntry, inMap);
        else {
            typename TMapType::iterator theIter(inserter.first);
            CreateHashMapSwapTransaction(__FILE__, __LINE__, m_Consumer, theIter->first,
                                         theIter->second, inEntry.second, inMap);
            theIter->second = inEntry.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // API Implementation

    ////////////////////////////////////////////////////////////////////////////////////
    // Sharing some utility objects
    IStringTable &GetStringTable() override { return m_DataCore->GetStringTable(); }
    TStringTablePtr GetStringTablePtr() override { return m_DataCore->GetStringTablePtr(); }
    TDataCorePtr GetDataCore() override { return m_DataCore; }

    ////////////////////////////////////////////////////////////////////////////////////
    // Canonical Instances
    void SetInstanceAsCanonical(Qt3DSDMInstanceHandle inInstance, TStrType inTypename) override
    {
        const wchar_t *theTypename(Intern(inTypename));
        if (g_DataModelDebugLogger)
            g_DataModelDebugLogger("IMetaData::SetInstanceAsCanonical Enter");
        if (g_DataModelDebugLogger)
            g_DataModelDebugLogger(GetStringTable().GetNarrowStr(inTypename.wide_str()));
        m_CanonicalTypeToInstances.insert(make_pair(theTypename, inInstance));
        m_InstancesToCanonicalType.insert(make_pair(inInstance, theTypename));
        CreateHashMapInsertTransaction(__FILE__, __LINE__, m_Consumer,
                                       make_pair(theTypename, inInstance),
                                       m_CanonicalTypeToInstances);
        CreateHashMapInsertTransaction(__FILE__, __LINE__, m_Consumer,
                                       make_pair(inInstance, theTypename),
                                       m_InstancesToCanonicalType);
        if (g_DataModelDebugLogger)
            g_DataModelDebugLogger("IMetaData::SetInstanceAsCanonical Leave");
    }

    Qt3DSDMInstanceHandle GetCanonicalInstanceForType(TStrType inTypename) override
    {
        TStrInstanceMap::iterator find = m_CanonicalTypeToInstances.find(Intern(inTypename));
        if (find != m_CanonicalTypeToInstances.end())
            return find->second;
        return 0;
    }

    Option<TCharStr> GetTypeForCanonicalInstance(Qt3DSDMInstanceHandle inInstance) override
    {
        TInstanceStrMap::iterator find = m_InstancesToCanonicalType.find(inInstance);
        if (find != m_InstancesToCanonicalType.end())
            return TCharStr(find->second);
        return Empty();
    }

    Option<TCharStr> GetTypeForInstance(Qt3DSDMInstanceHandle inInstance) override
    {
        Option<TCharStr> theType = GetTypeForCanonicalInstance(inInstance);
        if (theType.hasValue())
            return theType;
        GetDerivationChain(inInstance);
        for (size_t idx = 0, end = m_DerivationChain.size(); idx < end; ++idx) {
            theType = GetTypeForCanonicalInstance(m_DerivationChain[idx]);
            if (theType.hasValue())
                return theType;
        }
        return Empty();
    }

    QT3DSU32 GetGroupCountForInstance(Qt3DSDMInstanceHandle inInstance) override
    {
        std::vector<TCharStr> outNames;
        QT3DSU32 count = GetGroupNamesForInstance(inInstance, outNames);
        return (count == 0) ? 1 : count;
    }

    QT3DSU32 GetGroupNamesForInstance(Qt3DSDMInstanceHandle inInstance,
                                      std::vector<TCharStr> &outNames) override
    {
        TInstanceStrMap::iterator canonicalFind = m_InstancesToCanonicalType.find(inInstance);
        if (canonicalFind != m_InstancesToCanonicalType.end()) {
            TInstanceGroupMap::iterator find = m_InstanceGroupMap.find(inInstance);
            if (find != m_InstanceGroupMap.end()) {
                pair<typename TInstanceGroupMap::iterator, bool> inserter =
                        m_InstanceGroupMap.insert(make_pair(inInstance, vector<TCharPtr>()));
                vector<TCharPtr> &itemList = inserter.first->second;
                for (size_t i = 0, j = itemList.size(); i < j; ++i) {
                    bool alreadyInList = false;
                    // discard duplicates
                    for (size_t k = 0, l = outNames.size(); k < l; ++k) {
                        TCharStr curListName = itemList[i];
                        if (curListName == outNames[k].wide_str()) {
                            alreadyInList = true;
                            break;
                        }
                    }
                    if (!alreadyInList)
                        outNames.push_back(itemList[i]);
                }
            }
            return (QT3DSU32)outNames.size();
        }

        GetDerivationChain(inInstance);

        for (int idx = (int)m_DerivationChain.size() - 1, end = 0; idx >= end; --idx) {
            TInstanceGroupMap::iterator find = m_InstanceGroupMap.find(m_DerivationChain[idx]);
            if (find != m_InstanceGroupMap.end()) {
                pair<typename TInstanceGroupMap::iterator, bool> inserter =
                        m_InstanceGroupMap.insert(
                            make_pair(m_DerivationChain[idx], vector<TCharPtr>()));
                vector<TCharPtr> &itemList = inserter.first->second;
                for (size_t i = 0, j = itemList.size(); i < j; ++i) {
                    bool alreadyInList = false;
                    // discard duplicates
                    for (size_t k = 0, l = outNames.size(); k < l; ++k) {
                        TCharStr curListName = itemList[i];
                        if (curListName == outNames[k].wide_str()) {
                            alreadyInList = true;
                            break;
                        }
                    }
                    if (!alreadyInList)
                        outNames.push_back(itemList[i]);
                }
            }
        }

        return (QT3DSU32)outNames.size();
    }

    Option<TCharStr> GetGroupFilterNameForInstance(Qt3DSDMInstanceHandle inInstance,
                                                   long inIndex) override
    {
        std::vector<TCharStr> outNames;
        QT3DSU32 count = GetGroupNamesForInstance(inInstance, outNames);
        if (count > (QT3DSU32)inIndex)
            return outNames[inIndex];

        return Empty();
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Categories

    std::pair<Qt3DSDMCategoryHandle, bool> GetOrCreateCategory(TStrType inName) override
    {
        TCharPtr theName(Intern(inName));
        TNameCategoryMap::iterator find = m_NameToCategories.find(theName);
        if (find != m_NameToCategories.end())
            return make_pair(find->second, false);

        TCategoryHandle retval(GetNextId());
        InsertWithTransaction(__FILE__, __LINE__, make_pair(retval, SCategoryInfo(inName)),
                              m_Categories);
        InsertWithTransaction(__FILE__, __LINE__, make_pair(theName, retval), m_NameToCategories);
        return make_pair(retval, true);
    }

    void SetCategoryInfo(Qt3DSDMCategoryHandle inCategory, TStrType inIcon,
                         TStrType inHighlight, TStrType inDescription) override
    {
        SCategoryInfo *infoPtr(FindCategory(inCategory));
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        SCategoryInfo &newInfo(*infoPtr);
        SCategoryInfo oldInfo(newInfo);

        newInfo.m_Icon = inIcon;
        newInfo.m_HighlightIcon = inHighlight;
        newInfo.m_Description = inDescription;
        CreateHashMapSwapTransaction(__FILE__, __LINE__, m_Consumer, inCategory, oldInfo, newInfo,
                                     m_Categories);
    }

    void DestroyCategory(Qt3DSDMCategoryHandle inCategory) override
    {
        SCategoryInfo *infoPtr(FindCategory(inCategory));
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        EraseWithTransaction(__FILE__, __LINE__, Intern(infoPtr->m_Name), m_NameToCategories);
        EraseWithTransaction(__FILE__, __LINE__, inCategory, m_Categories);
    }
    Option<SCategoryInfo> GetCategoryInfo(Qt3DSDMCategoryHandle inCategory) override
    {
        SCategoryInfo *infoPtr(FindCategory(inCategory));
        if (infoPtr)
            return *infoPtr;
        return Empty();
    }
    Qt3DSDMCategoryHandle FindCategoryByName(TStrType inName) override
    {
        TCharPtr theName(Intern(inName));
        TNameCategoryMap::iterator find = m_NameToCategories.find(theName);
        if (find != m_NameToCategories.end())
            return find->second;
        return 0;
    }

    void GetCategories(vector<Qt3DSDMCategoryHandle> &outCategories) override
    {
        for (TCategoryMap::iterator iter = m_Categories.begin(), end = m_Categories.end();
             iter != end; ++iter)
            outCategories.push_back(iter->first);
    }

    Option<SCategoryInfo> GetEventCategory(TStrType inName) override
    {
        return GetCategoryInfo(FindCategoryByName(inName));
    }

    Option<SCategoryInfo> GetHandlerCategory(TStrType inName) override
    {
        return GetCategoryInfo(FindCategoryByName(inName));
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Properties

    Qt3DSDMMetaDataPropertyHandle CreateMetaDataProperty(Qt3DSDMInstanceHandle inInstance) override
    {
        return CreateItem<Qt3DSDMMetaDataPropertyHandle, SMetaDataPropertyInfo>(
                    __FILE__, __LINE__, inInstance, m_Properties, m_InstanceToProperties);
    }

    void EnsureDataCoreProperty(SMetaDataPropertyInfo &newInfo)
    {
        m_IgnorePropertyDeleted = true;
        // If the existing property under the new name doesn't match
        // the new info, then we also have to delete the property
        Qt3DSDMPropertyHandle theExistingProperty =
                m_DataCore->GetAggregateInstancePropertyByName(newInfo.m_Instance, newInfo.m_Name);
        // Ensure the types match.
        if (theExistingProperty.Valid()) {
            Qt3DSDMPropertyDefinition theDefinition(m_DataCore->GetProperty(theExistingProperty));
            if (theDefinition.m_Name != newInfo.m_Name
                    || theDefinition.m_Type != newInfo.GetDataType()) {
                m_DataCore->RemoveProperty(theExistingProperty);
                theExistingProperty = 0;
            }
        }

        // Finally, if we don't have a property at this point, create a new property
        if (theExistingProperty.Valid() == false)
            theExistingProperty = m_DataCore->AddProperty(
                        newInfo.m_Instance, newInfo.m_Name.wide_str(), newInfo.GetDataType());
        newInfo.m_Property = theExistingProperty;
        m_IgnorePropertyDeleted = false;
    }

    // If the type doesn't match the default, then it has no default.
    SValue VerifyDefaultPropertyType(DataModelDataType::Value inDataType, const SValue &inValue)
    {
        if (inValue.empty() == false) {
            DataModelDataType::Value theType = GetValueType(inValue);
            if (theType != inDataType) {
                return SValue(0.0f);
            }
        }
        return inValue;
    }
    // If the datatype doesn't match the value, force the value to match the type.
    // some types *have* to have values.
    TMetaDataData VerifyMetaDataDataType(AdditionalMetaDataType::Value inDataType,
                                         const TMetaDataData &inValue)
    {
        if (inDataType == AdditionalMetaDataType::StringList
                || inDataType == AdditionalMetaDataType::Range) {
            if (inValue.empty() == true || inDataType != GetMetaDataValueType(inValue)) {
                QT3DS_ASSERT(false);
                if (inDataType == AdditionalMetaDataType::StringList)
                    return TMetaDataStringList();
                if (inDataType == AdditionalMetaDataType::Range)
                    return SMetaDataRange(0, 1);
            }
        }
        return inValue;
    }

    void SetPropertyBaseInfo(SMetaPropertyBase &newInfo, TStrType inName, TStrType inFormalName,
                             TStrType inDescription, TStrType inUsage,
                             CompleteMetaDataType::Enum inDataType, const SValue &inDefaultValue,
                             const TMetaDataData &inMetaData)
    {
        newInfo.m_Name = inName;
        newInfo.m_FormalName = inFormalName;
        newInfo.m_Description = inDescription;
        newInfo.m_Usage = inUsage;
        newInfo.m_CompleteType = inDataType;
        newInfo.m_DefaultValue =
                VerifyDefaultPropertyType(CompleteMetaDataType::ToDataType(inDataType),
                                          inDefaultValue);
        newInfo.m_MetaDataData =
                VerifyMetaDataDataType(CompleteMetaDataType::ToAdditionalType(inDataType),
                                       inMetaData);
    }

    // For properties, you set the default values separately
    // This may delete the underlying data model property rebuild it.
    void SetMetaDataPropertyInfo(Qt3DSDMMetaDataPropertyHandle inPropertyHandle,
                                 TStrType inName, TStrType inFormalName,
                                 TStrType inDescription, TStrType inUsage,
                                 CompleteMetaDataType::Enum inDataType,
                                 const SValue &inDefaultValue,
                                 const TMetaDataData &inMetaData, TStrType inGroupName,
                                 bool inIsHidden, bool inIsAnimatable,
                                 bool inIsControllable) override
    {
        SMetaDataPropertyInfo *infoPtr = FindProperty(inPropertyHandle);
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        SMetaDataPropertyInfo &newInfo(*infoPtr);
        SMetaDataPropertyInfo oldInfo(newInfo);

        SetPropertyBaseInfo(newInfo, inName, inFormalName, inDescription, inUsage, inDataType,
                            inDefaultValue, inMetaData);
        newInfo.m_IsHidden = inIsHidden;
        newInfo.m_Animatable = inIsAnimatable;
        newInfo.m_Controllable = inIsControllable;
        newInfo.m_GroupName = inGroupName;
        EnsureDataCoreProperty(newInfo);

        SetItemInfo(__FILE__, __LINE__, inPropertyHandle, oldInfo, newInfo, m_Properties,
                    m_InstanceNameToProperties);

        SetPropertyDefault(newInfo, CompleteMetaDataType::ToDataType(inDataType));
    }

    void SetPropertyDefault(SMetaDataPropertyInfo &newInfo, DataModelDataType::Value inDataType)
    {
        if (newInfo.m_DefaultValue.empty() == false
                && GetValueType(newInfo.m_DefaultValue) == inDataType) {
            m_DataCore->SetInstancePropertyValue(newInfo.m_Instance, newInfo.m_Property,
                                                 newInfo.m_DefaultValue);
        }
    }

    // Destroy just this meta data property
    void DestroyMetaDataProperty(Qt3DSDMMetaDataPropertyHandle inProperty) override
    {
        SMetaDataPropertyInfo *infoPtr = FindProperty(inProperty);
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        if (DestroyItem(__FILE__, __LINE__, inProperty,
                        m_Properties, m_InstanceNameToProperties, m_InstanceToProperties)) {
            Q_EMIT internalMetaDataPropertyDestroyed(inProperty);
        }

        RemoveMetaDataPropertyFilters(inProperty);
    }

    Qt3DSDMMetaDataPropertyHandle GetMetaDataProperty(Qt3DSDMInstanceHandle inInstance,
                                                      TStrType inPropertyName) override
    {
        return FindItemByName<Qt3DSDMMetaDataPropertyHandle>(inInstance, Intern(inPropertyName),
                                                             m_InstanceNameToProperties);
    }
    Qt3DSDMMetaDataPropertyHandle GetMetaDataProperty(Qt3DSDMInstanceHandle inInstance,
                                                      Qt3DSDMPropertyHandle inProperty) override
    {
        Qt3DSDMPropertyDefinition propDef(m_DataCore->GetProperty(inProperty));
        return GetMetaDataProperty(inInstance, propDef.m_Name);
    }
    // Sets the value in the data core
    virtual Option<SMetaDataPropertyInfo>
    GetMetaDataPropertyInfo(Qt3DSDMMetaDataPropertyHandle inProperty) override
    {
        SMetaDataPropertyInfo *infoPtr = FindProperty(inProperty);
        if (infoPtr == NULL) {
            return Empty();
        }
        return *infoPtr;
    }

    void GetMetaDataProperties(Qt3DSDMInstanceHandle inInstance,
                               vector<Qt3DSDMMetaDataPropertyHandle> &outProperties) override
    {
        return GetHandleList<Qt3DSDMMetaDataPropertyHandle>(inInstance, m_InstanceToProperties,
                                                            m_Properties, outProperties);
    }
    virtual Qt3DSDMMetaDataPropertyHandle
    GetSpecificMetaDataProperty(Qt3DSDMInstanceHandle inInstance, TStrType inPropertyName)
    {
        TInstancePropertyNamePropertyMap::iterator theFind = m_InstanceNameToProperties.find(
                    make_pair(inInstance, m_StringTable.RegisterStr(inPropertyName.wide_str())));
        if (theFind != m_InstanceNameToProperties.end())
            return theFind->second;
        return 0;
    }
    virtual Qt3DSDMMetaDataPropertyHandle
    GetOrCreateSpecificMetaDataProperty(Qt3DSDMInstanceHandle inInstance,
                                        TStrType inPropertyName) override
    {
        Qt3DSDMMetaDataPropertyHandle theProp(
                    GetSpecificMetaDataProperty(inInstance, inPropertyName));
        if (theProp.Valid())
            return theProp;
        return CreateMetaDataProperty(inInstance);
    }

    void GetSpecificMetaDataProperties(Qt3DSDMInstanceHandle inInstance,
                                       vector<Qt3DSDMMetaDataPropertyHandle> &outProperties) override
    {
        TInstancePropertyMap::iterator find = m_InstanceToProperties.find(inInstance);
        if (find != m_InstanceToProperties.end())
            outProperties.insert(outProperties.end(), find->second.begin(), find->second.end());
    }

    TCharStr GetFormalName(Qt3DSDMInstanceHandle inInstance,
                           Qt3DSDMPropertyHandle inProperty) override
    {
        Qt3DSDMMetaDataPropertyHandle propHandle(GetMetaDataProperty(inInstance, inProperty));
        SMetaDataPropertyInfo *infoPtr = FindProperty(propHandle);
        if (infoPtr)
            return infoPtr->m_FormalName;
        return TCharStr();
    }
    AdditionalMetaDataType::Value GetAdditionalMetaDataType(
            Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) override
    {
        Qt3DSDMMetaDataPropertyHandle propHandle(GetMetaDataProperty(inInstance, inProperty));
        SMetaDataPropertyInfo *infoPtr = FindProperty(propHandle);
        if (infoPtr)
            return infoPtr->GetAdditionalType();
        return AdditionalMetaDataType::None;
    }
    TMetaDataData GetAdditionalMetaDataData(Qt3DSDMInstanceHandle inInstance,
                                            Qt3DSDMPropertyHandle inProperty) override
    {
        Qt3DSDMMetaDataPropertyHandle propHandle(GetMetaDataProperty(inInstance, inProperty));
        SMetaDataPropertyInfo *infoPtr = FindProperty(propHandle);
        if (propHandle.Valid())
            return infoPtr->m_MetaDataData;
        return TMetaDataData();
    }

    bool IsCustomInstance(Qt3DSDMInstanceHandle inInstance)
    {
        return m_InstancesToCanonicalType.find(inInstance) == m_InstancesToCanonicalType.end();
    }

    SValue GetDefaultValue(Qt3DSDMInstanceHandle inInstance,
                           Qt3DSDMPropertyHandle inProperty) override
    {
        Qt3DSDMMetaDataPropertyHandle theProperty(GetMetaDataProperty(inInstance, inProperty));
        if (theProperty.Valid() == false) {
            QT3DS_ASSERT(false);
            return SValue();
        }
        return FindProperty(theProperty)->m_DefaultValue;
    }

    bool IsCustomProperty(Qt3DSDMInstanceHandle inInstance,
                          Qt3DSDMPropertyHandle inProperty) override
    {
        Qt3DSDMMetaDataPropertyHandle propHandle(GetMetaDataProperty(inInstance, inProperty));
        SMetaDataPropertyInfo *infoPtr = FindProperty(propHandle);
        if (infoPtr)
            return IsCustomInstance(infoPtr->m_Instance);

        return false;
    }

    bool IsFilterValid(Qt3DSDMMetaDataPropertyHandle inProperty,
                       const SPropertyFilterInfo &inFilter)
    {
        SMetaDataPropertyInfo *infoPtr = FindProperty(inProperty);
        if (m_DataCore->IsProperty(inFilter.m_FilterProperty) == false) {
            QT3DS_ASSERT(false);
            return false;
        }

        Qt3DSDMPropertyDefinition theProp(m_DataCore->GetProperty(inFilter.m_FilterProperty));
        Qt3DSDMPropertyHandle propCheck =
                m_DataCore->GetAggregateInstancePropertyByName(infoPtr->m_Instance, theProp.m_Name);
        if (propCheck != inFilter.m_FilterProperty) {
            QT3DS_ASSERT(false);
            return false;
        }

        DataModelDataType::Value theType = GetValueType(inFilter.m_Value);
        if (theType != theProp.m_Type) {
            QT3DS_ASSERT(false);
            return false;
        }

        return true;
    }

    void SetMetaDataPropertyFilters(Qt3DSDMMetaDataPropertyHandle inProperty,
                                    NVConstDataRef<SPropertyFilterInfo> inFilters) override
    {
        SMetaDataPropertyInfo *infoPtr = FindProperty(inProperty);
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        eastl::vector<SPropertyFilterInfo> newFilters;
        for (QT3DSU32 idx = 0, end = inFilters.size(); idx < end; ++idx) {
            const SPropertyFilterInfo &theFilter(inFilters[idx]);
            if (IsFilterValid(inProperty, theFilter))
                newFilters.push_back(theFilter);
        }

        InsertOrUpdateWithTransaction(std::make_pair(inProperty, newFilters), m_PropertyFilters);
    }

    virtual NVConstDataRef<SPropertyFilterInfo>
    GetMetaDataPropertyFilters(Qt3DSDMMetaDataPropertyHandle inProperty) override
    {
        TMetaDataPropertyFilterMap::iterator theIter(m_PropertyFilters.find(inProperty));
        if (theIter != m_PropertyFilters.end())
            return qt3ds::foundation::toDataRef(theIter->second.data(), theIter->second.size());
        return NVConstDataRef<SPropertyFilterInfo>();
    }

    void RemoveMetaDataPropertyFilters(Qt3DSDMMetaDataPropertyHandle inProperty) override
    {
        TMetaDataPropertyFilterMap::iterator theIter(m_PropertyFilters.find(inProperty));
        if (theIter != m_PropertyFilters.end())
            EraseWithTransaction(__FILE__, __LINE__, inProperty, m_PropertyFilters);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Properties
    Qt3DSDMEventHandle CreateMetaDataEvent(TInstanceHandle inInstance) override
    {
        return CreateItem<Qt3DSDMEventHandle, SEvent>(__FILE__, __LINE__, inInstance, m_Events,
                                                      m_InstanceToEvents);
    }

    void SetEventInfo(Qt3DSDMEventHandle inEventHandle, TStrType inName,
                      TStrType inFormalName, TStrType inCategory, TStrType inDescription) override
    {
        SEvent *infoPtr = FindEvent(inEventHandle);
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        SEvent &newInfo(*infoPtr);
        SEvent oldInfo(newInfo);
        newInfo.m_Name = inName;
        newInfo.m_FormalName = inFormalName;
        newInfo.m_Category = inCategory;
        newInfo.m_Description = inDescription;
        SetItemInfo(__FILE__, __LINE__, inEventHandle, oldInfo, newInfo, m_Events,
                    m_InstanceNameToEvents);
    }

    void DestroyEvent(Qt3DSDMEventHandle inEventHandle) override
    {
        if (DestroyItem(__FILE__, __LINE__, inEventHandle, m_Events,
                        m_InstanceNameToEvents, m_InstanceToEvents)) {
            Q_EMIT internalEventDestroyed(inEventHandle);
        }
    }

    void GetEvents(Qt3DSDMInstanceHandle inInstance, TEventHandleList &outEvents) override
    {
        return GetHandleList<Qt3DSDMEventHandle>(inInstance, m_InstanceToEvents, m_Events,
                                                 outEvents);
    }

    Qt3DSDMEventHandle FindEvent(Qt3DSDMInstanceHandle inInstance, TStrType inName) override
    {
        return FindItemByName<Qt3DSDMEventHandle>(inInstance, Intern(inName),
                                                  m_InstanceNameToEvents);
    }

    Option<SEventInfo> GetEventInfo(Qt3DSDMEventHandle inEventHandle) override
    {
        SEvent *infoPtr = FindEvent(inEventHandle);
        if (infoPtr == NULL) {
            return Empty();
        }
        SEventInfo retval;
        retval.m_Name = infoPtr->m_Name;
        retval.m_FormalName = infoPtr->m_FormalName;
        retval.m_Description = infoPtr->m_Description;
        retval.m_Category = infoPtr->m_Category;
        return retval;
    }

    bool IsCustomEvent(Qt3DSDMEventHandle inEventHandle) override
    {
        SEvent *infoPtr = FindEvent(inEventHandle);
        if (infoPtr != NULL)
            return IsCustomInstance(infoPtr->m_Instance);
        return false;
    }

    void GetSpecificEvents(Qt3DSDMInstanceHandle inInstance, TEventHandleList &outEvents) override
    {
        TInstanceEventMap::iterator theFind(m_InstanceToEvents.find(inInstance));
        if (theFind != m_InstanceToEvents.end())
            outEvents.insert(outEvents.end(), theFind->second.begin(), theFind->second.end());
    }

    Qt3DSDMEventHandle GetOrCreateSpecificEvent(Qt3DSDMInstanceHandle inInstance,
                                                TStrType inName) override
    {
        TInstanceEventNameEventMap::iterator theFind(
                    m_InstanceNameToEvents.find(make_pair(inInstance, Intern(inName))));
        if (theFind != m_InstanceNameToEvents.end())
            return theFind->second;
        return CreateMetaDataEvent(inInstance);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Handlers

    Qt3DSDMHandlerHandle CreateHandler(Qt3DSDMInstanceHandle inInstance) override
    {
        return CreateItem<Qt3DSDMHandlerHandle, SHandler>(__FILE__, __LINE__, inInstance,
                                                          m_Handlers, m_InstanceToHandlers);
    }

    void SetHandlerInfo(Qt3DSDMHandlerHandle inHandle, TStrType inName,
                        TStrType inFormalName, TStrType inCategoryName,
                        TStrType inDescription) override
    {
        SHandler *infoPtr = FindHandler(inHandle);
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        SHandler &newInfo(*infoPtr);
        SHandler oldInfo(newInfo);
        newInfo.m_Name = inName;
        newInfo.m_FormalName = inFormalName;
        newInfo.m_Description = inDescription;
        newInfo.m_Category = inCategoryName;
        SetItemInfo(__FILE__, __LINE__, inHandle, oldInfo, newInfo, m_Handlers,
                    m_InstanceNameToHandlers);
    }

    void DestroyHandler(Qt3DSDMHandlerHandle inHandlerHandle) override
    {
        SHandler *infoPtr(FindHandler(inHandlerHandle));
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        while (infoPtr->m_Arguments.empty() == false)
            DestroyHandlerArgument(inHandlerHandle, (QT3DSU32)infoPtr->m_Arguments.size() - 1);
        if (DestroyItem(__FILE__, __LINE__, inHandlerHandle, m_Handlers,
                        m_InstanceNameToHandlers, m_InstanceToHandlers)) {
            Q_EMIT internalHandlerDestroyed(inHandlerHandle);
        }
    }

    Qt3DSDMHandlerHandle FindHandlerByName(Qt3DSDMInstanceHandle inInstance,
                                           TStrType inName) override
    {
        return FindItemByName<Qt3DSDMHandlerHandle>(inInstance, Intern(inName),
                                                    m_InstanceNameToHandlers);
    }

    Option<SHandlerInfo> GetHandlerInfo(Qt3DSDMHandlerHandle inHandlerHandle) override
    {
        SHandler *infoPtr = FindHandler(inHandlerHandle);
        if (infoPtr == NULL)
            return Empty();

        SHandlerInfo retval;
        retval.m_Name = infoPtr->m_Name;
        retval.m_FormalName = infoPtr->m_FormalName;
        retval.m_Category = infoPtr->m_Category;
        retval.m_Description = infoPtr->m_Description;

        return retval;
    }

    void GetHandlers(Qt3DSDMInstanceHandle inInstance, THandlerHandleList &outHandlers) override
    {
        return GetHandleList<Qt3DSDMHandlerHandle>(inInstance, m_InstanceToHandlers, m_Handlers,
                                                   outHandlers);
    }

    bool IsCustomHandler(Qt3DSDMHandlerHandle inHandle) override
    {
        SHandler *infoPtr = FindHandler(inHandle);
        if (infoPtr != NULL)
            return IsCustomInstance(infoPtr->m_Instance);
        return false;
    }

    void GetSpecificHandlers(Qt3DSDMInstanceHandle inInstance,
                             THandlerHandleList &outHandlers) override
    {
        TInstanceHandlerMap::iterator theFind = m_InstanceToHandlers.find(inInstance);
        if (theFind != m_InstanceToHandlers.end())
            outHandlers.insert(outHandlers.end(), theFind->second.begin(), theFind->second.end());
    }

    Qt3DSDMHandlerHandle GetOrCreateSpecificHandler(Qt3DSDMInstanceHandle inInstance,
                                                    TStrType inName) override
    {
        TInstanceHandlerNameHandlerMap::iterator theFind =
                m_InstanceNameToHandlers.find(make_pair(inInstance, Intern(inName)));
        if (theFind != m_InstanceNameToHandlers.end())
            return theFind->second;
        return CreateHandler(inInstance);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Handler Arguments

    void DoAddHandlerArgument(Qt3DSDMHandlerHandle inHandler,
                              const SMetaDataHandlerArgumentInfo &inArgHandle, QT3DSU32 inIdx)
    {
        SHandler *infoPtr = FindHandler(inHandler);
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        infoPtr->m_Arguments.insert(infoPtr->m_Arguments.begin() + inIdx, inArgHandle);
    }

    struct ArgNameEqual
    {
        const TCharStr &m_Name;
        ArgNameEqual(const TCharStr &nm)
            : m_Name(nm)
        {
        }
        bool operator()(const SMetaDataHandlerArgumentInfo &info) const
        {
            return m_Name == info.m_Name;
        }
    };

    void DoRemoveHandlerArgument(Qt3DSDMHandlerHandle inHandler,
                                 const SMetaDataHandlerArgumentInfo &inArgHandle)
    {
        SHandler *infoPtr = FindHandler(inHandler);
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        erase_if(infoPtr->m_Arguments, ArgNameEqual(inArgHandle.m_Name));
    }

    struct HandlerArgumentAddRemoveTransaction : public ITransaction
    {
        SNewMetaDataImpl &m_Impl;
        Qt3DSDMHandlerHandle m_Handler;
        SMetaDataHandlerArgumentInfo m_Arg;
        QT3DSU32 m_Idx;
        bool m_AddOnDo;

        HandlerArgumentAddRemoveTransaction(const char *inFile, int inLine, SNewMetaDataImpl &impl,
                                            Qt3DSDMHandlerHandle hdl,
                                            const SMetaDataHandlerArgumentInfo &arg, QT3DSU32 inIdx,
                                            bool inAddOnDo)
            : ITransaction(inFile, inLine)
            , m_Impl(impl)
            , m_Handler(hdl)
            , m_Arg(arg)
            , m_Idx(inIdx)
            , m_AddOnDo(inAddOnDo)
        {
        }
        void insert() { m_Impl.DoAddHandlerArgument(m_Handler, m_Arg, m_Idx); }
        void remove() { m_Impl.DoRemoveHandlerArgument(m_Handler, m_Arg); }
        void Do() override
        {
            if (m_AddOnDo)
                insert();
            else
                remove();
        }
        void Undo() override
        {
            if (m_AddOnDo)
                remove();
            else
                insert();
        }
    };

    QT3DSU32 AddHandlerArgument(Qt3DSDMHandlerHandle inHandler) override
    {
        SHandler *infoPtr = FindHandler(inHandler);
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return 0;
        }

        SMetaDataHandlerArgumentInfo theInfo(inHandler);
        QT3DSU32 idx = (QT3DSU32)infoPtr->m_Arguments.size();
        DoAddHandlerArgument(inHandler, theInfo, idx);
        if (m_Consumer)
            m_Consumer->OnTransaction(make_shared<HandlerArgumentAddRemoveTransaction>(
                                          __FILE__, __LINE__, ref(*this), inHandler, theInfo,
                                          idx, true));

        return idx;
    }

    void DoSetHandlerArgument(THandlerHandle inHandler, QT3DSU32 inIdx,
                              const SMetaDataHandlerArgumentInfo &inInfo)
    {
        SMetaDataHandlerArgumentInfo *infoPtr(FindHandlerArg(inHandler, inIdx));
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        *infoPtr = inInfo;
    }

    struct SetHandlerArgumentInfoTrans : public ITransaction
    {
        SNewMetaDataImpl &m_Impl;
        THandlerHandle m_Handler;
        QT3DSU32 m_Idx;
        SMetaDataHandlerArgumentInfo m_NewValue;
        SMetaDataHandlerArgumentInfo m_OldValue;
        SetHandlerArgumentInfoTrans(const char *inFile, int inLine, SNewMetaDataImpl &impl,
                                    THandlerHandle handler, QT3DSU32 inIdx,
                                    const SMetaDataHandlerArgumentInfo &inNewVal,
                                    const SMetaDataHandlerArgumentInfo &inOldVal)
            : ITransaction(inFile, inLine)
            , m_Impl(impl)
            , m_Handler(handler)
            , m_Idx(inIdx)
            , m_NewValue(inNewVal)
            , m_OldValue(inOldVal)
        {
        }
        void Do() override { m_Impl.DoSetHandlerArgument(m_Handler, m_Idx, m_NewValue); }
        void Undo() override { m_Impl.DoSetHandlerArgument(m_Handler, m_Idx, m_OldValue); }
    };

    void SetHandlerArgumentInfo(THandlerHandle inHandler, QT3DSU32 inIdx, TStrType inName,
                                TStrType inFormalName, TStrType inDescription,
                                CompleteMetaDataType::Enum inDataType,
                                const SValue &inDefaultValue,
                                const TMetaDataData &inMetaData,
                                HandlerArgumentType::Value inArgType) override
    {
        SMetaDataHandlerArgumentInfo *infoPtr(FindHandlerArg(inHandler, inIdx));
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        SMetaDataHandlerArgumentInfo &newInfo(*infoPtr);
        SMetaDataHandlerArgumentInfo oldInfo(newInfo);
        SetPropertyBaseInfo(newInfo, inName, inFormalName, inDescription, L"", inDataType,
                            inDefaultValue, inMetaData);
        newInfo.m_ArgType = inArgType;
        if (m_Consumer != NULL)
            m_Consumer->OnTransaction(make_shared<SetHandlerArgumentInfoTrans>(
                                          __FILE__, __LINE__, ref(*this), inHandler, inIdx,
                                          newInfo, oldInfo));
    }

    void DestroyHandlerArgument(THandlerHandle inHandler, QT3DSU32 inIdx) override
    {
        SHandler *ownerPtr = FindHandler(inHandler);
        SMetaDataHandlerArgumentInfo *infoPtr(FindHandlerArg(inHandler, inIdx));
        if (infoPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        if (ownerPtr == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        Q_EMIT internalHandlerArgDestroyed(inHandler, inIdx);

        if (m_Consumer)
            m_Consumer->OnTransaction(make_shared<HandlerArgumentAddRemoveTransaction>(
                                          __FILE__, __LINE__, ref(*this), infoPtr->m_Handler,
                                          *infoPtr, inIdx, false));

        DoRemoveHandlerArgument(infoPtr->m_Handler, *infoPtr);
    }

    Option<SMetaDataHandlerArgumentInfo> FindHandlerArgumentByName(THandlerHandle inHandler,
                                                                   TStrType inName) override
    {
        SHandler *ownerPtr = FindHandler(inHandler);
        if (ownerPtr == NULL) {
            return Empty();
        }
        for (size_t idx = 0, end = ownerPtr->m_Arguments.size(); idx < end; ++idx) {
            if (ownerPtr->m_Arguments[idx].m_Name == inName)
                return ownerPtr->m_Arguments[idx];
        }
        return Empty();
    }

    void GetHandlerArguments(THandlerHandle inHandler,
                             vector<SMetaDataHandlerArgumentInfo> &outArguments) override
    {
        SHandler *ownerPtr = FindHandler(inHandler);
        if (ownerPtr == NULL) {
            return;
        }
        outArguments.insert(outArguments.end(), ownerPtr->m_Arguments.begin(),
                            ownerPtr->m_Arguments.end());
    }

    virtual Option<SMetaDataHandlerArgumentInfo>
    GetHandlerArgumentInfo(Qt3DSDMHandlerHandle inHandle, QT3DSU32 inIdx) override
    {
        SMetaDataHandlerArgumentInfo *infoPtr(FindHandlerArg(inHandle, inIdx));
        if (infoPtr == NULL) {
            return Empty();
        }
        return *infoPtr;
    }

    QT3DSU32 GetNumHandlerArguments(Qt3DSDMHandlerHandle inHandler) override
    {
        SHandler *ownerPtr = FindHandler(inHandler);
        if (ownerPtr == NULL) {
            return 0;
        }
        return (QT3DSU32)ownerPtr->m_Arguments.size();
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // References
    void AddReference(Qt3DSDMInstanceHandle inInstance, TStrType inRefString) override
    {
        // trim whitespace from the beginning and the end of the string
        TCharStr::size_type startPos = inRefString.find_first_not_of(L"\n\r\t ");
        TCharStr::size_type endPos = inRefString.find_last_not_of(L"\n\r\t ");
        const wchar_t *theStr = NULL;
        if (startPos != TCharStr::npos) {
            TStrType temp = inRefString.substr(startPos, endPos - startPos + 1);
            theStr = Intern(temp);
        } else
            theStr = Intern(inRefString);

        QT3DSU32 idx = AddItemToInstanceList(inInstance, theStr, m_InstanceToReferences);
        if (m_Consumer) {
            m_Consumer->OnTransaction(
                        std::make_shared<InstanceListTransaction<TInstanceStringListMap,
                        const wchar_t *>>(
                            __FILE__, __LINE__, inInstance, theStr,
                            std::ref(m_InstanceToReferences), idx,
                            true));
        }
    }

    void DestroyReferences(Qt3DSDMInstanceHandle inInstance) override
    {
        TInstanceStringListMap::iterator find = m_InstanceToReferences.find(inInstance);

        if (find == m_InstanceToReferences.end())
            return;

        CreateHashMapEraseTransaction(__FILE__, __LINE__, m_Consumer,
                                      make_pair(find->first, find->second), m_InstanceToReferences);
        m_InstanceToReferences.erase(find);
    }

    struct InternSizeTOpType
    {
        SNewMetaDataImpl &m_Impl;
        InternSizeTOpType(SNewMetaDataImpl &inImpl)
            : m_Impl(inImpl)
        {
        }

        size_t operator()(const TCharStr &inHandle)
        {
            return reinterpret_cast<size_t>(m_Impl.Intern(inHandle));
        }
    };

    void GetReferences(Qt3DSDMInstanceHandle inInstance, vector<TCharStr> &outReferences) override
    {
        DoGetHandleList(inInstance, m_InstanceToReferences, outReferences,
                        InternSizeTOpType(*this));
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Instance-global functions
    // Destroy all meta data that relates to this instance.
    // Calling this on a derived instance does nothing, this only works if this specific
    // instance was mapped to a type.
    // This function must be completely undoable.
    void DestroyMetaData(Qt3DSDMInstanceHandle inInstance) override
    {
        ForEachItem<TMetaDataPropertyHandle>(
                    inInstance, m_InstanceToProperties,
                    bind(&SNewMetaDataImpl::DestroyMetaDataProperty, this,
                         std::placeholders::_1));
        ForEachItem<TEventHandle>(inInstance, m_InstanceToEvents,
                                  bind(&SNewMetaDataImpl::DestroyEvent, this,
                                       std::placeholders::_1));
        ForEachItem<THandlerHandle>(inInstance, m_InstanceToHandlers,
                                    bind(&SNewMetaDataImpl::DestroyHandler, this,
                                         std::placeholders::_1));
        DestroyReferences(inInstance);

        TInstanceStrMap::iterator find = m_InstancesToCanonicalType.find(inInstance);
        if (find != m_InstancesToCanonicalType.end()) {
            TCharPtr theName(find->second);
            m_CanonicalTypeToInstances.erase(theName);
            m_InstancesToCanonicalType.erase(inInstance);
            CreateHashMapEraseTransaction(__FILE__, __LINE__, m_Consumer,
                                          make_pair(theName, inInstance),
                                          m_CanonicalTypeToInstances);
            CreateHashMapEraseTransaction(__FILE__, __LINE__, m_Consumer,
                                          make_pair(inInstance, theName),
                                          m_InstancesToCanonicalType);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Serialization
    // You can either save out in canonical format (and it will only save canonical-instance-related
    // information *or* you can save out in normal format where we link directly to instance handle
    // instead of to typename
    template <typename THashType>
    void AddInstancesFromHash(THashType &inHash,
                              unordered_set<TInstanceHandle, hash<int>> &outInstances)
    {
        for (typename THashType::const_iterator iter = inHash.begin(), end = inHash.end();
             iter != end; ++iter)
            outInstances.insert(iter->first);
    }

    void SerializeMetaDataData(IDOMWriter &inWriter, TMetaDataData &inItem,
                               AdditionalMetaDataType::Value inType)
    {
        if (!inItem.empty()) {
            if (inType == AdditionalMetaDataType::Range) {
                SMetaDataRange theRange(get<SMetaDataRange>(inItem));
                inWriter.Att(L"min", theRange.m_min);
                inWriter.Att(L"max", theRange.m_max);
                inWriter.Att(L"decimals", theRange.m_decimals);
            } else if (inType == AdditionalMetaDataType::StringList) {
                const TMetaDataStringList &theList(get<TMetaDataStringList>(inItem));
                TCharStr theBuffer;
                for (uint32_t idx = 0, end = theList.size(); idx < end; ++idx) {
                    if (idx)
                        theBuffer.append(L":");
                    theBuffer.append(theList[idx]);
                }
                inWriter.Att(L"list", theBuffer.wide_str());
            }
        }
    }

    void SerializeDataModelValue(IDOMWriter &inWriter, const SValue &inValue,
                                 DataModelDataType::Value /*inType*/,
                                 const wchar_t *inName = L"default")
    {
        if (inValue.empty())
            return;

        m_TempBuffer.clear();
        WCharTWriter writer(m_TempBuffer);
        WStrOps<SValue>().ToBuf(inValue, writer);

        if (m_TempBuffer.size()) {
            m_TempBuffer.write((QT3DSU16)0);
            inWriter.Att(inName, (const wchar_t *)m_TempBuffer.begin());
        }
    }

    void SerializeDataModelValue(IDOMReader &inReader, SValue &outValue,
                                 DataModelDataType::Value inType,
                                 const wchar_t *inName = L"default")
    {
        const char8_t *theDefaultValue;
        qt3ds::foundation::ConvertUTF(
                    reinterpret_cast<const qt3ds::foundation::TWCharEASTLConverter::TCharType *>(
                        inName), 0,
                    m_ConvertStr);
        if (inReader.UnregisteredAtt(m_ConvertStr.c_str(), theDefaultValue)) {
            m_TempBuffer.clear();
            // We have to write a temporary value because the parsing of floats,
            // in order to be efficient, is destructive.
            if (theDefaultValue && *theDefaultValue)
                m_TempBuffer.write(theDefaultValue, (QT3DSU32)strlen(theDefaultValue) + 1);

            if (m_TempBuffer.size() == 0) {
                SetDefault(inType, outValue);
                return;
            }
            m_ReadBuffer.clear();

            char8_t *trashPtr = (char8_t *)m_TempBuffer.begin();
            WCharTReader theReader(trashPtr, m_ReadBuffer, m_StringTable);
            outValue = WStrOps<SValue>().BufTo(inType, theReader);
        } else
            SetDefault(inType, outValue);
    }

    void SerializeMetaDataData(IDOMReader &inReader, TMetaDataData &ioItem,
                               CompleteMetaDataType::Enum &ioType)
    {
        // Use the meta data extra information to force the type
        // to something that works correctly.
        SMetaDataRange theRange;
        if (inReader.Att("min", theRange.m_min) && inReader.Att("max", theRange.m_max)) {
            inReader.Att("decimals", theRange.m_decimals);
            ioItem = theRange;
            if (ioType == CompleteMetaDataType::Long)
                ioType = CompleteMetaDataType::LongRange;
            else
                ioType = CompleteMetaDataType::FloatRange;
        } else {
            TMetaDataStringList theList;
            TCharStr theItems;
            if (inReader.Att(L"list", theItems)) {
                for (TCharStr::size_type theIter = theItems.find_first_of(L":,");
                     theIter != theItems.npos; theIter = theItems.find_first_of(L":,")) {
                    theList.push_back(theItems.substr(0, theIter));
                    theItems = theItems.substr(theIter + 1);
                }
                theList.push_back(theItems);
                ioItem = theList;
                if (ioType != CompleteMetaDataType::StringListOrInt)
                    ioType = CompleteMetaDataType::StringList;
            }
        }
    }
    void EnsureCategory(IDOMWriter &, const TCharStr &, const TCharStr &) {}
    void EnsureCategory(IDOMReader &, TCharStr &inCategory, const TCharStr &inObjectName)
    {
        Qt3DSDMCategoryHandle theCategory = FindCategoryByName(inCategory);
        if (theCategory.Valid() == false) {
            if (inObjectName.empty()) {
                QT3DS_ASSERT(false);
            } else {
                if (inCategory.empty())
                    inCategory = inObjectName;

                pair<Qt3DSDMCategoryHandle, bool> theGetOrCreateResult(
                            GetOrCreateCategory(inCategory));

                if (theGetOrCreateResult.second == true) {
                    SetCategoryInfo(theGetOrCreateResult.first, L"Objects-Behavior-Normal.png",
                                    L"Objects-Behavior-Normal.png", inCategory);
                }
            }
        }
    }

    void SerializePropertyBase(IDOMWriter &inArchive, SMetaPropertyBase &inItem)
    {
        inArchive.Att(L"name", inItem.m_Name);
        if (inItem.m_FormalName != inItem.m_Name)
            inArchive.Att(L"formalName", inItem.m_FormalName);
        if (inItem.m_Description != inItem.m_Name)
            inArchive.Att(L"description", inItem.m_Description);
        if (inItem.m_CompleteType != CompleteMetaDataType::Float
                && inItem.m_CompleteType != CompleteMetaDataType::FloatRange
                && inItem.m_CompleteType != CompleteMetaDataType::LongRange
                && inItem.m_CompleteType != CompleteMetaDataType::StringList) {
            inArchive.Att(L"type", inItem.m_CompleteType);
        }

        // Ensure that all types work
        if (inItem.GetAdditionalType() != AdditionalMetaDataType::None)
            SerializeMetaDataData(inArchive, inItem.m_MetaDataData, inItem.GetAdditionalType());

        if (inItem.GetDataType() != DataModelDataType::None) {
            SValue theGlobalDefault;
            SetDefault(inItem.GetDataType(), theGlobalDefault);
            if (!Equals(theGlobalDefault.toOldSkool(), inItem.m_DefaultValue.toOldSkool()))
                SerializeDataModelValue(inArchive, inItem.m_DefaultValue, inItem.GetDataType());
        }
    }

    void SerializePropertyBase(IDOMReader &inArchive, SMetaPropertyBase &inItem)
    {
        inArchive.Att(L"name", inItem.m_Name);
        inArchive.Att(L"formalName", inItem.m_FormalName);
        inArchive.Att(L"usage", inItem.m_Usage);
        inArchive.Att(L"description", inItem.m_Description);
        inArchive.Att(L"type", inItem.m_CompleteType);
        // Setup reasonable defaults in the case where the users are specifying little information
        // in the file format
        if (inItem.m_FormalName.empty())
            inItem.m_FormalName = inItem.m_Name;
        if (inItem.m_Description.empty())
            inItem.m_Description = inItem.m_FormalName;

        // Ensure that users can use a float type and make minimal decisions
        SerializeMetaDataData(inArchive, inItem.m_MetaDataData, inItem.m_CompleteType);

        if (inItem.GetDataType() != DataModelDataType::None)
            SerializeDataModelValue(inArchive, inItem.m_DefaultValue, inItem.GetDataType());
    }

    void FinalizeCategory(IDOMWriter &, SCategoryInfo &) {}

    void FinalizeCategory(IDOMReader &, SCategoryInfo &inCategory)
    {
        if (inCategory.m_Description.empty())
            inCategory.m_Description = inCategory.m_Name;
    }

    template <typename TArchiveType>
    void SerializeItem(TArchiveType &inArchive, SCategoryInfo &inItem)
    {
        inArchive.Att(L"name", inItem.m_Name);
        inArchive.Att(L"description", inItem.m_Description);
        inArchive.Att(L"icon", inItem.m_Icon);
        inArchive.Att(L"highlightIcon", inItem.m_HighlightIcon);

        FinalizeCategory(inArchive, inItem);
    }

    void SerializeItem(IDOMWriter &inArchive, SMetaDataPropertyInfo &inItem,
                       Qt3DSDMMetaDataPropertyHandle inHandle)
    {
        SerializePropertyBase(inArchive, inItem);
        if (inItem.m_IsHidden == true)
            inArchive.Att("hidden", inItem.m_IsHidden);
        if (inItem.m_Animatable == false)
            inArchive.Att("animatable", inItem.m_Animatable);
        if (inItem.m_Controllable == true)
            inArchive.Att("controllable", inItem.m_Controllable);
        NVConstDataRef<SPropertyFilterInfo> theInfos = GetMetaDataPropertyFilters(inHandle);
        for (QT3DSU32 idx = 0, end = theInfos.size(); idx < end; ++idx) {
            const SPropertyFilterInfo &theInfo(theInfos[idx]);
            Qt3DSDMPropertyDefinition thePropertyInfo(
                        m_DataCore->GetProperty(theInfo.m_FilterProperty));

            const wchar_t *theFilterName;
            if (theInfo.m_FilterType == PropertyFilterTypes::HideIfEqual)
                theFilterName = L"HideIfEqual";
            else
                theFilterName = L"ShowIfEqual";

            IDOMWriter::Scope filterScope(inArchive, theFilterName);
            inArchive.Att(L"property", thePropertyInfo.m_Name);
            SerializeDataModelValue(inArchive, theInfo.m_Value, thePropertyInfo.m_Type, L"value");
        }
    }

    void SerializeItem(IDOMReader &inArchive, SMetaDataPropertyInfo &inItem,
                       Qt3DSDMMetaDataPropertyHandle inHandle)
    {
        QT3DS_ASSERT(inHandle.Valid());

        SerializePropertyBase(inArchive, inItem);
        inArchive.Att("hidden", inItem.m_IsHidden);
        inArchive.Att("animatable", inItem.m_Animatable);
        inArchive.Att("controllable", inItem.m_Controllable);
        inArchive.Att(L"category", inItem.m_GroupName);
    }

    void ReadChildren(IDOMReader &inArchive, SMetaDataPropertyInfo &inItem,
                      Qt3DSDMMetaDataPropertyHandle inHandle)
    {
        IDOMReader::Scope __readerScope(inArchive);
        eastl::vector<SPropertyFilterInfo> theFilters;
        for (bool success = inArchive.MoveToFirstChild(); success;
             success = inArchive.MoveToNextSibling()) {

            if (AreEqual(inArchive.GetElementName(), L"ShowIfEqual")) {
                TCharStr theName;
                inArchive.Att(L"property", theName);
                Qt3DSDMPropertyHandle theProperty =
                        m_DataCore->GetAggregateInstancePropertyByName(inItem.m_Instance, theName);
                if (theProperty.Valid() == false) {
                    QT3DS_ASSERT(false);
                    return;
                }
                Qt3DSDMPropertyDefinition thePropDef(m_DataCore->GetProperty(theProperty));

                SPropertyFilterInfo theInfo;
                theInfo.m_FilterType = PropertyFilterTypes::ShowIfEqual;
                theInfo.m_FilterProperty = theProperty;
                SerializeDataModelValue(inArchive, theInfo.m_Value, thePropDef.m_Type, L"value");
                theFilters.push_back(theInfo);
            } else if (AreEqual(inArchive.GetElementName(), L"HideIfEqual")) {
                TCharStr theName;
                inArchive.Att(L"property", theName);
                Qt3DSDMPropertyHandle theProperty =
                        m_DataCore->GetAggregateInstancePropertyByName(inItem.m_Instance, theName);
                if (theProperty.Valid() == false) {
                    QT3DS_ASSERT(false);
                    return;
                }
                Qt3DSDMPropertyDefinition thePropDef(m_DataCore->GetProperty(theProperty));

                SPropertyFilterInfo theInfo;
                theInfo.m_FilterType = PropertyFilterTypes::HideIfEqual;
                theInfo.m_FilterProperty = theProperty;
                SerializeDataModelValue(inArchive, theInfo.m_Value, thePropDef.m_Type, L"value");
                theFilters.push_back(theInfo);
            } else {
                QT3DS_ASSERT(false);
            }
        }
        if (theFilters.size())
            SetMetaDataPropertyFilters(
                        inHandle, qt3ds::foundation::toDataRef(theFilters.data(),
                                                               theFilters.size()));
    }

    void EnsureEventHandlerBase(IDOMWriter &, SEventAndHandlerBase &) {}

    void EnsureEventHandlerBase(IDOMReader &, SEventAndHandlerBase &inItem)
    {
        if (inItem.m_FormalName.empty())
            inItem.m_FormalName = inItem.m_Name;
        if (inItem.m_Description.empty())
            inItem.m_Description = inItem.m_Name;
    }

    void SerializeItem(IDOMWriter &inArchive, SEvent &inItem, Qt3DSDMEventHandle &)
    {
        inArchive.Att(L"name", inItem.m_Name);
        if (inItem.m_Name != inItem.m_FormalName)
            inArchive.Att(L"formalName", inItem.m_FormalName);
        if (inItem.m_Category != L"Default")
            inArchive.Att(L"category", inItem.m_Category);
        if (inItem.m_Description != inItem.m_Name)
            inArchive.Att(L"description", inItem.m_Description);

        EnsureCategory(inArchive, inItem.m_Category, m_ObjectName);
        EnsureEventHandlerBase(inArchive, inItem);
    }

    void SerializeItem(IDOMReader &inArchive, SEvent &inItem, Qt3DSDMEventHandle &)
    {
        inArchive.Att(L"name", inItem.m_Name);
        inArchive.Att(L"formalName", inItem.m_FormalName);
        inArchive.Att(L"category", inItem.m_Category);
        EnsureCategory(inArchive, inItem.m_Category, m_ObjectName);
        inArchive.Att(L"description", inItem.m_Description);
        EnsureEventHandlerBase(inArchive, inItem);
    }

    void SerializeHandlerArgument(IDOMWriter &inArchive, SMetaDataHandlerArgumentInfo &inArgument)
    {
        SerializePropertyBase(inArchive, inArgument);
        if (inArgument.m_ArgType != HandlerArgumentType::None)
            inArchive.Att(L"argumentType", inArgument.m_ArgType);
    }

    void SerializeHandlerArgument(IDOMReader &inArchive, SMetaDataHandlerArgumentInfo &inArgument)
    {
        SerializePropertyBase(inArchive, inArgument);
        inArchive.Att(L"argumentType", inArgument.m_ArgType);
    }

    void SerializeHandlerArguments(IDOMWriter &inWriter, SHandler &inItem, Qt3DSDMHandlerHandle &)
    {
        for (size_t idx = 0, end = inItem.m_Arguments.size(); idx < end; ++idx) {
            SMetaDataHandlerArgumentInfo &theArg(inItem.m_Arguments[idx]);
            IDOMWriter::Scope __argScope(inWriter, L"Argument");
            SerializeHandlerArgument(inWriter, theArg);
        }
    }

    void SerializeHandlerArguments(IDOMReader &inReader, SHandler &inItem, THandlerHandle &inHandle)
    {
        if (inHandle.GetHandleValue() == 0)
            inHandle = THandlerHandle(GetNextId());

        IDOMReader::Scope __handlerScope(inReader);
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            SMetaDataHandlerArgumentInfo theArg(inHandle);
            SerializeHandlerArgument(inReader, theArg);
            inItem.m_Arguments.push_back(theArg);
        }
    }

    void SerializeHandlerItem(IDOMWriter &inArchive, SHandler &inItem)
    {
        inArchive.Att(L"name", inItem.m_Name);
        if (inItem.m_FormalName != inItem.m_Name)
            inArchive.Att(L"formalName", inItem.m_FormalName);
        if (inItem.m_Category != L"Default")
            inArchive.Att(L"category", inItem.m_Category);
        if (inItem.m_Description != inItem.m_Name)
            inArchive.Att(L"description", inItem.m_Description);
        EnsureEventHandlerBase(inArchive, inItem);
    }

    void SerializeHandlerItem(IDOMReader &inArchive, SHandler &inItem)
    {
        inArchive.Att(L"name", inItem.m_Name);
        inArchive.Att(L"formalName", inItem.m_FormalName);
        inArchive.Att(L"category", inItem.m_Category);
        EnsureCategory(inArchive, inItem.m_Category, m_ObjectName);
        inArchive.Att(L"description", inItem.m_Description);
        EnsureEventHandlerBase(inArchive, inItem);
    }

    template <typename TArchiveType>
    void SerializeItem(TArchiveType &inArchive, SHandler &inItem, Qt3DSDMHandlerHandle &inHandle)
    {
        SerializeHandlerItem(inArchive, inItem);
        SerializeHandlerArguments(inArchive, inItem, inHandle);
    }

    template <typename TInfoType, typename THandleType, typename TListMapType, typename THashType,
              typename TNameMapType>
    void SerializeInstanceData(IDOMWriter &inArchive, TInstanceHandle inInstanceHandle,
                               TCharPtr inElementName,
                               unordered_map<THandleType, TInfoType, THashType> &inMap,
                               TListMapType &inListMap, TNameMapType & /*inNameMap*/)
    {
        typedef unordered_map<THandleType, TInfoType, THashType> TMapType;
        typename TListMapType::iterator find = inListMap.find(inInstanceHandle);
        if (find == inListMap.end())
            return;

        vector<THandleType> &itemList = find->second;
        for (size_t idx = 0, end = itemList.size(); idx < end; ++idx) {
            typename TMapType::iterator iter = inMap.find(itemList[idx]);
            if (iter == inMap.end()) {
                QT3DS_ASSERT(false);
                continue;
            }
            TInfoType &theType = iter->second;
            IDOMWriter::Scope __elemScope(inArchive, inElementName);
            SerializeItem(inArchive, theType, itemList[idx]);
        }
    }

    void CreateInstanceGroupInfo(SMetaDataPropertyInfo &inProperty)
    {
        if (!inProperty.m_GroupName.empty()) {
            const wchar_t *theGroupName = Intern(inProperty.m_GroupName);
            bool found = false;
            pair<typename TInstanceGroupMap::iterator, bool> inserter =
                    m_InstanceGroupMap.insert(make_pair(inProperty.m_Instance, vector<TCharPtr>()));
            vector<TCharPtr> &itemList = inserter.first->second;
            for (size_t idx = 0, end = itemList.size(); idx < end; ++idx) {
                TCharStr curListName = itemList[idx];
                if (curListName == theGroupName) {
                    found = true;
                    break;
                }
            }
            if (!found && !inProperty.m_IsHidden)
                inserter.first->second.push_back(theGroupName);
        }
    }
    void CreateInstanceGroupInfo(SEvent &) {}
    void CreateInstanceGroupInfo(SHandler &) {}

    // Make sure the data core
    void PostLoad(SMetaDataPropertyInfo &inProperty)
    {
        EnsureDataCoreProperty(inProperty);
        SetPropertyDefault(inProperty, inProperty.GetDataType());
    }

    void ReadChildren(IDOMReader &, SEvent &, Qt3DSDMEventHandle) {}
    void ReadChildren(IDOMReader &, SHandler &, Qt3DSDMHandlerHandle) {}

    void PostLoad(SEvent &) {}
    void PostLoad(SHandler &) {}
    template <typename TInfoType, typename THandleType, typename TListMapType, typename THashType,
              typename TNameMapType>
    void SerializeInstanceData(IDOMReader &inArchive, TInstanceHandle inInstanceHandle,
                               TCharPtr inElementName,
                               unordered_map<THandleType, TInfoType, THashType> &inMap,
                               TListMapType &inListMap, TNameMapType &inNameMap)
    {
        // Ensure we pop out to where we were.
        IDOMReader::Scope __readerScope(inArchive);
        for (bool success = inArchive.MoveToFirstChild(inElementName); success;
             success = inArchive.MoveToNextSibling(inElementName)) {
            TInfoType theInfo(inInstanceHandle);
            THandleType theHandle(GetNextId());
            SerializeItem(inArchive, theInfo, theHandle);
            PostLoad(theInfo);
            CreateInstanceGroupInfo(theInfo);
            inMap.insert(make_pair(theHandle, theInfo));
            this->AddItemToInstanceList(inInstanceHandle, theHandle, inListMap);
            inNameMap.insert(
                        make_pair(make_pair(inInstanceHandle, Intern(theInfo.m_Name)), theHandle));
            ReadChildren(inArchive, theInfo, theHandle);
        }
    }

    void SerializeInstanceData(IDOMWriter &inArchive, TInstanceHandle inInstance,
                               TCharPtr inElementName, TInstanceStringListMap &inMap)
    {
        TInstanceStringListMap::iterator iter = inMap.find(inInstance);
        if (iter == inMap.end())
            return;
        const vector<TCharPtr> &theValueList(iter->second);
        for (size_t idx = 0, end = theValueList.size(); idx < end; ++idx) {
            IDOMWriter::Scope __elemScope(inArchive, inElementName);
            inArchive.Value(theValueList[idx]);
        }
    }

    void SerializeInstanceData(IDOMReader &inArchive, TInstanceHandle inInstance,
                               TCharPtr inElementName, TInstanceStringListMap &inMap)
    {
        IDOMReader::Scope __readerScope(inArchive);
        for (bool success = inArchive.MoveToFirstChild(inElementName); success;
             success = inArchive.MoveToNextSibling(inElementName)) {
            const wchar_t *theValue;
            if (inArchive.RegisteredValue(theValue))
                AddItemToInstanceList(inInstance, theValue, inMap);
        }
    }

    template <typename TArchiveType>
    void SerializeInstance(TArchiveType &inArchive, TInstanceHandle inInstance)
    {
        SerializeInstanceData(inArchive, inInstance, L"Property", m_Properties,
                              m_InstanceToProperties, m_InstanceNameToProperties);
        SerializeInstanceData(inArchive, inInstance, L"Event", m_Events, m_InstanceToEvents,
                              m_InstanceNameToEvents);
        SerializeInstanceData(inArchive, inInstance, L"Handler", m_Handlers, m_InstanceToHandlers,
                              m_InstanceNameToHandlers);
        SerializeInstanceData(inArchive, inInstance, L"Reference", m_InstanceToReferences);
    }
    void SerializeCategories(IDOMWriter &inWriter)
    {
        for (TCategoryMap::iterator iter = m_Categories.begin(), end = m_Categories.end();
             iter != end; ++iter) {
            IDOMWriter::Scope __writerScope(inWriter, L"Category");
            SerializeItem(inWriter, iter->second);
        }
    }
    void SerializeCategories(IDOMReader &inReader)
    {
        IDOMReader::Scope __readerScope(inReader);
        for (bool success = inReader.MoveToFirstChild(L"Category"); success;
             success = inReader.MoveToNextSibling(L"Category")) {
            SCategoryInfo theInfo;
            SerializeItem(inReader, theInfo);
            TCategoryHandle newHandle(GetNextId());
            theInfo.m_Canonical = true;
            m_Categories.insert(make_pair(newHandle, theInfo));
            m_NameToCategories.insert(make_pair(Intern(theInfo.m_Name), newHandle));
        }
    }

    struct SInstanceSorter
    {
        const TInstanceStrMap &m_Map;
        SInstanceSorter(const TInstanceStrMap &inMap)
            : m_Map(inMap)
        {
        }
        bool operator()(TInstanceHandle lhs, TInstanceHandle rhs)
        {
            TInstanceStrMap::const_iterator lhsIter(m_Map.find(lhs));
            TInstanceStrMap::const_iterator rhsIter(m_Map.find(rhs));
            if (lhsIter == rhsIter)
                return false;
            if (lhsIter == m_Map.end())
                return false;
            if (rhsIter == m_Map.end())
                return true;
            return wcscmp(lhsIter->second, rhsIter->second) < 0;
        }
    };

    void Save(IDOMWriter &inWriter) override
    {
        SerializeCategories(inWriter);
        typedef unordered_set<TInstanceHandle, hash<int>> TInstanceListType;
        TInstanceListType theInstanceList;
        // Get the list of instances to serialize.

        AddInstancesFromHash(m_InstanceToProperties, theInstanceList);
        AddInstancesFromHash(m_InstanceToEvents, theInstanceList);
        AddInstancesFromHash(m_InstanceToHandlers, theInstanceList);
        AddInstancesFromHash(m_InstanceToReferences, theInstanceList);

        vector<TInstanceHandle> theInstances;
        theInstances.reserve(theInstanceList.size());
        theInstances.insert(theInstances.begin(), theInstanceList.begin(), theInstanceList.end());
        sort(theInstances.begin(), theInstances.end(), SInstanceSorter(m_InstancesToCanonicalType));

        for (vector<TInstanceHandle>::iterator iter = theInstances.begin(),
             end = theInstances.end();
             iter != end; ++iter) {
            TInstanceHandle instHandle = *iter;
            Option<TCharStr> theType(GetTypeForCanonicalInstance(*iter));
            if (theType.hasValue()) {
                IDOMWriter::Scope __instanceElem(inWriter, theType->wide_str());
                SerializeInstance(inWriter, *iter);
            }
        }
    }

    // Loading expects the canonical instances to be setup already which means that
    // it will perform lookups based on
    void Load(IDOMReader &inReader) override
    {
        if (!inReader.Att("NextId", m_NextId))
            m_NextId = 1;
        m_ObjectName = L"";
        SerializeCategories(inReader);
        IDOMReader::Scope __instanceGatherScope(inReader);
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            const wchar_t *elemName = inReader.GetElementName();
            TStrInstanceMap::iterator find = m_CanonicalTypeToInstances.find(elemName);
            if (find == m_CanonicalTypeToInstances.end()) {
                continue;
            }

            SerializeInstance(inReader, find->second);
        }
    }

    template <typename THandleType, typename TMapType>
    void RemoveOldItemsAndSetOrder(
            Qt3DSDMInstanceHandle inInstance, vector<THandleType> &inNewHandles,
            void (SNewMetaDataImpl::*inGetSpecificFun)(Qt3DSDMInstanceHandle, vector<THandleType> &),
            void (SNewMetaDataImpl::*inDestroyFun)(THandleType), TMapType &inListMap)
    {
        vector<THandleType> theHandles;
        (this->*inGetSpecificFun)(inInstance, theHandles);
        for (size_t idx = 0, end = theHandles.size(); idx < end; ++idx)
            if (find(inNewHandles.begin(), inNewHandles.end(), theHandles[idx])
                    == inNewHandles.end())
                (this->*inDestroyFun)(theHandles[idx]);

        InsertOrUpdateWithTransaction(make_pair(inInstance, inNewHandles), inListMap);
    }

    // Load meta data and apply it to just this instance
    // This needs to be undoable so we have to do this through a slightly different
    // system than we did before.
    void LoadInstance(IDOMReader &inReader, Qt3DSDMInstanceHandle inInstance,
                      const TCharStr &inObjectName,
                      std::vector<SMetaDataLoadWarning> &outWarnings) override
    {
        const wchar_t *theAtt;
        vector<Qt3DSDMMetaDataPropertyHandle> theProperties;
        vector<Qt3DSDMEventHandle> theEvents;
        vector<Qt3DSDMHandlerHandle> theHandlers;
        DestroyReferences(inInstance);
        m_ObjectName = inObjectName;
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            if (AreEqual(inReader.GetElementName(), L"Category")) {
                SCategoryInfo theInfo;
                SerializeItem(inReader, theInfo);
                Qt3DSDMCategoryHandle theCategoryInfo(GetOrCreateCategory(theInfo.m_Name).first);
                SetCategoryInfo(theCategoryInfo, theInfo.m_Icon, theInfo.m_HighlightIcon,
                                theInfo.m_Description);
            } else if (AreEqual(inReader.GetElementName(), L"Property")) {
                SMetaDataPropertyInfo theInfo(inInstance);
                if (inReader.Att(L"name", theAtt)) {
                    Qt3DSDMMetaDataPropertyHandle theProperty(
                                GetOrCreateSpecificMetaDataProperty(inInstance, theAtt));
                    SerializeItem(inReader, theInfo, theProperty);
                    SetMetaDataPropertyInfo(theProperty, theInfo.m_Name, theInfo.m_FormalName,
                                            theInfo.m_Description, theInfo.m_Usage,
                                            theInfo.m_CompleteType, theInfo.m_DefaultValue,
                                            theInfo.m_MetaDataData, theInfo.m_GroupName,
                                            theInfo.m_IsHidden, theInfo.m_Animatable,
                                            theInfo.m_Controllable);
                    CreateInstanceGroupInfo(theInfo);
                    theProperties.push_back(theProperty);
                    ReadChildren(inReader, theInfo, theProperty);
                } else
                    outWarnings.push_back(
                                SMetaDataLoadWarning(MetaDataLoadWarningType::InvalidProperty,
                                                     MetaDataLoadWarningMessage::MissingName));
            } else if (AreEqual(inReader.GetElementName(), L"Event")) {
                SEvent theInfo(inInstance);
                if (inReader.Att(L"name", theAtt)) {
                    Qt3DSDMEventHandle theEvent(GetOrCreateSpecificEvent(inInstance, theAtt));
                    SerializeItem(inReader, theInfo, theEvent);
                    SetEventInfo(theEvent, theInfo.m_Name, theInfo.m_FormalName, theInfo.m_Category,
                                 theInfo.m_Description);
                    theEvents.push_back(theEvent);
                } else
                    outWarnings.push_back(
                                SMetaDataLoadWarning(MetaDataLoadWarningType::InvalidEvent,
                                                     MetaDataLoadWarningMessage::MissingName));
            } else if (AreEqual(inReader.GetElementName(), L"Handler")) {
                if (inReader.Att(L"name", theAtt)) {
                    Qt3DSDMHandlerHandle theHandler(GetOrCreateSpecificHandler(inInstance, theAtt));
                    SHandler theInfo(inInstance);
                    SerializeHandlerItem(inReader, theInfo);
                    SetHandlerInfo(theHandler, theInfo.m_Name, theInfo.m_FormalName,
                                   theInfo.m_Category, theInfo.m_Description);
                    IDOMReader::Scope __argScope(inReader);
                    QT3DSU32 argIdx = 0;
                    for (bool argSuccess = inReader.MoveToFirstChild(); argSuccess;
                         argSuccess = inReader.MoveToNextSibling(), ++argIdx) {
                        SMetaDataHandlerArgumentInfo theArg(theHandler);
                        SerializeHandlerArgument(inReader, theArg);
                        while (argIdx >= GetNumHandlerArguments(theHandler))
                            AddHandlerArgument(theHandler);

                        if (argIdx < GetNumHandlerArguments(theHandler)) {
                            SetHandlerArgumentInfo(theHandler, argIdx, theArg.m_Name,
                                                   theArg.m_FormalName, theArg.m_Description,
                                                   theArg.m_CompleteType, theArg.m_DefaultValue,
                                                   theArg.m_MetaDataData, theArg.m_ArgType);
                        }
                    }
                    while (GetNumHandlerArguments(theHandler) > argIdx)
                        DestroyHandlerArgument(theHandler, GetNumHandlerArguments(theHandler) - 1);
                    theHandlers.push_back(theHandler);
                } else
                    outWarnings.push_back(
                                SMetaDataLoadWarning(MetaDataLoadWarningType::InvalidHandler,
                                                     MetaDataLoadWarningMessage::MissingName));
            } else if (AreEqual(inReader.GetElementName(), L"Reference")) {
                const wchar_t *theValue;
                if (inReader.Value(theValue))
                    AddReference(inInstance, theValue);
            } else {
                QT3DS_ASSERT(false);
            }
        }
        RemoveOldItemsAndSetOrder(
                    inInstance, theProperties, &SNewMetaDataImpl::GetSpecificMetaDataProperties,
                    &SNewMetaDataImpl::DestroyMetaDataProperty, m_InstanceToProperties);
        RemoveOldItemsAndSetOrder(inInstance, theEvents, &SNewMetaDataImpl::GetSpecificEvents,
                                  &SNewMetaDataImpl::DestroyEvent, m_InstanceToEvents);
        RemoveOldItemsAndSetOrder(inInstance, theHandlers, &SNewMetaDataImpl::GetSpecificHandlers,
                                  &SNewMetaDataImpl::DestroyHandler, m_InstanceToHandlers);
    }

    // Save just this instances meta data out to the writer
    void SaveInstance(IDOMWriter &inWriter, Qt3DSDMInstanceHandle inInstance) override
    {
        SerializeInstance(inWriter, inInstance);
    }

    // Helper to convert char to wchar_t
    eastl::basic_string<qt3ds::foundation::TWCharEASTLConverter::TCharType> m_Buf;
    const wchar_t *ConvertChar(const char *inName)
    {
        if (inName && *inName) {
            qt3ds::foundation::ConvertUTF(inName, 0, m_Buf);
            return reinterpret_cast<const wchar_t *>(m_Buf.c_str());
        }
        return NULL;
    }

    void LoadEffectInstance(const char *inShaderFile, Qt3DSDMInstanceHandle inInstance,
                            const TCharStr &inObjectName,
                            std::vector<SMetaDataLoadWarning> &outWarnings,
                            qt3ds::foundation::IInStream &inStream) override
    {
        QString shaderFile(inShaderFile);
        if (shaderFile.endsWith(".effect")) {
            LoadEffectXMLFromSourcePath(inShaderFile, inInstance, inObjectName, outWarnings,
                                        inStream);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    bool IsEffectInstanceRegistered(const char *inName) override
    {
        return m_EffectMap.find(Intern(inName)) != m_EffectMap.end();
    }

    inline qt3ds::render::NVRenderTextureFormats::Enum
    ConvertTypeAndFormatToTextureFormat(const char8_t *inType, const char8_t *inFormat)
    {
        qt3ds::render::NVRenderTextureFormats::Enum retval
                = qt3ds::render::NVRenderTextureFormats::RGBA8;
        if (AreEqual(inFormat, "source"))
            retval = qt3ds::render::NVRenderTextureFormats::Unknown;
        else if (AreEqual(inFormat, "depth24stencil8"))
            retval = qt3ds::render::NVRenderTextureFormats::Depth24Stencil8;
        else {
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
                else if (AreEqual(inFormat, "rg"))
                    retval = qt3ds::render::NVRenderTextureFormats::RG8;
            } else if (AreEqual(inType, "ushort")) {
                if (AreEqual(inFormat, "rgb"))
                    retval = qt3ds::render::NVRenderTextureFormats::RGB565;
                else if (AreEqual(inFormat, "rgba"))
                    retval = qt3ds::render::NVRenderTextureFormats::RGBA5551;
            } else if (AreEqual(inType, "fp16")) {
                if (AreEqual(inFormat, "rgba"))
                    retval = qt3ds::render::NVRenderTextureFormats::RGBA16F;
                else if (AreEqual(inFormat, "rg"))
                    retval = qt3ds::render::NVRenderTextureFormats::RG16F;
            } else if (AreEqual(inType, "fp32")) {
                if (AreEqual(inFormat, "rgba"))
                    retval = qt3ds::render::NVRenderTextureFormats::RGBA32F;
                else if (AreEqual(inFormat, "rg"))
                    retval = qt3ds::render::NVRenderTextureFormats::RG32F;
            } else {
                QT3DS_ASSERT(false);
                // inFoundation.error( QT3DS_INVALID_PARAMETER, "Unsupported texture type %s,
                // defaulting to RGBA8", inType );
            }
        }
        return retval;
    }

    static inline qt3ds::render::NVRenderTextureMagnifyingOp::Enum
    ConvertFilterToMagOp(const char8_t *inFilter)
    {
        if (AreEqual(inFilter, "linear"))
            return qt3ds::render::NVRenderTextureMagnifyingOp::Linear;
        if (IsTrivial(inFilter) || AreEqual(inFilter, "nearest"))
            return qt3ds::render::NVRenderTextureMagnifyingOp::Nearest;
        else {
            QT3DS_ASSERT(false);
            // inFoundation.error( QT3DS_INVALID_PARAMETER, "Unsupported filter type %s, defaulting to
            // linear", inFilter );
            return qt3ds::render::NVRenderTextureMagnifyingOp::Linear;
        }
    }

    static inline qt3ds::render::NVRenderTextureMinifyingOp::Enum
    ConvertFilterToMinOp(const char8_t *inFilter)
    {
        // we make the decision based on the texture usage
        if (AreEqual(inFilter, "linear"))
            return qt3ds::render::NVRenderTextureMinifyingOp::Linear;
        if (AreEqual(inFilter, "nearest"))
            return qt3ds::render::NVRenderTextureMinifyingOp::Nearest;
        if (AreEqual(inFilter, "linearMipmapLinear"))
            return qt3ds::render::NVRenderTextureMinifyingOp::LinearMipmapLinear;
        if (AreEqual(inFilter, "nearestMipmapNearest"))
            return qt3ds::render::NVRenderTextureMinifyingOp::NearestMipmapNearest;
        if (AreEqual(inFilter, "nearestMipmapLinear"))
            return qt3ds::render::NVRenderTextureMinifyingOp::NearestMipmapLinear;
        if (AreEqual(inFilter, "linearMipmapNearest"))
            return qt3ds::render::NVRenderTextureMinifyingOp::LinearMipmapNearest;
        else {
            QT3DS_ASSERT(false);
            // inFoundation.error( QT3DS_INVALID_PARAMETER, "Unsupported filter type %s, defaulting
            // to linear", inFilter );
            return qt3ds::render::NVRenderTextureMinifyingOp::Linear;
        }
    }

    static inline qt3ds::render::NVRenderTextureCoordOp::Enum
    ConvertTextureCoordOp(const char8_t *inWrap)
    {
        if (IsTrivial(inWrap) || AreEqual(inWrap, "clamp"))
            return qt3ds::render::NVRenderTextureCoordOp::ClampToEdge;
        if (AreEqual(inWrap, "repeat"))
            return qt3ds::render::NVRenderTextureCoordOp::Repeat;
        else {
            QT3DS_ASSERT(false);
            // inFoundation.error( QT3DS_INVALID_PARAMETER, "Unsupported wrap type %s, defaulting to
            // clamp", inWrap );
            return qt3ds::render::NVRenderTextureCoordOp::ClampToEdge;
        }
    }

    static inline qt3ds::render::NVRenderTextureTypeValue::Enum
    ConvertTextureType(const char8_t *inTexType)
    {
        // this usually comes from a MDL description file
        if (IsTrivial(inTexType))
            return qt3ds::render::NVRenderTextureTypeValue::Unknown;
        if (AreEqual(inTexType, "environment"))
            return qt3ds::render::NVRenderTextureTypeValue::Environment;
        if (AreEqual(inTexType, "diffuse"))
            return qt3ds::render::NVRenderTextureTypeValue::Diffuse;
        if (AreEqual(inTexType, "specular"))
            return qt3ds::render::NVRenderTextureTypeValue::Specular;
        if (AreEqual(inTexType, "bump"))
            return qt3ds::render::NVRenderTextureTypeValue::Bump;
        if (AreEqual(inTexType, "displacement"))
            return qt3ds::render::NVRenderTextureTypeValue::Displace;
        if (AreEqual(inTexType, "shadow"))
            return qt3ds::render::NVRenderTextureTypeValue::LightmapShadow;
        if (AreEqual(inTexType, "emissive"))
            return qt3ds::render::NVRenderTextureTypeValue::Emissive;
        if (AreEqual(inTexType, "emissive_mask"))
            return qt3ds::render::NVRenderTextureTypeValue::Emissive2;
        else {
            return qt3ds::render::NVRenderTextureTypeValue::Unknown;
        }
    }

    static inline qt3ds::render::NVRenderSrcBlendFunc::Enum
    ConvertToSrcBlendFunc(const char8_t *inFilter)
    {
        if (AreEqual(inFilter, "SrcAlpha"))
            return qt3ds::render::NVRenderSrcBlendFunc::SrcAlpha;
        if (AreEqual(inFilter, "OneMinusSrcAlpha"))
            return qt3ds::render::NVRenderSrcBlendFunc::OneMinusSrcAlpha;
        if (AreEqual(inFilter, "One"))
            return qt3ds::render::NVRenderSrcBlendFunc::One;
        else {
            QT3DS_ASSERT(false);
            // inFoundation.error( QT3DS_INVALID_PARAMETER, "Unsupported filter type %s, defaulting
            // to linear", inFilter );
            return qt3ds::render::NVRenderSrcBlendFunc::One;
        }
    }

    static inline qt3ds::render::NVRenderDstBlendFunc::Enum
    ConvertToDstBlendFunc(const char8_t *inFilter)
    {
        if (AreEqual(inFilter, "SrcAlpha"))
            return qt3ds::render::NVRenderDstBlendFunc::SrcAlpha;
        if (AreEqual(inFilter, "OneMinusSrcAlpha"))
            return qt3ds::render::NVRenderDstBlendFunc::OneMinusSrcAlpha;
        if (AreEqual(inFilter, "One"))
            return qt3ds::render::NVRenderDstBlendFunc::One;
        else {
            QT3DS_ASSERT(false);
            // inFoundation.error( QT3DS_INVALID_PARAMETER, "Unsupported filter type %s, defaulting
            // to linear", inFilter );
            return qt3ds::render::NVRenderDstBlendFunc::One;
        }
    }

    static inline qt3ds::render::NVRenderState::Enum ConvertRenderState(const char8_t *inState)
    {
        if (AreEqual(inState, "Stencil"))
            return qt3ds::render::NVRenderState::StencilTest;
        else {
            QT3DS_ASSERT(false);
            // inFoundation.error( QT3DS_INVALID_PARAMETER, "Unsupported filter type %s, defaulting
            // to linear", inFilter );
            return qt3ds::render::NVRenderState::StencilTest;
        }
    }

    static inline qt3ds::render::NVRenderImageAccessType::Enum
    ConvertToImageAccessType(const char8_t *inAccess)
    {
        if (AreEqual(inAccess, "read"))
            return qt3ds::render::NVRenderImageAccessType::Read;
        if (AreEqual(inAccess, "write"))
            return qt3ds::render::NVRenderImageAccessType::Write;
        if (AreEqual(inAccess, "readwrite"))
            return qt3ds::render::NVRenderImageAccessType::ReadWrite;
        else
            QT3DS_ASSERT(false);

        return qt3ds::render::NVRenderImageAccessType::ReadWrite;
    }

    static inline size_t GetTypeSize(const char8_t *inType)
    {
        if (AreEqual(inType, "uint"))
            return sizeof(QT3DSU32);
        else if (AreEqual(inType, "int"))
            return sizeof(QT3DSI32);
        else if (AreEqual(inType, "uvec4"))
            return sizeof(QT3DSU32) * 4;
        else
            QT3DS_ASSERT(false);

        return 1;
    }

    inline qt3ds::render::NVRenderBufferBindValues::Enum
    ConvertFormatToBufferBindFlags(const char8_t *inFormat)
    {
        if (AreEqual(inFormat, "storage"))
            return qt3ds::render::NVRenderBufferBindValues::Storage;
        else if (AreEqual(inFormat, "indirect"))
            return qt3ds::render::NVRenderBufferBindValues::Draw_Indirect;
        else
            QT3DS_ASSERT(false);

        return qt3ds::render::NVRenderBufferBindValues::Unknown;
    }

    static inline void AppendShaderUniform(const char8_t *type, const char8_t *name,
                                           eastl::string &shaderPrefix)
    {
        shaderPrefix.append("uniform ");
        shaderPrefix.append(type);
        shaderPrefix.append(" ");
        shaderPrefix.append(name);
        shaderPrefix.append(";\n");
    }
    static inline void AppendShaderCode(
            const char8_t *inCode, Qt3DSDMStr &ioStr,
            eastl::basic_string<qt3ds::foundation::TWCharEASTLConverter::TCharType>
            &inConvertBuffer)
    {
        qt3ds::foundation::ConvertUTF(inCode, 0, inConvertBuffer);
        ioStr.append(inConvertBuffer);
    }
    void HideEffectProperty(Qt3DSDMInstanceHandle inInstance, const char8_t *inParamName)
    {
        Qt3DSDMMetaDataPropertyHandle theProp =
                GetSpecificMetaDataProperty(inInstance, Intern(inParamName));
        if (theProp.Valid()) {
            SMetaDataPropertyInfo theInfo = GetMetaDataPropertyInfo(theProp);
            SetMetaDataPropertyInfo(theProp, theInfo.m_Name, theInfo.m_FormalName,
                                    theInfo.m_Description, theInfo.m_Usage, theInfo.m_CompleteType,
                                    theInfo.m_DefaultValue, theInfo.m_MetaDataData,
                                    theInfo.m_GroupName, true, theInfo.m_Animatable,
                                    theInfo.m_Controllable);
        }
    }

    static inline void GetShaderName(const TCharStr &inObjectName,
                                     const char8_t *inShaderSpecificName,
                                     eastl::string &outShaderName)
    {
        outShaderName.clear();
        qt3ds::foundation::ConvertUTF(inObjectName.c_str(), 0, outShaderName);
        outShaderName.append(" - ");
        outShaderName.append(inShaderSpecificName);
    }

    void LoadDynamicObjectProperties(IDOMReader &inStream, SMetaDataDynamicObjectImpl &ioObject,
                                     Qt3DSDMInstanceHandle inInstance, const TCharStr &inObjectName,
                                     std::vector<SMetaDataLoadWarning> &outWarnings,
                                     eastl::string &shaderPrefix)
    {
        eastl::string vertexUniforms;
        eastl::string fragmentUniforms;
        vertexUniforms += "#ifdef VERTEX_SHADER\n";
        fragmentUniforms += "#ifdef FRAGMENT_SHADER\n";
        using namespace qt3ds::render::dynamic;
        ioObject.m_Properties.clear();
        ioObject.ClearEnumValueNames();
        IDOMReader::Scope __readerScope(inStream);
        if (inStream.MoveToFirstChild("MetaData")) {
            {
                IDOMReader::Scope __readerScope(inStream);
                LoadInstance(inStream, inInstance, inObjectName, outWarnings);
            }

            vector<Qt3DSDMMetaDataPropertyHandle> theProperties;
            GetSpecificMetaDataProperties(inInstance, theProperties);
            size_t propIdx = 0, propEnd = theProperties.size();
            for (bool success = inStream.MoveToFirstChild("Property"); success && propIdx < propEnd;
                 success = inStream.MoveToNextSibling("Property"), ++propIdx) {
                ioObject.m_Properties.push_back();
                SPropertyDefinition &theNewDefinition = ioObject.m_Properties.back();
                SMetaDataPropertyInfo theInfo(GetMetaDataPropertyInfo(theProperties[propIdx]));
                theNewDefinition.m_Name =
                        m_StringTable.GetRenderStringTable().RegisterStr(theInfo.m_Name.c_str());
                const char8_t *xmlName;
                inStream.Att("name", xmlName);
                const char8_t *stage;
                inStream.Att("stage", stage);
                eastl::string uniforms;
                if (AreEqual(xmlName, theNewDefinition.m_Name.c_str())) {
                    switch (theInfo.GetDataType()) {
                    case DataModelDataType::Bool:
                        theNewDefinition.m_DataType =
                                qt3ds::render::NVRenderShaderDataTypes::QT3DSRenderBool;
                        AppendShaderUniform("bool", theNewDefinition.m_Name.c_str(), uniforms);
                        break;
                    case DataModelDataType::Long:
                        theNewDefinition.m_DataType
                                = qt3ds::render::NVRenderShaderDataTypes::QT3DSI32;
                        AppendShaderUniform("int", theNewDefinition.m_Name.c_str(), uniforms);
                        break;
                    case DataModelDataType::Float2:
                        theNewDefinition.m_DataType
                                = qt3ds::render::NVRenderShaderDataTypes::QT3DSVec2;
                        AppendShaderUniform("vec2", theNewDefinition.m_Name.c_str(), uniforms);
                        break;
                    case DataModelDataType::Float3:
                        theNewDefinition.m_DataType
                                = qt3ds::render::NVRenderShaderDataTypes::QT3DSVec3;
                        AppendShaderUniform("vec3", theNewDefinition.m_Name.c_str(), uniforms);
                        break;
                    case DataModelDataType::Float4:
                        theNewDefinition.m_DataType
                                = qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4;
                        AppendShaderUniform("vec4", theNewDefinition.m_Name.c_str(), uniforms);
                        break;
                    case DataModelDataType::String:
                        if (theInfo.m_CompleteType == CompleteMetaDataType::Texture) {
                            theNewDefinition.m_DataType =
                                    qt3ds::render::NVRenderShaderDataTypes::NVRenderTexture2DPtr;
                            const char8_t *filter = "linear", *minFilter = "linear",
                                    *clamp = "clamp", *usage = "", *path = "";
                            if (inStream.Att("filter", filter))
                                theNewDefinition.m_MagFilterOp = ConvertFilterToMagOp(filter);
                            if (inStream.Att("minfilter", minFilter))
                                theNewDefinition.m_MinFilterOp = ConvertFilterToMinOp(minFilter);
                            if (inStream.Att("clamp", clamp))
                                theNewDefinition.m_CoordOp = ConvertTextureCoordOp(clamp);
                            if (inStream.Att("usage", usage))
                                theNewDefinition.m_TexUsageType = ConvertTextureType(usage);
                            if (inStream.Att("default", path)) {
                                TDataStrPtr theDataStr(
                                            theInfo.m_DefaultValue.getData<TDataStrPtr>());
                                theNewDefinition.m_ImagePath =
                                        m_StringTable.GetRenderStringTable().RegisterStr(
                                            theDataStr->GetData());
                            }

                            // Output macro so we can change the set of variables used for this
                            // independent of the
                            // meta data system.
                            uniforms.append("SNAPPER_SAMPLER2D(");
                            uniforms.append(theNewDefinition.m_Name.c_str());
                            uniforms.append(", ");
                            uniforms.append(theNewDefinition.m_Name.c_str());
                            uniforms.append(", ");
                            uniforms.append(filter);
                            uniforms.append(", ");
                            uniforms.append(clamp);
                            uniforms.append(", ");
                            uniforms.append("false )\n");
                        } else if (theInfo.m_CompleteType == CompleteMetaDataType::StringList) {
                            theNewDefinition.m_DataType =
                                    qt3ds::render::NVRenderShaderDataTypes::QT3DSI32;
                            const TMetaDataStringList &theList =
                                    qt3dsdm::get<TMetaDataStringList>(theInfo.m_MetaDataData);
                            ioObject.m_EnumValueNames.push_back(
                                        new eastl::vector<qt3ds::foundation::CRegisteredString>());
                            eastl::vector<qt3ds::foundation::CRegisteredString> &theBack =
                                    *ioObject.m_EnumValueNames.back();
                            for (QT3DSU32 idx = 0, end = (QT3DSU32)theList.size(); idx < end; ++idx)
                                theBack.push_back(m_StringTable.GetRenderStringTable().RegisterStr(
                                                      theList[idx].c_str()));
                            theNewDefinition.m_EnumValueNames = VecToCRef(theBack);
                            theNewDefinition.m_IsEnumProperty = true;
                            AppendShaderUniform("int", theNewDefinition.m_Name.c_str(),
                                                uniforms);
                        } else if (theInfo.m_CompleteType == CompleteMetaDataType::Image2D) {
                            theNewDefinition.m_DataType =
                                    qt3ds::render::NVRenderShaderDataTypes::NVRenderImage2DPtr;
                            const char8_t *format = "", *binding = "", *access = "readonly";
                            uniforms.append("layout(");
                            inStream.Att("format", format);
                            uniforms.append(format);
                            if (inStream.Att("binding", binding)) {
                                uniforms.append(", binding = ");
                                uniforms.append(binding);
                            }
                            uniforms.append(") ");

                            // if we have format layout we cannot set an additional access qualifier
                            if (inStream.Att("access", access) && !AreEqual(format, ""))
                                uniforms.append(access);

                            uniforms.append(" uniform image2D ");
                            uniforms.append(theNewDefinition.m_Name.c_str());
                            uniforms.append(";\n");
                        } else if (theInfo.m_CompleteType == CompleteMetaDataType::Buffer) {
                            theNewDefinition.m_DataType =
                                    qt3ds::render::NVRenderShaderDataTypes::NVRenderDataBufferPtr;
                            const char8_t *align = "std140", *usage = "storage", *binding = "",
                                    *format = "float";
                            uniforms.append("layout(");

                            inStream.Att("format", format);
                            inStream.Att("usage", usage);
                            if (AreEqual(usage, "storage")) {
                                inStream.Att("align", align);
                                uniforms.append(align);

                                if (inStream.Att("binding", binding)) {
                                    uniforms.append(", binding = ");
                                    uniforms.append(binding);
                                }

                                uniforms.append(") ");

                                uniforms.append("buffer ");
                                uniforms.append(theNewDefinition.m_Name.c_str());
                                uniforms.append("\n{ \n");
                                uniforms.append(format);
                                uniforms.append(" ");
                                uniforms.append(theNewDefinition.m_Name.c_str());
                                uniforms.append("_data[]; \n};\n");
                            } else {
                                // currently we only handle storage counters
                                QT3DS_ASSERT(false);
                            }
                        }
                        break;
                    default:
                        QT3DS_ASSERT(false);
                        // Fallthrough intentional
                    case DataModelDataType::Float:
                        theNewDefinition.m_DataType = qt3ds::render::NVRenderShaderDataTypes::QT3DSF32;
                        AppendShaderUniform("float", theNewDefinition.m_Name.c_str(), uniforms);
                        break;
                    }
                } else {
                    // It is conceivable that we not every property tag gets parsed into a property.
                    QT3DS_ASSERT(false);
                    // So we will bump to the next item and try again.
                    --propIdx;
                }

                if (AreEqual(stage, "vertex")) {
                    vertexUniforms += uniforms;
                } else if (AreEqual(stage, "fragment")) {
                    fragmentUniforms += uniforms;
                } else {
                    vertexUniforms += uniforms;
                    fragmentUniforms += uniforms;
                }
            }
        }
        vertexUniforms += "#endif\n";
        fragmentUniforms += "#endif\n";
        shaderPrefix.append(vertexUniforms);
        shaderPrefix.append(fragmentUniforms);
    }

    void LoadDynamicObjectShaders(IDOMReader &inStream, SMetaDataDynamicObjectImpl &ioObject,
                                  eastl::string &shaderPrefix, const TCharStr &inObjectName)
    {
        eastl::string theShaderNameStr;
        eastl::string theShaderTypeStr;
        eastl::string theShaderVersionStr;
        {
            IDOMReader::Scope __readerScope(inStream);
            eastl::basic_string<qt3ds::foundation::TWCharEASTLConverter::TCharType> theConvertBuffer;
            if (inStream.MoveToFirstChild("Shaders")) {
                const char8_t *globalShared = "";
                const char8_t *globalVertexShared = "";
                const char8_t *globalFragmentShared = "";
                inStream.ChildValue("Shared", globalShared);
                inStream.ChildValue("VertexShaderShared", globalVertexShared);
                inStream.ChildValue("FragmentShaderShared", globalFragmentShared);

                inStream.Att("type", theShaderTypeStr); // can be empty
                inStream.Att("version", theShaderVersionStr); // can be empty

                for (bool success = inStream.MoveToFirstChild(); success;
                     success = inStream.MoveToNextSibling()) {
                    IDOMReader::Scope __shaderScope(inStream);
                    const char8_t *elemName = inStream.GetNarrowElementName();
                    // If this is neither a compute shader nor a normal shader, go on and ignore
                    // element.
                    if (!(AreEqual(elemName, "Shader") || AreEqual(elemName, "ComputeShader")))
                        continue;

                    ioObject.m_Shaders.push_back();
                    SMetaDataShader &theShader = ioObject.m_Shaders.back();

                    qt3ds::foundation::ConvertUTF(theShaderTypeStr.c_str(), theShaderTypeStr.size(),
                                                  theShader.m_Type);
                    qt3ds::foundation::ConvertUTF(theShaderVersionStr.c_str(),
                                                  theShaderVersionStr.size(), theShader.m_Version);
                    const char8_t *theName = "";
                    char8_t theITOABuffer[64] = { 0 };
                    if (!inStream.Att("name", theName)) {
                        sprintf(theITOABuffer, "%d", (int)ioObject.m_Shaders.size() - 1);
                        theName = theITOABuffer;
                    }
                    GetShaderName(inObjectName, theName, theShaderNameStr);
                    qt3ds::foundation::ConvertUTF(theShaderNameStr.c_str(), theShaderNameStr.size(),
                                                  theShader.m_Name);

                    if (AreEqual(elemName, "Shader")) {
                        const char8_t *shaderShared = "";
                        inStream.ChildValue("Shared", shaderShared);
                        const char8_t *vertexCode = "";
                        inStream.ChildValue("VertexShader", vertexCode);
                        if (IsTrivial(vertexCode))
                            vertexCode = "void vert(){}";
                        const char8_t *fragmentCode = "void frag(){}";
                        inStream.ChildValue("FragmentShader", fragmentCode);
                        if (IsTrivial(fragmentCode))
                            fragmentCode = "void frag(){}";
                        const char8_t *geomCode = "";
                        inStream.ChildValue("GeometryShader", geomCode);

                        AppendShaderCode(shaderPrefix.c_str(), theShader.m_Code, theConvertBuffer);
                        AppendShaderCode(globalShared, theShader.m_Code, theConvertBuffer);
                        AppendShaderCode(shaderShared, theShader.m_Code, theConvertBuffer);
                        AppendShaderCode("\n#ifdef VERTEX_SHADER\n", theShader.m_Code,
                                         theConvertBuffer);
                        AppendShaderCode(globalVertexShared, theShader.m_Code, theConvertBuffer);
                        AppendShaderCode(vertexCode, theShader.m_Code, theConvertBuffer);
                        AppendShaderCode("\n#endif\n", theShader.m_Code, theConvertBuffer);
                        if (!IsTrivial(geomCode)) {
                            AppendShaderCode("\n#ifdef USER_GEOMETRY_SHADER\n", theShader.m_Code,
                                             theConvertBuffer);
                            AppendShaderCode(geomCode, theShader.m_Code, theConvertBuffer);
                            AppendShaderCode("\n#endif\n", theShader.m_Code, theConvertBuffer);
                            theShader.m_HasGeomShader = true;
                        }
                        AppendShaderCode("\n#ifdef FRAGMENT_SHADER\n", theShader.m_Code,
                                         theConvertBuffer);
                        AppendShaderCode(globalFragmentShared, theShader.m_Code, theConvertBuffer);
                        AppendShaderCode(fragmentCode, theShader.m_Code, theConvertBuffer);
                        AppendShaderCode("\n#endif\n", theShader.m_Code, theConvertBuffer);
                    } else if (AreEqual(elemName, "ComputeShader")) {
                        const char8_t *shaderCode = "";
                        inStream.Value(shaderCode);
                        theShader.m_IsComputeShader = true;
                        AppendShaderCode(shaderCode, theShader.m_Code, theConvertBuffer);
                    }
                }
            }
        }
    }

    static size_t Align(size_t inValue)
    {
        if (inValue % 4)
            return inValue + (4 - (inValue % 4));
        return inValue;
    }

    static qt3ds::render::dynamic::SDepthStencilFlags ParseDepthStencilFlags(const char8_t *inFlags)
    {
        eastl::string parseStr(inFlags);
        eastl::string tokenStr;
        qt3ds::render::dynamic::SDepthStencilFlags retval;
        for (uint32_t pos = parseStr.find('|'); parseStr.empty() == false;
             pos = parseStr.find('|')) {
            if (pos != eastl::string::npos) {
                tokenStr = parseStr.substr(pos);
                parseStr.erase(parseStr.begin(), parseStr.begin() + pos + 1);
            } else {
                tokenStr = parseStr;
                parseStr.clear();
            }
            if (AreEqual(tokenStr.c_str(), "clear-stencil"))
                retval |= qt3ds::render::dynamic::DepthStencilFlagValues::ClearStencil;
            if (AreEqual(tokenStr.c_str(), "clear-depth"))
                retval |= qt3ds::render::dynamic::DepthStencilFlagValues::ClearDepth;
        }
        return retval;
    }

    static qt3ds::render::NVRenderBoolOp::Enum ParseBoolOp(const char8_t *inOp)
    {
        if (AreEqual(inOp, "never"))
            return qt3ds::render::NVRenderBoolOp::Never;
        if (AreEqual(inOp, "less"))
            return qt3ds::render::NVRenderBoolOp::Less;
        if (AreEqual(inOp, "less-than-or-equal"))
            return qt3ds::render::NVRenderBoolOp::LessThanOrEqual;
        if (AreEqual(inOp, "equal"))
            return qt3ds::render::NVRenderBoolOp::Equal;
        if (AreEqual(inOp, "not-equal"))
            return qt3ds::render::NVRenderBoolOp::NotEqual;
        if (AreEqual(inOp, "greater"))
            return qt3ds::render::NVRenderBoolOp::Greater;
        if (AreEqual(inOp, "greater-than-or-equal"))
            return qt3ds::render::NVRenderBoolOp::GreaterThanOrEqual;
        if (AreEqual(inOp, "always"))
            return qt3ds::render::NVRenderBoolOp::AlwaysTrue;

        QT3DS_ASSERT(false);
        return qt3ds::render::NVRenderBoolOp::Unknown;
    }

    static qt3ds::render::NVRenderStencilOp::Enum ParseStencilOp(const char8_t *inOp)
    {

        if (AreEqual(inOp, "keep"))
            return qt3ds::render::NVRenderStencilOp::Keep;
        if (AreEqual(inOp, "zero"))
            return qt3ds::render::NVRenderStencilOp::Zero;
        if (AreEqual(inOp, "replace"))
            return qt3ds::render::NVRenderStencilOp::Replace;
        if (AreEqual(inOp, "increment"))
            return qt3ds::render::NVRenderStencilOp::Increment;
        if (AreEqual(inOp, "increment-wrap"))
            return qt3ds::render::NVRenderStencilOp::IncrementWrap;
        if (AreEqual(inOp, "decrement"))
            return qt3ds::render::NVRenderStencilOp::Decrement;
        if (AreEqual(inOp, "decrement-wrap"))
            return qt3ds::render::NVRenderStencilOp::DecrementWrap;
        if (AreEqual(inOp, "invert"))
            return qt3ds::render::NVRenderStencilOp::Invert;

        QT3DS_ASSERT(false);
        return qt3ds::render::NVRenderStencilOp::Unknown;
    }

    // Reloads an effect if one is already loaded so we can replace the existing effect definition
    void LoadEffectXML(IDOMReader &inStream, Qt3DSDMInstanceHandle inInstance,
                       const TCharStr &inObjectName,
                       std::vector<SMetaDataLoadWarning> &outWarnings,
                       const TCharStr &inSourcePath) override
    {
        using namespace qt3ds::render::dynamic;
        std::pair<TEffectMap::iterator, bool> theInserter =
                m_EffectMap.insert(std::make_pair(Intern(inObjectName), SMetaDataEffectImpl()));
        /*if ( inStream.MoveToFirstChild( "Effect" ) == false )
        {
                outWarnings.push_back( SMetaDataLoadWarning( MetaDataLoadWarningType::Unknown,
        MetaDataLoadWarningMessage::GeneralError, L"File doesn't appear to be an effect xml file,
        missing top level Effect tag" ) );
                return;
        }*/
        eastl::string shaderPrefix = "#include \"effect.glsllib\"\n";

        SMetaDataEffectImpl &theEffect = theInserter.first->second;
        m_ObjectName = inObjectName;
        theEffect.m_Name = inObjectName;
        theEffect.m_SourcePath = inSourcePath;
        theEffect.ClearEffectCommands();
        LoadDynamicObjectProperties(inStream, theEffect, inInstance, inObjectName, outWarnings,
                                    shaderPrefix);
        theEffect.m_Shaders.clear();
        LoadDynamicObjectShaders(inStream, theEffect, shaderPrefix, inObjectName);
        eastl::string theShaderNameStr;
        {
            IDOMReader::Scope __readerScope(inStream);
            if (inStream.MoveToFirstChild("Passes")) {
                for (bool success = inStream.MoveToFirstChild(); success;
                     success = inStream.MoveToNextSibling()) {
                    IDOMReader::Scope __passScope(inStream);
                    if (AreEqual("Pass", inStream.GetNarrowElementName())) {
                        bool drawIndirect = false;
                        const char8_t *shader = "", *input = "[source]", *output = "[dest]",
                                *outputFormat = "rgba";
                        inStream.Att("shader", shader);
                        inStream.Att("input", input);
                        inStream.Att("output", output);
                        // this is only for the final output of the effect
                        inStream.Att("format", outputFormat);
                        qt3ds::render::NVRenderTextureFormats::Enum theOutputFormat =
                                ConvertTypeAndFormatToTextureFormat("ubyte", outputFormat);
                        if (AreEqual(output, "[dest]") || IsTrivial(output))
                            theEffect.m_EffectCommands.push_back(new SBindTarget(theOutputFormat));
                        else
                            theEffect.m_EffectCommands.push_back(
                                        new SBindBuffer(
                                            m_StringTable.GetRenderStringTable().RegisterStr(output),
                                            false));
                        GetShaderName(inObjectName, shader, theShaderNameStr);
                        theEffect.m_EffectCommands.push_back(
                                    new SBindShader(m_StringTable.GetRenderStringTable().RegisterStr(
                                                        theShaderNameStr.c_str())));
                        theEffect.m_EffectCommands.push_back(new SApplyInstanceValue());
                        if (AreEqual(input, "[source]") || IsTrivial(input))
                            theEffect.m_EffectCommands.push_back(
                                        new SApplyBufferValue(
                                            m_StringTable.GetRenderStringTable().RegisterStr(""),
                                            m_StringTable.GetRenderStringTable().RegisterStr("")));
                        else
                            theEffect.m_EffectCommands.push_back(
                                        new SApplyBufferValue(
                                            m_StringTable.GetRenderStringTable().RegisterStr(input),
                                            m_StringTable.GetRenderStringTable().RegisterStr("")));
                        for (bool bufParam = inStream.MoveToFirstChild(); bufParam;
                             bufParam = inStream.MoveToNextSibling()) {
                            if (AreEqual("BufferInput", inStream.GetNarrowElementName())) {
                                const char8_t *param = "";
                                const char8_t *value = "";
                                inStream.Att("param", param);
                                inStream.Att("value", value);
                                if (AreEqual("[source]", value))
                                    value = "";
                                theEffect.m_EffectCommands.push_back(
                                            new SApplyBufferValue(
                                                m_StringTable.GetRenderStringTable().RegisterStr(
                                                    value),
                                                m_StringTable.GetRenderStringTable().RegisterStr(
                                                    param)));
                                HideEffectProperty(inInstance, param);
                            } else if (AreEqual("DepthInput", inStream.GetNarrowElementName())) {
                                const char8_t *param = "";
                                inStream.Att("param", param);
                                theEffect.m_EffectCommands.push_back(
                                            new SApplyDepthValue(
                                                m_StringTable.GetRenderStringTable().RegisterStr(
                                                    param)));
                                HideEffectProperty(inInstance, param);
                            } else if (AreEqual("ImageInput", inStream.GetNarrowElementName())) {
                                bool useAsTexture = false;
                                bool needSync = false;
                                const char8_t *param = "";
                                const char8_t *value = "";
                                const char8_t *usage = "";
                                const char8_t *sync = "";
                                inStream.Att("param", param);
                                inStream.Att("value", value);
                                if (AreEqual("[source]", value))
                                    value = "";
                                inStream.Att("usage", usage);
                                if (AreEqual("texture", usage))
                                    useAsTexture = true;
                                inStream.Att("sync", sync);
                                if (AreEqual("true", sync))
                                    needSync = true;

                                theEffect.m_EffectCommands.push_back(
                                            new SApplyImageValue(
                                                m_StringTable.GetRenderStringTable().RegisterStr(
                                                    value),
                                                m_StringTable.GetRenderStringTable().RegisterStr(
                                                    param), useAsTexture, needSync));
                                HideEffectProperty(inInstance, param);
                            } else if (AreEqual("DataBufferInput",
                                                inStream.GetNarrowElementName())) {
                                const char8_t *param = "";
                                const char8_t *usage = "";
                                inStream.Att("param", param);
                                inStream.Att("usage", usage);
                                qt3ds::render::NVRenderBufferBindValues::Enum bufType =
                                        ConvertFormatToBufferBindFlags(usage);

                                // check if we using an indirect buffer for drawing
                                if (bufType == qt3ds::render::NVRenderBufferBindValues::Draw_Indirect)
                                    drawIndirect = true;

                                theEffect.m_EffectCommands.push_back(
                                            new SApplyDataBufferValue(
                                                m_StringTable.GetRenderStringTable().RegisterStr(
                                                    param), bufType));
                                HideEffectProperty(inInstance, param);
                            } else if (AreEqual("SetParam", inStream.GetNarrowElementName())) {
                                const char8_t *name = "";
                                inStream.Att("name", name);
                                const char8_t *value = "";
                                inStream.Att("value", value);
                                // find the param and the type.
                                qt3ds::foundation::CRegisteredString propName =
                                        m_StringTable.GetRenderStringTable().RegisterStr(name);
                                qt3ds::render::dynamic::SPropertyDefinition *theDefinition = NULL;
                                for (uint32_t propIdx = 0, propEnd = theEffect.m_Properties.size();
                                     propIdx < propEnd && theDefinition == NULL; ++propIdx) {
                                    if (theEffect.m_Properties[propIdx].m_Name == propName)
                                        theDefinition = &theEffect.m_Properties[propIdx];
                                }
                                if (theDefinition != NULL) {
                                    // Hack it for now because the shader datatypes don't have a
                                    // built in sizeof operator.
                                    QT3DSU32 valueSize = 4;
                                    qt3ds::render::NVRenderShaderDataTypes::Enum theDataType =
                                            theDefinition->m_DataType;
                                    size_t allocSize = sizeof(SApplyValue) + valueSize;
                                    QT3DSU8 *theCommandData = (QT3DSU8 *)malloc(allocSize);
                                    QT3DSU8 *theValueData = theCommandData + sizeof(SApplyValue);
                                    new (theCommandData) SApplyValue(propName, theDataType);
                                    SApplyValue *theCommand =
                                            reinterpret_cast<SApplyValue *>(theCommandData);
                                    switch (theDataType) {
                                    case qt3ds::render::NVRenderShaderDataTypes::QT3DSRenderBool: {
                                        bool &target = *reinterpret_cast<bool *>(theValueData);
                                        qt3ds::foundation::StringConversion<bool>().StrTo(value,
                                                                                          target);
                                    } break;
                                    case qt3ds::render::NVRenderShaderDataTypes::QT3DSI32: {
                                        QT3DSI32 &target = *reinterpret_cast<QT3DSI32 *>(
                                                    theValueData);
                                        qt3ds::foundation::StringConversion<QT3DSI32>().StrTo(
                                                    value, target);
                                    } break;
                                    default:
                                        QT3DS_ASSERT(false);
                                        // Fallthrough intentional
                                    case qt3ds::render::NVRenderShaderDataTypes::QT3DSF32: {
                                        QT3DSF32 &target = *reinterpret_cast<QT3DSF32 *>(
                                                    theValueData);
                                        qt3ds::foundation::StringConversion<QT3DSF32>().StrTo(
                                                    value, target);
                                    } break;
                                    }
                                    theCommand->m_Value = NVDataRef<QT3DSU8>(theValueData,
                                                                             valueSize);
                                    theEffect.m_EffectCommands.push_back(theCommand);
                                }
                            } else if (AreEqual("Blending", inStream.GetNarrowElementName())) {
                                const char8_t *theSrcBlendFuncStr = "", *theDestBlendFuncStr = "";
                                inStream.Att("source", theSrcBlendFuncStr);
                                inStream.Att("dest", theDestBlendFuncStr);

                                qt3ds::render::NVRenderSrcBlendFunc::Enum theSrcBlendFunc =
                                        ConvertToSrcBlendFunc(theSrcBlendFuncStr);
                                qt3ds::render::NVRenderDstBlendFunc::Enum theDstBlendFuc =
                                        ConvertToDstBlendFunc(theDestBlendFuncStr);

                                // this will setup blending
                                theEffect.m_EffectCommands.push_back(
                                            new SApplyBlending(theSrcBlendFunc, theDstBlendFuc));
                            } else if (AreEqual("RenderState", inStream.GetNarrowElementName())) {
                                const char8_t *name = "";
                                inStream.Att("name", name);
                                const char8_t *value = "";
                                inStream.Att("value", value);
                                // find the param and the type.
                                bool theStateEnable = false;
                                qt3ds::render::NVRenderState::Enum theState
                                        = ConvertRenderState(name);
                                if (AreEqual("true", value))
                                    theStateEnable = true;

                                // this will setup blending
                                theEffect.m_EffectCommands.push_back(
                                            new SApplyRenderState(theState, theStateEnable));
                            } else if (AreEqual("DepthStencil", inStream.GetNarrowElementName())) {
                                const char8_t *bufferName = "";
                                inStream.Att("buffer", bufferName);
                                QT3DSU32 stencilvalue = 0;
                                inStream.Att("reference", stencilvalue);
                                const char8_t *flags = "";
                                inStream.Att("flags", flags);
                                QT3DSU32 mask = QT3DS_MAX_U32;
                                inStream.Att("mask", mask);
                                qt3ds::render::NVRenderBoolOp::Enum stencilFunction =
                                        qt3ds::render::NVRenderBoolOp::Equal;
                                qt3ds::render::NVRenderStencilOp::Enum stencilFailOperation =
                                        qt3ds::render::NVRenderStencilOp::Keep;
                                qt3ds::render::NVRenderStencilOp::Enum depthPass =
                                        qt3ds::render::NVRenderStencilOp::Keep;
                                qt3ds::render::NVRenderStencilOp::Enum depthFail =
                                        qt3ds::render::NVRenderStencilOp::Keep;
                                const char8_t *temp;
                                if (inStream.Att("stencil-fail", temp))
                                    stencilFailOperation = ParseStencilOp(temp);
                                if (inStream.Att("depth-pass", temp))
                                    depthPass = ParseStencilOp(temp);
                                if (inStream.Att("depth-fail", temp))
                                    depthFail = ParseStencilOp(temp);
                                if (inStream.Att("stencil-function", temp))
                                    stencilFunction = ParseBoolOp(temp);

                                qt3ds::render::dynamic::SDepthStencilFlags flagValues =
                                        ParseDepthStencilFlags(flags);
                                theEffect.m_EffectCommands.push_back(
                                            new SDepthStencil(
                                                m_StringTable.GetRenderStringTable().RegisterStr(
                                                    bufferName), flagValues, stencilFailOperation,
                                                depthPass, depthFail, stencilFunction, stencilvalue,
                                                mask));
                            } else {
                                QT3DS_ASSERT(false);
                            }
                        }
                        theEffect.m_EffectCommands.push_back(new SRender(drawIndirect));
                    } else if (AreEqual("Buffer", inStream.GetNarrowElementName())
                               || AreEqual("Image", inStream.GetNarrowElementName())) {
                        SAllocateBufferFlags theFlags;
                        const char8_t *theLifetimeStr = "", *theType = "", *theFormat = "",
                                *theFilter = "", *theWrap = "", *theName = "";
                        QT3DSF32 theSize = 1.0f;
                        bool isImage = AreEqual("Image", inStream.GetNarrowElementName());
                        inStream.Att("name", theName);
                        inStream.Att("lifetime", theLifetimeStr);
                        inStream.Att("type", theType);
                        inStream.Att("format", theFormat);
                        inStream.Att("filter", theFilter);
                        inStream.Att("wrap", theWrap);
                        inStream.Att("size", theSize);
                        // Clamp the size to valid amounts
                        if (theSize <= 0)
                            theSize = 1.0f;
                        if (theSize > 1.0f)
                            theSize = 1.0f;
                        if (AreEqual(theLifetimeStr, "scene"))
                            theFlags.SetSceneLifetime(true);
                        qt3ds::render::NVRenderTextureFormats::Enum theTexFormat =
                                ConvertTypeAndFormatToTextureFormat(theType, theFormat);
                        qt3ds::render::NVRenderTextureMagnifyingOp::Enum theMagOp =
                                ConvertFilterToMagOp(theFilter);
                        qt3ds::render::NVRenderTextureCoordOp::Enum theCoordOp =
                                ConvertTextureCoordOp(theWrap);
                        if (*theName == 0) {
                            QT3DS_ASSERT(false);
                            // inFoundation.error( QT3DS_INVALID_PARAMETER, "Buffer is missing its
                            // name" );
                        } else if (isImage) {
                            const char8_t *temp;
                            qt3ds::render::NVRenderImageAccessType::Enum theAccess =
                                    qt3ds::render::NVRenderImageAccessType::ReadWrite;
                            if (inStream.Att("access", temp))
                                theAccess = ConvertToImageAccessType(temp);

                            theEffect.m_EffectCommands.push_back(
                                        new SAllocateImage(
                                            m_StringTable.GetRenderStringTable().RegisterStr(
                                                theName), theTexFormat, theMagOp, theCoordOp,
                                            theSize, theFlags, theAccess));
                        } else {
                            theEffect.m_EffectCommands.push_back(
                                        new SAllocateBuffer(
                                            m_StringTable.GetRenderStringTable().RegisterStr(
                                                theName), theTexFormat, theMagOp, theCoordOp,
                                            theSize, theFlags));
                        }
                    } else if (AreEqual("DataBuffer", inStream.GetNarrowElementName())) {
                        SAllocateBufferFlags theFlags;
                        const char8_t *theLifetimeStr = "", *theType = "", *theWrapName = "",
                                *theWrapType = "", *theFormat = "", *theName = "";
                        QT3DSF32 theSize = 1.0f;
                        qt3ds::render::NVRenderBufferBindValues::Enum bufType,
                                wrapBufType = qt3ds::render::NVRenderBufferBindValues::Unknown;

                        inStream.Att("name", theName);
                        inStream.Att("lifetime", theLifetimeStr);
                        inStream.Att("type", theType);
                        inStream.Att("format", theFormat);
                        inStream.Att("size", theSize);
                        if (AreEqual(theLifetimeStr, "scene"))
                            theFlags.SetSceneLifetime(true);
                        bufType = ConvertFormatToBufferBindFlags(theType);

                        if (inStream.Att("wrapType", theWrapType)) {
                            wrapBufType = ConvertFormatToBufferBindFlags(theWrapType);
                            inStream.Att("wrapName", theWrapName);
                            if (*theWrapName == 0)
                                QT3DS_ASSERT(false);
                        }

                        theSize *= GetTypeSize(theFormat);

                        if (theSize <= 0)
                            theSize = 1.0f;

                        if (*theName == 0) {
                            QT3DS_ASSERT(false);
                        } else {
                            theEffect.m_EffectCommands.push_back(
                                        new SAllocateDataBuffer(
                                            m_StringTable.GetRenderStringTable().RegisterStr(
                                                theName), bufType,
                                            m_StringTable.GetRenderStringTable().RegisterStr(
                                                theWrapName), wrapBufType, theSize, theFlags));
                        }
                    } else {
                        QT3DS_ASSERT(false); // Unrecognized effect passes member.
                    }
                }

            } else {
                if (theEffect.m_Shaders.size()) {
                    // Create the minimal set of commands that we could run the first shader with.
                    theEffect.m_EffectCommands.push_back(new SBindTarget());
                    theEffect.m_EffectCommands.push_back(
                                new SBindShader(m_StringTable.GetRenderStringTable().RegisterStr(
                                                    theEffect.m_Shaders[0].m_Name.c_str())));
                    theEffect.m_EffectCommands.push_back(new SApplyInstanceValue());
                    theEffect.m_EffectCommands.push_back(new SRender(false));
                }
            }
        }
    }

    bool LoadEffectXMLFromSourcePath(const char *inSourcePath,
                                     Qt3DSDMInstanceHandle inInstance,
                                     const TCharStr &inObjectName,
                                     std::vector<SMetaDataLoadWarning> &outWarnings,
                                     qt3ds::foundation::IInStream &inStream) override
    {
        std::shared_ptr<IDOMFactory> theFactory(
                    IDOMFactory::CreateDOMFactory(m_DataCore->GetStringTablePtr()));
        qt3dsdm::SDOMElement *theElem = CDOMSerializer::Read(*theFactory, inStream, NULL);
        if (theElem != NULL) {
            std::shared_ptr<IDOMReader> theReader(
                        IDOMReader::CreateDOMReader(*theElem, m_DataCore->GetStringTablePtr(),
                                                    theFactory));
            LoadEffectXML(*theReader, inInstance, inObjectName, outWarnings,
                          TCharStr(Intern(inSourcePath)));
            return true;
        }
        return false;
    }

    Option<SMetaDataEffect> GetEffectBySourcePath(const char *inSourcePath) override
    {
        QDir sourcePath = QDir::cleanPath(QString(inSourcePath));
        for (TEffectMap::iterator iter = m_EffectMap.begin(), end = m_EffectMap.end(); iter != end;
             ++iter) {
            QDir effectPath = QDir::cleanPath(QString::fromWCharArray(
                                                  iter->second.m_SourcePath.wide_str()));
            if (effectPath == sourcePath)
                return iter->second.ToEffect();
        }
        return Empty();
    }

    void LoadMaterialInstance(const char *inShaderFile, Qt3DSDMInstanceHandle inInstance,
                              const TCharStr &inName,
                              std::vector<SMetaDataLoadWarning> &outWarnings,
                              qt3ds::foundation::IInStream &inStream) override
    {
        QString shaderFile(inShaderFile);
        if (shaderFile.endsWith(".material") || shaderFile.endsWith(".shader")) {
            LoadMaterialClassFromSourcePath(inShaderFile, inInstance, inName, outWarnings,
                                            inStream);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    bool IsMaterialClassRegistered(const char *inName) override
    {
        return m_CustomMaterials.find(Intern(inName)) != m_CustomMaterials.end();
    }

    void LoadMaterialClassXML(IDOMReader &inStream, Qt3DSDMInstanceHandle inInstance,
                              const TCharStr &inObjectName,
                              std::vector<SMetaDataLoadWarning> &outWarnings,
                              const TCharStr &inSourcePath) override
    {
        using namespace qt3ds::render::dynamic;

        // Remove the previous custom material entry so that the new shader code is used
        m_CustomMaterials.erase(Intern(inObjectName));

        std::pair<TCustomMaterialMap::iterator, bool> theInserter = m_CustomMaterials.insert(
                    std::make_pair(Intern(inObjectName), SMetaDataCustomMaterialImpl()));
        /*if ( inStream.MoveToFirstChild( "Effect" ) == false )
        {
                outWarnings.push_back( SMetaDataLoadWarning( MetaDataLoadWarningType::Unknown,
        MetaDataLoadWarningMessage::GeneralError, L"File doesn't appear to be an effect xml file,
        missing top level Effect tag" ) );
                return;
        }*/
        eastl::string shaderPrefix = "#include \"customMaterial.glsllib\"\n";

        SMetaDataCustomMaterialImpl &theMaterial = theInserter.first->second;
        m_ObjectName = inObjectName;
        theMaterial.m_Name = inObjectName;
        theMaterial.m_SourcePath = inSourcePath;
        theMaterial.m_HasTransparency = false;
        theMaterial.m_HasRefraction = false;
        theMaterial.m_AlwaysDirty = false;
        theMaterial.m_ShaderKey = 0;
        theMaterial.m_LayerCount = 0;
        inStream.Att("always-dirty", theMaterial.m_AlwaysDirty);
        LoadDynamicObjectProperties(inStream, theMaterial, inInstance, inObjectName, outWarnings,
                                    shaderPrefix);
        LoadDynamicObjectShaders(inStream, theMaterial, shaderPrefix, inObjectName);

        // currently single pass shader only
        if (theMaterial.m_Shaders.size()) {
            eastl::string theShaderNameStr;

            // in Passes we store additional render commands
            IDOMReader::Scope __readerScope(inStream);
            if (inStream.MoveToFirstChild("Passes")) {
                for (bool success = inStream.MoveToFirstChild(); success;
                     success = inStream.MoveToNextSibling()) {
                    IDOMReader::Scope __passScope(inStream);
                    if (AreEqual("Pass", inStream.GetNarrowElementName())) {
                        const char8_t *typeStr;
                        if (!inStream.UnregisteredAtt("type", typeStr))
                            typeStr = "render";
                        if (AreEqual(typeStr, "render")) {
                            const char8_t *shader = "", *input = "[source]", *output = "[dest]",
                                    *outputFormat = "rgba", *clear = "";
                            bool needsClear = false;
                            inStream.Att("shader", shader);
                            inStream.Att("input", input);
                            inStream.Att("output", output);
                            // for multi pass materials
                            inStream.Att("clear", clear);
                            if (AreEqual("true", clear))
                                needsClear = true;

                            GetShaderName(inObjectName, shader, theShaderNameStr);
                            // this is only for the final output of the effect
                            inStream.Att("format", outputFormat);
                            qt3ds::render::NVRenderTextureFormats::Enum theOutputFormat =
                                    ConvertTypeAndFormatToTextureFormat("ubyte", outputFormat);

                            if (AreEqual(output, "[dest]") || IsTrivial(output)) {
                                theMaterial.m_CustomerMaterialCommands.push_back(
                                            new SBindTarget(theOutputFormat));
                            } else {
                                theMaterial.m_CustomerMaterialCommands.push_back(
                                            new SBindBuffer(
                                                m_StringTable.GetRenderStringTable().RegisterStr(
                                                    output), needsClear));
                            }

                            // add shader to command stream
                            qt3ds::render::CRegisteredString theShaderName;
                            if (!IsTrivial(shader)) {
                                for (QT3DSU32 idx = 0, end = theMaterial.m_Shaders.size();
                                     idx < end && theShaderName.IsValid() == false; ++idx) {
                                    qt3ds::render::CRegisteredString thePossibleNameStr =
                                            m_StringTable.GetRenderStringTable().RegisterStr(
                                                theMaterial.m_Shaders[idx].m_Name.c_str());
                                    if (AreEqual(thePossibleNameStr.c_str(),
                                                 theShaderNameStr.c_str()))
                                        theShaderName = thePossibleNameStr;
                                }
                            }
                            if (theShaderName.IsValid() == false
                                    && theMaterial.m_Shaders.empty() == false)
                                theShaderName = m_StringTable.GetRenderStringTable().RegisterStr(
                                            theMaterial.m_Shaders[0].m_Name.c_str());
                            if (theShaderName.IsValid()) {
                                theMaterial.m_CustomerMaterialCommands.push_back(
                                            new SBindShader(theShaderName));
                                // this is a place holder for our input values to the shader
                                theMaterial.m_CustomerMaterialCommands.push_back(
                                            new SApplyInstanceValue());

                                for (bool bufParam = inStream.MoveToFirstChild(); bufParam;
                                     bufParam = inStream.MoveToNextSibling()) {
                                    if (AreEqual("BufferBlit", inStream.GetNarrowElementName())) {
                                        const char8_t *value = "";
                                        const char8_t *dest = "";
                                        const char8_t *source = "";
                                        inStream.Att("dest", dest);
                                        if (AreEqual("[dest]", dest))
                                            value = "";
                                        inStream.Att("source", source);
                                        if (AreEqual("[source]", source))
                                            value = "";

                                        theMaterial.m_CustomerMaterialCommands.push_back(
                                                    new SApplyBlitFramebuffer(
                                                        m_StringTable.GetRenderStringTable().RegisterStr(
                                                            source),
                                                        m_StringTable.GetRenderStringTable().RegisterStr(
                                                            dest)));

                                        // note need a better way to pass information from MDL to
                                        // our input
                                        // We use buffer blits to simulate glass refraction
                                        theMaterial.m_HasRefraction = true;
                                    } else if (AreEqual("BufferInput",
                                                        inStream.GetNarrowElementName())) {
                                        const char8_t *param = "";
                                        const char8_t *value = "";
                                        inStream.Att("param", param);
                                        inStream.Att("value", value);
                                        if (AreEqual("[source]", value))
                                            value = "";
                                        theMaterial.m_CustomerMaterialCommands.push_back(
                                                    new SApplyBufferValue(
                                                        m_StringTable.GetRenderStringTable().RegisterStr(
                                                            value),
                                                        m_StringTable.GetRenderStringTable().RegisterStr(
                                                            param)));
                                        HideEffectProperty(inInstance, param);
                                    } else if (AreEqual("Blending",
                                                        inStream.GetNarrowElementName())) {
                                        const char8_t *theSrcBlendFuncStr = "",
                                                *theDestBlendFuncStr = "";
                                        inStream.Att("source", theSrcBlendFuncStr);
                                        inStream.Att("dest", theDestBlendFuncStr);

                                        qt3ds::render::NVRenderSrcBlendFunc::Enum theSrcBlendFunc =
                                                ConvertToSrcBlendFunc(theSrcBlendFuncStr);
                                        qt3ds::render::NVRenderDstBlendFunc::Enum theDstBlendFuc =
                                                ConvertToDstBlendFunc(theDestBlendFuncStr);

                                        // this will setup blending
                                        theMaterial.m_CustomerMaterialCommands.push_back(
                                                    new SApplyBlending(theSrcBlendFunc,
                                                                       theDstBlendFuc));
                                        // if we have blending we have transparency
                                        theMaterial.m_HasTransparency = true;
                                    } else if (AreEqual("RenderState",
                                                        inStream.GetNarrowElementName())) {
                                        // UdoL Todo: add this one
                                    }
                                }
                            }
                            // add the render command as last thing in the pass, it is a render
                            // pass.
                            theMaterial.m_CustomerMaterialCommands.push_back(new SRender(false));
                        }
                    } else if (AreEqual("ShaderKey", inStream.GetNarrowElementName())) {
                        QT3DSU32 theValue = 0;
                        inStream.Att("value", theValue);
                        theMaterial.m_ShaderKey = theValue;
                    } else if (AreEqual("LayerKey", inStream.GetNarrowElementName())) {
                        QT3DSU32 theValue = 0;
                        inStream.Att("count", theValue);
                        theMaterial.m_LayerCount = theValue;
                    } else if (AreEqual("Buffer", inStream.GetNarrowElementName())) {
                        SAllocateBufferFlags theFlags;
                        const char8_t *theLifetimeStr = "", *theType = "", *theFormat = "",
                                *theFilter = "", *theWrap = "", *theName = "";
                        QT3DSF32 theSize = 1.0f;
                        inStream.Att("name", theName);
                        inStream.Att("lifetime", theLifetimeStr);
                        inStream.Att("type", theType);
                        inStream.Att("format", theFormat);
                        inStream.Att("filter", theFilter);
                        inStream.Att("wrap", theWrap);
                        inStream.Att("size", theSize);
                        // Clamp the size to valid amounts
                        if (theSize <= 0)
                            theSize = 1.0f;
                        if (theSize > 1.0f)
                            theSize = 1.0f;
                        if (AreEqual(theLifetimeStr, "scene"))
                            theFlags.SetSceneLifetime(true);
                        qt3ds::render::NVRenderTextureFormats::Enum theTexFormat =
                                ConvertTypeAndFormatToTextureFormat(theType, theFormat);
                        qt3ds::render::NVRenderTextureMagnifyingOp::Enum theMagOp =
                                ConvertFilterToMagOp(theFilter);
                        qt3ds::render::NVRenderTextureCoordOp::Enum theCoordOp =
                                ConvertTextureCoordOp(theWrap);
                        if (*theName == 0) {
                            QT3DS_ASSERT(false);
                            // inFoundation.error( QT3DS_INVALID_PARAMETER, "Buffer is missing its
                            // name" );
                        } else {
                            theMaterial.m_CustomerMaterialCommands.push_back(
                                        new SAllocateBuffer(
                                            m_StringTable.GetRenderStringTable().RegisterStr(
                                                theName), theTexFormat, theMagOp, theCoordOp,
                                            theSize, theFlags));
                        }
                    }
                }
            }

            if (theMaterial.m_CustomerMaterialCommands.size() == 0) {
                // add minimal set
                // add shader to command stream
                theMaterial.m_CustomerMaterialCommands.push_back(
                            new SBindShader(m_StringTable.GetRenderStringTable().RegisterStr(
                                                theMaterial.m_Shaders[0].m_Name.c_str())));
                // this is a place holder for our input values to the shader
                theMaterial.m_CustomerMaterialCommands.push_back(new SApplyInstanceValue());
                // add the render command as last thing
                theMaterial.m_CustomerMaterialCommands.push_back(new SRender(false));
            }
        }
    }

    bool LoadMaterialClassFromSourcePath(const char *inSourcePath,
                                         Qt3DSDMInstanceHandle inInstance,
                                         const TCharStr &inObjectName,
                                         std::vector<SMetaDataLoadWarning> &outWarnings,
                                         qt3ds::foundation::IInStream &inStream) override
    {
        std::shared_ptr<IDOMFactory> theFactory(
                    IDOMFactory::CreateDOMFactory(m_DataCore->GetStringTablePtr()));
        qt3dsdm::SDOMElement *theElem = CDOMSerializer::Read(*theFactory, inStream, NULL);
        if (theElem != NULL) {
            std::shared_ptr<IDOMReader> theReader(
                        IDOMReader::CreateDOMReader(*theElem, m_DataCore->GetStringTablePtr(),
                                                    theFactory));
            LoadMaterialClassXML(*theReader, inInstance, inObjectName, outWarnings,
                                 TCharStr(Intern(inSourcePath)));
            return true;
        }
        return false;
    }

    Option<SMetaDataCustomMaterial> GetMaterialBySourcePath(const char *inSourcePath) override
    {
        TCharStr theSourcePath(Intern(inSourcePath));
        for (TCustomMaterialMap::iterator iter = m_CustomMaterials.begin(),
             end = m_CustomMaterials.end();
             iter != end; ++iter) {
            if (iter->second.m_SourcePath == theSourcePath)
                return iter->second.ToMaterial();
        }
        return Empty();
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Undo/Redo
    void SetConsumer(TTransactionConsumerPtr inConsumer) override { m_Consumer = inConsumer; }

    ////////////////////////////////////////////////////////////////////////////////////
    // Signals
    virtual TSignalConnectionPtr
    ConnectInternalCategoryDestroyed(function<void(Qt3DSDMCategoryHandle)> inCallback)
    {
        return CONNECT(&SNewMetaDataImpl::internalCategoryDestroyed);
    }
    virtual TSignalConnectionPtr
    ConnectInternalPropertyDestroyed(function<void(Qt3DSDMMetaDataPropertyHandle)> inCallback)
    {
        return CONNECT(&SNewMetaDataImpl::internalMetaDataPropertyDestroyed);
    }
    virtual TSignalConnectionPtr
    ConnectInternalEventDestroyed(function<void(Qt3DSDMEventHandle)> inCallback)
    {
        return CONNECT(&SNewMetaDataImpl::internalEventDestroyed);
    }
    virtual TSignalConnectionPtr
    ConnectInternalHandlerDestroyed(function<void(Qt3DSDMHandlerHandle)> inCallback)
    {
        return CONNECT(&SNewMetaDataImpl::internalHandlerDestroyed);
    }
    virtual TSignalConnectionPtr
    ConnectInternalHandlerArgDestroyed(function<void(Qt3DSDMHandlerHandle, QT3DSU32)> inCallback)
    {
        return CONNECT(&SNewMetaDataImpl::internalHandlerArgDestroyed);
    }
};
}

std::shared_ptr<IMetaData> IMetaData::CreateNewMetaData(std::shared_ptr<IDataCore> inDataCore)
{
    return std::make_shared<SNewMetaDataImpl>(inDataCore);
}
#ifndef QT3DSDM_META_DATA_NO_SIGNALS
#include "Qt3DSDMMetaData.moc"
#endif
