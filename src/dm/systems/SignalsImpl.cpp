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
#ifdef WIN32
#pragma warning(disable : 4103 4512 4503)
#endif
#include "SignalsImpl.h"

using namespace std;

namespace {
bool g_DataModelSignalsEnabled = true;

#define CHECK_SIGNALS_ENABLED()                                                                    \
    {                                                                                              \
        if (g_DataModelSignalsEnabled == false)                                                        \
            return;                                                                                \
    }
}

namespace qt3dsdm {
void SetDataModelSignalsEnabled(bool inEnabled)
{
    g_DataModelSignalsEnabled = inEnabled;
}
// Defaults to true
bool AreDataModelSignalsEnabled()
{
    return g_DataModelSignalsEnabled;
}

#define CONNECT_QT(x) TSignalConnectionPtr(new QtSignalConnection(x))
#define CONNECT_SIGNAL_QT(x) CONNECT_QT(QObject::connect(this, x, inCallback))

class CPropertyCoreSignaller : public QObject, public IInstancePropertyCoreSignalProvider,
                               public IInstancePropertyCoreSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void propertySignal(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue);
public:
    TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, const SValue &)>
            &inCallback) override
    {
        return CONNECT_QT(QObject::connect(this, &CPropertyCoreSignaller::propertySignal,
                                           inCallback));
    }
    void SignalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT propertySignal(inInstance, inProperty, inValue);
    }
};

TSignalItemPtr CreatePropertyCoreSignaller()
{
    return TSignalItemPtr(
        static_cast<IInstancePropertyCoreSignalProvider *>(new CPropertyCoreSignaller()));
}

class CDataCoreSignaller : public QObject, public IDataCoreSignalProvider,
        public IDataCoreSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void instanceCreated(Qt3DSDMInstanceHandle handle);
    void beforeInstanceDeleted(Qt3DSDMInstanceHandle handle);
    void instanceDeleted(Qt3DSDMInstanceHandle handle);
    void instanceDerived(Qt3DSDMInstanceHandle handle, Qt3DSDMInstanceHandle handle2);
    void instanceParentRemoved(Qt3DSDMInstanceHandle handle, Qt3DSDMInstanceHandle handle2);
    void propertyAdded(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, TCharPtr,
                       DataModelDataType::Value);
    void propertyRemoved(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, TCharPtr,
                         DataModelDataType::Value);

public:
    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_QT(QObject::connect(this, &CDataCoreSignaller::instanceCreated,
                                           inCallback));
    }
    virtual TSignalConnectionPtr
    ConnectBeforeInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_QT(QObject::connect(this, &CDataCoreSignaller::beforeInstanceDeleted,
                                           inCallback));
    }
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_QT(QObject::connect(this, &CDataCoreSignaller::instanceDeleted,
                                           inCallback));
    }
    TSignalConnectionPtr ConnectInstanceDerived(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_QT(QObject::connect(this, &CDataCoreSignaller::instanceDerived,
                                           inCallback));
    }
    TSignalConnectionPtr ConnectInstanceParentRemoved(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_QT(QObject::connect(this, &CDataCoreSignaller::instanceParentRemoved,
                                           inCallback));
    }
    virtual TSignalConnectionPtr
    ConnectPropertyAdded(const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                    TCharPtr, DataModelDataType::Value)> &inCallback) override
    {
        return CONNECT_QT(QObject::connect(this, &CDataCoreSignaller::propertyAdded, inCallback));
    }
    virtual TSignalConnectionPtr
    ConnectPropertyRemoved(const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                      TCharPtr, DataModelDataType::Value)> &inCallback) override
    {
        return CONNECT_QT(QObject::connect(this, &CDataCoreSignaller::propertyRemoved,
                                           inCallback));
    }

    void SignalInstanceCreated(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceCreated(inInstance);
    }
    void SignalBeforeInstanceDeleted(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT beforeInstanceDeleted(inInstance);
    }
    void SignalInstanceDeleted(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceDeleted(inInstance);
    }
    void SignalInstanceDerived(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMInstanceHandle inParent) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceDerived(inInstance, inParent);
    }
    void SignalInstanceParentRemoved(Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMInstanceHandle inParent) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceParentRemoved(inInstance, inParent);
    }
    void SignalPropertyAdded(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty, TCharPtr inName,
                                     DataModelDataType::Value inDataType) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT propertyAdded(inInstance, inProperty, inName, inDataType);
    }
    void SignalPropertyRemoved(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, TCharPtr inName,
                                       DataModelDataType::Value inDataType) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT propertyRemoved(inInstance, inProperty, inName, inDataType);
    }
};

TSignalItemPtr CreateDataCoreSignaller()
{
    return TSignalItemPtr(static_cast<IDataCoreSignalProvider *>(new CDataCoreSignaller()));
}

class CSlideCoreSignaller : public QObject, public ISlideCoreSignalProvider,
        public ISlideCoreSignalSender
{
    Q_OBJECT
Q_SIGNALS:

    void slideCreated(Qt3DSDMSlideHandle);
    void beforeSlideDeleted(Qt3DSDMSlideHandle);
    void slideDeleted(Qt3DSDMSlideHandle);
    void slideDerived(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, int);
    void instancePropertyValueSet(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                  const SValue &);
    qt3dsdm::SValue instancePropertyValueAdded(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle,
                                             Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                  const SValue &);
    void instancePropertyValueRemoved(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                      Qt3DSDMPropertyHandle, const SValue &);
    void slideTimeChanged(Qt3DSDMSlideHandle);

public:
    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideCoreSignaller::slideCreated);
    }
    virtual TSignalConnectionPtr
    ConnectBeforeSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideCoreSignaller::beforeSlideDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideCoreSignaller::slideDeleted);
    }
    TSignalConnectionPtr ConnectSlideDerived(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, int)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideCoreSignaller::slideDerived);
    }
    TSignalConnectionPtr ConnectInstancePropertyValueSet(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideCoreSignaller::instancePropertyValueSet);
    }
    virtual TSignalConnectionPtr ConnectInstancePropertyValueAdded(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                     Qt3DSDMPropertyHandle, const SValue &)> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CSlideCoreSignaller::instancePropertyValueAdded);
    }
    TSignalConnectionPtr ConnectInstancePropertyValueRemoved(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideCoreSignaller::instancePropertyValueRemoved);
    }
    virtual TSignalConnectionPtr
    ConnectSlideTimeChanged(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideCoreSignaller::slideTimeChanged);
    }

    void SendSlideCreated(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideCreated(inSlide);
    }
    void SendBeforeSlideDeleted(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT beforeSlideDeleted(inSlide);
    }
    void SendSlideDeleted(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideDeleted(inSlide);
    }
    void SendSlideDerived(Qt3DSDMSlideHandle inSlide, Qt3DSDMSlideHandle inParent,
                                  int inIndex) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideDerived(inSlide, inParent, inIndex);
    }
    void SendPropertyValueSet(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instancePropertyValueSet(inSlide, inInstance, inProperty, inValue);
    }
    virtual SValue SendPropertyValueAdded(Qt3DSDMSlideHandle inSource, Qt3DSDMSlideHandle inDest,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, const SValue &inValue)
    {
        if (!g_DataModelSignalsEnabled) return SValue();

        // check that we have one and only one slot connected to
        // instancePropertyValueAdded signal as it returns a value.
        Q_ASSERT(
                receivers(
                    SIGNAL(
                        instancePropertyValueAdded(
                            Qt3DSDMSlideHandle, Qt3DSDMSlideHandle,
                            Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,const SValue &))) == 1);
        // emit instancePropertyValueAdded
        return instancePropertyValueAdded(inSource, inDest, inInstance, inProperty, inValue);
    }
    void SendPropertyValueRemoved(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instancePropertyValueRemoved(inSlide, inInstance, inProperty, inValue);
    }
    void SendSlideTimeChanged(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideTimeChanged(inSlide);
    }
};

TSignalItemPtr CreateSlideCoreSignaller()
{
    return TSignalItemPtr(static_cast<ISlideCoreSignalProvider *>(new CSlideCoreSignaller()));
}

class CSlideGraphCoreSignaller : public QObject, public ISlideGraphCoreSignalProvider,
                                 public ISlideGraphCoreSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void graphCreated(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle);
    void graphDeleted(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle);
    void instanceAssociated(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle);
    void instanceDissociated(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle);
    void graphActiveSlide(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle);
    void graphSeconds(Qt3DSDMSlideGraphHandle, float);

public:
    TSignalConnectionPtr ConnectGraphCreated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideGraphCoreSignaller::graphCreated);
    }
    TSignalConnectionPtr ConnectGraphDeleted(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideGraphCoreSignaller::graphDeleted);
    }
    TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideGraphCoreSignaller::instanceAssociated);
    }
    TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideGraphCoreSignaller::instanceDissociated);
    }
    TSignalConnectionPtr ConnectGraphActiveSlide(
        const std::function<void(Qt3DSDMSlideGraphHandle, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideGraphCoreSignaller::graphActiveSlide);
    }
    virtual TSignalConnectionPtr
    ConnectGraphSeconds(const std::function<void(Qt3DSDMSlideGraphHandle, float)> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CSlideGraphCoreSignaller::graphSeconds);
    }

    void SendGraphCreated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT graphCreated(inGraph, inSlide);
    }
    void SendGraphDeleted(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT graphDeleted(inGraph, inSlide);
    }
    void SendInstanceAssociated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide,
                                        Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceAssociated(inGraph, inSlide, inInstance);
    }
    void SendInstanceDissociated(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide,
                                         Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceDissociated(inGraph, inSlide, inInstance);
    }
    void SendGraphActiveSlide(Qt3DSDMSlideGraphHandle inGraph, Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT graphActiveSlide(inGraph, inSlide);
    }
    virtual void SendGraphSeconds(Qt3DSDMSlideGraphHandle inGraph, float inSeconds)
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT graphSeconds(inGraph, inSeconds);
    }
};

TSignalItemPtr CreateSlideGraphCoreSignaller()
{
    return TSignalItemPtr(
        static_cast<ISlideGraphCoreSignalProvider *>(new CSlideGraphCoreSignaller()));
}

class CAnimationCoreSignaller : public QObject, public IAnimationCoreSignalProvider,
                                public IAnimationCoreSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void animationCreated(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                Qt3DSDMPropertyHandle, size_t, EAnimationType);
    void beforeAnimationDeleted(Qt3DSDMAnimationHandle);
    void animationDeleted(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                Qt3DSDMPropertyHandle, size_t, EAnimationType);
    void keyframeInserted(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &);
    void beforeKeyframeErased(Qt3DSDMKeyframeHandle);
    void keyframeErased(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &);
    void beforeAllKeyframesErased(Qt3DSDMAnimationHandle);
    void keyframeUpdated(Qt3DSDMKeyframeHandle, const TKeyframe &);
    void firstKeyframeDynamic(Qt3DSDMAnimationHandle, bool);

public:
    TSignalConnectionPtr ConnectAnimationCreated(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::animationCreated);
    }
    virtual TSignalConnectionPtr
    ConnectBeforeAnimationDeleted(const std::function<void(Qt3DSDMAnimationHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::beforeAnimationDeleted);
    }
    TSignalConnectionPtr ConnectAnimationDeleted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                   Qt3DSDMPropertyHandle, size_t, EAnimationType)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::animationDeleted);
    }
    TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::keyframeInserted);
    }
    virtual TSignalConnectionPtr
    ConnectBeforeKeyframeErased(const std::function<void(Qt3DSDMKeyframeHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::beforeKeyframeErased);
    }
    TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle, const TKeyframe &)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::keyframeErased);
    }
    virtual TSignalConnectionPtr
    ConnectBeforeAllKeyframesErased(const std::function<void(Qt3DSDMAnimationHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::beforeAllKeyframesErased);
    }
    TSignalConnectionPtr ConnectKeyframeUpdated(
        const std::function<void(Qt3DSDMKeyframeHandle, const TKeyframe &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::keyframeUpdated);
    }
    TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(Qt3DSDMAnimationHandle, bool)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CAnimationCoreSignaller::firstKeyframeDynamic);
    }

    void SendAnimationCreated(Qt3DSDMAnimationHandle inAnimation, Qt3DSDMSlideHandle inSlide,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT animationCreated(inAnimation, inSlide, inInstance, inProperty, inIndex,
                                inAnimationType);
    }
    void SendBeforeAnimationDeleted(Qt3DSDMAnimationHandle inAnimation) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT beforeAnimationDeleted(inAnimation);
    }
    void SendAnimationDeleted(Qt3DSDMAnimationHandle inAnimation, Qt3DSDMSlideHandle inSlide,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, size_t inIndex,
                                      EAnimationType inAnimationType) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT animationDeleted(inAnimation, inSlide, inInstance, inProperty, inIndex,
                                inAnimationType);
    }
    void SendKeyframeInserted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT keyframeInserted(inAnimation, inKeyframe, inData);
    }
    void SendBeforeKeyframeErased(Qt3DSDMKeyframeHandle inKeyframe) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT beforeKeyframeErased(inKeyframe);
    }
    void SendKeyframeErased(Qt3DSDMAnimationHandle inAnimation,
                                    Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT keyframeErased(inAnimation, inKeyframe, inData);
    }
    void SendBeforeAllKeyframesErased(Qt3DSDMAnimationHandle inAnimation) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT beforeAllKeyframesErased(inAnimation);
    }
    void SendKeyframeUpdated(Qt3DSDMKeyframeHandle inKeyframe, const TKeyframe &inData) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT keyframeUpdated(inKeyframe, inData);
    }
    void SendFirstKeyframeDynamicSet(Qt3DSDMAnimationHandle inAnimation,
                                             bool inKeyframeDynamic) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT firstKeyframeDynamic(inAnimation, inKeyframeDynamic);
    }
};

TSignalItemPtr CreateAnimationCoreSignaller()
{
    return TSignalItemPtr(
        static_cast<IAnimationCoreSignalProvider *>(new CAnimationCoreSignaller()));
}

class CActionCoreSignaller : public QObject, public IActionCoreSignalProvider,
        public IActionCoreSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void triggerObjectSet(Qt3DSDMActionHandle, SObjectRefType &);
    void targetObjectSet(Qt3DSDMActionHandle, SObjectRefType &);
    void eventHandleSet(Qt3DSDMActionHandle, const wstring &);
    void handlerHandleSet(Qt3DSDMActionHandle, const wstring &);

    void handlerArgumentAdded(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                              HandlerArgumentType::Value, DataModelDataType::Value);
    void handlerArgumentRemoved(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                HandlerArgumentType::Value, DataModelDataType::Value);
    void handlerArgumentValueSet(Qt3DSDMHandlerArgHandle, const SValue &);

public:
    TSignalConnectionPtr ConnectTriggerObjectSet(
        const std::function<void(Qt3DSDMActionHandle, SObjectRefType &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionCoreSignaller::triggerObjectSet);
    }
    TSignalConnectionPtr ConnectTargetObjectSet(
        const std::function<void(Qt3DSDMActionHandle, SObjectRefType &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionCoreSignaller::targetObjectSet);
    }
    virtual TSignalConnectionPtr
    ConnectEventSet(const std::function<void(Qt3DSDMActionHandle, const wstring &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionCoreSignaller::eventHandleSet);
    }
    virtual TSignalConnectionPtr
    ConnectHandlerSet(const std::function<void(Qt3DSDMActionHandle, const wstring &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionCoreSignaller::handlerHandleSet);
    }

    TSignalConnectionPtr ConnectHandlerArgumentAdded(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionCoreSignaller::handlerArgumentAdded);
    }
    TSignalConnectionPtr ConnectHandlerArgumentRemoved(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMHandlerArgHandle, const TCharStr &,
                                   HandlerArgumentType::Value, DataModelDataType::Value)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionCoreSignaller::handlerArgumentRemoved);
    }
    TSignalConnectionPtr ConnectHandlerArgumentValueSet(
        const std::function<void(Qt3DSDMHandlerArgHandle, const SValue &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionCoreSignaller::handlerArgumentValueSet);
    }

    void SendTriggerObjectSet(Qt3DSDMActionHandle inAction, SObjectRefType &inTriggerObject) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT triggerObjectSet(inAction, inTriggerObject);
    }
    void SendTargetObjectSet(Qt3DSDMActionHandle inAction, SObjectRefType &inTargetObject) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT targetObjectSet(inAction, inTargetObject);
    }
    void SendEventSet(Qt3DSDMActionHandle inAction, const wstring &inEventHandle) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT eventHandleSet(inAction, inEventHandle);
    }
    void SendHandlerSet(Qt3DSDMActionHandle inAction, const wstring &inHandlerHandle) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT handlerHandleSet(inAction, inHandlerHandle);
    }

    void SendHandlerArgumentAdded(Qt3DSDMActionHandle inAction,
                                          Qt3DSDMHandlerArgHandle inHandlerArgument,
                                          const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                          DataModelDataType::Value inValueType) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT handlerArgumentAdded(inAction, inHandlerArgument, inName, inArgType, inValueType);
    }
    void SendHandlerArgumentRemoved(Qt3DSDMActionHandle inAction,
                                            Qt3DSDMHandlerArgHandle inHandlerArgument,
                                            const TCharStr &inName, HandlerArgumentType::Value inArgType,
                                            DataModelDataType::Value inValueType) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT handlerArgumentRemoved(inAction, inHandlerArgument, inName, inArgType, inValueType);
    }
    void SendHandlerArgumentValueSet(Qt3DSDMHandlerArgHandle inHandlerArgument,
                                             const SValue &inValue) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT handlerArgumentValueSet(inHandlerArgument, inValue);
    }
};

TSignalItemPtr CreateActionCoreSignaller()
{
    return TSignalItemPtr(static_cast<IActionCoreSignalProvider *>(new CActionCoreSignaller()));
}

class CActionSystemSignaller : public QObject, public IActionSystemSignalProvider,
        public IActionSystemSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void actionCreated(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle);
    void actionDeleted(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle);

public:
    TSignalConnectionPtr ConnectActionCreated(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionSystemSignaller::actionCreated);
    }
    TSignalConnectionPtr ConnectActionDeleted(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CActionSystemSignaller::actionDeleted);
    }

    void SendActionCreated(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT actionCreated(inAction, inSlide, inOwner);
    }
    void SendActionDeleted(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT actionDeleted(inAction, inSlide, inOwner);
    }
};

TSignalItemPtr CreateActionSystemSignaller()
{
    return TSignalItemPtr(static_cast<IActionSystemSignalProvider *>(new CActionSystemSignaller()));
}

class CSlideSystemSignaller : public QObject, public ISlideSystemSignalProvider,
        public ISlideSystemSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void masterCreated(Qt3DSDMSlideHandle);
    void masterDeleted(Qt3DSDMSlideHandle);
    void slideCreated(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle);
    void slideDeleted(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle);
    void slideRearranged(Qt3DSDMSlideHandle, int, int);
    void componentSeconds(Qt3DSDMSlideHandle, float);
    void instanceAssociated(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle);
    void instanceDissociated(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle);
    void propertyLinked(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle);
    void propertyUnlinked(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle);
    void activeSlide(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle, Qt3DSDMSlideHandle);

    qt3dsdm::SValue valueCreated(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                 const SValue);
    void valueDestroyed(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                        const SValue);

public:
    virtual TSignalConnectionPtr
    ConnectMasterCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::masterCreated);
    }
    virtual TSignalConnectionPtr
    ConnectMasterDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::masterDeleted);
    }
    TSignalConnectionPtr ConnectSlideCreated(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::slideCreated);
    }
    TSignalConnectionPtr ConnectSlideDeleted(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::slideDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectSlideRearranged(const std::function<void(Qt3DSDMSlideHandle, int, int)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::slideRearranged);
    }
    virtual TSignalConnectionPtr
    ConnectComponentSeconds(const std::function<void(Qt3DSDMSlideHandle, float)> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::componentSeconds);
    }
    TSignalConnectionPtr ConnectInstanceAssociated(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::instanceAssociated);
    }
    TSignalConnectionPtr ConnectInstanceDissociated(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::instanceDissociated);
    }
    TSignalConnectionPtr ConnectPropertyLinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::propertyLinked);
    }
    TSignalConnectionPtr ConnectPropertyUnlinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::propertyUnlinked);
    }
    TSignalConnectionPtr ConnectActiveSlide(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle, Qt3DSDMSlideHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::activeSlide);
    }
    virtual TSignalConnectionPtr ConnectPropertyValueCreated(
        const std::function<SValue(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                     const SValue &)> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::valueCreated);
    }
    virtual TSignalConnectionPtr ConnectPropertyValueDestroyed(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                   const SValue &)> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CSlideSystemSignaller::valueDestroyed);
    }

    void SendMasterCreated(Qt3DSDMSlideHandle inMaster) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT masterCreated(inMaster);
    }
    void SendMasterDeleted(Qt3DSDMSlideHandle inMaster) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT masterDeleted(inMaster);
    }
    void SendSlideCreated(Qt3DSDMSlideHandle inMaster, int inIndex,
                                  Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideCreated(inMaster, inIndex, inSlide);
    }
    void SendSlideDeleted(Qt3DSDMSlideHandle inMaster, int inIndex,
                                  Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideDeleted(inMaster, inIndex, inSlide);
    }
    void SendSlideRearranged(Qt3DSDMSlideHandle inMaster, int inOldIndex, int inNewIndex) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideRearranged(inMaster, inOldIndex, inNewIndex);
    }
    virtual void SendComponentSeconds(Qt3DSDMSlideHandle inMaster, float inSeconds)
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT componentSeconds(inMaster, inSeconds);
    }
    void SendInstanceAssociated(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceAssociated(inMaster, inInstance);
    }
    void SendInstanceDissociated(Qt3DSDMSlideHandle inMaster,
                                         Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceDissociated(inMaster, inInstance);
    }
    void SendPropertyLinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT propertyLinked(inMaster, inInstance, inProperty);
    }
    void SendPropertyUnlinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT propertyUnlinked(inMaster, inInstance, inProperty);
    }
    void SendActiveSlide(Qt3DSDMSlideHandle inMaster, int inIndex,
                                 Qt3DSDMSlideHandle inOldSlide, Qt3DSDMSlideHandle inNewSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT activeSlide(inMaster, inIndex, inOldSlide, inNewSlide);
    }

    virtual SValue SendPropertyValueCreated(Qt3DSDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance,
                                            Qt3DSDMPropertyHandle inProperty, const SValue &inValue)
    {
        if (!g_DataModelSignalsEnabled) return SValue();

        // check that we have one and only one slot connected to
        // valueCreated signal as it returns a value.
        Q_ASSERT(
                receivers(
                    SIGNAL(
                        valueCreated(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle,
                                     Qt3DSDMPropertyHandle, const SValue &))) == 1);
        // emit valueCreated
        return valueCreated(inSlide, inInstance, inProperty, inValue);
    }
    virtual void SendPropertyValueDestroyed(Qt3DSDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance,
                                            Qt3DSDMPropertyHandle inProperty, const SValue &inValue)
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT valueDestroyed(inSlide, inInstance, inProperty, inValue);
    }
};

TSignalItemPtr CreateSlideSystemSignaller()
{
    return TSignalItemPtr(static_cast<ISlideSystemSignalProvider *>(new CSlideSystemSignaller()));
}

class CCustomPropCoreSignaller : public QObject, public ICustomPropCoreSignalProvider,
                                 public ICustomPropCoreSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void customPropertyCreated(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle);
    void customPropertyDeleted(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle);
    void customPropertyModified(Qt3DSDMPropertyHandle);
    void customEventCreated(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle);
    void customEventDeleted(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle);
    void customEventModified(Qt3DSDMEventHandle);
    void customHandlerCreated(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle);
    void customHandlerDeleted(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle);
    void customHandlerModified(Qt3DSDMHandlerHandle);
    void customHandlerParamCreated(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle);
    void customHandlerParamDeleted(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle);
    void customHandlerParamModified(Qt3DSDMHandlerParamHandle);
    void customReferencesModified(Qt3DSDMInstanceHandle, const TCharStr &);

public:
    TSignalConnectionPtr ConnectCustomPropertyCreated(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customPropertyCreated);
    }
    TSignalConnectionPtr ConnectCustomPropertyDeleted(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customPropertyDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomPropertyModified(const std::function<void(Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customPropertyModified);
    }
    TSignalConnectionPtr ConnectCustomEventCreated(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customEventCreated);
    }
    TSignalConnectionPtr ConnectCustomEventDeleted(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customEventDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomEventModified(const std::function<void(Qt3DSDMEventHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customEventModified);
    }
    TSignalConnectionPtr ConnectCustomHandlerCreated(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customHandlerCreated);
    }
    TSignalConnectionPtr ConnectCustomHandlerDeleted(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customHandlerDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomHandlerModified(const std::function<void(Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customHandlerModified);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamCreated(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customHandlerParamCreated);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamDeleted(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customHandlerParamDeleted);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamModified(
        const std::function<void(Qt3DSDMHandlerParamHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customHandlerParamModified);
    }
    TSignalConnectionPtr ConnectCustomReferencesModified(
        const std::function<void(Qt3DSDMInstanceHandle, const TCharStr &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CCustomPropCoreSignaller::customReferencesModified);
    }

    void SendCustomPropertyCreated(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customPropertyCreated(inProp, inOwner);
    }
    void SendCustomPropertyDeleted(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customPropertyDeleted(inProp, inOwner);
    }
    void SendCustomPropertyModified(Qt3DSDMPropertyHandle inProp) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customPropertyModified(inProp);
    }
    void SendCustomEventCreated(Qt3DSDMEventHandle inEvent, Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customEventCreated(inEvent, inOwner);
    }
    void SendCustomEventDeleted(Qt3DSDMEventHandle inEvent, Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customEventDeleted(inEvent, inOwner);
    }
    void SendCustomEventModified(Qt3DSDMEventHandle inEvent) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customEventModified(inEvent);
    }
    void SendCustomHandlerCreated(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerCreated(inHandler, inOwner);
    }
    void SendCustomHandlerDeleted(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerDeleted(inHandler, inOwner);
    }
    void SendCustomHandlerModified(Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerModified(inHandler);
    }
    void SendCustomHandlerParamCreated(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerParamCreated(inParameter, inHandler);
    }
    void SendCustomHandlerParamDeleted(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerParamDeleted(inParameter, inHandler);
    }
    void SendCustomHandlerParamModified(Qt3DSDMHandlerParamHandle inParameter) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerParamModified(inParameter);
    }
    void SendCustomReferencesModified(Qt3DSDMInstanceHandle inOwner,
                                              const TCharStr &inString) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customReferencesModified(inOwner, inString);
    }
};

TSignalItemPtr CreateCustomPropCoreSignaller()
{
    return TSignalItemPtr(
        static_cast<ICustomPropCoreSignalProvider *>(new CCustomPropCoreSignaller()));
}

class CStudioFullSystemSignaller : public QObject, public IStudioFullSystemSignalProvider,
                                   public IStudioFullSystemSignalSender
{
    Q_OBJECT
Q_SIGNALS:
    void changeSetBegin();
    void changeSetEnd();
    void animationBegin();
    void animationEnd();
    void slideCreated(Qt3DSDMSlideHandle);
    void slideDeleted(Qt3DSDMSlideHandle);
    void slideRearranged(Qt3DSDMSlideHandle, int, int);
    void componentSeconds(Qt3DSDMSlideHandle);
    void beginComponentSeconds(Qt3DSDMSlideHandle);
    void propertyLinked(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle);
    void propertyUnlinked(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle);
    void activeSlide(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle);
    void instanceCreated(Qt3DSDMInstanceHandle);
    void instanceDeleted(Qt3DSDMInstanceHandle);
    void animationCreated(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle);
    void animationDeleted(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle);
    void controlledToggled(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle);
    void keyframeInserted(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle);
    void keyframeErased(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle);
    void keyframeUpdated(Qt3DSDMKeyframeHandle);
    void connectFirstKeyframeDynamicSet(Qt3DSDMAnimationHandle, bool);
    void instancePropertyValue(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle);
    void actionCreated(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle);
    void actionDeleted(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle);
    void triggerObjectSet(Qt3DSDMActionHandle);
    void targetObjectSet(Qt3DSDMActionHandle);
    void eventHandleSet(Qt3DSDMActionHandle);
    void handlerHandleSet(Qt3DSDMActionHandle);
    void handlerArgumentValueSet(Qt3DSDMHandlerArgHandle);
    void customPropertyCreated(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle);
    void customPropertyDeleted(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle);
    void customPropertyModified(Qt3DSDMPropertyHandle);
    void customEventCreated(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle);
    void customEventDeleted(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle);
    void customEventModified(Qt3DSDMEventHandle);
    void customHandlerCreated(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle);
    void customHandlerDeleted(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle);
    void customHandlerModified(Qt3DSDMHandlerHandle);
    void customHandlerParamCreated(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle);
    void customHandlerParamDeleted(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle);
    void customHandlerParamModified(Qt3DSDMHandlerParamHandle);
    void customReferencesModified(Qt3DSDMInstanceHandle, const TCharStr &);
private:
    ISlideSystemSignalProvider *m_SlideSystemSignalProvider;

public:
    CStudioFullSystemSignaller(ISlideSystemSignalProvider *inProvider)
        : m_SlideSystemSignalProvider(inProvider)
    {
    }

    virtual TSignalConnectionPtr ConnectChangeSetBegin(const std::function<void()> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::changeSetBegin);
    }
    virtual TSignalConnectionPtr ConnectChangeSetEnd(const std::function<void()> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::changeSetEnd);
    }

    // Used when people start to set component times.
    virtual TSignalConnectionPtr ConnectAnimationSetBegin(const std::function<void()> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::animationBegin);
    }
    virtual TSignalConnectionPtr ConnectAnimationSetEnd(const std::function<void()> &inCallback)
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::animationEnd);
    }

    virtual TSignalConnectionPtr
    ConnectSlideCreated(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::slideCreated);
    }
    virtual TSignalConnectionPtr
    ConnectSlideDeleted(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::slideDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectSlideRearranged(const std::function<void(Qt3DSDMSlideHandle, int, int)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::slideRearranged);
    }
    virtual TSignalConnectionPtr
    ConnectBeginComponentSeconds(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::beginComponentSeconds);
    }
    virtual TSignalConnectionPtr
    ConnectComponentSeconds(const std::function<void(Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::componentSeconds);
    }
    TSignalConnectionPtr ConnectPropertyLinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::propertyLinked);
    }
    TSignalConnectionPtr ConnectPropertyUnlinked(
        const std::function<void(Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::propertyUnlinked);
    }
    TSignalConnectionPtr ConnectActiveSlide(
        const std::function<void(Qt3DSDMSlideHandle, int, Qt3DSDMSlideHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::activeSlide);
    }

    virtual TSignalConnectionPtr
    ConnectInstanceCreated(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::instanceCreated);
    }
    virtual TSignalConnectionPtr
    ConnectInstanceDeleted(const std::function<void(Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::instanceDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectAnimationCreated(const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle,
                                                       Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::animationCreated);
    }
    virtual TSignalConnectionPtr
    ConnectAnimationDeleted(const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMInstanceHandle,
                                                       Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::animationDeleted);
    }
    TSignalConnectionPtr ConnectKeyframeInserted(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::keyframeInserted);
    }
    TSignalConnectionPtr ConnectKeyframeErased(
        const std::function<void(Qt3DSDMAnimationHandle, Qt3DSDMKeyframeHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::keyframeErased);
    }
    virtual TSignalConnectionPtr
    ConnectKeyframeUpdated(const std::function<void(Qt3DSDMKeyframeHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::keyframeUpdated);
    }
    TSignalConnectionPtr ConnectInstancePropertyValue(
        const std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::instancePropertyValue);
    }
    TSignalConnectionPtr ConnectFirstKeyframeDynamicSet(
        const std::function<void(Qt3DSDMAnimationHandle, bool)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::connectFirstKeyframeDynamicSet);
    }

    TSignalConnectionPtr ConnectActionCreated(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::actionCreated);
    }
    TSignalConnectionPtr ConnectActionDeleted(
        const std::function<void(Qt3DSDMActionHandle, Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle)>
            &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::actionDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectTriggerObjectSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::triggerObjectSet);
    }
    virtual TSignalConnectionPtr
    ConnectTargetObjectSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::targetObjectSet);
    }
    virtual TSignalConnectionPtr
    ConnectEventSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::eventHandleSet);
    }
    virtual TSignalConnectionPtr
    ConnectHandlerSet(const std::function<void(Qt3DSDMActionHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::handlerHandleSet);
    }
    virtual TSignalConnectionPtr
    ConnectHandlerArgumentValueSet(const std::function<void(Qt3DSDMHandlerArgHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::handlerArgumentValueSet);
    }

    TSignalConnectionPtr ConnectCustomPropertyCreated(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customPropertyCreated);
    }
    TSignalConnectionPtr ConnectCustomPropertyDeleted(
        const std::function<void(Qt3DSDMPropertyHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customPropertyDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomPropertyModified(const std::function<void(Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customPropertyModified);
    }
    TSignalConnectionPtr ConnectCustomEventCreated(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customEventCreated);
    }
    TSignalConnectionPtr ConnectCustomEventDeleted(
        const std::function<void(Qt3DSDMEventHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customEventDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomEventModified(const std::function<void(Qt3DSDMEventHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customEventModified);
    }
    TSignalConnectionPtr ConnectCustomHandlerCreated(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customHandlerCreated);
    }
    TSignalConnectionPtr ConnectCustomHandlerDeleted(
        const std::function<void(Qt3DSDMHandlerHandle, Qt3DSDMInstanceHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customHandlerDeleted);
    }
    virtual TSignalConnectionPtr
    ConnectCustomHandlerModified(const std::function<void(Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customHandlerModified);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamCreated(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customHandlerParamCreated);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamDeleted(
        const std::function<void(Qt3DSDMHandlerParamHandle, Qt3DSDMHandlerHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customHandlerParamDeleted);
    }
    TSignalConnectionPtr ConnectCustomHandlerParamModified(
        const std::function<void(Qt3DSDMHandlerParamHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customHandlerParamModified);
    }
    TSignalConnectionPtr ConnectCustomReferencesModified(
        const std::function<void(Qt3DSDMInstanceHandle, const TCharStr &)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::customReferencesModified);
    }
    TSignalConnectionPtr ConnectControlledToggled(
        const std::function<void(
            Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> &inCallback) override
    {
        return CONNECT_SIGNAL_QT(&CStudioFullSystemSignaller::controlledToggled);
    }

    virtual void SendChangeSetBegin()
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT changeSetBegin();
    }
    virtual void SendChangeSetEnd()
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT changeSetEnd();
    }

    virtual void SendAnimationSetBegin()
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT animationBegin();
    }
    virtual void SendAnimationSetEnd()
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT animationEnd();
    }

    void SendSlideCreated(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideCreated(inSlide);
    }
    void SendSlideDeleted(Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideDeleted(inSlide);
    }
    void SendSlideRearranged(Qt3DSDMSlideHandle inMaster, int inOldIndex, int inNewIndex) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT slideRearranged(inMaster, inOldIndex, inNewIndex);
    }
    void SendBeginComponentSeconds(Qt3DSDMSlideHandle inMaster) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT beginComponentSeconds(inMaster);
    }
    void SendComponentSeconds(Qt3DSDMSlideHandle inMaster) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT componentSeconds(inMaster);
    }
    void SendPropertyLinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT propertyLinked(inMaster, inInstance, inProperty);
    }
    void SendPropertyUnlinked(Qt3DSDMSlideHandle inMaster, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT propertyUnlinked(inMaster, inInstance, inProperty);
    }
    void SendActiveSlide(Qt3DSDMSlideHandle inMaster, int inIndex, Qt3DSDMSlideHandle inSlide) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT activeSlide(inMaster, inIndex, inSlide);
    }

    void SendInstanceCreated(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceCreated(inInstance);
    }
    void SendInstanceDeleted(Qt3DSDMInstanceHandle inInstance) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instanceDeleted(inInstance);
    }

    void SendAnimationCreated(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT animationCreated(inAnimation, inInstance, inProperty);
    }
    void SendAnimationDeleted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT animationDeleted(inAnimation, inInstance, inProperty);
    }
    void SendKeyframeInserted(Qt3DSDMAnimationHandle inAnimation,
                                      Qt3DSDMKeyframeHandle inKeyframe) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT keyframeInserted(inAnimation, inKeyframe);
    }
    void SendKeyframeErased(Qt3DSDMAnimationHandle inAnimation,
                                    Qt3DSDMKeyframeHandle inKeyframe) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT keyframeErased(inAnimation, inKeyframe);
    }
    void SendKeyframeUpdated(Qt3DSDMKeyframeHandle inKeyframe) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT keyframeUpdated(inKeyframe);
    }
    void SendConnectFirstKeyframeDynamicSet(Qt3DSDMAnimationHandle inAnimation,
                                                    bool inDynamic) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT connectFirstKeyframeDynamicSet(inAnimation, inDynamic);
    }

    void SendInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                           Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT instancePropertyValue(inInstance, inProperty);
    }

    void SendActionCreated(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT actionCreated(inAction, inSlide, inOwner);
    }
    void SendActionDeleted(Qt3DSDMActionHandle inAction, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT actionDeleted(inAction, inSlide, inOwner);
    }
    void SendTriggerObjectSet(Qt3DSDMActionHandle inAction) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT triggerObjectSet(inAction);
    }
    void SendTargetObjectSet(Qt3DSDMActionHandle inAction) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT targetObjectSet(inAction);
    }
    void SendEventSet(Qt3DSDMActionHandle inAction) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT eventHandleSet(inAction);
    }
    void SendHandlerSet(Qt3DSDMActionHandle inAction) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT handlerHandleSet(inAction);
    }
    void SendHandlerArgumentValueSet(Qt3DSDMHandlerArgHandle inHandlerArgument) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT handlerArgumentValueSet(inHandlerArgument);
    }

    void SendCustomPropertyCreated(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customPropertyCreated(inProp, inOwner);
    }
    void SendCustomPropertyDeleted(Qt3DSDMPropertyHandle inProp,
                                           Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customPropertyDeleted(inProp, inOwner);
    }
    void SendCustomPropertyModified(Qt3DSDMPropertyHandle inProp) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customPropertyModified(inProp);
    }
    void SendCustomEventCreated(Qt3DSDMEventHandle inEvent, Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customEventCreated(inEvent, inOwner);
    }
    void SendCustomEventDeleted(Qt3DSDMEventHandle inEvent, Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customEventDeleted(inEvent, inOwner);
    }
    void SendCustomEventModified(Qt3DSDMEventHandle inEvent) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customEventModified(inEvent);
    }
    void SendCustomHandlerCreated(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerCreated(inHandler, inOwner);
    }
    void SendCustomHandlerDeleted(Qt3DSDMHandlerHandle inHandler,
                                          Qt3DSDMInstanceHandle inOwner) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerDeleted(inHandler, inOwner);
    }
    void SendCustomHandlerModified(Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerModified(inHandler);
    }
    void SendCustomHandlerParamCreated(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerParamCreated(inParameter, inHandler);
    }
    void SendCustomHandlerParamDeleted(Qt3DSDMHandlerParamHandle inParameter,
                                               Qt3DSDMHandlerHandle inHandler) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerParamDeleted(inParameter, inHandler);
    }
    void SendCustomHandlerParamModified(Qt3DSDMHandlerParamHandle inParameter) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customHandlerParamModified(inParameter);
    }
    void SendCustomReferencesModified(Qt3DSDMInstanceHandle inOwner,
                                              const TCharStr &inString) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT customReferencesModified(inOwner, inString);
    }

    void SendControlledToggled(Qt3DSDMInstanceHandle inInstance,
                               Qt3DSDMPropertyHandle inProperty) override
    {
        CHECK_SIGNALS_ENABLED();
        Q_EMIT controlledToggled(inInstance, inProperty);
    }
};

TSignalItemPtr
CreateStudioFullSystemSignaller(ISlideSystemSignalProvider *inSlideSystemSignalProvider)
{
    return TSignalItemPtr(static_cast<IStudioFullSystemSignalProvider *>(
        new CStudioFullSystemSignaller(inSlideSystemSignalProvider)));
}
}

#include "SignalsImpl.moc"
