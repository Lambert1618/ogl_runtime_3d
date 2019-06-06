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
#ifndef QT3DSDM_SLIDE_GRAPH_CORE_H
#define QT3DSDM_SLIDE_GRAPH_CORE_H

#include "Qt3DSDMHandles.h"
#include "HandleSystemBase.h"

namespace qt3dsdm {

typedef std::pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle> TSlideInstancePair;
typedef std::vector<TSlideInstancePair> TSlideInstancePairList;
typedef std::pair<Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle> TGraphSlidePair;

/**
 *	Binding instances to slide graphs.
 */
class ISlideGraphCore : public IHandleBase
{
public:
    virtual ~ISlideGraphCore() {}

    /**
     *	A slide graph is used to associate a set of instances to a set of slides.
     *	This allows rapid lookup of properties, as an implementation of these.
     *	There are a few assumptions here.  First is that a given slide can be a member
     *	of one and only one graph (i.e. it does not derive from another slide outside of the graph).
     *	Second is that an instance is a member of one and only one graph.
     */
    virtual Qt3DSDMSlideGraphHandle CreateSlideGraph(Qt3DSDMSlideHandle inRoot) = 0;
    virtual Qt3DSDMSlideHandle GetGraphRoot(Qt3DSDMSlideGraphHandle inGraph) const = 0;
    virtual Qt3DSDMSlideGraphHandle GetSlideGraph(Qt3DSDMSlideHandle inSlide) const = 0;
    virtual void GetSlideGraphs(TSlideGraphHandleList &outGraphs) const = 0;
    virtual void DeleteSlideGraph(Qt3DSDMSlideGraphHandle inHandle) = 0;

    /**
     *	Associate a given instance handle with a given graph.  This will ensure that property
     *lookups
     *	will travel through this graph before they hit the main data core.  Instances may be
     *associated
     *	with a sub-slide of a given graph, not just the root.  An instance associated with the root
     *is
     *	implicitly associated with any root-derived slides.
     */
    virtual void AssociateInstance(Qt3DSDMSlideGraphHandle inSlideGraph, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void GetAssociatedInstances(Qt3DSDMSlideGraphHandle inSlideGraph,
                                        TSlideInstancePairList &outAssociations) const = 0;
    virtual TGraphSlidePair GetAssociatedGraph(Qt3DSDMInstanceHandle inInstance) const = 0;
    virtual void DissociateInstance(Qt3DSDMInstanceHandle inInstance) = 0;

    /**
     *	All graphs always have an active slide.  This is assumed to be the root right off the bat.
     */
    virtual void SetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) = 0;
    virtual Qt3DSDMSlideHandle GetGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph) const = 0;
};

typedef std::shared_ptr<ISlideGraphCore> TSlideGraphCorePtr;
}

#endif