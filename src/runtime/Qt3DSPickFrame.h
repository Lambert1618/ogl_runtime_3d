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
#include "Qt3DSInputFrame.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class IPresentation;

//==============================================================================
/**
 *	Reporting structure describing a complete mouse pick including initial
 *	mouse XY screen position, which element was hit, UV point of hit, global
 *	intersection point and more.
 */
struct SPickFrame
{
    SInputFrame m_InputFrame; ///< Input values to process

    FLOAT m_PickOrigin[3]; ///< 3D pick ray start
    FLOAT m_PickDirection[3]; ///< 3D pick ray direction

    FLOAT m_LocalHit[2]; ///< 2D pick ray intersection
    FLOAT m_SquaredDistance; ///< Distance from camera to intersection
    TElement *m_Model; ///< Element picked
    // IPresentation*	m_SubPresentation;	///< The picked element has a subpresentation

    BOOL m_ResultValid; ///< Model found - Whole structure is valid
    INT8 m_Unused[3]; ///< Padding
};

} // namespace Q3DStudio
