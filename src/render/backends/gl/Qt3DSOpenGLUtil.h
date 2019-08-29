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

#ifndef QT3DSOPENGLUTIL_H
#define QT3DSOPENGLUTIL_H

#include "render/backends/gl/Qt3DSOpenGLPrefix.h"
#include "render/backends/gl/Qt3DSOpenGLTokens.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLExtraFunctions>

#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSVec4.h"
#include "foundation/Qt3DSFoundation.h"

// The actual binding to the hardware the does some minor conversions between gles and
// the nv render enumeration types
namespace qt3ds {
namespace render {

#ifndef GL_TEXTURE_2D_MULTISAMPLE
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#endif

#ifndef GL_IMAGE_2D
#define GL_IMAGE_2D 0x904D
#endif

#ifndef GL_MULTISAMPLE_EXT
#define GL_MULTISAMPLE_EXT 0x809D
#endif

#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT4 0x8CE4
#define GL_COLOR_ATTACHMENT5 0x8CE5
#define GL_COLOR_ATTACHMENT6 0x8CE6
#define GL_COLOR_ATTACHMENT7 0x8CE7
#endif

#ifndef GL_RED
#define GL_RED 0x1903
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#endif

#ifndef GL_PATCHES
#define GL_PATCHES 0x000E
#endif

#ifndef GL_READ_ONLY
#define GL_READ_ONLY 0x88B8
#define GL_WRITE_ONLY 0x88B9
#define GL_READ_WRITE 0x88BA
#endif

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

#ifndef GL_ATOMIC_COUNTER_BUFFER
#define GL_ATOMIC_COUNTER_BUFFER 0x92C0
#endif

#ifndef GL_DRAW_INDIRECT_BUFFER
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#endif

#ifndef GL_VERTEX_SHADER_BIT
#define GL_VERTEX_SHADER_BIT 0x00000001
#endif

#ifndef GL_FRAGMENT_SHADER_BIT
#define GL_FRAGMENT_SHADER_BIT 0x00000002
#endif

#ifndef GL_GEOMETRY_SHADER_BIT
#define GL_GEOMETRY_SHADER_BIT 0x00000004
#endif

#ifndef GL_TESS_CONTROL_SHADER_BIT
#define GL_TESS_CONTROL_SHADER_BIT 0x00000008
#endif

#ifndef GL_TESS_EVALUATION_SHADER_BIT
#define GL_TESS_EVALUATION_SHADER_BIT 0x00000010
#endif

#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES 0x8D64
#endif

#ifndef GL_COMPRESSED_RED_RGTC1
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif

#ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#endif

#ifndef GL_COMPRESSED_RG_RGTC2
#define GL_COMPRESSED_RG_RGTC2 0x8DBD
#endif

#ifndef GL_COMPRESSED_SIGNED_RG_RGTC2
#define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB 0x8E8F
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB 0x8E8E
#endif

#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
#endif

#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif

#ifndef GL_COMPRESSED_SRGB8_ETC2
#define GL_COMPRESSED_SRGB8_ETC2 0x9275
#endif

#ifndef GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#endif

#ifndef GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#endif

#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#endif

#ifndef GL_COMPRESSED_R11_EAC
#define GL_COMPRESSED_R11_EAC 0x9270
#endif

#ifndef GL_COMPRESSED_SIGNED_R11_EAC
#define GL_COMPRESSED_SIGNED_R11_EAC 0x9271
#endif

#ifndef GL_COMPRESSED_RG11_EAC
#define GL_COMPRESSED_RG11_EAC 0x9272
#endif

#ifndef GL_COMPRESSED_SIGNED_RG11_EAC
#define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#endif

#define QT3DS_RENDER_ITERATE_QT3DS_GL_COLOR_FUNC                                                         \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_ZERO, Zero)                                                  \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_ONE, One)                                                    \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_SRC_COLOR, SrcColor)                                         \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_ONE_MINUS_SRC_COLOR, OneMinusSrcColor)                       \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_DST_COLOR, DstColor)                                         \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_ONE_MINUS_DST_COLOR, OneMinusDstColor)                       \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_SRC_ALPHA, SrcAlpha)                                         \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_ONE_MINUS_SRC_ALPHA, OneMinusSrcAlpha)                       \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_DST_ALPHA, DstAlpha)                                         \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_ONE_MINUS_DST_ALPHA, OneMinusDstAlpha)                       \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_CONSTANT_COLOR, ConstantColor)                               \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_ONE_MINUS_CONSTANT_COLOR, OneMinusConstantColor)             \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_CONSTANT_ALPHA, ConstantAlpha)                               \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(GL_ONE_MINUS_CONSTANT_ALPHA, OneMinusConstantAlpha)             \
    QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY(GL_SRC_ALPHA_SATURATE, SrcAlphaSaturate)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_RENDER_FACE                                                        \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_FACE(GL_FRONT, Front)                                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_FACE(GL_BACK, Back)                                              \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_FACE(GL_FRONT_AND_BACK, FrontAndBack)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_RENDER_WINDING                                                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_WINDING(GL_CW, Clockwise)                                        \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_WINDING(GL_CCW, CounterClockwise)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_BOOL_OP                                                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(GL_NEVER, Never)                                                \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(GL_LESS, Less)                                                  \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(GL_EQUAL, Equal)                                                \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(GL_LEQUAL, LessThanOrEqual)                                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(GL_GREATER, Greater)                                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(GL_NOTEQUAL, NotEqual)                                          \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(GL_GEQUAL, GreaterThanOrEqual)                                  \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(GL_ALWAYS, AlwaysTrue)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_HINT                                                               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_HINT(GL_FASTEST, Fastest)                                               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_HINT(GL_NICEST, Nicest)                                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_HINT(GL_DONT_CARE, Unspecified)

#define QT3DS_RENDER_ITERATE_QT3DS_GL_STENCIL_OP                                                         \
    QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(GL_KEEP, Keep)                                               \
    QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(GL_ZERO, Zero)                                               \
    QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(GL_REPLACE, Replace)                                         \
    QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(GL_INCR, Increment)                                          \
    QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(GL_INCR_WRAP, IncrementWrap)                                 \
    QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(GL_DECR, Decrement)                                          \
    QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(GL_DECR_WRAP, DecrementWrap)                                 \
    QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(GL_INVERT, Invert)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_BUFFER_COMPONENT_TYPES                                             \
    QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE(GL_UNSIGNED_BYTE, QT3DSU8)                                  \
    QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE(GL_BYTE, QT3DSI8)                                           \
    QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE(GL_UNSIGNED_SHORT, QT3DSU16)                                \
    QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE(GL_SHORT, QT3DSI16)                                         \
    QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE(GL_UNSIGNED_INT, QT3DSU32)                                  \
    QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE_ALIAS(GL_INT, QT3DSI32)                                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE(GL_FLOAT, QT3DSF32)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_BUFFER_USAGE_TYPE                                                  \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BUFFER_USAGE_TYPE(GL_STATIC_DRAW, Static)                               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_BUFFER_USAGE_TYPE(GL_DYNAMIC_DRAW, Dynamic)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_TEXTURE_SCALE_OP                                                   \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP(GL_NEAREST, Nearest)                                   \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP(GL_LINEAR, Linear)                                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP(GL_NEAREST_MIPMAP_NEAREST, NearestMipmapNearest)   \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP(GL_LINEAR_MIPMAP_NEAREST, LinearMipmapNearest)     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP(GL_NEAREST_MIPMAP_LINEAR, NearestMipmapLinear)     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP(GL_LINEAR_MIPMAP_LINEAR, LinearMipmapLinear)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_TEXTURE_WRAP_OP                                                    \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_WRAP_OP(GL_CLAMP_TO_EDGE, ClampToEdge)                          \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_WRAP_OP(GL_MIRRORED_REPEAT, MirroredRepeat)                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_WRAP_OP(GL_REPEAT, Repeat)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_SHADER_UNIFORM_TYPES                                               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_FLOAT, QT3DSF32)                                   \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_FLOAT_VEC2, QT3DSVec2)                             \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_FLOAT_VEC3, QT3DSVec3)                             \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_FLOAT_VEC4, QT3DSVec4)                             \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_INT, QT3DSI32)                                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_INT_VEC2, QT3DSI32_2)                              \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_INT_VEC3, QT3DSI32_3)                              \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_INT_VEC4, QT3DSI32_4)                              \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_BOOL, QT3DSRenderBool)                             \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_BOOL_VEC2, bool_2)                              \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_BOOL_VEC3, bool_3)                              \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_BOOL_VEC4, bool_4)                              \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_UNSIGNED_INT, QT3DSU32)                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_UNSIGNED_INT_VEC2, QT3DSU32_2)                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_UNSIGNED_INT_VEC3, QT3DSU32_3)                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_UNSIGNED_INT_VEC4, QT3DSU32_4)                     \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_FLOAT_MAT3, QT3DSMat33)                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_FLOAT_MAT4, QT3DSMat44)                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_SAMPLER_2D, NVRenderTexture2DPtr)               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_SAMPLER_2D_ARRAY, NVRenderTexture2DArrayPtr)    \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_SAMPLER_CUBE, NVRenderTextureCubePtr)           \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(GL_IMAGE_2D, NVRenderImage2DPtr)
// cube Sampler and mat22 unsupported

#define QT3DS_RENDER_ITERATE_GL_QT3DS_SHADER_ATTRIB_TYPES                                                \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(GL_FLOAT, QT3DSF32, 1)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(GL_FLOAT_VEC2, QT3DSF32, 2)                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(GL_FLOAT_VEC3, QT3DSF32, 3)                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(GL_FLOAT_VEC4, QT3DSF32, 4)                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(GL_FLOAT_MAT2, QT3DSF32, 4)                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(GL_FLOAT_MAT3, QT3DSF32, 9)                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(GL_FLOAT_MAT4, QT3DSF32, 16)
#if defined(GL_DEPTH_COMPONENT32)
#define QT3DS_RENDER_ITERATE_GL_QT3DS_RENDERBUFFER_FORMATS                                               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_RGBA4, RGBA4)                                    \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_RGB565, RGB565)                                  \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_RGB5_A1, RGBA5551)                               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_DEPTH_COMPONENT16, Depth16)                      \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_DEPTH_COMPONENT24, Depth24)                      \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_DEPTH_COMPONENT32, Depth32)                      \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_STENCIL_INDEX8, StencilIndex8)
#else
#define QT3DS_RENDER_ITERATE_GL_QT3DS_RENDERBUFFER_FORMATS                                               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_RGBA4, RGBA4)                                    \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_RGB565, RGB565)                                  \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_RGB5_A1, RGBA5551)                               \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_DEPTH_COMPONENT16, Depth16)                      \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_DEPTH_COMPONENT24, Depth24)                      \
    QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(GL_STENCIL_INDEX8, StencilIndex8)
#endif

#define QT3DS_RENDER_ITERATE_GL_QT3DS_FRAMEBUFFER_ATTACHMENTS                                            \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(Color0, 0)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(Color1, 1)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(Color2, 2)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(Color3, 3)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(Color4, 4)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(Color5, 5)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(Color6, 6)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(Color7, 7)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_ATTACHMENT(GL_DEPTH_ATTACHMENT, Depth)                      \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_ATTACHMENT(GL_STENCIL_ATTACHMENT, Stencil)                  \
    QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_ATTACHMENT(GL_DEPTH_STENCIL_ATTACHMENT, DepthStencil)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_CLEAR_FLAGS                                                        \
    QT3DS_RENDER_HANDLE_GL_QT3DS_CLEAR_FLAGS(GL_COLOR_BUFFER_BIT, Color)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_CLEAR_FLAGS(GL_DEPTH_BUFFER_BIT, Depth)                                 \
    QT3DS_RENDER_HANDLE_GL_QT3DS_CLEAR_FLAGS(GL_STENCIL_BUFFER_BIT, Stencil)

#define QT3DS_RENDER_ITERATE_GL_QT3DS_RENDERBUFFER_COVERAGE_FORMATS
#define QT3DS_RENDER_ITERATE_GL_QT3DS_FRAMEBUFFER_COVERAGE_ATTACHMENTS
#define QT3DS_RENDER_ITERATE_GL_QT3DS_CLEAR_COVERAGE_FLAGS

    static bool IsGlEsContext(NVRenderContextType inContextType)
    {
        NVRenderContextType esContextTypes(NVRenderContextValues::GLES2
                                           | NVRenderContextValues::GLES3
                                           | NVRenderContextValues::GLES3PLUS);

        if ((inContextType & esContextTypes))
            return true;

        return false;
    }

    struct GLConversion
    {
        GLConversion()
        { }

        static const char *processGLError(GLenum error)
        {
            const char *errorString = "";
            switch (error) {
#define stringiseError(error)                                                                      \
    case error:                                                                                    \
        errorString = #error;                                                                      \
        break
                stringiseError(GL_NO_ERROR);
                stringiseError(GL_INVALID_ENUM);
                stringiseError(GL_INVALID_VALUE);
                stringiseError(GL_INVALID_OPERATION);
                stringiseError(GL_INVALID_FRAMEBUFFER_OPERATION);
                stringiseError(GL_OUT_OF_MEMORY);
#undef stringiseError
            default:
                errorString = "Unknown GL error";
                break;
            }
            return errorString;
        }

        static NVRenderSrcBlendFunc::Enum fromGLToSrcBlendFunc(QT3DSI32 value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(srcVal, enumVal)                                            \
    case srcVal:                                                                                   \
        return NVRenderSrcBlendFunc::enumVal;
#define QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY(srcVal, enumVal)                                   \
    case srcVal:                                                                                   \
        return NVRenderSrcBlendFunc::enumVal;
                QT3DS_RENDER_ITERATE_QT3DS_GL_COLOR_FUNC
#undef QT3DS_RENDER_HANDLE_GL_COLOR_FUNC
#undef QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY
            default:
                QT3DS_ASSERT(false);
                return NVRenderSrcBlendFunc::Unknown;
            }
        }

        static GLenum fromSrcBlendFuncToGL(NVRenderSrcBlendFunc::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(srcVal, enumVal)                                            \
    case NVRenderSrcBlendFunc::enumVal:                                                            \
        return srcVal;
#define QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY(srcVal, enumVal)                                   \
    case NVRenderSrcBlendFunc::enumVal:                                                            \
        return srcVal;
                QT3DS_RENDER_ITERATE_QT3DS_GL_COLOR_FUNC
#undef QT3DS_RENDER_HANDLE_GL_COLOR_FUNC
#undef QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY
            default:
                QT3DS_ASSERT(false);
                return 0;
            }
        }

        static NVRenderDstBlendFunc::Enum fromGLToDstBlendFunc(QT3DSI32 value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(srcVal, enumVal)                                            \
    case srcVal:                                                                                   \
        return NVRenderDstBlendFunc::enumVal;
#define QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY(srcVal, enumVal)
                QT3DS_RENDER_ITERATE_QT3DS_GL_COLOR_FUNC
#undef QT3DS_RENDER_HANDLE_GL_COLOR_FUNC
#undef QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY
            default:
                QT3DS_ASSERT(false);
                return NVRenderDstBlendFunc::Unknown;
            }
        }

        static GLenum fromDstBlendFuncToGL(NVRenderDstBlendFunc::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_COLOR_FUNC(srcVal, enumVal)                                            \
    case NVRenderDstBlendFunc::enumVal:                                                            \
        return srcVal;
#define QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY(srcVal, enumVal)
                QT3DS_RENDER_ITERATE_QT3DS_GL_COLOR_FUNC
#undef QT3DS_RENDER_HANDLE_GL_COLOR_FUNC
#undef QT3DS_RENDER_HANDLE_GL_COLOR_FUNC_SRC_ONLY
            default:
                QT3DS_ASSERT(false);
                return 0;
            }
        }

        static GLenum fromBlendEquationToGL(NVRenderBlendEquation::Enum value,
                                            bool nvAdvancedBlendSupported,
                                            bool khrAdvancedBlendSupported)
        {
            switch (value) {
            case NVRenderBlendEquation::Add:
                return GL_FUNC_ADD;
            case NVRenderBlendEquation::Subtract:
                return GL_FUNC_SUBTRACT;
            case NVRenderBlendEquation::ReverseSubtract:
                return GL_FUNC_REVERSE_SUBTRACT;
            default:
                QT3DS_ASSERT(nvAdvancedBlendSupported || khrAdvancedBlendSupported);
                break;
            }

            if (nvAdvancedBlendSupported) {
                switch (value) {
                case NVRenderBlendEquation::Overlay:
                    return GL_OVERLAY_NV;
                case NVRenderBlendEquation::ColorBurn:
                    return GL_COLORBURN_NV;
                case NVRenderBlendEquation::ColorDodge:
                    return GL_COLORDODGE_NV;
                default:
                    break;
                }
            }

#if defined(GL_KHR_blend_equation_advanced)
            if (khrAdvancedBlendSupported) {
                switch (value) {
                case NVRenderBlendEquation::Overlay:
                    return GL_OVERLAY_KHR;
                case NVRenderBlendEquation::ColorBurn:
                    return GL_COLORBURN_KHR;
                case NVRenderBlendEquation::ColorDodge:
                    return GL_COLORDODGE_KHR;
                default:
                    break;
                }
            }
#endif

            QT3DS_ASSERT(false);
            return GL_FUNC_ADD;
        }

        static NVRenderFaces::Enum fromGLToFaces(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_FACE(x, y)                                                   \
    case x:                                                                                        \
        return NVRenderFaces::y;
                QT3DS_RENDER_ITERATE_GL_QT3DS_RENDER_FACE
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_FACE
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderFaces::Unknown;
        }

        static GLenum fromFacesToGL(NVRenderFaces::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_FACE(x, y)                                                   \
    case NVRenderFaces::y:                                                                         \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_RENDER_FACE
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_FACE
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVReadFaces::Enum fromGLToReadFaces(GLenum value)
        {
            switch (value) {
            case GL_FRONT:
                return NVReadFaces::Front;
            case GL_BACK:
                return NVReadFaces::Back;
            case GL_COLOR_ATTACHMENT0:
                return NVReadFaces::Color0;
            case GL_COLOR_ATTACHMENT1:
                return NVReadFaces::Color1;
            case GL_COLOR_ATTACHMENT2:
                return NVReadFaces::Color2;
            case GL_COLOR_ATTACHMENT3:
                return NVReadFaces::Color3;
            case GL_COLOR_ATTACHMENT4:
                return NVReadFaces::Color4;
            case GL_COLOR_ATTACHMENT5:
                return NVReadFaces::Color5;
            case GL_COLOR_ATTACHMENT6:
                return NVReadFaces::Color6;
            case GL_COLOR_ATTACHMENT7:
                return NVReadFaces::Color7;

            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVReadFaces::Unknown;
        }

        static GLenum fromReadFacesToGL(NVReadFaces::Enum value)
        {
            switch (value) {
            case NVReadFaces::Front:
                return GL_FRONT;
            case NVReadFaces::Back:
                return GL_BACK;
            case NVReadFaces::Color0:
                return GL_COLOR_ATTACHMENT0;
            case NVReadFaces::Color1:
                return GL_COLOR_ATTACHMENT1;
            case NVReadFaces::Color2:
                return GL_COLOR_ATTACHMENT2;
            case NVReadFaces::Color3:
                return GL_COLOR_ATTACHMENT3;
            case NVReadFaces::Color4:
                return GL_COLOR_ATTACHMENT4;
            case NVReadFaces::Color5:
                return GL_COLOR_ATTACHMENT5;
            case NVReadFaces::Color6:
                return GL_COLOR_ATTACHMENT6;
            case NVReadFaces::Color7:
                return GL_COLOR_ATTACHMENT7;
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderWinding::Enum fromGLToWinding(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_WINDING(x, y)                                                \
    case x:                                                                                        \
        return NVRenderWinding::y;
                QT3DS_RENDER_ITERATE_GL_QT3DS_RENDER_WINDING
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_WINDING
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderWinding::Unknown;
        }

        static GLenum fromWindingToGL(NVRenderWinding::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_WINDING(x, y)                                                \
    case NVRenderWinding::y:                                                                       \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_RENDER_WINDING
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_RENDER_WINDING
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderBoolOp::Enum fromGLToBoolOp(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(x, y)                                                       \
    case x:                                                                                        \
        return NVRenderBoolOp::y;
                QT3DS_RENDER_ITERATE_GL_QT3DS_BOOL_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderBoolOp::Unknown;
        }

        static GLenum fromBoolOpToGL(NVRenderBoolOp::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(x, y)                                                       \
    case NVRenderBoolOp::y:                                                                        \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_BOOL_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderHint::Enum fromGLToHint(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_HINT(x, y)                                                          \
    case x:                                                                                        \
        return NVRenderHint::y;
                QT3DS_RENDER_ITERATE_GL_QT3DS_HINT
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_HINT
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderHint::Unknown;
        }

        static GLenum fromHintToGL(NVRenderHint::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_HINT(x, y)                                                          \
    case NVRenderHint::y:                                                                          \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_HINT
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_HINT
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderStencilOp::Enum fromGLToStencilOp(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(x, y)                                                    \
    case x:                                                                                        \
        return NVRenderStencilOp::y;
                QT3DS_RENDER_ITERATE_QT3DS_GL_STENCIL_OP
#undef QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP
            default:
                break;
            }

            QT3DS_ASSERT(false);
            return NVRenderStencilOp::Unknown;
        }

        static GLenum fromStencilOpToGL(NVRenderStencilOp::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP(x, y)                                                    \
    case NVRenderStencilOp::y:                                                                     \
        return x;
                QT3DS_RENDER_ITERATE_QT3DS_GL_STENCIL_OP
#undef QT3DS_RENDER_HANDLE_QT3DS_GL_STENCIL_OP
            default:
                break;
            }

            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderComponentTypes::Enum fromGLToBufferComponentTypes(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE(x, y)                                                \
    case x:                                                                                        \
        return NVRenderComponentTypes::y;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE_ALIAS(x, y)
                QT3DS_RENDER_ITERATE_GL_QT3DS_BUFFER_COMPONENT_TYPES
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE_ALIAS
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderComponentTypes::Unknown;
        }

        static GLenum fromBufferComponentTypesToGL(NVRenderComponentTypes::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE(x, y)                                                \
    case NVRenderComponentTypes::y:                                                                \
        return x;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE_ALIAS(x, y)                                          \
    case NVRenderComponentTypes::y:                                                                \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_BUFFER_COMPONENT_TYPES
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_COMPONENT_TYPE
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static GLenum fromIndexBufferComponentsTypesToGL(NVRenderComponentTypes::Enum value)
        {
            switch (value) {
            case NVRenderComponentTypes::QT3DSU8:
                return GL_UNSIGNED_BYTE;
            case NVRenderComponentTypes::QT3DSU16:
                return GL_UNSIGNED_SHORT;
            case NVRenderComponentTypes::QT3DSU32:
                return GL_UNSIGNED_INT;
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static GLenum fromBindBufferFlagsToGL(NVRenderBufferBindFlags flags)
        {
            QT3DSU32 value = flags;
            GLenum retval = GL_INVALID_ENUM;
            if (value & NVRenderBufferBindValues::Vertex)
                retval = GL_ARRAY_BUFFER;
            else if (value & NVRenderBufferBindValues::Index)
                retval = GL_ELEMENT_ARRAY_BUFFER;
            else if (value & NVRenderBufferBindValues::Constant)
                retval = GL_UNIFORM_BUFFER;
            else if (value & NVRenderBufferBindValues::Storage)
                retval = GL_SHADER_STORAGE_BUFFER;
            else if (value & NVRenderBufferBindValues::Atomic_Counter)
                retval = GL_ATOMIC_COUNTER_BUFFER;
            else if (value & NVRenderBufferBindValues::Draw_Indirect)
                retval = GL_DRAW_INDIRECT_BUFFER;
            else
                QT3DS_ASSERT(false);

            return retval;
        }

        static NVRenderBufferBindFlags fromGLToBindBufferFlags(GLenum value)
        {
            QT3DSU32 retval = 0;

            if (value == GL_ARRAY_BUFFER)
                retval |= NVRenderBufferBindValues::Vertex;
            else if (value == GL_ELEMENT_ARRAY_BUFFER)
                retval |= NVRenderBufferBindValues::Index;
            else if (value == GL_UNIFORM_BUFFER)
                retval |= NVRenderBufferBindValues::Constant;
            else if (value == GL_SHADER_STORAGE_BUFFER)
                retval |= NVRenderBufferBindValues::Storage;
            else if (value == GL_ATOMIC_COUNTER_BUFFER)
                retval |= NVRenderBufferBindValues::Atomic_Counter;
            else if (value == GL_DRAW_INDIRECT_BUFFER)
                retval |= NVRenderBufferBindValues::Draw_Indirect;
            else
                QT3DS_ASSERT(false);

            return NVRenderBufferBindFlags(retval);
        }

        static NVRenderBufferUsageType::Enum fromGLToBufferUsageType(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_BUFFER_USAGE_TYPE(x, y)                                             \
    case x:                                                                                        \
        return NVRenderBufferUsageType::y;
                QT3DS_RENDER_ITERATE_GL_QT3DS_BUFFER_USAGE_TYPE
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_BUFFER_USAGE_TYPE
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderBufferUsageType::Unknown;
        }

        static GLenum fromBufferUsageTypeToGL(NVRenderBufferUsageType::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_BUFFER_USAGE_TYPE(x, y)                                             \
    case NVRenderBufferUsageType::y:                                                               \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_BUFFER_USAGE_TYPE
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_BUFFER_USAGE_TYPE
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static GLenum fromQueryTypeToGL(NVRenderQueryType::Enum type)
        {
            GLenum retval = GL_INVALID_ENUM;
            if (type == NVRenderQueryType::Samples)
                retval = GL_ANY_SAMPLES_PASSED;
#if defined(GL_TIME_ELAPSED)
            else if (type == NVRenderQueryType::Timer)
                retval = GL_TIME_ELAPSED;
#elif defined(GL_TIME_ELAPSED_EXT)
            else if (type == NVRenderQueryType::Timer)
                retval = GL_TIME_ELAPSED_EXT;
#endif
            else
                QT3DS_ASSERT(false);

            return retval;
        }

        static GLenum fromQueryResultTypeToGL(NVRenderQueryResultType::Enum type)
        {
            GLenum retval = GL_INVALID_ENUM;
            if (type == NVRenderQueryResultType::ResultAvailable)
                retval = GL_QUERY_RESULT_AVAILABLE;
            else if (type == NVRenderQueryResultType::Result)
                retval = GL_QUERY_RESULT;
            else
                QT3DS_ASSERT(false);

            return retval;
        }

        static GLenum fromSyncTypeToGL(NVRenderSyncType::Enum type)
        {
            GLenum retval = GL_INVALID_ENUM;
            if (type == NVRenderSyncType::GpuCommandsComplete)
                retval = GL_SYNC_GPU_COMMANDS_COMPLETE;
            else
                QT3DS_ASSERT(false);

            return retval;
        }

        static NVRenderTextureFormats::Enum
        replaceDeprecatedTextureFormat(NVRenderContextType type, NVRenderTextureFormats::Enum value,
                                       NVRenderTextureSwizzleMode::Enum &swizzleMode)
        {
            NVRenderContextType deprecatedContextFlags(NVRenderContextValues::GL2
                                                       | NVRenderContextValues::GLES2);
            NVRenderTextureFormats::Enum newValue = value;
            swizzleMode = NVRenderTextureSwizzleMode::NoSwizzle;

            if (!(type & deprecatedContextFlags)) {
                switch (value) {
                case NVRenderTextureFormats::Luminance8:
                    newValue = NVRenderTextureFormats::R8;
                    swizzleMode = NVRenderTextureSwizzleMode::L8toR8;
                    break;
                case NVRenderTextureFormats::LuminanceAlpha8:
                    newValue = NVRenderTextureFormats::RG8;
                    swizzleMode = NVRenderTextureSwizzleMode::L8A8toRG8;
                    break;
                case NVRenderTextureFormats::Alpha8:
                    newValue = NVRenderTextureFormats::R8;
                    swizzleMode = NVRenderTextureSwizzleMode::A8toR8;
                    break;
                case NVRenderTextureFormats::Luminance16:
                    newValue = NVRenderTextureFormats::R16;
                    swizzleMode = NVRenderTextureSwizzleMode::L16toR16;
                    break;
                default:
                    break;
                }
            }

            return newValue;
        }

        static void
        NVRenderConvertSwizzleModeToGL(const NVRenderTextureSwizzleMode::Enum swizzleMode,
                                       GLint glSwizzle[4])
        {
            switch (swizzleMode) {
            case NVRenderTextureSwizzleMode::L16toR16:
            case NVRenderTextureSwizzleMode::L8toR8:
                glSwizzle[0] = GL_RED;
                glSwizzle[1] = GL_RED;
                glSwizzle[2] = GL_RED;
                glSwizzle[3] = GL_ONE;
                break;
            case NVRenderTextureSwizzleMode::L8A8toRG8:
                glSwizzle[0] = GL_RED;
                glSwizzle[1] = GL_RED;
                glSwizzle[2] = GL_RED;
                glSwizzle[3] = GL_GREEN;
                break;
            case NVRenderTextureSwizzleMode::A8toR8:
                glSwizzle[0] = GL_ZERO;
                glSwizzle[1] = GL_ZERO;
                glSwizzle[2] = GL_ZERO;
                glSwizzle[3] = GL_RED;
                break;
            case NVRenderTextureSwizzleMode::NoSwizzle:
            default:
                glSwizzle[0] = GL_RED;
                glSwizzle[1] = GL_GREEN;
                glSwizzle[2] = GL_BLUE;
                glSwizzle[3] = GL_ALPHA;
                break;
            }
        }

        static bool fromUncompressedTextureFormatToGL(NVRenderContextType type,
                                                      NVRenderTextureFormats::Enum value,
                                                      GLenum &outFormat, GLenum &outDataType,
                                                      GLenum &outInternalFormat)
        {
            switch (value) {
            case NVRenderTextureFormats::R8:
                if (type == NVRenderContextValues::GLES2) {
                    outFormat = GL_ALPHA;
                    outInternalFormat = GL_ALPHA;
                } else {
                    outFormat = GL_RED;
                    outInternalFormat = GL_R8;
                }
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            case NVRenderTextureFormats::RG8:
                outFormat = GL_RG;
                outInternalFormat = GL_RG8;
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            case NVRenderTextureFormats::RGBA8:
                outFormat = GL_RGBA;
                outInternalFormat = GL_RGBA8;
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            case NVRenderTextureFormats::RGB8:
                outFormat = GL_RGB;
                outInternalFormat = GL_RGB8;
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            case NVRenderTextureFormats::RGB565:
                outFormat = GL_RGB;
                outInternalFormat = GL_RGB8;
                outDataType = GL_UNSIGNED_SHORT_5_6_5;
                return true;
            case NVRenderTextureFormats::RGBA5551:
                outFormat = GL_RGBA;
                outInternalFormat = GL_RGBA8;
                outDataType = GL_UNSIGNED_SHORT_5_5_5_1;
                return true;
            case NVRenderTextureFormats::Alpha8:
                outFormat = GL_ALPHA;
                outInternalFormat = GL_ALPHA;
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            case NVRenderTextureFormats::Luminance8:
                outFormat = GL_LUMINANCE;
                outInternalFormat = GL_LUMINANCE;
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            case NVRenderTextureFormats::LuminanceAlpha8:
                outFormat = GL_LUMINANCE_ALPHA;
                outInternalFormat = GL_LUMINANCE_ALPHA;
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            case NVRenderTextureFormats::Luminance16:
#if defined(QT_OPENGL_ES)
                outFormat = GL_LUMINANCE16F_EXT;
                outInternalFormat = GL_LUMINANCE16F_EXT;
#else
                outFormat = GL_LUMINANCE;
                outInternalFormat = GL_LUMINANCE;
#endif
                outDataType = GL_UNSIGNED_INT;
                return true;
            default:
                break;
            }

            NVRenderContextType contextFlags(NVRenderContextValues::GL2
                                             | NVRenderContextValues::GLES2);
            // check extented texture formats
            if (!(type & contextFlags)) {
                switch (value) {
#if !defined(QT_OPENGL_ES)
                case NVRenderTextureFormats::R16: {
                    if (IsGlEsContext(type)) {
                        outFormat = GL_RED_INTEGER;
                        outInternalFormat = GL_R16UI;
                    } else {
                        outFormat = GL_RED;
                        outInternalFormat = GL_R16;
                    }
                    outDataType = GL_UNSIGNED_SHORT;
                    return true;
                }
#endif
                case NVRenderTextureFormats::R16F:
                    outFormat = GL_RED;
                    outInternalFormat = GL_R16F;
                    outDataType = GL_HALF_FLOAT;
                    return true;
                case NVRenderTextureFormats::R32UI:
                    outFormat = GL_RED_INTEGER;
                    outInternalFormat = GL_R32UI;
                    outDataType = GL_UNSIGNED_INT;
                    return true;
                case NVRenderTextureFormats::R32F:
                    outFormat = GL_RED;
                    outInternalFormat = GL_R32F;
                    outDataType = GL_FLOAT;
                    return true;
                case NVRenderTextureFormats::RGBA16F:
                    outFormat = GL_RGBA;
                    outInternalFormat = GL_RGBA16F;
                    outDataType = GL_HALF_FLOAT;
                    return true;
                case NVRenderTextureFormats::RG16F:
                    outFormat = GL_RG;
                    outInternalFormat = GL_RG16F;
                    outDataType = GL_HALF_FLOAT;
                    return true;
                case NVRenderTextureFormats::RG32F:
                    outFormat = GL_RG;
                    outInternalFormat = GL_RG32F;
                    outDataType = GL_FLOAT;
                    return true;
                case NVRenderTextureFormats::RGBA32F:
                    outFormat = GL_RGBA;
                    outInternalFormat = GL_RGBA32F;
                    outDataType = GL_FLOAT;
                    return true;
                case NVRenderTextureFormats::RGB32F:
                    outFormat = GL_RGB;
                    outInternalFormat = GL_RGB32F;
                    outDataType = GL_FLOAT;
                    return true;
                case NVRenderTextureFormats::R11G11B10:
                    outFormat = GL_RGB;
                    outInternalFormat = GL_R11F_G11F_B10F;
                    outDataType = GL_UNSIGNED_INT_10F_11F_11F_REV;
                    return true;
                case NVRenderTextureFormats::RGB9E5:
                    outFormat = GL_RGB;
                    outInternalFormat = GL_RGB9_E5;
                    outDataType = GL_UNSIGNED_INT_5_9_9_9_REV;
                    return true;
                case NVRenderTextureFormats::SRGB8:
                    outFormat = GL_RGB;
                    outInternalFormat = GL_SRGB8;
                    outDataType = GL_UNSIGNED_BYTE;
                    return true;
                case NVRenderTextureFormats::SRGB8A8:
                    outFormat = GL_RGBA;
                    outInternalFormat = GL_SRGB8_ALPHA8;
                    outDataType = GL_UNSIGNED_BYTE;
                    return true;
                default:
                    break;
                }
            }

            QT3DS_ASSERT(false);
            return false;
        }

        static GLenum fromCompressedTextureFormatToGL(NVRenderTextureFormats::Enum value)
        {
            switch (value) {
            case NVRenderTextureFormats::RGBA_DXT1:
                return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            case NVRenderTextureFormats::RGB_DXT1:
                return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            case NVRenderTextureFormats::RGBA_DXT3:
                return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            case NVRenderTextureFormats::RGBA_DXT5:
                return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            case NVRenderTextureFormats::R_ATI1N_UNorm:
                return GL_COMPRESSED_RED_RGTC1;
            case NVRenderTextureFormats::R_ATI1N_SNorm:
                return GL_COMPRESSED_SIGNED_RED_RGTC1;
            case NVRenderTextureFormats::RG_ATI2N_UNorm:
                return GL_COMPRESSED_RG_RGTC2;
            case NVRenderTextureFormats::RG_ATI2N_SNorm:
                return GL_COMPRESSED_SIGNED_RG_RGTC2;
            case NVRenderTextureFormats::RGB_BP_UNSIGNED_FLOAT:
                return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
            case NVRenderTextureFormats::RGB_BP_SIGNED_FLOAT:
                return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
            case NVRenderTextureFormats::RGB_BP_UNorm:
                return GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            case NVRenderTextureFormats::R11_EAC_UNorm:
                return GL_COMPRESSED_R11_EAC;
            case NVRenderTextureFormats::R11_EAC_SNorm:
                return GL_COMPRESSED_SIGNED_R11_EAC;
            case NVRenderTextureFormats::RG11_EAC_UNorm:
                return GL_COMPRESSED_RG11_EAC;
            case NVRenderTextureFormats::RG11_EAC_SNorm:
                return GL_COMPRESSED_SIGNED_RG11_EAC;
            case NVRenderTextureFormats::RGB8_ETC2:
                return GL_COMPRESSED_RGB8_ETC2;
            case NVRenderTextureFormats::SRGB8_ETC2:
                return GL_COMPRESSED_SRGB8_ETC2;
            case NVRenderTextureFormats::RGB8_PunchThrough_Alpha1_ETC2:
                return GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
            case NVRenderTextureFormats::SRGB8_PunchThrough_Alpha1_ETC2:
                return GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2;
            case NVRenderTextureFormats::RGBA8_ETC2_EAC:
                return GL_COMPRESSED_RGBA8_ETC2_EAC;
            case NVRenderTextureFormats::SRGB8_Alpha8_ETC2_EAC:
                return GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
            case NVRenderTextureFormats::RGB8_ETC1:
                return GL_ETC1_RGB8_OES;
#ifdef GL_KHR_texture_compression_astc_hdr
            case NVRenderTextureFormats::RGBA_ASTC_4x4:
                return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_5x4:
                return GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_5x5:
                return GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_6x5:
                return GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_6x6:
                return GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_8x5:
                return GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_8x6:
                return GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_8x8:
                return  GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_10x5:
                return GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_10x6:
                return GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_10x8:
                return GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_10x10:
                return GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_12x10:
                return GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
            case NVRenderTextureFormats::RGBA_ASTC_12x12:
                return GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_4x4:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_5x4:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_5x5:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_6x5:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_6x6:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_8x5:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_8x6:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_8x8:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_10x5:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_10x6:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_10x8:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_10x10:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_12x10:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR;
            case NVRenderTextureFormats::SRGB8_Alpha8_ASTC_12x12:
                return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR;
#endif // GL_KHR_texture_compression_astc_hdr
            default:
                break;
            }

            QT3DS_ASSERT(false);
            return 0;
        }

        static bool fromDepthTextureFormatToGL(NVRenderContextType type,
                                               NVRenderTextureFormats::Enum value,
                                               GLenum &outFormat, GLenum &outDataType,
                                               GLenum &outInternalFormat)
        {
            NVRenderContextType theContextFlags(NVRenderContextValues::GLES2
                                                | NVRenderContextValues::GL2);

            bool supportDepth24 = !(type & theContextFlags);
            bool supportDepth32f = !(type & theContextFlags);
            bool supportDepth24Stencil8 = !(type & theContextFlags);

            switch (value) {
            case NVRenderTextureFormats::Depth16:
                outFormat = GL_DEPTH_COMPONENT;
                outInternalFormat = GL_DEPTH_COMPONENT16;
                outDataType = GL_UNSIGNED_SHORT;
                return true;
            case NVRenderTextureFormats::Depth24:
                outFormat = GL_DEPTH_COMPONENT;
                outInternalFormat = (supportDepth24) ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16;
                outDataType = (supportDepth24) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
                return true;
            case NVRenderTextureFormats::Depth32:
                outFormat = GL_DEPTH_COMPONENT;
                outInternalFormat =
                    (supportDepth32f) ? GL_DEPTH_COMPONENT32F : GL_DEPTH_COMPONENT16;
                outDataType = (supportDepth32f) ? GL_FLOAT : GL_UNSIGNED_SHORT;
                return true;
            case NVRenderTextureFormats::Depth24Stencil8:
                outFormat = (supportDepth24Stencil8) ? GL_DEPTH_STENCIL : GL_DEPTH_COMPONENT;
                outInternalFormat =
                    (supportDepth24Stencil8) ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT16;
                outDataType = (supportDepth24Stencil8) ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_SHORT;
                return true;
            default:
                break;
            }

            QT3DS_ASSERT(false);
            return false;
        }

        static GLenum fromTextureTargetToGL(NVRenderTextureTargetType::Enum value)
        {
            GLenum retval = 0;
            if (value == NVRenderTextureTargetType::Texture2D)
                retval = GL_TEXTURE_2D;
            else if (value == NVRenderTextureTargetType::Texture2D_MS)
                retval = GL_TEXTURE_2D_MULTISAMPLE;
            else if (value == NVRenderTextureTargetType::Texture2D_Array)
                retval = GL_TEXTURE_2D_ARRAY;
            else if (value == NVRenderTextureTargetType::TextureCube)
                retval = GL_TEXTURE_CUBE_MAP;
            else if (value == NVRenderTextureTargetType::TextureCubeNegX)
                retval = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
            else if (value == NVRenderTextureTargetType::TextureCubePosX)
                retval = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            else if (value == NVRenderTextureTargetType::TextureCubeNegY)
                retval = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
            else if (value == NVRenderTextureTargetType::TextureCubePosY)
                retval = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
            else if (value == NVRenderTextureTargetType::TextureCubeNegZ)
                retval = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
            else if (value == NVRenderTextureTargetType::TextureCubePosZ)
                retval = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
            else
                QT3DS_ASSERT(false);

            return retval;
        }

        static NVRenderTextureTargetType::Enum fromGLToTextureTarget(GLenum value)
        {
            NVRenderTextureTargetType::Enum retval = NVRenderTextureTargetType::Unknown;

            if (value == GL_TEXTURE_2D)
                retval = NVRenderTextureTargetType::Texture2D;
            else if (value == GL_TEXTURE_2D_MULTISAMPLE)
                retval = NVRenderTextureTargetType::Texture2D_MS;
            else
                QT3DS_ASSERT(false);

            return retval;
        }

        static GLenum fromTextureUnitToGL(NVRenderTextureUnit::Enum value)
        {
            QT3DSU32 v = value;
            GLenum retval = GL_TEXTURE0;
            retval = GL_TEXTURE0 + v;

            return retval;
        }

        static GLenum fromGLToTextureUnit(GLenum value)
        {
            QT3DS_ASSERT(value > GL_TEXTURE0);

            QT3DSU32 v = value - GL_TEXTURE0;
            NVRenderTextureUnit::Enum retval =
                NVRenderTextureUnit::Enum(NVRenderTextureUnit::TextureUnit_0 + v);

            return retval;
        }

        static GLenum fromTextureMinifyingOpToGL(NVRenderTextureMinifyingOp::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP(x, y)                                              \
    case NVRenderTextureMinifyingOp::y:                                                            \
        return x;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP(x, y)                                          \
    case NVRenderTextureMinifyingOp::y:                                                            \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_TEXTURE_SCALE_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderTextureMinifyingOp::Enum fromGLToTextureMinifyingOp(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP(x, y)                                              \
    case x:                                                                                        \
        return NVRenderTextureMinifyingOp::y;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP(x, y)                                          \
    case x:                                                                                        \
        return NVRenderTextureMinifyingOp::y;
                QT3DS_RENDER_ITERATE_GL_QT3DS_TEXTURE_SCALE_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderTextureMinifyingOp::Unknown;
        }

        static GLenum fromTextureMagnifyingOpToGL(NVRenderTextureMagnifyingOp::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP(x, y)                                              \
    case NVRenderTextureMagnifyingOp::y:                                                           \
        return x;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP(x, y)
                QT3DS_RENDER_ITERATE_GL_QT3DS_TEXTURE_SCALE_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderTextureMagnifyingOp::Enum fromGLToTextureMagnifyingOp(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP(x, y)                                              \
    case x:                                                                                        \
        return NVRenderTextureMagnifyingOp::y;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP(x, y)
                QT3DS_RENDER_ITERATE_GL_QT3DS_TEXTURE_SCALE_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_SCALE_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_MINIFYING_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderTextureMagnifyingOp::Unknown;
        }

        static GLenum fromTextureCoordOpToGL(NVRenderTextureCoordOp::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_WRAP_OP(x, y)                                               \
    case NVRenderTextureCoordOp::y:                                                                \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_TEXTURE_WRAP_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_WRAP_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderTextureCoordOp::Enum fromGLToTextureCoordOp(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_WRAP_OP(x, y)                                               \
    case x:                                                                                        \
        return NVRenderTextureCoordOp::y;
                QT3DS_RENDER_ITERATE_GL_QT3DS_TEXTURE_WRAP_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_TEXTURE_WRAP_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderTextureCoordOp::Unknown;
        }

        static GLenum fromTextureCompareModeToGL(NVRenderTextureCompareMode::Enum value)
        {
            switch (value) {
            case NVRenderTextureCompareMode::NoCompare:
                return GL_NONE;
            case NVRenderTextureCompareMode::CompareToRef:
                return GL_COMPARE_REF_TO_TEXTURE;
            default:
                break;
            }

            QT3DS_ASSERT(false);
            return NVRenderTextureCompareMode::Unknown;
        }

        static GLenum fromGLToTextureCompareMode(GLenum value)
        {
            switch (value) {
            case GL_NONE:
                return NVRenderTextureCompareMode::NoCompare;
            case GL_COMPARE_REF_TO_TEXTURE:
                return NVRenderTextureCompareMode::CompareToRef;
            default:
                break;
            }

            QT3DS_ASSERT(false);
            return GL_INVALID_ENUM;
        }

        static GLenum fromTextureCompareFuncToGL(NVRenderTextureCompareOp::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP(x, y)                                                       \
    case NVRenderTextureCompareOp::y:                                                              \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_BOOL_OP
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_BOOL_OP
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static GLenum fromImageFormatToGL(NVRenderTextureFormats::Enum value)
        {
            switch (value) {
            case NVRenderTextureFormats::R8:
                return GL_R8;
            case NVRenderTextureFormats::R32I:
                return GL_R32I;
            case NVRenderTextureFormats::R32UI:
                return GL_R32UI;
            case NVRenderTextureFormats::R32F:
                return GL_R32F;
            case NVRenderTextureFormats::RGBA8:
                return GL_RGBA8;
            case NVRenderTextureFormats::SRGB8A8:
                return GL_RGBA8_SNORM;
            case NVRenderTextureFormats::RG16F:
                return GL_RG16F;
            case NVRenderTextureFormats::RGBA16F:
                return GL_RGBA16F;
            case NVRenderTextureFormats::RGBA32F:
                return GL_RGBA32F;
            default:
                break;
            }

            QT3DS_ASSERT(false);
            return GL_INVALID_ENUM;
        }


        static GLenum fromImageAccessToGL(NVRenderImageAccessType::Enum value)
        {
            switch (value) {
            case NVRenderImageAccessType::Read:
                return GL_READ_ONLY;
            case NVRenderImageAccessType::Write:
                return GL_WRITE_ONLY;
            case NVRenderImageAccessType::ReadWrite:
                return GL_READ_WRITE;
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return GL_INVALID_ENUM;
        }

        static GLbitfield fromBufferAccessBitToGL(NVRenderBufferAccessFlags flags)
        {
            QT3DSU32 value = flags;
            GLbitfield retval = 0;

            if (value & NVRenderBufferAccessTypeValues::Read)
                retval |= GL_MAP_READ_BIT;
            if (value & NVRenderBufferAccessTypeValues::Write)
                retval |= GL_MAP_WRITE_BIT;
            if (value & NVRenderBufferAccessTypeValues::Invalid)
                retval |= GL_MAP_INVALIDATE_BUFFER_BIT;
            if (value & NVRenderBufferAccessTypeValues::InvalidRange)
                retval |= GL_MAP_INVALIDATE_RANGE_BIT;

            QT3DS_ASSERT(retval);
            return retval;
        }

        static GLbitfield fromMemoryBarrierFlagsToGL(NVRenderBufferBarrierFlags flags)
        {
            QT3DSU32 value = flags;
            GLbitfield retval = 0;
#if !defined(QT_OPENGL_ES)
            if (value & NVRenderBufferBarrierValues::AtomicCounter)
                retval |= GL_ATOMIC_COUNTER_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::BufferUpdate)
                retval |= GL_BUFFER_UPDATE_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::CommandBuffer)
                retval |= GL_COMMAND_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::ElementArray)
                retval |= GL_ELEMENT_ARRAY_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::Framebuffer)
                retval |= GL_FRAMEBUFFER_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::PixelBuffer)
                retval |= GL_PIXEL_BUFFER_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::ShaderImageAccess)
                retval |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::ShaderStorage)
                retval |= GL_SHADER_STORAGE_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::TextureFetch)
                retval |= GL_TEXTURE_FETCH_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::TextureUpdate)
                retval |= GL_TEXTURE_UPDATE_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::TransformFeedback)
                retval |= GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::UniformBuffer)
                retval |= GL_UNIFORM_BARRIER_BIT;
            if (value & NVRenderBufferBarrierValues::VertexAttribArray)
                retval |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
#endif
            QT3DS_ASSERT(retval);
            return retval;
        }

        static GLbitfield fromShaderTypeFlagsToGL(NVRenderShaderTypeFlags flags)
        {
            QT3DSU32 value = flags;
            GLbitfield retval = 0;
            if (value & NVRenderShaderTypeValue::Vertex)
                retval |= GL_VERTEX_SHADER_BIT;
            if (value & NVRenderShaderTypeValue::Fragment)
                retval |= GL_FRAGMENT_SHADER_BIT;
            if (value & NVRenderShaderTypeValue::TessControl)
                retval |= GL_TESS_CONTROL_SHADER_BIT;
            if (value & NVRenderShaderTypeValue::TessEvaluation)
                retval |= GL_TESS_EVALUATION_SHADER_BIT;
            if (value & NVRenderShaderTypeValue::Geometry)
#if defined(QT_OPENGL_ES_3_1)
                retval |= GL_GEOMETRY_SHADER_BIT_EXT;
#else
                retval |= GL_GEOMETRY_SHADER_BIT;
#endif
            QT3DS_ASSERT(retval || !value);
            return retval;
        }

        static GLenum fromPropertyDataTypesToShaderGL(NVRenderShaderDataTypes::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(gl, nv)                                        \
    case NVRenderShaderDataTypes::nv:                                                              \
        return gl;
                QT3DS_RENDER_ITERATE_GL_QT3DS_SHADER_UNIFORM_TYPES
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderShaderDataTypes::Enum fromShaderGLToPropertyDataTypes(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES(gl, nv)                                        \
    case gl:                                                                                       \
        return NVRenderShaderDataTypes::nv;
                QT3DS_RENDER_ITERATE_GL_QT3DS_SHADER_UNIFORM_TYPES
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_UNIFORM_TYPES
            case GL_SAMPLER_2D_SHADOW:
                return NVRenderShaderDataTypes::NVRenderTexture2DPtr;
#if !defined(QT_OPENGL_ES)
            case GL_UNSIGNED_INT_ATOMIC_COUNTER:
                return NVRenderShaderDataTypes::QT3DSU32;
            case GL_UNSIGNED_INT_IMAGE_2D:
                return NVRenderShaderDataTypes::NVRenderImage2DPtr;
#endif
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderShaderDataTypes::Unknown;
        }

        static GLenum fromComponentTypeAndNumCompsToAttribGL(NVRenderComponentTypes::Enum compType,
                                                             QT3DSU32 numComps)
        {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(gl, ct, nc)                                     \
    if (compType == NVRenderComponentTypes::ct && numComps == nc)                                  \
        return gl;
            QT3DS_RENDER_ITERATE_GL_QT3DS_SHADER_ATTRIB_TYPES
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES
            QT3DS_ASSERT(false);
            return 0;
        }

        static void fromAttribGLToComponentTypeAndNumComps(
            GLenum enumVal, NVRenderComponentTypes::Enum &outCompType, QT3DSU32 &outNumComps)
        {
            switch (enumVal) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES(gl, ct, nc)                                     \
    case gl:                                                                                       \
        outCompType = NVRenderComponentTypes::ct;                                                  \
        outNumComps = nc;                                                                          \
        return;
                QT3DS_RENDER_ITERATE_GL_QT3DS_SHADER_ATTRIB_TYPES
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_SHADER_ATTRIB_TYPES
            default:
                break;
            }
            QT3DS_ASSERT(false);
            outCompType = NVRenderComponentTypes::Unknown;
            outNumComps = 0;
        }

        static GLenum
        fromRenderBufferFormatsToRenderBufferGL(NVRenderRenderBufferFormats::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(gl, nv)                                         \
    case NVRenderRenderBufferFormats::nv:                                                          \
        return gl;
                QT3DS_RENDER_ITERATE_GL_QT3DS_RENDERBUFFER_FORMATS
                QT3DS_RENDER_ITERATE_GL_QT3DS_RENDERBUFFER_COVERAGE_FORMATS
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderRenderBufferFormats::Enum
        fromRenderBufferGLToRenderBufferFormats(GLenum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT(gl, nv)                                         \
    case gl:                                                                                       \
        return NVRenderRenderBufferFormats::nv;
                QT3DS_RENDER_ITERATE_GL_QT3DS_RENDERBUFFER_FORMATS
                QT3DS_RENDER_ITERATE_GL_QT3DS_RENDERBUFFER_COVERAGE_FORMATS
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_RENDERBUFFER_FORMAT
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderRenderBufferFormats::Unknown;
        }

        static GLenum fromFramebufferAttachmentsToGL(NVRenderFrameBufferAttachments::Enum value)
        {
            switch (value) {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(x, idx)                                \
    case NVRenderFrameBufferAttachments::x:                                                        \
        return GL_COLOR_ATTACHMENT0 + idx;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_ATTACHMENT(x, y)                                        \
    case NVRenderFrameBufferAttachments::y:                                                        \
        return x;
                QT3DS_RENDER_ITERATE_GL_QT3DS_FRAMEBUFFER_ATTACHMENTS
                QT3DS_RENDER_ITERATE_GL_QT3DS_FRAMEBUFFER_COVERAGE_ATTACHMENTS
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_ATTACHMENT
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderFrameBufferAttachments::Unknown;
        }

        static NVRenderFrameBufferAttachments::Enum fromGLToFramebufferAttachments(GLenum value)
        {
#define QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT(x, idx)                                \
    if (value == GL_COLOR_ATTACHMENT0 + idx)                                                       \
        return NVRenderFrameBufferAttachments::x;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_ATTACHMENT(x, y)                                        \
    if (value == x)                                                                                \
        return NVRenderFrameBufferAttachments::y;
            QT3DS_RENDER_ITERATE_GL_QT3DS_FRAMEBUFFER_ATTACHMENTS
            QT3DS_RENDER_ITERATE_GL_QT3DS_FRAMEBUFFER_COVERAGE_ATTACHMENTS
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_COLOR_ATTACHMENT
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_FRAMEBUFFER_ATTACHMENT
            QT3DS_ASSERT(false);
            return NVRenderFrameBufferAttachments::Unknown;
        }

        static GLbitfield fromClearFlagsToGL(NVRenderClearFlags flags)
        {
            QT3DSU32 value = flags;
            GLbitfield retval = 0;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_CLEAR_FLAGS(gl, nv)                                                 \
    if ((value & NVRenderClearValues::nv))                                                         \
        retval |= gl;
            QT3DS_RENDER_ITERATE_GL_QT3DS_CLEAR_FLAGS
            QT3DS_RENDER_ITERATE_GL_QT3DS_CLEAR_COVERAGE_FLAGS
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_CLEAR_FLAGS
            return retval;
        }

        static NVRenderClearFlags fromGLToClearFlags(GLbitfield value)
        {
            QT3DSU32 retval = 0;
#define QT3DS_RENDER_HANDLE_GL_QT3DS_CLEAR_FLAGS(gl, nv)                                                 \
    if ((value & gl))                                                                              \
        retval |= NVRenderClearValues::nv;
            QT3DS_RENDER_ITERATE_GL_QT3DS_CLEAR_FLAGS
            QT3DS_RENDER_ITERATE_GL_QT3DS_CLEAR_COVERAGE_FLAGS
#undef QT3DS_RENDER_HANDLE_GL_QT3DS_CLEAR_FLAGS
            return NVRenderClearFlags(retval);
        }

        static GLenum fromDrawModeToGL(NVRenderDrawMode::Enum value, bool inTessellationSupported)
        {
            switch (value) {
            case NVRenderDrawMode::Points:
                return GL_POINTS;
            case NVRenderDrawMode::Lines:
                return GL_LINES;
            case NVRenderDrawMode::LineStrip:
                return GL_LINE_STRIP;
            case NVRenderDrawMode::LineLoop:
                return GL_LINE_LOOP;
            case NVRenderDrawMode::TriangleStrip:
                return GL_TRIANGLE_STRIP;
            case NVRenderDrawMode::TriangleFan:
                return GL_TRIANGLE_FAN;
            case NVRenderDrawMode::Triangles:
                return GL_TRIANGLES;
            case NVRenderDrawMode::Patches:
                return (inTessellationSupported) ? GL_PATCHES : GL_TRIANGLES;
            default:
                break;
            }

            QT3DS_ASSERT(false);
            return GL_INVALID_ENUM;
        }

        static NVRenderDrawMode::Enum fromGLToDrawMode(GLenum value)
        {
            switch (value) {
            case GL_POINTS:
                return NVRenderDrawMode::Points;
            case GL_LINES:
                return NVRenderDrawMode::Lines;
            case GL_LINE_STRIP:
                return NVRenderDrawMode::LineStrip;
            case GL_LINE_LOOP:
                return NVRenderDrawMode::LineLoop;
            case GL_TRIANGLE_STRIP:
                return NVRenderDrawMode::TriangleStrip;
            case GL_TRIANGLE_FAN:
                return NVRenderDrawMode::TriangleFan;
            case GL_TRIANGLES:
                return NVRenderDrawMode::Triangles;
            case GL_PATCHES:
                return NVRenderDrawMode::Patches;
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderDrawMode::Unknown;
        }

        static GLenum fromRenderStateToGL(NVRenderState::Enum value)
        {
            switch (value) {
            case NVRenderState::Blend:
                return GL_BLEND;
            case NVRenderState::CullFace:
                return GL_CULL_FACE;
            case NVRenderState::DepthTest:
                return GL_DEPTH_TEST;
            case NVRenderState::Multisample:
#if defined(QT_OPENGL_ES)
                return GL_MULTISAMPLE_EXT;
#else
                return GL_MULTISAMPLE;
#endif
            case NVRenderState::StencilTest:
                return GL_STENCIL_TEST;
            case NVRenderState::ScissorTest:
                return GL_SCISSOR_TEST;
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return 0;
        }

        static NVRenderState::Enum fromGLToRenderState(GLenum value)
        {
            switch (value) {
            case GL_BLEND:
                return NVRenderState::Blend;
            case GL_CULL_FACE:
                return NVRenderState::CullFace;
            case GL_DEPTH_TEST:
                return NVRenderState::DepthTest;
#if defined(QT_OPENGL_ES)
            case GL_MULTISAMPLE_EXT:
#else
            case GL_MULTISAMPLE:
#endif
                return NVRenderState::Multisample;
            case GL_STENCIL_TEST:
                return NVRenderState::StencilTest;
            case GL_SCISSOR_TEST:
                return NVRenderState::ScissorTest;
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return NVRenderState::Unknown;
        }

        static bool fromReadPixelsToGlFormatAndType(NVRenderReadPixelFormats::Enum inReadPixels,
                                                    GLuint *outFormat, GLuint *outType)
        {
            switch (inReadPixels) {
            case NVRenderReadPixelFormats::Alpha8:
                *outFormat = GL_ALPHA;
                *outType = GL_UNSIGNED_BYTE;
                break;
            case NVRenderReadPixelFormats::RGB565:
                *outFormat = GL_RGB;
                *outType = GL_UNSIGNED_SHORT_5_6_5;
            case NVRenderReadPixelFormats::RGB8:
                *outFormat = GL_RGB;
                *outType = GL_UNSIGNED_BYTE;
                break;
            case NVRenderReadPixelFormats::RGBA4444:
                *outFormat = GL_RGBA;
                *outType = GL_UNSIGNED_SHORT_4_4_4_4;
                break;
            case NVRenderReadPixelFormats::RGBA5551:
                *outFormat = GL_RGBA;
                *outType = GL_UNSIGNED_SHORT_5_5_5_1;
                break;
            case NVRenderReadPixelFormats::RGBA8:
                *outFormat = GL_RGBA;
                *outType = GL_UNSIGNED_BYTE;
                break;
            default:
                *outFormat = 0;
                *outType = 0;
                QT3DS_ASSERT(false);
                return false;
            };

            return true;
        }

        static GLenum fromPathFillModeToGL(NVRenderPathFillMode::Enum inMode)
        {
            GLenum glFillMode;

            switch (inMode) {
#if !defined(QT_OPENGL_ES)
            case NVRenderPathFillMode::Fill:
                glFillMode = GL_PATH_FILL_MODE_NV;
                break;
            case NVRenderPathFillMode::CountUp:
                glFillMode = GL_COUNT_UP_NV;
                break;
            case NVRenderPathFillMode::CountDown:
                glFillMode = GL_COUNT_DOWN_NV;
                break;
            case NVRenderPathFillMode::Invert:
                glFillMode = GL_INVERT;
                break;
#endif
            default:
                QT3DS_ASSERT(false);
                break;
            }

            return glFillMode;
        }

        static GLenum fromPathFontTargetToGL(NVRenderPathFontTarget::Enum inFontTarget)
        {
            GLenum glFontTarget;

            switch (inFontTarget) {
#if !defined(QT_OPENGL_ES)
            case NVRenderPathFontTarget::StandardFont:
                glFontTarget = GL_STANDARD_FONT_NAME_NV;
                break;
            case NVRenderPathFontTarget::SystemFont:
                glFontTarget = GL_SYSTEM_FONT_NAME_NV;
                break;
            case NVRenderPathFontTarget::FileFont:
                glFontTarget = GL_FILE_NAME_NV;
                break;
#endif
            default:
                QT3DS_ASSERT(false);
                break;
            }

            return glFontTarget;
        }

        static NVRenderPathReturnValues::Enum fromGLToPathFontReturn(GLenum inReturnValue)
        {
            NVRenderPathReturnValues::Enum returnValue;

            switch (inReturnValue) {
#if !defined(QT_OPENGL_ES)
            case GL_FONT_GLYPHS_AVAILABLE_NV:
                returnValue = NVRenderPathReturnValues::FontGlypsAvailable;
                break;
            case GL_FONT_TARGET_UNAVAILABLE_NV:
                returnValue = NVRenderPathReturnValues::FontTargetUnavailable;
                break;
            case GL_FONT_UNAVAILABLE_NV:
                returnValue = NVRenderPathReturnValues::FontUnavailable;
                break;
            case GL_FONT_UNINTELLIGIBLE_NV:
                returnValue = NVRenderPathReturnValues::FontUnintelligible;
                break;
#endif
            case GL_INVALID_ENUM:
            case GL_INVALID_VALUE:
                returnValue = NVRenderPathReturnValues::InvalidEnum;
                break;
            case GL_OUT_OF_MEMORY:
                returnValue = NVRenderPathReturnValues::OutOfMemory;
                break;
            default:
                QT3DS_ASSERT(false);
                returnValue = NVRenderPathReturnValues::FontTargetUnavailable;
                break;
            }

            return returnValue;
        }

        static GLenum fromPathMissingGlyphsToGL(NVRenderPathMissingGlyphs::Enum inHandleGlyphs)
        {
            GLenum glMissingGlyphs;

            switch (inHandleGlyphs) {
#if !defined(QT_OPENGL_ES)
            case NVRenderPathMissingGlyphs::SkipMissing:
                glMissingGlyphs = GL_SKIP_MISSING_GLYPH_NV;
                break;
            case NVRenderPathMissingGlyphs::UseMissing:
                glMissingGlyphs = GL_USE_MISSING_GLYPH_NV;
                break;
#endif
            default:
                QT3DS_ASSERT(false);
                break;
            }

            return glMissingGlyphs;
        }

        static GLenum fromPathListModeToGL(NVRenderPathListMode::Enum inListMode)
        {
            GLenum glListMode;

            switch (inListMode) {
#if !defined(QT_OPENGL_ES)
            case NVRenderPathListMode::AccumAdjacentPairs:
                glListMode = GL_ACCUM_ADJACENT_PAIRS_NV;
                break;
            case NVRenderPathListMode::AdjacentPairs:
                glListMode = GL_ADJACENT_PAIRS_NV;
                break;
            case NVRenderPathListMode::FirstToRest:
                glListMode = GL_FIRST_TO_REST_NV;
                break;
#endif
            default:
                QT3DS_ASSERT(false);
                break;
            }

            return glListMode;
        }

        static GLenum fromPathCoverModeToGL(NVRenderPathCoverMode::Enum inMode)
        {
            GLenum glCoverMode;

            switch (inMode) {
#if !defined(QT_OPENGL_ES)
            case NVRenderPathCoverMode::ConvexHull:
                glCoverMode = GL_CONVEX_HULL_NV;
                break;
            case NVRenderPathCoverMode::BoundingBox:
                glCoverMode = GL_BOUNDING_BOX_NV;
                break;
            case NVRenderPathCoverMode::BoundingBoxOfBoundingBox:
                glCoverMode = GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV;
                break;
            case NVRenderPathCoverMode::PathFillCover:
                glCoverMode = GL_PATH_FILL_COVER_MODE_NV;
                break;
            case NVRenderPathCoverMode::PathStrokeCover:
                glCoverMode = GL_PATH_STROKE_COVER_MODE_NV;
                break;
#endif
            default:
                QT3DS_ASSERT(false);
                break;
            }

            return glCoverMode;
        }

        static GLenum fromPathTypeToGL(NVRenderPathFormatType::Enum value)
        {
            switch (value) {
            case NVRenderPathFormatType::Byte:
                return GL_BYTE;
            case NVRenderPathFormatType::UByte:
                return GL_UNSIGNED_BYTE;
            case NVRenderPathFormatType::Short:
                return GL_SHORT;
            case NVRenderPathFormatType::UShort:
                return GL_UNSIGNED_SHORT;
            case NVRenderPathFormatType::Int:
                return GL_INT;
            case NVRenderPathFormatType::Uint:
                return GL_UNSIGNED_INT;
#if !defined(QT_OPENGL_ES)
            case NVRenderPathFormatType::Bytes2:
                return GL_2_BYTES_NV;
            case NVRenderPathFormatType::Bytes3:
                return GL_3_BYTES_NV;
            case NVRenderPathFormatType::Bytes4:
                return GL_4_BYTES_NV;
            case NVRenderPathFormatType::Utf8:
                return GL_UTF8_NV;
            case NVRenderPathFormatType::Utf16:
                return GL_UTF16_NV;
#endif
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return GL_UNSIGNED_BYTE;
        }

        static GLbitfield fromPathFontStyleToGL(NVRenderPathFontStyleFlags flags)
        {
            QT3DSU32 value = flags;
            GLbitfield retval = 0;
#if !defined(QT_OPENGL_ES)
            if (value & NVRenderPathFontStyleValues::Bold)
                retval |= GL_BOLD_BIT_NV;
            if (value & NVRenderPathFontStyleValues::Italic)
                retval |= GL_ITALIC_BIT_NV;
#endif
            QT3DS_ASSERT(retval || !value);
            return retval;
        }

        static GLenum fromPathTransformToGL(NVRenderPathTransformType::Enum value)
        {
            switch (value) {
            case NVRenderPathTransformType::NoTransform:
                return GL_NONE;
#if !defined(QT_OPENGL_ES)
            case NVRenderPathTransformType::TranslateX:
                return GL_TRANSLATE_X_NV;
            case NVRenderPathTransformType::TranslateY:
                return GL_TRANSLATE_Y_NV;
            case NVRenderPathTransformType::Translate2D:
                return GL_TRANSLATE_2D_NV;
            case NVRenderPathTransformType::Translate3D:
                return GL_TRANSLATE_3D_NV;
            case NVRenderPathTransformType::Affine2D:
                return GL_AFFINE_2D_NV;
            case NVRenderPathTransformType::Affine3D:
                return GL_AFFINE_3D_NV;
            case NVRenderPathTransformType::TransposeAffine2D:
                return GL_TRANSPOSE_AFFINE_2D_NV;
            case NVRenderPathTransformType::TransposeAffine3D:
                return GL_TRANSPOSE_AFFINE_3D_NV;
#endif
            default:
                break;
            }
            QT3DS_ASSERT(false);
            return GL_UNSIGNED_BYTE;
        }

        static GLbitfield fromPathMetricQueryFlagsToGL(NVRenderPathGlyphFontMetricFlags flags)
        {
            QT3DSU32 value = flags;
            GLbitfield retval = 0;
#if !defined(QT_OPENGL_ES)
            if (value & NVRenderPathGlyphFontMetricValues::GlyphWidth)
                retval |= GL_GLYPH_WIDTH_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::GlyphHeight)
                retval |= GL_GLYPH_HEIGHT_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::GlyphHorizontalBearingX)
                retval |= GL_GLYPH_HORIZONTAL_BEARING_X_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::GlyphHorizontalBearingY)
                retval |= GL_GLYPH_HORIZONTAL_BEARING_Y_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::GlyphHorizontalBearingAdvance)
                retval |= GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::GlyphVerticalBearingX)
                retval |= GL_GLYPH_VERTICAL_BEARING_X_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::GlyphVerticalBearingY)
                retval |= GL_GLYPH_VERTICAL_BEARING_Y_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::GlyphVerticalBearingAdvance)
                retval |= GL_GLYPH_VERTICAL_BEARING_ADVANCE_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::GlyphHasKerning)
                retval |= GL_GLYPH_HAS_KERNING_BIT_NV;

            if (value & NVRenderPathGlyphFontMetricValues::FontXMinBounds)
                retval |= GL_FONT_X_MIN_BOUNDS_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontYMinBounds)
                retval |= GL_FONT_Y_MIN_BOUNDS_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontXMaxBounds)
                retval |= GL_FONT_X_MAX_BOUNDS_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontYMaxBounds)
                retval |= GL_FONT_Y_MAX_BOUNDS_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontUnitsPerEm)
                retval |= GL_FONT_UNITS_PER_EM_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontAscender)
                retval |= GL_FONT_ASCENDER_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontDescender)
                retval |= GL_FONT_DESCENDER_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontHeight)
                retval |= GL_FONT_HEIGHT_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontMaxAdvanceWidth)
                retval |= GL_FONT_MAX_ADVANCE_WIDTH_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontMaxAdvanceHeight)
                retval |= GL_FONT_MAX_ADVANCE_HEIGHT_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontUnderlinePosition)
                retval |= GL_FONT_UNDERLINE_POSITION_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontMaxAdvanceWidth)
                retval |= GL_FONT_UNDERLINE_THICKNESS_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontHasKerning)
                retval |= GL_FONT_HAS_KERNING_BIT_NV;
            if (value & NVRenderPathGlyphFontMetricValues::FontNumGlyphIndices)
                retval |= GL_FONT_NUM_GLYPH_INDICES_BIT_NV;
#endif
            QT3DS_ASSERT(retval || !value);
            return retval;
        }
    };
}
}

#endif // QT3DSOPENGLUTIL_H
