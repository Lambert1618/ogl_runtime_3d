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
#ifndef QT3DS_RENDER_DEPTH_STENCIL_STATE_H
#define QT3DS_RENDER_DEPTH_STENCIL_STATE_H
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
namespace render {

    class NVRenderContextImpl;

    // currently this handles only stencil state
    class NVRenderDepthStencilState : public NVRefCounted
    {

    private:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation
        NVRenderBackend *m_Backend; ///< pointer to backend
        NVRenderBackend::NVRenderBackendDepthStencilStateObject
            m_StateHandle; ///< opaque backend handle

    public:
        /**
         * @brief constructor
         *
         * @param[in] context				Pointer to context
         * @param[in] fnd					Pointer to foundation
         * @param[in] enableDepth			enable depth test
         * @param[in] depthMask				enable depth writes
         * @param[in] depthFunc				depth compare function
         * @param[in] enableStencil			enable stencil test
         * @param[in] stencilFuncFront		stencil setup front faces
         * @param[in] stencilFuncBack		stencil setup back faces
         * @param[in] depthStencilOpFront	depth/stencil operations front faces
         * @param[in] depthStencilOpBack	depth/stencil operations back faces
         *
         * @return No return.
         */
        NVRenderDepthStencilState(NVRenderContextImpl &context, NVFoundationBase &fnd,
                                  bool enableDepth, bool depthMask, NVRenderBoolOp::Enum depthFunc,
                                  bool enableStencil,
                                  NVRenderStencilFunctionArgument &stencilFuncFront,
                                  NVRenderStencilFunctionArgument &stencilFuncBack,
                                  NVRenderStencilOperationArgument &depthStencilOpFront,
                                  NVRenderStencilOperationArgument &depthStencilOpBack);

        virtual ~NVRenderDepthStencilState();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        ///< various get functions
        const NVRenderStencilFunctionArgument GetStencilFunc(NVRenderFaces::Enum face) const
        {
            return (face == NVRenderFaces::Back) ? m_StencilFuncBack : m_StencilFuncFront;
        }
        const NVRenderStencilOperationArgument GetStencilOp(NVRenderFaces::Enum face) const
        {
            return (face == NVRenderFaces::Back) ? m_DepthStencilOpBack : m_DepthStencilOpFront;
        }
        NVRenderBoolOp::Enum GetDepthFunc() const { return m_DepthFunc; }
        bool GetDepthEnabled() const { return m_DepthEnabled; }
        bool GetStencilEnabled() const { return m_StencilEnabled; }
        bool GetDepthMask() const { return m_DepthMask; }

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        virtual NVRenderBackend::NVRenderBackendDepthStencilStateObject
        GetDepthStencilObjectHandle()
        {
            return m_StateHandle;
        }

        static NVRenderDepthStencilState *
        Create(NVRenderContextImpl &context, bool enableDepth, bool depthMask,
               NVRenderBoolOp::Enum depthFunc, bool enableStencil,
               NVRenderStencilFunctionArgument &stencilFuncFront,
               NVRenderStencilFunctionArgument &stencilFuncBack,
               NVRenderStencilOperationArgument &depthStencilOpFront,
               NVRenderStencilOperationArgument &depthStencilOpBack);

    private:
        bool m_DepthEnabled; ///< depth test enabled
        bool m_DepthMask; ///< depth writes enabled
        NVRenderBoolOp::Enum m_DepthFunc; ///< depth comparison func
        bool m_StencilEnabled; ///< stencil test enabled
        NVRenderStencilFunctionArgument m_StencilFuncFront; ///< stencil setup front faces
        NVRenderStencilFunctionArgument m_StencilFuncBack; ///< stencil setup back faces
        NVRenderStencilOperationArgument
            m_DepthStencilOpFront; ///< depth stencil operation front faces
        NVRenderStencilOperationArgument
            m_DepthStencilOpBack; ///< depth stencil operation back faces
    };
}
}

#endif
