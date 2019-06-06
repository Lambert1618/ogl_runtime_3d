/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

#pragma once
#include "Qt3DSTimer.h"
#include "Qt3DSTypes.h"

// You have to include egl before including this file.

typedef EGLuint64NV(EGLAPIENTRYP PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC)(void);
typedef EGLuint64NV(EGLAPIENTRYP PFNEGLGETSYSTEMTIMENVPROC)(void);
extern EGLuint64NV eglSystemTimeFrequency;
extern PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC eglGetSystemTimeFrequencyNVProc;
extern PFNEGLGETSYSTEMTIMENVPROC eglGetSystemTimeNVProc;

namespace Q3DStudio {

struct SEGLTimeProvider : public Q3DStudio::ITimeProvider
{
public:
    Q3DStudio::INT64 m_TimeFrequency;
    SEGLTimeProvider()
        : m_TimeFrequency(0)
    {
    }
    virtual Q3DStudio::INT64 GetCurrentTimeMicroSeconds()
    {
        // This can be called before EGL is initialized.
        if (eglGetSystemTimeFrequencyNVProc == NULL || eglGetSystemTimeNVProc == NULL)
            return 0;

        if (!m_TimeFrequency) {
            // Time frequency converts to seconds.
            m_TimeFrequency = eglGetSystemTimeFrequencyNVProc();
            // We need it to conver to microseconds
            m_TimeFrequency = m_TimeFrequency / 1000000ULL;
        }
        Q3DStudio::INT64 currentTime = eglGetSystemTimeNVProc();
        currentTime = currentTime / m_TimeFrequency;
        return currentTime;
    }
};
}