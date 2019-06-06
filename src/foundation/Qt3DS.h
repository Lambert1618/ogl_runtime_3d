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
#ifndef QT3DS_H
#define QT3DS_H
#include "EABase/eabase.h"
#include "foundation/Qt3DSPreprocessor.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/Qt3DSAssert.h"

namespace eastl {
}

namespace qt3ds {
class NVEmpty
{
};
class QT3DSMat33;
class QT3DSMat44;
class QT3DSVec2;
class QT3DSVec3;
class QT3DSVec4;
class NVTransform;

#if !defined QT3DS_DONT_PRAGMA_WARNINGS && defined EA_COMPILER_MSVC
#pragma warning(disable : 4512) // assignment operator not generated
#endif

#define QT3DS_SIGN_BITMASK 0x80000000
}

#endif
