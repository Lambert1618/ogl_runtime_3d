/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "RuntimePrefix.h"
#include "Qt3DSTypes.h"
#include "Qt3DSAnimationSystem.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSInvasiveSet.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSPool.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSBezierEval.h"
#include "foundation/SerializationTypes.h"
#include "foundation/IOStreams.h"
#include "EASTL/sort.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSIndexableLinkedList.h"

using namespace qt3ds::runtime;
using namespace qt3ds::runtime::element;

namespace {

struct SAnimationKey
{
    QT3DSF32 m_Time; ///< Time
    QT3DSF32 m_Value; ///< Value
    QT3DSF32 m_C1Time; ///< Control 1 time
    QT3DSF32 m_C1Value; ///< Control 1 value
    QT3DSF32 m_C2Time; ///< Control 2 time
    QT3DSF32 m_C2Value; ///< Control 2 value
    SAnimationKey()
        : m_Time(0)
        , m_Value(0)
        , m_C1Time(0)
        , m_C1Value(0)
        , m_C2Time(0)
        , m_C2Value(0)
    {
    }

    SAnimationKey(float time, float val, float c1time, float c1value, float c2time, float c2value)
        : m_Time(time)
        , m_Value(val)
        , m_C1Time(c1time)
        , m_C1Value(c1value)
        , m_C2Time(c2time)
        , m_C2Value(c2value)
    {
    }
};

struct SAnimationKeyNode
{
    enum {
        AnimationKeyArraySize = 4,
    };

    SAnimationKey m_Data[AnimationKeyArraySize];
    SAnimationKeyNode *m_NextNode;
    SAnimationKeyNode()
        : m_NextNode(NULL)
    {
    }
};

typedef IndexableLinkedList<SAnimationKeyNode, SAnimationKey,
                            SAnimationKeyNode::AnimationKeyArraySize>
    TAnimationKeyNodeList;

struct SAnimationTrack
{
    SElement *m_Element;
    QT3DSI32 m_Id;
    QT3DSU32 m_PropertyIndex;
    QT3DSU32 m_KeyCount;
    SAnimationKeyNode *m_Keys;
    QT3DSU32 m_ActiveSetIndex;
    bool m_Dynamic;

    SAnimationTrack()
        : m_Element(NULL)
        , m_Id(0)
        , m_PropertyIndex(0)
        , m_KeyCount(0)
        , m_Keys(NULL)
        , m_ActiveSetIndex(QT3DS_MAX_U32)
        , m_Dynamic(false)
    {
    }
    SAnimationTrack(SElement &inElement, QT3DSI32 inId, QT3DSU32 inPropertyIndex, bool inIsDynamic)
        : m_Element(&inElement)
        , m_Id(inId)
        , m_PropertyIndex(inPropertyIndex)
        , m_KeyCount(0)
        , m_Keys(NULL)
        , m_ActiveSetIndex(QT3DS_MAX_U32)
        , m_Dynamic(inIsDynamic)
    {
    }
};

struct GetAnimationTrackActiveSetIndex
{
    QT3DSU32 operator()(const SAnimationTrack &inTrack) const { return inTrack.m_ActiveSetIndex; }
};

struct SetAnimationTrackActiveSetIndex
{
    void operator()(SAnimationTrack &inTrack, QT3DSU32 inIdx) { inTrack.m_ActiveSetIndex = inIdx; }
};

typedef InvasiveSet<SAnimationTrack, GetAnimationTrackActiveSetIndex,
                    SetAnimationTrackActiveSetIndex>
    TAnimationTrackActiveSet;

struct SAnimationTrackComparator
{
    bool operator()(SAnimationTrack *lhs, SAnimationTrack *rhs) const
    {
        return lhs->m_Id < rhs->m_Id;
    }
};

struct SAnimSystem : public IAnimationSystem
{
    typedef nvhash_map<QT3DSI32, SAnimationTrack *> TAnimationTrackHash;

    NVFoundationBase &m_Foundation;
    Pool<SAnimationTrack, ForwardingAllocator> m_AnimationTrackPool;
    Pool<SAnimationKeyNode, ForwardingAllocator> m_AnimationKeyNodePool;
    TAnimationTrackHash m_Tracks;
    SAnimationTrack *m_LastInsertedTrack;
    TAnimationTrackActiveSet m_ActiveSet;
    NVDataRef<QT3DSU8> m_LoadData;
    QT3DSI32 m_NextTrackId;
    QT3DSI32 m_RefCount;
    // Keep element handle to track mapping so that we can query if an element has animation tracks,
    // and can turn them on/off. Combine element and property id:s to a single 64 bit hash
    // so we can utilise nvhash_map and allocators without needing to implement multimap,
    // or resort to Qt containers. (Single element can have several animation tracks associated.)
    nvhash_map<QT3DSI64, SAnimationTrack *> m_ElemPropsToActiveTracks;

    SAnimSystem(NVFoundationBase &inFoundation)
        : m_Foundation(inFoundation)
        , m_AnimationTrackPool(
              ForwardingAllocator(inFoundation.getAllocator(), "AnimationTrackPool"))
        , m_AnimationKeyNodePool(
              ForwardingAllocator(inFoundation.getAllocator(), "AnimationKeyNodePool"))
        , m_Tracks(inFoundation.getAllocator(), "m_Tracks")
        , m_LastInsertedTrack(NULL)
        , m_ActiveSet(inFoundation.getAllocator(), "m_ActiveSet")
        , m_NextTrackId(1)
        , m_RefCount(0)
        , m_ElemPropsToActiveTracks(inFoundation.getAllocator(), "m_ElemPropsToActiveTracks")
    {
    }

    static QT3DSI32 IncrementTrackId(QT3DSI32 inTrackId)
    {
        ++inTrackId;
        if (!inTrackId)
            ++inTrackId;
        return inTrackId;
    }

    void addRef() override { atomicIncrement(&m_RefCount); }

    void release() override
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc = m_Foundation.getAllocator();
            NVDelete(alloc, this);
        }
    }

    QT3DSI32 CreateAnimationTrack(element::SElement &inElement, QT3DSU32 inPropertyName,
                                       bool inDynamic) override
    {
        // first, make sure we can find the property.
        Option<QT3DSU32> theIndex = inElement.FindPropertyIndex(inPropertyName);
        Option<TPropertyDescAndValuePtr> theProperty;
        if (theIndex.hasValue())
            theProperty = inElement.GetPropertyByIndex(*theIndex);

        if (theProperty.hasValue() == false
            || theProperty->first.type() != Q3DStudio::ATTRIBUTETYPE_FLOAT) {
            QT3DS_ASSERT(false);
            return 0;
        }

        eastl::pair<TAnimationTrackHash::iterator, bool> inserter;
        // Find unused track id.
        for (inserter = m_Tracks.insert(eastl::make_pair(m_NextTrackId, (SAnimationTrack *)NULL));
             inserter.second == false; m_NextTrackId = IncrementTrackId(m_NextTrackId)) {
        }

        SAnimationTrack *theNewTrack =
            reinterpret_cast<SAnimationTrack *>(m_AnimationTrackPool.allocate(__FILE__, __LINE__));
        new (theNewTrack) SAnimationTrack(inElement, inserter.first->first, *theIndex, inDynamic);
        inserter.first->second = theNewTrack;
        m_LastInsertedTrack = theNewTrack;

        m_ElemPropsToActiveTracks.insert(
                    eastl::make_pair(static_cast<QT3DSI64>(inElement.m_Handle) << 32
                                     | static_cast<QT3DSI64>(inPropertyName),
                                     theNewTrack));
        ++m_NextTrackId;
        return inserter.first->first;
    }

    SAnimationKey *GetKeyByIndex(QT3DSU32 inIndex, SAnimationTrack &inTrack)
    {
        if (inIndex >= inTrack.m_KeyCount)
            return 0;

        return &TAnimationKeyNodeList::GetObjAtIdx(inTrack.m_Keys, inIndex);
    }

    SAnimationKey &GetOrCreateKeyByIndex(QT3DSU32 inIndex, SAnimationTrack &inTrack)
    {
        if (inIndex < inTrack.m_KeyCount)
            return *GetKeyByIndex(inIndex, inTrack);

        return TAnimationKeyNodeList::Create(inTrack.m_Keys, inTrack.m_KeyCount,
                                             m_AnimationKeyNodePool);
    }

    void AddKey(QT3DSF32 inTime, QT3DSF32 inValue, QT3DSF32 inC1Time, QT3DSF32 inC1Value,
                        QT3DSF32 inC2Time, QT3DSF32 inC2Value) override
    {
        if (m_LastInsertedTrack == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        SAnimationKey &theNewKey =
            GetOrCreateKeyByIndex(m_LastInsertedTrack->m_KeyCount, *m_LastInsertedTrack);
        theNewKey = SAnimationKey(inTime, inValue, inC1Time, inC1Value, inC2Time, inC2Value);
    }

    SAnimationTrack *GetAnimationTrack(QT3DSI32 inTrackId)
    {
        TAnimationTrackHash::iterator iter = m_Tracks.find(inTrackId);
        if (iter != m_Tracks.end())
            return iter->second;
        return NULL;
    }

    struct SNextKeyRetVal
    {
        SAnimationKeyNode *m_Node;
        SAnimationKey *m_Start;
        SAnimationKey *m_End;

        SNextKeyRetVal(SAnimationKeyNode &inNode)
            : m_Node(&inNode)
            , m_Start(NULL)
            , m_End(NULL)
        {
        }
        SNextKeyRetVal(SAnimationKeyNode &inNode, SAnimationKey &inLastKey)
            : m_Node(&inNode)
            , m_Start(NULL)
            , m_End(&inLastKey)
        {
        }
        SNextKeyRetVal(SAnimationKeyNode &inNode, SAnimationKey &inStart, SAnimationKey &inLastKey)
            : m_Node(&inNode)
            , m_Start(&inStart)
            , m_End(&inLastKey)
        {
        }
        SNextKeyRetVal()
            : m_Node(NULL)
            , m_Start(NULL)
            , m_End(NULL)
        {
        }
    };

    SNextKeyRetVal NextKeyIsGreaterThan(TAnimationKeyNodeList::iterator iter, QT3DSF32 inTime)
    {
        SAnimationKey *theStartKey = &(*iter);
        QT3DSU32 nextIdx = iter.m_Idx + 1;
        if (nextIdx == iter.m_Count)
            return SNextKeyRetVal(*iter.m_Node, *theStartKey);

        TAnimationKeyNodeList::iterator next = iter;
        ++next;

        SAnimationKey &nextKey = *next;

        if (nextKey.m_Time >= inTime)
            return SNextKeyRetVal(*iter.m_Node, *theStartKey, nextKey);
        return SNextKeyRetVal(*iter.m_Node);
    }

    QT3DSF32 Evaluate(SAnimationTrack &inTrack, QT3DSF32 inTime)
    {
        if (inTrack.m_KeyCount < 2) {
            if (inTrack.m_KeyCount == 1)
                return GetKeyByIndex(0, inTrack)->m_Value;
            return 0.0f;
        }

        TAnimationKeyNodeList::iterator iter =
            TAnimationKeyNodeList::begin(inTrack.m_Keys, inTrack.m_KeyCount);

        if ((*iter).m_Time >= inTime) {
            return (*iter).m_Value;
        }

        // We know it isn't the first key.
        SNextKeyRetVal theKeyData;
        for (theKeyData = NextKeyIsGreaterThan(iter, inTime); theKeyData.m_End == NULL;
             ++iter, theKeyData = NextKeyIsGreaterThan(iter, inTime)) {
        }

        if (theKeyData.m_Node == NULL && theKeyData.m_End == NULL) {
            QT3DS_ASSERT(false);
            return 0.0f;
        }

        if (theKeyData.m_Start != NULL) {
            SAnimationKey &start(*theKeyData.m_Start);
            SAnimationKey &end(*theKeyData.m_End);
            return Q3DStudio::EvaluateBezierKeyframe(
                inTime, start.m_Time, start.m_Value, start.m_C1Time, start.m_C1Value,
                start.m_C2Time, start.m_C2Value, end.m_Time, end.m_Value);
        }
        return theKeyData.m_End->m_Value;
    }

    void Update() override
    {
        for (QT3DSU32 idx = 0, end = m_ActiveSet.size(); idx < end; ++idx) {
            SAnimationTrack &theTrack = *m_ActiveSet[idx];
            SElement &theElement = *theTrack.m_Element;
            QT3DSF32 theTime = static_cast<QT3DSF32>(theElement.GetOuterTime());
            TPropertyDescAndValuePtr theProperty =
                *theElement.GetPropertyByIndex(theTrack.m_PropertyIndex);
            Q3DStudio::UVariant &theValue(*theProperty.second);
            QT3DSF32 newValue = Evaluate(theTrack, theTime);
            if (fabs(theValue.m_FLOAT - newValue) > SElement::SmallestDifference()) {
                theValue.m_FLOAT = newValue;
                theElement.SetDirty();
            }
        }
    }

    void SetActive(QT3DSI32 inTrackId, bool inActive) override
    {
        SAnimationTrack *theTrack = GetAnimationTrack(inTrackId);
        if (theTrack) {
            if (inActive)
                m_ActiveSet.insert(*theTrack);
            else
                m_ActiveSet.remove(*theTrack);
        }
    }

    QT3DSI32 getActiveTrackForElemProp(
            element::SElement *inElement, QT3DSU32 inPropertyHash) override
    {
        nvhash_map<QT3DSI64, SAnimationTrack *>::iterator iter
                = m_ElemPropsToActiveTracks.find(static_cast<QT3DSI64>(inElement->m_Handle) << 32
                                                 | static_cast<QT3DSI64>(inPropertyHash));
        if (iter == m_ElemPropsToActiveTracks.end())
            return 0;

        // Only return track if it is actually active
        if (!m_ActiveSet.contains(*iter->second))
            return 0;

        return iter->second->m_Id;
    }

    //==============================================================================
    /**
     *	Recomputes control point values for Studio animation based off of new start
     *	value.
     *	@param inNewStartValue	new start value for the keyframe
     *	@param inKeyStart		start index of the keyframes
     */
    void RecomputeControlPoints(QT3DSF32 inNewStartValue, SAnimationTrack &inTrack)
    {
        SAnimationKey &theFirstKey = *GetKeyByIndex(0, inTrack);
        SAnimationKey &theSecondKey = *GetKeyByIndex(1, inTrack);

        // Determine original control point ratios
        Q3DStudio::FLOAT theOriginalInterval =
            (theSecondKey.m_Value - theFirstKey.m_Value) * (1.0f / 3.0f);
        Q3DStudio::FLOAT theEaseOutRatio = theOriginalInterval != 0.0f
            ? (theFirstKey.m_C1Value - theFirstKey.m_Value) / theOriginalInterval
            : 0.0f;
        Q3DStudio::FLOAT theEaseInRatio = theOriginalInterval != 0.0f
            ? (theSecondKey.m_Value - theFirstKey.m_C2Value) / theOriginalInterval
            : 0.0f;

        // Apply ratios based off new start value
        Q3DStudio::FLOAT theNewInterval = (theSecondKey.m_Value - inNewStartValue) * (1.0f / 3.0f);
        theFirstKey.m_C1Value = inNewStartValue + (theNewInterval * theEaseOutRatio);
        theFirstKey.m_C2Value = theSecondKey.m_Value - (theNewInterval * theEaseInRatio);
    }

    void UpdateDynamicKey(QT3DSI32 inTrackId) override
    {
        SAnimationTrack *theTrack = GetAnimationTrack(inTrackId);
        if (theTrack && theTrack->m_Dynamic) {
            SAnimationKey *theFirstKey = GetKeyByIndex(0, *theTrack);
            if (theFirstKey) {
                Q3DStudio::UVariant *thePropData =
                    theTrack->m_Element->GetPropertyByIndex(theTrack->m_PropertyIndex)->second;

                // Recompute interpolation
                if (theTrack->m_KeyCount > 1)
                    RecomputeControlPoints(thePropData->m_FLOAT, *theTrack);

                // Update start value
                theFirstKey->m_Value = thePropData->m_FLOAT;
            }
        }
    }
};
}

IAnimationSystem &IAnimationSystem::CreateAnimationSystem(NVFoundationBase &inFoundation)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SAnimSystem)(inFoundation);
}
