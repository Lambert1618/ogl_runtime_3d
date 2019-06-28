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

#include "Qt3DSRenderTestOcclusionQuery.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderOcclusionQuery.h"

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

static const Vertex largeQuadPositions[] = { { QT3DSVec3(-0.7, 0.7, 0) }, { QT3DSVec3(-0.7, -0.7, 0) },
                                             { QT3DSVec3(0.7, -0.7, 0) }, { QT3DSVec3(-0.7, 0.7, 0) },
                                             { QT3DSVec3(0.7, -0.7, 0) }, { QT3DSVec3(0.7, 0.7, 0) } };

static const Vertex smallQuadPositions[] = { { QT3DSVec3(-0.5, 0.5, 0) }, { QT3DSVec3(-0.5, -0.5, 0) },
                                             { QT3DSVec3(0.5, -0.5, 0) }, { QT3DSVec3(-0.5, 0.5, 0) },
                                             { QT3DSVec3(0.5, -0.5, 0) }, { QT3DSVec3(0.5, 0.5, 0) } };

NVRenderTestOcclusionQuery::NVRenderTestOcclusionQuery()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestOcclusionQuery::~NVRenderTestOcclusionQuery()
{
}

bool NVRenderTestOcclusionQuery::isSupported(NVRenderContext *context)
{
    return context->IsSampleQuerySupported();
}

////////////////////////////////
// test for functionality
////////////////////////////////

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

bool NVRenderTestOcclusionQuery::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= occlusionPassTest(context, pUserData);
    _curTest++;
    success &= occlusionFailTest(context, pUserData);
    _curTest++;

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    return success;
}

bool NVRenderTestOcclusionQuery::renderQuad(NVRenderContext *context, userContextData *pUserData,
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
        "NVRenderTestOcclusionQuery shader", toRef(vertShader(vtxProg, isGLESContext(context))),
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
        qWarning() << "NVRenderTestOcclusionQuery: Failed to create vertex buffer";
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
        qWarning() << "NVRenderTestOcclusionQuery: Failed to create input assembler";
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

void NVRenderTestOcclusionQuery::renderPrim(NVRenderContext *context, userContextData *pUserData,
                                            void *pData, float zOffset, QT3DSVec3 color,
                                            NVRenderOcclusionQuery *pQuery)
{
    NVScopedRefCounted<NVRenderVertexBuffer> mVB;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mIA;
    Vertex *pVtxData = (Vertex *)pData;
    QT3DSMat44 proj = QT3DSMat44::createIdentity();
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&proj, -1, 1, -1, 1, -10, 10);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestOcclusionQuery shader", toRef(vertShader(vtxProg, isGLESContext(context))),
        toRef(fragShader(frgProg, isGLESContext(context))));
    if (!compResult.mShader) {
        return;
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
    NVDataRef<QT3DSU8> vertDataSmall((QT3DSU8 *)pVtxData, bufSize);
    mVB = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize, 3 * sizeof(QT3DSF32),
                                      vertDataSmall);
    if (!mVB)
        qWarning() << "NVRenderTestOcclusionQuery: Failed to create vertex buffer";

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 1));
    // create input Assembler
    QT3DSU32 strides = mVB->GetStride();
    QT3DSU32 offsets = 0;
    mIA = context->CreateInputAssembler(mAttribLayout, toConstDataRef(&mVB.mPtr, 1), NULL,
                                        toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    if (!mVB) {
        qWarning() << "NVRenderTestOcclusionQuery: Failed to create input assembler";
        return;
    }

    // make input assembler active
    context->SetInputAssembler(mIA);
    // setup translation
    QT3DSMat44 transZ;
    NvRenderTestMatrixTranslation(&transZ, 0.0, 0.0, zOffset);
    mvp = transZ * proj;

    // set program
    context->SetActiveShader(compResult.mShader);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);
    // set color
    compResult.mShader->SetPropertyValue("color", color);

    // start query
    if (pQuery)
        pQuery->Begin();

    context->Draw(NVRenderDrawMode::Triangles, 6, 0);

    // end query
    if (pQuery)
        pQuery->End();

    context->SetActiveShader(0);
    compResult.mShader->release();
}

bool NVRenderTestOcclusionQuery::occlusionPassTest(NVRenderContext *context,
                                                   userContextData *pUserData)
{
    NVScopedRefCounted<NVRenderOcclusionQuery> pQuery = context->CreateOcclusionQuery();

    renderPrim(context, pUserData, (void *)largeQuadPositions, 0.1, QT3DSVec3(0.0, 0.0, 1.0), NULL);
    // this quad should be covered by the previous one
    renderPrim(context, pUserData, (void *)smallQuadPositions, 0.2, QT3DSVec3(1.0, 1.0, 0.0), pQuery);
    // check visibility
    QT3DSU32 result = 0;
    pQuery->GetResult(&result);

    QT3DSVec3 color(0.0, 0.0, 0.0);
    if (result)
        color.x = 1.0; // wrong
    else
        color.y = 1.0; // right

    renderQuad(context, pUserData, color);

    return (result == 0);
}

bool NVRenderTestOcclusionQuery::occlusionFailTest(NVRenderContext *context,
                                                   userContextData *pUserData)
{
    NVScopedRefCounted<NVRenderOcclusionQuery> pQuery = context->CreateOcclusionQuery();

    renderPrim(context, pUserData, (void *)largeQuadPositions, 0.2, QT3DSVec3(0.0, 0.0, 1.0), NULL);
    // this quad should be visible by the previous one
    renderPrim(context, pUserData, (void *)smallQuadPositions, 0.1, QT3DSVec3(1.0, 1.0, 0.0), pQuery);
    // check visibility
    QT3DSU32 result = 0;
    pQuery->GetResult(&result);

    QT3DSVec3 color(0.0, 0.0, 0.0);
    if (result == 0)
        color.x = 1.0; // wrong
    else
        color.y = 1.0; // right

    renderQuad(context, pUserData, color);

    return (result == 1);
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestOcclusionQuery::runPerformance(NVRenderContext *context,
                                                userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestOcclusionQuery::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
