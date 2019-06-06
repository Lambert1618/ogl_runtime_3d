/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "Qt3DSRenderPathManager.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSContainers.h"
#include "EASTL/string.h"
#include "Qt3DSRenderContextCore.h"
#include "foundation/Utils.h"
#include "foundation/StringConversionImpl.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "render/Qt3DSRenderInputAssembler.h"
#include "Qt3DSRenderPath.h"
#include "EASTL/sort.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderVertexBuffer.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "Qt3DSRenderShaderCodeGenerator.h"
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderPathRenderContext.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "Qt3DSRenderDefaultMaterialShaderGenerator.h"
#include "Qt3DSRenderCustomMaterialShaderGenerator.h"
#include "Qt3DSRenderCustomMaterial.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "rendererimpl/Qt3DSVertexPipelineImpl.h"
#include "foundation/Qt3DSMathUtils.h"
#include "render/Qt3DSRenderPathRender.h"
#include "render/Qt3DSRenderPathSpecification.h"
#include "Qt3DSRenderPathSubPath.h"
#include "Qt3DSImportPath.h"
#include "Qt3DSRenderPathMath.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "foundation/Qt3DSMutex.h"

using namespace qt3ds::render;
using qt3ds::render::NVRenderCachedShaderProperty;
using qt3ds::render::NVRenderCachedShaderBuffer;
using qt3ds::render::NVRenderStencilFunctionArgument;
using qt3ds::render::NVRenderBoolOp;
using qt3ds::render::NVRenderStencilOperationArgument;
using qt3ds::render::NVRenderStencilOp;

typedef qt3dsimp::SPathBuffer TImportPathBuffer;
using namespace qt3ds::render::path;

typedef eastl::pair<CRegisteredString, CRegisteredString> TStrStrPair;

namespace eastl {
template <>
struct hash<TStrStrPair>
{
    size_t operator()(const TStrStrPair &item) const
    {
        return eastl::hash<CRegisteredString>()(item.first)
            ^ eastl::hash<CRegisteredString>()(item.second);
    }
};
}

struct SPathShaderMapKey
{
    CRegisteredString m_Name;
    SShaderDefaultMaterialKey m_MaterialKey;
    size_t m_HashCode;
    SPathShaderMapKey(CRegisteredString inName, SShaderDefaultMaterialKey inKey)
        : m_Name(inName)
        , m_MaterialKey(inKey)
    {
        m_HashCode = eastl::hash<TStrStrPair>()(m_Name) ^ m_MaterialKey.hash();
    }
    bool operator==(const SPathShaderMapKey &inKey) const
    {
        return m_Name == inKey.m_Name && m_MaterialKey == inKey.m_MaterialKey;
    }
};

namespace eastl {
template <>
struct hash<SPathShaderMapKey>
{
    size_t operator()(const SPathShaderMapKey &inKey) const { return inKey.m_HashCode; }
};
}

namespace {

struct SPathSubPathBuffer
{
    NVAllocatorCallback &m_Allocator;
    nvvector<SPathAnchorPoint> m_SourceData;
    SPathDirtyFlags m_Flags;
    SPathSubPath &m_SubPath;
    bool m_Closed;

    QT3DSI32 m_RefCount;

    SPathSubPathBuffer(NVAllocatorCallback &alloc, SPathSubPath &inSubPath)
        : m_Allocator(alloc)
        , m_SourceData(alloc, "m_SourceData")
        , m_SubPath(inSubPath)
        , m_Closed(false)
        , m_RefCount(0)
    {
    }

    void addRef() { atomicIncrement(&m_RefCount); }
    void release()
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(m_Allocator);
            NVDelete(alloc, this);
        }
    }
};

struct SImportPathWrapper
{
    NVAllocatorCallback &m_Alloc;
    qt3dsimp::SPathBuffer *m_Path;
    QT3DSI32 m_RefCount;

    SImportPathWrapper(NVAllocatorCallback &inAlloc, qt3dsimp::SPathBuffer &inPath)
        : m_Alloc(inAlloc)
        , m_Path(&inPath)
        , m_RefCount(0)
    {
    }

    ~SImportPathWrapper() { m_Path->Free(m_Alloc); }

    void addRef() { ++m_RefCount; }
    void release()
    {
        --m_RefCount;
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(m_Alloc);
            NVDelete(alloc, this);
        }
    }
};

typedef NVScopedRefCounted<SImportPathWrapper> TPathBufferPtr;

struct SPathBuffer
{
    NVAllocatorCallback &m_Allocator;
    nvvector<NVScopedRefCounted<SPathSubPathBuffer>> m_SubPaths;
    TPathBufferPtr m_PathBuffer;

    NVScopedRefCounted<NVRenderVertexBuffer> m_PatchData;
    NVScopedRefCounted<NVRenderInputAssembler> m_InputAssembler;
    NVScopedRefCounted<NVRenderPathRender> m_PathRender;

    QT3DSVec2 m_BeginTaperData;
    QT3DSVec2 m_EndTaperData;
    QT3DSU32 m_NumVertexes;
    PathTypes::Enum m_PathType;
    QT3DSF32 m_Width;
    QT3DSF32 m_CPUError;
    NVBounds3 m_Bounds;
    Option<STaperInformation> m_BeginTaper;
    Option<STaperInformation> m_EndTaper;
    CRegisteredString m_SourcePath;

    // Cached data for geometry paths

    SPathDirtyFlags m_Flags;

    QT3DSI32 m_RefCount;

    SPathBuffer(NVAllocatorCallback &alloc)
        : m_Allocator(alloc)
        , m_SubPaths(alloc, "m_SubPaths")
        , m_NumVertexes(0)
        , m_PathType(PathTypes::Geometry)
        , m_Width(0.0f)
        , m_CPUError(0.0f)
        , m_Bounds(NVBounds3::empty())
        , m_RefCount(0)
    {
    }

    void addRef() { atomicIncrement(&m_RefCount); }
    void release()
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(m_Allocator);
            NVDelete(alloc, this);
        }
    }

    void ClearGeometryPathData()
    {
        m_PatchData = NULL;
        m_InputAssembler = NULL;
    }

    void ClearPaintedPathData() { m_PathRender = NULL; }

    qt3dsimp::SPathBuffer GetPathData(qt3dsimp::IPathBufferBuilder &inSpec)
    {
        if (m_SubPaths.size()) {
            inSpec.Clear();
            for (QT3DSU32 idx = 0, end = m_SubPaths.size(); idx < end; ++idx) {
                const SPathSubPathBuffer &theSubPathBuffer(*m_SubPaths[idx]);
                for (QT3DSU32 equationIdx = 0, equationEnd = theSubPathBuffer.m_SourceData.size();
                     equationIdx < equationEnd; ++equationIdx) {
                    const SPathAnchorPoint &thePoint = theSubPathBuffer.m_SourceData[equationIdx];
                    if (equationIdx == 0) {
                        inSpec.MoveTo(thePoint.m_Position);
                    } else {
                        const SPathAnchorPoint &thePrevPoint =
                            theSubPathBuffer.m_SourceData[equationIdx - 1];
                        QT3DSVec2 c1 = IPathManager::GetControlPointFromAngleDistance(
                            thePrevPoint.m_Position, thePrevPoint.m_OutgoingAngle,
                            thePrevPoint.m_OutgoingDistance);
                        QT3DSVec2 c2 = IPathManager::GetControlPointFromAngleDistance(
                            thePoint.m_Position, thePoint.m_IncomingAngle,
                            thePoint.m_IncomingDistance);
                        QT3DSVec2 p2 = thePoint.m_Position;
                        inSpec.CubicCurveTo(c1, c2, p2);
                    }
                }
                if (theSubPathBuffer.m_Closed)
                    inSpec.Close();
            }
            return inSpec.GetPathBuffer();
        } else if (m_PathBuffer.mPtr)
            return *m_PathBuffer.mPtr->m_Path;
        return qt3dsimp::SPathBuffer();
    }

    void SetPathType(PathTypes::Enum inPathType)
    {
        if (inPathType != m_PathType) {
            switch (m_PathType) {
            case PathTypes::Geometry:
                ClearGeometryPathData();
                break;
            case PathTypes::Painted:
                ClearPaintedPathData();
                break;
            default:
                QT3DS_ALWAYS_ASSERT_MESSAGE("Unexpected path type");
                // No further processing for unexpected path type
                return;
            }
            m_Flags.clearOrSet(true, PathDirtyFlagValues::PathType);
        }
        m_PathType = inPathType;
    }

    static Option<STaperInformation> ToTaperInfo(PathCapping::Enum capping, QT3DSF32 capOffset,
                                                 QT3DSF32 capOpacity, QT3DSF32 capWidth)
    {
        if (capping == PathCapping::Noner)
            return Empty();

        return STaperInformation(capOffset, capOpacity, capWidth);
    }

    void SetBeginTaperInfo(PathCapping::Enum capping, QT3DSF32 capOffset, QT3DSF32 capOpacity,
                           QT3DSF32 capWidth)
    {
        Option<STaperInformation> newBeginInfo =
            ToTaperInfo(capping, capOffset, capOpacity, capWidth);
        if (!OptionEquals(newBeginInfo, m_BeginTaper)) {
            m_BeginTaper = newBeginInfo;
            m_Flags.clearOrSet(true, PathDirtyFlagValues::BeginTaper);
        }
    }

    void SetEndTaperInfo(PathCapping::Enum capping, QT3DSF32 capOffset, QT3DSF32 capOpacity,
                         QT3DSF32 capWidth)
    {
        Option<STaperInformation> newEndInfo =
            ToTaperInfo(capping, capOffset, capOpacity, capWidth);
        if (!OptionEquals(newEndInfo, m_EndTaper)) {
            m_EndTaper = newEndInfo;
            m_Flags.clearOrSet(true, PathDirtyFlagValues::EndTaper);
        }
    }

    void SetWidth(QT3DSF32 inWidth)
    {
        if (inWidth != m_Width) {
            m_Width = inWidth;
            m_Flags.clearOrSet(true, PathDirtyFlagValues::Width);
        }
    }

    void SetCPUError(QT3DSF32 inError)
    {
        if (inError != m_CPUError) {
            m_CPUError = inError;
            m_Flags.clearOrSet(true, PathDirtyFlagValues::CPUError);
        }
    }
};

struct SPathGeneratedShader
{
    NVAllocatorCallback &m_Allocator;
    NVRenderShaderProgram &m_Shader;
    NVRenderCachedShaderProperty<QT3DSF32> m_Width;
    NVRenderCachedShaderProperty<QT3DSF32> m_InnerTessAmount;
    NVRenderCachedShaderProperty<QT3DSF32> m_EdgeTessAmount;
    NVRenderCachedShaderProperty<QT3DSVec2> m_BeginTaperData;
    NVRenderCachedShaderProperty<QT3DSVec2> m_EndTaperData;
    NVRenderCachedShaderProperty<QT3DSMat44> m_WireframeViewMatrix;

    QT3DSI32 m_RefCount;

    SPathGeneratedShader(NVRenderShaderProgram &sh, NVAllocatorCallback &alloc)
        : m_Allocator(alloc)
        , m_Shader(sh)
        , m_Width("pathWidth", sh)
        , m_InnerTessAmount("tessInnerLevel", sh)
        , m_EdgeTessAmount("tessEdgeLevel", sh)
        , m_BeginTaperData("beginTaperInfo", sh)
        , m_EndTaperData("endTaperInfo", sh)
        , m_WireframeViewMatrix("viewport_matrix", sh)
        , m_RefCount(0)
    {
        m_Shader.addRef();
    }
    ~SPathGeneratedShader() { m_Shader.release(); }

    void addRef() { ++m_RefCount; }
    void release()
    {
        --m_RefCount;
        if (m_RefCount <= 0) {
            NVAllocatorCallback &allocator(m_Allocator);
            NVDelete(allocator, this);
        }
    }
};

struct SPathVertexPipeline : public SVertexPipelineImpl
{

    SPathVertexPipeline(IShaderProgramGenerator &inProgGenerator,
                        IMaterialShaderGenerator &inMaterialGenerator, NVAllocatorCallback &inAlloc,
                        IStringTable &inStringTable, bool inWireframe)
        : SVertexPipelineImpl(inAlloc, inMaterialGenerator, inProgGenerator, inStringTable,
                              inWireframe)
    {
    }

    // Trues true if the code was *not* set.
    bool SetCode(GenerationFlagValues::Enum inCode)
    {
        if (((QT3DSU32)m_GenerationFlags & inCode) != 0)
            return true;
        m_GenerationFlags |= inCode;
        return false;
    }

    void AssignTessEvalVarying(const char8_t *inVarName, const char8_t *inVarValueExpr)
    {
        const char8_t *ext = "";
        if (ProgramGenerator().GetEnabledStages() & ShaderGeneratorStages::Geometry)
            ext = "TE";
        TessEval() << "\t" << inVarName << ext << " = " << inVarValueExpr << ";" << Endl;
    }

    void AssignOutput(const char8_t *inVarName, const char8_t *inVarValueExpr) override
    {
        AssignTessEvalVarying(inVarName, inVarValueExpr);
    }

    void InitializeTessShaders()
    {
        IShaderStageGenerator &theTessControl(TessControl());
        IShaderStageGenerator &theTessEval(TessEval());

        // first setup tessellation control shader
        theTessControl.AddUniform("tessEdgeLevel", "float");
        theTessControl.AddUniform("tessInnerLevel", "float");

        theTessControl.AddInclude("tessellationPath.glsllib");

        theTessControl.Append("void main() {\n");
        theTessControl.Append(
            "\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
        theTessControl.Append("\ttessShader( tessEdgeLevel, tessInnerLevel );\n");

        bool hasGeometryShader =
            ProgramGenerator().GetStage(ShaderGeneratorStages::Geometry) != NULL;

        // second setup tessellation control shader
        eastl::string outExt("");
        if (hasGeometryShader)
            outExt = "TE";

        theTessEval.AddInclude("tessellationPath.glsllib");
        theTessEval.AddUniform("normal_matrix", "mat3");
        theTessEval.AddUniform("model_view_projection", "mat4");
        theTessEval.AddUniform("pathWidth", "float");
        theTessEval.AddUniform("material_diffuse", "vec4");
        AddInterpolationParameter("varTexCoord0", "vec2");
        AddInterpolationParameter("varTessOpacity", "float");

        theTessEval.Append("void main() {\n");
        theTessEval.Append("\tSTessShaderResult shaderResult = tessShader( pathWidth );\n");
        theTessEval.Append("\tvec3 pos = shaderResult.m_Position;\n");
        AssignTessEvalVarying("varTessOpacity", "shaderResult.m_Opacity");
        AssignTessEvalVarying("varTexCoord0", "shaderResult.m_TexCoord.xy");
        if (hasGeometryShader)
            theTessEval << "\tvec2 varTexCoord0 = shaderResult.m_TexCoord.xy;\n";

        theTessEval << "\tvec3 object_normal = vec3(0.0, 0.0, 1.0);\n";
        theTessEval << "\tvec3 world_normal = normal_matrix * object_normal;\n";
        theTessEval << "\tvec3 tangent = vec3( shaderResult.m_Tangent, 0.0 );\n";
        theTessEval << "\tvec3 binormal = vec3( shaderResult.m_Binormal, 0.0 );\n";

        // These are necessary for texture generation.
        theTessEval << "\tvec3 uTransform;" << Endl;
        theTessEval << "\tvec3 vTransform;" << Endl;

        if (m_DisplacementImage) {
            MaterialGenerator().GenerateImageUVCoordinates(*this, m_DisplacementIdx, 0,
                                                           *m_DisplacementImage);
            theTessEval.AddUniform("displaceAmount", "float");
            theTessEval.AddUniform("model_matrix", "mat4");
            theTessEval.AddInclude("defaultMaterialFileDisplacementTexture.glsllib");
            IDefaultMaterialShaderGenerator::SImageVariableNames theVarNames =
                MaterialGenerator().GetImageVariableNames(m_DisplacementIdx);

            theTessEval.AddUniform(theVarNames.m_ImageSampler, "sampler2D");
            IDefaultMaterialShaderGenerator::SImageVariableNames theNames =
                MaterialGenerator().GetImageVariableNames(m_DisplacementIdx);
            theTessEval << "\tpos = defaultMaterialFileDisplacementTexture( "
                        << theNames.m_ImageSampler << ", displaceAmount, "
                        << theNames.m_ImageFragCoords << outExt.c_str() << ", vec3( 0.0, 0.0, 1.0 )"
                        << ", pos.xyz );" << Endl;
        }
    }
    void FinalizeTessControlShader() {}

    void FinalizeTessEvaluationShader()
    {
        eastl::string outExt("");
        if (ProgramGenerator().GetEnabledStages() & ShaderGeneratorStages::Geometry)
            outExt = "TE";

        IShaderStageGenerator &tessEvalShader(
            *ProgramGenerator().GetStage(ShaderGeneratorStages::TessEval));
        tessEvalShader.Append("\tgl_Position = model_view_projection * vec4( pos, 1.0 );\n");
    }

    void BeginVertexGeneration(QT3DSU32 displacementImageIdx,
                                       SRenderableImage *displacementImage) override
    {
        SetupDisplacement(displacementImageIdx, displacementImage);

        TShaderGeneratorStageFlags theStages(IShaderProgramGenerator::DefaultFlags());
        theStages |= ShaderGeneratorStages::TessControl;
        theStages |= ShaderGeneratorStages::TessEval;
        if (m_Wireframe) {
            theStages |= ShaderGeneratorStages::Geometry;
        }
        ProgramGenerator().BeginProgram(theStages);
        InitializeTessShaders();
        if (m_Wireframe) {
            InitializeWireframeGeometryShader();
        }
        // Open up each stage.
        IShaderStageGenerator &vertexShader(Vertex());

        vertexShader.AddIncoming("attr_pos", "vec4");

        // useless vert shader because real work is done in TES.
        vertexShader << "void main()\n"
                        "{\n";
        vertexShader << "\tgl_Position = attr_pos;\n"; // if tessellation is enabled pass down
                                                       // object coordinates;
        vertexShader << "}\n";
    }

    void BeginFragmentGeneration() override
    {
        Fragment().AddUniform("material_diffuse", "vec4");
        Fragment() << "void main()" << Endl << "{" << Endl;
        // We do not pass object opacity through the pipeline.
        Fragment() << "\tfloat object_opacity = varTessOpacity * material_diffuse.a;" << Endl;
    }
    void DoGenerateUVCoords(QT3DSU32) override
    {
        // these are always generated regardless
    }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    void DoGenerateWorldNormal() override { AssignTessEvalVarying("varNormal", "world_normal"); }
    void DoGenerateObjectNormal() override
    {
        AssignTessEvalVarying("varObjectNormal", "object_normal");
    }
    void DoGenerateWorldPosition() override
    {
        TessEval().AddUniform("model_matrix", "mat4");
        TessEval()
            << "\tvec3 local_model_world_position = vec3((model_matrix * vec4(pos, 1.0)).xyz);\n";
    }
    void DoGenerateVarTangentAndBinormal() override
    {
        TessEval().AddUniform("normal_matrix", "mat3");
        AssignOutput("varTangent", "normal_matrix * tangent");
        AssignOutput("varBinormal", "normal_matrix * binormal");
    }

    void DoGenerateVertexColor() override
    {
        Vertex().AddIncoming("attr_color", "vec3");
        Vertex() << "\tvarColor = attr_color;" << Endl;
    }

    void EndVertexGeneration(bool) override
    {

        if (HasTessellation()) {
            // finalize tess control shader
            FinalizeTessControlShader();
            // finalize tess evaluation shader
            FinalizeTessEvaluationShader();

            TessControl().Append("}");
            TessEval().Append("}");
        }
        if (m_Wireframe) {
            // finalize geometry shader
            FinalizeWireframeGeometryShader();
            Geometry().Append("}");
        }
    }

    void EndFragmentGeneration(bool) override { Fragment().Append("}"); }

    void AddInterpolationParameter(const char8_t *inName, const char8_t *inType) override
    {
        m_InterpolationParameters.insert(eastl::make_pair(Str(inName), Str(inType)));
        Fragment().AddIncoming(inName, inType);
        if (HasTessellation()) {
            eastl::string nameBuilder;
            nameBuilder.assign(inName);
            if (ProgramGenerator().GetEnabledStages() & ShaderGeneratorStages::Geometry)
                nameBuilder.append("TE");

            TessEval().AddOutgoing(nameBuilder.c_str(), inType);
        }
    }

    IShaderStageGenerator &ActiveStage() override { return TessEval(); }
};

struct SPathXYGeneratedShader
{
    NVAllocatorCallback &m_Allocator;
    NVRenderShaderProgram &m_Shader;
    NVRenderCachedShaderProperty<QT3DSVec4> m_RectDimensions;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ModelMatrix;
    NVRenderCachedShaderProperty<QT3DSVec3> m_CameraPosition;
    NVRenderCachedShaderProperty<QT3DSVec2> m_CameraProperties;
    QT3DSI32 m_RefCount;

    SPathXYGeneratedShader(NVRenderShaderProgram &sh, NVAllocatorCallback &alloc)
        : m_Allocator(alloc)
        , m_Shader(sh)
        , m_RectDimensions("uni_rect_dimensions", sh)
        , m_ModelMatrix("model_matrix", sh)
        , m_CameraPosition("camera_position", sh)
        , m_CameraProperties("camera_properties", sh)
        , m_RefCount(0)
    {
        m_Shader.addRef();
    }
    virtual ~SPathXYGeneratedShader() { m_Shader.release(); }
    void addRef() { ++m_RefCount; }
    void release()
    {
        --m_RefCount;
        if (m_RefCount <= 0) {
            NVAllocatorCallback &allocator(m_Allocator);
            NVDelete(allocator, this);
        }
    }
};

// Helper implements the vertex pipeline for mesh subsets when bound to the default material.
// Should be completely possible to use for custom materials with a bit of refactoring.
struct SXYRectVertexPipeline : public SVertexPipelineImpl
{

    SXYRectVertexPipeline(IShaderProgramGenerator &inProgGenerator,
                          IMaterialShaderGenerator &inMaterialGenerator,
                          NVAllocatorCallback &inAlloc, IStringTable &inStringTable)
        : SVertexPipelineImpl(inAlloc, inMaterialGenerator, inProgGenerator, inStringTable, false)
    {
    }

    void BeginVertexGeneration(QT3DSU32 displacementImageIdx,
                                       SRenderableImage *displacementImage) override
    {
        m_DisplacementIdx = displacementImageIdx;
        m_DisplacementImage = displacementImage;

        TShaderGeneratorStageFlags theStages(IShaderProgramGenerator::DefaultFlags());
        ProgramGenerator().BeginProgram(theStages);
        // Open up each stage.
        IShaderStageGenerator &vertexShader(Vertex());
        vertexShader.AddIncoming("attr_pos", "vec2");
        vertexShader.AddUniform("uni_rect_dimensions", "vec4");

        vertexShader << "void main()" << Endl << "{" << Endl;
        vertexShader << "\tvec3 uTransform;" << Endl;
        vertexShader << "\tvec3 vTransform;" << Endl;

        vertexShader.AddUniform("model_view_projection", "mat4");
        vertexShader
            << "\tfloat posX = mix( uni_rect_dimensions.x, uni_rect_dimensions.z, attr_pos.x );"
            << Endl;
        vertexShader
            << "\tfloat posY = mix( uni_rect_dimensions.y, uni_rect_dimensions.w, attr_pos.y );"
            << Endl;
        vertexShader << "\tvec3  pos = vec3(posX, posY, 0.0 );" << Endl;
        vertexShader.Append("\tgl_Position = model_view_projection * vec4(pos, 1.0);");
    }

    void OutputParaboloidDepthShaders()
    {
        TShaderGeneratorStageFlags theStages(IShaderProgramGenerator::DefaultFlags());
        ProgramGenerator().BeginProgram(theStages);
        IShaderStageGenerator &vertexShader(Vertex());
        vertexShader.AddIncoming("attr_pos", "vec2");
        vertexShader.AddUniform("uni_rect_dimensions", "vec4");
        vertexShader.AddUniform("model_view_projection", "mat4");
        vertexShader << "void main()" << Endl << "{" << Endl;
        vertexShader
            << "\tfloat posX = mix( uni_rect_dimensions.x, uni_rect_dimensions.z, attr_pos.x );"
            << Endl;
        vertexShader
            << "\tfloat posY = mix( uni_rect_dimensions.y, uni_rect_dimensions.w, attr_pos.y );"
            << Endl;
        vertexShader << "\tvec3 pos = vec3(posX, posY, 0.0 );" << Endl;
        IShaderProgramGenerator::OutputParaboloidDepthTessEval(vertexShader);
        vertexShader << "}" << Endl;

        IShaderProgramGenerator::OutputParaboloidDepthFragment(Fragment());
    }

    void OutputCubeFaceDepthShaders()
    {
        TShaderGeneratorStageFlags theStages(IShaderProgramGenerator::DefaultFlags());
        ProgramGenerator().BeginProgram(theStages);
        IShaderStageGenerator &vertexShader(Vertex());
        IShaderStageGenerator &fragmentShader(Fragment());
        vertexShader.AddIncoming("attr_pos", "vec2");
        vertexShader.AddUniform("uni_rect_dimensions", "vec4");
        vertexShader.AddUniform("model_matrix", "mat4");
        vertexShader.AddUniform("model_view_projection", "mat4");

        vertexShader.AddOutgoing("world_pos", "vec4");
        vertexShader.Append("void main() {");
        vertexShader.Append(
            "   float posX = mix( uni_rect_dimensions.x, uni_rect_dimensions.z, attr_pos.x );");
        vertexShader.Append(
            "   float posY = mix( uni_rect_dimensions.y, uni_rect_dimensions.w, attr_pos.y );");
        vertexShader.Append("   world_pos = model_matrix * vec4( posX, posY, 0.0, 1.0 );");
        vertexShader.Append("   world_pos /= world_pos.w;");
        vertexShader.Append(
            "	gl_Position = model_view_projection * vec4( posX, posY, 0.0, 1.0 );");
        vertexShader.Append("}");

        fragmentShader.AddUniform("camera_position", "vec3");
        fragmentShader.AddUniform("camera_properties", "vec2");

        BeginFragmentGeneration();
        fragmentShader.Append(
            "\tfloat dist = 0.5 * length( world_pos.xyz - camera_position );"); // Why?
        fragmentShader.Append(
            "\tdist = (dist - camera_properties.x) / (camera_properties.y - camera_properties.x);");
        fragmentShader.Append("\tfragOutput = vec4(dist);");
        fragmentShader.Append("}");
    }

    void BeginFragmentGeneration() override
    {
        Fragment().AddUniform("material_diffuse", "vec4");
        Fragment() << "void main()" << Endl << "{" << Endl;
        // We do not pass object opacity through the pipeline.
        Fragment() << "\tfloat object_opacity = material_diffuse.a;" << Endl;
    }

    void AssignOutput(const char8_t *inVarName, const char8_t *inVarValue) override
    {
        Vertex() << "\t" << inVarName << " = " << inVarValue << ";\n";
    }
    void DoGenerateUVCoords(QT3DSU32) override { Vertex() << "\tvarTexCoord0 = attr_pos;" << Endl; }

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    void DoGenerateWorldNormal() override
    {
        IShaderStageGenerator &vertexGenerator(Vertex());
        vertexGenerator.AddUniform("normal_matrix", "mat3");
        vertexGenerator.Append(
            "\tvec3 world_normal = normalize(normal_matrix * vec3( 0.0, 0.0, 1.0) ).xyz;");
        vertexGenerator.Append("\tvarNormal = world_normal;");
    }

    void DoGenerateObjectNormal() override
    {
        AddInterpolationParameter("varObjectNormal", "vec3");
        Vertex().Append("\tvarObjectNormal = vec3(0.0, 0.0, 1.0 );");
    }

    void DoGenerateWorldPosition() override
    {
        Vertex().Append("\tvec3 local_model_world_position = (model_matrix * vec4(pos, 1.0)).xyz;");
        AssignOutput("varWorldPos", "local_model_world_position");
    }

    void DoGenerateVarTangentAndBinormal() override
    {
        Vertex().AddIncoming("attr_textan", "vec3");
        Vertex().AddIncoming("attr_binormal", "vec3");
        Vertex() << "\tvarTangent = normal_matrix * vec3(1.0, 0.0, 0.0);" << Endl
                 << "\tvarBinormal = normal_matrix * vec3(0.0, 1.0, 0.0);" << Endl;
    }

    void DoGenerateVertexColor() override
    {
        Vertex().AddIncoming("attr_color", "vec3");
        Vertex() << "\tvarColor = attr_color;" << Endl;
    }

    void EndVertexGeneration(bool) override { Vertex().Append("}"); }

    void EndFragmentGeneration(bool) override { Fragment().Append("}"); }

    void AddInterpolationParameter(const char8_t *inName, const char8_t *inType) override
    {
        m_InterpolationParameters.insert(eastl::make_pair(Str(inName), Str(inType)));
        Vertex().AddOutgoing(inName, inType);
        Fragment().AddIncoming(inName, inType);
    }

    IShaderStageGenerator &ActiveStage() override { return Vertex(); }
};

struct SPathManager : public IPathManager
{
    typedef nvhash_map<SPath *, NVScopedRefCounted<SPathBuffer>> TPathBufferHash;
    typedef nvhash_map<SPathSubPath *, NVScopedRefCounted<SPathSubPathBuffer>>
        TPathSubPathBufferHash;
    typedef nvhash_map<SPathShaderMapKey, NVScopedRefCounted<SPathGeneratedShader>> TShaderMap;
    typedef nvhash_map<SPathShaderMapKey, NVScopedRefCounted<SPathXYGeneratedShader>>
        TPaintedShaderMap;
    typedef nvhash_map<CRegisteredString, TPathBufferPtr> TStringPathBufferMap;

    IQt3DSRenderContextCore &m_CoreContext;
    IQt3DSRenderContext *m_RenderContext;
    eastl::string m_IdBuilder;
    TPathSubPathBufferHash m_SubPathBuffers;
    TPathBufferHash m_Buffers;
    nvvector<SResultCubic> m_SubdivResult;
    nvvector<QT3DSF32> m_KeyPointVec;
    nvvector<QT3DSVec4> m_PatchBuffer;
    TShaderMap m_PathGeometryShaders;
    TPaintedShaderMap m_PathPaintedShaders;
    TStringPathBufferMap m_SourcePathBufferMap;
    Mutex m_PathBufferMutex;

    NVScopedRefCounted<SPathGeneratedShader> m_DepthShader;
    NVScopedRefCounted<SPathGeneratedShader> m_DepthDisplacementShader;
    NVScopedRefCounted<SPathGeneratedShader> m_GeometryShadowShader;
    NVScopedRefCounted<SPathGeneratedShader> m_GeometryCubeShadowShader;
    NVScopedRefCounted<SPathGeneratedShader> m_GeometryDisplacementShadowShader;

    NVScopedRefCounted<SPathXYGeneratedShader> m_PaintedDepthShader;
    NVScopedRefCounted<SPathXYGeneratedShader> m_PaintedShadowShader;
    NVScopedRefCounted<SPathXYGeneratedShader> m_PaintedCubeShadowShader;
    NVScopedRefCounted<NVRenderInputAssembler> m_PaintedRectInputAssembler;
    NVScopedRefCounted<NVRenderVertexBuffer> m_PaintedRectVertexBuffer;
    NVScopedRefCounted<NVRenderIndexBuffer> m_PaintedRectIndexBuffer;

    nvvector<NVScopedRefCounted<NVRenderDepthStencilState>> m_DepthStencilStates;

    NVScopedRefCounted<NVRenderPathSpecification> m_PathSpecification;
    NVScopedRefCounted<qt3dsimp::IPathBufferBuilder> m_PathBuilder;

    QT3DSI32 m_RefCount;

    SPathManager(IQt3DSRenderContextCore &inRC)
        : m_CoreContext(inRC)
        , m_RenderContext(NULL)
        , m_SubPathBuffers(inRC.GetAllocator(), "m_SubPathBuffers")
        , m_Buffers(inRC.GetAllocator(), "m_Buffers")
        , m_SubdivResult(inRC.GetAllocator(), "m_SubdivResult")
        , m_KeyPointVec(inRC.GetAllocator(), "m_KeyPointVec")
        , m_PatchBuffer(inRC.GetAllocator(), "m_QuadStrip")
        , m_PathGeometryShaders(inRC.GetAllocator(), "m_PathGeometryShaders")
        , m_PathPaintedShaders(inRC.GetAllocator(), "m_PathPaintedShaders")
        , m_SourcePathBufferMap(inRC.GetAllocator(), "m_SourcePathBufferMap")
        , m_PathBufferMutex(inRC.GetAllocator())
        , m_DepthStencilStates(inRC.GetAllocator(), "m_DepthStencilStates")
        , m_RefCount(0)
    {
    }

    virtual ~SPathManager() { m_PaintedRectInputAssembler = NULL; }

    NVAllocatorCallback &GetAllocator() { return m_CoreContext.GetAllocator(); }
    IStringTable &GetStringTable() { return m_CoreContext.GetStringTable(); }
    NVFoundationBase &GetFoundation() { return m_CoreContext.GetFoundation(); }

    void addRef() override { atomicIncrement(&m_RefCount); }
    void release() override
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(GetAllocator());
            NVDelete(alloc, this);
        }
    }
    // Called during binary load which is heavily threaded.
    void SetPathSubPathData(const SPathSubPath &inPath,
                                    NVConstDataRef<SPathAnchorPoint> inPathCubicCurves) override
    {
        Mutex::ScopedLock __locker(m_PathBufferMutex);
        eastl::pair<TPathSubPathBufferHash::iterator, bool> inserter =
            m_SubPathBuffers.insert(eastl::make_pair((SPathSubPath *)&inPath,
                                                     NVScopedRefCounted<SPathSubPathBuffer>(NULL)));
        if (!inserter.first->second)
            inserter.first->second = QT3DS_NEW(GetAllocator(), SPathSubPathBuffer)(
                GetAllocator(), const_cast<SPathSubPath &>(inPath));
        SPathSubPathBuffer &theBuffer = *inserter.first->second.mPtr;
        theBuffer.m_SourceData.assign(inPathCubicCurves.begin(), inPathCubicCurves.end());
        theBuffer.m_Flags.clearOrSet(true, PathDirtyFlagValues::SourceData);
    }

    SPathBuffer *GetPathBufferObject(const SPath &inPath)
    {
        eastl::pair<TPathBufferHash::iterator, bool> inserter = m_Buffers.insert(
            eastl::make_pair((SPath *)&inPath, NVScopedRefCounted<SPathBuffer>(NULL)));
        if (inserter.second) {
            inserter.first->second = QT3DS_NEW(GetAllocator(), SPathBuffer)(GetAllocator());
        }
        return inserter.first->second.mPtr;
    }

    SPathSubPathBuffer *GetPathBufferObject(const SPathSubPath &inSubPath)
    {
        TPathSubPathBufferHash::iterator iter = m_SubPathBuffers.find((SPathSubPath *)&inSubPath);
        if (iter != m_SubPathBuffers.end())
            return iter->second.mPtr;
        return NULL;
    }

    NVDataRef<SPathAnchorPoint> GetPathSubPathBuffer(const SPathSubPath &inPath) override
    {
        SPathSubPathBuffer *theBuffer = GetPathBufferObject(inPath);
        if (theBuffer)
            return toDataRef(theBuffer->m_SourceData.data(), (QT3DSU32)theBuffer->m_SourceData.size());
        return NVDataRef<SPathAnchorPoint>();
    }

    NVDataRef<SPathAnchorPoint> ResizePathSubPathBuffer(const SPathSubPath &inPath,
                                                                QT3DSU32 inNumAnchors) override
    {
        SPathSubPathBuffer *theBuffer = GetPathBufferObject(inPath);
        if (theBuffer == NULL)
            SetPathSubPathData(inPath, NVConstDataRef<SPathAnchorPoint>());
        theBuffer = GetPathBufferObject(inPath);
        theBuffer->m_SourceData.resize(inNumAnchors);
        theBuffer->m_Flags.clearOrSet(true, PathDirtyFlagValues::SourceData);
        return toDataRef(theBuffer->m_SourceData.data(), (QT3DSU32)theBuffer->m_SourceData.size());
    }

    // This needs to be done using roots of the first derivative.
    NVBounds3 GetBounds(const SPath &inPath) override
    {
        NVBounds3 retval(NVBounds3::empty());

        SPathBuffer *thePathBuffer = GetPathBufferObject(inPath);
        if (thePathBuffer) {
            SPathDirtyFlags geomDirtyFlags(
                PathDirtyFlagValues::SourceData | PathDirtyFlagValues::BeginTaper
                | PathDirtyFlagValues::EndTaper | PathDirtyFlagValues::Width
                | PathDirtyFlagValues::CPUError);

            if ((((QT3DSU32)thePathBuffer->m_Flags) & (QT3DSU32)geomDirtyFlags) == 0) {
                return thePathBuffer->m_Bounds;
            }
        }

        for (SPathSubPath *theSubPath = inPath.m_FirstSubPath; theSubPath;
             theSubPath = theSubPath->m_NextSubPath) {
            SPathSubPathBuffer *theBuffer = GetPathBufferObject(*theSubPath);
            if (!theBuffer)
                continue;

            QT3DSU32 numAnchors = theBuffer->m_SourceData.size();
            for (QT3DSU32 idx = 0, end = numAnchors; idx < end; ++idx) {
                const SPathAnchorPoint &thePoint(theBuffer->m_SourceData[idx]);
                QT3DSVec2 position(thePoint.m_Position);
                retval.include(QT3DSVec3(position.x, position.y, 0.0f));
                if (idx) {
                    QT3DSVec2 incoming(IPathManagerCore::GetControlPointFromAngleDistance(
                        thePoint.m_Position, thePoint.m_IncomingAngle,
                        thePoint.m_IncomingDistance));
                    retval.include(QT3DSVec3(incoming.x, incoming.y, 0.0f));
                }

                if (idx < (numAnchors - 1)) {
                    QT3DSVec2 outgoing(IPathManagerCore::GetControlPointFromAngleDistance(
                        thePoint.m_Position, thePoint.m_OutgoingAngle,
                        thePoint.m_OutgoingDistance));
                    retval.include(QT3DSVec3(outgoing.x, outgoing.y, 0.0f));
                }
            }
        }

        return retval;
    }

    IPathManager &OnRenderSystemInitialize(IQt3DSRenderContext &context) override
    {
        m_RenderContext = &context;
        return *this;
    }

    // find a point that will join these two curves *if* they are not first derivative continuous
    static Option<QT3DSVec2> GetAdjoiningPoint(QT3DSVec2 prevC2, QT3DSVec2 point, QT3DSVec2 C1, QT3DSF32 pathWidth)
    {
        QT3DSVec2 incomingDxDy = (point - prevC2);
        QT3DSVec2 outgoingDxDy = (C1 - point);
        incomingDxDy.normalize();
        outgoingDxDy.normalize();
        float determinant = (incomingDxDy.x * outgoingDxDy.y) - (incomingDxDy.y * outgoingDxDy.x);
        if (fabs(determinant) > .001f) {
            float mult = determinant > 0.0f ? 1.0f : -1.0f;
            QT3DSVec2 incomingNormal(incomingDxDy.y, -incomingDxDy.x);
            QT3DSVec2 outgoingNormal(outgoingDxDy.y, -outgoingDxDy.x);

            QT3DSVec2 leftEdge = point + mult * incomingNormal * pathWidth;
            QT3DSVec2 rightEdge = point + mult * outgoingNormal * pathWidth;

            return (leftEdge + rightEdge) / 2.0f;
        }
        return Empty();
    }

    Option<eastl::pair<QT3DSU32, QT3DSF32>> FindBreakEquation(QT3DSF32 inTaperStart)
    {
        QT3DSF32 lengthTotal = 0;
        for (QT3DSU32 idx = 0, end = m_SubdivResult.size(); idx < end; ++idx) {
            if (lengthTotal + m_SubdivResult[idx].m_Length > inTaperStart) {
                QT3DSF32 breakTValue = (inTaperStart - lengthTotal) / m_SubdivResult[idx].m_Length;
                nvvector<SResultCubic>::iterator breakIter = m_SubdivResult.begin() + idx;
                SCubicBezierCurve theCurve(breakIter->m_P1, breakIter->m_C1, breakIter->m_C2,
                                           breakIter->m_P2);
                eastl::pair<SCubicBezierCurve, SCubicBezierCurve> subdivCurve =
                    theCurve.SplitCubicBezierCurve(breakTValue);
                QT3DSF32 originalBreakT =
                    breakIter->m_TStart + (breakIter->m_TStop - breakIter->m_TStart) * breakTValue;
                // Update the existing item to point to the second equation
                breakIter->m_P1 = subdivCurve.second.m_Points[0];
                breakIter->m_C1 = subdivCurve.second.m_Points[1];
                breakIter->m_C2 = subdivCurve.second.m_Points[2];
                breakIter->m_P2 = subdivCurve.second.m_Points[3];
                QT3DSF32 originalLength = breakIter->m_Length;
                QT3DSF32 originalStart = breakIter->m_TStart;
                breakIter->m_Length *= (1.0f - breakTValue);
                breakIter->m_TStart = originalBreakT;
                SResultCubic newCubic(subdivCurve.first.m_Points[0], subdivCurve.first.m_Points[1],
                                      subdivCurve.first.m_Points[2], subdivCurve.first.m_Points[3],
                                      breakIter->m_EquationIndex, originalStart, originalBreakT,
                                      originalLength * breakTValue);

                m_SubdivResult.insert(breakIter, newCubic);
                return eastl::make_pair(idx, breakTValue);
            }
            lengthTotal += m_SubdivResult[idx].m_Length;
        }
        return Empty();
    }

    bool PrepareGeometryPathForRender(const SPath &inPath, SPathBuffer &inPathBuffer)
    {

        m_SubdivResult.clear();
        m_KeyPointVec.clear();
        const SPath &thePath(inPath);

        inPathBuffer.SetBeginTaperInfo(thePath.m_BeginCapping, thePath.m_BeginCapOffset,
                                       thePath.m_BeginCapOpacity, thePath.m_BeginCapWidth);
        inPathBuffer.SetEndTaperInfo(thePath.m_EndCapping, thePath.m_EndCapOffset,
                                     thePath.m_EndCapOpacity, thePath.m_EndCapWidth);
        inPathBuffer.SetWidth(inPath.m_Width);
        inPathBuffer.SetCPUError(inPath.m_LinearError);

        SPathDirtyFlags geomDirtyFlags(PathDirtyFlagValues::SourceData
                                       | PathDirtyFlagValues::BeginTaper
                                       | PathDirtyFlagValues::EndTaper | PathDirtyFlagValues::Width
                                       | PathDirtyFlagValues::CPUError);

        bool retval = false;
        if (!inPathBuffer.m_PatchData
            || (((QT3DSU32)inPathBuffer.m_Flags) & (QT3DSU32)geomDirtyFlags) != 0) {
            qt3dsimp::SPathBuffer thePathData = inPathBuffer.GetPathData(*m_PathBuilder);

            QT3DSU32 dataIdx = 0;
            QT3DSVec2 prevPoint(0, 0);
            QT3DSU32 equationIdx = 0;
            for (QT3DSU32 commandIdx = 0, commandEnd = thePathData.m_Commands.size();
                 commandIdx < commandEnd; ++commandIdx) {
                switch (thePathData.m_Commands[commandIdx]) {
                case qt3dsimp::PathCommand::MoveTo:
                    prevPoint =
                        QT3DSVec2(thePathData.m_Data[dataIdx], thePathData.m_Data[dataIdx + 1]);
                    dataIdx += 2;
                    break;
                case qt3dsimp::PathCommand::CubicCurveTo: {
                    QT3DSVec2 c1(thePathData.m_Data[dataIdx], thePathData.m_Data[dataIdx + 1]);
                    dataIdx += 2;
                    QT3DSVec2 c2(thePathData.m_Data[dataIdx], thePathData.m_Data[dataIdx + 1]);
                    dataIdx += 2;
                    QT3DSVec2 p2(thePathData.m_Data[dataIdx], thePathData.m_Data[dataIdx + 1]);
                    dataIdx += 2;
                    OuterAdaptiveSubdivideBezierCurve(
                        m_SubdivResult, m_KeyPointVec, SCubicBezierCurve(prevPoint, c1, c2, p2),
                        NVMax(inPath.m_LinearError, 1.0f), equationIdx);
                    ++equationIdx;
                    prevPoint = p2;
                } break;
                case qt3dsimp::PathCommand::Close:
                    break;

                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }

            QT3DSF32 theLocalWidth = inPath.m_Width / 2.0f;

            QT3DSVec2 theBeginTaperData(theLocalWidth, thePath.m_GlobalOpacity);
            QT3DSVec2 theEndTaperData(theLocalWidth, thePath.m_GlobalOpacity);

            QT3DSF32 pathLength = 0.0f;
            for (QT3DSU32 idx = 0, end = m_SubdivResult.size(); idx < end; ++idx)
                pathLength += m_SubdivResult[idx].m_Length;

            if (thePath.m_BeginCapping == PathCapping::Taper
                || thePath.m_EndCapping == PathCapping::Taper) {
                QT3DSF32 maxTaperStart = pathLength / 2.0f;
                if (thePath.m_BeginCapping == PathCapping::Taper) {
                    // Can't start more than halfway across the path.
                    QT3DSF32 taperStart = NVMin(thePath.m_BeginCapOffset, maxTaperStart);
                    QT3DSF32 endTaperWidth = thePath.m_BeginCapWidth;
                    QT3DSF32 endTaperOpacity = thePath.m_GlobalOpacity * thePath.m_BeginCapOpacity;
                    theBeginTaperData = QT3DSVec2(endTaperWidth, endTaperOpacity);
                    // Find where we need to break the current equations.
                    Option<eastl::pair<QT3DSU32, QT3DSF32>> breakEquationAndT(
                        FindBreakEquation(taperStart));
                    if (breakEquationAndT.hasValue()) {
                        QT3DSU32 breakEquation = breakEquationAndT->first;

                        QT3DSF32 lengthTotal = 0;
                        for (QT3DSU32 idx = 0, end = breakEquation; idx <= end; ++idx) {
                            SResultCubic &theCubic = m_SubdivResult[idx];
                            theCubic.m_Mode = SResultCubic::BeginTaper;

                            theCubic.m_TaperMultiplier[0] = lengthTotal / taperStart;
                            lengthTotal += theCubic.m_Length;
                            theCubic.m_TaperMultiplier[1] = lengthTotal / taperStart;
                        }
                    }
                }
                if (thePath.m_EndCapping == PathCapping::Taper) {
                    QT3DSF32 taperStart = NVMin(thePath.m_EndCapOffset, maxTaperStart);
                    QT3DSF32 endTaperWidth = thePath.m_EndCapWidth;
                    QT3DSF32 endTaperOpacity = thePath.m_GlobalOpacity * thePath.m_EndCapOpacity;
                    theEndTaperData = QT3DSVec2(endTaperWidth, endTaperOpacity);
                    // Invert taper start so that the forward search works.
                    Option<eastl::pair<QT3DSU32, QT3DSF32>> breakEquationAndT(
                        FindBreakEquation(pathLength - taperStart));

                    if (breakEquationAndT.hasValue()) {
                        QT3DSU32 breakEquation = breakEquationAndT->first;
                        ++breakEquation;

                        QT3DSF32 lengthTotal = 0;
                        for (QT3DSU32 idx = breakEquation, end = m_SubdivResult.size(); idx < end;
                             ++idx) {
                            SResultCubic &theCubic = m_SubdivResult[idx];
                            theCubic.m_Mode = SResultCubic::EndTaper;

                            theCubic.m_TaperMultiplier[0] = 1.0f - (lengthTotal / taperStart);
                            lengthTotal += theCubic.m_Length;
                            theCubic.m_TaperMultiplier[1] = 1.0f - (lengthTotal / taperStart);
                        }
                    }
                }
            }

            NVRenderContext &theRenderContext(m_RenderContext->GetRenderContext());
            // Create quads out of each point.
            if (m_SubdivResult.empty())
                return false;

            // Generate patches.
            m_PatchBuffer.clear();
            QT3DSF32 pathWidth = thePath.m_Width / 2.0f;
            // texture coords
            float texCoordU = 0.0;

            for (QT3DSU32 idx = 0, end = m_SubdivResult.size(); idx < end; ++idx) {
                // create patches
                SResultCubic thePoint(m_SubdivResult[idx]);

                m_PatchBuffer.push_back(CreateVec4(thePoint.m_P1, thePoint.m_C1));
                m_PatchBuffer.push_back(CreateVec4(thePoint.m_C2, thePoint.m_P2));

                // Now we need to take care of cases where the control points of the adjoining
                // SubPaths
                // do not line up; i.e. there is a discontinuity of the 1st derivative
                // The simplest way to do this is to move the edge vertex to a halfway point
                // between a line bisecting the two control lines
                QT3DSVec2 incomingAdjoining(thePoint.m_P1);
                QT3DSVec2 outgoingAdjoining(thePoint.m_P2);
                if (idx) {
                    SResultCubic previousCurve = m_SubdivResult[idx - 1];
                    if (previousCurve.m_EquationIndex != thePoint.m_EquationIndex) {
                        QT3DSF32 anchorWidth =
                            thePoint.GetP1Width(pathWidth, theBeginTaperData.x, theEndTaperData.x);
                        Option<QT3DSVec2> adjoining = GetAdjoiningPoint(
                            previousCurve.m_C2, thePoint.m_P1, thePoint.m_C1, anchorWidth);
                        if (adjoining.hasValue())
                            incomingAdjoining = *adjoining;
                    }
                }
                if (idx < (end - 1)) {
                    SResultCubic nextCurve = m_SubdivResult[idx + 1];
                    if (nextCurve.m_EquationIndex != thePoint.m_EquationIndex) {
                        QT3DSF32 anchorWidth =
                            thePoint.GetP2Width(pathWidth, theBeginTaperData.x, theEndTaperData.x);
                        Option<QT3DSVec2> adjoining = GetAdjoiningPoint(thePoint.m_C2, thePoint.m_P2,
                                                                     nextCurve.m_C1, anchorWidth);
                        if (adjoining.hasValue())
                            outgoingAdjoining = *adjoining;
                    }
                }
                m_PatchBuffer.push_back(CreateVec4(incomingAdjoining, outgoingAdjoining));

                QT3DSVec4 taperData(0.0f);
                taperData.x = thePoint.m_TaperMultiplier.x;
                taperData.y = thePoint.m_TaperMultiplier.y;
                // Note we could put a *lot* more data into this thing.
                taperData.z = (QT3DSF32)thePoint.m_Mode;
                m_PatchBuffer.push_back(taperData);

                // texture coord generation
                // note we only generate u here. v is generated in the tess shader
                // u coord for P1 and C1
                QT3DSVec2 udata(texCoordU, texCoordU + (thePoint.m_Length / pathLength));
                texCoordU = udata.y;
                m_PatchBuffer.push_back(QT3DSVec4(udata.x, udata.y, 0.0, 0.0));
            }

            // buffer size is 3.0*4.0*bufSize
            QT3DSU32 bufSize = (QT3DSU32)m_PatchBuffer.size() * sizeof(QT3DSVec4);
            QT3DSU32 stride = sizeof(QT3DSVec4);

            if ((!inPathBuffer.m_PatchData) || inPathBuffer.m_PatchData->Size() < bufSize) {
                inPathBuffer.m_PatchData = theRenderContext.CreateVertexBuffer(
                    qt3ds::render::NVRenderBufferUsageType::Dynamic, bufSize, stride,
                    toU8DataRef(m_PatchBuffer.data(), (QT3DSU32)m_PatchBuffer.size()));
                inPathBuffer.m_NumVertexes = (QT3DSU32)m_PatchBuffer.size();
                inPathBuffer.m_InputAssembler = NULL;
            } else {
                QT3DS_ASSERT(inPathBuffer.m_PatchData->Size() >= bufSize);
                inPathBuffer.m_PatchData->UpdateBuffer(
                    toU8DataRef(m_PatchBuffer.data(), (QT3DSU32)m_PatchBuffer.size()));
            }

            if (!inPathBuffer.m_InputAssembler) {
                qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
                    qt3ds::render::NVRenderVertexBufferEntry(
                        "attr_pos", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 4),
                };

                NVRenderDrawMode::Enum primType = NVRenderDrawMode::Patches;

                NVRenderAttribLayout *theLayout =
                    theRenderContext.CreateAttributeLayout(toConstDataRef(theEntries, 1));
                // How many vertices the TCS shader has access to in order to produce its output
                // array of vertices.
                const QT3DSU32 inputPatchVertexCount = 5;
                inPathBuffer.m_InputAssembler = theRenderContext.CreateInputAssembler(
                    theLayout, toConstDataRef(inPathBuffer.m_PatchData.mPtr), NULL,
                    toConstDataRef(stride), toConstDataRef((QT3DSU32)0), primType,
                    inputPatchVertexCount);
            }
            inPathBuffer.m_BeginTaperData = theBeginTaperData;
            inPathBuffer.m_EndTaperData = theEndTaperData;

            // cache bounds
            NVBounds3 bounds = GetBounds(inPath);
            inPathBuffer.m_Bounds.minimum = bounds.minimum;
            inPathBuffer.m_Bounds.maximum = bounds.maximum;
        }

        return retval;
    }

    IMaterialShaderGenerator *GetMaterialShaderGenertator(SPathRenderContext &inRenderContext)
    {
        bool isDefaultMaterial =
            (inRenderContext.m_Material.m_Type == GraphObjectTypes::DefaultMaterial);

        IMaterialShaderGenerator *theMaterialGenerator = NULL;
        if (isDefaultMaterial)
            theMaterialGenerator = &m_RenderContext->GetDefaultMaterialShaderGenerator();
        else
            theMaterialGenerator = &m_RenderContext->GetCustomMaterialShaderGenerator();

        return theMaterialGenerator;
    }

    CRegisteredString GetMaterialNameForKey(SPathRenderContext &inRenderContext)
    {
        bool isDefaultMaterial =
            (inRenderContext.m_Material.m_Type == GraphObjectTypes::DefaultMaterial);

        if (!isDefaultMaterial) {
            ICustomMaterialSystem &theMaterialSystem(m_RenderContext->GetCustomMaterialSystem());
            const SCustomMaterial &theCustomMaterial(
                reinterpret_cast<const SCustomMaterial &>(inRenderContext.m_Material));

            return m_RenderContext->GetStringTable().RegisterStr(
                theMaterialSystem.GetShaderName(theCustomMaterial));
        }

        return m_RenderContext->GetStringTable().RegisterStr("");
    }

    bool PreparePaintedPathForRender(const SPath &inPath, SPathBuffer &inPathBuffer)
    {
        NVRenderContext &theContext(this->m_RenderContext->GetRenderContext());
        if (!inPathBuffer.m_PathRender
            || (((QT3DSU32)inPathBuffer.m_Flags) & PathDirtyFlagValues::SourceData)) {
            if (!inPathBuffer.m_PathRender) {
                inPathBuffer.m_PathRender = theContext.CreatePathRender();
            }

            if (inPathBuffer.m_PathRender == NULL || m_PathSpecification == NULL) {
                //	QT3DS_ASSERT( false );
                return false;
            }

            m_PathSpecification->Reset();
            qt3dsimp::SPathBuffer thePathData = inPathBuffer.GetPathData(*m_PathBuilder);

            QT3DSU32 dataIdx = 0;
            for (QT3DSU32 commandIdx = 0, commandEnd = thePathData.m_Commands.size();
                 commandIdx < commandEnd; ++commandIdx) {

                switch (thePathData.m_Commands[commandIdx]) {
                case qt3dsimp::PathCommand::MoveTo:
                    m_PathSpecification->MoveTo(
                        QT3DSVec2(thePathData.m_Data[dataIdx], thePathData.m_Data[dataIdx + 1]));
                    dataIdx += 2;
                    break;
                case qt3dsimp::PathCommand::CubicCurveTo: {
                    QT3DSVec2 c1(thePathData.m_Data[dataIdx], thePathData.m_Data[dataIdx + 1]);
                    dataIdx += 2;
                    QT3DSVec2 c2(thePathData.m_Data[dataIdx], thePathData.m_Data[dataIdx + 1]);
                    dataIdx += 2;
                    QT3DSVec2 p2(thePathData.m_Data[dataIdx], thePathData.m_Data[dataIdx + 1]);
                    dataIdx += 2;
                    m_PathSpecification->CubicCurveTo(c1, c2, p2);
                } break;
                case qt3dsimp::PathCommand::Close:
                    m_PathSpecification->ClosePath();
                    break;
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }

            inPathBuffer.m_PathRender->SetPathSpecification(*m_PathSpecification);

            // cache bounds
            NVBounds3 bounds = GetBounds(inPath);
            inPathBuffer.m_Bounds.minimum = bounds.minimum;
            inPathBuffer.m_Bounds.maximum = bounds.maximum;

            return true;
        }

        return false;
    }

    bool PrepareForRender(const SPath &inPath) override
    {
        SPathBuffer *thePathBuffer = GetPathBufferObject(inPath);
        if (!thePathBuffer) {
            return false;
        }
        NVRenderContext &theContext(this->m_RenderContext->GetRenderContext());
        if (!m_PathSpecification)
            m_PathSpecification = theContext.CreatePathSpecification();
        if (!m_PathSpecification)
            return false;
        if (!m_PathBuilder)
            m_PathBuilder = qt3dsimp::IPathBufferBuilder::CreateBuilder(GetFoundation());

        thePathBuffer->SetPathType(inPath.m_PathType);
        bool retval = false;
        if (inPath.m_PathBuffer.IsValid() == false) {
            thePathBuffer->m_PathBuffer = NULL;
            // Ensure the SubPath list is identical and clear, percolating any dirty flags up to the
            // path buffer.
            QT3DSU32 SubPathIdx = 0;
            for (const SPathSubPath *theSubPath = inPath.m_FirstSubPath; theSubPath;
                 theSubPath = theSubPath->m_NextSubPath, ++SubPathIdx) {
                SPathSubPathBuffer *theSubPathBuffer = GetPathBufferObject(*theSubPath);
                if (theSubPathBuffer == NULL)
                    continue;
                thePathBuffer->m_Flags =
                    (QT3DSU32)(thePathBuffer->m_Flags | theSubPathBuffer->m_Flags);

                if (theSubPathBuffer->m_Closed != theSubPath->m_Closed) {
                    thePathBuffer->m_Flags.clearOrSet(true, PathDirtyFlagValues::SourceData);
                    theSubPathBuffer->m_Closed = theSubPath->m_Closed;
                }

                if (thePathBuffer->m_SubPaths.size() <= SubPathIdx
                    || thePathBuffer->m_SubPaths[SubPathIdx] != theSubPathBuffer) {
                    thePathBuffer->m_Flags.clearOrSet(true, PathDirtyFlagValues::SourceData);
                    if (thePathBuffer->m_SubPaths.size() <= SubPathIdx)
                        thePathBuffer->m_SubPaths.push_back(theSubPathBuffer);
                    else
                        thePathBuffer->m_SubPaths[SubPathIdx] = theSubPathBuffer;
                }

                theSubPathBuffer->m_Flags.Clear();
            }

            if (SubPathIdx != thePathBuffer->m_SubPaths.size()) {
                thePathBuffer->m_SubPaths.resize(SubPathIdx);
                thePathBuffer->m_Flags.clearOrSet(true, PathDirtyFlagValues::SourceData);
            }
        } else {
            thePathBuffer->m_SubPaths.clear();
            eastl::pair<TStringPathBufferMap::iterator, bool> inserter =
                m_SourcePathBufferMap.insert(
                    eastl::make_pair(inPath.m_PathBuffer, TPathBufferPtr()));
            if (inserter.second) {
                NVScopedRefCounted<IRefCountedInputStream> theStream =
                    m_CoreContext.GetInputStreamFactory().GetStreamForFile(
                        inPath.m_PathBuffer.c_str());
                if (theStream) {
                    qt3dsimp::SPathBuffer *theNewBuffer =
                        qt3dsimp::SPathBuffer::Load(*theStream, GetFoundation());
                    if (theNewBuffer)
                        inserter.first->second = QT3DS_NEW(GetAllocator(), SImportPathWrapper)(
                            GetAllocator(), *theNewBuffer);
                }
            }
            if (thePathBuffer->m_PathBuffer != inserter.first->second) {
                thePathBuffer->m_PathBuffer = inserter.first->second;
                thePathBuffer->m_Flags.clearOrSet(true, PathDirtyFlagValues::SourceData);
            }
        }

        if (inPath.m_PathType == PathTypes::Geometry)
            retval = PrepareGeometryPathForRender(inPath, *thePathBuffer);
        else
            retval = PreparePaintedPathForRender(inPath, *thePathBuffer);
        thePathBuffer->m_Flags.Clear();
        return retval;
    }

    void SetMaterialProperties(NVRenderShaderProgram &inShader, SPathRenderContext &inRenderContext,
                               SLayerGlobalRenderProperties &inRenderProperties)
    {
        IMaterialShaderGenerator *theMaterialGenerator =
            GetMaterialShaderGenertator(inRenderContext);
        NVRenderContext &theRenderContext(m_RenderContext->GetRenderContext());
        theRenderContext.SetActiveShader(&inShader);

        theMaterialGenerator->SetMaterialProperties(
            inShader, inRenderContext.m_Material, inRenderContext.m_CameraVec,
            inRenderContext.m_ModelViewProjection, inRenderContext.m_NormalMatrix,
            inRenderContext.m_Path.m_GlobalTransform, inRenderContext.m_FirstImage,
            inRenderContext.m_Opacity, inRenderProperties);
    }

    void DoRenderGeometryPath(SPathGeneratedShader &inShader, SPathRenderContext &inRenderContext,
                              SLayerGlobalRenderProperties &inRenderProperties,
                              SPathBuffer &inPathBuffer)
    {
        if (inPathBuffer.m_InputAssembler == NULL)
            return;

        SetMaterialProperties(inShader.m_Shader, inRenderContext, inRenderProperties);
        NVRenderContext &theRenderContext(m_RenderContext->GetRenderContext());

        inShader.m_BeginTaperData.Set(inPathBuffer.m_BeginTaperData);
        inShader.m_EndTaperData.Set(inPathBuffer.m_EndTaperData);
        if (inRenderContext.m_EnableWireframe) {
            // we need the viewport matrix
            NVRenderRect theViewport(theRenderContext.GetViewport());
            QT3DSMat44 vpMatrix;
            vpMatrix.column0 = QT3DSVec4((float)theViewport.m_Width / 2.0f, 0.0, 0.0, 0.0);
            vpMatrix.column1 = QT3DSVec4(0.0, (float)theViewport.m_Height / 2.0f, 0.0, 0.0);
            vpMatrix.column2 = QT3DSVec4(0.0, 0.0, 1.0, 0.0);
            vpMatrix.column3 =
                QT3DSVec4((float)theViewport.m_Width / 2.0f + (float)theViewport.m_X,
                       (float)theViewport.m_Height / 2.0f + (float)theViewport.m_Y, 0.0, 1.0);

            inShader.m_WireframeViewMatrix.Set(vpMatrix);
        }

        QT3DSF32 tessEdgeValue = NVMin(64.0f, NVMax(1.0f, inRenderContext.m_Path.m_EdgeTessAmount));
        QT3DSF32 tessInnerValue = NVMin(64.0f, NVMax(1.0f, inRenderContext.m_Path.m_InnerTessAmount));
        inShader.m_EdgeTessAmount.Set(tessEdgeValue);
        inShader.m_InnerTessAmount.Set(tessInnerValue);
        inShader.m_Width.Set(inRenderContext.m_Path.m_Width / 2.0f);
        theRenderContext.SetInputAssembler(inPathBuffer.m_InputAssembler);
        theRenderContext.SetCullingEnabled(false);
        NVRenderDrawMode::Enum primType = NVRenderDrawMode::Patches;
        theRenderContext.Draw(primType, (QT3DSU32)inPathBuffer.m_NumVertexes, 0);
    }

    NVRenderDepthStencilState *GetDepthStencilState()
    {
        NVRenderContext &theRenderContext(m_RenderContext->GetRenderContext());
        NVRenderBoolOp::Enum theDepthFunction = theRenderContext.GetDepthFunction();
        bool isDepthEnabled = theRenderContext.IsDepthTestEnabled();
        bool isStencilEnabled = theRenderContext.IsStencilTestEnabled();
        bool isDepthWriteEnabled = theRenderContext.IsDepthWriteEnabled();
        for (QT3DSU32 idx = 0, end = m_DepthStencilStates.size(); idx < end; ++idx) {
            NVRenderDepthStencilState &theState = *m_DepthStencilStates[idx];
            if (theState.GetDepthFunc() == theDepthFunction
                && theState.GetDepthEnabled() == isDepthEnabled
                && theState.GetDepthMask() == isDepthWriteEnabled)
                return &theState;
        }
        NVRenderStencilFunctionArgument theArg(NVRenderBoolOp::NotEqual, 0, 0xFF);
        NVRenderStencilOperationArgument theOpArg(NVRenderStencilOp::Keep, NVRenderStencilOp::Keep,
                                                  NVRenderStencilOp::Zero);
        m_DepthStencilStates.push_back(theRenderContext.CreateDepthStencilState(
            isDepthEnabled, isDepthWriteEnabled, theDepthFunction, isStencilEnabled, theArg, theArg,
            theOpArg, theOpArg));
        return m_DepthStencilStates.back();
    }

    static void DoSetCorrectiveScale(const QT3DSMat44 &mvp, QT3DSMat44 &outScale, NVBounds3 pathBounds)
    {
        // Compute the projected locations for the paraboloid and regular projection
        // and thereby set the appropriate scaling factor.
        QT3DSVec3 points[4];
        QT3DSVec3 projReg[4], projParab[4];
        points[0] = pathBounds.minimum;
        points[1] = QT3DSVec3(pathBounds.maximum.x, pathBounds.minimum.y, pathBounds.minimum.z);
        points[2] = pathBounds.maximum;
        points[3] = QT3DSVec3(pathBounds.minimum.x, pathBounds.maximum.y, pathBounds.maximum.z);

        // Do the two different projections.
        for (int i = 0; i < 4; ++i) {
            QT3DSVec4 tmp;
            tmp = mvp.transform(QT3DSVec4(points[i], 1.0f));
            tmp /= tmp.w;
            projReg[i] = tmp.getXYZ();
            projParab[i] = tmp.getXYZ().getNormalized();
            projParab[i] /= projParab[i].z + 1.0f;
        }

        NVBounds3 boundsA, boundsB;
        for (int i = 0; i < 4; ++i) {
            boundsA.include(projReg[i]);
            boundsB.include(projParab[i]);
        }
        QT3DSF32 xscale =
            (boundsB.maximum.x - boundsB.minimum.x) / (boundsA.maximum.x - boundsA.minimum.x);
        QT3DSF32 yscale =
            (boundsB.maximum.y - boundsB.minimum.y) / (boundsA.maximum.y - boundsA.minimum.y);
        QT3DSF32 zscale = (boundsB.maximum - boundsB.minimum).magnitudeSquared()
            / (boundsA.maximum - boundsA.minimum).magnitudeSquared();
        // The default minimum here is just a stupid figure that looks good on our content because
        // we'd
        // been using it for a little while before.  Just for demo.
        xscale = NVMin<QT3DSF32>(0.5333333f, NVMin<QT3DSF32>(xscale, yscale));
        yscale = NVMin<QT3DSF32>(0.5333333f, NVMin<QT3DSF32>(xscale, yscale));
        outScale.scale(QT3DSVec4(xscale, yscale, zscale, 1.0f));
    }

    void DoRenderPaintedPath(SPathXYGeneratedShader &inShader, SPathRenderContext &inRenderContext,
                             SLayerGlobalRenderProperties &inRenderProperties,
                             SPathBuffer &inPathBuffer, bool isParaboloidPass = false)
    {
        if (!inPathBuffer.m_PathRender)
            return;
        NVRenderContext &theRenderContext(m_RenderContext->GetRenderContext());
        if (!m_PaintedRectInputAssembler) {
            QT3DSVec2 vertexes[] = {
                QT3DSVec2(0.0, 0.0), QT3DSVec2(1.0, 0.0), QT3DSVec2(1.0, 1.0), QT3DSVec2(0.0, 1.0),
            };

            QT3DSU8 indexes[] = {
                0, 1, 2, 2, 3, 0,
            };

            QT3DSU32 stride = sizeof(QT3DSVec2);

            NVRenderVertexBufferEntry theBufferEntries[] = { NVRenderVertexBufferEntry(
                "attr_pos", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2, 0) };

            m_PaintedRectVertexBuffer = theRenderContext.CreateVertexBuffer(
                qt3ds::render::NVRenderBufferUsageType::Static, 4 * sizeof(QT3DSVec2), sizeof(QT3DSVec2),
                toU8DataRef(vertexes, 4));
            m_PaintedRectIndexBuffer = theRenderContext.CreateIndexBuffer(
                qt3ds::render::NVRenderBufferUsageType::Static,
                qt3ds::render::NVRenderComponentTypes::QT3DSU8, 6, toU8DataRef(indexes, 6));
            NVRenderAttribLayout *theAttribLayout =
                theRenderContext.CreateAttributeLayout(toConstDataRef(theBufferEntries, 1));
            m_PaintedRectInputAssembler = theRenderContext.CreateInputAssembler(
                theAttribLayout, toConstDataRef(m_PaintedRectVertexBuffer.mPtr),
                m_PaintedRectIndexBuffer.mPtr, toConstDataRef(stride), toConstDataRef((QT3DSU32)0),
                qt3ds::render::NVRenderDrawMode::Triangles);
        }

        // our current render target needs stencil
        QT3DS_ASSERT(theRenderContext.GetStencilBits() > 0);

        theRenderContext.SetDepthStencilState(GetDepthStencilState());

        // http://developer.download.nvidia.com/assets/gamedev/files/Mixing_Path_Rendering_and_3D.pdf
        theRenderContext.SetPathStencilDepthOffset(-.05f, -1.0f);

        // Stencil out the geometry.
        QT3DSMat44 pathMdlView = QT3DSMat44::createIdentity();
        // Why is this happening?  Well, it's because the painted-on path rendering is always
        // a flat splatted 2D object.  This is bad because a paraboloid projection demands a very
        // different
        // non-linear space into which we must draw.  Path Rendering does not allow this sort of
        // spatial
        // warping internally, and all we end up passing in as a simple perspective projection.
        // So for the fix, I'm scaling the actual "object" size so that it fits into the correctly
        // projected
        // polygon inside the paraboloid depth pass.  Obviously, this scaling factor is wrong, and
        // not generic
        // enough to cover cases like polygons covering a large spread of the FOV and so on.  It's
        // really
        // just a filthy awful, morally deplorable HACK.  But it's basically the quickest fix at
        // hand.
        // This is also about the only possible approach that *could* work short of rendering the
        // paths in
        // a render-to-texture pass and splatting that texture on a sufficiently tessellated quad.
        // Unless
        // there's a way to program NVPR's internal projection scheme, that is.
        // Geometry-based paths will work out better, I think, because they're actually creating
        // geometry.
        // This is essentially a 2D painting process inside a quad where the actual rendered region
        // isn't
        // exactly where NVPR thinks it should be because they're not projecting points the same
        // way.
        if (isParaboloidPass) {
            DoSetCorrectiveScale(inRenderContext.m_ModelViewProjection, pathMdlView,
                                 inPathBuffer.m_PathRender->GetPathObjectStrokeBox());
        }

        bool isStencilEnabled = theRenderContext.IsStencilTestEnabled();
        theRenderContext.SetStencilTestEnabled(true);
        theRenderContext.SetPathProjectionMatrix(inRenderContext.m_ModelViewProjection);
        theRenderContext.SetPathModelViewMatrix(pathMdlView);

        if (inRenderContext.m_IsStroke) {
            inPathBuffer.m_PathRender->SetStrokeWidth(inRenderContext.m_Path.m_Width);
            inPathBuffer.m_PathRender->StencilStroke();
        } else
            inPathBuffer.m_PathRender->StencilFill();

        // The stencil buffer will dictate whether this object renders or not.  So we need to ignore
        // the depth test result.
        NVRenderBoolOp::Enum theDepthFunc = theRenderContext.GetDepthFunction();
        theRenderContext.SetDepthFunction(NVRenderBoolOp::AlwaysTrue);
        // Now render the path; this resets the stencil buffer.
        SetMaterialProperties(inShader.m_Shader, inRenderContext, inRenderProperties);
        NVBounds3 rectBounds = inPathBuffer.m_PathRender->GetPathObjectStrokeBox();
        if (isParaboloidPass) {
            rectBounds.scale(1.570796326795f);
        } // PKC : More of the same ugly hack.
        inShader.m_RectDimensions.Set(QT3DSVec4(rectBounds.minimum.x, rectBounds.minimum.y,
                                             rectBounds.maximum.x, rectBounds.maximum.y));
        theRenderContext.SetInputAssembler(m_PaintedRectInputAssembler);
        theRenderContext.SetCullingEnabled(false);
        // Render exactly two triangles
        theRenderContext.Draw(NVRenderDrawMode::Triangles, 6, 0);
        theRenderContext.SetStencilTestEnabled(isStencilEnabled);
        theRenderContext.SetDepthFunction(theDepthFunc);
    }

    void RenderDepthPrepass(SPathRenderContext &inRenderContext,
                                    SLayerGlobalRenderProperties inRenderProperties,
                                    TShaderFeatureSet inFeatureSet) override
    {
        SPathBuffer *thePathBuffer = GetPathBufferObject(inRenderContext.m_Path);
        if (!thePathBuffer) {
            return;
        }

        if (thePathBuffer->m_PathType == PathTypes::Geometry) {
            QT3DSU32 displacementIdx = 0;
            QT3DSU32 imageIdx = 0;
            SRenderableImage *displacementImage = 0;

            for (SRenderableImage *theImage = inRenderContext.m_FirstImage;
                 theImage != NULL && displacementImage == NULL;
                 theImage = theImage->m_NextImage, ++imageIdx) {
                if (theImage->m_MapType == ImageMapTypes::Displacement) {
                    displacementIdx = imageIdx;
                    displacementImage = theImage;
                }
            }

            NVScopedRefCounted<SPathGeneratedShader> &theDesiredDepthShader =
                displacementImage == NULL ? m_DepthShader : m_DepthDisplacementShader;

            if (!theDesiredDepthShader) {
                IDefaultMaterialShaderGenerator &theMaterialGenerator(
                    m_RenderContext->GetDefaultMaterialShaderGenerator());
                SPathVertexPipeline thePipeline(
                    m_RenderContext->GetShaderProgramGenerator(), theMaterialGenerator,
                    m_RenderContext->GetAllocator(), m_RenderContext->GetStringTable(), false);
                thePipeline.BeginVertexGeneration(displacementIdx, displacementImage);
                thePipeline.BeginFragmentGeneration();
                thePipeline.Fragment().Append("\tfragOutput = vec4(1.0, 1.0, 1.0, 1.0);");
                thePipeline.EndVertexGeneration(false);
                thePipeline.EndFragmentGeneration(false);
                const char8_t *shaderName = "path depth";
                if (displacementImage)
                    shaderName = "path depth displacement";

                SShaderCacheProgramFlags theFlags;
                NVRenderShaderProgram *theProgram =
                    thePipeline.ProgramGenerator().CompileGeneratedShader(shaderName, theFlags,
                                                                          inFeatureSet);
                if (theProgram) {
                    theDesiredDepthShader =
                        QT3DS_NEW(m_RenderContext->GetAllocator(),
                               SPathGeneratedShader)(*theProgram, m_RenderContext->GetAllocator());
                }
            }
            if (theDesiredDepthShader) {
                DoRenderGeometryPath(*theDesiredDepthShader, inRenderContext, inRenderProperties,
                                     *thePathBuffer);
            }
        } else {
            // painted path, go stroke route for now.
            if (!m_PaintedDepthShader) {
                IDefaultMaterialShaderGenerator &theMaterialGenerator(
                    m_RenderContext->GetDefaultMaterialShaderGenerator());
                SXYRectVertexPipeline thePipeline(
                    m_RenderContext->GetShaderProgramGenerator(), theMaterialGenerator,
                    m_RenderContext->GetAllocator(), m_RenderContext->GetStringTable());
                thePipeline.BeginVertexGeneration(0, NULL);
                thePipeline.BeginFragmentGeneration();
                thePipeline.Fragment().Append("\tfragOutput = vec4(1.0, 1.0, 1.0, 1.0);");
                thePipeline.EndVertexGeneration(false);
                thePipeline.EndFragmentGeneration(false);
                const char8_t *shaderName = "path painted depth";
                SShaderCacheProgramFlags theFlags;
                NVRenderShaderProgram *theProgram =
                    thePipeline.ProgramGenerator().CompileGeneratedShader(shaderName, theFlags,
                                                                          inFeatureSet);
                if (theProgram) {
                    m_PaintedDepthShader =
                        QT3DS_NEW(m_RenderContext->GetAllocator(), SPathXYGeneratedShader)(
                            *theProgram, m_RenderContext->GetAllocator());
                }
            }
            if (m_PaintedDepthShader) {

                DoRenderPaintedPath(*m_PaintedDepthShader, inRenderContext, inRenderProperties,
                                    *thePathBuffer);
            }
        }
    }

    void RenderShadowMapPass(SPathRenderContext &inRenderContext,
                                     SLayerGlobalRenderProperties inRenderProperties,
                                     TShaderFeatureSet inFeatureSet) override
    {
        SPathBuffer *thePathBuffer = GetPathBufferObject(inRenderContext.m_Path);
        if (!thePathBuffer) {
            return;
        }

        if (inRenderContext.m_Material.m_Type != GraphObjectTypes::DefaultMaterial)
            return;

        if (thePathBuffer->m_PathType == PathTypes::Painted) {
            // painted path, go stroke route for now.
            if (!m_PaintedShadowShader) {
                IDefaultMaterialShaderGenerator &theMaterialGenerator(
                    m_RenderContext->GetDefaultMaterialShaderGenerator());
                SXYRectVertexPipeline thePipeline(
                    m_RenderContext->GetShaderProgramGenerator(), theMaterialGenerator,
                    m_RenderContext->GetAllocator(), m_RenderContext->GetStringTable());
                thePipeline.OutputParaboloidDepthShaders();
                const char8_t *shaderName = "path painted paraboloid depth";
                SShaderCacheProgramFlags theFlags;
                NVRenderShaderProgram *theProgram =
                    thePipeline.ProgramGenerator().CompileGeneratedShader(shaderName, theFlags,
                                                                          inFeatureSet);
                if (theProgram) {
                    m_PaintedShadowShader =
                        QT3DS_NEW(m_RenderContext->GetAllocator(), SPathXYGeneratedShader)(
                            *theProgram, m_RenderContext->GetAllocator());
                }
            }
            if (m_PaintedShadowShader) {
                // Setup the shader paraboloid information.
                NVRenderContext &theRenderContext(m_RenderContext->GetRenderContext());
                theRenderContext.SetActiveShader(&m_PaintedShadowShader->m_Shader);

                DoRenderPaintedPath(*m_PaintedShadowShader, inRenderContext, inRenderProperties,
                                    *thePathBuffer, true);
            }
        } else {
            // Until we've also got a proper path render path for this, we'll call the old-fashioned
            // stuff.
            RenderDepthPrepass(inRenderContext, inRenderProperties, inFeatureSet);
            // QT3DS_ASSERT( false );
        }
    }

    void RenderCubeFaceShadowPass(SPathRenderContext &inRenderContext,
                                          SLayerGlobalRenderProperties inRenderProperties,
                                          TShaderFeatureSet inFeatureSet) override
    {
        SPathBuffer *thePathBuffer = GetPathBufferObject(inRenderContext.m_Path);
        if (!thePathBuffer) {
            return;
        }

        if (inRenderContext.m_Material.m_Type != GraphObjectTypes::DefaultMaterial)
            return;

        if (thePathBuffer->m_PathType == PathTypes::Painted) {
            if (!m_PaintedCubeShadowShader) {
                IDefaultMaterialShaderGenerator &theMaterialGenerator(
                    m_RenderContext->GetDefaultMaterialShaderGenerator());
                SXYRectVertexPipeline thePipeline(
                    m_RenderContext->GetShaderProgramGenerator(), theMaterialGenerator,
                    m_RenderContext->GetAllocator(), m_RenderContext->GetStringTable());
                thePipeline.OutputCubeFaceDepthShaders();
                const char8_t *shaderName = "path painted cube face depth";
                SShaderCacheProgramFlags theFlags;
                NVRenderShaderProgram *theProgram =
                    thePipeline.ProgramGenerator().CompileGeneratedShader(shaderName, theFlags,
                                                                          inFeatureSet);
                if (theProgram) {
                    m_PaintedCubeShadowShader =
                        QT3DS_NEW(m_RenderContext->GetAllocator(), SPathXYGeneratedShader)(
                            *theProgram, m_RenderContext->GetAllocator());
                }
            }
            if (m_PaintedCubeShadowShader) {
                // Setup the shader information.
                NVRenderContext &theRenderContext(m_RenderContext->GetRenderContext());
                theRenderContext.SetActiveShader(&m_PaintedCubeShadowShader->m_Shader);

                m_PaintedCubeShadowShader->m_CameraPosition.Set(
                    inRenderContext.m_Camera.GetGlobalPos());
                m_PaintedCubeShadowShader->m_CameraProperties.Set(
                    QT3DSVec2(1.0f, inRenderContext.m_Camera.m_ClipFar));
                m_PaintedCubeShadowShader->m_ModelMatrix.Set(inRenderContext.m_ModelMatrix);

                DoRenderPaintedPath(*m_PaintedCubeShadowShader, inRenderContext, inRenderProperties,
                                    *thePathBuffer, false);
            }
        } else {
            // Until we've also got a proper path render path for this, we'll call the old-fashioned
            // stuff.
            RenderDepthPrepass(inRenderContext, inRenderProperties, inFeatureSet);
        }
    }

    void RenderPath(SPathRenderContext &inRenderContext,
                            SLayerGlobalRenderProperties inRenderProperties,
                            TShaderFeatureSet inFeatureSet) override
    {
        SPathBuffer *thePathBuffer = GetPathBufferObject(inRenderContext.m_Path);
        if (!thePathBuffer) {
            return;
        }

        bool isDefaultMaterial =
            (inRenderContext.m_Material.m_Type == GraphObjectTypes::DefaultMaterial);

        if (thePathBuffer->m_PathType == PathTypes::Geometry) {
            IMaterialShaderGenerator *theMaterialGenerator =
                GetMaterialShaderGenertator(inRenderContext);

            // we need a more evolved key her for custom materials
            // the same key can still need a different shader
            SPathShaderMapKey sPathkey = SPathShaderMapKey(GetMaterialNameForKey(inRenderContext),
                                                           inRenderContext.m_MaterialKey);
            eastl::pair<TShaderMap::iterator, bool> inserter = m_PathGeometryShaders.insert(
                eastl::make_pair(sPathkey, NVScopedRefCounted<SPathGeneratedShader>(NULL)));
            if (inserter.second) {
                SPathVertexPipeline thePipeline(
                    m_RenderContext->GetShaderProgramGenerator(), *theMaterialGenerator,
                    m_RenderContext->GetAllocator(), m_RenderContext->GetStringTable(),
                    m_RenderContext->GetWireframeMode());

                NVRenderShaderProgram *theProgram = NULL;

                if (isDefaultMaterial) {
                    theProgram = theMaterialGenerator->GenerateShader(
                        inRenderContext.m_Material, inRenderContext.m_MaterialKey, thePipeline,
                        inFeatureSet, inRenderProperties.m_Lights, inRenderContext.m_FirstImage,
                        inRenderContext.m_Opacity < 1.0, "path geometry pipeline-- ");
                } else {
                    ICustomMaterialSystem &theMaterialSystem(
                        m_RenderContext->GetCustomMaterialSystem());
                    const SCustomMaterial &theCustomMaterial(
                        reinterpret_cast<const SCustomMaterial &>(inRenderContext.m_Material));

                    theProgram = theMaterialGenerator->GenerateShader(
                        inRenderContext.m_Material, inRenderContext.m_MaterialKey, thePipeline,
                        inFeatureSet, inRenderProperties.m_Lights, inRenderContext.m_FirstImage,
                        inRenderContext.m_Opacity < 1.0, "path geometry pipeline-- ",
                        theMaterialSystem.GetShaderName(theCustomMaterial));
                }

                if (theProgram)
                    inserter.first->second =
                        QT3DS_NEW(m_RenderContext->GetAllocator(),
                               SPathGeneratedShader)(*theProgram, m_RenderContext->GetAllocator());
            }
            if (!inserter.first->second)
                return;

            DoRenderGeometryPath(*inserter.first->second.mPtr, inRenderContext, inRenderProperties,
                                 *thePathBuffer);
        } else {
            IMaterialShaderGenerator *theMaterialGenerator =
                GetMaterialShaderGenertator(inRenderContext);

            // we need a more evolved key her for custom materials
            // the same key can still need a different shader
            SPathShaderMapKey sPathkey = SPathShaderMapKey(GetMaterialNameForKey(inRenderContext),
                                                           inRenderContext.m_MaterialKey);
            eastl::pair<TPaintedShaderMap::iterator, bool> inserter = m_PathPaintedShaders.insert(
                eastl::make_pair(sPathkey, NVScopedRefCounted<SPathXYGeneratedShader>(NULL)));

            if (inserter.second) {
                SXYRectVertexPipeline thePipeline(
                    m_RenderContext->GetShaderProgramGenerator(), *theMaterialGenerator,
                    m_RenderContext->GetAllocator(), m_RenderContext->GetStringTable());

                NVRenderShaderProgram *theProgram = NULL;

                if (isDefaultMaterial) {
                    theProgram = theMaterialGenerator->GenerateShader(
                        inRenderContext.m_Material, inRenderContext.m_MaterialKey, thePipeline,
                        inFeatureSet, inRenderProperties.m_Lights, inRenderContext.m_FirstImage,
                        inRenderContext.m_Opacity < 1.0, "path painted pipeline-- ");
                } else {
                    ICustomMaterialSystem &theMaterialSystem(
                        m_RenderContext->GetCustomMaterialSystem());
                    const SCustomMaterial &theCustomMaterial(
                        reinterpret_cast<const SCustomMaterial &>(inRenderContext.m_Material));

                    theProgram = theMaterialGenerator->GenerateShader(
                        inRenderContext.m_Material, inRenderContext.m_MaterialKey, thePipeline,
                        inFeatureSet, inRenderProperties.m_Lights, inRenderContext.m_FirstImage,
                        inRenderContext.m_Opacity < 1.0, "path painted pipeline-- ",
                        theMaterialSystem.GetShaderName(theCustomMaterial));
                }

                if (theProgram)
                    inserter.first->second =
                        QT3DS_NEW(m_RenderContext->GetAllocator(), SPathXYGeneratedShader)(
                            *theProgram, m_RenderContext->GetAllocator());
            }
            if (!inserter.first->second)
                return;

            DoRenderPaintedPath(*inserter.first->second.mPtr, inRenderContext, inRenderProperties,
                                *thePathBuffer);
        }
    }
};
}

QT3DSVec2 IPathManagerCore::GetControlPointFromAngleDistance(QT3DSVec2 inPosition, float inIncomingAngle,
                                                          float inIncomingDistance)
{
    if (inIncomingDistance == 0.0f)
        return inPosition;
    float angleRad = degToRad(inIncomingAngle);
    float angleSin = NVSin(angleRad);
    float angleCos = NVCos(angleRad);
    QT3DSVec2 relativeAngles = QT3DSVec2(angleCos * inIncomingDistance, angleSin * inIncomingDistance);
    return inPosition + relativeAngles;
}

QT3DSVec2 IPathManagerCore::GetAngleDistanceFromControlPoint(QT3DSVec2 inPosition, QT3DSVec2 inControlPoint)
{
    QT3DSVec2 relative = inControlPoint - inPosition;
    float angleRad = atan2(relative.y, relative.x);
    float distance = relative.magnitude();
    return QT3DSVec2(radToDeg(angleRad), distance);
}

IPathManagerCore &IPathManagerCore::CreatePathManagerCore(IQt3DSRenderContextCore &ctx)
{
    return *QT3DS_NEW(ctx.GetAllocator(), SPathManager)(ctx);
}
