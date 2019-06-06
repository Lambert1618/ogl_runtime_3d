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

#ifndef QT3DS_FOUNDATION_PSTIME_H
#define QT3DS_FOUNDATION_PSTIME_H

#include "foundation/Qt3DS.h"

#if defined(QT3DS_LINUX) || defined(QT3DS_ANDROID) || defined(QT3DS_QNX)
#include <time.h>
#endif

namespace qt3ds {
namespace foundation {

    struct CounterFrequencyToTensOfNanos
    {
        QT3DSU64 mNumerator;
        QT3DSU64 mDenominator;
        CounterFrequencyToTensOfNanos(QT3DSU64 inNum, QT3DSU64 inDenom)
            : mNumerator(inNum)
            , mDenominator(inDenom)
        {
        }

        // quite slow.
        QT3DSU64 toTensOfNanos(QT3DSU64 inCounter) const
        {
            return (inCounter * mNumerator) / mDenominator;
        }
    };

    class QT3DS_FOUNDATION_API Time
    {
    public:
        typedef double Second;
        static const QT3DSU64 sNumTensOfNanoSecondsInASecond = 100000000;
        // This is supposedly guaranteed to not change after system boot
        // regardless of processors, speedstep, etc.
        static const CounterFrequencyToTensOfNanos sCounterFreq;

        static CounterFrequencyToTensOfNanos getCounterFrequency();

        static QT3DSU64 getCurrentCounterValue();

        // SLOW!!
        // Thar be a 64 bit divide in thar!
        static QT3DSU64 getCurrentTimeInTensOfNanoSeconds()
        {
            QT3DSU64 ticks = getCurrentCounterValue();
            return sCounterFreq.toTensOfNanos(ticks);
        }

        Time();
        Second getElapsedSeconds();
        Second peekElapsedSeconds();
        Second getLastTime() const;

    private:
#if defined(QT3DS_LINUX) || defined(QT3DS_ANDROID) || defined(QT3DS_APPLE) || defined(QT3DS_PSP2)              \
    || defined(QT3DS_QNX)
        Second mLastTime;
#else
        QT3DSI64 mTickCount;
#endif
    };
} // namespace foundation
} // namespace qt3ds

#endif
