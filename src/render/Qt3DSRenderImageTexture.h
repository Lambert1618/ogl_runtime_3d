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
#ifndef QT3DS_RENDER_QT3DS_RENDER_IMAGE_TEXTURE_H
#define QT3DS_RENDER_QT3DS_RENDER_IMAGE_TEXTURE_H
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
namespace render {

    class NVRenderContextImpl;
    class NVRenderTexture2D;

    // a wrapper class for NVRenderTexture2D
    // to use with compute shaders and load / store image shaders

    class NVRenderImage2D : public NVRefCounted
    {

    private:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend
        NVRenderTexture2D *m_Texture2D; ///< pointer to texture
        QT3DSI32 m_TextureUnit; ///< texture unit this texture should use
        NVRenderImageAccessType::Enum
            m_AccessType; ///< texture / image access type ( read, write, read_write )
        QT3DSU32 m_TextureLevel; ///< texture level we use for this image

    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to context
         * @param[in] fnd			Pointer to foundation
         * @param[in] inTexture		Pointer to a NVRenderTexture2D object
         * @param[in] inAccess		Image access type ( read, write, read_write )
         *
         * @return No return.
         */
        NVRenderImage2D(NVRenderContextImpl &context, NVFoundationBase &fnd,
                        NVRenderTexture2D *inTexture, NVRenderImageAccessType::Enum inAccess);

        virtual ~NVRenderImage2D();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        /**
         * @brief	Set the access rights within the shader.
         *			Can be read, write or read_write.
         *
         * @param[in] inAccess		Image access type ( read, write, read_write )
         *
         * @return No return.
         */
        virtual void SetAccessType(NVRenderImageAccessType::Enum inAccess)
        {
            m_AccessType = inAccess;
        }

        /**
         * @brief	Set the texture level we use for this image
         *
         * @param[in] inLevel		texture level ( must be in range of max levels )
         *
         * @return No return.
         */
        virtual void SetTextureLevel(QT3DSI32 inLevel);

        /**
         * @brief	Get texture unit used
         *
         *
         * @return texture unit bound to.
         */
        virtual QT3DSU32 GetTextureUnit() const { return m_TextureUnit; }

        /**
         * @brief Bind a texture for shader access
         *
         * @param[in] unit		The binding point
         *
         * @return No return.
         */
        virtual void Bind(QT3DSU32 unit);

        /**
         * @brief get the backend object handle
         *		  here we return the handle from the wrapped texture
         *
         * @return the backend object handle.
         */
        virtual NVRenderBackend::NVRenderBackendTextureObject GetTextureObjectHandle();

        /**
         * @brief static creation function
         *
         * @param[in] context		Pointer to context
         * @param[in] inTexture		Pointer to a NVRenderTexture2D object
         * @param[in] inAccess		Image access type ( read, write, read_write )
         *
         * @return No return.
         */
        static NVRenderImage2D *Create(NVRenderContextImpl &context, NVRenderTexture2D *inTexture,
                                       NVRenderImageAccessType::Enum inAccess);
    };
}
}

#endif
