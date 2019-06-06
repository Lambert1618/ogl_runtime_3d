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
#ifndef QT3DSDM_TRANSACTIONS_H
#define QT3DSDM_TRANSACTIONS_H
#include "Qt3DSDMDataTypes.h"
#include "StandardExtensions.h"
#include <functional>

namespace qt3dsdm {

/**
 *	Transaction is some small entity that changes data.  A transaction consumer is expected to
 *execute
 *	a list of these either forward on a "redo" command or backwards on an "undo" command.
 *
 *	Transactions merely change data.  There are two other lists on consumers for "do"
 *notifications
 *	and "undo" notifications.  These lists control the events sent out to the UI about what
 *actually
 *	changed in the model.  Note that undo/do notifications may be more clever than simple
 *send-signal
 *	commands in that they may check if an object is alive or not before sending a notification
 *and may
 *	decline to send a notification if the target object is not currently alive.
 *
 *	Currently the undo/redo system first executes all of the transactions and then sends the
 *appropriate
 *	notifications.  This means that when the UI receives any notification, the model is in its
 *most-updated
 *	state.
 */
class ITransaction
{
public:
    const char *m_File;
    int m_Line;

    ITransaction(const char *inFile, int inLine)
        : m_File(inFile)
        , m_Line(inLine)
    {
    }
    virtual ~ITransaction() {}
    virtual void Do() = 0;
    virtual void Undo() = 0;
};

/**
 *	Merging allows us to efficient handle situations where the users are continuously
 *	modifying an object (or set of objects).  These are referred to as live-update
 *	scenarios.  Dragging an object in the 3d view or dragging keyframes in the timeline
 *	would be an example of live-update scenarios.
 */
template <typename TValueType>
class IMergeableTransaction
{
public:
    virtual ~IMergeableTransaction() {}
    // Called when a new value has arrived and we would rather update
    // an existing representation rather than create a new one.
    virtual void Update(const TValueType &inValue) = 0;
};

/**
 *	A consumer is an object that records the transaction.  This interface keeps the
 *	base producer objects from binding to how the transaction or the notifications
 *	are stored.
 */
class ITransactionConsumer
{
public:
    virtual ~ITransactionConsumer() {}
    virtual void OnTransaction(std::shared_ptr<ITransaction> inTransaction) = 0;
    // Notifications to be sent for undo/redo  These are used to
    // notify clients that something is different.
    virtual void OnDoNotification(std::function<void()> inNotification) = 0;
    virtual void OnUndoNotification(std::function<void()> inNotification) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation of helper objects and functions.
///////////////////////////////////////////////////////////////////////////////

typedef std::vector<std::function<void()>> TVoidFunctionList;

typedef std::shared_ptr<ITransactionConsumer> TTransactionConsumerPtr;

class ITransactionProducer
{
public:
    virtual ~ITransactionProducer() {}
    virtual void SetConsumer(TTransactionConsumerPtr inConsumer) = 0;
};

template <typename TDoTransaction, typename TUndoTransaction>
class CGenericTransaction : public ITransaction
{
    Q_DISABLE_COPY(CGenericTransaction)

    TUndoTransaction m_UndoTransaction;
    TDoTransaction m_DoTransaction;

public:
    CGenericTransaction(const char *inFile, int inLine, TDoTransaction inDo,
                        TUndoTransaction inUndo)
        : ITransaction(inFile, inLine)
        , m_UndoTransaction(inUndo)
        , m_DoTransaction(inDo)
    {
    }
    void Do() override { m_DoTransaction(); }
    void Undo() override { m_UndoTransaction(); }
};

template <typename TDoTransaction, typename TUndoTransaction>
ITransaction *DoCreateGenericTransaction(const char *inFile, int inLine, TDoTransaction inDo,
                                         TUndoTransaction inUndo)
{
    return static_cast<ITransaction *>(
        new CGenericTransaction<TDoTransaction, TUndoTransaction>(inFile, inLine, inDo, inUndo));
}

#define CREATE_GENERIC_TRANSACTION(inDo, inUndo)                                                   \
    DoCreateGenericTransaction(__FILE__, __LINE__, inDo, inUndo)

typedef std::shared_ptr<ITransaction> TTransactionPtr;
typedef std::vector<TTransactionPtr> TTransactionPtrList;

struct CTransactionConsumer : public ITransactionConsumer
{
    TTransactionPtrList m_TransactionList;
    TVoidFunctionList m_DoNotifications;
    TVoidFunctionList m_UndoNotifications;
    void OnTransaction(TTransactionPtr inTransaction) override
    {
        m_TransactionList.push_back(inTransaction);
    }
    void OnDoNotification(std::function<void()> inNotification) override
    {
        m_DoNotifications.push_back(inNotification);
    }
    void OnUndoNotification(std::function<void()> inNotification) override
    {
        m_UndoNotifications.push_back(inNotification);
    }

    // Merge with another CTransactionConsumer
    virtual void Merge(const ITransactionConsumer *inConsumer)
    {
        const CTransactionConsumer *theConsumer =
            static_cast<const CTransactionConsumer *>(inConsumer);
        m_TransactionList.insert(m_TransactionList.begin(), theConsumer->m_TransactionList.begin(),
                                 theConsumer->m_TransactionList.end());
    }
    void Reset()
    {
        m_TransactionList.clear();
        m_DoNotifications.clear();
        m_UndoNotifications.clear();
    }
};

template <typename TTransactionType>
inline void RunWithConsumer(TTransactionConsumerPtr inConsumer, TTransactionType inTransaction)
{
    if (inConsumer)
        inTransaction(inConsumer);
}

template <typename TDoTransaction, typename TUndoTransaction>
inline void CreateGenericTransactionWithConsumer(const char *inFile, int inLine,
                                                 TTransactionConsumerPtr inConsumer,
                                                 TDoTransaction inDoTransaction,
                                                 TUndoTransaction inUndoTransaction)
{
    if (inConsumer)
        inConsumer->OnTransaction(TTransactionPtr(
            DoCreateGenericTransaction(inFile, inLine, inDoTransaction, inUndoTransaction)));
}

template <typename TItemType>
inline void DoSetConsumer(TTransactionConsumerPtr inConsumer,
                          std::shared_ptr<TItemType> inTypePtr)
{
    ITransactionProducer *theProducer = dynamic_cast<ITransactionProducer *>(inTypePtr.get());
    if (theProducer)
        theProducer->SetConsumer(inConsumer);
}

template <typename TContainer>
inline void Undo(TContainer &inTransactions)
{
    std::for_each(inTransactions.rbegin(), inTransactions.rend(), std::bind(&ITransaction::Undo,
                                                                            std::placeholders::_1));
}

template <typename TContainer>
inline void Redo(TContainer &inTransactions)
{
    do_all(inTransactions, std::bind(&ITransaction::Do, std::placeholders::_1));
}

template <typename TItemType>
void Notify(std::vector<TItemType> &inNotifications)
{
    do_all(inNotifications, std::bind(&TItemType::operator(), std::placeholders::_1));
}

template <typename TItemType>
void NotifyReverse(std::vector<TItemType> &inNotifications)
{
    std::for_each(inNotifications.rbegin(), inNotifications.rend(),
                  std::bind(&TItemType::operator(), std::placeholders::_1));
}
}

#endif
