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
#ifndef QT3DS_RENDER_PATH_SPECIFICATION_H
#define QT3DS_RENDER_PATH_SPECIFICATION_H
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "render/backends/Qt3DSRenderBackend.h"

namespace qt3ds {
namespace render {

    using namespace foundation;

    class NVRenderContextImpl;

    class NVRenderPathSpecification : public NVRefCounted
    {
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
         *
         * @return No return.
         */
        NVRenderPathSpecification(NVRenderContextImpl &context, NVFoundationBase &fnd);

        /// @NVRenderPathSpecification destructor
        ~NVRenderPathSpecification();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        /**
         * @brief reset commands and coordiantes
         *
         * @return No return.
         */
        virtual void Reset();

        /**
         * @brief add new move to command
         *
         * @param[in] inPoint		Coordinate
         *
         * @return No return.
         */
        virtual void MoveTo(QT3DSVec2 inPoint);

        /**
         * @brief add new cubic curve command
         *
         * @param[in] inC1		control point 1
         * @param[in] inC2		control point 2
         * @param[in] inDest	final point
         *
         * @return No return.
         */
        virtual void CubicCurveTo(QT3DSVec2 inC1, QT3DSVec2 inC2, QT3DSVec2 inDest);

        /**
         * @brief add new close command
         *
         *
         * @return No return.
         */
        virtual void ClosePath();

        /**
         * @brief Get path command list
         *
         *
         * @return path commands
         */
        virtual NVConstDataRef<QT3DSU8> GetPathCommands() { return m_PathCommands; }

        /**
         * @brief Get path coordinates list
         *
         *
         * @return path coordinates
         */
        virtual NVConstDataRef<QT3DSF32> GetPathCoords() { return m_PathCoords; }

    private:
        nvvector<QT3DSU8> m_PathCommands;
        nvvector<QT3DSF32> m_PathCoords;

        /**
         * @brief add a new point to the coordinates
         *
         * @param[in] inPoint		Coordinate
         *
         * @return No return.
         */
        void P(QT3DSVec2 inData);

    public:
        static NVRenderPathSpecification *CreatePathSpecification(NVRenderContextImpl &context);
    };
}
}

#endif