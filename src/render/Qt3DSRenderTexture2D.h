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
#ifndef QT3DS_RENDER_QT3DS_RENDER_TEXTURE_2D_H
#define QT3DS_RENDER_QT3DS_RENDER_TEXTURE_2D_H
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

    class NVRenderTexture2D : public NVRenderTextureBase, public NVRenderImplemented
    {

    private:
        QT3DSU32 m_Width; ///< texture width
        QT3DSU32 m_Height; ///< texture height

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
        NVRenderTexture2D(
            NVRenderContextImpl &context, NVFoundationBase &fnd,
            NVRenderTextureTargetType::Enum texTarget = NVRenderTextureTargetType::Texture2D);

        virtual ~NVRenderTexture2D();

        // Get the texture details for mipmap level 0 if it was set.
        STextureDetails GetTextureDetails() const override;

        /**
         * @brief Create GL texture object and upload data
         *
         * @param[in] newBuffer			Texture data for level 0
         * @param[in] inMipLevel		Texture level count
         * @param[in] width				Texture width
         * @param[in] height			Texture height
         * @param[in] format			Texture data format
         * @param[in] formaInternal		Texture internal format
         *
         * @return No return.
         */
        virtual void SetTextureData(
            NVDataRef<QT3DSU8> newBuffer, QT3DSU8 inMipLevel, QT3DSU32 width, QT3DSU32 height,
            NVRenderTextureFormats::Enum format,
            NVRenderTextureFormats::Enum formaInternal = NVRenderTextureFormats::Unknown);

        /**
         * @brief Create memory storage for a texture object
         *		  This create a texture storage which is immutable in size and format
         *		  Use this for textures used within compute shaders
         *
         * @param[in] inLevels			Texture level count
         * @param[in] width				Texture width
         * @param[in] height			Texture height
         * @param[in] formaInternal		Texture internal format
         * @param[in] format			Texture data format of dataBuffer
         * @param[in] dataBuffer		Texture data for level 0
         *
         * @return No return.
         */
        virtual void
        SetTextureStorage(QT3DSU32 inLevels, QT3DSU32 width, QT3DSU32 height,
                          NVRenderTextureFormats::Enum formaInternal,
                          NVRenderTextureFormats::Enum format = NVRenderTextureFormats::Unknown,
                          NVDataRef<QT3DSU8> dataBuffer = NVDataRef<QT3DSU8>());

        virtual void SetTextureDataMultisample(QT3DSU32 sampleCount, QT3DSU32 width, QT3DSU32 height,
                                               NVRenderTextureFormats::Enum format);

        bool IsMultisampleTexture() const override
        {
            return (m_TexTarget == NVRenderTextureTargetType::Texture2D_MS);
        }
        QT3DSU32 GetSampleCount() const override { return m_SampleCount; }
        bool IsImmutableTexture() const override { return m_Immutable; }

        // Update a sub-rect of the image.  newBuffer is expected to be a continguous subrect of the
        // image.
        virtual void SetTextureSubData(NVDataRef<QT3DSU8> newBuffer, QT3DSU8 inMipLevel, QT3DSU32 inXOffset,
                                       QT3DSU32 inYOffset, QT3DSU32 inSubImageWidth,
                                       QT3DSU32 inSubImageHeight, NVRenderTextureFormats::Enum format);
        // Generate a set of mipmaps from mipLevel( 0 ).  Uses the graphis layer to do this if
        // possible
        // glGenerateMipmap
        virtual void GenerateMipmaps(NVRenderHint::Enum genType = NVRenderHint::Nicest);

        /**
         * @brief Bind a texture for shader access
         *
         *
         * @return No return.
         */
        void Bind() override;

        QT3DSU32 GetNumMipmaps() override { return m_MaxMipLevel; }

        /**
         * @brief Query if texture needs coordinate swizzle
         *
         * @return texture swizzle mode
         */
        NVRenderTextureSwizzleMode::Enum GetTextureSwizzleMode() override
        {
            // if our backend supports hardware texture swizzle then there is no need for a shader
            // swizzle
            return (m_Backend->GetRenderBackendCap(
                       NVRenderBackend::NVRenderBackendCaps::TexSwizzle))
                ? NVRenderTextureSwizzleMode::NoSwizzle
                : m_Backend->GetTextureSwizzleMode(m_Format);
        }

        // this will be obsolete
        const void *GetImplementationHandle() const override
        {
            return reinterpret_cast<void *>(m_TextureHandle);
        }

        static NVRenderTexture2D *Create(NVRenderContextImpl &context);
    };
}
}

#endif
