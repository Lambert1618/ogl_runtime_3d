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
#include "Qt3DSRenderDefaultMaterialShaderGenerator.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "Qt3DSRenderableImage.h"
#include "Qt3DSRenderImage.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSRenderLight.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderShadowMap.h"
#include "Qt3DSRenderCustomMaterial.h"
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "Qt3DSRenderLightConstantProperties.h"

using namespace qt3ds::render;
using qt3ds::render::NVRenderCachedShaderProperty;
using qt3ds::render::NVRenderCachedShaderBuffer;

namespace {

const QT3DSF32 MINATTENUATION = 0;
const QT3DSF32 MAXATTENUATION = 1000;

QT3DSF32 ClampFloat(QT3DSF32 value, QT3DSF32 min, QT3DSF32 max)
{
    return value < min ? min : ((value > max) ? max : value);
}

QT3DSF32 TranslateConstantAttenuation(QT3DSF32 attenuation)
{
    return attenuation * .01f;
}

QT3DSF32 TranslateLinearAttenuation(QT3DSF32 attenuation)
{
    attenuation = ClampFloat(attenuation, MINATTENUATION, MAXATTENUATION);
    return attenuation * 0.0001f;
}

QT3DSF32 TranslateQuadraticAttenuation(QT3DSF32 attenuation)
{
    attenuation = ClampFloat(attenuation, MINATTENUATION, MAXATTENUATION);
    return attenuation * 0.0000001f;
}

/**
 *	Cached texture property lookups, used one per texture so a shader generator for N
 *	textures will have an array of N of these lookup objects.
 */
struct SShaderTextureProperties
{
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_Sampler;
    NVRenderCachedShaderProperty<QT3DSVec3> m_Offsets;
    NVRenderCachedShaderProperty<QT3DSVec4> m_Rotations;
    NVRenderCachedShaderProperty<QT3DSVec2> m_Size;
    SShaderTextureProperties(const char *sampName, const char *offName, const char *rotName,
                             const char *sizeName,
                             NVRenderShaderProgram &inShader)
        : m_Sampler(sampName, inShader)
        , m_Offsets(offName, inShader)
        , m_Rotations(rotName, inShader)
        , m_Size(sizeName, inShader)
    {
    }
    SShaderTextureProperties() {}
};

/**
 *	Cached light property lookups, used one per light so a shader generator for N
 *	lights will have an array of N of these lookup objects.
 */
struct SShaderLightProperties
{
    // Color of the light
    QT3DSVec4 m_LightColor;
    SLightSourceShader m_LightData;

    SShaderLightProperties() {}
};

struct SShadowMapProperties
{
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_ShadowmapTexture; ///< shadow texture
    NVRenderCachedShaderProperty<NVRenderTextureCube *> m_ShadowCubeTexture; ///< shadow cubemap
    NVRenderCachedShaderProperty<QT3DSMat44>
        m_ShadowmapMatrix; ///< world to ligh space transform matrix
    NVRenderCachedShaderProperty<QT3DSVec4> m_ShadowmapSettings; ///< shadow rendering settings

    SShadowMapProperties() {}
    SShadowMapProperties(const char *shadowmapTextureName, const char *shadowcubeTextureName,
                         const char *shadowmapMatrixName, const char *shadowmapSettingsName,
                         NVRenderShaderProgram &inShader)
        : m_ShadowmapTexture(shadowmapTextureName, inShader)
        , m_ShadowCubeTexture(shadowcubeTextureName, inShader)
        , m_ShadowmapMatrix(shadowmapMatrixName, inShader)
        , m_ShadowmapSettings(shadowmapSettingsName, inShader)
    {
    }
};

/**
 *	The results of generating a shader.  Caches all possible variable names into
 *	typesafe objects.
 */
struct SShaderGeneratorGeneratedShader
{
    NVAllocatorCallback &m_Allocator;
    NVRenderShaderProgram &m_Shader;
    // Specific properties we know the shader has to have.
    NVRenderCachedShaderProperty<QT3DSMat44> m_MVP;
    NVRenderCachedShaderProperty<QT3DSMat33> m_NormalMatrix;
    NVRenderCachedShaderProperty<QT3DSMat44> m_GlobalTransform;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ViewProj;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ViewMatrix;
    NVRenderCachedShaderProperty<QT3DSVec4> m_MaterialDiffuse;
    NVRenderCachedShaderProperty<QT3DSVec4> m_MaterialProperties;
    // tint, ior
    NVRenderCachedShaderProperty<QT3DSVec4> m_MaterialSpecular;
    NVRenderCachedShaderProperty<QT3DSF32> m_BumpAmount;
    NVRenderCachedShaderProperty<QT3DSF32> m_DisplaceAmount;
    NVRenderCachedShaderProperty<QT3DSF32> m_TranslucentFalloff;
    NVRenderCachedShaderProperty<QT3DSF32> m_DiffuseLightWrap;
    NVRenderCachedShaderProperty<QT3DSF32> m_FresnelPower;
    NVRenderCachedShaderProperty<QT3DSVec3> m_DiffuseColor;
    NVRenderCachedShaderProperty<QT3DSVec3> m_CameraPosition;
    NVRenderCachedShaderProperty<QT3DSVec3> m_CameraDirection;
    QT3DSVec3 m_LightAmbientTotal;
    NVRenderCachedShaderProperty<QT3DSVec3> m_MaterialDiffuseLightAmbientTotal;
    NVRenderCachedShaderProperty<QT3DSVec2> m_CameraProperties;

    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_DepthTexture;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_AOTexture;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_LightProbe;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeProps;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeOpts;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeRot;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeOfs;
    NVRenderCachedShaderProperty<QT3DSVec2> m_LightProbeSize;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_LightProbe2;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbe2Props;
    NVRenderCachedShaderProperty<QT3DSVec2> m_LightProbe2Size;
    NVRenderCachedShaderProperty<QT3DSVec2> m_alphaTestOp;

    NVRenderCachedShaderBuffer<qt3ds::render::NVRenderShaderConstantBuffer *> m_AoShadowParams;
    NVRenderCachedShaderBuffer<qt3ds::render::NVRenderShaderConstantBuffer *> m_LightsBuffer;

    SLightConstantProperties<SShaderGeneratorGeneratedShader> *m_lightConstantProperties;

    // Cache the image property name lookups
    nvvector<SShaderTextureProperties> m_Images;
    nvvector<SShaderLightProperties> m_Lights;
    // Cache shadow map properties
    nvvector<SShadowMapProperties> m_ShadowMaps;

    QT3DSI32 m_RefCount;

    SShaderGeneratorGeneratedShader(NVRenderShaderProgram &inShader, NVRenderContext &inContext)
        : m_Allocator(inContext.GetAllocator())
        , m_Shader(inShader)
        , m_MVP("model_view_projection", inShader)
        , m_NormalMatrix("normal_matrix", inShader)
        , m_GlobalTransform("model_matrix", inShader)
        , m_ViewProj("view_projection_matrix", inShader)
        , m_ViewMatrix("view_matrix", inShader)
        , m_MaterialDiffuse("material_diffuse", inShader)
        , m_MaterialProperties("material_properties", inShader)
        , m_MaterialSpecular("material_specular", inShader)
        , m_BumpAmount("bumpAmount", inShader)
        , m_DisplaceAmount("displaceAmount", inShader)
        , m_TranslucentFalloff("translucentFalloff", inShader)
        , m_DiffuseLightWrap("diffuseLightWrap", inShader)
        , m_FresnelPower("fresnelPower", inShader)
        , m_DiffuseColor("diffuse_color", inShader)
        , m_CameraPosition("camera_position", inShader)
        , m_CameraDirection("camera_direction", inShader)
        , m_MaterialDiffuseLightAmbientTotal("light_ambient_total", inShader)
        , m_CameraProperties("camera_properties", inShader)
        , m_DepthTexture("depth_sampler", inShader)
        , m_AOTexture("ao_sampler", inShader)
        , m_LightProbe("light_probe", inShader)
        , m_LightProbeProps("light_probe_props", inShader)
        , m_LightProbeOpts("light_probe_opts", inShader)
        , m_LightProbeRot("light_probe_rotation", inShader)
        , m_LightProbeOfs("light_probe_offset", inShader)
        , m_LightProbeSize("light_probe_size", inShader)
        , m_LightProbe2("light_probe2", inShader)
        , m_LightProbe2Props("light_probe2_props", inShader)
        , m_LightProbe2Size("light_probe2_size", inShader)
        , m_alphaTestOp("alphaOpRef", inShader)
        , m_AoShadowParams("cbAoShadow", inShader)
        , m_LightsBuffer("cbBufferLights", inShader)
        , m_lightConstantProperties(nullptr)
        , m_Images(inContext.GetAllocator(), "SShaderGeneratorGeneratedShader::m_Images")
        , m_Lights(inContext.GetAllocator(), "SShaderGeneratorGeneratedShader::m_Lights")
        , m_ShadowMaps(inContext.GetAllocator(), "SShaderGeneratorGeneratedShader::m_ShadowMaps")
        , m_RefCount(0)
    {
        m_Shader.addRef();
    }
    ~SShaderGeneratorGeneratedShader()
    {
        if (m_lightConstantProperties)
            delete m_lightConstantProperties;
        m_Shader.release();
    }

    void addRef() { ++m_RefCount; }
    void release()
    {
        --m_RefCount;
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(m_Allocator);
            NVDelete(alloc, this);
        }
    }
};


#ifndef EA_PLATFORM_WINDOWS
#define _snprintf snprintf
#endif

struct SShaderGenerator : public IDefaultMaterialShaderGenerator
{
    typedef Qt3DSString TStrType;
    typedef nvhash_map<NVRenderShaderProgram *, NVScopedRefCounted<SShaderGeneratorGeneratedShader>>
        TProgramToShaderMap;
    typedef qt3ds::foundation::nvhash_map<CRegisteredString,
                                       NVScopedRefCounted<qt3ds::render::NVRenderConstantBuffer>>
        TStrConstanBufMap;

    IQt3DSRenderContext &m_RenderContext;
    IShaderProgramGenerator &m_ProgramGenerator;

    const SDefaultMaterial *m_CurrentMaterial;
    SShaderDefaultMaterialKey *m_CurrentKey;
    Qt3DSShadowMap *m_ShadowMapManager;
    IDefaultMaterialVertexPipeline *m_CurrentPipeline;
    TShaderFeatureSet m_CurrentFeatureSet;
    NVDataRef<SLight *> m_Lights;
    SRenderableImage *m_FirstImage;
    bool m_HasTransparency;
    bool m_LightsAsSeparateUniforms;

    TStrType m_ImageStem;
    TStrType m_ImageSampler;
    TStrType m_ImageOffsets;
    TStrType m_ImageRotations;
    TStrType m_ImageFragCoords;
    TStrType m_ImageTemp;
    TStrType m_ImageSamplerSize;

    TStrType m_TexCoordTemp;

    TStrType m_LightStem;
    TStrType m_LightColor;
    TStrType m_LightSpecularColor;
    TStrType m_LightAttenuation;
    TStrType m_LightConstantAttenuation;
    TStrType m_LightLinearAttenuation;
    TStrType m_LightQuadraticAttenuation;
    TStrType m_NormalizedDirection;
    TStrType m_LightDirection;
    TStrType m_LightPos;
    TStrType m_LightUp;
    TStrType m_LightRt;
    TStrType m_RelativeDistance;
    TStrType m_RelativeDirection;

    TStrType m_ShadowMapStem;
    TStrType m_ShadowCubeStem;
    TStrType m_ShadowMatrixStem;
    TStrType m_ShadowCoordStem;
    TStrType m_ShadowControlStem;

    TStrType m_TempStr;

    eastl::string m_GeneratedShaderString;

    SShaderDefaultMaterialKeyProperties m_DefaultMaterialShaderKeyProperties;
    TProgramToShaderMap m_ProgramToShaderMap;

    TStrConstanBufMap m_ConstantBuffers; ///< store all constants buffers

    QT3DSI32 m_RefCount;

    SShaderGenerator(IQt3DSRenderContext &inRc)
        : m_RenderContext(inRc)
        , m_ProgramGenerator(m_RenderContext.GetShaderProgramGenerator())
        , m_CurrentMaterial(nullptr)
        , m_CurrentKey(nullptr)
        , m_ShadowMapManager(nullptr)
        , m_CurrentPipeline(nullptr)
        , m_FirstImage(nullptr)
        , m_LightsAsSeparateUniforms(false)
        , m_ProgramToShaderMap(inRc.GetAllocator(), "m_ProgramToShaderMap")
        , m_ConstantBuffers(inRc.GetAllocator(), "m_ConstantBuffers")
        , m_RefCount(0)
    {
    }

    void addRef() override { atomicIncrement(&m_RefCount); }
    void release() override
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            m_ConstantBuffers.clear();
            NVDelete(m_RenderContext.GetAllocator(), this);
        }
    }
    IShaderProgramGenerator &ProgramGenerator() { return m_ProgramGenerator; }
    IDefaultMaterialVertexPipeline &VertexGenerator() { return *m_CurrentPipeline; }
    IShaderStageGenerator &FragmentGenerator()
    {
        return *m_ProgramGenerator.GetStage(ShaderGeneratorStages::Fragment);
    }
    SShaderDefaultMaterialKey &Key() { return *m_CurrentKey; }
    const SDefaultMaterial &Material() { return *m_CurrentMaterial; }
    TShaderFeatureSet FeatureSet() { return m_CurrentFeatureSet; }
    bool HasTransparency() { return m_HasTransparency; }

    void addFunction(IShaderStageGenerator &generator, QString functionName)
    {
        generator.AddFunction(functionName);
    }

    void SetupImageVariableNames(size_t imageIdx)
    {
        m_ImageStem = "image";
        char buf[16];
        _snprintf(buf, 16, "%d", int(imageIdx));
        m_ImageStem.append(buf);
        m_ImageStem.append("_");

        m_ImageSampler = m_ImageStem;
        m_ImageSampler.append("sampler");
        m_ImageOffsets = m_ImageStem;
        m_ImageOffsets.append("offsets");
        m_ImageRotations = m_ImageStem;
        m_ImageRotations.append("rotations");
        m_ImageFragCoords = m_ImageStem;
        m_ImageFragCoords.append("uv_coords");
        m_ImageSamplerSize = m_ImageStem;
        m_ImageSamplerSize.append("size");
    }

    void SetupTexCoordVariableName(size_t uvSet)
    {
        m_TexCoordTemp = "varTexCoord";
        char buf[16];
        _snprintf(buf, 16, "%d", int(uvSet));
        m_TexCoordTemp.append(buf);
    }

    SImageVariableNames GetImageVariableNames(QT3DSU32 inIdx) override
    {
        SetupImageVariableNames(inIdx);
        SImageVariableNames retval;
        retval.m_ImageSampler = m_ImageSampler.c_str();
        retval.m_ImageFragCoords = m_ImageFragCoords.c_str();
        return retval;
    }

    void AddLocalVariable(IShaderStageGenerator &inGenerator, const char8_t *inName,
                          const char8_t *inType)
    {
        inGenerator << "    " << inType << " " << inName << ";" << Endl;
    }

    void AddLocalVariable(IShaderStageGenerator &inGenerator, const TStrType &inName,
                          const char8_t *inType)
    {
        AddLocalVariable(inGenerator, inName.c_str(), inType);
    }

    void GenerateImageUVCoordinates(IShaderStageGenerator &inVertexPipeline, QT3DSU32 idx, QT3DSU32 uvSet,
                                    SRenderableImage &image) override
    {
        IDefaultMaterialVertexPipeline &vertexShader(
            static_cast<IDefaultMaterialVertexPipeline &>(inVertexPipeline));
        IShaderStageGenerator &fragmentShader(FragmentGenerator());
        SetupImageVariableNames(idx);
        SetupTexCoordVariableName(uvSet);
        fragmentShader.AddUniform(m_ImageSampler, "sampler2D");
        vertexShader.AddUniform(m_ImageOffsets, "vec3");
        vertexShader.AddUniform(m_ImageRotations, "vec4");

        if (image.m_Image.m_MappingMode == ImageMappingModes::Normal) {
            vertexShader << "    uTransform = vec3( " << m_ImageRotations << ".x, "
                         << m_ImageRotations << ".y, " << m_ImageOffsets << ".x );" << Endl;
            vertexShader << "    vTransform = vec3( " << m_ImageRotations << ".z, "
                         << m_ImageRotations << ".w, " << m_ImageOffsets << ".y );" << Endl;
            vertexShader.AddOutgoing(m_ImageFragCoords, "vec2");
            addFunction(vertexShader, "getTransformedUVCoords");
            vertexShader.GenerateUVCoords(uvSet);
            m_ImageTemp = m_ImageFragCoords;
            m_ImageTemp.append("temp");
            vertexShader << "    vec2 " << m_ImageTemp << " = getTransformedUVCoords( vec3( "
                         << m_TexCoordTemp << ", 1.0), uTransform, vTransform );" << Endl;
            if (image.m_Image.m_TextureData.m_TextureFlags.IsInvertUVCoords()) {
                vertexShader << "    " << m_ImageTemp << ".y = 1.0 - " << m_ImageTemp << ".y;"
                             << Endl;
            }

            vertexShader.AssignOutput(m_ImageFragCoords.c_str(), m_ImageTemp.c_str());
        } else {
            fragmentShader.AddUniform(m_ImageOffsets, "vec3");
            fragmentShader.AddUniform(m_ImageRotations, "vec4");
            fragmentShader << "    uTransform = vec3( " << m_ImageRotations << ".x, "
                           << m_ImageRotations << ".y, " << m_ImageOffsets << ".x );" << Endl;
            fragmentShader << "    vTransform = vec3( " << m_ImageRotations << ".z, "
                           << m_ImageRotations << ".w, " << m_ImageOffsets << ".y );" << Endl;
            vertexShader.GenerateEnvMapReflection();
            addFunction(fragmentShader, "getTransformedUVCoords");
            fragmentShader << "    vec2 " << m_ImageFragCoords
                           << " = getTransformedUVCoords( environment_map_reflection, uTransform, "
                              "vTransform );"
                           << Endl;

            if (image.m_Image.m_TextureData.m_TextureFlags.IsInvertUVCoords()) {
                fragmentShader << "    " << m_ImageFragCoords << ".y = 1.0 - " << m_ImageFragCoords
                               << ".y;" << Endl;
            }
        }
    }

    void GenerateImageUVCoordinates(QT3DSU32 idx, SRenderableImage &image, QT3DSU32 uvSet = 0)
    {
        GenerateImageUVCoordinates(VertexGenerator(), idx, uvSet, image);
    }

    void GenerateImageUVCoordinates(QT3DSU32 idx, SRenderableImage &image,
                                    IDefaultMaterialVertexPipeline &inShader)
    {
        if (image.m_Image.m_MappingMode == ImageMappingModes::Normal) {
            SetupImageVariableNames(idx);
            inShader.AddUniform(m_ImageSampler, "sampler2D");
            inShader.AddUniform(m_ImageOffsets, "vec3");
            inShader.AddUniform(m_ImageRotations, "vec4");

            inShader << "    uTransform = vec3( " << m_ImageRotations << ".x, " << m_ImageRotations
                     << ".y, " << m_ImageOffsets << ".x );" << Endl;
            inShader << "    vTransform = vec3( " << m_ImageRotations << ".z, " << m_ImageRotations
                     << ".w, " << m_ImageOffsets << ".y );" << Endl;
            inShader << "    vec2 " << m_ImageFragCoords << ";" << Endl;
            addFunction(inShader, "getTransformedUVCoords");
            inShader.GenerateUVCoords();
            inShader
                << "    " << m_ImageFragCoords
                << " = getTransformedUVCoords( vec3( varTexCoord0, 1.0), uTransform, vTransform );"
                << Endl;
            if (image.m_Image.m_TextureData.m_TextureFlags.IsInvertUVCoords())
                inShader << "    " << m_ImageFragCoords << ".y = 1.0 - " << m_ImageFragCoords << ".y;"
                         << Endl;
        }
    }

    void OutputSpecularEquation(DefaultMaterialSpecularModel::Enum inSpecularModel,
                                IShaderStageGenerator &fragmentShader, const char *inLightDir,
                                const char *inLightSpecColor)
    {
        switch (inSpecularModel) {
        case DefaultMaterialSpecularModel::KGGX: {
            fragmentShader.AddInclude("defaultMaterialPhysGlossyBSDF.glsllib");
            fragmentShader.AddUniform("material_specular", "vec4");
            fragmentShader << "    global_specular_light.rgb += lightAttenuation * specularAmount * "
                              "specularColor * kggxGlossyDefaultMtl( "
                           << "world_normal, tangent, -" << inLightDir << ".xyz, view_vector, "
                           << inLightSpecColor
                           << ".rgb, vec3(material_specular.xyz), "
                              "roughnessAmount ).rgb;"
                           << Endl;
        } break;
        case DefaultMaterialSpecularModel::KWard: {
            fragmentShader.AddInclude("defaultMaterialPhysGlossyBSDF.glsllib");
            fragmentShader.AddUniform("material_specular", "vec4");
            fragmentShader << "    global_specular_light.rgb += lightAttenuation * specularAmount * "
                              "specularColor * wardGlossyDefaultMtl( "
                           << "world_normal, tangent, -" << inLightDir << ".xyz, view_vector, "
                           << inLightSpecColor
                           << ".rgb, vec3(material_specular.xyz), "
                              "roughnessAmount).rgb;"
                           << Endl;
        } break;
        default:
            addFunction(fragmentShader, "specularBSDF");
            fragmentShader << "    global_specular_light.rgb += lightAttenuation * specularAmount * "
                              "specularColor * specularBSDF( "
                           << "world_normal, -" << inLightDir << ".xyz, view_vector, "
                           << inLightSpecColor << ".rgb, 2.56 / (roughnessAmount + "
                                                  "0.01)).rgb;"
                           << Endl;
            break;
        }
    }

    void OutputDiffuseAreaLighting(IShaderStageGenerator &infragmentShader, const char *inPos,
                                   TStrType inLightPrefix)
    {
        m_NormalizedDirection = inLightPrefix + "_areaDir";
        AddLocalVariable(infragmentShader, m_NormalizedDirection, "vec3");
        infragmentShader << "    lightAttenuation = calculateDiffuseAreaOld( " << m_LightDirection
                         << ".xyz, " << m_LightPos << ".xyz, " << m_LightUp << ", " << m_LightRt
                         << ", " << inPos << ", " << m_NormalizedDirection << " );" << Endl;
    }

    void OutputSpecularAreaLighting(IShaderStageGenerator &infragmentShader, const char *inPos,
                                    const char *inView, const char *inLightSpecColor)
    {
        addFunction(infragmentShader, "sampleAreaGlossyDefault");
        infragmentShader.AddUniform("material_specular", "vec4");
        infragmentShader << "global_specular_light.rgb += " << inLightSpecColor
                         << ".rgb * lightAttenuation * shadowFac * material_specular.rgb * "
                            "specularAmount * sampleAreaGlossyDefault( tanFrame, "
                         << inPos << ", " << m_NormalizedDirection << ", " << m_LightPos << ".xyz, "
                         << m_LightRt << ".w, " << m_LightUp << ".w, " << inView
                         << ", roughnessAmount).rgb;" << Endl;
    }

    void AddTranslucencyIrradiance(IShaderStageGenerator &infragmentShader, SRenderableImage *image,
                                   TStrType inLightPrefix, bool areaLight)
    {
        if (image == nullptr)
            return;

        addFunction(infragmentShader, "diffuseReflectionWrapBSDF");
        if (areaLight) {
            infragmentShader << "    global_diffuse_light.rgb += lightAttenuation * "
                                "translucent_thickness_exp * diffuseReflectionWrapBSDF( "
                                "-world_normal, "
                             << m_NormalizedDirection << ", " << m_LightColor
                             << ".rgb, diffuseLightWrap ).rgb;" << Endl;
        } else {
            infragmentShader << "    global_diffuse_light.rgb += lightAttenuation * "
                                "translucent_thickness_exp * diffuseReflectionWrapBSDF( "
                                "-world_normal, "
                             << "-" << m_NormalizedDirection << ", " << m_LightColor
                             << ".rgb, diffuseLightWrap ).rgb;" << Endl;
        }
    }

    void SetupShadowMapVariableNames(size_t lightIdx)
    {
        m_ShadowMapStem = "shadowmap";
        m_ShadowCubeStem = "shadowcube";
        char buf[16];
        _snprintf(buf, 16, "%d", int(lightIdx));
        m_ShadowMapStem.append(buf);
        m_ShadowCubeStem.append(buf);
        m_ShadowMatrixStem = m_ShadowMapStem;
        m_ShadowMatrixStem.append("_matrix");
        m_ShadowCoordStem = m_ShadowMapStem;
        m_ShadowCoordStem.append("_coord");
        m_ShadowControlStem = m_ShadowMapStem;
        m_ShadowControlStem.append("_control");
    }

    void AddShadowMapContribution(IShaderStageGenerator &inLightShader, QT3DSU32 lightIndex,
                                  RenderLightTypes::Enum inType)
    {
        SetupShadowMapVariableNames(lightIndex);

        inLightShader.AddInclude("shadowMapping.glsllib");
        if (inType == RenderLightTypes::Directional) {
            inLightShader.AddUniform(m_ShadowMapStem, "sampler2D");
        } else {
            inLightShader.AddUniform(m_ShadowCubeStem, "samplerCube");
        }
        inLightShader.AddUniform(m_ShadowControlStem, "vec4");
        inLightShader.AddUniform(m_ShadowMatrixStem, "mat4");

        /*
        if ( inType == RenderLightTypes::Area )
        {
                inLightShader << "vec2 " << m_ShadowCoordStem << ";" << Endl;
                inLightShader << "    shadow_map_occl = sampleParaboloid( " << m_ShadowMapStem << ", "
        << m_ShadowControlStem << ", "
                                                                                <<
        m_ShadowMatrixStem << ", varWorldPos, vec2(1.0, " << m_ShadowControlStem << ".z), "
                                                                                << m_ShadowCoordStem
        << " );" << Endl;
        }
        else */
        if (inType != RenderLightTypes::Directional) {
            inLightShader << "    shadow_map_occl = sampleCubemap( " << m_ShadowCubeStem << ", "
                          << m_ShadowControlStem << ", " << m_ShadowMatrixStem << ", " << m_LightPos
                          << ".xyz, varWorldPos, vec2(1.0, " << m_ShadowControlStem << ".z) );"
                          << Endl;
        } else
            inLightShader << "    shadow_map_occl = sampleOrthographic( " << m_ShadowMapStem << ", "
                          << m_ShadowControlStem << ", " << m_ShadowMatrixStem
                          << ", varWorldPos, vec2(1.0, " << m_ShadowControlStem << ".z) );" << Endl;
    }

    void AddDisplacementMappingForDepthPass(IShaderStageGenerator &inShader) override
    {
        inShader.AddIncoming("attr_uv0", "vec2");
        inShader.AddIncoming("attr_norm", "vec3");
        inShader.AddUniform("displacementSampler", "sampler2D");
        inShader.AddUniform("displaceAmount", "float");
        inShader.AddUniform("displacementMap_rot", "vec4");
        inShader.AddUniform("displacementMap_offset", "vec3");
        inShader.AddInclude("defaultMaterialFileDisplacementTexture.glsllib");

        inShader.Append("    vec3 uTransform = vec3( displacementMap_rot.x, displacementMap_rot.y, "
                        "displacementMap_offset.x );");
        inShader.Append("    vec3 vTransform = vec3( displacementMap_rot.z, displacementMap_rot.w, "
                        "displacementMap_offset.y );");
        addFunction(inShader, "getTransformedUVCoords");
        inShader.Append("    vec2 uv_coords = attr_uv0;");
        inShader << "    uv_coords = getTransformedUVCoords( vec3( uv_coords, 1.0), uTransform, "
                    "vTransform );\n";
        inShader << "    vec3 displacedPos = defaultMaterialFileDisplacementTexture( "
                    "displacementSampler , displaceAmount, uv_coords , attr_norm, attr_pos );"
                 << Endl;
        inShader.Append("    gl_Position = model_view_projection * vec4(displacedPos, 1.0);");
    }

    void AddDisplacementImageUniforms(IShaderStageGenerator &inGenerator,
                                      QT3DSU32 displacementImageIdx,
                                      SRenderableImage *displacementImage) override
    {
        if (displacementImage) {
            SetupImageVariableNames(displacementImageIdx);
            inGenerator.AddInclude("defaultMaterialFileDisplacementTexture.glsllib");
            inGenerator.AddUniform("model_matrix", "mat4");
            inGenerator.AddUniform("camera_position", "vec3");
            inGenerator.AddUniform("displaceAmount", "float");
            inGenerator.AddUniform(m_ImageSampler, "sampler2D");
        }
    }

    bool MaybeAddMaterialFresnel(IShaderStageGenerator &fragmentShader, NVConstDataRef<QT3DSU32> inKey,
                                 bool inFragmentHasSpecularAmount)
    {
        if (m_DefaultMaterialShaderKeyProperties.m_FresnelEnabled.GetValue(inKey)) {
            if (inFragmentHasSpecularAmount == false)
                fragmentShader << "    float specularAmount = 1.0;" << Endl;
            inFragmentHasSpecularAmount = true;
            fragmentShader.AddInclude("defaultMaterialFresnel.glsllib");
            fragmentShader.AddUniform("fresnelPower", "float");
            fragmentShader.AddUniform("material_specular", "vec4");
            fragmentShader << "    float fresnelRatio = defaultMaterialSimpleFresnel( world_normal, "
                              "view_vector, material_specular.w, fresnelPower );"
                           << Endl;
            fragmentShader << "    specularAmount *= fresnelRatio;" << Endl;
        }
        return inFragmentHasSpecularAmount;
    }
    void SetupLightVariableNames(size_t lightIdx, SLight &inLight)
    {
        if (m_LightsAsSeparateUniforms) {
            char buf[16];
            _snprintf(buf, 16, "light_%d", int(lightIdx));
            m_LightStem = buf;
            m_LightColor = m_LightStem;
            m_LightColor.append("_diffuse");
            m_LightDirection = m_LightStem;
            m_LightDirection.append("_direction");
            m_LightSpecularColor = m_LightStem;
            m_LightSpecularColor.append("_specular");
            if (inLight.m_LightType == RenderLightTypes::Point) {
                m_LightPos = m_LightStem;
                m_LightPos.append("_position");
                m_LightAttenuation = m_LightStem;
                m_LightAttenuation.append("_attenuation");
            } else if (inLight.m_LightType == RenderLightTypes::Area) {
                m_LightPos = m_LightStem;
                m_LightPos.append("_position");
                m_LightUp = m_LightStem;
                m_LightUp.append("_up");
                m_LightRt = m_LightStem;
                m_LightRt.append("_right");
            }
        } else {
            m_LightStem = "lights";
            char buf[16];
            _snprintf(buf, 16, "[%d].", int(lightIdx));
            m_LightStem.append(buf);

            m_LightColor = m_LightStem;
            m_LightColor.append("diffuse");
            m_LightDirection = m_LightStem;
            m_LightDirection.append("direction");
            m_LightSpecularColor = m_LightStem;
            m_LightSpecularColor.append("specular");
            if (inLight.m_LightType == RenderLightTypes::Point) {
                m_LightPos = m_LightStem;
                m_LightPos.append("position");
                m_LightConstantAttenuation = m_LightStem;
                m_LightConstantAttenuation.append("constantAttenuation");
                m_LightLinearAttenuation = m_LightStem;
                m_LightLinearAttenuation.append("linearAttenuation");
                m_LightQuadraticAttenuation = m_LightStem;
                m_LightQuadraticAttenuation.append("quadraticAttenuation");
            } else if (inLight.m_LightType == RenderLightTypes::Area) {
                m_LightPos = m_LightStem;
                m_LightPos.append("position");
                m_LightUp = m_LightStem;
                m_LightUp.append("up");
                m_LightRt = m_LightStem;
                m_LightRt.append("right");
            }
        }
    }

    void addDisplacementMapping(IDefaultMaterialVertexPipeline &inShader)
    {
        inShader.AddIncoming("attr_uv0", "vec2");
        inShader.AddIncoming("attr_norm", "vec3");
        inShader.AddUniform("displacementSampler", "sampler2D");
        inShader.AddUniform("displaceAmount", "float");
        inShader.AddUniform("displacementMap_rot", "vec4");
        inShader.AddUniform("displacementMap_offset", "vec3");
        inShader.AddInclude("defaultMaterialFileDisplacementTexture.glsllib");

        inShader.Append("    vec3 uTransform = vec3( displacementMap_rot.x, displacementMap_rot.y, "
                        "displacementMap_offset.x );");
        inShader.Append("    vec3 vTransform = vec3( displacementMap_rot.z, displacementMap_rot.w, "
                        "displacementMap_offset.y );");
        addFunction(inShader, "getTransformedUVCoords");
        inShader.GenerateUVCoords();
        inShader << "    varTexCoord0 = getTransformedUVCoords( vec3( varTexCoord0, 1.0), "
                    "uTransform, vTransform );\n";
        inShader << "    vec3 displacedPos = defaultMaterialFileDisplacementTexture( "
                    "displacementSampler , displaceAmount, varTexCoord0 , attr_norm, attr_pos );"
                 << Endl;
        inShader.Append("    gl_Position = model_view_projection * vec4(displacedPos, 1.0);");
    }

    void GenerateTextureSwizzle(NVRenderTextureSwizzleMode::Enum swizzleMode,
                                eastl::basic_string<char8_t> &texSwizzle,
                                eastl::basic_string<char8_t> &lookupSwizzle)
    {
        qt3ds::render::NVRenderContextType deprecatedContextFlags(NVRenderContextValues::GL2
                                                               | NVRenderContextValues::GLES2);

        if (!(m_RenderContext.GetRenderContext().GetRenderContextType() & deprecatedContextFlags)) {
            switch (swizzleMode) {
            case NVRenderTextureSwizzleMode::L8toR8:
            case NVRenderTextureSwizzleMode::L16toR16:
                texSwizzle.append(".rgb");
                lookupSwizzle.append(".rrr");
                break;
            case NVRenderTextureSwizzleMode::L8A8toRG8:
                texSwizzle.append(".rgba");
                lookupSwizzle.append(".rrrg");
                break;
            case NVRenderTextureSwizzleMode::A8toR8:
                texSwizzle.append(".a");
                lookupSwizzle.append(".r");
                break;
            default:
                break;
            }
        }
    }

    ///< get the light constant buffer and generate if necessary
    NVRenderConstantBuffer *GetLightConstantBuffer(QT3DSU32 inLightCount)
    {
        NVRenderContext &theContext(m_RenderContext.GetRenderContext());

        // we assume constant buffer support
        QT3DS_ASSERT(theContext.GetConstantBufferSupport());

        // we only create if if we have lights
        if (!inLightCount || !theContext.GetConstantBufferSupport())
            return nullptr;

        CRegisteredString theName = theContext.GetStringTable().RegisterStr("cbBufferLights");
        NVRenderConstantBuffer *pCB = theContext.GetConstantBuffer(theName);

        if (!pCB) {
            // create
            SLightSourceShader s[QT3DS_MAX_NUM_LIGHTS];
            NVDataRef<QT3DSU8> cBuffer((QT3DSU8 *)&s, (sizeof(SLightSourceShader) * QT3DS_MAX_NUM_LIGHTS)
                                        + (4 * sizeof(QT3DSI32)));
            pCB = theContext.CreateConstantBuffer(
                theName, qt3ds::render::NVRenderBufferUsageType::Static,
                (sizeof(SLightSourceShader) * QT3DS_MAX_NUM_LIGHTS) + (4 * sizeof(QT3DSI32)), cBuffer);
            if (!pCB) {
                QT3DS_ASSERT(false);
                return nullptr;
            }
            // init first set
            memset(&s[0], 0x0, sizeof(SLightSourceShader));
            QT3DSI32 cgLights = 0;
            pCB->UpdateRaw(0, NVDataRef<QT3DSU8>((QT3DSU8 *)&cgLights, sizeof(QT3DSI32)));
            pCB->UpdateRaw(4 * sizeof(QT3DSI32),
                           NVDataRef<QT3DSU8>((QT3DSU8 *)&s[0], sizeof(SLightSourceShader)));
            pCB->Update(); // update to hardware

            m_ConstantBuffers.insert(eastl::make_pair(theName, pCB));
        }

        return pCB;
    }

    void SetImageShaderVariables(SShaderGeneratorGeneratedShader &inShader,
                                 SRenderableImage &inImage, QT3DSU32 idx)
    {
        size_t numImageVariables = inShader.m_Images.size();
        for (size_t namesIdx = numImageVariables; namesIdx <= idx; ++namesIdx) {
            SetupImageVariableNames(idx);
            inShader.m_Images.push_back(
                SShaderTextureProperties(m_ImageSampler.c_str(), m_ImageOffsets.c_str(),
                                         m_ImageRotations.c_str(), m_ImageSamplerSize.c_str(),
                                         inShader.m_Shader));
        }
        SShaderTextureProperties &theShaderProps = inShader.m_Images[idx];
        const QT3DSMat44 &textureTransform = inImage.m_Image.m_TextureTransform;
        // We separate rotational information from offset information so that just maybe the shader
        // will attempt to push less information to the card.
        const QT3DSF32 *dataPtr(textureTransform.front());
        // The third member of the offsets contains a flag indicating if the texture was
        // premultiplied or not.
        // We use this to mix the texture alpha.
        QT3DSVec3 offsets(dataPtr[12], dataPtr[13],
                       inImage.m_Image.m_TextureData.m_TextureFlags.IsPreMultiplied() ? 1.0f
                                                                                      : 0.0f);
        // Grab just the upper 2x2 rotation matrix from the larger matrix.
        QT3DSVec4 rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

        // The image horizontal and vertical tiling modes need to be set here, before we set texture
        // on the shader.
        // because setting the image on the texture forces the textue to bind and immediately apply
        // any tex params.
        NVRenderTexture2D *imageTexture = inImage.m_Image.m_TextureData.m_Texture;
        inImage.m_Image.m_TextureData.m_Texture->SetTextureWrapS(
            inImage.m_Image.m_HorizontalTilingMode);
        inImage.m_Image.m_TextureData.m_Texture->SetTextureWrapT(
            inImage.m_Image.m_VerticalTilingMode);
        theShaderProps.m_Sampler.Set(imageTexture);
        theShaderProps.m_Offsets.Set(offsets);
        theShaderProps.m_Rotations.Set(rotations);
        theShaderProps.m_Size.Set(QT3DSVec2(imageTexture->GetTextureDetails().m_Width,
                                            imageTexture->GetTextureDetails().m_Height));
    }

    void GenerateShadowMapOcclusion(QT3DSU32 lightIdx, bool inShadowEnabled,
                                    RenderLightTypes::Enum inType)
    {
        if (inShadowEnabled) {
            VertexGenerator().GenerateWorldPosition();
            AddShadowMapContribution(FragmentGenerator(), lightIdx, inType);
            /*
            VertexGenerator().AddUniform( m_ShadowMatrixStem, "mat4" );
            VertexGenerator().AddOutgoing( m_ShadowCoordStem, "vec4" );
            VertexGenerator() << "    vec4 local_" << m_ShadowCoordStem << " = " << m_ShadowMatrixStem
            << " * vec4(local_model_world_position, 1.0);" << Endl;
            m_TempStr.assign( "local_" );
            m_TempStr.append( m_ShadowCoordStem );
            VertexGenerator().AssignOutput( m_ShadowCoordStem.c_str(), m_TempStr.c_str() );
            */
        } else {
            FragmentGenerator() << "    shadow_map_occl = 1.0;" << Endl;
        }
    }

    void GenerateVertexShader()
    {
        // vertex displacement
        QT3DSU32 imageIdx = 0;
        SRenderableImage *displacementImage = nullptr;
        QT3DSU32 displacementImageIdx = 0;

        for (SRenderableImage *img = m_FirstImage; img != nullptr;
             img = img->m_NextImage, ++imageIdx) {
            if (img->m_MapType == ImageMapTypes::Displacement) {
                displacementImage = img;
                displacementImageIdx = imageIdx;
                break;
            }
        }

        // the pipeline opens/closes up the shaders stages
        VertexGenerator().BeginVertexGeneration(displacementImageIdx, displacementImage);
    }

    void GenerateFragmentShader(SShaderDefaultMaterialKey &inKey)
    {
        bool specularEnabled = Material().IsSpecularEnabled();
        bool vertexColorsEnabled = Material().IsVertexColorsEnabled();

        bool hasLighting = Material().HasLighting();
        bool hasImage = m_FirstImage != nullptr;

        bool hasIblProbe = m_DefaultMaterialShaderKeyProperties.m_HasIbl.GetValue(inKey);
        bool hasSpecMap = false;
        bool hasEnvMap = false;
        bool hasEmissiveMap = false;
        bool hasLightmaps = false;
        // Pull the bump out as
        SRenderableImage *bumpImage = nullptr;
        QT3DSU32 imageIdx = 0;
        QT3DSU32 bumpImageIdx = 0;
        SRenderableImage *specularAmountImage = nullptr;
        QT3DSU32 specularAmountImageIdx = 0;
        SRenderableImage *roughnessImage = nullptr;
        QT3DSU32 roughnessImageIdx = 0;
        // normal mapping
        SRenderableImage *normalImage = nullptr;
        QT3DSU32 normalImageIdx = 0;
        // translucency map
        SRenderableImage *translucencyImage = nullptr;
        QT3DSU32 translucencyImageIdx = 0;
        // lightmaps
        SRenderableImage *lightmapIndirectImage = nullptr;
        QT3DSU32 lightmapIndirectImageIdx = 0;
        SRenderableImage *lightmapRadiosityImage = nullptr;
        QT3DSU32 lightmapRadiosityImageIdx = 0;
        SRenderableImage *lightmapShadowImage = nullptr;
        QT3DSU32 lightmapShadowImageIdx = 0;
        const bool supportStandardDerivatives
                = m_RenderContext.GetRenderContext().IsStandardDerivativesSupported();

        for (SRenderableImage *img = m_FirstImage; img != nullptr;
             img = img->m_NextImage, ++imageIdx) {
            hasSpecMap = img->m_MapType == ImageMapTypes::Specular;
            if (img->m_MapType == ImageMapTypes::Bump) {
                bumpImage = img;
                bumpImageIdx = imageIdx;
            } else if (img->m_MapType == ImageMapTypes::SpecularAmountMap) {
                specularAmountImage = img;
                specularAmountImageIdx = imageIdx;
            } else if (img->m_MapType == ImageMapTypes::Roughness) {
                roughnessImage = img;
                roughnessImageIdx = imageIdx;
            } else if (img->m_MapType == ImageMapTypes::Normal) {
                normalImage = img;
                normalImageIdx = imageIdx;
            } else if (img->m_Image.m_MappingMode == ImageMappingModes::Environment) {
                hasEnvMap = true;
            } else if (img->m_MapType == ImageMapTypes::Translucency) {
                translucencyImage = img;
                translucencyImageIdx = imageIdx;
            } else if (img->m_MapType == ImageMapTypes::Emissive) {
                hasEmissiveMap = true;
            } else if (img->m_MapType == ImageMapTypes::LightmapIndirect) {
                lightmapIndirectImage = img;
                lightmapIndirectImageIdx = imageIdx;
                hasLightmaps = true;
            } else if (img->m_MapType == ImageMapTypes::LightmapRadiosity) {
                lightmapRadiosityImage = img;
                lightmapRadiosityImageIdx = imageIdx;
                hasLightmaps = true;
            } else if (img->m_MapType == ImageMapTypes::LightmapShadow) {
                lightmapShadowImage = img;
                lightmapShadowImageIdx = imageIdx;
                hasLightmaps = true;
            }
        }

        bool enableFresnel = m_DefaultMaterialShaderKeyProperties.m_FresnelEnabled.GetValue(inKey);
        bool enableSSAO = false;
        bool enableSSDO = false;
        bool enableShadowMaps = false;
        bool enableBumpNormal = normalImage || bumpImage;
        bool enableAlphaTest
                = m_DefaultMaterialShaderKeyProperties.m_AlphaTestEnabled.GetValue(inKey);

        for (QT3DSU32 idx = 0; idx < FeatureSet().size(); ++idx) {
            eastl::string name(FeatureSet()[idx].m_Name.c_str());
            if (name == "QT3DS_ENABLE_SSAO")
                enableSSAO = FeatureSet()[idx].m_Enabled;
            else if (name == "QT3DS_ENABLE_SSDO")
                enableSSDO = FeatureSet()[idx].m_Enabled;
            else if (name == "QT3DS_ENABLE_SSM")
                enableShadowMaps = FeatureSet()[idx].m_Enabled;
        }

        bool includeSSAOSSDOVars = enableSSAO || enableSSDO || enableShadowMaps;

        VertexGenerator().BeginFragmentGeneration();
        IShaderStageGenerator &fragmentShader(FragmentGenerator());
        IDefaultMaterialVertexPipeline &vertexShader(VertexGenerator());

        // The fragment or vertex shaders may not use the material_properties or diffuse
        // uniforms in all cases but it is simpler to just add them and let the linker strip them.
        fragmentShader.AddUniform("material_diffuse", "vec4");
        fragmentShader.AddUniform("diffuse_color", "vec3");
        fragmentShader.AddUniform("material_properties", "vec4");

        // All these are needed for SSAO
        if (includeSSAOSSDOVars) {
            fragmentShader.AddInclude("SSAOCustomMaterial.glsllib");
            // fragmentShader.AddUniform( "ao_sampler", "sampler2D" );
        }

        if (hasIblProbe && hasLighting) {
            fragmentShader.AddInclude("sampleProbe.glsllib");
        }

        if (hasLighting) {
            if (!m_LightsAsSeparateUniforms)
                addFunction(fragmentShader, "sampleLightVars");
            addFunction(fragmentShader, "diffuseReflectionBSDF");
        }

        if (hasLighting && hasLightmaps) {
            fragmentShader.AddInclude("evalLightmaps.glsllib");
        }

        // view_vector, varWorldPos, world_normal are all used if there is a specular map
        // in addition to if there is specular lighting.  So they are lifted up here, always
        // generated.
        // we rely on the linker to strip out what isn't necessary instead of explicitly stripping
        // it for code simplicity.
        if (hasImage || hasLighting || hasLightmaps) {
            fragmentShader.Append("    vec3 uTransform;");
            fragmentShader.Append("    vec3 vTransform;");
        }

        if (includeSSAOSSDOVars || hasSpecMap || hasLighting || hasEnvMap || enableFresnel
                || hasIblProbe || enableBumpNormal) {
            vertexShader.GenerateViewVector();
            vertexShader.GenerateWorldNormal();
            vertexShader.GenerateWorldPosition();
        }
        if (includeSSAOSSDOVars || specularEnabled || hasIblProbe || enableBumpNormal)
            vertexShader.GenerateVarTangentAndBinormal();

        if (vertexColorsEnabled)
            vertexShader.GenerateVertexColor();
        else
            fragmentShader.Append("    vec3 vertColor = vec3(1.0);");

        // You do bump or normal mapping but not both
        if (bumpImage != nullptr) {
            GenerateImageUVCoordinates(bumpImageIdx, *bumpImage);
            fragmentShader.AddUniform("bumpAmount", "float");

            fragmentShader.AddUniform(m_ImageSamplerSize, "vec2");
            fragmentShader.AddInclude("defaultMaterialBumpNoLod.glsllib");
            fragmentShader << "    world_normal = defaultMaterialBumpNoLod( " << m_ImageSampler
                           << ", bumpAmount, " << m_ImageFragCoords
                           << ", tangent, binormal, world_normal, "
                           << m_ImageSamplerSize << ");" << Endl;
            // Do gram schmidt
            fragmentShader << "    binormal = normalize(cross(world_normal, tangent) );\n";
            fragmentShader << "    tangent = normalize(cross(binormal, world_normal) );\n";

        } else if (normalImage != nullptr) {
            GenerateImageUVCoordinates(normalImageIdx, *normalImage);

            fragmentShader.AddFunction("sampleNormalTexture");
            fragmentShader.AddUniform("bumpAmount", "float");

            fragmentShader << "    world_normal = sampleNormalTexture( "
                           << m_ImageSampler << ", bumpAmount, " << m_ImageFragCoords
                           << ", tangent, binormal, world_normal);" << Endl;
        }

        if (includeSSAOSSDOVars || specularEnabled || hasIblProbe || enableBumpNormal)
            fragmentShader << "    mat3 tanFrame = mat3(tangent, binormal, world_normal);" << Endl;

        bool fragmentHasSpecularAmount = false;

        if (hasEmissiveMap) {
            fragmentShader.Append("    vec3 global_emission = material_diffuse.rgb;");
        }

        if (hasLighting) {
            fragmentShader.AddUniform("light_ambient_total", "vec3");

            fragmentShader.Append(
                "    vec4 global_diffuse_light = vec4(light_ambient_total.xyz, 1.0);");
            fragmentShader.Append("    vec3 global_specular_light = vec3(0.0, 0.0, 0.0);");
            fragmentShader.Append("    float shadow_map_occl = 1.0;");

            if (specularEnabled) {
                vertexShader.GenerateViewVector();
                fragmentShader.AddUniform("material_properties", "vec4");
            }

            if (lightmapIndirectImage != nullptr) {
                GenerateImageUVCoordinates(lightmapIndirectImageIdx, *lightmapIndirectImage, 1);
                fragmentShader << "    vec4 indirect_light = texture2D( " << m_ImageSampler << ", "
                               << m_ImageFragCoords << ");" << Endl;
                fragmentShader << "    global_diffuse_light += indirect_light;" << Endl;
                if (specularEnabled) {
                    fragmentShader
                        << "    global_specular_light += indirect_light.rgb * material_properties.x;"
                        << Endl;
                }
            }

            if (lightmapRadiosityImage != nullptr) {
                GenerateImageUVCoordinates(lightmapRadiosityImageIdx, *lightmapRadiosityImage, 1);
                fragmentShader << "    vec4 direct_light = texture2D( " << m_ImageSampler << ", "
                               << m_ImageFragCoords << ");" << Endl;
                fragmentShader << "    global_diffuse_light += direct_light;" << Endl;
                if (specularEnabled) {
                    fragmentShader
                        << "    global_specular_light += direct_light.rgb * material_properties.x;"
                        << Endl;
                }
            }

            if (translucencyImage != nullptr) {
                fragmentShader.AddUniform("translucentFalloff", "float");
                fragmentShader.AddUniform("diffuseLightWrap", "float");

                GenerateImageUVCoordinates(translucencyImageIdx, *translucencyImage);

                fragmentShader << "    vec4 translucent_depth_range = texture2D( " << m_ImageSampler
                               << ", " << m_ImageFragCoords << ");" << Endl;
                fragmentShader << "    float translucent_thickness = translucent_depth_range.r * "
                                  "translucent_depth_range.r;"
                               << Endl;
                fragmentShader << "    float translucent_thickness_exp = exp( translucent_thickness "
                                  "* translucentFalloff);"
                               << Endl;
            }

            fragmentShader.Append("    float lightAttenuation = 1.0;");

            AddLocalVariable(fragmentShader, "aoFactor", "float");

            if (hasLighting && enableSSAO)
                fragmentShader.Append("    aoFactor = customMaterialAO();");
            else
                fragmentShader.Append("    aoFactor = 1.0;");

            AddLocalVariable(fragmentShader, "shadowFac", "float");

            if (specularEnabled) {
                fragmentShader << "    float specularAmount = material_properties.x;" << Endl;
                fragmentHasSpecularAmount = true;
            }
            // Fragment lighting means we can perhaps attenuate the specular amount by a texture
            // lookup.

            fragmentShader << "    vec3 specularColor = vec3(1.0);" << Endl;
            if (specularAmountImage) {
                if (!specularEnabled)
                    fragmentShader << "    float specularAmount = 1.0;" << Endl;
                GenerateImageUVCoordinates(specularAmountImageIdx, *specularAmountImage);
                fragmentShader << "    specularColor = texture2D( "
                               << m_ImageSampler << ", " << m_ImageFragCoords << " ).xyz;" << Endl;
                fragmentHasSpecularAmount = true;
            }

            fragmentShader << "    float roughnessAmount = material_properties.y;" << Endl;
            if (roughnessImage) {
                GenerateImageUVCoordinates(roughnessImageIdx, *roughnessImage);
                fragmentShader << "    float sampledRoughness = texture2D( "
                               << m_ImageSampler << ", " << m_ImageFragCoords << " ).x;" << Endl;
                //The roughness sampled from roughness textures is Disney roughness
                //which has to be squared to get the proper value
                fragmentShader << "    roughnessAmount = roughnessAmount * "
                               << "sampledRoughness * sampledRoughness;" << Endl;
            }

            fragmentHasSpecularAmount =
                MaybeAddMaterialFresnel(fragmentShader, inKey, fragmentHasSpecularAmount);

            // Iterate through all lights
            for (QT3DSU32 lightIdx = 0; lightIdx < m_Lights.size(); ++lightIdx) {
                SLight *lightNode = m_Lights[lightIdx];
                SetupLightVariableNames(lightIdx, *lightNode);
                bool isDirectional = lightNode->m_LightType == RenderLightTypes::Directional;
                bool isArea = lightNode->m_LightType == RenderLightTypes::Area;
                bool isShadow = enableShadowMaps && lightNode->m_CastShadow;

                fragmentShader.Append("");
                char buf[10];
                sprintf(buf, "%d", lightIdx);

                m_TempStr.assign("light");
                m_TempStr.append(buf);

                fragmentShader << "    //Light " << buf << Endl;
                fragmentShader << "    lightAttenuation = 1.0;" << Endl;
                if (isDirectional) {

                    if (m_LightsAsSeparateUniforms) {
                        fragmentShader.AddUniform(m_LightDirection, "vec4");
                        fragmentShader.AddUniform(m_LightColor, "vec4");
                    }

                    if (enableSSDO) {
                        fragmentShader << "    shadowFac = customMaterialShadow( " << m_LightDirection
                                       << ".xyz, varWorldPos );" << Endl;
                    } else {
                        fragmentShader << "    shadowFac = 1.0;" << Endl;
                    }

                    GenerateShadowMapOcclusion(lightIdx, enableShadowMaps && isShadow,
                                               lightNode->m_LightType);

                    if (specularEnabled && enableShadowMaps && isShadow)
                        fragmentShader << "    lightAttenuation *= shadow_map_occl;" << Endl;

                    fragmentShader << "    global_diffuse_light.rgb += shadowFac * shadow_map_occl * "
                                      "diffuseReflectionBSDF( world_normal, "
                                   << "-" << m_LightDirection << ".xyz, "
                                   << m_LightColor << ".rgb).rgb;" << Endl;

                    if (specularEnabled) {
                        if (m_LightsAsSeparateUniforms)
                            fragmentShader.AddUniform(m_LightSpecularColor, "vec4");
                        OutputSpecularEquation(Material().m_SpecularModel, fragmentShader,
                                               m_LightDirection.c_str(),
                                               m_LightSpecularColor.c_str());
                    }
                } else if (isArea) {
                    if (m_LightsAsSeparateUniforms) {
                        fragmentShader.AddUniform(m_LightColor, "vec4");
                        fragmentShader.AddUniform(m_LightPos, "vec4");
                        fragmentShader.AddUniform(m_LightDirection, "vec4");
                        fragmentShader.AddUniform(m_LightUp, "vec4");
                        fragmentShader.AddUniform(m_LightRt, "vec4");
                    } else {
                        addFunction(fragmentShader, "areaLightVars");
                    }
                    addFunction(fragmentShader, "calculateDiffuseAreaOld");
                    vertexShader.GenerateWorldPosition();
                    GenerateShadowMapOcclusion(lightIdx, enableShadowMaps && isShadow,
                                               lightNode->m_LightType);

                    // Debug measure to make sure paraboloid sampling was projecting to the right
                    // location
                    // fragmentShader << "    global_diffuse_light.rg += " << m_ShadowCoordStem << ";"
                    // << Endl;
                    m_NormalizedDirection = m_TempStr;
                    m_NormalizedDirection.append("_Frame");

                    AddLocalVariable(fragmentShader, m_NormalizedDirection, "mat3");
                    fragmentShader << m_NormalizedDirection << " = mat3( " << m_LightRt << ".xyz, "
                                   << m_LightUp << ".xyz, -" << m_LightDirection << ".xyz );"
                                   << Endl;

                    if (enableSSDO) {
                        fragmentShader << "    shadowFac = shadow_map_occl * customMaterialShadow( "
                                       << m_LightDirection << ".xyz, varWorldPos );" << Endl;
                    } else {
                        fragmentShader << "    shadowFac = shadow_map_occl;" << Endl;
                    }

                    if (specularEnabled) {
                        vertexShader.GenerateViewVector();
                        if (m_LightsAsSeparateUniforms)
                            fragmentShader.AddUniform(m_LightSpecularColor, "vec4");
                        OutputSpecularAreaLighting(fragmentShader, "varWorldPos", "view_vector",
                                                   m_LightSpecularColor.c_str());
                    }

                    OutputDiffuseAreaLighting(fragmentShader, "varWorldPos", m_TempStr);
                    fragmentShader << "    lightAttenuation *= shadowFac;" << Endl;

                    AddTranslucencyIrradiance(fragmentShader, translucencyImage, m_TempStr, true);

                    fragmentShader << "    global_diffuse_light.rgb += lightAttenuation * "
                                      "diffuseReflectionBSDF( world_normal, "
                                   << m_NormalizedDirection << ", " << m_LightColor
                                   << ".rgb).rgb;" << Endl;
                } else {

                    vertexShader.GenerateWorldPosition();
                    GenerateShadowMapOcclusion(lightIdx, enableShadowMaps && isShadow,
                                               lightNode->m_LightType);

                    if (m_LightsAsSeparateUniforms) {
                        fragmentShader.AddUniform(m_LightColor, "vec4");
                        fragmentShader.AddUniform(m_LightPos, "vec4");
                    }

                    m_RelativeDirection = m_TempStr;
                    m_RelativeDirection.append("_relativeDirection");

                    m_NormalizedDirection = m_RelativeDirection;
                    m_NormalizedDirection.append("_normalized");

                    m_RelativeDistance = m_TempStr;
                    m_RelativeDistance.append("_distance");

                    fragmentShader << "    vec3 " << m_RelativeDirection << " = varWorldPos - "
                                   << m_LightPos << ".xyz;" << Endl;
                    fragmentShader << "    float " << m_RelativeDistance << " = length( "
                                   << m_RelativeDirection << " );" << Endl;
                    fragmentShader << "    vec3 " << m_NormalizedDirection << " = "
                                   << m_RelativeDirection << " / " << m_RelativeDistance << ";"
                                   << Endl;

                    if (enableSSDO) {
                        fragmentShader << "    shadowFac = shadow_map_occl * customMaterialShadow( "
                                       << m_NormalizedDirection << ", varWorldPos );" << Endl;
                    } else {
                        fragmentShader << "    shadowFac = shadow_map_occl;" << Endl;
                    }

                    addFunction(fragmentShader, "calculatePointLightAttenuation");

                    if (m_LightsAsSeparateUniforms) {
                        fragmentShader.AddUniform(m_LightAttenuation, "vec3");
                        fragmentShader
                            << "    lightAttenuation = shadowFac * calculatePointLightAttenuation("
                            << "vec3( " << m_LightAttenuation << ".x, " << m_LightAttenuation
                            << ".y, " << m_LightAttenuation << ".z), " << m_RelativeDistance
                            << ");" << Endl;
                    } else {
                        fragmentShader
                            << "    lightAttenuation = shadowFac * calculatePointLightAttenuation("
                            << "vec3( " << m_LightConstantAttenuation << ", "
                            << m_LightLinearAttenuation << ", " << m_LightQuadraticAttenuation
                            << "), " << m_RelativeDistance << ");"
                            << Endl;
                    }



                    AddTranslucencyIrradiance(fragmentShader, translucencyImage, m_TempStr, false);

                    fragmentShader << "    global_diffuse_light.rgb += lightAttenuation * "
                                      "diffuseReflectionBSDF( world_normal, "
                                   << "-" << m_NormalizedDirection << ", "
                                   << m_LightColor << ".rgb).rgb;" << Endl;

                    if (specularEnabled) {
                        if (m_LightsAsSeparateUniforms)
                            fragmentShader.AddUniform(m_LightSpecularColor, "vec4");
                        OutputSpecularEquation(Material().m_SpecularModel, fragmentShader,
                                               m_NormalizedDirection.c_str(),
                                               m_LightSpecularColor.c_str());
                    }
                }
            }

            // This may be confusing but the light colors are already modulated by the base
            // material color.
            // Thus material color is the base material color * material emissive.
            // Except material_color.a *is* the actual opacity factor.
            // Furthermore object_opacity is something that may come from the vertex pipeline or
            // somewhere else.
            // We leave it up to the vertex pipeline to figure it out.
            fragmentShader << "    global_diffuse_light = vec4(global_diffuse_light.xyz * aoFactor, "
                              "object_opacity);"
                           << Endl << "    global_specular_light = vec3(global_specular_light.xyz);"
                           << Endl;
        } else // no lighting.
        {
            fragmentShader << "    vec4 global_diffuse_light = vec4(0.0, 0.0, 0.0, object_opacity);"
                           << Endl << "    vec3 global_specular_light = vec3(0.0, 0.0, 0.0);" << Endl;

            // We still have specular maps and such that could potentially use the fresnel variable.
            fragmentHasSpecularAmount =
                MaybeAddMaterialFresnel(fragmentShader, inKey, fragmentHasSpecularAmount);
        }

        if (!hasEmissiveMap)
            fragmentShader
                << "    global_diffuse_light.rgb += diffuse_color.rgb * material_diffuse.rgb;"
                << Endl;

        // since we already modulate our material diffuse color
        // into the light color we will miss it entirely if no IBL
        // or light is used
        if (hasLightmaps && !(m_Lights.size() || hasIblProbe))
            fragmentShader << "    global_diffuse_light.rgb *= diffuse_color.rgb;" << Endl;

        if (hasLighting && hasIblProbe) {
            vertexShader.GenerateWorldNormal();

            fragmentShader << "    global_diffuse_light.rgb += diffuse_color.rgb * aoFactor * "
                              "sampleDiffuse( tanFrame ).xyz;"
                           << Endl;

            if (specularEnabled) {

                fragmentShader.AddUniform("material_specular", "vec4");

                fragmentShader << "    global_specular_light.xyz += specularAmount * specularColor * "
                                  "vec3(material_specular.xyz) * sampleGlossy( tanFrame, "
                                  "view_vector, roughnessAmount ).xyz;"
                               << Endl;
            }
        }

        if (hasImage) {
            fragmentShader.Append("    vec4 texture_color;");
            QT3DSU32 idx = 0;
            for (SRenderableImage *image = m_FirstImage; image; image = image->m_NextImage, ++idx) {
                // Various maps are handled on a different locations
                if (image->m_MapType == ImageMapTypes::Bump
                    || image->m_MapType == ImageMapTypes::Normal
                    || image->m_MapType == ImageMapTypes::Displacement
                    || image->m_MapType == ImageMapTypes::SpecularAmountMap
                    || image->m_MapType == ImageMapTypes::Roughness
                    || image->m_MapType == ImageMapTypes::Translucency
                    || image->m_MapType == ImageMapTypes::LightmapIndirect
                    || image->m_MapType == ImageMapTypes::LightmapRadiosity) {
                    continue;
                }

                eastl::basic_string<char8_t> texSwizzle, lookupSwizzle, texLodStr;

                GenerateImageUVCoordinates(idx, *image, 0);

                GenerateTextureSwizzle(
                    image->m_Image.m_TextureData.m_Texture->GetTextureSwizzleMode(), texSwizzle,
                    lookupSwizzle);

                if (texLodStr.empty()) {
                    fragmentShader << "    texture_color" << texSwizzle.c_str() << " = texture2D( "
                                   << m_ImageSampler << ", " << m_ImageFragCoords << ")"
                                   << lookupSwizzle.c_str() << ";" << Endl;
                } else {
                    fragmentShader << "    texture_color" << texSwizzle.c_str() << "= textureLod( "
                                   << m_ImageSampler << ", " << m_ImageFragCoords << ", "
                                   << texLodStr.c_str() << " )" << lookupSwizzle.c_str() << ";"
                                   << Endl;
                }

                if (image->m_Image.m_TextureData.m_TextureFlags.IsPreMultiplied() == true)
                    fragmentShader << "    texture_color.rgb = texture_color.a > 0.0 ? "
                                      "texture_color.rgb / texture_color.a : vec3( 0, 0, 0 );"
                                   << Endl;

                // These mapping types honestly don't make a whole ton of sense to me.
                switch (image->m_MapType) {
                case ImageMapTypes::Diffuse: // assume images are premultiplied.
                case ImageMapTypes::LightmapShadow:
                    // We use image offsets.z to switch between incoming premultiplied textures or
                    // not premultiplied textures.
                    // If Z is 1, then we assume the incoming texture is already premultiplied, else
                    // we just read the rgb value.
                    fragmentShader.Append("    global_diffuse_light *= texture_color;");
                    break;
                case ImageMapTypes::Specular:
                    fragmentShader.AddUniform("material_specular", "vec4");
                    if (image->m_Image.m_TextureData.m_Texture->GetTextureDetails().m_Format != NVRenderTextureFormats::RGBE8) {
                        if (fragmentHasSpecularAmount) {
                            fragmentShader.Append("    global_specular_light.xyz += specularAmount * "
                                                  "specularColor * texture_color.xyz * "
                                                  "material_specular.xyz;");
                        } else {
                            fragmentShader.Append("    global_specular_light.xyz += texture_color.xyz * "
                                                  "material_specular.xyz;");
                        }
                        fragmentShader.Append("    global_diffuse_light.a *= texture_color.a;");
                    } else {
                        // RGBE map
                        if (fragmentHasSpecularAmount) {
                            fragmentShader.Append("    global_specular_light.xyz += specularAmount * "
                                                  "specularColor * texture_color.xyz * pow(2.0, texture_color.a * 255.0 - 128.0);"
                                                  "material_specular.xyz;");
                        } else {
                            fragmentShader.Append("    global_specular_light.xyz += texture_color.xyz * "
                                                  "material_specular.xyz * pow(2.0, texture_color.a * 255.0 - 128.0);");
                        }
                    }
                    break;
                case ImageMapTypes::Opacity:
                    fragmentShader.Append("    global_diffuse_light.a *= texture_color.a;");
                    break;
                case ImageMapTypes::Emissive:
                    fragmentShader.Append(
                        "    global_emission *= texture_color.xyz * texture_color.a;");
                    break;
                default:
                    QT3DS_ASSERT(false); // fallthrough intentional
                }
            }
        }

        // Alpha test
        if (enableAlphaTest) {
            fragmentShader.AddUniform("alphaOpRef", "vec2");
            fragmentShader.Append(
                        "    if (dot(vec2(global_diffuse_light.a, 1.0), alphaOpRef) <= 0.0)\n"
                        "        discard;");
        }

        if (hasEmissiveMap) {
            fragmentShader.Append("    global_diffuse_light.rgb += global_emission.rgb;");
        }

        // Ensure the rgb colors are in range.
        fragmentShader.Append("    fragOutput = vec4( clamp( vertColor * global_diffuse_light.xyz + "
                              "global_specular_light.xyz, 0.0, 65519.0 ), global_diffuse_light.a "
                              ");");

        if (VertexGenerator().HasActiveWireframe()) {
            fragmentShader.Append("vec3 edgeDistance = varEdgeDistance * gl_FragCoord.w;");
            fragmentShader.Append(
                "    float d = min(min(edgeDistance.x, edgeDistance.y), edgeDistance.z);");
            fragmentShader.Append("    float mixVal = smoothstep(0.0, 1.0, d);"); // line width 1.0

            fragmentShader.Append(
                "    fragOutput = mix( vec4(0.0, 1.0, 0.0, 1.0), fragOutput, mixVal);");
        }
    }

    // generate default material shader for depth only pass
    NVRenderShaderProgram *GenerateDepthPassShader(
        const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
        IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
        SRenderableImage *inFirstImage, bool inHasTransparency,
        const char8_t *inVertexPipelineName, const char8_t *inCustomMaterialName = "") override
    {
        QT3DS_ASSERT(inMaterial.m_Type == GraphObjectTypes::DefaultMaterial);
        m_CurrentMaterial = static_cast<const SDefaultMaterial *>(&inMaterial);
        m_CurrentKey = &inShaderDescription;
        m_CurrentPipeline = static_cast<IDefaultMaterialVertexPipeline *>(&inVertexPipeline);
        m_CurrentFeatureSet = inFeatureSet;
        m_FirstImage = inFirstImage;
        m_HasTransparency = inHasTransparency;

        ProgramGenerator().BeginProgram();

        bool enableAlphaTest = m_DefaultMaterialShaderKeyProperties
                .m_AlphaTestEnabled.GetValue(inShaderDescription);

        QT3DS_ASSERT(m_FirstImage != nullptr);
        if (!enableAlphaTest)
            return nullptr;
        QT3DS_ASSERT(enableAlphaTest);

        m_GeneratedShaderString.clear();
        m_GeneratedShaderString.assign(nonNull(inVertexPipelineName));
        m_GeneratedShaderString.append("depth--");

        SShaderDefaultMaterialKey theKey(Key());
        theKey.ToString(m_GeneratedShaderString, m_DefaultMaterialShaderKeyProperties,
                        SShaderDefaultMaterialKeyProperties::DepthKey);

        m_LightsAsSeparateUniforms = !m_RenderContext.GetRenderContext().GetConstantBufferSupport();

        VertexGenerator().BeginFragmentGeneration();
        IShaderStageGenerator &fragmentShader(FragmentGenerator());
        IDefaultMaterialVertexPipeline &vertexShader(VertexGenerator());

        vertexShader.AddIncoming("attr_pos", "vec3");
        vertexShader.AddUniform("model_view_projection", "mat4");
        vertexShader.Append("void main() {");
        vertexShader.Append("    gl_Position = model_view_projection * vec4( attr_pos, 1.0 );");
        vertexShader.Append("    vec3 uTransform;");
        vertexShader.Append("    vec3 vTransform;");

        fragmentShader << "    float global_opacity = object_opacity;" << Endl;

        fragmentShader.Append("    vec4 texture_color;");
        QT3DSU32 idx = 0;
        for (SRenderableImage *image = m_FirstImage; image; image = image->m_NextImage, ++idx) {
            // Various maps are handled on a different locations
            if (image->m_MapType == ImageMapTypes::Bump
                || image->m_MapType == ImageMapTypes::Normal
                || image->m_MapType == ImageMapTypes::Displacement
                || image->m_MapType == ImageMapTypes::SpecularAmountMap
                || image->m_MapType == ImageMapTypes::Roughness
                || image->m_MapType == ImageMapTypes::Translucency
                || image->m_MapType == ImageMapTypes::LightmapIndirect
                || image->m_MapType == ImageMapTypes::LightmapRadiosity
                || image->m_MapType == ImageMapTypes::Emissive) {
                continue;
            }

            eastl::basic_string<char8_t> texSwizzle, lookupSwizzle, texLodStr;

            GenerateImageUVCoordinates(idx, *image, 0);

            GenerateTextureSwizzle(
                image->m_Image.m_TextureData.m_Texture->GetTextureSwizzleMode(), texSwizzle,
                lookupSwizzle);

            if (texLodStr.empty()) {
                fragmentShader << "    texture_color" << texSwizzle.c_str() << " = texture2D( "
                               << m_ImageSampler << ", " << m_ImageFragCoords << ")"
                               << lookupSwizzle.c_str() << ";" << Endl;
            } else {
                fragmentShader << "    texture_color" << texSwizzle.c_str() << "= textureLod( "
                               << m_ImageSampler << ", " << m_ImageFragCoords << ", "
                               << texLodStr.c_str() << " )" << lookupSwizzle.c_str() << ";"
                               << Endl;
            }

            switch (image->m_MapType) {
            case ImageMapTypes::Diffuse: // assume images are premultiplied.
            case ImageMapTypes::LightmapShadow:
            case ImageMapTypes::Specular:
            case ImageMapTypes::Opacity:
                fragmentShader.Append("    global_opacity *= texture_color.a;");
                break;
            default:
                break;
            }
        }

        fragmentShader.AddUniform("alphaOpRef", "vec2");
        fragmentShader.Append("    if (dot(vec2(global_opacity, 1.0), alphaOpRef) <= 0.0)\n"
                              "        discard;");
        fragmentShader.Append("    fragOutput = vec4(0.0);");
        fragmentShader.Append("}");
        vertexShader.Append("}");

        return ProgramGenerator().CompileGeneratedShader(m_GeneratedShaderString.c_str(),
                                                         SShaderCacheProgramFlags(), FeatureSet());

    }

    // generate default material shader cube depth shader
    NVRenderShaderProgram *GenerateCubeDepthShader(
        const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
        IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
        SRenderableImage *inFirstImage, bool inHasTransparency,
        const char8_t *inVertexPipelineName, const char8_t *inCustomMaterialName = "") override
    {
        QT3DS_ASSERT(inMaterial.m_Type == GraphObjectTypes::DefaultMaterial);
        m_CurrentMaterial = static_cast<const SDefaultMaterial *>(&inMaterial);
        m_CurrentKey = &inShaderDescription;
        m_CurrentPipeline = static_cast<IDefaultMaterialVertexPipeline *>(&inVertexPipeline);
        m_CurrentFeatureSet = inFeatureSet;
        m_FirstImage = inFirstImage;
        m_HasTransparency = inHasTransparency;

        ProgramGenerator().BeginProgram();

        bool enableAlphaTest = m_DefaultMaterialShaderKeyProperties
                .m_AlphaTestEnabled.GetValue(inShaderDescription);

        QT3DS_ASSERT(m_FirstImage != nullptr);
        if (!enableAlphaTest)
            return nullptr;
        QT3DS_ASSERT(enableAlphaTest);

        m_GeneratedShaderString.clear();
        m_GeneratedShaderString.assign(nonNull(inVertexPipelineName));
        m_GeneratedShaderString.append("shadowcube--");

        SShaderDefaultMaterialKey theKey(Key());
        theKey.ToString(m_GeneratedShaderString, m_DefaultMaterialShaderKeyProperties,
                        SShaderDefaultMaterialKeyProperties::DepthKey);

        m_LightsAsSeparateUniforms = !m_RenderContext.GetRenderContext().GetConstantBufferSupport();

        VertexGenerator().BeginFragmentGeneration();
        IShaderStageGenerator &fragmentShader(FragmentGenerator());
        IDefaultMaterialVertexPipeline &vertexShader(VertexGenerator());

        vertexShader.AddIncoming("attr_pos", "vec3");
        vertexShader.AddUniform("model_matrix", "mat4");
        vertexShader.AddUniform("model_view_projection", "mat4");
        vertexShader.AddOutgoing("raw_pos", "vec4");
        vertexShader.AddOutgoing("world_pos", "vec4");

        vertexShader.Append("void main() {");
        vertexShader.Append("    vec3 uTransform;");
        vertexShader.Append("    vec3 vTransform;");
        vertexShader.Append("    world_pos = model_matrix * vec4( attr_pos, 1.0 );");
        vertexShader.Append("    world_pos /= world_pos.w;");
        vertexShader.Append("    gl_Position = model_view_projection * vec4( attr_pos, 1.0 );");
        vertexShader.Append("    raw_pos = vec4( attr_pos, 1.0 );");

        fragmentShader << "    float global_opacity = object_opacity;" << Endl;

        fragmentShader.Append("    vec4 texture_color;");
        QT3DSU32 idx = 0;
        for (SRenderableImage *image = m_FirstImage; image; image = image->m_NextImage, ++idx) {
            // Various maps are handled on different locations
            if (image->m_MapType == ImageMapTypes::Bump
                || image->m_MapType == ImageMapTypes::Normal
                || image->m_MapType == ImageMapTypes::Displacement
                || image->m_MapType == ImageMapTypes::SpecularAmountMap
                || image->m_MapType == ImageMapTypes::Roughness
                || image->m_MapType == ImageMapTypes::Translucency
                || image->m_MapType == ImageMapTypes::LightmapIndirect
                || image->m_MapType == ImageMapTypes::LightmapRadiosity
                || image->m_MapType == ImageMapTypes::Emissive) {
                continue;
            }

            eastl::basic_string<char8_t> texSwizzle, lookupSwizzle, texLodStr;

            GenerateImageUVCoordinates(idx, *image, 0);

            GenerateTextureSwizzle(
                image->m_Image.m_TextureData.m_Texture->GetTextureSwizzleMode(), texSwizzle,
                lookupSwizzle);

            if (texLodStr.empty()) {
                fragmentShader << "    texture_color" << texSwizzle.c_str() << " = texture2D( "
                               << m_ImageSampler << ", " << m_ImageFragCoords << ")"
                               << lookupSwizzle.c_str() << ";" << Endl;
            } else {
                fragmentShader << "    texture_color" << texSwizzle.c_str() << "= textureLod( "
                               << m_ImageSampler << ", " << m_ImageFragCoords << ", "
                               << texLodStr.c_str() << " )" << lookupSwizzle.c_str() << ";"
                               << Endl;
            }

            switch (image->m_MapType) {
            case ImageMapTypes::Diffuse: // assume images are premultiplied.
            case ImageMapTypes::LightmapShadow:
            case ImageMapTypes::Specular:
            case ImageMapTypes::Opacity:
                fragmentShader.Append("    global_opacity *= texture_color.a;");
                break;
            default:
                break;
            }
        }

        fragmentShader.AddUniform("camera_position", "vec3");
        fragmentShader.AddUniform("camera_properties", "vec2");
        fragmentShader.Append("    vec3 camPos = vec3(camera_position.x, camera_position.y, "
                              "-camera_position.z);");
        fragmentShader.Append("    float dist = length(world_pos.xyz - camPos);");
        fragmentShader.Append("    dist = max((dist - 1.0) / (camera_properties.y - 1.0), 0.0);");
        fragmentShader.AddUniform("alphaOpRef", "vec2");
        fragmentShader.Append("    if (dot(vec2(global_opacity, 1.0), alphaOpRef) <= 0.0)\n"
                              "        dist = 1.0;");
        fragmentShader.Append("    fragOutput = vec4(dist);");
        fragmentShader.Append("}");

        vertexShader.Append("}");

        return ProgramGenerator().CompileGeneratedShader(m_GeneratedShaderString.c_str(),
                                                         SShaderCacheProgramFlags(), FeatureSet());
    }

    // generate default material shader orthographic depth shader
    NVRenderShaderProgram *GenerateOrthoDepthShader(
        const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
        IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
        SRenderableImage *inFirstImage, bool inHasTransparency,
        const char8_t *inVertexPipelineName, const char8_t *inCustomMaterialName = "") override
    {
        QT3DS_ASSERT(inMaterial.m_Type == GraphObjectTypes::DefaultMaterial);
        m_CurrentMaterial = static_cast<const SDefaultMaterial *>(&inMaterial);
        m_CurrentKey = &inShaderDescription;
        m_CurrentPipeline = static_cast<IDefaultMaterialVertexPipeline *>(&inVertexPipeline);
        m_CurrentFeatureSet = inFeatureSet;
        m_FirstImage = inFirstImage;
        m_HasTransparency = inHasTransparency;

        ProgramGenerator().BeginProgram();

        bool enableAlphaTest = m_DefaultMaterialShaderKeyProperties
                .m_AlphaTestEnabled.GetValue(inShaderDescription);

        QT3DS_ASSERT(m_FirstImage != nullptr);
        if (!enableAlphaTest)
            return nullptr;
        QT3DS_ASSERT(enableAlphaTest);

        m_GeneratedShaderString.clear();
        m_GeneratedShaderString.assign(nonNull(inVertexPipelineName));
        m_GeneratedShaderString.append("shadowmap--");

        SShaderDefaultMaterialKey theKey(Key());
        theKey.ToString(m_GeneratedShaderString, m_DefaultMaterialShaderKeyProperties,
                        SShaderDefaultMaterialKeyProperties::DepthKey);

        m_LightsAsSeparateUniforms = !m_RenderContext.GetRenderContext().GetConstantBufferSupport();

        VertexGenerator().BeginFragmentGeneration();
        IShaderStageGenerator &fragmentShader(FragmentGenerator());
        IDefaultMaterialVertexPipeline &vertexShader(VertexGenerator());

        vertexShader.AddIncoming("attr_pos", "vec3");
        vertexShader.AddUniform("model_view_projection", "mat4");
        vertexShader.AddOutgoing("outDepth", "vec3");

        vertexShader.Append("void main() {");
        vertexShader.Append("    vec3 uTransform;");
        vertexShader.Append("    vec3 vTransform;");
        vertexShader.Append("    gl_Position = model_view_projection * vec4( attr_pos, 1.0 );");
        vertexShader.Append("    outDepth.x = gl_Position.z / gl_Position.w;");

        fragmentShader << "    float global_opacity = object_opacity;" << Endl;

        fragmentShader.Append("    vec4 texture_color;");
        QT3DSU32 idx = 0;
        for (SRenderableImage *image = m_FirstImage; image; image = image->m_NextImage, ++idx) {
            // Various maps are handled on different locations
            if (image->m_MapType == ImageMapTypes::Bump
                || image->m_MapType == ImageMapTypes::Normal
                || image->m_MapType == ImageMapTypes::Displacement
                || image->m_MapType == ImageMapTypes::SpecularAmountMap
                || image->m_MapType == ImageMapTypes::Roughness
                || image->m_MapType == ImageMapTypes::Translucency
                || image->m_MapType == ImageMapTypes::LightmapIndirect
                || image->m_MapType == ImageMapTypes::LightmapRadiosity
                || image->m_MapType == ImageMapTypes::Emissive) {
                continue;
            }

            eastl::basic_string<char8_t> texSwizzle, lookupSwizzle, texLodStr;

            GenerateImageUVCoordinates(idx, *image, 0);

            GenerateTextureSwizzle(
                image->m_Image.m_TextureData.m_Texture->GetTextureSwizzleMode(), texSwizzle,
                lookupSwizzle);

            if (texLodStr.empty()) {
                fragmentShader << "    texture_color" << texSwizzle.c_str() << " = texture2D( "
                               << m_ImageSampler << ", " << m_ImageFragCoords << ")"
                               << lookupSwizzle.c_str() << ";" << Endl;
            } else {
                fragmentShader << "    texture_color" << texSwizzle.c_str() << "= textureLod( "
                               << m_ImageSampler << ", " << m_ImageFragCoords << ", "
                               << texLodStr.c_str() << " )" << lookupSwizzle.c_str() << ";"
                               << Endl;
            }

            switch (image->m_MapType) {
            case ImageMapTypes::Diffuse: // assume images are premultiplied.
            case ImageMapTypes::LightmapShadow:
            case ImageMapTypes::Specular:
            case ImageMapTypes::Opacity:
                fragmentShader.Append("    global_opacity *= texture_color.a;");
                break;
            default:
                break;
            }
        }

        fragmentShader.AddUniform("alphaOpRef", "vec2");
        fragmentShader.Append("    float depth = (outDepth.x + 1.0) * 0.5;");
        fragmentShader.Append("    if (dot(vec2(global_opacity, 1.0), alphaOpRef) <= 0.0)\n"
                              "        depth = 1.0;");
        fragmentShader.Append("    fragOutput = vec4(depth);");
        fragmentShader.Append("}");
        vertexShader.Append("}");

        return ProgramGenerator().CompileGeneratedShader(m_GeneratedShaderString.c_str(),
                                                         SShaderCacheProgramFlags(), FeatureSet());
    }

    NVRenderShaderProgram *GenerateMaterialShader(const char8_t *inShaderPrefix)
    {
        // build a string that allows us to print out the shader we are generating to the log.
        // This is time consuming but I feel like it doesn't happen all that often and is very
        // useful to users
        // looking at the log file.

        m_GeneratedShaderString.clear();
        m_GeneratedShaderString.assign(nonNull(inShaderPrefix));

        SShaderDefaultMaterialKey theKey(Key());
        theKey.ToString(m_GeneratedShaderString, m_DefaultMaterialShaderKeyProperties,
                        SShaderDefaultMaterialKeyProperties::DefaultKey);

        m_LightsAsSeparateUniforms = !m_RenderContext.GetRenderContext().GetConstantBufferSupport();

        GenerateVertexShader();
        GenerateFragmentShader(theKey);

        VertexGenerator().EndVertexGeneration(false);
        VertexGenerator().EndFragmentGeneration(false);

        return ProgramGenerator().CompileGeneratedShader(m_GeneratedShaderString.c_str(),
                                                         SShaderCacheProgramFlags(), FeatureSet());
    }

    virtual NVRenderShaderProgram *
    GenerateShader(const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
                   IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
                   NVDataRef<SLight *> inLights, SRenderableImage *inFirstImage,
                   bool inHasTransparency, const char8_t *inVertexPipelineName,
                   const char8_t *) override
    {
        QString error;
        return GenerateShader(inMaterial, inShaderDescription, inVertexPipeline, inFeatureSet,
                              inLights, inFirstImage, inHasTransparency, inVertexPipelineName,
                              error, "");
    }

    virtual NVRenderShaderProgram *
    GenerateShader(const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
                   IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
                   NVDataRef<SLight *> inLights, SRenderableImage *inFirstImage,
                   bool inHasTransparency, const char8_t *inVertexPipelineName, QString &error,
                   const char8_t *) override
    {
        Q_UNUSED(error)
        QT3DS_ASSERT(inMaterial.m_Type == GraphObjectTypes::DefaultMaterial);
        m_CurrentMaterial = static_cast<const SDefaultMaterial *>(&inMaterial);
        m_CurrentKey = &inShaderDescription;
        m_CurrentPipeline = static_cast<IDefaultMaterialVertexPipeline *>(&inVertexPipeline);
        m_CurrentFeatureSet = inFeatureSet;
        m_Lights = inLights;
        m_FirstImage = inFirstImage;
        m_HasTransparency = inHasTransparency;

        return GenerateMaterialShader(inVertexPipelineName);
    }

    SShaderGeneratorGeneratedShader &GetShaderForProgram(NVRenderShaderProgram &inProgram)
    {
        eastl::pair<TProgramToShaderMap::iterator, bool> inserter =
            m_ProgramToShaderMap.insert(eastl::make_pair(
                &inProgram, NVScopedRefCounted<SShaderGeneratorGeneratedShader>(nullptr)));
        if (inserter.second) {
            NVAllocatorCallback &alloc(m_RenderContext.GetRenderContext().GetAllocator());
            inserter.first->second = QT3DS_NEW(alloc, SShaderGeneratorGeneratedShader)(
                inProgram, m_RenderContext.GetRenderContext());
        }
        return *inserter.first->second;
    }

    void SetGlobalProperties(NVRenderShaderProgram &inProgram, const SLayer & /*inLayer*/
                             ,
                             SCamera &inCamera, QT3DSVec3 inCameraDirection,
                             NVDataRef<SLight *> inLights, NVDataRef<QT3DSVec3> inLightDirections,
                             Qt3DSShadowMap *inShadowMapManager)
    {
        SShaderGeneratorGeneratedShader &shader(GetShaderForProgram(inProgram));
        m_RenderContext.GetRenderContext().SetActiveShader(&inProgram);

        m_ShadowMapManager = inShadowMapManager;

        SCamera &theCamera(inCamera);
        shader.m_CameraPosition.Set(theCamera.GetGlobalPos());
        shader.m_CameraDirection.Set(inCameraDirection);

        QT3DSMat44 viewProj;
        if (shader.m_ViewProj.IsValid()) {
            theCamera.CalculateViewProjectionMatrix(viewProj);
            shader.m_ViewProj.Set(viewProj);
        }

        if (shader.m_ViewMatrix.IsValid()) {
            viewProj = theCamera.m_GlobalTransform.getInverse();
            shader.m_ViewMatrix.Set(viewProj);
        }

        // update the constant buffer
        shader.m_AoShadowParams.Set();
        // We can't cache light properties because they can change per object.
        QT3DSVec4 theLightAmbientTotal = QT3DSVec4(0, 0, 0, 1);
        size_t numShaderLights = shader.m_Lights.size();
        size_t numShadowLights = shader.m_ShadowMaps.size();
        for (QT3DSU32 lightIdx = 0, shadowMapIdx = 0, lightEnd = inLights.size();
             lightIdx < lightEnd && lightIdx < QT3DS_MAX_NUM_LIGHTS; ++lightIdx) {
            SLight *theLight(inLights[lightIdx]);
            if (lightIdx >= numShaderLights) {
                shader.m_Lights.push_back(SShaderLightProperties());
                ++numShaderLights;
            }
            if (shadowMapIdx >= numShadowLights && numShadowLights < QT3DS_MAX_NUM_SHADOWS) {
                if (theLight->m_Scope == NULL && theLight->m_CastShadow) {
                    // PKC TODO : Fix multiple shadow issues.
                    // Need to know when the list of lights changes order, and clear shadow maps
                    // when that happens.
                    SetupShadowMapVariableNames(lightIdx);
                    shader.m_ShadowMaps.push_back(SShadowMapProperties(
                        m_ShadowMapStem.c_str(), m_ShadowCubeStem.c_str(),
                        m_ShadowMatrixStem.c_str(), m_ShadowControlStem.c_str(), inProgram));
                }
            }
            QT3DS_ASSERT(lightIdx < numShaderLights);
            SShaderLightProperties &theLightProperties(shader.m_Lights[lightIdx]);
            float brightness = TranslateConstantAttenuation(theLight->m_Brightness);

            // setup light data
            theLightProperties.m_LightColor =
                QT3DSVec4(theLight->m_DiffuseColor.getXYZ() * brightness, 1.0);
            theLightProperties.m_LightData.m_specular =
                QT3DSVec4(theLight->m_SpecularColor.getXYZ() * brightness, 1.0);
            theLightProperties.m_LightData.m_direction = QT3DSVec4(inLightDirections[lightIdx], 1.0);

            // TODO : This does potentially mean that we can create more shadow map entries than
            // we can actually use at once.
            if ((theLight->m_Scope == NULL) && (theLight->m_CastShadow && inShadowMapManager)) {
                SShadowMapProperties &theShadowMapProperties(shader.m_ShadowMaps[shadowMapIdx++]);
                SShadowMapEntry *pEntry = inShadowMapManager->GetShadowMapEntry(lightIdx);
                if (pEntry) {
                    // add fixed scale bias matrix
                    QT3DSMat44 bias(QT3DSVec4(0.5, 0.0, 0.0, 0.0), QT3DSVec4(0.0, 0.5, 0.0, 0.0),
                                 QT3DSVec4(0.0, 0.0, 0.5, 0.0), QT3DSVec4(0.5, 0.5, 0.5, 1.0));

                    if (theLight->m_LightType != RenderLightTypes::Directional) {
                        theShadowMapProperties.m_ShadowCubeTexture.Set(pEntry->m_DepthCube);
                        theShadowMapProperties.m_ShadowmapMatrix.Set(pEntry->m_LightView);
                    } else {
                        theShadowMapProperties.m_ShadowmapTexture.Set(pEntry->m_DepthMap);
                        theShadowMapProperties.m_ShadowmapMatrix.Set(bias * pEntry->m_LightVP);
                    }

                    theShadowMapProperties.m_ShadowmapSettings.Set(
                        QT3DSVec4(theLight->m_ShadowBias, theLight->m_ShadowFactor,
                               theLight->m_ShadowMapFar, 0.0f));
                } else {
                    // if we have a light casting shadow we should find an entry
                    QT3DS_ASSERT(false);
                }
            }

            if (theLight->m_LightType == RenderLightTypes::Point) {
                theLightProperties.m_LightData.m_position = QT3DSVec4(theLight->GetGlobalPos(), 1.0);
                theLightProperties.m_LightData.m_constantAttenuation = 1.0;
                theLightProperties.m_LightData.m_linearAttenuation =
                    TranslateLinearAttenuation(theLight->m_LinearFade);
                theLightProperties.m_LightData.m_quadraticAttenuation =
                    TranslateQuadraticAttenuation(theLight->m_ExponentialFade);
            } else if (theLight->m_LightType == RenderLightTypes::Area) {
                theLightProperties.m_LightData.m_position = QT3DSVec4(theLight->GetGlobalPos(), 1.0);

                QT3DSVec3 upDir = theLight->m_GlobalTransform.getUpper3x3().transform(QT3DSVec3(0, 1, 0));
                QT3DSVec3 rtDir = theLight->m_GlobalTransform.getUpper3x3().transform(QT3DSVec3(1, 0, 0));

                theLightProperties.m_LightData.m_up = QT3DSVec4(upDir, theLight->m_AreaHeight);
                theLightProperties.m_LightData.m_right = QT3DSVec4(rtDir, theLight->m_AreaWidth);
            }
            theLightAmbientTotal += theLight->m_AmbientColor;
        }
        shader.m_LightAmbientTotal = theLightAmbientTotal.getXYZ();
    }

    // Also sets the blend function on the render context.
    void SetMaterialProperties(NVRenderShaderProgram &inProgram, const SDefaultMaterial &inMaterial,
                               const QT3DSVec2 &inCameraVec, const QT3DSMat44 &inModelViewProjection,
                               const QT3DSMat33 &inNormalMatrix, const QT3DSMat44 &inGlobalTransform,
                               SRenderableImage *inFirstImage, QT3DSF32 inOpacity,
                               NVRenderTexture2D *inDepthTexture, NVRenderTexture2D *inSSaoTexture,
                               SImage *inLightProbe, SImage *inLightProbe2, QT3DSF32 inProbeHorizon,
                               QT3DSF32 inProbeBright, QT3DSF32 inProbe2Window, QT3DSF32 inProbe2Pos,
                               QT3DSF32 inProbe2Fade, QT3DSF32 inProbeFOV,
                               const QT3DSVec2 &alphaOpRef)
    {

        NVRenderContext &context(m_RenderContext.GetRenderContext());
        SShaderGeneratorGeneratedShader &shader(GetShaderForProgram(inProgram));
        shader.m_MVP.Set(inModelViewProjection);
        shader.m_NormalMatrix.Set(inNormalMatrix);
        shader.m_GlobalTransform.Set(inGlobalTransform);
        shader.m_DepthTexture.Set(inDepthTexture);

        shader.m_AOTexture.Set(inSSaoTexture);

        qt3ds::render::SImage *theLightProbe = inLightProbe;
        qt3ds::render::SImage *theLightProbe2 = inLightProbe2;

        // If the material has its own IBL Override, we should use that image instead.
        if ((inMaterial.m_IblProbe) && (inMaterial.m_IblProbe->m_TextureData.m_Texture)) {
            theLightProbe = inMaterial.m_IblProbe;
        }

        if (theLightProbe) {
            if (theLightProbe->m_TextureData.m_Texture) {
                NVRenderTextureCoordOp::Enum theHorzLightProbeTilingMode =
                    theLightProbe->m_HorizontalTilingMode;
                NVRenderTextureCoordOp::Enum theVertLightProbeTilingMode =
                    theLightProbe->m_VerticalTilingMode;
                theLightProbe->m_TextureData.m_Texture->SetTextureWrapS(
                    theHorzLightProbeTilingMode);
                theLightProbe->m_TextureData.m_Texture->SetTextureWrapT(
                    theVertLightProbeTilingMode);
                const QT3DSMat44 &textureTransform = theLightProbe->m_TextureTransform;
                // We separate rotational information from offset information so that just maybe the
                // shader
                // will attempt to push less information to the card.
                const QT3DSF32 *dataPtr(textureTransform.front());
                // The third member of the offsets contains a flag indicating if the texture was
                // premultiplied or not.
                // We use this to mix the texture alpha.
                QT3DSVec4 offsets(dataPtr[12], dataPtr[13],
                               theLightProbe->m_TextureData.m_TextureFlags.IsPreMultiplied() ? 1.0f
                                                                                             : 0.0f,
                               (float)theLightProbe->m_TextureData.m_Texture->GetNumMipmaps());

                // Grab just the upper 2x2 rotation matrix from the larger matrix.
                QT3DSVec4 rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

                shader.m_LightProbeRot.Set(rotations);
                shader.m_LightProbeOfs.Set(offsets);

                if ((!inMaterial.m_IblProbe) && (inProbeFOV < 180.f)) {
                    shader.m_LightProbeOpts.Set(
                        QT3DSVec4(0.01745329251994329547f * inProbeFOV, 0.0f, 0.0f, 0.0f));
                }

                // Also make sure to add the secondary texture, but it should only be added if the
                // primary
                // (i.e. background) texture is also there.
                if (theLightProbe2 && theLightProbe2->m_TextureData.m_Texture) {
                    theLightProbe2->m_TextureData.m_Texture->SetTextureWrapS(
                        theHorzLightProbeTilingMode);
                    theLightProbe2->m_TextureData.m_Texture->SetTextureWrapT(
                        theVertLightProbeTilingMode);
                    shader.m_LightProbe2.Set(theLightProbe2->m_TextureData.m_Texture);
                    shader.m_LightProbe2Props.Set(
                        QT3DSVec4(inProbe2Window, inProbe2Pos, inProbe2Fade, 1.0f));

                    const QT3DSMat44 &xform2 = theLightProbe2->m_TextureTransform;
                    const QT3DSF32 *dataPtr(xform2.front());
                    shader.m_LightProbeProps.Set(
                        QT3DSVec4(dataPtr[12], dataPtr[13], inProbeHorizon, inProbeBright * 0.01f));
                } else {
                    shader.m_LightProbe2Props.Set(QT3DSVec4(0.0f, 0.0f, 0.0f, 0.0f));
                    shader.m_LightProbeProps.Set(
                        QT3DSVec4(0.0f, 0.0f, inProbeHorizon, inProbeBright * 0.01f));
                }
                NVRenderTexture2D *textureImage = theLightProbe->m_TextureData.m_Texture;
                shader.m_LightProbe.Set(textureImage);
                shader.m_LightProbeSize.Set(QT3DSVec2(textureImage->GetTextureDetails().m_Width,
                                                      textureImage->GetTextureDetails().m_Height));
            } else {
                shader.m_LightProbeProps.Set(QT3DSVec4(0.0f, 0.0f, -1.0f, 0.0f));
                shader.m_LightProbe2Props.Set(QT3DSVec4(0.0f, 0.0f, 0.0f, 0.0f));
            }
        } else {
            shader.m_LightProbeProps.Set(QT3DSVec4(0.0f, 0.0f, -1.0f, 0.0f));
            shader.m_LightProbe2Props.Set(QT3DSVec4(0.0f, 0.0f, 0.0f, 0.0f));
        }

        QT3DSF32 emissivePower = 1.0;

        QT3DSU32 hasLighting = inMaterial.m_Lighting != DefaultMaterialLighting::NoLighting;
        if (hasLighting)
            emissivePower = inMaterial.m_EmissivePower / 100.0f;

        QT3DSVec4 material_diffuse = QT3DSVec4(inMaterial.m_EmissiveColor[0] * emissivePower,
                                         inMaterial.m_EmissiveColor[1] * emissivePower,
                                         inMaterial.m_EmissiveColor[2] * emissivePower, inOpacity);
        shader.m_MaterialDiffuse.Set(material_diffuse);
        shader.m_DiffuseColor.Set(inMaterial.m_DiffuseColor.getXYZ());
        QT3DSVec4 material_specular =
            QT3DSVec4(inMaterial.m_SpecularTint[0], inMaterial.m_SpecularTint[1],
                   inMaterial.m_SpecularTint[2], inMaterial.m_IOR);
        shader.m_MaterialSpecular.Set(material_specular);
        shader.m_CameraProperties.Set(inCameraVec);
        shader.m_FresnelPower.Set(inMaterial.m_FresnelPower);
        shader.m_alphaTestOp.Set(alphaOpRef);

        if (context.GetConstantBufferSupport()) {
            NVRenderConstantBuffer *pLightCb = GetLightConstantBuffer(shader.m_Lights.size());
            // if we have lights we need a light buffer
            QT3DS_ASSERT(shader.m_Lights.size() == 0 || pLightCb);

            for (QT3DSU32 idx = 0, end = shader.m_Lights.size(); idx < end && pLightCb; ++idx) {
                shader.m_Lights[idx].m_LightData.m_diffuse =
                    QT3DSVec4(shader.m_Lights[idx].m_LightColor.x * inMaterial.m_DiffuseColor.x,
                           shader.m_Lights[idx].m_LightColor.y * inMaterial.m_DiffuseColor.y,
                           shader.m_Lights[idx].m_LightColor.z * inMaterial.m_DiffuseColor.z, 1.0);

                // this is our final change update memory
                pLightCb->UpdateRaw(idx * sizeof(SLightSourceShader) + (4 * sizeof(QT3DSI32)),
                                    NVDataRef<QT3DSU8>((QT3DSU8 *)&shader.m_Lights[idx].m_LightData,
                                                    sizeof(SLightSourceShader)));
            }
            // update light buffer to hardware
            if (pLightCb) {
                QT3DSI32 cgLights = shader.m_Lights.size();
                pLightCb->UpdateRaw(0, NVDataRef<QT3DSU8>((QT3DSU8 *)&cgLights, sizeof(QT3DSI32)));
                shader.m_LightsBuffer.Set();
            }
        } else {
            SLightConstantProperties<SShaderGeneratorGeneratedShader> *pLightConstants
                    = GetLightConstantProperties(shader);

            // if we have lights we need a light buffer
            QT3DS_ASSERT(shader.m_Lights.size() == 0 || pLightConstants);

            for (QT3DSU32 idx = 0, end = shader.m_Lights.size();
                    idx < end && pLightConstants; ++idx) {
                shader.m_Lights[idx].m_LightData.m_diffuse =
                    QT3DSVec4(shader.m_Lights[idx].m_LightColor.x * inMaterial.m_DiffuseColor.x,
                           shader.m_Lights[idx].m_LightColor.y * inMaterial.m_DiffuseColor.y,
                           shader.m_Lights[idx].m_LightColor.z * inMaterial.m_DiffuseColor.z, 1.0);
            }
            // update light buffer to hardware
            if (pLightConstants)
                pLightConstants->updateLights(shader);
        }

        shader.m_MaterialDiffuseLightAmbientTotal.Set(
            QT3DSVec3(shader.m_LightAmbientTotal.x * inMaterial.m_DiffuseColor[0],
                   shader.m_LightAmbientTotal.y * inMaterial.m_DiffuseColor[1],
                   shader.m_LightAmbientTotal.z * inMaterial.m_DiffuseColor[2]));

        shader.m_MaterialProperties.Set(QT3DSVec4(
            inMaterial.m_SpecularAmount, inMaterial.m_SpecularRoughness, emissivePower, 0.0f));
        shader.m_BumpAmount.Set(inMaterial.m_BumpAmount);
        shader.m_DisplaceAmount.Set(inMaterial.m_DisplaceAmount);
        shader.m_TranslucentFalloff.Set(inMaterial.m_TranslucentFalloff);
        shader.m_DiffuseLightWrap.Set(inMaterial.m_DiffuseLightWrap);
        QT3DSU32 imageIdx = 0;
        for (SRenderableImage *theImage = inFirstImage; theImage;
             theImage = theImage->m_NextImage, ++imageIdx)
            SetImageShaderVariables(shader, *theImage, imageIdx);

        qt3ds::render::NVRenderBlendFunctionArgument blendFunc;
        qt3ds::render::NVRenderBlendEquationArgument blendEqua(NVRenderBlendEquation::Add,
                                                               NVRenderBlendEquation::Add);
        // The blend function goes:
        // src op
        // dst op
        // src alpha op
        // dst alpha op
        // All of our shaders produce non-premultiplied values.
        switch (inMaterial.m_BlendMode) {
        case DefaultMaterialBlendMode::Screen:
            blendFunc = qt3ds::render::NVRenderBlendFunctionArgument(
                NVRenderSrcBlendFunc::SrcAlpha, NVRenderDstBlendFunc::One,
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::One);
            break;
        case DefaultMaterialBlendMode::Multiply:
            blendFunc = qt3ds::render::NVRenderBlendFunctionArgument(
                NVRenderSrcBlendFunc::DstColor, NVRenderDstBlendFunc::Zero,
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::One);
            break;
        case DefaultMaterialBlendMode::Overlay:
            // SW fallback is not using blend equation
            // note blend func is not used here anymore
            if (context.IsAdvancedBlendHwSupported() || context.IsAdvancedBlendHwSupportedKHR())
                blendEqua = qt3ds::render::NVRenderBlendEquationArgument(
                    NVRenderBlendEquation::Overlay, NVRenderBlendEquation::Overlay);
            break;
        case DefaultMaterialBlendMode::ColorBurn:
            // SW fallback is not using blend equation
            // note blend func is not used here anymore
            if (context.IsAdvancedBlendHwSupported() || context.IsAdvancedBlendHwSupportedKHR())
                blendEqua = qt3ds::render::NVRenderBlendEquationArgument(
                    NVRenderBlendEquation::ColorBurn, NVRenderBlendEquation::ColorBurn);
            break;
        case DefaultMaterialBlendMode::ColorDodge:
            // SW fallback is not using blend equation
            // note blend func is not used here anymore
            if (context.IsAdvancedBlendHwSupported() || context.IsAdvancedBlendHwSupportedKHR())
                blendEqua = qt3ds::render::NVRenderBlendEquationArgument(
                    NVRenderBlendEquation::ColorDodge, NVRenderBlendEquation::ColorDodge);
            break;
        default:
            blendFunc = qt3ds::render::NVRenderBlendFunctionArgument(
                NVRenderSrcBlendFunc::SrcAlpha, NVRenderDstBlendFunc::OneMinusSrcAlpha,
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha);
            break;
        }
        context.SetBlendFunction(blendFunc);
        context.SetBlendEquation(blendEqua);
    }
    void SetMaterialProperties(NVRenderShaderProgram &inProgram,
                               const SGraphObject &inMaterial, const QT3DSVec2 &inCameraVec,
                               const QT3DSMat44 &inModelViewProjection,
                               const QT3DSMat33 &inNormalMatrix,
                               const QT3DSMat44 &inGlobalTransform,
                               SRenderableImage *inFirstImage, QT3DSF32 inOpacity,
                               SLayerGlobalRenderProperties inRenderProperties,
                               const QT3DSVec2 &alphaOpRef) override
    {
        const SDefaultMaterial &theMaterial(static_cast<const SDefaultMaterial &>(inMaterial));
        QT3DS_ASSERT(inMaterial.m_Type == GraphObjectTypes::DefaultMaterial);

        SetGlobalProperties(inProgram, inRenderProperties.m_Layer, inRenderProperties.m_Camera,
                            inRenderProperties.m_CameraDirection, inRenderProperties.m_Lights,
                            inRenderProperties.m_LightDirections,
                            inRenderProperties.m_ShadowMapManager);
        SetMaterialProperties(inProgram, theMaterial, inCameraVec, inModelViewProjection,
                              inNormalMatrix, inGlobalTransform, inFirstImage, inOpacity,
                              inRenderProperties.m_DepthTexture, inRenderProperties.m_SSaoTexture,
                              inRenderProperties.m_LightProbe, inRenderProperties.m_LightProbe2,
                              inRenderProperties.m_ProbeHorizon, inRenderProperties.m_ProbeBright,
                              inRenderProperties.m_Probe2Window, inRenderProperties.m_Probe2Pos,
                              inRenderProperties.m_Probe2Fade, inRenderProperties.m_ProbeFOV,
                              alphaOpRef);
    }

    SLightConstantProperties<SShaderGeneratorGeneratedShader> *GetLightConstantProperties(SShaderGeneratorGeneratedShader &shader)
    {
        if (!shader.m_lightConstantProperties
                || int(shader.m_Lights.size())
                > shader.m_lightConstantProperties->m_constants.size()) {
            if (shader.m_lightConstantProperties)
                delete shader.m_lightConstantProperties;
            shader.m_lightConstantProperties
                    = new SLightConstantProperties<SShaderGeneratorGeneratedShader>(
                            shader, m_LightsAsSeparateUniforms);
        }
        return shader.m_lightConstantProperties;
    }
};
}

IDefaultMaterialShaderGenerator &
IDefaultMaterialShaderGenerator::CreateDefaultMaterialShaderGenerator(IQt3DSRenderContext &inRc)
{
    return *QT3DS_NEW(inRc.GetAllocator(), SShaderGenerator)(inRc);
}
