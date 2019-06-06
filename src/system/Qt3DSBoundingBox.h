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
#include "Qt3DSVector3.h"

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
 *	Axis aligned 3D bounding box construction and manipulation.
 *
 *	Initial construction creates an inverted box signifying an Empty box instead
 *	of an infinitely small box at 0,0,0.  Keep this in mind when asking for
 *	Min or max on a box that has had no points added to it.
 *
 *	Transforming a box simply transforms all eight corners to span a new
 *	axis aligned bounding box.  Successive transformations can thus create a
 *	non-optimal box, much larger than needed.
 */
class CBoundingBox
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    RuntimeVector3 m_Min; ///< box minimum corner point
    RuntimeVector3 m_Max; ///< box maximum corner point

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CBoundingBox();

public: // Functions
    BOOL IsEmpty() const;
    void SetEmpty();
    void Add(const RuntimeVector3 &inPoint);
    void Add(const CBoundingBox &inBox);
    const RuntimeVector3 &GetMin() const;
    const RuntimeVector3 &GetMax() const;
    void Transform(const RuntimeMatrix &inMatrix);
};

} // namespace Q3DStudio
