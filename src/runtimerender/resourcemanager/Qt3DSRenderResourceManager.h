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
#pragma once
#ifndef QT3DS_RENDER_RESOURCE_MANAGER_H
#define QT3DS_RENDER_RESOURCE_MANAGER_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "render/Qt3DSRenderBaseTypes.h"

namespace qt3ds {
namespace render {
    /**
     *	Implements simple pooling of render resources
     */
    class IResourceManager : public NVRefCounted
    {
    protected:
        virtual ~IResourceManager() {}

    public:
        virtual NVRenderFrameBuffer *AllocateFrameBuffer() = 0;
        virtual void Release(NVRenderFrameBuffer &inBuffer) = 0;
        virtual NVRenderRenderBuffer *
        AllocateRenderBuffer(QT3DSU32 inWidth, QT3DSU32 inHeight,
                             NVRenderRenderBufferFormats::Enum inBufferFormat) = 0;
        virtual void Release(NVRenderRenderBuffer &inBuffer) = 0;
        virtual NVRenderTexture2D *AllocateTexture2D(QT3DSU32 inWidth, QT3DSU32 inHeight,
                                                     NVRenderTextureFormats::Enum inTextureFormat,
                                                     QT3DSU32 inSampleCount = 1,
                                                     bool immutable = false) = 0;
        virtual void Release(NVRenderTexture2D &inBuffer) = 0;
        virtual NVRenderTexture2DArray *
        AllocateTexture2DArray(QT3DSU32 inWidth, QT3DSU32 inHeight, QT3DSU32 inSlices,
                               NVRenderTextureFormats::Enum inTextureFormat,
                               QT3DSU32 inSampleCount = 1) = 0;
        virtual void Release(NVRenderTexture2DArray &inBuffer) = 0;
        virtual NVRenderTextureCube *
        AllocateTextureCube(QT3DSU32 inWidth, QT3DSU32 inHeight,
                            NVRenderTextureFormats::Enum inTextureFormat,
                            QT3DSU32 inSampleCount = 1) = 0;
        virtual void Release(NVRenderTextureCube &inBuffer) = 0;
        virtual NVRenderImage2D *AllocateImage2D(NVRenderTexture2D *inTexture,
                                                 NVRenderImageAccessType::Enum inAccess) = 0;
        virtual void Release(NVRenderImage2D &inBuffer) = 0;

        virtual NVRenderContext &GetRenderContext() = 0;
        virtual void DestroyFreeSizedResources() = 0;

        static IResourceManager &CreateResourceManager(NVRenderContext &inContext);
    };
}
}

#endif
