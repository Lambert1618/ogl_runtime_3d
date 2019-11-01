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
#include "Qt3DSRenderCustomMaterialShaderGenerator.h"
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
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderLightConstantProperties.h"

using namespace qt3ds::render;
using qt3ds::render::NVRenderCachedShaderProperty;
using qt3ds::render::NVRenderCachedShaderBuffer;

namespace {
struct SShaderLightProperties
{
    NVScopedRefCounted<NVRenderShaderProgram> m_Shader;
    RenderLightTypes::Enum m_LightType;
    SLightSourceShader m_LightData;
    volatile QT3DSI32 mRefCount;

    SShaderLightProperties(NVRenderShaderProgram &inShader)
        : m_Shader(inShader)
        , m_LightType(RenderLightTypes::Directional)
        , mRefCount(0)
    {
    }

    void Set(const SLight *inLight)
    {
        QT3DSVec3 dir(0, 0, 1);
        if (inLight->m_LightType == RenderLightTypes::Directional) {
            dir = inLight->GetScalingCorrectDirection();
            // we lit in world sapce
            dir *= -1;
            m_LightData.m_position = QT3DSVec4(dir, 0.0);
        } else if (inLight->m_LightType == RenderLightTypes::Area) {
            dir = inLight->GetScalingCorrectDirection();
            m_LightData.m_position = QT3DSVec4(inLight->GetGlobalPos(), 1.0);
        } else {
            dir = inLight->GetGlobalPos();
            m_LightData.m_position = QT3DSVec4(dir, 1.0);
        }

        m_LightType = inLight->m_LightType;

        m_LightData.m_direction = QT3DSVec4(dir, 0.0);

        float normalizedBrightness = inLight->m_Brightness / 100.0f;
        m_LightData.m_diffuse = QT3DSVec4(inLight->m_DiffuseColor.getXYZ() * normalizedBrightness,
                                          inLight->m_DiffuseColor.w);
        m_LightData.m_specular = QT3DSVec4(inLight->m_SpecularColor.getXYZ() * normalizedBrightness,
                                           inLight->m_DiffuseColor.w);

        if (inLight->m_LightType == RenderLightTypes::Area) {
            m_LightData.m_width = inLight->m_AreaWidth;
            m_LightData.m_height = inLight->m_AreaWidth;

            QT3DSMat33 theDirMatrix(inLight->m_GlobalTransform.getUpper3x3());
            m_LightData.m_right =
                QT3DSVec4(theDirMatrix.transform(QT3DSVec3(1, 0, 0)), inLight->m_AreaWidth);
            m_LightData.m_up =
                QT3DSVec4(theDirMatrix.transform(QT3DSVec3(0, 1, 0)), inLight->m_AreaHeight);
        } else {
            m_LightData.m_width = 0.0;
            m_LightData.m_height = 0.0;
            m_LightData.m_right = QT3DSVec4(0.0f);
            m_LightData.m_up = QT3DSVec4(0.0f);

            // These components only apply to CG lights
            m_LightData.m_ambient = inLight->m_AmbientColor;

            m_LightData.m_constantAttenuation = 1.0;
            m_LightData.m_linearAttenuation = inLight->m_LinearFade;
            m_LightData.m_quadraticAttenuation = inLight->m_ExponentialFade;
            m_LightData.m_spotCutoff = 180.0;
        }

        if (m_LightType == RenderLightTypes::Point) {
            m_LightData.m_shadowView = QT3DSMat44::createIdentity();
        } else {
            m_LightData.m_shadowView = inLight->m_GlobalTransform;
        }
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader->GetRenderContext().GetAllocator())

    static SShaderLightProperties CreateLightEntry(NVRenderShaderProgram &inShader)
    {
        return SShaderLightProperties(inShader);
    }
};

/**
 *	Cached texture property lookups, used one per texture so a shader generator for N
 *	textures will have an array of N of these lookup objects.
 */
struct SShaderTextureProperties
{
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_Sampler;
    NVRenderCachedShaderProperty<QT3DSVec3> m_Offsets;
    NVRenderCachedShaderProperty<QT3DSVec4> m_Rotations;
    SShaderTextureProperties(const char *sampName, const char *offName, const char *rotName,
                             NVRenderShaderProgram &inShader)
        : m_Sampler(sampName, inShader)
        , m_Offsets(offName, inShader)
        , m_Rotations(rotName, inShader)
    {
    }
    SShaderTextureProperties() {}
};

/* We setup some shared state on the custom material shaders */
struct SShaderGeneratorGeneratedShader
{
    typedef nvhash_map<QT3DSU32, SShaderTextureProperties> TCustomMaterialImagMap;

    NVAllocatorCallback &m_Allocator;
    NVRenderShaderProgram &m_Shader;
    // Specific properties we know the shader has to have.
    NVRenderCachedShaderProperty<QT3DSMat44> m_ModelMatrix;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ViewProjMatrix;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ViewMatrix;
    NVRenderCachedShaderProperty<QT3DSMat33> m_NormalMatrix;
    NVRenderCachedShaderProperty<QT3DSVec3> m_CameraPos;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ProjMatrix;
    NVRenderCachedShaderProperty<QT3DSMat44> m_ViewportMatrix;
    NVRenderCachedShaderProperty<QT3DSVec2> m_CamProperties;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_DepthTexture;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_AOTexture;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_LightProbe;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeProps;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeOpts;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeRot;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbeOfs;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_LightProbe2;
    NVRenderCachedShaderProperty<QT3DSVec4> m_LightProbe2Props;
    NVRenderCachedShaderProperty<QT3DSI32> m_LightCount;
    NVRenderCachedShaderProperty<QT3DSI32> m_AreaLightCount;
    NVRenderCachedShaderProperty<QT3DSI32> m_ShadowMapCount;
    NVRenderCachedShaderProperty<QT3DSI32> m_ShadowCubeCount;
    NVRenderCachedShaderProperty<QT3DSF32> m_Opacity;
    NVRenderCachedShaderBuffer<qt3ds::render::NVRenderShaderConstantBuffer *> m_AoShadowParams;
    NVRenderCachedShaderBuffer<qt3ds::render::NVRenderShaderConstantBuffer *> m_LightsBuffer;
    NVRenderCachedShaderBuffer<qt3ds::render::NVRenderShaderConstantBuffer *> m_AreaLightsBuffer;

    SLightConstantProperties<SShaderGeneratorGeneratedShader> *m_lightsProperties;
    SLightConstantProperties<SShaderGeneratorGeneratedShader> *m_areaLightsProperties;

    typedef NVRenderCachedShaderPropertyArray<NVRenderTexture2D *,
                                              QT3DS_MAX_NUM_SHADOWS> ShadowMapPropertyArray;
    typedef NVRenderCachedShaderPropertyArray<NVRenderTextureCube *,
                                              QT3DS_MAX_NUM_SHADOWS> ShadowCubePropertyArray;

    ShadowMapPropertyArray m_shadowMaps;
    ShadowCubePropertyArray m_shadowCubes;

    // Cache the image property name lookups
    TCustomMaterialImagMap m_Images; // Images external to custom material usage
    volatile QT3DSI32 m_RefCount;

    SShaderGeneratorGeneratedShader(NVRenderShaderProgram &inShader, NVRenderContext &inContext)
        : m_Allocator(inContext.GetAllocator())
        , m_Shader(inShader)
        , m_ModelMatrix("model_matrix", inShader)
        , m_ViewProjMatrix("model_view_projection", inShader)
        , m_ViewMatrix("view_matrix", inShader)
        , m_NormalMatrix("normal_matrix", inShader)
        , m_CameraPos("camera_position", inShader)
        , m_ProjMatrix("view_projection_matrix", inShader)
        , m_ViewportMatrix("viewport_matrix", inShader)
        , m_CamProperties("camera_properties", inShader)
        , m_DepthTexture("depth_sampler", inShader)
        , m_AOTexture("ao_sampler", inShader)
        , m_LightProbe("light_probe", inShader)
        , m_LightProbeProps("light_probe_props", inShader)
        , m_LightProbeOpts("light_probe_opts", inShader)
        , m_LightProbeRot("light_probe_rotation", inShader)
        , m_LightProbeOfs("light_probe_offset", inShader)
        , m_LightProbe2("light_probe2", inShader)
        , m_LightProbe2Props("light_probe2_props", inShader)
        , m_LightCount("uNumLights", inShader)
        , m_AreaLightCount("uNumAreaLights", inShader)
        , m_ShadowMapCount("uNumShadowMaps", inShader)
        , m_ShadowCubeCount("uNumShadowCubes", inShader)
        , m_Opacity("object_opacity", inShader)
        , m_AoShadowParams("cbAoShadow", inShader)
        , m_LightsBuffer("cbBufferLights", inShader)
        , m_AreaLightsBuffer("cbBufferAreaLights", inShader)
        , m_lightsProperties(nullptr)
        , m_areaLightsProperties(nullptr)
        , m_shadowMaps("shadowMaps[0]", inShader)
        , m_shadowCubes("shadowCubes[0]", inShader)
        , m_Images(inContext.GetAllocator(), "SShaderGeneratorGeneratedShader::m_Images")
        , m_RefCount(0)
    {
        m_Shader.addRef();
    }

    ~SShaderGeneratorGeneratedShader()
    {
        m_Shader.release();
        delete m_lightsProperties;
        delete m_areaLightsProperties;
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

    SLightConstantProperties<SShaderGeneratorGeneratedShader> *GetLightProperties(int count)
    {
        if (!m_lightsProperties || m_areaLightsProperties->m_lightCountInt < count) {
            if (m_lightsProperties)
                delete m_lightsProperties;
            m_lightsProperties = new SLightConstantProperties<SShaderGeneratorGeneratedShader>
                                            ("lights", "uNumLights", *this, false, count);
        }
        return m_lightsProperties;
    }
    SLightConstantProperties<SShaderGeneratorGeneratedShader> *GetAreaLightProperties(int count)
    {
        if (!m_areaLightsProperties || m_areaLightsProperties->m_lightCountInt < count) {
            if (m_areaLightsProperties)
                delete m_areaLightsProperties;
            m_areaLightsProperties = new SLightConstantProperties<SShaderGeneratorGeneratedShader>
                                                ("areaLights", "uNumAreaLights", *this, false, count);
        }
        return m_areaLightsProperties;
    }
};

struct SShaderGenerator : public ICustomMaterialShaderGenerator
{
    typedef Qt3DSString TStrType;
    typedef nvhash_map<NVRenderShaderProgram *, NVScopedRefCounted<SShaderGeneratorGeneratedShader>>
        TProgramToShaderMap;
    typedef eastl::pair<size_t, NVScopedRefCounted<SShaderLightProperties>>
        TCustomMaterialLightEntry;
    typedef eastl::pair<size_t, NVRenderCachedShaderProperty<NVRenderTexture2D *>> TShadowMapEntry;
    typedef eastl::pair<size_t, NVRenderCachedShaderProperty<NVRenderTextureCube *>>
        TShadowCubeEntry;
    typedef qt3ds::foundation::nvhash_map<CRegisteredString,
                                       NVScopedRefCounted<qt3ds::render::NVRenderConstantBuffer>>
        TStrConstanBufMap;

    IQt3DSRenderContext &m_RenderContext;
    IShaderProgramGenerator &m_ProgramGenerator;

    const SCustomMaterial *m_CurrentMaterial;
    SShaderDefaultMaterialKey *m_CurrentKey;
    IDefaultMaterialVertexPipeline *m_CurrentPipeline;
    TShaderFeatureSet m_CurrentFeatureSet;
    NVDataRef<SLight *> m_Lights;
    SRenderableImage *m_FirstImage;
    bool m_HasTransparency;

    TStrType m_ImageStem;
    TStrType m_ImageSampler;
    TStrType m_ImageFragCoords;
    TStrType m_ImageRotScale;
    TStrType m_ImageOffset;

    eastl::string m_GeneratedShaderString;

    SShaderDefaultMaterialKeyProperties m_DefaultMaterialShaderKeyProperties;
    TProgramToShaderMap m_ProgramToShaderMap;

    nvvector<TCustomMaterialLightEntry> m_LightEntries;

    TStrConstanBufMap m_ConstantBuffers; ///< store all constants buffers

    QT3DSI32 m_RefCount;

    SShaderGenerator(IQt3DSRenderContext &inRc)
        : m_RenderContext(inRc)
        , m_ProgramGenerator(m_RenderContext.GetShaderProgramGenerator())
        , m_CurrentMaterial(NULL)
        , m_CurrentKey(NULL)
        , m_CurrentPipeline(NULL)
        , m_FirstImage(NULL)
        , m_HasTransparency(false)
        , m_ProgramToShaderMap(inRc.GetAllocator(), "m_ProgramToShaderMap")
        , m_LightEntries(inRc.GetAllocator(), "m_LightEntries")
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
    const SCustomMaterial &Material() { return *m_CurrentMaterial; }
    TShaderFeatureSet FeatureSet() { return m_CurrentFeatureSet; }
    bool HasTransparency() { return m_HasTransparency; }

    QT3DSU32
    ConvertTextureTypeValue(ImageMapTypes::Enum inType)
    {
        NVRenderTextureTypeValue::Enum retVal = NVRenderTextureTypeValue::Unknown;

        switch (inType) {
        case ImageMapTypes::LightmapIndirect:
            retVal = NVRenderTextureTypeValue::LightmapIndirect;
            break;
        case ImageMapTypes::LightmapRadiosity:
            retVal = NVRenderTextureTypeValue::LightmapRadiosity;
            break;
        case ImageMapTypes::LightmapShadow:
            retVal = NVRenderTextureTypeValue::LightmapShadow;
            break;
        case ImageMapTypes::Bump:
            retVal = NVRenderTextureTypeValue::Bump;
            break;
        case ImageMapTypes::Diffuse:
            retVal = NVRenderTextureTypeValue::Diffuse;
            break;
        case ImageMapTypes::Displacement:
            retVal = NVRenderTextureTypeValue::Displace;
            break;
        default:
            retVal = NVRenderTextureTypeValue::Unknown;
            break;
        }

        QT3DS_ASSERT(retVal != NVRenderTextureTypeValue::Unknown);

        return (QT3DSU32)retVal;
    }

    SImageVariableNames GetImageVariableNames(QT3DSU32 imageIdx) override
    {
        // convert to NVRenderTextureTypeValue
        NVRenderTextureTypeValue::Enum texType = (NVRenderTextureTypeValue::Enum)imageIdx;
        m_ImageStem.assign(NVRenderTextureTypeValue::toString(texType));
        m_ImageStem.append("_");
        m_ImageSampler = m_ImageStem;
        m_ImageSampler.append("sampler");
        m_ImageFragCoords = m_ImageStem;
        m_ImageFragCoords.append("uv_coords");
        m_ImageRotScale = m_ImageStem;
        m_ImageRotScale.append("rot_scale");
        m_ImageOffset = m_ImageStem;
        m_ImageOffset.append("offset");

        SImageVariableNames retVal;
        retVal.m_ImageSampler = m_ImageSampler.c_str();
        retVal.m_ImageFragCoords = m_ImageFragCoords.c_str();
        return retVal;
    }

    void SetImageShaderVariables(SShaderGeneratorGeneratedShader &inShader,
                                 SRenderableImage &inImage)
    {
        // skip displacement and emissive mask maps which are handled differently
        if (inImage.m_MapType == ImageMapTypes::Displacement
            || inImage.m_MapType == ImageMapTypes::Emissive)
            return;

        SShaderGeneratorGeneratedShader::TCustomMaterialImagMap::iterator iter =
            inShader.m_Images.find(inImage.m_MapType);
        if (iter == inShader.m_Images.end()) {
            SImageVariableNames names =
                GetImageVariableNames(ConvertTextureTypeValue(inImage.m_MapType));
            inShader.m_Images.insert(eastl::make_pair(
                (QT3DSU32)inImage.m_MapType,
                SShaderTextureProperties(names.m_ImageSampler, m_ImageOffset.c_str(),
                                         m_ImageRotScale.c_str(), inShader.m_Shader)));
            iter = inShader.m_Images.find(inImage.m_MapType);
        }

        SShaderTextureProperties &theShaderProps = iter->second;
        const QT3DSMat44 &textureTransform = inImage.m_Image.m_TextureTransform;
        const QT3DSF32 *dataPtr(textureTransform.front());
        QT3DSVec3 offsets(dataPtr[12], dataPtr[13], 0.0f);
        // Grab just the upper 2x2 rotation matrix from the larger matrix.
        QT3DSVec4 rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

        // The image horizontal and vertical tiling modes need to be set here, before we set texture
        // on the shader.
        // because setting the image on the texture forces the textue to bind and immediately apply
        // any tex params.
        inImage.m_Image.m_TextureData.m_Texture->SetTextureWrapS(
            inImage.m_Image.m_HorizontalTilingMode);
        inImage.m_Image.m_TextureData.m_Texture->SetTextureWrapT(
            inImage.m_Image.m_VerticalTilingMode);

        theShaderProps.m_Sampler.Set(inImage.m_Image.m_TextureData.m_Texture);
        theShaderProps.m_Offsets.Set(offsets);
        theShaderProps.m_Rotations.Set(rotations);
    }

    void GenerateImageUVCoordinates(IShaderStageGenerator &, QT3DSU32, QT3DSU32, SRenderableImage &) override {}

    ///< get the light constant buffer and generate if necessary
    NVRenderConstantBuffer *GetLightConstantBuffer(const char *name, QT3DSU32 inLightCount)
    {
        NVRenderContext &theContext(m_RenderContext.GetRenderContext());

        // we assume constant buffer support
        QT3DS_ASSERT(theContext.GetConstantBufferSupport());
        // we only create if if we have lights
        if (!inLightCount || !theContext.GetConstantBufferSupport())
            return NULL;

        CRegisteredString theName = theContext.GetStringTable().RegisterStr(name);
        NVRenderConstantBuffer *pCB = theContext.GetConstantBuffer(theName);

        if (!pCB) {
            // create with size of all structures + int for light count
            SLightSourceShader s[QT3DS_MAX_NUM_LIGHTS];
            NVDataRef<QT3DSU8> cBuffer((QT3DSU8 *)&s, (sizeof(SLightSourceShader) * QT3DS_MAX_NUM_LIGHTS)
                                        + (4 * sizeof(QT3DSI32)));
            pCB = theContext.CreateConstantBuffer(
                name, qt3ds::render::NVRenderBufferUsageType::Static,
                (sizeof(SLightSourceShader) * QT3DS_MAX_NUM_LIGHTS) + (4 * sizeof(QT3DSI32)), cBuffer);
            if (!pCB) {
                QT3DS_ASSERT(false);
                return NULL;
            }
            // init first set
            memset(&s[0], 0x0, sizeof(SLightSourceShader) * QT3DS_MAX_NUM_LIGHTS);
            QT3DSI32 cgLights[4] = {0, 0, 0, 0};
            pCB->UpdateRaw(0, NVDataRef<QT3DSU8>((QT3DSU8 *)&cgLights, sizeof(QT3DSI32) * 4));
            pCB->UpdateRaw(4 * sizeof(QT3DSI32),
                           NVDataRef<QT3DSU8>((QT3DSU8 *)&s[0],
                           sizeof(SLightSourceShader) * QT3DS_MAX_NUM_LIGHTS));
            pCB->Update(); // update to hardware

            m_ConstantBuffers.insert(eastl::make_pair(theName, pCB));
        }

        return pCB;
    }

    bool GenerateVertexShader(SShaderDefaultMaterialKey &, const char8_t *inShaderPathName)
    {
        qt3ds::render::IDynamicObjectSystem &theDynamicSystem(
            m_RenderContext.GetDynamicObjectSystem());
        Qt3DSString theShaderBuffer;
        theDynamicSystem.GetShaderSource(
            m_RenderContext.GetStringTable().RegisterStr(inShaderPathName), theShaderBuffer);

        eastl::string srcString(theShaderBuffer.c_str());

        // Check if the vertex shader portion already contains a main function
        // The same string contains both the vertex and the fragment shader
        // The last "#ifdef FRAGMENT_SHADER" should mark the start of the fragment shader
        eastl_size_t fragmentDefStart = srcString.find("#ifdef FRAGMENT_SHADER");
        eastl_size_t nextIndex = fragmentDefStart;
        while (nextIndex != eastl::string::npos) {
            nextIndex = srcString.find("#ifdef FRAGMENT_SHADER", nextIndex + 1);
            if (nextIndex != eastl::string::npos)
                fragmentDefStart = nextIndex;
        }
        eastl_size_t mainStart = srcString.find("void main()");

        if (mainStart != eastl::string::npos && (fragmentDefStart == eastl::string::npos
                                                 || mainStart < fragmentDefStart)) {
            TShaderGeneratorStageFlags stages(IShaderProgramGenerator::DefaultFlags());
            ProgramGenerator().BeginProgram(stages);
            IDefaultMaterialVertexPipeline &vertexShader(VertexGenerator());
            vertexShader << "#define VERTEX_SHADER\n\n";
            vertexShader << srcString.data() << Endl;
            return true;
        }

        // vertex displacement
        QT3DSU32 imageIdx = 0;
        SRenderableImage *displacementImage = NULL;
        QT3DSU32 displacementImageIdx = 0;

        for (SRenderableImage *img = m_FirstImage; img != NULL;
             img = img->m_NextImage, ++imageIdx) {
            if (img->m_MapType == ImageMapTypes::Displacement) {
                displacementImage = img;
                displacementImageIdx = imageIdx;
                break;
            }
        }

        // the pipeline opens/closes up the shaders stages
        VertexGenerator().BeginVertexGeneration(displacementImageIdx, displacementImage);
        return false;
    }

    SShaderGeneratorGeneratedShader &GetShaderForProgram(NVRenderShaderProgram &inProgram)
    {
        eastl::pair<TProgramToShaderMap::iterator, bool> inserter =
            m_ProgramToShaderMap.insert(eastl::make_pair(
                &inProgram, NVScopedRefCounted<SShaderGeneratorGeneratedShader>(NULL)));
        if (inserter.second) {
            NVAllocatorCallback &alloc(m_RenderContext.GetRenderContext().GetAllocator());
            inserter.first->second = QT3DS_NEW(alloc, SShaderGeneratorGeneratedShader)(
                inProgram, m_RenderContext.GetRenderContext());
        }
        return *inserter.first->second;
    }

    virtual SShaderLightProperties *SetLight(NVRenderShaderProgram &inShader, size_t lightIdx,
                                             size_t shadeIdx, const SLight *inLight,
                                             SShadowMapEntry *inShadow, QT3DSI32 shadowIdx,
                                             QT3DSF32 shadowDist)
    {
        SShaderLightProperties *theLightEntry(NULL);
        for (QT3DSU32 idx = 0, end = m_LightEntries.size(); idx < end && theLightEntry == NULL;
             ++idx) {
            if (m_LightEntries[idx].first == lightIdx
                && m_LightEntries[idx].second->m_Shader.mPtr == &inShader
                && m_LightEntries[idx].second->m_LightType == inLight->m_LightType) {
                theLightEntry = m_LightEntries[idx].second;
            }
        }
        if (theLightEntry == NULL) {
            // create a new name
            eastl::string lightName;
            if (inLight->m_LightType == RenderLightTypes::Area)
                lightName = "arealights";
            else
                lightName = "lights";
            char buf[16];
            _snprintf(buf, 16, "[%d]", int(shadeIdx));
            lightName.append(buf);

            NVScopedRefCounted<SShaderLightProperties> theNewEntry =
                QT3DS_NEW(m_RenderContext.GetAllocator(),
                       SShaderLightProperties)(SShaderLightProperties::CreateLightEntry(inShader));
            m_LightEntries.push_back(eastl::make_pair(lightIdx, theNewEntry));
            theLightEntry = theNewEntry.mPtr;
        }
        theLightEntry->Set(inLight);
        theLightEntry->m_LightData.m_shadowControls =
            QT3DSVec4(inLight->m_ShadowBias, inLight->m_ShadowFactor, shadowDist, 0.0);
        theLightEntry->m_LightData.m_shadowIdx = (inShadow) ? shadowIdx : -1;

        return theLightEntry;
    }

    void SetShadowMaps(NVRenderShaderProgram &inProgram, SShadowMapEntry *inShadow,
                       QT3DSI32 &numShadowMaps, QT3DSI32 &numShadowCubes, bool shadowMap,
                       SShaderGeneratorGeneratedShader::ShadowMapPropertyArray &shadowMaps,
                       SShaderGeneratorGeneratedShader::ShadowCubePropertyArray &shadowCubes)
    {
        Q_UNUSED(inProgram)
        if (inShadow) {
            if (shadowMap == false && inShadow->m_DepthCube
                    && (numShadowCubes < QT3DS_MAX_NUM_SHADOWS)) {
                shadowCubes.m_array[numShadowCubes] = inShadow->m_DepthCube.mPtr;
                ++numShadowCubes;
            } else if (shadowMap && inShadow->m_DepthMap
                    && (numShadowMaps < QT3DS_MAX_NUM_SHADOWS)) {
                shadowMaps.m_array[numShadowMaps] = inShadow->m_DepthMap.mPtr;
                ++numShadowMaps;
            }
        }
    }

    void SetGlobalProperties(NVRenderShaderProgram &inProgram, const SLayer & /*inLayer*/
                             ,
                             SCamera &inCamera, QT3DSVec3, NVDataRef<SLight *> inLights,
                             NVDataRef<QT3DSVec3>, Qt3DSShadowMap *inShadowMaps)
    {
        SShaderGeneratorGeneratedShader &theShader(GetShaderForProgram(inProgram));
        m_RenderContext.GetRenderContext().SetActiveShader(&inProgram);

        SCamera &theCamera(inCamera);

        QT3DSVec2 camProps(theCamera.m_ClipNear, theCamera.m_ClipFar);
        theShader.m_CamProperties.Set(camProps);
        theShader.m_CameraPos.Set(theCamera.GetGlobalPos());

        if (theShader.m_ViewMatrix.IsValid())
            theShader.m_ViewMatrix.Set(theCamera.m_GlobalTransform.getInverse());

        if (theShader.m_ProjMatrix.IsValid()) {
            QT3DSMat44 vProjMat;
            inCamera.CalculateViewProjectionMatrix(vProjMat);
            theShader.m_ProjMatrix.Set(vProjMat);
        }

        // set lights separate for area lights
        QT3DSI32 cgLights = 0, areaLights = 0;
        QT3DSI32 numShadowMaps = 0, numShadowCubes = 0;

        // this call setup the constant buffer for ambient occlusion and shadow
        theShader.m_AoShadowParams.Set();

        if (m_RenderContext.GetRenderContext().GetConstantBufferSupport()) {
            NVRenderConstantBuffer *pLightCb =
                GetLightConstantBuffer("cbBufferLights", inLights.size());
            NVRenderConstantBuffer *pAreaLightCb =
                GetLightConstantBuffer("cbBufferAreaLights", inLights.size());

            // Split the count between CG lights and area lights
            for (QT3DSU32 lightIdx = 0; lightIdx < inLights.size() && pLightCb; ++lightIdx) {
                SShadowMapEntry *theShadow = NULL;
                if (inShadowMaps && inLights[lightIdx]->m_CastShadow)
                    theShadow = inShadowMaps->GetShadowMapEntry(lightIdx);

                QT3DSI32 shdwIdx = (inLights[lightIdx]->m_LightType
                                                != RenderLightTypes::Directional)
                    ? numShadowCubes
                    : numShadowMaps;
                SetShadowMaps(inProgram, theShadow, numShadowMaps, numShadowCubes,
                              inLights[lightIdx]->m_LightType == RenderLightTypes::Directional,
                              theShader.m_shadowMaps, theShader.m_shadowCubes);

                if (inLights[lightIdx]->m_LightType == RenderLightTypes::Area) {
                    SShaderLightProperties *theAreaLightEntry =
                        SetLight(inProgram, lightIdx, areaLights, inLights[lightIdx], theShadow,
                                 shdwIdx, inCamera.m_ClipFar);

                    if (theAreaLightEntry && pAreaLightCb) {
                        pAreaLightCb->UpdateRaw(
                            areaLights * sizeof(SLightSourceShader) + (4 * sizeof(QT3DSI32)),
                            NVDataRef<QT3DSU8>((QT3DSU8 *)&theAreaLightEntry->m_LightData,
                                                            sizeof(SLightSourceShader)));
                    }

                    areaLights++;
                } else {
                    SShaderLightProperties *theLightEntry =
                        SetLight(inProgram, lightIdx, cgLights, inLights[lightIdx], theShadow,
                                 shdwIdx, inCamera.m_ClipFar);

                    if (theLightEntry && pLightCb) {
                        pLightCb->UpdateRaw(
                            cgLights * sizeof(SLightSourceShader) + (4 * sizeof(QT3DSI32)),
                            NVDataRef<QT3DSU8>((QT3DSU8 *)&theLightEntry->m_LightData,
                                                            sizeof(SLightSourceShader)));
                    }

                    cgLights++;
                }
            }

            if (pLightCb) {
                pLightCb->UpdateRaw(0, NVDataRef<QT3DSU8>((QT3DSU8 *)&cgLights, sizeof(QT3DSI32)));
                theShader.m_LightsBuffer.Set();
            }
            if (pAreaLightCb) {
                pAreaLightCb->UpdateRaw(0, NVDataRef<QT3DSU8>((QT3DSU8 *)&areaLights,
                                                              sizeof(QT3DSI32)));
                theShader.m_AreaLightsBuffer.Set();
            }

            theShader.m_LightCount.Set(cgLights);
            theShader.m_AreaLightCount.Set(areaLights);
        } else {
            QVector<SShaderLightProperties *> lprop;
            QVector<SShaderLightProperties *> alprop;
            for (QT3DSU32 lightIdx = 0; lightIdx < inLights.size(); ++lightIdx) {

                SShadowMapEntry *theShadow = NULL;
                if (inShadowMaps && inLights[lightIdx]->m_CastShadow)
                    theShadow = inShadowMaps->GetShadowMapEntry(lightIdx);

                QT3DSI32 shdwIdx = (inLights[lightIdx]->m_LightType
                                    != RenderLightTypes::Directional)
                    ? numShadowCubes
                    : numShadowMaps;
                SetShadowMaps(inProgram, theShadow, numShadowMaps, numShadowCubes,
                              inLights[lightIdx]->m_LightType == RenderLightTypes::Directional,
                              theShader.m_shadowMaps, theShader.m_shadowCubes);

                SShaderLightProperties *p = SetLight(inProgram, lightIdx, areaLights,
                                                     inLights[lightIdx], theShadow,
                                                     shdwIdx, inCamera.m_ClipFar);
                if (inLights[lightIdx]->m_LightType == RenderLightTypes::Area)
                    alprop.push_back(p);
                else
                    lprop.push_back(p);
            }
            SLightConstantProperties<SShaderGeneratorGeneratedShader> *lightProperties
                    = theShader.GetLightProperties(lprop.size());
            SLightConstantProperties<SShaderGeneratorGeneratedShader> *areaLightProperties
                    = theShader.GetAreaLightProperties(alprop.size());

            lightProperties->updateLights(lprop);
            areaLightProperties->updateLights(alprop);

            theShader.m_LightCount.Set(lprop.size());
            theShader.m_AreaLightCount.Set(alprop.size());
        }
        for (int i = numShadowMaps; i < QT3DS_MAX_NUM_SHADOWS; ++i)
            theShader.m_shadowMaps.m_array[i] = NULL;
        for (int i = numShadowCubes; i < QT3DS_MAX_NUM_SHADOWS; ++i)
            theShader.m_shadowCubes.m_array[i] = NULL;
        theShader.m_shadowMaps.Set(numShadowMaps);
        theShader.m_shadowCubes.Set(numShadowCubes);
        theShader.m_ShadowMapCount.Set(numShadowMaps);
        theShader.m_ShadowCubeCount.Set(numShadowCubes);
    }

    void SetMaterialProperties(NVRenderShaderProgram &inProgram, const SCustomMaterial &inMaterial,
                               const QT3DSVec2 &, const QT3DSMat44 &inModelViewProjection,
                               const QT3DSMat33 &inNormalMatrix, const QT3DSMat44 &inGlobalTransform,
                               SRenderableImage *inFirstImage, QT3DSF32 inOpacity,
                               NVRenderTexture2D *inDepthTexture, NVRenderTexture2D *inSSaoTexture,
                               SImage *inLightProbe, SImage *inLightProbe2, QT3DSF32 inProbeHorizon,
                               QT3DSF32 inProbeBright, QT3DSF32 inProbe2Window, QT3DSF32 inProbe2Pos,
                               QT3DSF32 inProbe2Fade, QT3DSF32 inProbeFOV,
                               const QT3DSVec2 &alphaOpRef)
    {
        ICustomMaterialSystem &theMaterialSystem(m_RenderContext.GetCustomMaterialSystem());
        SShaderGeneratorGeneratedShader &theShader(GetShaderForProgram(inProgram));

        theShader.m_ViewProjMatrix.Set(inModelViewProjection);
        theShader.m_NormalMatrix.Set(inNormalMatrix);
        theShader.m_ModelMatrix.Set(inGlobalTransform);

        theShader.m_DepthTexture.Set(inDepthTexture);
        theShader.m_AOTexture.Set(inSSaoTexture);

        theShader.m_Opacity.Set(inOpacity);

        qt3ds::render::SImage *theLightProbe = inLightProbe;
        qt3ds::render::SImage *theLightProbe2 = inLightProbe2;

        if (inMaterial.m_IblProbe && inMaterial.m_IblProbe->m_TextureData.m_Texture) {
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
                // light_probe_offsets.w is now no longer being used to enable/disable fast IBL,
                // (it's now the only option)
                // So now, it's storing the number of mip levels in the IBL image.
                QT3DSVec4 offsets(dataPtr[12], dataPtr[13],
                               theLightProbe->m_TextureData.m_TextureFlags.IsPreMultiplied() ? 1.0f
                                                                                             : 0.0f,
                               (float)theLightProbe->m_TextureData.m_Texture->GetNumMipmaps());
                // Fast IBL is always on;
                // inRenderContext.m_Layer.m_FastIbl ? 1.0f : 0.0f );
                // Grab just the upper 2x2 rotation matrix from the larger matrix.
                QT3DSVec4 rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

                theShader.m_LightProbeRot.Set(rotations);
                theShader.m_LightProbeOfs.Set(offsets);

                if ((!inMaterial.m_IblProbe) && (inProbeFOV < 180.f)) {
                    theShader.m_LightProbeOpts.Set(
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
                    theShader.m_LightProbe2.Set(theLightProbe2->m_TextureData.m_Texture);
                    theShader.m_LightProbe2Props.Set(
                        QT3DSVec4(inProbe2Window, inProbe2Pos, inProbe2Fade, 1.0f));

                    const QT3DSMat44 &xform2 = theLightProbe2->m_TextureTransform;
                    const QT3DSF32 *dataPtr(xform2.front());

                    theShader.m_LightProbeProps.Set(
                        QT3DSVec4(dataPtr[12], dataPtr[13], inProbeHorizon, inProbeBright * 0.01f));
                } else {
                    theShader.m_LightProbe2Props.Set(QT3DSVec4(0.0f, 0.0f, 0.0f, 0.0f));
                    theShader.m_LightProbeProps.Set(
                        QT3DSVec4(0.0f, 0.0f, inProbeHorizon, inProbeBright * 0.01f));
                }
            } else {
                theShader.m_LightProbeProps.Set(QT3DSVec4(0.0f, 0.0f, -1.0f, 0.0f));
                theShader.m_LightProbe2Props.Set(QT3DSVec4(0.0f, 0.0f, 0.0f, 0.0f));
            }

            theShader.m_LightProbe.Set(theLightProbe->m_TextureData.m_Texture);

        } else {
            theShader.m_LightProbeProps.Set(QT3DSVec4(0.0f, 0.0f, -1.0f, 0.0f));
            theShader.m_LightProbe2Props.Set(QT3DSVec4(0.0f, 0.0f, 0.0f, 0.0f));
        }

        // finally apply custom material shader properties
        theMaterialSystem.ApplyShaderPropertyValues(inMaterial, inProgram);

        // additional textures
        for (SRenderableImage *theImage = inFirstImage; theImage; theImage = theImage->m_NextImage)
            SetImageShaderVariables(theShader, *theImage);
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
        const SCustomMaterial &theCustomMaterial(
            reinterpret_cast<const SCustomMaterial &>(inMaterial));
        QT3DS_ASSERT(inMaterial.m_Type == GraphObjectTypes::CustomMaterial);

        SetGlobalProperties(inProgram, inRenderProperties.m_Layer, inRenderProperties.m_Camera,
                            inRenderProperties.m_CameraDirection, inRenderProperties.m_Lights,
                            inRenderProperties.m_LightDirections,
                            inRenderProperties.m_ShadowMapManager);

        SetMaterialProperties(inProgram, theCustomMaterial, inCameraVec, inModelViewProjection,
                              inNormalMatrix, inGlobalTransform, inFirstImage, inOpacity,
                              inRenderProperties.m_DepthTexture, inRenderProperties.m_SSaoTexture,
                              inRenderProperties.m_LightProbe, inRenderProperties.m_LightProbe2,
                              inRenderProperties.m_ProbeHorizon, inRenderProperties.m_ProbeBright,
                              inRenderProperties.m_Probe2Window, inRenderProperties.m_Probe2Pos,
                              inRenderProperties.m_Probe2Fade, inRenderProperties.m_ProbeFOV,
                              alphaOpRef);
    }

    void GenerateLightmapIndirectFunc(IShaderStageGenerator &inFragmentShader,
                                      SImage *pEmissiveLightmap)
    {
        inFragmentShader << "\n";
        inFragmentShader << "vec3 computeMaterialLightmapIndirect()\n{\n";
        inFragmentShader << "  vec4 indirect = vec4( 0.0, 0.0, 0.0, 0.0 );\n";
        if (pEmissiveLightmap) {
            SImageVariableNames names =
                GetImageVariableNames(ConvertTextureTypeValue(ImageMapTypes::LightmapIndirect));
            inFragmentShader.AddUniform(names.m_ImageSampler, "sampler2D");
            inFragmentShader.AddUniform(m_ImageOffset, "vec3");
            inFragmentShader.AddUniform(m_ImageRotScale, "vec4");

            inFragmentShader << "\n  indirect = evalIndirectLightmap( " << m_ImageSampler
                             << ", varTexCoord1, ";
            inFragmentShader << m_ImageRotScale << ", ";
            inFragmentShader << m_ImageOffset << " );\n\n";
        }

        inFragmentShader << "  return indirect.rgb;\n";
        inFragmentShader << "}\n\n";
    }

    void GenerateLightmapRadiosityFunc(IShaderStageGenerator &inFragmentShader,
                                       SImage *pRadiosityLightmap)
    {
        inFragmentShader << "\n";
        inFragmentShader << "vec3 computeMaterialLightmapRadiosity()\n{\n";
        inFragmentShader << "  vec4 radiosity = vec4( 1.0, 1.0, 1.0, 1.0 );\n";
        if (pRadiosityLightmap) {
            SImageVariableNames names =
                GetImageVariableNames(ConvertTextureTypeValue(ImageMapTypes::LightmapRadiosity));
            inFragmentShader.AddUniform(names.m_ImageSampler, "sampler2D");
            inFragmentShader.AddUniform(m_ImageOffset, "vec3");
            inFragmentShader.AddUniform(m_ImageRotScale, "vec4");

            inFragmentShader << "\n  radiosity = evalRadiosityLightmap( " << m_ImageSampler
                             << ", varTexCoord1, ";
            inFragmentShader << m_ImageRotScale << ", ";
            inFragmentShader << m_ImageOffset << " );\n\n";
        }

        inFragmentShader << "  return radiosity.rgb;\n";
        inFragmentShader << "}\n\n";
    }

    void GenerateLightmapShadowFunc(IShaderStageGenerator &inFragmentShader,
                                    SImage *pBakedShadowMap)
    {
        inFragmentShader << "\n";
        inFragmentShader << "vec4 computeMaterialLightmapShadow()\n{\n";
        inFragmentShader << "  vec4 shadowMask = vec4( 1.0, 1.0, 1.0, 1.0 );\n";
        if (pBakedShadowMap) {
            SImageVariableNames names =
                GetImageVariableNames((QT3DSU32)NVRenderTextureTypeValue::LightmapShadow);
            // Add uniforms
            inFragmentShader.AddUniform(names.m_ImageSampler, "sampler2D");
            inFragmentShader.AddUniform(m_ImageOffset, "vec3");
            inFragmentShader.AddUniform(m_ImageRotScale, "vec4");

            inFragmentShader << "\n  shadowMask = evalShadowLightmap( " << m_ImageSampler
                             << ", texCoord0, ";
            inFragmentShader << m_ImageRotScale << ", ";
            inFragmentShader << m_ImageOffset << " );\n\n";
        }

        inFragmentShader << "  return shadowMask;\n";
        inFragmentShader << "}\n\n";
    }

    void GenerateLightmapIndirectSetupCode(IShaderStageGenerator &inFragmentShader,
                                           SRenderableImage *pIndirectLightmap,
                                           SRenderableImage *pRadiosityLightmap)
    {
        if (!pIndirectLightmap && !pRadiosityLightmap)
            return;

        eastl::string finalValue;

        inFragmentShader << "\n";
        inFragmentShader << "void initializeLayerVariablesWithLightmap(void)\n{\n";
        if (pIndirectLightmap) {
            inFragmentShader
                << "  vec3 lightmapIndirectValue = computeMaterialLightmapIndirect( );\n";
            finalValue.append("vec4(lightmapIndirectValue, 1.0)");
        }
        if (pRadiosityLightmap) {
            inFragmentShader
                << "  vec3 lightmapRadisoityValue = computeMaterialLightmapRadiosity( );\n";
            if (finalValue.empty())
                finalValue.append("vec4(lightmapRadisoityValue, 1.0)");
            else
                finalValue.append(" + vec4(lightmapRadisoityValue, 1.0)");
        }

        finalValue.append(";\n");

        char buf[16];
        for (QT3DSU32 idx = 0; idx < Material().m_LayerCount; idx++) {
            _snprintf(buf, 16, "[%d]", idx);
            inFragmentShader << "  layers" << buf << ".base += " << finalValue.c_str();
            inFragmentShader << "  layers" << buf << ".layer += " << finalValue.c_str();
        }

        inFragmentShader << "}\n\n";
    }

    void GenerateLightmapShadowCode(IShaderStageGenerator &inFragmentShader,
                                    SRenderableImage *pBakedShadowMap)
    {
        if (pBakedShadowMap) {
            inFragmentShader << " tmpShadowTerm *= computeMaterialLightmapShadow( );\n\n";
        }
    }

    void ApplyEmissiveMask(IShaderStageGenerator &inFragmentShader, SImage *pEmissiveMaskMap)
    {
        inFragmentShader << "\n";
        inFragmentShader << "vec3 computeMaterialEmissiveMask()\n{\n";
        inFragmentShader << "  vec3 emissiveMask = vec3( 1.0, 1.0, 1.0 );\n";
        if (pEmissiveMaskMap) {
            inFragmentShader << "  texture_coordinate_info tci;\n";
            inFragmentShader << "  texture_coordinate_info transformed_tci;\n";
            inFragmentShader << "  tci = textureCoordinateInfo( texCoord0, tangent, binormal );\n";
            inFragmentShader << "  transformed_tci = transformCoordinate( "
                                "rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), ";
            inFragmentShader << "vec3( 0.000000, 0.000000, 0.000000 ), vec3( 1.000000, 1.000000, "
                                "1.000000 ) ), tci );\n";
            inFragmentShader << "  emissiveMask = fileTexture( "
                             << pEmissiveMaskMap->m_ImageShaderName.c_str()
                             << ", vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_alpha, transformed_tci, ";
            inFragmentShader << "vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), "
                                "wrap_repeat, wrap_repeat, gamma_default ).tint;\n";
        }

        inFragmentShader << "  return emissiveMask;\n";
        inFragmentShader << "}\n\n";
    }

    // Returns true if custom shader provides main function
    bool GenerateFragmentShader(SShaderDefaultMaterialKey &, const char8_t *inShaderPathName,
                                bool hasCustomVertexShader)
    {
        qt3ds::render::IDynamicObjectSystem &theDynamicSystem(
            m_RenderContext.GetDynamicObjectSystem());
        Qt3DSString theShaderBuffer;
        theDynamicSystem.GetShaderSource(
            m_RenderContext.GetStringTable().RegisterStr(inShaderPathName), theShaderBuffer);

        QT3DS_ASSERT(theShaderBuffer.size() > 0);

        // light maps
        bool hasLightmaps = false;
        SRenderableImage *lightmapShadowImage = NULL;
        SRenderableImage *lightmapIndirectImage = NULL;
        SRenderableImage *lightmapRadisoityImage = NULL;

        for (SRenderableImage *img = m_FirstImage; img != NULL; img = img->m_NextImage) {
            if (img->m_MapType == ImageMapTypes::LightmapIndirect) {
                lightmapIndirectImage = img;
                hasLightmaps = true;
            } else if (img->m_MapType == ImageMapTypes::LightmapRadiosity) {
                lightmapRadisoityImage = img;
                hasLightmaps = true;
            } else if (img->m_MapType == ImageMapTypes::LightmapShadow) {
                lightmapShadowImage = img;
            }
        }

        if (!hasCustomVertexShader) {
            VertexGenerator().GenerateUVCoords(0);
            // for lightmaps we expect a second set of uv coordinates
            if (hasLightmaps)
                VertexGenerator().GenerateUVCoords(1);
        }

        IDefaultMaterialVertexPipeline &vertexShader(VertexGenerator());
        IShaderStageGenerator &fragmentShader(FragmentGenerator());

        eastl::string srcString(theShaderBuffer.c_str());

        if (m_RenderContext.GetRenderContext().GetRenderContextType()
                == NVRenderContextValues::GLES2) {
            eastl::string::size_type pos = 0;
            while ((pos = srcString.find("out vec4 fragColor", pos)) != eastl::string::npos) {
                srcString.insert(pos, "//");
                pos += int(strlen("//out vec4 fragColor"));
            }
        }

        fragmentShader << "#define FRAGMENT_SHADER\n\n";

        // Check if the fragment shader portion already contains a main function
        // The same string contains both the vertex and the fragment shader
        // The last "#ifdef FRAGMENT_SHADER" should mark the start of the fragment shader
        eastl_size_t fragmentDefStart = srcString.find("#ifdef FRAGMENT_SHADER");
        eastl_size_t nextIndex = fragmentDefStart;
        while (nextIndex != eastl::string::npos) {
            nextIndex = srcString.find("#ifdef FRAGMENT_SHADER", nextIndex + 1);
            if (nextIndex != eastl::string::npos)
                fragmentDefStart = nextIndex;
        }
        if (fragmentDefStart == eastl::string::npos)
            return false;

        eastl_size_t mainStart = srcString.find("void main()");
        if (mainStart != eastl::string::npos && mainStart < fragmentDefStart)
            mainStart = srcString.find("void main()", mainStart + 1);

        bool hasCustomFragmentShader = mainStart != eastl::string::npos;

        if (!hasCustomFragmentShader)
            fragmentShader.AddInclude("evalLightmaps.glsllib");

        // check dielectric materials
        if (!Material().IsDielectric())
            fragmentShader << "#define MATERIAL_IS_NON_DIELECTRIC 1\n\n";
        else
            fragmentShader << "#define MATERIAL_IS_NON_DIELECTRIC 0\n\n";

        fragmentShader << "#define QT3DS_ENABLE_RNM 0\n\n";

        fragmentShader << srcString.data() << Endl;

        if (hasCustomFragmentShader) {
            fragmentShader << "#define FRAGMENT_SHADER\n\n";
            if (!hasCustomVertexShader) {
                vertexShader.GenerateWorldNormal();
                vertexShader.GenerateVarTangentAndBinormal();
                vertexShader.GenerateWorldPosition();

                vertexShader.GenerateViewVector();
            }
            return true;
        }

        if (Material().HasLighting() && lightmapIndirectImage) {
            GenerateLightmapIndirectFunc(fragmentShader, &lightmapIndirectImage->m_Image);
        }
        if (Material().HasLighting() && lightmapRadisoityImage) {
            GenerateLightmapRadiosityFunc(fragmentShader, &lightmapRadisoityImage->m_Image);
        }
        if (Material().HasLighting() && lightmapShadowImage) {
            GenerateLightmapShadowFunc(fragmentShader, &lightmapShadowImage->m_Image);
        }

        if (Material().HasLighting() && (lightmapIndirectImage || lightmapRadisoityImage))
            GenerateLightmapIndirectSetupCode(fragmentShader, lightmapIndirectImage,
                                              lightmapRadisoityImage);

        if (Material().HasLighting()) {
            ApplyEmissiveMask(fragmentShader, Material().m_EmissiveMap2);
        }

        // setup main
        VertexGenerator().BeginFragmentGeneration();

        // since we do pixel lighting we always need this if lighting is enabled
        // We write this here because the functions below may also write to
        // the fragment shader
        if (Material().HasLighting()) {
            vertexShader.GenerateWorldNormal();
            vertexShader.GenerateVarTangentAndBinormal();
            vertexShader.GenerateWorldPosition();

            if (Material().IsSpecularEnabled())
                vertexShader.GenerateViewVector();
        }

        fragmentShader << "  initializeBaseFragmentVariables();" << Endl;
        fragmentShader << "  computeTemporaries();" << Endl;
        fragmentShader << "  normal = normalize( computeNormal() );" << Endl;
        fragmentShader << "  initializeLayerVariables();" << Endl;
        fragmentShader << "  float alpha = clamp( evalCutout(), 0.0, 1.0 );" << Endl;

        if (Material().IsCutOutEnabled()) {
            fragmentShader << "  if ( alpha <= 0.0f )" << Endl;
            fragmentShader << "    discard;" << Endl;
        }

        // indirect / direct lightmap init
        if (Material().HasLighting() && (lightmapIndirectImage || lightmapRadisoityImage))
            fragmentShader << "  initializeLayerVariablesWithLightmap();" << Endl;

        // shadow map
        GenerateLightmapShadowCode(fragmentShader, lightmapShadowImage);

        // main Body
        fragmentShader << "#include \"customMaterialFragBodyAO.glsllib\"" << Endl;

        // for us right now transparency means we render a glass style material
        if (m_HasTransparency && !Material().IsTransmissive())
            fragmentShader << " rgba = computeGlass( normal, materialIOR, alpha, rgba );" << Endl;
        if (Material().IsTransmissive())
            fragmentShader << " rgba = computeOpacity( rgba );" << Endl;

        if (VertexGenerator().HasActiveWireframe()) {
            fragmentShader.Append("vec3 edgeDistance = varEdgeDistance * gl_FragCoord.w;");
            fragmentShader.Append(
                "\tfloat d = min(min(edgeDistance.x, edgeDistance.y), edgeDistance.z);");
            fragmentShader.Append("\tfloat mixVal = smoothstep(0.0, 1.0, d);"); // line width 1.0

            fragmentShader.Append("\trgba = mix( vec4(0.0, 1.0, 0.0, 1.0), rgba, mixVal);");
        }
        fragmentShader << "  rgba.a *= object_opacity;" << Endl;
        if (m_RenderContext.GetRenderContext().GetRenderContextType()
                == NVRenderContextValues::GLES2)
            fragmentShader << "  gl_FragColor = rgba;" << Endl;
        else
            fragmentShader << "  fragColor = rgba;" << Endl;
        return false;
    }

    NVRenderShaderProgram *GenerateCustomMaterialShader(const char8_t *inShaderPrefix,
                                                        const char8_t *inCustomMaterialName,
                                                        QString &errors)
    {
        // build a string that allows us to print out the shader we are generating to the log.
        // This is time consuming but I feel like it doesn't happen all that often and is very
        // useful to users
        // looking at the log file.
        m_GeneratedShaderString.clear();
        m_GeneratedShaderString.assign(nonNull(inShaderPrefix));
        m_GeneratedShaderString.append(inCustomMaterialName);
        SShaderDefaultMaterialKey theKey(Key());
        theKey.ToString(m_GeneratedShaderString, m_DefaultMaterialShaderKeyProperties,
                        SShaderDefaultMaterialKeyProperties::DefaultKey);

        // Add hash of the shader code to the cache key so that custom materials with the same name
        // can have different shaders and the shaders are recompiled when the code is changed
        qt3ds::render::IDynamicObjectSystem &dynamicSystem
                = m_RenderContext.GetDynamicObjectSystem();
        Qt3DSString shaderBuffer;
        dynamicSystem.GetShaderSource(
            m_RenderContext.GetStringTable().RegisterStr(inCustomMaterialName), shaderBuffer);
        m_GeneratedShaderString.append(
                    QString::number(qHash(QString(shaderBuffer))).toUtf8().constData());

        bool hasCustomVertexShader = GenerateVertexShader(theKey, inCustomMaterialName);
        bool hasCustomFragmentShader = GenerateFragmentShader(theKey, inCustomMaterialName,
                                                              hasCustomVertexShader);

        VertexGenerator().EndVertexGeneration(hasCustomVertexShader);
        VertexGenerator().EndFragmentGeneration(hasCustomFragmentShader);

        NVRenderShaderProgram *program = ProgramGenerator().CompileGeneratedShader(
                    m_GeneratedShaderString.c_str(), SShaderCacheProgramFlags(), FeatureSet(),
                    errors);
        if (program && hasCustomVertexShader) {
            // Change uniforms names to match runtime 2.x uniforms
            SShaderGeneratorGeneratedShader &shader(GetShaderForProgram(*program));
            shader.m_ModelMatrix = NVRenderCachedShaderProperty<QT3DSMat44>("modelMatrix",
                                                                            *program);
            shader.m_ViewProjMatrix = NVRenderCachedShaderProperty<QT3DSMat44>(
                        "modelViewProjection", *program);
            shader.m_ViewMatrix = NVRenderCachedShaderProperty<QT3DSMat44>("viewMatrix", *program);
            shader.m_NormalMatrix = NVRenderCachedShaderProperty<QT3DSMat33>("modelNormalMatrix",
                                                                             *program);
            shader.m_ProjMatrix = NVRenderCachedShaderProperty<QT3DSMat44>("viewProjectionMatrix",
                                                                           *program);
            shader.m_ViewportMatrix = NVRenderCachedShaderProperty<QT3DSMat44>("viewportMatrix",
                                                                               *program);
            shader.m_CameraPos = NVRenderCachedShaderProperty<QT3DSVec3>("eyePosition", *program);
        }
        return program;
    }

    virtual NVRenderShaderProgram *
    GenerateShader(const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
                   IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
                   NVDataRef<SLight *> inLights, SRenderableImage *inFirstImage,
                   bool inHasTransparency, const char8_t *inShaderPrefix, QString &error,
                   const char8_t *inCustomMaterialName) override
    {
        QT3DS_ASSERT(inMaterial.m_Type == GraphObjectTypes::CustomMaterial);
        m_CurrentMaterial = reinterpret_cast<const SCustomMaterial *>(&inMaterial);
        m_CurrentKey = &inShaderDescription;
        m_CurrentPipeline = static_cast<IDefaultMaterialVertexPipeline *>(&inVertexPipeline);
        m_CurrentFeatureSet = inFeatureSet;
        m_Lights = inLights;
        m_FirstImage = inFirstImage;
        m_HasTransparency = inHasTransparency;

        return GenerateCustomMaterialShader(inShaderPrefix, inCustomMaterialName, error);
    }

    virtual NVRenderShaderProgram *
    GenerateShader(const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
                   IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
                   NVDataRef<SLight *> inLights, SRenderableImage *inFirstImage,
                   bool inHasTransparency, const char8_t *inShaderPrefix,
                   const char8_t *inCustomMaterialName) override
    {
        QString error;

        return GenerateShader(inMaterial, inShaderDescription, inVertexPipeline, inFeatureSet,
                              inLights, inFirstImage, inHasTransparency, inShaderPrefix,
                              error, inCustomMaterialName);
    }
};
}

ICustomMaterialShaderGenerator &
ICustomMaterialShaderGenerator::CreateCustomMaterialShaderGenerator(IQt3DSRenderContext &inRc)
{
    return *QT3DS_NEW(inRc.GetAllocator(), SShaderGenerator)(inRc);
}
