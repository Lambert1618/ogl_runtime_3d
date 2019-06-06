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

#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSRefCounted.h"
#include "EventSystem.h"

namespace qt3ds {
namespace evt {

    class CEventProviderRefCounted;

    class IEventSystem : public qt3ds::foundation::NVRefCounted
    {
    protected:
        virtual ~IEventSystem() {}
    public:
        // Providers once added are released if the poller goes out of scope.
        // Adding the same provider twice has no effect.
        virtual void AddProvider(IEventProvider &inProvider) = 0;

        // removeProvider does not release the provider; callers are expected to release the
        // provider themselves after calling remove provider.  Calling remove provider
        // with a provider that isn't in the system has no effect.
        virtual void RemoveProvider(IEventProvider &inProvider) = 0;

        // Ignore events from this provider.  Returns true if provider is added.
        virtual bool IgnoreProvider(IEventProvider &inProvider) = 0;

        // Listen on this provider.  Returns true if provider is added.
        virtual bool ActivateProvider(IEventProvider &inProvider) = 0;

        // Get a set of events from the poller.  The poller will simply run through and ask each
        // provider
        // for events and then begin returning the set of events.
        // Note: this interface needs to set the event fetched flag.
        virtual size_t GetNextEvents(Qt3DSEventSystemEvent **outBuffer, size_t bufLen) = 0;

        // Get the event factory used for this poller.
        virtual IEventFactory &GetEventFactory() = 0;

        virtual Qt3DSEventSystemEventPoller *GetCInterface() = 0;

        // Get and clear the flag indicates whether GetNextEvents has been called after last call of
        // this interface
        // Note: the event fetched flag is set by GetNextEvents
        virtual bool GetAndClearEventFetchedFlag() = 0;

        // Clear events in event system and events in all providers
        virtual void PurgeEvents() = 0;

        static IEventSystem &Create(qt3ds::NVFoundationBase &inFoundation);
    };
}
}

#endif // EVENT_SYSTEM_H
