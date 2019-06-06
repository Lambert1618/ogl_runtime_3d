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
#include "Qt3DSMemoryManager.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Static facade to memory statistics and allocation tracking data.
 *
 *	Use this class to examine the memory state of the system at any point.
 *	Heap, managers and pools are are connected to this point by registering
 *	themselves on creation.  Initiate the extraction of the collected data
 *	using this interface.
 */
class CMemoryStatistics
{
    //==============================================================================
    //	Constants
    //==============================================================================
public:
    const static INT32 REPORTSIZE = 256; ///< Max size of single report string
    const static INT32 MANAGERCOUNT = 5; ///< Fixed number of managers tracked
    const static INT32 HISTOGRAMCOUNT = 10; ///< Fixed number of managers tracked

    //==============================================================================
    //	Structs
    //==============================================================================
public:
    /// One row of statistics on allocation
    struct SFactoid
    {
        CHAR m_Name[32]; ///< Ascii identifier - manager name or pool description
        INT32 m_Bytes; ///< Bytes used - full pool size for example - zero if factoid unused
        INT32 m_Calls; ///< Function calls, i.e number of allocations
        INT32 m_Used; ///< Discrete chunk usage in percent - 10 used of 200 gives a value 5
        INT32 m_Align; ///< Packed efficiency in percent - all 24byte structs in 32byte pool give a
                       ///value 75
        INT32 m_Miss; ///< Allocation misses - due to full pools for example
    };

    /// Tuple describing one data point in a histogram
    struct SHistogramScore
    {
        INT32 m_Size; ///< Allocation request in bytes
        INT32 m_Count; ///< Number of allocation requests
    };

    /// Set of factoids describing a system memory state
    struct SFactSheet
    {
        SFactoid m_HeapState; ///< Heap facts
        SFactoid m_ManagerState; ///< Combined manager facts
        SFactoid m_Manager[MANAGERCOUNT]; ///< Individual manager facts
        SHistogramScore m_Histogram[MANAGERCOUNT][HISTOGRAMCOUNT]; ///< Top allocation requests
                                                                   ///sorted by bytes, global and
                                                                   ///never reset
        SFactoid m_Pool[MANAGERCOUNT][Q3DStudio_MEMORY_POOLCOUNT]; ///< Individual pool facts
    };

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    static INT32 s_Overhead; ///< Memory tracking structures report in here
    static CMemoryManager
        *s_PoolManagers[MANAGERCOUNT]; ///< Pointers to all created memory pool managers

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Registration
    static void AddManager(CMemoryManager *inManager);
    static void RemoveManager(CMemoryManager *inManager);
    static INT32 &Overhead();

public: // Access
    static void Reset();
    static void GetFacts(SFactSheet &outFacts, const BOOL inPeak = false,
                         const BOOL inGlobal = false);
    static INT32 GetRequestedBytes(const BOOL inPeak = false);
    static INT32 GetHeapBytes(const BOOL inPeak = false);

public: // Reporting
    static void SimpleReport(IStream *inStream = NULL);
    static void HeapReport(IStream *inStream = NULL);
    static void PoolReport(const BOOL inPeak = false, const BOOL inGlobal = false,
                           IStream *inStream = NULL);
    static void LineReport(IStream *inStream = NULL);

    static void FullReport(IStream *inStream = NULL);
    static void Report(IStream *inStream, const CHAR *inString);

protected: // Utility
    static void CullHistogram(const CMemoryProbe::SValue *inHistogram, const EMemoryValue inValue,
                              SHistogramScore outResults[HISTOGRAMCOUNT]);
    static int CompareHeap(const void *arg1, const void *arg2);
    static int CompareHistogram(const void *arg1, const void *arg2);
};

} // namespace Q3DStudio
