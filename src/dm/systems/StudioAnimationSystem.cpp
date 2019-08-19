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

    size_t arity = getVariantAnimatableArity(outValue);
    if (arity) {
        for (size_t index = 0; index < m_AnimationFloatPairs.size(); ++index)
            retval |= ApplyIfAnimationMatches(m_AnimationFloatPairs.at(index), inSlide, inInstance,
                                              inProperty, m_AnimationCore, outValue);
    }

    if (!retval) {
        bool animated = false;
        size_t numChannels = getVariantAnimatableArity(outValue);
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
    return fabs(getKeyframeTime(inKeyframe) - inSeconds) < .01f;
}

Qt3DSDMKeyframeHandle CStudioAnimationSystem::CreateKeyframeExplicit(
                                Qt3DSDMAnimationHandle inAnimation, float inValue, float inSeconds,
                                TKeyframe keyframeData)
{
    TKeyframeHandleList theKeyframes;
    m_AnimationCore->GetKeyframes(inAnimation, theKeyframes);
    function<TKeyframe(Qt3DSDMKeyframeHandle)> theConverter(
        std::bind(&IAnimationCore::GetKeyframeData, m_AnimationCore, std::placeholders::_1));

   TKeyframeHandleList::iterator theFind =
           std::find_if(theKeyframes.begin(), theKeyframes.end(),
                        [theConverter, inSeconds](const Qt3DSDMKeyframeHandle &handle)
   { return KeyframeNear(theConverter(handle), inSeconds); });

    if (keyframeData.getType()) { // not empty (paste keyframe case)
        // offset control points time for bezier keyframes
        offsetBezier(keyframeData, inSeconds - getKeyframeTime(keyframeData));

        keyframeData = setKeyframeTime(keyframeData, inSeconds);
    } else { // empty (insert new keyframe/change value case
        EAnimationType animType = m_AnimationCore->GetAnimationInfo(inAnimation).m_AnimationType;
        switch (animType) {
        case EAnimationTypeLinear:
            keyframeData = SLinearKeyframe(inSeconds, inValue);
            break;

        case EAnimationTypeEaseInOut: {
            float easeIn = m_SmoothInterpolation ? 100.f : 0.f;
            float easeOut = m_SmoothInterpolation ? 100.f : 0.f;
            keyframeData = SEaseInEaseOutKeyframe(inSeconds, inValue, easeIn, easeOut);
        } break;

        case EAnimationTypeBezier: {
            float timeIn, valueIn, timeOut, valueOut;
            // if keyframe exists, offset control points values
            if (theFind != theKeyframes.end()) {
                TKeyframe kfData = m_AnimationCore->GetKeyframeData(*theFind);
                getBezierValues(kfData, timeIn, valueIn, timeOut, valueOut);
                float dValue = inValue - getKeyframeValue(kfData);
                valueIn += dValue;
                valueOut += dValue;
            } else {
                valueIn = inValue;
                valueOut = inValue;
                timeIn = inSeconds - .5f;
                timeOut = inSeconds + .5f;
            }
            keyframeData = SBezierKeyframe(inSeconds, inValue, timeIn, valueIn, timeOut, valueOut);
        } break;

        default:
            Q_ASSERT(false);
            break;
        }
    }
    Qt3DSDMKeyframeHandle keyframeHandle;
    if (theFind != theKeyframes.end()) {
        keyframeHandle = *theFind;
        m_AnimationCore->SetKeyframeData(keyframeHandle, keyframeData);
    } else {
        keyframeHandle = m_AnimationCore->InsertKeyframe(inAnimation, keyframeData);
    }

    return keyframeHandle;
}

Qt3DSDMKeyframeHandle CStudioAnimationSystem::CreateKeyframe(Qt3DSDMAnimationHandle inAnimation,
                                    const SValue &inValue, float inSeconds,
                                    TAnimationCorePtr inAnimationCore)
{
    SAnimationInfo theInfo(inAnimationCore->GetAnimationInfo(inAnimation));

    return CreateKeyframeExplicit(inAnimation, GetAnimationValue(theInfo.m_Index, inValue),
                                  inSeconds);
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
    for (auto animation : inAnimationPairs) {
        MaybeAddAnimation(inSlide, inInstance, inProperty, animation.first, inAnimationCore,
                          std::ref(outAnimations));
    }

    if (outAnimations.empty()) {
        function<void(Qt3DSDMAnimationHandle)> theOperation(
            std::bind(MaybeAddAnimation, inSlide, inInstance, inProperty,
                      std::placeholders::_1, inAnimationCore,
                      std::ref(outAnimations)));

        TAnimationHandleList theAnimationHandles;
        inAnimationCore->GetAnimations(theAnimationHandles);
        do_all(theAnimationHandles, theOperation);
    }
    std::sort(outAnimations.begin(), outAnimations.end(),
              std::bind(SortAnimationHandlesByIndex,
                        std::placeholders::_1, std::placeholders::_2, inAnimationCore));
}

void CreateAnimationAndAdd(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                           Qt3DSDMPropertyHandle inProperty, qt3dsdm::EAnimationType animType,
                           size_t inIndex, TAnimationCorePtr inAnimationCore,
                           TAnimationHandleList &outAnimations)
{
    outAnimations.push_back(inAnimationCore->CreateAnimation(
        inSlide, inInstance, inProperty, inIndex, animType, false));
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

void CStudioAnimationSystem::DoKeyframeProperty(Qt3DSDMSlideHandle inSlide,
                        Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                        const SValue &inValue, bool inDoDiffValue)
{
    TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inInstance);
    float inTimeInSecs = m_SlideCore->GetSlideTime(m_SlideGraphCore
                                                   ->GetGraphActiveSlide(theGraphSlidePair.first));

    size_t arity = getVariantAnimatableArity(inValue);
    TAnimationHandleList thePresentAnimations;
    GetPresentAnimations(inSlide, inInstance, inProperty, std::cref(m_AnimationFloatPairs),
                         m_AnimationCore, thePresentAnimations);
    if (thePresentAnimations.empty()) {
        // default newly created keyframes to EaseInOut
        do_times(arity, std::bind(CreateAnimationAndAdd, inSlide, inInstance, inProperty,
                                          EAnimationTypeEaseInOut, std::placeholders::_1,
                                          m_AnimationCore, std::ref(thePresentAnimations)));
    }
    if (!inDoDiffValue
        || find_if(thePresentAnimations.begin(), thePresentAnimations.end(),
                   std::bind(AnimationValueDiffers,
                             std::placeholders::_1, inValue, inTimeInSecs, m_AnimationCore))
            != thePresentAnimations.end()) {
        do_all(thePresentAnimations,
               std::bind(&CStudioAnimationSystem::CreateKeyframe, this, std::placeholders::_1,
                         std::cref(inValue), inTimeInSecs, m_AnimationCore));
        erase_if(m_AnimationFloatPairs, std::bind(AnimationExistsInPresentAnimations,
                                                  std::placeholders::_1,
                                                  std::ref(thePresentAnimations)));
    }
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
    size_t arity = getVariantAnimatableArity(inValue);
    if (arity > 0) {
        // prerequisite for autoset-keyframes is that the property is already animated.
        if (m_AutoKeyframe && IsPropertyAnimated(inInstance, inProperty)) {
            DoKeyframeProperty(inSlide, inInstance, inProperty, inValue, true);
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
    size_t arity = getVariantAnimatableArity(inValue);
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
        size_t theArity = getDatatypeAnimatableArity(theDataType);
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
                               true);
    }
}

void CStudioAnimationSystem::Deanimate(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty)
{
    DataModelDataType::Value theDataType = m_PropertySystem->GetDataType(inProperty);
    size_t theArity = getDatatypeAnimatableArity(theDataType);
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
                           inDoDiffValue);
}
void CStudioAnimationSystem::SetOrCreateKeyframe(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty,
                                                 float inTimeInSeconds,
                                                 SGetOrSetKeyframeInfo *inKeyframeInfo,
                                                 size_t inNumInfos)
{
    qt3dsdm::DataModelDataType::Value thePropertyType = m_PropertySystem->GetDataType(inProperty);
    Qt3DSDMSlideHandle theApplicableSlide;
    size_t arity = getDatatypeAnimatableArity(thePropertyType);
    if (arity) {
        theApplicableSlide = m_SlideSystem->GetApplicableSlide(inInstance, inProperty);
        if (theApplicableSlide.Valid()) {
            TAnimationHandleList thePresentAnimations;
            GetPresentAnimations(theApplicableSlide, inInstance, inProperty,
                                 std::cref(m_AnimationFloatPairs), m_AnimationCore,
                                 thePresentAnimations);
            size_t numIterations = std::min(inNumInfos, arity);
            if (thePresentAnimations.empty()) {
                qt3dsdm::EAnimationType animType = inKeyframeInfo[0].m_keyframeData.getType();
                for (size_t idx = 0, end = numIterations; idx < end; ++idx) {
                    CreateAnimationAndAdd(theApplicableSlide, inInstance, inProperty, animType, idx,
                                          m_AnimationCore, thePresentAnimations);
                }
            }
            for (size_t idx = 0, end = numIterations; idx < end; ++idx) {
                CreateKeyframeExplicit(thePresentAnimations[idx],
                                       getKeyframeValue(inKeyframeInfo[idx].m_keyframeData),
                                       inTimeInSeconds, inKeyframeInfo[idx].m_keyframeData);
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
    size_t arity = getDatatypeAnimatableArity(dataType);
    if (arity)
        return m_SlideGraphCore->GetAssociatedGraph(inInstance).first.Valid();
    return false;
}

bool CStudioAnimationSystem::IsPropertyAnimated(Qt3DSDMInstanceHandle inInstance,
                                                Qt3DSDMPropertyHandle inProperty) const
{
    DataModelDataType::Value dataType = m_PropertySystem->GetDataType(inProperty);
    size_t arity = getDatatypeAnimatableArity(dataType);
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
