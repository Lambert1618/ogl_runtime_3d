/****************************************************************************
**
** Copyright (C) 2009-2011 NVIDIA Corporation.
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

#ifndef __INCLUDED_QT3DS_DEBUG_H
#define __INCLUDED_QT3DS_DEBUG_H

#define CT_ASSERT(tag, cond) enum { COMPILE_TIME_ASSERT__##tag = 1 / (cond) }

#define dimof(x) (sizeof(x) / sizeof(x[0]))
#include <android/log.h>

#define DBG_DETAILED 0

#if 0

        // the detailed prefix can be customised by setting DBG_DETAILED_PREFIX. See
        // below as a reference.
        // NOTE: fmt is the desired format string and must be in the prefix.
        //#ifndef DBG_DETAILED_PREFIX
        //	#define DBG_DETAILED_PREFIX "%s, %s, line %d: " fmt, __FILE__, __FUNCTION__, __LINE__,
        //#endif
        //#define DEBUG_D_(fmt, args...)
        //#define DEBUG_D(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, (DBG_DETAILED_PREFIX) ## args)

#else

#ifdef STRINGIFY
#pragma push_macro("STRINGIFY")
#undef STRINGIFY
#define STRINGIFYPUSHED_____
#endif
#define STRINGIFY(x) #x

// debug macro, includes file name function name and line number
#define TO(x) typeof(x)
#define DEBUG_D_(file, line, fmt, args...)                                                         \
    __android_log_print(ANDROID_LOG_DEBUG, MODULE, file ", %s, line(" STRINGIFY(line) "): " fmt,   \
                        __FUNCTION__, ##args)
#define DEBUG_D(fmt, args...) DEBUG_D_(__FILE__, __LINE__, fmt, ##args)

#ifdef STRINGIFYPUSHED_____
#undef STRINGIFYPUSHED_____
#pragma pop_macro("STRINGIFY")
#endif

#endif

// basic debug macro
#define DEBUG_(fmt, args...) (__android_log_print(ANDROID_LOG_DEBUG, MODULE, fmt, ##args))

// Debug macro that can be switched to spew a file name,
// function and line number using DEBUG_DETAILED
#if DBG_DETAILED == 1
#define DEBUG(fmt, args...) DEBUG_D(fmt, ##args)
#else
#define DEBUG(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, fmt, ##args)
#endif

#endif
