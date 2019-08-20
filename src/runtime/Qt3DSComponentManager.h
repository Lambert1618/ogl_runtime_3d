/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSIComponentManager.h"
#include "EASTL/hash_map.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {
class CTimePolicy;
class CPresentation;
struct SComponentTimePolicyOverride
{
    TElement *m_Element;
    FLOAT m_TimeMultiplier;
    TTimeUnit m_EndTime;
    IComponentTimeOverrideFinishedCallback *m_TimeCallback;
    SComponentTimePolicyOverride(TElement *inElement, FLOAT inMultiplier, TTimeUnit inEndTime,
                                 IComponentTimeOverrideFinishedCallback *inCallback)
        : m_Element(inElement)
        , m_TimeMultiplier(inMultiplier)
        , m_EndTime(inEndTime)
        , m_TimeCallback(inCallback)
    {
    }
    // Returns the local time and a boolean indicating if we have reached the end.
    // Implemented in UICTimePolicy.cpp so that I can compare the CTimePolicy::ComputeTime method
    // with
    // SComponentTimePolicyOverride::ComputLocalTime method
    eastl::pair<BOOL, TTimeUnit> ComputeLocalTime(CTimePolicy &inTimePolicy,
                                                  TTimeUnit inGlobalTime);
    // Implemented in UICTimePolicy.cpp
    void SetTime(CTimePolicy &inTimePolicy, TTimeUnit inLocalTime);
};

typedef eastl::hash_map<TElement *, SComponentGotoSlideData> TComponentGotoSlideDataMap;
typedef eastl::hash_map<TElement *, Q3DStudio::INT32> TComponentIntMap;

//==============================================================================
/**
 *	The Component Manager is a factory and container of all components
 *	within the presentation.
 */
class CComponentManager : public IComponentManager
{
    CPresentation &m_Presentation;
    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CComponentManager(IPresentation &inPresentation);

public: // Slide
    void GotoSlideIndex(TElement *inComponent, const SComponentGotoSlideData &inGotoData,
                        BOOL inSlideExit = true) override;
    void GotoSlideName(TElement *inComponent, const TStringHash inSlideHashName) override;
    void GoToBackSlide(TElement *inComponent) override;
    void GoToNextSlide(TElement *inComponent, const INT32 inDecrement = 1) override;
    void GoToPreviousSlide(TElement *inComponent, const INT32 inDecrement = 1) override;
    void PlaythroughToSlide(TElement *inComponent);
    void applyQueuedChanges(TElement *component) override;
    void queueChange(TElement *component, TElement *target, const char *attName,
                     const char *value) override;

    UINT8 GetSlideCount(TElement *inComponent) override;
    UINT8 GetCurrentSlide(TElement *inComponent) override;
    const CHAR *GetCurrentSlideName(TElement *inComponent) override;

    void OnElementDeactivated(TElement *inElement) override;

    void SetComponentTimeOverride(TElement *inElement, TTimeUnit inEndTime, FLOAT inInterpolation,
                                  IComponentTimeOverrideFinishedCallback *inCallback) override;

    // Allows multiple gotoslide command operating on the same element to work correctly.
    // This API is meant to fix a scenario where gotoslide is called multiple times in a frame
    // on the same component.  This isn't avoidable in some cases without very complex scene logic.

    // later calls override earlier calls
    void SetupComponentGotoSlideCommand(TElement *inElement,
                                        const SComponentGotoSlideData &inSlide) override;
    bool HasComponentGotoSlideCommand(TElement *inElement) override;
    SComponentGotoSlideData GetComponentGotoSlideCommand(TElement *inElement) override;
    void ReleaseComponentGotoSlideCommand(TElement *inElement) override;

public: // Time
    void GoToTime(TElement *inComponent, const TTimeUnit inTime) override;
    void SetPause(TElement *inComponent, const BOOL inPause) override;
    void SetTimePolicy(TElement *inComponent, const TTimeUnit inLoopDuration,
                       const UINT32 inRepetitions, const BOOL inPingPong, const BOOL inPlayThrough) override;

    TTimeUnit ComputeComponentLocalTime(TElement *inComponent, const TTimeUnit inGlobalTime);
    BOOL GetPause(TElement *inComponent) override;
    BOOL GetPlayBackDirection(TElement *inComponent) override;

protected: // Promotion
    TComponent *GetComponent(TElement *inElement) override;

private:
    // Disabled Copy Construction
    CComponentManager(const CComponentManager &);
    CComponentManager &operator=(const CComponentManager &);

    TComponentGotoSlideDataMap m_ComponentInitialSlideMap;
    TComponentGotoSlideDataMap m_ComponentGotoSlideMap;
    TComponentIntMap m_PlaythroughOverrideMap;

    QHash<TElement *, QHash<TElement *, QHash<TAttributeHash, UVariant>>> m_queuedChanges;

    //==============================================================================
    //	Friends
    //==============================================================================
    friend class CSlideBuilder;
};

} // namespace Q3DStudio
