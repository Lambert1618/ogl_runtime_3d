/****************************************************************************
**
** Copyright (C) 2001-2004 NovodeX AG.
** Copyright (C) 2004-2008 AGEIA Technologies, Inc.
** Copyright (C) 2008-2013 NVIDIA Corporation.
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
#include "foundation/Qt3DSTime.h"

#include <time.h>
#include <sys/time.h>

#if defined QT3DS_APPLE
#include <mach/mach_time.h>
#endif

// Use real-time high-precision timer.
#ifndef QT3DS_APPLE
#define CLOCKID CLOCK_REALTIME
#endif

namespace qt3ds {
namespace foundation {

    const CounterFrequencyToTensOfNanos Time::sCounterFreq = Time::getCounterFrequency();

    static Time::Second getTimeSeconds()
    {
        static struct timeval _tv;
        gettimeofday(&_tv, NULL);
        return double(_tv.tv_sec) + double(_tv.tv_usec) * 0.000001;
    }

    Time::Time() { mLastTime = getTimeSeconds(); }

    Time::Second Time::getElapsedSeconds()
    {
        Time::Second curTime = getTimeSeconds();
        Time::Second diff = curTime - mLastTime;
        mLastTime = curTime;
        return diff;
    }

    Time::Second Time::peekElapsedSeconds()
    {
        Time::Second curTime = getTimeSeconds();
        Time::Second diff = curTime - mLastTime;
        return diff;
    }

    Time::Second Time::getLastTime() const { return mLastTime; }

#ifdef QT3DS_APPLE
    CounterFrequencyToTensOfNanos Time::getCounterFrequency()
    {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        // mach_absolute_time * (info.numer/info.denom) is in units of nano seconds
        return CounterFrequencyToTensOfNanos(info.numer, info.denom * 10);
    }

    QT3DSU64 Time::getCurrentCounterValue() { return mach_absolute_time(); }

#else

    CounterFrequencyToTensOfNanos Time::getCounterFrequency()
    {
        return CounterFrequencyToTensOfNanos(1, 10);
    }

    PxU64 Time::getCurrentCounterValue()
    {
        struct timespec mCurrTimeInt;
        clock_gettime(CLOCKID, &mCurrTimeInt);
        // Convert to nanos as this doesn't cause a large divide here
        return (static_cast<PxU64>(mCurrTimeInt.tv_sec) * 1000000000)
            + (static_cast<PxU64>(mCurrTimeInt.tv_nsec));
    }
#endif

} // namespace foundation
} // namespace qt3ds
