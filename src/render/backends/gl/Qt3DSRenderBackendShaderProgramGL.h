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
#ifndef QT3DS_RENDER_BACKEND_SHADER_PROGRAM_GL_H
#define QT3DS_RENDER_BACKEND_SHADER_PROGRAM_GL_H

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Utils.h"
#include "render/Qt3DSRenderBaseTypes.h"

namespace qt3ds {
namespace render {

    struct NVRenderBackendShaderInputEntryGL
    {
        CRegisteredString m_AttribName; ///< must be the same name as used in the vertex shader
        QT3DSU32 m_AttribLocation; ///< attribute index
        QT3DSU32 m_Type; ///< GL vertex format type @sa GL_FLOAT, GL_INT
        QT3DSU32 m_NumComponents; ///< component count. max 4
    };

    ///< this class handles the shader input variables
    class NVRenderBackendShaderInputGL
    {
    public:
        ///< constructor
        NVRenderBackendShaderInputGL(NVDataRef<NVRenderBackendShaderInputEntryGL> entries)
            : m_ShaderInputEntries(entries)
        {
        }
        ///< destructor
        ~NVRenderBackendShaderInputGL(){}

        NVRenderBackendShaderInputEntryGL *getEntryByName(CRegisteredString entryName) const
        {
            QT3DS_FOREACH(idx, m_ShaderInputEntries.size())
            {
                if (m_ShaderInputEntries[idx].m_AttribName == entryName)
                    return &m_ShaderInputEntries.mData[idx];
            }
            return NULL;
        }

        Option<NVRenderBackendShaderInputEntryGL>
        getEntryByAttribLocation(QT3DSU32 attribLocation) const
        {
            QT3DS_FOREACH(idx, m_ShaderInputEntries.size())
            {
                if (m_ShaderInputEntries[idx].m_AttribLocation == attribLocation)
                    return m_ShaderInputEntries[idx];
            }
            return Empty();
        }

        NVDataRef<NVRenderBackendShaderInputEntryGL> m_ShaderInputEntries; ///< shader input entries
    };

    ///< this class represents the internals of a GL program
    class NVRenderBackendShaderProgramGL
    {
    public:
        ///< constructor
        NVRenderBackendShaderProgramGL(QT3DSU32 programID)
            : m_ProgramID(programID)
            , m_shaderInput(NULL)
        {
        }

        ///< destructor
        ~NVRenderBackendShaderProgramGL(){}

        QT3DSU32 m_ProgramID; ///< this is the OpenGL object ID
        NVRenderBackendShaderInputGL *m_shaderInput; ///< pointer to shader input object
    };
}
}

#endif
