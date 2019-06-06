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

#include "SystemPrefix.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSDataLogger.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

CDataLogger::SExternalFunctors CDataLogger::m_Functors;
CDataLogger::EDataLoggerLevel CDataLogger::m_LogLevel = CDataLogger::LOG_LEVEL_INVALID;
BOOL CDataLogger::m_Enabled = false;

// Developers - change out the = NULL to = class::LogEntry to log specific stuff.
template <>
TPerfLogGeneralEvent1::TLogEntryInternal TPerfLogGeneralEvent1::m_InternalLogFunc = NULL;
template <>
TPerfLogGeneralEvent2::TLogEntryInternal TPerfLogGeneralEvent2::m_InternalLogFunc = NULL;

template <>
TPerfLogRenderEvent1::TLogEntryInternal TPerfLogRenderEvent1::m_InternalLogFunc = NULL;
template <>
TPerfLogSceneEvent1::TLogEntryInternal TPerfLogSceneEvent1::m_InternalLogFunc = NULL;

template <>
TPerfLogMathEvent1::TLogEntryInternal TPerfLogMathEvent1::m_InternalLogFunc = NULL;
template <>
TPerfLogPresentationEvent1::TLogEntryInternal TPerfLogPresentationEvent1::m_InternalLogFunc = NULL;
template <>
TPerfLogRenderEvent2::TLogEntryInternal TPerfLogRenderEvent2::m_InternalLogFunc = NULL;

//==============================================================================
/**
 *	Set the application supplied functors to use for getting time and handling
 *  the writing out of data
 */
void CDataLogger::SetFunctors(SExternalFunctors inFunctors)
{
    m_Functors = inFunctors;
}

//==============================================================================
/**
*	Call into the specified functors to make stuff happen
*/
void CDataLogger::LogEntry(UINT32 inEntryEnum, BOOL inStartFlag)
{
    if (m_Enabled && m_Functors.m_GetTimeFunc && m_Functors.m_WriteFunc && m_Functors.m_UserData) {
        // Initialize to the correct values
        SPerfLogEntry theEntry(inEntryEnum, inStartFlag,
                               m_Functors.m_GetTimeFunc(m_Functors.m_UserData));
        m_Functors.m_WriteFunc(m_Functors.m_UserData, theEntry);
    }
}

//==============================================================================
/**
*	Enable logging
*/
void CDataLogger::Enable()
{
    SetLogLevel(m_LogLevel);
    m_Enabled = true;
}

//==============================================================================
/**
*	Disable logging
*/
void CDataLogger::Disable()
{
    m_Enabled = false;
}

//==============================================================================
/**
 *	XXX
 */
void CDataLogger::SetLogLevel(EDataLoggerLevel inLogLevel)
{
    switch (inLogLevel) {
    // No logging
    case LOG_LEVEL_NONE:
        TPerfLogGeneralEvent1::Disable();
        TPerfLogGeneralEvent2::Disable();

        TPerfLogRenderEvent1::Disable();
        TPerfLogSceneEvent1::Disable();

        TPerfLogMathEvent1::Disable();
        TPerfLogPresentationEvent1::Disable();
        TPerfLogRenderEvent2::Disable();
        break;

    case LOG_LEVEL_3:
        TPerfLogMathEvent1::Enable();
        TPerfLogPresentationEvent1::Enable();
        TPerfLogRenderEvent2::Enable();

    case LOG_LEVEL_2:
        TPerfLogRenderEvent1::Enable();
        TPerfLogSceneEvent1::Enable();

    case LOG_LEVEL_1:
        TPerfLogGeneralEvent1::Enable();
        TPerfLogGeneralEvent2::Enable();
        break;

    case LOG_LEVEL_INVALID:
    default:
        m_LogLevel = LOG_LEVEL_INVALID;
        return;
        break;
    }
    m_LogLevel = inLogLevel;
}

} // namespace Q3DStudio
