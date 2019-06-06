/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include <EASTL/algorithm.h>

#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAllocator.h"

#include "EventPoller.h"
#include "EventFactory.h"

#define MAX_EVENTS 256

using namespace qt3ds::evt;
using qt3ds::QT3DSU32;

IEventSystem &IEventSystem::Create(qt3ds::NVFoundationBase &inFoundation)
{
    return CPoller::Create(inFoundation);
}

CPoller &CPoller::Create(qt3ds::NVFoundationBase &inFoundation)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), CPoller)(inFoundation);
}

CPoller::~CPoller()
{
    for (QT3DSU32 idx = 0, end = m_Providers.size(); idx < end; ++idx)
        m_Providers[idx].first->Release();
    m_Providers.clear();
    ReleaseEvents();
    m_EventFactory.Release();
}

void CPoller::ReleaseEvents()
{
    m_EventFactory.ReleaseOutstandingEvents();

    m_EventList.clear();
    m_EventIndex = 0;
}

static void AddProviderFn(void *inPoller, Qt3DSEventSystemEventProvider inProvider)
{
    CPoller *thePoller = reinterpret_cast<CPoller *>(inPoller);
    thePoller->AddProvider(*thePoller->GetOrCreateEventProviderWrapper(inProvider));
}

static void RemoveProviderFn(void *inPoller, Qt3DSEventSystemEventProvider inProvider)
{
    CPoller *thePoller = reinterpret_cast<CPoller *>(inPoller);
    CPoller::TProvider theProvider = thePoller->GetEventProviderWrapper(inProvider);
    if (theProvider)
        thePoller->RemoveProvider(*theProvider);
    thePoller->ReleaseEventProviderWrapper(inProvider);
}

static int IgnoreProviderFn(void *inPoller, Qt3DSEventSystemEventProvider inProvider)
{
    CPoller *thePoller = reinterpret_cast<CPoller *>(inPoller);
    CPoller::TProvider theProvider = thePoller->GetEventProviderWrapper(inProvider);
    if (theProvider)
        return thePoller->IgnoreProvider(*theProvider);
    return 0;
}

static int ActivateProviderFn(void *inPoller, Qt3DSEventSystemEventProvider inProvider)
{
    CPoller *thePoller = reinterpret_cast<CPoller *>(inPoller);
    CPoller::TProvider theProvider = thePoller->GetEventProviderWrapper(inProvider);
    if (theProvider)
        return thePoller->ActivateProvider(*theProvider);
    return 0;
}

CPoller::CPoller(qt3ds::NVFoundationBase &inFoundation)
    : mRefCount(0)
    , m_Foundation(inFoundation)
    , m_EventFactory(CFactory::Create(inFoundation))
    , m_Providers(inFoundation.getAllocator(), "CPoller::m_Providers")
    , m_EventList(inFoundation.getAllocator(), "CPoller::m_EventList")
    , m_EventIndex(0)
    , m_EventFetched(false)
{
    m_CInterface.m_Poller = this;
    m_CInterface.addProvider = AddProviderFn;
    m_CInterface.removeProvider = RemoveProviderFn;
    m_CInterface.ignoreProvider = IgnoreProviderFn;
    m_CInterface.activateProvider = ActivateProviderFn;
}

struct SProviderSearch
{
    IEventProvider *m_Provider;
    SProviderSearch(IEventProvider &inProvider)
        : m_Provider(&inProvider)
    {
    }
    bool operator()(const eastl::pair<CPoller::TProvider, bool> &item)
    {
        return item.first == m_Provider;
    }
};

void CPoller::AddProvider(IEventProvider &inProvider)
{
    TProviderList::iterator iter =
        eastl::find_if(m_Providers.begin(), m_Providers.end(), SProviderSearch(inProvider));
    if (iter == m_Providers.end())
        m_Providers.push_back(eastl::make_pair(&inProvider, true));
}

void CPoller::RemoveProvider(IEventProvider &inProvider)
{
    TProviderList::iterator thePos =
        eastl::find_if(m_Providers.begin(), m_Providers.end(), SProviderSearch(inProvider));
    if (thePos != m_Providers.end())
        m_Providers.erase(thePos);
}

bool CPoller::IgnoreProvider(IEventProvider &inProvider)
{
    TProviderList::iterator thePos =
        eastl::find_if(m_Providers.begin(), m_Providers.end(), SProviderSearch(inProvider));
    if (thePos != m_Providers.end()) {
        thePos->second = false;
        return true;
    }
    return false;
}

bool CPoller::ActivateProvider(IEventProvider &inProvider)
{
    TProviderList::iterator thePos =
        eastl::find_if(m_Providers.begin(), m_Providers.end(), SProviderSearch(inProvider));
    if (thePos != m_Providers.end()) {
        thePos->second = true;
        return true;
    }
    return false;
}

Qt3DSEventSystemEvent *CPoller::GetNextEvent(bool inAllowResetBuffer)
{
    if (m_EventIndex < m_EventList.size()) {
        Qt3DSEventSystemEvent *retval = m_EventList[m_EventIndex];
        ++m_EventIndex;
        return retval;
    }
    if (inAllowResetBuffer) {
        ReleaseEvents();
    }
    size_t theNewEventCount = 0;
    for (QT3DSU32 idx = 0, end = m_Providers.size(); idx < end; ++idx) {
        if (m_Providers[idx].second) {
            Qt3DSEventSystemEvent *evtBuffer[MAX_EVENTS];
            for (QT3DSU32 numEvents = (QT3DSU32)(m_Providers[idx].first)
                                       ->GetNextEvents(m_EventFactory, evtBuffer, MAX_EVENTS);
                 numEvents;
                 numEvents = (QT3DSU32)(m_Providers[idx].first)
                                 ->GetNextEvents(m_EventFactory, evtBuffer, MAX_EVENTS)) {
                m_EventList.insert(m_EventList.end(), evtBuffer, evtBuffer + numEvents);
                ++theNewEventCount;
            }
        }
    }
    if (theNewEventCount)
        return GetNextEvent(inAllowResetBuffer);

    return NULL;
}

size_t CPoller::GetNextEvents(Qt3DSEventSystemEvent **outBuffer, size_t bufLen)
{
    m_EventFetched = true;
    size_t bufIdx = 0;
    bool theAllowResetBuffer = true;
    for (; bufIdx < bufLen; ++bufIdx) {
        Qt3DSEventSystemEvent *evt = GetNextEvent(theAllowResetBuffer);
        if (!evt)
            break;
        theAllowResetBuffer = false;
        outBuffer[bufIdx] = evt;
    }
    return bufIdx;
}

IEventFactory &CPoller::GetEventFactory()
{
    return m_EventFactory;
}

Qt3DSEventSystemEventPoller *CPoller::GetCInterface()
{
    return &m_CInterface;
}

bool CPoller::GetAndClearEventFetchedFlag()
{
    bool theOld = m_EventFetched;
    m_EventFetched = false;
    return theOld;
}

void CPoller::PurgeEvents()
{
    // Get events from providers and drop,
    // so the providers need not to provide an additional interface for clear events
    static const size_t EVENTS_NUM_ONCE_CLEAR = 512;
    Qt3DSEventSystemEvent *theEvents[EVENTS_NUM_ONCE_CLEAR];
    GetNextEvents(theEvents, EVENTS_NUM_ONCE_CLEAR);
    ReleaseEvents();
}

void CPoller::addRef()
{
    qt3ds::foundation::atomicIncrement(&mRefCount);
}

void CPoller::release()
{
    qt3ds::QT3DSI32 value = qt3ds::foundation::atomicDecrement(&mRefCount);
    if (value <= 0) {
        qt3ds::NVAllocatorCallback &alloc(m_Foundation.getAllocator());
        this->~CPoller();
        QT3DS_FREE(alloc, this);
    }
}

struct SProviderFinder
{
    void *m_Key;
    SProviderFinder(void *k)
        : m_Key(k)
    {
    }
    bool operator()(const CPoller::TCEventProviderPair &inPair)
    {
        if (inPair.first == m_Key)
            return true;
        return false;
    }
};

static Qt3DSEventSystemEvent *CreateEventFn(void *inFactory, int numData)
{
    IEventFactory *theFactory = reinterpret_cast<IEventFactory *>(inFactory);
    return &theFactory->CreateEvent(numData);
}

static size_t MaxNumEventDataFn(void *inFactory)
{
    IEventFactory *theFactory = reinterpret_cast<IEventFactory *>(inFactory);
    return theFactory->GetMaxNumEventData();
}

static size_t MaxStrLenFn(void *inFactory)
{
    IEventFactory *theFactory = reinterpret_cast<IEventFactory *>(inFactory);
    return theFactory->GetMaxStrLength();
}

static Qt3DSEventSystemRegisteredStr RegisterStrFn(void *inFactory, Qt3DSEventSystemStr inStr)
{
    IEventFactory *theFactory = reinterpret_cast<IEventFactory *>(inFactory);
    return theFactory->RegisterStr(inStr);
}

static Qt3DSEventSystemStr AllocateStrFn(void *inFactory, Qt3DSEventSystemStr inStr)
{
    IEventFactory *theFactory = reinterpret_cast<IEventFactory *>(inFactory);
    return theFactory->AllocateStr(inStr);
}

static Qt3DSEventSystemStr AllocateStrLenFn(void *inFactory, int len)
{
    IEventFactory *theFactory = reinterpret_cast<IEventFactory *>(inFactory);
    return theFactory->AllocateStr(len);
}

struct SWrappedProvider : public IEventProvider
{
    Qt3DSEventSystemEventProvider m_Provider;
    SWrappedProvider(Qt3DSEventSystemEventProvider prov)
        : m_Provider(prov)
    {
    }

    size_t GetNextEvents(IEventFactory &inFactory, Qt3DSEventSystemEvent **outBuffer,
                                 size_t bufLen) override
    {
        if (m_Provider.m_Provider) {
            Qt3DSEventSystemEventFactory theFactory;
            theFactory.m_Factory = &inFactory;
            theFactory.createEvent = CreateEventFn;
            theFactory.getMaxNumEventData = MaxNumEventDataFn;
            theFactory.getMaxStrLength = MaxStrLenFn;
            theFactory.registerStr = RegisterStrFn;
            theFactory.allocateStr = AllocateStrFn;
            theFactory.allocateStrLen = AllocateStrLenFn;
            return m_Provider.getNextEvents(m_Provider.m_Provider, theFactory, outBuffer, bufLen);
        }
        return 0;
    }

    void Release() override
    {
        if (m_Provider.m_Provider)
            m_Provider.release(m_Provider.m_Provider);
        delete this;
    }
};

CPoller::TProvider CPoller::GetEventProviderWrapper(Qt3DSEventSystemEventProvider inProvider)
{
    TCEventProviderList::iterator iter = eastl::find_if(
        m_CEventProviders.begin(), m_CEventProviders.end(), SProviderFinder(inProvider.m_Provider));
    if (iter != m_CEventProviders.end())
        return iter->second;
    return TProvider();
}

CPoller::TProvider CPoller::GetOrCreateEventProviderWrapper(Qt3DSEventSystemEventProvider inProvider)
{
    TCEventProviderList::iterator iter = eastl::find_if(
        m_CEventProviders.begin(), m_CEventProviders.end(), SProviderFinder(inProvider.m_Provider));
    if (iter == m_CEventProviders.end()) {
        TProvider retval = new SWrappedProvider(inProvider);
        m_CEventProviders.push_back(eastl::make_pair(inProvider.m_Provider, retval));
        return retval;
    }
    return iter->second;
}

void CPoller::ReleaseEventProviderWrapper(Qt3DSEventSystemEventProvider inProvider)
{
    TCEventProviderList::iterator iter = eastl::find_if(
        m_CEventProviders.begin(), m_CEventProviders.end(), SProviderFinder(inProvider.m_Provider));
    if (iter != m_CEventProviders.end()) {
        SWrappedProvider &wrappedProvider = static_cast<SWrappedProvider &>(*iter->second);
        wrappedProvider.m_Provider.m_Provider = NULL;
        m_CEventProviders.erase(
            iter); // deletes the wrapper but does not release the wrapped provider.
    }
}
