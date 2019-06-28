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

#include "Qt3DSRenderTestTexture2D.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "render/Qt3DSRenderTexture2DArray.h"

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
            "in vec3 attr_uv;			// texture coord\n"
            "out vec3 varTexCoord;\n"
            "void main()\n"
            "{\n"
            "  gl_Position = mat_mvp * vec4(attr_pos, 1.0);\n"
            "  varTexCoord = attr_uv;\n"
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

    prog += "uniform sampler2DArray inTex;\n"
            "in vec3 varTexCoord;\n"
            "out vec4 fragColor;\n"
            "void main()\n"
            "{\n"
            "	fragColor = texture(inTex, varTexCoord);\n"
            "}\n";

    return prog.c_str();
}

struct Vertex
{
    QT3DSVec3 positions;
    QT3DSVec3 texCoord;
};

static const Vertex vertexPositionsL0[] = {
    { QT3DSVec3(-0.9, 0.0, 0), QT3DSVec3(0, 0, 0) }, { QT3DSVec3(0.0, 0.9, 0), QT3DSVec3(1, 1, 0) },
    { QT3DSVec3(-0.9, 0.9, 0), QT3DSVec3(0, 1, 0) }, { QT3DSVec3(-0.9, 0.0, 0), QT3DSVec3(0, 0, 0) },
    { QT3DSVec3(0.0, 0.0, 0), QT3DSVec3(1, 0, 0) },  { QT3DSVec3(0.0, 0.9, 0), QT3DSVec3(1, 1, 0) }
};

static const Vertex vertexPositionsL1[] = {
    { QT3DSVec3(0.0, -0.9, 0), QT3DSVec3(0, 0, 1) }, { QT3DSVec3(0.9, 0.0, 0), QT3DSVec3(1, 1, 1) },
    { QT3DSVec3(0.0, 0.0, 0), QT3DSVec3(0, 1, 1) },  { QT3DSVec3(0.0, -0.9, 0), QT3DSVec3(0, 0, 1) },
    { QT3DSVec3(0.9, -0.9, 0), QT3DSVec3(1, 0, 1) }, { QT3DSVec3(0.9, 0.0, 0), QT3DSVec3(1, 1, 1) }
};

#define TEXTURE_LAYER_SIZE 2
#define TEXTURE_SIZE 64
#define PATTERN_SIZE 0x8

NVRenderTestTexture2D::NVRenderTestTexture2D()
{
    _curTest = 0;
    _maxColumn = 4;
    _pTextureData = NULL;
}

NVRenderTestTexture2D::~NVRenderTestTexture2D()
{
}

bool NVRenderTestTexture2D::isSupported(NVRenderContext *context)
{
    return context->IsTextureArraySupported();
}

////////////////////////////////
// test for functionality
////////////////////////////////

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

bool NVRenderTestTexture2D::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    // alloc data
    _pTextureData = new unsigned char[TEXTURE_SIZE * TEXTURE_SIZE * 4 * TEXTURE_LAYER_SIZE];
    CreateTexData(_pTextureData);

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= texArray2DTest(context, pUserData);
    _curTest++;

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    if (_pTextureData)
        delete _pTextureData;

    return success;
}

void NVRenderTestTexture2D::CreateTexData(unsigned char *_pOutData)
{
    if (!_pOutData)
        return;

    unsigned char *_pData = _pOutData;

    // Create a checkerboard pattern
    for (int i = 0; i < TEXTURE_LAYER_SIZE; i++) {
        for (int j = 0; j < TEXTURE_SIZE; j++) {
            for (int k = 0; k < TEXTURE_SIZE; k++) {
                unsigned char c = (((j & PATTERN_SIZE) == 0) ^ ((k & PATTERN_SIZE) == 0)) * 255;
                *_pData++ = 0x0;
                *_pData++ = c >> i;
                *_pData++ = 0x0;
                *_pData++ = 0xFF;
            }
        }
    }
}

bool NVRenderTestTexture2D::renderTexArrayQuad(NVRenderContext *context, userContextData *pUserData,
                                               NVRenderTexture2DArray *pTex, QT3DSU8 *vertexPositions)
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
        "NVRenderTestTexture2D shader", toRef(vertShader(vtxProg, isGLESContext(context))),
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
        NVRenderVertexBufferEntry("attr_uv", NVRenderComponentTypes::QT3DSF32, 3, 12),
    };

    QT3DSU32 bufSize = 6 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData(vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                6 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestTexture2D: Failed to create vertex buffer";
        return false;
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 2));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler =
        context->CreateInputAssembler(mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), NULL,
                                      toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1));
    if (!mInputAssembler) {
        qWarning() << "NVRenderTestTexture2D: Failed to create input assembler";
        return false;
    }

    // make input assembler active
    context->SetInputAssembler(mInputAssembler);
    // set program
    context->SetActiveShader(compResult.mShader);
    NVRenderCachedShaderProperty<NVRenderTexture2DArray *> mArrayTexture("inTex",
                                                                         *compResult.mShader);
    mArrayTexture.Set(pTex);
    compResult.mShader->SetPropertyValue("mat_mvp", mvp);
    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(NVRenderDrawMode::Triangles, 6, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

bool NVRenderTestTexture2D::texArray2DTest(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // create texture
    NVScopedRefCounted<NVRenderTexture2DArray> mArrayTexture;
    mArrayTexture = context->CreateTexture2DArray();
    mArrayTexture->SetTextureData(
        NVDataRef<QT3DSU8>(_pTextureData, TEXTURE_SIZE * TEXTURE_SIZE * 4 * TEXTURE_LAYER_SIZE), 0,
        TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_LAYER_SIZE, NVRenderTextureFormats::RGBA8);

    success &= renderTexArrayQuad(context, pUserData, mArrayTexture, (QT3DSU8 *)vertexPositionsL0);
    success &= renderTexArrayQuad(context, pUserData, mArrayTexture, (QT3DSU8 *)vertexPositionsL1);

    return success;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestTexture2D::runPerformance(NVRenderContext *context, userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestTexture2D::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
