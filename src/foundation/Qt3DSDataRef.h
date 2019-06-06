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

#ifndef QT3DS_FOUNDATION_DATA_REF_H
#define QT3DS_FOUNDATION_DATA_REF_H

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"

namespace qt3ds {
namespace foundation {

    template <typename TDataType>
    struct NVConstDataRef
    {
        const TDataType *mData;
        QT3DSU32 mSize;

        NVConstDataRef(const TDataType *inData, QT3DSU32 inSize)
            : mData(inData)
            , mSize(inSize)
        {
        }
        NVConstDataRef()
            : mData(0)
            , mSize(0)
        {
        }

        QT3DSU32 size() const { return mSize; }

        const TDataType *begin() const { return mData; }
        const TDataType *end() const { return mData + mSize; }

        const TDataType &operator[](QT3DSU32 index) const
        {
            QT3DS_ASSERT(index < mSize);
            return mData[index];
        }
    };

    template <typename TDataType>
    inline NVConstDataRef<TDataType> toConstDataRef(const TDataType &type)
    {
        return NVConstDataRef<TDataType>(&type, 1);
    }

    template <typename TDataType>
    inline NVConstDataRef<QT3DSU8> toU8ConstDataRef(const TDataType &type)
    {
        return NVConstDataRef<QT3DSU8>(reinterpret_cast<const QT3DSU8 *>(&type), sizeof(TDataType));
    }

    template <typename TDataType>
    inline NVConstDataRef<TDataType> toConstDataRef(const TDataType *type, QT3DSU32 count)
    {
        return NVConstDataRef<TDataType>(type, count);
    }

    template <typename TDataType>
    inline NVConstDataRef<QT3DSU8> toU8ConstDataRef(const TDataType *type, QT3DSU32 count)
    {
        return NVConstDataRef<QT3DSU8>(reinterpret_cast<const QT3DSU8 *>(type),
                                    sizeof(TDataType) * count);
    }

    template <typename TDataType>
    struct NVDataRef
    {
        TDataType *mData;
        QT3DSU32 mSize;

        NVDataRef(TDataType *inData, QT3DSU32 inSize)
            : mData(inData)
            , mSize(inSize)
        {
        }
        NVDataRef()
            : mData(0)
            , mSize(0)
        {
        }
        QT3DSU32 size() const { return mSize; }

        TDataType *begin() { return mData; }
        TDataType *end() { return mData + mSize; }

        TDataType *begin() const { return mData; }
        TDataType *end() const { return mData + mSize; }

        TDataType &operator[](QT3DSU32 index)
        {
            QT3DS_ASSERT(index < mSize);
            return mData[index];
        }

        const TDataType &operator[](QT3DSU32 index) const
        {
            QT3DS_ASSERT(index < mSize);
            return mData[index];
        }

        operator NVConstDataRef<TDataType>() const
        {
            return NVConstDataRef<TDataType>(mData, mSize);
        }
    };

    template <typename TDataType>
    inline NVDataRef<TDataType> toDataRef(TDataType &type)
    {
        return NVDataRef<TDataType>(&type, 1);
    }

    template <typename TDataType>
    inline NVDataRef<QT3DSU8> toU8DataRef(TDataType &type)
    {
        return NVDataRef<QT3DSU8>(reinterpret_cast<QT3DSU8 *>(&type), sizeof(TDataType));
    }

    template <typename TDataType>
    inline NVDataRef<TDataType> toDataRef(TDataType *type, QT3DSU32 count)
    {
        return NVDataRef<TDataType>(type, count);
    }

    template <typename TDataType>
    inline NVDataRef<QT3DSU8> toU8DataRef(TDataType *type, QT3DSU32 count)
    {
        return NVDataRef<QT3DSU8>(reinterpret_cast<QT3DSU8 *>(type), sizeof(TDataType) * count);
    }
}
}

#endif