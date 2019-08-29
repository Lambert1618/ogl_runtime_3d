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
#include "Qt3DSRenderableObjects.h"
#include "Qt3DSRendererImpl.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderCustomMaterialRenderContext.h"
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderPathManager.h"
#include "Qt3DSRenderPathRenderContext.h"
#include "Qt3DSRenderDefaultMaterialShaderGenerator.h"

using qt3ds::foundation::CRegisteredString;

namespace qt3ds {
namespace render {
    struct SRenderableImage;
    struct SShaderGeneratorGeneratedShader;
    struct SSubsetRenderable;
    using eastl::make_pair;
    using eastl::reverse;

    STextScaleAndOffset::STextScaleAndOffset(NVRenderTexture2D &inTexture,
                                             const STextTextureDetails &inTextDetails,
                                             const STextRenderInfo &inInfo)
        : m_TextOffset(0, 0)
        , m_TextScale(1, 1)

    {
        NVRenderTexture2D &theTexture = inTexture;
        STextureDetails theDetails(theTexture.GetTextureDetails());
        QT3DSVec2 textDimensions(inTextDetails.m_TextWidth / 2.0f, inTextDetails.m_TextHeight / 2.0f);
        textDimensions.x /= inTextDetails.m_ScaleFactor.x;
        textDimensions.y /= inTextDetails.m_ScaleFactor.y;
        QT3DSVec2 theTextScale(textDimensions.x, textDimensions.y);
        QT3DSVec2 theTextOffset(0, 0);

        // Set the offsets to use after scaling the rect coordinates.
        switch (inInfo.m_HorizontalAlignment) {
        case TextHorizontalAlignment::Left:
            theTextOffset[0] = theTextScale[0];
            break;
        case TextHorizontalAlignment::Center:
            break;
        case TextHorizontalAlignment::Right:
            theTextOffset[0] = -theTextScale[0];
            break;
        default:
            break;
        }

        switch (inInfo.m_VerticalAlignment) {
        case TextVerticalAlignment::Top:
            theTextOffset[1] = -theTextScale[1];
            break;
        case TextVerticalAlignment::Middle:
            break;
        case TextVerticalAlignment::Bottom:
            theTextOffset[1] = theTextScale[1];
            break;
        default:
            break;
        }
        m_TextScale = theTextScale;
        m_TextOffset = theTextOffset;
    }

    void SSubsetRenderableBase::RenderShadowMapPass(const QT3DSVec2 &inCameraVec,
                                                    const SLight *inLight, const SCamera &inCamera,
                                                    SShadowMapEntry *inShadowMapEntry)
    {
        NVRenderContext &context(m_Generator.GetContext());
        SRenderableDepthPrepassShader *shader = nullptr;
        NVRenderInputAssembler *pIA = nullptr;

        /*
        if ( inLight->m_LightType == RenderLightTypes::Area )
                shader = m_Generator.GetParaboloidDepthShader( m_TessellationMode );
        else if ( inLight->m_LightType == RenderLightTypes::Directional )
                shader = m_Generator.GetOrthographicDepthShader( m_TessellationMode );
        else if ( inLight->m_LightType == RenderLightTypes::Point )
                shader = m_Generator.GetCubeShadowDepthShader( m_TessellationMode );	// This
        will change to include a geometry shader pass.
        */

        if (inLight->m_LightType == RenderLightTypes::Directional)
            shader = m_Generator.GetOrthographicDepthShader(m_TessellationMode);
        else
            shader = m_Generator.GetCubeShadowDepthShader(m_TessellationMode);

        if (shader == nullptr || inShadowMapEntry == nullptr)
            return;

        // for phong and npatch tesselleation we need the normals too
        if (m_TessellationMode == TessModeValues::NoTess
            || m_TessellationMode == TessModeValues::TessLinear)
            pIA = m_Subset.m_InputAssemblerDepth;
        else
            pIA = m_Subset.m_InputAssembler;

        QT3DSMat44 theModelViewProjection = inShadowMapEntry->m_LightVP * m_GlobalTransform;
        // QT3DSMat44 theModelView = inLight->m_GlobalTransform.getInverse() * m_GlobalTransform;

        context.SetActiveShader(&shader->m_Shader);
        shader->m_MVP.Set(theModelViewProjection);
        shader->m_CameraPosition.Set(inCamera.m_Position);
        shader->m_GlobalTransform.Set(m_GlobalTransform);
        shader->m_CameraProperties.Set(inCameraVec);
        /*
        shader->m_CameraDirection.Set( inCamera.GetDirection() );

        shader->m_ShadowMV[0].Set( inShadowMapEntry->m_LightCubeView[0] * m_GlobalTransform );
        shader->m_ShadowMV[1].Set( inShadowMapEntry->m_LightCubeView[1] * m_GlobalTransform );
        shader->m_ShadowMV[2].Set( inShadowMapEntry->m_LightCubeView[2] * m_GlobalTransform );
        shader->m_ShadowMV[3].Set( inShadowMapEntry->m_LightCubeView[3] * m_GlobalTransform );
        shader->m_ShadowMV[4].Set( inShadowMapEntry->m_LightCubeView[4] * m_GlobalTransform );
        shader->m_ShadowMV[5].Set( inShadowMapEntry->m_LightCubeView[5] * m_GlobalTransform );
        shader->m_Projection.Set( inCamera.m_Projection );
        */

        // tessellation
        if (m_TessellationMode != TessModeValues::NoTess) {
            // set uniforms we need
            shader->m_Tessellation.m_EdgeTessLevel.Set(m_Subset.m_EdgeTessFactor);
            shader->m_Tessellation.m_InsideTessLevel.Set(m_Subset.m_InnerTessFactor);
            // the blend value is hardcoded
            shader->m_Tessellation.m_PhongBlend.Set(0.75);
            // set distance range value
            shader->m_Tessellation.m_DistanceRange.Set(inCameraVec);
            // disable culling
            shader->m_Tessellation.m_DisableCulling.Set(1.0);
        }

        context.SetInputAssembler(pIA);
        context.Draw(m_Subset.m_PrimitiveType, m_Subset.m_Count, m_Subset.m_Offset);
    }

    void SSubsetRenderableBase::RenderDepthPass(const QT3DSVec2 &inCameraVec,
                                                SRenderableImage *inDisplacementImage,
                                                float inDisplacementAmount)
    {
        NVRenderContext &context(m_Generator.GetContext());
        SRenderableDepthPrepassShader *shader = nullptr;
        NVRenderInputAssembler *pIA = nullptr;
        SRenderableImage *displacementImage = inDisplacementImage;

        if (m_Subset.m_PrimitiveType != NVRenderDrawMode::Patches) {
            shader = m_Generator.GetDepthPrepassShader(displacementImage != nullptr);
        } else {
            shader = m_Generator.GetDepthTessPrepassShader(m_TessellationMode,
                                                           displacementImage != nullptr);
        }

        if (shader == nullptr)
            return;

        // for phong and npatch tesselleation or displacement mapping we need the normals (and uv's)
        // too
        if ((m_TessellationMode == TessModeValues::NoTess
             || m_TessellationMode == TessModeValues::TessLinear)
            && !displacementImage)
            pIA = m_Subset.m_InputAssemblerDepth;
        else
            pIA = m_Subset.m_InputAssembler;

        context.SetActiveShader(&shader->m_Shader);
        context.SetCullingEnabled(true);

        shader->m_MVP.Set(m_ModelContext.m_ModelViewProjection);

        if (displacementImage) {
            // setup image transform
            const QT3DSMat44 &textureTransform = displacementImage->m_Image.m_TextureTransform;
            const QT3DSF32 *dataPtr(textureTransform.front());
            QT3DSVec3 offsets(dataPtr[12], dataPtr[13],
                           displacementImage->m_Image.m_TextureData.m_TextureFlags.IsPreMultiplied()
                               ? 1.0f
                               : 0.0f);
            QT3DSVec4 rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);
            displacementImage->m_Image.m_TextureData.m_Texture->SetTextureWrapS(
                displacementImage->m_Image.m_HorizontalTilingMode);
            displacementImage->m_Image.m_TextureData.m_Texture->SetTextureWrapT(
                displacementImage->m_Image.m_VerticalTilingMode);

            shader->m_DisplaceAmount.Set(inDisplacementAmount);
            shader->m_DisplacementProps.m_Offsets.Set(offsets);
            shader->m_DisplacementProps.m_Rotations.Set(rotations);
            shader->m_DisplacementProps.m_Sampler.Set(
                displacementImage->m_Image.m_TextureData.m_Texture);
        }

        // tessellation
        if (m_TessellationMode != TessModeValues::NoTess) {
            // set uniforms we need
            shader->m_GlobalTransform.Set(m_GlobalTransform);

            if (m_Generator.GetLayerRenderData() && m_Generator.GetLayerRenderData()->m_Camera)
                shader->m_CameraPosition.Set(
                    m_Generator.GetLayerRenderData()->m_Camera->GetGlobalPos());
            else if (m_Generator.GetLayerRenderData()->m_Camera)
                shader->m_CameraPosition.Set(QT3DSVec3(0.0, 0.0, 1.0));

            shader->m_Tessellation.m_EdgeTessLevel.Set(m_Subset.m_EdgeTessFactor);
            shader->m_Tessellation.m_InsideTessLevel.Set(m_Subset.m_InnerTessFactor);
            // the blend value is hardcoded
            shader->m_Tessellation.m_PhongBlend.Set(0.75);
            // set distance range value
            shader->m_Tessellation.m_DistanceRange.Set(inCameraVec);
            // enable culling
            shader->m_Tessellation.m_DisableCulling.Set(0.0);
        }

        context.SetInputAssembler(pIA);
        context.Draw(m_Subset.m_PrimitiveType, m_Subset.m_Count, m_Subset.m_Offset);
    }

    // An interface to the shader generator that is available to the renderables

    void SSubsetRenderable::Render(const QT3DSVec2 &inCameraVec, TShaderFeatureSet inFeatureSet,
                                   bool depth)
    {
        NVRenderContext &context(m_Generator.GetContext());
        SShaderGeneratorGeneratedShader *shader = nullptr;

        // Do not render if alpha test is enabled, but no alpha test in object or vice versa
        if (m_Generator.alphaTestEnabled() ^ m_RenderableFlags.hasAlphaTest())
            return;

        shader = m_Generator.GetShader(*this, inFeatureSet, depth);
        if (shader == nullptr)
            return;

        context.SetActiveShader(&shader->m_Shader);

        m_Generator.GetQt3DSContext().GetDefaultMaterialShaderGenerator().SetMaterialProperties(
            shader->m_Shader, m_Material, inCameraVec, m_ModelContext.m_ModelViewProjection,
            m_ModelContext.m_NormalMatrix, m_ModelContext.m_Model.m_GlobalTransform, m_FirstImage,
            m_Opacity, m_Generator.GetLayerGlobalRenderProperties(),
            QT3DSVec2(m_Generator.alphaOpRef()));

        // tessellation
        if (m_Subset.m_PrimitiveType == NVRenderDrawMode::Patches) {
            shader->m_Tessellation.m_EdgeTessLevel.Set(m_Subset.m_EdgeTessFactor);
            shader->m_Tessellation.m_InsideTessLevel.Set(m_Subset.m_InnerTessFactor);
            // the blend value is hardcoded
            shader->m_Tessellation.m_PhongBlend.Set(0.75);
            // this should finally be based on some user input
            shader->m_Tessellation.m_DistanceRange.Set(inCameraVec);
            // enable culling
            shader->m_Tessellation.m_DisableCulling.Set(0.0);

            if (m_Subset.m_WireframeMode) {
                // we need the viewport matrix
                NVRenderRect theViewport(context.GetViewport());
                QT3DSMat44 vpMatrix;
                vpMatrix.column0 = QT3DSVec4((float)theViewport.m_Width / 2.0f, 0.0, 0.0, 0.0);
                vpMatrix.column1 = QT3DSVec4(0.0, (float)theViewport.m_Height / 2.0f, 0.0, 0.0);
                vpMatrix.column2 = QT3DSVec4(0.0, 0.0, 1.0, 0.0);
                vpMatrix.column3 =
                    QT3DSVec4((float)theViewport.m_Width / 2.0f + (float)theViewport.m_X,
                           (float)theViewport.m_Height / 2.0f + (float)theViewport.m_Y, 0.0, 1.0);

                shader->m_ViewportMatrix.Set(vpMatrix);
            }
        }

        context.SetCullingEnabled(true);
        context.SetInputAssembler(m_Subset.m_InputAssembler);
        context.Draw(m_Subset.m_PrimitiveType, m_Subset.m_Count, m_Subset.m_Offset);
    }

    void SSubsetRenderable::RenderShadow(const QT3DSVec2 &inCameraVec,
                                         TShaderFeatureSet inFeatureSet, const SLight *inLight,
                                         const SCamera &inCamera, SShadowMapEntry *inShadowMapEntry)
    {
        NVRenderContext &context(m_Generator.GetContext());
        SRenderableDepthPrepassShader *shader = nullptr;

        // Do not render if alpha test is enabled, but no alpha test in object or vice versa
        if (m_Generator.alphaTestEnabled() ^ m_RenderableFlags.hasAlphaTest())
            return;

        shader = m_Generator.GetShadowShader(*this, inFeatureSet, inLight->m_LightType);
        if (shader == nullptr || inShadowMapEntry == nullptr)
            return;

        QT3DSMat44 theModelViewProjection = inShadowMapEntry->m_LightVP * m_GlobalTransform;

        context.SetActiveShader(&shader->m_Shader);
        m_Generator.GetQt3DSContext().GetDefaultMaterialShaderGenerator().SetMaterialProperties(
            shader->m_Shader, m_Material, inCameraVec, m_ModelContext.m_ModelViewProjection,
            m_ModelContext.m_NormalMatrix, m_ModelContext.m_Model.m_GlobalTransform, m_FirstImage,
            m_Opacity, m_Generator.GetLayerGlobalRenderProperties(),
            QT3DSVec2(m_Generator.alphaOpRef()));

        shader->m_MVP.Set(theModelViewProjection);
        shader->m_CameraPosition.Set(inCamera.m_Position);
        shader->m_GlobalTransform.Set(m_GlobalTransform);
        shader->m_CameraProperties.Set(inCameraVec);

        // tessellation
        if (m_Subset.m_PrimitiveType == NVRenderDrawMode::Patches) {
            shader->m_Tessellation.m_EdgeTessLevel.Set(m_Subset.m_EdgeTessFactor);
            shader->m_Tessellation.m_InsideTessLevel.Set(m_Subset.m_InnerTessFactor);
            // the blend value is hardcoded
            shader->m_Tessellation.m_PhongBlend.Set(0.75);
            // this should finally be based on some user input
            shader->m_Tessellation.m_DistanceRange.Set(inCameraVec);
            // enable culling
            shader->m_Tessellation.m_DisableCulling.Set(0.0);
        }

        context.SetInputAssembler(m_Subset.m_InputAssembler);
        context.Draw(m_Subset.m_PrimitiveType, m_Subset.m_Count, m_Subset.m_Offset);
    }

    void SSubsetRenderable::RenderDepthPass(const QT3DSVec2 &inCameraVec)
    {
        SRenderableImage *displacementImage = nullptr;
        for (SRenderableImage *theImage = m_FirstImage;
             theImage != nullptr && displacementImage == nullptr; theImage = theImage->m_NextImage) {
            if (theImage->m_MapType == ImageMapTypes::Displacement)
                displacementImage = theImage;
        }
        SSubsetRenderableBase::RenderDepthPass(inCameraVec, displacementImage,
                                               m_Material.m_DisplaceAmount);
    }

    void STextRenderable::Render(const QT3DSVec2 &inCameraVec)
    {
        NVRenderContext &context(m_Generator.GetContext());
        if (m_Generator.alphaTestEnabled())
            return;

        if (!m_Text.m_PathFontDetails) {

            STextRenderHelper theInfo = m_Generator.GetShader(*this, false);
            if (theInfo.m_Shader == nullptr)
                return;
            // All of our shaders produce premultiplied values.
            qt3ds::render::NVRenderBlendFunctionArgument blendFunc(
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha,
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha);

            qt3ds::render::NVRenderBlendEquationArgument blendEqu(NVRenderBlendEquation::Add,
                                                               NVRenderBlendEquation::Add);

            context.SetBlendFunction(blendFunc);
            context.SetBlendEquation(blendEqu);
            QT3DSVec4 theColor(m_Text.m_TextColor.x, m_Text.m_TextColor.y, m_Text.m_TextColor.z,
                               m_Text.m_GlobalOpacity);

            STextShader &shader(*theInfo.m_Shader);
            shader.Render(*m_Text.m_TextTexture, *this, theColor, m_ModelViewProjection,
                          inCameraVec, context, theInfo.m_QuadInputAssembler,
                          theInfo.m_QuadInputAssembler.GetIndexCount(), m_Text.m_TextTextureDetails,
                          QT3DSVec3(0, 0, 0));
        } else {
            QT3DS_ASSERT(context.IsPathRenderingSupported() && context.IsProgramPipelineSupported());

            STextRenderHelper theInfo = m_Generator.GetShader(*this, true);
            if (theInfo.m_Shader == nullptr)
                return;

            // All of our shaders produce premultiplied values.
            qt3ds::render::NVRenderBlendFunctionArgument blendFunc(
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha,
                NVRenderSrcBlendFunc::One, NVRenderDstBlendFunc::OneMinusSrcAlpha);

            qt3ds::render::NVRenderBlendEquationArgument blendEqu(NVRenderBlendEquation::Add,
                                                               NVRenderBlendEquation::Add);

            context.SetBlendFunction(blendFunc);
            context.SetBlendEquation(blendEqu);
            QT3DSVec4 theColor(m_Text.m_TextColor.x, m_Text.m_TextColor.y, m_Text.m_TextColor.z,
                               m_Text.m_GlobalOpacity);
            STextShader &shader(*theInfo.m_Shader);

            shader.RenderPath(*m_Text.m_PathFontItem, *m_Text.m_PathFontDetails, *this, theColor,
                              m_ViewProjection, m_GlobalTransform, inCameraVec, context,
                              m_Text.m_TextTextureDetails, QT3DSVec3(0, 0, 0));
        }
    }

    void STextRenderable::RenderDepthPass(const QT3DSVec2 &inCameraVec)
    {
        NVRenderContext &context(m_Generator.GetContext());
        STextDepthShader *theDepthShader = m_Generator.GetTextDepthShader();
        if (theDepthShader == nullptr || m_Generator.alphaTestEnabled())
            return;

        if (!m_Text.m_PathFontDetails) {
            // we may change stencil test state
            qt3ds::render::NVRenderContextScopedProperty<bool> __stencilTest(
                context, &NVRenderContext::IsStencilTestEnabled,
                &NVRenderContext::SetStencilTestEnabled, true);

            NVRenderShaderProgram &theShader(theDepthShader->m_Shader);
            context.SetCullingEnabled(false);
            context.SetActiveShader(&theShader);
            theDepthShader->m_MVP.Set(m_ModelViewProjection);
            theDepthShader->m_Sampler.Set(m_Text.m_TextTexture);
            const STextScaleAndOffset &theScaleAndOffset(*this);
            theDepthShader->m_Dimensions.Set(
                QT3DSVec4(theScaleAndOffset.m_TextScale.x, theScaleAndOffset.m_TextScale.y,
                       theScaleAndOffset.m_TextOffset.x, theScaleAndOffset.m_TextOffset.y));
            theDepthShader->m_CameraProperties.Set(inCameraVec);

            STextureDetails theTextureDetails = m_Text.m_TextTexture->GetTextureDetails();
            const STextTextureDetails &theTextTextureDetails(m_Text.m_TextTextureDetails);
            QT3DSF32 theWidthScale =
                (QT3DSF32)theTextTextureDetails.m_TextWidth / (QT3DSF32)theTextureDetails.m_Width;
            QT3DSF32 theHeightScale =
                (QT3DSF32)theTextTextureDetails.m_TextHeight / (QT3DSF32)theTextureDetails.m_Height;
            theDepthShader->m_TextDimensions.Set(
                QT3DSVec3(theWidthScale, theHeightScale, theTextTextureDetails.m_FlipY ? 1.0f : 0.0f));
            context.SetInputAssembler(&theDepthShader->m_QuadInputAssembler);
            context.Draw(NVRenderDrawMode::Triangles,
                         theDepthShader->m_QuadInputAssembler.GetIndexCount(), 0);
        } else {
            qt3ds::render::NVRenderBoolOp::Enum theDepthFunction = context.GetDepthFunction();
            bool isDepthEnabled = context.IsDepthTestEnabled();
            bool isStencilEnabled = context.IsStencilTestEnabled();
            bool isDepthWriteEnabled = context.IsDepthWriteEnabled();
            qt3ds::render::NVRenderStencilFunctionArgument theArg(qt3ds::render::NVRenderBoolOp::NotEqual,
                                                               0, 0xFF);
            qt3ds::render::NVRenderStencilOperationArgument theOpArg(
                qt3ds::render::NVRenderStencilOp::Keep, qt3ds::render::NVRenderStencilOp::Keep,
                qt3ds::render::NVRenderStencilOp::Zero);
            NVScopedRefCounted<NVRenderDepthStencilState> depthStencilState =
                context.CreateDepthStencilState(isDepthEnabled, isDepthWriteEnabled,
                                                theDepthFunction, false, theArg, theArg, theOpArg,
                                                theOpArg);

            context.SetActiveShader(nullptr);
            context.SetCullingEnabled(false);

            context.SetDepthStencilState(depthStencilState);

            // setup transform
            QT3DSMat44 offsetMatrix = QT3DSMat44::createIdentity();
            offsetMatrix.setPosition(QT3DSVec3(
                m_TextOffset.x - (QT3DSF32)m_Text.m_TextTextureDetails.m_TextWidth / 2.0f,
                m_TextOffset.y - (QT3DSF32)m_Text.m_TextTextureDetails.m_TextHeight / 2.0f, 0.0));

            QT3DSMat44 pathMatrix = m_Text.m_PathFontItem->GetTransform();

            context.SetPathProjectionMatrix(m_ViewProjection);
            context.SetPathModelViewMatrix(m_GlobalTransform * offsetMatrix * pathMatrix);

            // first pass
            m_Text.m_PathFontDetails->StencilFillPathInstanced(*m_Text.m_PathFontItem);

            // second pass
            context.SetStencilTestEnabled(true);
            m_Text.m_PathFontDetails->CoverFillPathInstanced(*m_Text.m_PathFontItem);

            context.SetStencilTestEnabled(isStencilEnabled);
            context.SetDepthFunction(theDepthFunction);
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
    void SDistanceFieldRenderable::Render(const QT3DSVec2 &inCameraVec)
    {
        m_distanceFieldText.renderText(m_text, m_mvp);
    }

    void SDistanceFieldRenderable::RenderDepthPass(const QT3DSVec2 &inCameraVec)
    {
        m_distanceFieldText.renderTextDepth(m_text, m_mvp);
    }
#endif

    void SCustomMaterialRenderable::Render(const QT3DSVec2 & /*inCameraVec*/,
                                           const SLayerRenderData &inLayerData,
                                           const SLayer &inLayer, NVDataRef<SLight *> inLights,
                                           const SCamera &inCamera,
                                           const NVRenderTexture2D *inDepthTexture,
                                           const NVRenderTexture2D *inSsaoTexture,
                                           TShaderFeatureSet inFeatureSet)
    {
        if (m_Generator.alphaTestEnabled())
            return;
        IQt3DSRenderContext &qt3dsContext(m_Generator.GetQt3DSContext());
        SCustomMaterialRenderContext theRenderContext(
            inLayer, inLayerData, inLights, inCamera, m_ModelContext.m_Model, m_Subset,
            m_ModelContext.m_ModelViewProjection, m_GlobalTransform, m_ModelContext.m_NormalMatrix,
            m_Material, inDepthTexture, inSsaoTexture, m_ShaderDescription, m_FirstImage,
            m_Opacity);

        qt3dsContext.GetCustomMaterialSystem().RenderSubset(theRenderContext, inFeatureSet);
    }

    void SCustomMaterialRenderable::RenderDepthPass(const QT3DSVec2 &inCameraVec,
                                                    const SLayer & /*inLayer*/,
                                                    NVConstDataRef<SLight *> /*inLights*/
                                                    ,
                                                    const SCamera & /*inCamera*/,
                                                    const NVRenderTexture2D * /*inDepthTexture*/)
    {
        if (m_Generator.alphaTestEnabled())
            return;
        IQt3DSRenderContext &qt3dsContext(m_Generator.GetQt3DSContext());
        if (!qt3dsContext.GetCustomMaterialSystem().RenderDepthPrepass(
                m_ModelContext.m_ModelViewProjection, m_Material, m_Subset)) {
            SRenderableImage *displacementImage = nullptr;
            for (SRenderableImage *theImage = m_FirstImage;
                 theImage != nullptr && displacementImage == nullptr;
                 theImage = theImage->m_NextImage) {
                if (theImage->m_MapType == ImageMapTypes::Displacement)
                    displacementImage = theImage;
            }

            SSubsetRenderableBase::RenderDepthPass(inCameraVec, displacementImage,
                                                   m_Material.m_DisplaceAmount);
        }
    }

    void SPathRenderable::RenderDepthPass(const QT3DSVec2 &inCameraVec, const SLayer & /*inLayer*/,
                                          NVConstDataRef<SLight *> inLights,
                                          const SCamera &inCamera,
                                          const NVRenderTexture2D * /*inDepthTexture*/)
    {
        IQt3DSRenderContext &qt3dsContext(m_Generator.GetQt3DSContext());
        SPathRenderContext theRenderContext(
            inLights, inCamera, m_Path, m_ModelViewProjection, m_GlobalTransform, m_NormalMatrix,
            m_Opacity, m_Material, m_ShaderDescription, m_FirstImage, qt3dsContext.GetWireframeMode(),
            inCameraVec, false, m_IsStroke);

        qt3dsContext.GetPathManager().RenderDepthPrepass(
            theRenderContext, m_Generator.GetLayerGlobalRenderProperties(), TShaderFeatureSet());
    }

    void SPathRenderable::Render(const QT3DSVec2 &inCameraVec, const SLayer & /*inLayer*/,
                                 NVConstDataRef<SLight *> inLights, const SCamera &inCamera,
                                 const NVRenderTexture2D * /*inDepthTexture*/
                                 ,
                                 const NVRenderTexture2D * /*inSsaoTexture*/
                                 ,
                                 TShaderFeatureSet inFeatureSet)
    {
        IQt3DSRenderContext &qt3dsContext(m_Generator.GetQt3DSContext());
        SPathRenderContext theRenderContext(
            inLights, inCamera, m_Path, m_ModelViewProjection, m_GlobalTransform, m_NormalMatrix,
            m_Opacity, m_Material, m_ShaderDescription, m_FirstImage, qt3dsContext.GetWireframeMode(),
            inCameraVec, m_RenderableFlags.HasTransparency(), m_IsStroke);

        qt3dsContext.GetPathManager().RenderPath(
            theRenderContext, m_Generator.GetLayerGlobalRenderProperties(), inFeatureSet);
    }

    void SPathRenderable::RenderShadowMapPass(const QT3DSVec2 &inCameraVec, const SLight *inLight,
                                              const SCamera &inCamera,
                                              SShadowMapEntry *inShadowMapEntry)
    {
        NVConstDataRef<SLight *> theLights;
        IQt3DSRenderContext &qt3dsContext(m_Generator.GetQt3DSContext());

        QT3DSMat44 theModelViewProjection = inShadowMapEntry->m_LightVP * m_GlobalTransform;
        SPathRenderContext theRenderContext(
            theLights, inCamera, m_Path, theModelViewProjection, m_GlobalTransform, m_NormalMatrix,
            m_Opacity, m_Material, m_ShaderDescription, m_FirstImage, qt3dsContext.GetWireframeMode(),
            inCameraVec, false, m_IsStroke);

        if (inLight->m_LightType != RenderLightTypes::Directional) {
            qt3dsContext.GetPathManager().RenderCubeFaceShadowPass(
                theRenderContext, m_Generator.GetLayerGlobalRenderProperties(),
                TShaderFeatureSet());
        } else
            qt3dsContext.GetPathManager().RenderShadowMapPass(
                theRenderContext, m_Generator.GetLayerGlobalRenderProperties(),
                TShaderFeatureSet());
    }
}
}
