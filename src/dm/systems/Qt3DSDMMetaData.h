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

#include "Qt3DSDMMetaDataTypes.h"
#include "foundation/Qt3DSOption.h"
#include <functional>
#include <memory>
#include "Qt3DSDMActionInfo.h"
#include "Qt3DSDMStringTable.h"
#include "Qt3DSDMValue.h"

struct SShaderParameters;
namespace qt3ds {
namespace foundation {
    class IInStream;
}
}

namespace qt3dsdm {

using qt3ds::foundation::Option;
using qt3ds::QT3DSU32;
using std::pair;
using std::vector;

class ISignalConnection;
class ITransactionConsumer;
class IDOMWriter;
class IDOMReader;
class IDataCore;
class CDataCoreProducer;

struct MetaDataLoadWarningType
{
    enum Enum {
        Unknown = 0,
        InvalidProperty,
        InvalidEvent,
        InvalidHandler,
    };
};

struct MetaDataLoadWarningMessage
{
    enum Enum {
        Unknown = 0,
        GeneralError,
        MissingName,
        InvalidDefault,
    };
};

struct SMetaDataLoadWarning
{
    MetaDataLoadWarningType::Enum m_Type;
    MetaDataLoadWarningMessage::Enum m_Message;
    TCharStr m_ExtraInfo;

    SMetaDataLoadWarning(MetaDataLoadWarningType::Enum inType,
                         MetaDataLoadWarningMessage::Enum inMessage, TCharStr inInfo = TCharStr())
        : m_Type(inType)
        , m_Message(inMessage)
        , m_ExtraInfo(inInfo)
    {
    }
    SMetaDataLoadWarning()
        : m_Type(MetaDataLoadWarningType::Unknown)
        , m_Message(MetaDataLoadWarningMessage::Unknown)
    {
    }
};

/**
 * Meta data class to hold meta data descriptions of the objects used in DataModel.
 * A user-visible type in DataModel (things in the timeline) has a specific set of metadata,
 * the type can have properties, events, handlers, and references.
 *
 * Properties are exactly what they sound like.  Events are messages the object can send,
 * and handlers are functions that are exported from the object.
 *
 * Events are hooked up to handlers using an ActionCore which takes tuples
 * of instance, event, handler and slide and maintains a specific instance of an action.
 *
 * References allow us to track which properties are being accessed.  Properties are stripped
 * from the engine portion of the runtime (although they still appear in the scenegraph)
 * if they aren't animated nor set in a given slide.
 */
class IMetaData
{
protected:
    virtual ~IMetaData() {}
public:
    typedef const TCharStr &TStrType;

    ////////////////////////////////////////////////////////////////////////////////////
    // Sharing some utility objects
    virtual IStringTable &GetStringTable() = 0;
    virtual TStringTablePtr GetStringTablePtr() = 0;
    virtual std::shared_ptr<IDataCore> GetDataCore() = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Canonical Instances
    // Map an instance to a given type name.  The type name is stored, along with the instance
    // handle in the file if this object is *not* canonical.
    // If this object is canonical, then we expect this mapping setup before loading the
    // canonical data, and we will use a type lookup instead of a direct handle lookup
    // when loading canonical information.
    virtual void SetInstanceAsCanonical(Qt3DSDMInstanceHandle inInstance, TStrType inTypename) = 0;

    virtual Qt3DSDMInstanceHandle GetCanonicalInstanceForType(TStrType inTypename) = 0;
    // If this instance wasn't registered as canonical, then we return empty.
    virtual Option<TCharStr> GetTypeForCanonicalInstance(Qt3DSDMInstanceHandle inInstance) = 0;
    // Gets the type for this instance via derivation
    virtual Option<TCharStr> GetTypeForInstance(Qt3DSDMInstanceHandle inInstance) = 0;
    // Get group count for instance
    virtual QT3DSU32 GetGroupCountForInstance(Qt3DSDMInstanceHandle inInstance) = 0;
    // Get all group names
    virtual QT3DSU32 GetGroupNamesForInstance(Qt3DSDMInstanceHandle inInstance,
                                              std::vector<TCharStr> &outNames) = 0;
    // Get group count for instance
    virtual Option<TCharStr> GetGroupFilterNameForInstance(Qt3DSDMInstanceHandle inInstance,
                                                           long inIndex) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Categories
    // Categories appear in the UI to divide up large lists of events, actions, and properties.
    // Returns <handle,true> if a category was created
    // else returns <handle,false>
    virtual pair<Qt3DSDMCategoryHandle, bool> GetOrCreateCategory(TStrType inName) = 0;

    virtual void SetCategoryInfo(Qt3DSDMCategoryHandle inCategory, TStrType inIcon,
                                 TStrType inHighlight, TStrType inDescription) = 0;
    virtual void DestroyCategory(Qt3DSDMCategoryHandle inCategory) = 0;

    virtual Option<SCategoryInfo> GetCategoryInfo(Qt3DSDMCategoryHandle inCategory) = 0;
    virtual Qt3DSDMCategoryHandle FindCategoryByName(TStrType inName) = 0;
    virtual void GetCategories(vector<Qt3DSDMCategoryHandle> &outCategories) = 0;
    virtual Option<SCategoryInfo> GetEventCategory(TStrType inName) = 0;
    virtual Option<SCategoryInfo> GetHandlerCategory(TStrType inName) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Properties
    // Will create the property on the instance associated with this type name if it doesn't exist.
    // Metadata properties can be redifined for child instances and this re-definition will override
    // the parent definition.  We currently use this to give different formal names and description
    // to the sourcepath property (for models it is mesh, for materials it is image).
    virtual Qt3DSDMMetaDataPropertyHandle
    CreateMetaDataProperty(Qt3DSDMInstanceHandle inInstance) = 0;

    // For properties, you set the default values separately
    // This may delete the underlying data model property rebuild it.
    virtual void SetMetaDataPropertyInfo(Qt3DSDMMetaDataPropertyHandle inPropertyHandle,
                                         TStrType inName, TStrType inFormalName,
                                         TStrType inDescription, TStrType inUsage,
                                         CompleteMetaDataType::Enum inDataType,
                                         const SValue &inDefaultValue,
                                         const TMetaDataData &inMetaData, TStrType inGroupName,
                                         bool inIsHidden = false, bool inIsAnimatable = false,
                                         bool inIsControllable = false) = 0;

    // Destroy just this meta data property
    // Does not destroy the underlying data core property, so this function isn't a perfect
    // reverse of the above system.  This *just* destroyed the meta data pointed to by
    // this property.
    virtual void DestroyMetaDataProperty(Qt3DSDMMetaDataPropertyHandle inProperty) = 0;

    virtual Qt3DSDMMetaDataPropertyHandle GetMetaDataProperty(Qt3DSDMInstanceHandle inInstance,
                                                              TStrType inPropertyName) = 0;
    virtual Qt3DSDMMetaDataPropertyHandle GetMetaDataProperty(Qt3DSDMInstanceHandle inInstance,
                                                              Qt3DSDMPropertyHandle inProperty) = 0;
    virtual Option<SMetaDataPropertyInfo>
    GetMetaDataPropertyInfo(Qt3DSDMMetaDataPropertyHandle inProperty) = 0;
    // Get all of the meta data properties defined on this object or its derivation parents
    virtual void GetMetaDataProperties(Qt3DSDMInstanceHandle inInstance,
                                       vector<Qt3DSDMMetaDataPropertyHandle> &outProperties) = 0;

    // Get the meta data properties defined on *only* this object, don't search parents
    virtual Qt3DSDMMetaDataPropertyHandle
    GetOrCreateSpecificMetaDataProperty(Qt3DSDMInstanceHandle inInstance,
                                        TStrType inPropertyName) = 0;
    virtual void
    GetSpecificMetaDataProperties(Qt3DSDMInstanceHandle inInstance,
                                  vector<Qt3DSDMMetaDataPropertyHandle> &outProperties) = 0;

    virtual TCharStr GetFormalName(Qt3DSDMInstanceHandle inInstance,
                                   Qt3DSDMPropertyHandle inProperty) = 0;
    virtual AdditionalMetaDataType::Value GetAdditionalMetaDataType(Qt3DSDMInstanceHandle inInstance,
                                                                    Qt3DSDMPropertyHandle inProperty) = 0;
    virtual TMetaDataData GetAdditionalMetaDataData(Qt3DSDMInstanceHandle inInstance,
                                                    Qt3DSDMPropertyHandle inProperty) = 0;
    virtual bool IsCustomProperty(Qt3DSDMInstanceHandle inInstance,
                                  Qt3DSDMPropertyHandle inProperty) = 0;
    virtual SValue GetDefaultValue(Qt3DSDMInstanceHandle inInstance,
                                   Qt3DSDMPropertyHandle inProperty) = 0;

    virtual void
    SetMetaDataPropertyFilters(Qt3DSDMMetaDataPropertyHandle inProperty,
                               qt3ds::foundation::NVConstDataRef<SPropertyFilterInfo> inFilters) = 0;
    virtual qt3ds::foundation::NVConstDataRef<SPropertyFilterInfo>
    GetMetaDataPropertyFilters(Qt3DSDMMetaDataPropertyHandle inProperty) = 0;
    virtual void RemoveMetaDataPropertyFilters(Qt3DSDMMetaDataPropertyHandle inProperty) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Events
    // CreateMetaDataEvent was original named CreateEvent but this collides with
    // the microsoft macro CreateEvent which expands to CreateEventA or CreateEventW
    virtual Qt3DSDMEventHandle CreateMetaDataEvent(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SetEventInfo(Qt3DSDMEventHandle inEventHandle, TStrType inName,
                              TStrType inFormalName, TStrType inCategory,
                              TStrType inDescription) = 0;

    virtual void DestroyEvent(Qt3DSDMEventHandle inEventHandle) = 0;

    virtual void GetEvents(Qt3DSDMInstanceHandle inInstance, TEventHandleList &outEvents) = 0;
    virtual Qt3DSDMEventHandle FindEvent(Qt3DSDMInstanceHandle inInstance, TStrType inName) = 0;
    virtual Option<SEventInfo> GetEventInfo(Qt3DSDMEventHandle inEventHandle) = 0;
    virtual bool IsCustomEvent(Qt3DSDMEventHandle inEventHandle) = 0;

    // Get/Find an event that occurs on just this instance, don't search parent instances
    virtual void GetSpecificEvents(Qt3DSDMInstanceHandle inInstance,
                                   TEventHandleList &outEvents) = 0;
    virtual Qt3DSDMEventHandle GetOrCreateSpecificEvent(Qt3DSDMInstanceHandle inInstance,
                                                        TStrType inName) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Handlers
    virtual Qt3DSDMHandlerHandle CreateHandler(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SetHandlerInfo(Qt3DSDMHandlerHandle inHandle, TStrType inName,
                                TStrType inFormalName, TStrType inCategory,
                                TStrType inDescription) = 0;
    virtual void DestroyHandler(Qt3DSDMHandlerHandle inHandlerHandle) = 0;

    virtual Qt3DSDMHandlerHandle FindHandlerByName(Qt3DSDMInstanceHandle inInstance,
                                                   TStrType inName) = 0;
    virtual Option<SHandlerInfo> GetHandlerInfo(Qt3DSDMHandlerHandle inHandlerHandle) = 0;
    virtual void GetHandlers(Qt3DSDMInstanceHandle inInstance, THandlerHandleList &outHandlers) = 0;
    virtual bool IsCustomHandler(Qt3DSDMHandlerHandle inEventHandle) = 0;

    virtual void GetSpecificHandlers(Qt3DSDMInstanceHandle inInstance,
                                     THandlerHandleList &outHandlers) = 0;
    virtual Qt3DSDMHandlerHandle GetOrCreateSpecificHandler(Qt3DSDMInstanceHandle inInstance,
                                                            TStrType inName) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Handler Arguments
    virtual QT3DSU32 AddHandlerArgument(Qt3DSDMHandlerHandle inHandler) = 0;
    virtual void
    SetHandlerArgumentInfo(Qt3DSDMHandlerHandle inHandler, QT3DSU32 inArgIndex, TStrType inName,
                           TStrType inFormalName, TStrType inDescription,
                           CompleteMetaDataType::Enum inDataType, const SValue &inDefaultValue,
                           const TMetaDataData &inMetaData, HandlerArgumentType::Value inArgType) = 0;

    virtual void DestroyHandlerArgument(Qt3DSDMHandlerHandle inHandler, QT3DSU32 inArgIndex) = 0;

    virtual Option<SMetaDataHandlerArgumentInfo>
    FindHandlerArgumentByName(Qt3DSDMHandlerHandle inHandler, TStrType inName) = 0;
    virtual void GetHandlerArguments(Qt3DSDMHandlerHandle inHandler,
                                     vector<SMetaDataHandlerArgumentInfo> &outArguments) = 0;
    virtual Option<SMetaDataHandlerArgumentInfo>
    GetHandlerArgumentInfo(Qt3DSDMHandlerHandle inHandle, QT3DSU32 inIndex) = 0;
    virtual QT3DSU32 GetNumHandlerArguments(Qt3DSDMHandlerHandle inHandler) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // References
    // Duplicate references are removed from this instance
    virtual void AddReference(Qt3DSDMInstanceHandle inInstance, TStrType inRefString) = 0;
    virtual void DestroyReferences(Qt3DSDMInstanceHandle inInstance) = 0;

    // Does the recursive gather from all the parents.  Duplicate references are removed from the
    // final list.
    virtual void GetReferences(Qt3DSDMInstanceHandle inInstance,
                               std::vector<TCharStr> &outReferences) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Instance-global functions
    // Destroy all meta data that relates to this instance.
    // Calling this on a derived instance does nothing, this only works if this specific
    // instance was mapped directly to meta data.
    virtual void DestroyMetaData(Qt3DSDMInstanceHandle inInstance) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Serialization
    // You can either save out in canonical format (and it will only save canonical-instance-related
    // information *or* you can save out in normal format where we link directly to instance handle
    // instead of to typename
    virtual void Save(IDOMWriter &inWriter) = 0;
    // Loading expects the canonical instances to be setup already which means that
    // it will perform lookups based on
    // One huge assumption is that the reader is using the same string table
    // that this object was built with
    // InIsCanonical tells us whether to mark loaded categories as canonical
    // or not.
    virtual void Load(IDOMReader &ioStream) = 0;

    // Load meta data and apply it to just this instance
    virtual void LoadInstance(IDOMReader &inReader, Qt3DSDMInstanceHandle inInstance,
                              const TCharStr &inName,
                              std::vector<SMetaDataLoadWarning> &outWarnings) = 0;

    // Save just this instances meta data out to the writer
    virtual void SaveInstance(IDOMWriter &inWriter, Qt3DSDMInstanceHandle inInstance) = 0;

    // Load effect meta data from file and apply it to just this instance
    virtual void LoadEffectInstance(const char *inShaderFile, Qt3DSDMInstanceHandle inInstance,
                                    const TCharStr &inName,
                                    std::vector<SMetaDataLoadWarning> &outWarnings,
                                    qt3ds::foundation::IInStream &inStream) = 0;

    virtual bool IsEffectInstanceRegistered(const char *inName) = 0;
    virtual void LoadEffectXML(IDOMReader &inStream, Qt3DSDMInstanceHandle inInstance,
                               const TCharStr &inObjectName,
                               std::vector<SMetaDataLoadWarning> &outWarnings,
                               const TCharStr &inSourcePath) = 0;
    virtual bool LoadEffectXMLFromSourcePath(const char *inSourcePath,
                                             Qt3DSDMInstanceHandle inInstance,
                                             const TCharStr &inObjectName,
                                             std::vector<SMetaDataLoadWarning> &outWarnings,
                                             qt3ds::foundation::IInStream &inStream) = 0;
    virtual Option<SMetaDataEffect> GetEffectBySourcePath(const char *inSourcePath) = 0;
    virtual Option<SMetaDataEffect> GetEffectByName(const char *inName) = 0;

    virtual void LoadMaterialInstance(const char *inShaderFile,
                                      Qt3DSDMInstanceHandle inInstance,
                                      const TCharStr &inName,
                                      std::vector<SMetaDataLoadWarning> &outWarnings,
                                      qt3ds::foundation::IInStream &inStream) = 0;
    virtual bool IsMaterialClassRegistered(const char *inName) = 0;
    virtual void LoadMaterialClassXML(IDOMReader &inStream, Qt3DSDMInstanceHandle inInstance,
                                      const TCharStr &inObjectName,
                                      std::vector<SMetaDataLoadWarning> &outWarnings,
                                      const TCharStr &inSourcePath) = 0;
    virtual bool LoadMaterialClassFromSourcePath(const char *inSourcePath,
                                                 Qt3DSDMInstanceHandle inInstance,
                                                 const TCharStr &inObjectName,
                                                 std::vector<SMetaDataLoadWarning> &outWarnings,
                                                 qt3ds::foundation::IInStream &inStream) = 0;
    virtual Option<SMetaDataCustomMaterial> GetMaterialBySourcePath(const char *inSourcePath) = 0;
    virtual Option<SMetaDataCustomMaterial> GetMaterialByName(const char *inName) = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    // Undo/Redo
    virtual void SetConsumer(std::shared_ptr<ITransactionConsumer> inConsumer) = 0;

#ifndef QT3DSDM_META_DATA_NO_SIGNALS
    ////////////////////////////////////////////////////////////////////////////////////
    // Signals
    // These events are sent out only once, they aren't set into the transaction consumer
    // to be sent out upon undo and redo.  These events are meant for chaining together side effects
    // so other systems don't have references to invalid handler arguments and handles.
    // Also, the objects in these messages are still valid so you can query information about them
    // at this time.  They will be destroyed subsequent to the event.
    virtual std::shared_ptr<ISignalConnection>
    ConnectInternalCategoryDestroyed(std::function<void(Qt3DSDMCategoryHandle)> inCallback) = 0;
    virtual std::shared_ptr<ISignalConnection> ConnectInternalPropertyDestroyed(
            std::function<void(Qt3DSDMMetaDataPropertyHandle)> inCallback) = 0;
    virtual std::shared_ptr<ISignalConnection>
    ConnectInternalEventDestroyed(std::function<void(Qt3DSDMEventHandle)> inCallback) = 0;
    virtual std::shared_ptr<ISignalConnection>
    ConnectInternalHandlerDestroyed(std::function<void(Qt3DSDMHandlerHandle)> inCallback) = 0;
    virtual std::shared_ptr<ISignalConnection> ConnectInternalHandlerArgDestroyed(
            std::function<void(Qt3DSDMHandlerHandle, QT3DSU32)> inCallback) = 0;
#endif

    ////////////////////////////////////////////////////////////////////////////////////
    // Creation/Lifetime Management
    friend class std::shared_ptr<IMetaData>;
    // The data core is used to create the properties if they don't exist or
    // query their underlying type if they do exist.  We also subscribe to
    // the datacore's beforeInstanceDelete function in order to ensure
    // that we delete our existing meta data for instances that are being deleted.
    static std::shared_ptr<IMetaData> CreateNewMetaData(std::shared_ptr<IDataCore> inDataCore);
};

typedef std::shared_ptr<IMetaData> TNewMetaDataPtr;
}
