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
#ifndef QT3DS_RENDER_QT3DS_RENDER_SAMPLER_H
#define QT3DS_RENDER_QT3DS_RENDER_SAMPLER_H
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
namespace render {

    class NVRenderContextImpl;

    class NVRenderTextureSampler
    {
    public:
        NVRenderTextureMinifyingOp::Enum m_MinFilter;
        NVRenderTextureMagnifyingOp::Enum m_MagFilter;
        NVRenderTextureCoordOp::Enum m_WrapS;
        NVRenderTextureCoordOp::Enum m_WrapT;
        NVRenderTextureCoordOp::Enum m_WrapR;
        NVRenderTextureSwizzleMode::Enum m_SwizzleMode;
        QT3DSF32 m_MinLod;
        QT3DSF32 m_MaxLod;
        QT3DSF32 m_LodBias;
        NVRenderTextureCompareMode::Enum m_CompareMode;
        NVRenderTextureCompareOp::Enum m_CompareOp;
        QT3DSF32 m_Anisotropy;
        QT3DSF32 m_BorderColor[4];

        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] fnd			Pointer to foundation
         * @param[in] minFilter		Texture min filter
         * @param[in] magFilter		Texture mag filter
         * @param[in] wrapS			Texture coord generation for S
         * @param[in] wrapT			Texture coord generation for T
         * @param[in] wrapR			Texture coord generation for R
         * @param[in] swizzleMode	Texture swizzle mode
         * @param[in] minLod		Texture min level of detail
         * @param[in] maxLod		Texture max level of detail
         * @param[in] lodBias		Texture level of detail bias (unused)
         * @param[in] compareMode	Texture compare mode
         * @param[in] compareFunc	Texture compare function
         * @param[in] anisoFilter	Aniso filter value [1.0, 16.0]
         * @param[in] borderColor	Texture border color float[4] (unused)
         *
         * @return No return.
         */
        NVRenderTextureSampler(
            NVRenderContextImpl &context, NVFoundationBase &fnd,
            NVRenderTextureMinifyingOp::Enum minFilter = NVRenderTextureMinifyingOp::Linear,
            NVRenderTextureMagnifyingOp::Enum magFilter = NVRenderTextureMagnifyingOp::Linear,
            NVRenderTextureCoordOp::Enum wrapS = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureCoordOp::Enum wrapT = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureCoordOp::Enum wrapR = NVRenderTextureCoordOp::ClampToEdge,
            NVRenderTextureSwizzleMode::Enum swizzleMode = NVRenderTextureSwizzleMode::NoSwizzle,
            QT3DSF32 minLod = -1000.0, QT3DSF32 maxLod = 1000.0, QT3DSF32 lodBias = 0.0,
            NVRenderTextureCompareMode::Enum compareMode = NVRenderTextureCompareMode::NoCompare,
            NVRenderTextureCompareOp::Enum compareFunc = NVRenderTextureCompareOp::LessThanOrEqual,
            QT3DSF32 anisotropy = 1.0, QT3DSF32 *borderColor = NULL);

        /**
         * @brief destructor
         *
         */
        virtual ~NVRenderTextureSampler();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        NVRenderBackend::NVRenderBackendSamplerObject GetSamplerHandle() const
        {
            return m_SamplerHandle;
        }

    private:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend
        NVRenderBackend::NVRenderBackendSamplerObject m_SamplerHandle; ///< opaque backend handle
    };
}
}

#endif
