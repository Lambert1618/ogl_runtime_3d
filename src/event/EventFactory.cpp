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

#include "EventFactory.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/StringTable.h"
#include "foundation/Utils.h"

using namespace qt3ds::evt;

typedef char TEventChar;
using qt3ds::QT3DSU32;

class CInitableRegisteredStr : public Qt3DSEventSystemRegisteredStr
{
public:
    CInitableRegisteredStr(const TEventStr inString) { m_Data = inString; }
};

CFactory &CFactory::Create(qt3ds::NVFoundationBase &inFoundation)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), CFactory)(inFoundation);
}

CFactory::CFactory(qt3ds::NVFoundationBase &inFoundation)
    : m_Foundation(inFoundation)
    , m_Allocator(inFoundation.getAllocator(), "event factory allocator")
    , m_StringTable(qt3ds::foundation::IStringTable::CreateStringTable(inFoundation.getAllocator()))
{
}

CFactory::~CFactory()
{
}

Qt3DSEventSystemEvent &CFactory::CreateEvent(int inNumData)
{
    // Ensure num data is in range
    inNumData = qt3ds::NVMax(0, inNumData);
    // Ensure we can actually allocate something that big
    QT3DSU32 dataSize = static_cast<QT3DSU32>(
        qt3ds::NVMin((size_t)inNumData * sizeof(Qt3DSEventSystemEventData),
                  TEventAllocator::getSlabSize() - sizeof(Qt3DSEventSystemEvent)));
    // get the actual num data after safety checks.
    inNumData = (int)(dataSize / sizeof(Qt3DSEventSystemEventData));

    QT3DSU32 allocSize = sizeof(Qt3DSEventSystemEvent) + inNumData * sizeof(Qt3DSEventSystemEventData);

    Qt3DSEventSystemEvent *theEvent = (Qt3DSEventSystemEvent *)m_Allocator.allocate(
        allocSize, "Event allocation", __FILE__, __LINE__);
    theEvent->m_NumData = inNumData;
    theEvent->m_Data = reinterpret_cast<Qt3DSEventSystemEventData *>(((qt3ds::QT3DSU8 *)theEvent)
                                                                    + sizeof(Qt3DSEventSystemEvent));

    // Initialize the event data to zero so that a free event won't cause a calamity.
    qt3ds::intrinsics::memZero(theEvent->m_Data, dataSize);

    return *theEvent;
}

size_t CFactory::GetMaxNumEventData()
{
    return m_Allocator.getSlabSize() - sizeof(Qt3DSEventSystemEvent);
}

size_t CFactory::GetMaxStrLength()
{
    return m_Allocator.getSlabSize();
}

void CFactory::ReleaseOutstandingEvents()
{
    m_Allocator.reset();
}

Qt3DSEventSystemRegisteredStr CFactory::RegisterStr(TEventStr inSrc)
{
    return CInitableRegisteredStr(m_StringTable->RegisterStr(inSrc));
}

TEventStr CFactory::AllocateStr(TEventStr inSrc)
{
    size_t theSizeInByte = sizeof(TEventChar) * (strlen(qt3ds::foundation::nonNull(inSrc)) + 1);
    theSizeInByte = qt3ds::NVMin(theSizeInByte, TEventAllocator::getSlabSize());

    TEventChar *theNewString = (TEventChar *)m_Allocator.allocate(
        theSizeInByte, "Qt3DS::evt::CFactory::UnManagedString", __FILE__, __LINE__);

    if (theNewString)
        memcpy(theNewString, inSrc, theSizeInByte);

    theNewString[theSizeInByte - 1] = 0;

    return theNewString;
}

TEventStr CFactory::AllocateStr(int inLength)
{
    ++inLength;
    int safeLen = (int)qt3ds::NVMin((size_t)inLength, TEventAllocator::getSlabSize());
    // Either give the users what they ask for or return nothing.
    if (safeLen != inLength)
        return NULL;

    void *retval = m_Allocator.allocate(inLength * sizeof(TEventChar),
                                        "Qt3DS::evt::CFactory::UnManagedString", __FILE__, __LINE__);
    qt3ds::intrinsics::memZero(retval, inLength);
    return (TEventStr)retval;
}

void CFactory::Release()
{
    this->~CFactory();
    QT3DS_FREE(m_Foundation.getAllocator(), this);
}
