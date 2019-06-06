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

#ifndef QT3DS_FOUNDATION_PSMATHUTILS_H
#define QT3DS_FOUNDATION_PSMATHUTILS_H

#include "foundation/Qt3DSTransform.h"
#include "foundation/Qt3DSMat33.h"
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSIntrinsics.h"
#include <stdlib.h>

// General guideline is: if it's an abstract math function, it belongs here.
// If it's a math function where the inputs have specific semantics (e.g.
// separateSwingTwist) it doesn't.

namespace qt3ds {
namespace foundation {
    using namespace intrinsics;
    /**
    \brief sign returns the sign of its argument. The sign of zero is undefined.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 sign(const QT3DSF32 a) {
        return intrinsics::sign(a);
    }

    /**
    \brief sign returns the sign of its argument. The sign of zero is undefined.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 sign(const QT3DSF64 a) { return (a >= 0.0) ? 1.0 : -1.0; }

    /**
    \brief sign returns the sign of its argument. The sign of zero is undefined.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSI32 sign(const QT3DSI32 a) { return (a >= 0) ? 1 : -1; }

    /**
    \brief Returns true if the two numbers are within eps of each other.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool equals(const QT3DSF32 a, const QT3DSF32 b, const QT3DSF32 eps)
    {
        return (NVAbs(a - b) < eps);
    }

    /**
    \brief Returns true if the two numbers are within eps of each other.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool equals(const QT3DSF64 a, const QT3DSF64 b, const QT3DSF64 eps)
    {
        return (NVAbs(a - b) < eps);
    }

    /**
    \brief The floor function returns a floating-point value representing the largest integer that
    is less than or equal to x.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 floor(const QT3DSF32 a) { return floatFloor(a); }

    /**
    \brief The floor function returns a floating-point value representing the largest integer that
    is less than or equal to x.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 floor(const QT3DSF64 a) { return ::floor(a); }

    /**
    \brief The ceil function returns a single value representing the smallest integer that is
    greater than or equal to x.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 ceil(const QT3DSF32 a) { return ::ceilf(a); }

    /**
    \brief The ceil function returns a double value representing the smallest integer that is
    greater than or equal to x.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 ceil(const QT3DSF64 a) { return ::ceil(a); }

    /**
    \brief mod returns the floating-point remainder of x / y.

    If the value of y is 0.0, mod returns a quiet NaN.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 mod(const QT3DSF32 x, const QT3DSF32 y)
    {
        return (QT3DSF32)::fmod(x, y);
    }

    /**
    \brief mod returns the floating-point remainder of x / y.

    If the value of y is 0.0, mod returns a quiet NaN.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 mod(const QT3DSF64 x, const QT3DSF64 y)
    {
        return ::fmod(x, y);
    }

    /**
    \brief Square.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 sqr(const QT3DSF32 a) { return a * a; }

    /**
    \brief Square.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 sqr(const QT3DSF64 a) { return a * a; }

    /**
    \brief Calculates x raised to the power of y.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 pow(const QT3DSF32 x, const QT3DSF32 y)
    {
        return ::powf(x, y);
    }

    /**
    \brief Calculates x raised to the power of y.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 pow(const QT3DSF64 x, const QT3DSF64 y) { return ::pow(x, y); }

    /**
    \brief Calculates e^n
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 exp(const QT3DSF32 a) { return QT3DSF32(::exp(a)); }
    /**

    \brief Calculates e^n
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 exp(const QT3DSF64 a) { return ::exp(a); }

    /**
    \brief Calculates logarithms.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 logE(const QT3DSF32 a) { return QT3DSF32(::log(a)); }

    /**
    \brief Calculates logarithms.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 logE(const QT3DSF64 a) { return ::log(a); }

    /**
    \brief Calculates logarithms.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 log2(const QT3DSF32 a)
    {
        return QT3DSF32(::log(a)) / 0.693147180559945309417f;
    }

    /**
    \brief Calculates logarithms.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 log2(const QT3DSF64 a)
    {
        return ::log(a) / 0.693147180559945309417;
    }

    /**
    \brief Calculates logarithms.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 log10(const QT3DSF32 a) { return (QT3DSF32)::log10(a); }

    /**
    \brief Calculates logarithms.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 log10(const QT3DSF64 a) { return ::log10(a); }

    /**
    \brief Converts degrees to radians.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 degToRad(const QT3DSF32 a)
    {
        return (QT3DSF32)0.01745329251994329547 * a;
    }

    /**
    \brief Converts degrees to radians.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 degToRad(const QT3DSF64 a)
    {
        return (QT3DSF64)0.01745329251994329547 * a;
    }

    /**
    \brief Converts radians to degrees.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 radToDeg(const QT3DSF32 a)
    {
        return (QT3DSF32)57.29577951308232286465 * a;
    }

    /**
    \brief Converts radians to degrees.
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF64 radToDeg(const QT3DSF64 a)
    {
        return (QT3DSF64)57.29577951308232286465 * a;
    }

    //! \brief compute sine and cosine at the same time. There is a 'fsincos' on PC that we probably want to use here
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void sincos(const QT3DSF32 radians, QT3DSF32 &sin, QT3DSF32 &cos)
    {
        /* something like:
        _asm fld  Local
        _asm fsincos
        _asm fstp LocalCos
        _asm fstp LocalSin
        */
        sin = NVSin(radians);
        cos = NVCos(radians);
    }

    /**
    \brief uniform random number in [a,b]
    */
    QT3DS_FORCE_INLINE QT3DSI32 rand(const QT3DSI32 a, const QT3DSI32 b)
    {
        return a + (QT3DSI32)(::rand() % (b - a + 1));
    }

    /**
    \brief uniform random number in [a,b]
    */
    QT3DS_FORCE_INLINE QT3DSF32 rand(const QT3DSF32 a, const QT3DSF32 b)
    {
        return a + (b - a) * ::rand() / RAND_MAX;
    }

    //! \brief return angle between two vectors in radians
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSF32 angle(const QT3DSVec3 &v0, const QT3DSVec3 &v1)
    {
        const QT3DSF32 cos = v0.dot(v1); // |v0|*|v1|*Cos(Angle)
        const QT3DSF32 sin = (v0.cross(v1)).magnitude(); // |v0|*|v1|*Sin(Angle)
        return NVAtan2(sin, cos);
    }

    //! If possible use instead fsel on the dot product /*fsel(d.dot(p),onething,anotherthing);*/
    //! Compares orientations (more readable, user-friendly function)
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE bool sameDirection(const QT3DSVec3 &d, const QT3DSVec3 &p)
    {
        return d.dot(p) >= 0.0f;
    }

    //! Checks 2 values have different signs
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE IntBool differentSign(NVReal f0, NVReal f1)
    {
        union {
            QT3DSU32 u;
            NVReal f;
        } u1, u2;
        u1.f = f0;
        u2.f = f1;
        return (u1.u ^ u2.u) & QT3DS_SIGN_BITMASK;
    }

    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSMat33 star(const QT3DSVec3 &v)
    {
        return QT3DSMat33(QT3DSVec3(0, v.z, -v.y), QT3DSVec3(-v.z, 0, v.x), QT3DSVec3(v.y, -v.x, 0));
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSVec3 log(const QT3DSQuat &q)
    {
        const NVReal s = q.getImaginaryPart().magnitude();
        if (s < 1e-12)
            return QT3DSVec3(0.0f);
        // force the half-angle to have magnitude <= pi/2
        NVReal halfAngle = q.w < 0 ? NVAtan2(-s, -q.w) : NVAtan2(s, q.w);
        QT3DS_ASSERT(halfAngle >= -NVPi / 2 && halfAngle <= NVPi / 2);

        return q.getImaginaryPart().getNormalized() * 2 * halfAngle;
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSQuat exp(const QT3DSVec3 &v)
    {
        const NVReal m = v.magnitudeSquared();
        return m < 1e-24 ? QT3DSQuat::createIdentity() : QT3DSQuat(NVSqrt(m), v * NVRecipSqrt(m));
    }

    // quat to rotate v0 t0 v1
    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSQuat rotationArc(const QT3DSVec3 &v0, const QT3DSVec3 &v1)
    {
        const QT3DSVec3 cross = v0.cross(v1);
        const NVReal d = v0.dot(v1);
        if (d <= -0.99999f)
            return (NVAbs(v0.x) < 0.1f ? QT3DSQuat(0.0f, v0.z, -v0.y, 0.0f)
                                       : QT3DSQuat(v0.y, -v0.x, 0.0, 0.0))
                .getNormalized();

        const NVReal s = NVSqrt((1 + d) * 2), r = 1 / s;

        return QT3DSQuat(cross.x * r, cross.y * r, cross.z * r, s * 0.5f).getNormalized();
    }

    //! Computes the maximum delta to another transform
    QT3DS_CUDA_CALLABLE QT3DS_INLINE NVReal maxComponentDelta(const NVTransform &t0,
                                                        const NVTransform &t1)
    {
        NVReal delta = NVAbs(t0.p.x - t1.p.x);
        delta = NVMax(delta, NVAbs(t0.p.y - t1.p.y));
        delta = NVMax(delta, NVAbs(t0.p.z - t1.p.z));
        delta = NVMax(delta, NVAbs(t0.q.x - t1.q.x));
        delta = NVMax(delta, NVAbs(t0.q.y - t1.q.y));
        delta = NVMax(delta, NVAbs(t0.q.z - t1.q.z));
        delta = NVMax(delta, NVAbs(t0.q.w - t1.q.w));

        return delta;
    }

    /**
    \brief returns largest axis
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSU32 largestAxis(const QT3DSVec3 &v)
    {
        QT3DSU32 m = v.y > v.x ? 1 : 0;
        return v.z > v[m] ? 2 : m;
    }

    /**
    \brief returns axis with smallest absolute value
    */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSU32 closestAxis(const QT3DSVec3 &v)
    {
        QT3DSU32 m = NVAbs(v.y) > NVAbs(v.x) ? 1 : 0;
        return NVAbs(v.z) > NVAbs(v[m]) ? 2 : m;
    }

    QT3DS_CUDA_CALLABLE QT3DS_INLINE QT3DSU32 closestAxis(const QT3DSVec3 &v, QT3DSU32 &j, QT3DSU32 &k)
    {
        // find largest 2D plane projection
        const QT3DSF32 absNV = NVAbs(v.x);
        const QT3DSF32 absNy = NVAbs(v.y);
        const QT3DSF32 absNz = NVAbs(v.z);

        QT3DSU32 m = 0; // x biggest axis
        j = 1;
        k = 2;
        if (absNy > absNV && absNy > absNz) {
            // y biggest
            j = 2;
            k = 0;
            m = 1;
        } else if (absNz > absNV) {
            // z biggest
            j = 0;
            k = 1;
            m = 2;
        }
        return m;
    }

    /*!
        Extend an edge along its length by a factor
        */
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE void makeFatEdge(QT3DSVec3 &p0, QT3DSVec3 &p1, NVReal fatCoeff)
    {
        QT3DSVec3 delta = p1 - p0;

        const NVReal m = delta.magnitude();
        if (m > 0.0f) {
            delta *= fatCoeff / m;
            p0 -= delta;
            p1 += delta;
        }
    }

    //! Compute point as combination of barycentric coordinates
    QT3DS_CUDA_CALLABLE QT3DS_FORCE_INLINE QT3DSVec3 computeBarycentricPoint(const QT3DSVec3 &p0,
                                                                    const QT3DSVec3 &p1,
                                                                    const QT3DSVec3 &p2, NVReal u,
                                                                    NVReal v)
    {
        // This seems to confuse the compiler...
        // return (1.0f - u - v)*p0 + u*p1 + v*p2;
        const QT3DSF32 w = 1.0f - u - v;
        return QT3DSVec3(w * p0.x + u * p1.x + v * p2.x, w * p0.y + u * p1.y + v * p2.y,
                      w * p0.z + u * p1.z + v * p2.z);
    }

    // generates a pair of quaternions (swing, twist) such that in = swing * twist, with
    // swing.x = 0
    // twist.y = twist.z = 0, and twist is a unit quat
    QT3DS_FORCE_INLINE void separateSwingTwist(const QT3DSQuat &q, QT3DSQuat &swing, QT3DSQuat &twist)
    {
        twist = q.x != 0.0f ? QT3DSQuat(q.x, 0, 0, q.w).getNormalized() : QT3DSQuat::createIdentity();
        swing = q * twist.getConjugate();
    }

    // generate two tangent vectors to a given normal
    QT3DS_FORCE_INLINE void normalToTangents(const QT3DSVec3 &normal, QT3DSVec3 &tangent0, QT3DSVec3 &tangent1)
    {
        tangent0 = NVAbs(normal.x) < 0.70710678f ? QT3DSVec3(0, -normal.z, normal.y)
                                                 : QT3DSVec3(-normal.y, normal.x, 0);
        tangent0.normalize();
        tangent1 = normal.cross(tangent0);
    }

    // todo: what is this function doing?
    QT3DS_FOUNDATION_API QT3DSQuat computeQuatFromNormal(const QT3DSVec3 &n);

    /**
    \brief computes a oriented bounding box around the scaled basis.
    \param basis Input = skewed basis, Output = (normalized) orthogonal basis.
    \return Bounding box extent.
    */
    QT3DS_FOUNDATION_API QT3DSVec3 optimizeBoundingBox(QT3DSMat33 &basis);

    QT3DS_FOUNDATION_API QT3DSQuat slerp(const NVReal t, const QT3DSQuat &left, const QT3DSQuat &right);

    QT3DS_INLINE QT3DSVec3 ellipseClamp(const QT3DSVec3 &point, const QT3DSVec3 &radii)
    {
        // This function need to be implemented in the header file because
        // it is included in a spu shader program.

        // finds the closest point on the ellipse to a given point

        // (p.y, p.z) is the input point
        // (e.y, e.z) are the radii of the ellipse

        // lagrange multiplier method with Newton/Halley hybrid root-finder.
        // see http://www.geometrictools.com/Documentation/DistancePointToEllipse2.pdf
        // for proof of Newton step robustness and initial estimate.
        // Halley converges much faster but sometimes overshoots - when that happens we take
        // a newton step instead

        // converges in 1-2 iterations where D&C works well, and it's good with 4 iterations
        // with any ellipse that isn't completely crazy

        const QT3DSU32 MAX_ITERATIONS = 20;
        const NVReal convergenceThreshold = 1e-4f;

        // iteration requires first quadrant but we recover generality later

        QT3DSVec3 q(0, NVAbs(point.y), NVAbs(point.z));
        const NVReal tinyEps =
            (NVReal)(1e-6f); // very close to minor axis is numerically problematic but trivial
        if (radii.y >= radii.z) {
            if (q.z < tinyEps)
                return QT3DSVec3(0, point.y > 0 ? radii.y : -radii.y, 0);
        } else {
            if (q.y < tinyEps)
                return QT3DSVec3(0, 0, point.z > 0 ? radii.z : -radii.z);
        }

        QT3DSVec3 denom, e2 = radii.multiply(radii), eq = radii.multiply(q);

        // we can use any initial guess which is > maximum(-e.y^2,-e.z^2) and for which f(t) is > 0.
        // this guess works well near the axes, but is weak along the diagonals.

        NVReal t = NVMax(eq.y - e2.y, eq.z - e2.z);

        for (QT3DSU32 i = 0; i < MAX_ITERATIONS; i++) {
            denom = QT3DSVec3(0, 1 / (t + e2.y), 1 / (t + e2.z));
            QT3DSVec3 denom2 = eq.multiply(denom);

            QT3DSVec3 fv = denom2.multiply(denom2);
            NVReal f = fv.y + fv.z - 1;

            // although in exact arithmetic we are guaranteed f>0, we can get here
            // on the first iteration via catastrophic cancellation if the point is
            // very close to the origin. In that case we just behave as if f=0

            if (f < convergenceThreshold)
                return e2.multiply(point).multiply(denom);

            NVReal df = fv.dot(denom) * -2.0f;
            t = t - f / df;
        }

        // we didn't converge, so clamp what we have
        QT3DSVec3 r = e2.multiply(point).multiply(denom);
        return r * NVRecipSqrt(sqr(r.y / radii.y) + sqr(r.z / radii.z));
    }

    QT3DS_INLINE NVReal tanHalf(NVReal sin, NVReal cos) { return sin / (1 + cos); }

    QT3DS_INLINE QT3DSQuat quatFromTanQVector(const QT3DSVec3 &v)
    {
        NVReal v2 = v.dot(v);
        if (v2 < 1e-12f)
            return QT3DSQuat::createIdentity();
        NVReal d = 1 / (1 + v2);
        return QT3DSQuat(v.x * 2, v.y * 2, v.z * 2, 1 - v2) * d;
    }

    QT3DS_FORCE_INLINE QT3DSVec3 cross100(const QT3DSVec3 &b) { return QT3DSVec3(0.0f, -b.z, b.y); }
    QT3DS_FORCE_INLINE QT3DSVec3 cross010(const QT3DSVec3 &b) { return QT3DSVec3(b.z, 0.0f, -b.x); }
    QT3DS_FORCE_INLINE QT3DSVec3 cross001(const QT3DSVec3 &b) { return QT3DSVec3(-b.y, b.x, 0.0f); }

    QT3DS_INLINE void decomposeVector(QT3DSVec3 &normalCompo, QT3DSVec3 &tangentCompo,
                                   const QT3DSVec3 &outwardDir, const QT3DSVec3 &outwardNormal)
    {
        normalCompo = outwardNormal * (outwardDir.dot(outwardNormal));
        tangentCompo = outwardDir - normalCompo;
    }

    //! \brief Return (i+1)%3
    // Avoid variable shift for XBox:
    // QT3DS_INLINE QT3DSU32 NV::getNextIndex3(QT3DSU32 i)			{	return (1<<i) & 3;
    // }
    QT3DS_INLINE QT3DSU32 getNextIndex3(QT3DSU32 i) { return (i + 1 + (i >> 1)) & 3; }

    QT3DS_INLINE QT3DSMat33 rotFrom2Vectors(const QT3DSVec3 &from, const QT3DSVec3 &to)
    {
        // See bottom of
        // http://www.euclideanspace.com/maths/algebra/matrix/orthogonal/rotation/index.htm

        // Early exit if to = from
        if ((from - to).magnitudeSquared() < 1e-4f)
            return QT3DSMat33::createIdentity();

        // Early exit if to = -from
        if ((from + to).magnitudeSquared() < 1e-4f)
            return QT3DSMat33::createDiagonal(QT3DSVec3(1.0f, -1.0f, -1.0f));

        QT3DSVec3 n = from.cross(to);

        NVReal C = from.dot(to), S = NVSqrt(1 - C * C), CC = 1 - C;

        NVReal xx = n.x * n.x, yy = n.y * n.y, zz = n.z * n.z, xy = n.x * n.y, yz = n.y * n.z,
               xz = n.x * n.z;

        QT3DSMat33 R;

        R(0, 0) = 1 + CC * (xx - 1);
        R(0, 1) = -n.z * S + CC * xy;
        R(0, 2) = n.y * S + CC * xz;

        R(1, 0) = n.z * S + CC * xy;
        R(1, 1) = 1 + CC * (yy - 1);
        R(1, 2) = -n.x * S + CC * yz;

        R(2, 0) = -n.y * S + CC * xz;
        R(2, 1) = n.x * S + CC * yz;
        R(2, 2) = 1 + CC * (zz - 1);

        return R;
    }

    QT3DS_FOUNDATION_API void integrateTransform(const NVTransform &curTrans, const QT3DSVec3 &linvel,
                                              const QT3DSVec3 &angvel, NVReal timeStep,
                                              NVTransform &result);

} // namespace foundation
} // namespace qt3ds

#endif
