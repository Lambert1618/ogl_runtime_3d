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

#ifndef AKDATALOGGERVIEWER_H_INCLUDED
#define AKDATALOGGERVIEWER_H_INCLUDED

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSTypes.h"
#include "Qt3DSDataLogger.h"
#include "Qt3DSDataLoggerEnums.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {
// Helps interpret and analyse data logger entries
class CDataLoggerViewer
{
public:
    typedef CArray<SPerfLogEntry> TPerfLogEntryList;

public:
    CDataLoggerViewer(TPerfLogEntryList *inPerfLogEntryList);
    CDataLoggerViewer(const char *inFilename);
    virtual ~CDataLoggerViewer();

public:
    float GetAverageTimeInMsForEntry(enum EDataLoggerEvents inEventType);
    void ResetPerfLogEntry(enum EDataLoggerEvents inEventType);
    void ResetAllEntries();

public:
    static bool ProcessKeyEvent(char inKey, TPerfLogEntryList *inList);

protected:
    TPerfLogEntryList *m_PerfLogEntryList;
    bool m_Dispose;
};

} // namespace NVUI

#endif // AKDATALOGGERVIEWER_H_INCLUDED
