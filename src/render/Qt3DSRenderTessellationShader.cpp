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
#include "render/Qt3DSRenderTessellationShader.h"

namespace qt3ds {
namespace render {

    NVRenderTessControlShader::NVRenderTessControlShader(NVRenderContextImpl &context,
                                                         NVFoundationBase &fnd,
                                                         NVConstDataRef<QT3DSI8> source,
                                                         bool binaryProgram)
        : NVRenderShader(context, fnd, source, binaryProgram)
        , m_ShaderHandle(NULL)
    {
        m_ShaderHandle = m_Backend->CreateTessControlShader(source, m_ErrorMessage, binaryProgram);
    }

    NVRenderTessControlShader::~NVRenderTessControlShader()
    {
        if (m_ShaderHandle) {
            m_Backend->ReleaseTessControlShader(m_ShaderHandle);
        }
    }

    NVRenderTessEvaluationShader::NVRenderTessEvaluationShader(NVRenderContextImpl &context,
                                                               NVFoundationBase &fnd,
                                                               NVConstDataRef<QT3DSI8> source,
                                                               bool binaryProgram)
        : NVRenderShader(context, fnd, source, binaryProgram)
        , m_ShaderHandle(NULL)
    {
        m_ShaderHandle =
            m_Backend->CreateTessEvaluationShader(source, m_ErrorMessage, binaryProgram);
    }

    NVRenderTessEvaluationShader::~NVRenderTessEvaluationShader()
    {
        if (m_ShaderHandle) {
            m_Backend->ReleaseTessEvaluationShader(m_ShaderHandle);
        }
    }
}
}
