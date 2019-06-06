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
#include "StudioFullSystem.h"
#include "StudioCoreSystem.h"
#include "StudioPropertySystem.h"
#include "SlideSystem.h"
#include "StudioAnimationSystem.h"
#include "ActionSystem.h"
#include "SignalsImpl.h"
#include "SimpleDataCore.h"
#include "SimpleSlideCore.h"
#include "SimpleSlideGraphCore.h"
#include "SimpleAnimationCore.h"
#include "DataCoreProducer.h"
#include "SlideCoreProducer.h"
#include "SlideGraphCoreProducer.h"
#include "ActionCoreProducer.h"

using namespace std;

namespace qt3dsdm {

template <typename TDoTransaction, typename TUndoTransaction>
inline void NotifyConsumer(TTransactionConsumerPtr inConsumer, TDoTransaction inDoNotification,
                           TUndoTransaction inUndoNotification)
{
    if (inConsumer) {
        inConsumer->OnDoNotification(inDoNotification);
        inConsumer->OnUndoNotification(inUndoNotification);
    } else {
        inDoNotification(); // No consumer, send notification right away
    }
}

void NotifySlideCreated(TTransactionConsumerPtr &inConsumer,
                        IStudioFullSystemSignalSender *inSender, Qt3DSDMSlideHandle inSlide)
{
    NotifyConsumer(inConsumer,
                   bind(&IStudioFullSystemSignalSender::SendSlideCreated, inSender, inSlide),
                   bind(&IStudioFullSystemSignalSender::SendSlideDeleted, inSender, inSlide));
}

void NotifySlideDeleted(TTransactionConsumerPtr &inConsumer,
                        IStudioFullSystemSignalSender *inSender, Qt3DSDMSlideHandle inSlide)
{
    NotifyConsumer(inConsumer,
                   bind(&IStudioFullSystemSignalSender::SendSlideDeleted, inSender, inSlide),
                   bind(&IStudioFullSystemSignalSender::SendSlideCreated, inSender, inSlide));
}

void NotifySlideRearranged(TTransactionConsumerPtr &inConsumer,
                           IStudioFullSystemSignalSender *inSender, Qt3DSDMSlideHandle inSlide,
                           int inOldIndex, int inNewIndex)
{
    NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendSlideRearranged, inSender,
                                    inSlide, inOldIndex, inNewIndex),
                   bind(&IStudioFullSystemSignalSender::SendSlideRearranged, inSender, inSlide,
                        inNewIndex, inOldIndex));
}

void SendInstancePropertyValueChanged(TDataCorePtr inCore, IStudioFullSystemSignalSender *inSender,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty)
{
    // Ignore when the instance is not an instance (undoing an add operation may create this, as
    // transactions are run first and notifications second).
    if (inCore->IsInstance(inInstance))
        inSender->SendInstancePropertyValue(inInstance, inProperty);
}

void NotifyInstancePropertyChanged(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                                   IStudioFullSystemSignalSender *inSender,
                                   Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                   bool inIsAggregate)
{
    if (inConsumer) {
        if (inIsAggregate == false) {
            NotifyConsumer(
                inConsumer,
                bind(SendInstancePropertyValueChanged, inCore, inSender, inInstance, inProperty),
                bind(SendInstancePropertyValueChanged, inCore, inSender, inInstance, inProperty));
        }
    } else {
        SendInstancePropertyValueChanged(inCore, inSender, inInstance, inProperty);
    }
}

void RunAnimations(IStudioFullSystemSignalSender *inSender, Qt3DSDMSlideHandle inMaster,
                   Qt3DSDMSlideHandle inSlide, TAnimationCorePtr inAnimationCore,
                   TDataCorePtr inDataCore, TTransactionConsumerPtr &inConsumer)
{
    TAnimationInfoList theAnimations;
    inAnimationCore->GetAnimations(theAnimations, inMaster, inSlide);
    size_t theEnd = theAnimations.size();
    for (size_t theIndex = 0; theIndex < theEnd; ++theIndex) {
        SAnimationInfo &theInfo(theAnimations[theIndex]);
        NotifyInstancePropertyChanged(inConsumer, inDataCore, inSender, theInfo.m_Instance,
                                      theInfo.m_Property, false);
    }
}

void NotifyComponentSeconds(TTransactionConsumerPtr &inConsumer,
                            IStudioFullSystemSignalSender *inSender, Qt3DSDMSlideHandle inSlide,
                            TDataCorePtr inCore, TAnimationCorePtr inAnimationCore,
                            TStudioAnimationSystemPtr inAnimationSystem,
                            TSlideSystemPtr inSlideSystem)
{
    Qt3DSDMSlideHandle theMaster = inSlideSystem->GetMasterSlide(inSlide);
    NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendBeginComponentSeconds,
                                    inSender, theMaster),
                   bind(&IStudioFullSystemSignalSender::SendComponentSeconds, inSender, theMaster));
    RunAnimations(inSender, theMaster, inSlide, inAnimationCore, inCore, inConsumer);
    dynamic_cast<CStudioAnimationSystem *>(inAnimationSystem.get())
        ->ClearTemporaryAnimationValues();
    NotifyConsumer(
        inConsumer, bind(&IStudioFullSystemSignalSender::SendComponentSeconds, inSender, theMaster),
        bind(&IStudioFullSystemSignalSender::SendBeginComponentSeconds, inSender, theMaster));
}

void NotifyPropertyLinked(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                          IStudioFullSystemSignalSender *inSender, Qt3DSDMSlideHandle inMaster,
                          Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                          bool inAggregate)
{
    if (inAggregate == false) {
        NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendPropertyLinked,
                                        inSender, inMaster, inInstance, inProperty),
                       bind(&IStudioFullSystemSignalSender::SendPropertyUnlinked, inSender,
                            inMaster, inInstance, inProperty));
        NotifyInstancePropertyChanged(inConsumer, inCore, inSender, inInstance, inProperty,
                                      inAggregate);
    }
}

void NotifyPropertyUnlinked(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                            IStudioFullSystemSignalSender *inSender, Qt3DSDMSlideHandle inMaster,
                            Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                            bool inAggregate)
{
    if (inAggregate == false) {
        NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendPropertyUnlinked,
                                        inSender, inMaster, inInstance, inProperty),
                       bind(&IStudioFullSystemSignalSender::SendPropertyLinked, inSender, inMaster,
                            inInstance, inProperty));
        NotifyInstancePropertyChanged(inConsumer, inCore, inSender, inInstance, inProperty,
                                      inAggregate);
    }
}

void NotifyActiveSlide(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                       IStudioFullSystemSignalSender *inSender, Qt3DSDMSlideHandle inMaster,
                       int /*inIndex*/, Qt3DSDMSlideHandle inOldSlide, Qt3DSDMSlideHandle inNewSlide,
                       TAnimationCorePtr inAnimationCore, TSlideSystemPtr inSlideSystem)
{
    TInstancePropertyPairList thePropertyList;
    inSlideSystem->GetUnionOfProperties(inOldSlide, inNewSlide, thePropertyList);
    for (size_t idx = 0, end = thePropertyList.size(); idx < end; ++idx)
        NotifyInstancePropertyChanged(inConsumer, inCore, inSender, thePropertyList[idx].first,
                                      thePropertyList[idx].second, false);

    RunAnimations(inSender, inMaster, inNewSlide, inAnimationCore, inCore, inConsumer);
}

void NotifyAnimationCreated(TTransactionConsumerPtr &inConsumer,
                            IStudioFullSystemSignalSender *inSender,
                            Qt3DSDMAnimationHandle inAnimation, Qt3DSDMInstanceHandle inInstance,
                            Qt3DSDMPropertyHandle inProperty, bool inIsAggregate)
{
    if (!inIsAggregate) {
        NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendAnimationCreated,
                                        inSender, inAnimation, inInstance, inProperty),
                       bind(&IStudioFullSystemSignalSender::SendAnimationDeleted, inSender,
                            inAnimation, inInstance, inProperty));
    }
}

void NotifyAnimationDeleted(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                            IStudioFullSystemSignalSender *inSender,
                            Qt3DSDMAnimationHandle inAnimation, Qt3DSDMInstanceHandle inInstance,
                            Qt3DSDMPropertyHandle inProperty)
{
    NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendAnimationDeleted, inSender,
                                    inAnimation, inInstance, inProperty),
                   bind(&IStudioFullSystemSignalSender::SendAnimationCreated, inSender, inAnimation,
                        inInstance, inProperty));
    NotifyInstancePropertyChanged(inConsumer, inCore, inSender, inInstance, inProperty, false);
}

void NotifyAnimationChanged(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                            IStudioFullSystemSignalSender *inSender,
                            Qt3DSDMAnimationHandle inAnimation, TAnimationCorePtr inAnimationCore,
                            bool inIsAggregate)
{
    if (!inIsAggregate) {
        SAnimationInfo theInfo(inAnimationCore->GetAnimationInfo(inAnimation));
        NotifyInstancePropertyChanged(inConsumer, inCore, inSender, theInfo.m_Instance,
                                      theInfo.m_Property, inIsAggregate);
    }
}

void NotifyKeyframeInserted(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                            IStudioFullSystemSignalSender *inSender,
                            Qt3DSDMAnimationHandle inAnimation, Qt3DSDMKeyframeHandle inKeyframe,
                            TAnimationCorePtr inAnimationCore, bool inIsAggregate)
{
    if (!inIsAggregate) {
        NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendKeyframeInserted,
                                        inSender, inAnimation, inKeyframe),
                       bind(&IStudioFullSystemSignalSender::SendKeyframeErased, inSender,
                            inAnimation, inKeyframe));
        NotifyAnimationChanged(inConsumer, inCore, inSender, inAnimation, inAnimationCore, false);
    }
}

void NotifyKeyframeErased(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                          IStudioFullSystemSignalSender *inSender,
                          Qt3DSDMAnimationHandle inAnimation, Qt3DSDMKeyframeHandle inKeyframe,
                          TAnimationCorePtr inAnimationCore)
{
    NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendKeyframeErased, inSender,
                                    inAnimation, inKeyframe),
                   bind(&IStudioFullSystemSignalSender::SendKeyframeInserted, inSender, inAnimation,
                        inKeyframe));
    NotifyAnimationChanged(inConsumer, inCore, inSender, inAnimation, inAnimationCore, false);
}

void NotifyKeyframeUpdated(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                           IStudioFullSystemSignalSender *inSender, Qt3DSDMKeyframeHandle inKeyframe,
                           TAnimationCorePtr inAnimationCore)
{
    NotifyConsumer(inConsumer,
                   bind(&IStudioFullSystemSignalSender::SendKeyframeUpdated, inSender, inKeyframe),
                   bind(&IStudioFullSystemSignalSender::SendKeyframeUpdated, inSender, inKeyframe));
    Qt3DSDMAnimationHandle theAnimation(inAnimationCore->GetAnimationForKeyframe(inKeyframe));
    NotifyAnimationChanged(inConsumer, inCore, inSender, theAnimation, inAnimationCore, false);
}

void NotifyConnectFirstKeyframeDynamicSet(TTransactionConsumerPtr &inConsumer, TDataCorePtr inCore,
                                          IStudioFullSystemSignalSender *inSender,
                                          Qt3DSDMAnimationHandle inAnimation, bool inDynamic,
                                          TAnimationCorePtr inAnimationCore)
{
    NotifyConsumer(inConsumer,
                   bind(&IStudioFullSystemSignalSender::SendConnectFirstKeyframeDynamicSet,
                        inSender, inAnimation, inDynamic),
                   bind(&IStudioFullSystemSignalSender::SendConnectFirstKeyframeDynamicSet,
                        inSender, inAnimation, inDynamic));
    NotifyAnimationChanged(inConsumer, inCore, inSender, inAnimation, inAnimationCore, false);
}

inline ISlideCoreSignalProvider *GetSlideSignaller(TSlideCorePtr inSlideCore)
{
    return dynamic_cast<CSlideCoreProducer *>(inSlideCore.get());
}

void NotifyInstanceCreated(TTransactionConsumerPtr &inConsumer,
                           IStudioFullSystemSignalSender *inSender, Qt3DSDMInstanceHandle instance)
{
    NotifyConsumer(inConsumer,
                   bind(&IStudioFullSystemSignalSender::SendInstanceCreated, inSender, instance),
                   bind(&IStudioFullSystemSignalSender::SendInstanceDeleted, inSender, instance));
}

void NotifyInstanceDeleted(TTransactionConsumerPtr &inConsumer,
                           IStudioFullSystemSignalSender *inSender, Qt3DSDMInstanceHandle instance)
{
    NotifyConsumer(inConsumer,
                   bind(&IStudioFullSystemSignalSender::SendInstanceDeleted, inSender, instance),
                   bind(&IStudioFullSystemSignalSender::SendInstanceCreated, inSender, instance));
}

void NotifyActionCreated(TTransactionConsumerPtr &inConsumer,
                         IStudioFullSystemSignalSender *inSender, Qt3DSDMActionHandle inAction,
                         Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance)
{
    NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendActionCreated, inSender,
                                    inAction, inSlide, inInstance),
                   bind(&IStudioFullSystemSignalSender::SendActionDeleted, inSender, inAction,
                        inSlide, inInstance));
}

void NotifyActionDestroyed(TTransactionConsumerPtr &inConsumer,
                           IStudioFullSystemSignalSender *inSender, Qt3DSDMActionHandle inAction,
                           Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance)
{
    NotifyConsumer(inConsumer, bind(&IStudioFullSystemSignalSender::SendActionDeleted, inSender,
                                    inAction, inSlide, inInstance),
                   bind(&IStudioFullSystemSignalSender::SendActionCreated, inSender, inAction,
                        inSlide, inInstance));
}

void SendActionEvent(Qt3DSDMActionHandle inAction, TActionCorePtr inCore,
                     function<void()> inFunction)
{
    if (inCore->HandleValid(inAction))
        inFunction();
}

void NotifyTriggerObjectSet(TTransactionConsumerPtr &inConsumer,
                            IStudioFullSystemSignalSender *inSender, Qt3DSDMActionHandle inAction,
                            TActionCorePtr inCore)
{
    function<void()> theFunc(
        bind(&IStudioFullSystemSignalSender::SendTriggerObjectSet, inSender, inAction));
    NotifyConsumer(inConsumer, bind(SendActionEvent, inAction, inCore, theFunc),
                   bind(SendActionEvent, inAction, inCore, theFunc));
}

void NotifyTargetObjectSet(TTransactionConsumerPtr &inConsumer,
                           IStudioFullSystemSignalSender *inSender, Qt3DSDMActionHandle inAction,
                           TActionCorePtr inCore)
{
    function<void()> theFunc(
        bind(&IStudioFullSystemSignalSender::SendTargetObjectSet, inSender, inAction));
    NotifyConsumer(inConsumer, bind(SendActionEvent, inAction, inCore, theFunc),
                   bind(SendActionEvent, inAction, inCore, theFunc));
}

void NotifyEventSet(TTransactionConsumerPtr &inConsumer, IStudioFullSystemSignalSender *inSender,
                    Qt3DSDMActionHandle inAction, TActionCorePtr inCore)
{
    function<void()> theFunc(
        bind(&IStudioFullSystemSignalSender::SendEventSet, inSender, inAction));
    NotifyConsumer(inConsumer, bind(SendActionEvent, inAction, inCore, theFunc),
                   bind(SendActionEvent, inAction, inCore, theFunc));
}

void NotifyHandlerSet(TTransactionConsumerPtr &inConsumer, IStudioFullSystemSignalSender *inSender,
                      Qt3DSDMActionHandle inAction, TActionCorePtr inCore)
{
    function<void()> theFunc(
        bind(&IStudioFullSystemSignalSender::SendHandlerSet, inSender, inAction));
    NotifyConsumer(inConsumer, bind(SendActionEvent, inAction, inCore, theFunc),
                   bind(SendActionEvent, inAction, inCore, theFunc));
}

void NotifyHandlerArgumentValueSet(TTransactionConsumerPtr &inConsumer,
                                   IStudioFullSystemSignalSender *inSender,
                                   Qt3DSDMHandlerArgHandle inAction, TActionCorePtr inCore)
{
    function<void()> theFunc(
        bind(&IStudioFullSystemSignalSender::SendHandlerArgumentValueSet, inSender, inAction));
    Qt3DSDMActionHandle theActionHdl(inCore->GetHandlerArgumentInfo(inAction).m_Action);
    NotifyConsumer(inConsumer, bind(SendActionEvent, theActionHdl, inCore, theFunc),
                   bind(SendActionEvent, theActionHdl, inCore, theFunc));
}

void NotifyAllKeyframesErased(TTransactionConsumerPtr &inConsumer,
                              IStudioFullSystemSignalSender *inSender,
                              TAnimationCorePtr inAnimationCore, Qt3DSDMAnimationHandle inAnimation)
{
    if (inConsumer) {
        TKeyframeHandleList theKeyframes;
        inAnimationCore->GetKeyframes(inAnimation, theKeyframes);
        for (size_t idx = 0, end = theKeyframes.size(); idx < end; ++idx) {
            inConsumer->OnDoNotification(bind(&IStudioFullSystemSignalSender::SendKeyframeErased,
                                              inSender, inAnimation, theKeyframes[idx]));
            inConsumer->OnUndoNotification(
                bind(&IStudioFullSystemSignalSender::SendKeyframeInserted, inSender, inAnimation,
                     theKeyframes[idx]));
        }
    }
}

CStudioFullSystem::CStudioFullSystem(std::shared_ptr<CStudioCoreSystem> inCoreSystem,
                                     Qt3DSDMInstanceHandle inSlideInstance,
                                     Qt3DSDMPropertyHandle inComponentGuidProperty,
                                     Qt3DSDMInstanceHandle inActionInstance,
                                     Qt3DSDMPropertyHandle inActionEyeball)
    : m_CoreSystem(inCoreSystem)
    , m_SlideSystem(new SSlideSystem(m_CoreSystem->GetDataCore(), m_CoreSystem->GetSlideCore(),
                                     m_CoreSystem->GetSlideGraphCore(),
                                     m_CoreSystem->GetAnimationCore(), inSlideInstance,
                                     inComponentGuidProperty))
    , m_ActionSystem(new CActionSystem(m_CoreSystem->GetDataCore(), m_CoreSystem->GetSlideCore(),
                                       m_CoreSystem->GetSlideGraphCore(),
                                       m_CoreSystem->GetActionCore(), m_SlideSystem,
                                       inActionInstance, inActionEyeball))
    , m_AggregateOperation(false)
{
    // TODO: Too many parameters passed in to the subsystem. Just make them know about FullSystem so
    // they can get whatever they want
    CStudioAnimationSystem *theAnimationSystem = new CStudioAnimationSystem(
        m_PropertySystem, m_SlideSystem, m_CoreSystem->GetSlideCore(),
        m_CoreSystem->GetSlideGraphCore(), m_CoreSystem->GetAnimationCore());
    m_AnimationSystem = std::shared_ptr<qt3dsdm::IStudioAnimationSystem>(theAnimationSystem);

    m_PropertySystem = std::shared_ptr<qt3dsdm::IPropertySystem>(
        new CStudioPropertySystem(m_CoreSystem->GetNewMetaData(), m_CoreSystem->GetDataCore(),
                                  m_SlideSystem, m_AnimationSystem));
    theAnimationSystem->SetPropertySystem(m_PropertySystem);

    TDataCorePtr dataCore(m_CoreSystem->GetDataCore());

    static_cast<SSlideSystem *>(m_SlideSystem.get())->SetPropertySystem(m_PropertySystem);

    ISlideSystemSignalProvider *theSlideSignaller =
        dynamic_cast<SSlideSystem *>(m_SlideSystem.get())->GetSignalProvider();
    m_Signaller = CreateStudioFullSystemSignaller(theSlideSignaller);
    IStudioFullSystemSignalSender *theSystemSender =
        dynamic_cast<IStudioFullSystemSignalSender *>(m_Signaller.get());
    m_Connections.push_back(theSlideSignaller->ConnectSlideRearranged(
        bind(NotifySlideRearranged, ref(m_Consumer), theSystemSender,
             std::placeholders::_1,
             std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theSlideSignaller->ConnectPropertyLinked(
        bind(NotifyPropertyLinked, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1,
             std::placeholders::_2, std::placeholders::_3,
             std::cref(m_AggregateOperation))));
    m_Connections.push_back(theSlideSignaller->ConnectPropertyUnlinked(
        bind(NotifyPropertyUnlinked, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1,
             std::placeholders::_2, std::placeholders::_3,
             std::cref(m_AggregateOperation))));

    m_Connections.push_back(theSlideSignaller->ConnectActiveSlide(
        bind(NotifyActiveSlide, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1,
             std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
             GetAnimationCore(), GetSlideSystem())));

    IDataCoreSignalProvider *theDataSignals =
        dynamic_cast<IDataCoreSignalProvider *>(m_CoreSystem->GetDataCore().get());
    m_Connections.push_back(theDataSignals->ConnectInstanceCreated(
        bind(NotifyInstanceCreated, ref(m_Consumer), theSystemSender, std::placeholders::_1)));
    m_Connections.push_back(theDataSignals->ConnectInstanceDeleted(
        bind(NotifyInstanceDeleted, ref(m_Consumer), theSystemSender, std::placeholders::_1)));

    ISlideCoreSignalProvider *theSlideCoreSignaller =
        dynamic_cast<ISlideCoreSignalProvider *>(m_CoreSystem->GetSlideCore().get());
    m_Connections.push_back(theSlideCoreSignaller->ConnectInstancePropertyValueSet(
        bind(NotifyInstancePropertyChanged, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_2, std::placeholders::_3,
             std::cref(m_AggregateOperation))));
    m_Connections.push_back(theSlideCoreSignaller->ConnectSlideTimeChanged(
        bind(NotifyComponentSeconds, ref(m_Consumer), theSystemSender, std::placeholders::_1,
             dataCore, GetAnimationCore(), GetAnimationSystem(), GetSlideSystem())));
    m_Connections.push_back(theSlideCoreSignaller->ConnectSlideCreated(
        bind(NotifySlideCreated, ref(m_Consumer), theSystemSender, std::placeholders::_1)));
    m_Connections.push_back(theSlideCoreSignaller->ConnectSlideDeleted(
        bind(NotifySlideDeleted, ref(m_Consumer), theSystemSender, std::placeholders::_1)));

    IAnimationCoreSignalProvider *theAnimationSignals =
        dynamic_cast<IAnimationCoreSignalProvider *>(GetAnimationCore().get());
    m_Connections.push_back(theAnimationSignals->ConnectAnimationCreated(
        bind(NotifyAnimationCreated, ref(m_Consumer), theSystemSender,
             std::placeholders::_1, std::placeholders::_3, std::placeholders::_4,
             std::cref(m_AggregateOperation))));
    m_Connections.push_back(theAnimationSignals->ConnectAnimationDeleted(
        bind(NotifyAnimationDeleted, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1, std::placeholders::_3, std::placeholders::_4)));
    m_Connections.push_back(theAnimationSignals->ConnectKeyframeInserted(
        bind(NotifyKeyframeInserted, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1, std::placeholders::_2,
             GetAnimationCore(), std::cref(m_AggregateOperation))));
    m_Connections.push_back(theAnimationSignals->ConnectKeyframeErased(
        bind(NotifyKeyframeErased, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1, std::placeholders::_2,
             GetAnimationCore())));
    m_Connections.push_back(theAnimationSignals->ConnectKeyframeUpdated(
        bind(NotifyKeyframeUpdated, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1, GetAnimationCore())));
    m_Connections.push_back(theAnimationSignals->ConnectFirstKeyframeDynamicSet(
        bind(NotifyConnectFirstKeyframeDynamicSet, ref(m_Consumer), dataCore,
             theSystemSender, std::placeholders::_1,
             std::placeholders::_2, GetAnimationCore())));
    m_Connections.push_back(theAnimationSignals->ConnectBeforeAllKeyframesErased(
        bind(NotifyAllKeyframesErased, ref(m_Consumer), theSystemSender,
             GetAnimationCore(), std::placeholders::_1)));

    IInstancePropertyCoreSignalProvider *thePropertyCoreSignaller =
        dynamic_cast<CStudioPropertySystem *>(m_PropertySystem.get())
            ->GetPropertyCoreSignalProvider();
    m_Connections.push_back(thePropertyCoreSignaller->ConnectInstancePropertyValue(
        bind(NotifyInstancePropertyChanged, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1, std::placeholders::_2, std::cref(m_AggregateOperation))));

    thePropertyCoreSignaller = dynamic_cast<CStudioPropertySystem *>(m_PropertySystem.get())
                                   ->GetImmediatePropertyCoreSignalProvider();
    m_Connections.push_back(thePropertyCoreSignaller->ConnectInstancePropertyValue(
        bind(&IStudioFullSystemSignalSender::SendInstancePropertyValue, theSystemSender,
             std::placeholders::_1, std::placeholders::_2)));

    thePropertyCoreSignaller =
        dynamic_cast<IInstancePropertyCoreSignalProvider *>(m_CoreSystem->GetDataCore().get());
    m_Connections.push_back(thePropertyCoreSignaller->ConnectInstancePropertyValue(
        bind(NotifyInstancePropertyChanged, ref(m_Consumer), dataCore, theSystemSender,
             std::placeholders::_1, std::placeholders::_2, std::cref(m_AggregateOperation))));

    IActionCoreSignalProvider *theActionSignals =
        dynamic_cast<IActionCoreSignalProvider *>(m_CoreSystem->GetActionCore().get());
    m_Connections.push_back(theActionSignals->ConnectTriggerObjectSet(
        bind(NotifyTriggerObjectSet, ref(m_Consumer), theSystemSender,
             std::placeholders::_1, GetActionCore())));
    m_Connections.push_back(theActionSignals->ConnectTargetObjectSet(
        bind(NotifyTargetObjectSet, ref(m_Consumer), theSystemSender,
             std::placeholders::_1, GetActionCore())));
    m_Connections.push_back(theActionSignals->ConnectEventSet(
        bind(NotifyEventSet, ref(m_Consumer), theSystemSender,
             std::placeholders::_1, GetActionCore())));
    m_Connections.push_back(theActionSignals->ConnectHandlerSet(
        bind(NotifyHandlerSet, ref(m_Consumer), theSystemSender,
             std::placeholders::_1, GetActionCore())));
    m_Connections.push_back(theActionSignals->ConnectHandlerArgumentValueSet(bind(
        NotifyHandlerArgumentValueSet, ref(m_Consumer), theSystemSender,
        std::placeholders::_1, GetActionCore())));

    IActionSystemSignalProvider *theActionSystemSignals =
        dynamic_cast<CActionSystem *>(m_ActionSystem.get())->GetSignalProvider();
    m_Connections.push_back(theActionSystemSignals->ConnectActionCreated(
        bind(NotifyActionCreated, ref(m_Consumer), theSystemSender,
             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theActionSystemSignals->ConnectActionDeleted(
        bind(NotifyActionDestroyed, ref(m_Consumer), theSystemSender,
             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
}

CStudioFullSystem::~CStudioFullSystem()
{
}

std::shared_ptr<IPropertySystem> CStudioFullSystem::GetPropertySystem()
{
    return m_PropertySystem;
}
std::shared_ptr<ISlideSystem> CStudioFullSystem::GetSlideSystem()
{
    return m_SlideSystem;
}
std::shared_ptr<ISlideCore> CStudioFullSystem::GetSlideCore()
{
    return m_CoreSystem->GetSlideCore();
}
std::shared_ptr<IAnimationCore> CStudioFullSystem::GetAnimationCore()
{
    return m_CoreSystem->GetAnimationCore();
}
std::shared_ptr<IStudioAnimationSystem> CStudioFullSystem::GetAnimationSystem()
{
    return m_AnimationSystem;
}
std::shared_ptr<IActionCore> CStudioFullSystem::GetActionCore()
{
    return m_CoreSystem->GetActionCore();
}
std::shared_ptr<IActionSystem> CStudioFullSystem::GetActionSystem()
{
    return m_ActionSystem;
}

std::shared_ptr<IPropertySystem> CStudioFullSystem::GetPropertySystem() const
{
    return m_PropertySystem;
}
std::shared_ptr<ISlideSystem> CStudioFullSystem::GetSlideSystem() const
{
    return m_SlideSystem;
}
std::shared_ptr<ISlideCore> CStudioFullSystem::GetSlideCore() const
{
    return m_CoreSystem->GetSlideCore();
}
std::shared_ptr<IAnimationCore> CStudioFullSystem::GetAnimationCore() const
{
    return m_CoreSystem->GetAnimationCore();
}
std::shared_ptr<IStudioAnimationSystem> CStudioFullSystem::GetAnimationSystem() const
{
    return m_AnimationSystem;
}
std::shared_ptr<IActionCore> CStudioFullSystem::GetActionCore() const
{
    return m_CoreSystem->GetActionCore();
}
std::shared_ptr<IActionSystem> CStudioFullSystem::GetActionSystem() const
{
    return m_ActionSystem;
}

std::shared_ptr<CStudioCoreSystem> CStudioFullSystem::GetCoreSystem()
{
    return m_CoreSystem;
}

bool CStudioFullSystem::GetCanonicalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                          Qt3DSDMPropertyHandle inProperty,
                                                          SValue &outValue) const
{
    SValue theTemp;
    bool retval = dynamic_cast<const CStudioPropertySystem *>(m_PropertySystem.get())
                      ->GetCanonicalInstancePropertyValue(inInstance, inProperty, theTemp);
    if (retval)
        outValue = theTemp.toOldSkool();
    return retval;
}

Qt3DSDMInstanceHandle CStudioFullSystem::FindInstanceByName(Qt3DSDMPropertyHandle inNameProperty,
                                                           const TCharStr &inName) const
{
    return m_CoreSystem->FindInstanceByName(inNameProperty, inName);
}

void CStudioFullSystem::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    m_CoreSystem->SetConsumer(inConsumer);
    DoSetConsumer(inConsumer, m_AnimationSystem);
    m_Consumer = inConsumer;
}

IStudioFullSystemSignalProvider *CStudioFullSystem::GetSignalProvider()
{
    return dynamic_cast<IStudioFullSystemSignalProvider *>(m_Signaller.get());
}

IStudioFullSystemSignalSender *CStudioFullSystem::GetSignalSender()
{
    return dynamic_cast<IStudioFullSystemSignalSender *>(m_Signaller.get());
}
}
