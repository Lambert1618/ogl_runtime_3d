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
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class RuntimeVector3;

//==============================================================================
/**
 *	A row major matrix for 3D transformation.
 *
 *	Matrix[row][column]
@code
        | 0 1 2 3 |  or  | 00 01 02 03 |  or  | Xx Xy Xz w |
        | 4 5 6 7 |      | 10 11 12 13 |      | Yx Yy Yz w |
        | 8 9 A B |      | 20 21 22 23 |      | Zx Zy Zz w |
        | C D E F |      | 30 31 32 33 |      | Tx Ty Tz w |
@endcode
 *	Rotations are concatenated in the the following order: YXZ
 *	All rotations are clockwise when viewed down the positive axis
 *	towards the origin.
 */
class RuntimeMatrix
{
    //==============================================================================
    //	Enumerations
    //==============================================================================
public:
    enum ERotationOrder {
        ROTATIONORDER_XYZ = 0,
        ROTATIONORDER_YZX,
        ROTATIONORDER_ZXY,
        ROTATIONORDER_XZY,
        ROTATIONORDER_YXZ,
        ROTATIONORDER_ZYX,

        ROTATIONORDER_XYZR,
        ROTATIONORDER_YZXR,
        ROTATIONORDER_ZXYR,
        ROTATIONORDER_XZYR,
        ROTATIONORDER_YXZR,
        ROTATIONORDER_ZYXR
    };

    enum EOrientation { ORIENTATION_LEFT_HANDED = 0, ORIENTATION_RIGHT_HANDED };

    //==============================================================================
    //	Constants
    //==============================================================================
public:
    static const RuntimeMatrix IDENTITY; ///< Enabling fast initialization from static identity matrix

    //==============================================================================
    //	Fields
    //==============================================================================
public:
    FLOAT m_Data[4][4]; ///< 16 elements in a 4 x 4

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    RuntimeMatrix(const BOOL inInitializeIdentity = true);
    RuntimeMatrix(const RuntimeMatrix &inMatrix);
    RuntimeMatrix(const FLOAT inComponents[4][4]);

public: // Initialization
    RuntimeMatrix &Zero();
    RuntimeMatrix &Identity();

    RuntimeMatrix &Set(const RuntimeMatrix &inMatrix);
    RuntimeMatrix &Set(const FLOAT inComponents[4][4]);
    RuntimeMatrix &Set(const RuntimeVector3 &inTranslation, const RuntimeVector3 &inRotation, const RuntimeVector3 &inScale,
                 const RuntimeVector3 &inPivot, const UINT8 inRotationOrder,
                 const UINT8 inCoordinateSystem);

    RuntimeMatrix &SetTranslate(const RuntimeVector3 &inTranslate);
    RuntimeMatrix &SetRotate(const RuntimeVector3 &inRotation, const UINT8 inRotationOrder = ROTATIONORDER_XYZ,
                       const UINT8 inCoordinateSystem = ORIENTATION_LEFT_HANDED);
    RuntimeMatrix &SetScale(const RuntimeVector3 &inScale);

public: // Functions
    BOOL IsIdentity() const;
    BOOL IsAffine() const;
    RuntimeMatrix &Translate(const RuntimeVector3 &inTranslate);
    RuntimeMatrix &Scale(const RuntimeVector3 &inScale);
    RuntimeMatrix &Transpose();
    FLOAT Invert();
    RuntimeMatrix &MultiplyAffine(const RuntimeMatrix &inMatrix);
    RuntimeMatrix &Multiply(const RuntimeMatrix &inMatrix);
    RuntimeMatrix &FlipCoordinateSystem();
    RuntimeMatrix &CloneRotation(const RuntimeMatrix &inMatrix, BOOL inMirrorFlag = false);
    FLOAT SquareDistance(const RuntimeMatrix &inMatrix) const;

public: // Operators
    RuntimeMatrix &operator=(const RuntimeMatrix &inMatrix);
    BOOL operator==(const RuntimeMatrix &inMatrix) const;
    BOOL operator!=(const RuntimeMatrix &inMatrix) const;

public: // Script based accessors
    inline FLOAT Get(const INT32 inRow, const INT32 inColumn) const
    {
        return m_Data[inRow][inColumn];
    }
    inline void Set(const INT32 inRow, const INT32 inColumn, const FLOAT inValue)
    {
        m_Data[inRow][inColumn] = inValue;
    }
};

} // namespace Q3DStudio
