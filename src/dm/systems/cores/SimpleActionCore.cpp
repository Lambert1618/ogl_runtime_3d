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
#include "Qt3DSDMPrefix.h"
#include "SimpleActionCore.h"

namespace qt3dsdm {

Qt3DSDMActionHandle CSimpleActionCore::CreateAction(Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMSlideHandle inSlide,
                                                   Qt3DSDMInstanceHandle inOwner,
                                                   SLong4 inTriggerTargetObjects)
{
    int nextId = GetNextId();
    Qt3DSDMActionHandle retval = CreateActionWithHandle(nextId, inInstance, inSlide, inOwner);
    SetTriggerObject(retval, inTriggerTargetObjects);
    SetTargetObject(retval, inTriggerTargetObjects);
    return retval;
}

void CSimpleActionCore::DeleteAction(Qt3DSDMActionHandle inAction, Qt3DSDMInstanceHandle &outInstance)
{
    SAction *theAction = GetActionNF(inAction, m_Objects);
    outInstance = theAction->m_ActionInfo.m_Instance;
    do_all(theAction->m_ActionInfo.m_HandlerArgs,
           std::bind(EraseHandle, std::placeholders::_1, std::ref(m_Objects)));
    EraseHandle(inAction, m_Objects);
}

const SActionInfo &CSimpleActionCore::GetActionInfo(Qt3DSDMActionHandle inAction) const
{
    const SAction *theAction = GetActionNF(inAction, m_Objects);
    return theAction->m_ActionInfo;
}

inline void AddIfActionMatches(const THandleObjectPair &inPair, Qt3DSDMSlideHandle inSlide,
                               Qt3DSDMInstanceHandle inOwner, TActionHandleList &outActions)
{
    if (inPair.second->GetType() == CHandleObject::EHandleObjectTypeAction) {
        const SAction *theAction = static_cast<SAction *>(inPair.second.get());
        if ((!inSlide.Valid() || inSlide == theAction->m_ActionInfo.m_Slide)
            && (!inOwner.Valid() || inOwner == theAction->m_ActionInfo.m_Owner))
            outActions.push_back(inPair.first);
    }
}

// Return all actions that belong to a certain instance in a certain slide
void CSimpleActionCore::GetActions(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                                   TActionHandleList &outActions) const
{
    outActions.clear();
    do_all(m_Objects,
           std::bind(AddIfActionMatches,
                     std::placeholders::_1, inSlide, inOwner, std::ref(outActions)));
}

// Return all actions that exist in a certain slide
void CSimpleActionCore::GetActions(Qt3DSDMSlideHandle inSlide, TActionHandleList &outActions) const
{
    GetActions(inSlide, 0, outActions);
}

// Return all actions that belong to a certain instance
void CSimpleActionCore::GetActions(Qt3DSDMInstanceHandle inOwner,
                                   TActionHandleList &outActions) const
{
    GetActions(0, inOwner, outActions);
}

// Return all actions
void CSimpleActionCore::GetActions(TActionHandleList &outActions) const
{
    outActions.clear();
    outActions.reserve(m_Objects.size());
    do_all(m_Objects,
           std::bind(MaybeAddObject<SAction, Qt3DSDMActionHandle>,
                     std::placeholders::_1, std::ref(outActions)));
}

// Return the instance that was allocated for this action.
Qt3DSDMInstanceHandle CSimpleActionCore::GetActionInstance(Qt3DSDMActionHandle inAction) const
{
    return GetActionNF(inAction, m_Objects)->m_ActionInfo.m_Instance;
}

inline bool ActionInstanceMatches(const THandleObjectPair &inPair, Qt3DSDMInstanceHandle inInstance)
{
    if (inPair.second->GetType() == CHandleObject::EHandleObjectTypeAction) {
        const SAction *theAction = static_cast<SAction *>(inPair.second.get());
        if (inInstance == theAction->m_ActionInfo.m_Instance)
            return true;
    }
    return false;
}

// Reverse lookup into the action system so you can match actions to instances.
Qt3DSDMActionHandle
CSimpleActionCore::GetActionByInstance(Qt3DSDMInstanceHandle inActionInstance) const
{
    THandleObjectMap::const_iterator theAction =
        find_if(m_Objects.begin(), m_Objects.end(),
                std::bind(ActionInstanceMatches, std::placeholders::_1, inActionInstance));
    if (theAction != m_Objects.end())
        return theAction->first;
    throw ActionNotFound(L"");
}

// Action Properties
void CSimpleActionCore::SetTriggerObject(Qt3DSDMActionHandle inAction,
                                         const SObjectRefType &inTriggerObject)
{
    SAction *theAction = GetActionNF(inAction, m_Objects);
    theAction->m_ActionInfo.m_TriggerObject = inTriggerObject;
}

void CSimpleActionCore::SetTargetObject(Qt3DSDMActionHandle inAction,
                                        const SObjectRefType &inTargetObject)
{
    SAction *theAction = GetActionNF(inAction, m_Objects);
    theAction->m_ActionInfo.m_TargetObject = inTargetObject;
}

void CSimpleActionCore::SetEvent(Qt3DSDMActionHandle inAction, const wstring &inEventHandle)
{
    SAction *theAction = GetActionNF(inAction, m_Objects);
    theAction->m_ActionInfo.m_Event = inEventHandle;
}

void CSimpleActionCore::SetHandler(Qt3DSDMActionHandle inAction, const wstring &inHandlerHandle)
{
    SAction *theAction = GetActionNF(inAction, m_Objects);
    theAction->m_ActionInfo.m_Handler = inHandlerHandle;
}

// Action Argument
Qt3DSDMHandlerArgHandle CSimpleActionCore::AddHandlerArgument(Qt3DSDMActionHandle inAction,
                                                             const TCharStr &inName,
                                                             HandlerArgumentType::Value inArgType,
                                                             DataModelDataType::Value inValueType)
{
    int nextId = GetNextId();
    return AddHandlerArgumentWithHandle(nextId, inAction, inName, inArgType, inValueType);
}

void CSimpleActionCore::RemoveHandlerArgument(Qt3DSDMHandlerArgHandle inHandlerArgument)
{
    SHandlerArgument *theHandlerArgument = GetHandlerArgumentNF(inHandlerArgument, m_Objects);
    SAction *theAction = GetActionNF(theHandlerArgument->m_HandlerArgInfo.m_Action, m_Objects);
    EraseHandle(inHandlerArgument, m_Objects);
    erase_if(theAction->m_ActionInfo.m_HandlerArgs,
             std::bind(equal_to<int>(), std::placeholders::_1, inHandlerArgument.GetHandleValue()));
}

const SHandlerArgumentInfo &
CSimpleActionCore::GetHandlerArgumentInfo(Qt3DSDMHandlerArgHandle inHandlerArgument) const
{
    if (HandleValid(inHandlerArgument)) {
        const SHandlerArgument *theHandlerArgument =
            GetHandlerArgumentNF(inHandlerArgument, m_Objects);
        return theHandlerArgument->m_HandlerArgInfo;
    } else {
        static SHandlerArgumentInfo dummy;
        return dummy;
    }
}

void CSimpleActionCore::GetHandlerArguments(Qt3DSDMActionHandle inAction,
                                            THandlerArgHandleList &outHandlerArguments) const
{
    const SAction *theAction = GetActionNF(inAction, m_Objects);
    outHandlerArguments = theAction->m_ActionInfo.m_HandlerArgs;
}

// Action Argument Properties
void CSimpleActionCore::GetHandlerArgumentValue(Qt3DSDMHandlerArgHandle inHandlerArgument,
                                                SValue &outValue) const
{
    const SHandlerArgument *theHandlerArgument = GetHandlerArgumentNF(inHandlerArgument, m_Objects);
    outValue = theHandlerArgument->m_HandlerArgInfo.m_Value;
}

void CSimpleActionCore::SetHandlerArgumentValue(Qt3DSDMHandlerArgHandle inHandlerArgument,
                                                const SValue &inValue)
{
    SHandlerArgument *theHandlerArgument = GetHandlerArgumentNF(inHandlerArgument, m_Objects);
    theHandlerArgument->m_HandlerArgInfo.m_Value = inValue;
}

// Helper functions
Qt3DSDMActionHandle CSimpleActionCore::CreateActionWithHandle(int inHandle,
                                                             Qt3DSDMInstanceHandle inInstance,
                                                             Qt3DSDMSlideHandle inSlide,
                                                             Qt3DSDMInstanceHandle inOwner)
{
    if (HandleValid(inHandle))
        throw HandleExists(L"");
    m_Objects.insert(make_pair(
        inHandle, (THandleObjectPtr) new SAction(inHandle, inInstance, inSlide, inOwner)));
    return inHandle;
}

Qt3DSDMHandlerArgHandle
CSimpleActionCore::AddHandlerArgumentWithHandle(int inHandle, Qt3DSDMActionHandle inAction,
                                                const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                                DataModelDataType::Value inValueType)
{
    if (HandleValid(inHandle))
        throw HandleExists(L"");
    m_Objects.insert(make_pair(inHandle, (THandleObjectPtr) new SHandlerArgument(
                                             inHandle, inAction, inName, inArgType, inValueType)));
    SAction *theAction = GetActionNF(inAction, m_Objects);
    theAction->m_ActionInfo.m_HandlerArgs.push_back(inHandle);
    return inHandle;
}
}
