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
#ifndef QT3DSDM_ANIMATION_H
#define QT3DSDM_ANIMATION_H

#include "Qt3DSDMHandles.h"
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMValue.h"

namespace qt3dsdm {

struct SLinearKeyframe
{
    float m_KeyframeSeconds;
    float m_KeyframeValue;

    SLinearKeyframe() = default;
    SLinearKeyframe(float seconds, float value)
    : m_KeyframeSeconds(seconds)
    , m_KeyframeValue(value) {}
};

struct SBezierKeyframe : public SLinearKeyframe
{
    float m_InTangentTime; // time
    float m_InTangentValue; // value offset
    float m_OutTangentTime; // time offset in seconds
    float m_OutTangentValue; // value offset

    SBezierKeyframe() = default;
    SBezierKeyframe(float seconds, float value, float tangentInTime, float tangentInValue,
                    float tangentOutTime, float tangentOutValue)
    : SLinearKeyframe(seconds, value)
    , m_InTangentTime(tangentInTime)
    , m_InTangentValue(tangentInValue)
    , m_OutTangentTime(tangentOutTime)
    , m_OutTangentValue(tangentOutValue) {}
};

typedef std::vector<SBezierKeyframe> TBezierKeyframeList;

/**
 *	Ease in/out are parameters that affect the bezier evaluation.
 *	Ease in/out at 100 means that the control values are at the end points thus creating
 *	a gradual deceleration.  Ease in/out at 0 means that interpolation is linear, the
 *	control points are at 1/3 and 2/3's the distance to the next value.
 *	Ease in/out go from 0.f to 100.0f
 */
struct SEaseInEaseOutKeyframe : public SLinearKeyframe
{
    float m_EaseIn;
    float m_EaseOut;

    SEaseInEaseOutKeyframe() = default;
    SEaseInEaseOutKeyframe(float seconds, float value, float easeIn, float easeOut)
    : SLinearKeyframe(seconds, value)
    , m_EaseIn(easeIn)
    , m_EaseOut(easeOut) {}
};

} // namespace qt3dsdm

namespace qt3ds {
namespace foundation {
    // Disable calling destructor of these pod types
    template <>
    struct DestructTraits<qt3dsdm::SEaseInEaseOutKeyframe>
    {
        void destruct(qt3dsdm::SEaseInEaseOutKeyframe &) {}
    };
    template <>
    struct DestructTraits<qt3dsdm::SBezierKeyframe>
    {
        void destruct(qt3dsdm::SBezierKeyframe &) {}
    };
    template <>
    struct DestructTraits<qt3dsdm::SLinearKeyframe>
    {
        void destruct(qt3dsdm::SLinearKeyframe &) {}
    };
}
}

namespace qt3dsdm {

enum EAnimationType {
    EAnimationTypeNone = 0,
    EAnimationTypeLinear,
    EAnimationTypeEaseInOut,
    EAnimationTypeBezier,
};

template <typename TDataType>
struct SAnimationTypeTraits
{
};

template <>
struct SAnimationTypeTraits<SBezierKeyframe>
{
    EAnimationType getType() { return EAnimationTypeBezier; }
};
template <>
struct SAnimationTypeTraits<SLinearKeyframe>
{
    EAnimationType getType() { return EAnimationTypeLinear; }
};
template <>
struct SAnimationTypeTraits<SEaseInEaseOutKeyframe>
{
    EAnimationType getType() { return EAnimationTypeEaseInOut; }
};

struct SKeyframeUnionTraits
{
    typedef EAnimationType TIdType;
    enum {
        TBufferSize = sizeof(SBezierKeyframe),
    };
    static TIdType getNoDataId() { return EAnimationTypeNone; }
    template <typename TDataType>
    static TIdType getType()
    {
        return SAnimationTypeTraits<TDataType>().getType();
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case EAnimationTypeBezier:
            return inVisitor(*reinterpret_cast<SBezierKeyframe *>(inData));
        case EAnimationTypeLinear:
            return inVisitor(*reinterpret_cast<SLinearKeyframe *>(inData));
        case EAnimationTypeEaseInOut:
            return inVisitor(*reinterpret_cast<SEaseInEaseOutKeyframe *>(inData));
        default:
            QT3DS_ASSERT(false);
        case EAnimationTypeNone:
            return inVisitor();
        }
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case EAnimationTypeBezier:
            return inVisitor(*reinterpret_cast<const SBezierKeyframe *>(inData));
        case EAnimationTypeLinear:
            return inVisitor(*reinterpret_cast<const SLinearKeyframe *>(inData));
        case EAnimationTypeEaseInOut:
            return inVisitor(*reinterpret_cast<const SEaseInEaseOutKeyframe *>(inData));
        default:
            QT3DS_ASSERT(false);
        case EAnimationTypeNone:
            return inVisitor();
        }
    }
};

typedef qt3ds::foundation::
    DiscriminatedUnion<qt3ds::foundation::
                           DiscriminatedUnionGenericBase<SKeyframeUnionTraits,
                                                         SKeyframeUnionTraits::TBufferSize>,
                       SKeyframeUnionTraits::TBufferSize>
        TKeyframe;

template <>
struct Qt3DSDMGetter<TKeyframe>
{
    template <typename TRetType>
    TRetType doGet(const TKeyframe &inValue)
    {
        return inValue.getData<TRetType>();
    }
};

typedef std::vector<TKeyframe> TKeyframeList;

struct SAnimationInfo
{
    Qt3DSDMSlideHandle m_Slide = 0;
    Qt3DSDMInstanceHandle m_Instance = 0;
    Qt3DSDMPropertyHandle m_Property = 0;
    size_t m_Index = 0;
    EAnimationType m_AnimationType = EAnimationTypeLinear;
    // Use the existing value for the value of the first keyframe.
    // Not reflected in studio at this time, purely a runtime problem.
    // Defaults to false
    bool m_DynamicFirstKeyframe = false;
    bool m_ArtistEdited = true;
    SAnimationInfo() = default;
    SAnimationInfo(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                   Qt3DSDMPropertyHandle inProperty, size_t inIndex, EAnimationType inAnimationType,
                   bool inDynamicFirstKeyframe, bool inArtistEdited)
        : m_Slide(inSlide)
        , m_Instance(inInstance)
        , m_Property(inProperty)
        , m_Index(inIndex)
        , m_AnimationType(inAnimationType)
        , m_DynamicFirstKeyframe(inDynamicFirstKeyframe)
        , m_ArtistEdited(inArtistEdited)
    {
    }
};

typedef std::pair<SAnimationInfo, TKeyframeList> TAnimationKeyframesPair;
typedef std::vector<TAnimationKeyframesPair> TAnimationKeyframesPairList;
typedef std::vector<SAnimationInfo> TAnimationInfoList;

/**
 *	Pure animation core.  Not wrapped in any niceties.
 */
class IAnimationCore
{
public:
    virtual ~IAnimationCore() {}
    virtual Qt3DSDMAnimationHandle CreateAnimation(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                                  EAnimationType inAnimationType,
                                                  bool inFirstKeyframeDynamic) = 0;
    virtual void DeleteAnimation(Qt3DSDMAnimationHandle inAnimation) = 0;
    virtual Qt3DSDMAnimationHandle GetAnimation(Qt3DSDMSlideHandle inSlide,
                                               Qt3DSDMInstanceHandle inInstance,
                                               Qt3DSDMPropertyHandle inProperty,
                                               size_t inIndex) const = 0;
    virtual SAnimationInfo GetAnimationInfo(Qt3DSDMAnimationHandle inAnimation) const = 0;
    virtual void GetAnimations(TAnimationHandleList &outAnimations) const = 0;
    virtual void GetAnimations(TAnimationInfoList &outAnimations,
                               Qt3DSDMSlideHandle inMaster = Qt3DSDMSlideHandle(),
                               Qt3DSDMSlideHandle inSlide = Qt3DSDMSlideHandle()) const = 0;
    virtual void GetSpecificInstanceAnimations(Qt3DSDMSlideHandle inSlide,
                                               Qt3DSDMInstanceHandle inInstance,
                                               TAnimationHandleList &outAnimations) = 0;

    virtual void SetFirstKeyframeDynamic(Qt3DSDMAnimationHandle inAnimation, bool inValue) = 0;

    // keyframe manipulation
    virtual Qt3DSDMKeyframeHandle InsertKeyframe(Qt3DSDMAnimationHandle inAnimation,
                                                const TKeyframe &inKeyframe) = 0;
    virtual void EraseKeyframe(Qt3DSDMKeyframeHandle inKeyframe) = 0;
    virtual void DeleteAllKeyframes(Qt3DSDMAnimationHandle inAnimation) = 0;

    // All of these mutators will force the artist edited property
    // of the animation to true.
    virtual Qt3DSDMAnimationHandle
    GetAnimationForKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const = 0;
    virtual TKeyframe GetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe) const = 0;
    virtual void SetKeyframeData(Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) = 0;
    virtual void GetKeyframes(Qt3DSDMAnimationHandle inAnimation,
                              TKeyframeHandleList &outKeyframes) const = 0;
    virtual size_t GetKeyframeCount(Qt3DSDMAnimationHandle inAnimation) const = 0;
    virtual bool IsFirstKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const = 0;
    virtual bool IsLastKeyframe(Qt3DSDMKeyframeHandle inKeyframe) const = 0;

    virtual void OffsetAnimations(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                  long inOffset) = 0;

    // Direct mutators of the artist edited feature of animations
    virtual void SetIsArtistEdited(Qt3DSDMAnimationHandle inAnimation, bool inEdited = true) = 0;
    virtual bool IsArtistEdited(Qt3DSDMAnimationHandle inAnimation) const = 0;

    // Animation Evaluation.
    virtual float EvaluateAnimation(Qt3DSDMAnimationHandle inAnimation, float inSeconds) const = 0;

    virtual bool KeyframeValid(Qt3DSDMKeyframeHandle inKeyframe) const = 0;
    virtual bool AnimationValid(Qt3DSDMAnimationHandle inAnimation) const = 0;

    virtual void CopyAnimations(Qt3DSDMSlideHandle inSourceSlide,
                                Qt3DSDMInstanceHandle inSourceInstance,
                                Qt3DSDMSlideHandle inDestSlide,
                                Qt3DSDMInstanceHandle inDestInstance) = 0;
};

typedef std::shared_ptr<IAnimationCore> TAnimationCorePtr;

struct SGetOrSetKeyframeInfo
{
    qt3dsdm::TKeyframe m_keyframeData;
    bool m_AnimationTrackIsDynamic = false;

    SGetOrSetKeyframeInfo(qt3dsdm::TKeyframe &keyframeData, bool inDynamic = false)
        : m_keyframeData(keyframeData)
        , m_AnimationTrackIsDynamic(inDynamic)
    {
    }
    SGetOrSetKeyframeInfo() = default;
};

/**
 *	Interface from studio into the animation system that speaks
 *	a language near to that of studio.  Public interface.
 */
class IStudioAnimationSystem
{
public:
    virtual ~IStudioAnimationSystem() {}

    /**
     *	When auto-keyframing is on, all calls to setinstancepropertyvalue will
     *	case a keyframe to be set if the instance is a member of a slide graph.
     */
    virtual void SetAutoKeyframe(bool inAutoKeyframe) = 0;
    /**
     *	Returns true when auto keyframing is set.
     */
    virtual bool GetAutoKeyframe() const = 0;
    /**
     *	Create animation on the property on this instance. If the property was animated and had
     *keyframes previously,
     *	this function will also set the keyframes accordingly.
     */
    virtual void Animate(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) = 0;
    /**
     *	Delete animation on the property on this instance
     */
    virtual void Deanimate(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) = 0;
    /**
     *	Set a keyframe on this property.  Uses the current time of the slide graph.
     *	If the keyframe is within a given time distance of another, this function will simply change
     *	the keyframed value of the other property to match the current value of this property.
     */
    virtual void KeyframeProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                  bool inDoDiffValue) = 0;
    /**
     *	Set the value of a existing a keyframe or create a new one, on this property, at the given
     *time.
     *  If ease in or ease out are unset then the keyframe gets the default ease in or out.
     */
    virtual void SetOrCreateKeyframe(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty, float inTimeInSeconds,
                                     SGetOrSetKeyframeInfo *inKeyframeInfo, size_t inNumInfos) = 0;
    /**
     *	Return the animation that is currently controlling this property.  This function will return
     *	an invalid handle value if there is currently no animation controlling this property.
     */
    virtual Qt3DSDMAnimationHandle GetControllingAnimation(Qt3DSDMInstanceHandle inInstance,
                                                          Qt3DSDMPropertyHandle inProperty,
                                                          size_t inIndex) const = 0;

    /**
     *	Return true if the given property is animatable.  You can begin animation by calling
     *KeyframeProperty *or*
     *	by setting auto keyframe to on and setting a property value through the
     *IInstancePropertyCore system.
     */
    virtual bool IsPropertyAnimatable(Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) const = 0;

    /**
     *	Return true if the given property is animated. Currently, either 0 or ALL channels are
     *animated. Hence, checking for index = 0 suffices.
     *	And if that changes, this function can be updated accordingly.
     */
    virtual bool IsPropertyAnimated(Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) const = 0;

    /**
     *	Specify if new keyframes should be created with smooth ( ie Ease In/Out values = 100 ) or
     *linear ( Ease In/Out values = 0 )
     */
    virtual void SetInterpolation(bool inSmooth) = 0;

    /**
     *	Get an instance property value.  Will override outValue only if the slide, instance,
     *property is animated.
     */
    virtual bool GetAnimatedInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty,
                                                  SValue &outValue) const = 0;
    /**
     *	Set an instance property value.  Will return true in the cases where
     *	the property is actually animated.  May only set local values, or may auto set a keyframed
     *value.
     */
    virtual bool SetAnimatedInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty,
                                                  const SValue &inValue) = 0;

    typedef std::function<void(Qt3DSDMInstanceHandle instance)> TRefreshCallbackFunc;
    virtual void setRefreshCallback(TRefreshCallbackFunc func) = 0;
};

typedef std::shared_ptr<IStudioAnimationSystem> TStudioAnimationSystemPtr;

inline SAnimationInfo CreateAnimationInfo(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                          EAnimationType inAnimationType,
                                          bool inFirstKeyframeDynamic, bool inArtistEdited)
{
    SAnimationInfo retval(inSlide, inInstance, inProperty, inIndex, inAnimationType,
                          inFirstKeyframeDynamic, inArtistEdited);
    return retval;
}

struct KeyframeValueGetter
{
    float operator()(const SLinearKeyframe &inKeyframe) const { return inKeyframe.m_KeyframeValue; }
    float operator()(const SBezierKeyframe &inKeyframe) const { return inKeyframe.m_KeyframeValue; }
    float operator()(const SEaseInEaseOutKeyframe &inKeyframe) const
    {
        return inKeyframe.m_KeyframeValue;
    }
    float operator()()
    {
        QT3DS_ASSERT(false);
        return 0.0f;
    }
};

inline float getKeyframeValue(const TKeyframe &inKeyframe)
{
    return inKeyframe.visit<float>(KeyframeValueGetter());
}

struct KeyframeTimeGetter
{
    template <typename TKeyframeType>
    float operator()(const TKeyframeType &inValue) const
    {
        return inValue.m_KeyframeSeconds;
    }
    float operator()()
    {
        QT3DS_ASSERT(false);
        return 0.0f;
    }
};

inline float getKeyframeTime(const TKeyframe &inValue)
{
    return inValue.visit<float>(KeyframeTimeGetter());
}

struct AnimArityGetter
{
    size_t operator()(bool) const
    {
        return 1;
    }
    size_t operator()(long) const
    {
        return 1;
    }
    size_t operator()(float) const
    {
        return 1;
    }
    size_t operator()(const SFloat2 &) const
    {
        return 2;
    }
    size_t operator()(const SFloat3 &) const
    {
        return 3;
    }
    size_t operator()(const SFloat4 &) const
    {
        return 4;
    }
    template <typename TDataType>
    size_t operator()(const TDataType &) const
    {
        return 0;
    }
    size_t operator()()
    {
        QT3DS_ASSERT(false);
        return 0;
    }
};

inline size_t getVariantAnimatableArity(const SValue &inValue)
{
    return inValue.visit<size_t>(AnimArityGetter());
}

inline size_t getDatatypeAnimatableArity(DataModelDataType::Value inDataType)
{
    switch (inDataType) {
    case DataModelDataType::Long:
    case DataModelDataType::Float:
        return 1;

    case DataModelDataType::Float2:
        return 2;

    case DataModelDataType::Float3:
        return 3;

    case DataModelDataType::Float4:
        return 4;

    default:
        return 0;
    }
}

template <typename TDataType>
inline TDataType SetFloatValue(float inValue, size_t inIndex, const TDataType &inDataType)
{
    TDataType retval(inDataType);
    retval[inIndex] = inValue;
    return retval;
}

struct SAnimationApplier
{
    float m_Value;
    size_t m_Index;
    SValue operator()(const bool &) { return m_Value > 0.5f; }
    SValue operator()(const qt3ds::QT3DSI32 &) { return static_cast<qt3ds::QT3DSI32>(m_Value + .5f); }
    SValue operator()(const float &) { return m_Value; }
    SValue operator()(const SFloat2 &inValue) { return SetFloatValue(m_Value, m_Index, inValue); }
    SValue operator()(const SFloat3 &inValue) { return SetFloatValue(m_Value, m_Index, inValue); }
    SValue operator()(const SFloat4 &inValue) { return SetFloatValue(m_Value, m_Index, inValue); }
    template <typename TDataType>
    SValue operator()(const TDataType &inValue)
    {
        return inValue;
    }
    SValue operator()()
    {
        QT3DS_ASSERT(false);
        return SValue();
    }
};

inline void SetAnimationValue(float inValue, size_t inIndex, SValue &ioValue)
{
    using namespace std;
    SAnimationApplier theApplier;
    theApplier.m_Value = inValue;
    theApplier.m_Index = inIndex;
    ioValue = ioValue.visit<SValue>(theApplier);
}

template <typename TDataType>
inline float GetFloatValue(const TDataType &inValue, size_t inIndex)
{
    return inValue[inIndex];
}

struct SAnimationGetter
{
    size_t m_Index;
    float operator()(const bool &inValue) const { return inValue ? 1.f : 0.f; }
    float operator()(const long &inValue) const { return static_cast<float>(inValue); }
    float operator()(const float &inValue) const { return inValue; }
    float operator()(const SFloat2 &inValue) const { return GetFloatValue(inValue, m_Index); }
    float operator()(const SFloat3 &inValue) const { return GetFloatValue(inValue, m_Index); }
    float operator()(const SFloat4 &inValue) const { return GetFloatValue(inValue, m_Index); }
    template <typename TDataType>
    float operator()(const TDataType & /*inValue*/) const
    {
        return 0.f;
    }
    float operator()()
    {
        QT3DS_ASSERT(false);
        return 0.0f;
    }
};

inline float GetAnimationValue(size_t inIndex, const SValue &ioValue)
{
    SAnimationGetter theGetter;
    theGetter.m_Index = inIndex;
    return ioValue.visit<float>(theGetter);
}

struct KeyframeTimeSetter
{
    float m_Seconds;

    TKeyframe operator()(const SLinearKeyframe &inValue) const
    {
        return SLinearKeyframe(m_Seconds, inValue.m_KeyframeValue);
    }
    TKeyframe operator()(const SBezierKeyframe &inValue) const
    {
        return SBezierKeyframe(m_Seconds, inValue.m_KeyframeValue, inValue.m_InTangentTime,
                               inValue.m_InTangentValue, inValue.m_OutTangentTime,
                               inValue.m_OutTangentValue);
    }
    TKeyframe operator()(const SEaseInEaseOutKeyframe &inValue) const
    {
        return SEaseInEaseOutKeyframe(m_Seconds, inValue.m_KeyframeValue, inValue.m_EaseIn,
                                      inValue.m_EaseOut);
    }
    TKeyframe operator()()
    {
        QT3DS_ASSERT(false);
        return TKeyframe();
    }
};

inline TKeyframe SetKeyframeTime(const TKeyframe &inKeyframe, float inSeconds)
{
    return inKeyframe.visit<TKeyframe>(KeyframeTimeSetter{inSeconds});
}

struct KeyframeValueSetter
{
    float m_Value;

    TKeyframe operator()(const SLinearKeyframe &inValue) const
    {
        return SLinearKeyframe(inValue.m_KeyframeSeconds, m_Value);
    }
    TKeyframe operator()(const SBezierKeyframe &inValue) const
    {
        return SBezierKeyframe(inValue.m_KeyframeSeconds, m_Value, inValue.m_InTangentTime,
                               inValue.m_InTangentValue, inValue.m_OutTangentTime,
                               inValue.m_OutTangentValue);
    }
    TKeyframe operator()(const SEaseInEaseOutKeyframe &inValue) const
    {
        return SEaseInEaseOutKeyframe(inValue.m_KeyframeSeconds, m_Value, inValue.m_EaseIn,
                                      inValue.m_EaseOut);
    }
    TKeyframe operator()()
    {
        QT3DS_ASSERT(false);
        return TKeyframe();
    }
};

inline TKeyframe setKeyframeValue(const TKeyframe &inKeyframe, float inValue)
{
    return inKeyframe.visit<TKeyframe>(KeyframeValueSetter{inValue});
}

inline SBezierKeyframe CreateBezierKeyframeFromEaseInOut(float prevTime,
                                                    SEaseInEaseOutKeyframe keyframe, float nextTime)
{
    float kfValue = keyframe.m_KeyframeValue;
    float kfTime = keyframe.m_KeyframeSeconds; // seconds
    float timeIn = kfTime;
    float valueIn = kfValue;
    float timeOut = kfTime;
    float valueOut = kfValue;

     // at 100 ease, the control point will be midway between the 2 keyframes
    float maxEasePerc = .5f;

    if (prevTime != -1.f) {
        float easeInPerc = qBound(0.f, keyframe.m_EaseIn / 100.f, 1.f) * maxEasePerc;
        timeIn = prevTime + (kfTime - prevTime) * (1.f - easeInPerc);
    }

    if (nextTime != -1.f) {
        float easeOutPerc = qBound(0.f, keyframe.m_EaseOut / 100.f, 1.f) * maxEasePerc;
        timeOut = kfTime + (nextTime - kfTime) * easeOutPerc;
    }

    return SBezierKeyframe(kfTime, kfValue, timeIn , valueIn, timeOut, valueOut);
}

void CopyKeyframes(const IAnimationCore &inSourceAnimationCore, IAnimationCore &inDestAnimationCore,
                   Qt3DSDMAnimationHandle inDestAnimation, const TKeyframeHandleList &inKeyframes);

Qt3DSDMAnimationHandle CopyAnimation(TAnimationCorePtr inSourceAnimationCore,
                                     Qt3DSDMAnimationHandle inAnimation,
                                     Qt3DSDMSlideHandle inNewSlide,
                                     Qt3DSDMInstanceHandle inNewInstance,
                                     Qt3DSDMPropertyHandle inNewProperty, size_t inNewIndex);

struct SEaseInOutGetter
{
    std::pair<float, float> operator()(const SLinearKeyframe &) const { return {}; }
    std::pair<float, float> operator()(const SBezierKeyframe &) const { return {}; }
    std::pair<float, float> operator()(const SEaseInEaseOutKeyframe &inValue) const
    {
        return {inValue.m_EaseIn, inValue.m_EaseOut};
    }

    std::pair<float, float> operator()()
    {
        QT3DS_ASSERT(false);
        return {};
    }
};
inline void getEaseInOutValues(const TKeyframe &inValue, float &outEaseIn, float &outEaseOut)
{
    std::pair<float, float> ease = inValue.visit<std::pair<float, float>>(SEaseInOutGetter());

    outEaseIn = ease.first;
    outEaseOut = ease.second;
}

struct EaseInOutSetter
{
    float m_easeIn;
    float m_easeOut;

    TKeyframe operator()(SLinearKeyframe &inValue) const { return inValue; }
    TKeyframe operator()(SBezierKeyframe &inValue) const { return inValue; }
    TKeyframe operator()(SEaseInEaseOutKeyframe &inKeyframe) const
    {
        inKeyframe.m_EaseIn = m_easeIn;
        inKeyframe.m_EaseOut = m_easeOut;
        return inKeyframe;
    }
    TKeyframe operator()()
    {
        QT3DS_ASSERT(false);
        return TKeyframe();
    }
};

inline TKeyframe setEaseInOutValues(TKeyframe &inKeyframe, float easeIn, float easeOut)
{
    return inKeyframe.visit<TKeyframe>(EaseInOutSetter{easeIn, easeOut});
}

struct BezierValuesGetter
{
    QVector<float> operator()(const SLinearKeyframe &) const { return {}; }
    QVector<float> operator()(const SEaseInEaseOutKeyframe &) const { return {}; }
    QVector<float> operator()(const SBezierKeyframe &inValue) const
    {
        return {inValue.m_InTangentTime, inValue.m_InTangentValue,
                inValue.m_OutTangentTime, inValue.m_OutTangentValue};
    }
    QVector<float> operator()()
    {
        QT3DS_ASSERT(false);
        return {};
    }
};

inline void getBezierValues(const TKeyframe &keyframe, float &tangentInTime, float &tangentInValue,
                            float &tangentOutTime, float &tangentOutValue)
{
    const QVector<float> values = keyframe.visit<QVector<float>>(BezierValuesGetter());
    if (!values.empty()) {
        tangentInTime = values[0];
        tangentInValue = values[1];
        tangentOutTime = values[2];
        tangentOutValue = values[3];
    }
}

struct BezierOffsetter
{
    float dt; // time offset in seconds

    TKeyframe operator()(SLinearKeyframe &inValue) const { return inValue; }
    TKeyframe operator()(SEaseInEaseOutKeyframe &inValue) const { return inValue; }
    TKeyframe operator()(SBezierKeyframe &inKeyframe) const
    {
        inKeyframe.m_InTangentTime += dt;
        inKeyframe.m_OutTangentTime += dt;

        return inKeyframe;
    }
    TKeyframe operator()()
    {
        QT3DS_ASSERT(false);
        return TKeyframe();
    }
};

// dt: time offset in seconds
inline TKeyframe offsetBezier(TKeyframe &inKeyframe, float dt)
{
    return inKeyframe.visit<TKeyframe>(BezierOffsetter{dt});
}

} // namespace qt3dsdm

#endif
