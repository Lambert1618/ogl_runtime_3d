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
#ifndef QT3DS_ANIMATION_SYSTEM_H
#define QT3DS_ANIMATION_SYSTEM_H
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSRefCounted.h"

#pragma once
namespace qt3ds {
namespace foundation {
    class IOutStream;
}
}

namespace qt3ds {
namespace runtime {
    using namespace qt3ds;
    using namespace qt3ds::foundation;

    namespace element {
        class SElement;
    }
    class IElementAllocator;

    class IAnimationSystem : public NVRefCounted
    {
    public:
        virtual QT3DSI32 CreateAnimationTrack(element::SElement &inElement, QT3DSU32 inPropertyName,
                                           bool inDynamic) = 0;
        virtual void AddKey(QT3DSF32 inTime, QT3DSF32 inValue, QT3DSF32 inC1Time, QT3DSF32 inC1Value,
                            QT3DSF32 inC2Time, QT3DSF32 inC2Value) = 0;
        virtual void Update() = 0;
        virtual void SetActive(QT3DSI32 inTrackId, bool inActive) = 0;
        virtual void UpdateDynamicKey(QT3DSI32 inTrackId) = 0;
        virtual QT3DSI32 getActiveTrackForElemProp(element::SElement *inElement,
                                                   QT3DSU32 inPropertyName) = 0;

        static IAnimationSystem &CreateAnimationSystem(NVFoundationBase &inFoundation);
    };
}
}

#endif
