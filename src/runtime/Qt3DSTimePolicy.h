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
#include "foundation/Qt3DSOption.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

struct TimePolicyModes
{
    enum Enum {
        StopAtEnd = 0,
        Looping,
        PingPong,
        Ping,
    };
};

//==============================================================================
/**
 *	Filter time based of parameters such as loop, duration, ping-pong etc.
 *
 *	The time policy is an agnostic class that transforms time units based on
 *	internal settings and times given to it.  Think of it as the minimal
 *	intelligence needed to support the time playback modes we use. It is simply
 *	a function that takes in a source time variable, adjusts it accordingly,
 *	and returns a filtered version.
 */

QT3DS_ALIGN_PREFIX(4) class CTimePolicy
{
    //==============================================================================
    //	Fields and Constants
    //==============================================================================
public:
    const static TTimeUnit FOREVER; ///< Forever repetitions means loop forever, Forever duration
                                    ///means infinite duration
    friend struct SComponentTimePolicyOverride; ///< This class needs to mangle some of our internal
                                                ///members to keep everything consistent

protected:
    // Time policy related ( 20 bytes )
    unsigned long m_LocalTime; ///< Component current local time; transformed by playmodes
    unsigned long m_LoopingDuration; ///< Looping duration
    SAlignedTimeUnit m_Offset; ///< Subtraction modifier
    FLOAT m_Rate; ///< Time policy rate
    UINT8 m_TimePadding[12]; ///< Padding to keep the size of the time policy the same.

    UINT8
        m_TimePolicyMode : 2; ///< Number of durations before halting - FOREVER means infinite reps
    UINT8 m_Paused : 1; ///< Paused or playing
    UINT8 m_OffsetInvalid : 1; ///< True if we need to reset our offset
    UINT8 m_Backward : 1; ///< True if are in 'reverse' mode

    UINT8 m_Unused[3]; ///< (Padding 3 bytes)

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    void Initialize(const TTimeUnit inLoopingDuration = FOREVER,
                    TimePolicyModes::Enum inMode = TimePolicyModes::StopAtEnd);
    void Initialize(const TTimeUnit inLoopingDuration, UINT32 inRepetitions, BOOL inPingPong);

public: // Access Methods
    FLOAT GetRate() const { return m_Rate; }
    void SetRate(float inRate) { m_Rate = inRate; }

    bool IsReverse() const { return m_Backward; }
    void SetReverse(bool inIsReverse) { m_Backward = inIsReverse ? 1 : 0; }

    TTimeUnit GetTime() const;
    void SetTime(TTimeUnit inTime);

    TTimeUnit GetLoopingDuration() const;
    void SetLoopingDuration(const TTimeUnit inTime);

    BOOL GetPaused() const;
    void SetPaused(const BOOL inPaused);

    BOOL GetPlayBackDirection() const;

public: // Time update
    // The addendum contains extra parameters used that can be specified in a goto-slide command
    // in the uia file.
    BOOL ComputeTime(const TTimeUnit inTime);

private:
    TTimeUnit InternalGetTime() const;

    BOOL UpdateTime(const TTimeUnit inTime);
} QT3DS_ALIGN_SUFFIX(4);

} // namespace Q3DStudio
