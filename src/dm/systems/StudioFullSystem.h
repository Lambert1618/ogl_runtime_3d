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
#ifndef STUDIOFULLSYSTEMH
#define STUDIOFULLSYSTEMH
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSignals.h"
#include "Qt3DSDMDataTypes.h"

namespace qt3dsdm {
class IPropertySystem;
class ISlideSystem;
class IStudioAnimationSystem;
class CStudioCoreSystem;
class IInstancePropertyCore;
class INotificationEngine;
class IAnimationCore;
class IActionCore;
class ISlideCore;
class IActionSystem;
class ISignalConnection;

class IStudioFullSystemSerializeHelper
{
public:
    virtual ~IStudioFullSystemSerializeHelper() {}
    /**
     *	Given an instance, set the name if possible.  Called for parents of existing instances
     *	so we can find instances by name later.  There is an implicit assumption for partial
     *	serialization that none of the instances that are serialized out are base instances
     *	for other serialized instances.  There is another implicit assumption that the system
     *	can name any instances used a base.
     */
    virtual bool GetInstanceName(Qt3DSDMInstanceHandle inInstance, TCharStr &outName) = 0;
};

typedef std::shared_ptr<IStudioFullSystemSerializeHelper> TStudioFullSystemSerializeHelperPtr;

// The full studio system needs extra information in the deserialize stage.  This information
// requires knowledge of the larger studio universe in order to work and thus clients need to
// pass in an interface.
class IStudioFullSystemDeserializeHelper
{
public:
    virtual ~IStudioFullSystemDeserializeHelper() {}
    /**
     *	Return an instance by name.  Whatever method was used above to name instances
     *	should be used to find a given instance by name.
     */
    virtual Qt3DSDMInstanceHandle FindInstanceByName(const TCharStr &inName) = 0;
    /**
     *	Translate a given guid.  For pass 1, you should only translate ids and just return identity
     *	for anything that isn't an id.
     */
    virtual void TranslateGuidPass1(Qt3DSDMPropertyHandle inProperty, SLong4 inValue) = 0;
    /**
     *	Now translate guids that are used as data variables.  These are links to other objects and
     *the GUID
     *	information should come from what was created in pass 1.
     */
    virtual SLong4 TranslateGuidPass2(Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, SLong4 inValue) = 0;
};

typedef std::shared_ptr<IStudioFullSystemDeserializeHelper> TStudioFullSystemDeserializeHelperPtr;

class CStudioFullSystem : public ITransactionProducer
{
    Q_DISABLE_COPY(CStudioFullSystem)

    // Base that the other types are built upon
    std::shared_ptr<CStudioCoreSystem> m_CoreSystem;

    // The data model exposed to studio
    std::shared_ptr<IPropertySystem> m_PropertySystem;
    std::shared_ptr<ISlideSystem> m_SlideSystem;
    std::shared_ptr<IStudioAnimationSystem> m_AnimationSystem;
    std::shared_ptr<IActionSystem> m_ActionSystem;

    std::vector<std::shared_ptr<ISignalConnection>> m_Connections;

    TTransactionConsumerPtr m_Consumer;
    TSignalItemPtr m_Signaller;
    bool m_AggregateOperation;

public:
    CStudioFullSystem(std::shared_ptr<CStudioCoreSystem> inCoreSystem,
                      Qt3DSDMInstanceHandle inSlideInstance,
                      Qt3DSDMPropertyHandle inComponentGuidProperty,
                      Qt3DSDMInstanceHandle inActionInstance, Qt3DSDMPropertyHandle inActionEyeball);
    virtual ~CStudioFullSystem();

    void BeginAggregateOperation() { m_AggregateOperation = true; }
    void EndAggregateOperation() { m_AggregateOperation = false; }

    std::shared_ptr<IPropertySystem> GetPropertySystem();
    std::shared_ptr<ISlideSystem> GetSlideSystem();
    std::shared_ptr<ISlideCore> GetSlideCore();
    std::shared_ptr<IAnimationCore> GetAnimationCore();
    std::shared_ptr<IStudioAnimationSystem> GetAnimationSystem();
    std::shared_ptr<IActionCore> GetActionCore();
    std::shared_ptr<IActionSystem> GetActionSystem();

    std::shared_ptr<IPropertySystem> GetPropertySystem() const;
    std::shared_ptr<ISlideSystem> GetSlideSystem() const;
    std::shared_ptr<ISlideCore> GetSlideCore() const;
    std::shared_ptr<IAnimationCore> GetAnimationCore() const;
    std::shared_ptr<IStudioAnimationSystem> GetAnimationSystem() const;
    std::shared_ptr<IActionCore> GetActionCore() const;
    std::shared_ptr<IActionSystem> GetActionSystem() const;

    // Don't use this unless you are a test harness or some other more-knowledgeable-than-ordinary
    // entity.
    std::shared_ptr<CStudioCoreSystem> GetCoreSystem();

    // Ignoring animation and the currently active slide, get the canonical instance property value
    // for this instance.
    bool GetCanonicalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                           Qt3DSDMPropertyHandle inProperty, SValue &outValue) const;
    Qt3DSDMInstanceHandle FindInstanceByName(Qt3DSDMPropertyHandle inNameProperty,
                                            const TCharStr &inName) const;

    void SetConsumer(TTransactionConsumerPtr inConsumer) override;
    TTransactionConsumerPtr GetConsumer() { return m_Consumer; }

    // This signal provider sends signals during undo/redo and during
    // animation runs.
    IStudioFullSystemSignalProvider *GetSignalProvider();

    // Return the signal sender so that we can activate/deactivate signals
    IStudioFullSystemSignalSender *GetSignalSender();
};

typedef std::shared_ptr<CStudioFullSystem> TStudioFullSystemPtr;
}

#endif
