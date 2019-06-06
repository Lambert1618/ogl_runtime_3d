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

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants used by the functions
//==============================================================================
const FLOAT M_PHI = 3.14159265358979323846f;

/* epsilon surrounding for near zero values */
const FLOAT EQN_EPS = 1e-9f;

#define ISZERO(x) ((x) > -EQN_EPS && (x) < EQN_EPS)

// cube-root
#ifndef CUBEROOT
#define CUBEROOT(x)                                                                                \
    ((x) > 0.0 ? static_cast<FLOAT>(pow((x), 1.0f / 3.0f))                                         \
               : ((x) < 0.0 ? -static_cast<FLOAT>(pow(-(x), 1.0f / 3.0f)) : 0.0f))
#endif

//==============================================================================
/**
 *	Find the root/s to a quadratic equation.
 *
 *	The equation is of the form: c[2]*x^2 + c[1]*x + c[0] = 0
 *	Note that this will fail if c[2] is zero, or this is a linear equation.
 *
 *	@param inConstants		The array of constants to the equation; see form above
 *	@param outSolutions		The array of solutions to the equation
 *	@return the number of solutions for the equation
 */
INT32 CCubicRoots::SolveQuadric(FLOAT inConstants[3], FLOAT outSolutions[2])
{
#ifdef _PERF_LOG
    PerfLogMathEvent1 thePerfLog(DATALOGGER_CUBICROOT);
#endif
    FLOAT theP, theQ, theD;
    INT32 theResult = 0;

    /* normal form: x^2 + px + theQ = 0 */

    theP = inConstants[1] / (2 * inConstants[2]);
    theQ = inConstants[0] / inConstants[2];

    theD = theP * theP - theQ;

    if (ISZERO(theD)) {
        outSolutions[0] = -theP;
        theResult = 1;
    } else if (theD < 0.0f) {
        theResult = 0;
    } else if (theD > 0.0f) {
        FLOAT theSquareRootD = sqrtf(theD);

        outSolutions[0] = theSquareRootD - theP;
        outSolutions[1] = -theSquareRootD - theP;
        theResult = 2;
    }

    return theResult;
}

//==============================================================================
/**
 *	Find the root/s to a cubic equation.
 *
 *	The equation is of the form: c[3]*x^3 + c[2]*x^2 + c[1]*x + c[0] = 0
 *	Note that this will fail if c[3] is zero, or this is a quadratic equation.
 *
 *	@param inConstants		The array of constants to the equation; see form above
 *	@param outSolutions		The array of solutions to the equation
 *	@return the number of solutions for the equation
 */
INT32 CCubicRoots::SolveCubic(FLOAT inConstants[4], FLOAT outSolutions[3])
{
#ifdef _PERF_LOG
    PerfLogMathEvent1 thePerfLog(DATALOGGER_CUBICROOT);
#endif

    INT32 theResult;
    FLOAT theSubstitute;
    FLOAT theVariableA, theVariableB, theVariableC;
    FLOAT theASquared, theVariableP, theVariableQ;
    FLOAT thePCubed, theVariableD;

    /* normal form: x^3 + Ax^2 + Bx + C = 0 */

    theVariableA = inConstants[2] / inConstants[3];
    theVariableB = inConstants[1] / inConstants[3];
    theVariableC = inConstants[0] / inConstants[3];

    /*  substitute x = y - A/3 to eliminate quadric term:
        x^3 +px + q = 0 */

    theASquared = theVariableA * theVariableA;
    theVariableP = 1.0f / 3.0f * (-1.0f / 3.0f * theASquared + theVariableB);
    theVariableQ = 1.0f / 2.0f * (2.0f / 27.0f * theVariableA * theASquared
                                  - 1.0f / 3.0f * theVariableA * theVariableB + theVariableC);

    /* use Cardano's formula */

    thePCubed = theVariableP * theVariableP * theVariableP;
    theVariableD = theVariableQ * theVariableQ + thePCubed;

    if (ISZERO(theVariableD)) {
        if (ISZERO(theVariableQ)) /* one triple solution */
        {
            outSolutions[0] = 0;
            theResult = 1;
        } else /* one single and one double solution */
        {
            FLOAT theVariableU = CUBEROOT(-theVariableQ);
            outSolutions[0] = 2.0f * theVariableU;
            outSolutions[1] = -theVariableU;
            theResult = 2;
        }
    } else if (theVariableD < 0.0f) /* Casus irreducibilis: three real solutions */
    {
        FLOAT thePhi = 1.0f / 3.0f * static_cast<FLOAT>(acos(-theVariableQ / sqrtf(-thePCubed)));
        FLOAT theVariableT = 2.0f * sqrtf(-theVariableP);

        outSolutions[0] = theVariableT * static_cast<FLOAT>(cos(thePhi));
        outSolutions[1] = -theVariableT * static_cast<FLOAT>(cos(thePhi + M_PHI / 3.0f));
        outSolutions[2] = -theVariableT * static_cast<FLOAT>(cos(thePhi - M_PHI / 3.0f));
        theResult = 3;
    } else /* one real solution */
    {
        FLOAT theSquareRootD = sqrtf(theVariableD);
        FLOAT theVariableU = CUBEROOT(theSquareRootD - theVariableQ);
        FLOAT theVariableV = -CUBEROOT(theSquareRootD + theVariableQ);

        outSolutions[0] = theVariableU + theVariableV;
        theResult = 1;
    }

    /* resubstitute */
    theSubstitute = 1.0f / 3.0f * theVariableA;

    for (INT32 theIndex = 0; theIndex < theResult; ++theIndex)
        outSolutions[theIndex] -= theSubstitute;

    return theResult;
}

}; // namespace Q3DStudio
