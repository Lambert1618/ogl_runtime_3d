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

#include "SystemPrefix.h"

//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSVector3.h"
#include "Qt3DSMatrix.h"
#include "Qt3DSDataLogger.h"
#include <math.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
extern const float RUNTIME_EPSILON = 0.0001f;
const float RUNTIME_SQUARE_EPSILON = 0.000001f;

//==============================================================================
/**
 *	Simple constructor - creates a NULL vector.
 */
RuntimeVector3::RuntimeVector3()
    : m_X(0.0f)
    , m_Y(0.0f)
    , m_Z(0.0f)
{
}

//==============================================================================
/**
 *	Construction from three array value.
 *	@param inVector		the source array
 */
RuntimeVector3::RuntimeVector3(const float inVector[3])
    : m_X(inVector[0])
    , m_Y(inVector[1])
    , m_Z(inVector[2])
{
}

//==============================================================================
/**
 *	Construction from three coordinates.
 *	@param inX		the m_X coordinate
 *	@param inY		the m_Y coordinate
 *	@param inZ		the m_Z coordinate
 */
RuntimeVector3::RuntimeVector3(const float inX, const float inY, const float inZ)
    : m_X(inX)
    , m_Y(inY)
    , m_Z(inZ)
{
}

//==============================================================================
/**
 *	Copy constructor
 *	@param inVector		the source vector
 */
RuntimeVector3::RuntimeVector3(const RuntimeVector3 &inVector)
    : m_X(inVector.m_X)
    , m_Y(inVector.m_Y)
    , m_Z(inVector.m_Z)
{
}

//==============================================================================
/**
 *	Assignment of coordinates.
 *	@param inX		the m_X coordinate
 *	@param inY		the m_Y coordinate
 *	@param inZ		the m_Z coordinate
 *	@return a reference to this modified vector
 */
RuntimeVector3 &RuntimeVector3::Set(const float inX, const float inY, const float inZ)
{
    m_X = inX;
    m_Y = inY;
    m_Z = inZ;
    return *this;
}

//==============================================================================
/**
 *	Assignment of coordinates using another vector.
 *	@param inVector		the vector to be copied.
 *	@return a reference to this modified vector
 */
RuntimeVector3 &RuntimeVector3::Set(const RuntimeVector3 &inVector)
{
    m_X = inVector.m_X;
    m_Y = inVector.m_Y;
    m_Z = inVector.m_Z;
    return *this;
}

//==============================================================================
/**
 *	Compare this vector with another. Exact match is not needed but instead
 *	an error of EPSILON is allowed.
 *	@param inVector	the vector we are compared with
 *	@return	true if the vectors are close to identical
 *	@see	EPSILON
 */
bool RuntimeVector3::operator==(const RuntimeVector3 &inVector) const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);

    return (::fabsf(m_X - inVector.m_X) < RUNTIME_EPSILON) && (::fabsf(m_Y - inVector.m_Y) < RUNTIME_EPSILON)
        && (::fabsf(m_Z - inVector.m_Z) < RUNTIME_EPSILON);
}

//==============================================================================
/**
 *	Compare this vector with another. Exact match is not needed but instead
 *	an error of EPSILON is allowed.
 *	@param inVector		the vector we are compared with
 *	@return	true if the vectors are not even close to identical
 *	@see	EPSILON
 */
bool RuntimeVector3::operator!=(const RuntimeVector3 &inVector) const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);

    return (::fabsf(m_X - inVector.m_X) > RUNTIME_EPSILON) || (::fabsf(m_Y - inVector.m_Y) > RUNTIME_EPSILON)
        || (::fabsf(m_Z - inVector.m_Z) > RUNTIME_EPSILON);
}

//==============================================================================
/**
 *	Add this vector with another but do not modify this vector.
 *	@param inVector		the second vector being added
 *	@return a new vector
 */
RuntimeVector3 RuntimeVector3::operator+(const RuntimeVector3 &inVector) const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);

    return RuntimeVector3(m_X + inVector.m_X, m_Y + inVector.m_Y, m_Z + inVector.m_Z);
}

//==============================================================================
/**
 *	Add two vectors but do not modify this vector.
 *	@param inVector		vector being subtracted
 *	@return a new vector
 */
RuntimeVector3 RuntimeVector3::operator-(const RuntimeVector3 &inVector) const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    return RuntimeVector3(m_X - inVector.m_X, m_Y - inVector.m_Y, m_Z - inVector.m_Z);
}

//==============================================================================
/**
 *	Get a scaled copy of this vector.
 *	@param inFactor		the factor that scales each coordinate
 *	@return a new scaled vector
 */
RuntimeVector3 RuntimeVector3::operator*(float inFactor) const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    return RuntimeVector3(m_X * inFactor, m_Y * inFactor, m_Z * inFactor);
}

//==============================================================================
/**
 *	Invert the vector.
 *	@return	the inverted copy of this vector
 */
RuntimeVector3 RuntimeVector3::operator-() const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    return RuntimeVector3(-m_X, -m_Y, -m_Z);
}

//==============================================================================
/**
 *	Simple assignment.
 *	@param inVector		the new vector
 *	@return	a reference to this modified vector
 */
RuntimeVector3 &RuntimeVector3::operator=(const RuntimeVector3 &inVector)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    if (&inVector != this) {
        m_X = inVector.m_X;
        m_Y = inVector.m_Y;
        m_Z = inVector.m_Z;
    }
    return *this;
}

//==============================================================================
/**
 *	Increment this vector.
 *	@param inVector		has the coordinates by which this vector will be incremented
 *	@return	a reference to this modified vector
 */
RuntimeVector3 &RuntimeVector3::operator+=(const RuntimeVector3 &inVector)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    m_X += inVector.m_X;
    m_Y += inVector.m_Y;
    m_Z += inVector.m_Z;
    return *this;
}

//==============================================================================
/**
 *	Decrement this vector.
 *	@param inVector		has the coordinates by which this vector will be decremented
 *	@return	a reference to this modified vector
 */
RuntimeVector3 &RuntimeVector3::operator-=(const RuntimeVector3 &inVector)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    m_X -= inVector.m_X;
    m_Y -= inVector.m_Y;
    m_Z -= inVector.m_Z;
    return *this;
}

//==============================================================================
/**
 *	Scale this vector by a factor.
 *	@param inFactor		the scale factor
 *	@return	a reference to this modified vector
 */
RuntimeVector3 &RuntimeVector3::operator*=(float inFactor)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    m_X *= inFactor;
    m_Y *= inFactor;
    m_Z *= inFactor;
    return *this;
}

//==============================================================================
/**
 *	Calculates the squared distance between this vector and another.
 *	This is often used in sorting situations where the real distance isn't needed.
 *	@param inVector		vector to which the distance is being calculated
 *	@return	the squared distance between vectors
 */
float RuntimeVector3::DistanceSquared(const RuntimeVector3 &inVector) const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    return (m_X - inVector.m_X) * (m_X - inVector.m_X) + (m_Y - inVector.m_Y) * (m_Y - inVector.m_Y)
        + (m_Z - inVector.m_Z) * (m_Z - inVector.m_Z);
}

//==============================================================================
/**
 *	Calculates the distance between this vector and another.
 *	@param inVector		vector to which the distance is being calculated
 *	@return	the distance between vectors
 */
float RuntimeVector3::Distance(const RuntimeVector3 &inVector) const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    return ::sqrtf((m_X - inVector.m_X) * (m_X - inVector.m_X)
                   + (m_Y - inVector.m_Y) * (m_Y - inVector.m_Y)
                   + (m_Z - inVector.m_Z) * (m_Z - inVector.m_Z));
}

//==============================================================================
/**
 *	Calculates the squared length (squared magnitude) of the current vector.
 *	@return	the squared length of this vector
 */
float RuntimeVector3::LengthSquared() const
{
    return m_X * m_X + m_Y * m_Y + m_Z * m_Z;
}

//==============================================================================
/**
 *	Calculates the length (magnitude) of the current vector.
 *	@return	the length of this vector
 */
float RuntimeVector3::Length() const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    return ::sqrtf(m_X * m_X + m_Y * m_Y + m_Z * m_Z);
}

//==============================================================================
/**
 *	Calculates the dot product of this vector and another.
 *	@param inVector		vector measuring with
 *	@return	the dot product
 *	@see	operator*
 */
float RuntimeVector3::DotProduct(const RuntimeVector3 &inVector) const
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    return m_X * inVector.m_X + m_Y * inVector.m_Y + m_Z * inVector.m_Z;
}

//==============================================================================
/**
 *	Calculates the cross product of this vector and another and modifies this vector
 *	@param inVector		other vector
 *	@return this cross product vector
 *	@see	operator%
 *	@see	operator%=
 */
RuntimeVector3 &RuntimeVector3::CrossProduct(const RuntimeVector3 &inVector)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    float theX = m_Y * inVector.m_Z - m_Z * inVector.m_Y;
    float theY = m_Z * inVector.m_X - m_X * inVector.m_Z;
    float theZ = m_X * inVector.m_Y - m_Y * inVector.m_X;

    m_X = theX;
    m_Y = theY;
    m_Z = theZ;

    return *this;
}

//==============================================================================
/**
 *	Normalizes the current vector making it a unit vector.
 *	The normalized vector is defined to be: V_norm = V / Magnitude(V).
  *	@return	this normalized vector
*/
RuntimeVector3 &RuntimeVector3::Normalize()
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    float theLengthSquared = LengthSquared();
    if ((theLengthSquared > RUNTIME_SQUARE_EPSILON) && ::fabsf(theLengthSquared - 1.0f) > RUNTIME_SQUARE_EPSILON) {
        float theInvLength = 1.0f / ::sqrtf(theLengthSquared);
        m_X *= theInvLength;
        m_Y *= theInvLength;
        m_Z *= theInvLength;
    }
    return *this;
}

//==============================================================================
/**
 *	Modifies the current vector to contain elements that are the minimum between
 *	this vector and another.
 *	@param inVector		vector that whose elements are tested against the
 *						current vector to build the minimum.
 *	@return	this minimized vector
*/
RuntimeVector3 &RuntimeVector3::Minimize(const RuntimeVector3 &inVector)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    m_X = Q3DStudio_min<float>(m_X, inVector.m_X);
    m_Y = Q3DStudio_min<float>(m_Y, inVector.m_Y);
    m_Z = Q3DStudio_min<float>(m_Z, inVector.m_Z);
    return *this;
}

//==============================================================================
/**
 *	Modifies the current vector to contain elements that are the maximum between
 *	this vector and another.
 *	@param inVector		vector that whose elements are tested against the
 *						current vector to build the maximum.
 *	@return	this maximized vector
*/
RuntimeVector3 &RuntimeVector3::Maximize(const RuntimeVector3 &inVector)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    m_X = Q3DStudio_max<float>(m_X, inVector.m_X);
    m_Y = Q3DStudio_max<float>(m_Y, inVector.m_Y);
    m_Z = Q3DStudio_max<float>(m_Z, inVector.m_Z);
    return *this;
}

//==============================================================================
/**
 *	Modifies the current vector by performing a linear interpolation
 *	between the current vector and the given vector.
 *
 *	If inFactor is 0.0 then this vector remains unchanged.  If inFactor is
 *	1.0 then this vector becomes inDestVector. If inFactor is between 0.0-1.0
 *	then this vector becomes a vector between this old vector and inDestVector
 *	proportionally interpolated based in inFactor.  If inFactor is less than 0
 *	or more than 1 then this vector is extrapolated.
 *
 *	@param inDestVector		Second vector for the interpolation
 *	@param inFactor			Weight for the interpolation
 *	@return	this interpolated vector
 */
RuntimeVector3 &RuntimeVector3::InterpolateLinear(const RuntimeVector3 &inDestVector, float inFactor)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    m_X += inFactor * (inDestVector.m_X - m_X);
    m_Y += inFactor * (inDestVector.m_Y - m_Y);
    m_Z += inFactor * (inDestVector.m_Z - m_Z);
    return *this;
}

//==============================================================================
/**
 *	Transforms this vector by the given matrix.
 *
 *	The matrix is row column of the form Matrix[row][column]:
@code
        | m0	m1	m2	w |
        | m4	m5	m6	w |
        | m8	m9	m10	w |
        | tX	tY	tZ	w |
@endcode
 *	@param inMatrix		transform matrix
 *	@return	this transformed vector
 */
RuntimeVector3 &RuntimeVector3::Transform(const RuntimeMatrix &inMatrix)
{
    PerfLogMathEvent1(DATALOGGER_VECTOR);
    float theX = m_X;
    float theY = m_Y;
    float theZ = m_Z;

    m_X = theX * inMatrix.Get(0, 0) + theY * inMatrix.Get(1, 0) + theZ * inMatrix.Get(2, 0);
    m_Y = theX * inMatrix.Get(0, 1) + theY * inMatrix.Get(1, 1) + theZ * inMatrix.Get(2, 1);
    m_Z = theX * inMatrix.Get(0, 2) + theY * inMatrix.Get(1, 2) + theZ * inMatrix.Get(2, 2);

    m_X += inMatrix.Get(3, 0);
    m_Y += inMatrix.Get(3, 1);
    m_Z += inMatrix.Get(3, 2);

    return *this;
}

} // namespace Q3DStudio
