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
#ifndef QT3DS_RENDER_MESH_H
#define QT3DS_RENDER_MESH_H
#include "Qt3DSRender.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "render/Qt3DSRenderIndexBuffer.h"
#include "render/Qt3DSRenderInputAssembler.h"
#include "foundation/Qt3DSBounds3.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSNoCopy.h"

namespace qt3ds {
namespace render {

    struct SRenderSubsetBase
    {
        QT3DSU32 m_Count;
        QT3DSU32 m_Offset;
        NVBounds3 m_Bounds; // Vertex buffer bounds
        SRenderSubsetBase() {}
        SRenderSubsetBase(const SRenderSubsetBase &inOther)
            : m_Count(inOther.m_Count)
            , m_Offset(inOther.m_Offset)
            , m_Bounds(inOther.m_Bounds)
        {
        }

        SRenderSubsetBase &operator=(const SRenderSubsetBase &inOther)
        {
            m_Count = inOther.m_Count;
            m_Offset = inOther.m_Offset;
            m_Bounds = inOther.m_Bounds;
            return *this;
        }
    };

    struct SRenderJoint
    {
        QT3DSI32 m_JointID;
        QT3DSI32 m_ParentID;
        QT3DSF32 m_invBindPose[16];
        QT3DSF32 m_localToGlobalBoneSpace[16];
    };

    struct SRenderSubset : public SRenderSubsetBase
    {
        NVRenderInputAssembler *m_InputAssembler;
        NVRenderInputAssembler *m_InputAssemblerDepth;
        NVRenderInputAssembler
            *m_InputAssemblerPoints; ///< similar to depth but ignores index buffer.
        NVRenderVertexBuffer *m_VertexBuffer;
        NVRenderVertexBuffer
            *m_PosVertexBuffer; ///< separate position buffer for fast depth path rendering
        NVRenderIndexBuffer *m_IndexBuffer;
        NVRenderDrawMode::Enum m_PrimitiveType; ///< primitive type used for drawing
        QT3DSF32 m_EdgeTessFactor; ///< edge tessellation amount used for tessellation shaders
        QT3DSF32 m_InnerTessFactor; ///< inner tessellation amount used for tessellation shaders
        bool m_WireframeMode; ///< true if we should draw the object as wireframe ( currently ony if
                              ///tessellation is enabled )
        NVConstDataRef<SRenderJoint> m_Joints;
        CRegisteredString m_Name;
        nvvector<SRenderSubsetBase> m_SubSubsets;

        SRenderSubset(NVAllocatorCallback &alloc)
            : m_InputAssembler(NULL)
            , m_InputAssemblerDepth(NULL)
            , m_InputAssemblerPoints(NULL)
            , m_VertexBuffer(NULL)
            , m_PosVertexBuffer(NULL)
            , m_IndexBuffer(NULL)
            , m_PrimitiveType(NVRenderDrawMode::Triangles)
            , m_EdgeTessFactor(1.0)
            , m_InnerTessFactor(1.0)
            , m_WireframeMode(false)
            , m_SubSubsets(alloc, "SRenderSubset::m_SubSubsets")
        {
        }
        SRenderSubset(const SRenderSubset &inOther)
            : SRenderSubsetBase(inOther)
            , m_InputAssembler(inOther.m_InputAssembler)
            , m_InputAssemblerDepth(inOther.m_InputAssemblerDepth)
            , m_InputAssemblerPoints(inOther.m_InputAssemblerPoints)
            , m_VertexBuffer(inOther.m_VertexBuffer)
            , m_PosVertexBuffer(inOther.m_PosVertexBuffer)
            , m_IndexBuffer(inOther.m_IndexBuffer)
            , m_PrimitiveType(inOther.m_PrimitiveType)
            , m_EdgeTessFactor(inOther.m_EdgeTessFactor)
            , m_InnerTessFactor(inOther.m_InnerTessFactor)
            , m_WireframeMode(inOther.m_WireframeMode)
            , m_Joints(inOther.m_Joints)
            , m_Name(inOther.m_Name)
            , m_SubSubsets(inOther.m_SubSubsets)
        {
        }
        // Note that subSubsets is *not* copied.
        SRenderSubset(NVAllocatorCallback &alloc, const SRenderSubset &inOther,
                      const SRenderSubsetBase &inBase)
            : SRenderSubsetBase(inBase)
            , m_InputAssembler(inOther.m_InputAssembler)
            , m_InputAssemblerDepth(inOther.m_InputAssemblerDepth)
            , m_InputAssemblerPoints(inOther.m_InputAssemblerPoints)
            , m_VertexBuffer(inOther.m_VertexBuffer)
            , m_PosVertexBuffer(inOther.m_PosVertexBuffer)
            , m_IndexBuffer(inOther.m_IndexBuffer)
            , m_PrimitiveType(inOther.m_PrimitiveType)
            , m_EdgeTessFactor(inOther.m_EdgeTessFactor)
            , m_InnerTessFactor(inOther.m_InnerTessFactor)
            , m_WireframeMode(inOther.m_WireframeMode)
            , m_Name(inOther.m_Name)
            , m_SubSubsets(alloc, "SRenderSubset::m_SubSubsets")
        {
        }

        SRenderSubset &operator=(const SRenderSubset &inOther)
        {
            if (this != &inOther) {
                SRenderSubsetBase::operator=(inOther);
                m_InputAssembler = inOther.m_InputAssembler;
                m_InputAssemblerDepth = inOther.m_InputAssemblerDepth;
                m_VertexBuffer = inOther.m_VertexBuffer;
                m_PosVertexBuffer = inOther.m_PosVertexBuffer;
                m_IndexBuffer = inOther.m_IndexBuffer;
                m_PrimitiveType = inOther.m_PrimitiveType;
                m_EdgeTessFactor = inOther.m_EdgeTessFactor;
                m_InnerTessFactor = inOther.m_InnerTessFactor;
                m_WireframeMode = inOther.m_WireframeMode;
                m_Joints = inOther.m_Joints;
                m_Name = inOther.m_Name;
                m_SubSubsets = inOther.m_SubSubsets;
            }
            return *this;
        }
    };

    struct SRenderMesh : public NoCopy
    {
        nvvector<SRenderSubset> m_Subsets;
        nvvector<SRenderJoint> m_Joints;
        NVRenderDrawMode::Enum m_DrawMode;
        NVRenderWinding::Enum m_Winding; // counterclockwise
        QT3DSU32 m_MeshId; // Id from the file of this mesh.

        SRenderMesh(NVRenderDrawMode::Enum inDrawMode, NVRenderWinding::Enum inWinding,
                    QT3DSU32 inMeshId, NVAllocatorCallback &alloc)
            : m_Subsets(alloc, "SRenderMesh::m_Subsets")
            , m_Joints(alloc, "SRenderMesh::Joints")
            , m_DrawMode(inDrawMode)
            , m_Winding(inWinding)
            , m_MeshId(inMeshId)
        {
        }
    };
}
}

#endif