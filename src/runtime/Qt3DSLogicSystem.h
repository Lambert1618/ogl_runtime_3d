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
#ifndef QT3DS_LOGIC_SYSTEM_H
#define QT3DS_LOGIC_SYSTEM_H
#include "Qt3DSElementSystem.h"

namespace qt3ds {
namespace runtime {

    class ILogicSystem : public NVRefCounted
    {
    public:
        // returns the action id
        virtual QT3DSI32 AddAction(element::SElement &inTrigger,
                                Q3DStudio::TEventCommandHash inEventNameHash,
                                element::SElement *inTarget, element::SElement *inOwner,
                                Q3DStudio::TEventCommandHash inType, Q3DStudio::UVariant inArg1,
                                Q3DStudio::UVariant inArg2, bool inActive) = 0;

        virtual void OnEvent(Q3DStudio::TEventCommandHash inEventName, element::SElement &inTarget,
                             Q3DStudio::IPresentation &inPresentation) const = 0;
        virtual void SetActive(QT3DSI32 inActionIndex, bool inActive,
                               IElementAllocator &inElemAllocator) = 0;

        virtual void SaveBinaryData(qt3ds::foundation::IOutStream &ioStream) = 0;
        virtual void LoadBinaryData(NVDataRef<QT3DSU8> inLoadData) = 0;

        static ILogicSystem &CreateLogicSystem(NVFoundationBase &inFnd);
    };
}
}
#endif