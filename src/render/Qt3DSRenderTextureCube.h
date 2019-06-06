/****************************************************************************
**
** Copyright (C) 2014 NVIDIA Corporation.
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
#ifndef QT3DS_RENDER_QT3DS_RENDER_TEXTURE_CUBE_H
#define QT3DS_RENDER_QT3DS_RENDER_TEXTURE_CUBE_H
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/backends/Qt3DSRenderBackend.h"
#include "render/Qt3DSRenderTextureBase.h"

namespace qt3ds {
namespace render {

    class NVRenderContextImpl;
    class NVRenderTextureSampler;

    class NVRenderTextureCube : public NVRenderTextureBase
    {
    private:
        QT3DSU32 m_Width; ///< texture width (per face)
        QT3DSU32 m_Height; ///< texture height (per face)

    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] fnd			Pointer to foundation
         * @param[in] texTarget		Texture target
         *
         * @return No return.
         */
        NVRenderTextureCube(
            NVRenderContextImpl &context, NVFoundationBase &fnd,
            NVRenderTextureTargetType::Enum texTarget = NVRenderTextureTargetType::TextureCube);

        virtual ~NVRenderTextureCube();

        /**
         * @brief constructor
         *
         * @param[in] newBuffer		Pointer to pixel buffer
         * @param[in] inMipLevel	Pointer to foundation
         * @param[in] width			Texture target
         * @param[in] height		Texture target
         * @param[in] slices		Texture target
         * @param[in] format		Texture target
         *
         * @return No return.
         */
        void SetTextureData(NVDataRef<QT3DSU8> newBuffer, QT3DSU8 inMipLevel,
                            NVRenderTextureCubeFaces::Enum inFace, QT3DSU32 width, QT3DSU32 height,
                            NVRenderTextureFormats::Enum format);

        // Get the texture details for mipmap level 0 if it was set.
        STextureDetails GetTextureDetails() const override;

        /**
         * @brief Bind a texture for shader access
         *
         *
         * @return No return.
         */
        void Bind() override;

        /**
         * @brief create a texture array object
         *
         *
         * @ return a texture array object
         */
        static NVRenderTextureCube *Create(NVRenderContextImpl &context);
    };
}
}

#endif
