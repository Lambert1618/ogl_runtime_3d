/****************************************************************************
**
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


#include "Qt3DSRenderTestDefaultMaterialGenerator.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSTypes.h"
#include "Qt3DSRenderRuntimeBinding.h"
#include "Qt3DSApplication.h"
#include "Qt3DSInputEngine.h"
#include "foundation/FileTools.h"
#include "Qt3DSWindowSystem.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderShaderCache.h"
#include "rendererimpl/Qt3DSRendererImpl.h"
#include "Qt3DSRenderLight.h"

#include <QTime>

#include <string>

using namespace qt3ds;
using namespace qt3ds::render;



namespace qt3ds {
namespace render {

Qt3DSRenderTestDefaultMaterialGenerator::Qt3DSRenderTestDefaultMaterialGenerator()
{

}

Qt3DSRenderTestDefaultMaterialGenerator::~Qt3DSRenderTestDefaultMaterialGenerator()
{

}


bool Qt3DSRenderTestDefaultMaterialGenerator::isSupported(NVRenderContext *context)
{
    Q_UNUSED(context);
    return true;
}

bool Qt3DSRenderTestDefaultMaterialGenerator::runPerformance(NVRenderContext *context,
                                                             userContextData *pContextData)
{
    Q_UNUSED(context);
    Q_UNUSED(pContextData);
    return false;
}

void Qt3DSRenderTestDefaultMaterialGenerator::cleanup(NVRenderContext *context,
                                                      userContextData *pUserData)
{
    Q_UNUSED(context);
    Q_UNUSED(pUserData);
}


#define MAX_TEST_KEY ((1llu<<32)-1)

struct TestKey
{
    unsigned int tessellation: 2;
    unsigned int wireframe: 1;
    unsigned int lighting: 2;
    unsigned int lights: 4;
    unsigned int indirectLightmap: 1;
    unsigned int radiosityLightmap: 1;
    unsigned int lightProbe: 2;
    unsigned int iblLightProbe: 1;
    unsigned int iblfow: 1;
    unsigned int diffuseMap: 2;
    unsigned int emissiveMap: 2;
    unsigned int specularModel: 2;
    unsigned int specularMap: 1;
    unsigned int specularReflection: 1;
    unsigned int fresnel: 1;
    unsigned int bumpmapping: 2;
    unsigned int displacementMap: 1;
    unsigned int opacityMap: 1;
    unsigned int translucencyMap: 1;
    unsigned int ssm: 1;
    unsigned int ssdo: 1;
    unsigned int ssao: 1;

    TestKey(TessModeValues::Enum tess = TessModeValues::NoTess, bool wire = false,
            DefaultMaterialLighting::Enum lmode = DefaultMaterialLighting::NoLighting,
            int lightCount = 0, bool indirect = false, bool radio = false, int lprobe = 0,
            bool iblProbe = false, bool iblf = false, int diffuse = 0, int emissive = 0,
            DefaultMaterialSpecularModel::Enum sModel = DefaultMaterialSpecularModel::Default,
            bool sMap = false, bool sRef = false, bool fre = false, int bump = 0,
            bool disp = false, bool opac = false, bool trans = false, bool shadow = false,
            bool ao = false, bool directOcc = false)
        : tessellation(tess), wireframe(wire), lighting(lmode), lights(lightCount)
        , indirectLightmap(indirect), radiosityLightmap(radio), lightProbe(lprobe)
        , iblLightProbe(iblProbe), iblfow(iblf), diffuseMap(diffuse), emissiveMap(emissive)
        , specularModel(sModel), specularMap(sMap), specularReflection(sRef)
        , fresnel(fre), bumpmapping(bump), displacementMap(disp), opacityMap(opac)
        , translucencyMap(trans), ssm(shadow), ssao(ao), ssdo(directOcc)
    {

    }

    bool operator == (const TestKey &other) const
    {
        return toInt() == other.toInt();
    }

    QString toString()
    {
        QString str;
        QTextStream stream(&str);

        stream << " tessellation: " << tessellation;
        stream << " wireframe: " << (wireframe ? "true" : "false");
        stream << " lighting: " << lighting;
        stream << " lights: " << lights;
        stream << " indirectLightmap: " << (indirectLightmap ? "true" : "false");
        stream << " radiosityLightmap: " << (radiosityLightmap ? "true" : "false");
        stream << " lightProbe: " << lightProbe;
        stream << " iblLightProbe: " << (iblLightProbe ? "true" : "false");
        stream << " iblfow: " << (iblfow ? "true" : "false");
        stream << " diffuseMap: " << diffuseMap;
        stream << " emissiveMap: " << emissiveMap;
        stream << " specularModel: " << specularModel;
        stream << " specularMap: " << (specularMap ? "true" : "false");
        stream << " specularReflection: " << (specularReflection ? "true" : "false");
        stream << " fresnel: " << (fresnel ? "true" : "false");
        stream << " bumpmapping: " << bumpmapping;
        stream << " displacementMap: " << (displacementMap ? "true" : "false");
        stream << " opacityMap: " << (opacityMap ? "true" : "false");
        stream << " translucencyMap: " << (translucencyMap ? "true" : "false");
        stream << " ssm: " << (ssm ? "true" : "false");
        stream << " ssdo: " << (ssdo ? "true" : "false");
        stream << " ssao: " << (ssao ? "true" : "false");

        return str;
    }

    void clampValues()
    {
        lighting = qMin(lighting, 2u);
        if (lighting == 1)
            lighting = 2;
        emissiveMap = qMin(emissiveMap, 2u);
        specularModel = qMin(specularModel, 2u);
        bumpmapping = qMin(bumpmapping, 2u);
        lightProbe = 0;
        if (!iblLightProbe)
            iblfow = 0;
        if (!tessellation)
            wireframe = 0;
    }

    uint64_t toInt() const
    {
        return (*(uint64_t *)this)&MAX_TEST_KEY;
    }
};

struct TestParams
{
    SRenderableObjectFlags flags;
    NVConstDataRef<QT3DSMat44> boneGlobals;
    SRenderSubset subset;
    SDefaultMaterial material;
    SModel model;
    QT3DSMat44 viewProjection;
    SModelContext modelContext;
    SRenderableImage *images;
    SShaderDefaultMaterialKey shaderkey;
    SSubsetRenderable renderable;
    eastl::vector<SShaderPreprocessorFeature> features;
    SLight light[QT3DS_MAX_NUM_LIGHTS];
    SLayer layer;
    SLayerRenderData layerData;
    SImage dummyImages[SShaderDefaultMaterialKeyProperties::ImageMapCount];
    NVRenderTexture2D *textures[4];
    eastl::vector<SRenderableImage*> renderableImages;
    qt3ds::render::Qt3DSRendererImpl *render;

    TestParams(NVRenderContext *context, qt3ds::render::Qt3DSRendererImpl *renderImpl)
        : subset(context->GetAllocator())
        , modelContext(model, viewProjection)
        , images(NULL)
        , renderable(flags, QT3DSVec3(), *renderImpl, subset, material, modelContext, 1.0f, images,
                     shaderkey, boneGlobals)
        , layerData(layer, *renderImpl)
        , render(renderImpl)
    {
        for (int i = 0; i < 4; ++i) {
            textures[i] = context->CreateTexture2D();
            NVRenderTextureFormats::Enum format = NVRenderTextureFormats::RGBA8;
            unsigned int data = 0;
            NVDataRef<QT3DSU8> buffer = toU8DataRef<unsigned int>(data);
            textures[i]->SetTextureData(buffer, 0, 1, 1, format);
        }
        for (int i = 0; i < SShaderDefaultMaterialKeyProperties::ImageMapCount; ++i)
            dummyImages[i].m_TextureData.m_Texture = textures[i%4];
        dummyImages[SShaderDefaultMaterialKeyProperties::DiffuseMap1].m_TextureData.
                m_TextureFlags.SetPreMultiplied(true);
        dummyImages[SShaderDefaultMaterialKeyProperties::EmissiveMap2].m_TextureData.
                m_TextureFlags.SetInvertUVCoords(true);
    }
    ~TestParams()
    {
        for (NVRenderTexture2D *tex : textures)
            tex->release();
        for (SRenderableImage *img : renderableImages)
            delete img;
    }
    void addRenderableImage(ImageMapTypes::Enum type,
                            SShaderDefaultMaterialKeyProperties::ImageMapNames name)
    {
        renderableImages.push_back(new SRenderableImage(type, dummyImages[name]));
        render->DefaultMaterialShaderKeyProperties().m_ImageMaps[name].SetEnabled(shaderkey, true);
    }
    void prepare()
    {
        for (unsigned int i = 0; i < renderableImages.size(); i++) {
            if (i == 0)
                images = renderableImages[0];
            else {
                renderableImages[i-1]->m_NextImage = renderableImages[i];
            }

        }
        renderable.m_ShaderDescription = shaderkey;
        renderable.m_FirstImage = images;
    }
};

TestKey randomizeTestKey()
{
    uint64_t v = (uint64_t(qrand()) << 32 | uint64_t(qrand()))&MAX_TEST_KEY;
    TestKey key = *reinterpret_cast<TestKey*>(&v);
    key.clampValues();
    return key;
}


TestParams *generateTest(qt3ds::render::Qt3DSRendererImpl *renderImpl,
                         NVRenderContext *context, TestKey key)
{
    // TODO: light probes
    TestParams *params = new TestParams(context, renderImpl);

    switch (key.tessellation) {
    case 1:
        params->model.m_TessellationMode = TessModeValues::TessLinear;
        break;
    case 2:
        params->model.m_TessellationMode = TessModeValues::TessPhong;
        break;
    case 3:
        params->model.m_TessellationMode = TessModeValues::TessNPatch;
        break;
    default:
        params->model.m_TessellationMode = TessModeValues::NoTess;
        break;
    }

    renderImpl->DefaultMaterialShaderKeyProperties()
            .m_TessellationMode.SetValue(params->shaderkey, params->model.m_TessellationMode);

    switch (key.lighting) {
    case 1:
        params->material.m_Lighting = DefaultMaterialLighting::VertexLighting;
        break;
    case 2:
        params->material.m_Lighting = DefaultMaterialLighting::FragmentLighting;
        break;
    default:
        params->material.m_Lighting = DefaultMaterialLighting::NoLighting;
        break;
    }
    if (key.lighting != 0) {
        renderImpl->DefaultMaterialShaderKeyProperties()
                .m_HasLighting.SetValue(params->shaderkey, true);
    }

    if (key.wireframe && key.tessellation > 0) {
        params->subset.m_WireframeMode = true;
        renderImpl->DefaultMaterialShaderKeyProperties().m_WireframeMode.SetValue(
            params->shaderkey, true);
    }
    bool castShadow = false;
    key.lights = qMin(int(key.lights), int(QT3DS_MAX_NUM_LIGHTS));
    for (unsigned int i = 0; i < key.lights; ++i) {
        params->light[i].m_LightType = static_cast<RenderLightTypes::Enum>((i%3)+1);
        if (params->light[i].m_LightType != RenderLightTypes::Directional) {
            renderImpl->DefaultMaterialShaderKeyProperties().m_LightFlags[i].SetValue(
                params->shaderkey, true);
        }
        if (params->light[i].m_LightType == RenderLightTypes::Area) {
            renderImpl->DefaultMaterialShaderKeyProperties()
                .m_LightAreaFlags[i]
                .SetValue(params->shaderkey, true);
        }
        if (params->light[i].m_LightType != RenderLightTypes::Point) {
            renderImpl->DefaultMaterialShaderKeyProperties()
                .m_LightShadowFlags[i]
                .SetValue(params->shaderkey, castShadow);
            castShadow = !castShadow;
        }
        params->layerData.m_Lights.push_back(&params->light[i]);
    }

    renderImpl->DefaultMaterialShaderKeyProperties().m_LightCount.SetValue(params->shaderkey,
                                                                          key.lights);

    // shadow lightmap is not used
    if (key.indirectLightmap) {
        params->material.m_Lightmaps.m_LightmapIndirect
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::LightmapIndirect];
        params->addRenderableImage(ImageMapTypes::LightmapIndirect,
                                   SShaderDefaultMaterialKeyProperties::LightmapIndirect);
    }
    if (key.radiosityLightmap) {
        params->material.m_Lightmaps.m_LightmapRadiosity
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::LightmapRadiosity];
        params->addRenderableImage(ImageMapTypes::LightmapRadiosity,
                                   SShaderDefaultMaterialKeyProperties::LightmapRadiosity);
    }

    for (unsigned int i = 0; i < key.diffuseMap; ++i) {
        params->material.m_DiffuseMaps[i]
                = &params->dummyImages[static_cast<SShaderDefaultMaterialKeyProperties::ImageMapNames>
                (SShaderDefaultMaterialKeyProperties::DiffuseMap0 + i)];
        params->addRenderableImage(ImageMapTypes::Diffuse,
                                   static_cast<SShaderDefaultMaterialKeyProperties::ImageMapNames>
                                   (SShaderDefaultMaterialKeyProperties::DiffuseMap0 + i));
    }

    if (key.emissiveMap >= 1) {
        params->material.m_EmissiveMap
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::EmissiveMap];
        params->addRenderableImage(ImageMapTypes::Emissive,
                                   SShaderDefaultMaterialKeyProperties::EmissiveMap);
    }
    if (key.emissiveMap == 2) {
        params->material.m_EmissiveMap2
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::EmissiveMap2];
        params->addRenderableImage(ImageMapTypes::Emissive,
                                   SShaderDefaultMaterialKeyProperties::EmissiveMap2);
    }

    switch (key.specularModel) {
    case 1:
        params->material.m_SpecularModel = DefaultMaterialSpecularModel::KGGX;
        break;
    case 2:
        params->material.m_SpecularModel = DefaultMaterialSpecularModel::KWard;
        break;
    default:
        params->material.m_SpecularModel = DefaultMaterialSpecularModel::Default;
        break;
    }

    if (key.specularMap) {
        params->material.m_SpecularMap =
                &params->dummyImages[SShaderDefaultMaterialKeyProperties::SpecularAmountMap];
        params->material.m_SpecularAmount = 1.0f;
        params->addRenderableImage(ImageMapTypes::SpecularAmountMap,
                                   SShaderDefaultMaterialKeyProperties::SpecularAmountMap);
    }
    if (key.specularReflection) {
        params->dummyImages[SShaderDefaultMaterialKeyProperties::SpecularMap].m_MappingMode
                = ImageMappingModes::Environment;
        params->material.m_SpecularAmount = 1.0f;
        params->material.m_SpecularReflection
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::SpecularMap];
        params->addRenderableImage(ImageMapTypes::Specular,
                                   SShaderDefaultMaterialKeyProperties::SpecularMap);
    }
    if (key.fresnel)
        params->material.m_FresnelPower = 1.0f;

    if (key.bumpmapping == 1) {
        params->material.m_BumpMap
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::BumpMap];
        params->material.m_BumpAmount = 1.0f;
        params->addRenderableImage(ImageMapTypes::Bump, SShaderDefaultMaterialKeyProperties::BumpMap);
    }
    if (key.bumpmapping == 2) {
        params->material.m_NormalMap
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::NormalMap];
        params->addRenderableImage(ImageMapTypes::Normal, SShaderDefaultMaterialKeyProperties::NormalMap);
    }

    if (key.displacementMap) {
        params->material.m_DisplacementMap
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::DisplacementMap];
        params->material.m_DisplaceAmount = 1.0f;
        params->addRenderableImage(ImageMapTypes::Displacement,
                                   SShaderDefaultMaterialKeyProperties::DisplacementMap);
    }

    if (key.opacityMap) {
        params->material.m_OpacityMap
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::OpacityMap];
        params->material.m_Opacity = 0.5f;
        params->addRenderableImage(ImageMapTypes::Opacity,
                                   SShaderDefaultMaterialKeyProperties::OpacityMap);
    }

    if (key.translucencyMap) {
        params->material.m_TranslucencyMap
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::TranslucencyMap];
        params->material.m_TranslucentFalloff = 0.1f;
        params->addRenderableImage(ImageMapTypes::Translucency,
                                   SShaderDefaultMaterialKeyProperties::TranslucencyMap);
    }

    if (key.ssm) {
        CRegisteredString str(renderImpl->GetQt3DSContext().GetStringTable()
                              .RegisterStr("QT3DS_ENABLE_SSM"));
        params->features.push_back(SShaderPreprocessorFeature(str, true));
    }
    if (key.ssao) {
        CRegisteredString str(renderImpl->GetQt3DSContext().GetStringTable()
                              .RegisterStr("QT3DS_ENABLE_SSAO"));
        params->features.push_back(SShaderPreprocessorFeature(str, true));
    }
    if (key.ssdo) {
        CRegisteredString str(renderImpl->GetQt3DSContext().GetStringTable()
                              .RegisterStr("QT3DS_ENABLE_SSDO"));
        params->features.push_back(SShaderPreprocessorFeature(str, true));
    }

    if (key.iblLightProbe) {
        renderImpl->DefaultMaterialShaderKeyProperties().m_HasIbl.SetValue(
            params->shaderkey, true);
        CRegisteredString str(renderImpl->GetQt3DSContext().GetStringTable()
                              .RegisterStr("QT3DS_ENABLE_LIGHT_PROBE"));
        params->features.push_back(SShaderPreprocessorFeature(str, true));
        if (key.iblfow) {
            CRegisteredString str(renderImpl->GetQt3DSContext().GetStringTable()
                                  .RegisterStr("QT3DS_ENABLE_IBL_FOV"));
            params->features.push_back(SShaderPreprocessorFeature(str, true));
        }
    }

    if (params->material.IsSpecularEnabled()) {
        renderImpl->DefaultMaterialShaderKeyProperties().m_SpecularEnabled.SetValue(
            params->shaderkey, true);
        renderImpl->DefaultMaterialShaderKeyProperties().m_SpecularModel.SetSpecularModel(
            params->shaderkey, params->material.m_SpecularModel);
    }
    if (params->material.IsFresnelEnabled()) {
        renderImpl->DefaultMaterialShaderKeyProperties().m_FresnelEnabled.SetValue(
            params->shaderkey, true);
    }

    params->prepare();
    return params;
}



bool Qt3DSRenderTestDefaultMaterialGenerator::run(NVRenderContext *context,
                                                  userContextData *pUserData)
{
    Q_UNUSED(pUserData);
    bool success = true;

    qsrand(QTime::currentTime().msec());

    QVector<TestKey> testKeys;

    testKeys.push_back(TestKey());
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 1));
    testKeys.push_back(TestKey(TessModeValues::TessLinear, false, DefaultMaterialLighting::FragmentLighting, 1));
    testKeys.push_back(TestKey(TessModeValues::TessNPatch, false, DefaultMaterialLighting::FragmentLighting, 1));
    testKeys.push_back(TestKey(TessModeValues::TessPhong, false, DefaultMaterialLighting::FragmentLighting, 1));
    testKeys.push_back(TestKey(TessModeValues::TessLinear, true, DefaultMaterialLighting::FragmentLighting, 1));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 6));
    // vertex lighting is not supported?
    //testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::VertexLighting, 6));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 1, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 1, true, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 2));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 3, true, true, 0, false, false, 3));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 3, true, true, 0, false, false, 1, 1));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 4, true, true, 0, false, false, 1, 2));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 4, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::KGGX, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 5, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::KWard, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 5, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 6, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 6, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true, 1));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true, 2));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true, 0, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true, 0, true, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true, 0, true, true, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true, 0, true, true, true, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true, 0, true, true, true, false, true));
    testKeys.push_back(TestKey(TessModeValues::NoTess, false, DefaultMaterialLighting::FragmentLighting, 2, true, true, 0, false, false, 1, 1, DefaultMaterialSpecularModel::Default, true, true, true, 0, true, true, true, true, false, true));

    while (testKeys.size() < 100) {
        TestKey key = randomizeTestKey();
        if (!testKeys.contains(key))
            testKeys.push_back(key);
    }
    // generated programs must be unique
    QVector<NVRenderShaderProgram *> programs;
    success = initializeQt3DSRenderer(context->format());
    if (success) {
        for (TestKey key : testKeys) {
            qDebug () << "testing key: " << key.toInt();
            TestParams *params = generateTest(qt3dsRenderer(), context, key);

            qt3dsRenderer()->BeginLayerRender(params->layerData);
            NVRenderShaderProgram *program
                    = qt3dsRenderer()->GenerateShader(params->renderable,
                                                   toConstDataRef(params->features.data(),
                                                                  (QT3DSU32)params->features.size()),
                                                      false);
            if (!program) {
                success = false;
            } else {
                if (programs.contains(program)) {
                    qDebug () << "Generated program is not unique vs " << testKeys[programs.indexOf(program)].toString();
                    success = false;
                }
                else {
                    programs.push_back(program);
                }
            }

            if (!success)
                qDebug () << "failing test key: " << key.toString();

            qt3dsRenderer()->EndLayerRender();
            delete params;
        }
    }

    return success;
}

} // render
} // qt3ds
