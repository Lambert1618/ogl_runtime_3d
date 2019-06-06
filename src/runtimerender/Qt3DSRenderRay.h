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
#ifndef QT3DS_RENDER_RAY_H
#define QT3DS_RENDER_RAY_H

#include "Qt3DSRender.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSMat44.h"
#include "foundation/Qt3DSBounds3.h"

namespace qt3ds {
namespace render {

    struct SBasisPlanes
    {
        enum Enum {
            XY = 0,
            YZ,
            XZ,
        };
    };

    struct SRayIntersectionResult
    {
        QT3DSF32 m_RayLengthSquared; // Length of the ray in world coordinates for the hit.
        QT3DSVec2 m_RelXY; // UV coords for further mouse picking against a offscreen-rendered object.
        SRayIntersectionResult()
            : m_RayLengthSquared(0)
            , m_RelXY(0, 0)
        {
        }
        SRayIntersectionResult(QT3DSF32 rl, QT3DSVec2 relxy)
            : m_RayLengthSquared(rl)
            , m_RelXY(relxy)
        {
        }
    };

    struct SRay
    {
        QT3DSVec3 m_Origin;
        QT3DSVec3 m_Direction;
        SRay()
            : m_Origin(0, 0, 0)
            , m_Direction(0, 0, 0)
        {
        }
        SRay(const QT3DSVec3 &inOrigin, const QT3DSVec3 &inDirection)
            : m_Origin(inOrigin)
            , m_Direction(inDirection)
        {
        }
        // If we are parallel, then no intersection of course.
        Option<QT3DSVec3> Intersect(const NVPlane &inPlane) const;

        Option<SRayIntersectionResult> IntersectWithAABB(const QT3DSMat44 &inGlobalTransform,
                                                         const NVBounds3 &inBounds,
                                                         bool inForceIntersect = false) const;

        Option<QT3DSVec2> GetRelative(const QT3DSMat44 &inGlobalTransform, const NVBounds3 &inBounds,
                                   SBasisPlanes::Enum inPlane) const;

        Option<QT3DSVec2> GetRelativeXY(const QT3DSMat44 &inGlobalTransform,
                                     const NVBounds3 &inBounds) const
        {
            return GetRelative(inGlobalTransform, inBounds, SBasisPlanes::XY);
        }
    };
}
}
#endif
