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

#ifndef QT3DS_FOUNDATION_PSFPU_H
#define QT3DS_FOUNDATION_PSFPU_H

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSUnionCast.h"

// unsigned integer representation of a floating-point value.
#ifdef QT3DS_PS3
QT3DS_FORCE_INLINE unsigned int QT3DS_IR(const float x)
{
    return qt3ds::NVUnionCast<unsigned int, float>(x);
}
#else
#define QT3DS_IR(x) ((QT3DSU32 &)(x))
#endif

// signed integer representation of a floating-point value.
#ifdef QT3DS_PS3
QT3DS_FORCE_INLINE int QT3DS_SIR(const float x)
{
    return qt3ds::NVUnionCast<int, float>(x);
}
#else
#define QT3DS_SIR(x) ((QT3DSI32 &)(x))
#endif

// Floating-point representation of a integer value.
#ifdef QT3DS_PS3
QT3DS_FORCE_INLINE float QT3DS_FR(const unsigned int x)
{
    return qt3ds::NVUnionCast<float, unsigned int>(x);
}
#else
#define QT3DS_FR(x) ((QT3DSF32 &)(x))
#endif

#ifdef QT3DS_PS3
QT3DS_FORCE_INLINE float *QT3DS_FPTR(unsigned int *x)
{
    return qt3ds::NVUnionCast<float *, unsigned int *>(x);
}

QT3DS_FORCE_INLINE float *QT3DS_FPTR(int *x)
{
    return qt3ds::NVUnionCast<float *, int *>(x);
}
#else
#define QT3DS_FPTR(x) ((QT3DSF32 *)(x))
#endif

#define QT3DS_SIGN_BITMASK 0x80000000

namespace qt3ds {
namespace foundation {
class QT3DS_FOUNDATION_API FPUGuard
{
public:
    FPUGuard(); // set fpu control word for PhysX
    ~FPUGuard(); // restore fpu control word
private:
    QT3DSU32 mControlWords[8];
};

} // namespace foundation
} // namespace qt3ds

#endif
