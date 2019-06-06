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
#ifndef QT3DSDM_SIGNALS_H
#define QT3DSDM_SIGNALS_H
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMAnimation.h"
#include "Qt3DSDMActionInfo.h"
#include "Qt3DSDMValue.h"
#include <functional>
#include <string>

namespace qt3dsdm {

class ISignalConnection
{
public:
    virtual ~ISignalConnection() {}
};

class ISignalItem
{
public:
    virtual ~ISignalItem() {}
};

typedef std::shared_ptr<ISignalItem> TSignalItemPtr;

typedef std::shared_ptr<ISignalConnection> TSignalConnectionPtr;

class QtSignalConnection : public ISignalConnection
{
    Q_DISABLE_COPY(QtSignalConnection)
private:
    QMetaObject::Connection m_connection;
public:
    QtSignalConnection(const QMetaObject::Connection &inConnection)
        : m_connection(inConnection)
    {
    }
    ~QtSignalConnection() override
    {
        QObject::disconnect(m_connection);
    }
};


class IInstancePropertyCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue &)>
            &inCallback) = 0;
};

class IInstancePropertyCoreSignalSender : public ISignalItem
{
public:
    virtual void SignalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMPropertyHandle inProperty,
                                             const SValue &inValue) = 0;
};

class IDataCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectBeforeInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceDerived(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceParentRemoved(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectPropertyAdded(const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                    TCharPtr, DataModelDataType::Value)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyRemoved(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, TCharPtr,
                                   DataModelDataType::Value)> &inCallback) = 0;
};

class IDataCoreSignalSender : public ISignalItem
{
public:
    virtual void SignalInstanceCreated(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SignalBeforeInstanceDeleted(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SignalInstanceDeleted(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SignalInstanceDerived(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMInstanceHandle inParent) = 0;
    virtual void SignalInstanceParentRemoved(Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMInstanceHandle inParent) = 0;
    virtual void SignalPropertyAdded(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty, TCharPtr inName,
                                     DataModelDataType::Value inDataType) = 0;
    virtual void SignalPropertyRemoved(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, TCharPtr inName,
                                       DataModelDataType::Value inDataType) = 0;
};

class ISlideCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectBeforeSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideDerived(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, int)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstancePropertyValueSet(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstancePropertyValueRemoved(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectSlideTimeChanged(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
};

class ISlideCoreSignalSender : public ISignalItem
{
public:
    virtual void SendSlideCreated(Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendBeforeSlideDeleted(Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendSlideDeleted(Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendSlideDerived(Qt3DSDMSlideHandle inSlide, Qt3DSDMSlideHandle inParent,
                                  int inIndex) = 0;
    virtual void SendPropertyValueSet(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inParent, const SValue &inValue) = 0;
    // This gives clients a chance to override a property value the first time it is set on a slide
    virtual void SendPropertyValueRemoved(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inParent, const SValue &inValue) = 0;
    virtual void SendSlideTimeChanged(Qt3DSDMSlideHandle inSlide) = 0;
};

class ISlideGraphCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectGraphCreated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectGraphDeleted(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectGraphActiveSlide(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) = 0;
};

class ISlideGraphCoreSignalSender : public ISignalItem
{
public:
    virtual void SendGraphCreated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendGraphDeleted(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendInstanceAssociated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide,
                                        Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendInstanceDissociated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide,
                                         Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph,
                                      Qt3DSDMSlideHandle inSlide) = 0;
};

class IAnimationCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectAnimationCreated(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectBeforeAnimationDeleted(
        const std::function<void(Qt3DSDMAnimationHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectAnimationDeleted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectBeforeKeyframeErased(const std::function<void(Qt3DSDMKeyframeHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectBeforeAllKeyframesErased(
        const std::function<void(Qt3DSDMAnimationHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeUpdated(
        const std::function<void(Qt3DSDMKeyframeHandle, const TKeyframe &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(Qt3DSDMAnimationHandle, bool)> &inCallback) = 0;
};

class IAnimationCoreSignalSender : public ISignalItem
{
public:
    virtual void SendAnimationCreated(Qt3DSDMAnimationHandle inAnimation, Qt3DSDMSlideHandle inSlide,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType) = 0;
    virtual void SendBeforeAnimationDeleted(Qt3DSDMAnimationHandle inAnimation) = 0;
    virtual void SendAnimationDeleted(Qt3DSDMAnimationHandle inAnimation, Qt3DSDMSlideHandle inSlide,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType) = 0;
    virtual void SendKeyframeInserted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) = 0;
    virtual void SendBeforeKeyframeErased(Qt3DSDMKeyframeHandle inAnimation) = 0;
    virtual void SendKeyframeErased(Qt3DSDMAnimationHandle inAnimation,
                                    Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) = 0;
    virtual void SendBeforeAllKeyframesErased(Qt3DSDMAnimationHandle inAnimation) = 0;
    virtual void SendKeyframeUpdated(Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) = 0;
    virtual void SendFirstKeyframeDynamicSet(Qt3DSDMAnimationHandle inAnimation,
                                             bool inKeyframeDynamic) = 0;
};

class IActionCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectTriggerObjectSet(
        const std::function<void(Qt3DSDMActionHandle, SObjectRefType &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectTargetObjectSet(
        const std::function<void(Qt3DSDMActionHandle, SObjectRefType &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectEventSet(
        const std::function<void(Qt3DSDMActionHandle, const wstring &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectHandlerSet(
        const std::function<void(Qt3DSDMActionHandle, const wstring &)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectHandlerArgumentAdded(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectHandlerArgumentRemoved(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectHandlerArgumentValueSet(
        const std::function<void(Qt3DSDMHandlerArgHandle, const SValue &)> &inCallback) = 0;
};

class IActionCoreSignalSender : public ISignalItem
{
public:
    virtual void SendTriggerObjectSet(Qt3DSDMActionHandle inAction,
                                      SObjectRefType &inTriggerObject) = 0;
    virtual void SendTargetObjectSet(Qt3DSDMActionHandle inAction,
                                     SObjectRefType &inTargetObject) = 0;
    virtual void SendEventSet(Qt3DSDMActionHandle inAction, const wstring &inEventHandle) = 0;
    virtual void SendHandlerSet(Qt3DSDMActionHandle inAction, const wstring &inActionName) = 0;

    virtual void SendHandlerArgumentAdded(Qt3DSDMActionHandle inAction,
                                          Qt3DSDMHandlerArgHandle inHandlerArgument,
                                          const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                          DataModelDataType::Value inValueType) = 0;
    virtual void SendHandlerArgumentRemoved(Qt3DSDMActionHandle inAction,
                                            Qt3DSDMHandlerArgHandle inHandlerArgument,
                                            const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                            DataModelDataType::Value inValueType) = 0;
    virtual void SendHandlerArgumentValueSet(Qt3DSDMHandlerArgHandle inHandlerArgument,
                                             const SValue &inValue) = 0;
};

class IActionSystemSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectActionCreated(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectActionDeleted(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
};

class IActionSystemSignalSender : public ISignalItem
{
public:
    virtual void SendActionCreated(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendActionDeleted(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) = 0;
};

class ICustomPropCoreSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr ConnectCustomPropertyCreated(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomPropertyDeleted(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomPropertyModified(
        const std::function<void(Qt3DSDMPropertyHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomEventCreated(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomEventDeleted(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectCustomEventModified(const std::function<void(Qt3DSDMEventHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomHandlerCreated(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerDeleted(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectCustomHandlerModified(const std::function<void(Qt3DSDMHandlerHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomHandlerParamCreated(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamDeleted(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamModified(
        const std::function<void(Qt3DSDMHandlerParamHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomReferencesModified(
        const std::function<void(Qt3DSDMInstanceHandle, const TCharStr &)> &inCallback) = 0;
};

class ICustomPropCoreSignalSender : public ISignalItem
{
public:
    virtual void SendCustomPropertyCreated(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomPropertyDeleted(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomPropertyModified(Qt3DSDMPropertyHandle inProp) = 0;

    virtual void SendCustomEventCreated(Qt3DSDMEventHandle inEvent,
                                        Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomEventDeleted(Qt3DSDMEventHandle inEvent,
                                        Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomEventModified(Qt3DSDMEventHandle inEvent) = 0;

    virtual void SendCustomHandlerCreated(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomHandlerDeleted(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomHandlerModified(Qt3DSDMHandlerHandle inHandler) = 0;

    virtual void SendCustomHandlerParamCreated(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamDeleted(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamModified(Qt3DSDMHandlerParamHandle inParameter) = 0;

    virtual void SendCustomReferencesModified(Qt3DSDMInstanceHandle inOwner,
                                              const TCharStr &inString) = 0;
};

class ISlideSystemSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr
    ConnectMasterCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectMasterDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideCreated(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideDeleted(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideRearranged(
        const std::function<void(Qt3DSDMSlideHandle, int, int)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyLinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyUnlinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectActiveSlide(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle, Qt3DSDMSlideHandle)>
            &inCallback) = 0;
};

class ISlideSystemSignalSender : public ISignalItem
{
public:
    virtual void SendMasterCreated(Qt3DSDMSlideHandle inMaster) = 0;
    virtual void SendMasterDeleted(Qt3DSDMSlideHandle inMaster) = 0;
    virtual void SendSlideCreated(Qt3DSDMSlideHandle inMaster, int inIndex,
                                  Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendSlideDeleted(Qt3DSDMSlideHandle inMaster, int inIndex,
                                  Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendSlideRearranged(Qt3DSDMSlideHandle inMaster, int inOldIndex,
                                     int inNewIndex) = 0;
    virtual void SendInstanceAssociated(Qt3DSDMSlideHandle inMaster,
                                        Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendInstanceDissociated(Qt3DSDMSlideHandle inMaster,
                                         Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendPropertyLinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendPropertyUnlinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendActiveSlide(Qt3DSDMSlideHandle inMaster, int inIndex,
                                 Qt3DSDMSlideHandle inOldSlide, Qt3DSDMSlideHandle inNewSlide) = 0;
};

class IStudioFullSystemSignalProvider : public ISignalItem
{
public:
    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectSlideRearranged(
        const std::function<void(Qt3DSDMSlideHandle, int, int)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectBeginComponentSeconds(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectComponentSeconds(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyLinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectPropertyUnlinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectActiveSlide(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr
    ConnectAnimationCreated(const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle,
                                                       Qt3DSDMPropertyHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectAnimationDeleted(const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle,
                                                       Qt3DSDMPropertyHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectKeyframeUpdated(const std::function<void(Qt3DSDMKeyframeHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(Qt3DSDMAnimationHandle, bool)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectActionCreated(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectActionDeleted(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectTriggerObjectSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectTargetObjectSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectEventSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectHandlerSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectHandlerArgumentValueSet(
        const std::function<void(Qt3DSDMHandlerArgHandle)> &inCallback) = 0;

    virtual TSignalConnectionPtr ConnectCustomPropertyCreated(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomPropertyDeleted(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomPropertyModified(
        const std::function<void(Qt3DSDMPropertyHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomEventCreated(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomEventDeleted(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectCustomEventModified(const std::function<void(Qt3DSDMEventHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerCreated(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerDeleted(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr
    ConnectCustomHandlerModified(const std::function<void(Qt3DSDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamCreated(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamDeleted(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomHandlerParamModified(
        const std::function<void(Qt3DSDMHandlerParamHandle)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectCustomReferencesModified(
        const std::function<void(Qt3DSDMInstanceHandle, const TCharStr &)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectControlledToggled(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> &inCallback) = 0;
};

class IStudioFullSystemSignalSender : public ISignalItem
{
public:
    virtual void SendSlideCreated(Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendSlideDeleted(Qt3DSDMSlideHandle inSlide) = 0;
    virtual void SendSlideRearranged(Qt3DSDMSlideHandle inMaster, int inOldIndex,
                                     int inNewIndex) = 0;
    virtual void SendComponentSeconds(Qt3DSDMSlideHandle inMaster) = 0;
    virtual void SendBeginComponentSeconds(Qt3DSDMSlideHandle inMaster) = 0;
    virtual void SendPropertyLinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendPropertyUnlinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendActiveSlide(Qt3DSDMSlideHandle inMaster, int inIndex,
                                 Qt3DSDMSlideHandle inSlide) = 0;

    virtual void SendInstanceCreated(Qt3DSDMInstanceHandle inInstance) = 0;
    virtual void SendInstanceDeleted(Qt3DSDMInstanceHandle inInstance) = 0;

    virtual void SendAnimationCreated(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendAnimationDeleted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void SendKeyframeInserted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMKeyframeHandle inKeyframe) = 0;
    virtual void SendKeyframeErased(Qt3DSDMAnimationHandle inAnimation,
                                    Qt3DSDMKeyframeHandle inKeyframe) = 0;
    virtual void SendKeyframeUpdated(Qt3DSDMKeyframeHandle inKeyframe) = 0;
    virtual void SendConnectFirstKeyframeDynamicSet(Qt3DSDMAnimationHandle inAnimation,
                                                    bool inDynamic) = 0;

    virtual void SendInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                           Qt3DSDMPropertyHandle inProperty) = 0;

    virtual void SendActionCreated(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendActionDeleted(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendTriggerObjectSet(Qt3DSDMActionHandle inAction) = 0;
    virtual void SendTargetObjectSet(Qt3DSDMActionHandle inAction) = 0;
    virtual void SendEventSet(Qt3DSDMActionHandle inAction) = 0;
    virtual void SendHandlerSet(Qt3DSDMActionHandle inAction) = 0;
    virtual void SendHandlerArgumentValueSet(Qt3DSDMHandlerArgHandle inHandlerArgument) = 0;

    virtual void SendCustomPropertyCreated(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomPropertyDeleted(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomPropertyModified(Qt3DSDMPropertyHandle inProp) = 0;
    virtual void SendCustomEventCreated(Qt3DSDMEventHandle inEvent,
                                        Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomEventDeleted(Qt3DSDMEventHandle inEvent,
                                        Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomEventModified(Qt3DSDMEventHandle inEvent) = 0;
    virtual void SendCustomHandlerCreated(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomHandlerDeleted(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) = 0;
    virtual void SendCustomHandlerModified(Qt3DSDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamCreated(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamDeleted(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) = 0;
    virtual void SendCustomHandlerParamModified(Qt3DSDMHandlerParamHandle inParameter) = 0;
    virtual void SendCustomReferencesModified(Qt3DSDMInstanceHandle inOwner,
                                              const TCharStr &inString) = 0;
    virtual void SendControlledToggled(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty) = 0;
};

// Use this if you want to register for only a specific instance or specific property
template <typename TTransaction>
inline void MaybackCallbackInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty,
                                                 Qt3DSDMInstanceHandle inDesiredInstance,
                                                 Qt3DSDMPropertyHandle inDesiredProperty,
                                                 TTransaction inCallback)
{
    if ((!inDesiredInstance.Valid() || (inDesiredInstance == inInstance))
        && (!inDesiredProperty.Valid() || (inDesiredProperty == inProperty)))
        inCallback(inInstance, inProperty);
}

void SetDataModelSignalsEnabled(bool inEnabled);
// Defaults to true
bool AreDataModelSignalsEnabled();
}

#endif
