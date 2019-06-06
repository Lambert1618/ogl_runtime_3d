/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QT3DS_FOUNDATION_PSALLOCATOR_H
#define QT3DS_FOUNDATION_PSALLOCATOR_H

#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"

#if (defined(QT3DS_WINDOWS) | defined(QT3DS_X360))
#include <typeinfo.h>
#endif
#if (defined(QT3DS_APPLE))
#include <typeinfo>
#endif

#include <new>

// Allocation macros going through user allocator
#define QT3DS_ALLOC(alloc, n, name) alloc.allocate(n, name, __FILE__, __LINE__)
#define QT3DS_ALLOC_TEMP(alloc, n, name) QT3DS_ALLOC(n, name)
#define QT3DS_FREE(alloc, x) alloc.deallocate(x)
#define QT3DS_FREE_AND_RESET(x)                                                                       \
{                                                                                              \
    QT3DS_FREE(x);                                                                                \
    x = 0;                                                                                     \
    }

// The following macros support plain-old-types and classes derived from UserAllocated.
#define QT3DS_NEW(alloc, T) new (QT3DS_ALLOC(alloc, sizeof(T), #T)) T
#define QT3DS_NEW_TEMP(alloc, T) QT3DS_NEW(alloc, T)
#define QT3DS_DELETE_POD(x)                                                                           \
{                                                                                              \
    QT3DS_FREE(x);                                                                                \
    x = 0;                                                                                     \
    }

namespace qt3ds {
namespace foundation {
template <typename TObjType>
inline void NVDelete(NVAllocatorCallback &alloc, TObjType *item)
{
    if (item) {
        item->~TObjType();
        alloc.deallocate(item);
    }
}
}
}

//! placement new macro to make it easy to spot bad use of 'new'
#define QT3DS_PLACEMENT_NEW(p, T) new (p) T

// Don't use inline for alloca !!!
#ifdef QT3DS_WINDOWS
#include <malloc.h>
#define NVAlloca(x) _alloca(x)
#elif defined(QT3DS_LINUX) || defined(QT3DS_ANDROID) || defined(QT3DS_QNX)
#if defined(__INTEGRITY)
#include <alloca.h>
#else
#include <malloc.h>
#endif
#define NVAlloca(x) alloca(x)
#elif defined(QT3DS_PSP2)
#include <alloca.h>
#define NVAlloca(x) alloca(x)
#elif defined(QT3DS_APPLE)
#include <stdlib.h>
#include <alloca.h>
#define NVAlloca(x) alloca(x)
#elif defined(QT3DS_PS3)
#include <alloca.h>
#define NVAlloca(x) alloca(x)
#elif defined(QT3DS_X360)
#include <malloc.h>
#define NVAlloca(x) _alloca(x)
#elif defined(QT3DS_WII)
#include <alloca.h>
#define NVAlloca(x) alloca(x)
#endif

namespace qt3ds {
namespace foundation {
/*
     * Bootstrap allocator using malloc/free.
     * Don't use unless your objects get allocated before foundation is initialized.
     */
class RawAllocator
{
public:
    RawAllocator(const char * = 0) {}
    void *allocate(size_t size, const char *, int)
    {
#if defined(QT3DS_APPLE)
        // malloc returns valid pointer for size==0, no need to check
        return malloc(size);
#else
        // malloc returns valid pointer for size==0, no need to check
        return ::malloc(size);
#endif
    }
    void deallocate(void *ptr)
    {
#if defined(QT3DS_APPLE)
        free(ptr);
#else
        // free(0) is guaranteed to have no side effect, no need to check
        ::free(ptr);
#endif
    }
};

struct ForwardingAllocator
{
    NVAllocatorCallback *mAllocator;
    const char *mTypeName;
    ForwardingAllocator(const char *typeName)
        : mTypeName(typeName)
    {
    }
    ForwardingAllocator(NVAllocatorCallback &alloc, const char *typeName)
        : mAllocator(&alloc)
        , mTypeName(typeName)
    {
    }
    ForwardingAllocator(NVAllocatorCallback *alloc = NULL)
        : mAllocator(alloc)
        , mTypeName("__error__")
    {
        QT3DS_ASSERT(false);
    }
    ForwardingAllocator(const ForwardingAllocator &other)
        : mAllocator(other.mAllocator)
        , mTypeName(other.mTypeName)
    {
    }
    ForwardingAllocator &operator=(const ForwardingAllocator &other)
    {
        mAllocator = other.mAllocator;
        mTypeName = other.mTypeName;
        return *this;
    }
    bool operator==(const ForwardingAllocator &other) const
    {
        return mAllocator == other.mAllocator;
    }
    NVAllocatorCallback &getAllocator() { return *mAllocator; }
    // flags are unused
    void *allocate(size_t size, const char *filename, int line, int flags = 0)
    {
        return getAllocator().allocate(size, mTypeName, filename, line, flags);
    }
    void *allocate(size_t size, const char *filename, int line, size_t alignment,
                   size_t alignmentOffset)
    {
        return getAllocator().allocate(size, mTypeName, filename, line, alignment,
                                       alignmentOffset);
    }
    void deallocate(void *ptr, size_t) { getAllocator().deallocate(ptr); }
    void deallocate(void *ptr) { getAllocator().deallocate(ptr); }
};

} // namespace foundation
} // namespace qt3ds

#endif
