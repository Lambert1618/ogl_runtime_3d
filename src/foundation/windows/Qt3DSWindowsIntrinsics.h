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

#ifndef QT3DS_FOUNDATION_QT3DS_WINDOWS_INTRINSICS_H
#define QT3DS_FOUNDATION_QT3DS_WINDOWS_INTRINSICS_H

#include "foundation/Qt3DS.h"

#if !defined QT3DS_WINDOWS && !defined QT3DS_WIN8ARM
#error "This file should only be included by Windows builds!!"
#endif

#ifdef QT3DS_GNUC
#include <cmath>
#else
#include <math.h>
#endif
#include <float.h>
#include <intrin.h>
#include <string.h>
#include "foundation/Qt3DSAssert.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
namespace intrinsics {
#endif

    //! \brief platform-specific absolute value
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float abs(float a) { return float(::fabs(a)); }

    //! \brief platform-specific select float
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float fsel(float a, float b, float c)
    {
        return (a >= 0.0f) ? b : c;
    }

    //! \brief platform-specific sign
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float sign(float a) { return (a >= 0.0f) ? 1.0f : -1.0f; }

    //! \brief platform-specific reciprocal
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float recip(float a) { return 1.0f / a; }

    //! \brief platform-specific square root
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float sqrt(float a) { return ::sqrtf(a); }

    //! \brief platform-specific reciprocal square root
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float recipSqrt(float a) { return 1.0f / ::sqrtf(a); }

    //! \brief platform-specific sine
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float sin(float a) { return ::sinf(a); }

    //! \brief platform-specific cosine
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float cos(float a) { return ::cosf(a); }

    //! \brief platform-specific minimum
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float selectMin(float a, float b) { return a < b ? a : b; }

    //! \brief platform-specific maximum
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float selectMax(float a, float b) { return a > b ? a : b; }

    //! \brief platform-specific finiteness check (not INF or NAN)
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isFinite(float a)
    {
#if defined(__CUDACC__)
        return isfinite(a)? true : false;
#elif defined(QT3DS_GNUC)
        return (std::isfinite(a) && !std::isinf(a)) ? true : false;
#else
    return (0 == ((_FPCLASS_SNAN | _FPCLASS_QNAN | _FPCLASS_NINF | _FPCLASS_PINF) & _fpclass(a)));
#endif
    }

    //! \brief platform-specific finiteness check (not INF or NAN)
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool isFinite(double a)
    {
#if defined(__CUDACC__)
        return isfinite(a)? true : false;
#elif defined(QT3DS_GNUC)
        return (std::isfinite(a) && !std::isinf(a)) ? true : false;
#else
    return (0 == ((_FPCLASS_SNAN | _FPCLASS_QNAN | _FPCLASS_NINF | _FPCLASS_PINF) & _fpclass(a)));
#endif
    }
    /*
* Implements a memory barrier
*/
    QT3DS_FORCE_INLINE void memoryBarrier()
    {
        _ReadWriteBarrier();
        /* long Barrier;
        __asm {
                xchg Barrier, eax
        }*/
    }

    /*!
	Returns the index of the highest set bit. Not valid for zero arg.
	*/
    QT3DS_FORCE_INLINE QT3DSU32 highestSetBitUnsafe(QT3DSU32 v)
    {
        unsigned long retval;
        _BitScanReverse(&retval, v);
        return retval;
    }

    /*!
	Returns the index of the highest set bit. Undefined for zero arg.
	*/
    QT3DS_FORCE_INLINE QT3DSU32 lowestSetBitUnsafe(QT3DSU32 v)
    {
        unsigned long retval;
        _BitScanForward(&retval, v);
        return retval;
    }

    /*!
	Returns the number of leading zeros in v. Returns 32 for v=0.
	*/
    QT3DS_FORCE_INLINE QT3DSU32 countLeadingZeros(QT3DSU32 v)
    {
        if (v) {
            unsigned long bsr = (unsigned long)-1;
            _BitScanReverse(&bsr, v);
            return 31 - bsr;
        } else
            return 32;
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
    QT3DS_FORCE_INLINE void prefetch128(const void *ptr, QT3DSU32 offset = 0)
    {
#ifdef QT3DS_WINDOWS
        _mm_prefetch(((const char *)ptr + offset), _MM_HINT_T0);
#endif
    }

    /*!
	Prefetch \c count bytes starting at \c ptr.
	*/
    QT3DS_FORCE_INLINE void prefetch(const void *ptr, QT3DSU32 count = 0)
    {
        for (QT3DSU32 i = 0; i <= count; i += 128)
            prefetch128(ptr, i);
    }

    //! \brief platform-specific reciprocal
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float recipFast(float a) { return 1.0f / a; }

    //! \brief platform-specific fast reciprocal square root
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float recipSqrtFast(float a) { return 1.0f / ::sqrtf(a); }

    //! \brief platform-specific floor
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE float floatFloor(float x) { return ::floorf(x); }

#ifndef QT3DS_DOXYGEN
} // namespace intrinsics
} // namespace qt3ds
#endif

#endif
