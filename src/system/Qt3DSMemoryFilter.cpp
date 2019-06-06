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
#include "Qt3DSMemoryFilter.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Allocate a chunk of memory and add a header to track it.
 *	@param	inSize		size of requested memory block, in bytes
 *	@param	inType		allocation description
 *	@param	inFile		file name
 *	@param	inFunction	method name
 *	@param	inLine		line number
 *	@param	inClear		true to clear allocated memory to zero
 *	@return	void* a pointer to a memory block large enough to hold inSize bytes
 */
void *CMemoryFilter::Allocate(const INT32 inSize, const CHAR *inType, const CHAR *inFile,
                              const INT32 inLine, const BOOL inClear)
{
    const INT32 theTotalSize = inSize + Q3DStudio_MEMORY_ALIGNMENT;

    INT32 *theMemory = reinterpret_cast<INT32 *>(
        Q3DStudio_HANDLER_FILTER.Allocate(theTotalSize, inType, inFile, inLine));

    theMemory[0] = FILTER_DOGTAG;
    theMemory[1] = theTotalSize;

    if (inClear)
        Q3DStudio_memset(reinterpret_cast<INT8 *>(theMemory) + Q3DStudio_MEMORY_ALIGNMENT, 0,
                         inSize);

    return reinterpret_cast<INT8 *>(theMemory) + Q3DStudio_MEMORY_ALIGNMENT;
}

//==============================================================================
/**
 *	Free a block of memory that was allocated and tagged here.
 *	@param	inPointer	pointer given in a previous filter allocation
 */
void CMemoryFilter::Free(void *inPointer)
{
    if (!inPointer)
        return;

    INT32 *theMemory =
        reinterpret_cast<INT32 *>(reinterpret_cast<INT8 *>(inPointer) - Q3DStudio_MEMORY_ALIGNMENT);

    // Sanity check:  Did we allocate this memory in the first place?
    // If this throws we may be calling Q3DStudio_virtual_delete on memory that was getten from
    // Q3DStudio_new
    Q3DStudio_ASSERT(FILTER_DOGTAG == theMemory[0]);
    Q3DStudio_HANDLER_FILTER.Free(theMemory, theMemory[1]);
}

//==============================================================================
/**
 *	Re-allocate an existing chunk of memory and track it.
 *	@param	inPointer	pointer given in a previous allocation
 *	@param	inNewSize	size of requested memory block, in bytes
 *	@param	inNewType	new allocation description
 *	@param	inFile		file name
 *	@param	inFunction	method name
 *	@param	inLine		line number
 *	@return	void* a pointer to a memory block large enough to hold inSize bytes
 */
void *CMemoryFilter::Reallocate(void *inPointer, const INT32 inNewSize, const CHAR *inNewType,
                                const CHAR *inFile, const INT32 inLine)
{
    INT32 *theNewMemory = NULL;
    const INT32 theTotalSize = inNewSize + Q3DStudio_MEMORY_ALIGNMENT;

    // Dogtag is not valid if the old pointer is NULL
    if (inPointer) {
        INT32 *theOldMemory = reinterpret_cast<INT32 *>(reinterpret_cast<INT8 *>(inPointer)
                                                        - Q3DStudio_MEMORY_ALIGNMENT);
        Q3DStudio_ASSERT(FILTER_DOGTAG == theOldMemory[0]);
        theNewMemory = reinterpret_cast<INT32 *>(Q3DStudio_HANDLER_FILTER.Reallocate(
            theOldMemory, theOldMemory[1], theTotalSize, inNewType, inFile, inLine));
    } else {
        theNewMemory = reinterpret_cast<INT32 *>(
            Q3DStudio_HANDLER_FILTER.Reallocate(NULL, 0, theTotalSize, inNewType, inFile, inLine));
    }

    theNewMemory[0] = FILTER_DOGTAG;
    theNewMemory[1] = theTotalSize;
    return reinterpret_cast<INT8 *>(theNewMemory) + Q3DStudio_MEMORY_ALIGNMENT;
}

} // namespace Q3DStudio
