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
#include "Qt3DSRenderPlugin.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/SerializationTypes.h"
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSSystem.h"
#include "foundation/FileTools.h"
#include "render/Qt3DSRenderContext.h"
#include "StringTools.h"
#include "Qt3DSRenderPluginPropertyValue.h"
#include "Qt3DSRenderInputStreamFactory.h"

#if defined(QT3DS_WINDOWS)
#include "windows/DynamicLibLoader.h"
#elif defined(QT3DS_ANDROID)
#include "android/DynamicLibLoader.h"
#elif defined(QT3DS_LINUX)
#include "linux/DynamicLibLoader.h"
#elif defined(QT3DS_APPLE)
#include "macos/DynamicLibLoader.h"
#elif defined(QT3DS_QNX)
#include "qnx/DynamicLibLoader.h"
#else
#error "Must define an operating system type (QT3DS_WINDOWS, QT3DS_ANDROID, QT3DS_LINUX, QT3DS_APPLE, QT3DS_QNX)"
#endif

using namespace qt3ds::render;

namespace {
// Legacy definitions...
// API version 1 definitions
typedef struct _RenderPluginSurfaceDescriptionV1
{
    long m_Width;
    long m_Height;
    enum QT3DSRenderPluginDepthTypes m_DepthBuffer;
    enum QT3DSRenderPluginTextureTypes m_ColorBuffer;
    TBool m_HasStencilBuffer;
} TRenderPluginSurfaceDescriptionV1;

typedef TNeedsRenderResult (*TNeedsRenderFunctionV1)(TRenderPluginClassPtr cls,
                                                     TRenderPluginInstancePtr instance,
                                                     TRenderPluginSurfaceDescriptionV1 surface,
                                                     TVec2 presScaleFactor);

typedef void (*TRenderFunctionV1)(TRenderPluginClassPtr cls, TRenderPluginInstancePtr instance,
                                  TRenderPluginSurfaceDescriptionV1 surface,
                                  TVec2 presScaleFactor,
                                  QT3DSRenderPluginColorClearState inClearColorBuffer);

// End API version 1 definitions

TRenderPluginSurfaceDescription ToCInterface(const SOffscreenRendererEnvironment &env)
{
    TRenderPluginSurfaceDescription retval;
    retval.m_Width = (long)env.m_Width;
    retval.m_Height = (long)env.m_Height;
    retval.m_ColorBuffer = static_cast<QT3DSRenderPluginTextureTypes>(env.m_Format);
    retval.m_DepthBuffer = static_cast<QT3DSRenderPluginDepthTypes>(env.m_Depth);
    retval.m_HasStencilBuffer = env.m_Stencil ? TTRUE : TFALSE;
    retval.m_MSAALevel = QT3DSRenderPluginMSAALevelNoMSAA;
    // note no supersampling AA support for plugins
    // we fall back to 4xMSAA
    switch (env.m_MSAAMode) {
    case AAModeValues::X2:
        retval.m_MSAALevel = QT3DSRenderPluginMSAALevelTwo;
        break;
    case AAModeValues::SSAA:
    case AAModeValues::X4:
        retval.m_MSAALevel = QT3DSRenderPluginMSAALevelFour;
        break;
    case AAModeValues::X8:
        retval.m_MSAALevel = QT3DSRenderPluginMSAALevelEight;
        break;
    default:
        QT3DS_ASSERT(false);
    // fallthrough intentional.
    case AAModeValues::NoAA:
        break;
    };
    return retval;
}

TRenderPluginSurfaceDescriptionV1 ToCInterfaceV1(const SOffscreenRendererEnvironment &env)
{
    TRenderPluginSurfaceDescriptionV1 retval;
    retval.m_Width = (long)env.m_Width;
    retval.m_Height = (long)env.m_Height;
    retval.m_ColorBuffer = static_cast<QT3DSRenderPluginTextureTypes>(env.m_Format);
    retval.m_DepthBuffer = static_cast<QT3DSRenderPluginDepthTypes>(env.m_Depth);
    retval.m_HasStencilBuffer = env.m_Stencil ? TTRUE : TFALSE;
    return retval;
}

TVec2 ToCInterface(const QT3DSVec2 &item)
{
    TVec2 retval = { item.x, item.y };
    return retval;
}

QT3DSRenderPluginColorClearState ToCInterface(SScene::RenderClearCommand inClearCommand)
{
    switch (inClearCommand) {
    case SScene::DoNotClear:
        return QT3DSRenderPluginColorClearStateDoNotClear;
    case SScene::AlwaysClear:
        return QT3DSRenderPluginColorClearStateAlwaysClear;
    default:
        QT3DS_ASSERT(false); // fallthrough intentional
    case SScene::ClearIsOptional:
        return QT3DSRenderPluginColorClearStateClearIsOptional;
    };
}

class SRenderPluginPropertyData
{
    SRenderPluginPropertyValue m_Value;
    bool m_Dirty;

public:
    SRenderPluginPropertyData()
        : m_Dirty(false)
    {
    }
    SRenderPluginPropertyData(const SRenderPluginPropertyData &other)
        : m_Value(other.m_Value)
        , m_Dirty(other.m_Dirty)
    {
    }
    SRenderPluginPropertyData &operator=(const SRenderPluginPropertyData &other)
    {
        m_Value = other.m_Value;
        m_Dirty = other.m_Dirty;
        return *this;
    }

    bool IsDirty() const
    {
        return m_Value.getType() != RenderPluginPropertyValueTypes::NoRenderPluginPropertyValue
            && m_Dirty;
    }
    void SetValue(const SRenderPluginPropertyValue &value)
    {
        m_Value = value;
        m_Dirty = true;
    }

    TRenderPluginPropertyUpdate ClearDirty(CRegisteredString inPropName)
    {
        m_Dirty = false;
        TRenderPluginPropertyUpdate retval;
        memset(&retval, 0, sizeof(TRenderPluginPropertyUpdate));
        retval.m_PropName = inPropName.c_str();
        switch (m_Value.getType()) {
        case RenderPluginPropertyValueTypes::Long: {
            retval.m_PropertyType = QT3DSRenderPluginPropertyTypeLong;
            long temp = (long)m_Value.getData<QT3DSI32>();
            retval.m_PropertyValue = *reinterpret_cast<void **>(&temp);
        } break;
        case RenderPluginPropertyValueTypes::Float: {
            retval.m_PropertyType = QT3DSRenderPluginPropertyTypeFloat;
            float temp = m_Value.getData<QT3DSF32>();
            retval.m_PropertyValue = *reinterpret_cast<void **>(&temp);
        } break;
        case RenderPluginPropertyValueTypes::Boolean: {
            retval.m_PropertyType = QT3DSRenderPluginPropertyTypeLong;
            long temp = m_Value.getData<bool>() ? TTRUE : TFALSE;
            retval.m_PropertyValue = *reinterpret_cast<void **>(&temp);
        } break;
        case RenderPluginPropertyValueTypes::String: {
            retval.m_PropertyType = QT3DSRenderPluginPropertyTypeCharPtr;
            const char *temp = m_Value.getData<CRegisteredString>().c_str();
            retval.m_PropertyValue = reinterpret_cast<void *>(const_cast<char *>(temp));
        } break;
        default:
            QT3DS_ASSERT(false);
        }
        return retval;
    }
};

typedef nvvector<SRenderPluginPropertyData> TPropertyValueList;

struct IInternalPluginClass : public IRenderPluginClass
{
    virtual void PushUpdates(TRenderPluginInstancePtr instance,
                             TPropertyValueList &propertyValues) = 0;
    virtual void Update(NVConstDataRef<SRenderPropertyValueUpdate> updateBuffer,
                        TPropertyValueList &propertyValues) = 0;
    virtual QT3DSI32 GetAPIVersion() = 0;
};

static NVRenderTextureFormats::Enum ToTextureFormat(QT3DSRenderPluginTextureTypes inTextureType)
{
    switch (inTextureType) {
    default:
    case QT3DSRenderPluginTextureTypeRGBA8:
        return NVRenderTextureFormats::RGBA8;
    case QT3DSRenderPluginTextureTypeRGB8:
        return NVRenderTextureFormats::RGB8;
    case QT3DSRenderPluginTextureTypeRGB565:
        return NVRenderTextureFormats::RGB565;
    case QT3DSRenderPluginTextureTypeRGBA5551:
        return NVRenderTextureFormats::RGBA5551;
    }
}

static OffscreenRendererDepthValues::Enum ToDepthValue(QT3DSRenderPluginDepthTypes inType)
{
    switch (inType) {
    default:
    case QT3DSRenderPluginDepthTypeDepth16:
        return OffscreenRendererDepthValues::Depth16;
    case QT3DSRenderPluginDepthTypeDepth24:
        return OffscreenRendererDepthValues::Depth24;
    case QT3DSRenderPluginDepthTypeDepth32:
        return OffscreenRendererDepthValues::Depth32;
    }
}

static AAModeValues::Enum ToAAMode(QT3DSRenderPluginMSAALevel inMode)
{
    switch (inMode) {
    case QT3DSRenderPluginMSAALevelTwo:
        return AAModeValues::X2;
    case QT3DSRenderPluginMSAALevelFour:
        return AAModeValues::X4;
    case QT3DSRenderPluginMSAALevelEight:
        return AAModeValues::X8;
    default:
        QT3DS_ASSERT(false); // fallthrough intentional
    case QT3DSRenderPluginMSAALevelNoMSAA:
        return AAModeValues::NoAA;
    }
}

struct InstanceImpl : public IRenderPluginInstance
{
    NVFoundationBase &m_Foundation;
    TRenderPluginInstancePtr m_Instance;
    TRenderPluginClass m_Class;
    NVScopedRefCounted<IInternalPluginClass> m_Owner;
    CRegisteredString m_RendererType;
    // Backing store of property values
    nvvector<SRenderPluginPropertyData> m_PropertyValues;
    bool m_Dirty;
    NVRenderContext *m_RenderContext;
    QT3DSI32 mRefCount;

    InstanceImpl(NVFoundationBase &fnd, TRenderPluginInstancePtr instance, TRenderPluginClass cls,
                 IInternalPluginClass &owner, IStringTable &strTable)
        : m_Foundation(fnd)
        , m_Instance(instance)
        , m_Class(cls)
        , m_Owner(owner)
        , m_RendererType(
              strTable.RegisterStr(IRenderPluginInstance::IRenderPluginOffscreenRendererType()))
        , m_PropertyValues(m_Foundation.getAllocator(), "InstanceImpl::m_PropertyValues")
        , m_Dirty(false)
        , m_RenderContext(NULL)
        , mRefCount(0)
    {
    }

    virtual ~InstanceImpl() { m_Class.ReleaseInstance(m_Class.m_Class, m_Instance); }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void addCallback(IOffscreenRendererCallback *cb) override
    {

    }
    void CreateScriptProxy(script_State *state) override
    {
        if (m_Class.CreateInstanceScriptProxy)
            m_Class.CreateInstanceScriptProxy(m_Class.m_Class, m_Instance, state);
    }

    // Arbitrary const char* returned to indicate the type of this renderer
    // Can be overloaded to form the basis of an RTTI type system.
    // Not currently used by the rendering system.
    CRegisteredString GetOffscreenRendererType() override { return m_RendererType; }

    SOffscreenRendererEnvironment GetDesiredEnvironment(QT3DSVec2 inPresentationScaleFactor) override
    {
        if (m_Class.QueryInstanceRenderSurface) {
            QT3DSRenderPluginMSAALevel theLevel = QT3DSRenderPluginMSAALevelNoMSAA;
            TRenderPluginSurfaceDescription desc = m_Class.QueryInstanceRenderSurface(
                m_Class.m_Class, m_Instance, ToCInterface(inPresentationScaleFactor));
            if (m_Owner->GetAPIVersion() > 1)
                theLevel = desc.m_MSAALevel;

            return SOffscreenRendererEnvironment(
                (QT3DSU32)desc.m_Width, (QT3DSU32)desc.m_Height, ToTextureFormat(desc.m_ColorBuffer),
                ToDepthValue(desc.m_DepthBuffer), desc.m_HasStencilBuffer ? true : false,
                ToAAMode(theLevel));
        } else {
            QT3DS_ASSERT(false);
        }
        return SOffscreenRendererEnvironment();
    }

    // Returns true of this object needs to be rendered, false if this object is not dirty
    SOffscreenRenderFlags NeedsRender(const SOffscreenRendererEnvironment &inEnvironment,
                                      QT3DSVec2 inPresentationScaleFactor,
                                      const SRenderInstanceId instanceId) override
    {
        if (m_Dirty) {
            m_Dirty = false;
            m_Owner->PushUpdates(m_Instance, m_PropertyValues);
        }
        if (m_Class.NeedsRenderFunction) {
            if (m_Owner->GetAPIVersion() > 1) {
                TNeedsRenderResult result = m_Class.NeedsRenderFunction(
                    m_Class.m_Class, m_Instance, ToCInterface(inEnvironment),
                    ToCInterface(inPresentationScaleFactor));
                return SOffscreenRenderFlags(result.HasTransparency ? true : false,
                                             result.HasChangedSinceLastFrame ? true : false);
            } else {
                TNeedsRenderFunctionV1 theV1Function =
                    reinterpret_cast<TNeedsRenderFunctionV1>(m_Class.NeedsRenderFunction);

                TNeedsRenderResult result =
                    theV1Function(m_Class.m_Class, m_Instance, ToCInterfaceV1(inEnvironment),
                                  ToCInterface(inPresentationScaleFactor));
                return SOffscreenRenderFlags(result.HasTransparency ? true : false,
                                             result.HasChangedSinceLastFrame ? true : false);
            }
        }
        return SOffscreenRenderFlags(true, true);
    }
    // Returns true if the rendered result image has transparency, or false
    // if it should be treated as a completely opaque image.
    // It is the IOffscreenRenderer's job to clear any buffers (color, depth, stencil) that it
    // needs to.  It should not assume that it's buffers are clear;
    // Sometimes we scale the width and height of the main presentation in order to fit a window.
    // If we do so, the scale factor tells the subpresentation renderer how much the system has
    // scaled.
    void Render(const SOffscreenRendererEnvironment &inEnvironment,
                NVRenderContext &inRenderContext, QT3DSVec2 inPresentationScaleFactor,
                SScene::RenderClearCommand inColorBufferNeedsClear,
                const SRenderInstanceId instanceId) override
    {
        m_RenderContext = &inRenderContext;
        if (m_Class.RenderInstance) {
            inRenderContext.PushPropertySet();
            if (m_Owner->GetAPIVersion() > 1) {
                m_Class.RenderInstance(m_Class.m_Class, m_Instance, ToCInterface(inEnvironment),
                                       ToCInterface(inPresentationScaleFactor),
                                       ToCInterface(inColorBufferNeedsClear));
            } else {
                TRenderFunctionV1 theV1Function =
                    reinterpret_cast<TRenderFunctionV1>(m_Class.RenderInstance);
                theV1Function(m_Class.m_Class, m_Instance, ToCInterfaceV1(inEnvironment),
                              ToCInterface(inPresentationScaleFactor),
                              ToCInterface(inColorBufferNeedsClear));
            }

            inRenderContext.PopPropertySet(true);
        }
    }

    void RenderWithClear(const SOffscreenRendererEnvironment &inEnvironment,
                         NVRenderContext &inRenderContext, QT3DSVec2 inPresScale,
                         SScene::RenderClearCommand inClearBuffer, QT3DSVec4 inClearColor,
                         const SRenderInstanceId id)
    {
        Q_ASSERT(false);
    }

    // Implementors should implement one of the two interfaces below.

    // If this renderer supports picking that can return graph objects
    // then return an interface here.
    IGraphObjectPickQuery *GetGraphObjectPickQuery(const SRenderInstanceId) override { return NULL; }

    // If you *don't* support the GraphObjectPickIterator interface, then you should implement this
    // interface
    // The system will just ask you to pick.
    // If you return true, then we will assume that you swallowed the pick and will continue no
    // further.
    // else we will assume you did not and will continue the picking algorithm.
    bool Pick(const QT3DSVec2 &inMouseCoords, const QT3DSVec2 &inViewportDimensions,
              const SRenderInstanceId instanceId) override
    {
        if (m_Class.Pick) {
            if (m_RenderContext) {
                m_RenderContext->PushPropertySet();
                bool retval = m_Class.Pick(m_Class.m_Class, m_Instance, ToCInterface(inMouseCoords),
                                           ToCInterface(inViewportDimensions))
                    ? true
                    : false;
                m_RenderContext->PopPropertySet(true);
                return retval;
            }
        }
        return false;
    }

    TRenderPluginInstancePtr GetRenderPluginInstance() override { return m_Instance; }
    void Update(NVConstDataRef<SRenderPropertyValueUpdate> updateBuffer) override
    {
        m_Dirty = true;
        m_Owner->Update(updateBuffer, m_PropertyValues);
    }
    IRenderPluginClass &GetPluginClass() override { return *m_Owner; }
};

typedef eastl::pair<CRegisteredString, RenderPluginPropertyValueTypes::Enum> TStringTypePair;

struct PluginClassImpl : public IInternalPluginClass
{
    typedef nvhash_map<CRegisteredString, QT3DSU32> TStringIndexMap;
    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    TRenderPluginClass m_Class;
    CRegisteredString m_Type;
    CLoadedDynamicLibrary *m_DynamicLibrary;
    nvvector<SRenderPluginPropertyDeclaration> m_RegisteredProperties;
    TStringIndexMap m_ComponentNameToComponentIndexMap;
    nvvector<TStringTypePair> m_FullPropertyList;
    nvvector<TRenderPluginPropertyUpdate> m_UpdateBuffer;
    Qt3DSString m_TempString;
    QT3DSI32 m_APIVersion;

    QT3DSI32 mRefCount;

    PluginClassImpl(NVFoundationBase &fnd, IStringTable &strTable, TRenderPluginClass inClass,
                    CRegisteredString inType, CLoadedDynamicLibrary *inLibrary)
        : m_Foundation(fnd)
        , m_StringTable(strTable)
        , m_Class(inClass)
        , m_Type(inType)
        , m_DynamicLibrary(inLibrary)
        , m_RegisteredProperties(m_Foundation.getAllocator(),
                                 "PluginClassImpl::m_RegisteredProperties")
        , m_ComponentNameToComponentIndexMap(m_Foundation.getAllocator(),
                                             "PluginClassImpl::m_ComponentNameToComponentIndexMap")
        , m_FullPropertyList(m_Foundation.getAllocator(), "PluginClassImpl::m_FullPropertyList")
        , m_UpdateBuffer(m_Foundation.getAllocator(), "PluginClassImpl::m_UpdateBuffer")
        , m_APIVersion(m_Class.GetRenderPluginAPIVersion(m_Class.m_Class))
        , mRefCount(0)
    {
    }
    ~PluginClassImpl()
    {
        if (m_Class.ReleaseClass)
            m_Class.ReleaseClass(m_Class.m_Class);
        if (m_DynamicLibrary)
            NVDelete(m_Foundation.getAllocator(), m_DynamicLibrary);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    NVScopedRefCounted<IRenderPluginInstance> CreateInstance() override
    {
        if (m_Class.CreateInstance) {
            TRenderPluginInstancePtr instance =
                m_Class.CreateInstance(m_Class.m_Class, m_Type.c_str());
            if (instance) {
                InstanceImpl *retval = QT3DS_NEW(m_Foundation.getAllocator(), InstanceImpl)(
                    m_Foundation, instance, m_Class, *this, m_StringTable);
                return retval;
            }
        }
        return NVScopedRefCounted<IRenderPluginInstance>();
    }

    QT3DSI32 GetAPIVersion() override { return m_APIVersion; }

    void AddFullPropertyType(const char *name, RenderPluginPropertyValueTypes::Enum inType)
    {
        QT3DSU32 itemIndex = (QT3DSU32)m_FullPropertyList.size();
        CRegisteredString regName = m_StringTable.RegisterStr(name);
        bool inserted =
            m_ComponentNameToComponentIndexMap.insert(eastl::make_pair(regName, itemIndex)).second;
        if (inserted) {
            m_FullPropertyList.push_back(eastl::make_pair(regName, inType));
        } else {
            // Duplicate property declaration.
            QT3DS_ASSERT(false);
        }
    }

    void AddFullPropertyType(const char *name, const char *extension,
                             RenderPluginPropertyValueTypes::Enum inType)
    {
        m_TempString.assign(name);
        if (!isTrivial(extension)) {
            m_TempString.append(".");
            m_TempString.append(extension);
        }
        AddFullPropertyType(m_TempString.c_str(), inType);
    }

    void RegisterProperty(const SRenderPluginPropertyDeclaration &dec) override
    {
        QT3DSU32 startOffset = (QT3DSU32)m_FullPropertyList.size();

        switch (dec.m_Type) {

        case SRenderPluginPropertyTypes::Vector2:
            AddFullPropertyType(dec.m_Name, "x", RenderPluginPropertyValueTypes::Float);
            AddFullPropertyType(dec.m_Name, "y", RenderPluginPropertyValueTypes::Float);
            break;
        case SRenderPluginPropertyTypes::Color:
            AddFullPropertyType(dec.m_Name, "r", RenderPluginPropertyValueTypes::Float);
            AddFullPropertyType(dec.m_Name, "g", RenderPluginPropertyValueTypes::Float);
            AddFullPropertyType(dec.m_Name, "b", RenderPluginPropertyValueTypes::Float);
            break;
        case SRenderPluginPropertyTypes::Vector3:
            AddFullPropertyType(dec.m_Name, "x", RenderPluginPropertyValueTypes::Float);
            AddFullPropertyType(dec.m_Name, "y", RenderPluginPropertyValueTypes::Float);
            AddFullPropertyType(dec.m_Name, "z", RenderPluginPropertyValueTypes::Float);
            break;
        case SRenderPluginPropertyTypes::Boolean:
            AddFullPropertyType(dec.m_Name, RenderPluginPropertyValueTypes::Boolean);
            break;
        case SRenderPluginPropertyTypes::Float:
            AddFullPropertyType(dec.m_Name, RenderPluginPropertyValueTypes::Float);
            break;
        case SRenderPluginPropertyTypes::Long:
            AddFullPropertyType(dec.m_Name, RenderPluginPropertyValueTypes::Long);
            break;
        case SRenderPluginPropertyTypes::String:
            AddFullPropertyType(dec.m_Name, RenderPluginPropertyValueTypes::String);
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
        m_RegisteredProperties.push_back(dec);
        m_RegisteredProperties.back().m_StartOffset = startOffset;
    }

    NVConstDataRef<SRenderPluginPropertyDeclaration> GetRegisteredProperties() override
    {
        return m_RegisteredProperties;
    }

    SRenderPluginPropertyDeclaration GetPropertyDeclaration(CRegisteredString inPropName) override
    {
        for (QT3DSU32 idx = 0, end = m_RegisteredProperties.size(); idx < end; ++idx) {
            if (m_RegisteredProperties[idx].m_Name == inPropName)
                return m_RegisteredProperties[idx];
        }
        QT3DS_ASSERT(false);
        return SRenderPluginPropertyDeclaration();
    }

    // From which you can get the property name breakdown
    virtual eastl::pair<CRegisteredString, RenderPluginPropertyValueTypes::Enum>
    GetPropertyValueInfo(QT3DSU32 inIndex) override
    {
        if (inIndex < m_FullPropertyList.size())
            return m_FullPropertyList[inIndex];
        QT3DS_ASSERT(false);
        return eastl::pair<CRegisteredString, RenderPluginPropertyValueTypes::Enum>(
            CRegisteredString(), RenderPluginPropertyValueTypes::NoRenderPluginPropertyValue);
    }

    void PushUpdates(TRenderPluginInstancePtr instance, TPropertyValueList &propertyValues) override
    {
        m_UpdateBuffer.clear();
        for (QT3DSU32 idx = 0, end = propertyValues.size(); idx < end; ++idx) {
            SRenderPluginPropertyData &theData(propertyValues[idx]);
            if (theData.IsDirty())
                m_UpdateBuffer.push_back(theData.ClearDirty(m_FullPropertyList[idx].first));
        }
        if (m_Class.UpdateInstance)
            m_Class.UpdateInstance(m_Class.m_Class, instance, m_UpdateBuffer.data(),
                                   (long)m_UpdateBuffer.size());
    }

    void Update(NVConstDataRef<SRenderPropertyValueUpdate> updateBuffer,
                        TPropertyValueList &propertyValues) override
    {
        for (QT3DSU32 idx = 0, end = updateBuffer.size(); idx < end; ++idx) {
            const SRenderPropertyValueUpdate &update = updateBuffer[idx];
            TStringIndexMap::iterator iter =
                m_ComponentNameToComponentIndexMap.find(update.m_PropertyName);
            if (iter == m_ComponentNameToComponentIndexMap.end()) {
                QT3DS_ASSERT(false);
                continue;
            }

            QT3DSU32 propIndex = iter->second;
            if (update.m_Value.getType() != m_FullPropertyList[propIndex].second) {
                QT3DS_ASSERT(false);
                continue;
            }
            if (propIndex >= propertyValues.size())
                propertyValues.resize(propIndex + 1);
            propertyValues[propIndex].SetValue(update.m_Value);
        }
    }
};

struct PluginInstanceKey
{
    CRegisteredString m_Path;
    void *m_InstanceKey;
    PluginInstanceKey(CRegisteredString p, void *ik)
        : m_Path(p)
        , m_InstanceKey(ik)
    {
    }
    bool operator==(const PluginInstanceKey &rhs) const
    {
        return m_Path == rhs.m_Path && m_InstanceKey == rhs.m_InstanceKey;
    }
};
}

namespace eastl {
template <>
struct hash<PluginInstanceKey>
{
    size_t operator()(const PluginInstanceKey &k) const
    {
        return hash<CRegisteredString>()(k.m_Path)
            ^ hash<size_t>()(reinterpret_cast<size_t>(k.m_InstanceKey));
    }
    bool operator()(const PluginInstanceKey &lhs, const PluginInstanceKey &rhs) const
    {
        return lhs.m_Path == rhs.m_Path && lhs.m_InstanceKey == rhs.m_InstanceKey;
    }
};
}

namespace {

struct SLoadedPluginData
{
    CRegisteredString m_PluginPath;
    eastl::vector<SRenderPluginPropertyDeclaration> m_Properties;
};

typedef eastl::vector<SLoadedPluginData> TLoadedPluginDataList;

struct PluginManagerImpl : public IRenderPluginManager, public IRenderPluginManagerCore
{
    typedef nvhash_map<CRegisteredString, NVScopedRefCounted<IRenderPluginClass>> TLoadedClassMap;
    typedef nvhash_map<PluginInstanceKey, NVScopedRefCounted<IRenderPluginInstance>> TInstanceMap;
    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    TLoadedClassMap m_LoadedClasses;
    TInstanceMap m_Instances;
    NVScopedRefCounted<NVRenderContext> m_RenderContext;
    IInputStreamFactory &m_InputStreamFactory;
    QT3DSI32 mRefCount;
    TStr m_DllDir;
    TLoadedPluginDataList m_LoadedPluginData;

    PluginManagerImpl(NVFoundationBase &fnd, IStringTable &st, IInputStreamFactory &inFactory)
        : m_Foundation(fnd)
        , m_StringTable(st)
        , m_LoadedClasses(fnd.getAllocator(), "PluginManagerImpl::m_LoadedClasses")
        , m_Instances(fnd.getAllocator(), "PluginManagerImpl::m_Instances")
        , m_InputStreamFactory(inFactory)
        , mRefCount(0)
        , m_DllDir(ForwardingAllocator(fnd.getAllocator(), "PluginManagerImpl::m_DllDir"))
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    IRenderPluginClass *GetRenderPlugin(CRegisteredString inRelativePath) override
    {
        TLoadedClassMap::iterator iter = m_LoadedClasses.find(inRelativePath);
        if (iter != m_LoadedClasses.end())
            return iter->second;

        return NVScopedRefCounted<IRenderPluginClass>();
    }

    IRenderPluginClass *GetOrCreateRenderPlugin(CRegisteredString inRelativePath) override
    {
        TLoadedClassMap::iterator iter = m_LoadedClasses.find(inRelativePath);
        if (iter != m_LoadedClasses.end()) {
            return iter->second;
        }

        // We insert right here to keep us from going down this path potentially for every instance.
        iter =
            m_LoadedClasses
                .insert(eastl::make_pair(inRelativePath, NVScopedRefCounted<IRenderPluginClass>()))
                .first;
        eastl::string xmlDir, fname, extension;

        CFileTools::Split(inRelativePath.c_str(), xmlDir, fname, extension);

        eastl::string sharedLibrary(xmlDir);
        eastl::string subdir(qt3ds::foundation::System::getPlatformGLStr());
        eastl::string libdir;
        eastl::string libpath;

        CFileTools::CombineBaseAndRelative(xmlDir.c_str(), subdir.c_str(), libdir);
        CFileTools::CombineBaseAndRelative(libdir.c_str(), fname.c_str(), libpath);
#ifdef _DEBUG
        libpath.append("d");
#endif
        libpath.append(qt3ds::foundation::System::g_DLLExtension);
        eastl::string loadPath;
        if (m_DllDir.size()) {
            // Then we have to copy the dll to the dll directory before loading because the
            // filesystem
            // the plugin is on may not be executable.
            eastl::string targetFile;
            CFileTools::CombineBaseAndRelative(m_DllDir.c_str(), fname.c_str(), targetFile);
#ifdef _DEBUG
            targetFile.append("d");
#endif
            targetFile.append(qt3ds::foundation::System::g_DLLExtension);

            qCInfo(TRACE_INFO, "Copying plugin shared library from %s to %s",
                libpath.c_str(), targetFile.c_str());

            // try to open the library.
            NVScopedRefCounted<IRefCountedInputStream> theStream =
                m_InputStreamFactory.GetStreamForFile(libpath.c_str());
            if (!theStream) {
                qCCritical(INVALID_OPERATION, "Failed to load render plugin %s",
                    libpath.c_str());
                return NULL;
            }
            CFileSeekableIOStream outStream(targetFile.c_str(), FileWriteFlags());
            if (!outStream.IsOpen()) {
                qCCritical(INVALID_OPERATION, "Failed to load render plugin %s",
                    targetFile.c_str());
                return NULL;
            }

            QT3DSU8 buf[1024] = { 0 };
            for (QT3DSU32 len = theStream->Read(toDataRef(buf, 1024)); len;
                 len = theStream->Read(toDataRef(buf, 1024))) {
                outStream.Write(toDataRef(buf, len));
            }
            loadPath = targetFile;
        } else {
            QString path;
            m_InputStreamFactory.GetPathForFile(libpath.c_str(), path);
            loadPath = path.toUtf8().data();
        }
        CLoadedDynamicLibrary *library = NULL;
        TRenderPluginClass newPluginClass;
        memSet(&newPluginClass, 0, sizeof(newPluginClass));

        // Do not load plugin dlls during compilation steps or when we don't have a valid render
        // context.
        // They may try opengl access at some point and that would end in disaster during binary
        // save steps.
        if ((QT3DSU32)m_RenderContext->GetRenderContextType() != NVRenderContextValues::NullContext) {
            library = CLoadedDynamicLibrary::Create(loadPath.c_str(), m_Foundation);
            if (!library) {
                // try loading it from the system instead of from this specific path.  This means do
                // not use any extensions or any special
                // sauce.
                loadPath = fname;
#ifdef _DEBUG
                loadPath.append("d");
#endif
                library = CLoadedDynamicLibrary::Create(loadPath.c_str(), m_Foundation);
            }
        }

        if (library) {
            TCreateRenderPluginClassFunction CreateClass =
                reinterpret_cast<TCreateRenderPluginClassFunction>(
                    library->FindFunction("CreateRenderPlugin"));
            if (CreateClass) {
                newPluginClass = CreateClass(fname.c_str());
                if (newPluginClass.m_Class) {
                    // Check that the required functions are there.
                    if (newPluginClass.CreateInstance == NULL
                        || newPluginClass.QueryInstanceRenderSurface == NULL
                        || newPluginClass.RenderInstance == NULL
                        || newPluginClass.ReleaseInstance == NULL
                        || newPluginClass.ReleaseClass == NULL) {
                        if (newPluginClass.ReleaseClass)
                            newPluginClass.ReleaseClass(newPluginClass.m_Class);
                        qCCritical(INVALID_OPERATION,
                            "Failed to load render plugin: %s, required functions "
                            "missing.  Required functions are:"
                            "CreateInstance, QueryInstanceRenderSurface, "
                            "RenderInstance, ReleaseInstance, ReleaseClass",
                            inRelativePath.c_str());
                        NVDelete(m_Foundation.getAllocator(), library);
                        memSet(&newPluginClass, 0, sizeof(newPluginClass));
                    }
                }
            }
        }
        if (newPluginClass.m_Class) {
            PluginClassImpl *retval = QT3DS_NEW(m_Foundation.getAllocator(), PluginClassImpl)(
                m_Foundation, m_StringTable, newPluginClass,
                m_StringTable.RegisterStr(fname.c_str()), library);

            iter->second = retval;
            if (newPluginClass.InitializeClassGLResources) {
                m_RenderContext->PushPropertySet();
                newPluginClass.InitializeClassGLResources(newPluginClass.m_Class, loadPath.c_str());
                m_RenderContext->PopPropertySet(true);
            }
            return iter->second;
        }
        return NULL;
    }

    void SetDllDir(const char *inDllDir) override { m_DllDir.assign(nonNull(inDllDir)); }

    IRenderPluginInstance *GetOrCreateRenderPluginInstance(CRegisteredString inRelativePath,
                                                                   void *inKey) override
    {
        PluginInstanceKey theKey(inRelativePath, inKey);
        TInstanceMap::iterator iter = m_Instances.find(theKey);
        if (iter == m_Instances.end()) {
            IRenderPluginClass *theClass = GetOrCreateRenderPlugin(inRelativePath);
            NVScopedRefCounted<IRenderPluginInstance> theInstance;
            if (theClass)
                theInstance = theClass->CreateInstance();

            iter = m_Instances.insert(eastl::make_pair(theKey, theInstance)).first;
        }
        return iter->second.mPtr;
    }

    void Save(qt3ds::render::SWriteBuffer &ioBuffer,
                      const qt3ds::render::SStrRemapMap &inRemapMap,
                      const char8_t * /*inProjectDir*/) const override
    {
        QT3DSU32 numClasses = m_LoadedClasses.size();
        ioBuffer.write(numClasses);
        for (TLoadedClassMap::const_iterator iter = m_LoadedClasses.begin(),
                                             end = m_LoadedClasses.end();
             iter != end; ++iter) {
            CRegisteredString saveStr = iter->first;
            saveStr.Remap(inRemapMap);
            ioBuffer.write(saveStr);
            if (iter->second) {
                NVConstDataRef<SRenderPluginPropertyDeclaration> theProperties =
                    const_cast<IRenderPluginClass &>((*iter->second)).GetRegisteredProperties();
                ioBuffer.write(theProperties.size());
                for (QT3DSU32 idx = 0, end = theProperties.size(); idx < end; ++idx) {
                    SRenderPluginPropertyDeclaration theDec(theProperties[idx]);
                    theDec.m_Name.Remap(inRemapMap);
                    ioBuffer.write(theDec);
                }
            } else
                ioBuffer.write((QT3DSU32)0);
        }
    }

    void Load(NVDataRef<QT3DSU8> inData, CStrTableOrDataRef inStrDataBlock,
                      const char8_t * /*inProjectDir*/) override
    {
        qt3ds::render::SDataReader theReader(inData.begin(), inData.end());
        QT3DSU32 numClasses = theReader.LoadRef<QT3DSU32>();
        ForwardingAllocator alloc(m_Foundation.getAllocator(), "tempstrings");
        qt3ds::foundation::TStr workStr(alloc);
        nvvector<SRenderPluginPropertyDeclaration> propertyBuffer(m_Foundation.getAllocator(),
                                                                  "tempprops");
        for (QT3DSU32 classIdx = 0; classIdx < numClasses; ++classIdx) {
            CRegisteredString classPath = theReader.LoadRef<CRegisteredString>();
            classPath.Remap(inStrDataBlock);
            QT3DSU32 numProperties = theReader.LoadRef<QT3DSU32>();
            propertyBuffer.clear();
            for (QT3DSU32 propIdx = 0; propIdx < numProperties; ++propIdx) {
                propertyBuffer.push_back(theReader.LoadRef<SRenderPluginPropertyDeclaration>());
                propertyBuffer.back().m_Name.Remap(inStrDataBlock);
            }
            m_LoadedPluginData.push_back(SLoadedPluginData());
            m_LoadedPluginData.back().m_PluginPath = classPath;
            m_LoadedPluginData.back().m_Properties.assign(propertyBuffer.begin(),
                                                          propertyBuffer.end());
        }
    }
    IRenderPluginManager &GetRenderPluginManager(NVRenderContext &rc) override
    {
        m_RenderContext = rc;
        for (QT3DSU32 idx = 0, end = m_LoadedPluginData.size(); idx < end; ++idx) {
            // Now we can attempt to load the class.
            IRenderPluginClass *theClass =
                GetOrCreateRenderPlugin(m_LoadedPluginData[idx].m_PluginPath);
            if (theClass) {
                eastl::vector<SRenderPluginPropertyDeclaration> &propertyBuffer(
                    m_LoadedPluginData[idx].m_Properties);
                for (QT3DSU32 propIdx = 0, propEnd = propertyBuffer.size(); propIdx < propEnd;
                     ++propIdx) {
                    theClass->RegisterProperty(propertyBuffer[propIdx]);
                }
            }
        }
        m_LoadedPluginData.clear();
        return *this;
    }
};
}

IRenderPluginManagerCore &IRenderPluginManagerCore::Create(NVFoundationBase &inFoundation,
                                                           IStringTable &strTable,
                                                           IInputStreamFactory &inFactory)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), PluginManagerImpl)(inFoundation, strTable,
                                                                   inFactory);
}
