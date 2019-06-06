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
#include "Qt3DSParametersSystem.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSIndexableLinkedList.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/SerializationTypes.h"
#include "foundation/IOStreams.h"

using namespace qt3ds::runtime;

namespace {

struct SParameterGroup
{
    enum {
        NumParametersPerGroup = 4,
    };
    TIdValuePair m_Data[NumParametersPerGroup];
    SParameterGroup *m_NextNode;

    SParameterGroup()
        : m_NextNode(NULL)
    {
    }
};

struct SParameterGroupEntry
{
    QT3DSU32 m_ParameterCount;
    SParameterGroup *m_FirstGroup;
    SParameterGroupEntry()
        : m_ParameterCount(0)
        , m_FirstGroup(NULL)
    {
    }
};

typedef IndexableLinkedList<SParameterGroup, TIdValuePair, SParameterGroup::NumParametersPerGroup>
    TParametersList;

struct SParamSystem : public IParametersSystem
{
    typedef nvhash_map<QT3DSI32, SParameterGroupEntry> TIdGroupHash;

    NVFoundationBase &m_Foundation;
    TParametersList::TPoolType m_ParametersPool;
    TIdGroupHash m_Groups;
    QT3DSI32 m_NextId;
    NVDataRef<QT3DSU8> m_LoadData;
    QT3DSI32 m_RefCount;

    SParamSystem(NVFoundationBase &fnd)
        : m_Foundation(fnd)
        , m_ParametersPool(ForwardingAllocator(fnd.getAllocator(), "ParametersPool"))
        , m_Groups(fnd.getAllocator(), "m_Groups")
        , m_NextId(1)
        , m_RefCount(0)
    {
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

    void IncrementId()
    {
        ++m_NextId;
        if (!m_NextId)
            ++m_NextId;
    }

    QT3DSI32 CreateParameterGroup() override
    {
        eastl::pair<TIdGroupHash::iterator, bool> inserter =
            m_Groups.insert(eastl::make_pair(m_NextId, SParameterGroupEntry()));
        while (inserter.second == false) {
            IncrementId();
            inserter = m_Groups.insert(eastl::make_pair(m_NextId, SParameterGroupEntry()));
        }
        return inserter.first->first;
    }

    void AddParameter(QT3DSI32 inGroup, QT3DSU32 inNameHash, Q3DStudio::UVariant inParam) override
    {
        TIdGroupHash::iterator iter = m_Groups.find(inGroup);
        if (iter == m_Groups.end()) {
            QT3DS_ASSERT(false);
            return;
        }

        SParameterGroupEntry &theEntry(iter->second);
        TIdValuePair &thePair = TParametersList::Create(
            theEntry.m_FirstGroup, theEntry.m_ParameterCount, m_ParametersPool);
        thePair = eastl::make_pair(inNameHash, inParam);
    }

    QT3DSU32 GetNumParameters(QT3DSI32 inGroup) const override
    {
        TIdGroupHash::const_iterator iter = m_Groups.find(inGroup);
        if (iter == m_Groups.end()) {
            QT3DS_ASSERT(false);
            return 0;
        }

        const SParameterGroupEntry &theEntry(iter->second);
        return theEntry.m_ParameterCount;
    }
    TIdValuePair GetParameter(QT3DSI32 inGroup, QT3DSU32 inIndex) const override
    {
        Q3DStudio::UVariant dummyValue;
        dummyValue.m_INT32 = 0;
        TIdValuePair retval = TIdValuePair(0, dummyValue);

        TIdGroupHash::const_iterator iter = m_Groups.find(inGroup);
        if (iter == m_Groups.end()) {
            QT3DS_ASSERT(false);
            return retval;
        }

        const SParameterGroupEntry &theEntry(iter->second);

        if (inIndex < theEntry.m_ParameterCount)
            return TParametersList::GetObjAtIdx(theEntry.m_FirstGroup, inIndex);

        QT3DS_ASSERT(false);
        return retval;
    }
};
}

IParametersSystem &IParametersSystem::CreateParametersSystem(NVFoundationBase &fnd)
{
    return *QT3DS_NEW(fnd.getAllocator(), SParamSystem)(fnd);
}
