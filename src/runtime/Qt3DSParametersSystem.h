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
#ifndef QT3DS_PARAMETERS_SYSTEM_H
#define QT3DS_PARAMETERS_SYSTEM_H
#pragma once
#include "Qt3DSKernelTypes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "EASTL/utility.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace foundation {
    class IOutStream;
}
}

namespace qt3ds {
namespace runtime {
    using namespace qt3ds;
    using namespace qt3ds::foundation;

    typedef eastl::pair<QT3DSU32, Q3DStudio::UVariant> TIdValuePair;

    class IParametersSystem : public NVRefCounted
    {
    public:
        virtual QT3DSI32 CreateParameterGroup() = 0;
        virtual void AddParameter(QT3DSI32 inGroup, QT3DSU32 inNameHash, Q3DStudio::UVariant inParam) = 0;

        virtual QT3DSU32 GetNumParameters(QT3DSI32 inGroupId) const = 0;
        virtual TIdValuePair GetParameter(QT3DSI32 inGroupId, QT3DSU32 inIndex) const = 0;

        static IParametersSystem &CreateParametersSystem(NVFoundationBase &inFoundation);
    };
}
}

#endif
