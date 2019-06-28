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

#include "Qt3DSRenderTestTimerQuery.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderTimerQuery.h"

#include <string>

using namespace qt3ds;
using namespace qt3ds::render;

static const char *vertShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330\n";
    }

    prog += "uniform mat4 mat_mvp;\n"
            "in vec3 attr_pos;			// Vertex pos\n"
            "void main()\n"
            "{\n"
            "	gl_Position = mat_mvp * vec4(attr_pos, 1.0);\n"
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
        prog += "#version 330\n";
    }

    prog += "uniform vec3 color;\n"
            "out vec4 fragColor;\n"
            "void main()\n"
            "{\n"
            "	fragColor = vec4(color, 1.0);\n"
            "}\n";

    return prog.c_str();
}

struct Vertex
{
    QT3DSVec3 positions;
};

static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) }, { QT3DSVec3(-0.9, -0.9, 0) },
                                          { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(-0.9, 0.9, 0) },
                                          { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(0.9, 0.9, 0) } };

NVRenderTestTimerQuery::NVRenderTestTimerQuery()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestTimerQuery::~NVRenderTestTimerQuery()
{
}

bool NVRenderTestTimerQuery::isSupported(NVRenderContext *context)
{
    return context->IsTimerQuerySupported();
}

////////////////////////////////
// test for functionality
////////////////////////////////

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

bool NVRenderTestTimerQuery::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= timerTest(context, pUserData);
    _curTest++;
    success &= absoluteTimerTest(context, pUserData);
    _curTest++;

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    return success;
}

bool NVRenderTestTimerQuery::renderQuad(NVRenderContext *context, userContextData *pUserData,
                                        QT3DSVec3 color)
{
    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestTimerQuery shader", toRef(vertShader(vtxProg, isGLESContext(context))),
        toRef(fragShader(frgProg, isGLESContext(context))));
    if (!compResult.mShader) {
        return false;
    }

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
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 3, 0),
    };

    QT3DSU32 bufSize = 6 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                3 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestTimerQuery: Failed to create vertex buffer";
        return false;
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 1));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler =
        context->CreateInputAssembler(mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), NULL,
                                      toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestTimerQuery: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);
    // set color
    compResult.mShader->SetPropertyValue("color", color);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(NVRenderDrawMode::Triangles, 6, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

bool NVRenderTestTimerQuery::timerTest(NVRenderContext *context, userContextData *pUserData)
{
    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    QT3DSMat44 proj = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&proj, -1, 1, -1, 1, -10, 10);
    QT3DSVec3 color(1.0, 1.0, 0.0);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestTimerQuery shader", toRef(vertShader(vtxProg, isGLESContext(context))),
        toRef(fragShader(frgProg, isGLESContext(context))));
    if (!compResult.mShader) {
        return false;
    }

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
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 3, 0),
    };

    QT3DSU32 bufSize = 6 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                3 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestTimerQuery: Failed to create vertex buffer";
        return false;
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 1));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler =
        context->CreateInputAssembler(mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), NULL,
                                      toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestTimerQuery: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);

    // setup translation
    QT3DSMat44 transZ;
    NvRenderTestMatrixTranslation(&transZ, 0.0, 0.0, 0.2);
    mvp = transZ * proj;

    // set program
    context->SetActiveShader(compResult.mShader);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);
    // set color
    compResult.mShader->SetPropertyValue("color", color);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    NVScopedRefCounted<NVRenderTimerQuery> pQuery = context->CreateTimerQuery();

    // render 1000 quads this should take at least some amount of time
    pQuery->Begin();
    for (QT3DSI32 i = 0; i < 1000; i++) {
        context->Draw(NVRenderDrawMode::Triangles, 6, 0);
    }
    pQuery->End();

    // get elapsed time in nano seconds
    QT3DSU64 result = 0;
    pQuery->GetResult(&result);
    // convert to milli second
    QT3DSF64 elapsed = double(result) / 1e06;

    /// it should take at least a fraction of a milli second
    if (elapsed > 0.0)
        color.x = 0.0; // right
    else
        color.y = 0.0; // wrong

    context->SetActiveShader(0);
    compResult.mShader->release();

    renderQuad(context, pUserData, color);

    return (elapsed > 0.0);
}

bool NVRenderTestTimerQuery::absoluteTimerTest(NVRenderContext *context, userContextData *pUserData)
{
    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    QT3DSMat44 proj = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&proj, -1, 1, -1, 1, -10, 10);
    QT3DSVec3 color(1.0, 1.0, 0.0);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestTimerQuery shader", toRef(vertShader(vtxProg, isGLESContext(context))),
        toRef(fragShader(frgProg, isGLESContext(context))));
    if (!compResult.mShader) {
        return false;
    }

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
        NVRenderVertexBufferEntry("attr_pos", NVRenderComponentTypes::QT3DSF32, 3, 0),
    };

    QT3DSU32 bufSize = 6 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                3 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestTimerQuery: Failed to create vertex buffer";
        return false;
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 1));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler =
        context->CreateInputAssembler(mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), NULL,
                                      toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestTimerQuery: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);

    // setup translation
    QT3DSMat44 transZ;
    NvRenderTestMatrixTranslation(&transZ, 0.0, 0.0, 0.2);
    mvp = transZ * proj;

    // set program
    context->SetActiveShader(compResult.mShader);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);
    // set color
    compResult.mShader->SetPropertyValue("color", color);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    NVScopedRefCounted<NVRenderTimerQuery> pQueryStart = context->CreateTimerQuery();
    NVScopedRefCounted<NVRenderTimerQuery> pQueryEnd = context->CreateTimerQuery();

    // render 1000 quads this should take at least some amount of time
    pQueryStart->SetTimerQuery();
    for (QT3DSI32 i = 0; i < 1000; i++) {
        context->Draw(NVRenderDrawMode::Triangles, 6, 0);
    }
    pQueryEnd->SetTimerQuery();

    // get absolute time in nano seconds
    QT3DSU64 start = 0;
    pQueryStart->GetResult(&start);
    QT3DSU64 end = 0;
    pQueryEnd->GetResult(&end);

    // convert to milli second
    QT3DSF64 elapsed = double(end - start) / 1e06;

    // it should take at least a fraction of a milli second
    if (elapsed > 0.0)
        color.x = 0.0; // right
    else
        color.y = 0.0; // wrong

    context->SetActiveShader(0);
    compResult.mShader->release();

    renderQuad(context, pUserData, color);

    return (elapsed > 0.0);
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestTimerQuery::runPerformance(NVRenderContext *context, userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestTimerQuery::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
