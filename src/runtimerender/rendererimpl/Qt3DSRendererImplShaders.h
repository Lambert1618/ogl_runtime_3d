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
#ifndef QT3DS_RENDERER_IMPL_SHADERS_H
#define QT3DS_RENDERER_IMPL_SHADERS_H
#include "Qt3DSRender.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderProgramPipeline.h"

namespace qt3ds {
namespace render {
    using qt3ds::render::NVRenderCachedShaderProperty;
    using qt3ds::render::NVRenderCachedShaderBuffer;

    /**
     *	Cached tessellation property lookups this is on a per mesh base
     */
    struct SShaderTessellationProperties
    {
        NVRenderCachedShaderProperty<QT3DSF32> m_EdgeTessLevel; ///< tessellation value for the edges
        NVRenderCachedShaderProperty<QT3DSF32> m_InsideTessLevel; ///< tessellation value for the inside
        NVRenderCachedShaderProperty<QT3DSF32>
            m_PhongBlend; ///< blending between linear and phong component
        NVRenderCachedShaderProperty<QT3DSVec2>
            m_DistanceRange; ///< distance range for min and max tess level
        NVRenderCachedShaderProperty<QT3DSF32> m_DisableCulling; ///< if set to 1.0 this disables
                                                              ///backface culling optimization in
                                                              ///the tess shader

        SShaderTessellationProperties() {}
        SShaderTessellationProperties(NVRenderShaderProgram &inShader)
            : m_EdgeTessLevel("tessLevelOuter", inShader)
            , m_InsideTessLevel("tessLevelInner", inShader)
            , m_PhongBlend("phongBlend", inShader)
            , m_DistanceRange("distanceRange", inShader)
            , m_DisableCulling("disableCulling", inShader)
        {
        }
    };

    /**
     *	The results of generating a shader.  Caches all possible variable names into
     *	typesafe objects.
     */
    struct SShaderGeneratorGeneratedShader
    {
        QT3DSU32 m_LayerSetIndex;
        CRegisteredString m_QueryString;
        NVRenderShaderProgram &m_Shader;
        NVRenderCachedShaderProperty<QT3DSMat44> m_ViewportMatrix;
        SShaderTessellationProperties m_Tessellation;

        SShaderGeneratorGeneratedShader(CRegisteredString inQueryString,
                                        NVRenderShaderProgram &inShader)
            : m_LayerSetIndex(QT3DS_MAX_U32)
            , m_QueryString(inQueryString)
            , m_Shader(inShader)
            , m_ViewportMatrix("viewport_matrix", inShader)
            , m_Tessellation(inShader)
        {
            m_Shader.addRef();
        }
        ~SShaderGeneratorGeneratedShader() { m_Shader.release(); }
        static QT3DSU32 GetLayerIndex(const SShaderGeneratorGeneratedShader &inShader)
        {
            return inShader.m_LayerSetIndex;
        }
        static void SetLayerIndex(SShaderGeneratorGeneratedShader &inShader, QT3DSU32 idx)
        {
            inShader.m_LayerSetIndex = idx;
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

    struct SRenderableDepthPrepassShader
    {
        NVAllocatorCallback &m_Allocator;
        NVRenderShaderProgram &m_Shader;
        NVRenderCachedShaderProperty<QT3DSMat44> m_MVP;
        NVRenderCachedShaderProperty<QT3DSMat44> m_GlobalTransform;
        NVRenderCachedShaderProperty<QT3DSMat44> m_Projection;
        NVRenderCachedShaderProperty<QT3DSVec3> m_CameraPosition;
        NVRenderCachedShaderProperty<QT3DSF32> m_DisplaceAmount;
        SShaderTextureProperties m_DisplacementProps;
        NVRenderCachedShaderProperty<QT3DSVec2> m_CameraProperties;
        NVRenderCachedShaderProperty<QT3DSVec3> m_CameraDirection;
        // NVRenderCachedShaderProperty<QT3DSMat44>	m_ShadowMV[6];

        QT3DSI32 m_RefCount;
        // Cache the tessellation property name lookups
        SShaderTessellationProperties m_Tessellation;

        SRenderableDepthPrepassShader(NVRenderShaderProgram &inShader, NVRenderContext &inContext)
            : m_Allocator(inContext.GetAllocator())
            , m_Shader(inShader)
            , m_MVP("model_view_projection", inShader)
            , m_GlobalTransform("model_matrix", inShader)
            , m_Projection("projection", inShader)
            , m_CameraPosition("camera_position", inShader)
            , m_DisplaceAmount("displaceAmount", inShader)
            , m_DisplacementProps("displacementSampler", "displacementMap_offset",
                                  "displacementMap_rot", inShader)
            , m_CameraProperties("camera_properties", inShader)
            , m_CameraDirection("camera_direction", inShader)
            , m_RefCount(0)
            , m_Tessellation(inShader)
        {
            /*
            m_ShadowMV[0].m_Shader = &inShader;
            m_ShadowMV[0].m_Constant = inShader.GetShaderConstant( "shadow_mv0" );
            m_ShadowMV[1].m_Shader = &inShader;
            m_ShadowMV[1].m_Constant = inShader.GetShaderConstant( "shadow_mv1" );
            m_ShadowMV[2].m_Shader = &inShader;
            m_ShadowMV[2].m_Constant = inShader.GetShaderConstant( "shadow_mv2" );
            m_ShadowMV[3].m_Shader = &inShader;
            m_ShadowMV[3].m_Constant = inShader.GetShaderConstant( "shadow_mv3" );
            m_ShadowMV[4].m_Shader = &inShader;
            m_ShadowMV[4].m_Constant = inShader.GetShaderConstant( "shadow_mv4" );
            m_ShadowMV[5].m_Shader = &inShader;
            m_ShadowMV[5].m_Constant = inShader.GetShaderConstant( "shadow_mv5" );
            */
            m_Shader.addRef();
        }

        ~SRenderableDepthPrepassShader() { m_Shader.release(); }

        void addRef() { ++m_RefCount; }
        void release()
        {
            --m_RefCount;
            if (m_RefCount <= 0)
                NVDelete(m_Allocator, this);
        }
    };

    struct SDefaultAoPassShader
    {
        NVAllocatorCallback &m_Allocator;
        NVRenderShaderProgram &m_Shader;
        NVRenderCachedShaderProperty<QT3DSMat44> m_ViewMatrix;
        NVRenderCachedShaderProperty<QT3DSVec2> m_CameraProperties;
        NVRenderCachedShaderProperty<QT3DSVec3> m_CameraDirection;
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_DepthTexture;
        NVRenderCachedShaderProperty<NVRenderTextureCube *> m_CubeTexture;
        NVRenderCachedShaderProperty<QT3DSVec2> m_DepthSamplerSize;

        NVRenderCachedShaderBuffer<qt3ds::render::NVRenderShaderConstantBuffer *> m_AoShadowParams;
        QT3DSI32 m_RefCount;

        SDefaultAoPassShader(NVRenderShaderProgram &inShader, NVRenderContext &inContext)
            : m_Allocator(inContext.GetAllocator())
            , m_Shader(inShader)
            , m_ViewMatrix("view_matrix", inShader)
            , m_CameraProperties("camera_properties", inShader)
            , m_CameraDirection("camera_direction", inShader)
            , m_DepthTexture("depth_sampler", inShader)
            , m_CubeTexture("depth_cube", inShader)
            , m_DepthSamplerSize("depth_sampler_size", inShader)
            , m_AoShadowParams("cbAoShadow", inShader)
            , m_RefCount(0)
        {
            m_Shader.addRef();
        }
        ~SDefaultAoPassShader() { m_Shader.release(); }

        void addRef() { ++m_RefCount; }
        void release()
        {
            --m_RefCount;
            if (m_RefCount <= 0)
                NVDelete(m_Allocator, this);
        }
    };

    struct STextShader
    {
        NVRenderShaderProgram &m_Shader;

        NVScopedRefCounted<NVRenderProgramPipeline> m_ProgramPipeline;

        NVRenderCachedShaderProperty<QT3DSMat44> m_MVP;
        // Dimensions and offsetting of the image.
        NVRenderCachedShaderProperty<QT3DSVec4> m_Dimensions;
        // The fourth member of text color is the opacity
        NVRenderCachedShaderProperty<QT3DSVec4> m_TextColor;
        NVRenderCachedShaderProperty<QT3DSVec3> m_BackgroundColor;
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_Sampler;
        // Dimensions and offsetting of the texture
        NVRenderCachedShaderProperty<QT3DSVec3> m_TextDimensions;
        NVRenderCachedShaderProperty<QT3DSVec2> m_CameraProperties;
        // Used only for onscreen text
        NVRenderCachedShaderProperty<QT3DSVec2> m_VertexOffsets;

        STextShader(NVRenderShaderProgram &shader, NVRenderProgramPipeline *pipeline = NULL)
            : m_Shader(shader)
            , m_ProgramPipeline(pipeline)
            , m_MVP("model_view_projection", shader)
            , m_Dimensions("text_dimensions", shader)
            , m_TextColor("text_textcolor", shader)
            , m_BackgroundColor("text_backgroundcolor", shader)
            , m_Sampler("text_image", shader)
            , m_TextDimensions("text_textdimensions", shader)
            , m_CameraProperties("camera_properties", shader)
            , m_VertexOffsets("vertex_offsets", shader)
        {
            if (!pipeline)
                m_Shader.addRef();
        }
        ~STextShader()
        {
            if (!m_ProgramPipeline.mPtr)
                m_Shader.release();
        }
        void Render(NVRenderTexture2D &inTexture, const STextScaleAndOffset &inScaleAndOffset,
                    const QT3DSVec4 &inTextColor, const QT3DSMat44 &inMVP, const QT3DSVec2 &inCameraVec,
                    NVRenderContext &inRenderContext,
                    NVRenderInputAssembler &inInputAssemblerBuffer, QT3DSU32 count,
                    const STextTextureDetails &inTextTextureDetails,
                    const QT3DSVec3 &inBackgroundColor);

        void RenderPath(NVRenderPathFontItem &inPathFontItem,
                        NVRenderPathFontSpecification &inPathFontSpec,
                        const STextScaleAndOffset &inScaleAndOffset, const QT3DSVec4 &inTextColor,
                        const QT3DSMat44 &inViewProjection, const QT3DSMat44 &inModel,
                        const QT3DSVec2 &inCameraVec, NVRenderContext &inRenderContext,
                        const STextTextureDetails &inTextTextureDetails,
                        const QT3DSVec3 &inBackgroundColor);

        void Render2D(NVRenderTexture2D &inTexture, const QT3DSVec4 &inTextColor, const QT3DSMat44 &inMVP,
                      NVRenderContext &inRenderContext,
                      NVRenderInputAssembler &inInputAssemblerBuffer, QT3DSU32 count,
                      QT3DSVec2 inVertexOffsets);
    };

    struct STextDepthShader
    {
        NVAllocatorCallback &m_Allocator;
        NVRenderShaderProgram &m_Shader;
        NVRenderCachedShaderProperty<QT3DSMat44> m_MVP;
        // Dimensions and offsetting of the image.
        NVRenderCachedShaderProperty<QT3DSVec4> m_Dimensions;
        NVRenderCachedShaderProperty<QT3DSVec3> m_TextDimensions;
        NVRenderCachedShaderProperty<QT3DSVec2> m_CameraProperties;
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_Sampler;
        NVRenderInputAssembler &m_QuadInputAssembler;
        QT3DSI32 m_RefCount;

        STextDepthShader(NVAllocatorCallback &alloc, NVRenderShaderProgram &prog,
                         NVRenderInputAssembler &assembler)
            : m_Allocator(alloc)
            , m_Shader(prog)
            , m_MVP("model_view_projection", prog)
            , m_Dimensions("text_dimensions", prog)
            , m_TextDimensions("text_textdimensions", prog)
            , m_CameraProperties("camera_properties", prog)
            , m_Sampler("text_image", prog)
            , m_QuadInputAssembler(assembler)
            , m_RefCount(0)
        {
            m_Shader.addRef();
        }
        ~STextDepthShader() { m_Shader.release(); }
        void addRef() { ++m_RefCount; }
        void release()
        {
            --m_RefCount;
            if (m_RefCount <= 0)
                NVDelete(m_Allocator, this);
        }
    };

    struct SLayerProgAABlendShader
    {
        NVScopedRefCounted<NVRenderShaderProgram> m_Shader;
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_AccumSampler;
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_LastFrame;
        NVRenderCachedShaderProperty<QT3DSVec2> m_BlendFactors;
        volatile QT3DSI32 mRefCount;
        SLayerProgAABlendShader(NVRenderShaderProgram &inShader)
            : m_Shader(inShader)
            , m_AccumSampler("accumulator", inShader)
            , m_LastFrame("last_frame", inShader)
            , m_BlendFactors("blend_factors", inShader)
            , mRefCount(0)
        {
        }
        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader->GetRenderContext().GetAllocator())
    };

    struct SLayerSceneShader
    {
        NVRenderShaderProgram &m_Shader;

        NVRenderCachedShaderProperty<QT3DSMat44> m_MVP;
        // Dimensions and offsetting of the image.
        NVRenderCachedShaderProperty<QT3DSVec2> m_Dimensions;
        // The fourth member of text color is the opacity
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_Sampler;

        volatile QT3DSI32 mRefCount;

        SLayerSceneShader(NVRenderShaderProgram &inShader)
            : m_Shader(inShader)
            , m_MVP("model_view_projection", inShader)
            , m_Dimensions("layer_dimensions", inShader)
            , m_Sampler("layer_image", inShader)
            , mRefCount(0)
        {
            m_Shader.addRef();
        }
        ~SLayerSceneShader() { m_Shader.release(); }

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader.GetRenderContext().GetAllocator())
    };

    struct SShadowmapPreblurShader
    {
        NVRenderShaderProgram &m_Shader;
        NVRenderCachedShaderProperty<QT3DSVec2> m_CameraProperties;
        NVRenderCachedShaderProperty<NVRenderTextureCube *> m_DepthCube;
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_DepthMap;

        volatile QT3DSI32 mRefCount;

        SShadowmapPreblurShader(NVRenderShaderProgram &inShader)
            : m_Shader(inShader)
            , m_CameraProperties("camera_properties", inShader)
            , m_DepthCube("depthCube", inShader)
            , m_DepthMap("depthSrc", inShader)
            , mRefCount(0)
        {
            m_Shader.addRef();
        }
        ~SShadowmapPreblurShader() { m_Shader.release(); }

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader.GetRenderContext().GetAllocator())
    };

#ifdef ADVANCED_BLEND_SW_FALLBACK
    struct SAdvancedModeBlendShader
    {
        NVRenderShaderProgram &m_Shader;
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_baseLayer;
        NVRenderCachedShaderProperty<NVRenderTexture2D *> m_blendLayer;

        volatile QT3DSI32 mRefCount;

        SAdvancedModeBlendShader(NVRenderShaderProgram &inShader)
            : m_Shader(inShader)
            , m_baseLayer("base_layer", inShader)
            , m_blendLayer("blend_layer", inShader)
            , mRefCount(0)
        {
            m_Shader.addRef();
        }
        ~SAdvancedModeBlendShader() { m_Shader.release(); }

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader.GetRenderContext().GetAllocator())

    };
#endif

    struct SGGSGet
    {
        QT3DSU32 operator()(const SShaderGeneratorGeneratedShader &inShader)
        {
            return inShader.m_LayerSetIndex;
        }
    };
    struct SGGSSet
    {
        void operator()(SShaderGeneratorGeneratedShader &inShader, QT3DSU32 idx)
        {
            inShader.m_LayerSetIndex = idx;
        }
    };
}
}
#endif
