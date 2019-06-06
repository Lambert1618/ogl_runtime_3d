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

#include "foundation/Qt3DSTime.h"
#include "foundation/windows/Qt3DSWindowsInclude.h"

namespace {
::qt3ds::QT3DSI64 getTimeTicks()
{
    LARGE_INTEGER a;
    QueryPerformanceCounter(&a);
    return a.QuadPart;
}

double getTickDuration()
{
    LARGE_INTEGER a;
    QueryPerformanceFrequency(&a);
    return 1.0f / double(a.QuadPart);
}

double sTickDuration = getTickDuration();
} // namespace

namespace qt3ds {
namespace foundation {

    const CounterFrequencyToTensOfNanos Time::sCounterFreq = Time::getCounterFrequency();

    CounterFrequencyToTensOfNanos Time::getCounterFrequency()
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return CounterFrequencyToTensOfNanos(Time::sNumTensOfNanoSecondsInASecond, freq.QuadPart);
    }

    QT3DSU64 Time::getCurrentCounterValue()
    {
        LARGE_INTEGER ticks;
        QueryPerformanceCounter(&ticks);
        return ticks.QuadPart;
    }

    Time::Time()
        : mTickCount(0)
    {
        getElapsedSeconds();
    }

    Time::Second Time::getElapsedSeconds()
    {
        QT3DSI64 lastTickCount = mTickCount;
        mTickCount = getTimeTicks();
        return (mTickCount - lastTickCount) * sTickDuration;
    }

    Time::Second Time::peekElapsedSeconds()
    {
        return (getTimeTicks() - mTickCount) * sTickDuration;
    }

    Time::Second Time::getLastTime() const { return mTickCount * sTickDuration; }

} // namespace foundation
} // namespace qt3ds
