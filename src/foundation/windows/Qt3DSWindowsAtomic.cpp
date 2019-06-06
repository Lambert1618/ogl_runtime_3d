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

#include "foundation/windows/Qt3DSWindowsInclude.h"
#include "foundation/Qt3DSAtomic.h"

namespace qt3ds {
namespace foundation {

    QT3DSI32 atomicExchange(volatile QT3DSI32 *val, QT3DSI32 val2)
    {
        return (QT3DSI32)InterlockedExchange((volatile LONG *)val, (LONG)val2);
    }

    QT3DSI32 atomicCompareExchange(volatile QT3DSI32 *dest, QT3DSI32 exch, QT3DSI32 comp)
    {
        return (QT3DSI32)InterlockedCompareExchange((volatile LONG *)dest, exch, comp);
    }

    void *atomicCompareExchangePointer(volatile void **dest, void *exch, void *comp)
    {
        return InterlockedCompareExchangePointer((volatile PVOID *)dest, exch, comp);
    }

    QT3DSI32 atomicIncrement(volatile QT3DSI32 *val)
    {
        return (QT3DSI32)InterlockedIncrement((volatile LONG *)val);
    }

    QT3DSI32 atomicDecrement(volatile QT3DSI32 *val)
    {
        return (QT3DSI32)InterlockedDecrement((volatile LONG *)val);
    }

    QT3DSI32 atomicAdd(volatile QT3DSI32 *val, QT3DSI32 delta)
    {
        LONG newValue, oldValue;
        do {
            oldValue = *val;
            newValue = oldValue + delta;
        } while (InterlockedCompareExchange((volatile LONG *)val, newValue, oldValue) != oldValue);

        return newValue;
    }

    QT3DSI32 atomicMax(volatile QT3DSI32 *val, QT3DSI32 val2)
    {
        // Could do this more efficiently in asm...

        LONG newValue, oldValue;

        do {
            oldValue = *val;

            if (val2 > oldValue)
                newValue = val2;
            else
                newValue = oldValue;

        } while (InterlockedCompareExchange((volatile LONG *)val, newValue, oldValue) != oldValue);

        return newValue;
    }

} // namespace foundation
} // namespace qt3ds
