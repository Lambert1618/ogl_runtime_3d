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
#ifndef QT3DS_RENDER_ALLOCATOR_H
#define QT3DS_RENDER_ALLOCATOR_H
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/Qt3DSMutex.h"
#include "EASTL/hash_map.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSLogging.h"

namespace qt3ds {
namespace foundation {

    // The simplest allocator.  Performs no alignment nor memory checking.
    struct MallocAllocator : public NVAllocatorCallback
    {
        void *allocate(size_t size, const char *, const char *, int, int = 0) override
        {
            return malloc(size);
        }
        void *allocate(size_t size, const char *, const char *, int, size_t, size_t) override
        {
            return malloc(size);
        }
        void deallocate(void *ptr) override { free(ptr); }
    };

    struct MemInfo
    {
        size_t size;
        void *originalAddress;
        const char *name;
        const char *file;
        int line;
        MemInfo(size_t _size = 0, void *addr = 0, const char *_name = "", const char *_file = "",
                int _line = 0)
            : size(_size)
            , originalAddress(addr)
            , name(_name)
            , file(_file)
            , line(_line)
        {
        }
    };

    struct FileAndLine
    {
        const char *mFile;
        QT3DSU32 mLine;
        FileAndLine(const char *f, QT3DSU32 l)
            : mFile(f)
            , mLine(l)
        {
        }
    };
} // namespace foundation
} // namespace qt3ds

namespace eastl {
template <>
struct hash<qt3ds::foundation::FileAndLine>
{
    size_t operator()(const qt3ds::foundation::FileAndLine &fl) const
    {
        return hash<const void *>()((const void *)fl.mFile) ^ hash<int>()(fl.mLine);
    }
};

template <>
struct equal_to<qt3ds::foundation::FileAndLine>
{
    bool operator()(const qt3ds::foundation::FileAndLine &fl1,
                    const qt3ds::foundation::FileAndLine &fl2) const
    {
        size_t fl1File(reinterpret_cast<size_t>(fl1.mFile));
        size_t fl2File(reinterpret_cast<size_t>(fl2.mFile));

        return fl1File == fl2File && fl1.mLine == fl2.mLine;
    }
};
}

namespace qt3ds {
namespace foundation {

    struct MemInfoAndCount : MemInfo
    {
        QT3DSU32 mCount;
        QT3DSU64 mTotalBytes;
        MemInfoAndCount(const MemInfo &info)
            : MemInfo(info)
            , mCount(1)
            , mTotalBytes(info.size)
        {
        }
        void increment(const MemInfo &otherInfo)
        {
            ++mCount;
            mTotalBytes += otherInfo.size;
        }
    };

    typedef nvhash_map<FileAndLine, MemInfoAndCount> TFileAndLineMemHash;

    class AllocationCheckPrinter
    {
    protected:
        virtual ~AllocationCheckPrinter() {}
    public:
        virtual void printMissing(const MemInfoAndCount &item) = 0;
        virtual void printIncorrectCount(const MemInfoAndCount &later,
                                         const MemInfoAndCount &earlier) = 0;
    };

    // All allocations are 16 byte aligned.
    // Prints out unallocated memory at destruction.
    class QT3DS_AUTOTEST_EXPORT CAllocator : public NVAllocatorCallback
    {
    public:
        typedef Mutex TMutexType;
        typedef Mutex::ScopedLock TLockType;
        typedef nvhash_map<void *, MemInfo> PtrToInfoMap;

    private:
        MallocAllocator mAlloc;
        PtrToInfoMap mAllocations;
        PtrToInfoMap mStoredAllocations;
        TMutexType mMutex;

    public:
        CAllocator()
            : mAllocations(mAlloc, "MemInfo")
            , mStoredAllocations(mAlloc, "MemInfo")
            , mMutex(mAlloc)
        {
        }
        virtual ~CAllocator();

        void *allocate(size_t size, const char *tn, const char *fl, int ln, int flags) override
        {
            QT3DS_ASSERT(size);
            if (size) {
                size += 15;
                TLockType locker(mMutex);
                void *original = mAlloc.allocate(size, tn, fl, ln, flags);
                size_t temp = (size_t)original;
                temp = (temp + 15) & (~15);
                void *retval = (void *)temp;
                mAllocations.insert(eastl::make_pair(retval, MemInfo(size, original, tn, fl, ln)));
                return retval;
            }
            return NULL;
        }
        void *allocate(size_t size, const char *tn, const char *fl, int ln, size_t, size_t) override
        {
            return allocate(size, tn, fl, ln, 0);
        }
        void deallocate(void *ptr) override
        {
            if (ptr) {
                TLockType locker(mMutex);
                PtrToInfoMap::iterator entry(mAllocations.find(ptr));
                if (entry != mAllocations.end()) {
                    mAlloc.deallocate(entry->second.originalAddress);
                    mAllocations.erase(ptr);
                } else {
                    QT3DS_ASSERT(false);
                }
            }
        }
        const PtrToInfoMap &getOutstandingAllocations() const { return mAllocations; }
        static void convert(const CAllocator::PtrToInfoMap &map, TFileAndLineMemHash &fl)
        {
            for (CAllocator::PtrToInfoMap::const_iterator iter = map.begin(), end = map.end();
                 iter != end; ++iter) {
                const MemInfo &info(iter->second);
                FileAndLine flKey(info.file, info.line);
                TFileAndLineMemHash::const_iterator find(fl.find(flKey));
                if (find == fl.end())
                    fl.insert(eastl::make_pair(flKey, MemInfoAndCount(info)));
                else
                    const_cast<MemInfoAndCount &>(find->second).increment(info);
            }
        }
        static void copyMap(const CAllocator::PtrToInfoMap &map1, CAllocator::PtrToInfoMap &map2)
        {
            for (CAllocator::PtrToInfoMap::const_iterator iter = map1.begin(), end = map1.end();
                 iter != end; ++iter) {
                map2.insert(eastl::make_pair(iter->first, iter->second));
            }
        }

        static void printDiffs(const TFileAndLineMemHash &fl1, const TFileAndLineMemHash &fl2,
                               AllocationCheckPrinter &printer)
        {
            using namespace std;
            for (TFileAndLineMemHash::const_iterator iter = fl1.begin(), end = fl1.end();
                 iter != end; ++iter) {
                TFileAndLineMemHash::const_iterator entry = fl2.find(iter->first);
                if (entry != fl2.end()) {
                    printer.printMissing(iter->second);
                } else if (iter->second.mCount > entry->second.mCount) {
                    printer.printIncorrectCount(iter->second, entry->second);
                }
            }
        }

        void printAllocationDiff(AllocationCheckPrinter &printer)
        {
            TFileAndLineMemHash map1fl(mAlloc, "printAllocationDiff");
            TFileAndLineMemHash map2fl(mAlloc, "printAllocationDiff");
            convert(mStoredAllocations, map1fl);
            convert(mAllocations, map2fl);
            printDiffs(map2fl, map1fl, printer);
        }
        void storeAllocations()
        {
            mStoredAllocations.clear();
            copyMap(mAllocations, mStoredAllocations);
        }
        bool hasStoredAllocations() { return mStoredAllocations.size() > 0; }
    };
}
}
#endif
