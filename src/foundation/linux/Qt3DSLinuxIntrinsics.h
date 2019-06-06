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

#ifndef QT3DS_FOUNDATION_QT3DS_LINUX_INTRINSICS_H
#define QT3DS_FOUNDATION_QT3DS_LINUX_INTRINSICS_H

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"

#if !(defined QT3DS_LINUX || defined QT3DS_ANDROID || defined QT3DS_APPLE || defined QT3DS_QNX)
#error "This file should only be included by Linux builds!!"
#endif

#include <cmath>
#include <math.h>
#include <float.h>
#include <string.h>

namespace qt3ds {
namespace intrinsics {
    //! \brief platform-specific absolute value
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float abs(float a) { return ::fabs(a); }

    //! \brief platform-specific select float
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float fsel(float a, float b, float c)
    {
        return (a >= 0.0f) ? b : c;
    }

    //! \brief platform-specific sign
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float sign(float a) { return (a >= 0.0f) ? 1.0f : -1.0f; }

    //! \brief platform-specific reciprocal
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float recip(float a) { return 1.0f / a; }

    //! \brief platform-specific reciprocal estimate
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float recipFast(float a) { return 1.0f / a; }

    //! \brief platform-specific square root
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float sqrt(float a) { return ::sqrtf(a); }

    //! \brief platform-specific reciprocal square root
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float recipSqrt(float a) { return 1.0f / ::sqrtf(a); }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float recipSqrtFast(float a) { return 1.0f / ::sqrtf(a); }

    //! \brief platform-specific sine
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float sin(float a) { return ::sinf(a); }

    //! \brief platform-specific cosine
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float cos(float a) { return ::cosf(a); }

    //! \brief platform-specific minimum
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float selectMin(float a, float b) { return a < b ? a : b; }

    //! \brief platform-specific maximum
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float selectMax(float a, float b) { return a > b ? a : b; }

    //! \brief platform-specific finiteness check (not INF or NAN)
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isFinite(float a) { return std::isfinite(a); }

    //! \brief platform-specific finiteness check (not INF or NAN)
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isFinite(double a) { return std::isfinite(a); }
    QT3DS_FORCE_INLINE void memoryBarrier()
    {
#if 0 //!defined (QT3DS_ARM)
		smp_mb();
#endif
    }

    /*!
	Return the index of the highest set bit. Undefined for zero arg.
	*/
    QT3DS_INLINE QT3DSU32 highestSetBitUnsafe(QT3DSU32 v)
    {

        // http://graphics.stanford.edu/~seander/bithacks.html
        static const QT3DSU32 MultiplyDeBruijnBitPosition[32] = { 0,  9,  1,  10, 13, 21, 2,  29,
                                                               11, 14, 16, 18, 22, 25, 3,  30,
                                                               8,  12, 20, 28, 15, 17, 24, 7,
                                                               19, 27, 23, 6,  26, 5,  4,  31 };

        v |= v >> 1; // first round up to one less than a power of 2
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;

        return MultiplyDeBruijnBitPosition[(QT3DSU32)(v * 0x07C4ACDDU) >> 27];
    }

    /*!
	Return the index of the highest set bit. Undefined for zero arg.
	*/
    QT3DS_INLINE QT3DSI32 lowestSetBitUnsafe(QT3DSU32 v)
    {
        // http://graphics.stanford.edu/~seander/bithacks.html
        static const QT3DSU32 MultiplyDeBruijnBitPosition[32] = { 0,  1,  28, 2,  29, 14, 24, 3,
                                                               30, 22, 20, 15, 25, 17, 4,  8,
                                                               31, 27, 13, 23, 21, 19, 16, 7,
                                                               26, 12, 18, 6,  11, 5,  10, 9 };
        QT3DSI32 w = v;
        return MultiplyDeBruijnBitPosition[(QT3DSU32)((w & -w) * 0x077CB531U) >> 27];
    }

    /*!
	Returns the index of the highest set bit. Undefined for zero arg.
	*/
    QT3DS_INLINE QT3DSU32 countLeadingZeros(QT3DSU32 v)
    {
        QT3DSI32 result = 0;
#ifdef _INTEGRITYPLATFORM
        QT3DSU32 testBit = (1U << 31);
#else
        QT3DSU32 testBit = (1 << 31);
#endif
        while ((v & testBit) == 0 && testBit != 0)
            result++, testBit >>= 1;
        return result;
    }

    /*!
	Sets \c count bytes starting at \c dst to zero.
	*/
    QT3DS_FORCE_INLINE void *memZero(void *QT3DS_RESTRICT dest, QT3DSU32 count)
    {
        return memset(dest, 0, count);
    }

    /*!
	Sets \c count bytes starting at \c dst to \c c.
	*/
    QT3DS_FORCE_INLINE void *memSet(void *QT3DS_RESTRICT dest, QT3DSI32 c, QT3DSU32 count)
    {
        return memset(dest, c, count);
    }

    /*!
	Copies \c count bytes from \c src to \c dst. User memMove if regions overlap.
	*/
    QT3DS_FORCE_INLINE void *memCopy(void *QT3DS_RESTRICT dest, const void *QT3DS_RESTRICT src, QT3DSU32 count)
    {
        return memcpy(dest, src, count);
    }

    /*!
	Copies \c count bytes from \c src to \c dst. Supports overlapping regions.
	*/
    QT3DS_FORCE_INLINE void *memMove(void *QT3DS_RESTRICT dest, const void *QT3DS_RESTRICT src, QT3DSU32 count)
    {
        return memmove(dest, src, count);
    }

    /*!
	Set 128B to zero starting at \c dst+offset. Must be aligned.
	*/
    QT3DS_FORCE_INLINE void memZero128(void *QT3DS_RESTRICT dest, QT3DSU32 offset = 0)
    {
        QT3DS_ASSERT(((size_t(dest) + offset) & 0x7f) == 0);
        memSet((char *QT3DS_RESTRICT)dest + offset, 0, 128);
    }

    /*!
	Prefetch aligned 128B around \c ptr+offset.
	*/
    QT3DS_FORCE_INLINE void prefetch128(const void *, QT3DSU32 = 0) {}

    /*!
	Prefetch \c count bytes starting at \c ptr.
	*/
    QT3DS_FORCE_INLINE void prefetch(const void *ptr, QT3DSU32 count = 0)
    {
        for (QT3DSU32 i = 0; i <= count; i += 128)
            prefetch128(ptr, i);
    }

    //! \brief platform-specific floor
    QT3DS_FORCE_INLINE float floatFloor(float x) { return ::floorf(x); }
} // namespace intrinsics
} // namespace qt3ds

#endif
