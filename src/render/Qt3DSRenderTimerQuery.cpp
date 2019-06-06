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

#include "render/Qt3DSRenderTimerQuery.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/StringTable.h"

namespace qt3ds {
namespace render {

    NVRenderTimerQuery::NVRenderTimerQuery(NVRenderContextImpl &context, NVFoundationBase &fnd)
        : NVRenderQueryBase(context, fnd)
    {
    }

    NVRenderTimerQuery::~NVRenderTimerQuery() {}

    void NVRenderTimerQuery::Begin()
    {
        m_Backend->BeginQuery(m_QueryHandle, NVRenderQueryType::Timer);
    }

    void NVRenderTimerQuery::End() { m_Backend->EndQuery(m_QueryHandle, NVRenderQueryType::Timer); }

    void NVRenderTimerQuery::GetResult(QT3DSU32 *params)
    {
        m_Backend->GetQueryResult(m_QueryHandle, NVRenderQueryResultType::Result, params);
    }

    void NVRenderTimerQuery::GetResult(QT3DSU64 *params)
    {
        m_Backend->GetQueryResult(m_QueryHandle, NVRenderQueryResultType::Result, params);
    }

    void NVRenderTimerQuery::SetTimerQuery() { m_Backend->SetQueryTimer(m_QueryHandle); }

    NVRenderTimerQuery *NVRenderTimerQuery::Create(NVRenderContextImpl &context)
    {
        if (!context.IsTimerQuerySupported())
            return NULL;

        NVRenderTimerQuery *retval = QT3DS_NEW(context.GetFoundation().getAllocator(),
                                            NVRenderTimerQuery)(context, context.GetFoundation());

        return retval;
    }
}
}
