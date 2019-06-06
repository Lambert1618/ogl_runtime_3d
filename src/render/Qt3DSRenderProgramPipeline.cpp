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

#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderProgramPipeline.h"
#include "render/Qt3DSRenderShaderProgram.h"

namespace qt3ds {
namespace render {

    NVRenderProgramPipeline::NVRenderProgramPipeline(NVRenderContextImpl &context,
                                                     NVFoundationBase &fnd)
        : m_Context(context)
        , m_Foundation(fnd)
        , m_Backend(context.GetBackend())
        , mRefCount(0)
        , m_Program(NULL)
        , m_VertexProgram(NULL)
        , m_FragmentProgram(NULL)
        , m_TessControlProgram(NULL)
        , m_TessEvalProgram(NULL)
        , m_GeometryProgram(NULL)
        , m_ComputProgram(NULL)
    {
        m_ProgramPipelineHandle = m_Backend->CreateProgramPipeline();
    }

    NVRenderProgramPipeline::~NVRenderProgramPipeline()
    {
        if (m_ProgramPipelineHandle) {
            m_Backend->ReleaseProgramPipeline(m_ProgramPipelineHandle);
        }

        if (m_VertexProgram)
            m_VertexProgram->release();
        if (m_FragmentProgram)
            m_FragmentProgram->release();
        if (m_TessControlProgram)
            m_TessControlProgram->release();
        if (m_TessEvalProgram)
            m_TessEvalProgram->release();
        if (m_GeometryProgram)
            m_GeometryProgram->release();
    }

    bool NVRenderProgramPipeline::IsValid() { return (m_ProgramPipelineHandle != NULL); }

    void NVRenderProgramPipeline::SetProgramStages(NVRenderShaderProgram *inProgram,
                                                   NVRenderShaderTypeFlags flags)
    {
        bool bDirty = false;

        if (flags & NVRenderShaderTypeValue::Vertex && inProgram != m_VertexProgram) {
            if (m_VertexProgram)
                m_VertexProgram->release();
            if (inProgram)
                inProgram->addRef();
            m_VertexProgram = inProgram;
            bDirty = true;
        }
        if (flags & NVRenderShaderTypeValue::Fragment && inProgram != m_FragmentProgram) {
            if (m_FragmentProgram)
                m_FragmentProgram->release();
            if (inProgram)
                inProgram->addRef();
            m_FragmentProgram = inProgram;
            bDirty = true;
        }
        if (flags & NVRenderShaderTypeValue::TessControl && inProgram != m_TessControlProgram) {
            if (m_TessControlProgram)
                m_TessControlProgram->release();
            if (inProgram)
                inProgram->addRef();
            m_TessControlProgram = inProgram;
            bDirty = true;
        }
        if (flags & NVRenderShaderTypeValue::TessEvaluation && inProgram != m_TessEvalProgram) {
            if (m_TessEvalProgram)
                m_TessEvalProgram->release();
            if (inProgram)
                inProgram->addRef();
            m_TessEvalProgram = inProgram;
            bDirty = true;
        }
        if (flags & NVRenderShaderTypeValue::Geometry && inProgram != m_GeometryProgram) {
            if (m_GeometryProgram)
                m_GeometryProgram->release();
            if (inProgram)
                inProgram->addRef();
            m_GeometryProgram = inProgram;
            bDirty = true;
        }

        if (bDirty) {
            m_Backend->SetProgramStages(m_ProgramPipelineHandle, flags,
                                        (inProgram) ? inProgram->GetShaderProgramHandle() : NULL);
        }
    }

    void NVRenderProgramPipeline::Bind()
    {
        m_Backend->SetActiveProgramPipeline(m_ProgramPipelineHandle);
    }
}
}
