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
#pragma once
#ifndef QT3DS_RENDER_SERIALIZATION_TYPES_H
#define QT3DS_RENDER_SERIALIZATION_TYPES_H
#include "EASTL/hash_map.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "foundation/Qt3DSContainers.h"

namespace qt3ds {
namespace foundation {

    struct SStrRemapMap : public nvhash_map<char8_t *, QT3DSU32>
    {
        typedef nvhash_map<char8_t *, QT3DSU32> TBaseType;
        SStrRemapMap(NVAllocatorCallback &inAlloc, const char *inName)
            : TBaseType(inAlloc, inName)
        {
        }
    };

    struct SPtrOffsetMap : public nvhash_map<const void *, QT3DSU32>
    {
        typedef nvhash_map<const void *, QT3DSU32> TBaseType;
        SPtrOffsetMap(NVAllocatorCallback &inAlloc, const char *inName)
            : TBaseType(inAlloc, inName)
        {
        }
    };

    struct SWriteBuffer : public MemoryBuffer<>
    {
        SWriteBuffer(NVAllocatorCallback &inAlloc, const char *inName)
            : MemoryBuffer<>(ForwardingAllocator(inAlloc, inName))
        {
        }
    };

    // Simple data reader that mimics a string
    struct SDataReader
    {
        QT3DSU8 *m_CurrentPtr;
        QT3DSU8 *m_EndPtr;
        SDataReader(QT3DSU8 *inStartPtr, QT3DSU8 *inEndPtr)
            : m_CurrentPtr(inStartPtr)
            , m_EndPtr(inEndPtr)
        {
        }

        template <typename TDataType>
        TDataType *Load()
        {
            if ((m_CurrentPtr + sizeof(TDataType)) > m_EndPtr) {
                QT3DS_ASSERT(false);
                return NULL;
            }
            TDataType *theType = reinterpret_cast<TDataType *>(m_CurrentPtr);
            m_CurrentPtr += sizeof(TDataType);
            return theType;
        }

        template <typename TDataType>
        void MemCopy(TDataType *dt, QT3DSU32 count)
        {
            if (count) {
                QT3DSU32 loadSize = count * sizeof(TDataType);
                QT3DSU32 amountLeft = (QT3DSU32)(m_EndPtr - m_CurrentPtr);
                QT3DSU32 amountLoaded = NVMin(loadSize, amountLeft);
                memCopy(dt, m_CurrentPtr, amountLoaded);
                m_CurrentPtr += amountLoaded;
                if (amountLoaded < loadSize) {
                    QT3DS_ASSERT(false);
                    QT3DSU8 *rawPtr = (QT3DSU8 *)dt;
                    rawPtr += amountLoaded;
                    size_t leftover = loadSize - amountLoaded;
                    // zeroing things out at least is an attempt to provide ordered
                    // data that may prevent a crash in some cases.
                    memZero(rawPtr, (QT3DSU32)leftover);
                }
            }
        }

        // Don't make a habit of using this a lot.
        template <typename TDataType>
        TDataType &LoadRef()
        {
            TDataType *retval = Load<TDataType>();
            if (!retval)
                QT3DS_ASSERT(false);
            return *retval;
        }

        void Align(size_t alignment)
        {
            size_t offset = (size_t)(m_CurrentPtr);
            if (offset % alignment)
                m_CurrentPtr += (alignment - (offset % alignment));
        }

        void Align() { Align(sizeof(void *)); }
    };
}
}
#endif