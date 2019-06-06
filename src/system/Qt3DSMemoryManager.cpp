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
#include "Qt3DSMemoryManager.h"
#include "Qt3DSMemoryHeap.h"
#include "Qt3DSMemoryProbe.h"
#include "Qt3DSMemoryTracker.h"
#include "Qt3DSMemoryStatistics.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Create a new empty memory pool manager.
 *
 *	The manager must be initialized before use or it will forward all requests
 *	to the heap.
 */
CMemoryManager::CMemoryManager()
{
    // Set default allocations functions if they are not already set
    if (NULL == CMemory::Malloc())
        CMemory::SetMemoryFunctions(&malloc, &free, &realloc);

    CMemoryStatistics::AddManager(this);

#if Q3DStudio_MEMORY_POOLTRACKING
    Q3DStudio_memset(m_Histogram, 0, sizeof(m_Histogram));
    CMemoryStatistics::Overhead() += sizeof(m_Histogram);
#endif // Q3DStudio_MEMORY_POOLTRACKING
}

//==============================================================================
/**
 *	Create a new initialized memory pool manager.
 *
 *	@param inName		short description of manager used in tracker
 *	@param inChunkSize	array of  values describing the chunk size of each pool
 *	@param inChunkCount	array of  values describing the number of chunks in each pool
 */
CMemoryManager::CMemoryManager(const CHAR *inName,
                               const INT32 inChunkSize[Q3DStudio_MEMORY_POOLCOUNT],
                               const INT32 inChunkCount[Q3DStudio_MEMORY_POOLCOUNT])
{
    // Set default allocations functions if they are not already set
    if (NULL == CMemory::Malloc())
        CMemory::SetMemoryFunctions(&malloc, &free, &realloc);

    CMemoryStatistics::AddManager(this);
    Initialize(inName, inChunkSize, inChunkCount);

#if Q3DStudio_MEMORY_POOLTRACKING
    Q3DStudio_memset(m_Histogram, 0, sizeof(m_Histogram));
    CMemoryStatistics::Overhead() -= sizeof(m_Histogram);
#endif // Q3DStudio_MEMORY_POOLTRACKING
}

//==============================================================================
/**
 *	Release the memory manager and all owned pools.
 *
 *	Nobody better point to memory in these pool because the it will be
 *	invalid after this.
 */
CMemoryManager::~CMemoryManager()
{
    CMemoryStatistics::RemoveManager(this);
    Release();
}

//==============================================================================
/**
 *	Configure the pool manager into separate pools of different chunk sizes.
 *
 *	The chunk size array has to be increasing values such as:
 *	inChunkSize = { 8, 16, 32, 64, 96, 128, 256, 512, 1024 }
 *	but count doesn't:
 *	inChunkCount = { 500, 200, 1000, 200, 200, 100, 250, 50, 20 }
 *
 *	The global memory manager settings are initialized in AKMemory.h
 *
 *	@param inName		short description of manager used in tracker
 *	@param inChunkSize	array of Q3DStudio_MEMORY_POOLCOUNT values describing the chunk size
 *of each pool
 *	@param inChunkCount	array of Q3DStudio_MEMORY_POOLCOUNT values describing the number of
 *chunks in each pool
 */
void CMemoryManager::Initialize(const CHAR *inName,
                                const INT32 inChunkSize[Q3DStudio_MEMORY_POOLCOUNT],
                                const INT32 inChunkCount[Q3DStudio_MEMORY_POOLCOUNT])
{
    // Don't initialize more than once
    Q3DStudio_ASSERT(m_Pool[0].GetChunkCount() == 0);
    Q3DStudio_sprintf(m_Name, sizeof(m_Name), "%s", inName);

    for (INT32 thePoolIndex = 0; thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT; ++thePoolIndex) {
        // Make sure pool are aligned to Q3DStudio_MEMORY_ALIGNMENT or smaller than
        // Q3DStudio_MEMORY_ALIGNMENT
        Q3DStudio_ASSERT(inChunkSize[thePoolIndex] < Q3DStudio_MEMORY_ALIGNMENT
                         || (inChunkSize[thePoolIndex] % Q3DStudio_MEMORY_ALIGNMENT) == 0);
        m_Pool[thePoolIndex].Initialize(inName, inChunkSize[thePoolIndex],
                                        inChunkCount[thePoolIndex]);
    }
}

//==============================================================================
/**
 *	Release all the memory allocated in Initialize - essentially "uninitialize".
 */
void CMemoryManager::Release()
{
    for (INT32 thePoolIndex = 0; thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT; ++thePoolIndex)
        m_Pool[thePoolIndex].Release();
}

//==============================================================================
//	ALLOCATION AND DEALLOCATION
//==============================================================================

//==============================================================================
/**
 *	Fetch a new chunk of memory from the smallest pool possible
 *	@param	inSize		size of requested memory block, in bytes
 *	@param	inType		allocation description such as class name or pool name
 *	@param	inFile		file name
 *	@param	inFunction	method name
 *	@param	inLine		line number
 *	@return	void* a pointer to a memory block large enough to hold inSize bytes
 */
void *CMemoryManager::Allocate(INT32 inSize, const CHAR *inType, const CHAR *inFile,
                               const INT32 inLine)
{
    Q3DStudio_ASSERT(inSize >= 0);

#if Q3DStudio_MEMORY_POOLTRACKING
    m_Histogram[Q3DStudio_min<INT32>(inSize, 511)].Add(1);
#endif // Q3DStudio_MEMORY_POOLTRACKING

#if Q3DStudio_MEMORY_LINETRACKING
    inSize += sizeof(CMemoryTracker::SMemoryInfo); // make room for SMemoryInfo
#endif // Q3DStudio_MEMORY_LINETRACKING

    // Find the smallest pool that fits
    void *thePointer = NULL;
    INT32 thePoolIndex = 0;
    while (thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT) {
        // Does this pool fit?
        if (inSize <= m_Pool[thePoolIndex].GetChunkSize()) {
            thePointer = m_Pool[thePoolIndex].Allocate();
            break;
        }
        ++thePoolIndex;
    }

    // // Record the amount of wasted bytes in this chunk
    if (thePointer) {
        // Basic tracking
        m_ManagerData.m_Aligned.Allocate(inSize);
#if Q3DStudio_MEMORY_POOLTRACKING
        m_PoolData[thePoolIndex].m_Aligned.Allocate(inSize);
#endif // Q3DStudio_MEMORY_POOLTRACKING
    }
    // If not, we go to the heap for now.
    else {
        // REFACTOR: Use the next pool size up instead?
        thePointer = CMemoryHeap::Allocate(inSize, inType, inFile, inLine);
        m_ManagerData.m_Overflow.Allocate(inSize);

#if Q3DStudio_MEMORY_POOLTRACKING
        if (thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT)
            m_PoolData[thePoolIndex].m_Overflow.Allocate(
                inSize); // Heap allocation due to full memory pool
#endif // Q3DStudio_MEMORY_POOLTRACKING
    }

#if Q3DStudio_MEMORY_LINETRACKING
    CMemoryTracker::SMemoryInfo *theMemoryInfo =
        reinterpret_cast<CMemoryTracker::SMemoryInfo *>(thePointer);

    theMemoryInfo->m_DogTag = CMemoryTracker::TRACKER_DOGTAG;
    theMemoryInfo->m_Line = static_cast<INT16>(inLine);
    theMemoryInfo->m_Size = static_cast<INT16>(inSize - sizeof(CMemoryTracker::SMemoryInfo));
    theMemoryInfo->m_File = inFile;
    theMemoryInfo->m_Type = inType;

    m_LineTracker.Remember(theMemoryInfo);
    thePointer = reinterpret_cast<CMemoryTracker::SMemoryInfo *>(thePointer) + 1;
#endif // Q3DStudio_MEMORY_LINETRACKING

    return thePointer;
}

//==============================================================================
/**
 *	Release a chunk of memory.
 *	@param inPointer	pointer to memory to be reused
 *	@param inSize		size of the memory we are releasing, in bytes
 */
void CMemoryManager::Free(void *inPointer, INT32 inSize)
{
    if (!inPointer)
        return;

    Q3DStudio_ASSERT(inSize >= 0);

#if Q3DStudio_MEMORY_POOLTRACKING
    m_Histogram[Q3DStudio_min<INT32>(inSize, 511)].Delete(1);
#endif // Q3DStudio_MEMORY_POOLTRACKING

#if Q3DStudio_MEMORY_LINETRACKING
    inPointer = reinterpret_cast<CMemoryTracker::SMemoryInfo *>(inPointer) - 1;
    m_LineTracker.Forget(reinterpret_cast<CMemoryTracker::SMemoryInfo *>(inPointer));
    inSize += sizeof(CMemoryTracker::SMemoryInfo);
#endif // Q3DStudio_MEMORY_LINETRACKING

    // Did we get a valid pool?
    INT32 thePoolIndex = FetchPoolIndex(inPointer);
    if (thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT) {
        m_Pool[thePoolIndex].Free(inPointer);
        m_ManagerData.m_Aligned.Free(inSize);
#if Q3DStudio_MEMORY_POOLTRACKING
        m_PoolData[thePoolIndex].m_Aligned.Free(inSize);
#endif // Q3DStudio_MEMORY_POOLTRACKING
    } else {
        CMemoryHeap::Free(inPointer, inSize);
        m_ManagerData.m_Overflow.Free(inSize);

#if Q3DStudio_MEMORY_POOLTRACKING
        if (inSize <= m_Pool[Q3DStudio_MEMORY_POOLCOUNT - 1].GetChunkSize())
            for (thePoolIndex = 0; thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT; ++thePoolIndex)
                if (inSize <= m_Pool[thePoolIndex].GetChunkSize()) {
                    m_PoolData[thePoolIndex].m_Overflow.Free(inSize);
                    // This is driving me crazy -CN.
                    // Q3DStudio_ASSERT( m_PoolData[thePoolIndex].m_Overflow.GetCalls(
                    // MEMSCOPE_GLOBAL, MEMVALUE_CURRENT ) >= 0 );
                    break;
                }
#endif // Q3DStudio_MEMORY_POOLTRACKING
    }
}

//==============================================================================
/**
 *	Grow the existing memory allocated
 *
 *	@param	inOldPointer	pointer given in a previous allocation
 *	@param	inOldSize		size of current memory block, in bytes
 *	@param	inNewSize		size of requested memory block, in bytes
 *	@param	inNewType		allocation description such as class name or pool name
 *	@param	inFile			file name
 *	@param	inFunction		method name
 *	@param	inLine			line number
 *	@return	void* a pointer to a memory block large enough to hold inSize bytes
 */
void *CMemoryManager::Reallocate(void *inOldPointer, const INT32 inOldSize, const INT32 inNewSize,
                                 const CHAR *inNewType, const CHAR *inFile, const INT32 inLine)
{
    Q3DStudio_ASSERT(inOldSize >= 0 && inNewSize >= 0);

    // It's legal to pass NULL as old pointer to realloc
    if (!inOldPointer)
        return Allocate(inNewSize, inNewType, inFile, inLine);

    if (inNewSize == 0) {
        Free(inOldPointer, inOldSize);
        return NULL;
    }

    // Get a new bigger chunk and transfer old data to it
    void *thePointer = Allocate(inNewSize, inNewType, inFile, inLine);
    Q3DStudio_memcpy(thePointer, inOldPointer, Q3DStudio_min(inOldSize, inNewSize));

    // Release old data
    Free(inOldPointer, inOldSize);
    return thePointer;
}

//==============================================================================
//	IMPLEMENTATION
//==============================================================================

//==============================================================================
/**
 *	Find the pool that owns the given pointer.
 *	@param inPointer	pointer to memory to be found
 *	@return INT32 index of pool or Q3DStudio_MEMORY_POOLCOUNT if the chunk can't be found
 */
INT32 CMemoryManager::FetchPoolIndex(const void *inPointer)
{
    // Find pool that owns the pointer
    INT32 thePoolIndex = 0;
    while (thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT) {
        if (m_Pool[thePoolIndex].OwnsChunk(inPointer))
            return thePoolIndex;
        ++thePoolIndex;
    }

    return Q3DStudio_MEMORY_POOLCOUNT;
}

//==============================================================================
//	STATISTICS
//==============================================================================

//==============================================================================
/**
 *	Return the text identifier of the manager
 *	@return the name, max 32 bytes long
 */
const CHAR *CMemoryManager::GetName()
{
    return m_Name;
}

//==============================================================================
/**
 *	Reset all probe statistics
 */
void CMemoryManager::Reset()
{
    m_ManagerData.m_Aligned.Reset();
    m_ManagerData.m_Overflow.Reset();

#if Q3DStudio_MEMORY_POOLTRACKING
    for (INT32 thePoolIndex = 0; thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT; ++thePoolIndex) {
        m_Pool[thePoolIndex].GetProbe().Reset();
        m_PoolData[thePoolIndex].m_Aligned.Reset();
        m_PoolData[thePoolIndex].m_Overflow.Reset();
    }
#endif // Q3DStudio_MEMORY_POOLTRACKING
}

//==============================================================================
/**
 *	Retrieve a particular memory pool
 *	@param inPoolIndex	index of the pool
 *	@return SPoolData& is a reference to the indicated pool data
 */
CMemoryPool &CMemoryManager::GetPool(const INT32 inPoolIndex)
{
    Q3DStudio_ASSERT(inPoolIndex < Q3DStudio_MEMORY_POOLCOUNT && inPoolIndex >= 0);
    return m_Pool[inPoolIndex];
}

//==============================================================================
/**
 *	Retrieve a particular memory pool
 *	@param inPoolIndex	index of the pool
 *	@return SPoolData& is a reference to the indicated pool data
 */
CMemoryManager::SPoolData *CMemoryManager::GetPoolData(const INT32 inPoolIndex)
{
    Q3DStudio_UNREFERENCED_PARAMETER(inPoolIndex);
    Q3DStudio_ASSERT(inPoolIndex < Q3DStudio_MEMORY_POOLCOUNT && inPoolIndex >= 0);

#if Q3DStudio_MEMORY_POOLTRACKING
    return m_PoolData + inPoolIndex;
#else
    return NULL;
#endif // Q3DStudio_MEMORY_POOLTRACKING
}

//==============================================================================
/**
 *	Retrieve the number of bytes that were allocated on the heap
 *	@return CMemoryProbe as the heap statistics
 */
CMemoryManager::SPoolData &CMemoryManager::GetManagerData()
{
    return m_ManagerData;
}

//==============================================================================
/**
 *	Sum the high level statistics of all the pools.
 *	@return CMemoryProbe as the statistics, calculated on the spot
 */
CMemoryProbe CMemoryManager::GetProbe()
{
    CMemoryProbe theProbe;
#if Q3DStudio_MEMORY_POOLTRACKING
    for (INT32 thePoolIndex = 0; thePoolIndex < Q3DStudio_MEMORY_POOLCOUNT; ++thePoolIndex)
        theProbe.Combine(m_Pool[thePoolIndex].GetProbe());
#endif // Q3DStudio_MEMORY_POOLTRACKING
    return theProbe;
}

//==============================================================================
/**
 *	Fetch the tracker if enabled by setting Q3DStudio_MEMORY_LINETRACKING
 *	@return CMemoryTracker pointer or NULL
 */
CMemoryTracker *CMemoryManager::GetLineTracker()
{
#if Q3DStudio_MEMORY_LINETRACKING
    return &m_LineTracker;
#else
    return NULL;
#endif // Q3DStudio_MEMORY_LINETRACKING
}

//==============================================================================
/**
 *	Fetch the histogram if enabled by setting Q3DStudio_MEMORY_HISTOGRAM.
 *	The histogram counts all requested allocations of the specific size.
 *	@return UINT16 array of 512 values, or NULL
 */
const CMemoryProbe::SValue *CMemoryManager::GetHistogram()
{
#if Q3DStudio_MEMORY_POOLTRACKING
    return m_Histogram;
#else
    return NULL;
#endif // Q3DStudio_MEMORY_POOLTRACKING
}

} // namespace Q3DStudio
