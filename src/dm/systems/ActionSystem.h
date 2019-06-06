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
#ifndef ACTIONSYSTEMH
#define ACTIONSYSTEMH
#include "Qt3DSDMActionSystem.h"
#include "Qt3DSDMActionCore.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMSlideCore.h"
#include "Qt3DSDMSlideGraphCore.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMTransactions.h"
#include "SignalsImpl.h"

namespace qt3dsdm {

class CActionSystem : public IActionSystem
{
    TDataCorePtr m_DataCore;
    TSlideCorePtr m_SlideCore;
    TSlideGraphCorePtr m_SlideGraphCore;
    TActionCorePtr m_ActionCore;

    TSlideSystemPtr m_SlideSystem;

    Qt3DSDMInstanceHandle m_ActionInstance;
    Qt3DSDMPropertyHandle m_ActionEyeball;

    std::shared_ptr<ISignalItem> m_Signaller;

public:
    CActionSystem(TDataCorePtr inDataCore, TSlideCorePtr inSlideCore,
                  TSlideGraphCorePtr inSlideGraphCore, TActionCorePtr inActionCore,
                  TSlideSystemPtr inSlideSystem, Qt3DSDMInstanceHandle inActionInstance,
                  Qt3DSDMPropertyHandle inActionEyeball);

    Qt3DSDMActionHandle CreateAction(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                                            SLong4 inTriggerTargetObjects) override;
    void DeleteAction(Qt3DSDMActionHandle inAction) override;
    void GetActions(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                            TActionHandleList &outActions) const override;

    bool GetActionEyeballValue(Qt3DSDMSlideHandle inActiveSlide,
                                       Qt3DSDMActionHandle inAction) const override;
    void SetActionEyeballValue(Qt3DSDMSlideHandle inActiveSlide, Qt3DSDMActionHandle inAction,
                                       bool inValue) override;

    virtual IActionSystemSignalProvider *GetSignalProvider();

private:
    virtual IActionSystemSignalSender *GetSignalSender();

    CActionSystem(const CActionSystem&) = delete;
    CActionSystem& operator=(const CActionSystem&) = delete;
};
}

#endif
