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
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/Qt3DSContainers.h"

namespace qt3ds {
namespace foundation {

    /**
     *	Allocator that allocates slabs of a certain size and objects then
     *	get allocated from those slabs.  This allocator is designed to reset,
     *	meaning every frame you can call reset and the objects get freed but
     *	not destructed.  This allocator does not call destructors!!
     */
    template <QT3DSU32 alignmentInBytes = 4, QT3DSU32 slabSize = 8192>
    struct SFastAllocator : NVAllocatorCallback
    {
        ForwardingAllocator m_Allocator;
        nvvector<QT3DSU8 *> m_Slabs;
        QT3DSU32 m_Offset;

        enum {
            SlabSize = slabSize,
        };

        static size_t getSlabSize() { return slabSize; }

        SFastAllocator(NVAllocatorCallback &alloc, const char *memName)
            : m_Allocator(alloc, memName)
            , m_Slabs(alloc, "SFastAllocator::m_Slabs")
            , m_Offset(0)
        {
        }

        ~SFastAllocator()
        {
            for (QT3DSU32 idx = 0, end = m_Slabs.size(); idx < end; ++idx)
                m_Allocator.deallocate(m_Slabs[idx]);
            m_Slabs.clear();
            m_Offset = 0;
        }
        void *allocate(size_t inSize, const char *inFile, int inLine, int inFlags = 0)
        {
            (void)inFlags;
            if (inSize > slabSize) {
                QT3DS_ASSERT(false);
                return NULL;
            }
            QT3DSU32 misalign = m_Offset % alignmentInBytes;
            if (misalign)
                m_Offset = m_Offset + (alignmentInBytes - misalign);

            QT3DSU32 currentSlab = m_Offset / slabSize;
            QT3DSU32 slabOffset = m_Offset % slabSize;
            QT3DSU32 amountLeftInSlab = slabSize - slabOffset;
            if (inSize > amountLeftInSlab) {
                ++currentSlab;
                slabOffset = 0;
                m_Offset = currentSlab * slabSize;
            }
            while (currentSlab >= m_Slabs.size()) {
                m_Slabs.push_back((QT3DSU8 *)m_Allocator.allocate(slabSize, inFile, inLine));
            }
            QT3DSU8 *data = m_Slabs[currentSlab] + slabOffset;
            // This would indicate the underlying allocator isn't handing back aligned memory.
            QT3DS_ASSERT(reinterpret_cast<size_t>(data) % alignmentInBytes == 0);
            m_Offset += (QT3DSU32)inSize;
            return data;
        }
        void *allocate(size_t size, const char * /*typeName*/, const char *filename, int line,
                       int flags = 0) override
        {
            return allocate(size, filename, line, flags);
        }
        void *allocate(size_t size, const char * /*typeName*/, const char *filename, int line,
                       size_t alignment, size_t /*alignmentOffset*/) override
        {
            QT3DS_ASSERT(alignment == alignmentInBytes);
            if (alignment == alignmentInBytes)
                return allocate(size, filename, line);
            return NULL;
        }
        // only reset works with deallocation
        void deallocate(void *) override {}
        void reset() { m_Offset = 0; }
    };
}
}
