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
#include "SimpleAnimationCore.h"
#include "Qt3DSBezierEval.h"

#ifdef _WIN32
#pragma warning(disable : 4503) // decorated name length exceeded
#endif

typedef long INT32;
typedef float FLOAT;
#define DATALOGGER_CUBICROOT 0

struct SPerfLogEvent
{
    SPerfLogEvent(int) {}
};

typedef SPerfLogEvent TPerfLogMathEvent1;

using namespace std;

namespace qt3dsdm {

Qt3DSDMAnimationHandle
CSimpleAnimationCore::CreateAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType, bool inFirstKeyframeDynamic)
{
    if (GetAnimation(inSlide, inInstance, inProperty, inIndex).Valid())
        throw AnimationExists(L"");

    int nextId = GetNextId();
    Qt3DSDMAnimationHandle retval = CreateAnimationWithHandle(
        nextId, inSlide, inInstance, inProperty, inIndex, inAnimationType, inFirstKeyframeDynamic);
    AddAnimationToLookupCache(retval);
    return retval;
}

void CSimpleAnimationCore::DeleteAnimation(Qt3DSDMAnimationHandle inAnimation)
{
    RemoveAnimationFromLookupCache(inAnimation);
    SAnimationTrack *theItem = GetAnimationNF(inAnimation, m_Objects);
    do_all(theItem->m_Keyframes, std::bind(EraseHandle,
                                           std::placeholders::_1, std::ref(m_Objects)));
    EraseHandle(inAnimation, m_Objects);
}

void CSimpleAnimationCore::EnsureAnimationCache() const
{
    if (m_AnimationMatchesCache.size() == 0) {
        for (THandleObjectMap::const_iterator theIter = m_Objects.begin(), theEnd = m_Objects.end();
             theIter != theEnd; ++theIter) {
            if (theIter->second->GetType() == CHandleObject::EHandleObjectTypeSAnimationTrack)
                AddAnimationToLookupCache(static_pointer_cast<SAnimationTrack>(theIter->second));
        }
    }
}

Qt3DSDMAnimationHandle CSimpleAnimationCore::GetAnimation(Qt3DSDMSlideHandle inSlide,
                                                         Qt3DSDMInstanceHandle inInstance,
                                                         Qt3DSDMPropertyHandle inProperty,
                                                         size_t inIndex) const
{
    EnsureAnimationCache();

    pair<TStateInstanceAnimationMap::const_iterator, TStateInstanceAnimationMap::const_iterator>
        theRange(m_AnimationMatchesCache.equal_range(
            make_pair(inSlide.GetHandleValue(), inInstance.GetHandleValue())));
    for (TStateInstanceAnimationMap::const_iterator theIter = theRange.first;
         theIter != theRange.second; ++theIter) {
        std::shared_ptr<SAnimationTrack> theItem = theIter->second;
        if (inSlide.GetHandleValue() == theItem->m_Slide
            && inInstance.GetHandleValue() == theItem->m_Instance
            && inProperty.GetHandleValue() == theItem->m_Property && inIndex == theItem->m_Index)
            return theItem->m_Handle;
    }
    return 0;
}

SAnimationInfo CSimpleAnimationCore::GetAnimationInfo(Qt3DSDMAnimationHandle inAnimation) const
{
    if (m_Objects.find(inAnimation) != m_Objects.end()) {
        const SAnimationTrack *animTrack = GetAnimationNF(inAnimation, m_Objects);
        return {animTrack->m_Slide, animTrack->m_Instance, animTrack->m_Property,
                animTrack->m_Index, animTrack->m_AnimationType, animTrack->m_FirstKeyframeDynamic,
                animTrack->m_ArtistEdited};
    }
    return {};
}

inline void AddIfAnimation(const THandleObjectPair &inPair, TAnimationHandleList &outAnimations)
{
    if (inPair.second->GetType() == CHandleObject::EHandleObjectTypeSAnimationTrack)
        outAnimations.push_back(inPair.first);
}

void CSimpleAnimationCore::GetAnimations(TAnimationHandleList &outAnimations) const
{
    outAnimations.clear();
    do_all(m_Objects, std::bind(AddIfAnimation, std::placeholders::_1, std::ref(outAnimations)));
}

void CSimpleAnimationCore::GetAnimations(TAnimationInfoList &outAnimations,
                                         Qt3DSDMSlideHandle inMaster,
                                         Qt3DSDMSlideHandle inSlide) const
{
    outAnimations.clear();
    for (THandleObjectMap::const_iterator iter = m_Objects.begin(), end = m_Objects.end();
         iter != end; ++iter) {
        if (iter->second->GetType() == CHandleObject::EHandleObjectTypeSAnimationTrack) {
            const SAnimationTrack *theItem =
                static_cast<const SAnimationTrack *>(iter->second.get());
            // If either master or slide is valid, then item slide must match.
            // If item slide matches neither, then we ignore unless neither are valid.
            bool keep = (theItem->m_Slide == inMaster || theItem->m_Slide == inSlide)
                        || (!inMaster.Valid() && !inSlide.Valid());
            if (keep) {
                outAnimations.push_back({theItem->m_Slide, theItem->m_Instance, theItem->m_Property,
                                         theItem->m_Index, theItem->m_AnimationType,
                                         theItem->m_FirstKeyframeDynamic, theItem->m_ArtistEdited});
            }
        }
    }
}

inline void AddSpecificAnimationsIf(const THandleObjectPair &inPair, Qt3DSDMSlideHandle inSlide,
                                    Qt3DSDMInstanceHandle inInstance,
                                    TAnimationHandleList &outAnimations)
{
    if (inPair.second->GetType() == CHandleObject::EHandleObjectTypeSAnimationTrack) {
        const SAnimationTrack *theTrack = static_cast<const SAnimationTrack *>(inPair.second.get());
        if (theTrack->m_Slide == inSlide && theTrack->m_Instance == inInstance)
            outAnimations.push_back(inPair.first);
    }
}

void CSimpleAnimationCore::GetSpecificInstanceAnimations(Qt3DSDMSlideHandle inSlide,
                                                         Qt3DSDMInstanceHandle inInstance,
                                                         TAnimationHandleList &outAnimations)
{
    EnsureAnimationCache();
    pair<TStateInstanceAnimationMap::const_iterator, TStateInstanceAnimationMap::const_iterator>
        theRange(m_AnimationMatchesCache.equal_range(
            make_pair(inSlide.GetHandleValue(), inInstance.GetHandleValue())));
    for (TStateInstanceAnimationMap::const_iterator iter = theRange.first; iter != theRange.second;
         ++iter)
        outAnimations.push_back(iter->second->m_Handle);
}

void CSimpleAnimationCore::SetFirstKeyframeDynamic(Qt3DSDMAnimationHandle inAnimation, bool inValue)
{
    SAnimationTrack *theItem = GetAnimationNF(inAnimation, m_Objects);
    theItem->m_FirstKeyframeDynamic = inValue;
    SetIsArtistEdited(inAnimation);
}

inline void CheckKeyframeType(EAnimationType inExpected, const TKeyframe &inKeyframe)
{
    if (inExpected != inKeyframe.getType())
        throw AnimationKeyframeTypeError(L"");
}

// keyframe manipulation
Qt3DSDMKeyframeHandle CSimpleAnimationCore::InsertKeyframe(Qt3DSDMAnimationHandle inAnimation,
                                                          const TKeyframe &inKeyframe)
{
    SAnimationTrack *theItem = GetAnimationNF(inAnimation, m_Objects);
    CheckKeyframeType(theItem->m_AnimationType, inKeyframe);
    int nextId = GetNextId();
    m_Objects.insert(
        make_pair(nextId, (THandleObjectPtr) new SKeyframe(nextId, inAnimation, inKeyframe)));
    theItem->m_Keyframes.push_back(nextId);
    theItem->m_KeyframesDirty = true;
    SetIsArtistEdited(inAnimation);
    return nextId;
}

void CSimpleAnimationCore::EraseKeyframe(Qt3DSDMKeyframeHandle inKeyframe)
{
    SKeyframe *theKeyframe = GetKeyframeNF(inKeyframe, m_Objects);
    int theAnimHandle(theKeyframe->m_Animation);
    SAnimationTrack *theItem = GetAnimationNF(theAnimHandle, m_Objects);
    EraseHandle(inKeyframe, m_Objects);
    erase_if(theItem->m_Keyframes, std::bind(equal_to<int>(),
                                             std::placeholders::_1, inKeyframe.GetHandleValue()));
    SetIsArtistEdited(theAnimHandle);
}

void CSimpleAnimationCore::DeleteAllKeyframes(Qt3DSDMAnimationHandle inAnimation)
{
    SAnimationTrack *theItem = GetAnimationNF(inAnimation, m_Objects);
    do_all(theItem->m_Keyframes, std::bind(EraseHandle,
                                           std::placeholders::_1, std::ref(m_Objects)));
    theItem->m_Keyframes.clear();
    SetIsArtistEdited(inAnimation);
}

Qt3DSDMAnimationHandle
CSimpleAnimationCore::GetAnimationForKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const
{
    const SKeyframe *theKeyframe = GetKeyframeNF(inKeyframe, m_Objects);
    return theKeyframe->m_Animation;
}

TKeyframe CSimpleAnimationCore::GetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe) const
{
    const SKeyframe *theKeyframe = GetKeyframeNF(inKeyframe, m_Objects);
    return theKeyframe->m_Keyframe;
}

void CSimpleAnimationCore::SetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData)
{
    DoSetKeyframeData(inKeyframe, inData);
    SKeyframe *theKeyframe = GetKeyframeNF(inKeyframe, m_Objects);
    SetIsArtistEdited(theKeyframe->m_Animation);
}

void CSimpleAnimationCore::DoSetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe,
                                             const TKeyframe &inData)
{
    SKeyframe *theKeyframe = GetKeyframeNF(inKeyframe, m_Objects);
    CheckKeyframeType(theKeyframe->m_Keyframe.getType(), inData);
    theKeyframe->m_Keyframe = inData;
    SAnimationTrack *theItem = GetAnimationNF(theKeyframe->m_Animation, m_Objects);
    theItem->m_KeyframesDirty = true;
}

bool KeyframeLessThan(int inLeftSide, int inRightSide, const THandleObjectMap &inObjects)
{
    const SKeyframe *theLeft = CSimpleAnimationCore::GetKeyframeNF(inLeftSide, inObjects);
    const SKeyframe *theRight = CSimpleAnimationCore::GetKeyframeNF(inRightSide, inObjects);
    return getKeyframeTime(theLeft->m_Keyframe) < getKeyframeTime(theRight->m_Keyframe);
}

void SortKeyframes(TKeyframeHandleList &inKeyframes, const THandleObjectMap &inObjects)
{
    return stable_sort(inKeyframes.begin(), inKeyframes.end(),
                       std::bind(KeyframeLessThan, std::placeholders::_1,
                                 std::placeholders::_2, std::ref(inObjects)));
}

void CheckKeyframesSorted(const SAnimationTrack *theItem, const THandleObjectMap &inObjects)
{
    if (theItem->m_KeyframesDirty) {
        SAnimationTrack *theNonConst = const_cast<SAnimationTrack *>(theItem);
        SortKeyframes(theNonConst->m_Keyframes, inObjects);
        theNonConst->m_KeyframesDirty = false;
    }
}

void CSimpleAnimationCore::GetKeyframes(Qt3DSDMAnimationHandle inAnimation,
                                        TKeyframeHandleList &outKeyframes) const
{
    const SAnimationTrack *theItem = GetAnimationNF(inAnimation, m_Objects);
    CheckKeyframesSorted(theItem, m_Objects);
    outKeyframes = theItem->m_Keyframes;
}

size_t CSimpleAnimationCore::GetKeyframeCount(Qt3DSDMAnimationHandle inAnimation) const
{
    const SAnimationTrack *theItem = GetAnimationNF(inAnimation, m_Objects);
    return theItem->m_Keyframes.size();
}

bool CSimpleAnimationCore::IsFirstKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const
{
    Qt3DSDMAnimationHandle theAnimation = GetAnimationForKeyframe(inKeyframe);
    if (theAnimation.Valid()) {
        const SAnimationTrack *theItem = GetAnimationNF(theAnimation, m_Objects);
        return theItem->m_Keyframes.size() && theItem->m_Keyframes[0] == inKeyframe;
    }
    return false;
}

bool CSimpleAnimationCore::IsLastKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const
{
    Qt3DSDMAnimationHandle theAnimation = GetAnimationForKeyframe(inKeyframe);
    if (theAnimation.Valid()) {
        const SAnimationTrack *theItem = GetAnimationNF(theAnimation, m_Objects);
        return theItem->m_Keyframes.size() && theItem->m_Keyframes[theItem->m_Keyframes.size() - 1]
                                              == inKeyframe;
    }
    return false;
}

void CSimpleAnimationCore::OffsetAnimations(Qt3DSDMSlideHandle /*inSlide*/,
                                            Qt3DSDMInstanceHandle /*inInstance*/, long /*inOffset*/)
{
    throw std::runtime_error("unimplemented");
}

void CSimpleAnimationCore::SetIsArtistEdited(Qt3DSDMAnimationHandle inAnimation, bool inEdited)
{
    if (m_Objects.find(inAnimation) == m_Objects.end()) {
        Q_ASSERT(false);
        return;
    }
    SAnimationTrack *theItem = GetAnimationNF(inAnimation, m_Objects);
    if (theItem->m_ArtistEdited != inEdited)
        theItem->m_ArtistEdited = inEdited;
}

bool CSimpleAnimationCore::IsArtistEdited(Qt3DSDMAnimationHandle inAnimation) const
{
    if (m_Objects.find(inAnimation) == m_Objects.end()) {
        return false;
    }
    const SAnimationTrack *theItem = GetAnimationNF(inAnimation, m_Objects);
    return theItem->m_ArtistEdited;
}

TKeyframe IntToKeyframe(int inKeyframe, const THandleObjectMap &inObjects)
{
    const SKeyframe *theKeyframe = CSimpleAnimationCore::GetKeyframeNF(inKeyframe, inObjects);
    return theKeyframe->m_Keyframe;
}

inline float KeyframeValue(int inKeyframe, const THandleObjectMap &inObjects)
{
    const SKeyframe *theKeyframe = CSimpleAnimationCore::GetKeyframeNF(inKeyframe, inObjects);
    return getKeyframeValue(theKeyframe->m_Keyframe);
}

inline float EvaluateLinear(long inS1, long inS2, float inV1, float inV2, long time)
{
    float amount = float(time - inS1) / (inS2 - inS1);
    return inV1 + amount * (inV2 - inV1);
}

inline float EvaluateLinearKeyframe(SLinearKeyframe &inK1, SLinearKeyframe &inK2, long time)
{
    return EvaluateLinear(inK1.m_time, inK2.m_time, inK1.m_value, inK2.m_value, time);
}

inline float DoBezierEvaluation(long time, const SBezierKeyframe &inK1,
                                const SBezierKeyframe &inK2)
{
    return Q3DStudio::EvaluateBezierKeyframe(
        time, inK1.m_time, inK1.m_value, inK1.m_OutTangentTime,
        inK1.m_OutTangentValue, inK2.m_InTangentTime, inK2.m_InTangentValue, inK2.m_time,
        inK2.m_value);
}

// Animation Evaluation.
float CSimpleAnimationCore::EvaluateAnimation(Qt3DSDMAnimationHandle animation, long time) const
{
    const SAnimationTrack *theItem = GetAnimationNF(animation, m_Objects);
    if (theItem->m_Keyframes.empty())
        return 0.0f;
    CheckKeyframesSorted(theItem, m_Objects);
    // Default to linear for now.
    SLinearKeyframe theKeyframe;
    theKeyframe.m_time = time;
    TKeyframe theSearchKey(theKeyframe);
    function<TKeyframe(int)> theIntToKeyframe(
        std::bind(IntToKeyframe, std::placeholders::_1, std::ref(m_Objects)));

    TKeyframeHandleList::const_iterator theBound =
            lower_bound(theItem->m_Keyframes.begin(), theItem->m_Keyframes.end(), theSearchKey,
                        [theIntToKeyframe](const Qt3DSDMKeyframeHandle &inLeft,
                                           const TKeyframe &inRight)
    {return getKeyframeTime(theIntToKeyframe(inLeft)) < getKeyframeTime(inRight);});

    if (theBound == theItem->m_Keyframes.end())
        return KeyframeValue(theItem->m_Keyframes.back(), m_Objects);
    if (theBound == theItem->m_Keyframes.begin())
        return KeyframeValue(*theItem->m_Keyframes.begin(), m_Objects);

    TKeyframeHandleList::const_iterator theStartIter = theBound;
    --theStartIter;

    // Both iterators must be valid at this point...
    Qt3DSDMKeyframeHandle theStart = *theStartIter;
    Qt3DSDMKeyframeHandle theFinish = *theBound;
    switch (theItem->m_AnimationType) {
    default:
        throw AnimationEvaluationError(L"");
    case EAnimationTypeLinear: {
        SLinearKeyframe k1 = get<SLinearKeyframe>(theIntToKeyframe(theStart));
        SLinearKeyframe k2 = get<SLinearKeyframe>(theIntToKeyframe(theFinish));
        return EvaluateLinearKeyframe(k1, k2, time);
    }
    case EAnimationTypeBezier: {
        SBezierKeyframe k1 = get<SBezierKeyframe>(theIntToKeyframe(theStart));
        SBezierKeyframe k2 = get<SBezierKeyframe>(theIntToKeyframe(theFinish));
        return DoBezierEvaluation(time, k1, k2);
    }
    case EAnimationTypeEaseInOut: {
        SEaseInEaseOutKeyframe k1 = get<SEaseInEaseOutKeyframe>(theIntToKeyframe(theStart));
        SEaseInEaseOutKeyframe k2 = get<SEaseInEaseOutKeyframe>(theIntToKeyframe(theFinish));

        return DoBezierEvaluation(
            time, CreateBezierKeyframeFromEaseInOut(-1, k1, k2.m_time),
            CreateBezierKeyframeFromEaseInOut(k1.m_time, k2, -1));
    }
    }
}

/**
 * Get max and min values of a segment of an animation channel between startTime and endTime. If
 * startTime or endTime is -1 the segment start/end is the starting/ending keyframe
 *
 * @param animation animation channel handle
 * @param startTime only include keyframes in the range starting at this time value
 * @param endTime only include keyframes in the range ending at this time value
 * @return <max, min> values pair
 */
std::pair<float, float> CSimpleAnimationCore::getAnimationExtrema(
                                                        Qt3DSDMAnimationHandle animation,
                                                        long startTime, long endTime) const
{
    const SAnimationTrack *track = GetAnimationNF(animation, m_Objects);
    TKeyframeHandleList keyframeHandles;
    GetKeyframes(animation, keyframeHandles);

    // If animation doesn't contain any keyframes, return some default range
    if (keyframeHandles.empty())
        return std::make_pair(0, 0);

    size_t idxFrom = 0;
    size_t idxTo = keyframeHandles.size() - 1;

    if (startTime != -1) {
        for (size_t i = 1; i < keyframeHandles.size(); ++i) {
            if (getKeyframeTime(GetKeyframeData(keyframeHandles[i])) > startTime) {
                idxFrom = i - 1;
                break;
            }
        }
    }

    if (endTime != -1) {
        for (size_t i = idxFrom; i < keyframeHandles.size(); ++i) {
            if (getKeyframeTime(GetKeyframeData(keyframeHandles[i])) > endTime) {
                idxTo = i;
                break;
            }
        }
    }

    std::pair<float, float> extrema {-FLT_MAX, FLT_MAX}; // <max, min>
    if (track->m_AnimationType == EAnimationTypeBezier) {
        for (size_t i = idxFrom + 1; i <= idxTo; ++i) {
            SBezierKeyframe kf1 = get<SBezierKeyframe>(GetKeyframeData(keyframeHandles[i - 1]));
            SBezierKeyframe kf2 = get<SBezierKeyframe>(GetKeyframeData(keyframeHandles[i]));
            std::pair<float, float> extrema_i = Q3DStudio::getBezierCurveExtrema(
                        kf1.m_time, kf1.m_value, kf1.m_OutTangentTime, kf1.m_OutTangentValue,
                        kf2.m_InTangentTime, kf2.m_InTangentValue, kf2.m_time, kf2.m_value);

            if (extrema_i.first > extrema.first)
                extrema.first = extrema_i.first;
            if (extrema_i.second < extrema.second)
                extrema.second = extrema_i.second;
        }
    } else { // Linear and EaseInOut
        for (size_t i = idxFrom; i <= idxTo; ++i) {
            float kfValue = getKeyframeValue(GetKeyframeData(keyframeHandles[i]));

            if (kfValue > extrema.first)
                extrema.first = kfValue;
            if (kfValue < extrema.second)
                extrema.second = kfValue;
        }
    }

    if (qFuzzyCompare(extrema.first, -FLT_MAX))
        extrema.first = 0;
    if (qFuzzyCompare(extrema.second, FLT_MAX))
        extrema.second = 0;

    return extrema;
}

bool CSimpleAnimationCore::KeyframeValid(Qt3DSDMKeyframeHandle inKeyframe) const
{
    return HandleObjectValid<SKeyframe>(inKeyframe, m_Objects);
}

bool CSimpleAnimationCore::AnimationValid(Qt3DSDMAnimationHandle inAnimation) const
{
    return HandleObjectValid<SAnimationTrack>(inAnimation, m_Objects);
}

Qt3DSDMAnimationHandle CSimpleAnimationCore::CreateAnimationWithHandle(
    int inHandle, Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
    Qt3DSDMPropertyHandle inProperty, size_t inIndex, EAnimationType inAnimationType,
    bool inFirstKeyframeDynamic)
{
    if (HandleValid(inHandle))
        throw HandleExists(L"");
    m_Objects.insert(make_pair(inHandle, (THandleObjectPtr) new SAnimationTrack(
                                             inHandle, inSlide, inInstance, inProperty, inIndex,
                                             inAnimationType, inFirstKeyframeDynamic, true)));
    return inHandle;
}

void CSimpleAnimationCore::AddAnimationToLookupCache(Qt3DSDMAnimationHandle inAnimation) const
{
    THandleObjectMap::const_iterator theAnim = m_Objects.find(inAnimation);
    if (theAnim != m_Objects.end())
        AddAnimationToLookupCache(static_pointer_cast<SAnimationTrack>(theAnim->second));
}
void CSimpleAnimationCore::RemoveAnimationFromLookupCache(Qt3DSDMAnimationHandle inAnimation) const
{
    THandleObjectMap::const_iterator theAnim = m_Objects.find(inAnimation);
    if (theAnim != m_Objects.end())
        RemoveAnimationFromLookupCache(static_pointer_cast<SAnimationTrack>(theAnim->second));
}

// Ensure there is only one of these in the multimap.
void CSimpleAnimationCore::AddAnimationToLookupCache(
    std::shared_ptr<SAnimationTrack> inAnimation) const
{
    Qt3DSDMSlideHandle theSlide = inAnimation->m_Slide;
    Qt3DSDMInstanceHandle theInstance = inAnimation->m_Instance;
    pair<TStateInstanceAnimationMap::iterator, TStateInstanceAnimationMap::iterator> theRange =
        m_AnimationMatchesCache.equal_range(
            make_pair(theSlide.GetHandleValue(), theInstance.GetHandleValue()));
    for (TStateInstanceAnimationMap::iterator theIter = theRange.first; theIter != theRange.second;
         ++theIter) {
        if (theIter->second == inAnimation)
            return;
    }
    m_AnimationMatchesCache.insert(
        make_pair(make_pair(theSlide.GetHandleValue(), theInstance.GetHandleValue()), inAnimation));
}

// Remove this from the multimap
void CSimpleAnimationCore::RemoveAnimationFromLookupCache(
    std::shared_ptr<SAnimationTrack> inAnimation) const
{
    TStateInstanceAnimationMap &theMap(m_AnimationMatchesCache);
    Qt3DSDMSlideHandle theSlide = inAnimation->m_Slide;
    Qt3DSDMInstanceHandle theInstance = inAnimation->m_Instance;
    pair<TStateInstanceAnimationMap::iterator, TStateInstanceAnimationMap::iterator> theRange =
        theMap.equal_range(make_pair(theSlide.GetHandleValue(), theInstance.GetHandleValue()));
    for (TStateInstanceAnimationMap::iterator theIter = theRange.first; theIter != theRange.second;
         ++theIter) {
        if (theIter->second == inAnimation) {
            theMap.erase(theIter);
            break;
        }
    }
}

//================================================================================
// UICDMAnimation.h function implementations
//================================================================================

void CopyKeyframe(Qt3DSDMAnimationHandle inAnimation, Qt3DSDMKeyframeHandle inKeyframe,
                  const IAnimationCore &inSourceAnimationCore, IAnimationCore &inDestAnimationCore)
{
    TKeyframe theData = inSourceAnimationCore.GetKeyframeData(inKeyframe);
    inDestAnimationCore.InsertKeyframe(inAnimation, theData);
}

void CopyKeyframes(const IAnimationCore &inSourceAnimationCore, IAnimationCore &inDestAnimationCore,
                   Qt3DSDMAnimationHandle inDestAnimation, const TKeyframeHandleList &inKeyframes)
{
    do_all(inKeyframes,
           std::bind(CopyKeyframe, inDestAnimation, std::placeholders::_1,
                     std::cref(inSourceAnimationCore), std::ref(inDestAnimationCore)));
}

Qt3DSDMAnimationHandle CopyAnimation(TAnimationCorePtr inAnimationCore,
                                    Qt3DSDMAnimationHandle inAnimation, Qt3DSDMSlideHandle inNewSlide,
                                    Qt3DSDMInstanceHandle inNewInstance,
                                    Qt3DSDMPropertyHandle inNewProperty, size_t inNewIndex)
{
    TKeyframeHandleList theKeyframes;
    inAnimationCore->GetKeyframes(inAnimation, theKeyframes);
    SAnimationInfo theInfo(inAnimationCore->GetAnimationInfo(inAnimation));
    Qt3DSDMAnimationHandle theAnimation =
        inAnimationCore->CreateAnimation(inNewSlide, inNewInstance, inNewProperty, inNewIndex,
                                         theInfo.m_AnimationType, theInfo.m_DynamicFirstKeyframe);
    CopyKeyframes(*inAnimationCore, *inAnimationCore, theAnimation, theKeyframes);
    return theAnimation;
}

SBezierKeyframe GetBezierKeyframeData(Qt3DSDMKeyframeHandle inKeyframe,
                                      const IAnimationCore &inAnimationCore)
{
    return get<SBezierKeyframe>(inAnimationCore.GetKeyframeData(inKeyframe));
}

SEaseInEaseOutKeyframe GetEaseInEaseOutKeyframeData(Qt3DSDMKeyframeHandle inKeyframe,
                                                    const IAnimationCore &inAnimationCore)
{
    return get<SEaseInEaseOutKeyframe>(inAnimationCore.GetKeyframeData(inKeyframe));
}

TKeyframeHandleList::iterator SafeIncrementIterator(TKeyframeHandleList::iterator inIter,
                                                    const TKeyframeHandleList::iterator &inEnd)
{
    return inIter == inEnd ? inEnd : inIter + 1;
}

} // namespace qt3dsdm
