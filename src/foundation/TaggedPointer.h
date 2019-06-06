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
#pragma once
#ifndef QT3DS_FOUNDATION_TAGGED_POINTER_H
#define QT3DS_FOUNDATION_TAGGED_POINTER_H
#include "foundation/StringTable.h"

namespace qt3ds {
namespace foundation {

    template <typename TDataType>
    struct SPointerTag
    {
        /* Expected API for runtime RTTI
        static CRegisteredString GetTag() { return g_dtype_specific_string; }
        */
    };

    // A pointer tagged with an identifier so we can have generic
    // user data that is still somewhat typesafe.
    struct STaggedPointer
    {
        void *m_UserData;
        CRegisteredString m_Tag;
        STaggedPointer()
            : m_UserData(NULL)
        {
        }

        STaggedPointer(void *inUserData, CRegisteredString inTag)
            : m_UserData(inUserData)
            , m_Tag(inTag)
        {
        }

        template <typename TDataType>
        STaggedPointer(TDataType *inType)
            : m_UserData(reinterpret_cast<void *>(inType))
            , m_Tag(SPointerTag<TDataType>::GetTag())
        {
        }

        template <typename TDataType>
        TDataType *DynamicCast() const
        {
            if (m_Tag == SPointerTag<TDataType>::GetTag())
                return reinterpret_cast<TDataType *>(m_UserData);
            return NULL;
        }
    };
}
}
#endif