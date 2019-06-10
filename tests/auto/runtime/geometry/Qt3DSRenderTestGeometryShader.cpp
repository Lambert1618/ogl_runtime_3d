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

#include "Qt3DSRenderTestGeometryShader.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderImageTexture.h"
#include "render/Qt3DSRenderShaderProgram.h"

#include <string>

using namespace qt3ds;
using namespace qt3ds::render;

static const char *vertShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 400\n";
    }

    prog += "uniform mat4 mat_mvp;\n"
            "in vec3 attr_pos;			// Vertex pos\n"
            "void main()\n"
            "{\n"
            "	gl_Position = vec4(attr_pos, 1.0);\n"
            "}\n";

    return prog.c_str();
}

static const char *fragShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 400\n";
    }

    prog += "uniform vec3 color;\n"
            "out vec4 fragColor;\n"
            "void main()\n"
            "{\n"
            "	fragColor = vec4(color, 1.0);\n"
            "}\n";

    return prog.c_str();
}

static const char *geometryShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_EXT_geometry_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 430\n";
    }

    // pass through shader
    prog += "layout (triangles) in;\n"
            "layout (triangle_strip, max_vertices = 3) out;\n"
            "void main()\n"
            "{\n"
            "	int i;\n"
            "   for(i=0; i<gl_in.length(); i++)\n"
            "   {\n"
            "      gl_Position = gl_in[i].gl_Position;\n"
            "      EmitVertex();\n"
            "   }\n"
            "   EndPrimitive();\n"
            "}\n";

    return prog.c_str();
}

static const char *wireframeShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_EXT_geometry_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 430\n";
    }

    // convert to wireframe
    prog += "layout (triangles) in;\n"
            "layout (line_strip, max_vertices = 4) out;\n"
            "void main()\n"
            "{\n"
            "	int i;\n"
            "   for(i=0; i<gl_in.length(); i++)\n"
            "   {\n"
            "      gl_Position = gl_in[i].gl_Position;\n"
            "      EmitVertex();\n"
            "   }\n"
            "   gl_Position = gl_in[0].gl_Position;\n"
            "   EmitVertex();\n"
            "   EndPrimitive();\n"
            "}\n";

    return prog.c_str();
}

struct Vertex
{
    QT3DSVec3 positions;
    QT3DSVec2 texCoord;
};

NVRenderTestGeometryShader::NVRenderTestGeometryShader()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestGeometryShader::~NVRenderTestGeometryShader()
{
}

bool NVRenderTestGeometryShader::isSupported(NVRenderContext *context)
{
    return context->IsGeometryStageSupported();
}

////////////////////////////////
// test for functionality
////////////////////////////////

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

bool NVRenderTestGeometryShader::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;

    context->SetRenderTarget(NULL);
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= geometryCompile(context, pUserData);
    _curTest++;
    success &= wireframe(context, pUserData);
    _curTest++;

    return success;
}

bool NVRenderTestGeometryShader::geometryCompile(NVRenderContext *context,
                                                 userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, -0.9, 0), QT3DSVec2(0, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0), QT3DSVec2(0, 0) },
                                              { QT3DSVec3(0.0, 0.9, 0), QT3DSVec2(0, 0) } };

    qt3ds::QT3DSVec3 color(0.0, 1.0, 0.0);

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestGeometryShader shader", toRef(vertShader(vtxProg, isGLESContext(context))),
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

    QT3DSU32 bufSize = 3 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                5 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestGeometryShader: Failed to create vertex buffer";
        return false;
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 1));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler = context->CreateInputAssembler(
        mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), mIndexBuffer.mPtr,
        toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1), NVRenderDrawMode::Triangles);
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestGeometryShader: Failed to create input assembler";
        return false;
    }

    // create a geometry shader which does nothing just as a compile check
    std::string geomProg;
    std::string vtxProgDummy;
    std::string frgProgDummy;
    NVRenderVertFragCompilationResult geomResult = context->CompileSource(
        "NVRenderTestGeometryShader shader",
        toRef(vertShader(vtxProgDummy, isGLESContext(context))),
        toRef(fragShader(frgProgDummy, isGLESContext(context))), NVConstDataRef<QT3DSI8>(),
        NVConstDataRef<QT3DSI8>(), toRef(geometryShader(geomProg, isGLESContext(context))));

    if (!geomResult.mShader) {
        color.x = 1.0;
        color.y = 0.0;
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
    context->Draw(mInputAssembler->GetPrimitiveType(), 3, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();
    if (geomResult.mShader)
        geomResult.mShader->release();

    return true;
}

bool NVRenderTestGeometryShader::wireframe(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, -0.9, 0), QT3DSVec2(0, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0), QT3DSVec2(0, 0) },
                                              { QT3DSVec3(0.0, 0.9, 0), QT3DSVec2(0, 0) } };

    qt3ds::QT3DSVec3 color(0.0, 1.0, 0.0);

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create shaders
    // geometry shader converts primitives from triangles to lines
    std::string vtxProg;
    std::string frgProg;
    std::string geomProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestGeometryShader shader", toRef(vertShader(vtxProg, isGLESContext(context))),
        toRef(fragShader(frgProg, isGLESContext(context))), NVConstDataRef<QT3DSI8>(),
        NVConstDataRef<QT3DSI8>(), toRef(wireframeShader(geomProg, isGLESContext(context))));
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

    QT3DSU32 bufSize = 3 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                5 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestGeometryShader: Failed to create vertex buffer";
        return false;
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 1));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler = context->CreateInputAssembler(
        mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), mIndexBuffer.mPtr,
        toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1), NVRenderDrawMode::Triangles);
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestGeometryShader: Failed to create input assembler";
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
    context->Draw(mInputAssembler->GetPrimitiveType(), 3, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestGeometryShader::runPerformance(NVRenderContext *context,
                                                userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestGeometryShader::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
