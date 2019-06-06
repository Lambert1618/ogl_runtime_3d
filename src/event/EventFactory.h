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

#ifndef EVENT_FACTORY_H
#define EVENT_FACTORY_H

#include "EventSystem.h"
#include "EventPollingSystem.h"

#include "foundation/Qt3DSRefCounted.h"
#include "foundation/FastAllocator.h"

namespace qt3ds {
class NVFoundationBase;
namespace foundation {
    class IStringTable;
}
}

namespace qt3ds {
namespace evt {

    class CEventProviderRefCounted;

    class CFactory : public IEventFactory
    {
    private:
        explicit CFactory(qt3ds::NVFoundationBase &inFoundation);
        virtual ~CFactory();

        CFactory &operator=(const CFactory &) { return *this; }
    public:
        // Interfaces from IEventFactory
        Qt3DSEventSystemEvent &CreateEvent(int inNumData) override;
        size_t GetMaxNumEventData() override;
        size_t GetMaxStrLength() override;

        Qt3DSEventSystemRegisteredStr RegisterStr(TEventStr inSrc) override;
        // Null terminates if strlen(inSrc) > getMaxStrLength
        TEventStr AllocateStr(TEventStr inSrc) override;

        // Returns null if inLength > getMaxStrLength
        TEventStr AllocateStr(int inLength) override;

        void ReleaseOutstandingEvents();
        void Release();

        static CFactory &Create(qt3ds::NVFoundationBase &inFoundation);

    private:
        typedef qt3ds::foundation::SFastAllocator<> TEventAllocator;

        qt3ds::NVFoundationBase &m_Foundation;
        TEventAllocator m_Allocator;
        qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> m_StringTable;
    };
}
}

#endif // EVENT_FACTORY_H
