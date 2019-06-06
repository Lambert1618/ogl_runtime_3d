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
#include "foundation/Qt3DSFPU.h"
#include "float.h"

#ifdef QT3DS_X64
#define _MCW_ALL _MCW_DN | _MCW_EM | _MCW_RC
#else
#define _MCW_ALL _MCW_DN | _MCW_EM | _MCW_IC | _MCW_RC | _MCW_PC
#endif

qt3ds::foundation::FPUGuard::FPUGuard()
{
// default plus FTZ and DAZ
#if defined(QT3DS_WINDOWS) && defined(QT3DS_VC)
#ifdef QT3DS_X64
    _controlfp_s(mControlWords, _CW_DEFAULT | _DN_FLUSH, _MCW_ALL);
#else
    __control87_2(_CW_DEFAULT | _DN_FLUSH, _MCW_ALL, mControlWords, mControlWords + 1);
#endif
#endif
}

qt3ds::foundation::FPUGuard::~FPUGuard()
{
#if defined(QT3DS_WINDOWS) && defined(QT3DS_VC)
#ifdef QT3DS_X64
    _controlfp_s(mControlWords, *mControlWords, _MCW_ALL);
#else
    __control87_2(mControlWords[0], _MCW_ALL, mControlWords, 0);
    __control87_2(mControlWords[1], _MCW_ALL, 0, mControlWords + 1);
#endif
#endif
}
