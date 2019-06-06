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
#ifndef SLIDEGRAPHCOREPRODUCERH
#define SLIDEGRAPHCOREPRODUCERH
#include "SimpleSlideGraphCore.h"
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMSignals.h"

namespace qt3dsdm {
class CSlideGraphCoreProducer : public ISlideGraphCore,
                                public ITransactionProducer,
                                public ISlideGraphCoreSignalProvider
{
    Q_DISABLE_COPY(CSlideGraphCoreProducer)

    TSimpleSlideGraphCorePtr m_Data;
    TTransactionConsumerPtr m_Consumer;
    TSignalItemPtr m_Signaller;

public:
    CSlideGraphCoreProducer(TStringTablePtr strTable = TStringTablePtr())
        : m_Data(new CSimpleSlideGraphCore(strTable))
    {
        InitSignaller();
    }
    TSimpleSlideGraphCorePtr GetTransactionlessSlideGraphCore() { return m_Data; }
    TSimpleSlideGraphCorePtr GetTransactionlessSlideGraphCore() const { return m_Data; }

    Qt3DSDMSlideGraphHandle CreateSlideGraph(Qt3DSDMSlideHandle inRoot) override;
    Qt3DSDMSlideHandle GetGraphRoot(Qt3DSDMSlideGraphHandle inGraph) const override;
    Qt3DSDMSlideGraphHandle GetSlideGraph(Qt3DSDMSlideHandle inSlide) const override;
    void GetSlideGraphs(TSlideGraphHandleList &outGraphs) const override;
    void DeleteSlideGraph(Qt3DSDMSlideGraphHandle inHandle) override;

    void AssociateInstance(Qt3DSDMSlideGraphHandle inSlideGraph, Qt3DSDMSlideHandle inSlide,
                           Qt3DSDMInstanceHandle inInstance) override;
    void GetAssociatedInstances(Qt3DSDMSlideGraphHandle inSlideGraph,
                                TSlideInstancePairList &outAssociations) const override;
    TGraphSlidePair GetAssociatedGraph(Qt3DSDMInstanceHandle inInstance) const override;
    void DissociateInstance(Qt3DSDMInstanceHandle inInstance) override;

    void SetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) override;
    Qt3DSDMSlideHandle GetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph) const override;

    bool HandleValid(int inHandle) const override;
    void SetConsumer(TTransactionConsumerPtr inConsumer) override;

    //===================================================================
    // Signals
    //===================================================================
    TSignalConnectionPtr ConnectGraphCreated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectGraphDeleted(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override;
    TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override;
    TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override;
    TSignalConnectionPtr ConnectGraphActiveSlide(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override;

private:
    void InitSignaller();
    ISlideGraphCoreSignalProvider *GetSignalProvider();
    ISlideGraphCoreSignalSender *GetSignalSender();
};
}

#endif
