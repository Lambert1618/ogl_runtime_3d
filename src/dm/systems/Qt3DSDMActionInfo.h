/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DSDM_ACTION_INFO_H
#define QT3DSDM_ACTION_INFO_H

#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMMetaDataTypes.h"

namespace qt3dsdm {
using std::wstring;
struct SActionInfo
{
    // InstanceHandle corresponding to this action (to store other properties not listed here)
    Qt3DSDMInstanceHandle m_Instance;

    // Where the action is added to
    Qt3DSDMSlideHandle m_Slide; // the slide that the action is added to
    Qt3DSDMInstanceHandle
        m_Owner; // the object that the action is added to (the owner of the action)

    // Trigger object
    SObjectRefType m_TriggerObject; // trigger object
    wstring m_Event; // the list of applicable events is based on object type and stored, by type,
                     // in metadata

    // Target object
    SObjectRefType m_TargetObject; // target object
    wstring m_Handler; // the list of applicable action handlers is loaded from metadata xml file
                       // and based on object type
    THandlerArgHandleList m_HandlerArgs; // the list of applicable action arguments is dependent on
                                         // the handler and loaded from the metadata xml file.

    SActionInfo() {}

    SActionInfo(Qt3DSDMInstanceHandle inInstance, Qt3DSDMSlideHandle inSlide,
                Qt3DSDMInstanceHandle inOwner)
        : m_Instance(inInstance)
        , m_Slide(inSlide)
        , m_Owner(inOwner)
    {
    }
};

struct SHandlerArgumentInfo
{
    Qt3DSDMActionHandle m_Action; // Action that owns this Action Argument
    TCharStr m_Name; // Name of the Action Argument
    HandlerArgumentType::Value m_ArgType; // m_ArgType will override m_ValueType
    DataModelDataType::Value m_ValueType; // m_ValueType is ignored if ArgType is specified
    SValue m_Value; // Value of the Action Argument

    SHandlerArgumentInfo()
        : m_ArgType(HandlerArgumentType::None)
        , m_ValueType(DataModelDataType::None)
    {
    }

    SHandlerArgumentInfo(Qt3DSDMActionHandle inAction, const TCharStr &inName,
                         HandlerArgumentType::Value inArgType, DataModelDataType::Value inValueType)
        : m_Action(inAction)
        , m_Name(inName)
        , m_ArgType(inArgType)
        , m_ValueType(inValueType)
    {
    }
};
}

#endif
