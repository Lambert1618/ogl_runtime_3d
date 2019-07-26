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
#ifndef QT3DS_RENDER_QT3DS_RENDER_TYPES_H
#define QT3DS_RENDER_QT3DS_RENDER_TYPES_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/Qt3DSMath.h"
#include "foundation/Qt3DSVec2.h"
#include <QtCore/qstring.h>

namespace qt3ds {

namespace render {
using namespace foundation;

#define QT3DS_RENDER_ITERATE_COMPONENT_TYPES                                                          \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSU8)                                                          \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSI8)                                                          \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSU16)                                                         \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSI16)                                                         \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSU32)                                                         \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSI32)                                                         \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSU64)                                                         \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSI64)                                                         \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSF16)                                                         \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSF32)                                                         \
    QT3DS_RENDER_HANDLE_COMPONENT_TYPE(QT3DSF64)

struct NVRenderComponentTypes
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_COMPONENT_TYPE(x) x,
        QT3DS_RENDER_ITERATE_COMPONENT_TYPES
#undef QT3DS_RENDER_HANDLE_COMPONENT_TYPE
    };
    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_COMPONENT_TYPE(x)                                                         \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_COMPONENT_TYPES
        #undef QT3DS_RENDER_HANDLE_COMPONENT_TYPE
                default:
            break;
        }
        return "Unknown";
    }

    static qt3ds::QT3DSU32 getSizeofType(Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_COMPONENT_TYPE(x)                                                         \
        case x:                                                                                        \
    return sizeof(qt3ds::x);
        QT3DS_RENDER_ITERATE_COMPONENT_TYPES
        #undef QT3DS_RENDER_HANDLE_COMPONENT_TYPE
                default:
            break;
        }
        QT3DS_ASSERT(false);
        return 0;
    }
};

/**
            Define a set of compile-time trait classes that map the enumerations
            to actual compile time types and sizeof so we can deal with the enumeration
            in generic ways.
    */
template <NVRenderComponentTypes::Enum TraitType>
struct NVRenderComponentTraits
{
    bool force_compile_error;
};

/**
            Define a compile time mapping from datatype to enumeration.  Not that if you
            use this with a type that isn't a component type you will get a compilation
            error.
    */
template <typename TDataType>
struct NVRenderComponentTypeToTypeMap
{
    bool force_compile_error;
};

#define QT3DS_RENDER_HANDLE_COMPONENT_TYPE(x)                                                         \
    template <>                                                                                    \
    struct NVRenderComponentTraits<NVRenderComponentTypes::x>                                      \
{                                                                                              \
    typedef x TComponentType;                                                                  \
    QT3DSU8 mComponentSize;                                                                       \
    NVRenderComponentTraits<NVRenderComponentTypes::x>()                                       \
    : mComponentSize(sizeof(x))                                                            \
{                                                                                          \
}                                                                                          \
};                                                                                             \
    template <>                                                                                    \
    struct NVRenderComponentTypeToTypeMap<x>                                                       \
{                                                                                              \
    NVRenderComponentTypes::Enum m_ComponentType;                                              \
    NVRenderComponentTypeToTypeMap<x>()                                                        \
    : m_ComponentType(NVRenderComponentTypes::x)                                           \
{                                                                                          \
}                                                                                          \
};

QT3DS_RENDER_ITERATE_COMPONENT_TYPES;

#undef QT3DS_RENDER_HANDLE_COMPONENT_TYPE

// Map at compile time from component type to datatype;
template <typename TDataType>
inline NVRenderComponentTypes::Enum getComponentTypeForType()
{
    return NVRenderComponentTypeToTypeMap<TDataType>().m_ComponentType;
}

struct NVRenderContextValues
{
    enum Enum {
        GLES2 = 1 << 0,
        GL2 = 1 << 1,
        GLES3 = 1 << 2,
        GL3 = 1 << 3,
        GLES3PLUS = 1 << 4,
        GL4 = 1 << 5,
        NullContext = 1 << 6,
    };
};

typedef NVFlags<NVRenderContextValues::Enum, QT3DSU32> NVRenderContextType;

struct NVRenderClearValues
{
    enum Enum {
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 3,
        Coverage = 1 << 4,
    };
};

typedef NVFlags<NVRenderClearValues::Enum, QT3DSU32> NVRenderClearFlags;

struct NVRenderQueryType
{
    enum Enum {
        Unknown = 0,
        Samples, ///< samples query object
        Timer, ///< timer query object
    };
};

struct NVRenderQueryResultType
{
    enum Enum {
        Unknown = 0,
        ResultAvailable, ///< Check if query result is available
        Result, ///< Get actual result
    };
};

struct NVRenderSyncType
{
    enum Enum {
        Unknown = 0,
        GpuCommandsComplete, ///< sync to Gpu commands finished
    };
};

struct NVRenderSyncValues
{
    enum Enum {
        Unknown = 0, ///< for future usage
    };
};

typedef NVFlags<NVRenderSyncValues::Enum, QT3DSU32> NVRenderSyncFlags;

struct NVRenderCommandFlushValues
{
    enum Enum {
        SyncFlushCommands = 0, ///< sync for flushing command
    };
};

typedef NVFlags<NVRenderCommandFlushValues::Enum, QT3DSU32> NVRenderCommandFlushFlags;

struct NVRenderBufferBindValues
{
    enum Enum {
        Unknown = 0,
        Vertex = 1 << 0, ///< Bind as vertex buffer
        Index = 1 << 1, ///< Bind as index buffer
        Constant = 1 << 2, ///< Bind as constant buffer
        Storage = 1 << 3, ///< Bind as shader storage buffer
        Atomic_Counter = 1 << 4, ///< Bind as atomic counter buffer
        Draw_Indirect = 1 << 5, ///< Bind as draw indirect buffer
    };
};

typedef NVFlags<NVRenderBufferBindValues::Enum, QT3DSU32> NVRenderBufferBindFlags;

struct NVRenderBufferUsageType
{
    enum Enum {
        Unknown = 0,
        Static, ///< Rarely updated
        Dynamic, ///< Most likely updated every frame
    };
};

struct NVRenderImageAccessType
{
    enum Enum {
        Unknown = 0,
        Read, ///< Read only access
        Write, ///< Write only access
        ReadWrite, ///< Read and write access
    };
};

struct NVRenderBufferAccessTypeValues
{
    enum Enum {
        Unknown = 0,
        Read = 1 << 0, ///< Read access
        Write = 1 << 1, ///< Write access
        Invalid = 1 << 2, ///< No sync
        InvalidRange = 1 << 3, ///< No sync

    };
};

typedef NVFlags<NVRenderBufferAccessTypeValues::Enum, QT3DSU32> NVRenderBufferAccessFlags;

///< defines a barrier of ordering the memory transactions to a command relative to those issued
///before the barrier
struct NVRenderBufferBarrierValues
{
    enum Enum {
        Unknown = 0,
        VertexAttribArray = 1 << 0, ///< Barrier for vertex attributes sourced from a buffer
        ElementArray = 1 << 1, ///< Barrier for indices sourced from a buffer
        UniformBuffer = 1 << 2, ///< Barrier for shader uniforms sourced from a buffer
        TextureFetch = 1 << 3, ///< Barrier for texture fetches within shaders
        ShaderImageAccess = 1 << 4, ///< Barrier for image access using load / store
        CommandBuffer = 1 << 5, ///< Barrier for indirect drawing
        PixelBuffer = 1 << 6, ///< Barrier for pixel buffer access
        TextureUpdate = 1 << 7, ///< Barrier for texture writes
        BufferUpdate = 1 << 8, ///< Barrier for buffer writes
        Framebuffer = 1 << 9, ///< Barrier for framebuffer writes
        TransformFeedback = 1 << 10, ///< Barrier for transform feedback writes
        AtomicCounter = 1 << 11, ///< Barrier for atomic counter writes
        ShaderStorage = 1 << 12, ///< Barrier for shader storage blocks writes
        All = 0xFFFF, ///< Barrier for all of the above
    };
};

typedef NVFlags<NVRenderBufferBarrierValues::Enum, QT3DSU32> NVRenderBufferBarrierFlags;

#define QT3DS_RENDER_ITERATE_RENDERBUFFER_FORMATS                                                     \
    QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(RGBA4)                                                    \
    QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(RGB565)                                                   \
    QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(RGBA5551)                                                 \
    QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(Depth16)                                                  \
    QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(Depth24)                                                  \
    QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(Depth32)                                                  \
    QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(StencilIndex8)                                            \
    QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(CoverageNV)

struct NVRenderRenderBufferFormats
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(x) x,
        QT3DS_RENDER_ITERATE_RENDERBUFFER_FORMATS
#undef QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT
    };
    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT(x)                                                    \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_RENDERBUFFER_FORMATS
        #undef QT3DS_RENDER_HANDLE_RENDERBUFFER_FORMAT
                default:
            break;
        }
        return "Unknown";
    }
};

#define QT3DS_RENDER_ITERATE_TEXTURE_FORMATS                                                    \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(R8)                                                      \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(R16)                                                     \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(R16F)                                                    \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(R32I)                                                    \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(R32UI)                                                   \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(R32F)                                                    \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RG8)                                                     \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RGBA8)                                                   \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RGB8)                                                    \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(SRGB8)                                                   \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(SRGB8A8)                                                 \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RGB565)                                                  \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RGBA5551)                                                \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(Alpha8)                                                  \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(Luminance8)                                              \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(Luminance16)                                             \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(LuminanceAlpha8)                                         \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RGBA16F)                                                 \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RG16F)                                                   \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RG32F)                                                   \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RGB32F)                                                  \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RGBA32F)                                                 \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(R11G11B10)                                               \
    QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(RGB9E5)                                                  \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_DXT1)                                    \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGB_DXT1)                                     \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_DXT3)                                    \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_DXT5)                                    \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGB8_ETC1)                                    \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGB8_ETC2)                                    \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_ETC2)                                   \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGB8_PunchThrough_Alpha1_ETC2)                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_PunchThrough_Alpha1_ETC2)               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(R11_EAC_UNorm)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(R11_EAC_SNorm)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RG11_EAC_UNorm)                               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RG11_EAC_SNorm)                               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA8_ETC2_EAC)                               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ETC2_EAC)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(R_ATI1N_UNorm)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(R_ATI1N_SNorm)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RG_ATI2N_UNorm)                               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RG_ATI2N_SNorm)                               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGB_BP_UNSIGNED_FLOAT)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGB_BP_SIGNED_FLOAT)                          \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGB_BP_UNorm)                                 \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_4x4)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_5x4)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_5x5)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_6x5)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_6x6)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_8x5)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_8x6)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_8x8)                                \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_10x5)                               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_10x6)                               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_10x8)                               \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_10x10)                              \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_12x10)                              \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(RGBA_ASTC_12x12)                              \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_4x4)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_5x4)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_5x5)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_6x5)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_6x6)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_8x5)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_8x6)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_8x8)                        \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_10x5)                       \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_10x6)                       \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_10x8)                       \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_10x10)                      \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_12x10)                      \
    QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(SRGB8_Alpha8_ASTC_12x12)                      \
    QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(Depth16)                                           \
    QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(Depth24)                                           \
    QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(Depth32)                                           \
    QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(Depth24Stencil8)

struct NVRenderTextureFormats
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(x) x,
#define QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(x) x,
#define QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(x) x,
        QT3DS_RENDER_ITERATE_TEXTURE_FORMATS
#undef QT3DS_RENDER_HANDLE_TEXTURE_FORMAT
#undef QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT
#undef QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT
    };

    static bool isUncompressedTextureFormat(NVRenderTextureFormats::Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(x)                                                         \
        case NVRenderTextureFormats::x:                                                                \
    return true;
#define QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(x)
#define QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(x)
        QT3DS_RENDER_ITERATE_TEXTURE_FORMATS
        #undef QT3DS_RENDER_HANDLE_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_GL_QT3DS_DEPTH_TEXTURE_FORMAT
                default:
            break;
        }
        return false;
    }

    static bool isCompressedTextureFormat(NVRenderTextureFormats::Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(x)
#define QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(x)                                              \
        case NVRenderTextureFormats::x:                                                                \
    return true;
#define QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(x)
        QT3DS_RENDER_ITERATE_TEXTURE_FORMATS
        #undef QT3DS_RENDER_HANDLE_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_GL_QT3DS_DEPTH_TEXTURE_FORMAT
                default:
            break;
        }
        return false;
    }

    static bool isDepthTextureFormat(NVRenderTextureFormats::Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(x)
#define QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(x)
#define QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(x)                                                   \
        case NVRenderTextureFormats::x:                                                                \
    return true;
        QT3DS_RENDER_ITERATE_TEXTURE_FORMATS
        #undef QT3DS_RENDER_HANDLE_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_GL_QT3DS_DEPTH_TEXTURE_FORMAT
                default:
            break;
        }
        return false;
    }

    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_TEXTURE_FORMAT(x)                                                         \
        case x:                                                                                        \
    return #x;
#define QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT(x)                                              \
        case x:                                                                                        \
    return #x;
#define QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT(x)                                                   \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_TEXTURE_FORMATS
        #undef QT3DS_RENDER_HANDLE_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_COMPRESSED_TEXTURE_FORMAT
        #undef QT3DS_RENDER_HANDLE_DEPTH_TEXTURE_FORMAT
                default:
            break;
        }
        return "Unknown";
    }

    static QT3DSU32 getSizeofFormat(Enum value)
    {
        switch (value) {
        case R8:
            return 1;
        case R16F:
            return 2;
        case R16:
            return 2;
        case R32I:
            return 4;
        case R32F:
            return 4;
        case RGBA8:
            return 4;
        case RGB8:
            return 3;
        case RGB565:
            return 2;
        case RGBA5551:
            return 2;
        case Alpha8:
            return 1;
        case Luminance8:
            return 1;
        case LuminanceAlpha8:
            return 1;
        case Depth16:
            return 2;
        case Depth24:
            return 3;
        case Depth32:
            return 4;
        case Depth24Stencil8:
            return 4;
        case RGB9E5:
            return 4;
        case SRGB8:
            return 3;
        case SRGB8A8:
            return 4;
        case RGBA16F:
            return 8;
        case RG16F:
            return 4;
        case RG32F:
            return 8;
        case RGBA32F:
            return 16;
        case RGB32F:
            return 12;
        case R11G11B10:
            return 4;
        default:
            break;
        }
        QT3DS_ASSERT(false);
        return 0;
    }

    static QT3DSU32 getNumberOfComponent(Enum value)
    {
        switch (value) {
        case R8:
            return 1;
        case R16F:
            return 1;
        case R16:
            return 1;
        case R32I:
            return 1;
        case R32F:
            return 1;
        case RGBA8:
            return 4;
        case RGB8:
            return 3;
        case RGB565:
            return 3;
        case RGBA5551:
            return 4;
        case Alpha8:
            return 1;
        case Luminance8:
            return 1;
        case LuminanceAlpha8:
            return 2;
        case Depth16:
            return 1;
        case Depth24:
            return 1;
        case Depth32:
            return 1;
        case Depth24Stencil8:
            return 2;
        case RGB9E5:
            return 3;
        case SRGB8:
            return 3;
        case SRGB8A8:
            return 4;
        case RGBA16F:
            return 4;
        case RG16F:
            return 2;
        case RG32F:
            return 2;
        case RGBA32F:
            return 4;
        case RGB32F:
            return 3;
        case R11G11B10:
            return 3;
        default:
            break;
        }
        QT3DS_ASSERT(false);
        return 0;
    }

    static void decodeToFloat(void *inPtr, QT3DSU32 byteOfs, float *outPtr,
                              NVRenderTextureFormats::Enum inFmt)
    {
        outPtr[0] = 0.0f;
        outPtr[1] = 0.0f;
        outPtr[2] = 0.0f;
        outPtr[3] = 0.0f;
        QT3DSU8 *src = reinterpret_cast<QT3DSU8 *>(inPtr);
        // float divisor;		// If we want to support RGBD?
        switch (inFmt) {
        case Alpha8:
            outPtr[0] = ((float)src[byteOfs]) / 255.0f;
            break;

        case Luminance8:
        case LuminanceAlpha8:
        case R8:
        case RG8:
        case RGB8:
        case RGBA8:
        case SRGB8:
        case SRGB8A8:
            // NOTE : RGBD Hack here for reference.  Not meant for installation.
            // divisor = (NVRenderTextureFormats::getSizeofFormat(inFmt) == 4) ?
            // ((float)src[byteOfs+3]) / 255.0f : 1.0f;
            for (QT3DSU32 i = 0; i < NVRenderTextureFormats::getSizeofFormat(inFmt); ++i) {
                float val = ((float)src[byteOfs + i]) / 255.0f;
                outPtr[i] = (i < 3) ? powf(val, 0.4545454545f) : val;
                // Assuming RGBA8 actually means RGBD (which is stupid, I know)
                // if ( NVRenderTextureFormats::getSizeofFormat(inFmt) == 4 ) { outPtr[i] /=
                // divisor; }
            }
            // outPtr[3] = divisor;
            break;

        case R32F:
            outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
            break;
        case RG32F:
            outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
            outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
            break;
        case RGBA32F:
            outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
            outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
            outPtr[2] = reinterpret_cast<float *>(src + byteOfs)[2];
            outPtr[3] = reinterpret_cast<float *>(src + byteOfs)[3];
            break;
        case RGB32F:
            outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
            outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
            outPtr[2] = reinterpret_cast<float *>(src + byteOfs)[2];
            break;

        case R16F:
        case RG16F:
        case RGBA16F:
            for (QT3DSU32 i = 0; i < (NVRenderTextureFormats::getSizeofFormat(inFmt) >> 1); ++i) {
                // NOTE : This only works on the assumption that we don't have any denormals,
                // Infs or NaNs.
                // Every pixel in our source image should be "regular"
                QT3DSU16 h = reinterpret_cast<QT3DSU16 *>(src + byteOfs)[i];
                QT3DSU32 sign = (h & 0x8000) << 16;
                QT3DSU32 exponent = (((((h & 0x7c00) >> 10) - 15) + 127) << 23);
                QT3DSU32 mantissa = ((h & 0x3ff) << 13);
                QT3DSU32 result = sign | exponent | mantissa;

                if (h == 0 || h == 0x8000) {
                    result = 0;
                } // Special case for zero and negative zero
                qt3ds::intrinsics::memCopy(reinterpret_cast<QT3DSU32 *>(outPtr) + i, &result, 4);
            }
            break;

        case R11G11B10:
            // place holder
            QT3DS_ASSERT(false);
            break;

        default:
            outPtr[0] = 0.0f;
            outPtr[1] = 0.0f;
            outPtr[2] = 0.0f;
            outPtr[3] = 0.0f;
            break;
        }
    }

    static void encodeToPixel(float *inPtr, void *outPtr, QT3DSU32 byteOfs,
                              NVRenderTextureFormats::Enum inFmt)
    {
        QT3DSU8 *dest = reinterpret_cast<QT3DSU8 *>(outPtr);
        switch (inFmt) {
        case NVRenderTextureFormats::Alpha8:
            dest[byteOfs] = QT3DSU8(inPtr[0] * 255.0f);
            break;

        case Luminance8:
        case LuminanceAlpha8:
        case R8:
        case RG8:
        case RGB8:
        case RGBA8:
        case SRGB8:
        case SRGB8A8:
            for (QT3DSU32 i = 0; i < NVRenderTextureFormats::getSizeofFormat(inFmt); ++i) {
                inPtr[i] = (inPtr[i] > 1.0f) ? 1.0f : inPtr[i];
                if (i < 3)
                    dest[byteOfs + i] = QT3DSU8(powf(inPtr[i], 2.2f) * 255.0f);
                else
                    dest[byteOfs + i] = QT3DSU8(inPtr[i] * 255.0f);
            }
            break;

        case R32F:
            reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
            break;
        case RG32F:
            reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
            reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
            break;
        case RGBA32F:
            reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
            reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
            reinterpret_cast<float *>(dest + byteOfs)[2] = inPtr[2];
            reinterpret_cast<float *>(dest + byteOfs)[3] = inPtr[3];
            break;
        case RGB32F:
            reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
            reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
            reinterpret_cast<float *>(dest + byteOfs)[2] = inPtr[2];
            break;

        case R16F:
        case RG16F:
        case RGBA16F:
            for (QT3DSU32 i = 0; i < (NVRenderTextureFormats::getSizeofFormat(inFmt) >> 1); ++i) {
                // NOTE : This also has the limitation of not handling  infs, NaNs and
                // denormals, but it should be
                // sufficient for our purposes.
                if (inPtr[i] > 65519.0f) {
                    inPtr[i] = 65519.0f;
                }
                if (fabs(inPtr[i]) < 6.10352E-5f) {
                    inPtr[i] = 0.0f;
                }
                QT3DSU32 f = reinterpret_cast<QT3DSU32 *>(inPtr)[i];
                QT3DSU32 sign = (f & 0x80000000) >> 16;
                QT3DSI32 exponent = (f & 0x7f800000) >> 23;
                QT3DSU32 mantissa = (f >> 13) & 0x3ff;
                exponent = exponent - 112;
                if (exponent > 31) {
                    exponent = 31;
                }
                if (exponent < 0) {
                    exponent = 0;
                }
                exponent = exponent << 10;
                reinterpret_cast<QT3DSU16 *>(dest + byteOfs)[i] =
                        QT3DSU16(sign | exponent | mantissa);
            }
            break;

        case R11G11B10:
            // place holder
            QT3DS_ASSERT(false);
            break;

        default:
            dest[byteOfs] = 0;
            dest[byteOfs + 1] = 0;
            dest[byteOfs + 2] = 0;
            dest[byteOfs + 3] = 0;
            break;
        }
    }
};

struct NVRenderTextureTargetType
{
    enum Enum {
        Unknown = 0,
        Texture2D,
        Texture2D_MS,
        Texture2D_Array,
        TextureCube,
        TextureCubePosX,
        TextureCubeNegX,
        TextureCubePosY,
        TextureCubeNegY,
        TextureCubePosZ,
        TextureCubeNegZ,
    };
};

struct NVRenderTextureUnit
{
    enum Enum {
        TextureUnit_0 = 0,
        TextureUnit_1,
        TextureUnit_2,
        TextureUnit_3,
        TextureUnit_4,
        TextureUnit_5,
        TextureUnit_6,
        TextureUnit_7,
        TextureUnit_8,
        TextureUnit_9,
        TextureUnit_10,
        TextureUnit_11,
        TextureUnit_12,
        TextureUnit_13,
        TextureUnit_14,
        TextureUnit_15,
        TextureUnit_16,
        TextureUnit_17,
        TextureUnit_18,
        TextureUnit_19,
        TextureUnit_20,
        TextureUnit_21,
        TextureUnit_22,
        TextureUnit_23,
        TextureUnit_24,
        TextureUnit_25,
        TextureUnit_26,
        TextureUnit_27,
        TextureUnit_28,
        TextureUnit_29,
        TextureUnit_30,
        TextureUnit_31
    };
};

struct NVRenderTextureCompareMode
{
    enum Enum { Unknown = 0, NoCompare, CompareToRef };
};

struct NVRenderTextureSwizzleMode
{
    enum Enum { NoSwizzle = 0, L8toR8, A8toR8, L8A8toRG8, L16toR16 };
};

#define QT3DS_RENDER_ITERATE_BOOL_OP                                                                  \
    QT3DS_RENDER_HANDLE_BOOL_OP(Never)                                                                \
    QT3DS_RENDER_HANDLE_BOOL_OP(Less)                                                                 \
    QT3DS_RENDER_HANDLE_BOOL_OP(LessThanOrEqual)                                                      \
    QT3DS_RENDER_HANDLE_BOOL_OP(Equal)                                                                \
    QT3DS_RENDER_HANDLE_BOOL_OP(NotEqual)                                                             \
    QT3DS_RENDER_HANDLE_BOOL_OP(Greater)                                                              \
    QT3DS_RENDER_HANDLE_BOOL_OP(GreaterThanOrEqual)                                                   \
    QT3DS_RENDER_HANDLE_BOOL_OP(AlwaysTrue)

struct NVRenderTextureCompareOp
{
    enum Enum {
#define QT3DS_RENDER_HANDLE_BOOL_OP(x) x,
        QT3DS_RENDER_ITERATE_BOOL_OP
#undef QT3DS_RENDER_HANDLE_BOOL_OP
    };
};

#define QT3DS_RENDER_ITERATE_TEXTURE_FILTER_OP                                                        \
    QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP(Nearest)                                                    \
    QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP(Linear)                                                     \
    QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP(NearestMipmapNearest)                                    \
    QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP(LinearMipmapNearest)                                     \
    QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP(NearestMipmapLinear)                                     \
    QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP(LinearMipmapLinear)

struct NVRenderTextureMinifyingOp
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP(x) x,
#define QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP(x) x,
        QT3DS_RENDER_ITERATE_TEXTURE_FILTER_OP
#undef QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP
#undef QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP
    };

    static const char *toString(const NVRenderTextureMinifyingOp::Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP(x)                                                      \
        case x:                                                                                        \
    return #x;
#define QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP(x)                                                   \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_TEXTURE_FILTER_OP
        #undef QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP
        #undef QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP
                default:
            break;
        }
        return "Unknown";
    }
};

struct NVRenderTextureMagnifyingOp
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP(x) x,
#define QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP(x)
        QT3DS_RENDER_ITERATE_TEXTURE_FILTER_OP
#undef QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP
#undef QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP
    };
    static const char *toString(const NVRenderTextureMagnifyingOp::Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP(x)                                                      \
        case x:                                                                                        \
    return #x;
#define QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP(x)
        QT3DS_RENDER_ITERATE_TEXTURE_FILTER_OP
        #undef QT3DS_RENDER_HANDLE_TEXTURE_MINIFYING_OP
        #undef QT3DS_RENDER_HANDLE_TEXTURE_FILTER_OP
                default:
            break;
        }
        return "Unknown";
    }
};

#define QT3DS_RENDER_ITERATE_TEXTURE_WRAP_OP                                                          \
    QT3DS_RENDER_HANDLE_TEXTURE_WRAP_OP(ClampToEdge)                                                  \
    QT3DS_RENDER_HANDLE_TEXTURE_WRAP_OP(MirroredRepeat)                                               \
    QT3DS_RENDER_HANDLE_TEXTURE_WRAP_OP(Repeat)

struct NVRenderTextureCoordOp
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_TEXTURE_WRAP_OP(x) x,
        QT3DS_RENDER_ITERATE_TEXTURE_WRAP_OP
#undef QT3DS_RENDER_HANDLE_TEXTURE_WRAP_OP
    };

    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_TEXTURE_WRAP_OP(x)                                                        \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_TEXTURE_WRAP_OP
        #undef QT3DS_RENDER_HANDLE_TEXTURE_WRAP_OP
                default:
            break;
        }
        return "Unknown";
    }
};

#define QT3DS_RENDER_ITERATE_HINT                                                                     \
    QT3DS_RENDER_HANDLE_HINT(Fastest)                                                                 \
    QT3DS_RENDER_HANDLE_HINT(Nicest)                                                                  \
    QT3DS_RENDER_HANDLE_HINT(Unspecified)

struct NVRenderHint
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_HINT(x) x,
        QT3DS_RENDER_ITERATE_HINT
#undef QT3DS_RENDER_HANDLE_HINT
    };

    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_HINT(x)                                                                   \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_HINT
        #undef QT3DS_RENDER_HANDLE_HINT
                default:
            break;
        }
        return "Unknown";
    }
};

class NVRenderImplemented
{
protected:
    virtual ~NVRenderImplemented() {}
public:
    // Get the handle that binds us to the implementation.
    // For instance, return the GLuint that came back from
    // glGenTextures.
    virtual const void *GetImplementationHandle() const = 0;
};

struct NVRenderVertexBufferEntry
{
    const char *m_Name;
    /** Datatype of the this entry points to in the buffer */
    NVRenderComponentTypes::Enum m_ComponentType;
    /** Number of components of each data member. 1,2,3, or 4.  Don't be stupid.*/
    QT3DSU32 m_NumComponents;
    /** Offset from the beginning of the buffer of the first item */
    QT3DSU32 m_FirstItemOffset;
    /** Attribute input slot used for this entry*/
    QT3DSU32 m_InputSlot;

    NVRenderVertexBufferEntry(const char *nm, NVRenderComponentTypes::Enum type,
                              QT3DSU32 numComponents, QT3DSU32 firstItemOffset = 0,
                              QT3DSU32 inputSlot = 0)
        : m_Name(nm)
        , m_ComponentType(type)
        , m_NumComponents(numComponents)
        , m_FirstItemOffset(firstItemOffset)
        , m_InputSlot(inputSlot)
    {
    }

    NVRenderVertexBufferEntry()
        : m_Name(NULL)
        , m_ComponentType(NVRenderComponentTypes::Unknown)
        , m_NumComponents(0)
        , m_FirstItemOffset(0)
        , m_InputSlot(0)
    {
    }

    NVRenderVertexBufferEntry(const NVRenderVertexBufferEntry &inOther)
        : m_Name(inOther.m_Name)
        , m_ComponentType(inOther.m_ComponentType)
        , m_NumComponents(inOther.m_NumComponents)
        , m_FirstItemOffset(inOther.m_FirstItemOffset)
        , m_InputSlot(inOther.m_InputSlot)
    {
    }

    NVRenderVertexBufferEntry &operator=(const NVRenderVertexBufferEntry &inOther)
    {
        if (this != &inOther) {
            m_Name = inOther.m_Name;
            m_ComponentType = inOther.m_ComponentType;
            m_NumComponents = inOther.m_NumComponents;
            m_FirstItemOffset = inOther.m_FirstItemOffset;
            m_InputSlot = inOther.m_InputSlot;
        }
        return *this;
    }
};

class NVRenderShaderProgram;

typedef NVConstDataRef<QT3DSI8> TConstI8Ref;

struct NVRenderVertFragCompilationResult
{
    const char *mShaderName;

    NVRenderShaderProgram *mShader; ///< contains the program

    QString errors;

    NVRenderVertFragCompilationResult()
        : mShaderName("")
        , mShader(NULL)
    {
    }
};

#define QT3DS_RENDER_ITERATE_FRAMEBUFFER_ATTACHMENTS                                                  \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Color0)                                                \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Color1)                                                \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Color2)                                                \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Color3)                                                \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Color4)                                                \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Color5)                                                \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Color6)                                                \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Color7)                                                \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Depth)                                                 \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(Stencil)                                               \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(DepthStencil)                                          \
    QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(CoverageNV)

struct NVRenderFrameBufferAttachments
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT(x) x,
        QT3DS_RENDER_ITERATE_FRAMEBUFFER_ATTACHMENTS
#undef QT3DS_RENDER_HANDLE_FRAMEBUFFER_ATTACHMENT
        LastAttachment,
    };
};

struct NVRenderDrawMode
{
    enum Enum {
        Unknown = 0,
        Points,
        LineStrip,
        LineLoop,
        Lines,
        TriangleStrip,
        TriangleFan,
        Triangles,
        Patches,
    };
};

struct NVRenderTextureCubeFaces
{
    enum Enum {
        InvalidFace = 0,
        CubePosX = 1,
        CubeNegX,
        CubePosY,
        CubeNegY,
        CubePosZ,
        CubeNegZ,
    };
};

// enums match the NV path extensions
struct NVRenderPathCommands
{
    enum Enum {
        Close = 0,
        MoveTo = 2,
        CubicCurveTo = 12,
    };
};

struct NVRenderPathFontTarget
{
    enum Enum {
        StandardFont = 0,
        SystemFont = 1,
        FileFont = 2,
    };
};

struct NVRenderPathMissingGlyphs
{
    enum Enum {
        SkipMissing = 0,
        UseMissing = 1,
    };
};

struct NVRenderPathFontStyleValues
{
    enum Enum {
        Bold = 1 << 0,
        Italic = 1 << 1,
    };
};

typedef NVFlags<NVRenderPathFontStyleValues::Enum, QT3DSU32> NVRenderPathFontStyleFlags;

struct NVRenderPathReturnValues
{
    enum Enum {
        FontGlypsAvailable = 0,
        FontTargetUnavailable = 1,
        FontUnavailable = 2,
        FontUnintelligible = 3,
        InvalidEnum = 4,
        OutOfMemory = 5,
    };
};

struct NVRenderPathFormatType
{
    enum Enum {
        Byte = 1,
        UByte,
        Short,
        UShort,
        Int,
        Uint,
        Float,
        Utf8,
        Utf16,
        Bytes2,
        Bytes3,
        Bytes4,
    };
};

struct NVRenderPathGlyphFontMetricValues
{
    enum Enum {
        GlyphWidth = 1 << 0,
        GlyphHeight = 1 << 1,
        GlyphHorizontalBearingX = 1 << 2,
        GlyphHorizontalBearingY = 1 << 3,
        GlyphHorizontalBearingAdvance = 1 << 4,
        GlyphVerticalBearingX = 1 << 5,
        GlyphVerticalBearingY = 1 << 6,
        GlyphVerticalBearingAdvance = 1 << 7,
        GlyphHasKerning = 1 << 8,

        FontXMinBounds = 1 << 9,
        FontYMinBounds = 1 << 10,
        FontXMaxBounds = 1 << 11,
        FontYMaxBounds = 1 << 12,
        FontUnitsPerEm = 1 << 13,
        FontAscender = 1 << 14,
        FontDescender = 1 << 15,
        FontHeight = 1 << 16,
        FontMaxAdvanceWidth = 1 << 17,
        FontMaxAdvanceHeight = 1 << 18,
        FontUnderlinePosition = 1 << 19,
        FontUnderlineThickness = 1 << 20,
        FontHasKerning = 1 << 21,
        FontNumGlyphIndices = 1 << 22,
    };
};

typedef NVFlags<NVRenderPathGlyphFontMetricValues::Enum, QT3DSU32>
NVRenderPathGlyphFontMetricFlags;

struct NVRenderPathListMode
{
    enum Enum {
        AccumAdjacentPairs = 1,
        AdjacentPairs,
        FirstToRest,
    };
};

struct NVRenderPathFillMode
{
    enum Enum {
        Fill = 1,
        CountUp,
        CountDown,
        Invert,
    };
};

struct NVRenderPathCoverMode
{
    enum Enum {
        ConvexHull = 1,
        BoundingBox,
        BoundingBoxOfBoundingBox,
        PathFillCover,
        PathStrokeCover,
    };
};

struct NVRenderPathTransformType
{
    enum Enum {
        NoTransform = 0,
        TranslateX,
        TranslateY,
        Translate2D,
        Translate3D,
        Affine2D,
        Affine3D,
        TransposeAffine2D,
        TransposeAffine3D,
    };
};

#define QT3DS_RENDER_ITERATE_WINDING                                                                  \
    QT3DS_RENDER_HANDLE_WINDING(Clockwise)                                                            \
    QT3DS_RENDER_HANDLE_WINDING(CounterClockwise)

struct NVRenderWinding
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_WINDING(x) x,
        QT3DS_RENDER_ITERATE_WINDING
#undef QT3DS_RENDER_HANDLE_WINDING
    };
    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_WINDING(x)                                                                \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_WINDING
        #undef QT3DS_RENDER_HANDLE_WINDING
                default:
            break;
        }
        return "Unknown";
    }
};

#define QT3DS_RENDER_ITERATE_RENDER_STATE                                                             \
    QT3DS_RENDER_HANDLE_RENDER_STATE(Blend)                                                           \
    QT3DS_RENDER_HANDLE_RENDER_STATE(CullFace)                                                        \
    QT3DS_RENDER_HANDLE_RENDER_STATE(DepthTest)                                                       \
    QT3DS_RENDER_HANDLE_RENDER_STATE(StencilTest)                                                     \
    QT3DS_RENDER_HANDLE_RENDER_STATE(ScissorTest)                                                     \
    QT3DS_RENDER_HANDLE_RENDER_STATE(DepthWrite)                                                      \
    QT3DS_RENDER_HANDLE_RENDER_STATE(Multisample)

struct NVRenderState
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_RENDER_STATE(x) x,
        QT3DS_RENDER_ITERATE_RENDER_STATE
#undef QT3DS_RENDER_HANDLE_RENDER_STATE
    };
    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_RENDER_STATE(x)                                                           \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_RENDER_STATE
        #undef QT3DS_RENDER_HANDLE_RENDER_STATE
                default:
            break;
        }
        return "Unknown";
    }
};

#define QT3DS_RENDER_ITERATE_BLEND_FUNC                                                               \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(Zero)                                                              \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(One)                                                               \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(SrcColor)                                                          \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(OneMinusSrcColor)                                                  \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(DstColor)                                                          \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(OneMinusDstColor)                                                  \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(SrcAlpha)                                                          \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(OneMinusSrcAlpha)                                                  \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(DstAlpha)                                                          \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(OneMinusDstAlpha)                                                  \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(ConstantColor)                                                     \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(OneMinusConstantColor)                                             \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(ConstantAlpha)                                                     \
    QT3DS_RENDER_HANDLE_BLEND_FUNC(OneMinusConstantAlpha)                                             \
    QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC(SrcAlphaSaturate)

struct NVRenderSrcBlendFunc
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_BLEND_FUNC(x) x,
#define QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC(x) x,
        QT3DS_RENDER_ITERATE_BLEND_FUNC
#undef QT3DS_RENDER_HANDLE_BLEND_FUNC
#undef QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC
    };

    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_BLEND_FUNC(x)                                                             \
        case x:                                                                                        \
    return #x;
#define QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC(x)                                                         \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_BLEND_FUNC
        #undef QT3DS_RENDER_HANDLE_BLEND_FUNC
        #undef QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC
                default:
            break;
        }
        return "Unknown";
    }
};

struct NVRenderDstBlendFunc
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_BLEND_FUNC(x) x,
#define QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC(x)
        QT3DS_RENDER_ITERATE_BLEND_FUNC
#undef QT3DS_RENDER_HANDLE_BLEND_FUNC
#undef QT3DS_RENDER_HANDLE_SRC_BLEND_FUNC
    };

    static const char *toString(const Enum value)
    {
        return NVRenderSrcBlendFunc::toString((NVRenderSrcBlendFunc::Enum)value);
    }
};

#define QT3DS_RENDER_ITERATE_BLEND_EQUATION                                                           \
    QT3DS_RENDER_HANDLE_BLEND_EQUATION(Add)                                                           \
    QT3DS_RENDER_HANDLE_BLEND_EQUATION(Subtract)                                                      \
    QT3DS_RENDER_HANDLE_BLEND_EQUATION(ReverseSubtract)                                               \
    QT3DS_RENDER_HANDLE_BLEND_EQUATION(Overlay)                                                       \
    QT3DS_RENDER_HANDLE_BLEND_EQUATION(ColorBurn)                                                     \
    QT3DS_RENDER_HANDLE_BLEND_EQUATION(ColorDodge)

struct NVRenderBlendEquation
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_BLEND_EQUATION(x) x,
        QT3DS_RENDER_ITERATE_BLEND_EQUATION
#undef QT3DS_RENDER_HANDLE_BLEND_EQUATION
    };
    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_BLEND_EQUATION(x)                                                         \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_BLEND_EQUATION
        #undef QT3DS_RENDER_HANDLE_BLEND_EQUATION
                default:
            break;
        }
        return "Unknown";
    }
};

#define QT3DS_RENDER_ITERATE_FACES                                                                    \
    QT3DS_RENDER_HANDLE_FACES(Front)                                                                  \
    QT3DS_RENDER_HANDLE_FACES(Back)                                                                   \
    QT3DS_RENDER_HANDLE_FACES(FrontAndBack)

struct NVRenderFaces
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_FACES(x) x,
        QT3DS_RENDER_ITERATE_FACES
#undef QT3DS_RENDER_HANDLE_FACES
    };
    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_FACES(x)                                                                  \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_FACES
        #undef QT3DS_RENDER_HANDLE_FACES
                default:
            break;
        }
        return "Unknown";
    }
};

#define QT3DS_RENDER_ITERATE_READ_FACES                                                               \
    QT3DS_RENDER_HANDLE_READ_FACES(Front)                                                             \
    QT3DS_RENDER_HANDLE_READ_FACES(Back)                                                              \
    QT3DS_RENDER_HANDLE_READ_FACES(Color0)                                                            \
    QT3DS_RENDER_HANDLE_READ_FACES(Color1)                                                            \
    QT3DS_RENDER_HANDLE_READ_FACES(Color2)                                                            \
    QT3DS_RENDER_HANDLE_READ_FACES(Color3)                                                            \
    QT3DS_RENDER_HANDLE_READ_FACES(Color4)                                                            \
    QT3DS_RENDER_HANDLE_READ_FACES(Color5)                                                            \
    QT3DS_RENDER_HANDLE_READ_FACES(Color6)                                                            \
    QT3DS_RENDER_HANDLE_READ_FACES(Color7)

struct NVReadFaces
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_READ_FACES(x) x,
        QT3DS_RENDER_ITERATE_READ_FACES
#undef QT3DS_RENDER_HANDLE_READ_FACES
    };
    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_READ_FACES(x)                                                             \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_READ_FACES
        #undef QT3DS_RENDER_HANDLE_READ_FACES
                default:
            break;
        }
        return "Unknown";
    }
};

struct NVRenderBoolOp
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_BOOL_OP(x) x,
        QT3DS_RENDER_ITERATE_BOOL_OP
#undef QT3DS_RENDER_HANDLE_BOOL_OP
    };

    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_BOOL_OP(x)                                                                \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_BOOL_OP
        #undef QT3DS_RENDER_HANDLE_BOOL_OP
                default:
            break;
        }
        return "Unknown";
    }
};

#define QT3DS_RENDER_ITERATE_STENCIL_OP                                                               \
    QT3DS_RENDER_HANDLE_STENCIL_OP(Keep)                                                              \
    QT3DS_RENDER_HANDLE_STENCIL_OP(Zero)                                                              \
    QT3DS_RENDER_HANDLE_STENCIL_OP(Replace)                                                           \
    QT3DS_RENDER_HANDLE_STENCIL_OP(Increment)                                                         \
    QT3DS_RENDER_HANDLE_STENCIL_OP(IncrementWrap)                                                     \
    QT3DS_RENDER_HANDLE_STENCIL_OP(Decrement)                                                         \
    QT3DS_RENDER_HANDLE_STENCIL_OP(DecrementWrap)                                                     \
    QT3DS_RENDER_HANDLE_STENCIL_OP(Invert)

struct NVRenderStencilOp
{
    enum Enum {
        Unknown = 0,
#define QT3DS_RENDER_HANDLE_STENCIL_OP(x) x,
        QT3DS_RENDER_ITERATE_STENCIL_OP
#undef QT3DS_RENDER_HANDLE_STENCIL_OP
    };
    static const char *toString(const Enum value)
    {
        switch (value) {
#define QT3DS_RENDER_HANDLE_STENCIL_OP(x)                                                             \
        case x:                                                                                        \
    return #x;
        QT3DS_RENDER_ITERATE_STENCIL_OP
        #undef QT3DS_RENDER_HANDLE_STENCIL_OP
                default:
            break;
        }
        return "Unknown";
    }
};

struct NVRenderBlendFunctionArgument
{
    NVRenderSrcBlendFunc::Enum m_SrcRGB;
    NVRenderDstBlendFunc::Enum m_DstRGB;
    NVRenderSrcBlendFunc::Enum m_SrcAlpha;
    NVRenderDstBlendFunc::Enum m_DstAlpha;

    NVRenderBlendFunctionArgument(NVRenderSrcBlendFunc::Enum srcRGB,
                                  NVRenderDstBlendFunc::Enum dstRGB,
                                  NVRenderSrcBlendFunc::Enum srcAlpha,
                                  NVRenderDstBlendFunc::Enum dstAlpha)
        : m_SrcRGB(srcRGB)
        , m_DstRGB(dstRGB)
        , m_SrcAlpha(srcAlpha)
        , m_DstAlpha(dstAlpha)
    {
    }

    // Default blend system premultiplies values.
    NVRenderBlendFunctionArgument()
        : m_SrcRGB(NVRenderSrcBlendFunc::SrcAlpha)
        , m_DstRGB(NVRenderDstBlendFunc::OneMinusSrcAlpha)
        , m_SrcAlpha(NVRenderSrcBlendFunc::One)
        , m_DstAlpha(NVRenderDstBlendFunc::OneMinusSrcAlpha)
    {
    }
};

struct NVRenderBlendEquationArgument
{
    NVRenderBlendEquation::Enum m_RGBEquation;
    NVRenderBlendEquation::Enum m_AlphaEquation;

    NVRenderBlendEquationArgument(NVRenderBlendEquation::Enum rgb,
                                  NVRenderBlendEquation::Enum alpha)
        : m_RGBEquation(rgb)
        , m_AlphaEquation(alpha)
    {
    }
    NVRenderBlendEquationArgument()
        : m_RGBEquation(NVRenderBlendEquation::Add)
        , m_AlphaEquation(NVRenderBlendEquation::Add)
    {
    }
};

struct NVRenderStencilOperationArgument
{
    NVRenderStencilOp::Enum m_StencilFail; // What happens when stencil test fails.
    // These values assume the stencil passed
    NVRenderStencilOp::Enum
    m_DepthFail; // What happens when the stencil passes but depth test fail.
    NVRenderStencilOp::Enum m_DepthPass; // What happens when the stencil and depth tests pass.

    NVRenderStencilOperationArgument(NVRenderStencilOp::Enum fail,
                                     NVRenderStencilOp::Enum depthFail,
                                     NVRenderStencilOp::Enum depthPass)
        : m_StencilFail(fail)
        , m_DepthFail(depthFail)
        , m_DepthPass(depthPass)
    {
    }
    NVRenderStencilOperationArgument()
        : m_StencilFail(NVRenderStencilOp::Keep)
        , m_DepthFail(NVRenderStencilOp::Keep)
        , m_DepthPass(NVRenderStencilOp::Keep)
    {
    }
    NVRenderStencilOperationArgument(const NVRenderStencilOperationArgument &StencilOp)
        : m_StencilFail(StencilOp.m_StencilFail)
        , m_DepthFail(StencilOp.m_DepthFail)
        , m_DepthPass(StencilOp.m_DepthPass)
    {
    }

    NVRenderStencilOperationArgument &operator=(const NVRenderStencilOperationArgument &rhs)
    {
        // Check for self-assignment!
        if (this == &rhs)
            return *this;

        m_StencilFail = rhs.m_StencilFail;
        m_DepthFail = rhs.m_DepthFail;
        m_DepthPass = rhs.m_DepthPass;

        return *this;
    }

    bool operator==(const NVRenderStencilOperationArgument &other) const
    {
        return (m_StencilFail == other.m_StencilFail && m_DepthFail == other.m_DepthFail
                && m_DepthPass == other.m_DepthPass);
    }
};

// see glStencilFuncSeparate
struct NVRenderStencilFunctionArgument
{
    NVRenderBoolOp::Enum m_Function;
    QT3DSU32 m_ReferenceValue;
    QT3DSU32 m_Mask;

    NVRenderStencilFunctionArgument(NVRenderBoolOp::Enum function, QT3DSU32 referenceValue,
                                    QT3DSU32 mask)
        : m_Function(function)
        , m_ReferenceValue(referenceValue)
        , m_Mask(mask)
    {
    }
    NVRenderStencilFunctionArgument()
        : m_Function(NVRenderBoolOp::AlwaysTrue)
        , m_ReferenceValue(0)
        , m_Mask((QT3DSU32)-1)
    {
    }
    NVRenderStencilFunctionArgument(const NVRenderStencilFunctionArgument &StencilFunc)
        : m_Function(StencilFunc.m_Function)
        , m_ReferenceValue(StencilFunc.m_ReferenceValue)
        , m_Mask(StencilFunc.m_Mask)
    {
    }

    NVRenderStencilFunctionArgument &operator=(const NVRenderStencilFunctionArgument &rhs)
    {
        // Check for self-assignment!
        if (this == &rhs)
            return *this;

        m_Function = rhs.m_Function;
        m_ReferenceValue = rhs.m_ReferenceValue;
        m_Mask = rhs.m_Mask;

        return *this;
    }

    bool operator==(const NVRenderStencilFunctionArgument &other) const
    {
        return (m_Function == other.m_Function && m_ReferenceValue == other.m_ReferenceValue
                && m_Mask == other.m_Mask);
    }
};

class NVRenderFrameBuffer;
class NVRenderVertexBuffer;
class NVRenderIndexBuffer;
class NVRenderShaderProgram;
class NVRenderProgramPipeline;
class NVRenderTextureBase;
class NVRenderTexture2D;
class NVRenderTexture2DArray;
class NVRenderTextureCube;
class NVRenderImage2D;
class NVRenderDataBuffer;
class NVRenderAttribLayout;
class NVRenderInputAssembler;

typedef NVRenderFrameBuffer *NVRenderFrameBufferPtr;
typedef NVRenderVertexBuffer *NVRenderVertexBufferPtr;
typedef NVRenderIndexBuffer *NVRenderIndexBufferPtr;
typedef NVRenderTexture2D *NVRenderTexture2DPtr;
typedef NVRenderTexture2DPtr *NVRenderTexture2DHandle;
typedef NVRenderTexture2DArray *NVRenderTexture2DArrayPtr;
typedef NVRenderTextureCube *NVRenderTextureCubePtr;
typedef NVRenderTextureCubePtr *NVRenderTextureCubeHandle;
typedef NVRenderImage2D *NVRenderImage2DPtr;
typedef NVRenderDataBuffer *NVRenderDataBufferPtr;
typedef const char *NVRenderConstCharPtr;
typedef bool QT3DSRenderBool;
typedef NVRenderShaderProgram *NVRenderShaderProgramPtr;
typedef NVDataRef<QT3DSU32> NVU32List;
typedef NVDataRef<const char *> NVConstCharPtrList;

template <typename TDataType>
struct NVRenderRectT
{
    typedef TDataType TRectType;
    TDataType m_X;
    TDataType m_Y;
    TDataType m_Width;
    TDataType m_Height;
    NVRenderRectT(TDataType x, TDataType y, TDataType w, TDataType h)
        : m_X(x)
        , m_Y(y)
        , m_Width(w)
        , m_Height(h)
    {
    }
    NVRenderRectT()
        : m_X(0)
        , m_Y(0)
        , m_Width(0)
        , m_Height(0)
    {
    }
    bool operator==(const NVRenderRectT<TDataType> &inOther) const
    {
        return m_X == inOther.m_X && m_Y == inOther.m_Y && m_Width == inOther.m_Width
                && m_Height == inOther.m_Height;
    }
    bool operator!=(const NVRenderRectT<TDataType> &inOther) const
    {
        return !(*this == inOther);
    }
    TDataType GetRightExtent() const { return m_X + m_Width; }
    TDataType GetBottomExtent() const { return m_Y + m_Height; }
    // Ensure this rect is inside the bounds of the other rect
    void EnsureInBounds(const NVRenderRectT<TDataType> &inOther)
    {
        TDataType rightExtent = qt3ds::NVMin(GetRightExtent(), inOther.GetRightExtent());
        TDataType bottomExtent = qt3ds::NVMin(GetBottomExtent(), inOther.GetBottomExtent());
        m_X = qt3ds::NVMax(m_X, inOther.m_X);
        m_Y = qt3ds::NVMax(m_Y, inOther.m_Y);
        m_Width = NVMax(static_cast<TDataType>(0), rightExtent - m_X);
        m_Height = NVMax(static_cast<TDataType>(0), bottomExtent - m_Y);
    }
};

// Render rects are setup to be in the coordinate space of the gl viewport,
// so x, y are left, bottom with increasing units going to the right and
// up respectively.
struct NVRenderRect : public NVRenderRectT<QT3DSI32>
{
    typedef NVRenderRectT<QT3DSI32> TBase;
    NVRenderRect(QT3DSI32 x, QT3DSI32 y, QT3DSI32 w, QT3DSI32 h)
        : TBase(x, y, w, h)
    {
    }
    NVRenderRect()
        : TBase()
    {
    }
};

struct NVRenderRectF : public NVRenderRectT<QT3DSF32>
{
    typedef NVRenderRectT<QT3DSF32> TBase;
    NVRenderRectF(QT3DSF32 x, QT3DSF32 y, QT3DSF32 w, QT3DSF32 h)
        : TBase(x, y, w, h)
    {
    }
    NVRenderRectF()
        : TBase()
    {
    }
    NVRenderRectF(const NVRenderRect &inRect)
        : TBase((QT3DSF32)inRect.m_X, (QT3DSF32)inRect.m_Y, (QT3DSF32)inRect.m_Width,
                (QT3DSF32)inRect.m_Height)
    {
    }
    NVRenderRect ToIntegerRect() const
    {
        return NVRenderRect((QT3DSI32)m_X, (QT3DSI32)m_Y, (QT3DSU32)(m_Width + .5f),
                            (QT3DSU32)(m_Height + .5f));
    }
    QT3DSVec2 BottomLeft() const { return QT3DSVec2(m_X, m_Y); }
    QT3DSVec2 Center() const
    {
        QT3DSVec2 halfDims = HalfDims();
        return QT3DSVec2(m_X + halfDims.x, m_Y + halfDims.y);
    }
    QT3DSVec2 HalfDims() const { return QT3DSVec2(m_Width / 2.0f, m_Height / 2.0f); }
    // Normalized coordinates are in the range of -1,1 where -1 is the left, bottom edges
    // and 1 is the top,right edges.
    QT3DSVec2 AbsoluteToNormalizedCoordinates(QT3DSVec2 absoluteCoordinates) const
    {
        QT3DSVec2 relativeCoords(ToRectRelative(absoluteCoordinates));
        return RelativeToNormalizedCoordinates(relativeCoords);
    };

    QT3DSVec2 RelativeToNormalizedCoordinates(QT3DSVec2 rectRelativeCoords) const
    {
        QT3DSVec2 halfDims(HalfDims());
        QT3DSVec2 retval((rectRelativeCoords.x / halfDims.x) - 1.0f,
                         (rectRelativeCoords.y / halfDims.y) - 1.0f);
        return retval;
    }

    // Take coordinates in global space and move local space where 0,0 is the center
    // of the rect but return value in pixels, not in normalized -1,1 range
    QT3DSVec2 ToNormalizedRectRelative(QT3DSVec2 absoluteCoordinates) const
    {
        // normalize them
        QT3DSVec2 relativeCoords(ToRectRelative(absoluteCoordinates));
        QT3DSVec2 halfDims(HalfDims());
        QT3DSVec2 normalized((relativeCoords.x / halfDims.x) - 1.0f,
                             (relativeCoords.y / halfDims.y) - 1.0f);
        return QT3DSVec2(normalized.x * halfDims.x, normalized.y * halfDims.y);
    }

    // Return coordinates in pixels but relative to this rect.
    QT3DSVec2 ToRectRelative(QT3DSVec2 absoluteCoordinates) const
    {
        return QT3DSVec2(absoluteCoordinates.x - m_X, absoluteCoordinates.y - m_Y);
    }

    QT3DSVec2 ToAbsoluteCoords(QT3DSVec2 inRelativeCoords) const
    {
        return QT3DSVec2(inRelativeCoords.x + m_X, inRelativeCoords.y + m_Y);
    }
};

template <typename TDataType>
struct NVRenderGenericVec2
{
    TDataType x;
    TDataType y;
    NVRenderGenericVec2(TDataType _x, TDataType _y)
        : x(_x)
        , y(_y)
    {
    }
    NVRenderGenericVec2() {}
    bool operator==(const NVRenderGenericVec2 &inOther) const
    {
        return x == inOther.x && y == inOther.y;
    }
};

template <typename TDataType>
struct NVRenderGenericVec3
{
    TDataType x;
    TDataType y;
    TDataType z;
    NVRenderGenericVec3(TDataType _x, TDataType _y, TDataType _z)
        : x(_x)
        , y(_y)
        , z(_z)
    {
    }
    NVRenderGenericVec3() {}
    bool operator==(const NVRenderGenericVec3 &inOther) const
    {
        return x == inOther.x && y == inOther.y && z == inOther.z;
    }
};

template <typename TDataType>
struct NVRenderGenericVec4
{
    TDataType x;
    TDataType y;
    TDataType z;
    TDataType w;
    NVRenderGenericVec4(TDataType _x, TDataType _y, TDataType _z, TDataType _w)
        : x(_x)
        , y(_y)
        , z(_z)
        , w(_w)
    {
    }
    NVRenderGenericVec4() {}
    bool operator==(const NVRenderGenericVec4 &inOther) const
    {
        return x == inOther.x && y == inOther.y && z == inOther.z && w == inOther.w;
    }
};

#define DECLARE_GENERIC_VECTOR_TYPE(type, numElems)                                                \
    typedef NVRenderGenericVec##numElems<type> type##_##numElems
DECLARE_GENERIC_VECTOR_TYPE(bool, 2);
DECLARE_GENERIC_VECTOR_TYPE(bool, 3);
DECLARE_GENERIC_VECTOR_TYPE(bool, 4);
DECLARE_GENERIC_VECTOR_TYPE(QT3DSU32, 2);
DECLARE_GENERIC_VECTOR_TYPE(QT3DSU32, 3);
DECLARE_GENERIC_VECTOR_TYPE(QT3DSU32, 4);
DECLARE_GENERIC_VECTOR_TYPE(QT3DSI32, 2);
DECLARE_GENERIC_VECTOR_TYPE(QT3DSI32, 3);
DECLARE_GENERIC_VECTOR_TYPE(QT3DSI32, 4);

#define ITERATE_QT3DS_SHADER_DATA_TYPES                                                               \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSI32)                                                              \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSI32_2)                                                            \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSI32_3)                                                            \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSI32_4)                                                            \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSRenderBool)                                                       \
    HANDLE_QT3DS_SHADER_DATA_TYPE(bool_2)                                                             \
    HANDLE_QT3DS_SHADER_DATA_TYPE(bool_3)                                                             \
    HANDLE_QT3DS_SHADER_DATA_TYPE(bool_4)                                                             \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSF32)                                                              \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSVec2)                                                             \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSVec3)                                                             \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSVec4)                                                             \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSU32)                                                              \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSU32_2)                                                            \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSU32_3)                                                            \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSU32_4)                                                            \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSMat33)                                                            \
    HANDLE_QT3DS_SHADER_DATA_TYPE(QT3DSMat44)                                                            \
    HANDLE_QT3DS_SHADER_DATA_TYPE(NVRenderTexture2DPtr)                                               \
    HANDLE_QT3DS_SHADER_DATA_TYPE(NVRenderTexture2DHandle)                                               \
    HANDLE_QT3DS_SHADER_DATA_TYPE(NVRenderTexture2DArrayPtr)                                          \
    HANDLE_QT3DS_SHADER_DATA_TYPE(NVRenderTextureCubePtr)                                             \
    HANDLE_QT3DS_SHADER_DATA_TYPE(NVRenderTextureCubeHandle)                                             \
    HANDLE_QT3DS_SHADER_DATA_TYPE(NVRenderImage2DPtr)                                                 \
    HANDLE_QT3DS_SHADER_DATA_TYPE(NVRenderDataBufferPtr)

struct NVRenderShaderDataTypes
{
    enum Enum {
        Unknown = 0,
#define HANDLE_QT3DS_SHADER_DATA_TYPE(type) type,
        ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE
    };
};

template <typename TDataType>
struct NVDataTypeToShaderDataTypeMap
{
};

#define HANDLE_QT3DS_SHADER_DATA_TYPE(type)                                                           \
    template <>                                                                                    \
    struct NVDataTypeToShaderDataTypeMap<type>                                                     \
{                                                                                              \
    static NVRenderShaderDataTypes::Enum GetType() { return NVRenderShaderDataTypes::type; }   \
};
ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE

template <>
struct NVDataTypeToShaderDataTypeMap<NVConstDataRef<QT3DSMat44>>
{
    static NVRenderShaderDataTypes::Enum GetType() { return NVRenderShaderDataTypes::QT3DSMat44; }
};

struct NVRenderShaderTypeValue
{
    enum Enum {
        Unknown = 1 << 0,
        Vertex = 1 << 1,
        Fragment = 1 << 2,
        TessControl = 1 << 3,
        TessEvaluation = 1 << 4,
        Geometry = 1 << 5
    };
};

typedef NVFlags<NVRenderShaderTypeValue::Enum, QT3DSU32> NVRenderShaderTypeFlags;

struct NVRenderTextureTypeValue
{
    enum Enum {
        Unknown = 0,
        Diffuse,
        Specular,
        Environment,
        Bump,
        Normal,
        Displace,
        Emissive,
        Emissive2,
        Anisotropy,
        Translucent,
        LightmapIndirect,
        LightmapRadiosity,
        LightmapShadow
    };

    static const char *toString(const NVRenderTextureTypeValue::Enum value)
    {
        switch (value) {
        case Unknown:
            return "Unknown";
        case Diffuse:
            return "Diffuse";
        case Specular:
            return "Specular";
        case Environment:
            return "Environment";
        case Bump:
            return "Bump";
        case Normal:
            return "Normal";
        case Displace:
            return "Displace";
        case Emissive:
            return "Emissive";
        case Emissive2:
            return "Emissive2";
        case Anisotropy:
            return "Anisotropy";
        case Translucent:
            return "Translucent";
        case LightmapIndirect:
            return "LightmapIndirect";
        case LightmapRadiosity:
            return "LightmapRadiosity";
        case LightmapShadow:
            return "LightmapShadow";
        default:
            return "Unknown";
        }
    }
};

#define ITERATE_QT3DS_READ_PIXEL_FORMATS                                                              \
    HANDLE_QT3DS_READ_PIXEL_FORMAT(Alpha8)                                                            \
    HANDLE_QT3DS_READ_PIXEL_FORMAT(RGB565)                                                            \
    HANDLE_QT3DS_READ_PIXEL_FORMAT(RGB8)                                                              \
    HANDLE_QT3DS_READ_PIXEL_FORMAT(RGBA4444)                                                          \
    HANDLE_QT3DS_READ_PIXEL_FORMAT(RGBA5551)                                                          \
    HANDLE_QT3DS_READ_PIXEL_FORMAT(RGBA8)

struct NVRenderReadPixelFormats
{
    enum Enum {
        Unknown = 0,
#define HANDLE_QT3DS_READ_PIXEL_FORMAT(format) format,
        ITERATE_QT3DS_READ_PIXEL_FORMATS
#undef HANDLE_QT3DS_READ_PIXEL_FORMAT
    };
};

// Now for scoped property access.
template <typename TBaseType, typename TDataType>
struct NVRenderGenericScopedProperty
{
    typedef void (TBaseType::*TSetter)(TDataType inType);
    typedef TDataType (TBaseType::*TGetter)() const;

    TBaseType &m_Context;
    TSetter m_Setter;
    TDataType m_InitialValue;
    NVRenderGenericScopedProperty(TBaseType &ctx, TGetter getter, TSetter setter)
        : m_Context(ctx)
        , m_Setter(setter)
        , m_InitialValue(((ctx).*getter)())
    {
    }
    NVRenderGenericScopedProperty(TBaseType &ctx, TGetter getter, TSetter setter,
                                  const TDataType &inNewValue)
        : m_Context(ctx)
        , m_Setter(setter)
        , m_InitialValue(((ctx).*getter)())
    {
        ((m_Context).*m_Setter)(inNewValue);
    }
    ~NVRenderGenericScopedProperty() { ((m_Context).*m_Setter)(m_InitialValue); }
};
}
}

#endif
