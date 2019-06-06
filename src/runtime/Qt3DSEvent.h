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
#include "Qt3DSKernelTypes.h"
#include "Qt3DSElementSystem.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

/// Common structure used as both Events and Commands
struct SEventCommand
{
    TElement *m_Target; ///< The target of this action
    TEventCommandHash m_Type; ///< Type of action to perform
    UVariant m_Arg1; ///< Argument 1
    UVariant m_Arg2; ///< Argument 2
    UINT8 m_Arg1Type; ///< EAttributeType for arg1 variant
    UINT8 m_Arg2Type; ///< EAttributeType for arg2 variant

    BOOL m_IsEvent : 1; ///< This is an Event or Command
    BOOL m_BubbleUp : 1; ///< Bubble up to scene parent (2.4.2: theEvent:stopPropagation)
    BOOL m_BubbleDown : 1; ///< Bubble down to scene children (2.4.2: theEvent:stopPropagation)
    BOOL m_Done : 1; ///< Stop further handling (2.4.3: theEvent:stopImmediatePropagation)

    UINT8 m_Unused; ///< (padding)
};

} // namespace Q3DStudio
