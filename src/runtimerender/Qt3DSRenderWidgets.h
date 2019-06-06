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
#ifndef QT3DS_RENDER_WIDGETS_H
#define QT3DS_RENDER_WIDGETS_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSMat44.h"
#include "foundation/Qt3DSMat33.h"
#include "foundation/Qt3DSBounds3.h"
#include "foundation/Qt3DSVec3.h"
#include "EASTL/utility.h"
#include "foundation/Qt3DSDataRef.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "render/Qt3DSRenderIndexBuffer.h"
#include "Qt3DSRenderText.h"

namespace qt3ds {
namespace render {

    struct SWidgetRenderInformation
    {
        // Just the rotation component of the nodeparenttocamera.
        QT3DSMat33 m_NormalMatrix;
        // The node parent's global transform multiplied by the inverse camera global transfrom;
        // basically the MV from model-view-projection
        QT3DSMat44 m_NodeParentToCamera;
        // Projection that accounts for layer scaling
        QT3DSMat44 m_LayerProjection;
        // Pure camera projection without layer scaling
        QT3DSMat44 m_PureProjection;
        // A look at matrix that will rotate objects facing directly up
        // the Z axis such that the point to the camera.
        QT3DSMat33 m_LookAtMatrix;
        // Conversion from world to camera position so world points not in object
        // local space can be converted to camera space without going through the node's
        // inverse global transform
        QT3DSMat44 m_CameraGlobalInverse;
        // Offset to add to the node's world position in camera space to move to the ideal camera
        // location so that scale will work.  This offset should be added *after* translation into
        // camera space
        QT3DSVec3 m_WorldPosOffset;
        // Position in camera space to center the widget around
        QT3DSVec3 m_Position;
        // Scale factor to scale the widget by.
        QT3DSF32 m_Scale;

        // The camera used to render this object.
        SCamera *m_Camera;
        SWidgetRenderInformation(const QT3DSMat33 &inNormal, const QT3DSMat44 &inNodeParentToCamera,
                                 const QT3DSMat44 &inLayerProjection, const QT3DSMat44 &inProjection,
                                 const QT3DSMat33 &inLookAt, const QT3DSMat44 &inCameraGlobalInverse,
                                 const QT3DSVec3 &inWorldPosOffset, const QT3DSVec3 &inPos, QT3DSF32 inScale,
                                 SCamera &inCamera)
            : m_NormalMatrix(inNormal)
            , m_NodeParentToCamera(inNodeParentToCamera)
            , m_LayerProjection(inLayerProjection)
            , m_PureProjection(inProjection)
            , m_LookAtMatrix(inLookAt)
            , m_CameraGlobalInverse(inCameraGlobalInverse)
            , m_WorldPosOffset(inWorldPosOffset)
            , m_Position(inPos)
            , m_Scale(inScale)
            , m_Camera(&inCamera)
        {
        }
        SWidgetRenderInformation()
            : m_Camera(NULL)
        {
        }
    };
    typedef eastl::pair<SShaderVertexCodeGenerator &, SShaderFragmentCodeGenerator &>
        TShaderGeneratorPair;

    struct RenderWidgetModes
    {
        enum Enum {
            Local,
            Global,
        };
    };
    // Context used to get render data for the widget.
    class IRenderWidgetContext
    {
    protected:
        virtual ~IRenderWidgetContext() {}
    public:
        virtual NVRenderVertexBuffer &
        GetOrCreateVertexBuffer(CRegisteredString &inStr, QT3DSU32 stride,
                                NVConstDataRef<QT3DSU8> bufferData = NVConstDataRef<QT3DSU8>()) = 0;
        virtual NVRenderIndexBuffer &
        GetOrCreateIndexBuffer(CRegisteredString &inStr,
                               qt3ds::render::NVRenderComponentTypes::Enum componentType, size_t size,
                               NVConstDataRef<QT3DSU8> bufferData = NVConstDataRef<QT3DSU8>()) = 0;
        virtual NVRenderAttribLayout &
        CreateAttributeLayout(NVConstDataRef<qt3ds::render::NVRenderVertexBufferEntry> attribs) = 0;
        virtual NVRenderInputAssembler &
        GetOrCreateInputAssembler(CRegisteredString &inStr, NVRenderAttribLayout *attribLayout,
                                  NVConstDataRef<NVRenderVertexBuffer *> buffers,
                                  const NVRenderIndexBuffer *indexBuffer,
                                  NVConstDataRef<QT3DSU32> strides, NVConstDataRef<QT3DSU32> offsets) = 0;

        virtual NVRenderVertexBuffer *GetVertexBuffer(CRegisteredString &inStr) = 0;
        virtual NVRenderIndexBuffer *GetIndexBuffer(CRegisteredString &inStr) = 0;
        virtual NVRenderInputAssembler *GetInputAssembler(CRegisteredString &inStr) = 0;

        virtual NVRenderShaderProgram *GetShader(CRegisteredString inStr) = 0;
        virtual IShaderProgramGenerator &GetProgramGenerator() = 0;
        // calls compile on the program generator and stores result under this name.
        virtual NVRenderShaderProgram *CompileAndStoreShader(CRegisteredString inStr) = 0;
        virtual STextDimensions MeasureText(const STextRenderInfo &inText) = 0;
        // Render text using a specific MVP
        virtual void RenderText(const STextRenderInfo &inText, const QT3DSVec3 &inTextColor,
                                const QT3DSVec3 &inBackgroundColor, const QT3DSMat44 &inMVP) = 0;
        // Given a node and a point in the node's local space (most likely its pivot point), we
        // return
        // a normal matrix so you can get the axis out, a transformation from node to camera
        // a new position and a floating point scale factor so you can render in 1/2 perspective
        // mode
        // or orthographic mode if you would like to.
        virtual SWidgetRenderInformation
        GetWidgetRenderInformation(SNode &inNode, const QT3DSVec3 &inPos,
                                   RenderWidgetModes::Enum inWidgetMode) = 0;
    };

    class IRenderWidget
    {
    protected:
        virtual ~IRenderWidget() {}
        SNode *m_Node;

    public:
        IRenderWidget(SNode &inNode)
            : m_Node(&inNode)
        {
        }
        IRenderWidget()
            : m_Node(NULL)
        {
        }
        virtual void Render(IRenderWidgetContext &inWidgetContext,
                            NVRenderContext &inRenderContext) = 0;
        SNode &GetNode() { return *m_Node; }

        // Pure widgets.
        static IRenderWidget &CreateBoundingBoxWidget(SNode &inNode, const NVBounds3 &inBounds,
                                                      const QT3DSVec3 &inColor,
                                                      NVAllocatorCallback &inAlloc);
        static IRenderWidget &CreateAxisWidget(SNode &inNode, NVAllocatorCallback &inAlloc);
    };
}
}

#endif
