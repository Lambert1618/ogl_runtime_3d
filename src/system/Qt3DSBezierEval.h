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

#include <vector>
#include <QtCore/qglobal.h>
#include <QtCore/qmath.h>

namespace Q3DStudio {

class CubicPolynomial
{
public:
    CubicPolynomial(double p0, double p1, double p2, double p3);

    std::vector<double> extrema() const;
    std::vector<double> roots() const;

private:
    double m_a;
    double m_b;
    double m_c;
    double m_d;
};

CubicPolynomial::CubicPolynomial(double p0, double p1, double p2, double p3)
    : m_a(p3 - 3.0 * p2 + 3.0 * p1 - p0)
    , m_b(3.0 * p2 - 6.0 * p1 + 3.0 * p0)
    , m_c(3.0 * p1 - 3.0 * p0)
    , m_d(p0)
{}

std::vector<double> CubicPolynomial::extrema() const
{
    std::vector<double> out;

    auto addValidValue = [&out](double value) {
        if (!std::isnan(value) && !std::isinf(value))
            out.push_back(qBound(0.0, value, 1.0));
    };

    // Find the roots of the first derivative of y.
    auto pd2 = (2.0 * m_b) / (3.0 * m_a) / 2.0;
    auto q = m_c / (3.0 * m_a);

    auto radi = std::pow(pd2, 2.0) - q;

    auto x1 = -pd2 + std::sqrt(radi);
    auto x2 = -pd2 - std::sqrt(radi);

    addValidValue(x1);
    addValidValue(x2);

    return out;
}

std::vector<double> CubicPolynomial::roots() const
{
    std::vector<double> out;

    auto addValidValue = [&out](double value) {
        if (!(std::isnan(value) || std::isinf(value)))
            out.push_back(value);
    };

    if (m_a == 0.0) {
        if (m_b == 0.0) { // Linear
            if (m_c != 0.0)
                out.push_back(-m_d / m_c);
        } else { // Quadratic
            const double p = m_c / m_b / 2.0;
            const double q = m_d / m_b;
            const double sqrt = std::sqrt(std::pow(p, 2.0) - q);
            addValidValue(-p + sqrt);
            addValidValue(-p - sqrt);
        }
    } else { // Cubic
        const double p = 3.0 * m_a * m_c - std::pow(m_b, 2.0);
        const double q = 2.0 * std::pow(m_b, 3.0) - 9.0 * m_a * m_b * m_c
                         + 27.0 * std::pow(m_a, 2.0) * m_d;

        auto disc = std::pow(q, 2.0) + 4.0 * std::pow(p, 3.0);

        auto toX = [&](double y) { return (y - m_b) / (3.0 * m_a); };

        if (disc >= 0) { // One real solution.
            double sqrt = 4.0 * std::sqrt(std::pow(q, 2.0) + 4.0 * std::pow(p, 3.0));
            double u = .5 * std::cbrt(-4.0 * q + sqrt);
            double v = .5 * std::cbrt(-4.0 * q - sqrt);

            addValidValue(toX(u + v));
        } else { // Three real solutions.
            double phi = acos(-q / (2 * std::sqrt(-std::pow(p, 3.0))));
            double sqrt = std::sqrt(-p) * 2.0;
            double y1 = sqrt * cos(phi / 3.0);
            double y2 = sqrt * cos((phi / 3.0) + (2.0 * M_PI / 3.0));
            double y3 = sqrt * cos((phi / 3.0) + (4.0 * M_PI / 3.0));

            addValidValue(toX(y1));
            addValidValue(toX(y2));
            addValidValue(toX(y3));
        }
    }
    return out;
}

double evaluateForT(double t, double p0, double p1, double p2, double p3)
{
    assert(t >= 0. && t <= 1.);

    const double it = 1.0 - t;

    return p0 * std::pow(it, 3.0) + p1 * 3.0 * std::pow(it, 2.0) * t
           + p2 * 3.0 * it * std::pow(t, 2.0) + p3 * std::pow(t, 3.0);
}

inline float EvaluateBezierKeyframe(long inTime, long inTime1, float inValue1, long inC1Time,
                                    float inC1Value, long inC2Time, float inC2Value, long inTime2,
                                    float inValue2)
{
    if (inTime == inTime1)
        return inValue1;

    if (inTime == inTime2)
        return inValue2;

    auto polynomial = CubicPolynomial(inTime1 - inTime, inC1Time - inTime, inC2Time - inTime,
                                      inTime2 - inTime);

    for (double t : polynomial.roots()) {
        if (t < 0.0 || t > 1.0)
            continue;

        // the curve handles are confined so that only 1 solution should ever exist for a give time
        return evaluateForT(t, inValue1, inC1Value, inC2Value, inValue2);
    }

    assert(false); // no solution found?!

    return 0.0;
}

} // namespace Qt3DStudio
