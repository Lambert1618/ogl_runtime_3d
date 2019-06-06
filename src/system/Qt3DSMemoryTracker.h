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
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Low level memory statistics tracking each individual allocation.
 *
 *	This class is instantiated in each CMemoryManager if tracking is enabled
 *	by setting ENABLE_MEMORY_LINETRACKING to 1 in AKConfig.h for the build
 *	configuration you want.
 *
 *	Tracking is much more invasive than simple statistics since it records each
 *	individual allocation and deallocation.  The overhead can be seen mostly in
 *	the memory load since we store a tracking hashbin of all memory allocation
 *	and a 16 byte SMemoryInfo structure is prepended to each allocation no matter
 *	how small.  This mode should be used when debugging memory to track down
 *	individual allocations.  It's turned off by default and should stay off
 *	most of the time.
 *
 *	The tracking information is usually dumped to a comma delimited .csv file
 *	for easy viewing as a spreadsheet of data.  This action is initiated by
 *	pressing CTRL-F1 in Quarterback on the PC but can be added to any viewer
 *	by calling CMemoryStatistics::TrackingDump.
 */
class CMemoryTracker
{
    //==============================================================================
    //	Constants
    //==============================================================================
public:
    const static UINT16 TRACKER_DOGTAG = 0xbead; ///< Watch guard value in SMemoryTrackInfo

    //==============================================================================
    //	Structs
    //==============================================================================
public:
    /// 16 bytes of details on each allocation when tracking is enabled
    struct SMemoryInfo
    {
        UINT16 m_DogTag; ///< 0xbead - asserting this to track overwrites
        INT16 m_Line; ///< line from which the allocation occurred
        INT32 m_Size; ///< allocation size in bytes
        const CHAR *m_File; ///< file from which the allocation occurred
        const CHAR *m_Type; ///< type of allocation, usually class name
    };

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    INT32 m_TrackingOverhead; ///< memory used to track allocations in bytes
    SMemoryInfo *m_TrackingHashBin[Q3DStudio_MEMORY_LINETRACKINGSIZE]; ///< hash table of all
                                                                       ///tracked allocations

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CMemoryTracker();
    ~CMemoryTracker();

public: // Tracking
    void Remember(SMemoryInfo *inData);
    void Forget(SMemoryInfo *inData);

public: // Reporting
    INT32 Report(IStream *inStream = NULL);
};

} // namespace Q3DStudio
