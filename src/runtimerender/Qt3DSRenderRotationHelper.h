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
#ifndef QT3DS_RENDER_ROTATION_HELPER_H
#define QT3DS_RENDER_ROTATION_HELPER_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderNode.h"
#include "EASTL/utility.h"

namespace qt3ds {
namespace render {
    /**
     *	Unfortunately we still use an XYZ-Euler rotation system.  This means that identical
     *rotations
     *	can be represented in various ways.  We need to ensure that the value that we write in the
     *	inspector palette are reasonable, however, and to do this we need a least-distance function
     *	from two different xyz tuples.
     */

    struct SRotationHelper
    {

        // Attempt to go for negative values intead of large positive ones
        // Goal is to keep the fabs of the angle low.
        static QT3DSF32 ToMinimalAngle(QT3DSF32 value)
        {
            QT3DSF32 epsilon = (QT3DSF32)M_PI + .001f;
            while (fabs(value) > epsilon) {
                QT3DSF32 tpi = (QT3DSF32)(2.0f * M_PI);
                if (value > 0.0f)
                    value -= tpi;
                else
                    value += tpi;
            }
            return value;
        }

        /**
         *	Convert an angle to a canonical form.  Return this canonical form.
         *
         *	The canonical form is defined as:
         *	1. XYZ all positive.
         *	2. XYZ all less than 360.
         *
         *	To do this we rely on two identities, the first is that given an angle, adding
         *	(or subtracting) pi from all three components does not change the angle.
         *
         *	The second is the obvious one that adding or subtracting 2*pi from any single
         *	component does not change the angle.
         *
         *	Note that this function works in radian space.
         */
        static QT3DSVec3 ToCanonicalFormStaticAxis(const QT3DSVec3 &inSrcAngle, QT3DSU32 inRotOrder)
        {
            // step 1 - reduce all components to less than 2*pi but greater than 0
            QT3DSVec3 retval(inSrcAngle);
            retval.x = ToMinimalAngle(retval.x);
            retval.y = ToMinimalAngle(retval.y);
            retval.z = ToMinimalAngle(retval.z);

            // step 2 - if any two components are equal to or greater than pi
            // then subtract pi from all three, then run two pi reduce again.

            QT3DSU32 greaterThanPiSum = 0;
            QT3DSF32 pi = (QT3DSF32)M_PI;
            for (QT3DSU32 idx = 0; idx < 3; ++idx)
                if (fabs(retval[idx]) >= pi)
                    greaterThanPiSum++;

            if (greaterThanPiSum > 1) {
                // For this identity to work, the middle axis angle needs to be subtracted from
                // 180 instead of added to 180 because the previous axis *reversed* it.
                QT3DSU32 theMiddleAxis = 0;

                switch (inRotOrder) {
                case EulOrdXYZs:
                    theMiddleAxis = 1;
                    break;
                case EulOrdXZYs:
                    theMiddleAxis = 2;
                    break;
                case EulOrdYXZs:
                    theMiddleAxis = 0;
                    break;
                case EulOrdYZXs:
                    theMiddleAxis = 2;
                    break;
                case EulOrdZYXs:
                    theMiddleAxis = 1;
                    break;
                case EulOrdZXYs:
                    theMiddleAxis = 0;
                    break;
                case EulOrdXYZr:
                    theMiddleAxis = 1;
                    break;
                case EulOrdXZYr:
                    theMiddleAxis = 2;
                    break;
                case EulOrdYXZr:
                    theMiddleAxis = 0;
                    break;
                case EulOrdYZXr:
                    theMiddleAxis = 2;
                    break;
                case EulOrdZYXr:
                    theMiddleAxis = 1;
                    break;
                case EulOrdZXYr:
                    theMiddleAxis = 0;
                    break;
                default:
                    QT3DS_ASSERT(false);
                    return inSrcAngle;
                }
                for (QT3DSU32 idx = 0; idx < 3; ++idx) {
                    if (idx == theMiddleAxis)
                        retval[idx] = pi - retval[idx];
                    else
                        retval[idx] = retval[idx] > 0.0f ? retval[idx] - pi : retval[idx] + pi;
                }
            }
            return retval;
        }

        static QT3DSVec3 ToMinimalAngleDiff(const QT3DSVec3 inDiff)
        {
            return QT3DSVec3(ToMinimalAngle(inDiff.x), ToMinimalAngle(inDiff.y),
                          ToMinimalAngle(inDiff.z));
        }

        /**
         *	Given an old angle and a new angle, return an angle has the same rotational value
         *	as the new angle *but* is as close to the old angle as possible.
         *	Works in radian space.  This function doesn't currently work for euler angles or
         *	with Euler angles with repeating axis.
         */
        static QT3DSVec3 ToNearestAngle(const QT3DSVec3 &inOldAngle, const QT3DSVec3 &inNewAngle,
                                     QT3DSU32 inRotOrder)
        {
            switch (inRotOrder) {
            case EulOrdXYZs:
            case EulOrdXZYs:
            case EulOrdYXZs:
            case EulOrdYZXs:
            case EulOrdZYXs:
            case EulOrdZXYs:
            case EulOrdXYZr:
            case EulOrdXZYr:
            case EulOrdYXZr:
            case EulOrdYZXr:
            case EulOrdZYXr:
            case EulOrdZXYr: {
                QT3DSVec3 oldA = ToCanonicalFormStaticAxis(inOldAngle, inRotOrder);
                QT3DSVec3 newA = ToCanonicalFormStaticAxis(inNewAngle, inRotOrder);
                QT3DSVec3 diff = newA - oldA;
                return inOldAngle + ToMinimalAngleDiff(diff);
            } break;
            default:
                return inNewAngle;
            }
        }
    };
}
}
#endif
