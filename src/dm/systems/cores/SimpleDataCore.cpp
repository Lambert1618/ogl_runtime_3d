/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "SimpleDataCore.h"

namespace qt3dsdm {
Qt3DSDMDebugLogFunction g_DataModelDebugLogger = NULL;
// Instances
Qt3DSDMInstanceHandle CSimpleDataCore::CreateInstance(Qt3DSDMInstanceHandle hdl)
{
    int nextId = hdl;
    if (hdl.Valid() == false)
        nextId = GetNextId();

    return CreateInstanceWithHandle(nextId);
}

inline void RemoveParentFromClass(int inParent, int inChild, THandleObjectMap &inMap)
{
    TDataModelInstancePtr theInstance = CSimpleDataCore::GetInstanceNF(inChild, inMap);
    theInstance->m_Parents.erase(theInstance->m_Parents.find(inParent));
    // Cascade the delete operation
}

void CSimpleDataCore::DeleteInstance(Qt3DSDMInstanceHandle inHandle)
{
    // Ensure it exists in the first place.
    CSimpleDataCore::GetInstanceNF(inHandle, m_Objects); // Check for instance existance
    CSimpleDataCore::EraseHandle(inHandle, m_Objects);
    TIntList properties;
    TIntList instances;
    do_all(m_Objects, std::bind(FindRelatedItemsForDelete, inHandle.GetHandleValue(),
                                std::ref(properties), std::ref(instances), std::placeholders::_1));
    do_all(instances, std::bind(RemoveParentFromClass, inHandle.GetHandleValue(),
                                std::placeholders::_1, std::ref(m_Objects)));
    do_all(properties, std::bind(EraseHandle, std::placeholders::_1, std::ref(m_Objects)));
}

void CSimpleDataCore::GetInstances(TInstanceHandleList &outInstances) const
{
    // reserve the vector to m_Objects.size() to prevent vector reallocation
    // m_Objects.size() is the upper bound of the no of instances
    // the exact no of instances = m_Objects.size() - the no of properties as added by
    // CClientDataModelBridge::InitializeDataCore (as of now, we have 31 properties)
    outInstances.reserve(m_Objects.size());
    do_all(m_Objects, std::bind(MaybeAddObject<CDataModelInstance, Qt3DSDMInstanceHandle>,
                                std::placeholders::_1, std::ref(outInstances)));
}

// Get instances that are derived from a specific parent
void CSimpleDataCore::GetInstancesDerivedFrom(TInstanceHandleList &outInstances,
                                              Qt3DSDMInstanceHandle inParentHandle) const
{
    do_all(m_Objects, std::bind(&CSimpleDataCore::AddInstanceIfDerivedFrom, this,
                                std::placeholders::_1,
                                std::ref(outInstances), inParentHandle));
}

// add instance that is derived from a specific parent
void CSimpleDataCore::AddInstanceIfDerivedFrom(const std::pair<int, THandleObjectPtr> &inItem,
                                               TInstanceHandleList &outInstances,
                                               Qt3DSDMInstanceHandle inParentHandle) const
{
    if (inItem.second->GetType() == CHandleObject::EHandleObjectTypeCDataModelInstance) {
        TDataModelInstancePtr theInstance = static_pointer_cast<CDataModelInstance>(inItem.second);
        if (IsInstanceOrDerivedFromHelper(theInstance, inParentHandle))
            outInstances.push_back(inItem.first);
    }
}

// Derivation
void CSimpleDataCore::DeriveInstance(Qt3DSDMInstanceHandle inInstance, Qt3DSDMInstanceHandle inParent)
{
    if (g_DataModelDebugLogger)
        g_DataModelDebugLogger("CSimpleDataCore::DeriveInstance Enter");
    TDataModelInstancePtr theInstance = CSimpleDataCore::GetInstanceNF(inInstance, m_Objects);
    if (theInstance->m_Parents.find(inParent) == theInstance->m_Parents.end()) {
        std::shared_ptr<CDataModelInstance> theParent(GetInstanceNF(inParent, m_Objects));
        theInstance->m_Parents.insert(make_pair(inParent, theParent));
    }
    if (g_DataModelDebugLogger)
        g_DataModelDebugLogger("CSimpleDataCore::DeriveInstance Leave");
}

void CSimpleDataCore::GetInstanceParents(Qt3DSDMInstanceHandle inHandle,
                                         TInstanceHandleList &outParents) const
{
    const TDataModelInstancePtr theInstance = GetInstanceNF(inHandle, m_Objects);
    for (CDataModelInstance::TInstancePairList::const_iterator theParent =
             theInstance->m_Parents.begin();
         theParent != theInstance->m_Parents.end(); ++theParent)
        outParents.push_back(theParent->first);
}

inline bool ComparePropertyNames(const TCharStr &inName, int inPropHandle,
                                 const THandleObjectMap &inObjects)
{
    if (CSimpleDataCore::GetPropertyDefinitionNF(inPropHandle, inObjects)->m_Definition.m_Name
        == inName)
        return true;
    return false;
}

inline const wchar_t *SafeStrPtr(const wchar_t *inData)
{
    return inData == NULL ? L"" : inData;
}

// Properties
Qt3DSDMPropertyHandle CSimpleDataCore::AddProperty(Qt3DSDMInstanceHandle inInstance, TCharPtr inName,
                                                  DataModelDataType::Value inPropType)
{
    QT3DSDM_LOG_FUNCTION("CSimpleDataCore::AddProperty");
    QT3DSDM_DEBUG_LOG(m_StringTable->GetNarrowStr(inName));
    TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);
    TCharStr theName(inName);
    if (find_if<TIntList::iterator>(
            theInstance->m_Properties,
            std::bind(ComparePropertyNames, std::ref(theName),
                      std::placeholders::_1, std::ref(m_Objects)))
        != theInstance->m_Properties.end()) {
        QT3DSDM_DEBUG_LOG("Property Exists!!");
        throw PropertyExists(L"");
    }

    int nextId = GetNextId();
    return AddPropertyWithHandle(nextId, inInstance, inName, inPropType);
}

void CSimpleDataCore::GetInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                            TPropertyHandleList &outProperties) const
{
    transformv_all(GetInstanceNF(inInstance, m_Objects)->m_Properties, outProperties);
}

const Qt3DSDMPropertyDefinition &CSimpleDataCore::GetProperty(Qt3DSDMPropertyHandle inProperty) const
{
    const CDataModelPropertyDefinitionObject *theProp =
        GetPropertyDefinitionNF(inProperty, m_Objects);
    return theProp->m_Definition;
}

void CSimpleDataCore::RemoveProperty(Qt3DSDMPropertyHandle inProperty)
{
    CDataModelPropertyDefinitionObject *theProp =
        CSimpleDataCore::GetPropertyDefinitionNF(inProperty, m_Objects);
    TDataModelInstancePtr theInstance =
        CSimpleDataCore::GetInstanceNF(theProp->m_Definition.m_Instance, m_Objects);
    erase_if(theInstance->m_Properties,
             std::bind(equal_to<int>(), inProperty.GetHandleValue(), std::placeholders::_1));
    CSimpleDataCore::EraseHandle(inProperty, m_Objects);
}

inline bool PropertyMatches(int inProperty, Qt3DSDMPropertyHandle inTarget)
{
    return inProperty == inTarget.GetHandleValue();
}

inline bool GetInstanceValue(Qt3DSDMInstanceHandle inInstanceHandle,
                             Qt3DSDMPropertyHandle inPropertyHandle, CSimpleDataCore &inDataCore,
                             SValue &outValue)
{
    const TDataModelInstancePtr theInstance =
        CSimpleDataCore::GetInstanceNF(inInstanceHandle, inDataCore.m_Objects);

    TPropertyPairHash::const_iterator theInstanceProp =
        theInstance->m_PropertyValues.find(inPropertyHandle.GetHandleValue());
    if (theInstanceProp != theInstance->m_PropertyValues.end()) {
        outValue = theInstanceProp->second.second.GetValue();
        return true;
    }
    return false;
}

inline void CopyInstanceProperty(Qt3DSDMPropertyHandle inSrcPropertyHandle,
                                 Qt3DSDMInstanceHandle inSrcInstanceHandle,
                                 Qt3DSDMInstanceHandle inInstanceHandle, CSimpleDataCore &inDataCore)
{
    // create the property definition that matches the source
    const Qt3DSDMPropertyDefinition &theProperty = inDataCore.GetProperty(inSrcPropertyHandle);
    Qt3DSDMPropertyHandle theNewProperty =
        inDataCore.AddProperty(inInstanceHandle, theProperty.m_Name.wide_str(), theProperty.m_Type);
    // copy the value if one exists on the src.
    SValue theValue;
    if (GetInstanceValue(inSrcInstanceHandle, inSrcPropertyHandle, inDataCore, theValue))
        inDataCore.SetInstancePropertyValue(inInstanceHandle, theNewProperty, theValue);
}

// logic : if destination property is one gained through derivation in inSrcInstanceHandle, copy the
// value over.
inline void CopyAggregatedPropertyValues(Qt3DSDMPropertyHandle inDestPropertyHandle,
                                         Qt3DSDMInstanceHandle inSrcInstanceHandle,
                                         Qt3DSDMInstanceHandle inInstanceHandle,
                                         const TPropertyHandleList &inSrcNonAggregateList,
                                         CSimpleDataCore &inDataCore)
{
    if (find_if(inSrcNonAggregateList.begin(), inSrcNonAggregateList.end(),
                bind(PropertyMatches, inDestPropertyHandle, std::placeholders::_1))
            == inSrcNonAggregateList.end()
        && inDataCore.HasAggregateInstanceProperty(inSrcInstanceHandle, inDestPropertyHandle)) {
        SValue theValue;
        if (GetInstanceValue(inSrcInstanceHandle, inDestPropertyHandle, inDataCore, theValue))
            inDataCore.SetInstancePropertyValue(inInstanceHandle, inDestPropertyHandle, theValue);
    }
}

void CSimpleDataCore::CopyInstanceProperties(Qt3DSDMInstanceHandle inSrcInstance,
                                             Qt3DSDMInstanceHandle inDestInstance)
{
    TPropertyHandleList theList;
    GetInstanceProperties(inSrcInstance, theList);
    do_all(theList, bind(CopyInstanceProperty, std::placeholders::_1,
                         inSrcInstance, inDestInstance, ref(*this)));

    TPropertyHandleList theDestList;
    GetAggregateInstanceProperties(inDestInstance, theDestList);
    do_all(theDestList, std::bind(CopyAggregatedPropertyValues, std::placeholders::_1,
                                  inSrcInstance, inDestInstance,
                                  theList, std::ref(*this)));
}

void CSimpleDataCore::RemoveCachedValues(Qt3DSDMInstanceHandle inInstance)
{
    const TDataModelInstancePtr theInstance = CSimpleDataCore::GetInstanceNF(inInstance, m_Objects);
    theInstance->RemoveCachedValues();
}

void GetAggregateProperties(const TDataModelInstancePtr inInstance, TIntList &inVisited,
                            TPropertyHandleList &outProperties)
{
    if (find(inVisited.begin(), inVisited.end(), inInstance->m_Handle) == inVisited.end()) {
        inVisited.push_back(inInstance->m_Handle);
        size_t theSize = outProperties.size();
        outProperties.resize(theSize + inInstance->m_Properties.size());
        transform(inInstance->m_Properties.begin(), inInstance->m_Properties.end(),
                  outProperties.begin() + theSize, identity<int>);
        for (CDataModelInstance::TInstancePairList::const_iterator theParent =
                 inInstance->m_Parents.begin();
             theParent != inInstance->m_Parents.end(); ++theParent)
            GetAggregateProperties(theParent->second, inVisited, outProperties);
    }
}

template <typename TPredicate>
inline std::tuple<bool, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle>
RecurseFindProperty(const TDataModelInstancePtr inInstance, TPredicate inPredicate)
{
    TIntList::const_iterator theFind = eastl::find_if(inInstance->m_Properties.begin(),
                                                      inInstance->m_Properties.end(), inPredicate);
    if (theFind != inInstance->m_Properties.end())
        return make_tuple(true, inInstance->m_Handle, *theFind);

    for (CDataModelInstance::TInstancePairList::const_iterator theParent =
             inInstance->m_Parents.begin();
         theParent != inInstance->m_Parents.end(); ++theParent) {
        std::tuple<bool, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle> theValue =
            RecurseFindProperty(theParent->second, inPredicate);
        if (get<0>(theValue))
            return theValue;
    }
    return make_tuple(false, 0, 0);
}

inline bool PropertyNameMatches(int inProperty, const THandleObjectMap &inObjects,
                                const TCharStr &inStr)
{
    const CDataModelPropertyDefinitionObject *theProp =
        CSimpleDataCore::GetPropertyDefinitionNF(inProperty, inObjects);
    return (theProp->m_Definition.m_Name == inStr);
}

Qt3DSDMPropertyHandle
CSimpleDataCore::GetAggregateInstancePropertyByName(Qt3DSDMInstanceHandle inInstance,
                                                    const TCharStr &inStr) const
{
    const TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);
    return get<2>(
        RecurseFindProperty(theInstance, std::bind(PropertyNameMatches,
                                                   std::placeholders::_1, std::ref(m_Objects),
                                                   std::ref(inStr))));
}

// A simplified RecurseFindProperty function
bool RecurseFindPropertyMatches(const TDataModelInstancePtr inInstance,
                                Qt3DSDMPropertyHandle inProperty)
{
    TIntList::const_iterator theFind =
        find(inInstance->m_Properties.begin(), inInstance->m_Properties.end(),
             inProperty.GetHandleValue());
    if (theFind != inInstance->m_Properties.end())
        return true;

    for (CDataModelInstance::TInstancePairList::const_iterator theParent =
             inInstance->m_Parents.begin();
         theParent != inInstance->m_Parents.end(); ++theParent) {
        if (RecurseFindPropertyMatches(theParent->second, inProperty))
            return true;
    }
    return false;
}

bool CSimpleDataCore::HasAggregateInstanceProperty(Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMPropertyHandle inProperty) const
{
    const TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);
    if (theInstance->m_PropertyValues.find(inProperty.GetHandleValue())
        != theInstance->m_PropertyValues.end())
        return true;

    return RecurseFindPropertyMatches(theInstance, inProperty);
}

void CSimpleDataCore::GetAggregateInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                                     TPropertyHandleList &outProperties) const
{
    TIntList inVisited;
    const TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);
    GetAggregateProperties(theInstance, inVisited, outProperties);
}

void CSimpleDataCore::CheckValue(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                 const SValue &inValue) const
{
    CheckPropertyExistence(GetInstanceNF(inInstance, m_Objects), inProperty, m_Objects);
    const CDataModelPropertyDefinitionObject *theDefinition =
        CSimpleDataCore::GetPropertyDefinitionNF(inProperty, m_Objects);
    CheckValueType(theDefinition->m_Definition.m_Type, inValue);
}

bool CSimpleDataCore::GetInstancePropertyValueHelper(const TDataModelInstancePtr inInstance,
                                                     Qt3DSDMPropertyHandle inProperty,
                                                     SValue &outValue) const
{
    TPropertyPairHash::const_iterator theInstanceProp =
        inInstance->m_PropertyValues.find(inProperty);
    if (theInstanceProp != inInstance->m_PropertyValues.end()) {
        outValue = theInstanceProp->second.second.GetValue();
        return true;
    } else {
        // Is the property valid?
        const CDataModelPropertyDefinitionObject *theProp =
            GetPropertyDefinitionNF(inProperty, m_Objects);
        int thePropInstance = theProp->m_Definition.m_Instance;
        if (thePropInstance != inInstance->m_Handle) {
            for (CDataModelInstance::TInstancePairList::const_iterator theParent =
                     inInstance->m_Parents.begin();
                 theParent != inInstance->m_Parents.end(); ++theParent) {
                if (IsInstanceOrDerivedFromHelper(theParent->second, thePropInstance)
                    && GetInstancePropertyValueHelper(theParent->second, inProperty, outValue)) {
                    // Quietly propagate to this instance so next time we won't go through this
                    // large property lookup chain.
                    inInstance->m_PropertyValues.insert(std::make_pair(
                        inProperty,
                        TPropertyValuePair(
                            PropertyValueFlags(PropertyValueFlags::SetViaAutoPropagate),
                            SInternValue::ISwearThisHasAlreadyBeenInternalized(outValue))));
                    return true;
                }
            }
        }
    }
    return false;
}

bool CSimpleDataCore::GetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                               Qt3DSDMPropertyHandle inProperty,
                                               SValue &outValue) const
{
    if (IsInstance(inHandle) == false || IsProperty(inProperty) == false) {
        Q_ASSERT(0);
        return false;
    }
    const TDataModelInstancePtr theInstance = GetInstanceNF(inHandle, m_Objects);
    return GetInstancePropertyValueHelper(theInstance, inProperty, outValue);
}

void CSimpleDataCore::SetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                               Qt3DSDMPropertyHandle inProperty,
                                               const SValue &inValue)
{
    if (IsInstance(inHandle) == false || IsProperty(inProperty) == false) {
        Q_ASSERT(0);
        return;
    }
    CheckValue(inHandle, inProperty, inValue);
    UncheckedSetSpecificInstancePropertyValue(inHandle, inProperty, inValue, PropertyValueFlags());
}

void CSimpleDataCore::GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inHandle,
                                                        TPropertyHandleValuePairList &outValues)
{
    if (IsInstance(inHandle) == false)
        return;

    TDataModelInstancePtr theInstance = GetInstanceNF(inHandle, m_Objects);
    for (TPropertyPairHash::const_iterator theIter = theInstance->m_PropertyValues.begin(),
                                           theEnd = theInstance->m_PropertyValues.end();
         theIter != theEnd; ++theIter) {
        const pair<int, TPropertyValuePair> &thePair(*theIter);
        if (thePair.second.first.IsSetViaAutoPropagation() == false) {
            Qt3DSDMPropertyHandle thePropertyHandle(thePair.first);
            SValue theValue(thePair.second.second.GetValue());
            outValues.push_back(make_pair(thePair.first, theValue));
        }
    }
}

bool CSimpleDataCore::IsInstance(int inHandle) const
{
    return HandleObjectValid<CDataModelInstance>(inHandle, m_Objects);
}
bool CSimpleDataCore::IsProperty(int inHandle) const
{
    return HandleObjectValid<CDataModelPropertyDefinitionObject>(inHandle, m_Objects);
}

Qt3DSDMInstanceHandle CSimpleDataCore::CreateInstanceWithHandle(int inHandle)
{
    if (g_DataModelDebugLogger)
        g_DataModelDebugLogger("CSimpleDataCore::CreateInstance Enter");
    if (HandleValid(inHandle)) {
        if (g_DataModelDebugLogger) {
            g_DataModelDebugLogger("CSimpleDataCore::CreateInstance Handle Exists!!");
            char buf[32];
            sprintf(buf, "%d", inHandle);
            g_DataModelDebugLogger(buf);
        }
        throw HandleExists(L"");
    }

    THandleObjectPtr theHandleObjectPtr(new CDataModelInstance(inHandle));
    const pair<int, THandleObjectPtr> thePair(std::make_pair(inHandle, theHandleObjectPtr));
    m_Objects.insert(thePair);

    if (g_DataModelDebugLogger)
        g_DataModelDebugLogger("CSimpleDataCore::CreateInstance Leave");

    return inHandle;
}

Qt3DSDMPropertyHandle CSimpleDataCore::AddPropertyWithHandle(int inHandle,
                                                            Qt3DSDMInstanceHandle inInstance,
                                                            TCharPtr inName,
                                                            DataModelDataType::Value inPropType)
{
    QT3DSDM_DEBUG_LOG("CSimpleDataCore::AddPropertyWithHandle Enter");
    QT3DSDM_DEBUG_LOG(m_StringTable->GetNarrowStr(inName));
    if (HandleValid(inHandle)) {
        if (g_DataModelDebugLogger) {
            g_DataModelDebugLogger("CSimpleDataCore::AddPropertyWithHandle Handle Exists!!");
            char buf[32];
            sprintf(buf, "%d", inHandle);
            g_DataModelDebugLogger(buf);
        }

        throw HandleExists(L"");
    }

    TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);

    Qt3DSDMPropertyDefinition theDefinition(inInstance, SafeStrPtr(inName), inPropType);
    THandleObjectPtr theHandleObjectPtr(
        new CDataModelPropertyDefinitionObject(inHandle, theDefinition));
    const pair<int, THandleObjectPtr> thePair(std::make_pair(inHandle, theHandleObjectPtr));
    m_Objects.insert(thePair);
    theInstance->m_Properties.push_back(inHandle);
    QT3DSDM_DEBUG_LOG("CSimpleDataCore::AddPropertyWithHandle Leave");
    return inHandle;
}

bool CSimpleDataCore::GetSpecificInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                       Qt3DSDMPropertyHandle inProperty,
                                                       SValue &outValue) const
{
    const TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);
    TPropertyPairHash::const_iterator theInstanceProp =
        theInstance->m_PropertyValues.find(inProperty);
    if (theInstanceProp != theInstance->m_PropertyValues.end()) {
        outValue = theInstanceProp->second.second.GetValue();
        return true;
    }
    return false;
}

void CSimpleDataCore::GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inInstance,
                                                        TPropertyPairHash &outValues) const
{
    const TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);
    outValues = theInstance->m_PropertyValues;
}

void CSimpleDataCore::UncheckedSetSpecificInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                                Qt3DSDMPropertyHandle inProperty,
                                                                const SValue &inValue,
                                                                PropertyValueFlags inIsUserSet)
{
    TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);
    TPropertyValuePair theValuePair(inIsUserSet, SInternValue(inValue, GetStringTable()));

    std::pair<TPropertyPairHash::iterator, bool> theInsert = theInstance->m_PropertyValues.insert(
        std::make_pair(inProperty.GetHandleValue(), theValuePair));
    if (theInsert.second == false)
        theInsert.first->second = theValuePair;
}

bool CSimpleDataCore::CheckParentPropertyExistenceWithFail(const TDataModelInstancePtr inInstance,
                                                           int inProperty,
                                                           const THandleObjectMap &inObjects)
{
    if (find(inInstance->m_Properties.begin(), inInstance->m_Properties.end(), inProperty)
        != inInstance->m_Properties.end())
        return true;
    for (CDataModelInstance::TInstancePairList::const_iterator theParent =
             inInstance->m_Parents.begin();
         theParent != inInstance->m_Parents.end(); ++theParent) {
        if (CheckParentPropertyExistenceWithFail(theParent->second, inProperty, inObjects))
            return true;
    }
    return false;
}

// safety check to see if property exists
void CSimpleDataCore::CheckPropertyExistence(const TDataModelInstancePtr inInstance, int inProperty,
                                             const THandleObjectMap &inObjects)
{
    if (!CheckParentPropertyExistenceWithFail(inInstance, inProperty, inObjects))
        throw PropertyNotFound(L"");
}

bool CSimpleDataCore::IsInstanceOrDerivedFromHelper(const TDataModelInstancePtr inInstance,
                                                    Qt3DSDMInstanceHandle inParent) const
{
    if (inInstance->m_Handle == inParent) // Am I this object?
        return true;
    return inInstance->IsDerivedFrom(inParent);
}

bool CSimpleDataCore::IsInstanceOrDerivedFrom(Qt3DSDMInstanceHandle inInstance,
                                              Qt3DSDMInstanceHandle inParent) const
{
    if (IsInstance(inInstance) == false || IsInstance(inParent) == false)
        return false;
    if (inInstance == inParent) // Am I this object?
        return true;
    const TDataModelInstancePtr theInstance = GetInstanceNF(inInstance, m_Objects);
    return IsInstanceOrDerivedFromHelper(theInstance, inParent);
}
}
