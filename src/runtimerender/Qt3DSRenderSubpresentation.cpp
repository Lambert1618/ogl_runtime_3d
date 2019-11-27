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
#include "Qt3DSRenderSubpresentation.h"
#include "Qt3DSRenderRenderList.h"
#ifdef _WIN32
#pragma warning(disable : 4355) // this used in initializer list.  I have never seen this result in
                                // a physical error
#endif
namespace qt3ds {
namespace render {

    Qt3DSRenderPickResult CSubPresentationPickQuery::Pick(const QT3DSVec2 &inMouseCoords,
                                                         const QT3DSVec2 &inViewportDimensions,
                                                         bool inPickEverything)
    {
        return m_Renderer.DoGraphQueryPick(inMouseCoords, inViewportDimensions, inPickEverything);
    }

    CSubPresentationRenderer::CSubPresentationRenderer(IQt3DSRenderContext &inRenderContext,
                                                       SPresentation &inPresentation)
        : m_RenderContext(inRenderContext)
        , m_Presentation(inPresentation)
        , mRefCount(0)
        , m_PickQuery(*this)
        , m_OffscreenRendererType(inRenderContext.GetStringTable().RegisterStr(GetRendererName()))
    {
    }

    SOffscreenRendererEnvironment
        CSubPresentationRenderer::GetDesiredEnvironment(QT3DSVec2 /*inPresScale*/)
    {
        NVRenderTextureFormats::Enum format = NVRenderTextureFormats::RGBA8;
        return SOffscreenRendererEnvironment((QT3DSU32)(m_Presentation.m_PresentationDimensions.x),
                                             (QT3DSU32)(m_Presentation.m_PresentationDimensions.y),
                                             format, OffscreenRendererDepthValues::Depth16, false,
                                             AAModeValues::NoAA);
    }

    SOffscreenRenderFlags
    CSubPresentationRenderer::NeedsRender(const SOffscreenRendererEnvironment & /*inEnvironment*/,
                                          QT3DSVec2 /*inPresScale*/,
                                          const SRenderInstanceId instanceId)
    {
        NVRenderRect theViewportSize(m_RenderContext.GetRenderList().GetViewport());
        bool wasDirty = m_Presentation.m_Scene->PrepareForRender(
            QT3DSVec2((QT3DSF32)theViewportSize.m_Width, (QT3DSF32)theViewportSize.m_Height),
            m_RenderContext, instanceId);
        // Always transparent
        return SOffscreenRenderFlags(true, wasDirty);
    }

    // Returns true if the rendered result image has transparency, or false
    // if it should be treated as a completely opaque image.
    void CSubPresentationRenderer::Render(const SOffscreenRendererEnvironment &inEnvironment,
                                          NVRenderContext &inRenderContext, QT3DSVec2,
                                          SScene::RenderClearCommand inClearColorBuffer,
                                          const SRenderInstanceId instanceId)
    {
        NVRenderRect theViewportSize(inRenderContext.GetViewport());
        m_Presentation.m_Scene->Render(
            QT3DSVec2((QT3DSF32)theViewportSize.m_Width, (QT3DSF32)theViewportSize.m_Height),
            m_RenderContext, inClearColorBuffer, instanceId);
        m_LastRenderedEnvironment = inEnvironment;
    }

    void CSubPresentationRenderer::RenderWithClear(
        const SOffscreenRendererEnvironment &inEnvironment,
        NVRenderContext &inRenderContext, QT3DSVec2 inPresScale,
        SScene::RenderClearCommand inClearBuffer, QT3DSVec4 inClearColor,
        const SRenderInstanceId id)
    {
        Q_UNUSED(inEnvironment);
        Q_UNUSED(inPresScale);
        NVRenderRect theViewportSize(inRenderContext.GetViewport());
        m_Presentation.m_Scene->RenderWithClear(
            QT3DSVec2((QT3DSF32)theViewportSize.m_Width, (QT3DSF32)theViewportSize.m_Height),
            m_RenderContext, inClearBuffer, inClearColor, id);
    }

    // You know the viewport dimensions because
    Qt3DSRenderPickResult CSubPresentationRenderer::DoGraphQueryPick(
        const QT3DSVec2 &inMouseCoords, const QT3DSVec2 &inViewportDimensions, bool inPickEverything)
    {
        Qt3DSRenderPickResult thePickResult;

        if (m_Presentation.m_Scene && m_Presentation.m_Scene->m_FirstChild) {
            thePickResult = m_RenderContext.GetRenderer().Pick(
                *m_Presentation.m_Scene->m_FirstChild, inViewportDimensions,
                QT3DSVec2(inMouseCoords.x, inMouseCoords.y), true, inPickEverything);
        }
        return thePickResult;
    }
}
}
