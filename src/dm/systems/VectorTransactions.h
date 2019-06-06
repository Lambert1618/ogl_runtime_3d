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
#ifndef VECTORTRANSACTIONSH
#define VECTORTRANSACTIONSH

namespace qt3dsdm {
template <typename TItemType>
inline void VecInsert(std::vector<TItemType> &inItems, const TItemType &inItem, size_t inIndex)
{
    inItems.insert(inItems.begin() + inIndex, inItem);
}

template <typename TItemType>
inline void VecErase(std::vector<TItemType> &inItems, const TItemType &inItem)
{
    erase_if(inItems, std::bind(std::equal_to<TItemType>(), inItem, std::placeholders::_1));
}

template <typename TItemType>
inline void VecInsertTransaction(const char *inFile, int inLine, std::vector<TItemType> &inItems,
                                 size_t inIndex, TTransactionConsumerPtr &inConsumer)
{
    TItemType theItem = inItems.at(inIndex);
    TTransactionPtr theTransaction(DoCreateGenericTransaction(
        inFile, inLine, std::bind(VecInsert<TItemType>, std::ref(inItems), theItem, inIndex),
        std::bind(VecErase<TItemType>, std::ref(inItems), theItem)));
    inConsumer->OnTransaction(theTransaction);
}

template <typename TItemType>
inline void VecEraseTransaction(const char *inFile, int inLine, std::vector<TItemType> &inItems,
                                size_t inIndex, const TItemType &inItem,
                                TTransactionConsumerPtr &inConsumer)
{
    TTransactionPtr theTransaction(DoCreateGenericTransaction(
        inFile, inLine, std::bind(VecErase<TItemType>, std::ref(inItems), inItem),
        std::bind(VecInsert<TItemType>, std::ref(inItems), inItem, inIndex)));
    inConsumer->OnTransaction(theTransaction);
}

template <typename TItemType>
inline void CreateVecInsertTransaction(const char *inFile, int inLine,
                                       TTransactionConsumerPtr inConsumer, const TItemType &inItem,
                                       std::vector<TItemType> &inItems)
{
    using namespace std;
    size_t theDistance = distance(inItems.begin(), find(inItems.begin(), inItems.end(), inItem));
    RunWithConsumer(inConsumer, std::bind(VecInsertTransaction<TItemType>, inFile, inLine,
                                            std::ref(inItems), theDistance, std::placeholders::_1));
}

template <typename TItemType>
inline void CreateVecEraseTransaction(const char *inFile, int inLine,
                                      TTransactionConsumerPtr inConsumer, const TItemType &inItem,
                                      std::vector<TItemType> &inItems)
{
    using namespace std;
    size_t theDistance = distance(inItems.begin(), find(inItems.begin(), inItems.end(), inItem));
    RunWithConsumer(inConsumer,
                    std::bind(VecEraseTransaction<TItemType>, inFile, inLine, std::ref(inItems),
                                theDistance, std::ref(inItem), std::placeholders::_1));
}

template <typename TKeyType, typename TValueType, typename THashType>
struct HashMapAction
{
    std::unordered_map<TKeyType, TValueType, THashType> &m_HashMap;
    std::pair<TKeyType, TValueType> m_Value;

    HashMapAction(std::unordered_map<TKeyType, TValueType, THashType> &map,
                  const std::pair<TKeyType, TValueType> &val)
        : m_HashMap(map)
        , m_Value(val)
    {
    }
    bool Exists() { return m_HashMap.find(m_Value.first) != m_HashMap.end(); }
    void Add()
    {
        Q_ASSERT(!Exists());
        m_HashMap.insert(m_Value);
    }
    void Remove()
    {
        Q_ASSERT(Exists());
        m_HashMap.erase(m_HashMap.find(m_Value.first));
    }
};

template <typename TKeyType, typename TValueType, typename THashType>
struct HashMapInsertTransaction : public HashMapAction<TKeyType, TValueType, THashType>,
                                  public ITransaction,
                                  public IMergeableTransaction<TValueType>
{
    typedef HashMapAction<TKeyType, TValueType, THashType> base;
    HashMapInsertTransaction(const char *inFile, int inLine,
                             std::unordered_map<TKeyType, TValueType, THashType> &map,
                             const std::pair<TKeyType, TValueType> &val)
        : HashMapAction<TKeyType, TValueType, THashType>(map, val)
        , ITransaction(inFile, inLine)
    {
    }
    void Do() override { base::Add(); }
    void Undo() override { base::Remove(); }
    void Update(const TValueType &inValue) override { base::m_Value.second = inValue; }
};

template <typename TKeyType, typename TValueType, typename THashType>
struct HashMapEraseTransaction : public HashMapAction<TKeyType, TValueType, THashType>,
                                 public ITransaction
{
    typedef HashMapAction<TKeyType, TValueType, THashType> base;
    HashMapEraseTransaction(const char *inFile, int inLine,
                            std::unordered_map<TKeyType, TValueType, THashType> &map,
                            const std::pair<TKeyType, TValueType> &val)
        : HashMapAction<TKeyType, TValueType, THashType>(map, val)
        , ITransaction(inFile, inLine)
    {
    }
    void Do() override { base::Remove(); }
    void Undo() override { base::Add(); }
};

template <typename TKeyType, typename TValueType, typename THashType>
inline std::shared_ptr<IMergeableTransaction<TValueType>>
CreateHashMapInsertTransaction(const char *inFile, int inLine, TTransactionConsumerPtr inConsumer,
                               const std::pair<TKeyType, TValueType> &inItem,
                               std::unordered_map<TKeyType, TValueType, THashType> &inItems)
{
    using namespace std;
    std::shared_ptr<IMergeableTransaction<TValueType>> retval;
    if (inConsumer) {
        std::shared_ptr<HashMapInsertTransaction<TKeyType, TValueType, THashType>> transaction(
            std::make_shared<HashMapInsertTransaction<TKeyType, TValueType, THashType>>(
                inFile, inLine, std::ref(inItems), std::cref(inItem)));
        retval = static_pointer_cast<IMergeableTransaction<TValueType>>(transaction);
        inConsumer->OnTransaction(static_pointer_cast<ITransaction>(transaction));
    }
    return retval;
}

template <typename TKeyType, typename TValueType, typename THashType>
inline void
CreateHashMapEraseTransaction(const char *inFile, int inLine, TTransactionConsumerPtr inConsumer,
                              const std::pair<TKeyType, TValueType> &inItem,
                              std::unordered_map<TKeyType, TValueType, THashType> &inItems)
{
    using namespace std;
    if (inConsumer)
        inConsumer->OnTransaction(static_pointer_cast<ITransaction>(
            std::make_shared<HashMapEraseTransaction<TKeyType, TValueType, THashType>>(
                inFile, inLine, std::ref(inItems), std::cref(inItem))));
}

template <typename TKeyType, typename TValueType, typename THashType>
struct HashMapSwapTransaction : public ITransaction, public IMergeableTransaction<TValueType>
{
    typedef std::unordered_map<TKeyType, TValueType, THashType> TMapType;

    TMapType &m_HashMap;
    TKeyType m_Key;
    TValueType m_OldValue;
    TValueType m_NewValue;

    HashMapSwapTransaction(const char *inFile, int inLine, TMapType &inMap, const TKeyType &inKey,
                           const TValueType &inOldVal, const TValueType &inNewVal)
        : ITransaction(inFile, inLine)
        , m_HashMap(inMap)
        , m_Key(inKey)
        , m_OldValue(inOldVal)
        , m_NewValue(inNewVal)
    {
    }

    bool Exists() { return m_HashMap.find(m_Key) != m_HashMap.end(); }
    void SetValue(const TValueType &inVal)
    {
        typename TMapType::iterator find(m_HashMap.find(m_Key));
        if (find != m_HashMap.end())
            find->second = inVal;
        else
            m_HashMap.insert(std::make_pair(m_Key, inVal));
    }
    void Do() override { SetValue(m_NewValue); }
    void Undo() override { SetValue(m_OldValue); }
    void Update(const TValueType &inValue) override { m_NewValue = inValue; }
};

template <typename TKeyType, typename TValueType, typename THashType>
inline std::shared_ptr<IMergeableTransaction<TValueType>>
CreateHashMapSwapTransaction(const char *inFile, int inLine, TTransactionConsumerPtr inConsumer,
                             const TKeyType &inKey, const TValueType &inOldValue,
                             const TValueType &inNewValue,
                             std::unordered_map<TKeyType, TValueType, THashType> &inItems)
{
    using namespace std;
    std::shared_ptr<IMergeableTransaction<TValueType>> retval;
    if (inConsumer) {
        std::shared_ptr<HashMapSwapTransaction<TKeyType, TValueType, THashType>> transaction =
            std::make_shared<HashMapSwapTransaction<TKeyType, TValueType, THashType>>(
                inFile, inLine, std::ref(inItems), inKey, inOldValue, inNewValue);
        retval = static_pointer_cast<IMergeableTransaction<TValueType>>(transaction);
        inConsumer->OnTransaction(static_pointer_cast<ITransaction>(transaction));
    }
    return retval;
}
}

#endif
