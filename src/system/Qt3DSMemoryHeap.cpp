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
#include "Qt3DSMemory.h"
#include "Qt3DSMemoryHeap.h"
#include "Qt3DSMemoryStatistics.h"
#include "Qt3DSIStream.h"
#include "foundation/Qt3DSLogging.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Static Initialzation
//==============================================================================
CMemoryProbe CMemoryHeap::s_Probe;

#if Q3DStudio_MEMORY_HEAPTRACKING
CMemoryHeap::TMemoryReport CMemoryHeap::s_MemoryReport;
#endif // Q3DStudio_MEMORY_HEAPTRACKING

//==============================================================================
/**
 *	Allocate a chunk of memory and potentially track it.
 *	@param	inSize				size of requested memory block, in bytes
 *	@param	inDescription		allocation description such as class name or pool name
 *	@param	inFile				file name
 *	@param	inFunction			method name
 *	@param	inLine				line number
 *	@return	void* a pointer to a memory block large enough to hold inSize bytes
 */
void *CMemoryHeap::Allocate(const INT32 inSize, const CHAR *inDescription, const CHAR *inFile,
                            const INT32 inLine)
{
    // Always track stats and trigger snapshot if this is a new record
    s_Probe.Allocate(inSize);

    // Check if we are exceeding the max memory limit
    if (Q3DStudio_MEMORY_LIMIT > 0
        && s_Probe.GetBytes(Q3DStudio::MEMSCOPE_GLOBAL, Q3DStudio::MEMVALUE_CURRENT)
            > Q3DStudio_MEMORY_LIMIT) {
        qCCritical (qt3ds::OUT_OF_MEMORY)
                << "Q3DStudio_MEMORY_UNIT exceeded " << Q3DStudio_MEMORY_LIMIT;
        Q3DStudio_ASSERT(false);
    }

#if Q3DStudio_MEMORY_HEAPTRACKING
    void *theAllocatedPtr = CMemory::Malloc()(static_cast<const size_t>(inSize));
    if (s_MemoryReport.GetCount() < s_MemoryReport.GetCapacity()) {
        SReportEntry theEntry = { theAllocatedPtr, inSize, inDescription, inFile, inLine };
        s_MemoryReport.Push(theEntry);
    } else {
        static BOOL s_FullWarning = false;
        if (!s_FullWarning) {
            qCWarning (qt3ds::TRACE_INFO) << "HeapTracker full. "
                                          << "Please increase Q3DStudio_MEMORY_HEAPTRACKINGSIZE("
                                          << Q3DStudio_MEMORY_HEAPTRACKINGSIZE << ") "
                                          << "to track all allocation and tune your pools to avoid "
                                          << "hitting the heap.";
            s_FullWarning = true;
        }
    }
    return theAllocatedPtr;
#else
    Q3DStudio_UNREFERENCED_PARAMETER(inLine);
    Q3DStudio_UNREFERENCED_PARAMETER(inFile);
    Q3DStudio_UNREFERENCED_PARAMETER(inDescription);

    return CMemory::Malloc()(static_cast<const size_t>(inSize));
#endif // Q3DStudio_MEMORY_HEAPTRACKING
}

//==============================================================================
/**
 *	Free a block of memory allocated using CMemoryHeap::Allocate
 *	@param	inPointer	pointer given in a previous allocation
 *	@param	inSize		size of given memory block, in bytes
 */
void CMemoryHeap::Free(void *inPointer, INT32 inSize)
{
    // No-Op on NULL
    if (!inPointer)
        return;

#if Q3DStudio_MEMORY_HEAPTRACKING
    RemoveReport(inPointer);
#endif // Q3DStudio_MEMORY_HEAPTRACKING

    s_Probe.Free(inSize);
    return CMemory::Free()(inPointer);
}

//==============================================================================
/**
 *	Re-allocate an existing chunk of memory and potentially track it.
 *	@param	inPointer			pointer given in a previous allocation
 *	@param	inOldSize			size of current memory block, in bytes
 *	@param	inNewSize			size of requested memory block, in bytes
 *	@param	inNewDescription	allocation description such as class name or pool name
 *	@param	inFile				file name
 *	@param	inFunction			method name
 *	@param	inLine				line number
 *	@return	void* a pointer to a memory block large enough to hold inSize bytes
 */
void *CMemoryHeap::Reallocate(void *inPointer, const INT32 inOldSize, const INT32 inNewSize,
                              const CHAR *inNewDescription, const CHAR *inFile, const INT32 inLine)
{
    // Make sure this isn't a new malloc
    if (!inPointer)
        return Allocate(inNewSize, inNewDescription, inFile, inLine);

    // Always track stats and record new high if it's a record
    s_Probe.Allocate(inNewSize);
    s_Probe.Free(inOldSize);

    // Check if we are exceeding the max memory limit
    Q3DStudio_ASSERT(Q3DStudio_MEMORY_LIMIT == 0
                         ? true
                         : s_Probe.GetBytes(Q3DStudio::MEMSCOPE_GLOBAL, Q3DStudio::MEMVALUE_CURRENT)
                             < Q3DStudio_MEMORY_LIMIT);

#if Q3DStudio_MEMORY_HEAPTRACKING
    RemoveReport(inPointer);

    void *theReallocatedPtr = CMemory::Realloc()(inPointer, static_cast<const size_t>(inNewSize));

    SReportEntry theEntry = { theReallocatedPtr, inNewSize, inNewDescription, inFile, inLine };
    s_MemoryReport.Push(theEntry);

    return theReallocatedPtr;
#else
    return CMemory::Realloc()(inPointer, static_cast<const size_t>(inNewSize));
#endif // Q3DStudio_MEMORY_HEAPTRACKING
}

//==============================================================================
/**
 *	Retrieves memory info about a particular allocation
 *	@param	inPointer	allocated memory
 *	@return SMemoryHeapReportEntry* memory report
 */
CMemoryHeap::SReportEntry *CMemoryHeap::FindReport(void *inPointer)
{
    Q3DStudio_UNREFERENCED_PARAMETER(inPointer);

#if Q3DStudio_MEMORY_HEAPTRACKING
    INT32 theEnd = s_MemoryReport.GetCount();
    for (INT32 theIndex = 0; theIndex < theEnd; ++theIndex)
        if (s_MemoryReport[theIndex].m_AllocatedPointer == inPointer)
            return &s_MemoryReport[theIndex];

    Q3DStudio_ASSERT(false);
#endif // Q3DStudio_MEMORY_HEAPTRACKING

    return NULL;
}

//==============================================================================
/**
 *	Removes memory info due to a deallocation.
 *	@param	inPointer	allocated memory
 */
void CMemoryHeap::RemoveReport(void *inPointer)
{
    Q3DStudio_UNREFERENCED_PARAMETER(inPointer);

#if Q3DStudio_MEMORY_HEAPTRACKING
    INT32 theEnd = s_MemoryReport.GetCount();
    for (INT32 theIndex = 0; theIndex < theEnd; ++theIndex) {
        if (s_MemoryReport[theIndex].m_AllocatedPointer == inPointer) {
            s_MemoryReport.Remove(theIndex);
            return;
        }
    }
#endif // Q3DStudio_MEMORY_HEAPTRACKING
}

//==============================================================================
/**
 *	The memory probe records basic heap activity
 *	@return	reference to probe
 */
CMemoryProbe &CMemoryHeap::GetProbe()
{
    return s_Probe;
}

//==============================================================================
/**
 *	The report records all heap activity
 *	@return	pointer to report if tracking is on, or NULL
 */
CMemoryHeap::TMemoryReport *CMemoryHeap::GetReport()
{
#if Q3DStudio_MEMORY_HEAPTRACKING
    return &s_MemoryReport;
#else
    return NULL;
#endif // Q3DStudio_MEMORY_HEAPTRACKING
}

} // namespace Q3DStudio
