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

#ifndef QT3DS_FOUNDATION_QT3DS_PREPROCESSOR_H
#define QT3DS_FOUNDATION_QT3DS_PREPROCESSOR_H

#include <stddef.h>
/** \addtogroup foundation
  @{
*/

/**
List of preprocessor defines used to configure the SDK
- NDEBUG/_DEBUG: enable asserts (exactly one needs to be defined)
- QT3DS_CHECKED: enable run time checks, mostly unused or equiv. to _DEBUG
- QT3DS_SUPPORT_VISUAL_DEBUGGER: ...
- AG_PERFMON: ...
*/

/**
Compiler define
*/
#ifdef _MSC_VER
#define QT3DS_VC
#if _MSC_VER >= 1600
#define QT3DS_VC10
#elif _MSC_VER >= 1500
#define QT3DS_VC9
#elif _MSC_VER >= 1400
#define QT3DS_VC8
#elif _MSC_VER >= 1300
#define QT3DS_VC7
#else
#define QT3DS_VC6
#endif
#elif __GNUC__ || __SNC__
#define QT3DS_GNUC
#elif defined(__MWERKS__)
#define QT3DS_CW
#elif defined(__ghs__)
#define QT3DS_GHS
#else
#error "Unknown compiler"
#endif

/**
Platform define
*/
#ifdef QT3DS_VC
#ifdef _M_IX86
#define QT3DS_X86
#define QT3DS_WINDOWS
#elif defined(_M_X64)
#define QT3DS_X64
#define QT3DS_WINDOWS
#elif defined(_M_PPC)
#define QT3DS_PPC
#define QT3DS_X360
#define QT3DS_VMX
#elif defined(_M_ARM)
#define QT3DS_ARM
#define QT3DS_WIN8ARM
#define QT3DS_ARM_NEON
#else
#error "Unknown platform"
#endif
#elif defined QT3DS_GNUC
#ifdef __CELLOS_LV2__
#define QT3DS_PS3
#define QT3DS_VMX
#elif defined(__arm__)
#define QT3DS_ARM
#if defined(__SNC__)
#define QT3DS_PSP2
#endif
#if defined(__ARM_PCS_VFP)
#define QT3DS_ARM_HARDFP
#else
#define QT3DS_ARM_SOFTFP
#endif
#elif defined(__aarch64__)
#define QT3DS_ARM
#elif defined(__i386__)
#define QT3DS_X86
#define QT3DS_VMX
#elif defined(__x86_64__)
#define QT3DS_X64
#elif defined(__ppc__)
#define QT3DS_PPC
#elif defined(__ppc64__)
#define QT3DS_PPC
#define QT3DS_PPC64
//#   elif defined(__aarch64__)
//#       define QT3DS_ARM_64
#else
#error "Unknown platform"
#endif
#if defined(ANDROID)
#define QT3DS_ANDROID
#elif defined(__linux__)
#define QT3DS_LINUX
#elif defined(__APPLE__)
#define QT3DS_APPLE
#if defined(__arm__)
#define QT3DS_APPLE_IOS
#endif
#elif defined(__CYGWIN__)
#define QT3DS_CYGWIN
#define QT3DS_LINUX
#elif defined(__QNX__)
#define QT3DS_QNX
#elif defined(_WIN32)
#define QT3DS_WINDOWS
#else
#error "Unkown OS"
#endif
#elif defined QT3DS_CW
#if defined(__PPCGEKKO__)
#if defined(RVL)
#define QT3DS_WII
#else
#define QT3DS_GC
#endif
#else
#error "Unknown platform"
#endif
#elif defined QT3DS_GHS
#define QT3DS_LINUX // INTEGRITY deviations flagged with __INTEGRITY
#if defined(__arm__) || defined(__aarch64__) || defined(__ARM64__)
#define QT3DS_ARM
#endif
#endif

/**
DLL export macros
*/
#ifndef QT3DS_C_EXPORT
#define QT3DS_C_EXPORT extern "C"
#endif

/**
Define API function declaration

QT3DS_FOUNDATION_EXPORTS - used by the DLL library (PhysXCommon) to export the API
QT3DS_FOUNDATION_NO_EXPORTS - exists because there are windows configurations where
                                                   the QT3DS_FOUNDATION_API is linked through standard
static linking
no definition - this will allow DLLs and libraries to use the exported API from PhysXCommon

*/
#include <qglobal.h>
#if defined(QT3DS_WINDOWS) && !defined(__CUDACC__)
#if defined QT3DS_FOUNDATION_EXPORTS
#define QT3DS_FOUNDATION_API Q_DECL_EXPORT
#elif defined QT3DS_FOUNDATION_NO_EXPORTS
#define QT3DS_FOUNDATION_API
#else
#define QT3DS_FOUNDATION_API Q_DECL_IMPORT
#endif
#else
#define QT3DS_FOUNDATION_API
#endif

#if defined(QT3DS_AUTOTESTS_ENABLED)
#if defined(QT3DS_BUILDING_LIBRARY)
#define QT3DS_AUTOTEST_EXPORT Q_DECL_EXPORT
#else
#define QT3DS_AUTOTEST_EXPORT Q_DECL_IMPORT
#endif
#else
#define QT3DS_AUTOTEST_EXPORT QT3DS_FOUNDATION_API
#endif

/**
Calling convention
*/
#ifndef QT3DS_CALL_CONV
#if defined QT3DS_WINDOWS
#define QT3DS_CALL_CONV __cdecl
#else
#define QT3DS_CALL_CONV
#endif
#endif

/**
Pack macros - disabled on SPU because they are not supported
*/
#if defined(QT3DS_VC)
#define QT3DS_PUSH_PACK_DEFAULT __pragma(pack(push, 8))
#define QT3DS_POP_PACK __pragma(pack(pop))
#elif defined(QT3DS_GNUC) && !defined(__SPU__)
#define QT3DS_PUSH_PACK_DEFAULT _Pragma("pack(push, 8)")
#define QT3DS_POP_PACK _Pragma("pack(pop)")
#else
#define QT3DS_PUSH_PACK_DEFAULT
#define QT3DS_POP_PACK
#endif

/**
Inline macro
*/
#if defined(QT3DS_WINDOWS) || defined(QT3DS_X360)
#define QT3DS_INLINE inline
#ifdef QT3DS_VC
#pragma inline_depth(255)
#endif
#else
#define QT3DS_INLINE inline
#endif

/**
Force inline macro
*/
#if defined(QT3DS_VC)
#define QT3DS_FORCE_INLINE __forceinline
#elif defined(QT3DS_LINUX)                                                                            \
    || defined(QT3DS_QNX) // Workaround; Fedora Core 3 do not agree with force inline and NVcPool
#define QT3DS_FORCE_INLINE inline
#elif defined(QT3DS_GNUC)
#define QT3DS_FORCE_INLINE inline __attribute__((always_inline))
#else
#define QT3DS_FORCE_INLINE inline
#endif

/**
Noinline macro
*/
#if defined QT3DS_WINDOWS
#define QT3DS_NOINLINE __declspec(noinline)
#elif defined(QT3DS_GNUC)
#define QT3DS_NOINLINE __attribute__((noinline))
#else
#define QT3DS_NOINLINE
#endif

/*! restrict macro */
#if __CUDACC__
#define QT3DS_RESTRICT __restrict__
#elif defined(QT3DS_GNUC) || defined(QT3DS_VC)
#define QT3DS_RESTRICT __restrict
#elif defined(QT3DS_CW) && __STDC_VERSION__ >= 199901L
#define QT3DS_RESTRICT restrict
#else
#define QT3DS_RESTRICT
#endif

#if defined(QT3DS_WINDOWS) || defined(QT3DS_X360)
#define QT3DS_NOALIAS __declspec(noalias)
#else
#define QT3DS_NOALIAS
#endif

/**
Alignment macros

QT3DS_ALIGN_PREFIX and QT3DS_ALIGN_SUFFIX can be used for type alignment instead of aligning individual
variables as follows:
QT3DS_ALIGN_PREFIX(16)
struct A {
...
} QT3DS_ALIGN_SUFFIX(16);
This declaration style is parsed correctly by Visual Assist.

*/
#ifndef QT3DS_ALIGN
#if defined(QT3DS_VC)
#define QT3DS_ALIGN(alignment, decl) __declspec(align(alignment)) decl
#define QT3DS_ALIGN_PREFIX(alignment) __declspec(align(alignment))
#define QT3DS_ALIGN_SUFFIX(alignment)
#elif defined(QT3DS_GNUC)
#define QT3DS_ALIGN(alignment, decl) decl __attribute__((aligned(alignment)))
#define QT3DS_ALIGN_PREFIX(alignment)
#define QT3DS_ALIGN_SUFFIX(alignment) __attribute__((aligned(alignment)))
#elif defined(QT3DS_CW)
#define QT3DS_ALIGN(alignment, decl) decl __attribute__((aligned(alignment)))
#define QT3DS_ALIGN_PREFIX(alignment)
#define QT3DS_ALIGN_SUFFIX(alignment) __attribute__((aligned(alignment)))
#else
#define QT3DS_ALIGN(alignment, decl)
#define QT3DS_ALIGN_PREFIX(alignment)
#define QT3DS_ALIGN_SUFFIX(alignment)
#endif
#endif

/**
Deprecated marco
*/
#if 0 // set to 1 to create warnings for deprecated functions
#define QT3DS_DEPRECATED __declspec(deprecated)
#else
#define QT3DS_DEPRECATED
#endif

// VC6 no '__FUNCTION__' workaround
#if defined QT3DS_VC6 && !defined __FUNCTION__
#define __FUNCTION__ "Undefined"
#endif

/**
General defines
*/

// Assertion type
template <bool b>
struct StaticAssert
{
};
// Specialisation with member function
template <>
struct StaticAssert<true>
{
public:
    static void valid_expression(){}
};

// static assert
#define QT3DS_COMPILE_TIME_ASSERT(exp) typedef char NVCompileTimeAssert_Dummy[(exp) ? 1 : -1]

#ifdef QT3DS_GNUC
#define QT3DS_OFFSET_OF(X, Y) __builtin_offsetof(X, Y)
#else
#define QT3DS_OFFSET_OF(X, Y) offsetof(X, Y)
#endif

// avoid unreferenced parameter warning (why not just disable it?)
// PT: or why not just omit the parameter's name from the declaration????
#define QT3DS_FORCE_PARAMETER_REFERENCE(_P) (void)(_P);
#define QT3DS_UNUSED(_P) QT3DS_FORCE_PARAMETER_REFERENCE(_P)

// check that exactly one of NDEBUG and _DEBUG is defined
#if !(defined NDEBUG ^ defined _DEBUG)
#error Exactly one of NDEBUG and _DEBUG needs to be defined by preprocessor
#endif

// make sure QT3DS_CHECKED is defined in all _DEBUG configurations as well
#if !defined(QT3DS_CHECKED) && _DEBUG
#define QT3DS_CHECKED
#endif

#ifdef __CUDACC__
#define QT3DS_CUDA_CALLABLE __host__ __device__
#else
#define QT3DS_CUDA_CALLABLE
#endif

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_PREPROCESSOR_H
