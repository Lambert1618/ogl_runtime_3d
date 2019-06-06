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
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderPathRender.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderPathSpecification.h"
#include "render/Qt3DSRenderPathFontSpecification.h"

namespace qt3ds {
namespace render {

    NVRenderPathRender::NVRenderPathRender(NVRenderContextImpl &context, NVFoundationBase &fnd,
                                           size_t range)
        : m_Context(context)
        , m_Foundation(fnd)
        , m_Backend(context.GetBackend())
        , mRefCount(0)
        , m_StrokeWidth(0.0f)
    {
        m_Range = range;
        m_PathRenderHandle = m_Backend->CreatePathNVObject(range);
    }

    NVRenderPathRender::~NVRenderPathRender()
    {
        if (m_PathRenderHandle) {
            m_Backend->ReleasePathNVObject(m_PathRenderHandle, m_Range);
        }
    }

    void NVRenderPathRender::SetPathSpecification(NVRenderPathSpecification &inCommandBuffer)
    {
        m_Backend->SetPathSpecification(m_PathRenderHandle, inCommandBuffer.GetPathCommands(),
                                        inCommandBuffer.GetPathCoords());
    }

    NVBounds3 NVRenderPathRender::GetPathObjectBoundingBox()
    {
        return m_Backend->GetPathObjectBoundingBox(m_PathRenderHandle);
    }

    NVBounds3 NVRenderPathRender::GetPathObjectFillBox()
    {
        return m_Backend->GetPathObjectFillBox(m_PathRenderHandle);
    }

    NVBounds3 NVRenderPathRender::GetPathObjectStrokeBox()
    {
        return m_Backend->GetPathObjectStrokeBox(m_PathRenderHandle);
    }

    void NVRenderPathRender::SetStrokeWidth(QT3DSF32 inStrokeWidth)
    {
        if (inStrokeWidth != m_StrokeWidth) {
            m_StrokeWidth = inStrokeWidth;
            m_Backend->SetStrokeWidth(m_PathRenderHandle, inStrokeWidth);
        }
    }

    QT3DSF32 NVRenderPathRender::GetStrokeWidth() const { return m_StrokeWidth; }

    void NVRenderPathRender::StencilStroke() { m_Backend->StencilStrokePath(m_PathRenderHandle); }

    void NVRenderPathRender::StencilFill() { m_Backend->StencilFillPath(m_PathRenderHandle); }

    NVRenderPathRender *NVRenderPathRender::Create(NVRenderContextImpl &context, size_t range)
    {
        if (!context.IsPathRenderingSupported())
            return NULL;

        NVRenderPathRender *retval =
            QT3DS_NEW(context.GetFoundation().getAllocator(),
                   NVRenderPathRender)(context, context.GetFoundation(), range);

        return retval;
    }
}
}
