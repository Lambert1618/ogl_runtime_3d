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

#ifndef EVENT_POLLER_H
#define EVENT_POLLER_H

#include <EASTL/vector.h>

#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSContainers.h"

#include "EventSystem.h"
#include "EventPollingSystem.h"

namespace qt3ds {
class NVFoundationBase;
}

namespace qt3ds {
namespace evt {

    class CFactory;

    class CPoller : public IEventSystem
    {
    public:
        typedef IEventProvider *TProvider;
        typedef qt3ds::foundation::nvvector<eastl::pair<TProvider, bool>> TProviderList;
        typedef qt3ds::foundation::nvvector<Qt3DSEventSystemEvent *> TEventList;
        typedef eastl::pair<void *, TProvider> TCEventProviderPair;
        typedef eastl::vector<TCEventProviderPair> TCEventProviderList;

    private:
        explicit CPoller(qt3ds::NVFoundationBase &inFoundation);
        virtual ~CPoller();

    public:
        // Providers once added are released if the poller goes out of scope.
        void AddProvider(IEventProvider &inProvider) override;
        void RemoveProvider(IEventProvider &inProvider) override;
        bool IgnoreProvider(IEventProvider &inProvider) override;
        bool ActivateProvider(IEventProvider &inProvider) override;
        size_t GetNextEvents(Qt3DSEventSystemEvent **outBuffer, size_t bufLen) override;
        IEventFactory &GetEventFactory() override;
        Qt3DSEventSystemEventPoller *GetCInterface() override;
        bool GetAndClearEventFetchedFlag() override;
        void PurgeEvents() override;

        void ReleaseEvents();

        void addRef() override;
        void release() override;

        TProvider GetEventProviderWrapper(Qt3DSEventSystemEventProvider inProvider);
        TProvider GetOrCreateEventProviderWrapper(Qt3DSEventSystemEventProvider inProvider);
        void ReleaseEventProviderWrapper(Qt3DSEventSystemEventProvider inProvider);

        static CPoller &Create(qt3ds::NVFoundationBase &inFoundation);

    private:
        Qt3DSEventSystemEvent *GetNextEvent(bool inAllowResetBuffer /* = true*/);

        qt3ds::QT3DSI32 mRefCount;
        qt3ds::NVFoundationBase &m_Foundation;
        CFactory &m_EventFactory;
        TProviderList m_Providers;
        TEventList m_EventList;
        qt3ds::QT3DSU32 m_EventIndex;
        Qt3DSEventSystemEventPoller m_CInterface;
        TCEventProviderList m_CEventProviders;
        bool m_EventFetched;
    };
}
}

#endif // EVENT_POLLER_H
