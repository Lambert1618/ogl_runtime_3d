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
#include "Qt3DSRender.h"
#include "Qt3DSRenderScene.h"
#include "Qt3DSRenderLayer.h"
#include "Qt3DSRenderContextCore.h"
#include "render/Qt3DSRenderContext.h"

using namespace qt3ds::render;

SScene::SScene()
    : SGraphObject(GraphObjectTypes::Scene)
    , m_Presentation(nullptr)
    , m_FirstChild(nullptr)
    , m_ClearColor(0.0f)
    , m_UseClearColor(true)
    , m_Dirty(true)
    , m_IsSubPresentationScene(false)
{
}

void SScene::AddChild(SLayer &inLayer)
{
    if (m_FirstChild == nullptr)
        m_FirstChild = &inLayer;
    else
        GetLastChild()->m_NextSibling = &inLayer;
    inLayer.m_Scene = this;
}

SLayer *SScene::GetLastChild()
{
    // empty loop intentional
    SLayer *child;
    for (child = m_FirstChild; child && child->m_NextSibling;
         child = (SLayer *)child->m_NextSibling) {
    }

    return child;
}

bool SScene::PrepareForRender(IQt3DSRenderContext &inContext,
                              const SRenderInstanceId id)
{
    // We need to iterate through the layers in reverse order and ask them to render.
    bool wasDirty = m_Dirty;
    m_Dirty = false;

    if (m_FirstChild) {
        wasDirty |=
            inContext.GetRenderer().PrepareLayerForRender(*m_FirstChild,
                                                          true, id);
    }
    return wasDirty;
}

void SScene::Render(const QT3DSVec2 &inViewportDimensions, IQt3DSRenderContext &inContext,
                    RenderClearCommand inClearColorBuffer, const SRenderInstanceId id)
{
    if ((inClearColorBuffer == SScene::ClearIsOptional && m_UseClearColor)
        || inClearColorBuffer == SScene::AlwaysClear) {
        QT3DSVec4 clearColor(0.0f, 0.0f, 0.0f, 0.0f);
        if (m_UseClearColor) {
            clearColor.x = m_ClearColor.x;
            clearColor.y = m_ClearColor.y;
            clearColor.z = m_ClearColor.z;
            clearColor.w = m_ClearColor.w;
            if (m_ClearColor.w < 1.0f) {
                clearColor.x *= m_ClearColor.w;
                clearColor.y *= m_ClearColor.w;
                clearColor.z *= m_ClearColor.w;
            }
        }
        if (inContext.isSubPresentationRenderInLayer() && clearColor.w < 1.0f) {
            inContext.GetRenderer().FillQuad(clearColor);
        } else {
            // Maybe clear and reset to previous clear color after we leave.
            qt3ds::render::NVRenderContextScopedProperty<QT3DSVec4> __clearColor(
                inContext.GetRenderContext(), &NVRenderContext::GetClearColor,
                &NVRenderContext::SetClearColor, clearColor);
            inContext.GetRenderContext().Clear(qt3ds::render::NVRenderClearValues::Color);
        }
    }
    if (m_FirstChild) {
        inContext.GetRenderer().RenderLayer(*m_FirstChild, inViewportDimensions, m_UseClearColor,
                                            m_ClearColor, true, id);
    }
}
void SScene::RenderWithClear(const QT3DSVec2 &inViewportDimensions,
                             IQt3DSRenderContext &inContext,
                             RenderClearCommand inClearColorBuffer,
                             QT3DSVec4 inClearColor,
                             const SRenderInstanceId id)
{
    // If this scene is not using clear color, we set the color
    // to background color from parent layer. This allows
    // fully transparent subpresentations (both scene and layer(s) transparent)
    // to inherit color from the layer that contains them.
    if (!m_UseClearColor) {
        m_ClearColor = inClearColor;
        m_UseClearColor = true;
    }
    Render(inViewportDimensions, inContext, inClearColorBuffer, id);
}
