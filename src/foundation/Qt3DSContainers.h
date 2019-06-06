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
#ifndef QT3DS_CONTAINERS_H
#define QT3DS_CONTAINERS_H
#include "foundation/Qt3DS.h"
#include "EASTL/vector.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace foundation {

    template <typename T>
    class nvvector : public eastl::vector<T, ForwardingAllocator>
    {
        typedef eastl::vector<T, ForwardingAllocator> base_type;

    public:
        using base_type::data;
        using base_type::size;

        nvvector(NVAllocatorCallback &inAllocator, const char *inTypeName)
            : eastl::vector<T, ForwardingAllocator>(ForwardingAllocator(inAllocator, inTypeName))
        {
        }
        nvvector(const nvvector<T> &inOther)
            : eastl::vector<T, ForwardingAllocator>(inOther)
        {
        }
        nvvector<T> &operator=(const nvvector<T> &inOther)
        {
            eastl::vector<T, ForwardingAllocator>::operator=(inOther);
            return *this;
        }
        operator NVConstDataRef<T>() const { return NVConstDataRef<T>(data(), size()); }
        operator NVDataRef<T>() { return NVDataRef<T>(data(), size()); }
    };

    template <typename TKey, typename TValue, typename THash = eastl::hash<TKey>,
              typename TPredicate = eastl::equal_to<TKey>>
    class nvhash_map : public eastl::hash_map<TKey, TValue, THash, TPredicate, ForwardingAllocator>
    {
        typedef eastl::hash_map<TKey, TValue, THash, TPredicate, ForwardingAllocator> base_type;

    public:
        using base_type::find;
        using base_type::end;

        nvhash_map(NVAllocatorCallback &inAllocator, const char *inTypeName)
            : eastl::hash_map<TKey, TValue, THash, TPredicate, ForwardingAllocator>(
                  ForwardingAllocator(inAllocator, inTypeName))
        {
        }
        nvhash_map(const nvhash_map<TKey, TValue, THash, TPredicate> &inOther)
            : eastl::hash_map<TKey, TValue, THash, TPredicate, ForwardingAllocator>(inOther)
        {
        }
        nvhash_map<TKey, TValue, THash, TPredicate> &
        operator=(const nvhash_map<TKey, TValue, THash, TPredicate> &inOther)
        {
            eastl::hash_map<TKey, TValue, THash, TPredicate, ForwardingAllocator>::operator=(
                inOther);
            return *this;
        }
        bool contains(const TKey &inKey) const { return find(inKey) != end(); }
    };

    template <typename TKey, typename THash = eastl::hash<TKey>,
              typename TPredicate = eastl::equal_to<TKey>>
    class nvhash_set : public eastl::hash_set<TKey, THash, TPredicate, ForwardingAllocator>
    {
        typedef eastl::hash_set<TKey, THash, TPredicate, ForwardingAllocator> base_type;

    public:
        using base_type::find;
        using base_type::end;

        nvhash_set(NVAllocatorCallback &inAllocator, const char *inTypeName)
            : eastl::hash_set<TKey, THash, TPredicate, ForwardingAllocator>(
                  ForwardingAllocator(inAllocator, inTypeName))
        {
        }
        nvhash_set(const nvhash_map<TKey, THash, TPredicate> &inOther)
            : eastl::hash_set<TKey, THash, TPredicate, ForwardingAllocator>(inOther)
        {
        }
        nvhash_set<TKey, THash, TPredicate> &
        operator=(const nvhash_set<TKey, THash, TPredicate> &inOther)
        {
            eastl::hash_set<TKey, THash, TPredicate, ForwardingAllocator>::operator=(inOther);
            return *this;
        }
        bool contains(const TKey &inKey) const { return find(inKey) != end(); }
    };
}
}

#endif
