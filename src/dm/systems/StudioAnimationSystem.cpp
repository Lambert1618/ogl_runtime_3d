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
#include "StudioAnimationSystem.h"
#include "StudioPropertySystem.h"

using namespace std;

namespace qt3dsdm {

bool AnimationFloatPairContainsAnimation(Qt3DSDMAnimationHandle inAnimation,
                                         const TAnimationFloatPair &inPair)
{
    return inAnimation == inPair.first;
}

CStudioAnimationSystem::CStudioAnimationSystem(TPropertySystemPtr inPropertySystem,
                                               TSlideSystemPtr inSlideSystem,
                                               TSlideCorePtr inSlideCore,
                                               TSlideGraphCorePtr inSlideGraphCore,
                                               TAnimationCorePtr inAnimationCore)
    : m_PropertySystem(inPropertySystem)
    , m_SlideSystem(inSlideSystem)
    , m_SlideCore(inSlideCore)
    , m_SlideGraphCore(inSlideGraphCore)
    , m_AnimationCore(inAnimationCore)
    , m_AutoKeyframe(false)
    , m_SmoothInterpolation(false)
{
    IAnimationCoreSignalProvider *theAnimationSignals =
        dynamic_cast<IAnimationCoreSignalProvider *>(m_AnimationCore.get());
    m_Connections.push_back(theAnimationSignals->ConnectAnimationDeleted(
        std::bind(&CStudioAnimationSystem::OnAnimationDeleted, this, std::placeholders::_1)));
}

void CStudioAnimationSystem::OnAnimationDeleted(Qt3DSDMAnimationHandle inAnimation)
{
    ClearTemporaryAnimationValues(inAnimation);
}

void CStudioAnimationSystem::ClearTemporaryAnimationValues()
{
    m_AnimationFloatPairs.clear();
}

void CStudioAnimationSystem::ClearTemporaryAnimationValues(Qt3DSDMAnimationHandle inAnimation)
{
    erase_if(m_AnimationFloatPairs,
             std::bind(AnimationFloatPairContainsAnimation, inAnimation, std::placeholders::_1));
}

inline bool IsAnimationInfoEqual(const SAnimationInfo &inInfo, Qt3DSDMSlideHandle inSlide,
                                 Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty)
{
    if (inInfo.m_Slide == inSlide && inInfo.m_Instance == inInstance
        && inInfo.m_Property == inProperty)
        return true;
    return false;
}
inline bool IsAnimationInfoEqual(const SAnimationInfo &inInfo, Qt3DSDMSlideHandle inSlide,
                                 Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                 size_t inIndex)
{
    if (IsAnimationInfoEqual(inInfo, inSlide, inInstance, inProperty) && inInfo.m_Index == inIndex)
        return true;
    return false;
}

inline bool ApplyIfAnimationMatches(TAnimationFloatPair inAnimationFloatPair,
                                    Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty,
                                    TAnimationCorePtr inAnimationCore, SValue &outValue)
{
    SAnimationInfo theInfo = inAnimationCore->GetAnimationInfo(inAnimationFloatPair.first);
    if (IsAnimationInfoEqual(theInfo, inSlide, inInstance, inProperty)) {
        SetAnimationValue(inAnimationFloatPair.second, theInfo.m_Index, outValue);
        return true;
    }
    return false;
}

void CStudioAnimationSystem::OverrideChannelIfAnimated(Qt3DSDMSlideHandle inSlide,
                                                       Qt3DSDMInstanceHandle inInstance,
                                                       Qt3DSDMPropertyHandle inProperty,
                                                       size_t inIndex, float inSeconds,
                                                       bool &ioAnimated, SValue &outValue) const
{
    Qt3DSDMAnimationHandle theAnimation =
        m_AnimationCore->GetAnimation(inSlide, inInstance, inProperty, inIndex);
    if (theAnimation.Valid() && m_AnimationCore->GetKeyframeCount(theAnimation)) {
        float theValue = m_AnimationCore->EvaluateAnimation(theAnimation, inSeconds);
        SetAnimationValue(theValue, inIndex, outValue);
        ioAnimated |= true;
    }
}

// Value must be *primed* first.
bool CStudioAnimationSystem::GetAnimatedInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                              Qt3DSDMInstanceHandle inInstance,
                                                              Qt3DSDMPropertyHandle inProperty,
                                                              SValue &outValue) const
{
    bool retval = false;

    size_t arity = GetVariantAnimatableArity(outValue);
    if (arity) {
        for (size_t index = 0; index < m_AnimationFloatPairs.size(); ++index)
            retval |= ApplyIfAnimationMatches(m_AnimationFloatPairs.at(index), inSlide, inInstance,
                                              inProperty, m_AnimationCore, outValue);
    }

    if (!retval) {
        bool animated = false;
        size_t numChannels = GetVariantAnimatableArity(outValue);
        if (numChannels > 0) {
            TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inInstance);
            float theSeconds = m_SlideCore->GetSlideTime(
                m_SlideGraphCore->GetGraphActiveSlide(theGraphSlidePair.first));

            do_times(numChannels, std::bind(&CStudioAnimationSystem::OverrideChannelIfAnimated,
                                            this, inSlide, inInstance, inProperty,
                                            std::placeholders::_1, theSeconds,
                                            std::ref(animated), std::ref(outValue)));
        }

        if (animated)
            retval = true;
    }

    return retval;
}

bool KeyframeNear(const TKeyframe &inKeyframe, float inSeconds)
{
    return fabs(KeyframeTime(inKeyframe) - inSeconds) < .01;
}
Qt3DSDMKeyframeHandle CreateKeyframeExplicit(Qt3DSDMAnimationHandle inAnimation, float inValue,
                                            float inSeconds, TAnimationCorePtr inAnimationCore,
                                            float inEaseIn, float inEaseOut)
{
    TKeyframeHandleList theKeyframes;
    float theValue = inValue;
    inAnimationCore->GetKeyframes(inAnimation, theKeyframes);
    function<TKeyframe(Qt3DSDMKeyframeHandle)> theConverter(
        std::bind(&IAnimationCore::GetKeyframeData, inAnimationCore, std::placeholders::_1));

   TKeyframeHandleList::iterator theFind =
           std::find_if(theKeyframes.begin(), theKeyframes.end(),
                        [theConverter, inSeconds](const Qt3DSDMKeyframeHandle &handle)
   { return KeyframeNear(theConverter(handle), inSeconds); });

    float theEaseIn = inEaseIn;
    float theEaseOut = inEaseOut;
    Qt3DSDMKeyframeHandle theKeyframe;
    if (theFind != theKeyframes.end()) {
        theKeyframe = *theFind;

    inAnimationCore->SetKeyframeData(
            theKeyframe, CreateEaseInEaseOutKeyframe(inSeconds, theValue, theEaseIn, theEaseOut));
    } else {
        theKeyframe = inAnimationCore->InsertKeyframe(
            inAnimation, CreateEaseInEaseOutKeyframe(inSeconds, theValue, theEaseIn, theEaseOut));
    }
    return theKeyframe;
}

Qt3DSDMKeyframeHandle CreateKeyframe(Qt3DSDMAnimationHandle inAnimation, const SValue &inValue,
                                    float inSeconds, TAnimationCorePtr inAnimationCore,
                                    float inEaseIn, float inEaseOut)
{
    SAnimationInfo theInfo(inAnimationCore->GetAnimationInfo(inAnimation));
    return CreateKeyframeExplicit(inAnimation, GetAnimationValue(theInfo.m_Index, inValue),
                                  inSeconds, inAnimationCore, inEaseIn, inEaseOut);
}

void MaybeAddAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                       Qt3DSDMPropertyHandle inProperty, Qt3DSDMAnimationHandle inAnimation,
                       TAnimationCorePtr inAnimationCore, TAnimationHandleList &outAnimations)
{
    SAnimationInfo theInfo(inAnimationCore->GetAnimationInfo(inAnimation));
    if (IsAnimationInfoEqual(theInfo, inSlide, inInstance, inProperty))
        outAnimations.push_back(inAnimation);
}

bool SortAnimationHandlesByIndex(Qt3DSDMAnimationHandle lhs, Qt3DSDMAnimationHandle rhs,
                                 TAnimationCorePtr inCore)
{
    return inCore->GetAnimationInfo(lhs).m_Index < inCore->GetAnimationInfo(rhs).m_Index;
}

void GetPresentAnimations(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                          Qt3DSDMPropertyHandle inProperty,
                          const TAnimationFloatPairList &inAnimationPairs,
                          TAnimationCorePtr inAnimationCore, TAnimationHandleList &outAnimations)
{
    function<void(Qt3DSDMAnimationHandle)> theOperation(
        std::bind(MaybeAddAnimation, inSlide, inInstance, inProperty,
                  std::placeholders::_1, inAnimationCore,
                  std::ref(outAnimations)));

    for (auto animation : inAnimationPairs) {
        MaybeAddAnimation(inSlide, inInstance, inProperty, animation.first, inAnimationCore,
                          std::ref(outAnimations));
    }

    if (outAnimations.empty()) {
        TAnimationHandleList theAnimationHandles;
        inAnimationCore->GetAnimations(theAnimationHandles);
        do_all(theAnimationHandles, theOperation);
    }
    std::sort(outAnimations.begin(), outAnimations.end(),
              std::bind(SortAnimationHandlesByIndex,
                        std::placeholders::_1, std::placeholders::_2, inAnimationCore));
}

void CreateAnimationAndAdd(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                           Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                           TAnimationCorePtr inAnimationCore, TAnimationHandleList &outAnimations)
{
    outAnimations.push_back(inAnimationCore->CreateAnimation(
        inSlide, inInstance, inProperty, inIndex, EAnimationTypeEaseInOut, false));
}

bool AnimationValueDiffers(Qt3DSDMAnimationHandle inAnimation, const SValue &inValue,
                           float inSeconds, TAnimationCorePtr inAnimationCore)
{
    SAnimationInfo theInfo(inAnimationCore->GetAnimationInfo(inAnimation));
    if (inAnimationCore->GetKeyframeCount(inAnimation) == 0)
        return true; // currently there is no keyframe, so return true
    float theValue = GetAnimationValue(theInfo.m_Index, inValue);
    float theAnimatedValue = inAnimationCore->EvaluateAnimation(inAnimation, inSeconds);
    return fabs(theValue - theAnimatedValue) > .001;
}

bool AnimationExistsInPresentAnimations(const TAnimationFloatPair &inAnimation,
                                        TAnimationHandleList &inPresentAnimations)
{
    return exists(inPresentAnimations,
                  std::bind(equal_to<Qt3DSDMAnimationHandle>(), inAnimation.first,
                            std::placeholders::_1));
}

void DoKeyframeProperty(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                        Qt3DSDMPropertyHandle inProperty, const SValue &inValue, float inTimeInSecs,
                        bool inDoDiffValue, TAnimationCorePtr inAnimationCore,
                        TAnimationFloatPairList &inAnimationFloatPairs, float inEaseIn,
                        float inEaseOut)
{
    size_t arity = GetVariantAnimatableArity(inValue);
    TAnimationHandleList thePresentAnimations;
    GetPresentAnimations(inSlide, inInstance, inProperty, std::cref(inAnimationFloatPairs),
                         inAnimationCore, thePresentAnimations);
    if (thePresentAnimations.empty())
        do_times(arity, std::bind(CreateAnimationAndAdd, inSlide, inInstance, inProperty,
                                          std::placeholders::_1,
                                          inAnimationCore, std::ref(thePresentAnimations)));
    if (!inDoDiffValue
        || find_if(thePresentAnimations.begin(), thePresentAnimations.end(),
                   std::bind(AnimationValueDiffers,
                             std::placeholders::_1, inValue, inTimeInSecs, inAnimationCore))
            != thePresentAnimations.end()) {
        do_all(thePresentAnimations,
               std::bind(CreateKeyframe, std::placeholders::_1,
                         std::cref(inValue), inTimeInSecs, inAnimationCore,
                         inEaseIn, inEaseOut));
        erase_if(inAnimationFloatPairs, std::bind(AnimationExistsInPresentAnimations,
                                                  std::placeholders::_1,
                                                  std::ref(thePresentAnimations)));
    }
}

void DoKeyframeProperty(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                        Qt3DSDMPropertyHandle inProperty, const SValue &inValue, bool inDoDiffValue,
                        TAnimationCorePtr inAnimationCore, TSlideGraphCorePtr inSlideGraphCore,
                        TSlideCorePtr inSlideCore, TAnimationFloatPairList &inAnimationFloatPairs,
                        float inEaseIn, float inEaseOut)
{
    TGraphSlidePair theGraphSlidePair = inSlideGraphCore->GetAssociatedGraph(inInstance);
    float theCurrentTime =
        inSlideCore->GetSlideTime(inSlideGraphCore->GetGraphActiveSlide(theGraphSlidePair.first));
    DoKeyframeProperty(inSlide, inInstance, inProperty, inValue, theCurrentTime, inDoDiffValue,
                       inAnimationCore, inAnimationFloatPairs, inEaseIn, inEaseOut);
}

void DoKeyframeProperty(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                        Qt3DSDMPropertyHandle inProperty, const SValue &inValue, bool inDoDiffValue,
                        TAnimationCorePtr inAnimationCore, TSlideGraphCorePtr inSlideGraphCore,
                        TSlideCorePtr inSlideCore, TAnimationFloatPairList &inAnimationFloatPairs,
                        bool inSmoothInterpolation)
{
    float theEaseIn = 0.0f, theEaseOut = 0.0f;
    if (inSmoothInterpolation)
        theEaseIn = theEaseOut = 100.f;
    DoKeyframeProperty(inSlide, inInstance, inProperty, inValue, inDoDiffValue, inAnimationCore,
                       inSlideGraphCore, inSlideCore, inAnimationFloatPairs, theEaseIn, theEaseOut);
}

TAnimationFloatPair CreateAnimationFloatPair(Qt3DSDMAnimationHandle inAnimation,
                                             const SValue &inValue,
                                             TAnimationCorePtr inAnimationCore)
{
    SAnimationInfo theInfo(inAnimationCore->GetAnimationInfo(inAnimation));
    float theValue = GetAnimationValue(theInfo.m_Index, inValue);
    return make_pair(inAnimation, theValue);
}

void InsertUniqueAnimationFloatPair(Qt3DSDMAnimationHandle inAnimation,
                                    TAnimationFloatPairList &inList, const SValue &inValue,
                                    TAnimationCorePtr inAnimationCore)
{
    TAnimationFloatPair thePair = CreateAnimationFloatPair(inAnimation, inValue, inAnimationCore);
    insert_or_update(thePair, inList,
                     std::bind(AnimationFloatPairContainsAnimation, inAnimation, std::placeholders::_1));
}

bool CStudioAnimationSystem::SetAnimatedInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                              Qt3DSDMInstanceHandle inInstance,
                                                              Qt3DSDMPropertyHandle inProperty,
                                                              const SValue &inValue)
{
    size_t arity = GetVariantAnimatableArity(inValue);
    if (arity > 0) {
        if (m_AutoKeyframe && IsPropertyAnimated(inInstance, inProperty)) // prerequisite for
                                                                          // autoset-keyframes is
                                                                          // that the property is
                                                                          // already animated.
        {
            DoKeyframeProperty(inSlide, inInstance, inProperty, inValue, true, m_AnimationCore,
                               m_SlideGraphCore, m_SlideCore, m_AnimationFloatPairs,
                               m_SmoothInterpolation);
            return true;
        }

        TAnimationHandleList thePresentAnimations;
        GetPresentAnimations(inSlide, inInstance, inProperty, std::cref(m_AnimationFloatPairs),
                             m_AnimationCore, thePresentAnimations);
        if (!thePresentAnimations.empty()) {
            TAnimationFloatPairList thePreList(m_AnimationFloatPairs);
            do_all(thePresentAnimations, std::bind(InsertUniqueAnimationFloatPair, std::placeholders::_1,
                                                     std::ref(m_AnimationFloatPairs),
                                                     std::cref(inValue), m_AnimationCore));


            if (m_Consumer && m_refreshCallback) {
                // Only create a single refresh per transaction stack
                if (((CTransactionConsumer *)m_Consumer.get())->m_TransactionList.size() == 0) {
                    CreateGenericTransactionWithConsumer(
                        __FILE__, __LINE__, m_Consumer,
                        std::bind(m_refreshCallback, inInstance),
                        std::bind(m_refreshCallback, inInstance));
                }
            }

            CreateGenericTransactionWithConsumer(
                __FILE__, __LINE__, m_Consumer,
                std::bind(assign_to<TAnimationFloatPairList>, m_AnimationFloatPairs,
                            std::ref(m_AnimationFloatPairs)),
                std::bind(assign_to<TAnimationFloatPairList>, thePreList,
                            std::ref(m_AnimationFloatPairs)));
            return true;
        }
    }
    return false;
}

void CStudioAnimationSystem::SetAutoKeyframe(bool inAutoKeyframe)
{
    m_AutoKeyframe = inAutoKeyframe;
}
bool CStudioAnimationSystem::GetAutoKeyframe() const
{
    return m_AutoKeyframe;
}

Qt3DSDMSlideHandle CStudioAnimationSystem::GetApplicableGraphAndSlide(
    Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty, const SValue &inValue)
{
    Qt3DSDMSlideHandle theApplicableSlide;
    size_t arity = GetVariantAnimatableArity(inValue);
    if (arity > 0)
        theApplicableSlide = m_SlideSystem->GetApplicableSlide(inInstance, inProperty);
    return theApplicableSlide;
}

void InsertUniqueAnimationKeyframesPair(TAnimationKeyframesPairList &ioList, SAnimationInfo &inInfo,
                                        TKeyframeList &inKeyframes)
{
    bool theFound = false;
    for (TAnimationKeyframesPairList::iterator theAnimationKeyframeIter = ioList.begin();
         theAnimationKeyframeIter < ioList.end(); ++theAnimationKeyframeIter) {
        if (IsAnimationInfoEqual(theAnimationKeyframeIter->first, inInfo.m_Slide, inInfo.m_Instance,
                                 inInfo.m_Property, inInfo.m_Index)) {
            theFound = true;
            // Overwrite the existing value
            SAnimationInfo &theInfo = theAnimationKeyframeIter->first;
            theInfo.m_AnimationType = inInfo.m_AnimationType;
            theInfo.m_DynamicFirstKeyframe = inInfo.m_DynamicFirstKeyframe;
            theAnimationKeyframeIter->second = inKeyframes;
            break;
        }
    }
    if (!theFound) {
        ioList.push_back(std::make_pair(inInfo, inKeyframes));
    }
}

void CStudioAnimationSystem::Animate(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty)
{
    SValue theValue;
    m_PropertySystem->GetInstancePropertyValue(inInstance, inProperty, theValue);
    Qt3DSDMSlideHandle theApplicableSlide =
        GetApplicableGraphAndSlide(inInstance, inProperty, theValue.toOldSkool());
    if (theApplicableSlide.Valid()) {
        // Check if previously we have set animation & keyframes
        DataModelDataType::Value theDataType = m_PropertySystem->GetDataType(inProperty);
        size_t theArity = GetDatatypeAnimatableArity(theDataType);
        bool theFound = false;
        for (size_t i = 0; i < theArity; ++i) {
            TAnimationKeyframesPairList::iterator theAnimationKeyframeIter =
                m_DeletedAnimationData.begin();
            for (; theAnimationKeyframeIter < m_DeletedAnimationData.end();
                 ++theAnimationKeyframeIter) {
                if (IsAnimationInfoEqual(theAnimationKeyframeIter->first, theApplicableSlide,
                                         inInstance, inProperty, i)) {
                    theFound = true;

                    // We use previously set animation & keyframes to create new animation &
                    // keyframe
                    SAnimationInfo &theInfo = theAnimationKeyframeIter->first;
                    Qt3DSDMAnimationHandle theAnimation = m_AnimationCore->CreateAnimation(
                        theInfo.m_Slide, theInfo.m_Instance, theInfo.m_Property, theInfo.m_Index,
                        theInfo.m_AnimationType, theInfo.m_DynamicFirstKeyframe);

                    TKeyframeList theKeyframes = theAnimationKeyframeIter->second;
                    for (TKeyframeList::const_iterator theKeyframeIter = theKeyframes.begin();
                         theKeyframeIter < theKeyframes.end(); ++theKeyframeIter)
                        m_AnimationCore->InsertKeyframe(theAnimation, *theKeyframeIter);

                    break;
                }
            }
        }

        if (!theFound)
            DoKeyframeProperty(theApplicableSlide, inInstance, inProperty, theValue.toOldSkool(),
                               true, m_AnimationCore, m_SlideGraphCore, m_SlideCore,
                               m_AnimationFloatPairs, m_SmoothInterpolation);
    }
}

void CStudioAnimationSystem::Deanimate(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty)
{
    DataModelDataType::Value theDataType = m_PropertySystem->GetDataType(inProperty);
    size_t theArity = GetDatatypeAnimatableArity(theDataType);
    for (size_t i = 0; i < theArity; ++i) {
        Qt3DSDMAnimationHandle theAnimationHandle =
            GetControllingAnimation(inInstance, inProperty, i);

        if (theAnimationHandle.Valid()) {
            // Get animation & keyframe data and save it
            SAnimationInfo theInfo(m_AnimationCore->GetAnimationInfo(theAnimationHandle));
            TKeyframeHandleList theKeyframeHandles;
            m_AnimationCore->GetKeyframes(theAnimationHandle, theKeyframeHandles);
            TKeyframeList theKeyframeList;
            for (TKeyframeHandleList::const_iterator theKeyframeIter = theKeyframeHandles.begin();
                 theKeyframeIter < theKeyframeHandles.end(); ++theKeyframeIter)
                theKeyframeList.push_back(m_AnimationCore->GetKeyframeData(*theKeyframeIter));
            InsertUniqueAnimationKeyframesPair(m_DeletedAnimationData, theInfo, theKeyframeList);

            // Delete the animation
            m_AnimationCore->DeleteAnimation(theAnimationHandle);
        }
    }
}

void CStudioAnimationSystem::KeyframeProperty(Qt3DSDMInstanceHandle inInstance,
                                              Qt3DSDMPropertyHandle inProperty, bool inDoDiffValue)
{
    SValue theValue;
    m_PropertySystem->GetInstancePropertyValue(inInstance, inProperty, theValue);
    Qt3DSDMSlideHandle theApplicableSlide =
        GetApplicableGraphAndSlide(inInstance, inProperty, theValue.toOldSkool());
    if (theApplicableSlide.Valid())
        DoKeyframeProperty(theApplicableSlide, inInstance, inProperty, theValue.toOldSkool(),
                           inDoDiffValue, m_AnimationCore, m_SlideGraphCore, m_SlideCore,
                           m_AnimationFloatPairs, m_SmoothInterpolation);
}
void CStudioAnimationSystem::SetOrCreateKeyframe(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty,
                                                 float inTimeInSeconds,
                                                 SGetOrSetKeyframeInfo *inKeyframeInfo,
                                                 size_t inNumInfos)
{
    qt3dsdm::DataModelDataType::Value thePropertyType = m_PropertySystem->GetDataType(inProperty);
    Qt3DSDMSlideHandle theApplicableSlide;
    size_t arity = GetDatatypeAnimatableArity(thePropertyType);
    if (arity) {
        theApplicableSlide = m_SlideSystem->GetApplicableSlide(inInstance, inProperty);
        if (theApplicableSlide.Valid()) {

            TAnimationHandleList thePresentAnimations;
            GetPresentAnimations(theApplicableSlide, inInstance, inProperty,
                                 std::cref(m_AnimationFloatPairs), m_AnimationCore,
                                 thePresentAnimations);
            size_t numIterations = std::min(inNumInfos, arity);
            if (thePresentAnimations.empty()) {
                for (size_t idx = 0, end = numIterations; idx < end; ++idx) {
                    CreateAnimationAndAdd(theApplicableSlide, inInstance, inProperty, idx,
                                          m_AnimationCore, thePresentAnimations);
                }
            }
            for (size_t idx = 0, end = numIterations; idx < end; ++idx) {
                CreateKeyframeExplicit(thePresentAnimations[idx], inKeyframeInfo[idx].m_Value,
                                       inTimeInSeconds, m_AnimationCore,
                                       inKeyframeInfo[idx].m_EaseIn, inKeyframeInfo[idx].m_EaseOut);
                if (inKeyframeInfo[idx].m_AnimationTrackIsDynamic)
                    m_AnimationCore->SetFirstKeyframeDynamic(thePresentAnimations[idx], true);
            }
            erase_if(m_AnimationFloatPairs, std::bind(AnimationExistsInPresentAnimations,
                                                      std::placeholders::_1,
                                                      std::ref(thePresentAnimations)));
        }
    }
}

inline bool FindMatchingAnimation(Qt3DSDMAnimationHandle inAnimation,
                                  TAnimationCorePtr inAnimationCore, size_t inIndex)
{
    return inAnimationCore->GetAnimationInfo(inAnimation).m_Index == inIndex;
}

Qt3DSDMAnimationHandle CStudioAnimationSystem::GetControllingAnimation(
    Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty, size_t inIndex) const
{
    Qt3DSDMSlideHandle theApplicableSlide =
        m_SlideSystem->GetApplicableSlide(inInstance, inProperty);
    // first check our anim float pairs
    for (size_t idx = 0, end = m_AnimationFloatPairs.size(); idx < end; ++idx) {
        const SAnimationInfo &theInfo =
            m_AnimationCore->GetAnimationInfo(m_AnimationFloatPairs[idx].first);
        if (IsAnimationInfoEqual(theInfo, theApplicableSlide, inInstance, inProperty, inIndex))
            return m_AnimationFloatPairs[idx].first;
    }
    // Use the cache lookup instead of linear search algorithm
    return m_AnimationCore->GetAnimation(theApplicableSlide, inInstance, inProperty, inIndex);
}

bool CStudioAnimationSystem::IsPropertyAnimatable(Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty) const
{
    DataModelDataType::Value dataType = m_PropertySystem->GetDataType(inProperty);
    size_t arity = GetDatatypeAnimatableArity(dataType);
    if (arity)
        return m_SlideGraphCore->GetAssociatedGraph(inInstance).first.Valid();
    return false;
}

bool CStudioAnimationSystem::IsPropertyAnimated(Qt3DSDMInstanceHandle inInstance,
                                                Qt3DSDMPropertyHandle inProperty) const
{
    DataModelDataType::Value dataType = m_PropertySystem->GetDataType(inProperty);
    size_t arity = GetDatatypeAnimatableArity(dataType);
    for (size_t i = 0; i < arity; ++i) {
        if (GetControllingAnimation(inInstance, inProperty, i).Valid())
            return true;
    }
    return false;
}

void CStudioAnimationSystem::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    m_Consumer = inConsumer;
}

void CStudioAnimationSystem::setRefreshCallback(TRefreshCallbackFunc func)
{
    m_refreshCallback = func;
}
}
