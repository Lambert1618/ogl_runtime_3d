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
#include "Qt3DSLogicSystem.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSIPresentation.h"
#include "Qt3DSApplication.h"
#include "foundation/SerializationTypes.h"
#include "foundation/IOStreams.h"

using namespace qt3ds::runtime;
using namespace qt3ds::foundation;
using namespace qt3ds;
using namespace qt3ds::runtime::element;

namespace {
struct SLogicKey
{
    QT3DSU32 m_ElementHandle;
    Q3DStudio::TEventCommandHash m_CommandHash;
    SLogicKey(QT3DSU32 hdl = 0, Q3DStudio::TEventCommandHash hash = 0)
        : m_ElementHandle(hdl)
        , m_CommandHash(hash)
    {
    }

    bool operator==(const SLogicKey &other) const
    {
        return m_ElementHandle == other.m_ElementHandle && m_CommandHash == other.m_CommandHash;
    }
    size_t hash() const
    {
        return eastl::hash<QT3DSU32>()(m_ElementHandle) ^ eastl::hash<QT3DSU32>()((QT3DSU32)m_CommandHash);
    }
};

struct SLogicData
{
    SLogicData *m_NextLogicData;
    QT3DSU32 m_Owner;
    QT3DSU32 m_Target;
    Q3DStudio::TEventCommandHash m_Type;
    Q3DStudio::UVariant m_Arg1;
    Q3DStudio::UVariant m_Arg2;
    QT3DSI32 m_Id;
    bool m_Active;

    SLogicData()
        : m_NextLogicData(NULL)
        , m_Owner(0)
        , m_Target(0)
        , m_Type(0)
        , m_Id(0)
        , m_Active(false)
    {
        m_Arg1.m_INT32 = 0;
        m_Arg2.m_INT32 = 0;
    }

    SLogicData(QT3DSU32 owner, QT3DSU32 target, Q3DStudio::TEventCommandHash type, Q3DStudio::UVariant a1,
               Q3DStudio::UVariant a2, bool active, QT3DSI32 inId)
        : m_NextLogicData(NULL)
        , m_Owner(owner)
        , m_Target(target)
        , m_Type(type)
        , m_Arg1(a1)
        , m_Arg2(a2)
        , m_Id(inId)
        , m_Active(active)
    {
    }
};

DEFINE_INVASIVE_SINGLE_LIST(LogicData);
IMPLEMENT_INVASIVE_SINGLE_LIST(LogicData, m_NextLogicData);
}

namespace eastl {
template <>
struct hash<SLogicKey>
{
    size_t operator()(const SLogicKey &inKey) const { return inKey.hash(); }
};
}

namespace {

struct SLogicSystem : public ILogicSystem
{
    typedef nvhash_map<SLogicKey, TLogicDataList> TLogicKeyHash;
    typedef nvhash_map<QT3DSI32, SLogicKey> TIdLogicKeyHash;
    typedef Pool<SLogicData, ForwardingAllocator> TLogicDataPool;

    NVFoundationBase &m_Foundation;

    TLogicKeyHash m_LogicKeys;
    TIdLogicKeyHash m_IdToLogicKeys;
    TLogicDataPool m_LogicDataPool;
    QT3DSI32 m_NextId;
    NVDataRef<QT3DSU8> m_LoadData;

    QT3DSI32 m_RefCount;

    SLogicSystem(NVFoundationBase &inFoundation)
        : m_Foundation(inFoundation)
        , m_LogicKeys(inFoundation.getAllocator(), "m_LogicKeys")
        , m_IdToLogicKeys(inFoundation.getAllocator(), "m_IdToLogicKeys")
        , m_LogicDataPool(ForwardingAllocator(inFoundation.getAllocator(), "m_LogicDataPool"))
        , m_NextId(1)
        , m_RefCount(0)
    {
    }

    void addRef() override { atomicIncrement(&m_RefCount); }
    void release() override
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(m_Foundation.getAllocator());
            NVDelete(alloc, this);
        }
    }
    void IncrementNextId()
    {
        ++m_NextId;
        if (m_NextId == 0)
            ++m_NextId;
    }
    static QT3DSU32 GetHandle(element::SElement *elem)
    {
        if (elem)
            return elem->GetHandle();
        return 0;
    }
    // returns the action id
    QT3DSI32 AddAction(element::SElement &inTrigger,
                            Q3DStudio::TEventCommandHash inEventNameHash,
                            element::SElement *inTarget, element::SElement *inOwner,
                            Q3DStudio::TEventCommandHash inType, Q3DStudio::UVariant inArg1,
                            Q3DStudio::UVariant inArg2, bool inActive) override
    {
        SLogicKey theKey(inTrigger.GetHandle(), inEventNameHash);
        eastl::pair<TLogicKeyHash::iterator, bool> inserter =
            m_LogicKeys.insert(eastl::make_pair(theKey, TLogicDataList()));

        eastl::pair<TIdLogicKeyHash::iterator, bool> idInserter;
        for (idInserter = m_IdToLogicKeys.insert(eastl::make_pair(m_NextId, theKey));
             idInserter.second == false;
             idInserter = m_IdToLogicKeys.insert(eastl::make_pair(m_NextId, theKey))) {
            IncrementNextId();
        }
        IncrementNextId();

        SLogicData *theData = m_LogicDataPool.construct(__FILE__, __LINE__);
        *theData = SLogicData(GetHandle(inOwner), GetHandle(inTarget), inType, inArg1, inArg2,
                              inActive, idInserter.first->first);
        inserter.first->second.push_back(*theData);
        return idInserter.first->first;
    }

    void OnEvent(Q3DStudio::TEventCommandHash inEventName, element::SElement &inTarget,
                         Q3DStudio::IPresentation &inPresentation) const override
    {
        SLogicKey theKey(inTarget.GetHandle(), inEventName);
        TLogicKeyHash::const_iterator iter = m_LogicKeys.find(theKey);
        if (iter != m_LogicKeys.end()) {
            qt3ds::runtime::IApplication &theApplication = inPresentation.GetApplication();
            qt3ds::runtime::IElementAllocator &theElemAllocator =
                theApplication.GetElementAllocator();
            for (TLogicDataList::const_iterator listIter = iter->second.begin(),
                                                endIter = iter->second.end();
                 listIter != endIter; ++listIter) {
                if (listIter->m_Active) {
                    inPresentation.FireCommand(
                        listIter->m_Type, theElemAllocator.FindElementByHandle(listIter->m_Target),
                        &listIter->m_Arg1, &listIter->m_Arg2);
                }
            }
        }
    }

    void SetActive(QT3DSI32 inActionIndex, bool inActive, IElementAllocator &inElemAllocator) override
    {
        TIdLogicKeyHash::iterator iter = m_IdToLogicKeys.find(inActionIndex);
        if (iter != m_IdToLogicKeys.end()) {
            TLogicKeyHash::iterator logicIter = m_LogicKeys.find(iter->second);
            if (logicIter != m_LogicKeys.end()) {
                for (TLogicDataList::iterator listIter = logicIter->second.begin(),
                                              listEnd = logicIter->second.end();
                     listIter != listEnd; ++listIter)
                    if (listIter->m_Id == inActionIndex) {
                        listIter->m_Active = inActive;
                        if (IApplication::isPickingEvent(logicIter->first.m_CommandHash)) {
                            SElement *theElement = inElemAllocator.FindElementByHandle(
                                logicIter->first.m_ElementHandle);
                            if (theElement && inActive)
                                theElement->SetFlag(Q3DStudio::ELEMENTFLAG_PICKENABLED, true);
                        }
                    }
            }
        }
    }

    void SaveBinaryData(qt3ds::foundation::IOutStream &ioStream) override
    {
        qt3ds::foundation::SWriteBuffer theWriter(m_Foundation.getAllocator(), "WriteBuffer");
        theWriter.write(m_NextId);
        theWriter.write((QT3DSU32)m_LogicKeys.size());
        for (TLogicKeyHash::iterator iter = m_LogicKeys.begin(), end = m_LogicKeys.end();
             iter != end; ++iter) {
            if (iter->second.m_Head == NULL)
                continue;

            theWriter.write(iter->first);
            QT3DSU32 datumOffset = theWriter.size();
            theWriter.write((QT3DSU32)0);
            QT3DSU32 datumCount = 0;
            for (SLogicData *theDatum = iter->second.m_Head; theDatum;
                 theDatum = theDatum->m_NextLogicData, ++datumCount) {
                theWriter.write(*theDatum);
            }
            QT3DSU32 *countPtr = reinterpret_cast<QT3DSU32 *>(theWriter.begin() + datumOffset);
            *countPtr = datumCount;
        }
        ioStream.Write(theWriter.begin(), theWriter.size());
    }

    void LoadBinaryData(NVDataRef<QT3DSU8> inLoadData) override
    {
        m_LoadData = inLoadData;
        SDataReader theReader(inLoadData.begin(), inLoadData.end());
        m_NextId = *theReader.Load<QT3DSI32>();
        QT3DSU32 numKeys = *theReader.Load<QT3DSU32>();
        for (QT3DSU32 idx = 0, end = numKeys; idx < end; ++idx) {
            SLogicKey theKey = *theReader.Load<SLogicKey>();
            QT3DSU32 numActions = *theReader.Load<QT3DSU32>();
            if (numActions == 0)
                continue;

            TLogicKeyHash::iterator inserter =
                m_LogicKeys.insert(eastl::make_pair(theKey, TLogicDataList())).first;
            SLogicData *lastDatum = NULL;
            for (QT3DSU32 actIdx = 0, actEnd = numActions; actIdx < actEnd; ++actIdx) {
                SLogicData *nextDatum = theReader.Load<SLogicData>();
                if (lastDatum == NULL)
                    inserter->second.m_Head = nextDatum;
                else
                    lastDatum->m_NextLogicData = nextDatum;
                lastDatum = nextDatum;
                m_IdToLogicKeys.insert(eastl::make_pair(nextDatum->m_Id, theKey));
            }
            if (lastDatum)
                lastDatum->m_NextLogicData = NULL;
        }
    }
};
}

ILogicSystem &ILogicSystem::CreateLogicSystem(NVFoundationBase &inFnd)
{
    return *QT3DS_NEW(inFnd.getAllocator(), SLogicSystem)(inFnd);
}
