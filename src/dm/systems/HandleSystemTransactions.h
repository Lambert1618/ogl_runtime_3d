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
#ifndef HANDLESYSTEMTRANSACTIONSH
#define HANDLESYSTEMTRANSACTIONSH
#include "HandleSystemBase.h"
#include "Qt3DSDMTransactions.h"

namespace qt3dsdm {
inline void InsertItemInMap(THandleObjectMap &inObjects,
                            const std::pair<int, THandleObjectPtr> &inItem)
{
    inObjects.insert(inItem);
}
inline void HandleCreateTransaction(const char *inFile, int inLine, int inHandle,
                                    THandleObjectMap &inObjects,
                                    TTransactionConsumerPtr &inConsumer)
{
    std::pair<int, THandleObjectPtr> theEntry = *inObjects.find(inHandle);
    std::function<void()> doOp(std::bind(InsertItemInMap, std::ref(inObjects), theEntry));
    std::function<void()> undoOp(
        std::bind(CHandleBase::EraseHandle, inHandle, std::ref(inObjects)));
    TTransactionPtr theTransaction(DoCreateGenericTransaction(inFile, inLine, doOp, undoOp));
    inConsumer->OnTransaction(theTransaction);
}

inline void HandleDeleteTransaction(const char *inFile, int inLine, int inHandle,
                                    THandleObjectMap &inObjects,
                                    TTransactionConsumerPtr &inConsumer)
{
    using namespace std;
    pair<int, THandleObjectPtr> theEntry = *inObjects.find(inHandle);
    TTransactionPtr theTransaction(DoCreateGenericTransaction(
        inFile, inLine, std::bind(CHandleBase::EraseHandle, inHandle, std::ref(inObjects)),
        std::bind(InsertItemInMap, std::ref(inObjects), theEntry)));
    inConsumer->OnTransaction(theTransaction);
}

inline void DoCreateHandleCreateTransaction(const char *inFile, int inLine,
                                            TTransactionConsumerPtr inConsumer, int inHandle,
                                            THandleObjectMap &inObjects)
{
    RunWithConsumer(inConsumer, std::bind(HandleCreateTransaction, inFile, inLine, inHandle,
                                            std::ref(inObjects), std::placeholders::_1));
}

#define CREATE_HANDLE_CREATE_TRANSACTION(inConsumer, inHandle, inObjects)                          \
    DoCreateHandleCreateTransaction(__FILE__, __LINE__, inConsumer, inHandle, inObjects);

inline void DoCreateHandleDeleteTransaction(const char *inFile, int inLine,
                                            TTransactionConsumerPtr inConsumer, int inHandle,
                                            THandleObjectMap &inObjects)
{
    RunWithConsumer(inConsumer, std::bind(HandleDeleteTransaction, inFile, inLine, inHandle,
                                            std::ref(inObjects), std::placeholders::_1));
}

#define CREATE_HANDLE_DELETE_TRANSACTION(inConsumer, inHandle, inObjects)                          \
    DoCreateHandleDeleteTransaction(__FILE__, __LINE__, inConsumer, inHandle, inObjects);
}

#endif
