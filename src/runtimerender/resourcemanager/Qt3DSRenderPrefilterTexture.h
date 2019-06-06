/****************************************************************************
**
** Copyright (C) 2008-2016 NVIDIA Corporation.
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
#pragma once
#ifndef QT3DS_RENDER_PREFILTER_TEXTURE_H
#define QT3DS_RENDER_PREFILTER_TEXTURE_H
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "Qt3DSRender.h"

#include "Qt3DSTypes.h"
#include "Qt3DSRenderLoadedTexture.h"

namespace qt3ds {
namespace render {

    class Qt3DSRenderPrefilterTexture : public NVRefCounted
    {
    public:
        Qt3DSRenderPrefilterTexture(NVRenderContext *inNVRenderContext, QT3DSI32 inWidth, QT3DSI32 inHeight,
                                  NVRenderTexture2D &inTexture,
                                  NVRenderTextureFormats::Enum inDestFormat,
                                  qt3ds::NVFoundationBase &inFnd);
        virtual ~Qt3DSRenderPrefilterTexture();

        virtual void Build(void *inTextureData, QT3DSI32 inTextureDataSize,
                           NVRenderTextureFormats::Enum inFormat) = 0;

        static Qt3DSRenderPrefilterTexture *Create(NVRenderContext *inNVRenderContext, QT3DSI32 inWidth,
                                                 QT3DSI32 inHeight, NVRenderTexture2D &inTexture,
                                                 NVRenderTextureFormats::Enum inDestFormat,
                                                 qt3ds::NVFoundationBase &inFnd);

    protected:
        NVFoundationBase &m_Foundation; ///< Foundation class for allocations and other base things
        volatile QT3DSI32 mRefCount; ///< reference count

        NVRenderTexture2D &m_Texture2D;
        NVRenderTextureFormats::Enum m_InternalFormat;
        NVRenderTextureFormats::Enum m_DestinationFormat;

        QT3DSI32 m_Width;
        QT3DSI32 m_Height;
        QT3DSI32 m_MaxMipMapLevel;
        QT3DSI32 m_SizeOfFormat;
        QT3DSI32 m_SizeOfInternalFormat;
        QT3DSI32 m_InternalNoOfComponent;
        QT3DSI32 m_NoOfComponent;
        NVRenderContext *m_NVRenderContext;
    };

    class Qt3DSRenderPrefilterTextureCPU : public Qt3DSRenderPrefilterTexture
    {
    public:
        Qt3DSRenderPrefilterTextureCPU(NVRenderContext *inNVRenderContext, QT3DSI32 inWidth,
                                     QT3DSI32 inHeight, NVRenderTexture2D &inTexture,
                                     NVRenderTextureFormats::Enum inDestFormat,
                                     qt3ds::NVFoundationBase &inFnd);

        void Build(void *inTextureData, QT3DSI32 inTextureDataSize,
                   NVRenderTextureFormats::Enum inFormat) override;

        STextureData CreateBsdfMipLevel(STextureData &inCurMipLevel, STextureData &inPrevMipLevel,
                                        QT3DSI32 width, QT3DSI32 height);
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

        int wrapMod(int a, int base);
        void getWrappedCoords(int &sX, int &sY, int width, int height);
    };

    class Qt3DSRenderPrefilterTextureCompute : public Qt3DSRenderPrefilterTexture
    {
    public:
        Qt3DSRenderPrefilterTextureCompute(NVRenderContext *inNVRenderContext, QT3DSI32 inWidth,
                                         QT3DSI32 inHeight, NVRenderTexture2D &inTexture,
                                         NVRenderTextureFormats::Enum inDestFormat,
                                         qt3ds::NVFoundationBase &inFnd);
        ~Qt3DSRenderPrefilterTextureCompute();

        void Build(void *inTextureData, QT3DSI32 inTextureDataSize,
                   NVRenderTextureFormats::Enum inFormat) override;

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

    private:
        void CreateLevel0Tex(void *inTextureData, QT3DSI32 inTextureDataSize,
                             NVRenderTextureFormats::Enum inFormat);

        NVScopedRefCounted<NVRenderShaderProgram> m_BSDFProgram;
        NVScopedRefCounted<NVRenderShaderProgram> m_UploadProgram_RGBA8;
        NVScopedRefCounted<NVRenderShaderProgram> m_UploadProgram_RGB8;
        NVScopedRefCounted<NVRenderTexture2D> m_Level0Tex;
        bool m_TextureCreated;

        void createComputeProgram(NVRenderContext *context);
        NVRenderShaderProgram *
        getOrCreateUploadComputeProgram(NVRenderContext *context,
                                        NVRenderTextureFormats::Enum inFormat);
    };
}
}

#endif
