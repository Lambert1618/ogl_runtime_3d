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
#include "Qt3DSPlatformSpecific.h"
#include "Qt3DSInputDefs.h"

#include <EASTL/map.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Input structure defining the values for keyboard input
 */
struct SKeyInputFrame
{
    UINT16 m_ModifierFlags; ///< Keyboard modifier state flags
    typedef eastl::map<INT32, UINT16> MAP_KEYCODE_COUNT;
    MAP_KEYCODE_COUNT
        m_KeyStates; ///< Keycode map representing number of frames since the keys are down
    ///< 0 - up, 255 - down for more than 254 frames

    SKeyInputFrame()
        : m_ModifierFlags(MODIFIER_NONE)
    {
    }

    UINT16 GetKeyCount(INT32 inKeyCode)
    {
        MAP_KEYCODE_COUNT::iterator theIter = m_KeyStates.find(inKeyCode);
        return theIter != m_KeyStates.end() ? theIter->second : 0;
    }
};

//==============================================================================
/**
 *	Input structure defining the values fed into input processing
 */
struct SInputFrame
{
    SKeyInputFrame m_KeyInputFrame; ///< Data for keyboard input

    typedef eastl::map<INT32, UINT16> MAP_BUTTONCODE_COUNT;
    MAP_BUTTONCODE_COUNT m_ButtonStates;

    FLOAT m_AxisStates[AXIS_TOTAL_COUNT];

    FLOAT m_PickX; ///< Horizontal screen location
    FLOAT m_PickY; ///< Vertical screen location

    INT32 m_MouseFlags; ///< Mouse button state flags

    // SK - gesture-specific variables should NOT be in core runtime, its tegra-specific
    INT32 m_GestureID; ///< a running counter/unique-id
    INT32 m_GestureKind; ///< Gesture state flags

    /* different gestures will interpret as different values.  could be:
     * DRAG: delta position
     * FLICK: velocities
     * ZOOM: second finger or quantized delta */
    INT16 m_DeltaX;
    INT16 m_DeltaY;

    BOOL m_PickValid; ///< X and Y are valid inputs
    INT16 m_ScrollValue; //< Value of scrollwheel scrolled

    CHAR m_Padding[1];

    SInputFrame()
        : m_PickX(0)
        , m_PickY(0)
        , m_MouseFlags(NO_INPUT)
        , m_GestureID(0)
        , m_GestureKind(0)
        , m_DeltaX(0)
        , m_DeltaY(0)
        , m_PickValid(false)
    {
        Q3DStudio_memset(m_AxisStates, 0, sizeof(m_AxisStates));
    }
};

} // namespace Q3DStudio
