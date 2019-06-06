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
#include "DataCoreProducer.h"
#include "HandleSystemTransactions.h"
#include "VectorTransactions.h"
#include "SignalsImpl.h"
#ifdef _WIN32
#pragma warning(disable : 4503) // decorated name length exceeded
#endif
namespace qt3dsdm {

Qt3DSDMInstanceHandle CDataCoreProducer::CreateInstance(Qt3DSDMInstanceHandle inTargetId)
{
    Qt3DSDMInstanceHandle retval = m_Data->CreateInstance(inTargetId);
    CREATE_HANDLE_CREATE_TRANSACTION(m_Consumer, retval, m_Data->m_Objects);
    GetDataCoreSender()->SignalInstanceCreated(retval);
    return retval;
}

inline tuple<Qt3DSDMPropertyHandle, Qt3DSDMPropertyDefinition>
TransformProperty(Qt3DSDMPropertyHandle inProperty, CSimpleDataCore &inData)
{
    return make_tuple(inProperty, inData.GetProperty(inProperty));
}

inline void SignalPropertyRemoved(Qt3DSDMInstanceHandle inInstance,
                                  tuple<Qt3DSDMPropertyHandle, Qt3DSDMPropertyDefinition> inData,
                                  IDataCoreSignalSender *inSender)
{
    inSender->SignalPropertyRemoved(inInstance, get<0>(inData), get<1>(inData).m_Name.wide_str(),
                                    get<1>(inData).m_Type);
}

void CDataCoreProducer::DeleteInstance(Qt3DSDMInstanceHandle inInstance)
{
    TIntList theProperties;
    TIntList theInstances;

    GetDataCoreSender()->SignalBeforeInstanceDeleted(inInstance);
    // Ensure the instance exists
    m_Data->GetInstanceNF(inInstance, m_Data->m_Objects);
    do_all(m_Data->m_Objects,
           std::bind(CSimpleDataCore::FindRelatedItemsForDelete, inInstance.GetHandleValue(),
                       std::ref(theProperties), std::ref(theInstances), std::placeholders::_1));

    if (m_Consumer) {
        CREATE_HANDLE_DELETE_TRANSACTION(m_Consumer, inInstance, m_Data->m_Objects);
        do_all(theProperties, std::bind(HandleDeleteTransaction, __FILE__, __LINE__,
                                        std::placeholders::_1,
                                        std::ref(m_Data->m_Objects), m_Consumer));
    }

    vector<tuple<Qt3DSDMPropertyHandle, Qt3DSDMPropertyDefinition>> theDefinitionList;
    theDefinitionList.resize(theProperties.size());

    function<tuple<Qt3DSDMPropertyHandle, Qt3DSDMPropertyDefinition>(Qt3DSDMPropertyHandle)>
        thePropertyTransform(bind(TransformProperty, std::placeholders::_1, ref(*m_Data)));
    transform(theProperties.begin(), theProperties.end(), theDefinitionList.begin(),
              thePropertyTransform);

    GetDataCoreSender()->SignalInstanceDeleted(inInstance);

    m_Data->DeleteInstance(inInstance);
    // Signal that these theProperties are no longer with us.
    do_all(theDefinitionList, bind(SignalPropertyRemoved, inInstance,
                                   std::placeholders::_1, GetDataCoreSender()));
}

bool CDataCoreProducer::IsInstanceOrDerivedFrom(Qt3DSDMInstanceHandle inInstance,
                                                Qt3DSDMInstanceHandle inParent) const
{
    return m_Data->IsInstanceOrDerivedFrom(inInstance, inParent);
}

void CDataCoreProducer::GetInstances(TInstanceHandleList &outInstances) const
{
    m_Data->GetInstances(outInstances);
}
void CDataCoreProducer::GetInstancesDerivedFrom(TInstanceHandleList &outInstances,
                                                Qt3DSDMInstanceHandle inParentHandle) const
{
    m_Data->GetInstancesDerivedFrom(outInstances, inParentHandle);
}

struct ClearInstanceParentCacheTransaction : public ITransaction
{
    const CDataModelInstance &m_Instance;
    ClearInstanceParentCacheTransaction(const char *inFile, int inLine,
                                        const CDataModelInstance &inst)
        : ITransaction(inFile, inLine)
        , m_Instance(inst)
    {
    }
    void Do() override { m_Instance.ClearParentCache(); }
    void Undo() override { m_Instance.ClearParentCache(); }
};

void CDataCoreProducer::DeriveInstance(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMInstanceHandle inParent)
{
    m_Data->DeriveInstance(inInstance, inParent);
    TDataModelInstancePtr theInstance =
        CSimpleDataCore::GetInstanceNF(inInstance, m_Data->m_Objects);
    if (m_Consumer) {
        CreateHashMapInsertTransaction(
            __FILE__, __LINE__, m_Consumer,
            make_pair(inParent.GetHandleValue(),
                      CSimpleDataCore::GetInstanceNF(inParent, m_Data->m_Objects)),
            theInstance->m_Parents);
        m_Consumer->OnTransaction(std::static_pointer_cast<ITransaction>(
            std::make_shared<ClearInstanceParentCacheTransaction>(__FILE__, __LINE__,
                                                                    *theInstance)));
    }
    GetDataCoreSender()->SignalInstanceDerived(inInstance, inParent);
}

void CDataCoreProducer::GetInstanceParents(Qt3DSDMInstanceHandle inHandle,
                                           TInstanceHandleList &outParents) const
{
    m_Data->GetInstanceParents(inHandle, outParents);
}

Qt3DSDMPropertyHandle CDataCoreProducer::AddProperty(Qt3DSDMInstanceHandle inInstance,
                                                    TCharPtr inName, DataModelDataType::Value inPropType)
{
    Qt3DSDMPropertyHandle retval = m_Data->AddProperty(inInstance, inName, inPropType);
    TDataModelInstancePtr theInstance =
        CSimpleDataCore::GetInstanceNF(inInstance, m_Data->m_Objects);
    CreateVecInsertTransaction<int>(__FILE__, __LINE__, m_Consumer, retval,
                                    theInstance->m_Properties);
    CREATE_HANDLE_CREATE_TRANSACTION(m_Consumer, retval, m_Data->m_Objects);
    GetDataCoreSender()->SignalPropertyAdded(inInstance, retval, inName, inPropType);
    return retval;
}

void CDataCoreProducer::GetInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                              TPropertyHandleList &outProperties) const
{
    m_Data->GetInstanceProperties(inInstance, outProperties);
}

const Qt3DSDMPropertyDefinition &
CDataCoreProducer::GetProperty(Qt3DSDMPropertyHandle inProperty) const
{
    return m_Data->GetProperty(inProperty);
}

void CDataCoreProducer::RemoveProperty(Qt3DSDMPropertyHandle inProperty)
{
    Qt3DSDMPropertyDefinition theDef = GetProperty(inProperty);
    TDataModelInstancePtr theInstance =
        CSimpleDataCore::GetInstanceNF(theDef.m_Instance, m_Data->m_Objects);
    if (find_if<TIntList::iterator>(theInstance->m_Properties,
                                    std::bind(equal_to<int>(), inProperty, std::placeholders::_1))
        != theInstance->m_Properties.end()) {
        CreateVecEraseTransaction<int>(__FILE__, __LINE__, m_Consumer, inProperty,
                                       theInstance->m_Properties);
        CREATE_HANDLE_DELETE_TRANSACTION(m_Consumer, inProperty, m_Data->m_Objects);
        GetDataCoreSender()->SignalPropertyRemoved(theDef.m_Instance, inProperty,
                                                   theDef.m_Name.wide_str(), theDef.m_Type);
        m_Data->RemoveProperty(inProperty);
    } else {
        throw PropertyNotFound(L"");
    }
}

inline void AddCopyInstancePropertyTransaction(int inProperty, const TIntList &inOriginalList,
                                               CDataModelInstance &inInstance,
                                               TTransactionConsumerPtr &inConsumer)
{
    // if this property was never in the original list
    if (find_if<TIntList::const_iterator>(inOriginalList,
                                          std::bind(equal_to<int>(), inProperty,
                                                    std::placeholders::_1))
        == inOriginalList.end())
        CreateVecInsertTransaction<int>(__FILE__, __LINE__, inConsumer,
                                        Qt3DSDMPropertyHandle(inProperty), inInstance.m_Properties);
}

inline void AddCopyInstanceValuePropertyTransaction(const TPropertyPair &inProperty,
                                                    const TPropertyPairHash &inOriginalList,
                                                    CDataModelInstance &inInstance,
                                                    TTransactionConsumerPtr &inConsumer)
{
    // if this property was never in the original list
    if (inOriginalList.end() == inOriginalList.find(inProperty.first))
        CreateHashMapInsertTransaction(__FILE__, __LINE__, inConsumer, inProperty,
                                       inInstance.m_PropertyValues);
}

struct InstancePropertyValuesTransaction : public ITransaction
{
    CDataModelInstance &m_Instance;
    TIntList m_OldProperties;
    TPropertyPairList m_OldValues;
    TIntList m_NewProperties;
    TPropertyPairList m_NewValues;

    InstancePropertyValuesTransaction(const char *inFile, int inLine,
                                      CDataModelInstance &inInstance)
        : ITransaction(inFile, inLine)
        , m_Instance(inInstance)
        , m_OldProperties(inInstance.m_Properties)
    {
        inInstance.ToPropertyPairList(m_OldValues);
    }

    void SetNew()
    {
        m_NewProperties = m_Instance.m_Properties;
        m_Instance.ToPropertyPairList(m_NewValues);
    }

    void Do() override
    {
        m_Instance.m_Properties = m_NewProperties;
        m_Instance.FromPropertyPairList(m_NewValues);
    }

    void Undo() override
    {
        m_Instance.m_Properties = m_OldProperties;
        m_Instance.FromPropertyPairList(m_OldValues);
    }
};

void CDataCoreProducer::CopyInstanceProperties(Qt3DSDMInstanceHandle inSrcInstance,
                                               Qt3DSDMInstanceHandle inDestInstance)
{
    TDataModelInstancePtr theInstance =
        CSimpleDataCore::GetInstanceNF(inDestInstance, m_Data->m_Objects);
    if (m_Consumer) {
        // Create the transaction object setting its 'old values' property to the current instance
        // property values.
        std::shared_ptr<InstancePropertyValuesTransaction> theTransaction(
            std::make_shared<InstancePropertyValuesTransaction>(__FILE__, __LINE__,
                                                                  ref(*theInstance)));

        // Change the current instance property values
        m_Data->CopyInstanceProperties(inSrcInstance, inDestInstance);

        // Ask the transaction to copy the new values into it's new values datastructure.
        theTransaction->SetNew();

        m_Consumer->OnTransaction(std::static_pointer_cast<ITransaction>(theTransaction));
    } else {
        m_Data->CopyInstanceProperties(inSrcInstance, inDestInstance);
    }
}

Qt3DSDMPropertyHandle
CDataCoreProducer::GetAggregateInstancePropertyByName(Qt3DSDMInstanceHandle inInstance,
                                                      const TCharStr &inStr) const
{
    return m_Data->GetAggregateInstancePropertyByName(inInstance, inStr);
}

void CDataCoreProducer::GetAggregateInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                                       TPropertyHandleList &outProperties) const
{
    m_Data->GetAggregateInstanceProperties(inInstance, outProperties);
}

void CDataCoreProducer::GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inHandle,
                                                          TPropertyHandleValuePairList &outValues)
{
    m_Data->GetSpecificInstancePropertyValues(inHandle, outValues);
}

bool CDataCoreProducer::HasAggregateInstanceProperty(Qt3DSDMInstanceHandle inInstance,
                                                     Qt3DSDMPropertyHandle inProperty) const
{
    return m_Data->HasAggregateInstanceProperty(inInstance, inProperty);
}

void CDataCoreProducer::CheckValue(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                   const SValue &inValue) const
{
    return m_Data->CheckValue(inInstance, inProperty, inValue);
}

bool CDataCoreProducer::GetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                                 Qt3DSDMPropertyHandle inProperty,
                                                 SValue &outValue) const
{
    return m_Data->GetInstancePropertyValue(inHandle, inProperty, outValue);
}

inline void EraseProperty(TPropertyPairHash &inProperties, int inProperty)
{
    inProperties.erase(
        find_if(inProperties.begin(), inProperties.end(),
                std::bind(CSimpleDataCore::InstancePropertyMatches, inProperty,
                          std::placeholders::_1)));
}

void CDataCoreProducer::SetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                                 Qt3DSDMPropertyHandle inProperty,
                                                 const SValue &inValue)
{
    // Two possible courses of actions.  The property exists, in which case
    // we need to get its old value.
    // If it doesn't exist, then we need to erase for undo.
    TPropertyValuePair theOldValue(PropertyValueFlags(PropertyValueFlags::SetViaAutoPropagate),
                                   SInternValue(0.0f, GetStringTable()));
    TDataModelInstancePtr theInstance = CSimpleDataCore::GetInstanceNF(inHandle, m_Data->m_Objects);
    TPropertyPairHash::iterator theItem =
        theInstance->m_PropertyValues.find(inProperty.GetHandleValue());
    bool foundProp = theItem != theInstance->m_PropertyValues.end();
    if (foundProp)
        theOldValue = theItem->second;
    m_Data->SetInstancePropertyValue(inHandle, inProperty, inValue);
    TPropertyValuePair theNewValue =
        theInstance->m_PropertyValues.find(inProperty.GetHandleValue())->second;
    if (m_Consumer) {
        // Check if we already have the entry
        TSlideInstancePropertyPair theKey(inHandle, inProperty);
        TPropertyMergeMap::iterator iter = m_PropertyMergeMap.find(theKey);
        // Then we merge the values in to the original event where they
        // first happened.
        if (iter != m_PropertyMergeMap.end()) {
            iter->second->Update(theNewValue);
            return; // don't send the signal if we just updated value
        } else // Else we add a new merge entry.
        {
            TPropertyMergeMapEntry theEntry;
            if (!foundProp)
                theEntry = CreateHashMapInsertTransaction(
                    __FILE__, __LINE__, m_Consumer,
                    make_pair(inProperty.GetHandleValue(), theNewValue),
                    theInstance->m_PropertyValues);
            else
                theEntry = CreateHashMapSwapTransaction(__FILE__, __LINE__, m_Consumer,
                                                        inProperty.GetHandleValue(), theOldValue,
                                                        theNewValue, theInstance->m_PropertyValues);
            m_PropertyMergeMap.insert(std::make_pair(theKey, theEntry));
        }
    }
    GetPropertyCoreSender()->SignalInstancePropertyValue(inHandle, inProperty, inValue);
}

bool CDataCoreProducer::HandleValid(int inHandle) const
{
    return m_Data->HandleValid(inHandle);
}

void CDataCoreProducer::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    m_Consumer = inConsumer;
    m_PropertyMergeMap.clear();
}

TSignalConnectionPtr CDataCoreProducer::ConnectInstancePropertyValue(
    const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue &)>
        &inCallback)
{
    return GetPropertyCoreProvider()->ConnectInstancePropertyValue(inCallback);
}

TSignalConnectionPtr CDataCoreProducer::ConnectInstanceCreated(
    const std::function<void(Qt3DSDMInstanceHandle)> &inCallback)
{
    return GetDataCoreProvider()->ConnectInstanceCreated(inCallback);
}

TSignalConnectionPtr CDataCoreProducer::ConnectInstanceDeleted(
    const std::function<void(Qt3DSDMInstanceHandle)> &inCallback)
{
    return GetDataCoreProvider()->ConnectInstanceDeleted(inCallback);
}

TSignalConnectionPtr CDataCoreProducer::ConnectBeforeInstanceDeleted(
    const std::function<void(Qt3DSDMInstanceHandle)> &inCallback)
{
    return GetDataCoreProvider()->ConnectBeforeInstanceDeleted(inCallback);
}

TSignalConnectionPtr CDataCoreProducer::ConnectInstanceDerived(
    const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback)
{
    return GetDataCoreProvider()->ConnectInstanceDerived(inCallback);
}

TSignalConnectionPtr CDataCoreProducer::ConnectInstanceParentRemoved(
    const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback)
{
    return GetDataCoreProvider()->ConnectInstanceParentRemoved(inCallback);
}

TSignalConnectionPtr CDataCoreProducer::ConnectPropertyAdded(
    const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, TCharPtr,
                               DataModelDataType::Value)> &inCallback)
{
    return GetDataCoreProvider()->ConnectPropertyAdded(inCallback);
}

TSignalConnectionPtr CDataCoreProducer::ConnectPropertyRemoved(
    const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, TCharPtr,
                               DataModelDataType::Value)> &inCallback)
{
    return GetDataCoreProvider()->ConnectPropertyRemoved(inCallback);
}

void CDataCoreProducer::InitSignallers()
{
    m_InstancePropertyCoreSignaller = CreatePropertyCoreSignaller();
    m_DataCoreSignaller = CreateDataCoreSignaller();
}

IInstancePropertyCoreSignalProvider *CDataCoreProducer::GetPropertyCoreProvider()
{
    return dynamic_cast<IInstancePropertyCoreSignalProvider *>(
        m_InstancePropertyCoreSignaller.get());
}

IInstancePropertyCoreSignalSender *CDataCoreProducer::GetPropertyCoreSender()
{
    return dynamic_cast<IInstancePropertyCoreSignalSender *>(m_InstancePropertyCoreSignaller.get());
}

IDataCoreSignalProvider *CDataCoreProducer::GetDataCoreProvider()
{
    return dynamic_cast<IDataCoreSignalProvider *>(m_DataCoreSignaller.get());
}

IDataCoreSignalSender *CDataCoreProducer::GetDataCoreSender()
{
    return dynamic_cast<IDataCoreSignalSender *>(m_DataCoreSignaller.get());
}
}
