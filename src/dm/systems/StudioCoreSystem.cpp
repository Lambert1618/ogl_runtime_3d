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
#include "StudioCoreSystem.h"
#include "SlideCoreProducer.h"
#include "SlideGraphCoreProducer.h"
#include "DataCoreProducer.h"
#include "AnimationCoreProducer.h"
#include "ActionCoreProducer.h"
#include "Qt3DSDMSignals.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMGuides.h"

using namespace std;

namespace qt3dsdm {

inline bool AnimationInstanceMatches(Qt3DSDMAnimationHandle inAnimation,
                                     TAnimationCorePtr inAnimationCore,
                                     Qt3DSDMInstanceHandle inInstance)
{
    return inInstance == inAnimationCore->GetAnimationInfo(inAnimation).m_Instance;
}

inline bool AnimationPropertyMatches(Qt3DSDMAnimationHandle inAnimation,
                                     TAnimationCorePtr inAnimationCore,
                                     Qt3DSDMPropertyHandle inProperty)
{
    return inProperty == inAnimationCore->GetAnimationInfo(inAnimation).m_Property;
}

inline bool AnimationSlideMatches(Qt3DSDMAnimationHandle inAnimation,
                                  TAnimationCorePtr inAnimationCore, Qt3DSDMSlideHandle inSlide)
{
    return inSlide == inAnimationCore->GetAnimationInfo(inAnimation).m_Slide;
}

inline bool AnimationSlideInstancePropertyMatch(Qt3DSDMAnimationHandle inAnimation,
                                                TAnimationCorePtr inAnimationCore,
                                                Qt3DSDMSlideHandle inSlide,
                                                Qt3DSDMInstanceHandle inInstance,
                                                Qt3DSDMPropertyHandle inProperty)
{
    SAnimationInfo theInfo = inAnimationCore->GetAnimationInfo(inAnimation);
    return theInfo.m_Slide == inSlide && theInfo.m_Instance == inInstance
        && theInfo.m_Property == inProperty;
}

inline bool AnimationInstancesPropertiesMatch(Qt3DSDMAnimationHandle inAnimation,
                                              TAnimationCorePtr inAnimationCore,
                                              const TInstanceHandleList &inInstances,
                                              const TPropertyHandleList &inProperties)
{
    SAnimationInfo theInfo = inAnimationCore->GetAnimationInfo(inAnimation);
    return exists(inInstances, std::bind(equal_to<int>(), theInfo.m_Instance,
                                         std::placeholders::_1))
        && exists(inProperties, std::bind(equal_to<int>(), theInfo.m_Property,
                                          std::placeholders::_1));
}

void EraseAnimationsThatMatch(TAnimationCorePtr inAnimationCore,
                              function<bool(Qt3DSDMAnimationHandle)> inPredicate)
{
    TAnimationHandleList theAnimations;
    inAnimationCore->GetAnimations(theAnimations);
    function<bool(Qt3DSDMAnimationHandle)> theComp(std::bind(
        complement<function<bool(Qt3DSDMAnimationHandle)>, Qt3DSDMAnimationHandle>, inPredicate,
                                                      std::placeholders::_1));
    TAnimationHandleList::iterator theRemovals =
        remove_if(theAnimations.begin(), theAnimations.end(), theComp);
    for_each(theAnimations.begin(), theRemovals,
             std::bind(&IAnimationCore::DeleteAnimation, inAnimationCore, std::placeholders::_1));
}

void EraseActions(TDataCorePtr inDataCore, TActionCorePtr inActionCore,
                  const TActionHandleList &inActions)
{
    for (TActionHandleList::const_iterator theActionIter = inActions.begin();
         theActionIter != inActions.end(); ++theActionIter) {
        Qt3DSDMInstanceHandle theActionInstance;
        inActionCore->DeleteAction(*theActionIter, theActionInstance);
        inDataCore->DeleteInstance(theActionInstance);
    }
}

void CascadeInstanceDelete(Qt3DSDMInstanceHandle inInstance, TDataCorePtr inDataCore,
                           TSlideCorePtr inSlideCore, TSlideGraphCorePtr inSlideGraphCore,
                           TAnimationCorePtr inAnimationCore, TActionCorePtr inActionCore)
{
    inSlideCore->DeleteAllInstanceEntries(inInstance);

    inSlideGraphCore->DissociateInstance(inInstance);

    function<bool(Qt3DSDMAnimationHandle)> thePredicate(
        std::bind(AnimationInstanceMatches, std::placeholders::_1, inAnimationCore, inInstance));
    EraseAnimationsThatMatch(inAnimationCore, thePredicate);

    TActionHandleList theActions;
    inActionCore->GetActions(inInstance, theActions);
    EraseActions(inDataCore, inActionCore, theActions);
}

void CascadePropertyRemove(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                           TSlideCorePtr inSlideCore, TSlideGraphCorePtr inSlideGraphCore,
                           TAnimationCorePtr inAnimationCore)
{
    inSlideCore->DeleteAllPropertyEntries(inProperty);
    function<bool(Qt3DSDMAnimationHandle)> thePredicate(
        std::bind(AnimationPropertyMatches, std::placeholders::_1, inAnimationCore, inProperty));
    EraseAnimationsThatMatch(inAnimationCore, thePredicate);
}

void CascadeSlideDelete(Qt3DSDMSlideHandle inSlide, TDataCorePtr inDataCore,
                        TSlideCorePtr inSlideCore, TSlideGraphCorePtr inSlideGraphCore,
                        TAnimationCorePtr inAnimationCore, TActionCorePtr inActionCore)
{
    Qt3DSDMSlideGraphHandle theGraph = inSlideGraphCore->GetSlideGraph(inSlide);
    if (theGraph.Valid()) {
        TSlideHandleList theChildren;
        inSlideCore->GetChildSlides(inSlide, theChildren);
        TInstanceHandleList instances;
        for (size_t idx = 0, end = theChildren.size(); idx < end; ++idx) {
            instances.clear();
            inSlideCore->DeleteSlide(theChildren[idx], instances);
            for (size_t instIdx = 0, instEnd = instances.size(); instIdx < instEnd; ++instIdx)
                inDataCore->DeleteInstance(instances[instIdx]);
        }
        inSlideGraphCore->DeleteSlideGraph(theGraph);
    }

    else {
        Qt3DSDMSlideHandle theMaster(inSlideCore->GetParentSlide(inSlide));
        if (theMaster.Valid()) {
            Qt3DSDMSlideGraphHandle theGraph = inSlideGraphCore->GetSlideGraph(theMaster);
            if (theGraph.Valid()) {
                /*
                tricky stuff.  The slide change was recorded in the transaction system when the
                slide was created.  But since there didn't *have* to be a slide change when the
                slide was destroyed there was no slide change recorded at that point.

                Then on the first redo, the system quietly switched the active slide to the new
                slide.  On the second redo the system crashed due to attempting to access the
                new slide after it had been deleted (as nothing switched the system back to the
                current slide).

                So the correct answer is that the cascading system needs to *always* set the
                active slide, even if it seems like it would be, at the moment, unnecessary
                because the slide getting deleted is not currently active.  This is the mirror
                of the fact that new slide *always* sets the active slide so it does make
                logical sense.*/
                Qt3DSDMSlideHandle theNewActiveSlide =
                    inSlideGraphCore->GetGraphActiveSlide(theGraph);
                if (theNewActiveSlide == inSlide) {
                    TSlideHandleList theChildren;
                    inSlideCore->GetChildSlides(theMaster, theChildren);
                    size_t idx = 0;
                    for (size_t end = theChildren.size(); idx < end; ++idx) {
                        if (theChildren[idx] != inSlide) {
                            theNewActiveSlide = theChildren[idx];
                            break;
                        }
                    }
                    if (theNewActiveSlide == inSlide)
                        theNewActiveSlide = theMaster;
                }
                inSlideGraphCore->SetGraphActiveSlide(theGraph, theNewActiveSlide);
            }
        }
    }
    EraseAnimationsThatMatch(inAnimationCore,
                             std::bind(AnimationSlideMatches,
                                       std::placeholders::_1, inAnimationCore, inSlide));

    TActionHandleList theActions;
    inActionCore->GetActions(inSlide, theActions);
    EraseActions(inDataCore, inActionCore, theActions);
}

void CascadeSlidePropertyRemove(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                Qt3DSDMPropertyHandle inProperty, TAnimationCorePtr inAnimationCore)
{
    EraseAnimationsThatMatch(inAnimationCore,
                             std::bind(AnimationSlideInstancePropertyMatch,
                                       std::placeholders::_1, inAnimationCore,
                                       inSlide, inInstance, inProperty));
}

void CascadeInstanceParentRemoved(Qt3DSDMInstanceHandle inInstance, Qt3DSDMInstanceHandle inParent,
                                  TDataCorePtr inDataCore, TSlideCorePtr inSlideCore,
                                  TAnimationCorePtr inAnimationCore)
{
    TPropertyHandleList theProperties;
    inDataCore->GetInstanceProperties(inParent, theProperties);
    TInstanceHandleList allInstances;
    inDataCore->GetInstances(allInstances);
    function<bool(Qt3DSDMInstanceHandle)> InstanceOrDerived(
        std::bind(&IDataCore::IsInstanceOrDerivedFrom, inDataCore,
                  std::placeholders::_1, inInstance));
    function<bool(Qt3DSDMInstanceHandle)> DerivedComplement(
        std::bind(complement<function<bool(Qt3DSDMInstanceHandle)>, Qt3DSDMInstanceHandle>,
                    InstanceOrDerived, std::placeholders::_1));
    TInstanceHandleList::iterator all_derived =
        remove_if(allInstances.begin(), allInstances.end(), DerivedComplement);
    TInstanceHandleList derivedInstances(allInstances.begin(), all_derived);
    inSlideCore->DeleteAllInstancePropertyEntries(derivedInstances, theProperties);
    EraseAnimationsThatMatch(
        inAnimationCore, std::bind(AnimationInstancesPropertiesMatch,
                                   std::placeholders::_1, inAnimationCore,
                                   std::cref(derivedInstances), std::cref(theProperties)));
}

void CascadeBeforeAnimationDeleted(Qt3DSDMAnimationHandle inAnimation,
                                   TAnimationCorePtr inAnimationCore, TDataCorePtr inDataCore,
                                   TSlideCorePtr inSlideCore)
{
    SAnimationInfo theInfo = inAnimationCore->GetAnimationInfo(inAnimation);
    SValue theSlideValue;
    if (inDataCore->IsInstance(theInfo.m_Instance) && inSlideCore->IsSlide(theInfo.m_Slide)
        && inSlideCore->GetSpecificInstancePropertyValue(theInfo.m_Slide, theInfo.m_Instance,
                                                         theInfo.m_Property, theSlideValue)
        && inAnimationCore->GetKeyframeCount(inAnimation)) {
        float theTime = inSlideCore->GetSlideTime(theInfo.m_Slide);
        float theValue = inAnimationCore->EvaluateAnimation(inAnimation, theTime);
        SetAnimationValue(theValue, theInfo.m_Index, theSlideValue);
        inSlideCore->ForceSetInstancePropertyValue(theInfo.m_Slide, theInfo.m_Instance,
                                                   theInfo.m_Property, theSlideValue);
    }
}

void CascadeBeforeKeyframeErased(Qt3DSDMKeyframeHandle inKeyframe, TAnimationCorePtr inAnimationCore,
                                 TDataCorePtr inDataCore, TSlideCorePtr inSlideCore)
{
    Qt3DSDMAnimationHandle theAnimation = inAnimationCore->GetAnimationForKeyframe(inKeyframe);
    if (inAnimationCore->GetKeyframeCount(theAnimation) == 1)
        CascadeBeforeAnimationDeleted(theAnimation, inAnimationCore, inDataCore, inSlideCore);
}

CStudioCoreSystem::CStudioCoreSystem(TStringTablePtr strTable)
    : m_StringTable(strTable.get() != NULL ? strTable : IStringTable::CreateStringTable())
    , m_DataCore(new CDataCoreProducer(m_StringTable))
    , m_SlideCore(new CSlideCoreProducer(m_StringTable))
    , m_SlideGraphCore(new CSlideGraphCoreProducer())
    , m_AnimationCore(new CAnimationCoreProducer())
    , m_ActionCore(new CActionCoreProducer(m_StringTable))
    , m_NewMetaData(
          IMetaData::CreateNewMetaData(dynamic_pointer_cast<CDataCoreProducer>(m_DataCore)))
    , m_GuideSystem(IGuideSystem::CreateGuideSystem())
{
    IDataCoreSignalProvider *theProvider =
        dynamic_cast<IDataCoreSignalProvider *>(m_DataCore.get());
    m_Connections.push_back(theProvider->ConnectInstanceDeleted(
        std::bind(CascadeInstanceDelete, std::placeholders::_1, m_DataCore, m_SlideCore,
                  m_SlideGraphCore, m_AnimationCore, m_ActionCore)));
    m_Connections.push_back(theProvider->ConnectPropertyRemoved(std::bind(
        CascadePropertyRemove, std::placeholders::_1,
        std::placeholders::_2, m_SlideCore, m_SlideGraphCore, m_AnimationCore)));
    m_Connections.push_back(theProvider->ConnectInstanceParentRemoved(std::bind(
        CascadeInstanceParentRemoved, std::placeholders::_1,
        std::placeholders::_2, m_DataCore, m_SlideCore, m_AnimationCore)));
    ISlideCoreSignalProvider *theSlideCoreSignalProvider =
        dynamic_cast<ISlideCoreSignalProvider *>(m_SlideCore.get());
    m_Connections.push_back(theSlideCoreSignalProvider->ConnectBeforeSlideDeleted(
        std::bind(CascadeSlideDelete, std::placeholders::_1, m_DataCore, m_SlideCore,
                  m_SlideGraphCore, m_AnimationCore, m_ActionCore)));
    m_Connections.push_back(theSlideCoreSignalProvider->ConnectInstancePropertyValueRemoved(
        std::bind(CascadeSlidePropertyRemove, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3, m_AnimationCore)));
    IAnimationCoreSignalProvider *theAnimProvider =
        dynamic_cast<IAnimationCoreSignalProvider *>(m_AnimationCore.get());
    m_Connections.push_back(theAnimProvider->ConnectBeforeAnimationDeleted(
        std::bind(CascadeBeforeAnimationDeleted, std::placeholders::_1,
                  m_AnimationCore, m_DataCore, m_SlideCore)));
    m_Connections.push_back(theAnimProvider->ConnectBeforeKeyframeErased(
        std::bind(CascadeBeforeKeyframeErased, std::placeholders::_1,
                  m_AnimationCore, m_DataCore, m_SlideCore)));
    m_Connections.push_back(theAnimProvider->ConnectBeforeAllKeyframesErased(
        std::bind(CascadeBeforeAnimationDeleted, std::placeholders::_1,
                  m_AnimationCore, m_DataCore, m_SlideCore)));
}

CStudioCoreSystem::~CStudioCoreSystem()
{
}

std::shared_ptr<IDataCore> CStudioCoreSystem::GetDataCore()
{
    return m_DataCore;
}
std::shared_ptr<ISlideCore> CStudioCoreSystem::GetSlideCore()
{
    return m_SlideCore;
}
std::shared_ptr<ISlideGraphCore> CStudioCoreSystem::GetSlideGraphCore()
{
    return m_SlideGraphCore;
}
std::shared_ptr<IAnimationCore> CStudioCoreSystem::GetAnimationCore()
{
    return m_AnimationCore;
}
std::shared_ptr<IActionCore> CStudioCoreSystem::GetActionCore()
{
    return m_ActionCore;
}
std::shared_ptr<ICustomPropCore> CStudioCoreSystem::GetCustomPropCore()
{
    return m_CustomPropCore;
}
std::shared_ptr<IMetaData> CStudioCoreSystem::GetNewMetaData()
{
    return m_NewMetaData;
}
std::shared_ptr<IGuideSystem> CStudioCoreSystem::GetGuideSystem()
{
    return m_GuideSystem;
}

std::shared_ptr<IDataCore> CStudioCoreSystem::GetTransactionlessDataCore()
{
    return dynamic_cast<CDataCoreProducer *>(m_DataCore.get())->GetTransactionlessDataCore();
}
std::shared_ptr<ISlideCore> CStudioCoreSystem::GetTransactionlessSlideCore()
{
    return dynamic_cast<CSlideCoreProducer *>(m_SlideCore.get())->GetTransactionlessSlideCore();
}
std::shared_ptr<ISlideGraphCore> CStudioCoreSystem::GetTransactionlessSlideGraphCore()
{
    return dynamic_cast<CSlideGraphCoreProducer *>(m_SlideGraphCore.get())
        ->GetTransactionlessSlideGraphCore();
}
std::shared_ptr<IAnimationCore> CStudioCoreSystem::GetTransactionlessAnimationCore()
{
    return dynamic_cast<CAnimationCoreProducer *>(m_AnimationCore.get())
        ->GetTransactionlessAnimationCore();
}
std::shared_ptr<IActionCore> CStudioCoreSystem::GetTransactionlessActionCore()
{
    return dynamic_cast<CActionCoreProducer *>(m_ActionCore.get())->GetTransactionlessActionCore();
}

inline bool InstanceSpecificNameMatches(Qt3DSDMInstanceHandle inInstance,
                                        Qt3DSDMPropertyHandle inProperty, const TCharStr &inName,
                                        const CSimpleDataCore &inData)
{
    TPropertyHandleList theProperties;
    inData.GetAggregateInstanceProperties(inInstance, theProperties);
    SValue theValue;
    if (exists(theProperties, std::bind(equal_to<int>(), inProperty, std::placeholders::_1))
        && inData.GetSpecificInstancePropertyValue(inInstance, inProperty, theValue)) {
        return inName == get<TDataStrPtr>(theValue)->GetData();
    }
    return false;
}

Qt3DSDMInstanceHandle CStudioCoreSystem::FindInstanceByName(Qt3DSDMPropertyHandle inNameProperty,
                                                           const TCharStr &inName) const
{
    TInstanceHandleList theInstances;
    m_DataCore->GetInstances(theInstances);
    TInstanceHandleList::iterator theFind = find_if<TInstanceHandleList::iterator>(
        theInstances,
        std::bind(InstanceSpecificNameMatches, std::placeholders::_1, inNameProperty, std::cref(inName),
                    std::cref(*dynamic_cast<CDataCoreProducer *>(m_DataCore.get())
                                     ->GetTransactionlessDataCore())));
    if (theFind != theInstances.end())
        return *theFind;
    return 0;
}

void CStudioCoreSystem::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    DoSetConsumer(inConsumer, m_DataCore);
    DoSetConsumer(inConsumer, m_SlideCore);
    DoSetConsumer(inConsumer, m_SlideGraphCore);
    DoSetConsumer(inConsumer, m_AnimationCore);
    DoSetConsumer(inConsumer, m_ActionCore);
    DoSetConsumer(inConsumer, m_GuideSystem);
    // In general the meta data doesn't participate in the undo/redo system except
    // in special cases.
    if (!inConsumer)
        DoSetConsumer(inConsumer, m_NewMetaData);
}
}
