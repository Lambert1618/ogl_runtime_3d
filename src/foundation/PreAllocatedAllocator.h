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
#ifndef QT3DS_RENDER_PRE_ALLOCATED_ALLOCATOR_H
#define QT3DS_RENDER_PRE_ALLOCATED_ALLOCATOR_H
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace foundation {

    // Ignores deallocate calls if they originate withing the pre-allocation block
    struct SPreAllocatedAllocator : public NVAllocatorCallback
    {
        NVAllocatorCallback &m_Allocator;
        NVDataRef<QT3DSU8> m_PreAllocatedBlock;
        bool m_OwnsMemory; // then we attempt to deallocate on destruction

        SPreAllocatedAllocator(NVAllocatorCallback &inAllocator)
            : m_Allocator(inAllocator)
            , m_OwnsMemory(false)
        {
        }

        SPreAllocatedAllocator(NVAllocatorCallback &inAllocator, NVDataRef<QT3DSU8> inData,
                               bool inOwnsMemory)
            : m_Allocator(inAllocator)
            , m_PreAllocatedBlock(inData)
            , m_OwnsMemory(inOwnsMemory)
        {
        }

        virtual ~SPreAllocatedAllocator()
        {
            if (m_OwnsMemory)
                m_Allocator.deallocate(m_PreAllocatedBlock.begin());
        }

        void *allocate(size_t size, const char *typeName, const char *filename, int line,
                               int flags = 0) override
        {
            return m_Allocator.allocate(size, typeName, filename, line, flags);
        }

        void *allocate(size_t size, const char *typeName, const char *filename, int line,
                               size_t alignment, size_t alignmentOffset) override
        {
            return m_Allocator.allocate(size, typeName, filename, line, alignment, alignmentOffset);
        }

        void deallocate(void *ptr) override
        {
            if (!ptr)
                return;
            if (ptr < m_PreAllocatedBlock.begin() || ptr >= m_PreAllocatedBlock.end())
                m_Allocator.deallocate(ptr);
        }
    };
}
}

#endif
