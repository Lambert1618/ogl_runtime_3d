/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DS_IMPORT_CONTAINERS_H
#define QT3DS_IMPORT_CONTAINERS_H

#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/vector.h"
#include "foundation/Qt3DSDataRef.h"
#include "Qt3DSDMWStrOps.h"

namespace qt3dsimp {

#define QT3DSIMP_FOREACH(idxnm, val)                                                                  \
    for (QT3DSU32 idxnm = 0, __numItems = (QT3DSU32)val; idxnm < __numItems; ++idxnm)

template <typename TKey, typename TValue, typename THash = eastl::hash<TKey>,
          typename TPredicate = eastl::equal_to<TKey>>
struct ImportHashMap : public eastl::hash_map<TKey, TValue, THash, TPredicate>
{
    using base_type = eastl::hash_map<TKey, TValue, THash, TPredicate>;

    ImportHashMap() {}



    bool contains(const TKey &key) const { return find(key) != base_type::end(); }
};
template <typename TKey>
struct ImportHashSet : public eastl::hash_set<TKey, eastl::hash<TKey>, eastl::equal_to<TKey>>
{
    using base_type = eastl::hash_set<TKey, eastl::hash<TKey>, eastl::equal_to<TKey>>;
    ImportHashSet() {}
    bool contains(const TKey &key) const { return base_type::find(key) != base_type::end(); }
};
template <typename TValue>
struct ImportArray : public eastl::vector<TValue>
{
    using base_type = eastl::vector<TValue>;

    ImportArray() {}
    operator NVConstDataRef<TValue>() const
    {
        return NVConstDataRef<TValue>(base_type::data(), (QT3DSU32) base_type::size());
    }

    operator NVDataRef<TValue>() {
        return NVDataRef<TValue>(base_type::data(), (QT3DSU32) base_type::size());
    }
};

inline NVConstDataRef<wchar_t> toRef(const wchar_t *data, QT3DSU32 len = 0)
{
    if (IsTrivial(data))
        return NVConstDataRef<wchar_t>(L"", 0);
    len = len ? len : (QT3DSU32)wcslen(data);
    return NVConstDataRef<wchar_t>(data, len);
}
}

#endif
