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

#include "Qt3DSRenderTestPrimitives.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"

using namespace qt3ds;
using namespace qt3ds::render;

static const char *PassthroughVertShader()
{
    return "uniform mat4 mat_mvp;\n"
           "attribute vec3 attr_pos;			// Vertex pos\n"
           "void main()\n"
           "{\n"
           "	gl_Position = mat_mvp * vec4(attr_pos, 1.0);\n"
           "}\n";
}

static const char *SimpleFragShader()
{
    return "#ifdef GL_ES\n"
           "precision mediump float;\n"
           "#endif\n"
           "void main()\n"
           "{\n"
           "gl_FragColor = vec4(0, 1, 0, 1);\n"
           "}\n";
}

NVRenderTestPrimitives::NVRenderTestPrimitives()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestPrimitives::~NVRenderTestPrimitives()
{
}

bool NVRenderTestPrimitives::isSupported(NVRenderContext *context)
{
    return true;
}

////////////////////////////////
// test for functionality
////////////////////////////////

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

bool NVRenderTestPrimitives::renderPrimitive(NVRenderContext *context,
                                             userContextData *pContextData,
                                             const Vertex *pVertexData, unsigned int vertexCount,
                                             const unsigned short *pIndexData,
                                             unsigned int indexCount,
                                             NVRenderDrawMode::Enum primType)
{
    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create shaders
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestPrimitives shader", toRef(PassthroughVertShader()), toRef(SimpleFragShader()));
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

    QT3DSU32 bufSize = vertexCount * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)pVertexData, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                3 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestPrimitives::Triangles: Failed to create vertex buffer";
        return false;
    }

    // index buffer
    if (pIndexData && indexCount) {
        bufSize = indexCount * sizeof(unsigned short);
        NVDataRef<QT3DSU8> idxData((QT3DSU8 *)pIndexData, bufSize);
        mIndexBuffer = context->CreateIndexBuffer(
            NVRenderBufferUsageType::Static, NVRenderComponentTypes::QT3DSU16, bufSize,
            NVConstDataRef<QT3DSU8>(reinterpret_cast<const QT3DSU8 *>(pIndexData), bufSize));
        if (!mIndexBuffer) {
            qWarning() << "NVRenderTestPrimitives::Triangles: Failed to create index buffer";
            return false;
        }
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 1));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler = context->CreateInputAssembler(
        mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), mIndexBuffer.mPtr,
        toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestPrimitives::Triangles: Failed to create inout assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    QT3DSU32 count = (mIndexBuffer.mPtr) ? indexCount : vertexCount;
    context->Draw(primType, count, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

bool NVRenderTestPrimitives::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= triangles(context, pUserData);
    _curTest++;
    success &= triangleStrip(context, pUserData);
    _curTest++;
    success &= lines(context, pUserData);
    _curTest++;
    success &= lineStrip(context, pUserData);
    _curTest++;
    success &= trianglesIndexed(context, pUserData);
    _curTest++;
    success &= triangleStripIndexed(context, pUserData);
    _curTest++;
    success &= linesIndexed(context, pUserData);
    _curTest++;
    success &= lineStripIndexed(context, pUserData);

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    return success;
}

bool NVRenderTestPrimitives::triangles(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) },  { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.85, -0.9, 0) }, { QT3DSVec3(-0.85, 0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) },  { QT3DSVec3(0.9, 0.9, 0) } };

    return renderPrimitive(context, pUserData, vertexPositions, 6, NULL, 0,
                           NVRenderDrawMode::Triangles);
}

bool NVRenderTestPrimitives::triangleStrip(NVRenderContext *context, userContextData *pUserData)
{
    const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0.0) },
                                       { QT3DSVec3(-0.9f, -0.9f, 0.0f) },
                                       { QT3DSVec3(0.9f, 0.9f, 0.0f) },
                                       { QT3DSVec3(0.9f, -0.9f, 0.0f) } };

    return renderPrimitive(context, pUserData, vertexPositions, 4, NULL, 0,
                           NVRenderDrawMode::TriangleStrip);
}

bool NVRenderTestPrimitives::lines(NVRenderContext *context, userContextData *pUserData)
{
    const Vertex vertexPositions[] = {
        { QT3DSVec3(0.9f, 0.9f, 0.0f) },    { QT3DSVec3(0.9f, -0.9f, 0.0f) },
        { QT3DSVec3(0.9f, -0.9f, 0.0f) },   { QT3DSVec3(-0.85f, -0.9f, 0.0f) },
        { QT3DSVec3(-0.85f, -0.9f, 0.0f) }, { QT3DSVec3(0.9f, 0.9f, 0.0f) },
        { QT3DSVec3(-0.9f, -0.9f, 0.0f) },  { QT3DSVec3(0.85f, 0.9f, 0.0f) },
        { QT3DSVec3(0.85f, 0.9f, 0.0f) },   { QT3DSVec3(-0.9f, 0.9f, 0.0f) },
        { QT3DSVec3(-0.9f, 0.9f, 0.0f) },   { QT3DSVec3(-0.9f, -0.9f, 0.0f) }
    };

    return renderPrimitive(context, pUserData, vertexPositions, 12, NULL, 0,
                           NVRenderDrawMode::Lines);
}

bool NVRenderTestPrimitives::lineStrip(NVRenderContext *context, userContextData *pUserData)
{
    const Vertex vertexPositions[] = {
        { QT3DSVec3(-0.9f, 0.9f, 0.0f) }, { QT3DSVec3(-0.9f, -0.9f, 0.0f) },
        { QT3DSVec3(0.9f, -0.9f, 0.0f) }, { QT3DSVec3(0.9f, 0.9f, 0.0f) },
        { QT3DSVec3(-0.9f, 0.9f, 0.0f) },
    };

    return renderPrimitive(context, pUserData, vertexPositions, 5, NULL, 0,
                           NVRenderDrawMode::LineStrip);
}

bool NVRenderTestPrimitives::trianglesIndexed(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) },  { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.85, -0.9, 0) }, { QT3DSVec3(-0.85, 0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) },  { QT3DSVec3(0.9, 0.9, 0) } };

    const unsigned short indices[] = { 0, 1, 2, 3, 4, 5 };

    return renderPrimitive(context, pUserData, vertexPositions, 6, indices, 6,
                           NVRenderDrawMode::Triangles);
}

bool NVRenderTestPrimitives::triangleStripIndexed(NVRenderContext *context,
                                                  userContextData *pUserData)
{
    const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0.0) },
                                       { QT3DSVec3(-0.9f, -0.9f, 0.0f) },
                                       { QT3DSVec3(0.9f, 0.9f, 0.0f) },
                                       { QT3DSVec3(0.9f, -0.9f, 0.0f) } };

    const unsigned short indices[] = { 0, 1, 2, 3 };

    return renderPrimitive(context, pUserData, vertexPositions, 4, indices, 4,
                           NVRenderDrawMode::TriangleStrip);
}

bool NVRenderTestPrimitives::linesIndexed(NVRenderContext *context, userContextData *pUserData)
{
    const Vertex vertexPositions[] = {
        { QT3DSVec3(0.9f, 0.9f, 0.0f) },    { QT3DSVec3(0.9f, -0.9f, 0.0f) },
        { QT3DSVec3(-0.85f, -0.9f, 0.0f) }, { QT3DSVec3(-0.9f, -0.9f, 0.0f) },
        { QT3DSVec3(0.85f, 0.9f, 0.0f) },   { QT3DSVec3(-0.9f, 0.9f, 0.0f) },
    };

    const unsigned short indices[] = { 0, 1, 1, 2, 2, 0, 3, 4, 4, 5, 5, 3 };

    return renderPrimitive(context, pUserData, vertexPositions, 6, indices, 12,
                           NVRenderDrawMode::Lines);
}

bool NVRenderTestPrimitives::lineStripIndexed(NVRenderContext *context, userContextData *pUserData)
{
    const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0.0) },
                                       { QT3DSVec3(-0.9f, -0.9f, 0.0f) },
                                       { QT3DSVec3(0.9f, -0.9f, 0.0f) },
                                       { QT3DSVec3(0.9f, 0.9f, 0.0f) } };

    const unsigned short indices[] = { 0, 1, 2, 3, 0 };

    return renderPrimitive(context, pUserData, vertexPositions, 4, indices, 5,
                           NVRenderDrawMode::LineStrip);
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestPrimitives::runPerformance(NVRenderContext *context, userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestPrimitives::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
