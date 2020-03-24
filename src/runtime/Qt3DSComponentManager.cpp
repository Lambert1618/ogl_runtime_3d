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

#include "RuntimePrefix.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSComponentManager.h"
#include "Qt3DSIPresentation.h"
#include "Qt3DSSlideSystem.h"
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSPresentationFrameData.h"
#include "Qt3DSApplication.h"
#include "Qt3DSActivationManager.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSQmlElementHelper.h"

using qt3ds::runtime::SSlideKey;
//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 *	@param inPresentation	reference to its presentation
 */
CComponentManager::CComponentManager(IPresentation &inPresentation)
    : m_Presentation(static_cast<CPresentation &>(inPresentation))
{
}

//==============================================================================
/**
 *	Enforces the contract that slide switching requires a reset to the master
 *	slide (slide 0) before switching to the desired slide index.
 *	Also updates the TComponent's current slide number.
 *	@param inComponent		TComponent to set slide on
 *	@param inSlideIndex		relative index of the slide to switch to
 *	@param inSlideExit		true if exiting the slide, false otherwise
 */
void CComponentManager::GotoSlideIndex(TElement *inComponent,
                                       const SComponentGotoSlideData &inGotoData,
                                       BOOL inSlideExit /*= true*/)
{
    TComponent *theComponent = GetComponent(inComponent);

    if (theComponent == NULL) {
        return;
    }

    if (inGotoData.m_Slide < 1 || inGotoData.m_Slide >= theComponent->GetSlideCount()) {
        qCCritical(qt3ds::INVALID_PARAMETER)
                << "GotoSlide: Index out of range: Index " << inGotoData.m_Slide
                << " SlideCount: "<< theComponent->GetSlideCount();
        return;
    }

    if (theComponent->GetActive() == false) {
        m_ComponentInitialSlideMap[inComponent] = inGotoData;
        return;
    }

    SComponentGotoSlideData theGotoSlideData(inGotoData);

    TComponentGotoSlideDataMap::iterator iter = m_ComponentInitialSlideMap.find(inComponent);
    if (iter != m_ComponentInitialSlideMap.end()) {
        theGotoSlideData = iter->second;
        m_ComponentInitialSlideMap.erase(iter);
    }

    const UINT8 theCurrentSlideIndex = theComponent->GetCurrentSlide();

    QString elementPath;
    // Primary presentation stores elementpaths with subpresentation prefix.
    // Prepend it here so that slide change notification is directed at a correct elementpath.
    if (&m_Presentation != m_Presentation.GetApplication().GetPrimaryPresentation())
        elementPath = m_Presentation.GetName() + QLatin1Char(':');

    elementPath.append(QString::fromUtf8(inComponent->path().c_str()));

    if (inSlideExit) {
        SEventCommand theEvent = { inComponent, EVENT_ONSLIDEEXIT };
        theEvent.m_IsEvent = true;
        m_Presentation.ProcessEvent(theEvent);

        m_Presentation.GetApplication().ComponentSlideExited(&m_Presentation, inComponent,
                                                             elementPath, theCurrentSlideIndex,
                                                             GetCurrentSlideName(inComponent));

        // Signal previous slide change
        m_Presentation.signalProxy()->SigSlideExited(elementPath, theCurrentSlideIndex,
                                                     GetCurrentSlideName(inComponent));

        // m_Presentation.FireEvent(EVENT_ONSLIDEEXIT, inComponent);
        // m_Presentation.FlushEventCommandQueue();
    }

    // Update dynamic keys to use current values before slide switching, with the exception of
    // master slides because playback only starts from non-master slides. Skip rollback
    // if we are expecting a gototime command, as rollback overwrites the attribute changes
    // caused by gototime.
    if (theCurrentSlideIndex > 0 && !HasComponentGotoTimeCommand(theComponent)) {
        m_Presentation.GetSlideSystem().InitializeDynamicKeys(
            SSlideKey(*theComponent, (qt3ds::QT3DSU8)theGotoSlideData.m_Slide),
            m_Presentation.GetAnimationSystem());
        // deactivate actions and animation tracks on the current slide.
        m_Presentation.GetSlideSystem().RollbackSlide(
            SSlideKey(*inComponent, (qt3ds::QT3DSU8)theCurrentSlideIndex),
            m_Presentation.GetAnimationSystem(), m_Presentation.GetLogicSystem());
    } else if (theCurrentSlideIndex == 0){
        m_Presentation.GetSlideSystem().ExecuteSlide(SSlideKey(*inComponent, 0),
                                                     m_Presentation.GetAnimationSystem(),
                                                     m_Presentation.GetLogicSystem());
    }
    // Execute new slide.
    m_Presentation.GetSlideSystem().ExecuteSlide(
        SSlideKey(*inComponent, (qt3ds::QT3DSU8)theGotoSlideData.m_Slide),
        m_Presentation.GetAnimationSystem(), m_Presentation.GetLogicSystem());
    CTimePolicy &thePolicy(theComponent->GetTimePolicy());
    TTimeUnit theLoopingDuration = thePolicy.GetLoopingDuration();
    if (theGotoSlideData.m_Mode.hasValue()) {
        inComponent->SetPlayThrough(false);
        switch (*theGotoSlideData.m_Mode) {
        case TimePolicyModes::Looping:
            thePolicy.Initialize(theLoopingDuration, 0, false);
            break;
        case TimePolicyModes::PingPong:
            thePolicy.Initialize(theLoopingDuration, 0, true);
            break;
        case TimePolicyModes::StopAtEnd:
            thePolicy.Initialize(theLoopingDuration, 1, false);
            break;
        case TimePolicyModes::Ping:
            thePolicy.Initialize(theLoopingDuration, 1, true);
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
    }

    // Update the current index which also updates the back index
    theComponent->SetCurrentSlide(static_cast<UINT8>(theGotoSlideData.m_Slide));
    m_PlaythroughOverrideMap.erase(inComponent);
    if (theGotoSlideData.m_PlaythroughTo.hasValue()) {
        m_PlaythroughOverrideMap.insert(
            eastl::make_pair(inComponent, *theGotoSlideData.m_PlaythroughTo));
        theComponent->SetPlayThrough(true);
    }

    // Reset time for component to 0 (unless we have gototime in queue).

    if (theGotoSlideData.m_Paused.hasValue())
        thePolicy.SetPaused(*theGotoSlideData.m_Paused);

    thePolicy.SetRate(theGotoSlideData.m_Rate);
    thePolicy.SetReverse(theGotoSlideData.m_Reverse);

    if (!HasComponentGotoTimeCommand(theComponent)) {
        if (theGotoSlideData.m_StartTime.hasValue())
            thePolicy.SetTime(*theGotoSlideData.m_StartTime);
    } else {
        thePolicy.SetTime(m_ComponentGotoTimeMap[theComponent]);
    }

    inComponent->SetDirty();
    m_Presentation.FireEvent(EVENT_ONSLIDEENTER, inComponent);
    qt3ds::runtime::IActivityZone *theZone = m_Presentation.GetActivityZone();
    if (theZone)
        theZone->OnSlideChange(*inComponent);

    ReleaseComponentGotoTimeCommand(inComponent);
    m_Presentation.GetApplication().ComponentSlideEntered(&m_Presentation, inComponent,
                                                          elementPath, theGotoSlideData.m_Slide,
                                                          GetCurrentSlideName(inComponent));
    // Signal current slide change
    m_Presentation.signalProxy()->SigSlideEntered(elementPath, GetCurrentSlide(inComponent),
                                                  GetCurrentSlideName(inComponent));
}

//==============================================================================
/**
 *	Sets a component to a specified slide using the slide name hash.
 *	Performs a switch to Slide 0 (master) first, before switching to the
 *	required slide.
 *	@param inComponent		reference to the TComponent
 *	@param inSlideHashName	hash of the slide to switch to
 */
void CComponentManager::GotoSlideName(TElement *inComponent, const TStringHash inSlideHashName)
{
    TComponent *theComponent = GetComponent(inComponent);
    UINT8 theSlideIndex = m_Presentation.GetSlideSystem().FindSlide(*theComponent, inSlideHashName);

    if (theSlideIndex < 1 || theSlideIndex >= theComponent->GetSlideCount()) {
        const char *slideName = m_Presentation.GetApplication().ReverseHash(inSlideHashName);
        if (slideName && *slideName)
            qCCritical(qt3ds::INVALID_PARAMETER) << "GotoSlide: Name not found: " << slideName;
        else
            qCCritical(qt3ds::INVALID_PARAMETER) << "GotoSlide: Name not found: " << inSlideHashName;

    } else
        GotoSlideIndex(inComponent, theSlideIndex);
}

//==============================================================================
/**
 *	Return the index of the current slide of the component.
 *	@param inComponent		reference to the TComponent
 *	@return the index of current slide
 */
UINT8 CComponentManager::GetCurrentSlide(TElement *inComponent)
{
    return GetComponent(inComponent)->GetCurrentSlide();
}

//==============================================================================
/**
 *	Return the number of slides on a component.
 *	@param inComponent		reference to the TComponent
  *	@return the number of slides
 */
UINT8 CComponentManager::GetSlideCount(TElement *inComponent)
{
    return GetComponent(inComponent)->GetSlideCount();
}

//==============================================================================
/**
 *	Performs a switch to the slide following the current slide (current slide + 1).
 *	@param inComponent		reference to the TComponent
 *	@param inIncrement		slide increment offset
 */
void CComponentManager::GoToNextSlide(TElement *inComponent, const INT32 inIncrement)
{
    TComponent *theComponent = GetComponent(inComponent);
    INT32 theNewIndex = Q3DStudio_clamp<INT32>(inIncrement + theComponent->GetCurrentSlide(), 1L,
                                               theComponent->GetSlideCount() - 1);

    // Trigger goto to slide only if the next slide is not the current
    if (theComponent->GetActive()) {
        if (theNewIndex != theComponent->GetCurrentSlide())
            GotoSlideIndex(inComponent, theNewIndex);
    } else {
        qCCritical(qt3ds::INVALID_PARAMETER)
                << "Runtime: Attempt to goto slide on an inactive component!";
    }
}

//==============================================================================
/**
 *	Performs a switch to the slide following the current slide (current slide - 1).
 *	@param inComponent	reference to the TComponent
 *	@param inDecrement	slide decrement offset
 */
void CComponentManager::GoToPreviousSlide(TElement *inComponent, const INT32 inDecrement)
{
    TComponent *theComponent = GetComponent(inComponent);
    INT32 theNewIndex = Q3DStudio_clamp<INT32>(theComponent->GetCurrentSlide() - inDecrement, 1L,
                                               theComponent->GetSlideCount() - 1);

    // Trigger goto to slide only if the next slide is not the current
    if (theNewIndex != theComponent->GetCurrentSlide())
        GotoSlideIndex(inComponent, theNewIndex);
}

//==============================================================================
/**
 *	Playthrough to the next slide that is specified.
 *	@param inComponent	reference to the TComponent
 */
void CComponentManager::PlaythroughToSlide(TElement *inComponent)
{
    TComponent *theComponent = GetComponent(inComponent);
    TComponentIntMap::iterator iter = m_PlaythroughOverrideMap.find(inComponent);
    INT32 thePlaythroughTo = 0;
    if (iter != m_PlaythroughOverrideMap.end()) {
        thePlaythroughTo = iter->second;
        m_PlaythroughOverrideMap.erase(iter);
    } else {
        thePlaythroughTo = m_Presentation.GetSlideSystem().GetPlaythroughToSlideIndex(
            SSlideKey(*theComponent, (qt3ds::QT3DSU8)theComponent->GetCurrentSlide()));
        if (thePlaythroughTo == -1) {
            qCCritical(qt3ds::INVALID_OPERATION) << "Missing slide to play through to";
            inComponent->SetPlayThrough(
                false); // clear this flag, so that this is equivalent to a "Stop At End"
        }
    }

    if (thePlaythroughTo == -2) {
        GoToPreviousSlide(inComponent, 1);
    } else if (thePlaythroughTo == -1) {
        GoToNextSlide(inComponent, 1);
    } else {
        GotoSlideIndex(inComponent, thePlaythroughTo);
    }
}

void CComponentManager::applyQueuedChanges(TElement *component)
{
    const auto &targetQueue = m_queuedChanges[component];
    for (auto itTarget = targetQueue.constBegin(); itTarget != targetQueue.constEnd();
         ++itTarget) {
        const auto &target = itTarget.key();
        const auto &attributeQueue = itTarget.value();
        for (auto itAttribute = attributeQueue.constBegin();
             itAttribute != attributeQueue.constEnd(); ++itAttribute) {
            const auto &attribute = itAttribute.key();
            target->SetAttribute(attribute, itAttribute.value());
        }
    }
    m_queuedChanges.remove(component);
}

void CComponentManager::queueChange(TElement *component, TElement *target, const char *attName,
                                    const char *value, TAttributeHash attrHash)
{
    CQmlElementHelper::TypedAttributeAndValue attributeAndValue
            = CQmlElementHelper::getTypedAttributeAndValue(target, attName, value, attrHash);
    if (attributeAndValue.attribute.m_Hash != 0) {
        auto &targetQueue = m_queuedChanges[component];
        auto &attQueue = targetQueue[target];
        attQueue[attributeAndValue.attribute.m_Hash] = attributeAndValue.value;
    }
}

bool CComponentManager::hasSlideChangeQueued(TElement *component)
{
    return m_ComponentInitialSlideMap.find(component) != m_ComponentInitialSlideMap.end();
}

//==============================================================================
/**
 *	Performs a switch to the previous slide.
 *	@param inComponent	reference to the TComponent
 */
void CComponentManager::GoToBackSlide(TElement *inComponent)
{
    TComponent *theComponent = GetComponent(inComponent);
    GotoSlideIndex(theComponent, theComponent->GetPreviousSlide());
}

//==============================================================================
/**
 *	Set the component's local time using the supplied time.
 *	@param inComponent		reference to the TComponent
 *	@param inTime			supplied time
 */
void CComponentManager::GoToTime(TElement *inComponent, const TTimeUnit inTime)
{
    if (inComponent == NULL)
        return;
    if (!inComponent->GetActive() && !inComponent->AboutToActivate()) {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "Runtime: Attempt to goto time on inactive component!";
        return;
    }
    SetupComponentGotoTimeCommand(inComponent, inTime);
    m_Presentation.GetActivityZone()->GoToTime(*inComponent, inTime);
    inComponent->SetDirty();
}

//==============================================================================
/**
 *	Get the component's playback state
 *	@param inComponent		reference to the TComponent
 *	@return true if it's paused, false if it's playing
 */
BOOL CComponentManager::GetPause(TElement *inComponent)
{
    CTimePolicy &theTimePolicy = GetComponent(inComponent)->GetTimePolicy();
    return theTimePolicy.GetPaused();
}

//==============================================================================
/**
 *	Get the component's ping pong direction
 *	@param inComponent		reference to the TComponent
 *	@return true if it's pingponging forward, false if it's pingponging backwards
 */
BOOL CComponentManager::GetPlayBackDirection(TElement *inComponent)
{
    return GetComponent(inComponent)->GetPlayBackDirection();
}

//==============================================================================
/**
 *	Update time policy of the component during each slide execution
 *	@param inComponent		reference to the TComponent
 *	@param inLoopDuration	loop duration of component at that slide
 *	@param inRepetitions	number of repetitions
 *	@param inPingPong		if true the component will move with respect to reverse time
 *sequence
 *	@param inPlayThrough	if true the component will continue to execute next slide
 */
void CComponentManager::SetTimePolicy(TElement *inComponent, const TTimeUnit inLoopDuration,
                                      const UINT32 inRepetitions, const BOOL inPingPong,
                                      const BOOL inPlayThrough)
{
    CTimePolicy &theTimePolicy = GetComponent(inComponent)->GetTimePolicy();
    theTimePolicy.Initialize(inLoopDuration, inRepetitions, inPingPong);
    inComponent->SetPlayThrough(inPlayThrough);
}

//==============================================================================
/**
 *	Set the component's playback to either play or pause
 *	@param inComponent		reference to the TComponent
 *	@param inPause			true = pause, false = play
 */
void CComponentManager::SetPause(TElement *inComponent, const BOOL inPause)
{
    CTimePolicy &theTimePolicy = GetComponent(inComponent)->GetTimePolicy();
    BOOL wasPaused = theTimePolicy.GetPaused();
    theTimePolicy.SetPaused(inPause);
    if (wasPaused != inPause)
        inComponent->SetDirty();
}

//==============================================================================
/**
 *	Promote from TElement* to TComponent.
 *	@param inElement	TElement pointer to be recast as TComponent
 *	@return pointer to the component
 */
TComponent *CComponentManager::GetComponent(TElement *inElement)
{
    Q3DStudio_ASSERT(inElement->IsComponent());
    return static_cast<TComponent *>(inElement);
}

//==============================================================================
/**
 *	Gets the string name of the current slide.
 *	@param inComponent		the component to query for it's current slide name
 *	@return the char buffer storing the name
 */
const CHAR *CComponentManager::GetCurrentSlideName(TElement *inComponent)
{
    TComponent *theComponent = GetComponent(inComponent);
    return m_Presentation.GetSlideSystem().GetSlideName(
        SSlideKey(*theComponent, (qt3ds::QT3DSU8)theComponent->GetCurrentSlide()));
}

void CComponentManager::OnElementDeactivated(TElement *)
{
}

void CComponentManager::SetComponentTimeOverride(TElement *inElement, TTimeUnit inEndTime,
                                                 FLOAT inInterpolation,
                                                 IComponentTimeOverrideFinishedCallback *inCallback)
{
    if (inElement->IsComponent()) {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "ComponentManager: SetComponentTimeOverride called on object that "
                << "wasn't an element";
        if (inCallback)
            inCallback->Release();
        Q3DStudio_ASSERT(false);
        return;
    }
    // sanitize end time.
    CTimePolicy &theTimePolicy = GetComponent(inElement)->GetTimePolicy();
    if (inEndTime < 0)
        inEndTime = 0;
    if (inEndTime > theTimePolicy.GetLoopingDuration())
        inEndTime = theTimePolicy.GetLoopingDuration();
    m_Presentation.GetActivityZone()->GetOrCreateItemComponentOverride(*inElement, inInterpolation,
                                                                       inEndTime, inCallback);

    // Force the time policy object to respect its own time.  If we played through till the end and
    // sat there for a while
    // we need the actual time will travel further and further from the time policy's local time.
    // If we then play backward we
    // will jump until the offset is synchronized with the actual time.
    theTimePolicy.SetTime(theTimePolicy.GetTime());
    inElement->SetDirty();
}

void CComponentManager::SetupComponentGotoSlideCommand(TElement *inElement,
                                                       const SComponentGotoSlideData &inSlide)
{
    m_ComponentGotoSlideMap[inElement] = inSlide;
}

bool CComponentManager::HasComponentGotoSlideCommand(TElement *inElement)
{
    return m_ComponentGotoSlideMap.find(inElement) != m_ComponentGotoSlideMap.end();
}

SComponentGotoSlideData CComponentManager::GetComponentGotoSlideCommand(TElement *inElement)
{
    TComponentGotoSlideDataMap::iterator iter = m_ComponentGotoSlideMap.find(inElement);
    if (iter != m_ComponentGotoSlideMap.end())
        return iter->second;
    return -1;
}

void CComponentManager::ReleaseComponentGotoSlideCommand(TElement *inElement)
{
    TComponentGotoSlideDataMap::iterator iter = m_ComponentGotoSlideMap.find(inElement);
    if (iter != m_ComponentGotoSlideMap.end())
        m_ComponentGotoSlideMap.erase(iter);
}

bool CComponentManager::HasComponentGotoTimeCommand(TElement *inElement)
{
    return m_ComponentGotoTimeMap.find(inElement) != m_ComponentGotoTimeMap.end();
}

void CComponentManager::ReleaseComponentGotoTimeCommand(TElement *inElement)
{
    TComponentGotoTimeMap::iterator iter = m_ComponentGotoTimeMap.find(inElement);
    if (iter != m_ComponentGotoTimeMap.end())
        m_ComponentGotoTimeMap.erase(iter);
}

void CComponentManager::ClearGotoTimeQueue()
{
    m_ComponentGotoTimeMap.clear();
}

void CComponentManager::SetupComponentGotoTimeCommand(TElement *inElement, TTimeUnit time)
{
    m_ComponentGotoTimeMap[inElement] = time;
}

} // namespace Q3DStudio
