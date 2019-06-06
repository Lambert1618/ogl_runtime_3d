/****************************************************************************
**
** Copyright (C) 2001-2004 NovodeX AG.
** Copyright (C) 2004-2008 AGEIA Technologies, Inc.
** Copyright (C) 2008-2013 NVIDIA Corporation.
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

#include "foundation/Qt3DSFPU.h"
#include <fenv.h>

#if !(defined(__CYGWIN__) || defined(PX_ANDROID))
QT3DS_COMPILE_TIME_ASSERT(8 * sizeof(qt3ds::QT3DSU32) >= sizeof(fenv_t));
#endif

qt3ds::foundation::FPUGuard::FPUGuard()
{
#if defined(__CYGWIN__)
#pragma message "FPUGuard::FPUGuard() is not implemented"
#elif defined(PX_ANDROID)
// not supported unless ARM_HARD_FLOAT is enabled.
#else
    fegetenv(reinterpret_cast<fenv_t *>(mControlWords));
    fesetenv(FE_DFL_ENV);
#endif
}

qt3ds::foundation::FPUGuard::~FPUGuard()
{
#if defined(__CYGWIN__)
#pragma message "FPUGuard::~FPUGuard() is not implemented"
#elif defined(PX_ANDROID)
// not supported unless ARM_HARD_FLOAT is enabled.
#else
    fesetenv(reinterpret_cast<fenv_t *>(mControlWords));
#endif
}
