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

#include "foundation/Qt3DSSystem.h"
#include "foundation/Qt3DSPreprocessor.h"
#include "EASTL/string.h"

using namespace qt3ds;
using namespace qt3ds::foundation;

#if defined(QT3DS_ANDROID)
const char *qt3ds::foundation::System::g_OS = "android";
const char *qt3ds::foundation::System::g_DLLExtension = ".so";
#elif defined(QT3DS_APPLE)
const char *qt3ds::foundation::System::g_OS = "osx";
const char *qt3ds::foundation::System::g_DLLExtension = ".dylib";
#elif defined(QT3DS_LINUX)
const char *qt3ds::foundation::System::g_OS = "linux";
const char *qt3ds::foundation::System::g_DLLExtension = ".so";
#elif defined(QT3DS_QNX)
const char *qt3ds::foundation::System::g_OS = "qnx";
const char *qt3ds::foundation::System::g_DLLExtension = ".so";
#elif defined(QT3DS_WINDOWS)
const char *qt3ds::foundation::System::g_OS = "windows";
const char *qt3ds::foundation::System::g_DLLExtension = ".dll";
#else
#error "Unknown Operating System"
#endif

#if defined(QT3DS_X86)
const char *qt3ds::foundation::System::g_Processor = "x86";
const char *qt3ds::foundation::System::g_BitWidth = "32";
const char *qt3ds::foundation::System::g_FloatingPointModel = "";
#elif defined(QT3DS_X64)
const char *qt3ds::foundation::System::g_Processor = "x64";
const char *qt3ds::foundation::System::g_BitWidth = "64";
const char *qt3ds::foundation::System::g_FloatingPointModel = "";
#elif defined(QT3DS_ARM)
#if defined(__aarch64__) || defined(__ARM64__)
const char *qt3ds::foundation::System::g_Processor = "arm";
const char *qt3ds::foundation::System::g_BitWidth = "64";
const char *qt3ds::foundation::System::g_FloatingPointModel = "softfp";
#else
const char *qt3ds::foundation::System::g_Processor = "arm";
const char *qt3ds::foundation::System::g_BitWidth = "32";
#if defined(QT3DS_ARM_HARDFP)
const char *qt3ds::foundation::System::g_FloatingPointModel = "hardfp";
#elif defined(QT3DS_ARM_SOFTFP)
const char *qt3ds::foundation::System::g_FloatingPointModel = "softfp";
#else
#error "Unknown floating point model!"
#endif
#endif
#else
#error "Unknown Platform"
#endif

#if defined(QT3DS_ARM)
#if defined(QT3DS_GRAPHICS_API_GLES2)
const char *qt3ds::foundation::System::g_GPUType = "gles2";
#elif defined(QT3DS_GRAPHICS_API_GL)
const char *qt3ds::foundation::System::g_GPUType = "gl";
#elif defined(QT3DS_GRAPHICS_API_GLES3)
const char *qt3ds::foundation::System::g_GPUType = "gles3";
#else
#error                                                                                             \
    "Must define a GPU type for arm platforms (QT3DS_GRAPHICS_API_GLES2, QT3DS_GRAPHICS_API_GLES3, QT3DS_GRAPHICS_API_GL)"
#endif
#elif defined(QT3DS_X86)
const char *qt3ds::foundation::System::g_GPUType = "";
#elif defined(QT3DS_X64)
const char *qt3ds::foundation::System::g_GPUType = "";
#else
#error "Must define a processor type (QT3DS_ARM or QT3DS_X86)"
#endif

namespace {
static const unsigned SYSTEM_STR_SIZE = 100;
void SystemAppendString(eastl::string &str, const char *delim, const char *string)
{
    if (string && *string) {
        str.append(delim);
        str.append(string);
    }
}
}
const char *System::getPlatformStr()
{
    static char text[SYSTEM_STR_SIZE];
    {
        eastl::string str(g_Processor);
        SystemAppendString(str, "_", g_BitWidth);
        SystemAppendString(str, "_", g_FloatingPointModel);
        SystemAppendString(str, "_", g_OS);
        strcpy(text, str.c_str());
    }
    return text;
}

const char *System::getPlatformGLStr()
{
    static char text[SYSTEM_STR_SIZE];
    {
        eastl::string str(g_Processor);
        SystemAppendString(str, "_", g_BitWidth);
        SystemAppendString(str, "_", g_FloatingPointModel);
        SystemAppendString(str, "_", g_GPUType);
        SystemAppendString(str, "_", g_OS);
        strcpy(text, str.c_str());
    }
    return text;
}