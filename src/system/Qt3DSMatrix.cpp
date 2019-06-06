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
//	Includes
//==============================================================================
#include "Qt3DSMatrix.h"
#include "Qt3DSVector3.h"
#include "Qt3DSEulerAngles.h"
#include "Qt3DSDataLogger.h"
#include <math.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
FLOAT g_IdentityInit[4][4] = { { 1.0f, 0.0f, 0.0f, 0.0f },
                               { 0.0f, 1.0f, 0.0f, 0.0f },
                               { 0.0f, 0.0f, 1.0f, 0.0f },
                               { 0.0f, 0.0f, 0.0f, 1.0f } };

const RuntimeMatrix RuntimeMatrix::IDENTITY = RuntimeMatrix(g_IdentityInit);
extern const FLOAT RUNTIME_EPSILON;

//==============================================================================
/**
 *	Empty constructor.
 *	Initializes this matrix to the identity matrix.
 */
RuntimeMatrix::RuntimeMatrix(const BOOL inInitializeIdentity /*= true*/)
{
    if (inInitializeIdentity)
        Q3DStudio_memcpy(&m_Data, &IDENTITY, sizeof(m_Data));
}

//==============================================================================
/**
 *	Copy constructor.
 *	@param inMatrix			the source matrix to copy
 */
RuntimeMatrix::RuntimeMatrix(const RuntimeMatrix &inMatrix)
{
    Q3DStudio_memcpy(&m_Data, &inMatrix.m_Data, sizeof(m_Data));
}

//==============================================================================
/**
 *	Assignment constructor.
 *	Initializes the matrix from an array.
 *	@param inComponents		an array of 16 values
 */
RuntimeMatrix::RuntimeMatrix(const FLOAT inComponents[4][4])
{
    if (inComponents)
        Q3DStudio_memcpy(&m_Data, inComponents, sizeof(m_Data));
}

//==============================================================================
/**
 *	Sets this matrix to a NULL matrix with all values at zero.
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::Zero()
{
    Q3DStudio_memset(&m_Data, 0, sizeof(m_Data));
    return *this;
}

//==============================================================================
/**
 *	Sets the matrix to the identity matrix.
 *	@return reference to this matrix
 */
RuntimeMatrix &RuntimeMatrix::Identity()
{
    Q3DStudio_memcpy(&m_Data, &IDENTITY, sizeof(m_Data));
    return *this;
}

//==============================================================================
/**
 *	Copies the elements from another matrix.
 *	@param inMatrix		the source matrix
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::Set(const RuntimeMatrix &inMatrix)
{
    Q3DStudio_memcpy(&m_Data, &inMatrix.m_Data, sizeof(m_Data));
    return *this;
}

//==============================================================================
/**
 *	Copies the given components to this matrix.
 *	@param inComponents		an array of 16 components
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::Set(const FLOAT inComponents[4][4])
{
    if (inComponents)
        Q3DStudio_memcpy(&m_Data, inComponents, sizeof(m_Data));
    return *this;
}

//==============================================================================
/**
 *	Heavily optimized assignment method.  This has the same result as creating
 *	four full 4x4 matrices and multiplying them together.
 *	@todo Look into optimized code.
 *	@param inTranslation		translation coordinates
 *	@param inRotation			Euler angle rotations
 *	@param inScale				scaling dimensions
 *	@param inPivot				pivot offset vector
 *	@param inRotationOrder		rotation order
 *	@param inCoordinateSystem	the coordindate system
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::Set(const RuntimeVector3 &inTranslation, const RuntimeVector3 &inRotation,
                      const RuntimeVector3 &inScale, const RuntimeVector3 &inPivot, const UINT8 inRotationOrder,
                      const UINT8 inCoordinateSystem)
{
    //	*this = m_ScaleMatrix * m_PivotMatrix * m_RotMatrix * m_TransMatrix;	// Distributed
    //original - plain speak

    RuntimeVector3 theScaledPivot;
    theScaledPivot.m_X = -inPivot.m_X * inScale.m_X;
    theScaledPivot.m_Y = -inPivot.m_Y * inScale.m_Y;
    theScaledPivot.m_Z = -inPivot.m_Z * inScale.m_Z;

    // This would take care of rotation in the right order
    RuntimeMatrix theRotation;
    theRotation.SetRotate(inRotation, inRotationOrder, inCoordinateSystem);

    Q3DStudio_memcpy(&m_Data, &IDENTITY, sizeof(m_Data));
    m_Data[0][0] = inScale.m_X;
    m_Data[1][1] = inScale.m_Y;
    m_Data[2][2] = inScale.m_Z;

    m_Data[3][0] = theScaledPivot.m_X;
    m_Data[3][1] = theScaledPivot.m_Y;

    if (inCoordinateSystem == ORIENTATION_LEFT_HANDED)
        m_Data[3][2] = theScaledPivot.m_Z;
    else
        m_Data[3][2] = -theScaledPivot.m_Z;

    MultiplyAffine(theRotation); // This could be optimized to only 9 multiplications instead of 16
                                 // if you do them by hand

    m_Data[3][0] += inTranslation.m_X;
    m_Data[3][1] += inTranslation.m_Y;

    if (inCoordinateSystem == ORIENTATION_LEFT_HANDED)
        m_Data[3][2] += inTranslation.m_Z;
    else
        m_Data[3][2] += -inTranslation.m_Z;

    if (inCoordinateSystem == ORIENTATION_LEFT_HANDED)
        return FlipCoordinateSystem();

    return *this;
}

//==============================================================================
/**
 *	Sets the translation portion of the matrix without affecting the rest.
 *
 *	If you want a pure translation matrix you must make sure to start with
 *	the identity matrix.
 *	@param inTranslate		a translation vector
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::SetTranslate(const RuntimeVector3 &inTranslate)
{
    m_Data[3][0] = inTranslate.m_X;
    m_Data[3][1] = inTranslate.m_Y;
    m_Data[3][2] = inTranslate.m_Z;
    return *this;
}

//==============================================================================
/**
 *	Sets the rotation portion of the matrix without affecting the rest.
 *
 *	FYI: The rotate and scale portions overlap.  If you want a pure scale matrix
 *	you must make sure to start with the identity matrix.
 *	@param inRotation		a quaternion describing the rotation
 *	@return reference to this modified matrix
 */
// CMatrix& CMatrix::SetRotate( const CQuaternion& inRotation )
//{
//	inRotation.ToMatrix( *this );
//	return *this;
//}

//==============================================================================
/**
 *	Sets the rotation portion of the matrix without affecting the rest.
 *
 *	FYI: The rotate and scale portions overlap.  If you want a pure rotation matrix
 *	you must make sure to start with the identity matrix.
 *	@param inRotation			Euler angle rotation
 *	@param inRotationOrder		rotation order
 *	@param inCoordinateSystem	the coordindate system
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::SetRotate(const RuntimeVector3 &inRotation, const UINT8 inRotationOrder,
                            const UINT8 inCoordinateSystem)
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);
    Q3DStudio_UNREFERENCED_PARAMETER(inCoordinateSystem);

    EulerAngles theEulerAngles;
    switch (inRotationOrder) {
    case ROTATIONORDER_XYZ:
        theEulerAngles.x = inRotation.m_X;
        theEulerAngles.y = inRotation.m_Y;
        theEulerAngles.z = inRotation.m_Z;
        theEulerAngles.w = EulOrdXYZs;
        break;
    case ROTATIONORDER_XYZR:
        theEulerAngles.x = inRotation.m_X;
        theEulerAngles.y = inRotation.m_Y;
        theEulerAngles.z = inRotation.m_Z;
        theEulerAngles.w = EulOrdXYZr;
        break;
    case ROTATIONORDER_YZX:
        theEulerAngles.x = inRotation.m_Y;
        theEulerAngles.y = inRotation.m_Z;
        theEulerAngles.z = inRotation.m_X;
        theEulerAngles.w = EulOrdYZXs;
        break;
    case ROTATIONORDER_YZXR:
        theEulerAngles.x = inRotation.m_Y;
        theEulerAngles.y = inRotation.m_Z;
        theEulerAngles.z = inRotation.m_X;
        theEulerAngles.w = EulOrdYZXr;
        break;
    case ROTATIONORDER_ZXY:
        theEulerAngles.x = inRotation.m_Z;
        theEulerAngles.y = inRotation.m_X;
        theEulerAngles.z = inRotation.m_Y;
        theEulerAngles.w = EulOrdZXYs;
        break;
    case ROTATIONORDER_ZXYR:
        theEulerAngles.x = inRotation.m_Z;
        theEulerAngles.y = inRotation.m_X;
        theEulerAngles.z = inRotation.m_Y;
        theEulerAngles.w = EulOrdZXYr;
        break;
    case ROTATIONORDER_XZY:
        theEulerAngles.x = inRotation.m_X;
        theEulerAngles.y = inRotation.m_Z;
        theEulerAngles.z = inRotation.m_Y;
        theEulerAngles.w = EulOrdXZYs;
        break;
    case ROTATIONORDER_XZYR:
        theEulerAngles.x = inRotation.m_X;
        theEulerAngles.y = inRotation.m_Z;
        theEulerAngles.z = inRotation.m_Y;
        theEulerAngles.w = EulOrdXZYr;
        break;
    case ROTATIONORDER_YXZ:
        theEulerAngles.x = inRotation.m_Y;
        theEulerAngles.y = inRotation.m_X;
        theEulerAngles.z = inRotation.m_Z;
        theEulerAngles.w = EulOrdYXZs;
        break;
    case ROTATIONORDER_YXZR:
        theEulerAngles.x = inRotation.m_Y;
        theEulerAngles.y = inRotation.m_X;
        theEulerAngles.z = inRotation.m_Z;
        theEulerAngles.w = EulOrdYXZr;
        break;
    case ROTATIONORDER_ZYX:
        theEulerAngles.x = inRotation.m_Z;
        theEulerAngles.y = inRotation.m_Y;
        theEulerAngles.z = inRotation.m_X;
        theEulerAngles.w = EulOrdZYXs;
        break;
    case ROTATIONORDER_ZYXR:
        theEulerAngles.x = inRotation.m_Z;
        theEulerAngles.y = inRotation.m_Y;
        theEulerAngles.z = inRotation.m_X;
        theEulerAngles.w = EulOrdZYXr;
        break;
    default: // defaults to Studio's rotation type
        theEulerAngles.x = inRotation.m_Y;
        theEulerAngles.y = inRotation.m_X;
        theEulerAngles.z = inRotation.m_Z;
        theEulerAngles.w = EulOrdYXZs;
        break;
    }

    theEulerAngles.x *= -1;
    theEulerAngles.y *= -1;
    theEulerAngles.z *= -1;

    CEulerAngleConverter theConverter;
    theConverter.Eul_ToHMatrix(theEulerAngles, m_Data);

    return *this;
}

//==============================================================================
/**
 *	Sets the scale portion of the matrix without affecting the rest.
 *
 *	FYI: The rotate and scale portions overlap.  If you want a pure scale matrix
 *	you must make sure to start with the identity matrix.
 *	@param inScale		scale vector
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::SetScale(const RuntimeVector3 &inScale)
{
    m_Data[0][0] = inScale.m_X;
    m_Data[1][1] = inScale.m_Y;
    m_Data[2][2] = inScale.m_Z;
    return *this;
}

//==============================================================================
/**
 *	Check this matrix against the identity matrix.
 *	@return true if this matrix is equivalent to the identity matrix
 */
BOOL RuntimeMatrix::IsIdentity() const
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    return *this == IDENTITY;
}

//==============================================================================
/**
 *	Checks to see if this matrix is affine.
 *	An affine matrix has the last row being [ 0 0 0 1 ]
 *	@return true if the matrix is affine
 */
BOOL RuntimeMatrix::IsAffine() const
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    return m_Data[0][3] == 0 && m_Data[1][3] == 0 && m_Data[2][3] == 0 && m_Data[3][3] == 1.0f;
}

//==============================================================================
/**
 *	Appends the matrix to include a translation.  Equivalent to post-multiplying
 *	this matrix with a transformation matrix derived from the given vector.
 *	@param inTranslation	transformation vector applied
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::Translate(const RuntimeVector3 &inTranslation)
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    m_Data[3][0] += inTranslation.m_X;
    m_Data[3][1] += inTranslation.m_Y;
    m_Data[3][2] += inTranslation.m_Z;
    return *this;
}

//==============================================================================
/**
 *	Appends the matrix to include a rotation.  Equivalent to post-multiplying
 *	this matrix with a rotation matrix derived from the given quaternion.
 *	@param inRotation		the rotation quaternion applied
 *	@return reference to this modified matrix
 */
// CMatrix& CMatrix::Rotate( const CQuaternion& inRotation )
//{
//	CMatrix		theRotation;
//	return MultiplyAffine( inRotation.ToMatrix( theRotation ) );
//}

//==============================================================================
/**
 *	Appends the matrix to include scaling.  Equivalent to post-multiplying
 *	this matrix with a scale matrix derived from the given vector.
 *	@param inScale		the scale vector applied
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::Scale(const RuntimeVector3 &inScale)
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    m_Data[0][0] *= inScale.m_X;
    m_Data[0][1] *= inScale.m_X;
    m_Data[0][2] *= inScale.m_X;

    m_Data[1][0] *= inScale.m_Y;
    m_Data[1][1] *= inScale.m_Y;
    m_Data[1][2] *= inScale.m_Y;

    m_Data[2][0] *= inScale.m_Z;
    m_Data[2][1] *= inScale.m_Z;
    m_Data[2][2] *= inScale.m_Z;
    return *this;
}

//==============================================================================
/**
 *	Flips the matrix elements around the identity diagonal.
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::Transpose()
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    FLOAT theSwap = m_Data[1][0];
    m_Data[1][0] = m_Data[0][1];
    m_Data[0][1] = theSwap;

    theSwap = m_Data[2][0];
    m_Data[2][0] = m_Data[0][2];
    m_Data[0][2] = theSwap;

    theSwap = m_Data[3][0];
    m_Data[3][0] = m_Data[0][3];
    m_Data[0][3] = theSwap;

    theSwap = m_Data[2][1];
    m_Data[2][1] = m_Data[1][2];
    m_Data[1][2] = theSwap;

    theSwap = m_Data[3][1];
    m_Data[3][1] = m_Data[1][3];
    m_Data[1][3] = theSwap;

    theSwap = m_Data[3][2];
    m_Data[3][2] = m_Data[2][3];
    m_Data[2][3] = theSwap;

    return *this;
}

//==============================================================================
/**
 * Compute the inverse of a 3D affine matrix; i.e. a matrix with a
 * dimensionality of 4 where the bottom row has the entries (0, 0, 0, 1).
 *
 * This procedure treats the 4 by 4 matrix as a block matrix and
 * calculates the inverse of one submatrix for a significant performance
 * improvement over a general procedure that can invert any non-singular matrix:
@code
       |           | -1    |   -1       -1 |
       | A       C |       |  A    -C A    |
  -1   |           |       |               |
 M  =  |           |   =   |               |
       | 0       1 |       |  0       1    |
       |           |       |               |
@endcode
 * where M is a 4 by 4 matrix,
 * A is the 3 by 3 upper left submatrix of M,
 * C is the 3 by 1 upper right submatrix of M.
 *
 * @return the determinant of matrix
 */
FLOAT RuntimeMatrix::Invert()
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    const FLOAT PRECISIONLIMIT = 1.0e-07f;
    FLOAT thePositiveDet = 0.0f;
    FLOAT theNegativeDet = 0.0f;
    FLOAT theTempDet;

    // Calculate the determinant of submatrix A and determine if the
    // the matrix is singular as limited by the float precision.
    theTempDet = m_Data[0][0] * m_Data[1][1] * m_Data[2][2];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = m_Data[0][1] * m_Data[1][2] * m_Data[2][0];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = m_Data[0][2] * m_Data[1][0] * m_Data[2][1];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m_Data[0][2] * m_Data[1][1] * m_Data[2][0];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m_Data[0][1] * m_Data[1][0] * m_Data[2][2];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    theTempDet = -m_Data[0][0] * m_Data[1][2] * m_Data[2][1];
    if (theTempDet >= 0.0f)
        thePositiveDet += theTempDet;
    else
        theNegativeDet += theTempDet;

    // Is the submatrix A nonsingular? (i.e. there is an inverse?)
    FLOAT theDeterminant = thePositiveDet + theNegativeDet;
    if (theDeterminant != 0
        || ::fabs(theDeterminant / (thePositiveDet - theNegativeDet)) >= PRECISIONLIMIT) {
        RuntimeMatrix theInverse;
        FLOAT theInvDeterminant = 1.0f / theDeterminant;

        // Calculate inverse(A) = adj(A) / det(A)
        theInverse.m_Data[0][0] =
            (m_Data[1][1] * m_Data[2][2] - m_Data[1][2] * m_Data[2][1]) * theInvDeterminant;
        theInverse.m_Data[1][0] =
            -(m_Data[1][0] * m_Data[2][2] - m_Data[1][2] * m_Data[2][0]) * theInvDeterminant;
        theInverse.m_Data[2][0] =
            (m_Data[1][0] * m_Data[2][1] - m_Data[1][1] * m_Data[2][0]) * theInvDeterminant;
        theInverse.m_Data[0][1] =
            -(m_Data[0][1] * m_Data[2][2] - m_Data[0][2] * m_Data[2][1]) * theInvDeterminant;
        theInverse.m_Data[1][1] =
            (m_Data[0][0] * m_Data[2][2] - m_Data[0][2] * m_Data[2][0]) * theInvDeterminant;
        theInverse.m_Data[2][1] =
            -(m_Data[0][0] * m_Data[2][1] - m_Data[0][1] * m_Data[2][0]) * theInvDeterminant;
        theInverse.m_Data[0][2] =
            (m_Data[0][1] * m_Data[1][2] - m_Data[0][2] * m_Data[1][1]) * theInvDeterminant;
        theInverse.m_Data[1][2] =
            -(m_Data[0][0] * m_Data[1][2] - m_Data[0][2] * m_Data[1][0]) * theInvDeterminant;
        theInverse.m_Data[2][2] =
            (m_Data[0][0] * m_Data[1][1] - m_Data[0][1] * m_Data[1][0]) * theInvDeterminant;

        // Calculate -C * inverse(A)
        theInverse.m_Data[3][0] =
            -(m_Data[3][0] * theInverse.m_Data[0][0] + m_Data[3][1] * theInverse.m_Data[1][0]
              + m_Data[3][2] * theInverse.m_Data[2][0]);
        theInverse.m_Data[3][1] =
            -(m_Data[3][0] * theInverse.m_Data[0][1] + m_Data[3][1] * theInverse.m_Data[1][1]
              + m_Data[3][2] * theInverse.m_Data[2][1]);
        theInverse.m_Data[3][2] =
            -(m_Data[3][0] * theInverse.m_Data[0][2] + m_Data[3][1] * theInverse.m_Data[1][2]
              + m_Data[3][2] * theInverse.m_Data[2][2]);

        // assign ourselves to the inverse
        *this = theInverse;
    }

    return theDeterminant;
}

//==============================================================================
/**
 *	Fast multiplication of two affine 4x4 matrices.  No affine pre-check.
 *  @todo MF - Convert to SSE Assembly Code
 *	@param inMatrix		the source matrix
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::MultiplyAffine(const RuntimeMatrix &inMatrix)
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    FLOAT theMult[4][4];

    theMult[0][0] = m_Data[0][0] * inMatrix.m_Data[0][0] + m_Data[0][1] * inMatrix.m_Data[1][0]
        + m_Data[0][2] * inMatrix.m_Data[2][0];
    theMult[0][1] = m_Data[0][0] * inMatrix.m_Data[0][1] + m_Data[0][1] * inMatrix.m_Data[1][1]
        + m_Data[0][2] * inMatrix.m_Data[2][1];
    theMult[0][2] = m_Data[0][0] * inMatrix.m_Data[0][2] + m_Data[0][1] * inMatrix.m_Data[1][2]
        + m_Data[0][2] * inMatrix.m_Data[2][2];
    theMult[0][3] = 0;

    theMult[1][0] = m_Data[1][0] * inMatrix.m_Data[0][0] + m_Data[1][1] * inMatrix.m_Data[1][0]
        + m_Data[1][2] * inMatrix.m_Data[2][0];
    theMult[1][1] = m_Data[1][0] * inMatrix.m_Data[0][1] + m_Data[1][1] * inMatrix.m_Data[1][1]
        + m_Data[1][2] * inMatrix.m_Data[2][1];
    theMult[1][2] = m_Data[1][0] * inMatrix.m_Data[0][2] + m_Data[1][1] * inMatrix.m_Data[1][2]
        + m_Data[1][2] * inMatrix.m_Data[2][2];
    theMult[1][3] = 0;

    theMult[2][0] = m_Data[2][0] * inMatrix.m_Data[0][0] + m_Data[2][1] * inMatrix.m_Data[1][0]
        + m_Data[2][2] * inMatrix.m_Data[2][0];
    theMult[2][1] = m_Data[2][0] * inMatrix.m_Data[0][1] + m_Data[2][1] * inMatrix.m_Data[1][1]
        + m_Data[2][2] * inMatrix.m_Data[2][1];
    theMult[2][2] = m_Data[2][0] * inMatrix.m_Data[0][2] + m_Data[2][1] * inMatrix.m_Data[1][2]
        + m_Data[2][2] * inMatrix.m_Data[2][2];
    theMult[2][3] = 0;

    theMult[3][0] = m_Data[3][0] * inMatrix.m_Data[0][0] + m_Data[3][1] * inMatrix.m_Data[1][0]
        + m_Data[3][2] * inMatrix.m_Data[2][0] + inMatrix.m_Data[3][0];
    theMult[3][1] = m_Data[3][0] * inMatrix.m_Data[0][1] + m_Data[3][1] * inMatrix.m_Data[1][1]
        + m_Data[3][2] * inMatrix.m_Data[2][1] + inMatrix.m_Data[3][1];
    theMult[3][2] = m_Data[3][0] * inMatrix.m_Data[0][2] + m_Data[3][1] * inMatrix.m_Data[1][2]
        + m_Data[3][2] * inMatrix.m_Data[2][2] + inMatrix.m_Data[3][2];
    theMult[3][3] = 1.0f;

    return Set(theMult);
}

//==============================================================================
/**
 *	Standard matrix multiplication
 *  @todo MF - Convert to SSE Assembly Code
 *	@param inMatrix		matrix to multiply with
 */
RuntimeMatrix &RuntimeMatrix::Multiply(const RuntimeMatrix &inMatrix)
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    FLOAT theMult[4][4];

    theMult[0][0] = m_Data[0][0] * inMatrix.m_Data[0][0] + m_Data[1][0] * inMatrix.m_Data[0][1]
        + m_Data[2][0] * inMatrix.m_Data[0][2] + m_Data[3][0] * inMatrix.m_Data[0][3];
    theMult[0][1] = m_Data[0][1] * inMatrix.m_Data[0][0] + m_Data[1][1] * inMatrix.m_Data[0][1]
        + m_Data[2][1] * inMatrix.m_Data[0][2] + m_Data[3][1] * inMatrix.m_Data[0][3];
    theMult[0][2] = m_Data[0][2] * inMatrix.m_Data[0][0] + m_Data[1][2] * inMatrix.m_Data[0][1]
        + m_Data[2][2] * inMatrix.m_Data[0][2] + m_Data[3][2] * inMatrix.m_Data[0][3];
    theMult[0][3] = m_Data[0][3] * inMatrix.m_Data[0][0] + m_Data[1][3] * inMatrix.m_Data[0][1]
        + m_Data[2][3] * inMatrix.m_Data[0][2] + m_Data[3][3] * inMatrix.m_Data[0][3];
    theMult[1][0] = m_Data[0][0] * inMatrix.m_Data[1][0] + m_Data[1][0] * inMatrix.m_Data[1][1]
        + m_Data[2][0] * inMatrix.m_Data[1][2] + m_Data[3][0] * inMatrix.m_Data[1][3];
    theMult[1][1] = m_Data[0][1] * inMatrix.m_Data[1][0] + m_Data[1][1] * inMatrix.m_Data[1][1]
        + m_Data[2][1] * inMatrix.m_Data[1][2] + m_Data[3][1] * inMatrix.m_Data[1][3];
    theMult[1][2] = m_Data[0][2] * inMatrix.m_Data[1][0] + m_Data[1][2] * inMatrix.m_Data[1][1]
        + m_Data[2][2] * inMatrix.m_Data[1][2] + m_Data[3][2] * inMatrix.m_Data[1][3];
    theMult[1][3] = m_Data[0][3] * inMatrix.m_Data[1][0] + m_Data[1][3] * inMatrix.m_Data[1][1]
        + m_Data[2][3] * inMatrix.m_Data[1][2] + m_Data[3][3] * inMatrix.m_Data[1][3];
    theMult[2][0] = m_Data[0][0] * inMatrix.m_Data[2][0] + m_Data[1][0] * inMatrix.m_Data[2][1]
        + m_Data[2][0] * inMatrix.m_Data[2][2] + m_Data[3][0] * inMatrix.m_Data[2][3];
    theMult[2][1] = m_Data[0][1] * inMatrix.m_Data[2][0] + m_Data[1][1] * inMatrix.m_Data[2][1]
        + m_Data[2][1] * inMatrix.m_Data[2][2] + m_Data[3][1] * inMatrix.m_Data[2][3];
    theMult[2][2] = m_Data[0][2] * inMatrix.m_Data[2][0] + m_Data[1][2] * inMatrix.m_Data[2][1]
        + m_Data[2][2] * inMatrix.m_Data[2][2] + m_Data[3][2] * inMatrix.m_Data[2][3];
    theMult[2][3] = m_Data[0][3] * inMatrix.m_Data[2][0] + m_Data[1][3] * inMatrix.m_Data[2][1]
        + m_Data[2][3] * inMatrix.m_Data[2][2] + m_Data[3][3] * inMatrix.m_Data[2][3];

    theMult[3][0] = m_Data[0][0] * inMatrix.m_Data[3][0] + m_Data[1][0] * inMatrix.m_Data[3][1]
        + m_Data[2][0] * inMatrix.m_Data[3][2] + m_Data[3][0] * inMatrix.m_Data[3][3];
    theMult[3][1] = m_Data[0][1] * inMatrix.m_Data[3][0] + m_Data[1][1] * inMatrix.m_Data[3][1]
        + m_Data[2][1] * inMatrix.m_Data[3][2] + m_Data[3][1] * inMatrix.m_Data[3][3];
    theMult[3][2] = m_Data[0][2] * inMatrix.m_Data[3][0] + m_Data[1][2] * inMatrix.m_Data[3][1]
        + m_Data[2][2] * inMatrix.m_Data[3][2] + m_Data[3][2] * inMatrix.m_Data[3][3];
    theMult[3][3] = m_Data[0][3] * inMatrix.m_Data[3][0] + m_Data[1][3] * inMatrix.m_Data[3][1]
        + m_Data[2][3] * inMatrix.m_Data[3][2] + m_Data[3][3] * inMatrix.m_Data[3][3];

    return Set(theMult);
}

//==============================================================================
/**
 *	Toggle between left-hand and right-hand coordinate system
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::FlipCoordinateSystem()
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    // rotation conversion
    m_Data[0][2] *= -1;
    m_Data[1][2] *= -1;
    m_Data[2][0] *= -1;
    m_Data[2][1] *= -1;

    // translation conversion
    m_Data[3][2] *= -1;

    return *this;
}

//==============================================================================
/**
 *	Rotate this matrix to align with the rotation of the specified matrix.
 *
 *	@param inMatrix		the maxtrix we are cloning.
 *	@param inMirrorFlag	flag indicating that we should flip the z and face the opposite
 *direction.
 *	@return	This matrix after the rotation.
 */
RuntimeMatrix &RuntimeMatrix::CloneRotation(const RuntimeMatrix &inMatrix, BOOL inMirrorFlag /*= false*/)
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    // Create the axes
    RuntimeVector3 theZ(inMatrix.Get(2, 0), inMatrix.Get(2, 1), inMatrix.Get(2, 2));
    if (inMirrorFlag)
        theZ *= -1;
    RuntimeVector3 theY(inMatrix.Get(1, 0), inMatrix.Get(1, 1), inMatrix.Get(1, 2));
    RuntimeVector3 theX(theY);
    theX.CrossProduct(theZ);

    // Normalize
    theX.Normalize();
    theY.Normalize();
    theZ.Normalize();

    // Copy it into the matrix
    m_Data[0][0] = theX.m_X;
    m_Data[0][1] = theX.m_Y;
    m_Data[0][2] = theX.m_Z;

    m_Data[1][0] = theY.m_X;
    m_Data[1][1] = theY.m_Y;
    m_Data[1][2] = theY.m_Z;

    m_Data[2][0] = theZ.m_X;
    m_Data[2][1] = theZ.m_Y;
    m_Data[2][2] = theZ.m_Z;

    return *this;
}

//==============================================================================
/**
 *	Compute the square distance between the position vectors of two transforms.
 *	@param inMatrix the other transform, signifying a global object position for example
 *	@return the square of the "distance" between the two transforms
 */
FLOAT RuntimeMatrix::SquareDistance(const RuntimeMatrix &inMatrix) const
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    FLOAT theResult = 0;
    FLOAT theFactor;

    theFactor = m_Data[3][0] - inMatrix.m_Data[3][0];
    theFactor *= theFactor;
    theResult += theFactor;

    theFactor = m_Data[3][1] - inMatrix.m_Data[3][1];
    theFactor *= theFactor;
    theResult += theFactor;

    theFactor = m_Data[3][2] - inMatrix.m_Data[3][2];
    theFactor *= theFactor;
    theResult += theFactor;

    return theResult;
}

//==============================================================================
/**
 *	Simple assignment operator.
 *	@param inMatrix	new matrix being assigned
 *	@return reference to this modified matrix
 */
RuntimeMatrix &RuntimeMatrix::operator=(const RuntimeMatrix &inMatrix)
{
    Q3DStudio_memcpy(&m_Data, &inMatrix.m_Data, sizeof(m_Data));
    return *this;
}

//==============================================================================
/**
 *	Compares this matrix's elements with another matrix's and returns true
 *	if the matrices are equivalent.
 *	@param inMatrix		the matrix we are comparing against
 *	@return true if all elements are within EPSILON of each other.
 */
BOOL RuntimeMatrix::operator==(const RuntimeMatrix &inMatrix) const
{
    PerfLogMathEvent1(DATALOGGER_MATRIX);

    for (INT32 iRow = 0; iRow < 4; ++iRow) {
        for (INT32 iCol = 0; iCol < 4; ++iCol) {
            if (::fabs(m_Data[iRow][iCol] - inMatrix.m_Data[iRow][iCol]) > RUNTIME_EPSILON)
                return false;
        }
    }
    return true;
}

//==============================================================================
/**
 *	Compares this matrix's elements with another matrix's and returns true
 *	if the matrices are not equivalent.
 *	@param inMatrix		the matrix we are comparing against
 *	@return true if one or more elements are more than EPSILON from each other
 */
BOOL RuntimeMatrix::operator!=(const RuntimeMatrix &inMatrix) const
{
    // Reuse same code path..
    return !(*this == inMatrix);
}

//==============================================================================
/**
 *	Standardized conversion to string.
 *	@param outString	string becoming a representation for the matrix
 */
// void CMatrix::ToString( AK_STRING& outString ) const
//{
//	INT8 theBuffer[ 256 ];
//
//	Q3DStudio_sprintf
//	(
//		theBuffer, 255,
//		"%.2f %.2f %.2f %.2f "
//		"%.2f %.2f %.2f %.2f "
//		"%.2f %.2f %.2f %.2f "
//		"%.2f %.2f %.2f %.2f",
//
//		m_Data.m[ 0 ][ 0 ],
//		m_Data.m[ 0 ][ 1 ],
//		m_Data.m[ 0 ][ 2 ],
//		m_Data.m[ 0 ][ 3 ],
//
//		m_Data.m[ 1 ][ 0 ],
//		m_Data.m[ 1 ][ 1 ],
//		m_Data.m[ 1 ][ 2 ],
//		m_Data.m[ 1 ][ 3 ],
//
//		m_Data.m[ 2 ][ 0 ],
//		m_Data.m[ 2 ][ 1 ],
//		m_Data.m[ 2 ][ 2 ],
//		m_Data.m[ 2 ][ 3 ],
//
//		m_Data.m[ 3 ][ 0 ],
//		m_Data.m[ 3 ][ 1 ],
//		m_Data.m[ 3 ][ 2 ],
//		m_Data.m[ 3 ][ 3 ]
//	);
//
//	outString = theBuffer;
//}
//
////==============================================================================
///**
// *	Standardized conversion from string.
// *	@param inString		string being a representation for the matrix
// */
// void CMatrix::FromString( const AK_STRING& inString )
//{
//	std::sscanf
//	(
//		inString.c_str( ),
//
//		"%f %f %f %f "
//		"%f %f %f %f "
//		"%f %f %f %f "
//		"%f %f %f %f",
//
//		&m_Data.m[ 0 ][ 0 ],
//		&m_Data.m[ 0 ][ 1 ],
//		&m_Data.m[ 0 ][ 2 ],
//		&m_Data.m[ 0 ][ 3 ],
//
//		&m_Data.m[ 1 ][ 0 ],
//		&m_Data.m[ 1 ][ 1 ],
//		&m_Data.m[ 1 ][ 2 ],
//		&m_Data.m[ 1 ][ 3 ],
//
//		&m_Data.m[ 2 ][ 0 ],
//		&m_Data.m[ 2 ][ 1 ],
//		&m_Data.m[ 2 ][ 2 ],
//		&m_Data.m[ 2 ][ 3 ],
//
//		&m_Data.m[ 3 ][ 0 ],
//		&m_Data.m[ 3 ][ 1 ],
//		&m_Data.m[ 3 ][ 2 ],
//		&m_Data.m[ 3 ][ 3 ]
//	);
//}

} // namespace Q3DStudio
