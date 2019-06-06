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

namespace Q3DStudio {

//==============================================================================
/**
 *	Generic Bezier parametric curve evaluation at a given parametric value.
 *	@param inP0		control point P0
 *	@param inP1		control point P1
 *	@param inP2		control point P2
 *	@param inP3		control point P3
 *	@param inS		the variable
 *	@return the evaluated value on the bezier curve
 */
inline FLOAT EvaluateBezierCurve(FLOAT inP0, FLOAT inP1, FLOAT inP2, FLOAT inP3, const FLOAT inS)
{
    // Using:
    //		Q(s) = Sum i=0 to 3 ( Pi * Bi,3(s))
    // where:
    //		Pi is a control point and
    //		Bi,3 is a basis function such that:
    //
    //		B0,3(s) = (1 - s)^3
    //		B1,3(s) = (3 * s) * (1 - s)^2
    //		B2,3(s) = (3 * s^2) * (1 - s)
    //		B3,3(s) = s^3

    /*	FLOAT	theSSquared = inS * inS;										//
       t^2
            FLOAT	theSCubed = theSSquared * inS;									//
       t^3

            FLOAT	theSDifference = 1 - inS;										// (1 -
       t)
            FLOAT	theSDifferenceSquared = theSDifference * theSDifference;		// (1 -
       t)^2
            FLOAT	theSDifferenceCubed = theSDifferenceSquared * theSDifference;	// (1 - t)^3

            FLOAT	theFirstTerm = theSDifferenceCubed;								// (1 -
       t)^3
            FLOAT	theSecondTerm = ( 3 * inS ) * theSDifferenceSquared;			// (3 * t) * (1
       - t)^2
            FLOAT	theThirdTerm = ( 3 * theSSquared ) * theSDifference;			// (3 * t^2) *
       (1 - t)
            FLOAT	theFourthTerm = theSCubed;										//
       t^3

            // Q(t) = ( p0 * (1 - t)^3 ) + ( p1 * (3 * t) * (1 - t)^2 ) + ( p2 * (3 * t^2) * (1 - t)
       ) + ( p3 * t^3 )
            return ( inP0 * theFirstTerm ) + ( inP1 * theSecondTerm ) + ( inP2 * theThirdTerm ) + (
       inP3 * theFourthTerm );*/

    FLOAT theFactor = inS * inS;
    inP1 *= 3 * inS;
    inP2 *= 3 * theFactor;
    theFactor *= inS;
    inP3 *= theFactor;

    theFactor = 1 - inS;
    inP2 *= theFactor;
    theFactor *= 1 - inS;
    inP1 *= theFactor;
    theFactor *= 1 - inS;
    inP0 *= theFactor;

    return inP0 + inP1 + inP2 + inP3;
}

//==============================================================================
/**
 *	Inverse Bezier parametric curve evaluation to get parametric value for a given output.
 *	This is equal to finding the root(s) of the Bezier cubic equation.
 *	@param inP0		control point P0
 *	@param inP1		control point P1
 *	@param inP2		control point P2
 *	@param inP3		control point P3
 *	@param inX		the variable
 *	@return the evaluated value
 */
inline FLOAT EvaluateInverseBezierCurve(const FLOAT inP0, const FLOAT inP1, const FLOAT inP2,
                                        const FLOAT inP3, const FLOAT inX)
{
    FLOAT theResult = 0;

    // Using:
    //		Q(s) = Sum i=0 to 3 ( Pi * Bi,3(s))
    // where:
    //		Pi is a control point and
    //		Bi,3 is a basis function such that:
    //
    //		B0,3(s) = (1 - s)^3
    //		B1,3(s) = (3 * s) * (1 - s)^2
    //		B2,3(s) = (3 * s^2) * (1 - s)
    //		B3,3(s) = s^3

    // The Bezier cubic equation:
    // inX = inP0*(1-s)^3 + inP1*(3*s)*(1-s)^2 + inP2*(3*s^2)*(1-s) + inP3*s^3
    //     = s^3*( -inP0 + 3*inP1 - 3*inP2 +inP3 ) + s^2*( 3*inP0 - 6*inP1 + 3*inP2 ) + s*( -3*inP0
    //     + 3*inP1 ) + inP0
    // For cubic eqn of the form: c[0] + c[1]*x + c[2]*x^2 + c[3]*x^3 = 0
    FLOAT theConstants[4];
    theConstants[0] = static_cast<FLOAT>(inP0 - inX);
    theConstants[1] = static_cast<FLOAT>(-3 * inP0 + 3 * inP1);
    theConstants[2] = static_cast<FLOAT>(3 * inP0 - 6 * inP1 + 3 * inP2);
    theConstants[3] = static_cast<FLOAT>(-inP0 + 3 * inP1 - 3 * inP2 + inP3);

    FLOAT theSolution[3] = { 0 };

    if (theConstants[3] == 0) {
        if (theConstants[2] == 0) {
            if (theConstants[1] == 0)
                theResult = 0;
            else
                theResult = -theConstants[0] / theConstants[1]; // linear
        } else {
            // quadratic
            INT32 theNumRoots = CCubicRoots::SolveQuadric(theConstants, theSolution);
            theResult = static_cast<FLOAT>(theSolution[theNumRoots / 2]);
        }
    } else {
        INT32 theNumRoots = CCubicRoots::SolveCubic(theConstants, theSolution);
        theResult = static_cast<FLOAT>(theSolution[theNumRoots / 3]);
    }

    return theResult;
}

inline FLOAT EvaluateBezierKeyframe(FLOAT inTime, FLOAT inTime1, FLOAT inValue1, FLOAT inC1Time,
                                    FLOAT inC1Value, FLOAT inC2Time, FLOAT inC2Value, FLOAT inTime2,
                                    FLOAT inValue2)
{

    // The special case of C1Time=0 and C2Time=0 is used to indicate Studio-native animation.
    // Studio uses a simplified version of the bezier animation where the time control points
    // are equally spaced between the starting and ending times. This avoids calling the expensive
    // InverseBezierCurve function to find the right 's' given 't'.
    FLOAT theParameter;
    if (inC1Time == 0 && inC2Time == 0) {
        // Special case signaling that it's ok to treat time as "s"
        // This is done by assuming that Key1Val,Key1C1,Key1C2,Key2Val (aka P0,P1,P2,P3)
        // are evenly distributed over time.
        theParameter = (inTime - inTime1) / (inTime2 - inTime1);
    } else {
        // Compute the "s" parameter on the Bezier given the time
        theParameter = EvaluateInverseBezierCurve(inTime1, inC1Time, inC2Time, inTime2, inTime);
        if (theParameter <= 0.0f)
            return inValue1;
        if (theParameter >= 1.0f)
            return inValue2;
    }

    return EvaluateBezierCurve(inValue1, inC1Value, inC2Value, inValue2, theParameter);
}
}
