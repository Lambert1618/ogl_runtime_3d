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

#ifndef QT3DS_FOUNDATION_QT3DS_BROADCASTING_ALLOCATOR_H
#define QT3DS_FOUNDATION_QT3DS_BROADCASTING_ALLOCATOR_H

#include "foundation/Qt3DSAllocatorCallback.h"

/** \addtogroup foundation
  @{
*/
#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

/**
\brief Abstract listener class that listens to allocation and deallocation events from the
        foundation memory system.

<b>Threading:</b> All methods of this class should be thread safe as it can be called from the user
thread
or the physics processing thread(s).
*/
class NVAllocationListener
{
protected:
    virtual ~NVAllocationListener() {}

public:
    /**
    \brief callback when memory is allocated.
    \param size Size of the allocation in bytes.
    \param typeName Type this data is being allocated for.
    \param filename File the allocation came from.
    \param line the allocation came from.
    \param allocatedMemory memory that will be returned from the allocation.
    */
    virtual void onAllocation(size_t size, const char *typeName, const char *filename, int line,
                              void *allocatedMemory) = 0;

    /**
    /brief callback when memory is deallocated.
    /param allocatedMemory memory just before allocation.
    */
    virtual void onDeallocation(void *allocatedMemory) = 0;
};

/**
\brief Abstract base class for an application defined memory allocator that allows an external
listener
to audit the memory allocations.

<b>Threading:</b> Register/deregister are *not* threadsafe!!!
You need to be sure multiple threads are using this allocator when you are adding
new listeners.
*/
class NVBroadcastingAllocator : public NVAllocatorCallback
{
protected:
    virtual ~NVBroadcastingAllocator() {}

public:
    /**
    \brief Register an allocation listener.  This object will be notified whenever an
    allocation happens.

    <b>Threading:</b>Not threadsafe if you are allocating and deallocating in another
    thread using this allocator.
    */
    virtual void registerAllocationListener(NVAllocationListener &inListener) = 0;
    /**
    \brief Deregister an allocation listener.  This object will no longer receive
    notifications upon allocation.

    <b>Threading:</b>Not threadsafe if you are allocating and deallocating in another
    thread using this allocator.
    */
    virtual void deregisterAllocationListener(NVAllocationListener &inListener) = 0;
};

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_BROADCASTING_ALLOCATOR_H
