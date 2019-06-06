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
#include "Qt3DSBoundingBox.h"
#include "Qt3DSMatrix.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor - creates an empty BoundingBox
 */
CBoundingBox::CBoundingBox()
    : m_Min(1.0f, 1.0f, 1.0f)
    , m_Max(-1.0f, -1.0f, -1.0f)
{
}

//==============================================================================
/**
 *	Make BoundingBox invalid
 */
void CBoundingBox::SetEmpty()
{
    m_Min.Set(1.0f, 1.0f, 1.0f);
    m_Max.Set(-1.0f, -1.0f, -1.0f);
}

//==============================================================================
/**
 *	Is the BoundingBox empty?
 *	@return true if bounding box is empty, false otherwise
 */
BOOL CBoundingBox::IsEmpty() const
{
    return (m_Min.m_X > m_Max.m_X);
}

//==============================================================================
/**
 *	Add a point to the BoundingBox
 *	@param inPoint		a 3D point
 */
void CBoundingBox::Add(const RuntimeVector3 &inPoint)
{
    if (IsEmpty()) {
        m_Min = inPoint;
        m_Max = inPoint;
    } else {
        m_Min.Minimize(inPoint);
        m_Max.Maximize(inPoint);
    }
}

//==============================================================================
/**
 *	Add another BoundingBox to this BoundingBox
 *	@param inBox	a 3D BoundingBox
 */
void CBoundingBox::Add(const CBoundingBox &inBox)
{
    if (!inBox.IsEmpty()) {
        Add(inBox.GetMin());
        Add(inBox.GetMax());
    }
}

//==============================================================================
/**
 *	Get the bounding box's smallest position
 *	@return the smallest position of the bounding box
 */
const RuntimeVector3 &CBoundingBox::GetMin() const
{
    return m_Min;
}

//==============================================================================
/**
 *	Get the bounding box's largest position
 *	@return the largest position of the bounding box
 */
const RuntimeVector3 &CBoundingBox::GetMax() const
{
    return m_Max;
}

//==============================================================================
/**
 *	Apply a transform to the BoundingBox
 *	@param	inMatrix a 4x4 trasnform matrix
 */
void CBoundingBox::Transform(const RuntimeMatrix &inMatrix)
{
    // Apply transform to the 8 corners of the BoundingBox and renormalize to majox axes
    CBoundingBox theTransformedBox;
    RuntimeVector3 theCorners[8];

    theCorners[0].Set(m_Min.m_X, m_Min.m_Y, m_Min.m_Z);
    theCorners[1].Set(m_Min.m_X, m_Min.m_Y, m_Max.m_Z);
    theCorners[2].Set(m_Min.m_X, m_Max.m_Y, m_Min.m_Z);
    theCorners[3].Set(m_Min.m_X, m_Max.m_Y, m_Max.m_Z);
    theCorners[4].Set(m_Max.m_X, m_Min.m_Y, m_Min.m_Z);
    theCorners[5].Set(m_Max.m_X, m_Min.m_Y, m_Max.m_Z);
    theCorners[6].Set(m_Max.m_X, m_Max.m_Y, m_Min.m_Z);
    theCorners[7].Set(m_Max.m_X, m_Max.m_Y, m_Max.m_Z);

    for (INT32 theIndex = 0; theIndex < 8; ++theIndex) {
        theCorners[theIndex].Transform(inMatrix);
        theTransformedBox.Add(theCorners[theIndex]);
    }
    *this = theTransformedBox;
}

} // namespace Q3DStudio
