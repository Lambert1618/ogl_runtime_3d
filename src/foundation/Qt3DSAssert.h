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
#ifndef QT3DS_ASSERT_H
#define QT3DS_ASSERT_H
#include "foundation/Qt3DSPreprocessor.h"

// Force the user to define the Qt3DSAssert function
namespace qt3ds {
void Qt3DSAssert(const char *exp, const char *file, int line, bool *ignore);
}

#ifdef __CUDACC__
#define QT3DS_ASSERT(exp) ((void)0)
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp) ((void)0)
#else // __CUDACC__
#ifndef NDEBUG
#define QT3DS_ASSERT(exp)                                                                          \
    {                                                                                              \
        static bool ignore = false;                                                                \
        (void)((!!(exp)) || (qt3ds::Qt3DSAssert(#exp, __FILE__, __LINE__, &ignore), false));       \
    }
#else // NDEBUG
#define QT3DS_ASSERT(exp) ((void)0)
#endif // NDEBUG
#define QT3DS_ALWAYS_ASSERT_MESSAGE(exp)                                                           \
    {                                                                                              \
        static bool ignore = false;                                                                \
        (void)((qt3ds::Qt3DSAssert(exp, __FILE__, __LINE__, &ignore), false));                     \
    }
#endif // __CUDACC__

#define QT3DS_ALWAYS_ASSERT() QT3DS_ASSERT(0)

#endif
