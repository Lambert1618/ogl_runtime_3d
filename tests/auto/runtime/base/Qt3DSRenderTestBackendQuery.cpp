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

#include "Qt3DSRenderTestBackendQuery.h"
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
           "uniform vec3 color;\n"
           "void main()\n"
           "{\n"
           "gl_FragColor = vec4( color, 1);\n"
           "}\n";
}

struct Vertex
{
    QT3DSVec3 positions;
};

static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, 0.9, 0) }, { QT3DSVec3(-0.9, -0.9, 0) },
                                          { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(-0.9, 0.9, 0) },
                                          { QT3DSVec3(0.9, -0.9, 0) }, { QT3DSVec3(0.9, 0.9, 0) } };

NVRenderTestBackendQuery::NVRenderTestBackendQuery()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestBackendQuery::~NVRenderTestBackendQuery()
{
}

bool NVRenderTestBackendQuery::isSupported(NVRenderContext *context)
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

bool NVRenderTestBackendQuery::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= depthBitsTest(context, pUserData);
    _curTest++;
    success &= depthBitsFBOTest(context, pUserData);
    _curTest++;

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    return success;
}

bool NVRenderTestBackendQuery::renderQuad(NVRenderContext *context, userContextData *pUserData,
                                          QT3DSVec3 color)
{
    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&mvp, -1, 1, -1, 1, -10, 10);

    // create shaders
    NVRenderVertFragCompilationResult compResult =
        context->CompileSource("NVRenderTestBackendQuery shader", toRef(PassthroughVertShader()),
                               toRef(SimpleFragShader()));
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
        qWarning() << "NVRenderTestBackendQuery::depthBitsTest: Failed to create vertex buffer";
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
        qWarning() << "NVRenderTestBackendQuery::depthBitsTest: Failed to create input assembler";
        return false;
    }

    // check if default buffer bit size is in an acceptable size range
    // we accept a range from 16 to 32
    const QT3DSU32 bits = context->GetDepthBits();
    bool passed = (bits >= 16 && bits <= 32);

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

    return passed;
}

bool NVRenderTestBackendQuery::depthBitsTest(NVRenderContext *context, userContextData *pUserData)
{
    QT3DSVec3 color(0.0, 0.0, 0.0);
    // check if default buffer bit size is in an acceptable size range
    // we accept a range from 16 to 32
    const QT3DSU32 bits = context->GetDepthBits();
    bool passed = (bits >= 16 && bits <= 32);
    if (passed)
        color.y = 1.0;
    else
        color.x = 1.0;

    passed &= renderQuad(context, pUserData, color);

    return passed;
}

bool NVRenderTestBackendQuery::depthBitsFBOTest(NVRenderContext *context,
                                                userContextData *pUserData)
{
    // depneding on the context we get different values
    NVRenderContextType theContextFlags(NVRenderContextValues::GLES2 | NVRenderContextValues::GL2);
    NVRenderContextType type = context->GetRenderContextType();
    bool depth16_Only = (type & theContextFlags);
    // create a FBO
    NVScopedRefCounted<NVRenderFrameBuffer> m_FBO;
    NVScopedRefCounted<NVRenderTexture2D> m_ColorTexture;
    NVScopedRefCounted<NVRenderTexture2D> m_Depth16Texture;
    NVScopedRefCounted<NVRenderTexture2D> m_Depth24Texture;
    NVScopedRefCounted<NVRenderTexture2D> m_Depth32Texture;
    NVScopedRefCounted<NVRenderTexture2D> m_Depth24Stencil8Texture;

    m_ColorTexture = context->CreateTexture2D();
    m_ColorTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, 256, 256, NVRenderTextureFormats::RGBA8);
    m_Depth16Texture = context->CreateTexture2D();
    m_Depth16Texture->SetTextureData(NVDataRef<QT3DSU8>(), 0, 256, 256,
                                     NVRenderTextureFormats::Depth16);

    m_FBO = context->CreateFrameBuffer();
    m_FBO->Attach(NVRenderFrameBufferAttachments::Color0,
                  NVRenderTextureOrRenderBuffer(*m_ColorTexture));
    m_FBO->Attach(NVRenderFrameBufferAttachments::Depth,
                  NVRenderTextureOrRenderBuffer(*m_Depth16Texture));

    if (!m_FBO->IsComplete()) {
        qWarning() << "NVRenderTestBackendQuery::depthBitsFBOTest: Failed to create FBO";
        return false;
    }

    context->SetRenderTarget(m_FBO);

    QT3DSVec3 color(0.0, 0.0, 0.0);
    // check depth bit count
    QT3DSU32 bits = context->GetDepthBits();
    QT3DSU32 bitsExpected = 16;
    bool passed = (bits == bitsExpected);

    // detach depth
    m_FBO->Attach(NVRenderFrameBufferAttachments::Depth, NVRenderTextureOrRenderBuffer());

    m_Depth24Texture = context->CreateTexture2D();
    m_Depth24Texture->SetTextureData(NVDataRef<QT3DSU8>(), 0, 256, 256,
                                     NVRenderTextureFormats::Depth24);
    m_FBO->Attach(NVRenderFrameBufferAttachments::Depth,
                  NVRenderTextureOrRenderBuffer(*m_Depth24Texture));
    if (!m_FBO->IsComplete()) {
        qWarning() << "NVRenderTestBackendQuery::depthBitsFBOTest: Failed to create FBO";
        return false;
    }
    // check depth bit count
    bits = context->GetDepthBits();
    bitsExpected = (depth16_Only) ? 16 : 24;
    passed &= (bits == bitsExpected);

    // detach depth
    m_FBO->Attach(NVRenderFrameBufferAttachments::Depth, NVRenderTextureOrRenderBuffer());

    m_Depth32Texture = context->CreateTexture2D();
    m_Depth32Texture->SetTextureData(NVDataRef<QT3DSU8>(), 0, 256, 256,
                                     NVRenderTextureFormats::Depth32);
    m_FBO->Attach(NVRenderFrameBufferAttachments::Depth,
                  NVRenderTextureOrRenderBuffer(*m_Depth32Texture));
    if (!m_FBO->IsComplete()) {
        qWarning() << "NVRenderTestBackendQuery::depthBitsFBOTest: Failed to create FBO";
        return false;
    }
    // check depth bit count
    bits = context->GetDepthBits();
    bitsExpected = (depth16_Only) ? 16 : 32;
    passed &= (bits == bitsExpected);

    // detach depth
    m_FBO->Attach(NVRenderFrameBufferAttachments::Depth, NVRenderTextureOrRenderBuffer());

    // only test depth stencil if supported
    if (context->IsDepthStencilSupported()) {
        m_Depth24Stencil8Texture = context->CreateTexture2D();
        m_Depth24Stencil8Texture->SetTextureData(NVDataRef<QT3DSU8>(), 0, 256, 256,
                                                 NVRenderTextureFormats::Depth24Stencil8);
        m_FBO->Attach(NVRenderFrameBufferAttachments::DepthStencil,
                      NVRenderTextureOrRenderBuffer(*m_Depth24Stencil8Texture));
        if (!m_FBO->IsComplete()) {
            qWarning() << "NVRenderTestBackendQuery::depthBitsFBOTest: Failed to create FBO";
            return false;
        }
        // check depth bit count
        bits = context->GetDepthBits();
        bitsExpected = (depth16_Only) ? 16 : 24;
        passed &= (bits == bitsExpected);
    }

    context->SetRenderTarget(NULL);

    if (passed)
        color.y = 1.0;
    else
        color.x = 1.0;

    passed &= renderQuad(context, pUserData, color);

    return passed;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestBackendQuery::runPerformance(NVRenderContext *context, userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestBackendQuery::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
