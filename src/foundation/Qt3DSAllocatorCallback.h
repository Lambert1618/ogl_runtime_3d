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

#ifndef QT3DS_FOUNDATION_QT3DS_ALLOCATOR_CALLBACK_H
#define QT3DS_FOUNDATION_QT3DS_ALLOCATOR_CALLBACK_H

/** \addtogroup foundation
@{
*/

#include "foundation/Qt3DS.h"
#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

/**
\brief Abstract base class for an application defined memory allocator that can be used by the NV
library.

\note The SDK state should not be modified from within any allocation/free function.

<b>Threading:</b> All methods of this class should be thread safe as it can be called from the user
thread
or the physics processing thread(s).
*/

class NVAllocatorCallback
{
public:
    /**
    \brief destructor
    */
    virtual ~NVAllocatorCallback() {}

    /**
    \brief Allocates size bytes of memory, which must be 16-byte aligned.

    This method should never return NULL.  If you run out of memory, then
    you should terminate the app or take some other appropriate action.

    <b>Threading:</b> This function should be thread safe as it can be called in the context of the
    user thread
    and physics processing thread(s).

    \param size			Number of bytes to allocate.
    \param typeName		Name of the datatype that is being allocated
    \param filename		The source file which allocated the memory
    \param line			The source line which allocated the memory
    \return				The allocated block of memory.
    */
    virtual void *allocate(size_t size, const char *typeName, const char *filename, int line,
                           int flags = 0) = 0;
    virtual void *allocate(size_t size, const char *typeName, const char *filename, int line,
                           size_t alignment, size_t alignmentOffset) = 0;

    /**
    \brief Frees memory previously allocated by allocate().

    <b>Threading:</b> This function should be thread safe as it can be called in the context of the
    user thread
    and physics processing thread(s).

    \param ptr Memory to free.
    */
    virtual void deallocate(void *ptr) = 0;
};

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif
