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
#include "Qt3DSIStream.h"
#include "Qt3DSMemoryStatistics.h"
#include "Qt3DSMemoryTracker.h"
#include "Qt3DSMemoryHeap.h"
#include "foundation/Qt3DSLogging.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Static Fields
//==============================================================================
INT32 CMemoryStatistics::s_Overhead = 0;
CMemoryManager *CMemoryStatistics::s_PoolManagers[MANAGERCOUNT] = { NULL };

//==============================================================================
//	REGISTRATION
//==============================================================================

//==============================================================================
/**
 *	Add the manager from the static list done automatically on manager construction.
 *	@param inManager	the manager we are beginning to track
 */
void CMemoryStatistics::AddManager(CMemoryManager *inManager)
{
    INT32 theIndex = 0;
    while (s_PoolManagers[theIndex] && theIndex < MANAGERCOUNT - 1)
        ++theIndex;

    if (!s_PoolManagers[theIndex]) {
        s_PoolManagers[theIndex] = inManager;
    } else {
        qCWarning(qt3ds::TRACE_INFO)
                << "Could not add memory manager tracker. Limit: " << MANAGERCOUNT;
    }
}

//==============================================================================
/**
 *	Remove the manager from the static list done automatically on manager deletion.
 *	@param inManager	the manager we are ceasing to track
 */
void CMemoryStatistics::RemoveManager(CMemoryManager *inManager)
{
    INT32 theIndex = 0;
    while (inManager != s_PoolManagers[theIndex] && theIndex < MANAGERCOUNT - 1)
        ++theIndex;

    if (inManager == s_PoolManagers[theIndex])
        s_PoolManagers[theIndex] = NULL;
}

//==============================================================================
/**
 *	Read/write access to the static tracking field that keeps tab on how
 *	much memory we are spending _tracking_ memory.
 *	@return reference to the field tracking the overhead in bytes
 */
INT32 &CMemoryStatistics::Overhead()
{
    return s_Overhead;
}

//==============================================================================
//	OPERATION
//==============================================================================

//==============================================================================
/**
 *	Reset all memory probes
 */
void CMemoryStatistics::Reset()
{
    qCInfo(qt3ds::TRACE_INFO) << "Resetting Memory Statistics";
    CMemoryHeap::GetProbe().Reset();

    for (INT32 theIndex = 0; theIndex < MANAGERCOUNT; ++theIndex)
        if (s_PoolManagers[theIndex])
            s_PoolManagers[theIndex]->Reset();
}

//==============================================================================
//	ACCESS
//==============================================================================

//==============================================================================
/**
 *	Quick access to memory usage of the first memory manager.
 *	@param inPeak is true if you want the peak, false if you want the current state
 *	@return the current explict usage in bytes
 */
INT32 CMemoryStatistics::GetRequestedBytes(BOOL inPeak /*=false*/)
{
    Q3DStudio_ASSERT(s_PoolManagers[0]);

    EMemoryValue theValue = inPeak ? MEMVALUE_PEAK : MEMVALUE_CURRENT;
    CMemoryManager::SPoolData &theUsageData = s_PoolManagers[0]->GetManagerData();

    return theUsageData.m_Aligned.GetBytes(MEMSCOPE_RESET, theValue)
        + theUsageData.m_Overflow.GetBytes(MEMSCOPE_RESET, theValue);
}

//==============================================================================
/**
 *	Quick access to heap usage.
 *	@param inPeak is true if you want the peak, false if you want the current state
 *	@return the current heap usage in bytes
 */
INT32 CMemoryStatistics::GetHeapBytes(BOOL inPeak /*=false*/)
{
    return Q3DStudio::CMemoryHeap::GetProbe().GetBytes(MEMSCOPE_RESET,
                                                       inPeak ? MEMVALUE_PEAK : MEMVALUE_CURRENT);
}

//==============================================================================
/**
 *	Fill out a SFactSheet with memory statistics.
 *	@param outFacts is filled out with factoids showing the system state
 *	@param inPeak is true if you want the peak, false if you want the current state
 *	@param inGlobal is true if you want values since process start, false since last reset
 */
void CMemoryStatistics::GetFacts(SFactSheet &outFacts, BOOL inPeak /*= true*/,
                                 BOOL inGlobal /*= true*/)
{
    INT32 thePresentationBytes = 0;
    Q3DStudio_UNREFERENCED_PARAMETER(thePresentationBytes);
    Q3DStudio_memset(&outFacts, 0, sizeof(SFactSheet));

    // Translate flags to enumerations
    EMemoryScope theScope = inGlobal ? MEMSCOPE_GLOBAL : MEMSCOPE_RESET;
    EMemoryValue theValue = inPeak ? MEMVALUE_PEAK : MEMVALUE_CURRENT;

    // Capture heap state
    outFacts.m_HeapState.m_Bytes = CMemoryHeap::GetProbe().GetBytes(theScope, theValue);
    outFacts.m_HeapState.m_Calls = CMemoryHeap::GetProbe().GetCalls(theScope, theValue);

#if Q3DStudio_MEMORY_POOLTRACKING
    INT32 theTotalChunks = 0;
    INT32 theTotalBytes = 0;

    // Scan all registered pool managers
    Q3DStudio_sprintf(outFacts.m_ManagerState.m_Name, 32, "All Managers");
    for (INT32 theManagerIndex = 0; theManagerIndex < MANAGERCOUNT; ++theManagerIndex) {
        CMemoryManager *theManager = s_PoolManagers[theManagerIndex];
        SFactoid &theManagerFact = outFacts.m_Manager[theManagerIndex];
        INT32 theManagerChunks = 0;
        INT32 theManagerBytes = 0;

        // Ignore empty managers
        if (!theManager)
            break;

        // Scan each pool in the manager
        Q3DStudio_sprintf(theManagerFact.m_Name, 32, "  %s", theManager->GetName());
        for (INT32 thePoolIndex = 0; thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT; ++thePoolIndex) {
            CMemoryPool &thePool = theManager->GetPool(thePoolIndex);
            CMemoryManager::SPoolData *thePoolData = theManager->GetPoolData(thePoolIndex);
            Q3DStudio_ASSERT(thePoolData);

            SFactoid &thePoolFact = outFacts.m_Pool[theManagerIndex][thePoolIndex];
            INT32 theUsed = thePool.GetProbe().GetCalls(theScope, theValue);
            INT32 theAlign = thePoolData->m_Aligned.GetBytes(theScope, theValue);

            // Ignore empty pools
            if (thePool.GetChunkCount() == 0)
                break;

            // Extract all the factoids
            Q3DStudio_sprintf(thePoolFact.m_Name, 32, "    %d: %4db x %4d", thePoolIndex,
                              thePool.GetChunkSize(), thePool.GetChunkCount());
            thePoolFact.m_Bytes = thePool.GetChunkSize() * thePool.GetChunkCount();
            thePoolFact.m_Calls = thePool.GetProbe().GetCalls(theScope, theValue);
            thePoolFact.m_Used = 100 * theUsed / thePool.GetChunkCount();

            thePoolFact.m_Align = 100;
            if (theUsed > 0)
                thePoolFact.m_Align = 100 * theAlign / (theUsed * thePool.GetChunkSize());
            thePoolFact.m_Miss = thePoolData->m_Overflow.GetCalls(theScope, theValue);

            thePresentationBytes += theAlign + thePoolData->m_Overflow.GetBytes(theScope, theValue);

            theManagerFact.m_Bytes += thePoolFact.m_Bytes;
            theManagerFact.m_Calls += thePoolFact.m_Calls;
            theManagerFact.m_Used += theUsed;
            theManagerFact.m_Align += theAlign;
            theManagerFact.m_Miss += thePoolFact.m_Miss;

            outFacts.m_ManagerState.m_Bytes += thePoolFact.m_Bytes;
            outFacts.m_ManagerState.m_Calls += thePoolFact.m_Calls;
            outFacts.m_ManagerState.m_Used += theUsed;
            outFacts.m_ManagerState.m_Align += theAlign;
            outFacts.m_ManagerState.m_Miss += thePoolFact.m_Miss;

            theManagerChunks += thePool.GetChunkCount();
            theManagerBytes += theUsed * thePool.GetChunkSize();
        }

        thePresentationBytes +=
            theManager->GetManagerData().m_Overflow.GetBytes(theScope, theValue);

        // Both the manager sub-total and grand-total need to track bytes separately
        // since the percentages can't just be added or averaged.  Those totals
        // are instead weighted and this is the tracking math that does it.
        theTotalChunks += theManagerChunks;
        theTotalBytes += theManagerBytes;

        theManagerFact.m_Used = 0;
        if (theManagerChunks > 0)
            theManagerFact.m_Used = 100 * theManagerFact.m_Used / theManagerChunks;

        theManagerFact.m_Align = 100;
        if (theManagerBytes > 0)
            theManagerFact.m_Align = 100 * theManagerFact.m_Align / theManagerBytes;

        CullHistogram(theManager->GetHistogram(), theValue, outFacts.m_Histogram[theManagerIndex]);
    }

    // Again this is the grand-total separate percentage computation that has
    // to be done in the end when all sums have been added.
    outFacts.m_ManagerState.m_Used = 0;
    if (theTotalChunks > 0)
        outFacts.m_ManagerState.m_Used = 100 * outFacts.m_ManagerState.m_Used / theTotalChunks;

    outFacts.m_ManagerState.m_Align = 100;
    if (theTotalBytes > 0)
        outFacts.m_ManagerState.m_Align = 100 * outFacts.m_ManagerState.m_Align / theTotalBytes;

#endif // Q3DStudio_MEMORY_POOLTRACKING

    //	return thePresentationBytes;
}

//==============================================================================
/**
 *	Find the HISTOGRAMLIMIT highest performers in the histogram.
 *	@param inHistogram array of CMemoryManager::HISTOGRAMCOUNT values
 *			each showing the number of allocation of that bytesize
 *	@param outResults collects the highest points of the histogram
 */
void CMemoryStatistics::CullHistogram(const CMemoryProbe::SValue *inHistogram,
                                      const EMemoryValue inValue,
                                      SHistogramScore outResults[HISTOGRAMCOUNT])
{
    // Always clear even if there is no data
    Q3DStudio_memset(outResults, 0, sizeof(SHistogramScore));

    // Return quickly when no data
    if (!inHistogram)
        return;

    // Scan full histogram
    INT32 theLowestIndex = 0;
    for (UINT16 theSize = 0; theSize < 512; ++theSize) {
        // Is this count higher than the lowest recorded count?
        if (inHistogram[theSize].m_Value[inValue] > outResults[theLowestIndex].m_Count) {
            outResults[theLowestIndex].m_Count = inHistogram[theSize].m_Value[inValue];
            outResults[theLowestIndex].m_Size = theSize;

            // Find new low in high score
            theLowestIndex = 0;
            for (UINT16 theResult = 1; theResult < HISTOGRAMCOUNT; ++theResult)
                if (outResults[theResult].m_Count < outResults[theLowestIndex].m_Count)
                    theLowestIndex = theResult;
        }
    }

    // Sort histogram - brute force for now so don't call this all the time
    ::qsort(outResults, 10, sizeof(SHistogramScore), CompareHistogram);
}

//==============================================================================
/**
 *	Helper comparison function for CullHistogram
 */
int CMemoryStatistics::CompareHistogram(const void *arg1, const void *arg2)
{
    return reinterpret_cast<const SHistogramScore *>(arg2)->m_Count
        - reinterpret_cast<const SHistogramScore *>(arg1)->m_Count;
}

//==============================================================================
/**
 *	Helper comparison function for CMemoryStatistics::HeapReport
 */
int CMemoryStatistics::CompareHeap(const void *inEntry1, const void *inEntry2)
{
    return reinterpret_cast<const CMemoryHeap::SReportEntry *>(inEntry1)->m_Size
        - reinterpret_cast<const CMemoryHeap::SReportEntry *>(inEntry2)->m_Size;
}

//==============================================================================
//	REPORTING
//==============================================================================

//==============================================================================
/**
 *	Simple memory usage report covering both bytes allocated and call count.
 *	@param inStream	the stream we are sending the report or NULL to use logger
 */
void CMemoryStatistics::SimpleReport(IStream *inStream /*=NULL*/)
{
    CHAR theReportBuffer[REPORTSIZE];
    CMemoryProbe theProbe;

    // Basic info is always available
    Report(inStream, "\n --- RUNTIME SIMPLE MEMORY REPORT ---\n");

    // Only the first manager is reported for now
    Q3DStudio_ASSERT(s_PoolManagers[0]);
    theProbe = s_PoolManagers[0]->GetManagerData().m_Aligned;
    theProbe.Combine(s_PoolManagers[0]->GetManagerData().m_Overflow);
    Report(inStream, "PRESENTATION:    Current        Peak           Alloc          Free\n");
    Q3DStudio_sprintf(theReportBuffer, sizeof(theReportBuffer),
                      "Global:%12dk(%4d)%8dk(%4d)%8dk(%4d)%8dk(%4d)\n",
                      theProbe.GetBytes(MEMSCOPE_GLOBAL, MEMVALUE_CURRENT) / 1024,
                      theProbe.GetCalls(MEMSCOPE_GLOBAL, MEMVALUE_CURRENT),
                      theProbe.GetBytes(MEMSCOPE_GLOBAL, MEMVALUE_PEAK) / 1024,
                      theProbe.GetCalls(MEMSCOPE_GLOBAL, MEMVALUE_PEAK),
                      theProbe.GetBytes(MEMSCOPE_GLOBAL, MEMVALUE_ADDS) / 1024,
                      theProbe.GetCalls(MEMSCOPE_GLOBAL, MEMVALUE_ADDS),
                      theProbe.GetBytes(MEMSCOPE_GLOBAL, MEMVALUE_DELETES) / 1024,
                      theProbe.GetCalls(MEMSCOPE_GLOBAL, MEMVALUE_DELETES));
    Report(inStream, theReportBuffer);
    Q3DStudio_sprintf(theReportBuffer, sizeof(theReportBuffer),
                      "Reset: %12dk(%4d)%8dk(%4d)%8dk(%4d)%8dk(%4d)\n",
                      theProbe.GetBytes(MEMSCOPE_RESET, MEMVALUE_CURRENT) / 1024,
                      theProbe.GetCalls(MEMSCOPE_RESET, MEMVALUE_CURRENT),
                      theProbe.GetBytes(MEMSCOPE_RESET, MEMVALUE_PEAK) / 1024,
                      theProbe.GetCalls(MEMSCOPE_RESET, MEMVALUE_PEAK),
                      theProbe.GetBytes(MEMSCOPE_RESET, MEMVALUE_ADDS) / 1024,
                      theProbe.GetCalls(MEMSCOPE_RESET, MEMVALUE_ADDS),
                      theProbe.GetBytes(MEMSCOPE_RESET, MEMVALUE_DELETES) / 1024,
                      theProbe.GetCalls(MEMSCOPE_RESET, MEMVALUE_DELETES));
    Report(inStream, theReportBuffer);

    // Overflow is memory not serviced by memory pools - removed to make the simple report...
    // simpler
    /*	theProbe = s_PoolManagers[0]->GetManagerData( ).m_Overflow;
            Report( inStream, "OVERFLOW:\n" );
            Q3DStudio_sprintf( theReportBuffer, sizeof( theReportBuffer ),
       "Global:%12ldk(%4ld)%8ldk(%4ld)%8ldk(%4ld)%8ldk(%4ld)\n",
                    theProbe.GetBytes( MEMSCOPE_GLOBAL, MEMVALUE_CURRENT )/1024,
       theProbe.GetCalls( MEMSCOPE_GLOBAL, MEMVALUE_CURRENT ),
                    theProbe.GetBytes( MEMSCOPE_GLOBAL, MEMVALUE_PEAK )/1024,
       theProbe.GetCalls( MEMSCOPE_GLOBAL, MEMVALUE_PEAK ),
                    theProbe.GetBytes( MEMSCOPE_GLOBAL, MEMVALUE_ADDS )/1024,
       theProbe.GetCalls( MEMSCOPE_GLOBAL, MEMVALUE_ADDS ),
                    theProbe.GetBytes( MEMSCOPE_GLOBAL, MEMVALUE_DELETES )/1024,
       theProbe.GetCalls( MEMSCOPE_GLOBAL, MEMVALUE_DELETES ) );
            Report( inStream, theReportBuffer );
            Q3DStudio_sprintf( theReportBuffer, sizeof( theReportBuffer ), "Reset:
       %12ldk(%4ld)%8ldk(%4ld)%8ldk(%4ld)%8ldk(%4ld)\n",
                    theProbe.GetBytes( MEMSCOPE_GLOBAL, MEMVALUE_CURRENT )/1024,
       theProbe.GetCalls( MEMSCOPE_GLOBAL, MEMVALUE_CURRENT ),
                    theProbe.GetBytes( MEMSCOPE_GLOBAL, MEMVALUE_PEAK )/1024,
       theProbe.GetCalls( MEMSCOPE_GLOBAL, MEMVALUE_PEAK ),
                    theProbe.GetBytes( MEMSCOPE_GLOBAL, MEMVALUE_ADDS )/1024,
       theProbe.GetCalls( MEMSCOPE_GLOBAL, MEMVALUE_ADDS ),
                    theProbe.GetBytes( MEMSCOPE_GLOBAL, MEMVALUE_DELETES )/1024,
       theProbe.GetCalls( MEMSCOPE_GLOBAL, MEMVALUE_DELETES ) );
            Report( inStream, theReportBuffer );*/

    // All managers share the same heap
    theProbe = CMemoryHeap::GetProbe();
    Report(inStream, "HEAP:\n");
    Q3DStudio_sprintf(theReportBuffer, sizeof(theReportBuffer),
                      "Global:%12dk(%4d)%8dk(%4d)%8dk(%4d)%8dk(%4d)\n",
                      theProbe.GetBytes(MEMSCOPE_GLOBAL, MEMVALUE_CURRENT) / 1024,
                      theProbe.GetCalls(MEMSCOPE_GLOBAL, MEMVALUE_CURRENT),
                      theProbe.GetBytes(MEMSCOPE_GLOBAL, MEMVALUE_PEAK) / 1024,
                      theProbe.GetCalls(MEMSCOPE_GLOBAL, MEMVALUE_PEAK),
                      theProbe.GetBytes(MEMSCOPE_GLOBAL, MEMVALUE_ADDS) / 1024,
                      theProbe.GetCalls(MEMSCOPE_GLOBAL, MEMVALUE_ADDS),
                      theProbe.GetBytes(MEMSCOPE_GLOBAL, MEMVALUE_DELETES) / 1024,
                      theProbe.GetCalls(MEMSCOPE_GLOBAL, MEMVALUE_DELETES));
    Report(inStream, theReportBuffer);
    Q3DStudio_sprintf(theReportBuffer, sizeof(theReportBuffer),
                      "Reset: %12dk(%4d)%8dk(%4d)%8dk(%4d)%8dk(%4d)\n",
                      theProbe.GetBytes(MEMSCOPE_RESET, MEMVALUE_CURRENT) / 1024,
                      theProbe.GetCalls(MEMSCOPE_RESET, MEMVALUE_CURRENT),
                      theProbe.GetBytes(MEMSCOPE_RESET, MEMVALUE_PEAK) / 1024,
                      theProbe.GetCalls(MEMSCOPE_RESET, MEMVALUE_PEAK),
                      theProbe.GetBytes(MEMSCOPE_RESET, MEMVALUE_ADDS) / 1024,
                      theProbe.GetCalls(MEMSCOPE_RESET, MEMVALUE_ADDS),
                      theProbe.GetBytes(MEMSCOPE_RESET, MEMVALUE_DELETES) / 1024,
                      theProbe.GetCalls(MEMSCOPE_RESET, MEMVALUE_DELETES));
    Report(inStream, theReportBuffer);

    Q3DStudio_sprintf(theReportBuffer, sizeof(theReportBuffer), "Tracking overhead: %dk\n",
#if Q3DStudio_MEMORY_HEAPTRACKING
                      (s_Overhead + sizeof(CMemoryHeap::TMemoryReport)) / 1024);
#else
                      s_Overhead / 1024);
#endif // Q3DStudio_MEMORY_HEAPTRACKING

    Report(inStream, theReportBuffer);
}

//==============================================================================
/**
 *	Output the memory report log.
 *
 *  The is always watched by a CMemoryProbe that records all activity.  Most
 *	individual allocations will not be recorded though since they are handled
 *	by memory pools.
 *
 * A probe records three axis of information (value, scope and call/bytes).
 *
 * # Value (current,peak,add,del) tracks the current value along with its
 * peak value, and also separately tracks adds(allocations) and deletes(frees)
 * such that current = add - del.
 * # Scope (local,global) tracks values since last reset or dawn of heap.
 * # Call/Bytes tracks separately bytes and calls so you can distinguish between
 * many small allocations and a few large.
 *
 * @param inStream	the stream we are sending the report or NULL to use logger
 */
void CMemoryStatistics::HeapReport(IStream *inStream /*=NULL*/)
{
    Report(inStream, "\n --- RUNTIME HEAP MEMORY REPORT ---\n");

#if Q3DStudio_MEMORY_HEAPTRACKING
    // Detailed info needs Q3DStudio_MEMORY_HEAPTRACKING defined
    INT32 theCount = CMemoryHeap::GetReport()->GetCount();
    ::qsort(&CMemoryHeap::GetReport()[0], static_cast<size_t>(theCount),
            sizeof(CMemoryHeap::SReportEntry), CompareHeap);

    CHAR theReportBuffer[REPORTSIZE];
    for (INT32 theIndex = 0; theIndex < theCount; ++theIndex) {
        CMemoryHeap::SReportEntry &theEntry = CMemoryHeap::GetReport()->operator[](theIndex);
        Q3DStudio_sprintf(theReportBuffer, sizeof(theReportBuffer),
                          "%3d. %8d bytes - \"%s\" - %s (%d)\n", theIndex, theEntry.m_Size,
                          theEntry.m_Description, theEntry.m_File, theEntry.m_Line);
        Report(inStream, theReportBuffer);
    }
#else
    Report(inStream, " Unavailable: Q3DStudio_MEMORY_HEAPTRACKING not defined in this build\n");
#endif
}

/* Example output of CMemoryStatistics::Report

=POOL=					HEAP	CALLS	USED	ALIGN	MISSES
---------------------------------------------------------------
All Managers			722k	38		72%		19%

  Global                482k
     0: 16b x 1024		16k		2182	 3%		13%
         1: 32b x 2048		64k		4123	96%		 9%		38
         2: 48b x 512		24k		212		72%		21%
         3: 64b x 512		32k		454		11%		 3%
      .
        8: 512b x 128		64k		68		64%		22%
*/

//==============================================================================
/**
 *	Return a text buffer describing the state of the memory system.
 */
void CMemoryStatistics::PoolReport(BOOL inPeak /*= true*/, BOOL inGlobal /*= true*/,
                                   IStream *inStream /*=NULL*/)
{
    CHAR theLine[256] = { 0 };
    SFactSheet theFacts;

    GetFacts(theFacts, inPeak, inGlobal);

    Report(inStream, "\n --- RUNTIME POOL MEMORY REPORT ---\n");
    Q3DStudio_sprintf(theLine, 256,
                      "Mode:           %s %s\nTotal Heap:   %7dk(%d)\nPresentation: %7dk\n",
                      inGlobal ? "GLOBAL" : "RESET", inPeak ? "PEAK" : "CURRENT",
                      theFacts.m_HeapState.m_Bytes / 1024, theFacts.m_HeapState.m_Calls,
                      GetRequestedBytes(inPeak) / 1024);
    Report(inStream, theLine);

#if Q3DStudio_MEMORY_POOLTRACKING
    Report(inStream, "=POOLS=                  HEAP     HITS   USED  ALIGN   MISS"
                     "\n");
    Report(inStream, "-----------------------------------------------------------------"
                     "\n");
    Q3DStudio_sprintf(theLine, 256, "All Managers         %7dk %8d %5d%% %5d%% %6d"
                                    "\n\n",
                      theFacts.m_ManagerState.m_Bytes / 1024, theFacts.m_ManagerState.m_Calls,
                      theFacts.m_ManagerState.m_Used, theFacts.m_ManagerState.m_Align,
                      theFacts.m_ManagerState.m_Miss);
    Report(inStream, theLine);

    for (INT32 theManagerIndex = 0; theManagerIndex < MANAGERCOUNT; ++theManagerIndex) {
        SFactoid &theManagerFact = theFacts.m_Manager[theManagerIndex];
        if (theManagerFact.m_Bytes > 0) {
            Q3DStudio_sprintf(theLine, 256, "%-20s %7dk %8d %5d%% %5d%% %6d\n",
                              theManagerFact.m_Name, theManagerFact.m_Bytes / 1024,
                              theManagerFact.m_Calls, theManagerFact.m_Used, theManagerFact.m_Align,
                              theManagerFact.m_Miss);
            Report(inStream, theLine);

            for (INT32 thePoolIndex = 0; thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT;
                 ++thePoolIndex) {
                SFactoid &thePoolFact = theFacts.m_Pool[theManagerIndex][thePoolIndex];
                if (theManagerFact.m_Bytes > 0) {
                    Q3DStudio_sprintf(theLine, 256, "%-20s %7dk %8d %5d%% %5d%% %6d\n",
                                      thePoolFact.m_Name, thePoolFact.m_Bytes / 1024,
                                      thePoolFact.m_Calls, thePoolFact.m_Used, thePoolFact.m_Align,
                                      thePoolFact.m_Miss);
                    Report(inStream, theLine);
                }
            }

            // Histogram only valid for Current-Reset
            if (inGlobal) {
                Report(inStream, "    Histogram: Only available in RESET mode\n");
            } else {
                Q3DStudio_sprintf(
                    theLine, 256,
                    "    Histogram: 1:%3db(%3d) 2:%3db(%3d) 3:%3db(%3d) 4:%3db(%3d)  5:%3db(%3d)\n",
                    theFacts.m_Histogram[theManagerIndex][0].m_Size,
                    theFacts.m_Histogram[theManagerIndex][0].m_Count,
                    theFacts.m_Histogram[theManagerIndex][1].m_Size,
                    theFacts.m_Histogram[theManagerIndex][1].m_Count,
                    theFacts.m_Histogram[theManagerIndex][2].m_Size,
                    theFacts.m_Histogram[theManagerIndex][2].m_Count,
                    theFacts.m_Histogram[theManagerIndex][3].m_Size,
                    theFacts.m_Histogram[theManagerIndex][3].m_Count,
                    theFacts.m_Histogram[theManagerIndex][4].m_Size,
                    theFacts.m_Histogram[theManagerIndex][4].m_Count);
                Report(inStream, theLine);
                Q3DStudio_sprintf(
                    theLine, 256,
                    "               6:%3db(%3d) 7:%3db(%3d) 8:%3db(%3d) 9:%3db(%3d) 10:%3db(%3d)\n",
                    theFacts.m_Histogram[theManagerIndex][5].m_Size,
                    theFacts.m_Histogram[theManagerIndex][5].m_Count,
                    theFacts.m_Histogram[theManagerIndex][6].m_Size,
                    theFacts.m_Histogram[theManagerIndex][6].m_Count,
                    theFacts.m_Histogram[theManagerIndex][7].m_Size,
                    theFacts.m_Histogram[theManagerIndex][7].m_Count,
                    theFacts.m_Histogram[theManagerIndex][8].m_Size,
                    theFacts.m_Histogram[theManagerIndex][8].m_Count,
                    theFacts.m_Histogram[theManagerIndex][9].m_Size,
                    theFacts.m_Histogram[theManagerIndex][9].m_Count);
                Report(inStream, theLine);
            }
        }
    }
#else
    Report(inStream, " Unavailable: Q3DStudio_MEMORY_POOLTRACKING not defined in this build\n");
#endif
}

//==============================================================================
/**
 *	Dump the low-level memory list stored in each manager's hash bin.
 *
 * @note Q3DStudio_MEMORY_LINETRACKING has to be set to 1 in AKMemorySettings.h or
 *	the compiler for this method to save out any data.
 *
 * @param inStream	the stream we are sending the report or NULL to use logger
 */
void CMemoryStatistics::LineReport(IStream *inStream /*=NULL*/)
{
    Report(inStream, "\n--- RUNTIME LINE MEMORY REPORT ---\n");
#if Q3DStudio_MEMORY_LINETRACKING
    Report(inStream, "ADDR, SIZE, TYPE, LINE, FILE, \n");

    INT32 theTotalBytes = 0;
    for (INT32 theManagerIndex = 0; theManagerIndex < MANAGERCOUNT; ++theManagerIndex)
        if (s_PoolManagers[theManagerIndex] && s_PoolManagers[theManagerIndex]->GetLineTracker())
            theTotalBytes += s_PoolManagers[theManagerIndex]->GetLineTracker()->Report(inStream);

    CHAR theTotal[64];
    theTotal[0] = '\0';

    Q3DStudio_sprintf(theTotal, sizeof(theTotal), "TOTAL:    , %8d\n", theTotalBytes);
    Report(inStream, theTotal);
#else
    Report(inStream, " Unavailable: Q3DStudio_MEMORY_LINETRACKING not defined in this build\n");
#endif // Q3DStudio_MEMORY_LINETRACKING
}

//==============================================================================
/**
 *	Output to log or stream if available.
 *	@param inMessage string to be reported
 *	@param inStream stream to be used or output to log if NULL
 */
void CMemoryStatistics::Report(IStream *inStream, const CHAR *inMessage)
{
    if (inStream)
        inStream->WriteRaw(inMessage, (INT32)::strlen(inMessage));
    else
        qCInfo(qt3ds::TRACE_INFO) << inMessage;
}

//==============================================================================
/**
 *	Output full memory report to log or stream if available.
 *	@param inStream stream to be used or output to log if NULL
 */
void CMemoryStatistics::FullReport(IStream *inStream /*=NULL*/)
{
    SimpleReport(inStream);
    HeapReport(inStream);
    LineReport(inStream);
    PoolReport(false, false, inStream);
    PoolReport(true, false, inStream);
    PoolReport(false, true, inStream);
    PoolReport(true, true, inStream);
}

} // namespace Q3DStudio
