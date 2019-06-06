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
#ifndef QT3DS_RENDER_PROGRAM_PIPLINE_H
#define QT3DS_RENDER_PROGRAM_PIPLINE_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderContext.h"
#include "render/backends/Qt3DSRenderBackend.h"
#include <EASTL/string.h>

namespace qt3ds {
namespace render {
    using namespace foundation;

    class NVRenderContextImpl;
    class NVRenderShaderProgram;

    ///< A program pipeline is a collection of a multiple programs (vertex, fragment, geometry,....)
    class QT3DS_AUTOTEST_EXPORT NVRenderProgramPipeline : public NVRefCounted
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
         *
         * @return No return.
         */
        NVRenderProgramPipeline(NVRenderContextImpl &context, NVFoundationBase &fnd);

        /// @brief destructor
        ~NVRenderProgramPipeline();

        // define refcount functions
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation)

        /**
         * @brief Query if pipeline is valid
         *
         * @return True if valid.
         */
        bool IsValid();

        /**
         * @brief enable / disable a program stage in the pipeline
         *
         * @param[in] pProgram	Pointer to program. If NULL stage will be disabled
         * @param[in] flags		Flags to which stage this program is bound to. Can more than
         * one stage
         *
         * @return no return.
         */
        void SetProgramStages(NVRenderShaderProgram *pProgram, NVRenderShaderTypeFlags flags);

        /**
         * @brief Make the program pipeline active
         *
         * @return True if valid.
         */
        void Bind();

        /**
         * @brief get the backend object handle
         *
         * @return the backend object handle.
         */
        NVRenderBackend::NVRenderBackendProgramPipeline GetShaderHandle()
        {
            return m_ProgramPipelineHandle;
        }

        /**
         * @brief get the vertex stage program
         *
         * @return the backend object handle.
         */
        NVRenderShaderProgram *GetVertexStage() { return m_VertexProgram; }

    private:
        NVRenderBackend::NVRenderBackendProgramPipeline
            m_ProgramPipelineHandle; ///< opaque backend handle

        NVRenderShaderProgram
            *m_Program; ///< for non separable programs this contains the entire program
        NVRenderShaderProgram
            *m_VertexProgram; ///< for separable programs this contains the vertex program
        NVRenderShaderProgram
            *m_FragmentProgram; ///< for separable programs this contains the fragment program
        NVRenderShaderProgram *m_TessControlProgram; ///< for separable programs this contains the
                                                     ///tessellation control program
        NVRenderShaderProgram *m_TessEvalProgram; ///< for separable programs this contains the
                                                  ///tessellation evaluation program
        NVRenderShaderProgram
            *m_GeometryProgram; ///< for separable programs this contains the geometry program
        NVRenderShaderProgram
            *m_ComputProgram; ///< for separable programs this contains the compute program
    };
}
}

#endif
