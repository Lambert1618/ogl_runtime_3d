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
#include "SimpleSlideGraphCore.h"

using namespace std;

namespace qt3dsdm {

Qt3DSDMSlideGraphHandle CSimpleSlideGraphCore::CreateSlideGraph(Qt3DSDMSlideHandle inRoot)
{
    int nextId = GetNextId();
    return CreateSlideGraphWithHandle(nextId, inRoot);
}

Qt3DSDMSlideHandle CSimpleSlideGraphCore::GetGraphRoot(Qt3DSDMSlideGraphHandle inGraph) const
{
    return GetSlideGraphNF(inGraph, m_Objects)->m_Root;
}

inline bool RootSlideMatches(const THandleObjectPair &inPair, Qt3DSDMSlideHandle inSlide)
{
    const SSlideGraph *theGraph = static_cast<SSlideGraph *>(inPair.second.get());
    if (theGraph->m_Root == inSlide)
        return true;
    return false;
}

Qt3DSDMSlideGraphHandle CSimpleSlideGraphCore::GetSlideGraph(Qt3DSDMSlideHandle inSlide) const
{
    THandleObjectMap::const_iterator theFind = find_if<THandleObjectMap::const_iterator>(
        m_Objects, std::bind(RootSlideMatches, std::placeholders::_1, inSlide));
    if (theFind != m_Objects.end())
        return theFind->first;
    return 0;
}

inline Qt3DSDMSlideGraphHandle ToGraphHandle(const THandleObjectPair &inPair)
{
    return inPair.first;
}

void CSimpleSlideGraphCore::GetSlideGraphs(TSlideGraphHandleList &outGraphs) const
{
    outGraphs.resize(m_Objects.size());
    transform(m_Objects.begin(), m_Objects.end(), outGraphs.begin(), ToGraphHandle);
}

void CSimpleSlideGraphCore::DeleteSlideGraph(Qt3DSDMSlideGraphHandle inHandle)
{
    TSlideInstancePairList theAssociatedInstances;
    GetAssociatedInstances(inHandle, theAssociatedInstances);
    for (size_t idx = 0, end = theAssociatedInstances.size(); idx < end; ++idx)
        DissociateInstance(theAssociatedInstances[idx].second);
    EraseHandle(inHandle, m_Objects);
}

void CSimpleSlideGraphCore::AssociateInstance(Qt3DSDMSlideGraphHandle inSlideGraph,
                                              Qt3DSDMSlideHandle inSlide,
                                              Qt3DSDMInstanceHandle inInstance)
{
    pair<TInstanceToGraphMap::iterator, bool> theResult =
        m_InstanceToGraph.insert(make_pair(inInstance, make_pair(inSlideGraph, inSlide)));
    Q_ASSERT(theResult.second);
    if (theResult.second == false) {
        theResult.first->second.first = inSlideGraph;
        theResult.first->second.second = inSlide;
    }
    pair<TGraphToInstanceMap::iterator, bool> theGraphResult =
        m_GraphToInstances.insert(make_pair(inSlideGraph, TSlideInstancePairList()));
    insert_unique(theGraphResult.first->second, make_pair(inSlide, inInstance));
}

void CSimpleSlideGraphCore::GetAssociatedInstances(Qt3DSDMSlideGraphHandle inSlideGraph,
                                                   TSlideInstancePairList &outAssociations) const
{
    TGraphToInstanceMap::const_iterator theFind = m_GraphToInstances.find(inSlideGraph);
    if (theFind != m_GraphToInstances.end())
        outAssociations.insert(outAssociations.end(), theFind->second.begin(),
                               theFind->second.end());
}

TGraphSlidePair CSimpleSlideGraphCore::GetAssociatedGraph(Qt3DSDMInstanceHandle inInstance) const
{
    TInstanceToGraphMap::const_iterator theResult = m_InstanceToGraph.find(inInstance);
    if (theResult != m_InstanceToGraph.end())
        return theResult->second;
    return TGraphSlidePair();
}

struct SInstanceMatcher
{
    Qt3DSDMInstanceHandle m_Instance;
    SInstanceMatcher(Qt3DSDMInstanceHandle inInst)
        : m_Instance(inInst)
    {
    }
    bool operator()(const pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle> &inItem) const
    {
        return m_Instance == inItem.second;
    }
};

void CSimpleSlideGraphCore::DissociateInstance(Qt3DSDMInstanceHandle inInstance)
{
    TGraphSlidePair theAssociatedGraph(GetAssociatedGraph(inInstance));

    TGraphToInstanceMap::iterator theFind = m_GraphToInstances.find(theAssociatedGraph.first);
    if (theFind != m_GraphToInstances.end()) {
        erase_if(theFind->second, SInstanceMatcher(inInstance));
        if (theFind->second.size() == 0)
            m_GraphToInstances.erase(theAssociatedGraph.first);
    }

    m_InstanceToGraph.erase(inInstance);
}

void CSimpleSlideGraphCore::SetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph,
                                                Qt3DSDMSlideHandle inSlide)
{
    GetSlideGraphNF(inGraph, m_Objects)->m_ActiveSlide = inSlide;
}

Qt3DSDMSlideHandle CSimpleSlideGraphCore::GetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph) const
{
    const SSlideGraph *theSlide = GetSlideGraphNF(inGraph, m_Objects);
    if (theSlide->m_ActiveSlide)
        return theSlide->m_ActiveSlide;
    return theSlide->m_Root;
}

bool CSimpleSlideGraphCore::HandleValid(int inHandle) const
{
    return CHandleBase::HandleValid(inHandle);
}

Qt3DSDMSlideGraphHandle CSimpleSlideGraphCore::CreateSlideGraphWithHandle(int inHandle,
                                                                         Qt3DSDMSlideHandle inRoot)
{
    m_Objects.insert(make_pair(inHandle, (THandleObjectPtr) new SSlideGraph(inHandle, inRoot)));
    return inHandle;
}
}
