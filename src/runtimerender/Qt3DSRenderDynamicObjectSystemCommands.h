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
#ifndef QT3DS_RENDER_EFFECT_SYSTEM_COMMANDS_H
#define QT3DS_RENDER_EFFECT_SYSTEM_COMMANDS_H
#include "Qt3DSRender.h"
#include "foundation/StringTable.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSFlags.h"

namespace qt3ds {
namespace render {
    namespace dynamic {
        using qt3ds::render::NVRenderBufferBarrierFlags;

        struct CommandTypes
        {
            enum Enum {
                Unknown = 0,
                AllocateBuffer,
                BindTarget,
                BindBuffer,
                BindShader,
                ApplyInstanceValue,
                ApplyBufferValue,
                // Apply the depth buffer as an input texture.
                ApplyDepthValue,
                Render, // Render to current FBO
                ApplyDepth,
                ApplyBlending,
                ApplyRenderState, // apply a render state
                ApplyBlitFramebuffer,
                ApplyValue,
                DepthStencil,
                AllocateImage,
                ApplyImageValue,
                AllocateDataBuffer,
                ApplyDataBufferValue,
                ApplyCulling
            };
        };

#define QT3DS_RENDER_EFFECTS_ITERATE_COMMAND_TYPES                                                   \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(AllocateBuffer)                                        \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(BindTarget)                                            \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(BindBuffer)                                            \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(BindShader)                                            \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyInstanceValue)                                    \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyBufferValue)                                      \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyDepthValue)                                       \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(Render)                                                \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyBlending)                                         \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyRenderState)                                      \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyBlitFramebuffer)                                  \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyValue)                                            \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(DepthStencil)                                          \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(AllocateImage)                                         \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyImageValue)                                       \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(AllocateDataBuffer)                                    \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyDataBufferValue)                                  \
    QT3DS_RENDER_EFFECTS_HANDLE_COMMAND_TYPES(ApplyCulling)

        // All commands need at least two constructors.  One for when they are created that should
        // setup all their member variables and one for when we are copying commands from an outside
        // entity into the effect system.  We have to re-register strings in that case because we
        // can't assume the outside entity was using the same string table we are...
        struct SCommand
        {
            CommandTypes::Enum m_Type;
            SCommand(CommandTypes::Enum inType)
                : m_Type(inType)
            {
            }
            SCommand()
                : m_Type(CommandTypes::Unknown)
            {
            }
            // Implemented in UICRenderEffectSystem.cpp
            static QT3DSU32 GetSizeofCommand(const SCommand &inCommand);
            static void CopyConstructCommand(QT3DSU8 *inDataBuffer, const SCommand &inCommand,
                                             IStringTable &inStrTable);
        };

        struct AllocateBufferFlagValues
        {
            enum Enum {
                SceneLifetime = 1,
            };
        };

        struct SAllocateBufferFlags : public NVFlags<AllocateBufferFlagValues::Enum, QT3DSU32>
        {
            SAllocateBufferFlags(QT3DSU32 inValues)
                : NVFlags<AllocateBufferFlagValues::Enum, QT3DSU32>(inValues)
            {
            }
            SAllocateBufferFlags() {}
            void SetSceneLifetime(bool inValue)
            {
                clearOrSet(inValue, AllocateBufferFlagValues::SceneLifetime);
            }
            // If isSceneLifetime is unset the buffer is assumed to be frame lifetime and will be
            // released after this render operation.
            bool IsSceneLifetime() const
            {
                return this->operator&(AllocateBufferFlagValues::SceneLifetime);
            }
        };

        struct SAllocateBuffer : public SCommand
        {
            CRegisteredString m_Name;
            NVRenderTextureFormats::Enum m_Format;
            NVRenderTextureMagnifyingOp::Enum m_FilterOp;
            NVRenderTextureCoordOp::Enum m_TexCoordOp;
            QT3DSF32 m_SizeMultiplier;
            SAllocateBufferFlags m_BufferFlags;
            SAllocateBuffer()
                : SCommand(CommandTypes::AllocateBuffer)
                , m_Format(NVRenderTextureFormats::RGBA8)
                , m_FilterOp(NVRenderTextureMagnifyingOp::Linear)
                , m_TexCoordOp(NVRenderTextureCoordOp::ClampToEdge)
                , m_SizeMultiplier(1.0f)
            {
            }
            SAllocateBuffer(CRegisteredString inName, NVRenderTextureFormats::Enum inFormat,
                            NVRenderTextureMagnifyingOp::Enum inFilterOp,
                            NVRenderTextureCoordOp::Enum inCoordOp, QT3DSF32 inMultiplier,
                            SAllocateBufferFlags inFlags)
                : SCommand(CommandTypes::AllocateBuffer)
                , m_Name(inName)
                , m_Format(inFormat)
                , m_FilterOp(inFilterOp)
                , m_TexCoordOp(inCoordOp)
                , m_SizeMultiplier(inMultiplier)
                , m_BufferFlags(inFlags)
            {
            }
            SAllocateBuffer(const SAllocateBuffer &inOther, IStringTable &inStrTable)
                : SCommand(CommandTypes::AllocateBuffer)
                , m_Name(inStrTable.RegisterStr(inOther.m_Name))
                , m_Format(inOther.m_Format)
                , m_FilterOp(inOther.m_FilterOp)
                , m_TexCoordOp(inOther.m_TexCoordOp)
                , m_SizeMultiplier(inOther.m_SizeMultiplier)
                , m_BufferFlags(inOther.m_BufferFlags)
            {
            }
        };

        struct SAllocateImage : public SAllocateBuffer
        {
            NVRenderImageAccessType::Enum m_Access;

            SAllocateImage()
                : SAllocateBuffer()
                , m_Access(NVRenderImageAccessType::ReadWrite)
            {
                m_Type = CommandTypes::AllocateImage;
            }
            SAllocateImage(CRegisteredString inName, NVRenderTextureFormats::Enum inFormat,
                           NVRenderTextureMagnifyingOp::Enum inFilterOp,
                           NVRenderTextureCoordOp::Enum inCoordOp, QT3DSF32 inMultiplier,
                           SAllocateBufferFlags inFlags, NVRenderImageAccessType::Enum inAccess)
                : SAllocateBuffer(inName, inFormat, inFilterOp, inCoordOp, inMultiplier, inFlags)
                , m_Access(inAccess)
            {
                m_Type = CommandTypes::AllocateImage;
            }

            SAllocateImage(const SAllocateImage &inOther, IStringTable &inStrTable)
                : SAllocateBuffer(inStrTable.RegisterStr(inOther.m_Name), inOther.m_Format,
                                  inOther.m_FilterOp, inOther.m_TexCoordOp,
                                  inOther.m_SizeMultiplier, inOther.m_BufferFlags)
                , m_Access(inOther.m_Access)
            {
                m_Type = CommandTypes::AllocateImage;
            }
        };

        struct SAllocateDataBuffer : public SCommand
        {
            CRegisteredString m_Name;
            NVRenderBufferBindValues::Enum m_DataBufferType;
            CRegisteredString m_WrapName;
            NVRenderBufferBindValues::Enum m_DataBufferWrapType;
            QT3DSF32 m_Size;
            SAllocateBufferFlags m_BufferFlags;

            SAllocateDataBuffer()
                : SCommand(CommandTypes::AllocateDataBuffer)
            {
            }

            SAllocateDataBuffer(CRegisteredString inName,
                                NVRenderBufferBindValues::Enum inBufferType,
                                CRegisteredString inWrapName,
                                NVRenderBufferBindValues::Enum inBufferWrapType, QT3DSF32 inSize,
                                SAllocateBufferFlags inFlags)
                : SCommand(CommandTypes::AllocateDataBuffer)
                , m_Name(inName)
                , m_DataBufferType(inBufferType)
                , m_WrapName(inWrapName)
                , m_DataBufferWrapType(inBufferWrapType)
                , m_Size(inSize)
                , m_BufferFlags(inFlags)
            {
            }

            SAllocateDataBuffer(const SAllocateDataBuffer &inOther, IStringTable &inStrTable)
                : SCommand(CommandTypes::AllocateDataBuffer)
                , m_Name(inStrTable.RegisterStr(inOther.m_Name))
                , m_DataBufferType(inOther.m_DataBufferType)
                , m_WrapName(inStrTable.RegisterStr(inOther.m_WrapName))
                , m_DataBufferWrapType(inOther.m_DataBufferWrapType)
                , m_Size(inOther.m_Size)
                , m_BufferFlags(inOther.m_BufferFlags)
            {
            }
        };

        struct SBindTarget : public SCommand
        {
            NVRenderTextureFormats::Enum m_OutputFormat;

            SBindTarget(NVRenderTextureFormats::Enum inFormat = NVRenderTextureFormats::RGBA8)
                : SCommand(CommandTypes::BindTarget)
                , m_OutputFormat(inFormat)
            {
            }
            SBindTarget(const SBindTarget &inOther, IStringTable &)
                : SCommand(CommandTypes::BindTarget)
                , m_OutputFormat(inOther.m_OutputFormat)
            {
            }
        };

        struct SBindBuffer : public SCommand
        {
            CRegisteredString m_BufferName;
            bool m_NeedsClear;
            SBindBuffer(CRegisteredString inBufName, bool inNeedsClear)
                : SCommand(CommandTypes::BindBuffer)
                , m_BufferName(inBufName)
                , m_NeedsClear(inNeedsClear)
            {
            }
            SBindBuffer(const SBindBuffer &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::BindBuffer)
                , m_BufferName(inTable.RegisterStr(inOther.m_BufferName))
                , m_NeedsClear(inOther.m_NeedsClear)
            {
            }
        };

        struct SBindShader : public SCommand
        {
            CRegisteredString m_ShaderPath;
            // One GLSL file can hold multiple shaders in the case of multipass effects.
            // This makes it significantly easier for authors to reason about the shader
            // but it means we need to #define a preprocessor token to indicate which
            // effect we intend to compile at this point.
            CRegisteredString m_ShaderDefine;
            SBindShader(CRegisteredString inShaderPath,
                        CRegisteredString inShaderDefine = CRegisteredString())
                : SCommand(CommandTypes::BindShader)
                , m_ShaderPath(inShaderPath)
                , m_ShaderDefine(inShaderDefine)
            {
            }
            SBindShader()
                : SCommand(CommandTypes::BindShader)
            {
            }
            SBindShader(const SBindShader &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::BindShader)
                , m_ShaderPath(inTable.RegisterStr(inOther.m_ShaderPath))
                , m_ShaderDefine(inTable.RegisterStr(inOther.m_ShaderDefine))
            {
            }
        };

        // The value sits immediately after the 'this' object
        // in memory.
        // If propertyName is not valid then we attempt to apply all of the effect property values
        // to the shader, ignoring ones that don't match up.
        struct SApplyInstanceValue : public SCommand
        {
            // Name of value to apply in shader
            CRegisteredString m_PropertyName;
            // type of value
            NVRenderShaderDataTypes::Enum m_ValueType;
            // offset in the effect data section of value.
            QT3DSU32 m_ValueOffset;
            SApplyInstanceValue(CRegisteredString inName, NVRenderShaderDataTypes::Enum inValueType,
                                QT3DSU32 inValueOffset)
                : SCommand(CommandTypes::ApplyInstanceValue)
                , m_PropertyName(inName)
                , m_ValueType(inValueType)
                , m_ValueOffset(inValueOffset)
            {
            }
            // Default will attempt to apply all effect values to the currently bound shader
            SApplyInstanceValue()
                : SCommand(CommandTypes::ApplyInstanceValue)
                , m_ValueType(NVRenderShaderDataTypes::Unknown)
                , m_ValueOffset(0)
            {
            }
            SApplyInstanceValue(const SApplyInstanceValue &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::ApplyInstanceValue)
                , m_PropertyName(inTable.RegisterStr(inOther.m_PropertyName))
                , m_ValueType(inOther.m_ValueType)
                , m_ValueOffset(inOther.m_ValueOffset)
            {
            }
        };

        struct SApplyValue : public SCommand
        {
            CRegisteredString m_PropertyName;
            NVRenderShaderDataTypes::Enum m_ValueType;
            NVDataRef<QT3DSU8> m_Value;
            SApplyValue(CRegisteredString inName, NVRenderShaderDataTypes::Enum inValueType)
                : SCommand(CommandTypes::ApplyValue)
                , m_PropertyName(inName)
                , m_ValueType(inValueType)
            {
            }
            // Default will attempt to apply all effect values to the currently bound shader
            SApplyValue()
                : SCommand(CommandTypes::ApplyValue)
                , m_ValueType(NVRenderShaderDataTypes::Unknown)
            {
            }

            SApplyValue(const SApplyValue &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::ApplyValue)
                , m_PropertyName(inTable.RegisterStr(inOther.m_PropertyName))
                , m_ValueType(inOther.m_ValueType)
                , m_Value(inOther.m_Value)
            {
            }
        };

        // bind a buffer to a given shader parameter.
        struct SApplyBufferValue : public SCommand
        {
            // If no buffer name is given then the special buffer [source]
            // is assumed.
            CRegisteredString m_BufferName;
            // If no param name is given, the buffer is bound to the
            // input texture parameter (texture0).
            CRegisteredString m_ParamName;

            SApplyBufferValue(CRegisteredString bufferName, CRegisteredString shaderParam)
                : SCommand(CommandTypes::ApplyBufferValue)
                , m_BufferName(bufferName)
                , m_ParamName(shaderParam)
            {
            }
            SApplyBufferValue(const SApplyBufferValue &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::ApplyBufferValue)
                , m_BufferName(inTable.RegisterStr(inOther.m_BufferName))
                , m_ParamName(inTable.RegisterStr(inOther.m_ParamName))
            {
            }
        };

        // bind a buffer to a given shader parameter.
        struct SApplyImageValue : public SCommand
        {
            CRegisteredString m_ImageName; ///< name which the image was allocated
            CRegisteredString m_ParamName; ///< must match the name in the shader
            bool m_BindAsTexture; ///< bind image as texture
            bool m_NeedSync; ///< if true we add a memory barrier before usage

            SApplyImageValue(CRegisteredString bufferName, CRegisteredString shaderParam,
                             bool inBindAsTexture, bool inNeedSync)
                : SCommand(CommandTypes::ApplyImageValue)
                , m_ImageName(bufferName)
                , m_ParamName(shaderParam)
                , m_BindAsTexture(inBindAsTexture)
                , m_NeedSync(inNeedSync)
            {
            }
            SApplyImageValue(const SApplyImageValue &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::ApplyImageValue)
                , m_ImageName(inTable.RegisterStr(inOther.m_ImageName))
                , m_ParamName(inTable.RegisterStr(inOther.m_ParamName))
                , m_BindAsTexture(inOther.m_BindAsTexture)
                , m_NeedSync(inOther.m_NeedSync)
            {
            }
        };

        // bind a buffer to a given shader parameter.
        struct SApplyDataBufferValue : public SCommand
        {
            CRegisteredString m_ParamName; ///< must match the name in the shader
            NVRenderBufferBindValues::Enum m_BindAs; ///< to which target we bind this buffer

            SApplyDataBufferValue(CRegisteredString inShaderParam,
                                  NVRenderBufferBindValues::Enum inBufferType)
                : SCommand(CommandTypes::ApplyDataBufferValue)
                , m_ParamName(inShaderParam)
                , m_BindAs(inBufferType)
            {
            }
            SApplyDataBufferValue(const SApplyDataBufferValue &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::ApplyDataBufferValue)
                , m_ParamName(inTable.RegisterStr(inOther.m_ParamName))
                , m_BindAs(inOther.m_BindAs)
            {
            }
        };

        struct SApplyDepthValue : public SCommand
        {
            // If no param name is given, the buffer is bound to the
            // input texture parameter (texture0).
            CRegisteredString m_ParamName;
            SApplyDepthValue(CRegisteredString param)
                : SCommand(CommandTypes::ApplyDepthValue)
                , m_ParamName(param)
            {
            }
            SApplyDepthValue(const SApplyDepthValue &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::ApplyDepthValue)
                , m_ParamName(inTable.RegisterStr(inOther.m_ParamName))
            {
            }
        };

        struct SRender : public SCommand
        {
            bool m_DrawIndirect;
            SRender(bool inDrawIndirect)
                : SCommand(CommandTypes::Render)
                , m_DrawIndirect(inDrawIndirect)
            {
            }

            SRender(const SRender &inOther, IStringTable &)
                : SCommand(CommandTypes::Render)
                , m_DrawIndirect(inOther.m_DrawIndirect)
            {
            }
        };

        struct SApplyCulling : public SCommand
        {
            NVRenderFaces::Enum m_CullMode;
            SApplyCulling(NVRenderFaces::Enum cullMode)
                : SCommand(CommandTypes::ApplyCulling)
                , m_CullMode(cullMode)
            {
            }
            SApplyCulling(const SApplyCulling &inOther, IStringTable &)
                : SCommand(CommandTypes::ApplyCulling)
                , m_CullMode(inOther.m_CullMode)
            {
            }
        };

        struct SApplyDepth : public SCommand
        {
            NVRenderBoolOp::Enum m_depthFunc;
            bool m_depthMask;

            SApplyDepth(NVRenderBoolOp::Enum depthFunc, bool depthMask)
                : SCommand(CommandTypes::ApplyDepth)
                , m_depthFunc(depthFunc)
                , m_depthMask(depthMask)
            {
            }

            SApplyDepth(const SApplyDepth &other, IStringTable &)
                : SCommand(CommandTypes::ApplyDepth)
                , m_depthFunc(other.m_depthFunc)
                , m_depthMask(other.m_depthMask)
            {
            }
        };

        struct SApplyBlending : public SCommand
        {
            NVRenderSrcBlendFunc::Enum m_SrcBlendFunc;
            NVRenderDstBlendFunc::Enum m_DstBlendFunc;

            SApplyBlending(NVRenderSrcBlendFunc::Enum inSrcBlendFunc,
                           NVRenderDstBlendFunc::Enum inDstBlendFunc)
                : SCommand(CommandTypes::ApplyBlending)
                , m_SrcBlendFunc(inSrcBlendFunc)
                , m_DstBlendFunc(inDstBlendFunc)
            {
            }

            SApplyBlending(const SApplyBlending &inOther, IStringTable &)
                : SCommand(CommandTypes::ApplyBlending)
                , m_SrcBlendFunc(inOther.m_SrcBlendFunc)
                , m_DstBlendFunc(inOther.m_DstBlendFunc)
            {
            }
        };

        struct SApplyRenderState : public SCommand
        {
            NVRenderState::Enum m_RenderState;
            bool m_Enabled;

            SApplyRenderState(qt3ds::render::NVRenderState::Enum inRenderStateValue, bool inEnabled)
                : SCommand(CommandTypes::ApplyRenderState)
                , m_RenderState(inRenderStateValue)
                , m_Enabled(inEnabled)
            {
            }

            SApplyRenderState(const SApplyRenderState &inOther, IStringTable &)
                : SCommand(CommandTypes::ApplyRenderState)
                , m_RenderState(inOther.m_RenderState)
                , m_Enabled(inOther.m_Enabled)
            {
            }
        };

        struct SApplyBlitFramebuffer : public SCommand
        {
            // If no buffer name is given then the special buffer [source]
            // is assumed. Which is the default render target
            CRegisteredString m_SourceBufferName;
            // If no buffer name is given then the special buffer [dest]
            // is assumed. Which is the default render target
            CRegisteredString m_DestBufferName;

            SApplyBlitFramebuffer(CRegisteredString inSourceBufferName,
                                  CRegisteredString inDestBufferName)
                : SCommand(CommandTypes::ApplyBlitFramebuffer)
                , m_SourceBufferName(inSourceBufferName)
                , m_DestBufferName(inDestBufferName)
            {
            }

            SApplyBlitFramebuffer(const SApplyBlitFramebuffer &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::ApplyBlitFramebuffer)
                , m_SourceBufferName(inTable.RegisterStr(inOther.m_SourceBufferName))
                , m_DestBufferName(inTable.RegisterStr(inOther.m_DestBufferName))
            {
            }
        };

        struct DepthStencilFlagValues
        {
            enum Enum {
                NoFlagValue = 0,
                ClearStencil = 1 << 0,
                ClearDepth = 1 << 1,
            };
        };

        struct SDepthStencilFlags : public NVFlags<DepthStencilFlagValues::Enum>
        {
            bool HasClearStencil() const { return operator&(DepthStencilFlagValues::ClearStencil); }
            void SetClearStencil(bool value)
            {
                clearOrSet(value, DepthStencilFlagValues::ClearStencil);
            }

            bool HasClearDepth() const { return operator&(DepthStencilFlagValues::ClearDepth); }
            void SetClearDepth(bool value)
            {
                clearOrSet(value, DepthStencilFlagValues::ClearDepth);
            }
        };

        struct SDepthStencil : public SCommand
        {
            CRegisteredString m_BufferName;
            SDepthStencilFlags m_Flags;
            qt3ds::render::NVRenderStencilOp::Enum m_StencilFailOperation;
            qt3ds::render::NVRenderStencilOp::Enum m_DepthPassOperation;
            qt3ds::render::NVRenderStencilOp::Enum m_DepthFailOperation;
            qt3ds::render::NVRenderBoolOp::Enum m_StencilFunction;
            QT3DSU32 m_Reference;
            QT3DSU32 m_Mask;

            SDepthStencil()
                : SCommand(CommandTypes::DepthStencil)
                , m_StencilFailOperation(qt3ds::render::NVRenderStencilOp::Keep)
                , m_DepthPassOperation(qt3ds::render::NVRenderStencilOp::Keep)
                , m_DepthFailOperation(qt3ds::render::NVRenderStencilOp::Keep)
                , m_StencilFunction(qt3ds::render::NVRenderBoolOp::Equal)
                , m_Reference(0)
                , m_Mask(QT3DS_MAX_U32)
            {
            }

            SDepthStencil(CRegisteredString bufName, SDepthStencilFlags flags,
                          qt3ds::render::NVRenderStencilOp::Enum inStencilOp,
                          qt3ds::render::NVRenderStencilOp::Enum inDepthPassOp,
                          qt3ds::render::NVRenderStencilOp::Enum inDepthFailOp,
                          qt3ds::render::NVRenderBoolOp::Enum inStencilFunc, QT3DSU32 value, QT3DSU32 mask)
                : SCommand(CommandTypes::DepthStencil)
                , m_BufferName(bufName)
                , m_Flags(flags)
                , m_StencilFailOperation(inStencilOp)
                , m_DepthPassOperation(inDepthPassOp)
                , m_DepthFailOperation(inDepthFailOp)
                , m_StencilFunction(inStencilFunc)
                , m_Reference(value)
                , m_Mask(mask)
            {
            }

            SDepthStencil(const SDepthStencil &inOther, IStringTable &inTable)
                : SCommand(CommandTypes::DepthStencil)
                , m_BufferName(inTable.RegisterStr(inOther.m_BufferName))
                , m_Flags(inOther.m_Flags)
                , m_StencilFailOperation(inOther.m_StencilFailOperation)
                , m_DepthPassOperation(inOther.m_DepthPassOperation)
                , m_DepthFailOperation(inOther.m_DepthFailOperation)
                , m_StencilFunction(inOther.m_StencilFunction)
                , m_Reference(inOther.m_Reference)
                , m_Mask(inOther.m_Mask)
            {
            }
        };
    }
}
}

#endif
