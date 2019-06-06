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
#include "Qt3DSOldNBustedRenderPlugin.h"
#include "SystemPrefix.h"
#include "Qt3DSDLLManager.h"
#include "render/Qt3DSRenderContext.h"

#ifdef WIN32
#pragma warning(disable : 4355) // this used in initializer list.  I have never seen this result in
                                // a physical error
#endif

namespace qt3ds {
namespace render {

    COldNBustedPluginRenderer::COldNBustedPluginRenderer(IQt3DSRenderContext &inRenderContext,
                                                         long inDLLHandle)
        : m_RenderContext(inRenderContext)
        , m_DLLHandle(inDLLHandle)
        , mRefCount(0)
        , m_OffscreenRendererType(inRenderContext.GetStringTable().RegisterStr(GetRendererName()))
    {
        if (m_DLLHandle != -1) {
            Q3DStudio::CDLLManager &theManager = Q3DStudio::CDLLManager::GetDLLManager();

            // Grab function procs
            m_GetTextureSizeProc = reinterpret_cast<PROC_GetDesiredTextureSize>(
                theManager.GetProc("GetDesiredTextureSize", m_DLLHandle));
            Q3DStudio_ASSERT(m_GetTextureSizeProc);

            m_RenderProc = reinterpret_cast<PROC_Render>(theManager.GetProc("Render", m_DLLHandle));
            Q3DStudio_ASSERT(m_RenderProc);
        }
    }

    NVRenderTextureFormats::Enum convertTextureFormat(ETEXTUREFORMAT fmt)
    {
        NVRenderTextureFormats::Enum ret = NVRenderTextureFormats::RGBA8;
        switch (fmt) {

        case ETEXTUREFORMAT_RGBA8:
            break;

        case ETEXTUREFORMAT_RGB8:
            ret = NVRenderTextureFormats::RGB8;
            break;

        default:
            break;

        }
        return ret;
    }

    SOffscreenRendererEnvironment
    COldNBustedPluginRenderer::GetDesiredEnvironment(QT3DSVec2 inPresScale)
    {
        long width, height;
        ETEXTUREFORMAT format;

        m_GetTextureSizeProc(&width, &height, &format);

        return SOffscreenRendererEnvironment(
            (QT3DSU32)(width * inPresScale.x), (QT3DSU32)(height * inPresScale.y),
            convertTextureFormat(format), OffscreenRendererDepthValues::Depth24, false,
            AAModeValues::NoAA);
    }

    SOffscreenRenderFlags
    COldNBustedPluginRenderer::NeedsRender(const SOffscreenRendererEnvironment & /*inEnvironment*/,
                                           QT3DSVec2 /*inPresScale*/,
                                           const SRenderInstanceId)
    {
        return SOffscreenRenderFlags(true, true);
    }

    // Returns true if the rendered result image has transparency, or false
    // if it should be treated as a completely opaque image.
    void COldNBustedPluginRenderer::Render(const SOffscreenRendererEnvironment &inEnvironment,
                                           NVRenderContext &inRenderContext, QT3DSVec2 /*inPresScale*/,
                                           SScene::RenderClearCommand /*inClearColorBuffer*/,
                                           const SRenderInstanceId)
    {
        inRenderContext.PushPropertySet();

        m_RenderProc(inEnvironment.m_Width, inEnvironment.m_Height, 1);

        inRenderContext.PopPropertySet(true);
    }
}
}
