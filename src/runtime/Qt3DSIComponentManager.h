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
#include "Qt3DSTimePolicy.h"
#include "foundation/Qt3DSOption.h"

//==============================================================================
//	Includes
//==============================================================================

namespace qt3ds {
namespace runtime {
    namespace element {
        struct TElement;
        struct TComponent;
    }
}
}
//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

struct IComponentTimeOverrideFinishedCallback
{
protected:
    virtual ~IComponentTimeOverrideFinishedCallback() {}
public:
    virtual void OnTimeFinished() = 0;
    virtual void Release() = 0;
};

typedef qt3ds::foundation::Option<TimePolicyModes::Enum> TTimePolicyModeOption;
typedef qt3ds::foundation::Option<INT32> TInt32Option;
typedef qt3ds::foundation::Option<unsigned long> TULongOption;
typedef qt3ds::foundation::Option<bool> TBoolOption;

struct SComponentGotoSlideData
{
    INT32 m_Slide;
    TTimePolicyModeOption m_Mode;
    //-2 means previous, -1 means next, 1-N means slide
    TInt32Option m_PlaythroughTo;
    TULongOption m_StartTime;
    FLOAT m_Rate;
    bool m_Reverse;
    TBoolOption m_Paused;

    // No mode means use the mode in the slide data.
    SComponentGotoSlideData(INT32 inSlide = 0,
                            TTimePolicyModeOption inMode = TTimePolicyModeOption(),
                            TInt32Option inPlayThrough = TInt32Option(),
                            TULongOption inStartTime = TULongOption(), FLOAT inRate = 1.0f,
                            bool inReverse = false, TBoolOption inPaused = TBoolOption())
        : m_Slide(inSlide)
        , m_Mode(inMode)
        , m_PlaythroughTo(inPlayThrough)
        , m_StartTime(inStartTime)
        , m_Rate(inRate)
        , m_Reverse(inReverse)
        , m_Paused(inPaused)
    {
    }
};

//==============================================================================
/**
 *	@interface	IComponentManager
 *	Base interface of the container of all the components in the presentation.
 *	This specify the basic operations required to implement a Component manager
 *	object that works well with the kernel.
 */
class IComponentManager
{
    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    virtual ~IComponentManager() {}

public: // Promotion
    virtual TComponent *GetComponent(TElement *inElement) = 0;

public: // Slides
    virtual void GotoSlideIndex(TElement *inComponent, const SComponentGotoSlideData &inGotoData,
                                BOOL inSlideExit = true) = 0;
    virtual void GotoSlideName(TElement *inComponent, const TStringHash inSlideHashName) = 0;
    virtual void GoToBackSlide(TElement *inComponent) = 0;
    virtual void GoToNextSlide(TElement *inComponent, const INT32 inDecrement = 1) = 0;
    virtual void GoToPreviousSlide(TElement *inComponent, const INT32 inDecrement = 1) = 0;

    virtual UINT8 GetSlideCount(TElement *inComponent) = 0;
    virtual UINT8 GetCurrentSlide(TElement *inComponent) = 0;
    virtual const CHAR *GetCurrentSlideName(TElement *inComponent) = 0;
    virtual void applyQueuedChanges(TElement *component) = 0;
    virtual void queueChange(TElement *component, TElement *target, const char *attName,
                             const char *value) = 0;

    virtual void OnElementDeactivated(TElement *inElement) = 0;

    virtual void SetComponentTimeOverride(TElement *inElement, TTimeUnit inEndTime,
                                          FLOAT inInterpolation,
                                          IComponentTimeOverrideFinishedCallback *inCallback) = 0;

    virtual void SetupComponentGotoSlideCommand(TElement *inElement,
                                                const SComponentGotoSlideData &inGotoSlideData) = 0;
    virtual bool HasComponentGotoSlideCommand(TElement *inElement) = 0;
    virtual SComponentGotoSlideData GetComponentGotoSlideCommand(TElement *inElement) = 0;
    virtual void ReleaseComponentGotoSlideCommand(TElement *inElement) = 0;

public: // Time
    virtual void GoToTime(TElement *inComponent, const TTimeUnit inTime) = 0;
    virtual void SetPause(TElement *inComponent, const BOOL inPause) = 0;
    virtual void SetTimePolicy(TElement *inComponent, const TTimeUnit inLoopDuration,
                               const UINT32 inRepetitions, const BOOL inPingPong,
                               const BOOL inPlayThrough) = 0;

    virtual BOOL GetPause(TElement *inComponent) = 0;
    virtual BOOL GetPlayBackDirection(TElement *inComponent) = 0;
};

} // namespace Q3DStudio
