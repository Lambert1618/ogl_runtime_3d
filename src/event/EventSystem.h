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

#ifndef MESSAGE_SYSTEM_H
#define MESSAGE_SYSTEM_H
#include <cstdlib>
#include "EventSystemC.h"

namespace qt3ds {
namespace evt {

    typedef Qt3DSEventSystemStr TEventStr;

    // Factory from the provider's perspective.
    class IEventFactory
    {
    protected:
        virtual ~IEventFactory() {}
    public:
        virtual Qt3DSEventSystemEvent &CreateEvent(int numData) = 0;
        // Guaranteed to be at least 4K - sizeof(Qt3DSEventSystemEvent)
        virtual size_t GetMaxNumEventData() = 0;
        // Guaranteed to be at least 4K
        virtual size_t GetMaxStrLength() = 0;

        virtual Qt3DSEventSystemRegisteredStr RegisterStr(TEventStr inSrc) = 0;
        // Null terminates if strlen(inSrc) > getMaxStrLength
        virtual TEventStr AllocateStr(TEventStr inSrc) = 0;
        // Returns null if inLength > getMaxStrLength
        virtual TEventStr AllocateStr(int inLength) = 0;
    };

    class IEventProvider
    {
    protected:
        virtual ~IEventProvider() {}
    public:
        // Return the next N events, placed into the event buffer.
        virtual size_t GetNextEvents(IEventFactory &inFactory, Qt3DSEventSystemEvent **outBuffer,
                                     size_t bufLen) = 0;
        virtual void Release() = 0;
    };
}
}

#endif
