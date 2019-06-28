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


#include "Qt3DSRenderTestCustomMaterialGenerator.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderCustomMaterialRenderContext.h"
#include "Qt3DSRenderCustomMaterialShaderGenerator.h"
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "Qt3DSRenderDynamicObjectSystemCommands.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSTypes.h"
#include "Qt3DSRenderRuntimeBinding.h"
#include "Qt3DSApplication.h"
#include "Qt3DSInputEngine.h"
#include "foundation/FileTools.h"
#include "Qt3DSWindowSystem.h"
#include "Qt3DSRenderShaderCache.h"
#include "rendererimpl/Qt3DSRendererImpl.h"
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderUIPLoader.h"
#include "Qt3DSDMMetaDataTypes.h"

#include <QTime>
#include <QString>
#include <QTextStream>

#include <string>

using namespace qt3ds;
using namespace qt3ds::render;



namespace qt3ds {
namespace render {

Qt3DSRenderTestCustomMaterialGenerator::Qt3DSRenderTestCustomMaterialGenerator()
{

}

Qt3DSRenderTestCustomMaterialGenerator::~Qt3DSRenderTestCustomMaterialGenerator()
{

}


bool Qt3DSRenderTestCustomMaterialGenerator::isSupported(NVRenderContext *context)
{
    Q_UNUSED(context);
    return true;
}

bool Qt3DSRenderTestCustomMaterialGenerator::runPerformance(NVRenderContext *context,
                                                             userContextData *pContextData)
{
    Q_UNUSED(context);
    Q_UNUSED(pContextData);
    return false;
}

void Qt3DSRenderTestCustomMaterialGenerator::cleanup(NVRenderContext *context,
                                                      userContextData *pUserData)
{
    Q_UNUSED(context);
    Q_UNUSED(pUserData);
}

struct CustomTestParams
{
    SRenderableObjectFlags flags;
    dynamic::SDynamicShaderProgramFlags dynamicFlags;
    QT3DSF32 opacity;
    SLayer layer;
    SLayerRenderData layerData;
    SRenderableImage *images;
    SLight light[QT3DS_MAX_NUM_LIGHTS];
    SModel model;
    SRenderSubset subset;
    SShaderDefaultMaterialKey shaderkey;
    SCustomMaterial *material;
    eastl::vector<SShaderPreprocessorFeature> features;
    Option<qt3dsdm::SMetaDataCustomMaterial> metaMaterial;
    SImage dummyImages[SShaderDefaultMaterialKeyProperties::ImageMapCount];
    eastl::vector<SRenderableImage*> renderableImages;
    qt3ds::render::Qt3DSRendererImpl *render;
    SImage iblLightProbe;
    NVRenderTexture2D *texture;

    CustomTestParams(Qt3DSRendererImpl &impl)
        : layerData(layer, impl)
        , images(NULL)
        , subset(impl.GetContext().GetAllocator())
        , material(NULL)
        , render(&impl)
        , texture(NULL)
    {
    }
    ~CustomTestParams()
    {
        if (texture)
            texture->release();
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
            else
                renderableImages[i-1]->m_NextImage = renderableImages[i];
        }
    }
};

struct CustomTestKey
{
    unsigned int tessellation : 2;
    unsigned int wireframe : 1;
    unsigned int lighting: 1;
    unsigned int indirectLightmap: 1;
    unsigned int radiosityLightmap: 1;
    unsigned int shadowLightmap: 1;
    unsigned int transparency : 1;
    unsigned int refraction : 1;
    unsigned int iblprobe : 1;
    unsigned int emissiveMap : 1;
    unsigned int displacementMap : 1;
    unsigned int diffuse = 1;
    unsigned int specular = 1;
    unsigned int glossy = 1;
    unsigned int cutout = 1;
    unsigned int transmissive :  1;
    unsigned int ssao : 1;
    unsigned int ssdo : 1;
    unsigned int ssm : 1;

    CustomTestKey(TessModeValues::Enum tess = TessModeValues::NoTess, bool wire = false,
                  bool lights = false, bool indirect = false, bool radiosity = false,
                  bool shadow = false, bool trans = false, bool refract = false, bool ibl = false,
                  bool emissive = false, bool disp = false, bool diff = false,
                  bool spec = false, bool glo = false, bool cut = false, bool transm = false,
                  bool ao = false, bool direct = false, bool shadowssm = false)
        : tessellation(tess), wireframe(wire), lighting(lights), indirectLightmap(indirect)
        , radiosityLightmap(radiosity), shadowLightmap(shadow), transparency(trans)
        , refraction(refract), iblprobe(ibl), emissiveMap(emissive), displacementMap(disp)
        , diffuse(diff), specular(spec), glossy(glo), cutout(cut), transmissive(transm)
        , ssao(ao), ssdo(direct), ssm(shadowssm)
    {

    }

    bool operator == (const CustomTestKey &other) const
    {
        uint64_t a = *(uint64_t *)this;
        uint64_t b = *(uint64_t *)&other;
        return a == b;
    }

    QString toString()
    {
        QString str;
        QTextStream stream(&str);
        stream << "Custom Key tessellation: " << int(tessellation);
        stream << " wireframe: " << (wireframe ? "true" : "false");
        stream << " lighting: " << (lighting ? "true" : "false");
        stream << " indirectLightmap: " << (indirectLightmap ? "true" : "false");
        stream << " radiosityLightmap: " << (radiosityLightmap ? "true" : "false");
        stream << " shadowLightmap: " << (shadowLightmap ? "true" : "false");
        stream << " transparency: " << (transparency ? "true" : "false");
        stream << " refraction: " << (refraction ? "true" : "false");
        stream << " iblprobe: " << (iblprobe ? "true" : "false");
        stream << " emissiveMap: " << (emissiveMap ? "true" : "false");
        stream << " displacementMap: " << (displacementMap ? "true" : "false");
        stream << " diffuse: " << (diffuse ? "true" : "false");
        stream << " specular: " << (specular ? "true" : "false");
        stream << " glossy: " << (glossy ? "true" : "false");
        stream << " cutout: " << (cutout ? "true" : "false");
        stream << " transmissive: " << (transmissive ? "true" : "false");
        stream << " ssao: " << (ssao ? "true" : "false");
        stream << " ssdo: " << (ssdo ? "true" : "false");
        stream << " ssm: " << (ssm ? "true" : "false");
        return str;
    }
};

#define MAX_TEST_KEY ((1llu<<20)-1)

static CustomTestKey randomizeTestKey()
{
    uint64_t v = (uint64_t(qrand()))%MAX_TEST_KEY;
    return *reinterpret_cast<CustomTestKey*>(&v);
}

CustomTestParams *generateTest(qt3ds::render::Qt3DSRendererImpl *renderImpl,
                         NVRenderContext *context, CustomTestKey key, SCustomMaterial *material)
{
    CustomTestParams *params = new CustomTestParams(*renderImpl);
    params->material = material;
    params->material->m_ShaderKeyValues = (SCustomMaterialShaderKeyFlags)0;
    params->material->m_Lightmaps = SLightmaps();
    params->material->m_DisplacementMap = NULL;
    params->material->m_EmissiveMap2 = NULL;
    params->material->m_hasTransparency = false;
    params->material->m_IblProbe = NULL;
    params->material->m_hasRefraction = false;
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

    if (key.wireframe && key.tessellation > 0) {
        params->subset.m_WireframeMode = true;
        renderImpl->DefaultMaterialShaderKeyProperties().m_WireframeMode.SetValue(
            params->shaderkey, true);
    }

    CRegisteredString lighting =
                  renderImpl->GetContext().GetStringTable().RegisterStr("QT3DS_ENABLE_CG_LIGHTING");
    params->features.push_back(SShaderPreprocessorFeature(lighting, key.lighting));

    if (key.indirectLightmap) {
        params->material->m_Lightmaps.m_LightmapIndirect
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::LightmapIndirect];
        params->addRenderableImage(ImageMapTypes::LightmapIndirect,
                                   SShaderDefaultMaterialKeyProperties::LightmapIndirect);
    }
    if (key.radiosityLightmap) {
        params->material->m_Lightmaps.m_LightmapRadiosity
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::LightmapRadiosity];
        params->addRenderableImage(ImageMapTypes::LightmapRadiosity,
                                   SShaderDefaultMaterialKeyProperties::LightmapRadiosity);
    }
    if (key.shadowLightmap) {
        params->material->m_Lightmaps.m_LightmapShadow
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::LightmapShadow];
        params->addRenderableImage(ImageMapTypes::LightmapRadiosity,
                                   SShaderDefaultMaterialKeyProperties::LightmapShadow);
    }

    if (key.diffuse)
        params->material->m_ShaderKeyValues &= SCustomMaterialShaderKeyValues::diffuse;

    // TODO: emissive mask doesn't work
//    if (key.emissiveMap >= 1) {
//        params->material->m_EmissiveMap2
//                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::EmissiveMap];
//        params->addRenderableImage(ImageMapTypes::Emissive,
//                                   SShaderDefaultMaterialKeyProperties::EmissiveMap);
//    }

    if (key.specular)
        params->material->m_ShaderKeyValues &= SCustomMaterialShaderKeyValues::specular;

    if (key.displacementMap) {
        params->material->m_DisplacementMap
                = &params->dummyImages[SShaderDefaultMaterialKeyProperties::DisplacementMap];
        params->material->m_DisplacementMap->m_ImageShaderName
                = renderImpl->GetQt3DSContext().GetStringTable()
                    .RegisterStr("DisplacementMap");
        params->material->m_DisplaceAmount = 1.0f;
        params->addRenderableImage(ImageMapTypes::Displacement,
                                   SShaderDefaultMaterialKeyProperties::DisplacementMap);

        params->material->m_ShaderKeyValues &= SCustomMaterialShaderKeyValues::displace;
    }

    if (key.iblprobe) {
        renderImpl->DefaultMaterialShaderKeyProperties().m_HasIbl.SetValue(
            params->shaderkey, true);
        CRegisteredString str(renderImpl->GetQt3DSContext().GetStringTable()
                              .RegisterStr("QT3DS_ENABLE_LIGHT_PROBE"));
        params->features.push_back(SShaderPreprocessorFeature(str, true));

        params->material->m_IblProbe = &params->iblLightProbe;
        params->texture = context->CreateTexture2D();
        NVRenderTextureFormats::Enum format = NVRenderTextureFormats::RGBA8;
        unsigned int data = 0;
        NVDataRef<QT3DSU8> buffer = toU8DataRef<unsigned int>(data);
        params->texture->SetTextureData(buffer, 0, 1, 1, format);
        params->iblLightProbe.m_TextureData.m_Texture = params->texture;
    } else {
        CRegisteredString str(renderImpl->GetQt3DSContext().GetStringTable()
                              .RegisterStr("QT3DS_ENABLE_LIGHT_PROBE"));
        params->features.push_back(SShaderPreprocessorFeature(str, false));
    }

    // these requires calculateGlass function in material
//    if (key.transparency) {
//        params->material->m_ShaderKeyValues &= SCustomMaterialShaderKeyValues::transparent;
//        params->material->m_hasTransparency = true;
//    }
//    if (key.refraction) {
//        params->material->m_ShaderKeyValues &= SCustomMaterialShaderKeyValues::refraction;
//        params->material->m_hasRefraction = true;
//    }
    if (key.glossy)
        params->material->m_ShaderKeyValues &= SCustomMaterialShaderKeyValues::glossy;

    if (key.cutout)
        params->material->m_ShaderKeyValues &= SCustomMaterialShaderKeyValues::cutout;

    if (key.transmissive)
        params->material->m_ShaderKeyValues &= SCustomMaterialShaderKeyValues::transmissive;

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

    params->prepare();
    return params;
}

bool GenShader(IQt3DSRenderContext &qt3dsContext, CustomTestParams &params)
{
    bool success = true;
    ICustomMaterialShaderGenerator &theMaterialGenerator(qt3dsContext.GetCustomMaterialShaderGenerator());

    SCustomMaterialVertexPipeline thePipeline(&qt3dsContext,
                                              params.model.m_TessellationMode);

    for (int i = 0; i < params.metaMaterial->m_CustomMaterialCommands.size(); i++) {
        dynamic::SCommand &command = *params.metaMaterial->m_CustomMaterialCommands[i];
        if (command.m_Type == dynamic::CommandTypes::Enum::BindShader) {
            dynamic::SBindShader *bindShader = static_cast<dynamic::SBindShader *>(&command);
            NVRenderShaderProgram *theProgram = theMaterialGenerator.GenerateShader(
                *params.material, params.shaderkey, thePipeline,
                        toConstDataRef(params.features.data(), (QT3DSU32)params.features.size()),
                params.layerData.m_Lights, params.images,
                (params.material->m_hasTransparency || params.material->m_hasRefraction),
                "custom material pipeline-- ", bindShader->m_ShaderPath);
            if (!theProgram) {
                success = false;
                break;
            }
        }
    }

    return success;
}

bool Qt3DSRenderTestCustomMaterialGenerator::run(NVRenderContext *context,
                                                  userContextData *pUserData)
{
    Q_UNUSED(pUserData);
    bool success = true;

    QVector<CustomTestKey> testKeys;
    testKeys.push_back(CustomTestKey());
    testKeys.push_back(CustomTestKey(TessModeValues::TessLinear));
    testKeys.push_back(CustomTestKey(TessModeValues::TessNPatch));
    testKeys.push_back(CustomTestKey(TessModeValues::TessPhong));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true));
    testKeys.push_back(CustomTestKey(TessModeValues::NoTess, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true));

    while (testKeys.size() < 100) {
        CustomTestKey key = randomizeTestKey();
        if (!testKeys.contains(key))
            testKeys.push_back(key);
    }

    if (success) {
        CRegisteredString name = context->GetStringTable().RegisterStr("qrc:/copper.shader");

        metadata()->LoadMaterialXMLFile("CustomMaterial", "", "copper", "qrc:/copper.shader");
        Option<qt3dsdm::SMetaDataCustomMaterial> metaMaterial =
                            metadata()->GetMaterialMetaDataBySourcePath("qrc:/copper.shader");

        if (metaMaterial.hasValue()) {
            qt3ds::render::IUIPLoader::CreateMaterialClassFromMetaMaterial(
                    name, context->GetFoundation(),
                    qt3dsRenderer()->GetQt3DSContext().GetCustomMaterialSystem(), *metaMaterial,
                    context->GetStringTable());
             SCustomMaterial *material = qt3dsRenderer()->GetQt3DSContext().GetCustomMaterialSystem()
                    .CreateCustomMaterial(name, qt3dsRenderer()->GetContext().GetAllocator());

            for (CustomTestKey key : testKeys) {
                CustomTestParams *params = generateTest(qt3dsRenderer(), context, key, material);
                params->metaMaterial = metaMaterial;
                success &= GenShader(qt3dsRenderer()->GetQt3DSContext(), *params);
                if (!success)
                    qDebug () << "failing key: " << key.toString();
                delete params;
            }
            delete material;
        }
    }

    return success;
}

} // render
} // qt3ds
