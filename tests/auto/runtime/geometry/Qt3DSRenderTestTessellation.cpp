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

#include "Qt3DSRenderTestTessellation.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/backends/gl/Qt3DSOpenGLUtil.h"

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

static const char *vertPhongShader(std::string &prog, bool binESContext)
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
            "in vec3 attr_norm;			// normal pos\n"
            "out vec3 ctNorm;			// output normal control patch\n"
            "void main()\n"
            "{\n"
            "	ctNorm = attr_norm;\n"
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

static const char *fragPhongShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 400\n";
    }

    prog += "uniform vec3 color;\n"
            "in vec3 normWorld;\n"
            "out vec4 fragColor;\n"
            "void main()\n"
            "{\n"
            "	fragColor = vec4(color, 1.0);\n"
            "}\n";

    return prog.c_str();
}

static const char *tessControlShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_EXT_tessellation_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 400\n";
    }

    prog += "// number of CPs in patch\n"
            "layout (vertices = 3) out;\n"
            "uniform float tessLevelInner; // controlled by keyboard buttons\n"
            "uniform float tessLevelOuter; // controlled by keyboard buttons\n"
            "void main () {\n"
            "gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
            "// Calculate the tessellation levels\n"
            "gl_TessLevelInner[0] = tessLevelInner; // number of nested primitives to generate\n"
            "gl_TessLevelOuter[0] = tessLevelOuter; // times to subdivide first side\n"
            "gl_TessLevelOuter[1] = tessLevelOuter; // times to subdivide second side\n"
            "gl_TessLevelOuter[2] = tessLevelOuter; // times to subdivide third side\n"
            "}\n";

    return prog.c_str();
}

static const char *tessPhongControlShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_EXT_tessellation_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 400\n";
    }

    prog +=
        "// number of CPs in patch\n"
        "layout (vertices = 3) out;\n"
        "// phong tessellation per patch data\n"
        "struct PhongTessPatch {\n"
        "  float projIJ;\n"
        "  float projJK;\n"
        "  float projIK;\n"
        "};\n"
        "in vec3 ctNorm[];				// control point normal\n"
        "out vec3 normObj[];			// output normal pos\n"
        "uniform float tessLevels;\n"
        "uniform float tessBlend;\n"
        "out PhongTessPatch tcTessPatch[3];\n"
        "float PIi(int i, vec3 q)\n"
        "{\n"
        "  vec3 q_minus_p = q - gl_in[i].gl_Position.xyz;\n"
        "  return q[gl_InvocationID] - dot(q_minus_p, ctNorm[i]) * ctNorm[i][gl_InvocationID];\n"
        "}\n"
        "void main () {\n"
        "  // path through data\n"
        "  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
        "  normObj[gl_InvocationID] = ctNorm[gl_InvocationID];\n"
        "  // compute projections separate for each xyz component\n"
        "  tcTessPatch[gl_InvocationID].projIJ = PIi(0, gl_in[1].gl_Position.xyz) + PIi(1, "
        "gl_in[0].gl_Position.xyz);\n"
        "  tcTessPatch[gl_InvocationID].projJK = PIi(1, gl_in[2].gl_Position.xyz) + PIi(2, "
        "gl_in[1].gl_Position.xyz);\n"
        "  tcTessPatch[gl_InvocationID].projIK = PIi(2, gl_in[0].gl_Position.xyz) + PIi(0, "
        "gl_in[2].gl_Position.xyz);\n"
        "  // set the tessellation levels\n"
        "  gl_TessLevelInner[0] = tessLevels; // number of nested primitives to generate\n"
        "  gl_TessLevelOuter[gl_InvocationID] = tessLevels; // times to subdivide first side\n"
        "}\n";

    return prog.c_str();
}

static const char *tessEvaluationShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_EXT_tessellation_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 400\n";
    }

    prog += "// triangles, quads, or isolines\n"
            "layout (triangles, equal_spacing, ccw) in;\n"
            "uniform mat4 mat_mvp;\n"
            "// gl_TessCoord is location within the patch\n"
            "// (barycentric for triangles, UV for quads)\n"
            "void main () {\n"
            "vec4 p0 = gl_TessCoord.x * gl_in[0].gl_Position; // x is one corner\n"
            "vec4 p1 = gl_TessCoord.y * gl_in[1].gl_Position; // y is the 2nd corner\n"
            "vec4 p2 = gl_TessCoord.z * gl_in[2].gl_Position; // z is the 3rd corner (ignore when "
            "using quads)\n"
            "vec4 pos = p0 + p1 + p2;\n"
            "gl_Position = mat_mvp * pos;\n"
            "}\n";

    return prog.c_str();
}

static const char *tessPhongEvaluationShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_EXT_tessellation_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n";
    } else {
        prog += "#version 400\n";
    }

    prog += "// triangles, quads, or isolines\n"
            "layout (triangles, fractional_odd_spacing, ccw) in;\n"
            "struct PhongTessPatch {\n"
            "  float projIJ;\n"
            "  float projJK;\n"
            "  float projIK;\n"
            "};\n"
            "in vec3 normObj[];				// control point normal\n"
            "out vec3 normWorld;				// output normal pos\n"
            "in PhongTessPatch tcTessPatch[];\n"
            "uniform mat4 mat_mvp;\n"
            "uniform float tessBlend;\n"
            "// gl_TessCoord is location within the patch\n"
            "// (barycentric for triangles, UV for quads)\n"
            "void main () {\n"
            "  // output normal\n"
            "  // pre compute square tesselation coord\n"
            "  vec3 tessSquared = gl_TessCoord * gl_TessCoord;\n"
            "  vec3 norm = gl_TessCoord.x * normObj[0]\n"
            "		     + gl_TessCoord.y * normObj[1]\n"
            "		     + gl_TessCoord.z * normObj[2]; // z is the 3rd corner (ignore when "
            "using quads)\n"
            "  // barycentric linear position\n"
            "  vec3 linearPos = gl_TessCoord.x * gl_in[0].gl_Position.xyz\n"
            "		    + gl_TessCoord.y * gl_in[1].gl_Position.xyz\n"
            "		    + gl_TessCoord.z * gl_in[2].gl_Position.xyz;\n"
            "  // projective terms\n"
            "  vec3 projJI = vec3(tcTessPatch[0].projIJ, tcTessPatch[1].projIJ, "
            "tcTessPatch[2].projIJ);\n"
            "  vec3 projKJ = vec3(tcTessPatch[0].projJK, tcTessPatch[1].projJK, "
            "tcTessPatch[2].projJK);\n"
            "  vec3 projIK = vec3(tcTessPatch[0].projIK, tcTessPatch[1].projIK, "
            "tcTessPatch[2].projIK);\n"
            "  // phong interpolated position\n"
            "  vec3 phongPos = tessSquared.x * gl_in[0].gl_Position.xyz\n"
            "				 + tessSquared.y * gl_in[1].gl_Position.xyz\n"
            "				 + tessSquared.z * gl_in[2].gl_Position.xyz\n"
            "				 + gl_TessCoord.x * gl_TessCoord.y * projJI\n"
            "				 + gl_TessCoord.y * gl_TessCoord.z * projKJ\n"
            "				 + gl_TessCoord.z * gl_TessCoord.x * projIK;\n"
            "  // final position\n"
            "  vec3 finalPos = (1.0-tessBlend)*linearPos + tessBlend*phongPos;\n"
            "  gl_Position = mat_mvp * vec4(finalPos, 1.0);\n"
            "  normWorld = norm;\n"
            "}\n";

    return prog.c_str();
}

struct Vertex
{
    QT3DSVec3 positions;
    QT3DSVec3 normals;
};

NVRenderTestTessellation::NVRenderTestTessellation()
{
    _curTest = 0;
    _maxColumn = 4;
}

NVRenderTestTessellation::~NVRenderTestTessellation()
{
}

bool NVRenderTestTessellation::isSupported(NVRenderContext *context)
{
    NVRenderContextType ctxType = context->GetRenderContextType();
    NVRenderContextType nonSupportedFlags(
        NVRenderContextValues::GL2 | NVRenderContextValues::GLES2 | NVRenderContextValues::GL3
        | NVRenderContextValues::GLES3 | NVRenderContextValues::GLES3PLUS);

    // This is currently only supported on >= GL4 && >= GLES 3.1
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

bool NVRenderTestTessellation::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;

    context->SetRenderTarget(NULL);
    // conpute cell width
    _cellSize = pUserData->winWidth / _maxColumn;

    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 1.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color | NVRenderClearValues::Depth));

    success &= trianglePatches(context, pUserData);
    _curTest++;
    success &= phongPatches(context, pUserData);
    _curTest++;

    return success;
}

bool NVRenderTestTessellation::trianglePatches(NVRenderContext *context, userContextData *pUserData)
{
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, -0.9, 0) },
                                              { QT3DSVec3(0.9, -0.9, 0) },
                                              { QT3DSVec3(0.0, 0.9, 0) } };

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
    std::string tcProg;
    std::string teProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestTessellation shader", toRef(vertShader(vtxProg, isGLESContext(context))),
        toRef(fragShader(frgProg, isGLESContext(context))),
        toRef(tessControlShader(tcProg, isGLESContext(context))),
        toRef(tessEvaluationShader(teProg, isGLESContext(context))));
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
                                                6 * sizeof(QT3DSF32), vertData);
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
        toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1), NVRenderDrawMode::Patches, 3);
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
    // set tessellation values
    float tessLevelInner = 4.0;
    compResult.mShader->SetPropertyValue("tessLevelInner", tessLevelInner);
    float tessLevelOuter = 4.0;
    compResult.mShader->SetPropertyValue("tessLevelOuter", tessLevelOuter);

    context->SetDepthTestEnabled(true);
    context->SetDepthWriteEnabled(true);

    // draw
    context->Draw(mInputAssembler->GetPrimitiveType(), 3, 0);

    context->SetActiveShader(0);
    compResult.mShader->release();

    return true;
}

bool NVRenderTestTessellation::phongPatches(NVRenderContext *context, userContextData *pUserData)
{
    QT3DSVec3 n1(-1.0, 0.5, 1.0);
    n1.normalize();
    QT3DSVec3 n2(1.0, 0.5, 1.0);
    n2.normalize();
    QT3DSVec3 n3(0.0, 1.0, 1.0);
    n3.normalize();
    static const Vertex vertexPositions[] = { { QT3DSVec3(-0.9, -0.9, 0.0), n1 },
                                              { QT3DSVec3(0.9, -0.9, 0.0), n2 },
                                              { QT3DSVec3(0.0, 0.9, -0.0), n3 } };

    qt3ds::QT3DSVec3 color(0.0, 1.0, 0.0);

    NVScopedRefCounted<NVRenderVertexBuffer> mVertexBuffer;
    NVScopedRefCounted<NVRenderAttribLayout> mAttribLayout;
    NVScopedRefCounted<NVRenderInputAssembler> mInputAssembler;
    NVScopedRefCounted<NVRenderIndexBuffer> mIndexBuffer;
    QT3DSMat44 p = QT3DSMat44::createIdentity();
    QT3DSMat44 rotX = QT3DSMat44::createIdentity();
    QT3DSMat44 rotY = QT3DSMat44::createIdentity();
    QT3DSMat44 mv = QT3DSMat44::createIdentity();
    QT3DSMat44 mvp = QT3DSMat44::createIdentity();
    NvGl2DemoMatrixOrtho(&p, -1, 1, -1, 1, -10, 10);
    // NvRenderTestMatrixRotY( &rotY, 45.0 );
    // NvRenderTestMatrixRotX( &rotX, 90.0 );
    mv = rotY * rotX;
    mvp = mv * p;

    // create shaders
    std::string vtxProg;
    std::string frgProg;
    std::string tcProg;
    std::string teProg;
    NVRenderVertFragCompilationResult compResult = context->CompileSource(
        "NVRenderTestTessellation shader", toRef(vertPhongShader(vtxProg, isGLESContext(context))),
        toRef(fragPhongShader(frgProg, isGLESContext(context))),
        toRef(tessPhongControlShader(tcProg, isGLESContext(context))),
        toRef(tessPhongEvaluationShader(teProg, isGLESContext(context))));
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
        NVRenderVertexBufferEntry("attr_norm", NVRenderComponentTypes::QT3DSF32, 3, 12),
    };

    QT3DSU32 bufSize = 3 * sizeof(Vertex);
    NVDataRef<QT3DSU8> vertData((QT3DSU8 *)vertexPositions, bufSize);
    mVertexBuffer = context->CreateVertexBuffer(NVRenderBufferUsageType::Static, bufSize,
                                                6 * sizeof(QT3DSF32), vertData);
    if (!mVertexBuffer) {
        qWarning() << "NVRenderTestFboMsaa: Failed to create vertex buffer";
        return false;
    }

    // create our attribute layout
    mAttribLayout = context->CreateAttributeLayout(toConstDataRef(entries, 2));
    // create input Assembler
    QT3DSU32 strides = mVertexBuffer->GetStride();
    QT3DSU32 offsets = 0;
    mInputAssembler = context->CreateInputAssembler(
        mAttribLayout, toConstDataRef(&mVertexBuffer.mPtr, 1), mIndexBuffer.mPtr,
        toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1), NVRenderDrawMode::Patches, 3);
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
    // set tessellation values
    float tessLevels = 8.0;
    compResult.mShader->SetPropertyValue("tessLevels", tessLevels);
    float tessBlend = 1.0;
    compResult.mShader->SetPropertyValue("tessBlend", tessBlend);

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
bool NVRenderTestTessellation::runPerformance(NVRenderContext *context, userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestTessellation::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(.0f, .0f, .0f, 0.f));
    // dummy
    context->SetViewport(NVRenderRect(0, 0, pUserData->winWidth, pUserData->winHeight));
}
