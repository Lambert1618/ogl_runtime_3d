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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSAssert.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Easy iterations over arrays.
 *	@param array_type		element type
 *	@param array_element	element variable name getting each entry
 *	@param array_container	element array we are traversing
 */
#define FOR_ARRAY(array_type, array_element, array_container)                                      \
    for (array_type *array_element = (array_container).Begin();                                    \
         array_element != (array_container).End(); ++array_element)

#define FOR_ARRAY_REVERSE(array_type, array_element, array_container)                              \
    for (array_type *array_element = (array_container).End() - 1;                                  \
         array_element != (array_container).Begin() - 1; --array_element)

//==============================================================================
/**
 *	Return the smallest of two numbers/objects.
 *	@param inA		one value to compare
 *	@param inB		another value to compare
 *	@return	a reference, not a copy, to the smallest of the two values
 */
template <class T>
inline const T Q3DStudio_abs(const T &inVal)
{
    return inVal >= 0 ? inVal : -inVal;
}

//==============================================================================
/**
 *	Return the smallest of two numbers/objects.
 *	@param inA		one value to compare
 *	@param inB		another value to compare
 *	@return	a reference, not a copy, to the smallest of the two values
 */
template <typename T>
inline const T &Q3DStudio_min(const T &inA, const T &inB)
{
    return inA < inB ? inA : inB;
}

//==============================================================================
/**
 *	Return the largest of two numbers/objects.
 *	@param inA		one value to compare
 *	@param inB		another value to compare
 *	@return	a reference, not a copy, to the largest of the two values
 */
template <class T>
inline const T &Q3DStudio_max(const T &inA, const T &inB)
{
    return inA > inB ? inA : inB;
}

//==============================================================================
/**
 *	Return the smallest of three values.
 *	@param inA		one value to compare
 *	@param inB		another value to compare
 *	@param inC		a third value to compare
 *	@return	a reference, not a copy, to the smallest of the three values
 */
template <class T>
inline const T &Q3DStudio_min3(const T &inA, const T &inB, const T &inC)
{
    return inA < inB ? Q3DStudio_min(inA, inC) : Q3DStudio_min(inB, inC);
}

//==============================================================================
/**
 *	Return the largest of three values.
 *	@param inA		one value to compare
 *	@param inB		another value to compare
 *	@param inC		a third value to compare
 *	@return	a reference, not a copy, to the largest of the three values
 */
template <class T>
inline const T &Q3DStudio_max3(const T &inA, const T &inB, const T &inC)
{
    return inA > inB ? Q3DStudio_max(inA, inC) : Q3DStudio_max(inB, inC);
}

//==============================================================================
/**
 *	Make sure a value is in-bounds between two other values.
 *	@param inVal	 the value to clamp
 *	@param inMin	the lowest inclusive value
 *	@param inMax	the highest inclusive value
 *	@return	a reference to inVal if the value is in bounds,
                        inMin if lower than the minimum value, or
                        inMax if higher than maximum value.
 */
template <class T>
inline const T &Q3DStudio_clamp(const T &inVal, const T &inMin, const T &inMax)
{
    if (inVal <= inMin)
        return inMin;
    else if (inVal >= inMax)
        return inMax;

    return inVal;
}

//==============================================================================
/**
 *	Return the identified lower and higher values.
 *	@param inA		one value to compare
 *	@param inB		another value to compare
 *	@param outMin	is assigned to the lower of the two compared values
 *	@param outMax	is assigned to the higher of the two compared values
 */
template <class T>
inline void Q3DStudio_minmax(const T &inA, const T &inB, T &outMin, T &outMax)
{
    if (inA < inB) {
        outMin = inA;
        outMax = inB;
    } else {
        outMin = inB;
        outMax = inA;
    }
}

//==============================================================================
/**
 *	Identify needed parameters that are not currently referenced.
 */
#define Q3DStudio_UNREFERENCED_PARAMETER(P)                                                        \
    {                                                                                              \
        (void)P;                                                                                   \
    }

//==============================================================================
/**
 *	Assert
 */
#if defined(_DEBUG) || defined(_PROFILE)
#define Q3DStudio_ASSERT(inExpression)                                                             \
    {                                                                                              \
        if (!(inExpression))                                                                       \
            Q3DStudio::CAssert::GetFunction()(#inExpression, __FILE__, __LINE__, __FUNCTION__);    \
    }
#else
#define Q3DStudio_ASSERT(inExpression) ((void)0)
#endif // _DEBUG or _PROFILE

} // namespace Q3DStudio
