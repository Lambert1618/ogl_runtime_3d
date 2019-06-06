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
#include "Qt3DSTimePolicy.h"
#include "foundation/Qt3DSUnionCast.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "Qt3DSComponentManager.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const TTimeUnit CTimePolicy::FOREVER = 0;

SAlignedTimeUnit::SAlignedTimeUnit(const TTimeUnit &inUnit)
{
    StaticAssert<sizeof(SAlignedTimeUnit) == sizeof(TTimeUnit)>::valid_expression();
    qt3ds::intrinsics::memCopy(this, &inUnit, sizeof(TTimeUnit));
}

SAlignedTimeUnit::operator TTimeUnit() const
{
    TTimeUnit retval;
    qt3ds::intrinsics::memCopy(&retval, this, sizeof(TTimeUnit));
    return retval;
}

void SAlignedTimeUnit::operator-=(const TTimeUnit &inTime)
{
    TTimeUnit theThis(*this);
    theThis -= inTime;
    *this = SAlignedTimeUnit(theThis);
}

void SAlignedTimeUnit::operator%=(const TTimeUnit &inTime)
{
    TTimeUnit theThis(*this);
    theThis %= inTime;
    *this = SAlignedTimeUnit(theThis);
}

//==============================================================================
/**
 *	Initialization
 */

void CTimePolicy::Initialize(const TTimeUnit inLoopingDuration, TimePolicyModes::Enum inMode)
{
    m_LocalTime = 0;
    m_LoopingDuration = (unsigned long)inLoopingDuration;
    m_TimePolicyMode = inMode;
    m_Offset = 0;
    m_Paused = false;
    m_OffsetInvalid = 1;
    m_Backward = 0;
    m_Rate = 1.0f;
}

void CTimePolicy::Initialize(const TTimeUnit inLoopingDuration /*= FOREVER*/,
                             UINT32 inRepetitions /*= 1*/, BOOL inPingPong /*= false*/)
{
    // UINT8		m_Repetitions		: 2;					///< Number of traversal ( 0 = forever, 1 = stop at end, 2 =
    // ping )
    TimePolicyModes::Enum theMode = TimePolicyModes::StopAtEnd;
    if (inRepetitions == 1)
        theMode = TimePolicyModes::StopAtEnd;
    else if (inRepetitions == 0) {
        if (inPingPong)
            theMode = TimePolicyModes::PingPong;
        else
            theMode = TimePolicyModes::Looping;
    } else
        theMode = TimePolicyModes::Ping;

    Initialize(inLoopingDuration, theMode);
}

//==============================================================================
/**
 *	Return the last computed time. This is subjected to playmodes.
 *	@return local time
 */
TTimeUnit CTimePolicy::InternalGetTime() const
{
    if (m_LocalTime < m_LoopingDuration)
        return m_LocalTime;
    else {
        if (m_LoopingDuration == 0)
            return m_LocalTime;

        switch (m_TimePolicyMode) {
        case TimePolicyModes::StopAtEnd:
            return m_LoopingDuration;
        case TimePolicyModes::Looping:
            return m_LocalTime % m_LoopingDuration;
        case TimePolicyModes::Ping: {
            unsigned long interval = m_LocalTime / m_LoopingDuration;
            unsigned long leftover = m_LocalTime % m_LoopingDuration;
            if (interval == 1)
                return m_LoopingDuration - leftover;
            return 0;
        }
        case TimePolicyModes::PingPong: {
            unsigned long interval = m_LocalTime / m_LoopingDuration;
            unsigned long leftover = m_LocalTime % m_LoopingDuration;
            if (interval % 2)
                return m_LoopingDuration - leftover;
            return leftover;
        }
        default:
            QT3DS_ASSERT(false);
            return 0;
            break;
        }
    }
}

TTimeUnit CTimePolicy::GetTime() const
{
    TTimeUnit retval = InternalGetTime();
    // Apply direction and rate.
    bool isPlayingBack = m_Backward;
    if (m_Rate < 0)
        isPlayingBack = !isPlayingBack;

    if (isPlayingBack)
        retval = m_LoopingDuration - retval;
    return retval;
}

static unsigned long clampTime(long inTime, unsigned long inDuration)
{
    if (inTime < 0)
        return 0;
    if (inTime > (long)inDuration)
        return inDuration;
    return (unsigned long)inTime;
}

//==============================================================================
/**
 *	Trigger the policy to return inTime next GetTime is called.
 *	This is tricky math and the m_Offset field is being used both normally to
 *	simply change the time and here to calibrate itself next time ComputeTime
 *	is called.
 *	@param inTime	time to be returned next ComputeTime
 */
void CTimePolicy::SetTime(TTimeUnit inTime)
{

    bool isPlayingBack = m_Backward;
    if (m_Rate < 0)
        isPlayingBack = !isPlayingBack;

    if (isPlayingBack)
        inTime = m_LoopingDuration - inTime;

    m_OffsetInvalid = 1;

    // Setup m_LocalTime such that it reflects intime.
    if (m_LoopingDuration == 0) {
        m_LocalTime = (unsigned long)inTime;
        return;
    }

    switch (m_TimePolicyMode) {
    case TimePolicyModes::StopAtEnd:
        m_LocalTime = clampTime((long)inTime, m_LoopingDuration);
        break;
    case TimePolicyModes::Looping: {
        long input = (long)inTime;
        while (input < 0)
            input += (long)m_LoopingDuration;
        m_LocalTime = ((unsigned long)input) % m_LoopingDuration;
    } break;
    case TimePolicyModes::Ping: {
        long input = (long)inTime;
        if (input < 0)
            input = 0;

        long totalInterval = (long)(m_LoopingDuration * 2);
        if (input > totalInterval)
            m_LocalTime = (unsigned long)totalInterval;
        else
            m_LocalTime = (unsigned long)input;
    } break;
    case TimePolicyModes::PingPong: {
        long totalInterval = (long)(m_LoopingDuration * 2);
        long input = (long)inTime;
        while (input < 0)
            input += totalInterval;
        m_LocalTime = (unsigned long)(input % totalInterval);
    } break;
    default:
        QT3DS_ASSERT(false);
    }
}

//==============================================================================
/**
 *	Retrieves the looping duration.
 *	@see CTimePolicy::SetLoopingDuration( )
 *	@return TTimeUnit looping duration
 */
TTimeUnit CTimePolicy::GetLoopingDuration() const
{
    return m_LoopingDuration;
}

//==============================================================================
/**
 *	Set the looping duration.
 *	Looping duration is defined as the time it takes before stopping, pinging or
 *	looping.
 *	@param inTime	looping duration
 */
void CTimePolicy::SetLoopingDuration(const TTimeUnit inTime)
{
    m_LoopingDuration = (unsigned long)inTime;
}

//==============================================================================
/**
 *	Return the paused state.
 *	@return BOOL pause state
 */
BOOL CTimePolicy::GetPaused() const
{
    return m_Paused;
}

BOOL CTimePolicy::GetPlayBackDirection() const
{
    bool reversePlaybackDirection = m_Backward;
    if (m_Rate < 0)
        reversePlaybackDirection = !reversePlaybackDirection;

    bool retval = m_LocalTime < m_LoopingDuration;

    if (reversePlaybackDirection)
        retval = !retval;
    return retval;
}

//==============================================================================
/**
 *	Sets the paused state
 *	@param inPaused	pause state
 */
void CTimePolicy::SetPaused(const BOOL inPaused)
{
    m_Paused = static_cast<UINT8>(inPaused);
}

BOOL CTimePolicy::UpdateTime(const TTimeUnit inTime)
{
    if (fabs(m_Rate) > .001f)
        m_LocalTime =
            static_cast<unsigned long>(((unsigned long)(inTime + m_Offset)) * fabs(m_Rate));
    // else we are effectively stopped and we can't update the time.

    if (m_LoopingDuration == 0)
        return FALSE;

    unsigned long maxUsefulDuration = m_LoopingDuration * 2;
    BOOL retval = 0;
    switch (m_TimePolicyMode) {
    case TimePolicyModes::StopAtEnd:
        if (m_LocalTime > m_LoopingDuration) {
            retval = 1;
            m_LocalTime = m_LoopingDuration;
        }
        break;
    case TimePolicyModes::Looping:
        m_LocalTime = m_LocalTime % m_LoopingDuration;
        break;
    case TimePolicyModes::Ping:
        if (m_LocalTime > maxUsefulDuration) {
            retval = 1;
            m_LocalTime = maxUsefulDuration;
        }
        break;
    case TimePolicyModes::PingPong:
        m_LocalTime = m_LocalTime % maxUsefulDuration;
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }
    return retval;
}
//==============================================================================
/**
 *	Compute the local time based off the current global time and the settings
 *	such as duration, repetitions and ping-pong.
 *	@param inTime	global time being fed
 *	@return BOOL	true if policy has reached it's end time
 */
BOOL CTimePolicy::ComputeTime(const TTimeUnit inTime)
{
    // Return the last computed time if paused
    if (m_Paused || m_OffsetInvalid) {
        // m_LocalTime = inTime + m_Offset;
        if (fabs(m_Rate) > .001)
            m_Offset = (TTimeUnit)(m_LocalTime / fabs(m_Rate)) - inTime;
        else // rate is zero, this should not happen.
            m_Offset = (TTimeUnit)(m_LocalTime)-inTime;
        m_OffsetInvalid = 0;
        return false;
    }
    return UpdateTime(inTime);
}

eastl::pair<BOOL, TTimeUnit>
SComponentTimePolicyOverride::ComputeLocalTime(CTimePolicy &inTimePolicy, TTimeUnit inGlobalTime)
{
    TTimeUnit retval = 0;
    BOOL finished = FALSE;
    if (inTimePolicy.m_Paused || inTimePolicy.m_OffsetInvalid) {
        finished = inTimePolicy.ComputeTime(inGlobalTime);
    } else {
        Q3DStudio_ASSERT(m_EndTime <= inTimePolicy.GetLoopingDuration());
        unsigned long newLocalTime = (unsigned long)(inGlobalTime + inTimePolicy.m_Offset);

        float diff = static_cast<float>(newLocalTime - inTimePolicy.GetTime());
        diff *= m_TimeMultiplier;
        float updatedLocalTime = static_cast<float>(inTimePolicy.m_LocalTime) + diff;

        if (updatedLocalTime < 0.0f)
            updatedLocalTime = 0.0f;

        inTimePolicy.m_LocalTime = static_cast<unsigned long>(updatedLocalTime);

        bool runningBackwards = m_TimeMultiplier < 0;

        if (runningBackwards) {
            if (inTimePolicy.m_LocalTime <= m_EndTime) {
                finished = TRUE;
                inTimePolicy.m_LocalTime = (unsigned long)m_EndTime;
            }
        } else // running forwards
        {
            if (inTimePolicy.m_LocalTime >= m_EndTime) {
                finished = TRUE;
                inTimePolicy.m_LocalTime = (unsigned long)m_EndTime;
            }
        }
        inTimePolicy.m_Offset = inTimePolicy.m_LocalTime - inGlobalTime;
    }
    retval = inTimePolicy.GetTime();
    return eastl::make_pair(finished, retval);
}

void SComponentTimePolicyOverride::SetTime(CTimePolicy &inTimePolicy, TTimeUnit inLocalTime)
{
    if (inLocalTime < 0)
        inLocalTime = 0;
    if (m_TimeMultiplier < 0) {
        if (inLocalTime <= m_EndTime) {
            inLocalTime = m_EndTime;
        }
    } else // running forwards
    {
        if (inLocalTime >= m_EndTime) {
            inLocalTime = m_EndTime;
        }
    }
    inTimePolicy.SetTime(inLocalTime);
}

} // namespace Q3DStudio
