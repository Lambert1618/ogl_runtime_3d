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

#ifndef QT3DS_FOUNDATION_PERFTIMER_H
#define QT3DS_FOUNDATION_PERFTIMER_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSTime.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/StringTable.h"

namespace qt3ds {
namespace foundation {

    class IPerfTimer : public NVRefCounted
    {
    protected:
        virtual ~IPerfTimer() {}
    public:
        // amount is in counter frequency units
        virtual void Update(const char *inTag, QT3DSU64 inAmount) = 0;
        // Dump current summation of timer data.
        virtual void OutputTimerData(QT3DSU32 inFrameCount = 0) = 0;
        virtual void ResetTimerData() = 0;

        static IPerfTimer &CreatePerfTimer(NVFoundationBase &inFoundation);
    };

    // Specialize this struct to get the perf timer in different contexts.
    template <typename TTimerProvider>
    struct STimerProvider
    {
        static IPerfTimer &getPerfTimer(TTimerProvider &inProvider)
        {
            return inProvider.getPerfTimer();
        }
    };

    template <typename TTimerProvider>
    IPerfTimer &getPerfTimer(TTimerProvider &inProvider)
    {
        return STimerProvider<TTimerProvider>::getPerfTimer(inProvider);
    }

    struct SStackPerfTimer
    {
        IPerfTimer *m_Timer;
        QT3DSU64 m_Start;
        const char *m_Id;

        SStackPerfTimer(IPerfTimer &destination, const char *inId)
            : m_Timer(&destination)
            , m_Start(Time::getCurrentCounterValue())
            , m_Id(inId)
        {
        }

        SStackPerfTimer(IPerfTimer *destination, const char *inId)
            : m_Timer(destination)
            , m_Start(Time::getCurrentCounterValue())
            , m_Id(inId)
        {
        }

        ~SStackPerfTimer()
        {
            if (m_Timer) {
                QT3DSU64 theStop = Time::getCurrentCounterValue();
                QT3DSU64 theAmount = theStop - m_Start;
                m_Timer->Update(m_Id, theAmount);
            }
        }
    };
}
}

#ifdef QT3DS_ENABLE_PERF_LOGGING
#define QT3DS_PERF_SCOPED_TIMER(timer, name)                                             \
    SStackPerfTimer __perfTimer(timer, name);
#else
/// Empty macro
#define QT3DS_PERF_SCOPED_TIMER(timer, name)
#endif

#endif
