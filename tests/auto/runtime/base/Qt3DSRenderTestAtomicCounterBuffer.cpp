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

#include "Qt3DSRenderTestAtomicCounterBuffer.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderAtomicCounterBuffer.h"
#include "render/Qt3DSRenderShaderProgram.h"

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
        prog += "#version 430\n";
    }

    prog += "layout(binding = 2, offset = 0) uniform atomic_uint ac_raster;\n"
            "out vec4 fragColor;\n"
            "void main()\n"
            "{\n"
            "   uint counter = atomicCounterIncrement(ac_raster);\n"
            "   float g = (float(counter)/255.0) / 255.0;\n"
            "	fragColor = vec4(0.0, g, 0.0, 1.0);\n"
            "}\n";

    return prog.c_str();
}

NVRenderTestAtomicCounterBuffer::NVRenderTestAtomicCounterBuffer()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestAtomicCounterBuffer::~NVRenderTestAtomicCounterBuffer()
{
}

bool NVRenderTestAtomicCounterBuffer::isSupported(NVRenderContext *context)
{
    // This is currently only supported on GL 4 and GLES 3.1
    if (!context->IsAtomicCounterBufferSupported())
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

bool NVRenderTestAtomicCounterBuffer::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= rasterizerTest(context, pUserData);
    _curTest++;

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    return success;
}

bool NVRenderTestAtomicCounterBuffer::rasterizerTest(NVRenderContext *context,
                                                     userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, -0.9, 0) }, { QT3DSVec3(0.9, 0.9, 0) },
                                              { QT3DSVec3(-0.9, 0.9, 0) },  { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) },  { QT3DSVec3(0.9, 0.9, 0) } };

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    NVScopedRefCounted<NVRenderAtomicCounterBuffer> mAtomicCounterBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create atomic counter buffer
    QT3DSU32 acCounter = 0;
    NVDataRef<QT3DSU8> acData((QT3DSU8 *)&acCounter, sizeof(QT3DSU32));
    mAtomicCounterBuffer = context->CreateAtomicCounterBuffer(
        "ac_buffer", NVRenderBufferUsageType::Static, sizeof(QT3DSU32), acData);
    if (!mAtomicCounterBuffer) {
        qWarning() << "NVRenderTestAtomicCounterBuffer: Failed to create atomic counter buffer";
        return false;
    }
    // add a parameter
    CRegisteredString theACName(context->GetStringTable().RegisterStr("ac_raster"));
    mAtomicCounterBuffer->AddParam(theACName, 0);

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    NVRenderVertFragCompilationResult compResult =
        context->CompileSource("NVRenderTestAtomicCounterBuffer shader",
                               toRef(vertShader(vtxProg, isGLESContext(context))),
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
        qWarning() << "NVRenderTestAtomicCounterBuffer: Failed to create vertex buffer";
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
        qWarning() << "NVRenderTestAtomicCounterBuffer: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);
    qt3ds::render::NVRenderCachedShaderBuffer<NVRenderShaderAtomicCounterBuffer *> atomicBuffer(
        "ac_buffer", *compResult.mShader);
    atomicBuffer.Set();

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(mInputAssembler->GetPrimitiveType(), 6, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestAtomicCounterBuffer::runPerformance(NVRenderContext *context,
                                                     userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestAtomicCounterBuffer::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
