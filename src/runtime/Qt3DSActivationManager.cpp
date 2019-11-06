/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#include "Qt3DSActivationManager.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSComponentManager.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSInvasiveSet.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/StringTable.h"
#include "EASTL/sort.h"
#include "Qt3DSAttributeHashes.h"
#include "Qt3DSPresentation.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSSync.h"
#include "Qt3DSRenderThreadPool.h"
#include "foundation/Qt3DSSimpleTypes.h"

using namespace qt3ds::runtime;
using namespace qt3ds::runtime::element;
using qt3ds::render::IThreadPool;
using qt3ds::foundation::Time;

namespace {

typedef nvvector<TElementAndSortKey> TElementAndSortKeyList;

struct STimeEvent
{
    enum Type {
        Start = 0,
        Stop,
    };

    Type m_Type;
    TTimeUnit m_Time;
    nvvector<SElement *> m_AffectedNodes;

    STimeEvent(Type inType, NVAllocatorCallback &alloc, TTimeUnit inTime)
        : m_Type(inType)
        , m_Time(inTime)
        , m_AffectedNodes(alloc, "TimeEvent")
    {
    }

    void Reset()
    {
        m_Time = 0;
        m_AffectedNodes.clear();
    }
};

struct STimeEventGreaterThan
{
    bool operator()(const STimeEvent *lhs, const STimeEvent *rhs) const
    {
        if (lhs->m_Time == rhs->m_Time)
            return lhs->m_Type > rhs->m_Type;

        return lhs->m_Time > rhs->m_Time;
    }
};

struct STimeEventLessThan
{
    bool operator()(const STimeEvent *lhs, const STimeEvent *rhs) const
    {
        if (lhs->m_Time == rhs->m_Time)
            return lhs->m_Type < rhs->m_Type;

        return lhs->m_Time < rhs->m_Time;
    }
};

typedef nvvector<STimeEvent *> TTimeEventList;
struct STimeContext;
typedef NVDataRef<NVScopedRefCounted<STimeContext>> TTimeContextSet;

// Tree navigation

SElement *GetElementNodeFirstChild(SElement &inNode)
{
    return inNode.m_Child;
}
SElement *GetElementNodeNextSibling(SElement &inNode)
{
    return inNode.m_Sibling;
}
SElement *GetElementNodeParent(SElement &inNode)
{
    return inNode.m_Parent;
}

struct SScanBufferEntry
{
    SElement *m_Node;
    QT3DSU32 m_StartTime;
    QT3DSU32 m_EndTime;

    SScanBufferEntry()
        : m_Node(NULL)
        , m_StartTime(0)
        , m_EndTime(QT3DS_MAX_U32)
    {
    }
    SScanBufferEntry(SElement *inNode)
        : m_Node(inNode)
        , m_StartTime(0)
        , m_EndTime(QT3DS_MAX_U32)
    {
    }
    SScanBufferEntry(SElement *inNode, QT3DSU32 start, QT3DSU32 end)
        : m_Node(inNode)
        , m_StartTime(start)
        , m_EndTime(end)
    {
    }

    SScanBufferEntry(SElement *inNode, bool inParentActive)
        : m_Node(inNode)
        , m_StartTime(inParentActive ? 1 : 0)
        , m_EndTime(0)
    {
    }

    bool IsParentActive() const { return m_StartTime ? true : false; }
};

typedef nvvector<SScanBufferEntry> TScanBuffer;
typedef nvvector<SElement *> TElementNodePtrList;

struct SElementIndexPairSorter
{
    bool operator()(const eastl::pair<element::SElement *, QT3DSU32> &lhs,
                    const eastl::pair<element::SElement *, QT3DSU32> &rhs) const
    {
        return lhs.second < rhs.second;
    }
};

struct SElementPtrSort
{
    bool operator()(const SElement *lhs, const SElement *rhs)
    {
        return lhs->Depth() < rhs->Depth();
    }
};

DEFINE_INVASIVE_SINGLE_LIST(TimeContext);

struct STimeContext
{
    NVAllocatorCallback &m_Allocator;
    SComponent &m_Component;
    CTimePolicy m_TimePolicy;
    SComponentTimePolicyOverride *m_TimePolicyOverride;
    TTimeUnit m_CurrentTime;
    TElementNodePtrList m_Elements;
    TTimeEventList m_TimeEventBackingStore;
    TTimeEventList m_TimeEvents;
    SActivationManagerNodeDirtyList m_DirtyList;
    SActivationManagerNodeDirtyList m_ControlledList;
    QT3DSU32 m_TimeEventBackingStoreIndex;
    QT3DSI32 mRefCount;
    STimeContext *m_NextSibling;
    TTimeContextList m_Children;
    Mutex &m_ElementAccessMutex;
    bool m_AllNodesDirty;
    bool m_TimeDataNeedsUpdate;

    STimeContext(NVAllocatorCallback &alloc, SComponent &inComponent, Mutex &inElementAccessMutex)
        : m_Allocator(alloc)
        , m_Component(inComponent)
        , m_TimePolicyOverride(NULL)
        , m_CurrentTime(-1)
        , m_Elements(alloc, "m_Elements")
        , m_TimeEventBackingStore(alloc, "TimeEventBackingStore")
        , m_TimeEvents(alloc, "TimeEvents")
        , m_DirtyList(alloc)
        , m_ControlledList(alloc)
        , m_TimeEventBackingStoreIndex(0)
        , mRefCount(0)
        , m_NextSibling(NULL)
        , m_ElementAccessMutex(inElementAccessMutex)
        , m_AllNodesDirty(true)
        , m_TimeDataNeedsUpdate(true)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator)
    ~STimeContext()
    {
        Reset();
        for (TTimeEventList::iterator iter = m_TimeEventBackingStore.begin(),
                                      end = m_TimeEventBackingStore.end();
             iter != end; ++iter) {
            NVDelete(m_Allocator, *iter);
        }

        m_TimeEventBackingStore.clear();
        RemoveOverride();
    }

    void Reset()
    {
        for (TTimeEventList::iterator iter = m_TimeEvents.begin(), end = m_TimeEvents.end();
             iter != end; ++iter)
            (*iter)->Reset();

        m_TimeEvents.clear();
        m_TimeEventBackingStoreIndex = 0;
        m_CurrentTime = -1;
        m_DirtyList.clear();
        m_AllNodesDirty = true;
        m_TimeDataNeedsUpdate = true;
    }

    STimeEvent &GetTimeEventForTime(TTimeUnit inTime, STimeEvent::Type inType)
    {
        TTimeEventList::iterator inserter = m_TimeEvents.end();
        for (TTimeEventList::iterator iter = m_TimeEvents.begin(), end = m_TimeEvents.end();
             iter != end && inserter == end; ++iter) {
            if ((*iter)->m_Time == inTime) {
                if ((*iter)->m_Type == inType)
                    return **iter;
                // Start comes before stop
                if ((*iter)->m_Type == STimeEvent::Stop)
                    inserter = iter;
            }

            if ((*iter)->m_Time > inTime)
                inserter = iter;
        }

        if (m_TimeEventBackingStoreIndex == m_TimeEventBackingStore.size())
            m_TimeEventBackingStore.push_back(
                QT3DS_NEW(m_Allocator, STimeEvent)(inType, m_Allocator, inTime));

        STimeEvent *newEvent = m_TimeEventBackingStore[m_TimeEventBackingStoreIndex];
        ++m_TimeEventBackingStoreIndex;

        newEvent->m_Time = inTime;
        newEvent->m_Type = inType;
        newEvent->m_AffectedNodes.clear();

        if (inserter == m_TimeEvents.end())
            m_TimeEvents.push_back(newEvent);
        else
            m_TimeEvents.insert(inserter, newEvent);

        return *newEvent;
    };

    void UpdateNodeList(nvvector<SElement *> &inList, bool timeActive)
    {
        for (QT3DSU32 elemIdx = 0, endIdx = inList.size(); elemIdx < endIdx; ++elemIdx) {
            SActivationManagerNode &theNode = inList[elemIdx]->m_ActivationManagerNode;
            if (timeActive != theNode.m_Flags.IsTimeActive()) {
                theNode.m_Flags.SetTimeActive(timeActive);
                SetElementDirty(*inList[elemIdx]);
            }
        }
    }

    // Returns true if we have reached the end time.
    bool CalculateNewTime(TTimeUnit inGlobalTime)
    {
        Option<TTimeUnit> theNewTime;
        if (m_TimePolicyOverride) {
            SComponentTimePolicyOverride *theOverride = m_TimePolicyOverride;
            eastl::pair<Q3DStudio::BOOL, TTimeUnit> theEvalResult =
                theOverride->ComputeLocalTime(m_TimePolicy, inGlobalTime);
            if (theEvalResult.first && theOverride->m_TimeCallback) {
                Mutex::ScopedLock __locker(m_ElementAccessMutex);
                theOverride->m_TimeCallback->OnTimeFinished();
                theOverride->m_TimeCallback->Release();
                theOverride->m_TimeCallback = NULL;
            }
            return false;
        } else {
            Q3DStudio::BOOL theReachedEndTime = m_TimePolicy.ComputeTime(inGlobalTime);
            if (theReachedEndTime)
                return true;

            return false;
        }
    }

    // There are four conditions this happens:
    // 1.  This object is getting destroyed
    // 2.  A new override is being set.
    // 3.  The compoent switched slides.
    // 4.  The component was deactivated.
    void RemoveOverride()
    {
        if (m_TimePolicyOverride) {
            if (m_TimePolicyOverride->m_TimeCallback) {
                Mutex::ScopedLock __locker(m_ElementAccessMutex);
                m_TimePolicyOverride->m_TimeCallback->Release();
                m_TimePolicyOverride->m_TimeCallback = NULL;
            }

            NVDelete(m_Allocator, m_TimePolicyOverride);
            m_TimePolicyOverride = NULL;
        }
    }

    void UpdateLocalTime(TTimeUnit inNewTime, TScanBuffer &inScanBuffer, IActivityZone &inZone)
    {
        if (m_TimeDataNeedsUpdate) {
            BuildTimeContext(inScanBuffer, inZone);
        }

        TTimeUnit newTime = inNewTime;
        if (newTime != m_CurrentTime) {
            SComponent &myNode = m_Component;
            {
                Mutex::ScopedLock __locker(m_ElementAccessMutex);
                myNode.SetDirty();
            }
            STimeEvent searchEvent(STimeEvent::Start, m_Allocator, m_CurrentTime);
            bool forward = newTime > m_CurrentTime;
            m_CurrentTime = NVMax((TTimeUnit)0, newTime);

            if (forward) {
                TTimeEventList::iterator iter = eastl::lower_bound(
                    m_TimeEvents.begin(), m_TimeEvents.end(), &searchEvent, STimeEventLessThan());

                for (TTimeEventList::iterator end = m_TimeEvents.end();
                     iter != end && (*iter)->m_Time <= newTime; ++iter) {
                    // Stop only works if the time is greater.
                    // start works if the time is equal.
                    bool isActive = true;
                    if ((*iter)->m_Type == STimeEvent::Stop)
                        isActive = newTime <= (*iter)->m_Time;

                    UpdateNodeList((*iter)->m_AffectedNodes, isActive);
                }
            } else {
                searchEvent.m_Type = STimeEvent::Stop;
                TTimeEventList::reverse_iterator iter =
                    eastl::lower_bound(m_TimeEvents.rbegin(), m_TimeEvents.rend(), &searchEvent,
                                       STimeEventGreaterThan());

                for (TTimeEventList::reverse_iterator end = m_TimeEvents.rend();
                     iter != end && (*iter)->m_Time >= newTime; ++iter) {
                    bool isActive = true;
                    if ((*iter)->m_Type == STimeEvent::Start)
                        isActive = newTime >= (*iter)->m_Time;

                    UpdateNodeList((*iter)->m_AffectedNodes, isActive);
                }
            }
        }
    }

    void UpdateNodeElementInformation(SElement &inNode)
    {
        Q3DStudio::UVariant attValue;
        if (inNode.GetAttribute(Q3DStudio::ATTRIBUTE_STARTTIME, attValue))
            inNode.m_ActivationManagerNode.m_StartTime = (QT3DSU32)attValue.m_INT32;

        if (inNode.GetAttribute(Q3DStudio::ATTRIBUTE_ENDTIME, attValue))
            inNode.m_ActivationManagerNode.m_StopTime = (QT3DSU32)attValue.m_INT32;

        inNode.m_ActivationManagerNode.m_Flags.SetTimeActive(false);
        // Note that we don't set the script information here.  The reason is that this requires the
        // script list
        // and we don't have access to it.  Plus it can't change when a slide changes so it is kind
        // of irrelevant.
    }

    bool BuildTimeContextElementNode(SElement &inNode, QT3DSU32 inGlobalMin, QT3DSU32 inGlobalMax)
    {
        // Note that just because a node itself does not participate in the time graph,
        // this does not mean that its children do not participate in the time graph.
        if (inNode.DoesParticipateInTimeGraph()) {
            UpdateNodeElementInformation(inNode);
            if (inNode.IsUserActive()) {
                // Start time does not get updated if the element doesn't have ATTRIBUTE_STARTTIME.
                if (inNode.m_ActivationManagerNode.m_StartTime != QT3DS_MAX_U32)
                    inGlobalMin = NVMax(inNode.m_ActivationManagerNode.m_StartTime, inGlobalMin);
                inGlobalMax = NVMin(inNode.m_ActivationManagerNode.m_StopTime, inGlobalMax);
                if (inGlobalMin < inGlobalMax) {
                    GetTimeEventForTime(inGlobalMin, STimeEvent::Start)
                        .m_AffectedNodes.push_back(&inNode);
                    GetTimeEventForTime(inGlobalMax, STimeEvent::Stop)
                        .m_AffectedNodes.push_back(&inNode);
                }
            }
        } else {
            inNode.m_ActivationManagerNode.m_Flags.SetTimeActive(true);
            // Carry global min/max information down so children get setup correctly.
            inNode.m_ActivationManagerNode.m_StartTime = inGlobalMin;
            inNode.m_ActivationManagerNode.m_StopTime = inGlobalMax;
        }
        // Is it worth looking at children at all
        return inNode.IsUserActive();
    }

    void BuildTimeContext(TScanBuffer &inScanBuffer, IActivityZone &inZone)
    {
        Mutex::ScopedLock __locker(m_ElementAccessMutex);
        QT3DS_ASSERT(m_TimeDataNeedsUpdate);
        m_TimeDataNeedsUpdate = false;
        SComponent &myNode = m_Component;
        SElement *theChild = myNode.m_Child;
        if (theChild == NULL) {
            return;
        }
        inScanBuffer.clear();
        for (SElement *theChild = myNode.m_Child; theChild; theChild = theChild->m_Sibling) {
            inScanBuffer.push_back(theChild);
        }
        for (QT3DSU32 idx = 0, end = (QT3DSU32)inScanBuffer.size(); idx < end; ++idx) {
            SScanBufferEntry theEntry(inScanBuffer[idx]);
            bool careAboutChildren = BuildTimeContextElementNode(
                *theEntry.m_Node, theEntry.m_StartTime, theEntry.m_EndTime);
            if (careAboutChildren && theEntry.m_Node->IsComponent() == false) {
                for (SElement *theChild = theEntry.m_Node->m_Child; theChild;
                     theChild = theChild->m_Sibling) {
                    inScanBuffer.push_back(SScanBufferEntry(
                        theChild, theEntry.m_Node->m_ActivationManagerNode.m_StartTime,
                        theEntry.m_Node->m_ActivationManagerNode.m_StopTime));
                }
                end = (QT3DSU32)inScanBuffer.size();
            }
            if (theEntry.m_Node->IsComponent())
                inZone.AddActivityItems(*theEntry.m_Node);
        }
    }

    void SetElementDirty(SElement &inNode)
    {
        SElement *theComponentParent = &inNode.GetComponentParent();
        if (inNode.IsComponent() && inNode.m_Parent)
            theComponentParent = &inNode.m_Parent->GetComponentParent();

        QT3DS_ASSERT(theComponentParent == &m_Component);
        if (m_AllNodesDirty == false) {
            // We check independent items every frame anyway so it isn't worth setting them dirty
            // here.
            if (inNode.IsIndependent() == false) {
                m_DirtyList.insert(inNode);
            }

            // Dirty to root time context.  This ensures that if we run an activate scan starting at
            // a parent
            // we won't have to re-run the scan at this node because it will not be dirty.
            // This also makes sure that if we mark all nodes dirty we continue the activate scan
            // till we hit at least
            // this node.
            SElement *theParent = inNode.m_Parent;
            while (theParent) {
                if (theParent->IsComponent()
                    || theParent->m_ActivationManagerNode.m_Flags.IsChildDirty())
                    theParent = NULL;
                else {
                    SActivationManagerNode &theNode = theParent->m_ActivationManagerNode;
                    theNode.m_Flags.SetChildDirty();
                    theParent = theParent->m_Parent;
                }
            }
        }
    }

    void setControlled(TActivityItem &item)
    {
        if (!m_ControlledList.contains(item)) {
            m_ControlledList.insert(item);
            qCInfo(TRACE_INFO) << "Element" << item.m_Name.c_str()
                               << "visibility now controlled by datainput. "
                                  "Visibility will no longer be affected by slide transitions.";
        }
    }

    static void UpdateItemScriptStatus(SElement &inNode, bool activeAndHasScript,
                                       TElementAndSortKeyList &scriptBuffer)
    {
        QT3DSU32 theIndex = inNode.Depth();
        eastl::pair<element::SElement *, QT3DSU32> theInsertedItem(&inNode, theIndex);
        if (activeAndHasScript) {
            if (inNode.m_ActivationManagerNode.m_Flags.HasScript() == false) {
                scriptBuffer.push_back(theInsertedItem);
                inNode.m_ActivationManagerNode.m_Flags.SetHasScript(true);
            }
        } else {
            if (inNode.m_ActivationManagerNode.m_Flags.HasScript() == true) {
                TElementAndSortKeyList::iterator theFind =
                    eastl::find(scriptBuffer.begin(), scriptBuffer.end(), theInsertedItem);
                if (theFind != scriptBuffer.end())
                    scriptBuffer.erase(theFind);
                inNode.m_ActivationManagerNode.m_Flags.SetHasScript(false);
            }
        }
    }

    static void HandleActivationChange(SElement &inNode, TElementAndSortKeyList &activateBuffer,
                                       TElementAndSortKeyList &deactivateBuffer,
                                       TElementAndSortKeyList &scriptBuffer,
                                       Mutex &inElementAccessMutex, bool &scriptBufferRequiresSort,
                                       bool inIsActive)
    {
        Mutex::ScopedLock __locker(inElementAccessMutex);
        TElementAndSortKey theKey(&inNode, inNode.Depth());
        if (inIsActive) {
            activateBuffer.push_back(TElementAndSortKey(theKey));
        } else {
            deactivateBuffer.push_back(theKey);
        }

        if (inNode.Flags().HasScriptCallbacks()) {
            UpdateItemScriptStatus(inNode, inIsActive, scriptBuffer);
            scriptBufferRequiresSort = true;
        }
        inNode.SetGlobalActive(inIsActive);
    }

    // Runs once we notice that the item's global active status has changed.
    // Note we explicitly do not update independent items because those are updated
    // as the first step of each time context update itself.
    // We do, however, remove them from the dirty list here.
    static void RunActivateScan(SElement &inNode, TScanBuffer &inScanBuffer,
                                TElementAndSortKeyList &activateBuffer,
                                TElementAndSortKeyList &deactivateBuffer,
                                TElementAndSortKeyList &scriptBuffer,
                                SActivationManagerNodeDirtyList &controlledList,
                                Mutex &inElementAccessMutex,
                                bool &scriptBufferRequiresSort, bool inRunFullScan)
    {
        inScanBuffer.clear();

        // Block used to hide variables to avoid an error.
        {
            bool parentActive = true;
            SElement *theParent = inNode.m_Parent;
            if (theParent != NULL)
                parentActive = theParent->IsGlobalActive();

            inScanBuffer.push_back(SScanBufferEntry(&inNode, parentActive));
        }

        for (QT3DSU32 idx = 0, end = inScanBuffer.size(); idx < end; ++idx) {
            SScanBufferEntry theEntry(inScanBuffer[idx]);
            SElement *theScanNode = theEntry.m_Node;
            QT3DS_ASSERT(theScanNode->IsIndependent() == false);
            bool parentActive = theEntry.IsParentActive();
            bool wasActive = theScanNode->IsGlobalActive();

            bool isControlledByDi
                    = controlledList.contains(*theScanNode) && theScanNode->m_OnMaster;

            // Override visibility for master slide elements that have datainput eyeball controller.
            bool isActive = isControlledByDi ? theScanNode->IsControlledActive()
                                             : theScanNode->IsGlobalActive(parentActive);
            bool wasChildDirty = theScanNode->m_ActivationManagerNode.m_Flags.IsChildDirty();
            theScanNode->m_ActivationManagerNode.m_Flags.ClearChildDirty();
            bool activateChange = isActive != wasActive;
            bool checkChildren = activateChange || (isActive && (wasChildDirty || inRunFullScan));
            if (activateChange) {
                HandleActivationChange(*theScanNode, activateBuffer, deactivateBuffer, scriptBuffer,
                                       inElementAccessMutex, scriptBufferRequiresSort, isActive);
            } else if (isControlledByDi) {
                qCInfo(TRACE_INFO) << "Element" << theScanNode->m_Name.c_str()
                                   << "visibility persistently controlled by datainput.";
            }

            if (checkChildren && theScanNode->m_Child) {
                for (SElement *theScanNodeChild = theScanNode->m_Child; theScanNodeChild;
                     theScanNodeChild = theScanNodeChild->m_Sibling) {
                    // Override visibility for master slide elements that have datainput
                    // eyeball controller.
                    isControlledByDi
                            = controlledList.contains(*theScanNodeChild)
                              && theScanNodeChild->m_OnMaster;

                    if (isControlledByDi)
                        theScanNodeChild->SetExplicitActive(theScanNodeChild->IsControlledActive());
                    if (theScanNodeChild->IsIndependent() == false)
                        inScanBuffer.push_back(SScanBufferEntry(theScanNodeChild, isActive));
                }
                end = inScanBuffer.size();
            }
        }
    }

    void RunDirtyScan(TScanBuffer &inScanBuffer, TElementNodePtrList &inTempDirtyList,
                      TElementAndSortKeyList &activateBuffer,
                      TElementAndSortKeyList &deactivateBuffer,
                      TElementAndSortKeyList &scriptBuffer, bool &scriptBufferRequiresSort)
    {
        if (m_AllNodesDirty == true) {
            inTempDirtyList.clear();
            SComponent &myNode = m_Component;
            for (SElement *theChild = myNode.m_Child; theChild; theChild = theChild->m_Sibling) {
                // Independent nodes don't need to be in the dirty list.
                if (theChild->IsIndependent() == false)
                    inTempDirtyList.push_back(theChild);
            }
        } else {
            inTempDirtyList.assign(m_DirtyList.begin(), m_DirtyList.end());
            // Reset nodes that are dirty to *not* be dirty.
            m_DirtyList.clear();
            // We have to sort the dirty list because it may have nodes out of order and the active
            // algorithm requires parents before children.  We use the depth member variable to
            // achieve this.
            eastl::sort(inTempDirtyList.begin(), inTempDirtyList.end(), SElementPtrSort());
        }
        for (QT3DSU32 idx = 0, end = (QT3DSU32)inTempDirtyList.size(); idx < end; ++idx) {
            SElement &dirtyNode(*inTempDirtyList[idx]);
            // This is slightly inefficient in the case where a both a child and parent are in the
            // dirty list.
            // This case has got to be extremely rare in practice, however.
            RunActivateScan(dirtyNode, inScanBuffer, activateBuffer, deactivateBuffer, scriptBuffer,
                            m_ControlledList, m_ElementAccessMutex, scriptBufferRequiresSort,
                            m_AllNodesDirty);
        }
        // Set at end so while debugging you can see if all nodes were dirty on method entry.
        m_AllNodesDirty = false;
    }

    bool Update(TTimeUnit inGlobalTime, TScanBuffer &inScanBuffer,
                TElementNodePtrList &inTempDirtyList, TElementAndSortKeyList &activateBuffer,
                TElementAndSortKeyList &deactivateBuffer, TElementAndSortKeyList &scriptBuffer,
                bool &scriptBufferRequiresSort, Q3DStudio::CComponentManager &inComponentManager,
                IPerfTimer &inPerfTimer, IActivityZone &inZone)
    {
        QT3DSU64 start = qt3ds::foundation::Time::getCurrentCounterValue();
        SComponent &theContextNode = m_Component;
        bool parentActive = true;
        SElement *theParent = theContextNode.m_Parent;
        if (theParent != NULL)
            parentActive = theParent->IsGlobalActive();

        bool wasActive = theContextNode.IsGlobalActive();
        bool isControlledByDi
                = m_ControlledList.contains(theContextNode) && theContextNode.m_OnMaster;

        // Override visibility for master slide elements that have datainput eyeball controller.
        bool isActive = isControlledByDi ? theContextNode.IsControlledActive()
                                         : theContextNode.IsGlobalActive(parentActive);
        bool activationChange = isActive != wasActive;
        inPerfTimer.Update("ActivationManager - Update Initial Vars",
                           qt3ds::foundation::Time::getCurrentCounterValue() - start);
        if (activationChange) {
            SStackPerfTimer __timer(inPerfTimer, "ActivationManager - Activation Change");
            HandleActivationChange(theContextNode, activateBuffer, deactivateBuffer, scriptBuffer,
                                   m_ElementAccessMutex, scriptBufferRequiresSort, isActive);
            m_DirtyList.clear();
            m_AllNodesDirty = true;
            if (isActive) {
                Mutex::ScopedLock __locker(m_ElementAccessMutex);
                // When the component is activated the first time go to the first slide
                // Subsequent activations should continue from the deactivated slide
                if (theContextNode.GetCurrentSlide() == 0
                        || inComponentManager.hasSlideChangeQueued(&theContextNode)) {
                    inComponentManager.GotoSlideIndex(&theContextNode, 1, false);
                    inComponentManager.applyQueuedChanges(&theContextNode);
                }
            } else
                RemoveOverride();
        }
        if (isActive) {
            SStackPerfTimer __timer(inPerfTimer, "ActivationManager - Update Local Time");
            bool atEndOfTime = CalculateNewTime(inGlobalTime);
            if (atEndOfTime) {
                Mutex::ScopedLock __locker(m_ElementAccessMutex);
                if (theContextNode.IsPlayThrough())
                    inComponentManager.PlaythroughToSlide(&theContextNode);
            }
            // Note the slide may have changed here and thus updated our time policy.
            // Set time active flags based on time.
            UpdateLocalTime(m_TimePolicy.GetTime(), inScanBuffer, inZone);
        }
        if (isActive || activationChange) {
            if (m_AllNodesDirty || m_DirtyList.size()) {
                SStackPerfTimer __timer(inPerfTimer, "ActivationManager - Dirty Scan");
                RunDirtyScan(inScanBuffer, inTempDirtyList, activateBuffer, deactivateBuffer,
                             scriptBuffer, scriptBufferRequiresSort);
            }
        }
        return isActive || activationChange;
    }

    void GoToTime(TTimeUnit inTime)
    {
        if (m_TimePolicyOverride) {
            m_TimePolicyOverride->SetTime(m_TimePolicy, inTime);
        } else {
            m_TimePolicy.SetTime(inTime);
        }
    }
};

IMPLEMENT_INVASIVE_SINGLE_LIST(TimeContext, m_NextSibling);

struct SActivityZone : public IActivityZone
{
    typedef nvhash_map<SElement *, NVScopedRefCounted<STimeContext>> TComponentTimeContextMap;
    NVFoundationBase &m_Foundation;
    Q3DStudio::CPresentation &m_Presentation;
    IStringTable &m_StringTable;
    nvvector<SElement *> m_RootElements;
    TComponentTimeContextMap m_TimeContexts;
    // These could potentially not be sorted but for 7.5 and TZ3 I don't want to risk the
    // bugs assocated with not producing the exact same output as the original algorithm.
    TElementAndSortKeyList m_ActivatedItems;
    TElementAndSortKeyList m_DeactivatedItems;

    // This will always need to be sorted because it is maintained across update calls
    TElementAndSortKeyList m_ScriptItems;

    // Temporaries used while scanning nodes for various features.
    TScanBuffer m_ScanBuffer;
    TElementNodePtrList m_TempDirtyList;
    nvvector<STimeContext *> m_TimeContextScanBuffer;

    TTimeContextList m_RootContexts;
    Mutex &m_ElementAccessMutex;
    Mutex m_UpdateCheckMutex;
    Sync m_UpdateSync;
    TTimeUnit m_GlobalTime;
    IPerfTimer *m_PerfTimer;
    STypeDesc m_DummyTypeDesc;
    SComponent m_DummyComponent;
    STimeContext m_DummyContext;
    bool m_Active;
    // True if m_ScriptItems needs to be resorted.
    bool m_ScriptRequiresSort;
    bool m_Updating;

    QT3DSI32 mRefCount;

    SActivityZone(NVFoundationBase &inFnd, Q3DStudio::CPresentation &inPres,
                  IStringTable &inStrTable, Mutex &inElementAccessMutex)
        : m_Foundation(inFnd)
        , m_Presentation(inPres)
        , m_StringTable(inStrTable)
        , m_RootElements(inFnd.getAllocator(), "Elements")
        , m_TimeContexts(inFnd.getAllocator(), "TimeContexts")
        , m_ActivatedItems(inFnd.getAllocator(), "m_ActivatedItems")
        , m_DeactivatedItems(inFnd.getAllocator(), "m_DeactivatedItems")
        , m_ScriptItems(inFnd.getAllocator(), "m_ScriptItems")
        , m_ScanBuffer(inFnd.getAllocator(), "m_ScanBuffer")
        , m_TempDirtyList(inFnd.getAllocator(), "m_TempDirtyList")
        , m_TimeContextScanBuffer(inFnd.getAllocator(), "m_TimeContextScanBuffer")
        , m_ElementAccessMutex(inElementAccessMutex)
        , m_UpdateCheckMutex(inFnd.getAllocator())
        , m_UpdateSync(inFnd.getAllocator())
        , m_GlobalTime(0)
        , m_PerfTimer(NULL)
        , m_DummyComponent(m_DummyTypeDesc)
        , m_DummyContext(m_Foundation.getAllocator(), m_DummyComponent, m_ElementAccessMutex)
        , m_Active(true)
        , m_ScriptRequiresSort(true)
        , m_Updating(false)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    Q3DStudio::CPresentation &GetPresentation() override { return m_Presentation; }

    static void SortElementBuffer(nvvector<eastl::pair<element::SElement *, QT3DSU32>> &inBuffer)
    {
        eastl::sort(inBuffer.begin(), inBuffer.end(), SElementIndexPairSorter());
    }

    static void AddElementNodeToBuffer(nvvector<eastl::pair<element::SElement *, QT3DSU32>> &inBuffer,
                                       SElement &inNode)
    {
        eastl::pair<element::SElement *, QT3DSU32> insertItem(&inNode, inNode.Depth());
        QT3DS_ASSERT(eastl::find(inBuffer.begin(), inBuffer.end(), insertItem) == inBuffer.end());
        inBuffer.push_back(insertItem);
    }

    TActivityItemBuffer GetActivatedItems() override { return m_ActivatedItems; }

    TActivityItemBuffer GetDeactivatedItems() override { return m_DeactivatedItems; }
    // All active items that are script enabled.
    TActivityItemBuffer GetScriptItems() override
    {
        // Created in a delayed way.
        if (m_ScriptRequiresSort) {
            m_ScriptRequiresSort = false;
            SortElementBuffer(m_ScriptItems);
        }
        return m_ScriptItems;
    }

    void SetZoneActive(bool inActive) override { m_Active = inActive; }
    bool IsZoneActive() override { return m_Active; }

    SActivationManagerNode *GetElementNodeForElement(TActivityItem item)
    {
        return &item.m_ActivationManagerNode;
    }

    STimeContext &GetItemTimeContext(SElement &inNode)
    {
        SElement &theComponent = inNode.GetComponentParent();
        if (!theComponent.IsComponent()) {
            QT3DS_ASSERT(false);
            return m_DummyContext;
        }

        eastl::pair<TComponentTimeContextMap::iterator, bool> inserter = m_TimeContexts.insert(
            eastl::make_pair(&theComponent, NVScopedRefCounted<STimeContext>()));
        if (inserter.second) {
            inserter.first->second = QT3DS_NEW(m_Foundation.getAllocator(), STimeContext)(
                m_Foundation.getAllocator(), static_cast<SComponent &>(theComponent),
                m_ElementAccessMutex);
            STimeContext &theNewContext = *inserter.first->second;
            if (theComponent.m_Parent == NULL)
                m_RootContexts.push_back(theNewContext);
            else {
                STimeContext &parentContext = GetItemTimeContext(*theComponent.m_Parent);
                parentContext.m_Children.push_back(theNewContext);
            }
        }
        return *inserter.first->second.mPtr;
    }

    void AddActivityItems(TActivityItem inNode) override
    {
        if (inNode.m_Parent == NULL)
            m_RootElements.push_back(&inNode);

        GetItemTimeContext(inNode);
    }

    CTimePolicy *GetOwnedTimePolicy(TActivityItem item) override
    {
        return &GetItemTimeContext(item).m_TimePolicy;
    }

    virtual SComponentTimePolicyOverride *
    GetOrCreateItemComponentOverride(TActivityItem item, float inMultiplier, TTimeUnit inEndTime,
                                     IComponentTimeOverrideFinishedCallback *inCallback) override
    {
        STimeContext &theContext = GetItemTimeContext(item);

        theContext.RemoveOverride();
        theContext.m_TimePolicyOverride =
            QT3DS_NEW(m_Foundation.getAllocator(),
                   SComponentTimePolicyOverride)(&item, inMultiplier, inEndTime, inCallback);
        theContext.m_TimePolicy.SetPaused(false);

        return theContext.m_TimePolicyOverride;
    }

    // If I am independent, then I am my own time parent.
    // else travel up the chain till you find an indpendent node.
    SElement &GetItemTimeParentNode(SElement &inNode)
    {
        if (inNode.m_Parent != NULL) {
            SElement &theParent = *inNode.m_Parent;
            if (theParent.IsIndependent())
                return theParent;
            return GetItemTimeParentNode(theParent);
        }
        // All roots are time independent by definition.
        return inNode;
    }

    TActivityItemPtr GetItemTimeParent(SElement &inNode) override
    {
        return &GetItemTimeParentNode(inNode);
    }

    void InsertIntoAppropriateDirtyList(SElement &inNode)
    {
        // This may need to be faster at some point...
        GetItemTimeContext(inNode).SetElementDirty(inNode);
    }

    bool GetItemUserActive(TActivityItem item) override { return item.IsExplicitActive(); }

    void UpdateItemScriptStatus(TActivityItem item) override
    {
        if (item.IsGlobalActive()) {
            STimeContext::UpdateItemScriptStatus(item, item.Flags().HasScriptCallbacks(),
                                                 m_ScriptItems);
            m_ScriptRequiresSort = true;
        }
    }

    void UpdateItemInfo(TActivityItem item) override { GetItemTimeContext(item).Reset(); }

    // Order of events will be:
    // 1. On Slide Change
    // 2. Time Update
    // 3. On Slide Change
    // 4. Update Local Time
    void OnSlideChange(TActivityItem item) override
    {
        STimeContext &theContext = GetItemTimeContext(item);
        // Rebuilding the context sets all the elements dirty.
        // This takes care of everything *but* the script item changes.
        theContext.Reset();
        theContext.RemoveOverride();
    }

    bool GetItemTimeActive(TActivityItem item) override
    {
        SActivationManagerNode *theNode = GetElementNodeForElement(item);
        if (theNode)
            return theNode->m_Flags.IsTimeActive();
        return false;
    }

    TTimeUnit GetItemLocalTime(TActivityItem item) override
    {
        SElement *theElem = &item.GetComponentParent();
        if (item.IsComponent() && item.m_Parent)
            theElem = &item.m_Parent->GetComponentParent();

        return GetItemTimeContext(*theElem).m_CurrentTime;
    }

    TTimeUnit GetItemComponentTime(TActivityItem item) override
    {
        return GetItemTimeContext(item).m_CurrentTime;
    }

    bool IsUpdating() override
    {
        Mutex::ScopedLock __locker(m_UpdateCheckMutex);
        return m_Updating;
    }

    void EndUpdate() override
    {
        m_UpdateSync.reset();
        while (IsUpdating())
            m_UpdateSync.wait();
    }

    void DoUpdate()
    {
        SStackPerfTimer __timer(m_PerfTimer, "ActivationManager - DoUpdate");
        if (m_Active) {
            // We know that parent elements are added before children.
            // So we know the time contexts are in an appropriate order, assuming they completely
            // resolve their results before the next time context runs.
            Q3DStudio::CComponentManager &theManager =
                static_cast<Q3DStudio::CComponentManager &>(m_Presentation.GetComponentManager());
            m_TimeContextScanBuffer.clear();

            for (TTimeContextList::iterator iter = m_RootContexts.begin(),
                                            end = m_RootContexts.end();
                 iter != end; ++iter)
                m_TimeContextScanBuffer.push_back(&(*iter));

            for (QT3DSU32 idx = 0, end = m_TimeContextScanBuffer.size(); idx < end; ++idx) {
                STimeContext &theContext = *m_TimeContextScanBuffer[idx];
                bool checkChildren =
                    theContext.Update(m_GlobalTime, m_ScanBuffer, m_TempDirtyList, m_ActivatedItems,
                                      m_DeactivatedItems, m_ScriptItems, m_ScriptRequiresSort,
                                      theManager, *m_PerfTimer, *this);
                if (checkChildren) {
                    for (TTimeContextList::iterator timeIter = theContext.m_Children.begin(),
                                                    timeEnd = theContext.m_Children.end();
                         timeIter != timeEnd; ++timeIter) {
                        m_TimeContextScanBuffer.push_back(&(*timeIter));
                    }
                    end = m_TimeContextScanBuffer.size();
                }
            }
        }

        eastl::sort(m_ActivatedItems.begin(), m_ActivatedItems.end(), SElementIndexPairSorter());
        eastl::sort(m_DeactivatedItems.begin(), m_DeactivatedItems.end(),
                    SElementIndexPairSorter());
        Mutex::ScopedLock __locker(m_UpdateCheckMutex);
        {
            m_Updating = false;
            m_UpdateSync.set();
        }
    }

    static void UpdateCallback(void *data)
    {
        SActivityZone *theZone = reinterpret_cast<SActivityZone *>(data);
        theZone->DoUpdate();
    }

    void BeginUpdate(TTimeUnit inGlobalTime, IPerfTimer &inPerfTimer,
                             IThreadPool &inThreadPool) override
    {
        // It is expected that an update is not running.
        m_ActivatedItems.clear();
        m_DeactivatedItems.clear();
        m_GlobalTime = inGlobalTime;
        m_PerfTimer = &inPerfTimer;
        m_Updating = true;
        inThreadPool.AddTask(this, UpdateCallback, NULL);
    }

    void GoToTime(TActivityItem inItem, TTimeUnit inTime) override
    {
        GetItemTimeContext(inItem).GoToTime(inTime);
    }

    void setControlled(TActivityItem inItem) override
    {
        GetItemTimeContext(inItem).setControlled(inItem);
    }
};

struct SActivityZoneManager : public IActivityZoneManager
{
    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    // Zones may access the element mutex on destruction *so* it is important that
    // it is destroyed *after* the zone list.
    Mutex m_ElementAccessMutex;
    nvvector<NVScopedRefCounted<SActivityZone>> m_Zones;
    QT3DSI32 mRefCount;

    SActivityZoneManager(NVFoundationBase &fnd, IStringTable &inStrTable)
        : m_Foundation(fnd)
        , m_StringTable(inStrTable)
        , m_ElementAccessMutex(m_Foundation.getAllocator())
        , m_Zones(m_Foundation.getAllocator(), "m_Zones")
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    IActivityZone &CreateActivityZone(Q3DStudio::CPresentation &inPresentation) override
    {
        m_Zones.push_back(QT3DS_NEW(m_Foundation.getAllocator(), SActivityZone)(
            m_Foundation, inPresentation, m_StringTable, m_ElementAccessMutex));
        return *m_Zones.back().mPtr;
    }
};
}

IActivityZoneManager &
IActivityZoneManager::CreateActivityZoneManager(NVFoundationBase &inFoundation,
                                                IStringTable &inStrTable)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SActivityZoneManager)(inFoundation, inStrTable);
}
