/****************************************************************************
**
** Copyright (C) 2009-2011 NVIDIA Corporation.
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

#ifndef INCLUDED_QT3DS_MATH_CPP_VEC_H
#define INCLUDED_QT3DS_MATH_CPP_VEC_H

#include "misc.h"

struct _NvVec3
{
    union {
        struct
        {
            float x, y, z;
        };
        float v[3];
    };
};

struct NvVec3 : public _NvVec3
{
    NvVec3() { zero(); }

    NvVec3(float x, float y, float z)
    {
        v[0] = x;
        v[1] = y;
        v[2] = z;
    }

    NvVec3(NvVec3 const &that)
    {
        v[0] = that.v[0];
        v[1] = that.v[1];
        v[2] = that.v[2];
    }

    inline void zero()
    {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
    }

    inline float &operator[](int i)
    {
        assert((i < 3) && (i >= 0));
        return v[i];
    }

    inline float const &operator[](int i) const
    {
        assert((i < 3) && (i >= 0));
        return v[i];
    }

    NvVec3 &operator+=(NvVec3 const &that)
    {
        v[0] += that.v[0];
        v[1] += that.v[1];
        v[2] += that.v[2];
        return *this;
    }

    NvVec3 &operator-=(NvVec3 const &that)
    {
        v[0] -= that.v[0];
        v[1] -= that.v[1];
        v[2] -= that.v[2];
        return *this;
    }

    NvVec3 &operator*=(float s)
    {
        v[0] *= s;
        v[1] *= s;
        v[2] *= s;
        return *this;
    }

    NvVec3 &operator/=(float s)
    {
        v[0] /= s;
        v[1] /= s;
        v[2] /= s;
        return *this;
    }
};

inline float dot(NvVec3 const &a, NvVec3 const &b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline NvVec3 operator+(NvVec3 const &a, NvVec3 const &b)
{

    return NvVec3(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2]);
}

inline NvVec3 operator-(NvVec3 const &a, NvVec3 const &b)
{

    return NvVec3(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2]);
}

inline NvVec3 operator-(NvVec3 const &a)
{

    return NvVec3(-a.v[0], -a.v[1], -a.v[2]);
}

inline NvVec3 operator*(NvVec3 const &a, float s)
{

    return NvVec3(a.v[0] * s, a.v[1] * s, a.v[2] * s);
}

inline NvVec3 operator*(float s, NvVec3 const &a)
{

    return NvVec3(s * a.v[0], s * a.v[1], s * a.v[2]);
}

inline NvVec3 operator/(NvVec3 const &a, float s)
{

    return NvVec3(a.v[0] / s, a.v[1] / s, a.v[2] / s);
}

inline float magnitude(NvVec3 const &v)
{
    return sqrtf(dot(v, v));
}

inline NvVec3 normalize(NvVec3 const &v)
{
    float l = magnitude(v);
    return NvVec3(v[0] / l, v[1] / l, v[2] / l);
}

inline NvVec3 cross(NvVec3 const &a, NvVec3 const &b)
{

    return NvVec3(a.v[1] * b.v[2] - a.v[2] * b.v[1], a.v[2] * b.v[0] - a.v[0] * b.v[2],
                  a.v[0] * b.v[1] - a.v[1] * b.v[0]);
}

inline bool operator==(NvVec3 const &a, NvVec3 const &b)
{
    return (a.v[0] == b.v[0] && a.v[1] == b.v[1] && a.v[2] == b.v[2]);
}

inline bool operator!=(NvVec3 const &a, NvVec3 const &b)
{
    return (a.v[0] != b.v[0] || a.v[1] != b.v[1] || a.v[2] != b.v[2]);
}

struct NvVec2f
{
    union {
        struct
        {
            float x, y;
            float v[2];
        };
    };

    NvVec2f() { zero(); }

    NvVec2f(float x, float y)
    {
        v[0] = x;
        v[1] = y;
    }

    NvVec2f(NvVec2f const &that)
    {
        v[0] = that.v[0];
        v[1] = that.v[1];
    }

    inline void zero()
    {
        v[0] = 0.0f;
        v[1] = 0.0f;
    }

    inline float &operator[](int i)
    {
        assert((i < 2) && (i >= 0));
        return v[i];
    }

    inline float const &operator[](int i) const
    {
        assert((i < 2) && (i >= 0));
        return v[i];
    }

    NvVec2f &operator+=(NvVec2f const &that)
    {
        v[0] += that.v[0];
        v[1] += that.v[1];
        return *this;
    }

    NvVec2f &operator-=(NvVec2f const &that)
    {
        v[0] -= that.v[0];
        v[1] -= that.v[1];
        return *this;
    }

    NvVec2f &operator*=(float s)
    {
        v[0] *= s;
        v[1] *= s;
        return *this;
    }

    NvVec2f &operator/=(float s)
    {
        v[0] /= s;
        v[1] /= s;
        return *this;
    }
};

inline float dot(NvVec2f const &a, NvVec2f const &b)
{

    return a[0] * b[0] + a[1] * b[1];
}

inline NvVec2f operator+(NvVec2f const &a, NvVec2f const &b)
{

    return NvVec2f(a.v[0] + b.v[0], a.v[1] + b.v[1]);
}

inline NvVec2f operator-(NvVec2f const &a, NvVec2f const &b)
{

    return NvVec2f(a.v[0] - b.v[0], a.v[1] - b.v[1]);
}

inline NvVec2f operator-(NvVec2f const &a)
{

    return NvVec2f(-a.v[0], -a.v[1]);
}

inline NvVec2f operator*(NvVec2f const &a, float s)
{

    return NvVec2f(a.v[0] * s, a.v[1] * s);
}

inline NvVec2f operator*(float s, NvVec2f const &a)
{

    return NvVec2f(s * a.v[0], s * a.v[1]);
}

inline NvVec2f operator/(NvVec2f const &a, float s)
{

    return NvVec2f(a.v[0] / s, a.v[1] / s);
}

inline float magnitude(NvVec2f const &v)
{
    return sqrtf(dot(v, v));
}

inline NvVec2f normalize(NvVec2f const &v)
{
    float l = dot(v, v);
    return NvVec2f(v[0] / l, v[1] / l);
}

inline float cross(NvVec2f const &a, NvVec2f const &b)
{

    return a.v[0] * b.v[1] - b.v[0] * a.v[1];
}

#endif
