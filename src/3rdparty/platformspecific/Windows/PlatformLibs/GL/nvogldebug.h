/****************************************************************************
**
** Copyright (C) 2005 NVIDIA Corporation.
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

/*
* This file defines the "__NvOglDebugConfig" and NVX_debug_control interfaces. 
* Its purpose is to aid debugging and development by providing a simple way for 
* applications to control global "debug variables" in the driver.
*/

#ifndef __nvogldebug_h_
#define __nvogldebug_h_

//  An OpenGL implementation can have combinations of internal driver
//  and hardware state that are difficult to achieve by clever use of
//  the extended OpenGL API alone.  This extension provides for
//  arbitrary modifications to driver state and behavior at runtime.
//  The exact driver behaviors that can be modified are not specified
//  here, but may be specified by layered extensions.

typedef enum __GLdebugControlFunctionEnum {
    GL_DEBUG_CONTROL_FUNCTION_IS_SUPPORTED_NVX = 10,
    GL_DEBUG_CONTROL_FUNCTION_EVICT_OBJECT_NVX = 11,
    GL_DEBUG_CONTROL_FUNCTION_GET_BUGFIX_PRESENT = 12,
    GL_DEBUG_CONTROL_FUNCTION_ENABLES = 13,
    GL_DEBUG_CONTROL_FUNCTION_DETECT_FALLBACK = 14,
    GL_DEBUG_CONTROL_FUNCTION_CLAW = 15,
    GL_DEBUG_CONTROL_FUNCTION_GET_STATISTIC = 16,
    GL_DEBUG_CONTROL_FUNCTION_DO_QUADRO_SLOWDOWN = 17,
    GL_DEBUG_CONTROL_FUNCTION_BUFFER_OBJECT = 18,
    GL_DEBUG_CONTROL_FUNCTION_GET_CAPS = 19,
    GL_DEBUG_CONTROL_FUNCTION_DO_QUADRO_SLOWDOWN_US = 20,
    GL_DEBUG_CONTROL_FUNCTION_GET_GENERIC_STATISTIC = 21,
    GL_DEBUG_CONTROL_FUNCTION_INJECT_RC_ERROR = 22,
    GL_DEBUG_CONTROL_FUNCTION_FORCE_MEMSTRATEGY = 23,
    GL_DEBUG_CONTROL_FUNCTION_QUERY_APPDB = 24,
    GL_DEBUG_CONTROL_FUNCTION_SEND_3D_METHOD = 25,
    GL_DEBUG_CONTROL_FUNCTION_GL_VERSION_3_2 = 26,
    GL_DEBUG_CONTROL_FUNCTION_SET_CYCLESTATS_STATE = 27,
    GL_DEBUG_CONTROL_FUNCTION_WWMP = 28,
    GL_DEBUG_CONTROL_FUNCTION_SET_STATE_OBJECT_GPU = 29,
    GL_DEBUG_CONTROL_FUNCTION_NVOGTEST_TEST_START = 30,
    GL_DEBUG_CONTROL_FUNCTION_DUAL_CORE = 31,
    GL_DEBUG_CONTROL_FUNCTION_THREADING_OPTIMIZATIONS = 32,
    GL_DEBUG_CONTROL_FUNCTION_NX6PLUSMODE_FORCE_SYNC = 33,
    GL_DEBUG_CONTROL_FUNCTION_LOOP_ENHANCE = 34,
    GL_DEBUG_CONTROL_FUNCTION_APICOGL = 35,
    GL_DEBUG_CONTROL_FUNCTION_GET_GPU_ADDRESS = 36,
    GL_DEBUG_CONTROL_FUNCTION_EXTRA_CGC_OPTIONS = 37,
    GL_DEBUG_CONTROL_FUNCTION_ES2_READ_BUFFER_AUTO_STEREO = 38,
    GL_DEBUG_CONTROL_FUNCTION_ES2_SET_STEREO_PARAMS = 39,
    GL_DEBUG_CONTROL_FUNCTION_ES2_DISK_CACHE_ENABLE = 40,
    GL_DEBUG_CONTROL_FUNCTION_INDEX_REORDERING_CONFIG = 41,
    GL_DEBUG_CONTROL_FUNCTION_INDEX_REORDERING_STATS = 42,
} __GLdebugControlFunction;

typedef struct __GLdebugControlIsSupportedRec {
    __GLdebugControlFunction function; // [IN]
    GLboolean isSupported;             // [OUT]
} __GLdebugControlIsSupported;

typedef struct __GLdebugControlEvictObjectRec {
    GLuint objType; // [IN]
    GLuint id;      // [IN]
} __GLdebugControlEvictObject;

typedef enum {
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_393272 = 1,         // memhdrs not locked properly on download/eviction
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_LAYERED_CLEAR_PUSHBUFFER_OVERFLOW = 2,
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_395052 = 3,         // xfb falls back when raster disabled
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_399272 = 4,         // geometry program output size hardware bug
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_391451 = 5,         // xfb can't capture layer
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_389986 = 6,         // invalid shader uniform array element being updated
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_393139 = 7,         // General Vista fixes, especially multi-threading related
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_412399 = 8,         // ZCull GL_NEVER bug.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_369989 = 9,         // update internal bindings in PopClientAttribs()
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_424295 = 10,        // CopyTexImage path crashed when reading from a source with S8D24 internal buffer but no stencil image bound
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_429286 = 11,        // __glGenerateMipmaps copies texture level specification correctly
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_426399 = 12,        // ActiveTexture produces GL_INVALID_ENUM, not GL_INVALID_OPERATION for invalid textures
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_431407 = 13,        // CopyTexSubImage crashed when the texture was not initialized
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_438799 = 14,        // NP2 texture base_level bugs.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_440068 = 15,        // AA pbuffers incorrectly swapped
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_441074 = 16,        // GetRenderbufferParameteriv returns 0 for STENCIL_SIZE
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_416094 = 17,        // corruption in primitive restart
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_462805 = 18,        // wrong memspace for PBO texsubimage
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_468172 = 19,        // DSA uniform calls crash with invalid program ID
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_469086 = 20,        // VBO pulling of an edge flag array crashes
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_MAP_BUFFER_RANGE = 21,  // Assorted MBR bugs from first implementation
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_466357 = 22,        // UINT variable modifier is supported
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_455345 = 23,        // Texture API target mismatch bug (tested by tex_match nvogtest)
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_306875 = 24,        // Erroneous in-band updates to constant buffers (PaBO)
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_480517 = 25,        // glMatrixMode returns GL_INVALID_OPERATION when active texture is out of range
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_482923 = 26,        // Bufferobject counter normalization bug
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_445457 = 27,        // Texture array mipmap generation bug
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_489472 = 28,        // Shared VBO data store changes not handled
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_458167 = 29,        // Dangling gc->buffers.fbo.zeroObject->dp pointer
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_SHADEBLO_GLSL = 30,     // ShadeBLo GLSL support, developed after the extension string was added
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_491100 = 31,        // Failing to download missing mip levels when BASE_LEVEL is changed
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_VBUM_CLIENTATTRIB = 32, // Interaction between VBUM and ClientAttribDefault
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_505040 = 33,        // Interop counters cleanup
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_502904 = 34,        // EXT_texture_swizzle interaction with sRGB
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_502904_VTG = 35,    // Vertex/Tess/Geometry program EXT_texture_swizzle interaction with sRGB
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_TEX_MAXSIZE_BDR = 36,   // cubemap and array textures don't handle border properly
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_489261 = 37,        // Unbind a VBO from a VAO
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_430878 = 38,        // Fall out of SFR for a 3D blit
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_MODS_CLIPID_TEST = 39,  // driver has a MODS hack to force the use of clip IDs for basic functional testing (GL_DEBUG_CONTROL_ENABLE_MODS_CLIPID_TEST)
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_505040_2 = 40,      // changes for state machine of VBO memory allocation strategy (state_init)
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_505040_3 = 41,      // changes for state machine of VBO memory allocation strategy (state_vid)
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_LOD_FP_ONLY = 42,       // Only allow the LOD instruction in fragment programs
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_522575 = 43,        // Keep track of lockedCount in the mutex table for GL_DEBUG_CONTROL_GET_GENERIC_STATISTIC_MUTEX_WORD
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_515687 = 44,        // ARB_texture_rg: GL_RED should be legal GL_DEPTH_TEXTURE_MODE
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_EXPMS_VCAA = 45,        // VCAA depth surface can now be fetch by explicit_multisample
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_VBUM_MBR = 46,          // MapBufferRange orphan+discard doesn't work with vbum/shadeblo
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_MAPPED_BUFFER_CRASH = 47, // Deleting a context while a buffer is mapped will crash
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_MAPPED_BUFFER_CRASH_NV4X = 48, // Deleting a context while a buffer is mapped will crash (on NV4X)
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_NULL_COMPRESSED_TEXIMG = 49, // Allow NULL for glCompressedTexImage*D
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_SHADER_SUBROUTINE_CHANGE = 50, // Change to NV_shader_subroutine API
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_DSA_TOKENHELPER_BUG_560077 = 51,
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_571236 = 52,        // Interactions of dualcore, client-state, and deprecation
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_578086 = 53,        // Fixes for max attrib fragment programs
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_SEAMLESS_CUBEMAP = 54,  // seamless cubemap enum changed
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_GL32_MAX_COMPONENTS_QUERIES = 55,   // driver implements new GL3.2 MAX_*_COMPONENTS queries
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_GL32_GETBUFFERPARAMETERI64V = 56,   // driver implements missing 64b buffer parameter query
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_576302 = 57,
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_LOADIM_MULTISAMPLE_TEX = 58, // image_load_store converted to use GL3.2 multisample texture syntax
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_VOTE_IMPLEMENTED = 59,  // does NV_gpu_program5 code include support for TGALL/TGANY/TGEQ?
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_424330 = 60,
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_ALPHA_CB_FIX = 61,      // Set alpha circular buffer size for gf10y
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_GETINTEGER64I_V = 62,   // glGetInteger64i_v is present.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_2DMSARRAY_RENAMED = 63, // Assembly token 2DMSARRAY has been renamed to ARRAY2DMS
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_607673 = 64,        // Assembler properly rejects POPA with a CC mask
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_599222 = 65,        // Disabled vbo slots are still remembered across context switches.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_TEXBO_GETTEXLEVELPARAM = 66, // Support gettexlevelparameter on buffer textures
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_618201 = 67,        // Subsequent VBO ArrayElement calls with different enables cause crash
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_INT64_DIV_MOD = 68,     // Supports 64 bit integer DIV and MOD instructions
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_635764 = 69,        // Draw without attrib 0 enabled
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_PIXELPATH_PACKED_PIXELS = 71, // support for packed pixel types in the pixel path, part of ARB_texture_rgb10_a2ui
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BEGINQUERYINDEXED = 72, // XFB3 change from using separate query targets to using indexed query targets
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_INDEXED_TXWAR = 73,     // Support texture format WAR on indexed texture
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_SHADER_SUBROUTINE_CHANGE2 = 74,// Further shader subroutine changes
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_FRAGMENT_SMID = 75,     // Supports the "fragment.smid" binding for MODS (bug 663067).
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BIND_SAMPLER_UINT = 76, // ARB_sampler_objects' BindSampler implementation accepts a uint (bug 668718).
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_MISC_GL33_GL40_FIXES = 77,  // Contains fixes for miscellaneous OpenGL 3.3/4.0 impl. bugs
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_663779 = 78,        // Support IPA instruction on FPOS attribute
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_368610 = 79,        // Sharing FBOs between display connections and closing owning connection
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_674780 = 80,        // Driver does not crash when given negative values for sampler texture indices
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_681447 = 81,        // glGetIntegerv() support for GL_COPY_READ_BUFFER and GL_COPY_WRITE_BUFFER
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_SUBROUTINE_QUERY_CRASH = 82, // querying information about compatible subroutines from an unbound program crashes
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_IPA_FOGCOORD = 83,      // Support IPA instruction on fogcoord
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_710489 = 84,        // DSA + VAO fixes, mostly for glGetVertexArrayIntegeri_vEXT
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_PPO_FIXES = 85,         // Deleting a bound user pipeline updates properly and bindprogrampipeline updates active programs
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_676759 = 86,        // Linker fixes for uniform arrays with 64-bit components.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_741073 = 87,        // Linker handles Cg bug in handling matrix uniform arrays where only the last element is used.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_664244 = 88,        // Problems with arrays of GLSL subroutine uniforms, crashes on R256 x64
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_744350 = 89,        // Problems with FL-predicated subroutine calls
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_751609 = 90,        // Crash can occur when doing too many CE operations back-to-back.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_728033 = 91,        // New format support for cuda interop has been added to R270 and above.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_762723 = 92,        // UBO only allows 12 bindings 
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_763409 = 93,        // Supports DX11 - OGL interop, R270 and above.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_747891 = 94,        // Errors during GLX context creation can cause a hang without this fix.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_758921 = 95,        // ClientAttribDefault resets the primitive restart state.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_762974 = 96,        // Bad interaction of UBO and EXT_bindable_uniform in the same program.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_779707 = 97,        // BPTC textures don't require CompressedSubImage3D on 4 block boundaries
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_746642 = 98,        // Addresses returned by VBUM are made virtual to allow modeswitching with XP
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_790472 = 99,        // Querying GLSL uniform array names returns "foo[0]" and UBO uniforms return locations of -1
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_792522 = 100,       // Fix issue with aa downsample of fbo with mixed dimensions attachments
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_796862 = 101,       // Context creation of version 1.0 should result in latest OpenGL version
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_625498 = 102,       // Fix dual-core issue with glCallLists (see dlist_hell nvogtest)
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_809142 = 103,       // Incorrect restore of the program binary refcount causes memory leak
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_800902 = 104,       // X crashes when making current to GLX window whose X window is gone
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_362397 = 105,       // DivByZero expection when a Buffer attached to a texbo is resized to less than 1px.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_826092 = 106,       // GL_ARB_pixel_buffer_object: check if GL errors are correctly set
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_822935 = 107,       // LoopEnhance nvogtest support
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_857702 = 108,       // driver good enough to handle glsl_ils_fmt* tests
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_845118 = 109,       // Need to look at read drawable during read fbo validation
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_856656 = 110,       // no known compiler exceptions on glsl_ils_misc* (can run test)
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_852161 = 111,       // glMaterial doesn't update GLSL material built-in inside dlist
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_860989 = 112,       // program tracking bits not updated by glPopAttrib
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_870498 = 113,       // If drawbuffer settings do not match FBO attachment causes a crash
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_876641 = 114,       // crash binding the same FBO to two threads simultaneously
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_879671 = 115,       // drawElementsInstanced might crash if we copy not enabled vertex data
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_906242 = 116,       // glFramebufferTexture and friends are not setting a GL error when the <texture> argument is zero
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_909200 = 117,       // assembly CBUFFER and BUFFER4 arrays are sized with correct units
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_918415 = 118,       // glMakeBufferNonResidentNV doesn't ensure the buffer is resident before doing its business
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_874617 = 119,       // Fix for use-after-delete using FBO
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_920825 = 120,       // Fix crash on calling glReadPixels with invalid parameters
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_930245 = 121,       // Fix crash on calling glGetBufferPointervOES if  buffer object name 0 is bound to target
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_930248 = 122,       // Fix crash on glCopyTexSubImage3DOES when x or y is negative value
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_930253 = 123,       // Fix crash on glDrawBuffersNV when <bufs> is NULL
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_930290 = 124,       // call to glEGLImageTargetTexture2DOES when texture target is 0 excutes without error
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_934987 = 125,       // Properly round non-antialiased line widths
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_874933 = 126,       // LDDM non-CUDA-interop buffers allocated within CUDA reserved address range
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_936396 = 127,       // Linux: Damage events are not generated on parents of GL drawables when GL drawable is created after the Damage object
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_882002 = 128,       // Fix mip-mapped compressed NPOT texture's edges
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_937411 = 129,       // Fix issues with drawElementeInstanced path and usage of vertex attrib divisor
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_940255 = 130,       // Fix crash on querying EGL_STEREO_LAYOUT_NVX surface attribute value using eglQuerySurface
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_945327 = 131,       // Fixed MT event and thread handle leaks
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_IS_OBJECT  = 132,       // The IsObject routines behave correct in that they return FALSE until first bind
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_953084 = 133,       // Fix texture completeness check for EGLImage creation
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_960878 = 134,       // NV_path_rendering is broken when programmable shaders are in use 
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_874652 = 135,       // [Cg/AR20] Illegit shader compiles without error: Illegit unreferenced function does not fail to compile
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_952196 = 136,       // [Cg/AR20] Assert in vertex shader compilation when same attribute is assigned to multiple varyings
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_968228 = 137,       // Default state for UNIFORM_BUFFER_{START,SIZE} is fixed to 0, instead of original -1.
    GL_DEBUG_CONTROL_GET_BUGFIX_PRESENT_BUG_968768 = 138,       // Early Z logic is broken for stencil func GL_NEVER
} __GLdebugControlGetBugfixPresentEnum;

typedef struct __GLdebugControlGetBugfixPresentRec {
    __GLdebugControlGetBugfixPresentEnum bugfix; // [IN]
    GLboolean isSupported;                       // [OUT]
} __GLdebugControlGetBugfixPresent;

typedef enum {
    GL_DEBUG_CONTROL_ENABLE_AGGRESSIVE_EVICTION = 1,
    GL_DEBUG_CONTROL_ENABLE_MODS_CLIPID_TEST = 2,
    GL_DEBUG_CONTROL_ENABLE_WSPERF_PUSH = 3,
    GL_DEBUG_CONTROL_ENABLE_WSPERF_POP = 4,
    GL_DEBUG_CONTROL_ENABLE_WSPERF_REINIT_GC = 5,   // Required before changes take affect
    GL_DEBUG_CONTROL_ENABLE_WS_VARRAY = 6,
    GL_DEBUG_CONTROL_ENABLE_SDDR = 7,
    GL_DEBUG_CONTROL_ENABLE_SDSR = 8,
    GL_DEBUG_CONTROL_ENABLE_LAZY_VBUM = 9,
    GL_DEBUG_CONTROL_ENABLE_LOOP_ENHANCE = 10,
    GL_DEBUG_CONTROL_ENABLE_VAB_IMMEDIATE_MODE = 11,
    GL_DEBUG_CONTROL_ENABLE_VAB_VERTEXARRAY = 12,
    GL_DEBUG_CONTROL_ENABLE_CLAW = 13,
} __GLdebugControlEnableEnum;

// may return an error if the enable isn't supported
typedef struct __GLdebugControlEnablesRec {
    __GLdebugControlEnableEnum cap;              // [IN]
    GLboolean enable;                            // [IN]
    GLboolean success;                           // [OUT]
} __GLdebugControlEnables;

typedef enum {
    GL_DEBUG_CONTROL_DETECT_FALLBACK_SOFTWARE_VALIDATE  = 1,
} __GLdebugControlDetectFallbackType;

typedef struct __GLdebugControlDetectFallbackRec {
    // If a fallback of the specified type has occurred since the last reset,
    // this operation returns GL_TRUE in <result>.  If <reset> is GL_TRUE, the
    // fallback flag will be reset.
    __GLdebugControlDetectFallbackType      type;       // [IN]
    GLboolean                               reset;      // [IN]
    GLboolean                               result;     // [OUT]
} __GLdebugControlDetectFallback;

typedef enum {
    GL_DEBUG_CONTROL_DUAL_CORE_CMD_CHECK_POSSIBLE = 1,
    GL_DEBUG_CONTROL_DUAL_CORE_CMD_CURRENT_MODE,
    GL_DEBUG_CONTROL_DUAL_CORE_CMD_ENTER,
    GL_DEBUG_CONTROL_DUAL_CORE_CMD_LEAVE
} __GLdebugControlDualCoreCmd;

typedef struct __GLdebugControlDualCoreRec {
    int                     size;     // [IN] size of this struct; the cmd is ignored if there is a size mismatch
    __GLdebugControlDualCoreCmd cmd;      // [IN]
    GLint                   arg1;     // [IN]
    GLint                   result;   // [OUT]
    GLint                   success;    // [OUT]
} __GLdebugControlDualCore;


typedef enum {
    GL_DEBUG_CONTROL_LE_INIT,
    GL_DEBUG_CONTROL_LE_GET_LOOP,
    GL_DEBUG_CONTROL_LE_CHECK_LOOP,
    GL_DEBUG_CONTROL_LE_RESET_LOOP,
    GL_DEBUG_CONTROL_LE_DESTROY
} __GLdebugControlLEcmd;

typedef enum {
    GL_DEBUG_CONTROL_LE_SUCCESS,
    GL_DEBUG_CONTROL_LE_LOOP_REPLACED,
    GL_DEBUG_CONTROL_LE_NO_MORE_AVAILABLE,   // indicates no more loops or applicatiosns available
    GL_DEBUG_CONTROL_LE_FAIL,
    GL_DEBUG_CONTROL_LE_FAIL_DUE_TO_OGL32,
    GL_DEBUG_CONTROL_LE_FAIL_STRUCT_MISMATCH,
} __GLdebugControlLEreturnCodes;


typedef struct __GLdebugControlLERec  {
    __GLdebugControlLEcmd cmd;               // [IN]
    int                   structSize;        // [IN]
    __GLdebugControlLEreturnCodes result;    // [OUT]
    union {
        struct {
            int appNumber;                   // [IN]
        } init;
        struct {
            int loopNumber;                  // [IN]
            void *pFunction;                 // [OUT]
            GLboolean color;                 // [OUT]
            GLboolean texture;               // [OUT]
            GLboolean index;                 // [OUT]
        } getLoop;
    } u;
} __GLdebugControlLE;


typedef struct __GLdebugControlApicOglRec {
     void (*callbackProcDrawArrays)(void* basePtr, const GLint *first, const GLsizei *count, GLsizei primcount); // [IN]
} __GLdebugControlApicOgl;

typedef enum {
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_DISABLE_ALL = 1,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_GET_GLSL_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_SET_GLSL_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_GET_VIEWPORT_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_SET_VIEWPORT_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_GET_MATRIX_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_SET_MATRIX_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_GET_GLSTATE_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_SET_GLSTATE_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_GET_FB_STATUS_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_SET_FB_STATUS_CACHE,
    GL_DEBUG_CONTROL_THREADING_OPT_CMD_ENABLE_ALL = 0x0000FFFF,
} __GLdebugControlThreadingOptimizationsCmd;

typedef struct __GLdebugControlThreadingOptimizationsRec {
    int                     size;     // [IN] size of this struct; the cmd is ignored if there is a size mismatch
    __GLdebugControlThreadingOptimizationsCmd cmd;      // [IN]
    GLint                   arg1;     // [IN]
    GLint                   result;   // [OUT]
    GLint                   success;    // [OUT]
} __GLdebugControlThreadingOptimizations;

typedef enum {
    GL_DEBUG_CONTROL_CLAW_CMD_RESET = 1,
    GL_DEBUG_CONTROL_CLAW_CMD_GET_ENABLES,
    GL_DEBUG_CONTROL_CLAW_CMD_SET_ENABLES_AND_RESET,
    GL_DEBUG_CONTROL_CLAW_CMD_DISALLOW,
    GL_DEBUG_CONTROL_CLAW_CMD_SET_OVERRIDE_ENABLES_AND_RESET,
} __GLdebugControlClawCmd;

typedef struct __GLdebugControlClawRec {
    int                     size;     // [IN] size of this struct; the cmd is ignored if there is a size mismatch
    __GLdebugControlClawCmd cmd;      // [IN]
    GLint                   arg1;     // [IN]
    GLint                   result;   // [OUT]
    GLint                   error;    // [OUT]
} __GLdebugControlClaw;

typedef enum {
    // nvogtest test tagging
    GL_DEBUG_CONTROL_GET_GENERIC_STATISTIC_TAG_TEST = 0,
    GL_DEBUG_CONTROL_GET_GENERIC_STATISTIC_MUTEX_WORD = 1,
} __GLdebugControlGetGenericStatisticType;

typedef struct __GLdebugControlGetGenericStatisticRec {
    // If a fallback of the specified type has occurred since the last reset,
    // this operation returns GL_TRUE in <result>.  If <reset> is GL_TRUE, the
    // fallback flag will be reset. If the driver understands the type, <success>
    // will be set to GL_TRUE, GL_FALSE otherwise.
    __GLdebugControlGetGenericStatisticType type;       // [IN]
    GLboolean                               reset;      // [IN]
    GLboolean                               success;    // [OUT]
    unsigned int                            result;     // [OUT]
} __GLdebugControlGetGenericStatistic;

typedef enum {
    // for blits (copypixels, drawpixels, readpixels, pbo teximage, copyteximage),
    // report which path performed the blit.  Also count number of shaders
    // replaced, and fragment shaders that use constant color rendering.
    GL_DEBUG_CONTROL_GET_STATISTIC_BLIT_2D  = 1,
    GL_DEBUG_CONTROL_GET_STATISTIC_BLIT_3D  = 2,
    GL_DEBUG_CONTROL_GET_STATISTIC_BLIT_CE  = 3,
    GL_DEBUG_CONTROL_GET_STATISTIC_BLIT_M2M = 4,
    GL_DEBUG_CONTROL_GET_STATISTIC_SHADER_REPLACEMENT = 5,
    GL_DEBUG_CONTROL_GET_STATISTIC_CONSTANT_COLOR_RENDERING = 6,
    GL_DEBUG_CONTROL_GET_STATISTIC_BLIT_3D_PFM  = 7,    // PixelsFromMemory in 3D class (GK20A+)
} __GLdebugControlGetStatisticType;

typedef struct __GLdebugControlGetStatisticRec {
    // If a fallback of the specified type has occurred since the last reset,
    // this operation returns GL_TRUE in <result>.  If <reset> is GL_TRUE, the
    // fallback flag will be reset.
    __GLdebugControlGetStatisticType        type;       // [IN]
    GLboolean                               reset;      // [IN]
    unsigned int                            result;     // [OUT]
} __GLdebugControlGetStatistic;

typedef struct __GLdebugControlDoQuadroSlowdownRec {
    // Activates a test which will hang if the card is a GeForce.
    // when used with GL_DEBUG_CONTROL_FUNCTION_DO_QUADRO_SLOWDOWN_US, value used as microseconds
    // when used with GL_DEBUG_CONTROL_FUNCTION_DO_QUADRO_SLOWDOWN, value used as clock ticks
    unsigned int                        value;      // [IN]
} __GLdebugControlDoQuadroSlowdown;

typedef enum {
    __GL_DEBUG_CONTROL_BUFFER_OBJECT_STATE_INIT,
    __GL_DEBUG_CONTROL_BUFFER_OBJECT_STATE_STREAM_VID,
    __GL_DEBUG_CONTROL_BUFFER_OBJECT_STATE_STREAM_SYSHEAP,
    __GL_DEBUG_CONTROL_BUFFER_OBJECT_STATE_MAP_FOR_READ,
    __GL_DEBUG_CONTROL_BUFFER_OBJECT_STATE_FALLBACK,
    __GL_DEBUG_CONTROL_BUFFER_OBJECT_STATE_MIGHT_STALL,
    __GL_DEBUG_CONTROL_BUFFER_OBJECT_STATE_INTEROP
} __GLdebugControlBufferObjectState;

typedef enum {
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_GPU_READ_VTX = 0,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_GPU_READ_INDEX,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_GPU_READ_BLIT,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_CPU_READ,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_CPU_WRITE,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_GPU_WRITE_BLIT,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_USER_MAP_AS_UPDATE,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_USER_MAP_AS_READ,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_SYNC_UPDATE_STALL,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_GPU_WRITE_BLIT_NONCOHERENT,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_GPU_HIGH_PRIORITY,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_INTEROP_VIDMEM,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_INTEROP_SYSMEM,
    __GL_DEBUG_CONTROL_BUFFER_COUNTER_COUNT
} __GLdebugControlBufferCounter;

typedef enum {
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_INVALID = 0,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_MALLOC,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_SYSHEAP,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_DMA_CACHED,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_DMA_CACHED_NONCOHERENT,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_VID_ONLY,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_VID_PLUS_MALLOC,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_VID_PLUS_SYSHEAP,
    
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_ANY_GPU_PREFER_SYSHEAP,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_ANY_GPU_PREFER_VID,
    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_ANY_GPU_PREFER_CACHED,

    __GL_DEBUG_CONTROL_BUFFER_ALLOC_STRATEGY_COUNT,
} __GLdebugControlBufferAllocStrategy;

typedef enum {
    GL_DEBUG_CONTROL_BUFFER_OBJECT_SET_STALLING   = 1,
    GL_DEBUG_CONTROL_BUFFER_OBJECT_GET_STATE      = 2,
    GL_DEBUG_CONTROL_BUFFER_OBJECT_HIDE_STALLING  = 3,
    GL_DEBUG_CONTROL_BUFFER_OBJECT_TAP_COUNTER    = 4,
    GL_DEBUG_CONTROL_BUFFER_OBJECT_SET_COUNTER    = 5,
} __GLdebugControlBufferObjectCmd;

typedef struct __GLdebugControlBufferObjectRec {
    __GLdebugControlBufferObjectCmd cmd;           // [IN]
    unsigned int                    bit;           // [IN]
    unsigned int                    inc;           // [IN]
    GLenum                          target;        // [IN]
    GLboolean                       hideStalling;  // [IN]
    unsigned int                    state;         // [OUT]
} __GLdebugControlBufferObject;

typedef enum {
    GL_DEBUG_CONTROL_INDEX_REORDERING_CONFIG_READ,
    GL_DEBUG_CONTROL_INDEX_REORDERING_CONFIG_WRITE,
} __GLdebugControlIndexReorderingConfigCmd;

typedef struct __GLdebugControlIndexReorderingConfigRec {
    __GLdebugControlIndexReorderingConfigCmd cmd;           // [IN]
    unsigned int enabled;
    unsigned int minHitCnt;
    unsigned int minIndCnt;
    unsigned int synchronous;
    unsigned int subranges;
    float margin;
    float thresholdACMR;
    unsigned int ignoreThreshold; //Mostly for testing
    unsigned int forceReorder; //Mostly for testing
    unsigned int postTransformCacheSize;
    unsigned int cacheModel;
} __GLdebugControlIndexReorderingConfig;

typedef struct __GLdebugControlIndexReorderingStatsRec {
    unsigned int count;
} __GLdebugControlIndexReorderingStats;

typedef enum {
    // Capability types to be returned by GL_DEBUG_CONTROL_FUNCTION_GET_CAPS.
    // The type of <result> depends on the requested capability. The comment
    // after the type specifies the return type.
    GL_DEBUG_CONTROL_GET_CAPS_IS_QUADRO = 1,            // Out Type: GLboolean
    GL_DEBUG_CONTROL_GET_CAPS_IS_FORCED_SYSMEM = 2,     // Out Type: GLboolean
    GL_DEBUG_CONTROL_GET_CAPS_HAS_SHADOW_MAPPING_BUG_280195 = 3,     // Out Type: GLboolean

    GL_DEBUG_CONTROL_GET_CAPS_HAS_INDEX_LIMIT = 4, // Out Type: GLboolean
} __GLdebugControlGetCapsType;

typedef struct __GLdebugControlGetCapsRec {
    // Returns internal data directly from the driver. <success> is set to
    // GL_TRUE if the operation was successful.
    // The type of <result> depends upon the capability that is requested.
    __GLdebugControlGetCapsType             type;       // [IN]
    GLboolean                               success;    // [OUT]
    void*                                   result;     // [OUT]
} __GLdebugControlGetCaps;

typedef enum {
    GL_DEBUG_CONTROL_INJECT_RC_ERROR_PARSE_ERROR,
    GL_DEBUG_CONTROL_INJECT_RC_ERROR_DATA_ERROR,
} __GLdebugControlInjectRcErrorType;

typedef struct __GLdebugControlInjectRcErrorRec {
    __GLdebugControlInjectRcErrorType type;    // [IN] 
    GLboolean                         success; // [OUT]
} __GLdebugControlInjectRcError;

typedef struct __GLdebugControlQueryAppdbRec {
    GLuint *size;                     // [IN / OUT]
    GLchar *data;                     // [OUT, optional]
} __GLdebugControlQueryAppdb;

// Sends a 3D subchannel method
// **DO NOT CALL IN CHECKED IN TESTS**
typedef struct __GLdebugControlSend3DMethod {
    GLuint method;                     // [IN]
    GLuint data;                       // [IN]
} __GLdebugControlSend3DMethod;

typedef struct __GLdebugControlSetCycleStatsState {
    GLuint stateIndex;                 // [IN]
    GLuint stateValue;                 // [IN]
} __GLdebugControlSetCycleStatsState;

typedef enum {
    GL_DEBUG_CONTROL_WWMP_INSTALL_SIGNAL_HANDLER,
    GL_DEBUG_CONTROL_WWMP_ADD_WRITE_WATCH,
    GL_DEBUG_CONTROL_WWMP_DISABLE,
    GL_DEBUG_CONTROL_WWMP_GET_STATE,
} __GLdebugControlWWMPCmd;

typedef struct __GLdebugControlWWMPRec  {
    __GLdebugControlWWMPCmd cmd;             // [IN]
    union {
        struct {
            GLboolean result;                // [OUT]
            void* trackAdr;                  // [IN]
            volatile GLintptr **watchAdr;    // [OUT]
            volatile GLintptr *watchValue;   // [OUT]
            void** trackDat;                 // [OUT]
            GLboolean *isTracked;            // [OUT]
        } addWriteWatch;
        struct {
            GLboolean enabled;               // [OUT]
            int sigactionPending;            // [OUT]
            int sigactionInitialized;        // [OUT]
        }getState;
    } u;
} __GLdebugControlWWMP;

typedef struct __GLdebugControlSetStateObjectGPURec {
    GLuint64EXT handle;                     // [IN]
    GLint dynamiccount;                     // [IN]
    const GLvoid *dynamicprops;             // [IN]
} __GLdebugControlSetStateObjectGPU;

typedef enum {
    GL_DEBUG_CONTROL_GET_GPU_ADDRESS_BUFFER_OBJECT = 0,
} __GLdebugControlGetGPUAddressObjectType;

typedef struct __GLdebugControlGetGPUAddressRec {
    __GLdebugControlGetGPUAddressObjectType objectType; // [IN]
    GLenum target;                                      // [IN]
    GLboolean success;                                  // [OUT]
    GLuint64EXT address;                                // [OUT]
    GLuint64EXT size;                                   // [OUT]
} __GLdebugControlGetGPUAddress;

typedef struct __GLdebugControlExtraCgcOptionsRec {
    GLuint argc;                                        // [IN]
    GLchar **argv;                                      // [IN]
    GLboolean success;                                  // [OUT]
} __GLdebugControlExtraCgcOptions;

typedef struct __GLdebugControlNvogtestTestStartRec {
    GLboolean dcWasRestored;                            // [OUT]
} __GLdebugControlNvogtestTestStart;

typedef enum {
    GL_DEBUG_CONTROL_ES2_LEFT_EYE,
    GL_DEBUG_CONTROL_ES2_RIGHT_EYE,
} __GLdebugControlES2StereoEye;

typedef struct __GLdebugControlES2ReadBufferAutoStereoRec {
    GLuint eye;                                         // [IN]
    GLenum mode;                                        // [IN]
} __GLdebugControlES2ReadBufferAutoStereo;

typedef struct __GLdebugControlES2SetStereoParamsRec {
    GLuint control;     // 0 => Stereo Disabled, else Enabled
    GLuint method;      // NvGlStereoMethod
    GLuint changeSep;
    float  separation;
    float  convergence;
    float  cutoff;
    float  cutoffDir;
    GLuint screenWidth;
} __GLdebugControlES2SetStereoParams;

typedef struct __GLdebugControlES2DiskCacheCtrl {
    GLboolean enable;
    GLuint numEntries;
    GLuint numHits;
    GLboolean deleteCachedFiles;
    GLboolean blobEnable;
    GLuint numBlobEntries;
    GLuint numBlobHits;
} __GLdebugControlES2DiskCacheCtrl;

struct __GLdebugControlParametersRec 
{
    __GLdebugControlFunction function;

    // Note: This union is only aligned to 4B. Adding a member with a uint64 in it
    // will cause it to be aligned to 8B and break compatibility.
    union 
    {
        __GLdebugControlIsSupported isSupported;
        __GLdebugControlEvictObject evictObject;
        __GLdebugControlGetBugfixPresent getBugfixPresent;
        __GLdebugControlEnables enables;
        __GLdebugControlDetectFallback detectFallback;
        __GLdebugControlClaw claw;
        __GLdebugControlGetGenericStatistic getGenericStatistic;
        __GLdebugControlGetStatistic getStatistic;
        __GLdebugControlDoQuadroSlowdown slowdown;
        __GLdebugControlBufferObject bufObject;
        __GLdebugControlGetCaps getCaps;
        __GLdebugControlInjectRcError injectRcError;
        __GLdebugControlQueryAppdb queryAppdb;
        __GLdebugControlSend3DMethod send3DMethod;
        __GLdebugControlSetCycleStatsState setCycleStatsState;
        __GLdebugControlWWMP wwmp;
        // Add new entries to u2 instead of u.
    } u;

    // This union is 8B-aligned.
    union
    {
        GLuint64EXT x; // force 8B alignment.
        __GLdebugControlSetStateObjectGPU setStateObjectGPU;
        __GLdebugControlThreadingOptimizations threadOptimizations;
        __GLdebugControlDualCore dualCore;
        __GLdebugControlLE loopEnhance;
        __GLdebugControlApicOgl apicOgl;
        __GLdebugControlGetGPUAddress getGPUAddress;
        __GLdebugControlExtraCgcOptions extraCgcOptions;
        __GLdebugControlNvogtestTestStart nvogtestTestStart;
        __GLdebugControlES2ReadBufferAutoStereo readBufferAutoStereo;
        __GLdebugControlES2SetStereoParams setStereoParams;
        __GLdebugControlES2DiskCacheCtrl diskCacheCtrl;
        __GLdebugControlIndexReorderingConfig ibrCfg;
        __GLdebugControlIndexReorderingStats ibrStats;
    } u2;
};


// All implementations that export this entrypoint support the function
// IS_SUPPORTED.  Whether or not other functions are supported is
// implementation defined.
//
// The correct usage is as follows.  Let's say you want to use the function
// EVICT_OBJECT:
//
//   1) glDebugControlNVX( {IS_SUPPORTED, EVICT_OBJECT, &result} );
//   2) stop if result is FALSE
//   3) glDebugControlNVX( {EVICT_OBJECT, objType, id} );
//
#define PROC_DebugControlNVX gl6813513874685413


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// The remainder of this file is the legacy interface.  It is still used by
// nvogtest and nvapi.  Eventually uses of the old interface will be ported to
// the new interface, at which time the old interface will be removed.
//
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



// bump this every time you change the list of recognized <pname> values.
#define __NVOGLDEBUG_ENUM_VERSION   3

// The following values may be passed as <pname> to
// __NvOglDebugSeti and __NvOglDebugGeti.
//
// __NVOGLDEBUG_USER_1 and __NVOGLDEBUG_USER_2 should never be used in
// checked-in code.  Their purpose is for quick hacks while debugging or
// developing new code.  By including them here a priori, an engineer can
// quickly use them without modifying a key .h file that causes recompilation
// of the world.
//
// __NVOGLDEBUG_NVTRACE_LEVEL refers to the nvTraceLevel.
//
#define __NVOGLDEBUG_USER_1                             0x8C50
#define __NVOGLDEBUG_USER_2                             0x8C51
#define __NVOGLDEBUG_NVTRACE_LEVEL                      0x8C52
#define __NVOGLDEBUG_NVTRACE_OPTIONS                    0x8C53
#define __NVOGLDEBUG_VERSION                            0x8C54
#define __NVOGLDEBUG_TAG_TEST                           0x8C55

#define __NVOGLDEBUG_NVEXPERT_LEVEL                     0x8C56
#define __NVOGLDEBUG_NVEXPERT_CALLBACK                  0x8C57
#define __NVOGLDEBUG_NVEXPERT_MASK0                     0x8C58

#define __NVOGLDEBUG_TEMP0                              0x8C59
#define __NVOGLDEBUG_TEMP1                              0x8C5A
#define __NVOGLDEBUG_TEMP2                              0x8C5B
#define __NVOGLDEBUG_TEMP3                              0x8C5C
#define __NVOGLDEBUG_TEMP4                              0x8C5D
#define __NVOGLDEBUG_INSTRUMENTATION_ENABLED            0x8C5E
#define __NVOGLDEBUG_OPTIONS                            0x8C5F

#define __NVOGLDEBUG_NVTRACE_MASK0                      0x8C60
#define __NVOGLDEBUG_NVTRACE_MASK1                      0x8C61
#define __NVOGLDEBUG_NVTRACE_MASK2                      0x8C62
#define __NVOGLDEBUG_NVTRACE_MASK3                      0x8C63
#define __NVOGLDEBUG_NVTRACE_MASK4                      0x8C64
#define __NVOGLDEBUG_NVTRACE_MASK5                      0x8C65
#define __NVOGLDEBUG_NVTRACE_MASK6                      0x8C66
#define __NVOGLDEBUG_NVTRACE_MASK7                      0x8C67
#define __NVOGLDEBUG_NVTRACE_MASK8                      0x8C68
#define __NVOGLDEBUG_NVTRACE_MASK9                      0x8C69
#define __NVOGLDEBUG_NVTRACE_MASK10                     0x8C6A
#define __NVOGLDEBUG_NVTRACE_MASK11                     0x8C6B
#define __NVOGLDEBUG_NVTRACE_MASK12                     0x8C6C
#define __NVOGLDEBUG_NVTRACE_MASK13                     0x8C6D
#define __NVOGLDEBUG_NVTRACE_MASK14                     0x8C6E
#define __NVOGLDEBUG_NVTRACE_MASK15                     0x8C6F


// Used to set the value of a debug variable.  Returns the old value.
// XXX deprecated, do not use
int GLAPIENTRY __NvOglDebugSeti (unsigned int pname, int param);
typedef int (GLAPIENTRY * PFN__NVOGLDEBUGSETIPROC) (unsigned int pname, int param);

// Used to get the value of a debug variable.
// XXX deprecated, do not use
int GLAPIENTRY __NvOglDebugGeti (unsigned int pname);
typedef int (GLAPIENTRY * PFN__NVOGLDEBUGGETIPROC) (unsigned int pname);

// Used to set the value of a debug variable.  Returns the old value.
// XXX deprecated, do not use
void* GLAPIENTRY __NvOglDebugSetv (unsigned int pname, void* param);
typedef void* (GLAPIENTRY * PFN__NVOGLDEBUGSETVPROC) (unsigned int pname, void* param);

// Used to get the value of a debug variable.
// XXX deprecated, do not use
void* GLAPIENTRY __NvOglDebugGetv (unsigned int pname);
typedef void* (GLAPIENTRY * PFN__NVOGLDEBUGGETVPROC) (unsigned int pname);

// keep this in sync with the ogl driver!!!
#define OGL_PERF_STRAT_CLAW_ENABLED                  0x00000001
#define OGL_PERF_STRAT_CLAW_IGNORE_TRASH             0x00000040
#define OGL_PERF_STRAT_CLAW_FORCE_EXECUTE_BLOCK      0x00000200
#define OGL_PERF_STRAT_CLAW_FORCE_EXECUTE_PIKACHOU   0x00000400
#define OGL_PERF_STRAT_CLAW_PIXEL_ACCURATE           0x00008000

#if defined(QT3DS_MACINTOSH_OSX)

// Used to emulate the debug control API via gldSetInteger.  Values of 5000 or greater
// are reserved for internal driver use.
enum GLDVendorIntegerParameter
{
    GLD_NVIDIA_DEBUG_CONTROL = 5000,
};

#endif

#endif
