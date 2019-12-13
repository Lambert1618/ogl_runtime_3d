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
#include "foundation/Qt3DSPerfTimer.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "EASTL/hash_map.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "EASTL/sort.h"

using namespace qt3ds::foundation;
using namespace qt3ds;

namespace {
struct STimerEntry
{
    QT3DSU64 m_Total;
    QT3DSU64 m_Max;
    QT3DSU64 m_CumulativeTime;
    QT3DSU32 m_UpdateCount;
    CRegisteredString m_Tag;
    size_t m_Order;

    STimerEntry(CRegisteredString tag, size_t order)
        : m_Total(0)
        , m_Max(0)
        , m_CumulativeTime(0)
        , m_UpdateCount(0)
        , m_Tag(tag)
        , m_Order(order)
    {
    }
    void Update(QT3DSU64 increment, QT3DSU64 cumulativeTime = 0)
    {
        m_Total += increment;
        m_Max = increment > m_Max ? increment : m_Max;
        m_CumulativeTime = cumulativeTime;
        ++m_UpdateCount;
    }

    void Output(QT3DSU32 idx, QT3DSU32 inFramesPassed)
    {
        if (m_Total) {
            QT3DSU64 tensNanos = Time::sCounterFreq.toTensOfNanos(m_Total);
            QT3DSU64 maxNanos = Time::sCounterFreq.toTensOfNanos(m_Max);
            QT3DSU64 cumNanos = Time::sCounterFreq.toTensOfNanos(m_CumulativeTime);

            double milliseconds = tensNanos / 100000.0;
            double maxMilliseconds = maxNanos / 100000.0;
            double cumulativeMs = cumNanos / 100000.0;

            if (inFramesPassed == 0) {
                qCWarning(WARNING, PERF_INFO, "%d. (%d) %s - %fms = %fms", (idx+1),
                          m_UpdateCount, m_Tag.c_str(), milliseconds, cumulativeMs);
            } else {
                milliseconds /= inFramesPassed;
                qCWarning(WARNING, PERF_INFO, "%s - %fms/frame-total %fms-max %u hits",
                    m_Tag.c_str(), milliseconds, maxMilliseconds, m_UpdateCount);
            }
        }
    }

    void Reset()
    {
        m_Total = 0;
        m_Max = 0;
        m_UpdateCount = 0;
    }

    bool operator<(const STimerEntry &other) const { return m_CumulativeTime < other.m_CumulativeTime; }
};
struct SPerfTimer : public IPerfTimer
{
    typedef eastl::hash_map<CRegisteredString, STimerEntry> THashMapType;
    NVFoundationBase &m_Foundation;
    // This object needs its own string table because it is used during the binary load process with
    // the application string table gets booted up.
    NVScopedRefCounted<IStringTable> m_StringTable;
    THashMapType m_Entries;
    eastl::vector<STimerEntry> m_PrintEntries;
    Mutex m_Mutex;
    QT3DSI32 mRefCount;
    QT3DSU64 m_startTime;

    SPerfTimer(NVFoundationBase &fnd)
        : m_Foundation(fnd)
        , m_StringTable(IStringTable::CreateStringTable(fnd.getAllocator()))
        , m_Mutex(fnd.getAllocator())
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    void StartMeasuring() override
    {
        Mutex::ScopedLock __locker(m_Mutex);
        m_startTime = Time::getCurrentCounterValue();
    }

    void Update(const char *inId, QT3DSU64 inAmount, QT3DSU64 inStop) override
    {
        Mutex::ScopedLock __locker(m_Mutex);
        QT3DSU64 cumulativeTime = inStop - m_startTime;
        CRegisteredString theStr(m_StringTable->RegisterStr(inId));
        THashMapType::iterator theFind =
            m_Entries.insert(eastl::make_pair(theStr, STimerEntry(theStr, m_Entries.size()))).first;
        theFind->second.Update(inAmount, cumulativeTime);
    }

    // Dump current summation of timer data.
    void OutputTimerData(QT3DSU32 inFramesPassed = 0) override
    {
        Mutex::ScopedLock __locker(m_Mutex);
        m_PrintEntries.clear();
        for (THashMapType::iterator iter = m_Entries.begin(), end = m_Entries.end(); iter != end;
             ++iter) {
            m_PrintEntries.push_back(iter->second);
            iter->second.Reset();
        }

        eastl::sort(m_PrintEntries.begin(), m_PrintEntries.end());

        for (QT3DSU32 idx = 0, end = (QT3DSU32)m_PrintEntries.size(); idx < end; ++idx) {
            m_PrintEntries[idx].Output(idx, inFramesPassed);
        }
    }

    void ResetTimerData() override
    {
        Mutex::ScopedLock __locker(m_Mutex);
        for (THashMapType::iterator iter = m_Entries.begin(), end = m_Entries.end(); iter != end;
             ++iter) {
            iter->second.Reset();
        }
    }

    double CurrentDuration() override
    {
        QT3DSU64 duration = Time::getCurrentCounterValue() - m_startTime;
        QT3DSU64 tensNanos = Time::sCounterFreq.toTensOfNanos(duration);
        double durationMs = tensNanos / 100000.0;
        return durationMs;
    }

    virtual void ClearPerfKeys()
    {
        Mutex::ScopedLock __locker(m_Mutex);
        m_Entries.clear();
    }
};
}

IPerfTimer &IPerfTimer::CreatePerfTimer(NVFoundationBase &fnd)
{
    return *QT3DS_NEW(fnd.getAllocator(), SPerfTimer)(fnd);
}
