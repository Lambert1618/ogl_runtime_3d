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

#include "Qt3DSRenderTestFboMsaa.h"
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
           "gl_FragColor = vec4( color, 1.0);\n"
           "}\n";
}

struct Vertex
{
    QT3DSVec3 positions;
};

static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, -0.9, 0) },
                                          { QT3DSVec3(0.9, -0.9, 0) },
                                          { QT3DSVec3(0.0, 0.9, 0) } };

NVRenderTestFboMsaa::NVRenderTestFboMsaa()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestFboMsaa::~NVRenderTestFboMsaa()
{
}

bool NVRenderTestFboMsaa::isSupported(NVRenderContext *context)
{
    NVRenderContextType ctxType = context->GetRenderContextType();
    NVRenderContextType nonSupportedFlags(NVRenderContextValues::GL2 | NVRenderContextValues::GLES2
                                          | NVRenderContextValues::GLES3);

    // This is currently only supported on >= GL3 && >= GLES 3.1
    if ((ctxType & nonSupportedFlags))
        return false;

    return true;
}

bool NVRenderTestFboMsaa::run(NVRenderContext *context, userContextData *pUserData)
{
    if (!setupResolveFbo(context, pUserData))
        return false;

    bool success = true;
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetRenderTarget(NULL);
    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= simpleMsaaTest(context, pUserData);
    _curTest++;

    // cleanup
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    return success;
}

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

bool NVRenderTestFboMsaa::renderTriangle(NVRenderContext *context, QT3DSVec3 color)
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

    QT3DSU32 bufSize = 3 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                3 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestFboMsaa: Failed to create vertex buffer";
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
        qWarning() << "NVRenderTestFboMsaa: Failed to create input assembler";
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
    context->Draw(NVRenderDrawMode::Triangles, 3, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

bool NVRenderTestFboMsaa::setupResolveFbo(NVRenderContext *context, userContextData *pUserData)
{
    // color texture
    m_ResolveColorTexture = context->CreateTexture2D();
    m_ResolveColorTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, pUserData->winWidth,
                                          pUserData->winHeight, NVRenderTextureFormats::RGBA8);
    // depth texture
    m_ResolveDepthTexture = context->CreateTexture2D();
    m_ResolveDepthTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, pUserData->winWidth,
                                          pUserData->winHeight, NVRenderTextureFormats::Depth24);
    // create resolve FBO
    m_ResolveFbo = context->CreateFrameBuffer();
    m_ResolveFbo->Attach(NVRenderFrameBufferAttachments::Color0,
                         NVRenderTextureOrRenderBuffer(*m_ResolveColorTexture));
    m_ResolveFbo->Attach(NVRenderFrameBufferAttachments::Depth,
                         NVRenderTextureOrRenderBuffer(*m_ResolveDepthTexture));

    return m_ResolveFbo->IsComplete();
}

bool NVRenderTestFboMsaa::simpleMsaaTest(NVRenderContext *context, userContextData *pUserData)
{
    // create a multisampled FBO
    NVScopedRefCounted<NVRenderFrameBuffer> msFBO;
    NVScopedRefCounted<NVRenderTexture2D> msColorTexture;
    NVScopedRefCounted<NVRenderTexture2D> msDepth24Texture;

    msColorTexture = context->CreateTexture2D();
    msColorTexture->SetTextureDataMultisample(4, pUserData->winWidth, pUserData->winHeight,
                                              NVRenderTextureFormats::RGBA8);
    msDepth24Texture = context->CreateTexture2D();
    msDepth24Texture->SetTextureDataMultisample(4, pUserData->winWidth, pUserData->winHeight,
                                                NVRenderTextureFormats::Depth24);

    msFBO = context->CreateFrameBuffer();
    msFBO->Attach(NVRenderFrameBufferAttachments::Color0,
                  NVRenderTextureOrRenderBuffer(*msColorTexture),
                  NVRenderTextureTargetType::Texture2D_MS);
    msFBO->Attach(NVRenderFrameBufferAttachments::Depth,
                  NVRenderTextureOrRenderBuffer(*msDepth24Texture),
                  NVRenderTextureTargetType::Texture2D_MS);

    if (!msFBO->IsComplete())
        return false;

    // clear and draw to multisampled buffer
    context->SetRenderTarget(msFBO);
    context->SetMultisampleEnabled(true);
    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));
    renderTriangle(context, qt3ds::QT3DSVec3(0.0, 1.0, 0.0));
    context->SetMultisampleEnabled(false);

    // do resolve blit
    // first we must setup the render target
    context->SetRenderTarget(m_ResolveFbo);
    // second setup read target
    context->SetReadTarget(msFBO);
    context->SetReadBuffer(NVReadFaces::Color0);

    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    context->BlitFramebuffer(0, 0, pUserData->winWidth, pUserData->winHeight, 0, 0,
                             pUserData->winWidth, pUserData->winHeight, NVRenderClearValues::Color,
                             NVRenderTextureMagnifyingOp::Nearest);

    // copy to default buffer
    // first we must setup the render target
    context->SetRenderTarget(NULL);
    // second setup read target
    context->SetReadTarget(m_ResolveFbo);
    context->SetReadBuffer(NVReadFaces::Color0);

    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    context->BlitFramebuffer(0, 0, pUserData->winWidth, pUserData->winHeight, 0, 0,
                             pUserData->winWidth, pUserData->winHeight, NVRenderClearValues::Color,
                             NVRenderTextureMagnifyingOp::Nearest);

    context->SetReadTarget(NULL);

    return true;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestFboMsaa::runPerformance(NVRenderContext *context, userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestFboMsaa::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    m_ResolveColorTexture->release();
    m_ResolveDepthTexture->release();
    m_ResolveFbo->release();

    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));

    context->SetRenderTarget(NULL);
}
