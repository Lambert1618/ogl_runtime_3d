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

#ifndef QT3DS_FOUNDATION_PSUTILITIES_H
#define QT3DS_FOUNDATION_PSUTILITIES_H

#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSIntrinsics.h"

namespace qt3ds {
namespace foundation {

    // PT: checked casts
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSU16 to16(QT3DSU32 value)
    {
        QT3DS_ASSERT(value <= 0xffff);
        return QT3DSU16(value);
    }
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSU8 to8(QT3DSU16 value)
    {
        QT3DS_ASSERT(value <= 0xff);
        return QT3DSU8(value);
    }
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSU8 to8(QT3DSU32 value)
    {
        QT3DS_ASSERT(value <= 0xff);
        return QT3DSU8(value);
    }

    template <class T>
    QT3DS_CUDA_CALLABLE QT3DS_INLINE void swap(T &x, T &y)
    {
// crash with optimizations enabled, ticket in Sony
#ifdef QT3DS_PSP2
#pragma control % push O = 0
#endif
        T tmp = x;
        x = y;
        y = tmp;
#ifdef QT3DS_PSP2
#pragma control % pop O
#endif
    }

    /*!
Get number of elements in array
*/
    template <typename T, size_t N>
    char (&ArraySizeHelper(T (&array)[N]))[N];
#define QT3DS_ARRAY_SIZE(_array) (sizeof(qt3ds::foundation::ArraySizeHelper(_array)))

    /*!
Sort two elements using operator<

On return x will be the smaller of the two
*/
    template <class T>
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void order(T &x, T &y)
    {
        if (y < x)
            swap(x, y);
    }

    // most architectures can do predication on real comparisons, and on VMX, it matters

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void order(NVReal &x, NVReal &y)
    {
        NVReal newX = NVMin(x, y);
        NVReal newY = NVMax(x, y);
        x = newX;
        y = newY;
    }

    /*!
        Sort two elements using operator< and also keep order
        of any extra data
        */
    template <class T, class E1>
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void order(T &x, T &y, E1 &xe1, E1 &ye1)
    {
        if (y < x) {
            swap(x, y);
            swap(xe1, ye1);
        }
    }

    QT3DS_INLINE void debugBreak()
    {
#if defined(QT3DS_WINDOWS)
        __debugbreak();
#elif defined(QT3DS_LINUX) || defined(QT3DS_ANDROID) || defined(QT3DS_QNX)
        asm("int $3");
#elif defined(QT3DS_GNUC)
        __builtin_trap();
#else
        QT3DS_ASSERT(false);
#endif
    }

    bool checkValid(const float &);
    bool checkValid(const QT3DSVec3 &);
    bool checkValid(const QT3DSQuat &);
    bool checkValid(const QT3DSMat33 &);
    bool checkValid(const NVTransform &);
    bool checkValid(const char *);

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSI32 getPadding2(size_t value, QT3DSU32 alignment)
    {
        const QT3DSI32 mask = alignment - 1;
        const QT3DSI32 overhead = QT3DSI32(value) & mask;
        return (alignment - overhead) & mask;
    }

    // PT: "After doing a dcbz128, there is a delay of about 140 cycles before writes to that cache
    // line can proceed without stalling.
    // This is much faster than an L2 cache miss, but for ideal performance, it is best to avoid
    // this stall by doing the cache-line
    // zeroing a few cache lines ahead of where you are writing."
    QT3DS_FORCE_INLINE void invalidateCache(void *QT3DS_RESTRICT voidPtr, QT3DSI32 size)
    {
#ifdef QT3DS_X360
        QT3DSU8 *QT3DS_RESTRICT ptr = reinterpret_cast<QT3DSU8 *>(voidPtr);
        const QT3DSI32 padding = getPadding2(size_t(ptr), 128);
        const QT3DSI32 sizeToCover = size - padding;
        if (sizeToCover >= 128) {
            QT3DSU8 *ptr128 = ptr + padding;
            QT3DSU32 nb128 = sizeToCover / 128;
            while (nb128--) {
                //				NV::memZero128(ptr128);
                qt3ds::foundation::memZero128(ptr128);
                ptr128 += 128;
            }
        }
#else
        (void)voidPtr;
        (void)size;
#endif
    }

    // equivalent to std::max_element
    template <typename T>
    inline const T *maxElement(const T *first, const T *last)
    {
        const T *m = first;
        for (const T *it = first + 1; it < last; ++it)
            if (*m < *it)
                m = it;

        return m;
    }

} // namespace foundation
} // namespace qt3ds

#define QT3DS_STRINGIZE_HELPER(X) #X
#define QT3DS_STRINGIZE(X) QT3DS_STRINGIZE_HELPER(X)

#define QT3DS_CONCAT_HELPER(X, Y) X##Y
#define QT3DS_CONCAT(X, Y) QT3DS_CONCAT_HELPER(X, Y)

#endif
