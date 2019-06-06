/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QT3DS_FOUNDATION_OPTION_H
#define QT3DS_FOUNDATION_OPTION_H

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"

namespace qt3ds {
namespace foundation {

    struct Empty
    {
    };

    template <typename TDataType>
    class Option
    {
        TDataType mData;
        bool mHasValue;

    public:
        Option(const TDataType &data)
            : mData(data)
            , mHasValue(true)
        {
        }
        Option(const Empty &)
            : mHasValue(false)
        {
        }
        Option()
            : mHasValue(false)
        {
        }
        Option(const Option &other)
            : mData(other.mData)
            , mHasValue(other.mHasValue)
        {
        }
        Option &operator=(const Option &other)
        {
            mData = other.mData;
            mHasValue = other.mHasValue;
            return *this;
        }

        bool isEmpty() const { return !mHasValue; }
        void setEmpty() { mHasValue = false; }
        bool hasValue() const { return mHasValue; }

        const TDataType &getValue() const
        {
            QT3DS_ASSERT(mHasValue);
            return mData;
        }
        TDataType &getValue()
        {
            QT3DS_ASSERT(mHasValue);
            return mData;
        }
        TDataType &unsafeGetValue() { return mData; }

        operator const TDataType &() const { return getValue(); }
        operator TDataType &() { return getValue(); }

        const TDataType *operator->() const { return &getValue(); }
        TDataType *operator->() { return &getValue(); }

        const TDataType &operator*() const { return getValue(); }
        TDataType &operator*() { return getValue(); }
    };
}
}

#endif