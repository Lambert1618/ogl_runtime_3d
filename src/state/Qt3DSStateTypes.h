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
#pragma once
#ifndef QT3DS_STATE_TYPES_H
#define QT3DS_STATE_TYPES_H
#include "Qt3DSState.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/StringTable.h"
#include "foundation/TaggedPointer.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Utils.h"

namespace qt3ds {
namespace state {
    // Events, because they created both inside and outside
    // the state system by various parties.
    class IEvent : public NVRefCounted
    {
    protected:
        virtual ~IEvent() {}
    public:
        virtual CRegisteredString GetName() const = 0;
        // Optional type string for rtti.  No string means this is an opaque event
        // that cannot be safely cast to any specific event type.
        virtual CRegisteredString GetEventType() const { return CRegisteredString(); }
    };

    typedef NVScopedRefCounted<IEvent> TEventPtr;

    DEFINE_INVASIVE_SINGLE_LIST(Data);

    struct SDataModel
    {
        CRegisteredString m_Id;
        const char8_t *m_Source;
        const char8_t *m_Expression;
        TDataList m_Data;

        SDataModel()
            : m_Source(nullptr)
            , m_Expression(nullptr)
        {
        }
    };

    struct SData
    {
        CRegisteredString m_Id;
        const char8_t *m_Source;
        const char8_t *m_Expression;
        SData *m_NextSibling;
        SData()
            : m_Source(nullptr)
            , m_Expression(nullptr)
            , m_NextSibling(nullptr)
        {
        }
    };

    IMPLEMENT_INVASIVE_SINGLE_LIST(Data, m_NextSibling);
}
}

#endif
