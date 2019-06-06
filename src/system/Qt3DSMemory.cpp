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

//==============================================================================
//	OS level memory routines
//==============================================================================
Q3DStudio::CMemory::TMalloc Q3DStudio::CMemory::s_Malloc = NULL;
Q3DStudio::CMemory::TRealloc Q3DStudio::CMemory::s_Realloc = NULL;
Q3DStudio::CMemory::TFree Q3DStudio::CMemory::s_Free = NULL;

//==============================================================================
/**
 *	Overrides basic memory allocation/deallocation routines
 *	@param	inMalloc	memory allocation routine
 *	@param	inFree		memory deallocation routine
 *	@param	inRealloc	memory reallocation routine
 */
void Q3DStudio::CMemory::SetMemoryFunctions(const TMalloc inMalloc, const TFree inFree,
                                            const TRealloc inRealloc)
{
    s_Malloc = inMalloc;
    s_Realloc = inRealloc;
    s_Free = inFree;
}

static Q3DStudio::CMemoryManager *s_globalManager = nullptr;
static Q3DStudio::CMemoryHeap *s_globalHeap = nullptr;

//==============================================================================
/**
 *	Boot up the pooled memory manager and return it.
 *	@note The manager has to be initialized before use:
 *		GetMemoryManager( ).Initialize( "GlobalManager", g_ChunkSize, g_ChunkCount );
 *	@return Q3DStudio::CMemoryManager reference to the global object
 */
Q3DStudio::CMemoryManager &GetMemoryManager()
{
    if (!s_globalManager)
        s_globalManager = new Q3DStudio::CMemoryManager;
    return *s_globalManager;
}

//==============================================================================
/**
 *	Return a reference to the global heap object.
 *	@return Q3DStudio::CMemoryHeap reference to the global object
 */
Q3DStudio::CMemoryHeap &GetMemoryHeap()
{
    if (!s_globalHeap)
        s_globalHeap = new Q3DStudio::CMemoryHeap;
    return *s_globalHeap;
}

//==============================================================================
//	Q3DStudio_new operator prototypes (5 args)
//==============================================================================
void *operator new(size_t inReportedSize, size_t inOfficialSize, const char *inType,
                   const char *inFile, int inLine)
{
    Q3DStudio_UNREFERENCED_PARAMETER(inReportedSize);
    Q3DStudio_ASSERT(inReportedSize == inOfficialSize);

    return Q3DStudio_HANDLER_NEW.Allocate(static_cast<const Q3DStudio::INT32>(inOfficialSize),
                                          inType, inFile, inLine);
}

//==============================================================================
/**
 *	Override 'operator delete' in order to track memory usage.
 *
 *	So what's the use of the overloaded delete with special arguments? There is
 *	actually one case in which it will be called--when an exception is thrown
 *	during object construction. As you might recall, there is a contract implicit
 *	in the language that if an exception happens during the construction of an object,
 *	the memory for this object will be automatically deallocated. It so happens
 *	that during object's construction the compiler is still aware of which version
 *	of operator new was called to allocate memory. It is therefore able to generate
 *	a call to the corresponding version of delete, in case an exception is thrown.
 *	After the successful completion of construction, this information is no longer
 *	available and the compiler has no means to guess which version of global delete
 *	is appropriate for a given object.
 */
void operator delete(void *inReportedAddress, size_t inOfficialSize, const char *, const char *,
                     int)
{
    Q3DStudio_HANDLER_NEW.Free(inReportedAddress,
                               static_cast<const Q3DStudio::INT32>(inOfficialSize));
}

//==============================================================================
//	Q3DStudio_virtual_new operators (4 args)
//==============================================================================
void *operator new(size_t inReportedSize, const char *inType, const char *inFile, int inLine)
{
    return Q3DStudio::CMemoryFilter::Allocate(static_cast<Q3DStudio::INT32>(inReportedSize), inType,
                                              inFile, inLine, false);
}

void operator delete(void *inReportedAddress, const char *, const char *, int)
{
    Q3DStudio::CMemoryFilter::Free(inReportedAddress);
}
