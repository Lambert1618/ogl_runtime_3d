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
#include "Qt3DSMemoryProbe.h"
#include "Qt3DSFixedArray.h"
#include "Qt3DSMemorySettings.h"

//==============================================================================
/**
 *	Low level entry points to the heap.
 *
 *	Do not call these directly.  Use Q3DStudio_new or Q3DStudio_allocate instead since
 *	those use the pools.
 *	@see CMemoryHeap
 */
#define Q3DStudio_heap_allocate(size, desc)                                                        \
    Q3DStudio::CMemoryHeap::Allocate(size, desc, __FILE__, __LINE__)
#define Q3DStudio_heap_reallocate(ptr, oldsize, newsize, newdesc)                                  \
    Q3DStudio::CMemoryHeap::Reallocate(ptr, oldsize, newsize, newdesc, __FILE__, _LINE__)
#define Q3DStudio_heap_free(ptr, size) Q3DStudio::CMemoryHeap::Free(ptr, size)

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class IStream;

//==============================================================================
/**
 *	Last memory interface before calls to memory callbacks or OS malloc.
 *
 *	First of all, don't call this class directy. Use the macros
 *	Q3DStudio_heap_allocate and Q3DStudio_heap_free instead since they fill out file,
 *	line and function parameters for the tracking system.
 *
 *	This is the single exit point for all heap memory allocations.  Change this
 *	code to redirect memory to your own subsystems or add more tracking code to
 *	measure memory usage.  Note however that most memory requests during runtime
 *	will never reach this point since memory pools will handle those requests.
 *	Large memory requests and emergency scenarios will still end up here.
 *
 *	@note This class contains the only three real memory calls in Runtime.
 */
class CMemoryHeap
{
    //==============================================================================
    //	Structs
    //==============================================================================
public:
    /// Simple memory log entry
    struct SReportEntry
    {
        const void *m_AllocatedPointer; ///< allocated pointer
        INT32 m_Size; ///< size in bytes
        const CHAR *m_Description; ///< simple description
        const CHAR *m_File; ///< filename
        INT32 m_Line; ///< line number
    };

    //==============================================================================
    //	Typedefs
    //==============================================================================
public:
    typedef CFixedArray<SReportEntry, Q3DStudio_MEMORY_HEAPTRACKINGSIZE> TMemoryReport;

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    static CMemoryProbe s_Probe; ///< Light overall allocation statistics
#if Q3DStudio_MEMORY_HEAPTRACKING
    static TMemoryReport s_MemoryReport; ///< Storage for memory logs
#endif // Q3DStudio_MEMORY_HEAPTRACKING

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Allocation/Deallocation
    static void *Allocate(const INT32 inSize, const CHAR *inDescription, const CHAR *inFile,
                          const INT32 inLine);
    static void Free(void *inPointer, INT32 inSize);
    static void *Reallocate(void *inPointer, const INT32 inOldSize, const INT32 inNewSize,
                            const CHAR *inNewDescription, const CHAR *inFile, const INT32 inLine);

public: // Statistics
    static CMemoryProbe &GetProbe();
    static TMemoryReport *GetReport();

protected: // Low level memory overview
    static SReportEntry *FindReport(void *inPointer);
    static void RemoveReport(void *inPointer);

public: // Report request
    static void Report(IStream *inStream = NULL);
};

} // namespace Q3DStudio
