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
#include "Qt3DSMemoryPool.h"
#include "Qt3DSMemoryHeap.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Create a new memory pool.
 *
 */
CMemoryPool::CMemoryPool()
    : m_ChunkSize(0)
    , m_ChunkCount(0)
    , m_Ceiling(NULL)
    , m_Memory(NULL)
    , m_Top(NULL)
    , m_Hole(NULL)
{
    m_Description[0] = '\0';
}

//==============================================================================
/**
 *	Release the memory pool.
 *
 *	Nobody better point to this pool because it's illegal after this.
 *	We could blast the memory in debug mode to trigger bugs early but most
 *	compilers already do this.
 */
CMemoryPool::~CMemoryPool()
{
    Release();
}

//==============================================================================
/**
 *	Configure the memory pool.
 *
 *	The pool will allocate inChunkSize * inChunkCount bytes of memory from
 *	the heap using Q3DStudio_new, aligned on 4 byte boundaries.
 *
 *	@param inName			short description of pool - used in tracker
 *	@param inChunkSize		size in bytes of each memory block, aka chunk
 *	@param inChunkCount		number of chunks the pool holds
 */
void CMemoryPool::Initialize(const CHAR *inName, const INT32 inChunkSize, const INT32 inChunkCount)
{
    Q3DStudio_ASSERT(!m_Memory);

    // Round up chunk size to the next even pointer size
    // It needs to be at least pointer size to store the hole linked list
    INT32 theRoundedChunkSize =
        static_cast<INT32>(((inChunkSize + sizeof(void *) - 1) / sizeof(void *)) * sizeof(void *));

    // Create pool description
    Q3DStudio_sprintf(m_Description, sizeof(m_Description), "POOL: %s(%dx%d)", inName,
                      theRoundedChunkSize, inChunkCount);

    // Reserve memory pool
    m_ChunkSize = theRoundedChunkSize;
    m_ChunkCount = inChunkCount;
    m_Memory = reinterpret_cast<INT8 *>(
        Q3DStudio_heap_allocate(m_ChunkSize * m_ChunkCount, m_Description));
    m_Top = m_Memory;
    m_Ceiling = m_Memory + m_ChunkSize * m_ChunkCount;
}

//==============================================================================
/**
 *	Release all the memory allocated in Initialize - essentially "uninitialize".
 */
void CMemoryPool::Release()
{
    Q3DStudio_heap_free(m_Memory, m_ChunkSize * m_ChunkCount);

    m_ChunkSize = 0, m_ChunkCount = 0;
    m_Ceiling = NULL;
    m_Memory = NULL;
    m_Top = NULL;
    m_Hole = NULL;
}

//==============================================================================
//	ALLOCATION AND DEALLOCATION
//==============================================================================

//==============================================================================
/**
 *	Fetch a new chunk of memory.
 *
 *	If there is a pool hole we use that, otherwise feed of the head.
 *	Every alloction is tracked in m_Statistics.
 *	@return a pointer to inChunkSize bytes of usable memory
 */
void *CMemoryPool::Allocate()
{
    if (m_Hole)
        return PopHole();
    else
        return UseTop();
}

//==============================================================================
/**
 *	Release a chunk of memory and track changes in statistics
 *	@param inChunk		a pointer to memory to be reused
 */
void CMemoryPool::Free(void *inChunk)
{
    Q3DStudio_ASSERT(OwnsChunk(inChunk));
    PushHole(inChunk);
}

//==============================================================================
/**
 *	Verify that a pointer is owned by a specific pool
 *	@param inChunk		a pointer to be checked
 *	@return BOOL true if the chunk is owned by this pool
 */
BOOL CMemoryPool::OwnsChunk(const void *inChunk) const
{
#ifdef _DEBUG
    // Scan all holes to prevent double releasing memory
    if (!(inChunk >= m_Ceiling || inChunk < m_Memory)) {
        // No freeing anything above the top?
        Q3DStudio_ASSERT(inChunk < m_Top);

        // Not freeing any holes?
        void *theHoleChunk = m_Hole;
        while (theHoleChunk) {
            Q3DStudio_ASSERT(inChunk != theHoleChunk);
            theHoleChunk = reinterpret_cast<void *>(*reinterpret_cast<size_t *>(theHoleChunk));
        }
    }
#endif // DEBUG

    return !(inChunk >= m_Ceiling || inChunk < m_Memory);
}

//==============================================================================
/**
 *	Quick check to see if there are chunks available
 *	@return BOOL true if the pool has available chunks
 */
BOOL CMemoryPool::IsFull() const
{
    return m_Hole == NULL && GetFreeTops() == 0;
}

//==============================================================================
/**
 *	Get the total number of free chunks in this pool.
 *	@return INT32 is the nunber of free chunks
 */
INT32 CMemoryPool::GetFreeChunks() const
{
    return GetFreeTops() + GetFreeHoles();
}

//==============================================================================
/**
 *	Get the number of holes in this pool.
 *	@return INT32 is the nunber of holes
 */
INT32 CMemoryPool::GetFreeHoles() const
{
    INT32 theHoleCounter = 0;
    void *theHoleChunk = m_Hole;

    // Walk the hole list until there are no more holes
    while (theHoleChunk) {
        theHoleChunk = reinterpret_cast<void *>(*reinterpret_cast<size_t *>(theHoleChunk));
        ++theHoleCounter;
    }

    return theHoleCounter;
}

//==============================================================================
/**
 *	Get the number of chunks left at the top.
 *	@return INT32 is the nunber of continous chunks at the top
 */
INT32 CMemoryPool::GetFreeTops() const
{
    return static_cast<INT32>((m_Ceiling - m_Top)) / m_ChunkSize;
}

//==============================================================================
//	IMPLEMENTATION
//==============================================================================

//==============================================================================
/**
 *	Return a chunch from the top.
 *
 *	The top is only used if no pool holes are available.
 *	@return a pointer to the chunk at the top
 */
void *CMemoryPool::UseTop()
{
    // Out of memory?
    // This is not assert since it may happen during normal runtime.
    if (m_Top >= m_Ceiling)
        return NULL;

    void *theChunk = m_Top;
    m_Top += m_ChunkSize;

#if Q3DStudio_MEMORY_POOLTRACKING
    m_Probe.Allocate(m_ChunkSize);
#endif // Q3DStudio_MEMORY_POOLTRACKING
    return theChunk;
}

//==============================================================================
/**
 *	Add a new hole to the pool list.
 *
 *	The holes are used in a stack-like fashion so the chunk we push in most
 *	recently is the first one to be returned next New call.
 *	The holes are linked by using the first four bytes to point to the next
 *	chunk.  We are hijacking them to create a linked list of free chunks.
 *	@param inChunk is the just deleted chunk
 */
void CMemoryPool::PushHole(void *inChunk)
{
#if Q3DStudio_MEMORY_POOLTRACKING
    m_Probe.Free(m_ChunkSize);
#endif // Q3DStudio_MEMORY_POOLTRACKING

    // Set the pushed chunk to be the new top hole after we
    // link the content of the new chunk to the last top.
    *reinterpret_cast<size_t *>(inChunk) = reinterpret_cast<size_t>(m_Hole);
    m_Hole = inChunk;
}

//==============================================================================
/**
 *	Get the topmost hole as our chunk.
 *
 *	Return the latest hole and relink the list of holes to the top.
 *	@return void* is the most recently deleted chunk
 */
void *CMemoryPool::PopHole()
{
    Q3DStudio_ASSERT(m_Hole);

    // Return the old hole as our new chunk after we
    // set the hole to the second hole in the list.
    void *theChunk = m_Hole;
    m_Hole = reinterpret_cast<void *>(*reinterpret_cast<size_t *>(m_Hole));

#if Q3DStudio_MEMORY_POOLTRACKING
    m_Probe.Allocate(m_ChunkSize);
#endif // Q3DStudio_MEMORY_POOLTRACKING
    return theChunk;
}

} // namespace Q3DStudio
