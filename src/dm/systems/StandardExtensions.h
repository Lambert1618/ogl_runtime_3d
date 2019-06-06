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
#ifndef STANDARDEXTENSIONSH
#define STANDARDEXTENSIONSH
#include <algorithm>

namespace qt3dsdm {
template <typename TContainer, typename TPred>
inline void erase_if(TContainer &inContainer, TPred inPred)
{
    inContainer.erase(std::remove_if(inContainer.begin(), inContainer.end(), inPred),
                      inContainer.end());
}

template <typename TRetType, typename TContainerType, typename TPred>
inline TRetType find_if(TContainerType &inContainer, TPred inPred)
{
    return std::find_if(inContainer.begin(), inContainer.end(), inPred);
}

template <typename TContainer, typename TTransaction>
inline void do_all(TContainer &inContainer, TTransaction inTransaction)
{
    std::for_each(inContainer.begin(), inContainer.end(), inTransaction);
}

template <typename TNumType, typename TTransaction>
inline void do_times(TNumType numberOfTimes, TTransaction inTransaction)
{
    for (TNumType index = 0; index < numberOfTimes; ++index)
        inTransaction(index);
}

template <typename TCountFunc, typename TItemByIndexFunc, typename TTransaction>
inline void for_each_item(TCountFunc inCountFunc, TItemByIndexFunc inItemByIndex,
                          TTransaction inTransaction)
{
    int theEnd = static_cast<int>(inCountFunc());
    for (int index = 0; index < theEnd; ++index)
        inTransaction(inItemByIndex(index));
}

template <typename TContainerType, typename TItemType>
inline void insert_unique(TContainerType &inContainer, const TItemType &inItem)
{
    if (find(inContainer.begin(), inContainer.end(), inItem) == inContainer.end())
        inContainer.insert(inContainer.end(), inItem);
}

template <typename TContainerType, typename TItemType, typename TPred>
inline void insert_unique_if(TContainerType &inContainer, const TItemType &inItem, TPred inPred)
{
    if (find_if(inContainer.begin(), inContainer.end(), inPred) == inContainer.end())
        inContainer.insert(inContainer.end(), inItem);
}

template <typename TItemType>
inline TItemType identity(const TItemType &inValue)
{
    return inValue;
}

template <typename TItemType, typename TOutContainerType>
inline void transformv_all(const std::vector<TItemType> &inSource, TOutContainerType &outDest)
{
    outDest.resize(inSource.size());
    std::transform(inSource.begin(), inSource.end(), outDest.begin(), identity<TItemType>);
}

template <typename TContainerType, typename Pred>
inline bool exists(TContainerType &inContainer, Pred inPredicate)
{
    return std::find_if(inContainer.begin(), inContainer.end(), inPredicate) != inContainer.end();
}

// Always return a default value (useful for default true for false)
template <typename TRetType>
inline TRetType always(TRetType inValue)
{
    return inValue;
}

template <typename TRetType, typename TIgnoreType>
inline TRetType always_ignore(TIgnoreType, TRetType inValue)
{
    return inValue;
}

template <typename TArgument, typename TTransaction, typename TPredicate>
inline void predicate_apply(TArgument inArgument, TTransaction inTransaction,
                            TPredicate inPredicate)
{
    if (inPredicate(inArgument))
        inTransaction(inArgument);
}

template <typename TItemType>
inline void assign_to(const TItemType &inSource, TItemType &inDest)
{
    inDest = inSource;
}

template <typename TPredType, typename TArgType>
bool complement(TPredType inPredicate, TArgType inArgument)
{
    return !(inPredicate(inArgument));
}

template <typename TItemType, typename TPredicate>
void insert_or_update(const TItemType &inItem, std::vector<TItemType> &inItems,
                      TPredicate inPredicate)
{
    typename std::vector<TItemType>::iterator theIter =
        find_if<typename std::vector<TItemType>::iterator>(inItems, inPredicate);
    if (theIter == inItems.end())
        inItems.push_back(inItem);
    else
        *theIter = inItem;
}

template <typename TItemType>
typename std::vector<TItemType>::iterator binary_sort_find(std::vector<TItemType> &inItems,
                                                           const TItemType &inItem)
{
    typedef typename std::vector<TItemType>::iterator TIterType;
    TIterType insertPos = std::lower_bound(inItems.begin(), inItems.end(), inItem);
    if (insertPos != inItems.end() && *insertPos == inItem)
        return insertPos;
    return inItems.end();
}

template <typename TItemType, typename TPredicate>
std::pair<typename std::vector<TItemType>::iterator, bool>
binary_sort_insert_unique(std::vector<TItemType> &inItems, const TItemType &inItem,
                          TPredicate inPredicate)
{
    typedef typename std::vector<TItemType>::iterator TIterType;
    TIterType insertPos = std::lower_bound(inItems.begin(), inItems.end(), inItem, inPredicate);
    // OK, insertPos can equal begin, it can equal end, or somewhere in between.
    // If it doesn't equal end, then we may be pointing at the object and we let
    // the caller figure out what to do.  Else we insert
    if (insertPos != inItems.end() && *insertPos == inItem)
        return std::make_pair(insertPos, false);
    size_t diff = insertPos - inItems.begin();
    inItems.insert(insertPos, inItem);
    return std::make_pair(inItems.begin() + diff, true);
}

template <typename TItemType>
std::pair<typename std::vector<TItemType>::iterator, bool>
binary_sort_insert_unique(std::vector<TItemType> &inItems, const TItemType &inItem)
{
    return binary_sort_insert_unique(inItems, inItem, std::less<TItemType>());
}

template <typename TItemType>
bool binary_sort_erase(std::vector<TItemType> &inItems, const TItemType &inItem)
{
    typedef typename std::vector<TItemType>::iterator TIterType;
    TIterType insertPos = binary_sort_find(inItems, inItem);
    if (insertPos != inItems.end()) {
        inItems.erase(insertPos);
        return true;
    }
    return false;
}
}

#endif
