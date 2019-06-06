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
#include "ActionSystem.h"

using namespace std;

namespace qt3dsdm {

CActionSystem::CActionSystem(TDataCorePtr inDataCore, TSlideCorePtr inSlideCore,
                             TSlideGraphCorePtr inSlideGraphCore, TActionCorePtr inActionCore,
                             TSlideSystemPtr inSlideSystem, Qt3DSDMInstanceHandle inActionInstance,
                             Qt3DSDMPropertyHandle inActionEyeball)
    : m_DataCore(inDataCore)
    , m_SlideCore(inSlideCore)
    , m_SlideGraphCore(inSlideGraphCore)
    , m_ActionCore(inActionCore)
    , m_SlideSystem(inSlideSystem)
    , m_ActionInstance(inActionInstance)
    , m_ActionEyeball(inActionEyeball)
{
    m_Signaller = CreateActionSystemSignaller();
}

Qt3DSDMActionHandle CActionSystem::CreateAction(Qt3DSDMSlideHandle inSlide,
                                               Qt3DSDMInstanceHandle inOwner,
                                               SLong4 inTriggerTargetObjects)
{
    Q_ASSERT(inSlide.Valid() && inOwner.Valid());

    // Create Action instance handle that derives from Action instance
    Qt3DSDMInstanceHandle theActionInstance = m_DataCore->CreateInstance();
    m_DataCore->DeriveInstance(theActionInstance, m_ActionInstance);

    // Associate Action instance handle with Slide
    m_SlideSystem->AssociateInstanceWithSlide(inSlide, theActionInstance);

    // Unlink the eyeball property because Action can be eyeballed-on/off per-slide
    m_SlideSystem->UnlinkProperty(theActionInstance, m_ActionEyeball);

    // Create the Action handle
    Qt3DSDMActionHandle retval =
        m_ActionCore->CreateAction(theActionInstance, inSlide, inOwner, inTriggerTargetObjects);

    GetSignalSender()->SendActionCreated(retval, inSlide, inOwner);
    return retval;
}

void CActionSystem::DeleteAction(Qt3DSDMActionHandle inAction)
{
    Qt3DSDMInstanceHandle theActionInstance;
    SActionInfo theActionInfo = m_ActionCore->GetActionInfo(inAction);
    m_ActionCore->DeleteAction(inAction, theActionInstance);
    m_DataCore->DeleteInstance(theActionInstance);
    GetSignalSender()->SendActionDeleted(inAction, theActionInfo.m_Slide, theActionInfo.m_Owner);
}

void CActionSystem::GetActions(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                               TActionHandleList &outActions) const
{
    // Get all actions that exist in inSlide
    m_ActionCore->GetActions(inSlide, inOwner, outActions);

    // if inSlide is not master, get all actions that exist in master slide
    Qt3DSDMSlideHandle theMaster = m_SlideSystem->GetMasterSlide(inSlide);
    if (theMaster != inSlide) {
        TActionHandleList theMasterActions;
        m_ActionCore->GetActions(theMaster, inOwner, theMasterActions);
        outActions.insert(outActions.end(), theMasterActions.begin(), theMasterActions.end());
        sort(outActions.begin(), outActions.end());
    }
}

bool CActionSystem::GetActionEyeballValue(Qt3DSDMSlideHandle inActiveSlide,
                                          Qt3DSDMActionHandle inAction) const
{
    SValue theValue;
    Qt3DSDMInstanceHandle theInstance = m_ActionCore->GetActionInstance(inAction);
    // Get the eyeball property value from SlideCore. There is no animation on eyeball so we can
    // query SlideCore directly.
    m_SlideCore->GetInstancePropertyValue(inActiveSlide, theInstance, m_ActionEyeball, theValue);
    if (m_SlideCore->IsSlide(inActiveSlide))
        return qt3dsdm::get<bool>(theValue);
    return false;
}

void CActionSystem::SetActionEyeballValue(Qt3DSDMSlideHandle inActiveSlide,
                                          Qt3DSDMActionHandle inAction, bool inValue)
{
    Qt3DSDMInstanceHandle theInstance = m_ActionCore->GetActionInstance(inAction);
    // Set the eyeball property value to SlideCore.
    m_SlideCore->ForceSetInstancePropertyValue(inActiveSlide, theInstance, m_ActionEyeball,
                                               inValue);
}

IActionSystemSignalProvider *CActionSystem::GetSignalProvider()
{
    return dynamic_cast<IActionSystemSignalProvider *>(m_Signaller.get());
}

IActionSystemSignalSender *CActionSystem::GetSignalSender()
{
    return dynamic_cast<IActionSystemSignalSender *>(m_Signaller.get());
}
}
