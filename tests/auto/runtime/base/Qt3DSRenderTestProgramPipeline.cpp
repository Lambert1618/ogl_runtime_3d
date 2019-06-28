/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "Qt3DSRenderTestProgramPipeline.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderProgramPipeline.h"

#include <string>

using namespace qt3ds;
using namespace qt3ds::render;

struct Vertex
{
    QT3DSVec3 positions;
};

static const char *vertShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 430\n";

        prog += "out gl_PerVertex\n"
                "{\n"
                "\tvec4 gl_Position;\n"
                "\tfloat gl_PointSize;\n"
                "\tfloat gl_ClipDistance[];\n"
                "};\n";
    }

    prog += "uniform mat4 mat_mvp;\n"
            "in vec3 attr_pos;			// Vertex pos\n"
            "in vec3 attr_col;			// Vertex col\n"
            "out vec3 color;			// output color\n"
            "void main()\n"
            "{\n"
            "	gl_Position = mat_mvp * vec4(attr_pos, 1.0);\n"
            "   color = attr_col;\n"
            "}\n";

    return prog.c_str();
}

static const char *fragShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 430\n";
    }

    prog += "in vec3 color;			// input color\n"
            "out vec4 fragColor;\n"
            "void main()\n"
            "{\n"
            "  fragColor = vec4( color, 1.0);\n"
            "}\n";

    return prog.c_str();
}

NVRenderTestProgramPipeline::NVRenderTestProgramPipeline()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestProgramPipeline::~NVRenderTestProgramPipeline()
{
}

bool NVRenderTestProgramPipeline::isSupported(NVRenderContext *context)
{
    return context->IsProgramPipelineSupported();
}

////////////////////////////////
// test for functionality
////////////////////////////////

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

bool NVRenderTestProgramPipeline::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= vertFragSeparateTest(context, pUserData);
    _curTest++;
    success &= vertFragCombinedTest(context, pUserData);
    _curTest++;

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    return success;
}

bool NVRenderTestProgramPipeline::vertFragSeparateTest(NVRenderContext *context,
                                                       userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) },
                                              { QT3DSVec3(0.0, 0.9, 0) } };

    static const Vertex vertexColors[] = { { QT3DSVec3(0.0, 1.0, 0.0) },
                                           { QT3DSVec3(0.0, 1.0, 0.0) },
                                           { QT3DSVec3(0.0, 1.0, 0.0) } };

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderVertexBuffer> mColorBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderProgramPipeline> mProgramPipeline;
    NVRenderVertexBuffer *attribBuffers[2];
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);
    qt3ds::QT3DSVec3 color(0.0, 1.0, 0.0);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult vtxResult = context->CompileSource(
        "NVRenderTestProgramPipeline vertex shader",
        toRef(vertShader(vtxProg, isGLESContext(context))), NVConstDataRef<QT3DSI8>(),
        NVConstDataRef<QT3DSI8>(), NVConstDataRef<QT3DSI8>(), NVConstDataRef<QT3DSI8>(), true);
    if (!vtxResult.mShader) {
        return false;
    }

    NVRenderVertFragCompilationResult fragResult = context->CompileSource(
        "NVRenderTestProgramPipeline fragment shader", NVConstDataRef<QT3DSI8>(),
        toRef(fragShader(frgProg, isGLESContext(context))), NVConstDataRef<QT3DSI8>(),
        NVConstDataRef<QT3DSI8>(), NVConstDataRef<QT3DSI8>(), true);

    if (!fragResult.mShader) {
        return false;
    }

    // setup program pipeline
    mProgramPipeline = context->CreateProgramPipeline();
    if (!mProgramPipeline) {
        qWarning() << "NVRenderTestProgramPipeline: Failed to create program pipeline";
        return false;
    }

    mProgramPipeline->SetProgramStages(vtxResult.mShader,
                                       NVRenderShaderTypeFlags(NVRenderShaderTypeValue::Vertex));
    mProgramPipeline->SetProgramStages(fragResult.mShader,
                                       NVRenderShaderTypeFlags(NVRenderShaderTypeValue::Fragment));

    unsigned int curY = 0;
    unsigned int curX = _curTest;
    if (_curTest >= _maxColumn) {
        curY = (_curTest / _maxColumn);
        curX = (_curTest % _maxColumn);
    }

    // set viewport
    context->SetViewport(NVRenderRect(curX * _cellSize, curY * _cellSize, _cellSize, _cellSize));

    // this is the layout
    NVRenderVertexBufferEntry entries[] = {
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 3, 0, 0),
        NVRenderVertexBufferEntry("attr_col", NVRenderComponentTypes::QT3DSF32, 3, 0, 1),
    };

    // position buffer
    QT3DSU32 bufSize = 3 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                3 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestAttribBuffers: Failed to create vertex buffer";
        return false;
    }
    // color buffer
    NVDataRef<QT3DSU8> colorData((QT3DSU8 *)vertexColors, bufSize);
    mColorBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                               3 * sizeof(QT3DSF32), colorData);
    if (!mColorBuffer) {
        qWarning() << "NVRenderTestAttribBuffers: Failed to create color buffer";
        return false;
    }

    attribBuffers[0] = mVertexBuffer;
    attribBuffers[1] = mColorBuffer;

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 2));
    // create input Assembler
    QT3DSU32 strides[2];
    QT3DSU32 offsets[2];
    strides[0] = mVertexBuffer->GetStride();
    offsets[0] = 0;
    strides[1] = mColorBuffer->GetStride();
    offsets[1] = 0;

    mInputAssembler = context->CreateInputAssembler(
        mAttribLayout, NVConstDataRef<NVRenderVertexBuffer *>(attribBuffers, 2), NULL,
        toConstDataRef(strides, 2), toConstDataRef(offsets, 2), NVRenderDrawMode::Triangles);
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestAttribBuffers: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveProgramPipeline(mProgramPipeline);
    vtxResult.mShader->SetPropertyValue("mat_mvp", mvp);
    // set color
    vtxResult.mShader->SetPropertyValue("color", color);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(mInputAssembler->GetPrimitiveType(), 3, 0);

    context->SetActiveProgramPipeline(0);
    return true;
}

bool NVRenderTestProgramPipeline::vertFragCombinedTest(NVRenderContext *context,
                                                       userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) },
                                              { QT3DSVec3(0.0, 0.9, 0) } };

    static const Vertex vertexColors[] = { { QT3DSVec3(0.0, 1.0, 0.0) },
                                           { QT3DSVec3(0.0, 1.0, 0.0) },
                                           { QT3DSVec3(0.0, 1.0, 0.0) } };

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderVertexBuffer> mColorBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderProgramPipeline> mProgramPipeline;
    NVRenderVertexBuffer *attribBuffers[2];
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);
    qt3ds::QT3DSVec3 color(0.0, 1.0, 0.0);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestProgramPipeline vertex shader",
        toRef(vertShader(vtxProg, isGLESContext(context))),
        toRef(fragShader(frgProg, isGLESContext(context))), NVConstDataRef<QT3DSI8>(),
        NVConstDataRef<QT3DSI8>(), NVConstDataRef<QT3DSI8>(), true);
    if (!compResult.mShader) {
        return false;
    }

    // setup program pipeline
    mProgramPipeline = context->CreateProgramPipeline();
    if (!mProgramPipeline) {
        qWarning() << "NVRenderTestProgramPipeline: Failed to create program pipeline";
        return false;
    }

    mProgramPipeline->SetProgramStages(
        compResult.mShader, NVRenderShaderTypeFlags(NVRenderShaderTypeValue::Vertex
                                                    | NVRenderShaderTypeValue::Fragment));

    unsigned int curY = 0;
    unsigned int curX = _curTest;
    if (_curTest >= _maxColumn) {
        curY = (_curTest / _maxColumn);
        curX = (_curTest % _maxColumn);
    }

    // set viewport
    context->SetViewport(NVRenderRect(curX * _cellSize, curY * _cellSize, _cellSize, _cellSize));

    // this is the layout
    NVRenderVertexBufferEntry entries[] = {
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 3, 0, 0),
        NVRenderVertexBufferEntry("attr_col", NVRenderComponentTypes::QT3DSF32, 3, 0, 1),
    };

    // position buffer
    QT3DSU32 bufSize = 3 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                3 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestAttribBuffers: Failed to create vertex buffer";
        return false;
    }
    // color buffer
    NVDataRef<QT3DSU8> colorData((QT3DSU8 *)vertexColors, bufSize);
    mColorBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                               3 * sizeof(QT3DSF32), colorData);
    if (!mColorBuffer) {
        qWarning() << "NVRenderTestAttribBuffers: Failed to create color buffer";
        return false;
    }

    attribBuffers[0] = mVertexBuffer;
    attribBuffers[1] = mColorBuffer;

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 2));
    // create input Assembler
    QT3DSU32 strides[2];
    QT3DSU32 offsets[2];
    strides[0] = mVertexBuffer->GetStride();
    offsets[0] = 0;
    strides[1] = mColorBuffer->GetStride();
    offsets[1] = 0;

    mInputAssembler = context->CreateInputAssembler(
        mAttribLayout, NVConstDataRef<NVRenderVertexBuffer *>(attribBuffers, 2), NULL,
        toConstDataRef(strides, 2), toConstDataRef(offsets, 2), NVRenderDrawMode::Triangles);
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestAttribBuffers: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveProgramPipeline(mProgramPipeline);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);
    // set color
    compResult.mShader->SetPropertyValue("color", color);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(mInputAssembler->GetPrimitiveType(), 3, 0);

    context->SetActiveProgramPipeline(0);

    return true;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestProgramPipeline::runPerformance(NVRenderContext *context,
                                                 userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestProgramPipeline::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
