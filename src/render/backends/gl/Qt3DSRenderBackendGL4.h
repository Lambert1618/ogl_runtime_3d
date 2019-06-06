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
#ifndef QT3DS_RENDER_BACKEND_GL4_H
#define QT3DS_RENDER_BACKEND_GL4_H

/// @file Qt3DSRenderBackendGL4.h
///       NVRender OpenGL 4 backend definition.

#include "foundation/Qt3DSAtomic.h"
#include "render/backends/gl/Qt3DSRenderBackendGL3.h"

namespace qt3ds {
namespace render {

    using namespace foundation;

    class NVRenderBackendGL4Impl : public NVRenderBackendGL3Impl
    {
    public:
        /// constructor
        NVRenderBackendGL4Impl(NVFoundationBase &fnd,
            qt3ds::foundation::IStringTable &stringTable,
            const QSurfaceFormat &format);
        /// destructor
        virtual ~NVRenderBackendGL4Impl();

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation)

    public:
        void DrawIndirect(NVRenderDrawMode::Enum drawMode, const void *indirect) override;
        void DrawIndexedIndirect(NVRenderDrawMode::Enum drawMode,
                                         NVRenderComponentTypes::Enum type, const void *indirect) override;

        void CreateTextureStorage2D(NVRenderBackendTextureObject to,
                                            NVRenderTextureTargetType::Enum target, QT3DSU32 levels,
                                            NVRenderTextureFormats::Enum internalFormat,
                                            size_t width, size_t height) override;

        void SetMultisampledTextureData2D(NVRenderBackendTextureObject to,
                                                  NVRenderTextureTargetType::Enum target,
                                                  size_t samples,
                                                  NVRenderTextureFormats::Enum internalFormat,
                                                  size_t width, size_t height,
                                                  bool fixedsamplelocations) override;

        void SetConstantValue(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                              NVRenderShaderDataTypes::Enum type, QT3DSI32 count, const void *value,
                              bool transpose) override;

        void SetPatchVertexCount(NVRenderBackendInputAssemblerObject iao, QT3DSU32 count) override;
        virtual NVRenderBackendTessControlShaderObject
        CreateTessControlShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage,
                                bool binary) override;
        virtual NVRenderBackendTessEvaluationShaderObject
        CreateTessEvaluationShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage,
                                   bool binary) override;
        virtual NVRenderBackendGeometryShaderObject
        CreateGeometryShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage, bool binary) override;

        QT3DSI32 GetStorageBufferCount(NVRenderBackendShaderProgramObject po) override;
        QT3DSI32 GetStorageBufferInfoByID(NVRenderBackendShaderProgramObject po, QT3DSU32 id,
                                               QT3DSU32 nameBufSize, QT3DSI32 *paramCount,
                                               QT3DSI32 *bufferSize, QT3DSI32 *length, char *nameBuf) override;
        void ProgramSetStorageBuffer(QT3DSU32 index, NVRenderBackendBufferObject bo) override;

        QT3DSI32 GetAtomicCounterBufferCount(NVRenderBackendShaderProgramObject po) override;
        QT3DSI32 GetAtomicCounterBufferInfoByID(NVRenderBackendShaderProgramObject po,
                                                     QT3DSU32 id, QT3DSU32 nameBufSize, QT3DSI32 *paramCount,
                                                     QT3DSI32 *bufferSize, QT3DSI32 *length,
                                                     char *nameBuf) override;
        void ProgramSetAtomicCounterBuffer(QT3DSU32 index, NVRenderBackendBufferObject bo) override;

        void SetMemoryBarrier(NVRenderBufferBarrierFlags barriers) override;
        void BindImageTexture(NVRenderBackendTextureObject to, QT3DSU32 unit, QT3DSI32 level,
                                      bool layered, QT3DSI32 layer,
                                      NVRenderImageAccessType::Enum access,
                                      NVRenderTextureFormats::Enum format) override;

        virtual NVRenderBackendComputeShaderObject
        CreateComputeShader(NVConstDataRef<QT3DSI8> source, eastl::string &errorMessage, bool binary) override;
        void DispatchCompute(NVRenderBackendShaderProgramObject po, QT3DSU32 numGroupsX,
                                     QT3DSU32 numGroupsY, QT3DSU32 numGroupsZ) override;

        NVRenderBackendProgramPipeline CreateProgramPipeline() override;
        void ReleaseProgramPipeline(NVRenderBackendProgramPipeline ppo) override;
        void SetActiveProgramPipeline(NVRenderBackendProgramPipeline ppo) override;
        void SetProgramStages(NVRenderBackendProgramPipeline ppo,
                                      NVRenderShaderTypeFlags flags,
                                      NVRenderBackendShaderProgramObject po) override;

        void SetBlendEquation(const NVRenderBlendEquationArgument &pBlendEquArg) override;
        void SetBlendBarrier(void) override;

        NVRenderBackendPathObject CreatePathNVObject(size_t range) override;
        void SetPathSpecification(NVRenderBackendPathObject inPathObject,
                                          NVConstDataRef<QT3DSU8> inPathCommands,
                                          NVConstDataRef<QT3DSF32> inPathCoords) override;
        NVBounds3 GetPathObjectBoundingBox(NVRenderBackendPathObject inPathObject) override;
        NVBounds3 GetPathObjectFillBox(NVRenderBackendPathObject inPathObject) override;
        NVBounds3 GetPathObjectStrokeBox(NVRenderBackendPathObject inPathObject) override;
        void SetStrokeWidth(NVRenderBackendPathObject inPathObject, QT3DSF32 inStrokeWidth) override;

        void SetPathProjectionMatrix(const QT3DSMat44 inPathProjection) override;
        void SetPathModelViewMatrix(const QT3DSMat44 inPathModelview) override;
        void SetPathStencilDepthOffset(QT3DSF32 inSlope, QT3DSF32 inBias) override;
        void SetPathCoverDepthFunc(NVRenderBoolOp::Enum inDepthFunction) override;
        void StencilStrokePath(NVRenderBackendPathObject inPathObject) override;
        void StencilFillPath(NVRenderBackendPathObject inPathObject) override;
        void ReleasePathNVObject(NVRenderBackendPathObject po, size_t range) override;

        void StencilFillPathInstanced(
            NVRenderBackendPathObject po, size_t numPaths, NVRenderPathFormatType::Enum type,
            const void *charCodes, NVRenderPathFillMode::Enum fillMode, QT3DSU32 stencilMask,
            NVRenderPathTransformType::Enum transformType, const QT3DSF32 *transformValues) override;
        void StencilStrokePathInstancedN(NVRenderBackendPathObject po, size_t numPaths,
                                                 NVRenderPathFormatType::Enum type,
                                                 const void *charCodes, QT3DSI32 stencilRef,
                                                 QT3DSU32 stencilMask,
                                                 NVRenderPathTransformType::Enum transformType,
                                                 const QT3DSF32 *transformValues) override;
        void CoverFillPathInstanced(NVRenderBackendPathObject po, size_t numPaths,
                                            NVRenderPathFormatType::Enum type,
                                            const void *charCodes,
                                            NVRenderPathCoverMode::Enum coverMode,
                                            NVRenderPathTransformType::Enum transformType,
                                            const QT3DSF32 *transformValues) override;
        void CoverStrokePathInstanced(NVRenderBackendPathObject po, size_t numPaths,
                                              NVRenderPathFormatType::Enum type,
                                              const void *charCodes,
                                              NVRenderPathCoverMode::Enum coverMode,
                                              NVRenderPathTransformType::Enum transformType,
                                              const QT3DSF32 *transformValues) override;
        void LoadPathGlyphs(NVRenderBackendPathObject po,
                                    NVRenderPathFontTarget::Enum fontTarget, const void *fontName,
                                    NVRenderPathFontStyleFlags fontStyle, size_t numGlyphs,
                                    NVRenderPathFormatType::Enum type, const void *charCodes,
                                    NVRenderPathMissingGlyphs::Enum handleMissingGlyphs,
                                    NVRenderBackendPathObject pathParameterTemplate, QT3DSF32 emScale) override;
        virtual NVRenderPathReturnValues::Enum
        LoadPathGlyphsIndexed(NVRenderBackendPathObject po, NVRenderPathFontTarget::Enum fontTarget,
                              const void *fontName, NVRenderPathFontStyleFlags fontStyle,
                              QT3DSU32 firstGlyphIndex, size_t numGlyphs,
                              NVRenderBackendPathObject pathParameterTemplate, QT3DSF32 emScale) override;
        virtual NVRenderBackendPathObject
        LoadPathGlyphsIndexedRange(NVRenderPathFontTarget::Enum fontTarget, const void *fontName,
                                   NVRenderPathFontStyleFlags fontStyle,
                                   NVRenderBackendPathObject pathParameterTemplate, QT3DSF32 emScale,
                                   QT3DSU32 *count) override;
        void LoadPathGlyphRange(NVRenderBackendPathObject po,
                                        NVRenderPathFontTarget::Enum fontTarget,
                                        const void *fontName, NVRenderPathFontStyleFlags fontStyle,
                                        QT3DSU32 firstGlyph, size_t numGlyphs,
                                        NVRenderPathMissingGlyphs::Enum handleMissingGlyphs,
                                        NVRenderBackendPathObject pathParameterTemplate,
                                        QT3DSF32 emScale) override;
        void GetPathMetrics(NVRenderBackendPathObject po, size_t numPaths,
                                    NVRenderPathGlyphFontMetricFlags metricQueryMask,
                                    NVRenderPathFormatType::Enum type, const void *charCodes,
                                    size_t stride, QT3DSF32 *metrics) override;
        void GetPathMetricsRange(NVRenderBackendPathObject po, size_t numPaths,
                                         NVRenderPathGlyphFontMetricFlags metricQueryMask,
                                         size_t stride, QT3DSF32 *metrics) override;
        void GetPathSpacing(NVRenderBackendPathObject po, size_t numPaths,
                                    NVRenderPathListMode::Enum pathListMode,
                                    NVRenderPathFormatType::Enum type, const void *charCodes,
                                    QT3DSF32 advanceScale, QT3DSF32 kerningScale,
                                    NVRenderPathTransformType::Enum transformType, QT3DSF32 *spacing) override;
    private:
#if !defined(QT_OPENGL_ES)
        QOpenGLExtension_NV_path_rendering *m_nvPathRendering;
        QOpenGLExtension_EXT_direct_state_access *m_directStateAccess;
#endif
    };
}
}

#endif
