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

#include "foundation/Qt3DSMathUtils.h"
#include "foundation/Qt3DSUtilities.h"
#include "foundation/Qt3DSMat33.h"

using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace qt3ds::intrinsics;

QT3DSQuat qt3ds::foundation::computeQuatFromNormal(const QT3DSVec3 &n)
{
    // parallel or anti-parallel
    if (n.x > 0.9999f) {
        // parallel
        return QT3DSQuat::createIdentity();
    } else if (n.x < -0.9999f) {
        // anti-parallel
        // contactQuaternion.fromAngleAxisFast(PXD_PI, Vector3(0.0f, 1.0f, 0.0f));
        return QT3DSQuat(0.0f, 1.0f, 0.0f, 0.0f);
    } else {
        QT3DSVec3 rotVec(0.0f, -n.z, n.y);

        // Convert to quat
        NVReal angle = rotVec.magnitude();
        rotVec *= 1.0f / angle;
        //		if(angle > 1.0f) angle = 1.0f;
        angle = selectMin(angle, 1.0f);

        // djs: injudiciously imbecilic use of trig functions, good thing Adam is going to trample
        // this path like a
        // frustrated rhinoceros in mating season

        angle = NVAsin(angle);

        //		if(n.x < 0)
        //			angle = NVPi - angle;
        angle = fsel(n.x, angle, NVPi - angle);

        return QT3DSQuat(angle, rotVec);
    }
}

/**
\brief computes a oriented bounding box around the scaled basis.
\param basis Input = skewed basis, Output = (normalized) orthogonal basis.
\return Bounding box extent.
*/
QT3DSVec3 qt3ds::foundation::optimizeBoundingBox(QT3DSMat33 &basis)
{
    QT3DSVec3 *QT3DS_RESTRICT vec = &basis[0]; // PT: don't copy vectors if not needed...

    // PT: since we store the magnitudes to memory, we can avoid the FCMNV afterwards
    QT3DSVec3 magnitude(vec[0].magnitudeSquared(), vec[1].magnitudeSquared(),
                     vec[2].magnitudeSquared());

// find indices sorted by magnitude
#ifdef QT3DS_X360
    int i = (QT3DSU32 &)(magnitude[1]) > (QT3DSU32 &)(magnitude[0]) ? 1 : 0;
    int j = (QT3DSU32 &)(magnitude[2]) > (QT3DSU32 &)(magnitude[1 - i]) ? 2 : 1 - i;
#else
    int i = magnitude[1] > magnitude[0] ? 1 : 0;
    int j = magnitude[2] > magnitude[1 - i] ? 2 : 1 - i;
#endif
    const int k = 3 - i - j;
#ifdef QT3DS_X360
    if ((QT3DSU32 &)(magnitude[i]) < (QT3DSU32 &)(magnitude[j]))
#else
    if (magnitude[i] < magnitude[j])
#endif
        swap(i, j);

    // ortho-normalize basis

    NVReal invSqrt = NVRecipSqrt(magnitude[i]);
    magnitude[i] *= invSqrt;
    vec[i] *= invSqrt; // normalize the first axis
    NVReal dotij = vec[i].dot(vec[j]);
    NVReal dotik = vec[i].dot(vec[k]);
    magnitude[i] += NVAbs(dotij) + NVAbs(dotik); // elongate the axis by projection of the other two
    vec[j] -= vec[i] * dotij; // orthogonize the two remaining axii relative to vec[i]
    vec[k] -= vec[i] * dotik;

    magnitude[j] = vec[j].normalize();
    NVReal dotjk = vec[j].dot(vec[k]);
    magnitude[j] += NVAbs(dotjk); // elongate the axis by projection of the other one
    vec[k] -= vec[j] * dotjk; // orthogonize vec[k] relative to vec[j]

    magnitude[k] = vec[k].normalize();

    return magnitude;
}

QT3DSQuat qt3ds::foundation::slerp(const NVReal t, const QT3DSQuat &left, const QT3DSQuat &right)
{
    const NVReal quatEpsilon = (NVReal(1.0e-8f));

    NVReal cosine = left.dot(right);
    NVReal sign = NVReal(1);
    if (cosine < 0) {
        cosine = -cosine;
        sign = NVReal(-1);
    }

    NVReal sine = NVReal(1) - cosine * cosine;

    if (sine >= quatEpsilon * quatEpsilon) {
        sine = NVSqrt(sine);
        const NVReal angle = NVAtan2(sine, cosine);
        const NVReal i_sin_angle = NVReal(1) / sine;

        const NVReal leftw = NVSin(angle * (NVReal(1) - t)) * i_sin_angle;
        const NVReal rightw = NVSin(angle * t) * i_sin_angle * sign;

        return left * leftw + right * rightw;
    }

    return left;
}

void qt3ds::foundation::integrateTransform(const NVTransform &curTrans, const QT3DSVec3 &linvel,
                                        const QT3DSVec3 &angvel, NVReal timeStep, NVTransform &result)
{
    result.p = curTrans.p + linvel * timeStep;

    // from void NVsDynamicsContext::integrateAtomPose(NVsRigidBody* atom, Cm::BitMap
    // &shapeChangedMap) const:
    // Integrate the rotation using closed form quaternion integrator
    NVReal w = angvel.magnitudeSquared();

    if (w != 0.0f) {
        w = NVSqrt(w);
        if (w != 0.0f) {
            const NVReal v = timeStep * w * 0.5f;
            const NVReal q = NVCos(v);
            const NVReal s = NVSin(v) / w;

            const QT3DSVec3 pqr = angvel * s;
            const QT3DSQuat quatVel(pqr.x, pqr.y, pqr.z, 0);
            QT3DSQuat out; // need to have temporary, otherwise we may overwrite input if &curTrans ==
                        // &result.
            out = quatVel * curTrans.q;
            out.x += curTrans.q.x * q;
            out.y += curTrans.q.y * q;
            out.z += curTrans.q.z * q;
            out.w += curTrans.q.w * q;
            result.q = out;
            return;
        }
    }
    // orientation stays the same - convert from quat to matrix:
    result.q = curTrans.q;
}
