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
#include "Qt3DSDMPrefix.h"
#include "SlideGraphCoreProducer.h"
#include "HandleSystemTransactions.h"
#include "VectorTransactions.h"
#include "SignalsImpl.h"
using namespace std;

namespace qt3dsdm {

Qt3DSDMSlideGraphHandle CSlideGraphCoreProducer::CreateSlideGraph(Qt3DSDMSlideHandle inRoot)
{
    Qt3DSDMSlideGraphHandle retval(m_Data->CreateSlideGraph(inRoot));
    CREATE_HANDLE_CREATE_TRANSACTION(m_Consumer, retval, m_Data->m_Objects);
    GetSignalSender()->SendGraphCreated(retval, inRoot);
    return retval;
}

Qt3DSDMSlideHandle CSlideGraphCoreProducer::GetGraphRoot(Qt3DSDMSlideGraphHandle inGraph) const
{
    return m_Data->GetGraphRoot(inGraph);
}

Qt3DSDMSlideGraphHandle CSlideGraphCoreProducer::GetSlideGraph(Qt3DSDMSlideHandle inSlide) const
{
    return m_Data->GetSlideGraph(inSlide);
}

void CSlideGraphCoreProducer::GetSlideGraphs(TSlideGraphHandleList &outGraphs) const
{
    return m_Data->GetSlideGraphs(outGraphs);
}

struct DissocateAllInstanceTrans : public ITransaction
{
    std::shared_ptr<CSimpleSlideGraphCore> m_Graph;
    Qt3DSDMSlideGraphHandle m_Handle;
    TSlideInstancePairList m_Instances;
    DissocateAllInstanceTrans(const char *inFile, int inLine,
                              std::shared_ptr<CSimpleSlideGraphCore> inGraph,
                              Qt3DSDMSlideGraphHandle inHandle)
        : ITransaction(inFile, inLine)
        , m_Graph(inGraph)
        , m_Handle(inHandle)
    {
        inGraph->GetAssociatedInstances(m_Handle, m_Instances);
    }
    void Do() override
    {
        for (size_t idx = 0, end = m_Instances.size(); idx < end; ++idx)
            m_Graph->DissociateInstance(m_Instances[idx].second);
    }
    void Undo() override
    {
        for (size_t idx = 0, end = m_Instances.size(); idx < end; ++idx)
            m_Graph->AssociateInstance(m_Handle, m_Instances[idx].first, m_Instances[idx].second);
    }
};

void CSlideGraphCoreProducer::DeleteSlideGraph(Qt3DSDMSlideGraphHandle inHandle)
{
    SSlideGraph *theGraph = CSimpleSlideGraphCore::GetSlideGraphNF(inHandle, m_Data->m_Objects);
    Qt3DSDMSlideHandle theRootSlide(theGraph->m_Root);
    if (m_Consumer)
        m_Consumer->OnTransaction(
            make_shared<DissocateAllInstanceTrans>(__FILE__, __LINE__, m_Data, inHandle));
    CREATE_HANDLE_DELETE_TRANSACTION(m_Consumer, inHandle, m_Data->m_Objects);
    m_Data->DeleteSlideGraph(inHandle);
    GetSignalSender()->SendGraphDeleted(inHandle, theRootSlide);
}

struct SInstanceAssociateTrans : public ITransaction
{
    std::shared_ptr<CSimpleSlideGraphCore> m_Graph;
    Qt3DSDMSlideGraphHandle m_GraphHandle;
    Qt3DSDMSlideHandle m_Slide;
    Qt3DSDMInstanceHandle m_Instance;
    bool m_InsertOnDo;
    SInstanceAssociateTrans(const char *inFile, int inLine,
                            std::shared_ptr<CSimpleSlideGraphCore> inGraph,
                            Qt3DSDMSlideGraphHandle inGraphHandle, Qt3DSDMSlideHandle inSlideHandle,
                            Qt3DSDMInstanceHandle inInstance, bool inInsertOnDo)
        : ITransaction(inFile, inLine)
        , m_Graph(inGraph)
        , m_GraphHandle(inGraphHandle)
        , m_Slide(inSlideHandle)
        , m_Instance(inInstance)
        , m_InsertOnDo(inInsertOnDo)
    {
    }
    void Insert() { m_Graph->AssociateInstance(m_GraphHandle, m_Slide, m_Instance); }
    void Remove() { m_Graph->DissociateInstance(m_Instance); }

    void Do() override
    {
        if (m_InsertOnDo)
            Insert();
        else
            Remove();
    }
    void Undo() override
    {
        if (m_InsertOnDo)
            Remove();
        else
            Insert();
    }
};

void CSlideGraphCoreProducer::AssociateInstance(Qt3DSDMSlideGraphHandle inSlideGraph,
                                                Qt3DSDMSlideHandle inSlide,
                                                Qt3DSDMInstanceHandle inInstance)
{
    m_Data->AssociateInstance(inSlideGraph, inSlide, inInstance);
    if (m_Consumer)
        m_Consumer->OnTransaction(make_shared<SInstanceAssociateTrans>(
            __FILE__, __LINE__, m_Data, inSlideGraph, inSlide, inInstance, true));

    GetSignalSender()->SendInstanceAssociated(inSlideGraph, inSlide, inInstance);
}
void CSlideGraphCoreProducer::GetAssociatedInstances(Qt3DSDMSlideGraphHandle inSlideGraph,
                                                     TSlideInstancePairList &outAssociations) const
{
    m_Data->GetAssociatedInstances(inSlideGraph, outAssociations);
}

TGraphSlidePair CSlideGraphCoreProducer::GetAssociatedGraph(Qt3DSDMInstanceHandle inInstance) const
{
    return m_Data->GetAssociatedGraph(inInstance);
}

void CSlideGraphCoreProducer::DissociateInstance(Qt3DSDMInstanceHandle inInstance)
{
    TGraphSlidePair theAssociatedGraph(m_Data->GetAssociatedGraph(inInstance));

    if (theAssociatedGraph.first.Valid()) {
        m_Data->DissociateInstance(inInstance);
        if (m_Consumer)
            m_Consumer->OnTransaction(make_shared<SInstanceAssociateTrans>(
                __FILE__, __LINE__, m_Data, theAssociatedGraph.first, theAssociatedGraph.second,
                inInstance, false));
        GetSignalSender()->SendInstanceDissociated(theAssociatedGraph.first,
                                                   theAssociatedGraph.second, inInstance);
    }
}

void CSlideGraphCoreProducer::SetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph,
                                                  Qt3DSDMSlideHandle inSlide)
{
    if (m_Consumer) {
        Qt3DSDMSlideHandle current = m_Data->GetGraphActiveSlide(inGraph);
        TTransactionPtr theTransaction(CREATE_GENERIC_TRANSACTION(
            bind(&CSimpleSlideGraphCore::SetGraphActiveSlide, m_Data, inGraph, inSlide),
            bind(&CSimpleSlideGraphCore::SetGraphActiveSlide, m_Data, inGraph, current)));
        m_Consumer->OnTransaction(theTransaction);
    }
    m_Data->SetGraphActiveSlide(inGraph, inSlide);
    GetSignalSender()->SendGraphActiveSlide(inGraph, inSlide);
}

Qt3DSDMSlideHandle CSlideGraphCoreProducer::GetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph) const
{
    return m_Data->GetGraphActiveSlide(inGraph);
}

bool CSlideGraphCoreProducer::HandleValid(int inHandle) const
{
    return m_Data->HandleValid(inHandle);
}

void CSlideGraphCoreProducer::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    m_Consumer = inConsumer;
}

TSignalConnectionPtr CSlideGraphCoreProducer::ConnectGraphCreated(
    const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectGraphCreated(inCallback);
}
TSignalConnectionPtr CSlideGraphCoreProducer::ConnectGraphDeleted(
    const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectGraphDeleted(inCallback);
}
TSignalConnectionPtr CSlideGraphCoreProducer::ConnectInstanceAssociated(
    const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
        &inCallback)
{
    return GetSignalProvider()->ConnectInstanceAssociated(inCallback);
}
TSignalConnectionPtr CSlideGraphCoreProducer::ConnectInstanceDissociated(
    const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
        &inCallback)
{
    return GetSignalProvider()->ConnectInstanceDissociated(inCallback);
}
TSignalConnectionPtr CSlideGraphCoreProducer::ConnectGraphActiveSlide(
    const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback)
{
    return GetSignalProvider()->ConnectGraphActiveSlide(inCallback);
}

void CSlideGraphCoreProducer::InitSignaller()
{
    m_Signaller = CreateSlideGraphCoreSignaller();
}

ISlideGraphCoreSignalProvider *CSlideGraphCoreProducer::GetSignalProvider()
{
    return dynamic_cast<ISlideGraphCoreSignalProvider *>(m_Signaller.get());
}

ISlideGraphCoreSignalSender *CSlideGraphCoreProducer::GetSignalSender()
{
    return dynamic_cast<ISlideGraphCoreSignalSender *>(m_Signaller.get());
}
}
