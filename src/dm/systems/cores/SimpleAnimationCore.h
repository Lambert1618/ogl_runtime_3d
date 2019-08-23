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
#pragma once
#ifndef ANIMATIONCOREH
#define ANIMATIONCOREH
#include "Qt3DSDMAnimation.h"
#include "HandleSystemBase.h"
#include <unordered_map>

namespace {
struct pair_hash {
       template <class T1, class T2>
       std::size_t operator () (const std::pair<T1, T2> &p) const {
           auto h1 = std::hash<T1>{}(p.first);
           auto h2 = std::hash<T2>{}(p.second);

           return h1 ^ h2;
       }
   };
}

namespace qt3dsdm {
struct SAnimationTrack : public CHandleObject
{
    int m_Slide = 0;
    int m_Instance = 0;
    int m_Property = 0;
    EAnimationType m_AnimationType = EAnimationTypeLinear;
    size_t m_Index = 0;
    TKeyframeHandleList m_Keyframes;
    bool m_KeyframesDirty = false;
    bool m_FirstKeyframeDynamic = false;
    bool m_ArtistEdited = true;

    SAnimationTrack() = default;

    SAnimationTrack(int inHandle, Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                    Qt3DSDMPropertyHandle inProperty, size_t inIndex, EAnimationType inAnimationType,
                    bool inFirstKeyframeDynamic, bool inArtistEdited)
        : CHandleObject(inHandle)
        , m_Slide(inSlide)
        , m_Instance(inInstance)
        , m_Property(inProperty)
        , m_AnimationType(inAnimationType)
        , m_Index(inIndex)
        , m_FirstKeyframeDynamic(inFirstKeyframeDynamic)
        , m_ArtistEdited(inArtistEdited)
    {
    }

    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeSAnimationTrack;
    EHandleObjectType GetType() override { return s_Type; }
};

struct SKeyframe : public CHandleObject
{
    TKeyframe m_Keyframe;
    int m_Animation;

    SKeyframe(int inHandle, int inAnimation, const TKeyframe &inKeyframe)
        : CHandleObject(inHandle)
        , m_Keyframe(inKeyframe)
        , m_Animation(inAnimation)
    {
    }

    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeSKeyframe;
    EHandleObjectType GetType() override { return s_Type; }
};

class CAnimationCoreProducer;

class CSimpleAnimationCore : public CHandleBase, public IAnimationCore
{
    TStringTablePtr m_StringTable;
    typedef std::unordered_multimap<std::pair<int, int>,
    std::shared_ptr<SAnimationTrack>, pair_hash> TStateInstanceAnimationMap;
    // state,instance pair map to animation handle to speed up querying if a particular
    // property is animated.
    mutable TStateInstanceAnimationMap m_AnimationMatchesCache;

public: // Use
    friend class CAnimationCoreProducer;
    // We don't use the string table ptr we are constructed with
    // but the testing system needs an unified interface to creating
    // objects
    CSimpleAnimationCore() {}
    CSimpleAnimationCore(TStringTablePtr strTable)
        : m_StringTable(strTable)
    {
    }
    TStringTablePtr GetStringTablePtr() const { return m_StringTable; }

    Qt3DSDMAnimationHandle CreateAnimation(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                          EAnimationType inAnimationType,
                                          bool inFirstKeyframeDynamic) override;
    void DeleteAnimation(Qt3DSDMAnimationHandle inAnimation) override;
    Qt3DSDMAnimationHandle GetAnimation(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, size_t inIndex) const override;
    SAnimationInfo GetAnimationInfo(Qt3DSDMAnimationHandle inAnimation) const override;
    void GetAnimations(TAnimationHandleList &outAnimations) const override;
    void GetAnimations(TAnimationInfoList &outAnimations, Qt3DSDMSlideHandle inMaster,
                       Qt3DSDMSlideHandle inSlide) const override;
    void GetSpecificInstanceAnimations(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                       TAnimationHandleList &outAnimations) override;

    void SetFirstKeyframeDynamic(Qt3DSDMAnimationHandle inAnimation, bool inValue) override;

    // keyframe manipulation
    Qt3DSDMKeyframeHandle InsertKeyframe(Qt3DSDMAnimationHandle inAnimation,
                                        const TKeyframe &inKeyframe) override;
    void EraseKeyframe(Qt3DSDMKeyframeHandle inKeyframe) override;
    void DeleteAllKeyframes(Qt3DSDMAnimationHandle inAnimation) override;
    Qt3DSDMAnimationHandle GetAnimationForKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const override;
    TKeyframe GetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe) const override;
    void SetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) override;
    // Set the keyframe data, but don't set the artist edited flag.  Used for undo/redo operations
    // where the artist edited flag has handeled by a different transaction
    void DoSetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData);
    void GetKeyframes(Qt3DSDMAnimationHandle inAnimation, TKeyframeHandleList &outKeyframes) const override;
    size_t GetKeyframeCount(Qt3DSDMAnimationHandle inAnimation) const override;
    bool IsFirstKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const override;
    bool IsLastKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const override;
    // Only implemented in the producer for now.
    void OffsetAnimations(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                          long inOffset) override;

    void SetIsArtistEdited(Qt3DSDMAnimationHandle inAnimation, bool inEdited = true) override;
    bool IsArtistEdited(Qt3DSDMAnimationHandle inAnimation) const override;

    // Animation Evaluation.
    float EvaluateAnimation(Qt3DSDMAnimationHandle animation, long time) const override;

    bool KeyframeValid(Qt3DSDMKeyframeHandle inKeyframe) const override;
    bool AnimationValid(Qt3DSDMAnimationHandle inAnimation) const override;

    // Only implemented at the producer level, not at the simple core level.
    void CopyAnimations(Qt3DSDMSlideHandle /*inSourceSlide*/,
                        Qt3DSDMInstanceHandle /*inSourceInstance*/,
                        Qt3DSDMSlideHandle /*inDestSlide*/, Qt3DSDMInstanceHandle /*inDestInstance*/) override
    {
        throw AnimationNotFound(L"");
    }

    // Lookup cache management so we can find particular animations quickly.
    void ClearAnimationMatchesLookupCache() const { m_AnimationMatchesCache.clear(); }
    void AddAnimationToLookupCache(Qt3DSDMAnimationHandle inAnimation) const;
    void RemoveAnimationFromLookupCache(Qt3DSDMAnimationHandle inAnimation) const;
    void AddAnimationToLookupCache(std::shared_ptr<SAnimationTrack> inAnimation) const;
    void RemoveAnimationFromLookupCache(std::shared_ptr<SAnimationTrack> inAnimation) const;

    void EnsureAnimationCache() const;

    Qt3DSDMAnimationHandle CreateAnimationWithHandle(int inHandle, Qt3DSDMSlideHandle inSlide,
                                                    Qt3DSDMInstanceHandle inInstance,
                                                    Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                                    EAnimationType inAnimationType,
                                                    bool inFirstKeyframeDynamic);

    static SAnimationTrack *GetAnimationNF(int inHandle, THandleObjectMap &inObjects)
    {
        return const_cast<SAnimationTrack *>(
            GetAnimationNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const SAnimationTrack *GetAnimationNF(int inHandle, const THandleObjectMap &inObjects)
    {
        const SAnimationTrack *theAnimation = GetHandleObject<SAnimationTrack>(inHandle, inObjects);
        if (theAnimation)
            return theAnimation;
        throw AnimationNotFound(L"");
    }

    static SKeyframe *GetKeyframeNF(int inHandle, THandleObjectMap &inObjects)
    {
        return const_cast<SKeyframe *>(
            GetKeyframeNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const SKeyframe *GetKeyframeNF(int inHandle, const THandleObjectMap &inObjects)
    {
        const SKeyframe *theItem = GetHandleObject<SKeyframe>(inHandle, inObjects);
        if (theItem)
            return theItem;
        throw AnimationKeyframeNotFound(L"");
    }
};

typedef std::shared_ptr<CSimpleAnimationCore> TSimpleAnimationCorePtr;
}

#endif
