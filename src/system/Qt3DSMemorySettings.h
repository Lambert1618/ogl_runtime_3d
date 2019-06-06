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
#include "Qt3DSTypes.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	GlobalMemoryManager constants
 *	NOTE THAT THESE NUMBERS BELOW SHOULD BE ADJUSTED TO FIT YOUR APPLICATION.
 *
 *	See the Memory Management section in the documentation for guidelines
 *	on memory diagnostics that enable you to optimize these values:
 */

//==============================================================================
//	Memory Pool Configuration
//==============================================================================

#ifndef Q3DStudio_MEMORY_POOLCOUNT
#define Q3DStudio_MEMORY_POOLCOUNT 12
#endif // Q3DStudio_MEMORY_POOLCOUNT

/// A memory pool has a chunk size that defines he size in bytes of each slot
/// and a chunk count defining how many slots there are of that size.
/// The pools below thus has 128 slots of 4 bytes, 256 slots of 8 bytes etc:
const INT32 g_ChunkSize[Q3DStudio_MEMORY_POOLCOUNT] = { 4,  8,   16,  32,  48,  64,
                                                        96, 128, 160, 256, 384, 640 };
const INT32 g_ChunkCount[Q3DStudio_MEMORY_POOLCOUNT] = { 128, 256, 256, 1536, 128, 128,
                                                         256, 64,  32,  32,   32,  32 };

//==============================================================================
//	Memory Settings
//==============================================================================

/// Define the upper memory limit used by Runtime.
/// 0 = unlimited
#ifndef Q3DStudio_MEMORY_LIMIT
#define Q3DStudio_MEMORY_LIMIT 0
#endif // Q3DStudio_MEMORY_LIMIT

// Pool sizes and diagnostics are aligned to the required boundary.
#ifndef Q3DStudio_MEMORY_ALIGNMENT
#define Q3DStudio_MEMORY_ALIGNMENT 8
#endif // Q3DStudio_MEMORY_ALIGNMENT

//==============================================================================
//	Memory Diagnostics
//==============================================================================

/// Note that the simple report is always on.	(F1 in Quarterback)

/// CMemoryHeap usage report.					(F2 in Quarterback)
/// Tracks large allocations such as pool buffers, managers and overflows.
/// Not much overhead since most allocations should be intercepted by pools.
#ifndef Q3DStudio_MEMORY_HEAPTRACKING
#define Q3DStudio_MEMORY_HEAPTRACKING 0
#endif // Q3DStudio_MEMORY_HEAPTRACKING

/// Max number of 20 byte SReportEntry heap entries
/// Increase this number if you get a log warning but you should really tune
/// your memory pools to avoid hitting the heap.
#ifndef Q3DStudio_MEMORY_HEAPTRACKINGSIZE
#define Q3DStudio_MEMORY_HEAPTRACKINGSIZE 4000
#endif // Q3DStudio_MEMORY_HEAPTRACKINGSIZE

/// Invasive allocation tracker.				(F3 in Quarterback)
/// Track detailed memory usage per allocation through CMemoryManager by tracking
/// every Runtime allocation, line by line using __FILE__ and __LINE__.
/// Note that this adds a 16byte SMemoryInfo to each allocation and thus
/// changes pool usage.
///  === Do not tune pools with line tracking enabled! ===
#ifndef Q3DStudio_MEMORY_LINETRACKING
#define Q3DStudio_MEMORY_LINETRACKING 0
#endif // Q3DStudio_MEMORY_LINETRACKING

// Hashbin size of entries pointing to SMemoryInfo allocation headers
#ifndef Q3DStudio_MEMORY_LINETRACKINGSIZE
#define Q3DStudio_MEMORY_LINETRACKINGSIZE (128 * 1024)
#endif // Q3DStudio_MEMORY_LINETRACKINGSIZE

/// CMemoryManager pooled memory usage report.	(F5-F8 in Quarterback)
/// Track most used allocation sizes to tune pool sizes and count.
/// Enabled by default since it allows tracking precise presentation
/// memory usage without much of an overhead.
/// It also includes a histogram when showing Reset data (F5,F6)
#ifndef Q3DStudio_MEMORY_POOLTRACKING
#ifdef _DEBUG
#define Q3DStudio_MEMORY_POOLTRACKING 1
#else
#define Q3DStudio_MEMORY_POOLTRACKING 0
#endif
#endif // Q3DStudio_MEMORY_POOLTRACKING

} // namespace Q3DStudio
