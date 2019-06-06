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
#ifndef QT3DS_FOUNDATION_AUTO_DEALLOCATOR_ALLOCATOR_H
#define QT3DS_FOUNDATION_AUTO_DEALLOCATOR_ALLOCATOR_H
#pragma once

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"

namespace qt3ds {
namespace foundation {

    using eastl::make_pair;
    using eastl::pair;

    struct SSAutoDeallocatorAllocator : public NVAllocatorCallback
    {
        NVAllocatorCallback &m_Allocator;
        nvhash_map<void *, size_t> m_Allocations;
        SSAutoDeallocatorAllocator(NVFoundationBase &inFnd)
            : m_Allocator(inFnd.getAllocator())
            , m_Allocations(inFnd.getAllocator(), "SSAutoDeallocatorAllocator::m_Allocations")
        {
        }

        SSAutoDeallocatorAllocator(NVAllocatorCallback &inAlloc)
            : m_Allocator(inAlloc)
            , m_Allocations(inAlloc, "SSAutoDeallocatorAllocator::m_Allocations")
        {
        }

        // Automatically deallocates everything that hasn't already been deallocated.
        ~SSAutoDeallocatorAllocator() { deallocateAllAllocations(); }

        void deallocateAllAllocations()
        {
            for (nvhash_map<void *, size_t>::iterator iter = m_Allocations.begin(),
                                                      end = m_Allocations.end();
                 iter != end; ++iter)
                m_Allocator.deallocate(iter->first);
            m_Allocations.clear();
        }

        void *allocate(size_t size, const char *typeName, const char *filename, int line,
                               int flags = 0) override
        {
            void *value = m_Allocator.allocate(size, typeName, filename, line, flags);
            m_Allocations.insert(make_pair(value, size));
            return value;
        }

        void *allocate(size_t size, const char *typeName, const char *filename, int line,
                               size_t alignment, size_t alignmentOffset) override
        {
            void *value =
                m_Allocator.allocate(size, typeName, filename, line, alignment, alignmentOffset);
            m_Allocations.insert(make_pair(value, size));
            return value;
        }
        void deallocate(void *ptr) override
        {
            nvhash_map<void *, size_t>::iterator iter = m_Allocations.find(ptr);
            QT3DS_ASSERT(iter != m_Allocations.end());
            m_Allocator.deallocate(iter->first);
            m_Allocations.erase(iter);
        }
    };
}
}

#endif
