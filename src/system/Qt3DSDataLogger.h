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

#ifndef _DATA_LOGGER_H_
#define _DATA_LOGGER_H_

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSTypes.h"
#include "Qt3DSDataLoggerEnums.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

typedef TTimeUnit TDataLoggerTimeUnit;

//==============================================================================
/**
*	The structure that holds all the info we collect
*/
struct SPerfLogEntry
{
    UINT32 m_EntryEnum : 31;
    UINT32 m_StartBool : 1;

    TDataLoggerTimeUnit m_Time;

    SPerfLogEntry(UINT32 inEntryEnum, BOOL inStartFlag, TDataLoggerTimeUnit inTime)
        : m_EntryEnum(inEntryEnum)
        , m_StartBool(inStartFlag ? 1U : 0U)
        , m_Time(inTime)
    {
        // Make sure our enums never use the high bit
        Q3DStudio_ASSERT(false == (inEntryEnum & (1 << 31)));
    }
};

//==============================================================================
/**
*	The stack based helper class that wraps the actually logging of start and end messages
*	for use with start/end time logging.
*/
template <typename T>
class CPerfLogPairedEventWrapper
{
public:
    typedef void (*TLogEntryInternal)(UINT32 inEntryEnum, BOOL inStartFlag);

private:
    UINT32 m_EntryEnum;
    static TLogEntryInternal m_InternalLogFunc;

private:
    static void LogEntry(UINT32 inEntryEnum, BOOL inStartFlag);

public:
    CPerfLogPairedEventWrapper(UINT32 inEntryEnum);
    ~CPerfLogPairedEventWrapper();

    static void Enable();
    static void Disable();
};

//==============================================================================
/**
*/
class CPerfLogPairedEventWrapperDummy
{
public:
    CPerfLogPairedEventWrapperDummy(UINT32 /*inEntryEnum*/) {}
    ~CPerfLogPairedEventWrapperDummy() {}
};

// Dummy structs, so we get one 'type' of class per log event
// so we can turn things on and off
struct SPerfLogGeneralEvent1
{
};
struct SPerfLogGeneralEvent2
{
};

struct SPerfLogRenderEvent1
{
};
struct SPerfLogSceneEvent1
{
};

struct SPerfLogMathEvent1
{
};
struct SPerfLogPresentationEvent1
{
};
struct SPerfLogRenderEvent2
{
};

// level 0
// no profiling at all...

// level 1
typedef CPerfLogPairedEventWrapper<SPerfLogGeneralEvent1>
    TPerfLogGeneralEvent1; // Frame, update presentations, update scene, render scene
typedef CPerfLogPairedEventWrapper<SPerfLogGeneralEvent2>
    TPerfLogGeneralEvent2; // Event processing and manager updates,
// call frame and slide callbacks,
// opaque, transparent, layer render, finalize drawlist and pick
// level 2
typedef CPerfLogPairedEventWrapper<SPerfLogRenderEvent1>
    TPerfLogRenderEvent1; // More detailed rendering
typedef CPerfLogPairedEventWrapper<SPerfLogSceneEvent1>
    TPerfLogSceneEvent1; // More detailed scene update

// level 3
typedef CPerfLogPairedEventWrapper<SPerfLogMathEvent1> TPerfLogMathEvent1; // vector and matrix math
typedef CPerfLogPairedEventWrapper<SPerfLogPresentationEvent1>
    TPerfLogPresentationEvent1; // More detailed scene update
typedef CPerfLogPairedEventWrapper<SPerfLogRenderEvent2> TPerfLogRenderEvent2; //

// Defines
#ifdef _PERF_LOG
#define PerfLogGeneralEvent1(inEventID) TPerfLogGeneralEvent1 thePerfLog(inEventID);
#define PerfLogGeneralEvent2(inEventID) TPerfLogGeneralEvent2 thePerfLog(inEventID);

#define PerfLogRenderEvent1(inEventID) TPerfLogRenderEvent1 thePerfLog(inEventID);
#define PerfLogSceneEvent1(inEventID) TPerfLogSceneEvent1 thePerfLog(inEventID);

#define PerfLogMathEvent1(inEventID) TPerfLogMathEvent1 thePerfLog(inEventID);
#define PerfLogPresentationEvent1(inEventID) TPerfLogPresentationEvent1 thePerfLog(inEventID);
#define PerfLogRenderEvent2(inEventID) TPerfLogRenderEvent2 thePerfLog(inEventID);
#else
#define PerfLogGeneralEvent1(inEventID)
#define PerfLogGeneralEvent2(inEventID)

#define PerfLogRenderEvent1(inEventID)
#define PerfLogSceneEvent1(inEventID)

#define PerfLogMathEvent1(inEventID)
#define PerfLogPresentationEvent1(inEventID)
#define PerfLogRenderEvent2(inEventID)
#endif

//==============================================================================
/**
*/
class CDataLogger
{
private:
    // App level function to write this out to a stream/file... set externally
    typedef void (*TDataLoggerWriteFunc)(void *inUserData, SPerfLogEntry inEntry);
    typedef TDataLoggerTimeUnit (*TGetTimeFunc)(void *inUserData);

public:
    struct SExternalFunctors
    {
        void *m_UserData;
        TDataLoggerWriteFunc m_WriteFunc;
        TGetTimeFunc m_GetTimeFunc;

        SExternalFunctors()
            : m_UserData(NULL)
            , m_WriteFunc(NULL)
            , m_GetTimeFunc(NULL)
        {
        }
    };

    enum EDataLoggerLevel {
        LOG_LEVEL_NONE,
        LOG_LEVEL_1,
        LOG_LEVEL_2,
        LOG_LEVEL_3,
        LOG_LEVEL_INVALID, // use this if you want to selectively enable or disable loggers
    };

private:
    static SExternalFunctors m_Functors;
    static EDataLoggerLevel m_LogLevel;
    static BOOL m_Enabled;

public:
    static void SetFunctors(SExternalFunctors inFunctors);
    static void LogEntry(UINT32 inEntryEnum, BOOL inStartFlag);
    static void Enable();
    static void Disable();

    static void SetLogLevel(EDataLoggerLevel inLogLevel);
};

} // namespace Q3DStudio

#include "Qt3DSDataLogger.hpp"

#endif // _DATA_LOGGER_H_
