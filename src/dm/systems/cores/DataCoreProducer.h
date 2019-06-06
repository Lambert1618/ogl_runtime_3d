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
#ifndef DATACOREPRODUCERH
#define DATACOREPRODUCERH

#include "Qt3DSDMTransactions.h"
#include "SimpleDataCore.h"
#include "Qt3DSDMSignals.h"
#include "SimpleSlideCore.h"

namespace qt3dsdm {

class CDataCoreProducer : public IDataCore,
                          public ITransactionProducer,
                          public IInstancePropertyCoreSignalProvider,
                          public IDataCoreSignalProvider
{
    typedef std::shared_ptr<IMergeableTransaction<TPropertyValuePair>> TPropertyMergeMapEntry;
    typedef std::unordered_map<TSlideInstancePropertyPair, TPropertyMergeMapEntry>
        TPropertyMergeMap;

    TTransactionConsumerPtr m_Consumer;
    TSimpleDataCorePtr m_Data;
    TSignalItemPtr m_InstancePropertyCoreSignaller;
    TSignalItemPtr m_DataCoreSignaller;

    TPropertyMergeMap m_PropertyMergeMap;

public:
    CDataCoreProducer(TStringTablePtr inStringTable)
        : m_Data(new CSimpleDataCore(inStringTable))
    {
        InitSignallers();
    }
    virtual ~CDataCoreProducer() {}

    IStringTable &GetStringTable() const override { return m_Data->GetStringTable(); }
    std::shared_ptr<IStringTable> GetStringTablePtr() const override
    {
        return m_Data->GetStringTablePtr();
    }

    // IHandleBase
    bool HandleValid(int inHandle) const override;

    // IInstancePropertyCore
    //===============================================================
    Qt3DSDMPropertyHandle GetAggregateInstancePropertyByName(Qt3DSDMInstanceHandle inInstance,
                                                            const TCharStr &inStr) const override;
    void GetAggregateInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                                TPropertyHandleList &outProperties) const override;
    void GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inHandle,
                                                   TPropertyHandleValuePairList &outValues) override;
    bool HasAggregateInstanceProperty(Qt3DSDMInstanceHandle inInstance,
                                              Qt3DSDMPropertyHandle inProperty) const override;

    void CheckValue(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                    const SValue &inValue) const override;
    bool GetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty, SValue &outValue) const override;
    void SetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override;
    //===============================================================

    // IDataCore
    //===============================================================
    Qt3DSDMInstanceHandle CreateInstance(Qt3DSDMInstanceHandle hdl = Qt3DSDMInstanceHandle()) override;
    void GetInstances(TInstanceHandleList &outInstances) const override;
    void GetInstancesDerivedFrom(TInstanceHandleList &outInstances,
                                         Qt3DSDMInstanceHandle inParent) const override;
    void DeleteInstance(Qt3DSDMInstanceHandle inHandle) override;

    void DeriveInstance(Qt3DSDMInstanceHandle inInstance, Qt3DSDMInstanceHandle inParent) override;
    void GetInstanceParents(Qt3DSDMInstanceHandle inHandle,
                                    TInstanceHandleList &outParents) const override;
    bool IsInstanceOrDerivedFrom(Qt3DSDMInstanceHandle inInstance,
                                         Qt3DSDMInstanceHandle inParent) const override;

    Qt3DSDMPropertyHandle AddProperty(Qt3DSDMInstanceHandle inInstance, TCharPtr inName,
                                             DataModelDataType::Value inPropType) override;
    void GetInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                       TPropertyHandleList &outProperties) const override;
    const Qt3DSDMPropertyDefinition &GetProperty(Qt3DSDMPropertyHandle inProperty) const override;
    void RemoveProperty(Qt3DSDMPropertyHandle inProperty) override;
    void CopyInstanceProperties(Qt3DSDMInstanceHandle inSrcInstance,
                                        Qt3DSDMInstanceHandle inDestInstance) override;

    void RemoveCachedValues(Qt3DSDMInstanceHandle inInstance) override
    {
        m_Data->RemoveCachedValues(inInstance);
    }
    bool IsInstance(int inHandle) const override { return m_Data->IsInstance(inHandle); }
    bool IsProperty(int inHandle) const override { return m_Data->IsProperty(inHandle); }
    //===============================================================

    //===============================================================
    //	Set the current consumer
    //===============================================================
    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    //===============================================================
    //	Return a serializable data model for load/save
    //===============================================================
    virtual TSimpleDataCorePtr GetTransactionlessDataCore() { return m_Data; }
    virtual TSimpleDataCorePtr GetTransactionlessDataCore() const { return m_Data; }

    //===============================================================
    //	Signal provider implementation
    //===============================================================
    TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue &)>
            &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectBeforeInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectInstanceDerived(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectInstanceParentRemoved(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectPropertyAdded(const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                    TCharPtr, DataModelDataType::Value)> &inCallback) override;
    virtual TSignalConnectionPtr
    ConnectPropertyRemoved(const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                      TCharPtr, DataModelDataType::Value)> &inCallback) override;

private:
    CDataCoreProducer(const CDataCoreProducer&) = delete;
    CDataCoreProducer& operator=(const CDataCoreProducer&) = delete;

    template <typename TTransactionType>
    inline void RunWithConsumer(TTransactionType inTransaction)
    {
        qt3dsdm::RunWithConsumer(m_Consumer, inTransaction);
    }

    void InitSignallers();
    IInstancePropertyCoreSignalProvider *GetPropertyCoreProvider();
    IInstancePropertyCoreSignalSender *GetPropertyCoreSender();
    IDataCoreSignalProvider *GetDataCoreProvider();
    IDataCoreSignalSender *GetDataCoreSender();
};
}

#endif
