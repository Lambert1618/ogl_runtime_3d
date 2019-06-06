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
#ifndef ACTIONCOREPRODUCERH
#define ACTIONCOREPRODUCERH

#include "SimpleActionCore.h"
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMSignals.h"
#include "Qt3DSDMStringTable.h"

namespace qt3dsdm {
class CActionCoreProducer : public IActionCore,
                            public ITransactionProducer,
                            public IActionCoreSignalProvider
{
    Q_DISABLE_COPY(CActionCoreProducer)

    TSimpleActionCorePtr m_Data;
    TTransactionConsumerPtr m_Consumer;
    TSignalItemPtr m_Signaller;

public:
    CActionCoreProducer(TStringTablePtr inStringTable)
        : m_Data(new CSimpleActionCore(inStringTable))
    {
        InitSignaller();
    }

    TSimpleActionCorePtr GetTransactionlessActionCore() const { return m_Data; }

    // IActionCore implementation
    IStringTable &GetStringTable() const override { return m_Data->GetStringTable(); }
    TStringTablePtr GetStringTablePtr() const override { return m_Data->GetStringTablePtr(); }
    // Action
    Qt3DSDMActionHandle CreateAction(Qt3DSDMInstanceHandle inInstance, Qt3DSDMSlideHandle inSlide,
                                    Qt3DSDMInstanceHandle inOwner, SLong4 inTriggerTargetObjects) override;
    void DeleteAction(Qt3DSDMActionHandle inAction, Qt3DSDMInstanceHandle &outInstance) override;
    const SActionInfo &GetActionInfo(Qt3DSDMActionHandle inAction) const override;
    void GetActions(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                    TActionHandleList &outActions) const override;
    void GetActions(Qt3DSDMSlideHandle inSlide, TActionHandleList &outActions) const override;
    void GetActions(Qt3DSDMInstanceHandle inOwner, TActionHandleList &outActions) const override;
    void GetActions(TActionHandleList &outActions) const override;
    Qt3DSDMInstanceHandle GetActionInstance(Qt3DSDMActionHandle inAction) const override;
    Qt3DSDMActionHandle GetActionByInstance(Qt3DSDMInstanceHandle inActionInstance) const override;

    // Action Properties
    void SetTriggerObject(Qt3DSDMActionHandle inAction, const SObjectRefType &inTriggerObject) override;
    void SetTargetObject(Qt3DSDMActionHandle inAction, const SObjectRefType &inTargetObject) override;
    void SetEvent(Qt3DSDMActionHandle inAction, const wstring &inEventHandle) override;
    void SetHandler(Qt3DSDMActionHandle inAction, const wstring &inHandlerHandle) override;

    // Action Argument
    Qt3DSDMHandlerArgHandle AddHandlerArgument(Qt3DSDMActionHandle inAction, const TCharStr &inName,
                                              HandlerArgumentType::Value inArgType,
                                              DataModelDataType::Value inValueType) override;
    void RemoveHandlerArgument(Qt3DSDMHandlerArgHandle inHandlerArgument) override;
    const SHandlerArgumentInfo &
    GetHandlerArgumentInfo(Qt3DSDMHandlerArgHandle inHandlerArgument) const override;
    void GetHandlerArguments(Qt3DSDMActionHandle inAction,
                             THandlerArgHandleList &outHandlerArguments) const override;

    // Action Argument Properties
    void GetHandlerArgumentValue(Qt3DSDMHandlerArgHandle inHandlerArgument, SValue &outValue) const override;
    void SetHandlerArgumentValue(Qt3DSDMHandlerArgHandle inHandlerArgument, const SValue &inValue) override;

    // CHandleBase
    bool HandleValid(int inHandle) const override;

    // ITransactionProducer implementation
    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    TSignalConnectionPtr ConnectTriggerObjectSet(
        const std::function<void(Qt3DSDMActionHandle, SObjectRefType &)> &inCallback) override;
    TSignalConnectionPtr ConnectTargetObjectSet(
        const std::function<void(Qt3DSDMActionHandle, SObjectRefType &)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectEventSet(const std::function<void(Qt3DSDMActionHandle, const wstring &)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectHandlerSet(const std::function<void(Qt3DSDMActionHandle, const wstring &)> &inCallback) override;

    TSignalConnectionPtr ConnectHandlerArgumentAdded(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) override;
    TSignalConnectionPtr ConnectHandlerArgumentRemoved(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) override;
    TSignalConnectionPtr ConnectHandlerArgumentValueSet(
        const std::function<void(Qt3DSDMHandlerArgHandle, const SValue &)> &inCallback) override;

private:
    void InitSignaller();
    IActionCoreSignalProvider *GetSignalProvider();
    IActionCoreSignalSender *GetSignalSender();
};
}

#endif
