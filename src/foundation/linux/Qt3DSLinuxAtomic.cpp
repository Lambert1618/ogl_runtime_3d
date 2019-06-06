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
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAtomic.h"
#ifdef __QNX__
#include <atomic.h>
#endif
#ifdef __INTEGRITY
#include <QtCore/qmutex.h>
#endif

#define PAUSE() asm("nop")

namespace qt3ds {
namespace foundation {
#ifdef __QNX__
    QT3DSI32 atomicIncrement(volatile QT3DSI32 *val)
    {
        atomic_add((volatile unsigned *)val, 1);
        return *val;
    }

    QT3DSI32 atomicDecrement(volatile QT3DSI32 *val)
    {
        atomic_sub((volatile unsigned *)val, 1);
        return *val;
    }
#elif defined (__INTEGRITY)
    // TestAndSet and AtomicModify take in volatile Address* leading to overflow error if used

    QT3DSI32 atomicCompareExchange(volatile QT3DSI32 *dest, QT3DSI32 exch, QT3DSI32 comp)
    {
        static QMutex mutex;
        QMutexLocker lock(&mutex);
        QT3DSI32 ret = *dest;
        *dest = (*dest == comp) ? exch : ret;
        return ret;
    }

    QT3DSI32 atomicIncrement(volatile QT3DSI32 *val)
    {
        static QMutex mutex;
        QMutexLocker lock(&mutex);
        return ++(*val);
    }

    QT3DSI32 atomicDecrement(volatile QT3DSI32 *val)
    {
        static QMutex mutex;
        QMutexLocker lock(&mutex);
        return --(*val);
    }
#else
    void *atomicCompareExchangePointer(volatile void **dest, void *exch, void *comp)
    {
        return __sync_val_compare_and_swap((void **)dest, comp, exch);
    }

    QT3DSI32 atomicCompareExchange(volatile QT3DSI32 *dest, QT3DSI32 exch, QT3DSI32 comp)
    {
        return __sync_val_compare_and_swap(dest, comp, exch);
    }

    QT3DSI32 atomicIncrement(volatile QT3DSI32 *val) { return __sync_add_and_fetch(val, 1); }

    QT3DSI32 atomicDecrement(volatile QT3DSI32 *val) { return __sync_sub_and_fetch(val, 1); }

    QT3DSI32 atomicAdd(volatile QT3DSI32 *val, QT3DSI32 delta) { return __sync_add_and_fetch(val, delta); }

    QT3DSI32 atomicMax(volatile QT3DSI32 *val, QT3DSI32 val2)
    {
        QT3DSI32 oldVal, newVal;

        do {
            PAUSE();
            oldVal = *val;

            if (val2 > oldVal)
                newVal = val2;
            else
                newVal = oldVal;

        } while (atomicCompareExchange(val, newVal, oldVal) != oldVal);

        return *val;
    }

    QT3DSI32 atomicExchange(volatile QT3DSI32 *val, QT3DSI32 val2)
    {
        QT3DSI32 newVal, oldVal;

        do {
            PAUSE();
            oldVal = *val;
            newVal = val2;
        } while (atomicCompareExchange(val, newVal, oldVal) != oldVal);

        return newVal;
    }
#endif
} // namespace foundation
} // namespace qt3ds
