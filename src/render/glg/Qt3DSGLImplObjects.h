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
#ifndef QT3DS_RENDER_GL_IMPL_OBJECTS_H
#define QT3DS_RENDER_GL_IMPL_OBJECTS_H
#include "render/backends/gl/Qt3DSOpenGLUtil.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSContainers.h"

namespace qt3ds {
namespace render {

    // The set of all properties as they are currently set in hardware.
    struct SNVGLHardPropertyContext
    {
        NVRenderFrameBuffer *m_FrameBuffer;
        NVRenderShaderProgram *m_ActiveShader;
        NVRenderProgramPipeline *m_ActiveProgramPipeline;
        NVRenderInputAssembler *m_InputAssembler;
        NVRenderBlendFunctionArgument m_BlendFunction;
        NVRenderBlendEquationArgument m_BlendEquation;
        bool m_CullingEnabled;
        NVRenderBoolOp::Enum m_DepthFunction;
        bool m_BlendingEnabled;
        bool m_DepthWriteEnabled;
        bool m_DepthTestEnabled;
        bool m_StencilTestEnabled;
        bool m_ScissorTestEnabled;
        bool m_ColorWritesEnabled;
        bool m_MultisampleEnabled;
        NVRenderRect m_ScissorRect;
        NVRenderRect m_Viewport;
        QT3DSVec4 m_ClearColor;

        SNVGLHardPropertyContext()
            : m_FrameBuffer(NULL)
            , m_ActiveShader(NULL)
            , m_ActiveProgramPipeline(NULL)
            , m_InputAssembler(NULL)
            , m_CullingEnabled(true)
            , m_DepthFunction(NVRenderBoolOp::Less)
            , m_BlendingEnabled(true)
            , m_DepthWriteEnabled(true)
            , m_DepthTestEnabled(true)
            , m_StencilTestEnabled(false)
            , m_ScissorTestEnabled(true)
            , m_ColorWritesEnabled(true)
            , m_MultisampleEnabled(false)
            , m_ClearColor(0, 0, 0, 1)
        {
        }
    };
}
}
#endif
