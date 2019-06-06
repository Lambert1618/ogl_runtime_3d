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

#ifndef QT3DS_FOUNDATION_PSATOMIC_H
#define QT3DS_FOUNDATION_PSATOMIC_H

#include "foundation/Qt3DS.h"

namespace qt3ds {
namespace foundation {
    /* set *dest equal to val. Return the old value of *dest */
    QT3DS_AUTOTEST_EXPORT QT3DSI32 atomicExchange(volatile QT3DSI32 *dest, QT3DSI32 val);

    /* if *dest == comp, replace with exch. Return original value of *dest */
    QT3DS_AUTOTEST_EXPORT QT3DSI32 atomicCompareExchange(volatile QT3DSI32 *dest, QT3DSI32 exch, QT3DSI32 comp);

    /* if *dest == comp, replace with exch. Return original value of *dest */
    QT3DS_AUTOTEST_EXPORT void *atomicCompareExchangePointer(volatile void **dest, void *exch,
                                                         void *comp);

    /* increment the specified location. Return the incremented value */
    QT3DS_AUTOTEST_EXPORT QT3DSI32 atomicIncrement(volatile QT3DSI32 *val);

    /* decrement the specified location. Return the decremented value */
    QT3DS_AUTOTEST_EXPORT QT3DSI32 atomicDecrement(volatile QT3DSI32 *val);

    /* add delta to *val. Return the new value */
    QT3DS_AUTOTEST_EXPORT QT3DSI32 atomicAdd(volatile QT3DSI32 *val, QT3DSI32 delta);

    /* compute the maximum of dest and val. Return the new value */
    QT3DS_AUTOTEST_EXPORT QT3DSI32 atomicMax(volatile QT3DSI32 *val, QT3DSI32 val2);

} // namespace foundation
} // namespace qt3ds

#endif
