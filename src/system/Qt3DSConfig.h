/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

//==============================================================================
//	This file should only contain macros that configure the build environment
//	and build options.
//==============================================================================
#include "Qt3DSMemorySettings.h"

/// Number of chars to evaluate when generating a hash from a string.
/// Verify that hash strings are unique after changing this value.
#ifndef HASH_LIMIT
#define HASH_LIMIT 100
#endif // HASH_LIMIT

/// Define this macro use use sync primitives(mutex/critical sections) when accessing
/// shared data structures
#ifndef Q3DStudio_USE_SYNC_PRIMITIVES
#define Q3DStudio_USE_SYNC_PRIMITIVES 1
#endif // Q3DStudio_USE_SYNC_PRIMITIVES
