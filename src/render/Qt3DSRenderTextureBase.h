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
#ifndef QT3DS_RENDER_QT3DS_RENDER_TEXTURE_BUFFER_H
#define QT3DS_RENDER_QT3DS_RENDER_TEXTURE_BUFFER_H
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
namespace render {

    class NVRenderContextImpl;
    class NVRenderTextureSampler;

    struct STextureDetails
    {
        QT3DSU32 m_Width;
        QT3DSU32 m_Height;
        QT3DSU32 m_Depth;
        QT3DSU32 m_SampleCount;
        NVRenderTextureFormats::Enum m_Format;

        STextureDetails(QT3DSU32 w, QT3DSU32 h, QT3DSU32 d, QT3DSU32 samples, NVRenderTextureFormats::Enum f)
            : m_Width(w)
            , m_Height(h)
            , m_Depth(d)
            , m_SampleCount(samples)
            , m_Format(f)
        {
        }
        STextureDetails()
            : m_Width(0)
            , m_Height(0)
            , m_Depth(0)
            , m_SampleCount(1)
            , m_Format(NVRenderTextureFormats::Unknown)
        {
        }
    };

    class NVRenderTextureBase : public NVRefCounted
    {

    protected:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend
        NVRenderBackend::NVRenderBackendTextureObject m_TextureHandle; ///< opaque backend handle
        QT3DSU32 m_TextureUnit; ///< texture unit this texture should use
        bool m_SamplerParamsDirty; ///< true if sampler state is dirty
        bool m_TexStateDirty; ///< true if texture object state is dirty
        QT3DSU32 m_SampleCount; ///< texture height
        NVRenderTextureFormats::Enum m_Format; ///< texture format
        NVRenderTextureTargetType::Enum m_TexTarget; ///< texture target
        NVRenderTextureSampler *m_Sampler; ///< current texture sampler state
        QT3DSI32 m_BaseLevel; ///< minimum lod specified
        QT3DSI32 m_MaxLevel; ///< maximum lod specified
        QT3DSU32 m_MaxMipLevel; ///< highest mip level
        bool m_Immutable; ///< true if this is a immutable texture ( size and format )

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
        NVRenderTextureBase(NVRenderContextImpl &context, NVFoundationBase &fnd,
                            NVRenderTextureTargetType::Enum texTarget);

        virtual ~NVRenderTextureBase();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        virtual void SetMinFilter(NVRenderTextureMinifyingOp::Enum value);
        virtual void SetMagFilter(NVRenderTextureMagnifyingOp::Enum value);

        virtual void SetBaseLevel(QT3DSI32 value);
        virtual void SetMaxLevel(QT3DSI32 value);

        virtual void SetTextureWrapS(NVRenderTextureCoordOp::Enum value);
        virtual void SetTextureWrapT(NVRenderTextureCoordOp::Enum value);

        virtual void SetTextureCompareMode(NVRenderTextureCompareMode::Enum value);
        virtual void SetTextureCompareFunc(NVRenderTextureCompareOp::Enum value);

        virtual void SetTextureUnit(QT3DSU32 unit) { m_TextureUnit = unit; }
        virtual QT3DSU32 GetTextureUnit() const { return m_TextureUnit; }

        // Get the texture details for mipmap level 0 if it was set.
        virtual STextureDetails GetTextureDetails() const = 0;

        virtual bool IsMultisampleTexture() const
        {
            return (m_TexTarget == NVRenderTextureTargetType::Texture2D_MS);
        }
        virtual QT3DSU32 GetSampleCount() const { return m_SampleCount; }
        virtual bool IsImmutableTexture() const { return m_Immutable; }

        /**
         * @brief Bind a texture for shader access
         *
         *
         * @return No return.
         */
        virtual void Bind() = 0;

        virtual QT3DSU32 GetNumMipmaps() { return m_MaxMipLevel; }

        /**
         * @brief Query if texture needs coordinate swizzle
         *
         * @return texture swizzle mode
         */
        virtual NVRenderTextureSwizzleMode::Enum GetTextureSwizzleMode()
        {
            // if our backend supports hardware texture swizzle then there is no need for a shader
            // swizzle
            return (m_Backend->GetRenderBackendCap(
                       NVRenderBackend::NVRenderBackendCaps::TexSwizzle))
                ? NVRenderTextureSwizzleMode::NoSwizzle
                : m_Backend->GetTextureSwizzleMode(m_Format);
        }

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        virtual NVRenderBackend::NVRenderBackendTextureObject GetTextureObjectHandle()
        {
            return m_TextureHandle;
        }

    protected:
        void applyTexParams();
        void applyTexSwizzle();
    };
}
}

#endif
