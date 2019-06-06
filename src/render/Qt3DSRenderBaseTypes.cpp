/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtCore/qdebug.h>

#include "Qt3DSRenderBaseTypes.h"

namespace qt3ds {
namespace render {

using namespace foundation;

QDebug operator<<(QDebug debug, const NVRenderComponentTypes::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderComponentTypes::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderContextValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderContextValues::Enum::GLES2:
        debug.nospace() << "GLES2";
        break;
    case NVRenderContextValues::Enum::GL2:
        debug.nospace() << "GL2";
        break;
    case NVRenderContextValues::Enum::GLES3:
        debug.nospace() << "GLES3";
        break;
    case NVRenderContextValues::Enum::GL3:
        debug.nospace() << "GL3";
        break;
    case NVRenderContextValues::Enum::GLES3PLUS:
        debug.nospace() << "GLES3PLUS";
        break;
    case NVRenderContextValues::Enum::GL4:
        debug.nospace() << "GL4";
        break;
    case NVRenderContextValues::Enum::NullContext:
        debug.nospace() << "NullContext";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderClearValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderClearValues::Enum::Color:
        debug.nospace() << "Color";
        break;
    case NVRenderClearValues::Enum::Depth:
        debug.nospace() << "Depth";
        break;
    case NVRenderClearValues::Enum::Stencil:
        debug.nospace() << "Stencil";
        break;
    case NVRenderClearValues::Enum::Coverage:
        debug.nospace() << "Coverage";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderQueryType::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderQueryType::Enum::Samples:
        debug.nospace() << "Samples";
        break;
    case NVRenderQueryType::Enum::Timer:
        debug.nospace() << "Timer";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderQueryResultType::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderQueryResultType::Enum::ResultAvailable:
        debug.nospace() << "ResultAvailable";
        break;
    case NVRenderQueryResultType::Enum::Result:
        debug.nospace() << "Result";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderSyncType::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderSyncType::Enum::GpuCommandsComplete:
        debug.nospace() << "GpuCommandsComplete";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderSyncValues::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Unknown";
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderCommandFlushValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderCommandFlushValues::Enum::SyncFlushCommands:
        debug.nospace() << "SyncFlushCommands";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderBufferBindValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderBufferBindValues::Enum::Vertex:
        debug.nospace() << "Vertex";
        break;
    case NVRenderBufferBindValues::Enum::Index:
        debug.nospace() << "Index";
        break;
    case NVRenderBufferBindValues::Enum::Constant:
        debug.nospace() << "Constant";
        break;
    case NVRenderBufferBindValues::Enum::Storage:
        debug.nospace() << "Storage";
        break;
    case NVRenderBufferBindValues::Enum::Atomic_Counter:
        debug.nospace() << "Atomic_Counter";
        break;
    case NVRenderBufferBindValues::Enum::Draw_Indirect:
        debug.nospace() << "Draw_Indirect";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderBufferUsageType::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderBufferUsageType::Enum::Static:
        debug.nospace() << "Static";
        break;
    case NVRenderBufferUsageType::Enum::Dynamic:
        debug.nospace() << "Dynamic";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderImageAccessType::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderImageAccessType::Enum::Read:
        debug.nospace() << "Read";
        break;
    case NVRenderImageAccessType::Enum::Write:
        debug.nospace() << "Write";
        break;
    case NVRenderImageAccessType::Enum::ReadWrite:
        debug.nospace() << "ReadWrite";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderBufferAccessTypeValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderBufferAccessTypeValues::Enum::Read:
        debug.nospace() << "Read";
        break;
    case NVRenderBufferAccessTypeValues::Enum::Write:
        debug.nospace() << "Write";
        break;
    case NVRenderBufferAccessTypeValues::Enum::Invalid:
        debug.nospace() << "Invalid";
        break;
    case NVRenderBufferAccessTypeValues::Enum::InvalidRange:
        debug.nospace() << "InvalidRange";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderBufferBarrierValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderBufferBarrierValues::Enum::VertexAttribArray:
        debug.nospace() << "VertexAttribArray";
        break;
    case NVRenderBufferBarrierValues::Enum::ElementArray:
        debug.nospace() << "ElementArray";
        break;
    case NVRenderBufferBarrierValues::Enum::UniformBuffer:
        debug.nospace() << "UniformBuffer";
        break;
    case NVRenderBufferBarrierValues::Enum::TextureFetch:
        debug.nospace() << "TextureFetch";
        break;
    case NVRenderBufferBarrierValues::Enum::ShaderImageAccess:
        debug.nospace() << "ShaderImageAccess";
        break;
    case NVRenderBufferBarrierValues::Enum::CommandBuffer:
        debug.nospace() << "CommandBuffer";
        break;
    case NVRenderBufferBarrierValues::Enum::PixelBuffer:
        debug.nospace() << "PixelBuffer";
        break;
    case NVRenderBufferBarrierValues::Enum::TextureUpdate:
        debug.nospace() << "TextureUpdate";
        break;
    case NVRenderBufferBarrierValues::Enum::BufferUpdate:
        debug.nospace() << "BufferUpdate";
        break;
    case NVRenderBufferBarrierValues::Enum::Framebuffer:
        debug.nospace() << "Framebuffer";
        break;
    case NVRenderBufferBarrierValues::Enum::TransformFeedback:
        debug.nospace() << "TransformFeedback";
        break;
    case NVRenderBufferBarrierValues::Enum::AtomicCounter:
        debug.nospace() << "AtomicCounter";
        break;
    case NVRenderBufferBarrierValues::Enum::ShaderStorage:
        debug.nospace() << "ShaderStorage";
        break;
    case NVRenderBufferBarrierValues::Enum::All:
        debug.nospace() << "All";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderRenderBufferFormats::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderRenderBufferFormats::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureFormats::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderTextureFormats::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureTargetType::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderTextureTargetType::Enum::Texture2D:
        debug.nospace() << "Texture2D";
        break;
    case NVRenderTextureTargetType::Enum::Texture2D_MS:
        debug.nospace() << "Texture2D_MS";
        break;
    case NVRenderTextureTargetType::Enum::Texture2D_Array:
        debug.nospace() << "Texture2D_Array";
        break;
    case NVRenderTextureTargetType::Enum::TextureCube:
        debug.nospace() << "TextureCube";
        break;
    case NVRenderTextureTargetType::Enum::TextureCubePosX:
        debug.nospace() << "TextureCubePosX";
        break;
    case NVRenderTextureTargetType::Enum::TextureCubeNegX:
        debug.nospace() << "TextureCubeNegX";
        break;
    case NVRenderTextureTargetType::Enum::TextureCubePosY:
        debug.nospace() << "TextureCubePosY";
        break;
    case NVRenderTextureTargetType::Enum::TextureCubeNegY:
        debug.nospace() << "TextureCubeNegY";
        break;
    case NVRenderTextureTargetType::Enum::TextureCubePosZ:
        debug.nospace() << "TextureCubePosZ";
        break;
    case NVRenderTextureTargetType::Enum::TextureCubeNegZ:
        debug.nospace() << "TextureCubeNegZ";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureUnit::Enum value)
{
    QDebugStateSaver saver(debug);
    if (value > NVRenderTextureUnit::Enum::TextureUnit_31)
        debug.nospace() << "Unknown";
    else
        debug.nospace() << "TextureUnit_" << value;
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureCompareMode::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderTextureCompareMode::Enum::NoCompare:
        debug.nospace() << "NoCompare";
        break;
    case NVRenderTextureCompareMode::Enum::CompareToRef:
        debug.nospace() << "CompareToRef";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureSwizzleMode::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderTextureSwizzleMode::Enum::NoSwizzle:
        debug.nospace() << "NoSwizzle";
        break;
    case NVRenderTextureSwizzleMode::Enum::L8toR8:
        debug.nospace() << "L8toR8";
        break;
    case NVRenderTextureSwizzleMode::Enum::A8toR8:
        debug.nospace() << "A8toR8";
        break;
    case NVRenderTextureSwizzleMode::Enum::L8A8toRG8:
        debug.nospace() << "L8A8toRG8";
        break;
    case NVRenderTextureSwizzleMode::Enum::L16toR16:
        debug.nospace() << "L16toR16";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureCompareOp::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
#define QT3DS_RENDER_HANDLE_BOOL_OP(x)      \
    case NVRenderTextureCompareOp::Enum::x: \
    debug.nospace() << #x;                  \
    break;
    QT3DS_RENDER_ITERATE_BOOL_OP
        #undef QT3DS_RENDER_HANDLE_BOOL_OP
            default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureMinifyingOp::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderTextureMinifyingOp::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureMagnifyingOp::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderTextureMagnifyingOp::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureCoordOp::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderTextureCoordOp::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderHint::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderHint::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderVertexBufferEntry value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NVRenderVertexBufferEntry(name:" << value.m_Name
                    << ",componentType:" << value.m_ComponentType
                    << ",numComponents" << value.m_NumComponents
                    << ",firstItemOffset" << value.m_FirstItemOffset
                    << ",inputSlot" << value.m_InputSlot;
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderVertFragCompilationResult value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NVRenderVertFragCompilationResult(ShaderName:" << value.mShaderName
                    << ",Shader:" << value.mShader;
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderDrawMode::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderDrawMode::Enum::Points:
        debug.nospace() << "Points";
        break;
    case NVRenderDrawMode::Enum::LineStrip:
        debug.nospace() << "LineStrip";
        break;
    case NVRenderDrawMode::Enum::LineLoop:
        debug.nospace() << "LineLoop";
        break;
    case NVRenderDrawMode::Enum::Lines:
        debug.nospace() << "Lines";
        break;
    case NVRenderDrawMode::Enum::TriangleStrip:
        debug.nospace() << "TriangleStrip";
        break;
    case NVRenderDrawMode::Enum::TriangleFan:
        debug.nospace() << "TriangleFan";
        break;
    case NVRenderDrawMode::Enum::Triangles:
        debug.nospace() << "Triangles";
        break;
    case NVRenderDrawMode::Enum::Patches:
        debug.nospace() << "Patches";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureCubeFaces::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderTextureCubeFaces::Enum::InvalidFace:
        debug.nospace() << "InvalidFace";
        break;
    case NVRenderTextureCubeFaces::Enum::CubePosX:
        debug.nospace() << "CubePosX";
        break;
    case NVRenderTextureCubeFaces::Enum::CubeNegX:
        debug.nospace() << "CubeNegX";
        break;
    case NVRenderTextureCubeFaces::Enum::CubePosY:
        debug.nospace() << "CubePosY";
        break;
    case NVRenderTextureCubeFaces::Enum::CubeNegY:
        debug.nospace() << "CubeNegY";
        break;
    case NVRenderTextureCubeFaces::Enum::CubePosZ:
        debug.nospace() << "CubePosZ";
        break;
    case NVRenderTextureCubeFaces::Enum::CubeNegZ:
        debug.nospace() << "CubeNegZ";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathCommands::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathCommands::Enum::Close:
        debug.nospace() << "Close";
        break;
    case NVRenderPathCommands::Enum::MoveTo:
        debug.nospace() << "MoveTo";
        break;
    case NVRenderPathCommands::Enum::CubicCurveTo:
        debug.nospace() << "CubicCurveTo";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathFontTarget::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathFontTarget::Enum::StandardFont:
        debug.nospace() << "StandardFont";
        break;
    case NVRenderPathFontTarget::Enum::SystemFont:
        debug.nospace() << "SystemFont";
        break;
    case NVRenderPathFontTarget::Enum::FileFont:
        debug.nospace() << "FileFont";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathMissingGlyphs::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathMissingGlyphs::Enum::SkipMissing:
        debug.nospace() << "SkipMissing";
        break;
    case NVRenderPathMissingGlyphs::Enum::UseMissing:
        debug.nospace() << "UseMissing";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathFontStyleValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathFontStyleValues::Enum::Bold:
        debug.nospace() << "Bold";
        break;
    case NVRenderPathFontStyleValues::Enum::Italic:
        debug.nospace() << "Italic";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathReturnValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathReturnValues::Enum::FontGlypsAvailable:
        debug.nospace() << "FontGlypsAvailable";
        break;
    case NVRenderPathReturnValues::Enum::FontTargetUnavailable:
        debug.nospace() << "FontTargetUnavailable";
        break;
    case NVRenderPathReturnValues::Enum::FontUnavailable:
        debug.nospace() << "FontUnavailable";
        break;
    case NVRenderPathReturnValues::Enum::FontUnintelligible:
        debug.nospace() << "FontUnintelligible";
        break;
    case NVRenderPathReturnValues::Enum::InvalidEnum:
        debug.nospace() << "InvalidEnum";
        break;
    case NVRenderPathReturnValues::Enum::OutOfMemory:
        debug.nospace() << "OutOfMemory";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathFormatType::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathFormatType::Enum::Byte:
        debug.nospace() << "Byte";
        break;
    case NVRenderPathFormatType::Enum::UByte:
        debug.nospace() << "UByte";
        break;
    case NVRenderPathFormatType::Enum::Short:
        debug.nospace() << "Short";
        break;
    case NVRenderPathFormatType::Enum::UShort:
        debug.nospace() << "UShort";
        break;
    case NVRenderPathFormatType::Enum::Int:
        debug.nospace() << "Int";
        break;
    case NVRenderPathFormatType::Enum::Uint:
        debug.nospace() << "Uint";
        break;
    case NVRenderPathFormatType::Enum::Float:
        debug.nospace() << "Float";
        break;
    case NVRenderPathFormatType::Enum::Utf8:
        debug.nospace() << "Utf8";
        break;
    case NVRenderPathFormatType::Enum::Utf16:
        debug.nospace() << "Utf16";
        break;
    case NVRenderPathFormatType::Enum::Bytes2:
        debug.nospace() << "Bytes2";
        break;
    case NVRenderPathFormatType::Enum::Bytes3:
        debug.nospace() << "Bytes3";
        break;
    case NVRenderPathFormatType::Enum::Bytes4:
        debug.nospace() << "Bytes4";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathGlyphFontMetricValues::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphWidth:
        debug.nospace() << "GlyphWidth";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphHeight:
        debug.nospace() << "GlyphHeight";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphHorizontalBearingX:
        debug.nospace() << "GlyphHorizontalBearingX";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphHorizontalBearingY:
        debug.nospace() << "GlyphHorizontalBearingY";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphHorizontalBearingAdvance:
        debug.nospace() << "GlyphHorizontalBearingAdvance";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphVerticalBearingX:
        debug.nospace() << "GlyphVerticalBearingX";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphVerticalBearingY:
        debug.nospace() << "GlyphVerticalBearingY";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphVerticalBearingAdvance:
        debug.nospace() << "GlyphVerticalBearingAdvance";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::GlyphHasKerning:
        debug.nospace() << "GlyphHasKerning";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontXMinBounds:
        debug.nospace() << "FontXMinBounds";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontYMinBounds:
        debug.nospace() << "FontYMinBounds";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontXMaxBounds:
        debug.nospace() << "FontXMaxBounds";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontYMaxBounds:
        debug.nospace() << "FontYMaxBounds";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontUnitsPerEm:
        debug.nospace() << "FontUnitsPerEm";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontAscender:
        debug.nospace() << "FontAscender";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontDescender:
        debug.nospace() << "FontDescender";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontHeight:
        debug.nospace() << "FontHeight";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontMaxAdvanceWidth:
        debug.nospace() << "FontMaxAdvanceWidth";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontMaxAdvanceHeight:
        debug.nospace() << "FontMaxAdvanceHeight";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontUnderlinePosition:
        debug.nospace() << "FontUnderlinePosition";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontUnderlineThickness:
        debug.nospace() << "FontUnderlineThickness";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontHasKerning:
        debug.nospace() << "FontHasKerning";
        break;
    case NVRenderPathGlyphFontMetricValues::Enum::FontNumGlyphIndices:
        debug.nospace() << "FontNumGlyphIndices";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathListMode::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathListMode::Enum::AccumAdjacentPairs:
        debug.nospace() << "AccumAdjacentPairs";
        break;
    case NVRenderPathListMode::Enum::AdjacentPairs:
        debug.nospace() << "AdjacentPairs";
        break;
    case NVRenderPathListMode::Enum::FirstToRest:
        debug.nospace() << "FirstToRest";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathFillMode::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathFillMode::Enum::Fill:
        debug.nospace() << "Fill";
        break;
    case NVRenderPathFillMode::Enum::CountUp:
        debug.nospace() << "CountUp";
        break;
    case NVRenderPathFillMode::Enum::CountDown:
        debug.nospace() << "CountDown";
        break;
    case NVRenderPathFillMode::Enum::Invert:
        debug.nospace() << "Invert";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathCoverMode::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathCoverMode::Enum::ConvexHull:
        debug.nospace() << "ConvexHull";
        break;
    case NVRenderPathCoverMode::Enum::BoundingBox:
        debug.nospace() << "BoundingBox";
        break;
    case NVRenderPathCoverMode::Enum::BoundingBoxOfBoundingBox:
        debug.nospace() << "BoundingBoxOfBoundingBox";
        break;
    case NVRenderPathCoverMode::Enum::PathFillCover:
        debug.nospace() << "PathFillCover";
        break;
    case NVRenderPathCoverMode::Enum::PathStrokeCover:
        debug.nospace() << "PathStrokeCover";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderPathTransformType::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderPathTransformType::Enum::NoTransform:
        debug.nospace() << "NoTransform";
        break;
    case NVRenderPathTransformType::Enum::TranslateX:
        debug.nospace() << "TranslateX";
        break;
    case NVRenderPathTransformType::Enum::TranslateY:
        debug.nospace() << "TranslateY";
        break;
    case NVRenderPathTransformType::Enum::Translate2D:
        debug.nospace() << "Translate2D";
        break;
    case NVRenderPathTransformType::Enum::Translate3D:
        debug.nospace() << "Translate3D";
        break;
    case NVRenderPathTransformType::Enum::Affine2D:
        debug.nospace() << "Affine2D";
        break;
    case NVRenderPathTransformType::Enum::Affine3D:
        debug.nospace() << "Affine3D";
        break;
    case NVRenderPathTransformType::Enum::TransposeAffine2D:
        debug.nospace() << "TransposeAffine2D";
        break;
    case NVRenderPathTransformType::Enum::TransposeAffine3D:
        debug.nospace() << "TransposeAffine3D";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderWinding::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderWinding::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderState::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderState::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderSrcBlendFunc::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderSrcBlendFunc::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderDstBlendFunc::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderDstBlendFunc::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderBlendEquation::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderBlendEquation::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderFaces::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderFaces::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVReadFaces::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVReadFaces::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderBoolOp::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderBoolOp::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderStencilOp::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderStencilOp::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderBlendFunctionArgument &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NVRenderBlendFunctionArgument(SrcRGB:" << value.m_SrcRGB
                    << ", DstRGB:" << value.m_DstRGB
                    << ", SrcAlpha:" << value.m_SrcAlpha
                    << ", DstAlpha:" << value.m_DstAlpha
                    << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderBlendEquationArgument &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NVRenderBlendEquationArgument(RGBEquation:" << value.m_RGBEquation
                    << ", AlphaEquation:" << value.m_AlphaEquation
                    << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderStencilOperationArgument &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NVRenderStencilOperationArgument(StencilFail:" << value.m_StencilFail
                    << ", DepthFail:" << value.m_DepthFail
                    << ", DepthPass:" << value.m_DepthPass
                    << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderStencilFunctionArgument &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NVRenderStencilFunctionArgument(Function:" << value.m_Function
                    << ", ReferenceValue:" << value.m_ReferenceValue
                    << ", Mask:" << value.m_Mask
                    << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderRect &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NVRenderRect(x:" << value.m_X
                    << ", y:" << value.m_Y
                    << ", width:" << value.m_Width
                    << ", height:" << value.m_Height
                    << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderRectF &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NVRenderRectF(x:" << value.m_X
                    << ", y:" << value.m_Y
                    << ", width:" << value.m_Width
                    << ", height:" << value.m_Height
                    << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderShaderDataTypes::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
#define HANDLE_QT3DS_SHADER_DATA_TYPE(x)   \
    case NVRenderShaderDataTypes::Enum::x: \
    debug.nospace() << #x;                 \
    break;
    ITERATE_QT3DS_SHADER_DATA_TYPES
        #undef HANDLE_QT3DS_SHADER_DATA_TYPE
            default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderShaderTypeValue::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case NVRenderShaderTypeValue::Enum::Vertex:
        debug.nospace() << "Vertex";
        break;
    case NVRenderShaderTypeValue::Enum::Fragment:
        debug.nospace() << "Fragment";
        break;
    case NVRenderShaderTypeValue::Enum::TessControl:
        debug.nospace() << "TessControl";
        break;
    case NVRenderShaderTypeValue::Enum::TessEvaluation:
        debug.nospace() << "TessEvaluation";
        break;
    case NVRenderShaderTypeValue::Enum::Geometry:
        debug.nospace() << "Geometry";
        break;
    default:
        debug.nospace() << "Unknown";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderTextureTypeValue::Enum value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << NVRenderTextureTypeValue::toString(value);
    return debug;
}

QDebug operator<<(QDebug debug, const NVRenderReadPixelFormats::Enum value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
#define HANDLE_QT3DS_READ_PIXEL_FORMAT(x)   \
    case NVRenderReadPixelFormats::Enum::x: \
    debug.nospace() << #x;                  \
    break;
    ITERATE_QT3DS_READ_PIXEL_FORMATS
        #undef HANDLE_QT3DS_READ_PIXEL_FORMAT
            default:
        debug.nospace() << "Unknown";
    }
    return debug;
}
}
}
