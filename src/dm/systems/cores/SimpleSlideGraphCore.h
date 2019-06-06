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
#ifndef SLIDEGRAPHCOREH
#define SLIDEGRAPHCOREH
#include "Qt3DSDMStringTable.h"
#include "Qt3DSDMSlideGraphCore.h"
#include "HandleSystemBase.h"
#include "Qt3DSDMErrors.h"

namespace qt3dsdm {
using namespace std;
struct SSlideGraph : public CHandleObject
{
    SSlideGraph()
        : m_Root(0)
        , m_ActiveSlide(0)
    {
    }
    SSlideGraph(int inHandle, int inSlideRoot)
        : CHandleObject(inHandle)
        , m_Root(inSlideRoot)
        , m_ActiveSlide(0)
    {
    }
    Qt3DSDMSlideHandle m_Root;
    Qt3DSDMSlideHandle m_ActiveSlide;

    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeSSlideGraph;
    EHandleObjectType GetType() override { return s_Type; }
};

class CSimpleSlideGraphCore : public CHandleBase, public ISlideGraphCore
{
    TStringTablePtr m_StringTable;
    typedef std::unordered_map<Qt3DSDMInstanceHandle,
                                 pair<Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle>, std::hash<int>>
        TInstanceToGraphMap;
    typedef std::unordered_map<Qt3DSDMSlideGraphHandle, TSlideInstancePairList, std::hash<int>>
        TGraphToInstanceMap;

    TInstanceToGraphMap m_InstanceToGraph;
    TGraphToInstanceMap m_GraphToInstances;

public:
    CSimpleSlideGraphCore(TStringTablePtr strTable = TStringTablePtr())
        : m_StringTable(strTable)
    {
    }
    TStringTablePtr GetStringTablePtr() { return m_StringTable; }
    /**
     *	A slide graph is used to associate a set of instances to a set of slides.
     *	This allows rapid lookup of properties, as an implementation of these.
     *	There are a few assumptions here.  First is that a given slide can be a member
     *	of one and only one graph (i.e. it does not derive from another slide outside of the graph).
     *	Second is that an instance is a member of one and only one graph.
     */
    Qt3DSDMSlideGraphHandle CreateSlideGraph(Qt3DSDMSlideHandle inRoot) override;
    Qt3DSDMSlideHandle GetGraphRoot(Qt3DSDMSlideGraphHandle inGraph) const override;
    Qt3DSDMSlideGraphHandle GetSlideGraph(Qt3DSDMSlideHandle inSlide) const override;
    void GetSlideGraphs(TSlideGraphHandleList &outGraphs) const override;
    void DeleteSlideGraph(Qt3DSDMSlideGraphHandle inHandle) override;

    /**
     *	Associate a given instance handle with a given graph.  This will ensure that property
     *lookups
     *	will travel through this graph before they hit the main data core.  Instances may be
     *associated
     *	with a sub-slide of a given graph, not just the root.  An instance associated with the root
     *is
     *	implicitly associated with any root-derived slides.
     */
    void AssociateInstance(Qt3DSDMSlideGraphHandle inSlideGraph, Qt3DSDMSlideHandle inSlide,
                           Qt3DSDMInstanceHandle inInstance) override;
    void GetAssociatedInstances(Qt3DSDMSlideGraphHandle inSlideGraph,
                                TSlideInstancePairList &outAssociations) const override;
    TGraphSlidePair GetAssociatedGraph(Qt3DSDMInstanceHandle inInstance) const override;
    void DissociateInstance(Qt3DSDMInstanceHandle inInstance) override;

    /**
     *	All graphs always have an active slide.  This is assumed to be the root right off the bat.
     */
    void SetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) override;
    Qt3DSDMSlideHandle GetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph) const override;

    bool HandleValid(int inHandle) const override;

    Qt3DSDMSlideGraphHandle CreateSlideGraphWithHandle(int inHandle, Qt3DSDMSlideHandle inRoot);

    static SSlideGraph *GetSlideGraphNF(int inHandle, THandleObjectMap &inObjects)
    {
        return const_cast<SSlideGraph *>(
            GetSlideGraphNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const SSlideGraph *GetSlideGraphNF(int inHandle, const THandleObjectMap &inObjects)
    {
        const SSlideGraph *theSlide = GetHandleObject<SSlideGraph>(inHandle, inObjects);
        if (theSlide)
            return theSlide;
        throw SlideGraphNotFound(L"");
    }
};

typedef std::shared_ptr<CSimpleSlideGraphCore> TSimpleSlideGraphCorePtr;
}

#endif
