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

//==============================================================================
//	Commands
//==============================================================================

// Command types are 31-bit hash instead of 32-bit
// The 1 bit flag is being used to differentiate between a Command and an Event
// Commands trigger execution, both commands and events can trigger logic

// Asset
const TEventCommandHash COMMAND_SETPROPERTY = CHash::HashEventCommand("COMMAND_SETPROPERTY");
const TEventCommandHash COMMAND_FIREEVENT = CHash::HashEventCommand("COMMAND_FIREEVENT");
const TEventCommandHash COMMAND_SCRIPTLET = CHash::HashEventCommand("COMMAND_SCRIPTLET");
const TEventCommandHash COMMAND_PLAYSOUND = CHash::HashEventCommand("COMMAND_PLAYSOUND");
const TEventCommandHash COMMAND_EMITSIGNAL = CHash::HashEventCommand("COMMAND_EMITSIGNAL");

// Time
const TEventCommandHash COMMAND_PLAY = CHash::HashEventCommand("COMMAND_PLAY");
const TEventCommandHash COMMAND_PAUSE = CHash::HashEventCommand("COMMAND_PAUSE");
const TEventCommandHash COMMAND_GOTOTIME = CHash::HashEventCommand("COMMAND_GOTOTIME");
const TEventCommandHash COMMAND_GOTOTIMELABEL = CHash::HashEventCommand("COMMAND_GOTOTIMELABEL");

// Slide
const TEventCommandHash COMMAND_GOTOSLIDE = CHash::HashEventCommand("COMMAND_GOTOSLIDE");
const TEventCommandHash COMMAND_GOTOSLIDENAME = CHash::HashEventCommand("COMMAND_GOTOSLIDENAME");
const TEventCommandHash COMMAND_GOTONEXTSLIDE = CHash::HashEventCommand("COMMAND_GOTONEXTSLIDE");
const TEventCommandHash COMMAND_GOTOPREVIOUSSLIDE =
    CHash::HashEventCommand("COMMAND_GOTOPREVIOUSSLIDE");
const TEventCommandHash COMMAND_BACKSLIDE = CHash::HashEventCommand("COMMAND_BACKSLIDE");

// Behavior
const TEventCommandHash COMMAND_CUSTOMACTION = CHash::HashEventCommand("COMMAND_CUSTOMACTION");
const TEventCommandHash COMMAND_CUSTOMCALLBACK = CHash::HashEventCommand("COMMAND_CALLBACK");
//==============================================================================
//	Events
//==============================================================================

// Slide Events
const TEventCommandHash EVENT_ONSLIDEENTER = CHash::HashEventCommand("onSlideEnter");
const TEventCommandHash EVENT_ONSLIDEEXIT = CHash::HashEventCommand("onSlideExit");

} // namespace Q3DStudio
