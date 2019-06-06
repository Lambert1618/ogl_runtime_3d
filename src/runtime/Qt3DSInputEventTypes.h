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
#include "Qt3DSHash.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

// Mouse events
const TEventCommandHash ON_MOUSEOVER = CHash::HashEventCommand("onMouseOver");
const TEventCommandHash ON_MOUSEOUT = CHash::HashEventCommand("onMouseOut");
const TEventCommandHash ON_GROUPEDMOUSEOVER = CHash::HashEventCommand("onGroupedMouseOver");
const TEventCommandHash ON_GROUPEDMOUSEOUT = CHash::HashEventCommand("onGroupedMouseOut");

// Crude hack to pretend mouse events are gesture events, since gesture events are all you
// can specify in studio.
// TODO: Fix properly, preferably by bringing back gesture support
const TEventCommandHash ON_MOUSEDOWN = CHash::HashEventCommand("onPressureDown");
const TEventCommandHash ON_MOUSEUP = CHash::HashEventCommand("onPressureUp");
const TEventCommandHash ON_MOUSECLICK = CHash::HashEventCommand("onTap");
const TEventCommandHash ON_MOUSEDBLCLICK = CHash::HashEventCommand("onDoubleTap");
//const TEventCommandHash ON_MOUSEDOWN = CHash::HashEventCommand("onMouseDown");
//const TEventCommandHash ON_MOUSEUP = CHash::HashEventCommand("onMouseUp");
//const TEventCommandHash ON_MOUSECLICK = CHash::HashEventCommand("onMouseClick");
//const TEventCommandHash ON_MOUSEDBLCLICK = CHash::HashEventCommand("onMouseDblClick");

const TEventCommandHash ON_MIDDLEMOUSEDOWN = CHash::HashEventCommand("onMiddleMouseDown");
const TEventCommandHash ON_MIDDLEMOUSEUP = CHash::HashEventCommand("onMiddleMouseUp");
const TEventCommandHash ON_MIDDLEMOUSECLICK = CHash::HashEventCommand("onMiddleMouseClick");
const TEventCommandHash ON_MIDDLEDBLMOUSECLICK = CHash::HashEventCommand("onMiddleMouseDblClick");

const TEventCommandHash ON_RIGHTMOUSEDOWN = CHash::HashEventCommand("onRightMouseDown");
const TEventCommandHash ON_RIGHTMOUSEUP = CHash::HashEventCommand("onRightMouseUp");
const TEventCommandHash ON_RIGHTMOUSECLICK = CHash::HashEventCommand("onRightMouseClick");
const TEventCommandHash ON_RIGHTDBLMOUSECLICK = CHash::HashEventCommand("onRightMouseDblClick");

const TEventCommandHash ON_HORIZONTALSCROLLWHEEL =
    CHash::HashEventCommand("onHorizontalScrollWheel");
const TEventCommandHash ON_VERTICALSCROLLWHEEL = CHash::HashEventCommand("onVerticalScrollWheel");

const TEventCommandHash ON_KEYUP =
    CHash::HashEventCommand("onKeyUp"); ///< Studio's keyboard event string
const TEventCommandHash ON_KEYDOWN =
    CHash::HashEventCommand("onKeyDown"); ///< Studio's keyboard event string
const TEventCommandHash ON_KEYREPEAT =
    CHash::HashEventCommand("onKeyRepeat"); ///< Studio's keyboard event string

const TEventCommandHash ON_BUTTONUP = CHash::HashEventCommand("onButtonUp");
const TEventCommandHash ON_BUTTONDOWN = CHash::HashEventCommand("onButtonDown");
const TEventCommandHash ON_BUTTONREPEAT = CHash::HashEventCommand("onButtonRepeat");

const TEventCommandHash ON_AXISMOVED = CHash::HashEventCommand("onAxisMoved");

// Used by UIContract
const TEventCommandHash ON_LEFT = CHash::HashEventCommand("onLeft");
const TEventCommandHash ON_RIGHT = CHash::HashEventCommand("onRight");
const TEventCommandHash ON_UP = CHash::HashEventCommand("onUp");
const TEventCommandHash ON_DOWN = CHash::HashEventCommand("onDown");
const TEventCommandHash ON_BACK = CHash::HashEventCommand("onBack");
const TEventCommandHash ON_SELECT = CHash::HashEventCommand("onSelect");

} // namespace Q3DStudio
