/****************************************************************************
**
** Copyright (C) 2013 - 2016 NVIDIA Corporation.
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

#ifndef QT3DS_ELAPSED_TIMER_H
#define QT3DS_ELAPSED_TIMER_H
#include <QElapsedTimer>

namespace Q3DSViewer {

///< @brief this class implementes a elapsed timer using QElapsedTimer
class Qt3DSElapsedTimer
{
public:
    QElapsedTimer m_elapsedTimer;
    QElapsedTimer::ClockType m_clockType;

    Qt3DSElapsedTimer()
    {
        m_clockType = QElapsedTimer::clockType();
        reset();
    }

    void reset() { m_elapsedTimer.start(); }

    qint64 getNanoSeconds()
    {
        qint64 time = m_elapsedTimer.nsecsElapsed();
        reset();
        return time;
    }
};

///< @brief this class implementes a timer using QElapsedTimer
class Qt3DSTimer
{
public:
    QElapsedTimer m_timer;

    Qt3DSTimer() { start(); }

    void start() { m_timer.start(); }

    qint64 getNanoSeconds() { return static_cast<qint64>(m_timer.elapsed() * 1000 * 1000); }
};

///< @brief this class implementes a circular buffer
template <typename TDataType, unsigned long TBufferSize = 20>
struct NumericCircularBuffer
{
    unsigned long m_TotalNumElements; ///< holds the current number of elements
    unsigned long m_NumElements; ///< restricts the size to TBufferSize and wrappes around
    unsigned long m_NextIndex; ///< next index
    TDataType m_RunningTotal; ///< summed value of all entries
    TDataType m_Elements[TBufferSize]; ///< element buffer

    NumericCircularBuffer() { clear(); }
    void clear()
    {
        m_TotalNumElements = 0;
        m_NumElements = 0;
        m_NextIndex = 0;
        m_RunningTotal = 0;
        memset(m_Elements, 0, sizeof(TDataType) * TBufferSize);
    }

    void push_back(const TDataType &data)
    {
        if (m_NumElements < TBufferSize)
            ++m_NumElements;
        else
            m_RunningTotal -= m_Elements[m_NextIndex];
        m_RunningTotal += data;
        ++m_TotalNumElements;
        m_Elements[m_NextIndex] = data;
        m_NextIndex = (m_NextIndex + 1) % TBufferSize;
    }
    unsigned long size() const { return m_NumElements; }
    TDataType average() const
    {
        TDataType mySize = ((TDataType)size());
        if (mySize)
            return m_RunningTotal / mySize;
        return 0;
    }

    unsigned long getTotalItems() const { return m_TotalNumElements; }

    static unsigned long getBufferSize() { return TBufferSize; }
};

} // end namspace

#endif // QT3DS_ELAPSED_TIMER_H
