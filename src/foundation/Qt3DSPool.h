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
#pragma once
#ifndef QT3DS_POOL_H
#define QT3DS_POOL_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSMath.h" //NVMax
#include "EASTL/vector.h"

namespace qt3ds {
namespace foundation {
    // Pool of fixed size objects.
    template <typename TObjType, typename TAllocator = EASTLAllocatorType,
              QT3DSU32 alignmentInBytes = 4, QT3DSU32 slabSize = 4096>
    class Pool
    {
    protected:
        TAllocator mAllocator;
        struct Link
        {
            Link *mNext;
            Link(Link *next = NULL)
                : mNext(next)
            {
            }
        };
        eastl::vector<void *, TAllocator> mSlabs;
        Link *mFreeList;

    public:
        Pool(const TAllocator &alloc = TAllocator())
            : mAllocator(alloc)
            , mSlabs(alloc)
            , mFreeList(NULL)
        {
            StaticAssert<sizeof(TObjType) < slabSize>::valid_expression();
        }
        ~Pool()
        {
            for (QT3DSU32 idx = 0, end = mSlabs.size(); idx < end; ++idx)
                mAllocator.deallocate(mSlabs[idx], slabSize);
            mSlabs.clear();
            mFreeList = NULL;
        }
        void appendToFreeList(Link *inLink)
        {
            inLink->mNext = mFreeList;
            mFreeList = inLink;
        }
        void appendSlabToFreeList(void *slab)
        {
            QT3DSU8 *newMem = reinterpret_cast<QT3DSU8 *>(slab);
            QT3DSU32 objSize = (QT3DSU32)qt3ds::NVMax(sizeof(TObjType), sizeof(Link));
            // align the memory correctly
            if (objSize % alignmentInBytes)
                objSize += alignmentInBytes - (objSize % alignmentInBytes);

            QT3DSU32 numObjsInSlab = slabSize / objSize;
            for (QT3DSU32 idx = 0; idx < numObjsInSlab; ++idx) {
                QT3DSU8 *objPtr = newMem + idx * objSize;
                appendToFreeList(reinterpret_cast<Link *>(objPtr));
            }
        }
        void allocateSlab(const char *file, int line)
        {
            QT3DSU32 objSize = (QT3DSU32)NVMax(sizeof(TObjType), sizeof(Link));
            // align the memory correctly
            if (objSize % alignmentInBytes)
                objSize += alignmentInBytes - (objSize % alignmentInBytes);

            QT3DSU8 *newMem = (QT3DSU8 *)mAllocator.allocate(slabSize, file, line);
            if (newMem == NULL)
                return; // out of mem, bad error
            mSlabs.push_back(newMem);
            appendSlabToFreeList(newMem);
        }

        void *allocate(const char *file, int line)
        {
            if (!mFreeList)
                allocateSlab(file, line);

            if (mFreeList) {
                Link *retval = mFreeList;
                mFreeList = retval->mNext;
                return retval;
            }

            return NULL;
        }

        void deallocate(void *inPtr)
        {
#if _DEBUG
            // Ensure inPtr came from a known slab.
            bool found = false;
            for (QT3DSU32 idx = 0, end = mSlabs.size(); idx < end && !found; ++idx) {
                QT3DSU8 *slabMem = reinterpret_cast<QT3DSU8 *>(mSlabs[idx]);
                QT3DSU8 *slabEnd = slabMem + slabSize;
                QT3DSU8 *memPtr = reinterpret_cast<QT3DSU8 *>(inPtr);

                if (memPtr >= mSlabs[idx] && memPtr < slabEnd)
                    found = true;
            }
            QT3DS_ASSERT(found);
#endif
            appendToFreeList(reinterpret_cast<Link *>(inPtr));
        }

        template <typename TArg1, typename TArg2>
        TObjType *construct(const TArg1 &arg1, const TArg2 &arg2, const char *file, int line)
        {
            TObjType *newMem = (TObjType *)allocate(file, line);
            return new (newMem) TObjType(arg1, arg2);
        }

        template <typename TArg1>
        TObjType *construct(const TArg1 &arg1, const char *file, int line)
        {
            TObjType *newMem = (TObjType *)allocate(file, line);
            return new (newMem) TObjType(arg1);
        }

        TObjType *construct(const char *file, int line)
        {
            TObjType *newMem = (TObjType *)allocate(file, line);
            return new (newMem) TObjType();
        }
    };
}
}
#endif
