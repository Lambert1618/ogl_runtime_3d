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
#ifndef QT3DS_RENDER_PATH_RENDER_H
#define QT3DS_RENDER_PATH_RENDER_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/backends/Qt3DSRenderBackend.h"
#include "foundation/Qt3DSBounds3.h"
#include <EASTL/string.h>

namespace qt3ds {
namespace render {
    using namespace foundation;

    class NVRenderContextImpl;
    class NVRenderPathSpecification;
    class NVRenderPathFontSpecification;

    ///< A program pipeline is a collection of a multiple programs (vertex, fragment, geometry,....)
    class NVRenderPathRender : public NVRefCounted
    {
    protected:
        NVRenderContextImpl &m_Context; ///< pointer to context
        NVFoundationBase &m_Foundation; ///< pointer to foundation
        NVRenderBackend *m_Backend; ///< pointer to backend
        volatile QT3DSI32 mRefCount; ///< Using foundations' naming convention to ease implementation

    public:
        /**
         * @brief constructor
         *
         * @param[in] context		Pointer to render context
         * @param[in] fnd			Pointer to foundation
          * @param[in] range		Number of internal objects
         *
         * @return No return.
         */
        NVRenderPathRender(NVRenderContextImpl &context, NVFoundationBase &fnd, size_t range);

        /// @brief destructor
        ~NVRenderPathRender();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        NVRenderBackend::NVRenderBackendPathObject GetPathHandle() { return m_PathRenderHandle; }

        // The render context can create a path specification object.
        void SetPathSpecification(NVRenderPathSpecification &inCommandBuffer);

        NVBounds3 GetPathObjectBoundingBox();
        NVBounds3 GetPathObjectFillBox();
        NVBounds3 GetPathObjectStrokeBox();

        void SetStrokeWidth(QT3DSF32 inStrokeWidth);
        QT3DSF32 GetStrokeWidth() const;

        void StencilStroke();
        void StencilFill();

        /**
         * @brief static create function
         *
         * @param[in] context		Pointer to render context
         * @param[in] range			Number of internal objects
         *
         * @return the backend object handle.
         */
        static NVRenderPathRender *Create(NVRenderContextImpl &context, size_t range);

    private:
        NVRenderBackend::NVRenderBackendPathObject m_PathRenderHandle; ///< opaque backend handle
        size_t m_Range; ///< range of internal objects
        QT3DSF32 m_StrokeWidth;
    };
}
}

#endif
