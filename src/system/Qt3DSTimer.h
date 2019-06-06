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
#include "Qt3DSITimer.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
/**
 *	Time provider interface abstracting time from the usage of time.
 */
class ITimeProvider
{
protected:
    virtual ~ITimeProvider() {}
public:
    virtual INT64 GetCurrentTimeMicroSeconds() = 0;
};

// Pause in a way that is completely transparent to the underlying system.
class CPausingTimeProvider : public ITimeProvider
{
    ITimeProvider &m_Provider;
    INT64 m_PauseOffset;
    INT64 m_Offset;
    bool m_Paused;

public:
    CPausingTimeProvider(ITimeProvider &inProvider)
        : m_Provider(inProvider)
        , m_PauseOffset(0)
        , m_Offset(0)
        , m_Paused(false)
    {
    }

    INT64 GetCurrentTimeMicroSeconds() override
    {
        INT64 currentTime = 0;
        if (m_Paused)
            currentTime = m_PauseOffset;
        else
            currentTime = m_Provider.GetCurrentTimeMicroSeconds();

        return currentTime - m_Offset;
    }

    void Pause()
    {
        if (!m_Paused) {
            m_Paused = true;
            m_PauseOffset = m_Provider.GetCurrentTimeMicroSeconds();
        }
    }

    void UnPause()
    {
        if (m_Paused) {
            m_Paused = false;
            INT64 timePaused = m_Provider.GetCurrentTimeMicroSeconds() - m_PauseOffset;
            m_Offset += timePaused;
        }
    }

    bool IsPaused() { return m_Paused; }

    void Reset() { m_PauseOffset = m_Offset = 0; }
};

//==============================================================================
/**
 *	@class	CTimer
 *	@brief	A very simple platform optimized utility class to return lapsed time
 *
 *	Gets current time based on the time lapsed from the point where timer is started.
 */

class CTimer : public ITimer
{

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    ITimeProvider &m_TimeProvider;
    INT64 m_StartTime;
    BOOL m_IsRunning; // Whether this timer is running or stopped

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CTimer(ITimeProvider &inProvider);

public: // Operation
    void Start() override;
    void Stop() override;
    void Reset() override;
    TTimeUnit GetTimeMilliSecs() override;
    TMicroSeconds GetTimeMicroSecs() override;
};

} // namespace Q3DStudio
