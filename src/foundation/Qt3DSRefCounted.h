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

#ifndef QT3DS_FOUNDATION_QT3DS_REFCOUNTED_H
#define QT3DS_FOUNDATION_QT3DS_REFCOUNTED_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSNoCopy.h"

namespace qt3ds {
namespace foundation {
    /**
            Marker class for objects that have a release method that is expected
            to destroy or release the object.
    */
    class NVReleasable
    {
    protected:
        virtual ~NVReleasable() {}
    public:
        virtual void release() = 0;
    };

    template <typename TObjType>
    inline void NVSafeRelease(TObjType *&item)
    {
        if (item) {
            item->release();
            item = NULL;
        }
    }

    /**Scoped pointer that releases its data
            when it is being destroyed*/
    template <typename TObjType>
    struct NVScopedReleasable : public NoCopy
    {
        TObjType *mPtr;
        NVScopedReleasable()
            : mPtr(NULL)
        {
        }
        NVScopedReleasable(TObjType *item)
            : mPtr(item)
        {
        }
        NVScopedReleasable(TObjType &item)
            : mPtr(&item)
        {
        }
        ~NVScopedReleasable() { NVSafeRelease(mPtr); }

        NVScopedReleasable &operator=(TObjType *inItem)
        {
            if (inItem != mPtr) {
                if (mPtr)
                    mPtr->release();
                mPtr = inItem;
            }
            return *this;
        }

        NVScopedReleasable &operator=(const NVScopedReleasable<TObjType> inItem)
        {
            QT3DS_ASSERT(false);
            // try to do the right thing.
            mPtr = inItem.mPtr;
            const_cast<NVScopedReleasable<TObjType> &>(inItem).mPtr = NULL;
            return *this;
        }

        TObjType *forget_unsafe()
        {
            mPtr = NULL;
            return mPtr;
        }

        TObjType *operator->() { return mPtr; }
        const TObjType *operator->() const { return mPtr; }
        TObjType &operator*() { return *mPtr; }
        const TObjType &operator*() const { return *mPtr; }
        operator TObjType *() { return mPtr; }
        operator const TObjType *() const { return mPtr; }
    };

    // Marker class for objects that are ref counted.
    class NVRefCounted : public NVReleasable
    {
    public:
        virtual void addRef() = 0;
    };

/**Helpers to make implementing ref counted objects as concise as possible*/
#define QT3DS_IMPLEMENT_REF_COUNT_RELEASE(alloc)                                                      \
    QT3DSI32 value = atomicDecrement(&mRefCount);                                                     \
    if (value <= 0)                                                                                \
        NVDelete(alloc, this);

#define QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(alloc)                                               \
    void addRef() { atomicIncrement(&mRefCount); }                                                 \
    void release() { QT3DS_IMPLEMENT_REF_COUNT_RELEASE(alloc); }


#define QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(alloc)                                               \
    void addRef() override { atomicIncrement(&mRefCount); }                                                 \
    void release() override { QT3DS_IMPLEMENT_REF_COUNT_RELEASE(alloc); }

    /**Safe function that checks for null before addrefing the object*/
    template <typename TObjType>
    inline TObjType *NVSafeAddRef(TObjType *item)
    {
        if (item) {
            item->addRef();
        }
        return item;
    }

    /**Scoped pointer that addref's its data upon acquisition and releases its data
            when it is being destroyed*/
    template <typename TObjType>
    struct NVScopedRefCounted
    {
        TObjType *mPtr;
        ~NVScopedRefCounted() { NVSafeRelease(mPtr); }
        NVScopedRefCounted(TObjType *item = NULL)
            : mPtr(item)
        {
            NVSafeAddRef(mPtr);
        }
        NVScopedRefCounted(TObjType &item)
            : mPtr(&item)
        {
            NVSafeAddRef(mPtr);
        }
        NVScopedRefCounted(const NVScopedRefCounted<TObjType> &other)
            : mPtr(const_cast<TObjType *>(other.mPtr))
        {
            NVSafeAddRef(mPtr);
        }
        NVScopedRefCounted<TObjType> &operator=(const NVScopedRefCounted<TObjType> &other)
        {
            if (other.mPtr != mPtr) {
                NVSafeRelease(mPtr);
                mPtr = const_cast<TObjType *>(other.mPtr);
                NVSafeAddRef(mPtr);
            }
            return *this;
        }
        TObjType *forget_unsafe()
        {
            mPtr = NULL;
            return mPtr;
        }

        TObjType *operator->() { return mPtr; }
        const TObjType *operator->() const { return mPtr; }
        TObjType &operator*() { return *mPtr; }
        const TObjType &operator*() const { return *mPtr; }
        operator TObjType *() { return mPtr; }
        operator const TObjType *() const { return mPtr; }
        bool operator==(NVScopedRefCounted<TObjType> &inOther) const
        {
            return mPtr == inOther.mPtr;
        }
        bool operator!=(NVScopedRefCounted<TObjType> &inOther) const
        {
            return mPtr != inOther.mPtr;
        }
    };
}
}

#endif
