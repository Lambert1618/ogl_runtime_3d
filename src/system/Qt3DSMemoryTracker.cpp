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
#include "Qt3DSMemoryTracker.h"
#include "foundation/Qt3DSLogging.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Construct and reset the tracker.
 */
CMemoryTracker::CMemoryTracker()
    : m_TrackingOverhead(0)
{
    Q3DStudio_memset(&m_TrackingHashBin, 0, sizeof(m_TrackingHashBin));
    CMemoryStatistics::Overhead() += sizeof(m_TrackingHashBin);
}

//==============================================================================
/**
 *	Destructor
 */
CMemoryTracker::~CMemoryTracker()
{
    CMemoryStatistics::Overhead() -= sizeof(m_TrackingHashBin);
}

//==============================================================================
/**
 *	Add information about allocation
 *	@param inPtr	contains info about a new allocation
 */
void CMemoryTracker::Remember(SMemoryInfo *inPointer)
{
    if (inPointer) {
        // Update global tracking
        m_TrackingOverhead += sizeof(SMemoryInfo);

        // Locate a suitable hashbin
        size_t theHashBin =
            (reinterpret_cast<size_t>(inPointer) >> 2) % Q3DStudio_MEMORY_LINETRACKINGSIZE;
        size_t theStartHashBin = theHashBin;

        while (m_TrackingHashBin[theHashBin]) {
            ++theHashBin;
            if (theHashBin >= Q3DStudio_MEMORY_LINETRACKINGSIZE)
                theHashBin = 0;

            if (theHashBin == theStartHashBin) {
                // We've run out of room in the hashbin.
                // Abort and increase the bin size.
                qCCritical(qt3ds::OUT_OF_MEMORY)
                        << "Memory Tracker Error: Ran out of room in tracker hashbin "
                        << Q3DStudio_MEMORY_LINETRACKINGSIZE;
                // exit( -1 );
            }
        }

        // Set WatchGuard and store the pointer in the hash bin
        inPointer->m_DogTag = TRACKER_DOGTAG;
        m_TrackingHashBin[theHashBin] = inPointer;
    }
}

//==============================================================================
/**
 *	Remove information on an allocation
 *	@param inPtr	contains info about the allocation we are releasing
 */
void CMemoryTracker::Forget(SMemoryInfo *inPointer)
{
    if (inPointer) {
        // Update global tracking
        m_TrackingOverhead -= sizeof(SMemoryInfo);

        // Locate the pointer in the hash bin
        size_t theHashBin = Q3DStudio_max<size_t>(0, (reinterpret_cast<size_t>(inPointer) >> 2)
                                                      % Q3DStudio_MEMORY_LINETRACKINGSIZE);
        size_t theStartHashBin = theHashBin;

        while (m_TrackingHashBin[theHashBin] != inPointer) {
            ++theHashBin;
            if (theHashBin >= Q3DStudio_MEMORY_LINETRACKINGSIZE)
                theHashBin = 0;

            if (theHashBin == theStartHashBin) {
                // We were unable to locate the pointer in the hash bin.
                // This is really bad, but not catastrophic
                qCWarning(qt3ds::OUT_OF_MEMORY)
                        << "Memory Tracker Warning. Can't find pointer in tracker hashbin";
                return;
            }
        }

        // Verify watch guard.  Something is trashing memory if this call fails.
        Q3DStudio_ASSERT(m_TrackingHashBin[theHashBin]->m_DogTag == TRACKER_DOGTAG);

        // Clear the pointer from the hash bin
        m_TrackingHashBin[theHashBin] = NULL;
    }
}

//==============================================================================
/**
 *	Dump the memory list stored in the hash bin.
 *	@param	inFileName	the report filename or NULL to dump to the logger
 */
INT32 CMemoryTracker::Report(IStream *inStream /*=NULL*/)
{
    INT32 theTotalBytes = 0;
    CHAR theLine[256];

    for (INT32 theBinIndex = 0; theBinIndex < Q3DStudio_MEMORY_LINETRACKINGSIZE; ++theBinIndex) {
        if (m_TrackingHashBin[theBinIndex]) {
            CMemoryTracker::SMemoryInfo *theInfo = m_TrackingHashBin[theBinIndex];

            Q3DStudio_sprintf(theLine, sizeof(theLine), "0x%p, %8d, %s, %s(%hd)\n", theInfo,
                              theInfo->m_Size, theInfo->m_Type, theInfo->m_File, theInfo->m_Line);

            CMemoryStatistics::Report(inStream, theLine);
            theTotalBytes += theInfo->m_Size;
        }
    }

    return theTotalBytes;
}

} // namespace Q3DStudio
