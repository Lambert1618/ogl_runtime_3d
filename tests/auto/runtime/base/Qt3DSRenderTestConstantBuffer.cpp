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

#include "Qt3DSRenderTestConstantBuffer.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"

#include <string>

using namespace qt3ds;
using namespace qt3ds::render;

struct Vertex
{
    QT3DSVec3 positions;
};

static const char *scalarVertShader(bool bESContext, std::string &prog)
{
    if (bESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330 compatibility\n";
    }

    prog += "layout (std140) uniform cbBuffer { \n"
            "float red;\n"
            "float green;\n"
            "mat4 mat_mvp;\n"
            "float blue;\n };\n"
            "in vec3 attr_pos; // Vertex pos\n"
            "void main()\n"
            "{\n"
            "	gl_Position = mat_mvp * vec4(attr_pos, 1.0);\n"
            "}\n";

    return prog.c_str();
}

static const char *scalarFragShader(bool bESContext, std::string &prog)
{
    if (bESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330 compatibility\n";
    }

    prog += "layout (std140) uniform cbBuffer { \n"
            "float red;\n"
            "float green;\n"
            "mat4 mat_mvp;\n"
            "float blue;\n };\n"
            "void main()\n"
            "{\n"
            "	gl_FragColor = vec4(red, green, blue, 1.0);\n"
            "}\n";

    return prog.c_str();
}

static const char *vectorVertShader(bool bESContext, std::string &prog)
{
    if (bESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330 compatibility\n";
    }

    prog += "layout (std140) uniform cbBuffer { \n"
            "vec2 rg;\n"
            "mat4 mat_mvp;\n"
            "vec2 ba;\n };\n"
            "in vec3 attr_pos; // Vertex pos\n"
            "void main()\n"
            "{\n"
            "	gl_Position = mat_mvp * vec4(attr_pos, 1.0);\n"
            "}\n";

    return prog.c_str();
}

static const char *vectorFragShader(bool bESContext, std::string &prog)
{
    if (bESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330 compatibility\n";
    }

    prog += "layout (std140) uniform cbBuffer { \n"
            "vec2 rg;\n"
            "mat4 mat_mvp;\n"
            "vec2 ba;\n };\n"
            "void main()\n"
            "{\n"
            "	gl_FragColor = vec4(rg[0], rg[1], ba[0], ba[1]);\n"
            "}\n";

    return prog.c_str();
}

static const char *structVertShader(bool bESContext, std::string &prog)
{
    if (bESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330 compatibility\n";
    }

    prog += "struct sampleStruct {\n"
            "vec2    rg;\n"
            "mat4	mat_mvp;\n"
            "float	blue;\n"
            "float	alpha; };\n"
            "layout (std140) uniform cbBuffer { \n"
            "sampleStruct s[2]; };\n"
            "in vec3 attr_pos; // Vertex pos\n"
            "void main()\n"
            "{\n"
            "	gl_Position = s[0].mat_mvp * vec4(attr_pos, 1.0);\n"
            "}\n";

    return prog.c_str();
}

static const char *structFragShader(bool bESContext, std::string &prog)
{
    if (bESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330 compatibility\n";
    }

    prog += "struct sampleStruct {\n"
            "vec2    rg;\n"
            "mat4	mat_mvp;\n"
            "float	blue;\n"
            "float	alpha; };\n"
            "layout (std140) uniform cbBuffer { \n"
            "sampleStruct s[2]; };\n"
            "void main()\n"
            "{\n"
            "	gl_FragColor = vec4(s[0].rg[0], s[0].rg[1], s[0].blue, s[0].alpha);\n"
            "}\n";

    return prog.c_str();
}

static const char *multiCBVertShader(bool bESContext, std::string &prog)
{
    if (bESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330 compatibility\n";
    }

    prog += "layout (std140) uniform cbBufferTrans { \n"
            "mat4 mat_mvp;\n };\n"
            "layout (std140) uniform cbBufferCol { \n"
            "float red;\n"
            "float green;\n"
            "float blue;\n };\n"
            "in vec3 attr_pos; // Vertex pos\n"
            "void main()\n"
            "{\n"
            "	gl_Position = mat_mvp * vec4(attr_pos, 1.0);\n"
            "}\n";

    return prog.c_str();
}

static const char *multiCBFragShader(bool bESContext, std::string &prog)
{
    if (bESContext) {
        prog += "#version 300 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 330 compatibility\n";
    }

    prog += "layout (std140) uniform cbBufferTrans { \n"
            "mat4 mat_mvp;\n };\n"
            "layout (std140) uniform cbBufferCol { \n"
            "float red;\n"
            "float green;\n"
            "float blue;\n };\n"
            "void main()\n"
            "{\n"
            "	gl_FragColor = vec4(red, green, blue, 1.0);\n"
            "}\n";

    return prog.c_str();
}

NVRenderTestConstantBuffer::NVRenderTestConstantBuffer()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestConstantBuffer::~NVRenderTestConstantBuffer()
{
}

bool NVRenderTestConstantBuffer::isSupported(NVRenderContext *context)
{
    NVRenderContextType ctxType = context->GetRenderContextType();
    NVRenderContextType nonSupportedFlags(NVRenderContextValues::GL2
                                          | NVRenderContextValues::GLES2);

    // This is currently only supported on GL(Es) >= 3
    if ((ctxType & nonSupportedFlags))
        return false;

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

bool NVRenderTestConstantBuffer::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= scalarTest(context, pUserData);
    _curTest++;
    success &= vectorTest(context, pUserData);
    _curTest++;
    success &= structTest(context, pUserData);
    _curTest++;
    success &= rawTest(context, pUserData);
    _curTest++;
    success &= multiCBTest(context, pUserData);
    _curTest++;

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    return success;
}

bool NVRenderTestConstantBuffer::scalarTest(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) }, { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(-0.9, 0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(0.9, 0.9, 0) } };

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    NVScopedRefCounted<NVRenderConstantBuffer> mConstantBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create constant buffer referred in the program
    mConstantBuffer = context->CreateConstantBuffer("cbBuffer", NVRenderBufferUsageType::Static, 0,
                                                    NVDataRef<QT3DSU8>());
    // buffer parameter. They must be in the order of appearance
    CRegisteredString theRedName(context->GetStringTable().RegisterStr("red"));
    mConstantBuffer->AddParam(theRedName, NVRenderShaderDataTypes::QT3DSF32, 1);
    CRegisteredString theGreenName(context->GetStringTable().RegisterStr("green"));
    mConstantBuffer->AddParam(theGreenName, NVRenderShaderDataTypes::QT3DSF32, 1);
    CRegisteredString theMatName(context->GetStringTable().RegisterStr("mat_mvp"));
    mConstantBuffer->AddParam(theMatName, NVRenderShaderDataTypes::QT3DSMat44, 1);
    CRegisteredString theBlueName(context->GetStringTable().RegisterStr("blue"));
    mConstantBuffer->AddParam(theBlueName, NVRenderShaderDataTypes::QT3DSF32, 1);

    // set values
    QT3DSF32 red = 0.0;
    mConstantBuffer->UpdateParam("red", NVDataRef<QT3DSU8>((QT3DSU8 *)&red, 1));
    QT3DSF32 green = 1.0;
    mConstantBuffer->UpdateParam("green", NVDataRef<QT3DSU8>((QT3DSU8 *)&green, 1));
    QT3DSF32 blue = 0.0;
    mConstantBuffer->UpdateParam("blue", NVDataRef<QT3DSU8>((QT3DSU8 *)&blue, 1));
    mConstantBuffer->UpdateParam("mat_mvp", NVDataRef<QT3DSU8>((QT3DSU8 *)mvp.front(), 1));

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult =
        context->CompileSource("NVRenderTestConstantBuffer::scalarTest",
                               toRef(scalarVertShader(isGLESContext(context), vtxProg)),
                               toRef(scalarFragShader(isGLESContext(context), frgProg)));
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
        qWarning() << "NVRenderTestConstantBuffer::scalarTest: Failed to create vertex buffer";
        return false;
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
        qWarning() << "NVRenderTestConstantBuffer::scalarTest: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    qt3ds::render::NVRenderCachedShaderBuffer<NVRenderShaderConstantBuffer *> cb("cbBuffer",
                                                                              *compResult.mShader);
    mConstantBuffer->Update();
    cb.Set();

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(NVRenderDrawMode::Triangles, 6, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

bool NVRenderTestConstantBuffer::vectorTest(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) }, { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(-0.9, 0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(0.9, 0.9, 0) } };

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    NVScopedRefCounted<NVRenderConstantBuffer> mConstantBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create constant buffer referred in the program
    mConstantBuffer = context->CreateConstantBuffer("cbBuffer", NVRenderBufferUsageType::Static, 0,
                                                    NVDataRef<QT3DSU8>());
    // buffer parameter. They must be in the order of appearance
    CRegisteredString theRGName(context->GetStringTable().RegisterStr("rg"));
    mConstantBuffer->AddParam(theRGName, NVRenderShaderDataTypes::QT3DSVec2, 1);
    CRegisteredString theMatName(context->GetStringTable().RegisterStr("mat_mvp"));
    mConstantBuffer->AddParam(theMatName, NVRenderShaderDataTypes::QT3DSMat44, 1);
    CRegisteredString theBAName(context->GetStringTable().RegisterStr("ba"));
    mConstantBuffer->AddParam(theBAName, NVRenderShaderDataTypes::QT3DSVec2, 1);

    // set values
    QT3DSVec2 rg;
    rg[0] = 0.0;
    rg[1] = 1.0;
    mConstantBuffer->UpdateParam("rg", NVDataRef<QT3DSU8>((QT3DSU8 *)&rg, 1));
    QT3DSVec2 ba;
    ba[0] = 0.0;
    ba[1] = 1.0;
    mConstantBuffer->UpdateParam("ba", NVDataRef<QT3DSU8>((QT3DSU8 *)&ba, 1));
    mConstantBuffer->UpdateParam("mat_mvp", NVDataRef<QT3DSU8>((QT3DSU8 *)mvp.front(), 1));

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult =
        context->CompileSource("NVRenderTestConstantBuffer::vectorTest",
                               toRef(vectorVertShader(isGLESContext(context), vtxProg)),
                               toRef(vectorFragShader(isGLESContext(context), frgProg)));
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
        qWarning() << "NVRenderTestConstantBuffer::vectorTest: Failed to create vertex buffer";
        return false;
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
        qWarning() << "NVRenderTestConstantBuffer::vectorTest: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    qt3ds::render::NVRenderCachedShaderBuffer<NVRenderShaderConstantBuffer *> cb("cbBuffer",
                                                                              *compResult.mShader);
    cb.Set();

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(NVRenderDrawMode::Triangles, 6, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

bool NVRenderTestConstantBuffer::structTest(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) }, { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(-0.9, 0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(0.9, 0.9, 0) } };

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    NVScopedRefCounted<NVRenderConstantBuffer> mConstantBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create constant buffer referred in the program
    mConstantBuffer = context->CreateConstantBuffer("cbBuffer", NVRenderBufferUsageType::Static, 0,
                                                    NVDataRef<QT3DSU8>());
    // buffer parameter. They must be in the order of appearance
    CRegisteredString theRGName(context->GetStringTable().RegisterStr("s[0].rg"));
    mConstantBuffer->AddParam(theRGName, NVRenderShaderDataTypes::QT3DSVec2, 1);
    CRegisteredString theMatName(context->GetStringTable().RegisterStr("s[0].mat_mvp"));
    mConstantBuffer->AddParam(theMatName, NVRenderShaderDataTypes::QT3DSMat44, 1);
    CRegisteredString theBlueName(context->GetStringTable().RegisterStr("s[0].blue"));
    mConstantBuffer->AddParam(theBlueName, NVRenderShaderDataTypes::QT3DSF32, 1);
    CRegisteredString theAlphaName(context->GetStringTable().RegisterStr("s[0].alpha"));
    mConstantBuffer->AddParam(theAlphaName, NVRenderShaderDataTypes::QT3DSF32, 1);

    // set values
    QT3DSVec2 rg;
    rg[0] = 0.0;
    rg[1] = 1.0;
    mConstantBuffer->UpdateParam("s[0].rg", NVDataRef<QT3DSU8>((QT3DSU8 *)&rg, 1));
    QT3DSF32 blue, alpha;
    blue = 0.0;
    alpha = 1.0;
    mConstantBuffer->UpdateParam("s[0].blue", NVDataRef<QT3DSU8>((QT3DSU8 *)&blue, 1));
    mConstantBuffer->UpdateParam("s[0].alpha", NVDataRef<QT3DSU8>((QT3DSU8 *)&alpha, 1));
    mConstantBuffer->UpdateParam("s[0].mat_mvp", NVDataRef<QT3DSU8>((QT3DSU8 *)mvp.front(), 1));

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult =
        context->CompileSource("NVRenderTestConstantBuffer::structTest",
                               toRef(structVertShader(isGLESContext(context), vtxProg)),
                               toRef(structFragShader(isGLESContext(context), frgProg)));
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
        qWarning() << "NVRenderTestConstantBuffer::structTest: Failed to create vertex buffer";
        return false;
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
        qWarning() << "NVRenderTestConstantBuffer::structTest: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    qt3ds::render::NVRenderCachedShaderBuffer<NVRenderShaderConstantBuffer *> cb("cbBuffer",
                                                                              *compResult.mShader);
    cb.Set();

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(NVRenderDrawMode::Triangles, 6, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

bool NVRenderTestConstantBuffer::rawTest(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) }, { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(-0.9, 0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(0.9, 0.9, 0) } };

    struct sampleStruct
    {
        float rg[2];
        float padding[2];
        QT3DSMat44 mat_mvp; // matrices start on 16 byte boundaries
        float blue;
        float alpha;
    } s;

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    NVScopedRefCounted<NVRenderConstantBuffer> mConstantBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    NVDataRef<QT3DSU8> cBuffer((QT3DSU8 *)&s, sizeof(sampleStruct));
    // create constant buffer referred in the program
    mConstantBuffer = context->CreateConstantBuffer("cbBuffer", NVRenderBufferUsageType::Static,
                                                    sizeof(sampleStruct), cBuffer);
    // set values
    s.rg[0] = 0.0;
    s.rg[1] = 1.0;
    s.mat_mvp = mvp;
    s.blue = 0.0;
    s.alpha = 0.0;
    mConstantBuffer->UpdateRaw(0, cBuffer);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult =
        context->CompileSource("NVRenderTestConstantBuffer::rawTest",
                               toRef(structVertShader(isGLESContext(context), vtxProg)),
                               toRef(structFragShader(isGLESContext(context), frgProg)));
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
        qWarning() << "NVRenderTestConstantBuffer::rawTest: Failed to create vertex buffer";
        return false;
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
        qWarning() << "NVRenderTestConstantBuffer::rawTest: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    qt3ds::render::NVRenderCachedShaderBuffer<NVRenderShaderConstantBuffer *> cb("cbBuffer",
                                                                              *compResult.mShader);
    cb.Set();

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(NVRenderDrawMode::Triangles, 6, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

///< test of multiple constant buffers
bool NVRenderTestConstantBuffer::multiCBTest(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) }, { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(-0.9, 0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(0.9, 0.9, 0) } };

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    NVScopedRefCounted<NVRenderConstantBuffer> mConstantBufferTrans;
    NVScopedRefCounted<NVRenderConstantBuffer> mConstantBufferCol;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create constant buffer referred in the program
    mConstantBufferTrans = context->CreateConstantBuffer(
        "cbBufferTrans", NVRenderBufferUsageType::Static, 0, NVDataRef<QT3DSU8>());
    // buffer parameter. They must be in the order of appearance
    CRegisteredString theMatName(context->GetStringTable().RegisterStr("mat_mvp"));
    mConstantBufferTrans->AddParam(theMatName, NVRenderShaderDataTypes::QT3DSMat44, 1);

    // create constant buffer referred in the program
    mConstantBufferCol = context->CreateConstantBuffer(
        "cbBufferCol", NVRenderBufferUsageType::Static, 0, NVDataRef<QT3DSU8>());
    CRegisteredString theRedName(context->GetStringTable().RegisterStr("red"));
    mConstantBufferCol->AddParam(theRedName, NVRenderShaderDataTypes::QT3DSF32, 1);
    CRegisteredString theGreenName(context->GetStringTable().RegisterStr("green"));
    mConstantBufferCol->AddParam(theGreenName, NVRenderShaderDataTypes::QT3DSF32, 1);
    CRegisteredString theBlueName(context->GetStringTable().RegisterStr("blue"));
    mConstantBufferCol->AddParam(theBlueName, NVRenderShaderDataTypes::QT3DSF32, 1);

    // set values
    mConstantBufferTrans->UpdateParam("mat_mvp", NVDataRef<QT3DSU8>((QT3DSU8 *)mvp.front(), 1));

    QT3DSF32 red = 0.0;
    mConstantBufferCol->UpdateParam("red", NVDataRef<QT3DSU8>((QT3DSU8 *)&red, 1));
    QT3DSF32 green = 1.0;
    mConstantBufferCol->UpdateParam("green", NVDataRef<QT3DSU8>((QT3DSU8 *)&green, 1));
    QT3DSF32 blue = 0.0;
    mConstantBufferCol->UpdateParam("blue", NVDataRef<QT3DSU8>((QT3DSU8 *)&blue, 1));

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult =
        context->CompileSource("NVRenderTestConstantBuffer::multiCBTest",
                               toRef(multiCBVertShader(isGLESContext(context), vtxProg)),
                               toRef(multiCBFragShader(isGLESContext(context), frgProg)));
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
        qWarning() << "NVRenderTestConstantBuffer::scalarTest: Failed to create vertex buffer";
        return false;
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
        qWarning() << "NVRenderTestConstantBuffer::scalarTest: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    qt3ds::render::NVRenderCachedShaderBuffer<NVRenderShaderConstantBuffer *> cbTrans(
        "cbBufferTrans", *compResult.mShader);
    mConstantBufferTrans->Update();
    cbTrans.Set();
    qt3ds::render::NVRenderCachedShaderBuffer<NVRenderShaderConstantBuffer *> cbCol(
        "cbBufferCol", *compResult.mShader);
    mConstantBufferCol->Update();
    cbCol.Set();

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(NVRenderDrawMode::Triangles, 6, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestConstantBuffer::runPerformance(NVRenderContext *context,
                                                userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestConstantBuffer::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
