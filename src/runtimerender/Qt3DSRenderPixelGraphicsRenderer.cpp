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
#include "Qt3DSRenderPixelGraphicsRenderer.h"
#include "Qt3DSRenderPixelGraphicsTypes.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderShaderCodeGenerator.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "Qt3DSRenderShaderCache.h"

using namespace qt3ds;
using namespace qt3ds::render;

namespace {

struct SPGRectShader
{
    NVScopedRefCounted<NVRenderShaderProgram> m_RectShader;
    NVRenderShaderConstantBase *mvp;
    NVRenderShaderConstantBase *rectColor;
    NVRenderShaderConstantBase *leftright;
    NVRenderShaderConstantBase *bottomtop;

    SPGRectShader()
        : mvp(NULL)
        , rectColor(NULL)
        , leftright(NULL)
        , bottomtop(NULL)
    {
    }
    void SetShader(NVRenderShaderProgram *program)
    {
        m_RectShader = program;
        if (program) {
            mvp = program->GetShaderConstant("model_view_projection");
            rectColor = program->GetShaderConstant("rect_color");
            leftright = program->GetShaderConstant("leftright[0]");
            bottomtop = program->GetShaderConstant("bottomtop[0]");
        }
    }

    void Apply(QT3DSMat44 &inVP, const SPGRect &inObject)
    {
        if (mvp)
            m_RectShader->SetConstantValue(mvp, toConstDataRef(inVP), 1);
        if (rectColor)
            m_RectShader->SetConstantValue(rectColor, inObject.m_FillColor, 1);
        if (leftright) {
            QT3DSF32 theData[] = { inObject.m_Left, inObject.m_Right };
            m_RectShader->SetConstantValue(leftright, *theData, 2);
        }
        if (bottomtop) {
            QT3DSF32 theData[] = { inObject.m_Bottom, inObject.m_Top };
            m_RectShader->SetConstantValue(bottomtop, *theData, 2);
        }
    }

    operator bool() { return m_RectShader.mPtr != NULL; }
};

struct SPGRenderer : public IPixelGraphicsRenderer
{
    IQt3DSRenderContext &m_RenderContext;
    IStringTable &m_StringTable;
    NVScopedRefCounted<NVRenderVertexBuffer> m_QuadVertexBuffer;
    NVScopedRefCounted<NVRenderIndexBuffer> m_QuadIndexBuffer;
    NVScopedRefCounted<NVRenderInputAssembler> m_QuadInputAssembler;
    NVScopedRefCounted<NVRenderAttribLayout> m_QuadAttribLayout;
    SShaderVertexCodeGenerator m_VertexGenerator;
    SShaderFragmentCodeGenerator m_FragmentGenerator;
    SPGRectShader m_RectShader;
    QT3DSI32 mRefCount;

    SPGRenderer(IQt3DSRenderContext &ctx, IStringTable &strt)
        : m_RenderContext(ctx)
        , m_StringTable(strt)
        , m_VertexGenerator(m_StringTable, ctx.GetAllocator(),
                            m_RenderContext.GetRenderContext().GetRenderContextType())
        , m_FragmentGenerator(m_VertexGenerator, ctx.GetAllocator(),
                              m_RenderContext.GetRenderContext().GetRenderContextType())
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_RenderContext.GetAllocator())
    void GetRectShaderProgram()
    {
        if (!m_RectShader) {
            m_VertexGenerator.Begin();
            m_FragmentGenerator.Begin();
            m_VertexGenerator.AddAttribute("attr_pos", "vec2");
            m_VertexGenerator.AddUniform("model_view_projection", "mat4");
            m_VertexGenerator.AddUniform("leftright[2]", "float");
            m_VertexGenerator.AddUniform("bottomtop[2]", "float");
            m_FragmentGenerator.AddVarying("rect_uvs", "vec2");
            m_FragmentGenerator.AddUniform("rect_color", "vec4");
            m_VertexGenerator << "void main() {" << Endl
                              << "\tgl_Position = model_view_projection * vec4( "
                                 "leftright[int(attr_pos.x)], bottomtop[int(attr_pos.y)], 0.0, 1.0 "
                                 ");"
                              << Endl << "\trect_uvs = attr_pos;" << Endl << "}" << Endl;

            m_FragmentGenerator << "void main() {" << Endl << "\tfragOutput = rect_color;" << Endl
                                << "}" << Endl;

            m_VertexGenerator.BuildShaderSource();
            m_FragmentGenerator.BuildShaderSource();

            m_RectShader.SetShader(m_RenderContext.GetShaderCache().CompileProgram(
                m_StringTable.RegisterStr("PixelRectShader"),
                m_VertexGenerator.m_FinalShaderBuilder.c_str(),
                m_FragmentGenerator.m_FinalShaderBuilder.c_str(), NULL // no tess control shader
                ,
                NULL // no tess eval shader
                ,
                NULL // no geometry shader
                ,
                SShaderCacheProgramFlags(), ShaderCacheNoFeatures()));
        }
    }
    void GenerateXYQuad()
    {
        NVRenderContext &theRenderContext(m_RenderContext.GetRenderContext());

        qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
            qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                  qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2),
        };

        QT3DSVec2 pos[] = { QT3DSVec2(0, 0), QT3DSVec2(0, 1), QT3DSVec2(1, 1), QT3DSVec2(1, 0) };

        if (m_QuadVertexBuffer == NULL) {
            size_t bufSize = sizeof(pos);
            m_QuadVertexBuffer = theRenderContext.CreateVertexBuffer(
                qt3ds::render::NVRenderBufferUsageType::Static, bufSize, 2 * sizeof(QT3DSF32),
                toU8DataRef(pos, 4));
        }

        if (m_QuadIndexBuffer == NULL) {
            QT3DSU8 indexData[] = {
                0, 1, 2, 0, 2, 3,
            };
            m_QuadIndexBuffer = theRenderContext.CreateIndexBuffer(
                qt3ds::render::NVRenderBufferUsageType::Static,
                qt3ds::render::NVRenderComponentTypes::QT3DSU8, sizeof(indexData),
                toU8DataRef(indexData, sizeof(indexData)));
        }

        if (m_QuadAttribLayout == NULL) {
            // create our attribute layout
            m_QuadAttribLayout =
                theRenderContext.CreateAttributeLayout(toConstDataRef(theEntries, 1));
        }

        if (m_QuadInputAssembler == NULL) {

            // create input assembler object
            QT3DSU32 strides = m_QuadVertexBuffer->GetStride();
            QT3DSU32 offsets = 0;
            m_QuadInputAssembler = theRenderContext.CreateInputAssembler(
                m_QuadAttribLayout, toConstDataRef(&m_QuadVertexBuffer.mPtr, 1), m_QuadIndexBuffer,
                toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
        }
    }

    void RenderPixelObject(QT3DSMat44 &inProjection, const SPGRect &inObject)
    {
        GenerateXYQuad();
        GetRectShaderProgram();
        if (m_RectShader) {
            m_RenderContext.GetRenderContext().SetActiveShader(m_RectShader.m_RectShader.mPtr);
            m_RectShader.Apply(inProjection, inObject);

            m_RenderContext.GetRenderContext().SetInputAssembler(m_QuadInputAssembler.mPtr);
            m_RenderContext.GetRenderContext().Draw(NVRenderDrawMode::Triangles,
                                                    m_QuadInputAssembler->GetIndexCount(), 0);
        }
    }

    void RenderPixelObject(QT3DSMat44 &inProjection, const SPGVertLine &inObject)
    {
        // lines are really just rects, but they grow in width in a sort of odd way.
        // specifically, they grow the increasing coordinate on even boundaries and centered on odd
        // boundaries.
        SPGRect theRect;
        theRect.m_Top = inObject.m_Top;
        theRect.m_Bottom = inObject.m_Bottom;
        theRect.m_FillColor = inObject.m_LineColor;
        theRect.m_Left = inObject.m_X;
        theRect.m_Right = theRect.m_Left + 1.0f;
        RenderPixelObject(inProjection, theRect);
    }

    void RenderPixelObject(QT3DSMat44 &inProjection, const SPGHorzLine &inObject)
    {
        SPGRect theRect;
        theRect.m_Right = inObject.m_Right;
        theRect.m_Left = inObject.m_Left;
        theRect.m_FillColor = inObject.m_LineColor;
        theRect.m_Bottom = inObject.m_Y;
        theRect.m_Top = theRect.m_Bottom + 1.0f;
        RenderPixelObject(inProjection, theRect);
    }

    void Render(NVConstDataRef<SPGGraphObject *> inObjects) override
    {
        NVRenderContext &theRenderContext(m_RenderContext.GetRenderContext());
        theRenderContext.PushPropertySet();
        // Setup an orthographic camera that places the center at the
        // lower left of the viewport.
        NVRenderRectF theViewport = theRenderContext.GetViewport();
        // With no projection at all, we are going to get a square view box
        // with boundaries from -1,1 in all dimensions.  This is close to what we want.
        theRenderContext.SetDepthTestEnabled(false);
        theRenderContext.SetDepthWriteEnabled(false);
        theRenderContext.SetScissorTestEnabled(false);
        theRenderContext.SetBlendingEnabled(true);
        theRenderContext.SetCullingEnabled(false);
        // Colors are expected to be non-premultiplied, so we premultiply alpha into them at this
        // point.
        theRenderContext.SetBlendFunction(qt3ds::render::NVRenderBlendFunctionArgument(
            NVRenderSrcBlendFunc::SrcAlpha, NVRenderDstBlendFunc::OneMinusSrcAlpha,
            NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha));
        theRenderContext.SetBlendEquation(qt3ds::render::NVRenderBlendEquationArgument(
            NVRenderBlendEquation::Add, NVRenderBlendEquation::Add));

        SCamera theCamera;
        theCamera.m_Position.z = -5;
        theCamera.m_ClipNear = 1.0f;
        theCamera.m_ClipFar = 10.0f;
        theCamera.m_Flags.SetOrthographic(true);
        // Setup camera projection
        theCamera.ComputeFrustumOrtho(theViewport,
                                      QT3DSVec2(theViewport.m_Width, theViewport.m_Height));
        // Translate such that 0, 0 is lower left of screen.
        NVRenderRectF theIdealViewport = theViewport;
        theIdealViewport.m_X -= theViewport.m_Width / 2.0f;
        theIdealViewport.m_Y -= theViewport.m_Height / 2.0f;
        QT3DSMat44 theProjectionMatrix = NVRenderContext::ApplyVirtualViewportToProjectionMatrix(
            theCamera.m_Projection, theViewport, theIdealViewport);
        theCamera.m_Projection = theProjectionMatrix;
        // Explicitly call the node's calculate global variables so that the camera doesn't attempt
        // to change the projection we setup.
        static_cast<SNode &>(theCamera).CalculateGlobalVariables();
        QT3DSMat44 theVPMatrix(QT3DSMat44::createIdentity());
        theCamera.CalculateViewProjectionMatrix(theVPMatrix);

        QT3DSVec4 theTest(60, 200, 0, 1);
        QT3DSVec4 theResult = theVPMatrix.transform(theTest);

        (void)theTest;
        (void)theResult;

        for (QT3DSU32 idx = 0, end = inObjects.size(); idx < end; ++idx) {
            const SPGGraphObject &theObject(*inObjects[idx]);

            switch (theObject.m_Type) {
            case SGTypes::VertLine:
                RenderPixelObject(theVPMatrix, static_cast<const SPGVertLine &>(theObject));
                break;
            case SGTypes::HorzLine:
                RenderPixelObject(theVPMatrix, static_cast<const SPGHorzLine &>(theObject));
                break;
            case SGTypes::Rect:
                RenderPixelObject(theVPMatrix, static_cast<const SPGRect &>(theObject));
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }

        theRenderContext.PopPropertySet(false);
    }
};
}

IPixelGraphicsRenderer &IPixelGraphicsRenderer::CreateRenderer(IQt3DSRenderContext &ctx,
                                                               IStringTable &strt)
{
    return *QT3DS_NEW(ctx.GetAllocator(), SPGRenderer)(ctx, strt);
}
