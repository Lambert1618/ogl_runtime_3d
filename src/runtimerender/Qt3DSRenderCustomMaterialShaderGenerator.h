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
#ifndef QT3DS_RENDER_CUSTOM_MATERIAL_SHADER_GENERATOR_H
#define QT3DS_RENDER_CUSTOM_MATERIAL_SHADER_GENERATOR_H
#include "Qt3DSRenderMaterialShaderGenerator.h"

namespace qt3ds {
namespace render {

    class Qt3DSShadowMap;

    class ICustomMaterialShaderGenerator : public IMaterialShaderGenerator
    {
    public:
        SImageVariableNames GetImageVariableNames(QT3DSU32 inIdx) override = 0;
        void GenerateImageUVCoordinates(IShaderStageGenerator &inVertexPipeline, QT3DSU32 idx,
                                                QT3DSU32 uvSet, SRenderableImage &image) override = 0;

        // inPipelineName needs to be unique else the shader cache will just return shaders from
        // different pipelines.
        NVRenderShaderProgram *GenerateShader(
            const SGraphObject &inMaterial, SShaderDefaultMaterialKey inShaderDescription,
            IShaderStageGenerator &inVertexPipeline, TShaderFeatureSet inFeatureSet,
            NVDataRef<SLight *> inLights, SRenderableImage *inFirstImage, bool inHasTransparency,
            const char8_t *inVertexPipelineName, const char8_t *inCustomMaterialName = "") override = 0;

        // Also sets the blend function on the render context.
        virtual void
        SetMaterialProperties(NVRenderShaderProgram &inProgram, const SGraphObject &inMaterial,
                              const QT3DSVec2 &inCameraVec, const QT3DSMat44 &inModelViewProjection,
                              const QT3DSMat33 &inNormalMatrix, const QT3DSMat44 &inGlobalTransform,
                              SRenderableImage *inFirstImage, QT3DSF32 inOpacity,
                              SLayerGlobalRenderProperties inRenderProperties) override = 0;

        static ICustomMaterialShaderGenerator &
        CreateCustomMaterialShaderGenerator(IQt3DSRenderContext &inRenderContext);
    };
}
}

#endif
