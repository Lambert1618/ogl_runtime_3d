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
#ifndef STUDIOANIMATIONSYSTEMIMPLH
#define STUDIOANIMATIONSYSTEMIMPLH

#include "Qt3DSDMAnimation.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMSlideCore.h"
#include "Qt3DSDMSlideGraphCore.h"
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMSignals.h"

namespace qt3dsdm {

typedef std::pair<Qt3DSDMAnimationHandle, float> TAnimationFloatPair;
typedef std::vector<TAnimationFloatPair> TAnimationFloatPairList;

class CStudioAnimationSystem : public IStudioAnimationSystem,
                               public ITransactionProducer
{
    Q_DISABLE_COPY(CStudioAnimationSystem)

    TPropertySystemPtr m_PropertySystem;
    TSlideSystemPtr m_SlideSystem;
    TSlideCorePtr m_SlideCore;
    TSlideGraphCorePtr m_SlideGraphCore;
    TAnimationCorePtr m_AnimationCore;
    TRefreshCallbackFunc m_refreshCallback = nullptr;

    bool m_AutoKeyframe;
    // When a property has an animation associated with it, then writes
    // do not go all the way down to the db.  They stay on this object.
    TAnimationFloatPairList m_AnimationFloatPairs;
    bool m_SmoothInterpolation;

    TAnimationKeyframesPairList m_DeletedAnimationData; // list to store deanimated animation &
                                                        // keyframe data. This will be use if user
                                                        // wants to animate the same property again

    TTransactionConsumerPtr m_Consumer;

    std::vector<std::shared_ptr<ISignalConnection>> m_Connections;

public:
    CStudioAnimationSystem(TPropertySystemPtr inPropertySystem, TSlideSystemPtr inSlideSystem,
                           TSlideCorePtr inSlideCore, TSlideGraphCorePtr inSlideGraphCore,
                           TAnimationCorePtr inAnimationCore);

    void ClearTemporaryAnimationValues();
    void ClearTemporaryAnimationValues(Qt3DSDMAnimationHandle inAnimation);

    void SetPropertySystem(TPropertySystemPtr inPropertySystem)
    {
        m_PropertySystem = inPropertySystem;
    } // HACK: TODO: We should really consider having all the subsytem know everyone else without
      // passing in so many things in the constructor

    //====================================================================
    // IStudioAnimationSystem implementation
    //====================================================================
    void SetAutoKeyframe(bool inAutoKeyframe) override;
    bool GetAutoKeyframe() const override;
    void Animate(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) override;
    void Deanimate(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) override;
    void KeyframeProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                  bool inDoDiffValue) override;
    void SetOrCreateKeyframe(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty, float inTimeInSeconds,
                                     SGetOrSetKeyframeInfo *inKeyframeInfo, size_t inNumInfos) override;
    Qt3DSDMAnimationHandle GetControllingAnimation(Qt3DSDMInstanceHandle inInstance,
                                                          Qt3DSDMPropertyHandle inProperty,
                                                          size_t inIndex) const override;
    bool IsPropertyAnimatable(Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) const override;
    bool IsPropertyAnimated(Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) const override;
    void SetInterpolation(bool inSmooth) override { m_SmoothInterpolation = inSmooth; }

    bool GetAnimatedInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty,
                                                  SValue &outValue) const override;
    bool SetAnimatedInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty,
                                                  const SValue &inValue) override;
    //====================================================================

    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    void setRefreshCallback(TRefreshCallbackFunc func) override;

private:
    void OnAnimationDeleted(Qt3DSDMAnimationHandle inAnimation);

    Qt3DSDMSlideHandle GetApplicableGraphAndSlide(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty,
                                                 const SValue &inValue);

    void OverrideChannelIfAnimated(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                   Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                   float inSeconds, bool &ioAnimated, SValue &outValue) const;

    void DoKeyframeProperty(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                            Qt3DSDMPropertyHandle inProperty, const SValue &inValue,
                            bool inDoDiffValue);

    Qt3DSDMKeyframeHandle CreateKeyframe(Qt3DSDMAnimationHandle inAnimation, const SValue &inValue,
                                         float inSeconds, TAnimationCorePtr inAnimationCore);
    Qt3DSDMKeyframeHandle CreateKeyframeExplicit(Qt3DSDMAnimationHandle inAnimation, float inValue,
                                                 float inSeconds, TKeyframe kfData = {});

};
}

#endif
