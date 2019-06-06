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

#include "Qt3DSRenderLayer.h"
#include "Qt3DSRenderShadowMap.h"
#include "Qt3DSRenderResourceManager.h"
#include "rendererimpl/Qt3DSRendererImplLayerRenderData.h"
#include "render/Qt3DSRenderShaderConstant.h"
#include "render/Qt3DSRenderShaderProgram.h"

using namespace qt3ds::render;
using qt3ds::render::NVRenderContextScopedProperty;
using qt3ds::render::NVRenderCachedShaderProperty;

Qt3DSShadowMap::Qt3DSShadowMap(IQt3DSRenderContext &inContext)
    : m_Context(inContext)
    , mRefCount(0)
    , m_ShadowMapList(inContext.GetAllocator(), "Qt3DSShadowMap::m_ShadowMapList")
{
}

Qt3DSShadowMap::~Qt3DSShadowMap()
{
    m_ShadowMapList.clear();
}

namespace {
bool IsDepthFormat(NVRenderTextureFormats::Enum format)
{
    switch (format) {
    case NVRenderTextureFormats::Depth16:
    case NVRenderTextureFormats::Depth24:
    case NVRenderTextureFormats::Depth32:
    case NVRenderTextureFormats::Depth24Stencil8:
        return true;
    default:
        return false;
    }
}
}

void Qt3DSShadowMap::AddShadowMapEntry(QT3DSU32 index, QT3DSU32 width, QT3DSU32 height,
                                     NVRenderTextureFormats::Enum format, QT3DSU32 samples,
                                     ShadowMapModes::Enum mode, ShadowFilterValues::Enum filter)
{
    IResourceManager &theManager(m_Context.GetResourceManager());
    SShadowMapEntry *pEntry = NULL;

    if (index < m_ShadowMapList.size())
        pEntry = &m_ShadowMapList[index];

    if (pEntry) {
        if ((NULL != pEntry->m_DepthMap) && (mode == ShadowMapModes::CUBE)) {
            theManager.Release(*pEntry->m_DepthMap);
            theManager.Release(*pEntry->m_DepthCopy);
            theManager.Release(*pEntry->m_DepthRender);
            pEntry->m_DepthCube = theManager.AllocateTextureCube(width, height, format, samples);
            pEntry->m_CubeCopy = theManager.AllocateTextureCube(width, height, format, samples);
            pEntry->m_DepthRender = theManager.AllocateTexture2D(
                width, height, NVRenderTextureFormats::Depth24Stencil8, samples);
            pEntry->m_DepthMap = NULL;
            pEntry->m_DepthCopy = NULL;
        } else if ((NULL != pEntry->m_DepthCube) && (mode != ShadowMapModes::CUBE)) {
            theManager.Release(*pEntry->m_DepthCube);
            theManager.Release(*pEntry->m_CubeCopy);
            theManager.Release(*pEntry->m_DepthRender);
            pEntry->m_DepthMap = theManager.AllocateTexture2D(width, height, format, samples);
            pEntry->m_DepthCopy = theManager.AllocateTexture2D(width, height, format, samples);
            pEntry->m_DepthCube = NULL;
            pEntry->m_CubeCopy = NULL;
            pEntry->m_DepthRender = theManager.AllocateTexture2D(
                width, height, NVRenderTextureFormats::Depth24Stencil8, samples);
        } else if (NULL != pEntry->m_DepthMap) {
            STextureDetails theDetails(pEntry->m_DepthMap->GetTextureDetails());

            // If anything differs about the map we're looking for, let's recreate it.
            if (theDetails.m_Format != format || theDetails.m_Width != width
                || theDetails.m_Height != height || theDetails.m_SampleCount != samples) {
                // release texture
                theManager.Release(*pEntry->m_DepthMap);
                theManager.Release(*pEntry->m_DepthCopy);
                theManager.Release(*pEntry->m_DepthRender);
                pEntry->m_DepthMap = theManager.AllocateTexture2D(width, height, format, samples);
                pEntry->m_DepthCopy = theManager.AllocateTexture2D(width, height, format, samples);
                pEntry->m_DepthCube = NULL;
                pEntry->m_CubeCopy = NULL;
                pEntry->m_DepthRender = theManager.AllocateTexture2D(
                    width, height, NVRenderTextureFormats::Depth24Stencil8, samples);
            }
        } else {
            STextureDetails theDetails(pEntry->m_DepthCube->GetTextureDetails());

            // If anything differs about the map we're looking for, let's recreate it.
            if (theDetails.m_Format != format || theDetails.m_Width != width
                || theDetails.m_Height != height || theDetails.m_SampleCount != samples) {
                // release texture
                theManager.Release(*pEntry->m_DepthCube);
                theManager.Release(*pEntry->m_CubeCopy);
                theManager.Release(*pEntry->m_DepthRender);
                pEntry->m_DepthCube =
                    theManager.AllocateTextureCube(width, height, format, samples);
                pEntry->m_CubeCopy = theManager.AllocateTextureCube(width, height, format, samples);
                pEntry->m_DepthRender = theManager.AllocateTexture2D(
                    width, height, NVRenderTextureFormats::Depth24Stencil8, samples);
                pEntry->m_DepthMap = NULL;
                pEntry->m_DepthCopy = NULL;
            }
        }

        pEntry->m_ShadowMapMode = mode;
        pEntry->m_ShadowFilterFlags = filter;
    } else if (mode == ShadowMapModes::CUBE) {
        NVRenderTextureCube *theDepthTex =
            theManager.AllocateTextureCube(width, height, format, samples);
        NVRenderTextureCube *theDepthCopy =
            theManager.AllocateTextureCube(width, height, format, samples);
        NVRenderTexture2D *theDepthTemp = theManager.AllocateTexture2D(
            width, height, NVRenderTextureFormats::Depth24Stencil8, samples);

        m_ShadowMapList.push_back(
            SShadowMapEntry(index, mode, filter, *theDepthTex, *theDepthCopy, *theDepthTemp));

        pEntry = &m_ShadowMapList.back();
    } else {
        NVRenderTexture2D *theDepthMap =
            theManager.AllocateTexture2D(width, height, format, samples);
        NVRenderTexture2D *theDepthCopy =
            theManager.AllocateTexture2D(width, height, format, samples);
        NVRenderTexture2D *theDepthTemp = theManager.AllocateTexture2D(
            width, height, NVRenderTextureFormats::Depth24Stencil8, samples);

        m_ShadowMapList.push_back(
            SShadowMapEntry(index, mode, filter, *theDepthMap, *theDepthCopy, *theDepthTemp));

        pEntry = &m_ShadowMapList.back();
    }

    if (pEntry) {
        // setup some texture settings
        if (pEntry->m_DepthMap) {
            pEntry->m_DepthMap->SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Linear);
            pEntry->m_DepthMap->SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Linear);
            pEntry->m_DepthMap->SetTextureWrapS(NVRenderTextureCoordOp::ClampToEdge);
            pEntry->m_DepthMap->SetTextureWrapT(NVRenderTextureCoordOp::ClampToEdge);

            pEntry->m_DepthCopy->SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Linear);
            pEntry->m_DepthCopy->SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Linear);
            pEntry->m_DepthCopy->SetTextureWrapS(NVRenderTextureCoordOp::ClampToEdge);
            pEntry->m_DepthCopy->SetTextureWrapT(NVRenderTextureCoordOp::ClampToEdge);

            pEntry->m_DepthRender->SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Linear);
            pEntry->m_DepthRender->SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Linear);
            pEntry->m_DepthRender->SetTextureWrapS(NVRenderTextureCoordOp::ClampToEdge);
            pEntry->m_DepthRender->SetTextureWrapT(NVRenderTextureCoordOp::ClampToEdge);
        } else {
            pEntry->m_DepthCube->SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Linear);
            pEntry->m_DepthCube->SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Linear);
            pEntry->m_DepthCube->SetTextureWrapS(NVRenderTextureCoordOp::ClampToEdge);
            pEntry->m_DepthCube->SetTextureWrapT(NVRenderTextureCoordOp::ClampToEdge);

            pEntry->m_CubeCopy->SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Linear);
            pEntry->m_CubeCopy->SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Linear);
            pEntry->m_CubeCopy->SetTextureWrapS(NVRenderTextureCoordOp::ClampToEdge);
            pEntry->m_CubeCopy->SetTextureWrapT(NVRenderTextureCoordOp::ClampToEdge);

            pEntry->m_DepthRender->SetMinFilter(qt3ds::render::NVRenderTextureMinifyingOp::Linear);
            pEntry->m_DepthRender->SetMagFilter(qt3ds::render::NVRenderTextureMagnifyingOp::Linear);
            pEntry->m_DepthRender->SetTextureWrapS(NVRenderTextureCoordOp::ClampToEdge);
            pEntry->m_DepthRender->SetTextureWrapT(NVRenderTextureCoordOp::ClampToEdge);
        }

        pEntry->m_LightIndex = index;
    }
}

SShadowMapEntry *Qt3DSShadowMap::GetShadowMapEntry(QT3DSU32 index)
{
    SShadowMapEntry *pEntry = NULL;

    for (QT3DSU32 i = 0; i < m_ShadowMapList.size(); i++) {
        pEntry = &m_ShadowMapList[i];
        if (pEntry->m_LightIndex == index)
            return pEntry;
    }

    return NULL;
}

Qt3DSShadowMap *Qt3DSShadowMap::Create(IQt3DSRenderContext &inContext)
{
    return QT3DS_NEW(inContext.GetFoundation().getAllocator(), Qt3DSShadowMap)(inContext);
}
