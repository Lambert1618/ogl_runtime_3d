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

#include "Qt3DSRenderWidgets.h"
#include "Qt3DSRenderNode.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "render/Qt3DSRenderShaderProgram.h"

using namespace qt3ds::render;

namespace {

struct SWidgetBBox : public IRenderWidget
{
    NVBounds3 m_Bounds;
    QT3DSVec3 m_Color;
    NVRenderVertexBuffer *m_BoxVertexBuffer;
    NVRenderIndexBuffer *m_BoxIndexBuffer;
    NVRenderInputAssembler *m_BoxInputAssembler;
    NVRenderShaderProgram *m_BoxShader;
    CRegisteredString m_ItemName;
    SWidgetBBox(SNode &inNode, const NVBounds3 &inBounds, const QT3DSVec3 &inColor)
        : IRenderWidget(inNode)
        , m_Bounds(inBounds)
        , m_Color(inColor)
        , m_BoxVertexBuffer(NULL)
        , m_BoxIndexBuffer(NULL)
        , m_BoxInputAssembler(NULL)
        , m_BoxShader(NULL)
    {
    }

    void SetupBoxShader(IRenderWidgetContext &inContext)
    {
        m_BoxShader = inContext.GetShader(m_ItemName);
        if (!m_BoxShader) {
            qt3ds::render::IShaderProgramGenerator &theGenerator(inContext.GetProgramGenerator());
            theGenerator.BeginProgram();
            qt3ds::render::IShaderStageGenerator &theVertexGenerator(
                *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Vertex));
            qt3ds::render::IShaderStageGenerator &theFragmentGenerator(
                *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Fragment));

            theVertexGenerator.AddIncoming("attr_pos", "vec3");
            theVertexGenerator.AddUniform("model_view_projection", "mat4");
            theVertexGenerator.Append("void main() {");
            theVertexGenerator.Append(
                "\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
            theVertexGenerator.Append("}");
            theFragmentGenerator.AddUniform("output_color", "vec3");
            theFragmentGenerator.Append("void main() {");
            theFragmentGenerator.Append("\tgl_FragColor.rgb = output_color;");
            theFragmentGenerator.Append("\tgl_FragColor.a = 1.0;");
            theFragmentGenerator.Append("}");
            m_BoxShader = inContext.CompileAndStoreShader(m_ItemName);
        }
    }

    void SetupBoundingBoxGraphicsObjects(IRenderWidgetContext &inContext,
                                         NVDataRef<QT3DSVec3> thePoints)
    {
        qt3ds::render::NVRenderVertexBufferEntry theEntry(
            "attr_pos", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3);
        m_BoxVertexBuffer = &inContext.GetOrCreateVertexBuffer(
            m_ItemName, 3 * sizeof(QT3DSF32), toU8DataRef(thePoints.begin(), thePoints.size()));
        m_BoxIndexBuffer = inContext.GetIndexBuffer(m_ItemName);
        if (!m_BoxIndexBuffer) {
            // The way the bounds lays out the bounds for the box
            // capitalization indicates whether this was a max or min value.
            enum _Indexes {
                xyz = 0,
                Xyz,
                xYz,
                xyZ,
                XYZ,
                xYZ,
                XyZ,
                XYz,
            };
            QT3DSU8 indexes[] = {
                // The toBoxBounds function lays out points such that
                // xyz, Xyz, xYz, xyZ, XYZ, xYZ, XyZ, XYz
                // Min corner
                xyz, Xyz, xyz, xYz, xyz, xyZ,

                // Max corner
                XYZ, xYZ, XYZ, XyZ, XYZ, XYz,

                // Now connect the rest of the dots.
                // the rules are that only one letter can change
                // else you are connecting *across* the box somehow.

                Xyz, XYz, Xyz, XyZ,

                xYz, XYz, xYz, xYZ,

                xyZ, XyZ, xyZ, xYZ,
            };
            m_BoxIndexBuffer = &inContext.GetOrCreateIndexBuffer(
                m_ItemName, qt3ds::render::NVRenderComponentTypes::QT3DSU8, sizeof(indexes),
                toU8DataRef(indexes, sizeof(indexes)));
        }

        m_BoxInputAssembler = inContext.GetInputAssembler(m_ItemName);
        if (!m_BoxInputAssembler && m_BoxIndexBuffer && m_BoxVertexBuffer) {
            // create our attribute layout
            NVRenderAttribLayout *theAttribLAyout =
                &inContext.CreateAttributeLayout(toConstDataRef(&theEntry, 1));

            QT3DSU32 strides = m_BoxVertexBuffer->GetStride();
            QT3DSU32 offsets = 0;
            m_BoxInputAssembler = &inContext.GetOrCreateInputAssembler(
                m_ItemName, theAttribLAyout, toConstDataRef(&m_BoxVertexBuffer, 1),
                m_BoxIndexBuffer, toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
        }
        SetupBoxShader(inContext);
    }

    void Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext) override
    {
        m_ItemName = inRenderContext.GetStringTable().RegisterStr("SWidgetBBox");
        SWidgetRenderInformation theInfo(inWidgetContext.GetWidgetRenderInformation(
            *m_Node, m_Node->m_Position, RenderWidgetModes::Local));
        TNVBounds2BoxPoints thePoints;
        m_Bounds.expand(thePoints);
        QT3DSMat44 theNodeRotation;
        QT3DSMat44 theNodeToCamera = theInfo.m_NodeParentToCamera * m_Node->m_LocalTransform;
        for (QT3DSU32 idx = 0; idx < 8; ++idx)
            thePoints[idx] = theNodeToCamera.transform(thePoints[idx]);
        SetupBoundingBoxGraphicsObjects(inWidgetContext, toDataRef(thePoints, 8));
        if (m_BoxShader && m_BoxInputAssembler) {
            inRenderContext.SetBlendingEnabled(false);
            inRenderContext.SetDepthWriteEnabled(true);
            inRenderContext.SetDepthTestEnabled(true);
            inRenderContext.SetCullingEnabled(false);
            inRenderContext.SetActiveShader(m_BoxShader);
            m_BoxShader->SetPropertyValue("model_view_projection", theInfo.m_LayerProjection);
            m_BoxShader->SetPropertyValue("output_color", m_Color);
            inRenderContext.SetInputAssembler(m_BoxInputAssembler);
            inRenderContext.Draw(qt3ds::render::NVRenderDrawMode::Lines,
                                 m_BoxInputAssembler->GetIndexCount(), 0);
        }
    }
};

struct SWidgetAxis : public IRenderWidget
{
    NVRenderVertexBuffer *m_AxisVertexBuffer;
    NVRenderInputAssembler *m_AxisInputAssembler;
    NVRenderShaderProgram *m_AxisShader;
    CRegisteredString m_ItemName;

    SWidgetAxis(SNode &inNode)
        : IRenderWidget(inNode)
        , m_AxisVertexBuffer(NULL)
        , m_AxisInputAssembler(NULL)
        , m_AxisShader(NULL)
    {
    }

    void SetupAxisShader(IRenderWidgetContext &inContext)
    {
        m_AxisShader = inContext.GetShader(m_ItemName);
        if (!m_AxisShader) {
            qt3ds::render::IShaderProgramGenerator &theGenerator(inContext.GetProgramGenerator());
            theGenerator.BeginProgram();
            qt3ds::render::IShaderStageGenerator &theVertexGenerator(
                *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Vertex));
            qt3ds::render::IShaderStageGenerator &theFragmentGenerator(
                *theGenerator.GetStage(qt3ds::render::ShaderGeneratorStages::Fragment));
            theVertexGenerator.AddIncoming("attr_pos", "vec3");
            theVertexGenerator.AddIncoming("attr_color", "vec3");
            theVertexGenerator.AddOutgoing("output_color", "vec3");
            theVertexGenerator.AddUniform("model_view_projection", "mat4");
            theVertexGenerator.Append("void main() {");
            theVertexGenerator.Append(
                "\tgl_Position = model_view_projection * vec4(attr_pos, 1.0);");
            theVertexGenerator.Append("\toutput_color = attr_color;");
            theVertexGenerator.Append("}");
            theFragmentGenerator.Append("void main() {");
            theFragmentGenerator.Append("\tgl_FragColor.rgb = output_color;");
            theFragmentGenerator.Append("\tgl_FragColor.a = 1.0;");
            theFragmentGenerator.Append("}");
            m_AxisShader = inContext.CompileAndStoreShader(m_ItemName);
        }
    }

    void SetupAxesGraphicsObjects(IRenderWidgetContext &inContext, NVDataRef<QT3DSVec3> theAxes)
    {
        qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
            qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
            qt3ds::render::NVRenderVertexBufferEntry("attr_color",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3, 12),
        };

        m_AxisVertexBuffer = &inContext.GetOrCreateVertexBuffer(
            m_ItemName, 6 * sizeof(QT3DSF32), toU8DataRef(theAxes.begin(), theAxes.size()));
        m_AxisInputAssembler = inContext.GetInputAssembler(m_ItemName);
        if (!m_AxisInputAssembler && m_AxisVertexBuffer) {
            // create our attribute layout
            NVRenderAttribLayout *theAttribLAyout =
                &inContext.CreateAttributeLayout(toConstDataRef(theEntries, 2));

            QT3DSU32 strides = m_AxisVertexBuffer->GetStride();
            QT3DSU32 offsets = 0;
            m_AxisInputAssembler = &inContext.GetOrCreateInputAssembler(
                m_ItemName, theAttribLAyout, toConstDataRef(&m_AxisVertexBuffer, 1), nullptr,
                toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
        }
    }

    inline QT3DSVec3 TransformDirection(const QT3DSMat33 &inMatrix, const QT3DSVec3 &inDir)
    {
        QT3DSVec3 retval = inMatrix.transform(inDir);
        return retval;
    }

    void Render(IRenderWidgetContext &inWidgetContext, NVRenderContext &inRenderContext) override
    {
        m_ItemName = inRenderContext.GetStringTable().RegisterStr("SWidgetAxis");

        SetupAxisShader(inWidgetContext);

        if (m_AxisShader) {
            static const QT3DSVec3 pivotCol = QT3DSVec3(0, 0, 1);
            if (m_Node->m_Parent && m_Node->m_Parent->m_Type != GraphObjectTypes::Layer) {
                m_Node->m_Parent->CalculateGlobalVariables();
            }
            QT3DSVec3 thePivot(m_Node->m_Pivot);
            if (m_Node->m_Flags.IsLeftHanded())
                thePivot.z *= -1;

            SWidgetRenderInformation theInfo(inWidgetContext.GetWidgetRenderInformation(
                *m_Node, QT3DSVec3(0, 0, 0), RenderWidgetModes::Local));

            QT3DSMat44 theNodeRotation;
            m_Node->CalculateRotationMatrix(theNodeRotation);
            if (m_Node->m_Flags.IsLeftHanded())
                SNode::FlipCoordinateSystem(theNodeRotation);

            QT3DSMat33 theRotationMatrix(theNodeRotation.column0.getXYZ(),
                                      theNodeRotation.column1.getXYZ(),
                                      theNodeRotation.column2.getXYZ());

            // Move the camera position into camera space.  This is so that when we render we don't
            // have to account
            // for scaling done in the camera's MVP.
            QT3DSVec3 theItemPosition = theInfo.m_Position;

            QT3DSMat33 theAxisTransform = theInfo.m_NormalMatrix * theRotationMatrix;

            // Scale the effective pivot line end point according to node scale
            // so that pivot line always hits object center.
            thePivot = thePivot.multiply(m_Node->m_Scale);
            QT3DSVec3 pivotVec = TransformDirection(
                        theAxisTransform, QT3DSVec3(-thePivot.x, -thePivot.y, -thePivot.z));

            QT3DSVec3 thePivotLine[] = {
                theItemPosition, pivotCol, theItemPosition + pivotVec, pivotCol
            };

            SetupAxesGraphicsObjects(inWidgetContext, toDataRef(thePivotLine, 4));

            if (m_AxisInputAssembler) {
                inRenderContext.SetBlendingEnabled(false);
                inRenderContext.SetDepthWriteEnabled(false);
                inRenderContext.SetDepthTestEnabled(false);
                inRenderContext.SetCullingEnabled(false);
                inRenderContext.SetActiveShader(m_AxisShader);
                m_AxisShader->SetPropertyValue("model_view_projection", theInfo.m_LayerProjection);
                inRenderContext.SetInputAssembler(m_AxisInputAssembler);
                // Draw line from pivot to object center.
                inRenderContext.Draw(qt3ds::render::NVRenderDrawMode::Lines, 2, 0);
            }
        }
    }
};
}

IRenderWidget &IRenderWidget::CreateBoundingBoxWidget(SNode &inNode, const NVBounds3 &inBounds,
                                                      const QT3DSVec3 &inColor,
                                                      NVAllocatorCallback &inAlloc)
{
    return *QT3DS_NEW(inAlloc, SWidgetBBox)(inNode, inBounds, inColor);
}

IRenderWidget &IRenderWidget::CreateAxisWidget(SNode &inNode, NVAllocatorCallback &inAlloc)
{
    return *QT3DS_NEW(inAlloc, SWidgetAxis)(inNode);
}
