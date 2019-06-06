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

#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "EASTL/vector.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderRasterizerState.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace render {

    NVRenderRasterizerState::NVRenderRasterizerState(NVRenderContextImpl &context,
                                                     NVFoundationBase &fnd, QT3DSF32 depthBias,
                                                     QT3DSF32 depthScale, NVRenderFaces::Enum cullFace)
        : m_Context(context)
        , m_Foundation(fnd)
        , mRefCount(0)
        , m_Backend(context.GetBackend())
    {
        // create backend handle
        m_StateHandle = m_Backend->CreateRasterizerState(depthBias, depthScale, cullFace);
    }

    NVRenderRasterizerState::~NVRenderRasterizerState()
    {
        if (m_StateHandle) {
            m_Backend->ReleaseRasterizerState(m_StateHandle);
            m_Context.StateDestroyed(*this);
        }
    }

    NVRenderRasterizerState *NVRenderRasterizerState::Create(NVRenderContextImpl &context,
                                                             QT3DSF32 depthBias, QT3DSF32 depthScale,
                                                             NVRenderFaces::Enum cullFace)
    {
        return QT3DS_NEW(context.GetFoundation().getAllocator(), NVRenderRasterizerState)(
            context, context.GetFoundation(), depthBias, depthScale, cullFace);
    }
}
}
