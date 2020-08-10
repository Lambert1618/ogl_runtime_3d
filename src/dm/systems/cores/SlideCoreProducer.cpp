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
#ifdef _WIN32
#pragma warning(disable : 4503) // decorated name length exceeded
#endif
#include "SlideCoreProducer.h"
#include "HandleSystemTransactions.h"
#include "VectorTransactions.h"
#include "SignalsImpl.h"

using namespace std;

namespace qt3dsdm {

Qt3DSDMSlideHandle CSlideCoreProducer::CreateSlide(Qt3DSDMInstanceHandle inInstance)
{
    Qt3DSDMSlideHandle retval = m_Data->CreateSlide(inInstance);
    CREATE_HANDLE_CREATE_TRANSACTION(m_Consumer, retval, m_Data->m_Objects);
    GetSignalSender()->SendSlideCreated(retval);
    return retval;
}

Qt3DSDMInstanceHandle CSlideCoreProducer::GetSlideInstance(Qt3DSDMSlideHandle inSlide) const
{
    return m_Data->GetSlideInstance(inSlide);
}

Qt3DSDMSlideHandle CSlideCoreProducer::GetSlideByInstance(Qt3DSDMInstanceHandle inSlide) const
{
    return m_Data->GetSlideByInstance(inSlide);
}

void SendPropertyRemoved(const TSlideEntry &inEntry, Qt3DSDMSlideHandle inSlide,
                         ISlideCoreSignalSender *inSender)
{
    inSender->SendPropertyValueRemoved(inSlide, get<0>(inEntry), get<1>(inEntry), get<2>(inEntry));
}

void RecurseCreateDeleteTransactions(TTransactionConsumerPtr inConsumer, Qt3DSDMSlideHandle inSlide,
                                     THandleObjectMap &inObjects, ISlideCoreSignalSender *inSender)
{
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(inSlide, inObjects);
    inSender->SendBeforeSlideDeleted(theSlide->m_Handle);
    do_all(theSlide->m_Children, std::bind(RecurseCreateDeleteTransactions, inConsumer,
                                           std::placeholders::_1,
                                           std::ref(inObjects), inSender));
    if (inConsumer)
        CREATE_HANDLE_DELETE_TRANSACTION(inConsumer, inSlide, inObjects);
    inSender->SendSlideDeleted(theSlide->m_Handle);
}

void CSlideCoreProducer::DeleteSlide(Qt3DSDMSlideHandle inSlide, TInstanceHandleList &outInstances)
{
    // Ensure exists
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(inSlide, m_Data->m_Objects);
    if (theSlide->m_Parent) {
        SSlide *theParent = CSimpleSlideCore::GetSlideNF(theSlide->m_Parent, m_Data->m_Objects);
        CreateVecEraseTransaction<int>(__FILE__, __LINE__, m_Consumer, inSlide,
                                       theParent->m_Children);
    }
    RecurseCreateDeleteTransactions(m_Consumer, inSlide, m_Data->m_Objects, GetSignalSender());
    // Not any more...
    m_Data->DeleteSlide(inSlide, outInstances);
}

void CSlideCoreProducer::GetSlides(TSlideHandleList &outSlides) const
{
    m_Data->GetSlides(outSlides);
}

long CSlideCoreProducer::GetSlideTime(Qt3DSDMSlideHandle inSlide) const
{
    return m_Data->GetSlideTime(inSlide);
}

void CSlideCoreProducer::SetSlideTime(Qt3DSDMSlideHandle inSlide, long inNewTime)
{
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(inSlide, m_Data->m_Objects);
    long oldTime = theSlide->m_Time;
    theSlide->m_Time = inNewTime;
    if (m_Consumer) {
        m_Consumer->OnTransaction(std::shared_ptr<ITransaction>(CREATE_GENERIC_TRANSACTION(
            std::bind(&CSlideCoreProducer::SetSlideTime, this, inSlide, inNewTime),
            std::bind(&CSlideCoreProducer::SetSlideTime, this, inSlide, oldTime))));
    } else
        GetSignalSender()->SendSlideTimeChanged(inSlide);
}

inline void SetSlideParent(THandleObjectMap &inObjects, int inSlide, int inParent)
{
    CSimpleSlideCore::GetSlideNF(inSlide, inObjects)->m_Parent = inParent;
}

void CSlideCoreProducer::DeriveSlide(Qt3DSDMSlideHandle inSlide, Qt3DSDMSlideHandle inParent,
                                     int inIndex)
{
    // Integrity checks to ensure operation will proceed
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(inSlide, m_Data->m_Objects);
    SSlide *theNewParent = NULL;
    if (inParent)
        theNewParent = CSimpleSlideCore::GetSlideNF(inParent, m_Data->m_Objects);
    SSlide *theOldParent = NULL;
    if (theSlide->m_Parent)
        theOldParent = CSimpleSlideCore::GetSlideNF(theSlide->m_Parent, m_Data->m_Objects);

    if (theSlide->m_Parent && m_Consumer) {
        m_Consumer->OnTransaction(TTransactionPtr(CREATE_GENERIC_TRANSACTION(
            std::bind(SetSlideParent, std::ref(m_Data->m_Objects), inSlide, inParent),
            std::bind(SetSlideParent, std::ref(m_Data->m_Objects), inSlide,
                        theSlide->m_Parent))));
        if (theOldParent)
            CreateVecEraseTransaction<int>(__FILE__, __LINE__, m_Consumer, inSlide,
                                           theOldParent->m_Children);
        GetSignalSender()->SendSlideDerived(inSlide, 0, inIndex);
    }
    m_Data->DeriveSlide(inSlide, inParent, inIndex);
    if (theNewParent)
        CreateVecInsertTransaction<int>(__FILE__, __LINE__, m_Consumer, inSlide,
                                        theNewParent->m_Children);
    GetSignalSender()->SendSlideDerived(inSlide, inParent, inIndex);
}

Qt3DSDMSlideHandle CSlideCoreProducer::GetParentSlide(Qt3DSDMSlideHandle inSlide) const
{
    return m_Data->GetParentSlide(inSlide);
}

void CSlideCoreProducer::GetChildSlides(Qt3DSDMSlideHandle inSlide,
                                        TSlideHandleList &outChildren) const
{
    m_Data->GetChildSlides(inSlide, outChildren);
}

int CSlideCoreProducer::GetChildIndex(Qt3DSDMSlideHandle inParent, Qt3DSDMSlideHandle inChild) const
{
    return m_Data->GetChildIndex(inParent, inChild);
}

bool CSlideCoreProducer::GetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle inHandle,
                                                  Qt3DSDMPropertyHandle inProperty,
                                                  SValue &outValue) const
{
    return m_Data->GetInstancePropertyValue(inSlide, inHandle, inProperty, outValue);
}

inline void EraseProperty(TSlideEntryList &inProperties, int inInstance, int inProperty)
{
    erase_if(inProperties,
             std::bind(CSimpleSlideCore::PropertyFound, inInstance, inProperty,
                       std::placeholders::_1));
}

void CSlideCoreProducer::SetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle inHandle,
                                                  Qt3DSDMPropertyHandle inProperty,
                                                  const SValue &inValue)
{
    std::pair<SSlide *, SInternValue *> slideAndProp(
        m_Data->ResolveSetInstancePropertyValue(inSlide, inHandle, inProperty));
    DoForceSetInstancePropertyValue(slideAndProp.first->m_Handle, inHandle, inProperty, inValue);
}

struct HashMapDataValueInsertTransaction
    : public HashMapInsertTransaction<TSlideInstancePropertyPair, SInternValue,
                                      TSlideEntryHash::hasher>
{
    typedef HashMapInsertTransaction<TSlideInstancePropertyPair, SInternValue,
                                     TSlideEntryHash::hasher>
        TBase;
    HashMapDataValueInsertTransaction(
        const char *inFile, int inLine, TSlideEntryHash &map,
        const std::pair<TSlideInstancePropertyPair, SInternValue> &val)
        : TBase(inFile, inLine, map, val)
    {
    }
    void Do() override
    {
        SValue theTempValue = m_Value.second.GetValue();
        TDataStrPtr theStrPtr;
        if (GetValueType(theTempValue) == DataModelDataType::String) {
            theStrPtr = qt3dsdm::get<TDataStrPtr>(theTempValue);
        }
        (void)theStrPtr;
        TBase::Add();
    }
};

inline void CSlideCoreProducer::DoForceSetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                                Qt3DSDMInstanceHandle inHandle,
                                                                Qt3DSDMPropertyHandle inProperty,
                                                                const SValue &inValue)
{
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(inSlide, m_Data->m_Objects);
    SInternValue theNewValue(inValue, m_Data->GetStringTable());
    SInternValue *theCurrentValue(theSlide->GetInstancePropertyValue(inHandle, inProperty));

    const TInstancePropertyPair theKey(inHandle, inProperty);
    SlideInstancePropertyKey mergeMapKey(inSlide, inHandle, inProperty);
    TSlidePropertyMergeMap::iterator iter = m_PropertyMergeMap.find(mergeMapKey);
    if (iter != m_PropertyMergeMap.end()) {
        const SValue &theTempValue(theNewValue.GetValue());
        CSimpleSlideCore::ForceSetPropertyValue(m_Data->GetStringTable(), m_Data->m_Objects,
                                                inSlide, inHandle, inProperty, theTempValue);
        iter->second->Update(theNewValue);
        if (GetValueType(theTempValue) == DataModelDataType::String) {
            TDataStrPtr theStrPtr = qt3dsdm::get<TDataStrPtr>(theTempValue);
            QT3DSDM_DEBUG_LOG(m_Data->GetStringTable().GetNarrowStr(theStrPtr->GetData()));
        }
        return; // don't signal
    } else {
        TSlidePropertyMergeMapEntry theEntry;
        if (theCurrentValue) {
            theEntry =
                CreateHashMapSwapTransaction(__FILE__, __LINE__, m_Consumer, theKey,
                                             *theCurrentValue, theNewValue, theSlide->m_Properties);
        } else {
            if (m_Consumer) {
                std::shared_ptr<HashMapDataValueInsertTransaction> theTransaction =
                    std::make_shared<HashMapDataValueInsertTransaction>(
                        __FILE__, __LINE__, std::ref(theSlide->m_Properties),
                        std::make_pair(theKey, theNewValue));
                m_Consumer->OnTransaction(theTransaction);
                theEntry = theTransaction;
            }
        }
        if (theEntry)
            m_PropertyMergeMap.insert(make_pair(mergeMapKey, theEntry));
        CSimpleSlideCore::ForceSetPropertyValue(m_Data->GetStringTable(), m_Data->m_Objects,
                                                inSlide, inHandle, inProperty,
                                                theNewValue.GetValue());
        GetSignalSender()->SendPropertyValueSet(inSlide, inHandle, inProperty,
                                                theNewValue.GetValue());
    }
}

void CSlideCoreProducer::ForceSetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                       Qt3DSDMInstanceHandle inHandle,
                                                       Qt3DSDMPropertyHandle inProperty,
                                                       const SValue &inValue)
{
    if (m_Consumer)
        DoForceSetInstancePropertyValue(inSlide, inHandle, inProperty, inValue);
    else
        m_Data->ForceSetInstancePropertyValue(inSlide, inHandle, inProperty, inValue);
}

void CSlideCoreProducer::forceSetInstancePropertyValueOnAllSlides(Qt3DSDMInstanceHandle inInstance,
                                                                  Qt3DSDMPropertyHandle inProperty,
                                                                  const SValue &inValue)
{
    m_Data->forceSetInstancePropertyValueOnAllSlides(inInstance, inProperty, inValue);
}

bool CSlideCoreProducer::GetSpecificInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                          Qt3DSDMInstanceHandle inInstance,
                                                          Qt3DSDMPropertyHandle inProperty,
                                                          SValue &outValue) const
{
    return m_Data->GetSpecificInstancePropertyValue(inSlide, inInstance, inProperty, outValue);
}

bool CSlideCoreProducer::ContainsProperty(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty) const
{
    return m_Data->ContainsProperty(inSlide, inHandle, inProperty);
}

void CSlideCoreProducer::GetSlidePropertyEntries(Qt3DSDMSlideHandle inSlide,
                                                 TSlideEntryList &outEntries) const
{
    return m_Data->GetSlidePropertyEntries(inSlide, outEntries);
}

void CSlideCoreProducer::PushPropertyValueToChildren(Qt3DSDMSlideHandle inParent,
                                                     Qt3DSDMInstanceHandle inHandle,
                                                     Qt3DSDMPropertyHandle inProperty,
                                                     const SValue &inValue)
{
    SSlide *theParent = CSimpleSlideCore::GetSlideNF(inParent, m_Data->m_Objects);
    DoForceSetInstancePropertyValue(inParent, inHandle, inProperty, inValue);
    do_all(theParent->m_Children, std::bind(&CSlideCoreProducer::DoForceSetInstancePropertyValue,
                                            this, std::placeholders::_1,
                                            inHandle, inProperty, inValue));
}

inline void ClearValueWithTransactions(TTransactionConsumerPtr inConsumer,
                                       THandleObjectMap &inObjects,
                                       ISlideCoreSignalSender *inSignalSender, int inSlide,
                                       Qt3DSDMInstanceHandle inHandle,
                                       Qt3DSDMPropertyHandle inProperty)
{
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(inSlide, inObjects);
    SInternValue *theCurrentValue(theSlide->GetInstancePropertyValue(inHandle, inProperty));
    if (theCurrentValue) {
        SValue theValue(theCurrentValue->GetValue());
        TInstancePropertyPair theKey(inHandle, inProperty);
        CreateHashMapEraseTransaction(__FILE__, __LINE__, inConsumer,
                                      std::make_pair(theKey, *theCurrentValue),
                                      theSlide->m_Properties);
        CSimpleSlideCore::ClearPropertyValue(inObjects, inSlide, inHandle, inProperty);
        inSignalSender->SendPropertyValueRemoved(inSlide, inHandle, inProperty, theValue);
    }
}

void CSlideCoreProducer::ClearChildrenPropertyValues(Qt3DSDMSlideHandle inParent,
                                                     Qt3DSDMInstanceHandle inHandle,
                                                     Qt3DSDMPropertyHandle inProperty)
{
    SSlide *theParent = CSimpleSlideCore::GetSlideNF(inParent, m_Data->m_Objects);
    do_all(theParent->m_Children,
           std::bind(ClearValueWithTransactions, m_Consumer, std::ref(m_Data->m_Objects),
                       GetSignalSender(), std::placeholders::_1, inHandle, inProperty));
}

typedef tuple<int, TSlideEntryList, TSlideEntryList> TSlideSlideEntryTuple;
typedef std::vector<TSlideSlideEntryTuple> TSlideSlideEntryTupleList;

inline void CreateVectorPreReplaceData(THandleObjectPair inPair,
                                       TSlideSlideEntryTupleList &outSlideSlideEntries,
                                       function<bool(const TSlideEntry &)> inPredicate)
{
    if (inPair.second->GetType() == CHandleObject::EHandleObjectTypeSSlide) {
        SSlide *theSlide = static_cast<SSlide *>(inPair.second.get());
        if (theSlide->HasProperty(inPredicate)) {
            outSlideSlideEntries.push_back(
                TSlideSlideEntryTuple(inPair.first, TSlideEntryList(), TSlideEntryList()));
            theSlide->DeleteSlideEntries(get<1>(outSlideSlideEntries.back()), inPredicate);
        }
    }
}

inline void RunInsert(tuple<int, TSlideEntryList, TSlideEntryList> &inTuple,
                      THandleObjectMap &inObjects, IStringTable &inStringTable)
{
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(get<0>(inTuple), inObjects);
    theSlide->InsertSlideEntries(get<1>(inTuple), inStringTable);
}

inline void RunFullInsert(std::shared_ptr<TSlideSlideEntryTupleList> inList,
                          THandleObjectMap &inObjects, IStringTable &inStringTable)
{
    for (TSlideSlideEntryTupleList::iterator theIter = inList->begin(); theIter != inList->end();
         ++theIter)
        RunInsert(*theIter, inObjects, inStringTable);
}

inline void RunFullErase(std::shared_ptr<TSlideSlideEntryTupleList> inList,
                         THandleObjectMap &inObjects)
{
    for (TSlideSlideEntryTupleList::iterator theIter = inList->begin(); theIter != inList->end();
         ++theIter) {
        SSlide *theSlide = CSimpleSlideCore::GetSlideNF(get<0>(*theIter), inObjects);
        theSlide->DeleteEntriesFromList(std::get<1>(*theIter));
    }
}

void DeleteAllSlideEntriesWithUndo(TTransactionConsumerPtr inConsumer, IStringTable &inStringTable,
                                   THandleObjectMap &inObjects,
                                   function<bool(const TSlideEntry &)> inPredicate)
{
    std::shared_ptr<TSlideSlideEntryTupleList> theEntries(new TSlideSlideEntryTupleList);
    do_all(inObjects,
           std::bind(CreateVectorPreReplaceData,
                     std::placeholders::_1, std::ref(*theEntries), inPredicate));
    if (inConsumer)
        CreateGenericTransactionWithConsumer(
            __FILE__, __LINE__, inConsumer,
            std::bind(RunFullErase, theEntries, std::ref(inObjects)),
            std::bind(RunFullInsert, theEntries, std::ref(inObjects),
                        std::ref(inStringTable)));
}

void CSlideCoreProducer::DeleteAllInstanceEntries(Qt3DSDMInstanceHandle inInstance)
{
    DeleteAllSlideEntriesWithUndo(
        m_Consumer, GetStringTable(), m_Data->m_Objects,
        std::bind(CSimpleSlideCore::SlideEntryInstanceMatches, std::placeholders::_1, inInstance));
}
void CSlideCoreProducer::DeleteAllPropertyEntries(Qt3DSDMPropertyHandle inHandle)
{
    DeleteAllSlideEntriesWithUndo(
        m_Consumer, GetStringTable(), m_Data->m_Objects,
        std::bind(CSimpleSlideCore::SlideEntryPropertyMatches, std::placeholders::_1, inHandle));
}

void CSlideCoreProducer::DeleteAllInstancePropertyEntries(const TInstanceHandleList &inInstances,
                                                          const TPropertyHandleList &inProperties)
{
    DeleteAllSlideEntriesWithUndo(m_Consumer, GetStringTable(), m_Data->m_Objects,
                                  std::bind(CSimpleSlideCore::SlideEntryInstancePropertyMatches,
                                              std::placeholders::_1, std::cref(inInstances),
                                              std::cref(inProperties)));
}

void CSlideCoreProducer::GetIntersectingProperties(Qt3DSDMSlideHandle inSlide1,
                                                   Qt3DSDMSlideHandle inSlide2,
                                                   TSlideEntryList &outEntries) const
{
    return m_Data->GetIntersectingProperties(inSlide1, inSlide2, outEntries);
}

bool InstancePropMatches(Qt3DSDMInstanceHandle instance, Qt3DSDMPropertyHandle prop,
                         const TSlideEntry &entry)
{
    return instance == get<0>(entry) && prop == get<1>(entry);
}

bool SendPropertyAddedIfNotInList(Qt3DSDMInstanceHandle instance, Qt3DSDMPropertyHandle prop,
                                  SValue & /*value*/, const TSlideEntryList &inList,
                                  Qt3DSDMSlideHandle inSource, Qt3DSDMSlideHandle inSlide,
                                  ISlideCoreSignalSender * /*inSignalSender*/)
{
    if (find_if<TSlideEntryList::const_iterator>(
            inList, std::bind(InstancePropMatches, instance, prop, std::placeholders::_1))
        == inList.end()) {
        return true;
    }
    return false;
}

// destination gets the properties from slide 1 that have corresponding entries in slide 2
void CSlideCoreProducer::PushIntersectingProperties(Qt3DSDMSlideHandle inSlide1,
                                                    Qt3DSDMSlideHandle inSlide2,
                                                    Qt3DSDMSlideHandle inDestination)
{
    SSlide *theDest = CSimpleSlideCore::GetSlideNF(inDestination, m_Data->m_Objects);
    TSlideEntryList theProperties;
    theDest->ToSlideEntryList(theProperties);
    m_Data->PushIntersectingProperties(inSlide1, inSlide2, inDestination);
    theDest->SetPropertyValuesIf(GetStringTable(), std::bind(SendPropertyAddedIfNotInList,
                                                             std::placeholders::_1,
                                                             std::placeholders::_2,
                                                             std::placeholders::_3,
                                                             theProperties, inSlide1,
                                                             inDestination, GetSignalSender()));
    if (m_Consumer) {
        TSlideEntryList theResult;
        theDest->ToSlideEntryList(theResult);
        m_Consumer->OnTransaction(TTransactionPtr(
            CREATE_GENERIC_TRANSACTION(std::bind(&SSlide::FromSlideEntryList, theDest, theResult,
                                                   std::ref(GetStringTable())),
                                       std::bind(&SSlide::FromSlideEntryList, theDest,
                                                   theProperties, std::ref(GetStringTable())))));
    }
}

void CSlideCoreProducer::CopyProperties(Qt3DSDMSlideHandle inSourceSlide,
                                        Qt3DSDMInstanceHandle inSourceInstance,
                                        Qt3DSDMSlideHandle inDestSlide,
                                        Qt3DSDMInstanceHandle inDestInstance)
{
    SSlide *sourceSlide = CSimpleSlideCore::GetSlideNF(inSourceSlide, m_Data->m_Objects);

    for (TSlideEntryHash::iterator theIter = sourceSlide->m_Properties.begin(),
                                   theEnd = sourceSlide->m_Properties.end();
         theIter != theEnd; ++theIter) {
        if (theIter->first.first == inSourceInstance) {
            // Set it once so it appears in the slide
            // Then call the main method that will send the events.
            DoForceSetInstancePropertyValue(inDestSlide, inDestInstance, theIter->first.second,
                                            theIter->second.GetValue());
        }
    }
}

bool CSlideCoreProducer::HandleValid(int inHandle) const
{
    return m_Data->HandleValid(inHandle);
}

bool CSlideCoreProducer::IsSlide(Qt3DSDMSlideHandle inSlide) const
{
    return m_Data->IsSlide(inSlide);
}

void CSlideCoreProducer::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    m_Consumer = inConsumer;
    m_PropertyMergeMap.clear();
}

TSignalConnectionPtr
CSlideCoreProducer::ConnectSlideCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectSlideCreated(inCallback);
}
TSignalConnectionPtr CSlideCoreProducer::ConnectBeforeSlideDeleted(
    const std::function<void(Qt3DSDMSlideHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectBeforeSlideDeleted(inCallback);
}
TSignalConnectionPtr
CSlideCoreProducer::ConnectSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectSlideDeleted(inCallback);
}
TSignalConnectionPtr CSlideCoreProducer::ConnectSlideDerived(
    const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, int)> &inCallback)
{
    return GetSignalProvider()->ConnectSlideDerived(inCallback);
}
TSignalConnectionPtr CSlideCoreProducer::ConnectInstancePropertyValueSet(
    const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                               const SValue &)> &inCallback)
{
    return GetSignalProvider()->ConnectInstancePropertyValueSet(inCallback);
}
TSignalConnectionPtr CSlideCoreProducer::ConnectInstancePropertyValueRemoved(
    const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                               const SValue &)> &inCallback)
{
    return GetSignalProvider()->ConnectInstancePropertyValueRemoved(inCallback);
}
TSignalConnectionPtr CSlideCoreProducer::ConnectSlideTimeChanged(
    const std::function<void(Qt3DSDMSlideHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectSlideTimeChanged(inCallback);
}

void CSlideCoreProducer::InitSignaller()
{
    m_SlideCoreSignaller = CreateSlideCoreSignaller();
}

ISlideCoreSignalProvider *CSlideCoreProducer::GetSignalProvider()
{
    return dynamic_cast<ISlideCoreSignalProvider *>(m_SlideCoreSignaller.get());
}
ISlideCoreSignalSender *CSlideCoreProducer::GetSignalSender()
{
    return dynamic_cast<ISlideCoreSignalSender *>(m_SlideCoreSignaller.get());
}
}
