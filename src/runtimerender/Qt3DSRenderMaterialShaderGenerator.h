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
#pragma once
#ifndef QT3DS_RENDER_MATERIAL_SHADER_GENERATOR_H
#define QT3DS_RENDER_MATERIAL_SHADER_GENERATOR_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "Qt3DSRenderShaderKeys.h"
#include "Qt3DSRenderShaderCache.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"

namespace qt3ds {
namespace render {

// these are our current shader limits
#define QT3DS_MAX_NUM_LIGHTS 16
#define QT3DS_MAX_NUM_SHADOWS 8

    // note this struct must exactly match the memory layout of the
    // struct sampleLight.glsllib and sampleArea.glsllib. If you make changes here you need
    // to adjust the code in sampleLight.glsllib and sampleArea.glsllib as well
    struct SLightSourceShader
    {
        QT3DSVec4 m_position;
        QT3DSVec4 m_direction; // Specifies the light direction in world coordinates.
        QT3DSVec4 m_up;
        QT3DSVec4 m_right;
        QT3DSVec4 m_diffuse;
        QT3DSVec4 m_ambient;
        QT3DSVec4 m_specular;
        QT3DSF32 m_spotExponent; // Specifies the intensity distribution of the light.
        QT3DSF32 m_spotCutoff; // Specifies the maximum spread angle of the light.
        QT3DSF32 m_constantAttenuation; // Specifies the constant light attenuation factor.
        QT3DSF32 m_linearAttenuation; // Specifies the linear light attenuation factor.
        QT3DSF32 m_quadraticAttenuation; // Specifies the quadratic light attenuation factor.
        QT3DSF32 m_range; // Specifies the maximum distance of the light influence
        QT3DSF32 m_width; // Specifies the width of the area light surface.
        QT3DSF32 m_height; // Specifies the height of the area light surface;
        QT3DSVec4 m_shadowControls;
        QT3DSMat44 m_shadowView;
        QT3DSI32 m_shadowIdx;
        QT3DSF32 m_padding1[3];
    };

    struct SLayerGlobalRenderProperties
    {
        const SLayer &m_Layer;
        SCamera &m_Camera;
        QT3DSVec3 m_CameraDirection;
        NVDataRef<SLight *> m_Lights;
        NVDataRef<QT3DSVec3> m_LightDirections;
        Qt3DSShadowMap *m_ShadowMapManager;
        NVRenderTexture2D *m_DepthTexture;
        NVRenderTexture2D *m_SSaoTexture;
        SImage *m_LightProbe;
        SImage *m_LightProbe2;
        QT3DSF32 m_ProbeHorizon;
        QT3DSF32 m_ProbeBright;
        QT3DSF32 m_Probe2Window;
        QT3DSF32 m_Probe2Pos;
        QT3DSF32 m_Probe2Fade;
        QT3DSF32 m_ProbeFOV;

        SLayerGlobalRenderProperties(const SLayer &inLayer, SCamera &inCamera,
                                     QT3DSVec3 inCameraDirection, NVDataRef<SLight *> inLights,
                                     NVDataRef<QT3DSVec3> inLightDirections,
                                     Qt3DSShadowMap *inShadowMapManager,
                                     NVRenderTexture2D *inDepthTexture,
                                     NVRenderTexture2D *inSSaoTexture, SImage *inLightProbe,
                                     SImage *inLightProbe2, QT3DSF32 inProbeHorizon,
                                     QT3DSF32 inProbeBright, QT3DSF32 inProbe2Window, QT3DSF32 inProbe2Pos,
                                     QT3DSF32 inProbe2Fade, QT3DSF32 inProbeFOV)
            : m_Layer(inLayer)
            , m_Camera(inCamera)
            , m_CameraDirection(inCameraDirection)
            , m_Lights(inLights)
            , m_LightDirections(inLightDirections)
            , m_ShadowMapManager(inShadowMapManager)
            , m_DepthTexture(inDepthTexture)
            , m_SSaoTexture(inSSaoTexture)
            , m_LightProbe(inLightProbe)
            , m_LightProbe2(inLightProbe2)
            , m_ProbeHorizon(inProbeHorizon)
            , m_ProbeBright(inProbeBright)
            , m_Probe2Window(inProbe2Window)
            , m_Probe2Pos(inProbe2Pos)
            , m_Probe2Fade(inProbe2Fade)
            , m_ProbeFOV(inProbeFOV)
        {
        }
    };

    class IMaterialShaderGenerator : public NVRefCounted
    {
    public:
        struct SImageVariableNames
        {
            const char8_t *m_ImageSampler;
            const char8_t *m_ImageFragCoords;
        };

        virtual SImageVariableNames GetImageVariableNames(QT3DSU32 inIdx) = 0;
        virtual void GenerateImageUVCoordinates(IShaderStageGenerator &inVertexPipeline, QT3DSU32 idx,
                                                QT3DSU32 uvSet, SRenderableImage &image) = 0;

        // inPipelineName needs to be unique else the shader cache will just return shaders from
        // different pipelines.
        virtual NVRenderShaderProgram *GenerateShader(
            const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
            IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
            NVDataRef<SLight *> inLights, SRenderableImage *inFirstImage, bool inHasTransparency,
            const char8_t *inVertexPipelineName, QString &error,
            const char8_t *inCustomMaterialName = "") = 0;

        virtual NVRenderShaderProgram *GenerateShader(
            const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
            IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
            NVDataRef<SLight *> inLights, SRenderableImage *inFirstImage, bool inHasTransparency,
            const char8_t *inVertexPipelineName, const char8_t *inCustomMaterialName = "") = 0;

        // Also sets the blend function on the render context.
        virtual void
        SetMaterialProperties(NVRenderShaderProgram &inProgram, const SGraphObject &inMaterial,
                              const QT3DSVec2 &inCameraVec, const QT3DSMat44 &inModelViewProjection,
                              const QT3DSMat33 &inNormalMatrix, const QT3DSMat44 &inGlobalTransform,
                              SRenderableImage *inFirstImage, QT3DSF32 inOpacity,
                              SLayerGlobalRenderProperties inRenderProperties,
                              const QT3DSVec2 &alphaOpRef) = 0;
    };
}
}

#endif
