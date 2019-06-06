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

#ifdef WIN32
#pragma warning(disable : 4201)
#endif

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class RuntimeMatrix;

//==============================================================================
/**
 *	3D vector with automatic notification on change.
 *
 *	This implementation of a 3D vector along with all the sibling and helper
 *	classes in the geometry section, try to strike a balance between clean code,
 *	ease-of-use and speed.  Fields are public and most methods are designed to
 *	give the impression of a very intelligent and capable structure. Heavier
 *	code such as intersections is encouraged to be added externally instead
 *	of here.
 *
 *	The vector is a member of the geometry classes next to CMatrix and CRotation
 *	@see	CMatrix
 *	@see	CRotation
 */
class RuntimeVector3
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    union {
        float m[3];
        struct
        {
            float m_X;
            float m_Y;
            float m_Z;
        };
    };

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    RuntimeVector3();
    RuntimeVector3(const float inVector[3]);
    RuntimeVector3(const float inX, const float inY, const float inZ);
    RuntimeVector3(const RuntimeVector3 &inVector);

public: // Initialization
    RuntimeVector3 &Set(const float inX, const float inY, const float inZ);
    RuntimeVector3 &Set(const RuntimeVector3 &inVector);

public: // Functions
    float DistanceSquared(const RuntimeVector3 &inVector) const;
    float Distance(const RuntimeVector3 &inVector) const;
    float LengthSquared() const;
    float Length() const;
    float DotProduct(const RuntimeVector3 &inVector) const;
    RuntimeVector3 &CrossProduct(const RuntimeVector3 &inVector);
    RuntimeVector3 &Normalize();
    RuntimeVector3 &Minimize(const RuntimeVector3 &inVector);
    RuntimeVector3 &Maximize(const RuntimeVector3 &inVector);
    RuntimeVector3 &InterpolateLinear(const RuntimeVector3 &inDestVector, float inFactor);
    RuntimeVector3 &Transform(const RuntimeMatrix &inMatrix);

public: // Operators
    bool operator==(const RuntimeVector3 &inVector) const;
    bool operator!=(const RuntimeVector3 &inVector) const;
    RuntimeVector3 operator+(const RuntimeVector3 &inVector) const;
    RuntimeVector3 operator-(const RuntimeVector3 &inVector) const;
    RuntimeVector3 operator*(float inFactor) const;
    RuntimeVector3 &operator=(const RuntimeVector3 &inVector);
    RuntimeVector3 &operator+=(const RuntimeVector3 &inVector);
    RuntimeVector3 &operator-=(const RuntimeVector3 &inVector);
    RuntimeVector3 &operator*=(float inFactor);
    RuntimeVector3 operator-() const;
};

} // namespace Q3DStudio
