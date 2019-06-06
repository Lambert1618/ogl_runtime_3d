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
#include "Qt3DSMemoryPool.h"
#include "Qt3DSMemoryProbe.h"
#include "Qt3DSMemoryTracker.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class CMemoryTracker;

//==============================================================================
/**
 *	Allocation switch-board forwarding requests to pools.
 *
 *	This is a memory pool hub acting as a complete heap replacement.  The number
 *	of pools is hardcoded in Q3DStudio_MEMORY_POOLCOUNT but can be changed with a
 *	recompile. The main manager is created in AKMemory and is preconfigured to
 *	be generous with memory.  Tune these values to match your title since
 *	unused chunks are wasteful.
 *
 *	@note A pool manager survives fine even when not initialized.  In this state
 *	it simply forwards each request to the CMemoryHeap.  This setup is intentional
 *	to enable the default manager to survive long enough to allow future runtime
 *	initialization based on complexity of the level or presentation being loaded.
 */
class CMemoryManager
{
    //==============================================================================
    //	Structs
    //==============================================================================
public:
    /// External pool usage information
    struct SPoolData
    {
        CMemoryProbe
            m_Aligned; ///< Exact sum of memory use (28byte struct in 32byte pool is still 28 here)
        CMemoryProbe m_Overflow; ///< Failed memory allocation because the pool was full
    };

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    CHAR m_Name[32]; ///< Manager identifier for statistics
    CMemoryPool m_Pool[Q3DStudio_MEMORY_POOLCOUNT]; ///< Actual pool memory objects
    SPoolData m_ManagerData; ///< Track allocations forwarded to heap because they were too large

#if Q3DStudio_MEMORY_POOLTRACKING
    SPoolData
        m_PoolData[Q3DStudio_MEMORY_POOLCOUNT]; ///< Trace pools of different sizes with their usage
    CMemoryProbe::SValue
        m_Histogram[512]; ///< Allocation histogram if active using Q3DStudio_MEMORY_HEAPTRACKING
#endif // Q3DStudio_MEMORY_POOLTRACKING

#if Q3DStudio_MEMORY_LINETRACKING
    CMemoryTracker
        m_LineTracker; ///< Line allocation tracker if active using Q3DStudio_MEMORY_LINETRACKING
#endif // Q3DStudio_MEMORY_LINETRACKING

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CMemoryManager();
    CMemoryManager(const CHAR *inName, const INT32 inChunkSize[Q3DStudio_MEMORY_POOLCOUNT],
                   const INT32 inChunkCount[Q3DStudio_MEMORY_POOLCOUNT]);
    ~CMemoryManager();

public: // Initialization
    void Initialize(const CHAR *inName, const INT32 inChunkSize[Q3DStudio_MEMORY_POOLCOUNT],
                    const INT32 inChunkCount[Q3DStudio_MEMORY_POOLCOUNT]);
    void Release();

public: // Allocation and Deallocation
    void *Allocate(INT32 inSize, const CHAR *inType, const CHAR *inFile, const INT32 inLine);
    void Free(void *inOldPointer, INT32 inSize);
    void *Reallocate(void *inOldPointer, const INT32 inOldSize, const INT32 inNewSize,
                     const CHAR *inNewType, const CHAR *inFile, const INT32 inLine);

protected: // Implementation
    INT32 FetchPoolIndex(const void *inPointer);

public: // Statistics
    void Reset();
    const CHAR *GetName();
    CMemoryPool &GetPool(const INT32 inPoolIndex);
    SPoolData *GetPoolData(const INT32 inPoolIndex);
    SPoolData &GetManagerData();
    CMemoryProbe GetProbe();
    CMemoryTracker *GetLineTracker();
    const CMemoryProbe::SValue *GetHistogram();
};

} // namespace Q3DStudio
