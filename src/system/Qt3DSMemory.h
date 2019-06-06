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
#ifndef _INTEGRITYPLATFORM
#include <memory.h>
#endif
#include "Qt3DSMemoryHeap.h"
#include "Qt3DSMemoryManager.h"
#include "Qt3DSMemoryFilter.h"
#include "Qt3DSMemoryStatistics.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Single overridable exit point for all low level memory calls.
 */
class CMemory
{
    //==============================================================================
    //	Typedefs
    //==============================================================================
public:
    typedef void *(*TMalloc)(size_t inSize);
    typedef void (*TFree)(void *inPtr);
    typedef void *(*TRealloc)(void *inPtr, size_t inSize);

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    static TMalloc s_Malloc; ///< function pointer to malloc operation
    static TFree s_Free; ///< function pointer to free operation
    static TRealloc s_Realloc; ///< function pointer to realloc operation

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Memory override
    static void SetMemoryFunctions(const TMalloc inMalloc, const TFree inFree,
                                   const TRealloc inRealloc);

public: // Function access
    static TMalloc Malloc() { return s_Malloc; }
    static TFree Free() { return s_Free; }
    static TRealloc Realloc() { return s_Realloc; }
};

} // namespace Q3DStudio

//==============================================================================
//	Globals
//==============================================================================
Q3DStudio::CMemoryManager &GetMemoryManager();
Q3DStudio::CMemoryHeap &GetMemoryHeap();

//==============================================================================
//	Handlers
//==============================================================================
#ifndef Q3DStudio_HANDLER_NEW
#define Q3DStudio_HANDLER_NEW GetMemoryManager()
#endif // Q3DStudio_HANDLER_NEW

#ifndef Q3DStudio_HANDLER_ALLOC
#define Q3DStudio_HANDLER_ALLOC GetMemoryManager()
#endif // Q3DStudio_HANDLER_ALLOC

#ifndef Q3DStudio_HANDLER_FILTER
#define Q3DStudio_HANDLER_FILTER GetMemoryManager()
#endif // Q3DStudio_HANDLER_FILTER

//==============================================================================
//	Q3DStudio new defines
//==============================================================================
#undef new
#undef delete

#ifndef Q3DStudio_new
#define Q3DStudio_new(type) new (sizeof(type), "class " #type, __FILE__, __LINE__)
#endif // Q3DStudio_new

#ifndef Q3DStudio_delete
#define Q3DStudio_delete(ptr, type)                                                                \
    if (ptr) {                                                                                     \
        reinterpret_cast<type *>(ptr)->~type();                                                    \
        Q3DStudio_HANDLER_NEW.Free(ptr, sizeof(type));                                             \
    }
#endif // Q3DStudio_delete

#ifndef Q3DStudio_virtual_new
#define Q3DStudio_virtual_new(type) new ("class " #type, __FILE__, __LINE__)
#endif // Q3DStudio_virtual_new

#ifndef Q3DStudio_virtual_delete
#define Q3DStudio_virtual_delete(ptr, type)                                                        \
    if (ptr) {                                                                                     \
        static_cast<type *>(ptr)->~type();                                                         \
        Q3DStudio::CMemoryFilter::Free(ptr);                                                       \
    }
#endif // Q3DStudio_virtual_delete

//==============================================================================
//	Q3DStudio alloc defines
//==============================================================================
#ifndef Q3DStudio_allocate
#define Q3DStudio_allocate(type, count)                                                            \
    reinterpret_cast<type *>(Q3DStudio_HANDLER_ALLOC.Allocate(                                     \
        static_cast<Q3DStudio::INT32>(sizeof(type) * (count)), #type "[]", __FILE__, __LINE__))
#endif // Q3DStudio_allocate

#ifndef Q3DStudio_allocate_desc
#define Q3DStudio_allocate_desc(type, count, desc)                                                 \
    reinterpret_cast<type *>(Q3DStudio_HANDLER_ALLOC.Allocate(                                     \
        static_cast<Q3DStudio::INT32>(sizeof(type) * (count)), desc, __FILE__, __LINE__))
#endif // Q3DStudio_allocate_desc

#ifndef Q3DStudio_reallocate
#define Q3DStudio_reallocate(ptr, type, oldcount, newcount)                                        \
    reinterpret_cast<type *>(Q3DStudio_HANDLER_ALLOC.Reallocate(                                   \
        ptr, static_cast<Q3DStudio::INT32>(sizeof(type) * (oldcount)),                             \
        static_cast<INT32>(sizeof(type) * (newcount)), #type "[]", __FILE__, __LINE__))
#endif // Q3DStudio_reallocate

#ifndef Q3DStudio_reallocate_desc
#define Q3DStudio_reallocate_desc(ptr, type, oldcount, newcount, desc)                             \
    reinterpret_cast<type *>(Q3DStudio_HANDLER_ALLOC.Reallocate(                                   \
        ptr, static_cast<Q3DStudio::INT32>(sizeof(type) * (oldcount)),                             \
        static_cast<INT32>(sizeof(type) * (newcount)), desc, __FILE__, __LINE__))
#endif // Q3DStudio_reallocate_desc

#ifndef Q3DStudio_free
#define Q3DStudio_free(ptr, type, count)                                                           \
    Q3DStudio_HANDLER_ALLOC.Free(ptr, static_cast<Q3DStudio::INT32>(sizeof(type) * (count)))
#endif // Q3DStudio_free

//==============================================================================
//	Q3DStudio_new - operator prototypes (5 args)
//==============================================================================
void *operator new(size_t inReportedSize, size_t inOfficialSize, const char *inType,
                   const char *inFile, int inLine);
void operator delete(void *inReportedAddress, size_t inOfficialSize, const char *, const char *,
                     int);

//==============================================================================
//	Q3DStudio_virtual_new and new - operator prototypes (4 args)
//==============================================================================
void *operator new(size_t inReportedSize, const char *inType, const char *inFile, int inLine);
void operator delete(void *inReportedAddress, const char *, const char *, int);
