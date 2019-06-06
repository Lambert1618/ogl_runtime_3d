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
#include "Qt3DSDMSignalSystem.h"
#include "EASTL/hash_map.h"

using namespace qt3dsdm;

namespace {

struct SSignalListener;
class ISignalSystemImpl
{
public:
    virtual ~ISignalSystemImpl() {}
    virtual void RemoveListener(SSignalListener &inListener) = 0;
};

struct SSignalSystemKey
{
    void *m_Sender;
    const char *m_SignalName;
    SSignalSystemKey(void *inSender, const char *inSigName, IStringTable &inStrTable)
        : m_Sender(inSender)
        , m_SignalName(inStrTable.GetNarrowStr(inSigName))
    {
    }
    SSignalSystemKey(const SSignalSystemKey &inOther)
        : m_Sender(inOther.m_Sender)
        , m_SignalName(inOther.m_SignalName)
    {
    }

    SSignalSystemKey &operator=(const SSignalSystemKey &inOther)
    {
        if (this != &inOther) {
            m_Sender = inOther.m_Sender;
            m_SignalName = inOther.m_SignalName;
        }
        return *this;
    }

    bool operator==(const SSignalSystemKey &inOther) const
    {
        return m_Sender == inOther.m_Sender && m_SignalName == inOther.m_SignalName;
    }
};

struct SSignalListener : ISignalConnection
{
    shared_ptr<ISignalSystemImpl> m_SignalSystem;
    TGenericSignalHandlerFunc m_Handler;
    SSignalSystemKey m_Key;
    SSignalListener(shared_ptr<ISignalSystemImpl> inSystem, TGenericSignalHandlerFunc inHandler,
                    SSignalSystemKey inKey)
        : m_SignalSystem(inSystem)
        , m_Handler(inHandler)
        , m_Key(inKey)
    {
    }
    ~SSignalListener() { m_SignalSystem->RemoveListener(*this); }
    void Signal(void *inSender, const char *inName, const char *inData, size_t inDataSize)
    {
        m_Handler(inSender, inName, inData, inDataSize);
    }
};

typedef eastl::vector<SSignalListener *> TSignalListenerPtrList;
}

namespace eastl {
template <>
struct hash<SSignalSystemKey>
{
    size_t operator()(const SSignalSystemKey &inKey) const
    {
        return hash<void *>()(inKey.m_Sender) ^ hash<const char *>()(inKey.m_SignalName);
    }
};
}

namespace {
typedef eastl::hash_map<SSignalSystemKey, TSignalListenerPtrList> TSignalHandlerMap;

struct SSignalSystemImpl : public ISignalSystemImpl
{
    TSignalHandlerMap m_SignalHandlers;
    shared_ptr<IStringTable> m_StringTable;
    SSignalSystemImpl(shared_ptr<IStringTable> inStrTable)
        : m_StringTable(inStrTable)
    {
    }

    TSignalListenerPtrList *FindListenerList(const SSignalSystemKey &inKey)
    {
        TSignalHandlerMap::iterator theIter = m_SignalHandlers.find(inKey);
        if (theIter != m_SignalHandlers.end())
            return &theIter->second;
        return NULL;
    }

    void Signal(void *inSender, const char *inName, const char *inData, size_t inDataSize)
    {
        TSignalListenerPtrList *theSignals =
            FindListenerList(SSignalSystemKey(inSender, inName, *m_StringTable));
        if (theSignals) {
            for (eastl_size_t idx = 0, end = theSignals->size(); idx < end; ++idx) {
                (*theSignals)[idx]->Signal(inSender, inName, inData, inDataSize);
            }
        }
    }

    void RemoveListener(SSignalListener &inListener) override
    {
        TSignalListenerPtrList *theSignals = FindListenerList(inListener.m_Key);
        if (theSignals) {
            // If we could allow unordered access then this could potentially be a lot faster.
            TSignalListenerPtrList::iterator theFind =
                eastl::find(theSignals->begin(), theSignals->end(), &inListener);
            if (theFind != theSignals->end())
                theSignals->erase(theFind);
        }
    }
};

struct SSignalSystem : public ISignalSystem
{
    shared_ptr<SSignalSystemImpl> m_System;
    SSignalSystem(shared_ptr<IStringTable> inStrTable)
    {
        m_System = std::make_shared<SSignalSystemImpl>(inStrTable);
    }

    shared_ptr<ISignalConnection> AddListener(void *inSender, const char *inName,
                                                      TGenericSignalHandlerFunc inFunc) override
    {
        SSignalSystemKey theKey(inSender, inName, *m_System->m_StringTable);
        TSignalHandlerMap::iterator theIter =
            m_System->m_SignalHandlers.insert(eastl::make_pair(theKey, TSignalListenerPtrList()))
                .first;
        TSignalListenerPtrList &theList(theIter->second);
        shared_ptr<SSignalListener> theSender =
            std::make_shared<SSignalListener>(m_System, inFunc, theKey);
        theList.push_back(theSender.get());
        return theSender;
    }

    void Signal(void *inSender, const char *inName, const char *inData, size_t inDataSize) override
    {
        m_System->Signal(inSender, inName, inData, inDataSize);
    }
};
}

shared_ptr<ISignalSystem> ISignalSystem::CreateSignalSystem(shared_ptr<IStringTable> inStrTable)
{
    return std::make_shared<SSignalSystem>(inStrTable);
}
