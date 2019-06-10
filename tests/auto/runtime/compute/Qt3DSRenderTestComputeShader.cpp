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

#include "Qt3DSRenderTestComputeShader.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderImageTexture.h"
#include "render/Qt3DSRenderStorageBuffer.h"
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

static const char *vertTexShader(std::string &prog, bool binESContext)
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
            "in vec2 attr_uv;			// texture coord\n"
            "out vec2 varTexCoord;\n"
            "void main()\n"
            "{\n"
            "  gl_Position = vec4(attr_pos, 1.0);\n"
            "  varTexCoord = attr_uv;\n"
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

static const char *fragTexShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 400\n";
    }

    prog += "uniform sampler2D inTex;\n"
            "in vec2 varTexCoord;\n"
            "out vec4 fragColor;\n"
            "void main()\n"
            "{\n"
            "	fragColor = texture(inTex, varTexCoord);\n"
            "}\n";

    return prog.c_str();
}

static const char *computeShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_ARB_compute_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 430\n"
                "#extension GL_ARB_compute_shader : enable\n";
    }

    prog += "// Set workgroup layout;\n"
            "layout (local_size_x =16, local_size_y = 16) in;\n\n"
            "void main()\n"
            "{\n"
            " // do nothing\n"
            "}\n";

    return prog.c_str();
}

static const char *computeWorkShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_ARB_compute_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n"
                "precision mediump image2D;\n";
    } else {
        prog += "#version 430\n"
                "#extension GL_ARB_compute_shader : enable\n";
    }

    prog += "// Set workgroup layout;\n"
            "layout (local_size_x = 32, local_size_y = 32) in;\n\n"
            "layout (rgba8, binding = 2) uniform image2D outputImage;\n\n"
            "void main()\n"
            "{\n"
            "  imageStore( outputImage, ivec2(gl_GlobalInvocationID.xy), vec4( "
            "vec2(gl_LocalInvocationID.xy) / vec2(gl_WorkGroupSize.xy), 0.0, 1.0 ) );\n"
            "}\n";

    return prog.c_str();
}

static const char *computeStorageShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_ARB_compute_shader : enable\n"
                "#extension GL_ARB_shader_storage_buffer_object : enable\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 430\n"
                "#extension GL_ARB_compute_shader : enable\n"
                "#extension GL_ARB_shader_storage_buffer_object : enable\n";
    }

    prog += "layout( std140, binding=4 ) buffer Pos\n"
            "{\n"
            "  vec4 Positions[ ]; // array of positions\n"
            "};\n"
            "// Set workgroup layout;\n"
            "layout (local_size_x = 32, local_size_y = 1) in;\n\n"
            "void main()\n"
            "{\n"
            "  uint gid = gl_GlobalInvocationID.x;\n"
            "  if ( gid < uint(1000) ) {\n"
            "    Positions[gid].x = float(gl_GlobalInvocationID.x);\n"
            "  }\n"
            "}\n";

    return prog.c_str();
}

struct Vertex
{
    QT3DSVec3 positions;
    QT3DSVec2 texCoord;
};

NVRenderTestComputeShader::NVRenderTestComputeShader()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestComputeShader::~NVRenderTestComputeShader()
{
}

bool NVRenderTestComputeShader::isSupported(NVRenderContext *context)
{
    return context->IsComputeSupported();
}

////////////////////////////////
// test for functionality
////////////////////////////////

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

bool NVRenderTestComputeShader::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;

    context->SetRenderTarget(NULL);
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= computeCompile(context, pUserData);
    _curTest++;
    success &= computeWorkgroup(context, pUserData);
    _curTest++;
    success &= computeStorage(context, pUserData);
    _curTest++;

    return success;
}

bool NVRenderTestComputeShader::computeCompile(NVRenderContext *context, userContextData *pUserData)
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
        "NVRenderTestComputeShader shader", toRef(vertShader(vtxProg, isGLESContext(context))),
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
        qWarning() << "NVRenderTestComputeShader: Failed to create vertex buffer";
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
        qWarning() << "NVRenderTestComputeShader: Failed to create input assembler";
        return false;
    }

    // create a compute shader which does nothing just as a compile check
    std::string computeProg;
    NVRenderVertFragCompilationResult computeResult = context->CompileComputeSource(
        "Compute nothing shader", toRef(computeShader(computeProg, isGLESContext(context))));

    if (!computeResult.mShader) {
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
    if (computeResult.mShader)
        computeResult.mShader->release();

    return true;
}

#define WORKGROUP_SIZE 32

bool NVRenderTestComputeShader::computeWorkgroup(NVRenderContext *context,
                                                 userContextData *pUserData)
{
    static const Vertex vertexPositions[] = {
        { QT3DSVec3(-0.9, -0.9, 0), QT3DSVec2(0, 0) }, { QT3DSVec3(0.9, 0.9, 0), QT3DSVec2(1, 1) },
        { QT3DSVec3(-0.9, 0.9, 0), QT3DSVec2(0, 1) },  { QT3DSVec3(-0.9, -0.9, 0), QT3DSVec2(0, 0) },
        { QT3DSVec3(0.9, -0.9, 0), QT3DSVec2(1, 0) },  { QT3DSVec3(0.9, 0.9, 0), QT3DSVec2(1, 1) }
    };

    qt3ds::QT3DSVec3 color(0.0, 1.0, 0.0);

    // create texture
    NVScopedRefCounted<NVRenderTexture2D> mColorTexture;
    mColorTexture = context->CreateTexture2D();
    mColorTexture->SetTextureStorage(1, pUserData->winWidth, pUserData->winHeight,
                                     NVRenderTextureFormats::RGBA8);
    // create a image buffer wrapper
    NVScopedRefCounted<NVRenderImage2D> mColorImage;
    mColorImage = context->CreateImage2D(mColorTexture, NVRenderImageAccessType::Write);

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
        "NVRenderTestComputeShader shader", toRef(vertTexShader(vtxProg, isGLESContext(context))),
        toRef(fragTexShader(frgProg, isGLESContext(context))));
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
        NVRenderVertexBufferEntry("attr_uv", NVRenderComponentTypes::QT3DSF32, 2, 12),
    };

    QT3DSU32 bufSize = 6 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                5 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestComputeShader: Failed to create vertex buffer";
        return false;
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 2));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler = context->CreateInputAssembler(
        mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), mIndexBuffer.mPtr,
        toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1), NVRenderDrawMode::Triangles);
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestComputeShader: Failed to create input assembler";
        return false;
    }

    // create a compute shader which outputs the workgroups as color codes
    std::string computeProg;
    NVRenderVertFragCompilationResult computeResult = context->CompileComputeSource(
        "Compute workgroup shader", toRef(computeWorkShader(computeProg, isGLESContext(context))));

    if (!computeResult.mShader) {
        qWarning() << "NVRenderTestComputeShader: Failed to create compute shader";
        return false;
    }

    // set program
    context->SetActiveShader(computeResult.mShader);
    NVRenderCachedShaderProperty<NVRenderImage2D *> mOutputImage("outputImage",
                                                                 *computeResult.mShader);
    mOutputImage.Set(mColorImage);
    // run compute shader
    context->DispatchCompute(computeResult.mShader, pUserData->winWidth / WORKGROUP_SIZE,
                             pUserData->winHeight / WORKGROUP_SIZE, 1);
    NVRenderBufferBarrierFlags flags(NVRenderBufferBarrierValues::ShaderImageAccess);
    // sync
    context->SetMemoryBarrier(flags);

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);
    // set color
    compResult.mShader->SetPropertyValue("color", color);
    // set texture
    NVRenderCachedShaderProperty<NVRenderTexture2D *> mInputImage("inTex", *compResult.mShader);
    mInputImage.Set(mColorTexture);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(mInputAssembler->GetPrimitiveType(), 6, 0);

    context->SetActiveShader(0);

    compResult.mShader->release();
    if (computeResult.mShader)
        computeResult.mShader->release();

    return true;
}

bool NVRenderTestComputeShader::computeStorage(NVRenderContext *context, userContextData *pUserData)
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

    // create vertex buffer for compute shader usage
    NVScopedRefCounted<NVRenderVertexBuffer> mComputeVertexBuffer;
    QT3DSF32 *storageData = new QT3DSF32[1000 * 4]; // vec 4 in shader program
    NVDataRef<QT3DSU8> storData((QT3DSU8 *)storageData, 1000 * sizeof(QT3DSF32) * 4);
    mComputeVertexBuffer = context->CreateVertexBuffer(
        NVRenderBufferUsageType::Static, 1000 * sizeof(QT3DSF32) * 4, sizeof(QT3DSF32), storData);
    if (!mComputeVertexBuffer) {
        qWarning() << "NVRenderTestComputeShader: Failed to create compute vertex buffer";
        return false;
    }
    // create storage wrapper for vertex buffer
    NVScopedRefCounted<NVRenderStorageBuffer> mComputeStorageBuffer;
    mComputeStorageBuffer =
        context->CreateStorageBuffer("Pos", NVRenderBufferUsageType::Static,
                                     1000 * sizeof(QT3DSF32) * 4, storData, mComputeVertexBuffer.mPtr);
    if (!mComputeStorageBuffer) {
        qWarning() << "NVRenderTestComputeShader: Failed to create compute storage buffer";
        return false;
    }

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestComputeShader shader", toRef(vertShader(vtxProg, isGLESContext(context))),
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
        qWarning() << "NVRenderTestComputeShader: Failed to create vertex buffer";
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
        qWarning() << "NVRenderTestComputeShader: Failed to create input assembler";
        return false;
    }

    // create a compute shader which places id's into the buffer
    std::string computeProg;
    NVRenderVertFragCompilationResult computeResult = context->CompileComputeSource(
        "Compute storage shader", toRef(computeStorageShader(computeProg, isGLESContext(context))));

    if (!computeResult.mShader) {
        qWarning() << "NVRenderTestComputeShader: Failed to create compute shader";
        return false;
    }

    // set and run compute program
    context->SetActiveShader(computeResult.mShader);
    qt3ds::render::NVRenderCachedShaderBuffer<NVRenderShaderStorageBuffer *> storageBuffer(
        "Pos", *computeResult.mShader);
    storageBuffer.Set();
    // run compute shader
    context->DispatchCompute(computeResult.mShader, 1024 / WORKGROUP_SIZE, 1, 1);
    NVRenderBufferBarrierFlags flags(NVRenderBufferBarrierValues::ShaderStorage
                                     | NVRenderBufferBarrierValues::VertexAttribArray);
    // sync
    context->SetMemoryBarrier(flags);

    // check content
    bool contentOK = true;
    mComputeVertexBuffer->Bind();
    NVDataRef<QT3DSU8> pData = mComputeVertexBuffer->MapBuffer();
    QT3DSF32 *fData = (QT3DSF32 *)pData.begin();
    QT3DSU32 size = pData.size() / 4;
    for (QT3DSU32 i = 0, k = 0; i < size; i += 4, k++) {
        if (fData[i] != (float)k)
            contentOK = false;
    }

    mComputeVertexBuffer->UnmapBuffer();

    if (!contentOK) {
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
    if (computeResult.mShader)
        computeResult.mShader->release();

    delete storageData;

    return true;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestComputeShader::runPerformance(NVRenderContext *context, userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestComputeShader::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
