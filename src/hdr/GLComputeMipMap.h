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

#ifndef GLCOMPUTE_BSDF_MIMAP_H
#define GLCOMPUTE_BSDF_MIMAP_H

#include "render/Qt3DSRenderBaseTypes.h"
#include "render/backends/Qt3DSRenderBackend.h"
#include "MipmapBSDF.h"

#include "Qt3DSRenderLoadedTexture.h"
#include "Qt3DSTypes.h"

using namespace qt3ds::render;

class qt3ds::render::NVRenderContext;
class qt3ds::render::NVRenderShaderProgram;
class qt3ds::render::NVRenderTexture2D;

class GLComputeMipMap : public BSDFMipMap
{
public:
    GLComputeMipMap(NVRenderContext *inNVRenderContext, int inWidth, int inHeight,
                    NVRenderTexture2D &inTexture, NVRenderTextureFormats::Enum inDestFormat,
                    qt3ds::NVFoundationBase &inFnd);
    ~GLComputeMipMap();
    void Build(void *inTextureData, int inTextureDataSize,
               NVRenderBackend::NVRenderBackendTextureObject inTextureHandle,
               NVRenderTextureFormats::Enum inFormat);
    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation);

private:
    void CreateLevel0Tex(void *inTextureData, int inTextureDataSize,
                         NVRenderTextureFormats::Enum inFormat);

    NVScopedRefCounted<NVRenderShaderProgram> m_BSDFProgram;
    NVScopedRefCounted<NVRenderShaderProgram> m_UploadProgram_RGBA8;
    NVScopedRefCounted<NVRenderShaderProgram> m_UploadProgram_RGB8;
    NVScopedRefCounted<NVRenderTexture2D> m_Level0Tex;
    bool m_TextureCreated;

    void createComputeProgram(NVRenderContext *context);
    NVRenderShaderProgram *getOrCreateUploadComputeProgram(NVRenderContext *context,
                                                           NVRenderTextureFormats::Enum inFormat);
};

#endif
