/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DSDM_SIGNAL_STRUCTS_H
#define QT3DSDM_SIGNAL_STRUCTS_H
#include "Qt3DSDMSignalSystem.h"
// Helper structs for signals of different arity.
namespace qt3dsdm {
// Helper defines to create static tables of signals
// Macros with the same number of args but with 'declare' replaced by 'define
// are available in UICDMSignalStructImpl.h.  You will need to place these somewhere but
// they rely on std::bind so I don't place them here.
#define QT3DSDM_SIGNALS_DECLARE_SPECIFIC_SIGNAL_STRUCT0(name)                                        \
    struct Sig##name                                                                               \
    {                                                                                              \
        static std::shared_ptr<ISignalConnection>                                                \
        AddListener(ISignalSystem &inSystem, void *inSender, std::function<void()> inHandler);   \
        static void Send(ISignalSystem &system, void *sender);                                     \
    };

// Helper defines to create static tables of signals
#define QT3DSDM_SIGNALS_DECLARE_SPECIFIC_SIGNAL_STRUCT1(name, d1)                                    \
    struct Sig##name                                                                               \
    {                                                                                              \
        static std::shared_ptr<ISignalConnection>                                                \
        AddListener(ISignalSystem &inSystem, void *inSender, std::function<void(d1)> inHandler); \
        static void Send(ISignalSystem &system, void *sender, const d1 &arg);                      \
    };

// Helper defines to create static tables of signals
#define QT3DSDM_SIGNALS_DECLARE_SPECIFIC_SIGNAL_STRUCT2(name, d1, d2)                                \
    struct Sig##name                                                                               \
    {                                                                                              \
        static std::shared_ptr<ISignalConnection>                                                \
        AddListener(ISignalSystem &inSystem, void *inSender,                                       \
                    std::function<void(d1, d2)> inHandler);                                      \
        static void Send(ISignalSystem &system, void *sender, const d1 &arg1, const d2 &arg2);     \
    };

// Helper defines to create static tables of signals
#define QT3DSDM_SIGNALS_DECLARE_SPECIFIC_SIGNAL_STRUCT3(name, d1, d2, d3)                            \
    struct Sig##name                                                                               \
    {                                                                                              \
        static std::shared_ptr<ISignalConnection>                                                \
        AddListener(ISignalSystem &inSystem, void *inSender,                                       \
                    std::function<void(d1, d2, d3)> inHandler);                                  \
        static void Send(ISignalSystem &system, void *sender, const d1 &arg1, const d2 &arg2,      \
                         const d3 &arg3);                                                          \
    };

// Helper defines to create static tables of signals
#define QT3DSDM_SIGNALS_DECLARE_SPECIFIC_SIGNAL_STRUCT4(name, d1, d2, d3, d4)                        \
    struct Sig##name                                                                               \
    {                                                                                              \
        static std::shared_ptr<ISignalConnection>                                                \
        AddListener(ISignalSystem &inSystem, void *inSender,                                       \
                    std::function<void(d1, d2, d3, d4)> inHandler);                              \
        static void Send(ISignalSystem &system, void *sender, const d1 &arg1, const d2 &arg2,      \
                         const d3 &arg3, const d4 &arg4);                                          \
    };
}

#endif
