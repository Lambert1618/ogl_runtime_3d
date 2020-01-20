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

#include "RuntimePrefix.h"

//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSQmlEngine.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSInputEngine.h"
#include "Qt3DSSceneManager.h"
#include "Qt3DSVector3.h"
#include "Qt3DSColor.h"
#include "Qt3DSAttributeHashes.h"
#include "Qt3DSFileStream.h"
#include "Qt3DSDataLogger.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSSystem.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSSlideSystem.h"
#include "Qt3DSRenderModel.h"

#include "EASTL/vector.h"
#include "EASTL/list.h"
#include <sys/stat.h>
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSApplication.h"
#include "Qt3DSRuntimeFactory.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "Qt3DSRenderThreadPool.h"
#include "Qt3DSRenderImageBatchLoader.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSAudioPlayer.h"
#include "Qt3DSActivationManager.h"
#include "Qt3DSParametersSystem.h"
#include "Qt3DSQmlElementHelper.h"
#include "q3dsqmlscript.h"
#include "Qt3DSRenderRuntimeBindingImpl.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSImportMesh.h"
#include "Qt3DSRenderer.h"
#include "q3dsmaterialdefinitionparser.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "Qt3DSRenderMaterialHelpers.h"
#include "Qt3DSRenderUIPLoader.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSRenderUIPSharedTranslation.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qjsengine.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qfileinfo.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

#if defined(_DEBUG) || (_PROFILE)
#define Q3DStudio_LOG_EVENT(S) // something
#else
#define Q3DStudio_LOG_EVENT(S)
#endif

using qt3ds::runtime::IApplication;
using namespace qt3ds;

namespace __SQmlEngineImpl_Basic_Structs__ {
    struct QmlScopedLock
    {
        qt3ds::foundation::Mutex *m_Mutex;

        QmlScopedLock(qt3ds::foundation::Mutex *mtx)
            : m_Mutex(mtx)
        {
            if (m_Mutex)
                m_Mutex->lock();
        }

        ~QmlScopedLock()
        {
            if (m_Mutex)
                m_Mutex->unlock();
        }
    };
}

using namespace __SQmlEngineImpl_Basic_Structs__;
#define QML_ENGINE_MULTITHREAD_PROTECT_METHOD QmlScopedLock __locker(m_MultithreadedMutex);

//==============================================================================
// Callback handling
/**
*	Constructor
*/
CScriptCallbacks::CScriptCallbacks()
    : m_CallbackList(0, 0, "CallbackList")
{
}

/**
*	Destructor
*/
CScriptCallbacks::~CScriptCallbacks()
{
    FOR_ARRAY(SFrameCallbackEntry *, theEntry, m_CallbackList)
    Q3DStudio_delete(*theEntry, SFrameCallbackEntry);

    m_CallbackList.Clear();
}

bool CScriptCallbacks::RegisterCallback(Q3DStudio::UINT32 callbackType,
                                        const TScriptCallback inCallback, void *inUserData)
{
    // currently we only support these two types of callbacks
    if (callbackType != SCRIPT_ON_INITIALIZE && callbackType != SCRIPT_ON_UPDATE) {
        return false;
    }

    SFrameCallbackEntry *theFrameCallbackEntry = NULL;

    try {
        // note we don't care if someone registers the same function twice.
        theFrameCallbackEntry = Q3DStudio_new(SFrameCallbackEntry) SFrameCallbackEntry();

        if (!theFrameCallbackEntry) {
            throw "failed to allocated SFrameCallbackEntry";
        }

        SScriptCallbackEntry *theScriptCallbackEntry =
            Q3DStudio_new(SScriptCallbackEntry) SScriptCallbackEntry();
        if (!theScriptCallbackEntry) {
            throw "failed to allocated SScriptCallbackEntry";
        }

        theScriptCallbackEntry->m_CallbackType = callbackType;
        theScriptCallbackEntry->m_Function = inCallback;
        theScriptCallbackEntry->m_UserData = inUserData;
        theScriptCallbackEntry->m_Processed = false;

        theFrameCallbackEntry->m_Callbacks.Push(theScriptCallbackEntry);

        m_CallbackList.Push(theFrameCallbackEntry);

    } catch (const char *) {
        if (theFrameCallbackEntry)
            Q3DStudio_delete(theFrameCallbackEntry, SFrameCallbackEntry);

        return false;
    }

    return true;
}

void CScriptCallbacks::UnregisterCallback(Q3DStudio::UINT32, const TScriptCallback)
{
    // not used so far
}

void CScriptCallbacks::ProcessCallbacks()
{
    // Call onInitialize function
    FOR_ARRAY(SFrameCallbackEntry *, theFrameEntry, m_CallbackList)
    {
        FOR_ARRAY(SScriptCallbackEntry *, theEntry, (*theFrameEntry)->m_Callbacks)
        {
            if (((*theEntry)->m_CallbackType == SCRIPT_ON_INITIALIZE) && !(*theEntry)->m_Processed
                && (*theEntry)->m_Function) {
                (*theEntry)->m_Function((*theEntry)->m_UserData);
                (*theEntry)->m_Processed = true;
            }
        }
    }

    // Call onUpdate functions
    FOR_ARRAY(SFrameCallbackEntry *, theFrameEntry, m_CallbackList)
    {
        FOR_ARRAY(SScriptCallbackEntry *, theEntry, (*theFrameEntry)->m_Callbacks)
        {
            // int type = (*theEntry)->m_CallbackType;
            if ((*theEntry)->m_CallbackType == SCRIPT_ON_UPDATE && (*theEntry)->m_Function) {
                (*theEntry)->m_Function((*theEntry)->m_UserData);
            }
        }
    }
}

//==============================================================================
//*	End of helper class handling element operations
//==============================================================================

//==============================================================================
/**
*	Helper class handling command operations
*/
class CQmlCommandHelper
{
    //==============================================================================
    //	Methods
    //==============================================================================
private: // Constructors
    CQmlCommandHelper();
    virtual ~CQmlCommandHelper();

public: // static Functions
    static bool SetupGotoSlideCommand(TElement &inElement, const char *slideName,
                                      const SScriptEngineGotoSlideArgs &inArgs);
    static bool SetupGotoSlideCommand(TElement &inElement, Q3DStudio::INT32 inSlideIndex,
                                      const SScriptEngineGotoSlideArgs &inArgs);

protected: // Static Hidden Helpers
    // static TElement*	FireCommand(TEventCommandHash inCommand);

public: // Static Helpers
    static TElement *GetComponentParent(TElement *inParent);
};

//==============================================================================
/**
*	Constructor
*/
CQmlCommandHelper::CQmlCommandHelper()
{
}

//==============================================================================
/**
*	Destructor
*/
CQmlCommandHelper::~CQmlCommandHelper()
{
}

static Option<Q3DStudio::UINT8> FindSlide(TElement &inElement, const char *slideName)
{
    IPresentation *thePresentation = inElement.GetBelongedPresentation();

    int theSlideHashName = thePresentation->GetApplication().HashString(slideName);
    TComponent *theComponent = thePresentation->GetComponentManager().GetComponent(&inElement);
    UINT8 theSlideIndex =
        thePresentation->GetSlideSystem().FindSlide(*theComponent, theSlideHashName);
    if (theSlideIndex >= theComponent->GetSlideCount()) {
        qt3ds::foundation::CRegisteredString elemPath = thePresentation->GetElementPath(inElement);
        if (elemPath.IsValid()) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "QMLCommandHelper: goToSlide: Unable to find slide "
                    << slideName <<"on element " << elemPath.c_str();
        } else {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "QMLCommandHelper: goToSlide: Unable to find slide "
                    << slideName;
        }
        return Empty();
    }

    return theSlideIndex;
}

TElement *CQmlCommandHelper::GetComponentParent(TElement *inElement)
{
    Q3DStudio_ASSERT(inElement);
    return &inElement->GetComponentParent();
}

bool CQmlCommandHelper::SetupGotoSlideCommand(TElement &inElement, Q3DStudio::INT32 inSlideIndex,
                                              const SScriptEngineGotoSlideArgs &inArgs)
{
    IPresentation *thePresentation = inElement.GetBelongedPresentation();
    TElement *theTarget = GetComponentParent(&inElement);
    SComponentGotoSlideData theSlideData(inSlideIndex);
    theSlideData.m_Mode = inArgs.m_Mode;
    theSlideData.m_Paused = inArgs.m_Paused;
    theSlideData.m_Rate = inArgs.m_Rate;
    theSlideData.m_Reverse = inArgs.m_Reverse;
    theSlideData.m_StartTime = inArgs.m_StartTime;
    // Resolve playthroughto if it has a valid value.
    if (!isTrivial(inArgs.m_PlaythroughTo)) {
        if (AreEqual(inArgs.m_PlaythroughTo, "next"))
            theSlideData.m_PlaythroughTo = -1;
        else if (AreEqual(inArgs.m_PlaythroughTo, "previous"))
            theSlideData.m_PlaythroughTo = -2;
        else {
            // Find the slide if possible.  If not, then just error leave things as they are.

            Option<UINT8> theSlideIndex = FindSlide(inElement, inArgs.m_PlaythroughTo);
            if (theSlideIndex.hasValue())
                theSlideData.m_PlaythroughTo = *theSlideIndex;
        }
    }
    thePresentation->GetComponentManager().SetupComponentGotoSlideCommand(theTarget, theSlideData);
    UVariant theArg1;
    UVariant theArg2;
    qt3ds::intrinsics::memZero(&theArg1, sizeof(UVariant));
    qt3ds::intrinsics::memZero(&theArg2, sizeof(UVariant));
    thePresentation->FireCommand(COMMAND_GOTOSLIDE, theTarget, &theArg1, &theArg2);
    return true;
}

bool CQmlCommandHelper::SetupGotoSlideCommand(TElement &inElement, const char *slideName,
                                              const SScriptEngineGotoSlideArgs &inArgs)
{
    Option<UINT8> theSlideIndex = FindSlide(inElement, slideName);
    if (theSlideIndex.hasValue())
        return SetupGotoSlideCommand(inElement, *theSlideIndex, inArgs);

    return false;
}

//==============================================================================
//*	End of helper class handling command operations
//==============================================================================

struct SEmitSignalData : public NVRefCounted
{
    NVAllocatorCallback &m_Alloc;
    volatile QT3DSI32 mRefCount;
    INT32 m_SignalNameHandler; ///< The name of the signal to emit
    TElement *m_TargetElement; ///< The source element where the event is going to occur

    SEmitSignalData(NVAllocatorCallback &alloc, INT32 inSignalName, TElement *inElement)
        : m_Alloc(alloc)
        , mRefCount(0)
        , m_SignalNameHandler(inSignalName)
        , m_TargetElement(inElement)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Alloc)
};

class CQmlEngineImpl : public CQmlEngine
{
    using TEmitSignalPtr = NVScopedRefCounted<SEmitSignalData>;
    using TEmitSignalQueue = eastl::list<TEmitSignalPtr, ForwardingAllocator>;
    using TPropertyDescAndValueList = eastl::vector<qt3ds::runtime::element::TPropertyDescAndValue>;
    using TPropertyDesc = qt3ds::runtime::element::SPropertyDesc;

    static const int MAX_ACTION_QUEUE_SIZE = 100;

private:
    qt3ds::NVFoundationBase &m_Foundation;
    IApplication *m_Application;
    IApplication *m_ApplicationCore;
    qt3ds::foundation::Mutex m_PreloadMutex;
    qt3ds::foundation::Mutex *m_MultithreadedMutex;

    TEmitSignalQueue m_EmitSignalDataList;

    QT3DSI32 mRefCount;

    CScriptCallbacks m_ScriptCallbacks;

    QScopedPointer<QQmlEngine> m_engine;
    QMap<QString, QQmlComponent *> m_components;
    QVector<Q3DSQmlScript *> m_scripts;

    QSet<int> m_availableIds;
    QHash<TElement *, int> m_elementIdMap;
    QVector<QPair<TElement *, QString>> m_deferredScriptLoads;

public:
    CQmlEngineImpl(NVFoundationBase &fnd, ITimeProvider &inTimeProvider);

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    // functions from IScriptBridge
    void EnableMultithreadedAccess() override;
    void DisableMultithreadedAccess() override;

    void SetApplicationCore(qt3ds::runtime::IApplication &inApplication) override;
    void SetApplication(qt3ds::runtime::IApplication &inApplication) override;
    qt3ds::runtime::IApplication *GetApplication() override;
    void Initialize() override;

    void loadScript(TElement *element, const QString &sourcePath);
    void LoadScript(IPresentation *presentation, TElement *element, const CHAR *path,
                    bool initInRenderThread) override;
    void loadDeferredScripts() override;
    Q3DStudio::INT32 InitializeApplicationBehavior(const char *) override
    {
        return -1;
    }

    void ProcessFrameCallbacks(IPresentation *) override;
    void ExecuteApplicationScriptFunction(Q3DStudio::INT32, const char *) override {}
    void CallFunction(const char *, const char *,
                              CScriptEngineCallFunctionArgRetriever &) override {}

    void ProcessSignal(IPresentation *inPresentation,
                       const SEventCommand &inCommand) override;
    void ProcessCustomActions(IPresentation *inPresentation,
                                      const SEventCommand &inCommand) override;
    void ProcessCustomCallback(IPresentation *, const SEventCommand &) override {}

    void SetTableForElement(TElement &, IScriptTableProvider &) override {}
    void SetAttribute(TElement *target, const QString &attName, const char *value) override;
    void SetAttribute(const QString &element, const QString &attName, const char *value) override;
    bool GetAttribute(TElement *target, const char *attName, char *value) override;
    bool GetAttribute(const char *element, const char *attName, char *value) override;
    void FireEvent(const char *element, const char *evtName) override;
    void SetDataInputValue(const QString &name, const QVariant &value,
                           qt3ds::runtime::DataInputValueRole valueRole) override;
    void createElements(const QString &parentElementPath, const QString &slideName,
                        const QVector<QHash<QString, QVariant>> &properties,
                        qt3ds::render::IQt3DSRenderer *renderer) override;
    void deleteElements(const QStringList &elementPaths,
                        qt3ds::render::IQt3DSRenderer *renderer) override;
    void createMaterials(const QString &subPresId, const QStringList &materialDefinitions,
                         qt3ds::render::ICustomMaterialSystem *customMaterialSystem,
                         IDynamicObjectSystem *dynamicObjectSystem,
                         qt3ds::render::IQt3DSRenderer *renderer) override;
    void deleteMaterials(const QStringList &materialNames,
                         qt3ds::render::IQt3DSRenderer *renderer) override;
    void createMesh(const QString &name, qt3dsimp::Mesh *mesh,
                    qt3ds::render::IBufferManager *bufferManager) override;
    void deleteMeshes(const QStringList &meshNames,
                      qt3ds::render::IBufferManager *bufferManager) override;
    uint textureId(const QString &elementPath,
                   qt3ds::render::IQt3DSRenderer *renderer) override;

    void GotoSlide(const char *component, const char *slideName,
                           const SScriptEngineGotoSlideArgs &inArgs) override;
    void GotoSlideRelative(const char *, bool, bool, const SScriptEngineGotoSlideArgs &) override;

    void SetPresentationAttribute(const char *, const char *, const char *) override;

    // No need to implement here, as sound playing is done in Qt3DSViewerApp
    bool PlaySoundFile(const char *) override { return false; }

    void EnableDebugging(qt3ds::state::debugger::IMultiProtocolSocket &) override {}
    void EnableProfiling() override {}
    void StepGC() override {}

    // functions from CQMLEngine
    bool PeekSignal(TElement *&outElement, char *&outName) override;
    void GotoSlideIndex(const char *component, const Q3DStudio::INT32 slideIndex,
                                const SScriptEngineGotoSlideArgs &inArgs) override;
    bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                      QString &currentName, QString &previousName) override;
    void GotoTime(const char *component, const Q3DStudio::FLOAT time) override;
    bool RegisterCallback(Q3DStudio::UINT32 callbackType, const TScriptCallback inCallback,
                                  void *inUserData) override;
    void Shutdown(qt3ds::NVFoundationBase &inFoundation) override;

private:
    TElement *createMaterialContainer(TElement *parent, CPresentation *presentation);
    void createComponent(QQmlComponent *component, TElement *element);
    TElement *getTarget(const QString &component);
    void listAllElements(TElement *root, QList<TElement *> &elements);
    void initializeDataInputsInPresentation(CPresentation &presentation, bool isPrimary,
                                            bool isDynamicAdd = false,
                                            QList<TElement *> inElements = QList<TElement *>());
    void initializeDataOutputsInPresentation(CPresentation &presentation, bool isPrimary);
    void removeDataInputControl(const QVector<TElement *> &inElements);
    // Splits down vector attributes to components as Runtime does not really
    // handle vectors at this level anymore
    bool getAttributeVector4(QVector<QByteArray> &outAttVec, const QByteArray &attName,
                             TElement *elem);
    bool getAttributeVector3(QVector<QByteArray> &outAttVec, const QByteArray &attName,
                             TElement *elem);
    bool getAttributeVector2(QVector<QByteArray> &outAttVec, const QByteArray &attName,
                             TElement *elem);
    // build and evaluate Evaluator datainput type expression
    QJSValue buildJSFunc(const QString &userFunc);
    // pass controller name for error reporting purposes
    QVariant callJSFunc(const QString &controllerName, qt3ds::runtime::DataInputDef &diDef,
                        const QVariant::Type type);
    // matches all numeric datatypes so we do not get datatype mismatch when JS
    // decides to change result datatype (f.ex from double to int when result
    // fractional part for specific input values happens to be exactly zero)
    bool isMatchingDatatype(QVariant::Type resultType, QVariant::Type propertyType);
    // find out which datainputs are used in the expression
    QVector<QString> resolveDependentDatainputs(const QString &expression,
                                                const QString &controllerName);

    // Methods to add element attributes to list for element creation
    void addStringAttribute(qt3ds::foundation::IStringTable &strTable,
                            TPropertyDescAndValueList &list,
                            const QString &inAttName, const QString &inValue);
    void addIntAttribute(qt3ds::foundation::IStringTable &strTable,
                         TPropertyDescAndValueList &list,
                         const QString &inAttName, int inValue);
    void addBoolAttribute(qt3ds::foundation::IStringTable &strTable,
                          TPropertyDescAndValueList &list,
                          const QString &inAttName, bool inValue);
    void addFloatAttribute(qt3ds::foundation::IStringTable &strTable,
                           TPropertyDescAndValueList &list,
                           const QString &inAttName, float inValue);
    void addFloat2Attribute(qt3ds::foundation::IStringTable &strTable,
                            TPropertyDescAndValueList &list,
                            const QStringList &inAttNames, const QVector2D &inValue);
    void addFloat3Attribute(qt3ds::foundation::IStringTable &strTable,
                            TPropertyDescAndValueList &list,
                            const QStringList &inAttNames, const QVector3D &inValue);
    void addFloat4Attribute(qt3ds::foundation::IStringTable &strTable,
                            TPropertyDescAndValueList &list,
                            const QStringList &inAttNames, const QVector4D &inValue);
    void addElementRefAttribute(qt3ds::foundation::IStringTable &strTable,
                                TPropertyDescAndValueList &list,
                                const QString &inAttName, TElement *element);
    template <typename TDataType>
    void setDynamicObjectProperty(qt3ds::render::SDynamicObject &material,
                                  const qt3ds::render::dynamic::SPropertyDefinition &propDesc,
                                  const TDataType &propValue);
    QVector2D parseFloat2Property(const QString &propValue);
    QVector3D parseFloat3Property(const QString &propValue);
    QVector4D parseFloat4Property(const QString &propValue);

    void notifyElementCreation(const QStringList &elementNames, const QString &error);
    void notifyMaterialCreation(const QStringList &materialNames, const QString &error);
    void deleteElements(const QVector<TElement *> &elements,
                        qt3ds::render::IQt3DSRenderer *renderer);
    int getAvailableId();
    void releaseId(int id);
};

CQmlEngineImpl::CQmlEngineImpl(NVFoundationBase &fnd, ITimeProvider &)
    : m_Foundation(fnd)
    , m_Application(NULL)
    , m_ApplicationCore(NULL)
    , m_PreloadMutex(fnd.getAllocator())
    , m_MultithreadedMutex(NULL)
    , m_EmitSignalDataList(
          ForwardingAllocator(fnd.getAllocator(), "CQmlEngineImpl::m_EmitSignalDataList"))
    , mRefCount(0)
{
    qmlRegisterType<Q3DSQmlBehavior>("QtStudio3D.Behavior", 1, 0, "Behavior");
    qmlRegisterType<Q3DSQmlBehavior, 1>("QtStudio3D.Behavior", 1, 1, "Behavior");
}

void CQmlEngineImpl::Shutdown(qt3ds::NVFoundationBase &inFoundation)
{
}

void CQmlEngineImpl::EnableMultithreadedAccess()
{
    m_MultithreadedMutex = &m_PreloadMutex;
}

void CQmlEngineImpl::DisableMultithreadedAccess()
{
    m_MultithreadedMutex = NULL;
}

void CQmlEngineImpl::SetApplicationCore(runtime::IApplication &inApplication)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_ApplicationCore = &inApplication;
}

void CQmlEngineImpl::SetApplication(qt3ds::runtime::IApplication &inApplication)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_Application = &inApplication;
}

qt3ds::runtime::IApplication *CQmlEngineImpl::GetApplication()
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    return m_Application;
}

void CQmlEngineImpl::Initialize()
{
    // Gather data input controlled properties
    QList<CPresentation *> presentations = m_Application->GetPresentationList();

    for (int i = 0; i < presentations.size(); ++i) {
        initializeDataInputsInPresentation(*presentations[i], i == 0);
        initializeDataOutputsInPresentation(*presentations[i], i == 0);
    }
}

bool queueAttributeChange(TElement *target, const char *attName, const char *value,
                          TAttributeHash attrHash)
{
    if (target->m_BelongedPresentation->GetActivityZone()) {
        TElement *componentElement = target->GetActivityZone().GetItemTimeParent(*target);
        TComponent *component = static_cast<TComponent *>(componentElement);
        // Queue changes to elements inside components that have not been activated even once
        if (component->GetCurrentSlide() == 0) {
            IPresentation *presentation = target->GetBelongedPresentation();
            CQmlElementHelper::EnsureAttribute(target, attName, attrHash);
            presentation->GetComponentManager().queueChange(componentElement, target,
                                                            attName, value, attrHash);
            return true;
        }
    }
    return false;
}

void CQmlEngineImpl::SetAttribute(TElement *target, const QString &attName, const char *value)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;

    if (target) {
        QByteArray att = attName.toUtf8();
        auto attrHash = CHash::HashAttribute(attName);
        if (!queueAttributeChange(target, att.constData(), value, attrHash)) {
            bool success = CQmlElementHelper::SetAttribute(target, att.constData(), value,
                                                           attrHash);
            if (!success) {
                qCCritical(qt3ds::INVALID_OPERATION)
                        << "CQmlEngineImpl::SetAttribute: "
                        << "failed to set attribute on element"
                        << target->path().c_str() << ":" << attName << ":" << value;
            }
        }
    }
}

void CQmlEngineImpl::SetAttribute(const QString &element, const QString &attName, const char *value)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;

    TElement *theTarget = getTarget(element);
    if (theTarget) {
        QByteArray att = attName.toUtf8();
        auto attrHash = CHash::HashAttribute(attName);
        if (!queueAttributeChange(theTarget, att.constData(), value, attrHash)) {
            bool success = CQmlElementHelper::SetAttribute(theTarget, att.constData(), value,
                                                           attrHash);
            if (!success) {
                qCCritical(qt3ds::INVALID_OPERATION)
                        << "CQmlEngineImpl::SetAttribute: "
                        << "failed to set attribute on element"
                        << element << ":" << attName << ":" << value;
            }
        }
    }
}

bool CQmlEngineImpl::GetAttribute(TElement *target, const char *attName, char *value)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;

    if (target) {
        bool success = CQmlElementHelper::GetAttribute(target, attName, value);
        if (!success) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "CQmlEngineImpl::GetAttribute: "
                    << "failed to get attribute on element"
                    << target << ":" << attName << ":" << value;
        }
        return success;
    }

    return false;
}

bool CQmlEngineImpl::GetAttribute(const char *element, const char *attName, char *value)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;

    TElement *theTarget = getTarget(element);
    if (theTarget) {
        bool success = CQmlElementHelper::GetAttribute(theTarget, attName, value);
        if (!success) {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "CQmlEngineImpl::GetAttribute: "
                    << "failed to get attribute on element"
                    << element << ":" << attName << ":" << value;
        }
        return success;
    }

    return false;
}

void CQmlEngineImpl::loadScript(TElement *element, const QString &sourcePath)
{
    if (m_engine.isNull())
        m_engine.reset(new QQmlEngine);

    if (!m_components.contains(sourcePath)) {
        m_components[sourcePath] = new QQmlComponent(m_engine.data(),
                                                     QUrl::fromLocalFile(sourcePath),
                                                     m_engine.data());
    }

    QQmlComponent *component = m_components[sourcePath];
    if (component->status() == QQmlComponent::Ready) {
           createComponent(component, element);
    } else if (component->status() == QQmlComponent::Error) {
        qWarning() << "Error while loading script"
                   << component->url().toString()
                   << ":" << component->errorString();
    } else {
        QObject::connect(component, &QQmlComponent::statusChanged,
            [this, component, element] (QQmlComponent::Status status) {
                if (status == QQmlComponent::Ready) {
                    createComponent(component, element);
                } else {
                    qWarning() << "Error while loading script"
                               << component->url().toString()
                               << ":" << component->errorString();
                }
            }
        );
    }
}

void CQmlEngineImpl::LoadScript(IPresentation *presentation, TElement *element, const CHAR *path,
                                bool initInRenderThread)
{
    QString presPath = QFileInfo(presentation->GetFilePath()).absolutePath();

    QString sourcePath(presPath + QLatin1Char('/') + path);
    sourcePath.replace(QLatin1Char('\\'), QLatin1Char('/'));

    TElement *parent = element->GetParent();
    if (!parent)
        return;

    // Defer engine initialization until we are in render thread
    if (initInRenderThread)
        loadScript(element, sourcePath);
    else
        m_deferredScriptLoads.append({element, sourcePath});
}

void CQmlEngineImpl::loadDeferredScripts()
{
    for (const auto &data : qAsConst(m_deferredScriptLoads))
        loadScript(data.first, data.second);
    m_deferredScriptLoads.clear();
}

void CQmlEngineImpl::FireEvent(const char *element, const char *evtName)
{
    TElement *theElement = getTarget(element);
    if (theElement && theElement->GetActive() == true) {
        IPresentation *thePresentation = theElement->GetBelongedPresentation();
        thePresentation->FireEvent(CHash::HashEventCommand(evtName), theElement);
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::FireEvent: "
                << "failed to find element: "
                << element << " " << evtName;
    }
}

void CQmlEngineImpl::SetDataInputValue(
        const QString &name, const QVariant &value,
        qt3ds::runtime::DataInputValueRole valueRole = qt3ds::runtime::DataInputValueRole::Value)
{
    qt3ds::runtime::DataInputMap &diMap = m_Application->dataInputMap();
    if (diMap.contains(name)) {
        qt3ds::runtime::DataInputDef &diDef = diMap[name];
        switch (valueRole) {
        case qt3ds::runtime::DataInputValueRole::Value: { // switch (valueRole)
            diDef.value = value;
            const QVector<qt3ds::runtime::DataInOutAttribute> &ctrlAtt
                    = diDef.controlledAttributes;
            for (const qt3ds::runtime::DataInOutAttribute &ctrlElem : ctrlAtt) {
                switch (ctrlElem.propertyType) {
                case ATTRIBUTETYPE_DATAINPUT_TIMELINE: {
                    // Quietly ignore other than number type data inputs when adjusting timeline
                    if (diDef.type == qt3ds::runtime::DataInOutTypeRangedNumber) {
                        TTimeUnit endTime = 0;
                        TElement *element = getTarget(ctrlElem.elementPath.constData());
                        TComponent *component = static_cast<TComponent *>(element);
                        endTime = component->GetTimePolicy().GetLoopingDuration();

                        // Normalize the value to dataInput range
                        qreal newTime = qreal(endTime) * (qreal(value.toFloat() - diDef.min)
                                                          / qreal(diDef.max - diDef.min));
                        GotoTime(ctrlElem.elementPath.constData(), float(newTime / 1000.0));
                    }
                    break;
                }
                case ATTRIBUTETYPE_DATAINPUT_SLIDE: {
                    // Quietly ignore other than string type when adjusting slide
                    if (diDef.type == qt3ds::runtime::DataInOutTypeString) {
                        const QByteArray valueStr = value.toString().toUtf8();
                        GotoSlide(ctrlElem.elementPath.constData(), valueStr.constData(),
                                  SScriptEngineGotoSlideArgs());
                    }
                    break;
                }
                    // Silently ignore invalid incoming type if it does not
                    // match with the datainput type except with type Variant, for which
                    // the incoming value is cast to target property type without checking.
                    // Caveat emptor.

                    // For Evaluator, typecheck the JS evaluation result to see if it
                    // matches with the target property.

                    // Handle ranged number similarly to generic float
                    // if it is bound to properties other
                    // than timeline animation i.e. disregard range min and max
                case ATTRIBUTETYPE_FLOAT: {
                    float valueFloat;
                    if (diDef.type == qt3ds::runtime::DataInOutTypeFloat
                            || diDef.type == qt3ds::runtime::DataInOutTypeRangedNumber
                            || diDef.type == qt3ds::runtime::DataInOutTypeVariant) {
                        valueFloat = value.toFloat();
                    } else if (diDef.type == qt3ds::runtime::DataInOutTypeEvaluator) {
                        valueFloat = callJSFunc(name, diDef, QVariant::Type::Double).toFloat();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }

                    SetAttribute(ctrlElem.elementPath.constData(),
                                 ctrlElem.attributeName.first().constData(),
                                 reinterpret_cast<const char *>(&valueFloat));
                    break;
                }
                case ATTRIBUTETYPE_FLOAT4: {
                    QVector4D valueVec;
                    if (diDef.type == qt3ds::runtime::DataInOutTypeVector4) {
                        valueVec = value.value<QVector4D>();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }
                    // Set the values of vector attribute components separately
                    for (int i = 0; i < 4; i++) {
                        const float val = valueVec[i];
                        SetAttribute(ctrlElem.elementPath.constData(),
                                     ctrlElem.attributeName[i].constData(),
                                     reinterpret_cast<const char *>(&val));
                    }
                    break;
                }
                case ATTRIBUTETYPE_FLOAT3: {
                    QVector3D valueVec;
                    if (diDef.type == qt3ds::runtime::DataInOutTypeVector3
                            || diDef.type == qt3ds::runtime::DataInOutTypeVariant) {
                        valueVec = value.value<QVector3D>();
                    } else if (diDef.type == qt3ds::runtime::DataInOutTypeEvaluator) {
                        const QVariant res = callJSFunc(name, diDef, QVariant::Type::Vector3D);
                        valueVec = res.value<QVector3D>();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }
                    // Set the values of vector attribute components separately
                    for (int i = 0; i < 3; i++) {
                        const float val = valueVec[i];
                        SetAttribute(ctrlElem.elementPath.constData(),
                                     ctrlElem.attributeName[i].constData(),
                                     reinterpret_cast<const char *>(&val));
                    }
                    break;
                }
                case ATTRIBUTETYPE_FLOAT2:
                {
                    QVector2D valueVec;
                    if (diDef.type == qt3ds::runtime::DataInOutTypeVector2
                            || diDef.type == qt3ds::runtime::DataInOutTypeVariant) {
                        valueVec = value.value<QVector2D>();
                    } else if (diDef.type == qt3ds::runtime::DataInOutTypeEvaluator) {
                        const QVariant res = callJSFunc(name, diDef, QVariant::Type::Vector2D);
                        valueVec = res.value<QVector2D>();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }
                    // Set the values of vector attribute components separately
                    for (int i = 0; i < 2; i++) {
                        const float val = valueVec[i];
                        SetAttribute(ctrlElem.elementPath.constData(),
                                     ctrlElem.attributeName[i].constData(),
                                     reinterpret_cast<const char *>(&val));
                    }
                    break;
                }
                case ATTRIBUTETYPE_BOOL: {
                    uint valueBool; // SetAttribute requires at least 32-bit variable
                    if (diDef.type == qt3ds::runtime::DataInOutTypeBoolean
                            || diDef.type == qt3ds::runtime::DataInOutTypeVariant) {
                        valueBool = value.toBool();
                    } else if (diDef.type == qt3ds::runtime::DataInOutTypeEvaluator) {
                        valueBool = callJSFunc(name, diDef, QVariant::Type::Bool).toBool();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }
                    SetAttribute(ctrlElem.elementPath.constData(),
                                 ctrlElem.attributeName.first().constData(),
                                 reinterpret_cast<const char *>(&valueBool));

                    // Special case for eyeball (visibility) controller that targets elements
                    // on master slide, and whose visibility setting must be persistent over
                    // slide changes.
                    TElement *element = getTarget(ctrlElem.elementPath.constData());
                    auto hash = CHash::HashAttribute(QString(ctrlElem.attributeName.first()));

                    if (hash == Q3DStudio::ATTRIBUTE_EYEBALL && element->m_OnMaster) {
                        element->GetActivityZone().setControlled(*element);
                        element->SetControlledActive(valueBool);
                    }

                    break;
                }
                case ATTRIBUTETYPE_STRING: {
                    QByteArray valueStr;
                    // Allow scalar number types also as inputs to string attribute
                    if (diDef.type == qt3ds::runtime::DataInOutTypeString
                            || diDef.type == qt3ds::runtime::DataInOutTypeRangedNumber
                            || diDef.type == qt3ds::runtime::DataInOutTypeFloat
                            || diDef.type == qt3ds::runtime::DataInOutTypeVariant) {
                        valueStr = value.toString().toUtf8();
                    } else if (diDef.type == qt3ds::runtime::DataInOutTypeEvaluator) {
                        valueStr = callJSFunc(name, diDef, QVariant::Type::String)
                                .toString().toUtf8();
                    } else {
                        qWarning() << __FUNCTION__ << "Property type "
                                   << ctrlElem.propertyType
                                   << " not matching with Datainput " << name
                                   << " data type "
                                   << diDef.type;
                        break;
                    }

                    SetAttribute(ctrlElem.elementPath.constData(),
                                 ctrlElem.attributeName.first().constData(),
                                 valueStr.constData());
                    break;
                }
                default:
                    QT3DS_ALWAYS_ASSERT_MESSAGE("Unexpected data input type");
                    break;
                }
            }

            // Trigger re-evaluation of Evaluator datainputs that use this datainput
            // as source data. Do this by calling setDataInputValue for evaluator
            // with the current set value of the Evaluator (_not_ the evaluator result)
            for (auto dependent : diDef.dependents) {
                // Dependent list also contains the name of this datainput if
                // the value of this datainput is used as source data. In this case
                // obviously do not cause infinite recursion.
                if (dependent != name)
                    SetDataInputValue(dependent, diMap[dependent].value);
            }
            break;
        }
        case qt3ds::runtime::DataInputValueRole::Max: { // switch (valueRole)
            diDef.max = value.toFloat();
            break;
        }
        case qt3ds::runtime::DataInputValueRole::Min: { // switch (valueRole)
            diDef.min = value.toFloat();
            break;
        }
        }
    }
}

void CQmlEngineImpl::createElements(const QString &parentElementPath, const QString &slideName,
                                    const QVector<QHash<QString, QVariant>> &properties,
                                    qt3ds::render::IQt3DSRenderer *renderer)
{
    using namespace qt3ds::render;

    int elementIndex = -1;
    QString error;
    CPresentation *presentation = nullptr;
    QStringList elementPaths;
    elementPaths.reserve(properties.size());
    QVector<QHash<QString, QVariant>> theProperties = properties;
    const QString namePropName = QStringLiteral("name");
    QVector<int> idNumbers(theProperties.size());

    for (int i = 0; i < theProperties.size(); ++i) {
        auto &props = theProperties[i];
        QString newElementName = props.value(namePropName).toString();
        idNumbers[i] = getAvailableId();
        if (newElementName.isEmpty()) {
            // The id number on generated name will match generated graph object identifiers
            newElementName = QStringLiteral("NewElement_%1").arg(idNumbers[i]);
            props.insert(namePropName, newElementName);
        }
        elementPaths << parentElementPath + QLatin1Char('.') + newElementName;
    }
    QVector<TElement *> createdElements;
    TElement *parentElement = nullptr;

    auto handleError = [&]() {
        if (!error.isEmpty()) {
            for (int id : qAsConst(idNumbers))
                releaseId(id);
            deleteElements(createdElements, renderer);
        }
        notifyElementCreation(elementPaths, error);
    };

    // Resolve parent element
    QByteArray theParentPath = parentElementPath.toUtf8();
    parentElement = getTarget(theParentPath.constData());

    if (!parentElement) {
        error = QObject::tr("Invalid parent element: '%1'").arg(parentElementPath);
        handleError();
        return;
    }

    auto parentTranslator = static_cast<Qt3DSTranslator *>(parentElement->GetAssociation());

    if (!parentTranslator || !GraphObjectTypes::IsNodeType(
                parentTranslator->GetUIPType())) {
        error = QObject::tr("Parent element is not a valid node: '%1'").arg(parentElementPath);
        handleError();
        return;
    }

    TElement &component = parentElement->GetComponentParent();
    auto &parentObject = static_cast<SNode &>(parentTranslator->RenderObject());
    presentation = static_cast<CPresentation *>(parentElement->GetBelongedPresentation());

    // Resolve slide
    QByteArray theSlideName = slideName.toUtf8();
    ISlideSystem &slideSystem = presentation->GetSlideSystem();
    int slideIndex = slideSystem.FindSlide(component, theSlideName.constData());
    int currentSlide = static_cast<TComponent &>(component).GetCurrentSlide();
    if (slideIndex == 0xff) {
        error = QObject::tr("Invalid slide name for time context: '%1'").arg(slideName);
        handleError();
        return;
    }

    auto &strTable = presentation->GetStringTable();

    const QString sourcePathPropName = QStringLiteral("sourcepath");
    const QString startTimePropName = QStringLiteral("starttime");
    const QString endTimePropName = QStringLiteral("endtime");
    const QString eyeBallPropName = QStringLiteral("eyeball");
    const QString typePropName = QStringLiteral("type");

    Q3DStudio::UVariant attValue;
    parentElement->GetAttribute(Q3DStudio::ATTRIBUTE_STARTTIME, attValue);
    const int parentStartTime = int(attValue.m_INT32);
    parentElement->GetAttribute(Q3DStudio::ATTRIBUTE_ENDTIME, attValue);
    const int parentEndTime = int(attValue.m_INT32);

    for (int i = 0; i < theProperties.size(); ++i) {
        const auto &currentProps = theProperties[i];
        ++elementIndex;

        // Remove properties requiring custom handling
        QHash<QString, QVariant> fixedProps = currentProps;
        QString newElementName = fixedProps.take(namePropName).toString();
        QByteArray newElementNameBa = newElementName.toUtf8();

        QString refMatName = fixedProps.take(QStringLiteral("material")).toString();
        int colonIndex = refMatName.indexOf(QLatin1Char(':'));
        if (colonIndex != -1)
            refMatName = refMatName.mid(colonIndex + 1);
        if (refMatName.startsWith(QLatin1Char('#'))) // Absolute reference
            refMatName = refMatName.mid(1);
        else if (!refMatName.isEmpty() && !refMatName.contains(QLatin1Char('/')))
            refMatName = QStringLiteral("/") + refMatName;

        // Make sure the name is not duplicate
        TElement *existingChild
                = parentElement->FindChild(CHash::HashString(newElementName));
        if (existingChild) {
            error = QObject::tr("Element already exists: '%1'").arg(elementPaths[elementIndex]);
            handleError();
            return;
        }

        const CRegisteredString regName = strTable.RegisterStr(newElementName);
        TPropertyDescAndValueList elementProperties;
        CRegisteredString metaType;
        const CRegisteredString elementSubType;
        GraphObjectTypes::Enum objectType = GraphObjectTypes::Unknown;

        QString typeStr = fixedProps.take(typePropName).toString();
        if (typeStr.isEmpty() || typeStr.compare(QLatin1String("model"),
                                                 Qt::CaseInsensitive) == 0) {
            metaType = strTable.RegisterStr("Model");
            objectType = GraphObjectTypes::Model;
        } else if (typeStr.compare(QLatin1String("group"), Qt::CaseInsensitive) == 0) {
            metaType = strTable.RegisterStr("Group");
            objectType = GraphObjectTypes::Node;
        }

        // Set default values for missing mandatory properties
        bool eyeBall = true;
        fixedProps.value(eyeBallPropName, true).toBool();
        if (objectType == GraphObjectTypes::Model && !fixedProps.contains(sourcePathPropName)) {
            addStringAttribute(strTable, elementProperties, sourcePathPropName,
                               QStringLiteral("#Cube"));
        }
        if (!fixedProps.contains(startTimePropName))
            addIntAttribute(strTable, elementProperties, startTimePropName, parentStartTime);
        if (!fixedProps.contains(endTimePropName))
            addIntAttribute(strTable, elementProperties, endTimePropName, parentEndTime);
        if (!fixedProps.contains(eyeBallPropName))
            addBoolAttribute(strTable, elementProperties, eyeBallPropName, true);
        else
            eyeBall = fixedProps.value(eyeBallPropName).toBool();

        QHashIterator<QString, QVariant> it(fixedProps);
        while (it.hasNext()) {
            it.next();
            switch (it.value().type()) {
            case QVariant::Double:
                addFloatAttribute(strTable, elementProperties, it.key(), it.value().toFloat());
                break;
            case QVariant::Bool:
                addBoolAttribute(strTable, elementProperties, it.key(), it.value().toBool());
                break;
            case QVariant::Int:
                addIntAttribute(strTable, elementProperties, it.key(), it.value().toInt());
                break;
            case QVariant::String:
                addStringAttribute(strTable, elementProperties, it.key(), it.value().toString());
                break;
            case QVariant::Vector3D: {
                QVector3D vec = it.value().value<QVector3D>();
                if (it.key() == QLatin1String("rotation")) {
                    vec.setX(qDegreesToRadians(vec.x()));
                    vec.setY(qDegreesToRadians(vec.y()));
                    vec.setZ(qDegreesToRadians(vec.z()));
                }
                // TODO: Need to support also colors if non-model elements what need colors are
                // TODO: supported (QT3DS-3381)
                QStringList atts;
                atts << (it.key() + QLatin1String(".x"))
                     << (it.key() + QLatin1String(".y"))
                     << (it.key() + QLatin1String(".z"));
                addFloat3Attribute(strTable, elementProperties, atts, vec);
                break;
            }
            default:
                error = QObject::tr("Unsupported property type for: '%1'").arg(it.key());
                handleError();
                return;
            }
        }

        // Create new element
        QString localElementPath = elementPaths[elementIndex];
        colonIndex = localElementPath.indexOf(QLatin1Char(':'));
        if (colonIndex != -1)
            localElementPath = localElementPath.mid(colonIndex + 1);
        TElement &newElem = m_Application->GetElementAllocator().CreateElement(
                    regName, metaType, elementSubType,
                    toConstDataRef(elementProperties.data(), QT3DSU32(elementProperties.size())),
                    presentation, parentElement, false);
        newElem.setPath(strTable.RegisterStr(localElementPath));

        // Insert the new element into the correct slide
        if (!slideSystem.addSlideElement(component, slideIndex, newElem, eyeBall)) {
            // Delete created element if adding to slide failed
            m_Application->GetElementAllocator().ReleaseElement(newElem, true);
            error = QObject::tr("Failed to add the new element to a slide");
            handleError();
            return;
        }

        SGraphObject *referencedMaterial = nullptr;
        if (objectType == GraphObjectTypes::Model) {
            // Create material element
            const CRegisteredString matName = strTable.RegisterStr("refmat");
            const CRegisteredString matType = strTable.RegisterStr("ReferencedMaterial");
            TPropertyDescAndValueList matProperties;
            TElement &newMatElem = m_Application->GetElementAllocator().CreateElement(
                        matName, matType, elementSubType,
                        toConstDataRef(matProperties.data(), QT3DSU32(matProperties.size())),
                        presentation, &newElem, false);

            QString matElemPath = localElementPath + QLatin1String(".refmat");
            newMatElem.setPath(strTable.RegisterStr(matElemPath));

            if (!slideSystem.addSlideElement(component, slideIndex, newMatElem, eyeBall)) {
                // Delete created element and material element if adding to slide failed
                m_Application->GetElementAllocator().ReleaseElement(newElem, true);
                error = QObject::tr("Failed to add the new material element to a slide");
                handleError();
                return;
            }
            // First check if we can resolve the referenced material before creating any objects
            // Find a match in material container
            // If the specified material is not available in original presentation, or was not
            // specified, use the first material found as placeholder
            TElement *rootElement = presentation->GetRoot();
            TElement *container = rootElement->FindChild(CHash::HashString("__Container"));
            TElement *firstChild = nullptr;
            if (container) {
                auto children = container->children();
                if (!children.isEmpty())
                    firstChild = children.first();
                for (auto nextChild : children) {
                    QString childName = QString::fromUtf8(nextChild->name());
                    if (childName.endsWith(refMatName)) {
                        auto tr = static_cast<Qt3DSTranslator *>(
                                    nextChild->GetAssociation());
                        referencedMaterial = static_cast<SGraphObject *>(
                                    &tr->RenderObject());
                        break;
                    }
                }
            }

            if (!referencedMaterial) {
                // Empty material is assumed to be deliberate, so don't warn in that case
                if (!refMatName.isEmpty()) {
                    qWarning() << __FUNCTION__ << "Requested material" << refMatName
                               << "was not found. Trying to find a fallback material.";
                }
                if (firstChild) {
                    auto tr = static_cast<Qt3DSTranslator *>(firstChild->GetAssociation());
                    referencedMaterial = static_cast<SGraphObject *>(&tr->RenderObject());
                }
                if (!referencedMaterial) {
                    // We could create default material into the container in case there is
                    // no materials in there, but it is unlikely that such a presentation would
                    // be used in practice.
                    m_Application->GetElementAllocator().ReleaseElement(newElem, true);
                    error = QObject::tr("Unable to resolve a fallback material");
                    handleError();
                    return;
                }
            }
        }

        // Create model SGraphObject
        NVAllocatorCallback &allocator = presentation->GetScene()->allocator();
        SModel *newObject = QT3DS_NEW(allocator, SModel)();
        newObject->m_Id = strTable.RegisterStr((QByteArrayLiteral("__newObj_")
                                                + QByteArray::number(idNumbers[i])).constData());
        parentObject.AddChild(*newObject);

        Qt3DSTranslator::CreateTranslatorForElement(newElem, *newObject, allocator);

        if (referencedMaterial) {
            // Create material SGraphObject
            SReferencedMaterial *newMaterial = QT3DS_NEW(allocator, SReferencedMaterial)();
            newMaterial->m_Id = strTable.RegisterStr(
                        (QByteArrayLiteral("__newMat_")
                         + QByteArray::number(idNumbers[i])).constData());
            newMaterial->m_ReferencedMaterial = referencedMaterial;
            newObject->AddMaterial(*newMaterial);
        }

        // Determine if element should be active based on start/end times
        TTimeUnit startTime = 0;
        TTimeUnit stopTime = 0;
        if (newElem.GetAttribute(Q3DStudio::ATTRIBUTE_STARTTIME, attValue))
            startTime = TTimeUnit(attValue.m_INT32);
        if (newElem.GetAttribute(Q3DStudio::ATTRIBUTE_ENDTIME, attValue))
            stopTime = TTimeUnit(attValue.m_INT32);
        TTimeUnit localTime = newElem.GetActivityZone().GetItemLocalTime(newElem);

        bool isActiveRightNow = eyeBall && localTime >= startTime && localTime <= stopTime
                && currentSlide == slideIndex;

        newElem.SetActive(isActiveRightNow);
        newObject->m_Flags.SetActive(isActiveRightNow);
        if (eyeBall)
            newElem.GetActivityZone().UpdateItemInfo(newElem);

        createdElements << &newElem;
        m_elementIdMap.insert(&newElem, idNumbers[i]);
    }

    bool isPrimary = presentation == m_Application->GetPrimaryPresentation() ? true : false;
    initializeDataInputsInPresentation(*presentation, isPrimary, true, createdElements.toList());

    renderer->ChildrenUpdated(parentObject);

    handleError();
}

// Only supports deleting element types that can be added via createElement.
void CQmlEngineImpl::deleteElements(const QStringList &elementPaths,
                                    qt3ds::render::IQt3DSRenderer *renderer)
{
    QVector<TElement *> elements;
    // Convert the path list to set for quicker lookups
    QSet<QString> pathSet = elementPaths.toSet();

    for (auto &elementPath : elementPaths) {
        // Check that parent is not already included in the deleted elements
        int idx = elementPath.lastIndexOf(QLatin1Char('.'));
        bool parentFound = false;
        while (idx != -1) {
            QString parentPath = elementPath.left(idx);
            if (pathSet.contains(parentPath)) {
                parentFound = true;
                break;
            }
            idx = parentPath.lastIndexOf(QLatin1Char('.'));
        }
        if (!parentFound) {
            // Resolve element
            QByteArray thePath = elementPath.toUtf8();
            TElement *element = getTarget(thePath.constData());
            if (element)
                elements << element;
        }
    }
    deleteElements(elements, renderer);
}

TElement *CQmlEngineImpl::createMaterialContainer(TElement *parent, CPresentation *presentation)
{
    TPropertyDescAndValueList prop;
    auto &strTable = presentation->GetStringTable();
    const auto matName = strTable.RegisterStr(QStringLiteral("__Container"));
    const auto matType = strTable.RegisterStr(QStringLiteral("Material"));
    const auto matClass = CRegisteredString();
    TElement &element = m_Application->GetElementAllocator().CreateElement(
                        matName, matType, matClass,
                        toConstDataRef(prop.data(), prop.size()),
                        presentation, parent, false);
    return &element;
}

/**
    Creates material into material container of the specified subpresentation.
    The materialDefinition parameter can contain a .materialdef file path or
    the entire material definition in the .materialdef format.
*/
void CQmlEngineImpl::createMaterials(const QString &subPresId,
                                     const QStringList &materialDefinitions,
                                     ICustomMaterialSystem *customMaterialSystem,
                                     IDynamicObjectSystem *dynamicObjectSystem,
                                     qt3ds::render::IQt3DSRenderer *renderer)
{
    CPresentation *presentation = nullptr;
    QString error;
    QString presPath;
    QString projPath;
    struct MaterialInfo {
        QString materialDefinition;
        QString materialName;
        QMap<QString, QString> materialProps;
        QMap<QString, QMap<QString, QString>> textureProps;
    };
    QVector<MaterialInfo *> materialInfos;
    QVector<TElement *> createdElements;
    QVector<int> idNumbers(materialDefinitions.size());
    for (int i = 0; i < materialDefinitions.size(); ++i)
        idNumbers[i] = getAvailableId();

    auto handleError = [&]() {
        if (!error.isEmpty()) {
            for (int id : qAsConst(idNumbers))
                releaseId(id);
            deleteElements(createdElements, renderer);
        }
        QStringList materialNames;
        QString prefix;
        if (!subPresId.isEmpty())
            prefix = subPresId + QLatin1Char(':');
        for (auto &materialInfo : materialInfos)
            materialNames << prefix + materialInfo->materialName;
        qDeleteAll(materialInfos);
        notifyMaterialCreation(materialNames, error);
    };

    auto getMaterialInfos = [&]() {
        for (auto &materialDefinition : materialDefinitions) {
            MaterialInfo *info = new MaterialInfo;
            info->materialDefinition = materialDefinition;
            Q3DSMaterialDefinitionParser::getMaterialInfo(materialDefinition, projPath, presPath,
                                                          info->materialName, info->materialProps,
                                                          info->textureProps);
            materialInfos << info;
        }
    };

    QByteArray theSubPresId = subPresId.toUtf8();
    if (theSubPresId.isEmpty())
        presentation = m_Application->GetPrimaryPresentation();
    else
        presentation = m_Application->LoadAndGetPresentationById(theSubPresId.constData());

    if (!presentation) {
        error = QObject::tr("Invalid presentation ID: '%1'").arg(subPresId);
        presentation = m_Application->GetPrimaryPresentation();
        presPath = QFileInfo(presentation->GetFilePath()).absolutePath();
        projPath = presentation->getProjectPath();
        getMaterialInfos();
        handleError();
        return;
    }

    presPath = QFileInfo(presentation->GetFilePath()).absolutePath();
    projPath = presentation->getProjectPath();
    getMaterialInfos();

    // Find material container
    auto &strTable = presentation->GetStringTable();
    NVAllocatorCallback &allocator = presentation->GetScene()->allocator();
    TElement *rootElement = presentation->GetRoot();
    const auto containerName = strTable.RegisterStr("__Container");
    TElement *container = rootElement->FindChild(CHash::HashString(containerName.c_str()));
    if (!container)
        container = createMaterialContainer(rootElement, presentation);

    for (int i = 0; i < materialInfos.size(); ++i) {
        const auto &materialInfo = materialInfos[i];
        if (materialInfo->materialName.isEmpty() || materialInfo->materialProps.isEmpty()) {
            error = QObject::tr("Invalid material definition: '%1'")
                    .arg(materialInfo->materialDefinition);
            handleError();
            return;
        }

        // We don't care about the path parameter
        const QString pathStr = QStringLiteral("path");
        if (materialInfo->materialProps.contains(pathStr))
            materialInfo->materialProps.remove(pathStr);

        // Check that the material doesn't already exist in container
        const auto children = container->children();
        for (auto nextChild : children) {
            QString childName = QString::fromUtf8(nextChild->name());
            if (childName == materialInfo->materialName) {
                error = QObject::tr("Material already exists in material container");
                handleError();
                return;
            }
        }

        // Create material element in the container based on the material definition
        auto &metaData = m_Application->GetMetaData();
        const auto matName = strTable.RegisterStr(materialInfo->materialName);
        const bool isCustomMaterial = (materialInfo->materialProps.value(QStringLiteral("type"))
                                       == QLatin1String("CustomMaterial"));
        CRegisteredString matType;
        CRegisteredString matClass;
        QHash<QString, qt3ds::render::dynamic::SPropertyDefinition> dynPropDefs;

        if (isCustomMaterial) {
            CRegisteredString sourcePath
                    = strTable.RegisterStr(materialInfo->materialProps.value(
                                               QStringLiteral("sourcepath")).toUtf8());
            matType = strTable.RegisterStr("CustomMaterial");
            matClass = sourcePath; // Create just one class per shader
            if (sourcePath.IsValid()) {
                Option<qt3dsdm::SMetaDataCustomMaterial> matMetaData =
                    metaData.GetMaterialMetaDataBySourcePath(sourcePath.c_str());
                if (!matMetaData.hasValue()) {
                    metaData.LoadMaterialXMLFile(matType.c_str(), matClass.c_str(), matName.c_str(),
                                                 sourcePath.c_str());
                    matMetaData = metaData.GetMaterialMetaDataBySourcePath(sourcePath.c_str());
                }
                if (matMetaData.hasValue()) {
                    qt3ds::render::IUIPLoader::CreateMaterialClassFromMetaMaterial(
                                matClass, m_Foundation, *customMaterialSystem, matMetaData,
                                strTable);
                    NVConstDataRef<qt3ds::render::dynamic::SPropertyDefinition> customProperties;
                    customProperties = dynamicObjectSystem->GetProperties(matClass);
                    for (QT3DSU32 i = 0, end = customProperties.size(); i < end; ++i) {
                        QString propName = QString::fromUtf8(customProperties[i].m_Name.c_str());
                        if (materialInfo->materialProps.contains(propName))
                            dynPropDefs.insert(propName, customProperties[i]);
                    }
                } else {
                    error = QObject::tr("Could not resolve properties for CustomMaterial");
                    handleError();
                    return;
                }
            } else {
                error = QObject::tr("Missing sourcepath in definition of CustomMaterial");
                handleError();
                return;
            }
        } else {
            matType = strTable.RegisterStr("Material");
        }

        auto createElementPropsFromDefProps = [&](const QMap<QString, QString> &defProps,
                                                  TPropertyDescAndValueList &elementProps,
                                                  const CRegisteredString &elementType) {
            QMapIterator<QString, QString> propIter(defProps);
            while (propIter.hasNext()) {
                propIter.next();
                if (dynPropDefs.contains(propIter.key()))
                    continue; // Dynamic properties are added directly to graph objects later

                auto propName = strTable.RegisterStr(propIter.key());
                ERuntimeDataModelDataType dataType;
                ERuntimeAdditionalMetaDataType additionalType;
                dataType = metaData.GetPropertyType(elementType, propName);
                additionalType = metaData.GetAdditionalType(elementType, propName);

                switch (dataType) {
                case ERuntimeDataModelDataTypeLong: {
                    addIntAttribute(strTable, elementProps, propIter.key(),
                                    propIter.value().toInt());
                    break;
                }
                case ERuntimeDataModelDataTypeFloat: {
                    addFloatAttribute(strTable, elementProps, propIter.key(),
                                      propIter.value().toFloat());
                    break;
                }
                case ERuntimeDataModelDataTypeFloat2: {
                    QVector2D vec = parseFloat2Property(propIter.value());
                    QStringList atts;
                    atts << (propIter.key() + QLatin1String(".x"))
                         << (propIter.key() + QLatin1String(".y"));
                    addFloat2Attribute(strTable, elementProps, atts, vec);
                    break;
                }
                case ERuntimeDataModelDataTypeFloat3: {
                    QVector3D vec = parseFloat3Property(propIter.value());
                    if (additionalType == ERuntimeAdditionalMetaDataTypeRotation) {
                        vec.setX(qDegreesToRadians(vec.x()));
                        vec.setY(qDegreesToRadians(vec.y()));
                        vec.setZ(qDegreesToRadians(vec.z()));
                    }
                    QStringList atts;
                    atts << (propIter.key() + QLatin1String(".x"))
                         << (propIter.key() + QLatin1String(".y"))
                         << (propIter.key() + QLatin1String(".z"));
                    addFloat3Attribute(strTable, elementProps, atts, vec);
                    break;
                }
                case ERuntimeDataModelDataTypeFloat4: {
                    QVector4D vec = parseFloat4Property(propIter.value());
                    QStringList atts;
                    if (additionalType == ERuntimeAdditionalMetaDataTypeColor) {
                        atts << (propIter.key() + QLatin1String(".r"))
                             << (propIter.key() + QLatin1String(".g"))
                             << (propIter.key() + QLatin1String(".b"))
                             << (propIter.key() + QLatin1String(".a"));
                    } else {
                        atts << (propIter.key() + QLatin1String(".x"))
                             << (propIter.key() + QLatin1String(".y"))
                             << (propIter.key() + QLatin1String(".z"))
                             << (propIter.key() + QLatin1String(".w"));
                    }
                    addFloat4Attribute(strTable, elementProps, atts, vec);
                    break;
                }
                case ERuntimeDataModelDataTypeBool: {
                    bool boolValue = propIter.value().compare(QLatin1String("true"),
                                                              Qt::CaseInsensitive) == 0;
                    addBoolAttribute(strTable, elementProps, propIter.key(), boolValue);
                    break;
                }
                case ERuntimeDataModelDataTypeStringRef:
                case ERuntimeDataModelDataTypeString: {
                    addStringAttribute(strTable, elementProps, propIter.key(), propIter.value());
                    break;
                }
                case ERuntimeDataModelDataTypeLong4: {
                    if (additionalType == ERuntimeAdditionalMetaDataTypeImage) {
                        // Insert placeholder for now, will be patched later
                        addElementRefAttribute(strTable, elementProps, propIter.key(), nullptr);
                    }
                    break;
                }
                default:
                    error = QObject::tr("Unsupported material property type for property: '%1'")
                            .arg(propIter.key());
                    return;
                }
            }
        };

        TPropertyDescAndValueList elementProps;
        createElementPropsFromDefProps(materialInfo->materialProps, elementProps, matType);
        if (!error.isEmpty()) {
            handleError();
            return;
        }

        TElement &newMatElem = m_Application->GetElementAllocator().CreateElement(
                    matName, matType, matClass,
                    toConstDataRef(elementProps.data(), QT3DSU32(elementProps.size())),
                    presentation, container, false);
        newMatElem.SetActive(true);

        // Create image elements
        CRegisteredString imageType = strTable.RegisterStr("Image");
        QMapIterator<QString, QMap<QString, QString>> texIter(materialInfo->textureProps);
        QHash<QString, TElement *> imageElementMap;
        while (texIter.hasNext()) {
            texIter.next();
            elementProps.clear();
            createElementPropsFromDefProps(texIter.value(), elementProps, imageType);
            if (!error.isEmpty()) {
                m_Application->GetElementAllocator().ReleaseElement(newMatElem, true);
                handleError();
                return;
            }
            CRegisteredString imageName = strTable.RegisterStr(texIter.key());

            TElement &newImageElem = m_Application->GetElementAllocator().CreateElement(
                        imageName, imageType, CRegisteredString(),
                        toConstDataRef(elementProps.data(), QT3DSU32(elementProps.size())),
                        presentation, &newMatElem, false);
            imageElementMap.insert(texIter.key(), &newImageElem);
            newImageElem.SetActive(true);
        }

        // Create render object for the material
        qt3ds::render::SGraphObject *newMaterial = nullptr;
        CRegisteredString newMatId = strTable.RegisterStr(
                    (QByteArrayLiteral("__newMat_") + QByteArray::number(idNumbers[i]))
                    .constData());
        if (isCustomMaterial) {
            newMaterial = customMaterialSystem->CreateCustomMaterial(matClass, allocator);
            newMaterial->m_Id = newMatId;
            auto dynObj = static_cast<qt3ds::render::SDynamicObject *>(newMaterial);

            QHashIterator<QString, qt3ds::render::dynamic::SPropertyDefinition>
                    dynPropIter(dynPropDefs);
            while (dynPropIter.hasNext()) {
                dynPropIter.next();
                QByteArray propValStr = materialInfo->materialProps.value(
                            dynPropIter.key()).toUtf8();
                const auto propDesc = dynPropIter.value();
                switch (propDesc.m_DataType) {
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSRenderBool: {
                    bool boolValue = propValStr.compare(QByteArrayLiteral("true"),
                                                        Qt::CaseInsensitive) == 0;
                    setDynamicObjectProperty(*dynObj, propDesc, boolValue);
                    break;
                }
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSF32:
                    setDynamicObjectProperty(*dynObj, propDesc, propValStr.toFloat());
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSI32:
                    if (!propDesc.m_IsEnumProperty) {
                        setDynamicObjectProperty(*dynObj, propDesc, propValStr.toInt());
                    } else {
                        const NVConstDataRef<CRegisteredString> &enumNames
                                = propDesc.m_EnumValueNames;
                        for (QT3DSU32 i = 0, end = enumNames.size(); i < end; ++i) {
                            if (propValStr.compare(enumNames[i].c_str()) == 0) {
                                setDynamicObjectProperty(*dynObj, propDesc, i);
                                break;
                            }
                        }
                    }
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec2:
                    setDynamicObjectProperty(*dynObj, propDesc, parseFloat2Property(propValStr));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec3:
                    setDynamicObjectProperty(*dynObj, propDesc, parseFloat3Property(propValStr));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::QT3DSVec4:
                    setDynamicObjectProperty(*dynObj, propDesc, parseFloat4Property(propValStr));
                    break;
                case qt3ds::render::NVRenderShaderDataTypes::NVRenderTexture2DPtr:
                case qt3ds::render::NVRenderShaderDataTypes::NVRenderImage2DPtr: {
                    CRegisteredString regStr;
                    regStr = strTable.RegisterStr(propValStr);
                    setDynamicObjectProperty(*dynObj, propDesc, regStr);
                    break;
                }
                default:
                    error = QObject::tr("Unsupported custom material property type for '%1'")
                            .arg(dynPropIter.key());
                    m_Application->GetElementAllocator().ReleaseElement(newMatElem, true);
                    handleError();
                    return;
                }
            }
        } else {
            newMaterial = QT3DS_NEW(allocator, qt3ds::render::SDefaultMaterial)();
            newMaterial->m_Id = newMatId;

            // Update element refs for image elements and create graph objects & translators
            QHashIterator<QString, TElement *> imageIter(imageElementMap);
            while (imageIter.hasNext()) {
                imageIter.next();
                TElement *imageElem = imageIter.value();
                UVariant *propValue = newMatElem.FindPropertyValue(imageElem->name());
                if (propValue) {
                    propValue->m_ElementHandle = imageIter.value()->GetHandle();

                    qt3ds::render::SImage *newImageObj
                            = QT3DS_NEW(allocator, qt3ds::render::SImage)();
                    const int imageId = getAvailableId();
                    newImageObj->m_Id = strTable.RegisterStr(
                                (QByteArrayLiteral("__newImage_")
                                 + QByteArray::number(imageId)).constData());
                    qt3ds::render::Qt3DSTranslator::CreateTranslatorForElement(
                                *imageElem, *newImageObj, allocator);
                    m_elementIdMap.insert(imageElem, imageId);
                }
            }
        }
        createdElements << &newMatElem;
        m_elementIdMap.insert(&newMatElem, idNumbers[i]);

        qt3ds::render::Qt3DSTranslator::CreateTranslatorForElement(newMatElem, *newMaterial,
                                                                   allocator);
    }

    handleError();
}

void CQmlEngineImpl::deleteMaterials(const QStringList &materialNames, IQt3DSRenderer *renderer)
{
    // Material class (i.e. the shader) is not deleted as those can be shared between materials,
    // so we just delete the material elements from the container and the related render objects

    // Sort materials to presentations
    QMultiHash<QString, QString> presMaterialMap;
    QSet<QString> presIds;
    for (const auto &matName : materialNames) {
        QString presId;
        QString localName = matName;
        int index = matName.indexOf(QLatin1Char(':'));
        if (index != -1) {
            presId = matName.left(index);
            localName = matName.mid(index + 1);
        }
        presMaterialMap.insert(presId, localName);
        presIds.insert(presId);
    }
    for (const auto &presId : qAsConst(presIds)) {
        QByteArray theId = presId.toUtf8();
        CPresentation *presentation = nullptr;
        if (presId.isEmpty())
            presentation = m_Application->GetPrimaryPresentation();
        else
            presentation = m_Application->LoadAndGetPresentationById(theId.constData());

        if (presentation) {
            // Find material container
            auto &strTable = presentation->GetStringTable();
            TElement *rootElement = presentation->GetRoot();
            const auto containerName = strTable.RegisterStr("__Container");
            TElement *container = rootElement->FindChild(CHash::HashString(containerName.c_str()));
            Q_ASSERT_X(container, __FUNCTION__,
                       QStringLiteral("No material container found for presentation: '%1'")
                       .arg(presId).toUtf8());

            QVector<TElement *> elementsToDelete;
            const QList<QString> matNames = presMaterialMap.values(presId);
            for (const auto &materialName : matNames) {
                const auto children = container->children();
                bool added = false;
                for (auto nextChild : children) {
                    QString childName = QString::fromUtf8(nextChild->name());
                    if (childName == materialName) {
                        elementsToDelete << nextChild;
                        added = true;
                        break;
                    }
                }
                if (!added) {
                    if (presId.isEmpty()) {
                        qWarning() << __FUNCTION__
                                   << QStringLiteral("Could not find material '%1'")
                                      .arg(materialName);
                    } else {
                        qWarning() << __FUNCTION__
                                   << QStringLiteral("Could not find material '%1' in '%2'")
                                      .arg(materialName).arg(presId);
                    }
                }
            }
            deleteElements(elementsToDelete, renderer);
        } else {
            qWarning() << __FUNCTION__ << "Warning: Presentation ID could not be resolved:"
                       << presId;
        }
    }
}

void CQmlEngineImpl::createMesh(const QString &name, qt3dsimp::Mesh *mesh,
                                qt3ds::render::IBufferManager *bufferManager)
{
    // Add the custom meshes to buffer manager.
    bufferManager->loadCustomMesh(name, mesh);
}

void CQmlEngineImpl::deleteMeshes(const QStringList &meshNames,
                                  qt3ds::render::IBufferManager *bufferManager)
{
    for (const auto &meshName : meshNames) {
        if (!meshName.isEmpty()) {
            CRegisteredString regName = bufferManager->GetStringTable().RegisterStr(meshName);
            bufferManager->InvalidateBuffer(regName);
        }
    }
}

uint CQmlEngineImpl::textureId(const QString &elementPath,
                               qt3ds::render::IQt3DSRenderer *renderer)
{
    TElement *elem = getTarget(elementPath.toUtf8().constData());
    if (elem) {
        auto translator = static_cast<qt3ds::render::Qt3DSTranslator *>(elem->GetAssociation());
        if (translator) {
            qt3ds::render::GraphObjectTypes::Enum type = translator->GetUIPType();
            if (type == qt3ds::render::GraphObjectTypes::Layer) {
                auto layer = static_cast<qt3ds::render::SLayer *>(&translator->RenderObject());
                return renderer->getLayerTextureId(*layer);
            } else if (type == qt3ds::render::GraphObjectTypes::Image) {
                auto image = static_cast<qt3ds::render::SImage *>(&translator->RenderObject());
                if (image->m_TextureData.m_Texture) {
                    return static_cast<uint>(reinterpret_cast<size_t>(
                                image->m_TextureData.m_Texture->GetTextureObjectHandle()));
                }
            }
        }
    }
    return 0;
}

void CQmlEngineImpl::GotoSlide(const char *component, const char *slideName,
                               const SScriptEngineGotoSlideArgs &inArgs)
{
    TElement *theTarget = getTarget(component);
    if (theTarget) {
        CQmlCommandHelper::SetupGotoSlideCommand(*theTarget, slideName, inArgs);
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::GotoSlide: Unable to find component " << component;
    }
}

void CQmlEngineImpl::GotoSlideRelative(const char *component, bool inNextSlide, bool inWrap,
                                       const SScriptEngineGotoSlideArgs &inArgs)
{
    TElement *theTarget = getTarget(component);
    if (theTarget) {
        theTarget = &theTarget->GetComponentParent();
        if (theTarget && theTarget->GetActive()) {
            TComponent *theComponent = static_cast<TComponent *>(theTarget);
            Q3DStudio::INT32 theSlide = theComponent->GetCurrentSlide();
            Q3DStudio::INT32 theSlideCount = theComponent->GetSlideCount();
            theSlide = inNextSlide ? theSlide + 1 : theSlide - 1;
            if (theSlide < 1) {
                if (inWrap)
                    theSlide = theSlideCount - 1;
                else
                    theSlide = 1;
            } else if (theSlide == theSlideCount) {
                if (inWrap)
                    theSlide = 1;
                else
                    theSlide = theSlideCount - 1;
            }
            if (theSlide != theComponent->GetCurrentSlide()) {
                CQmlCommandHelper::SetupGotoSlideCommand(*theTarget, theSlide, inArgs);
            }
        } else {
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "QmlEngine::GotoSlideRelative: Component is not active: " << component;
        }
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "QmlEngine::GotoSlideRelative: failed to find component: " << component;
    }
}

void CQmlEngineImpl::SetPresentationAttribute(const char *presId, const char *, const char *value)
{
    if (isTrivial(presId))
        return;
    if (presId[0] == '#')
        ++presId;
    CPresentation *thePresentation = m_Application->LoadAndGetPresentationById(presId);
    if (thePresentation) {
        bool active = AreEqualCaseless(nonNull(value), "True");
        thePresentation->SetActive(active);
    }
}

void CQmlEngineImpl::GotoSlideIndex(const char *component, const Q3DStudio::INT32 slideIndex,
                                    const SScriptEngineGotoSlideArgs &inArgs)
{
    TElement *theTarget = getTarget(component);
    if (theTarget) {
        CQmlCommandHelper::SetupGotoSlideCommand(*theTarget, slideIndex, inArgs);
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::GotoSlide: Unable to find component " << component;
    }
}

bool CQmlEngineImpl::GetSlideInfo(const char *element, int &currentIndex, int &previousIndex,
                                  QString &currentName, QString &previousName)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;

    TElement *theTarget = getTarget(element);
    if (theTarget && theTarget->IsComponent()) {
        IPresentation *thePresentation = theTarget->GetBelongedPresentation();
        TComponent *theComponent = thePresentation->GetComponentManager().GetComponent(theTarget);
        currentIndex = int(theComponent->GetCurrentSlide());
        previousIndex = int(theComponent->GetPreviousSlide());
        if (currentIndex > 0) {
            currentName = QString::fromLocal8Bit(
                        thePresentation->GetSlideSystem().GetSlideName(
                            qt3ds::runtime::SSlideKey(*theComponent, QT3DSU32(currentIndex))));
        } else {
            currentName.clear();
        }
        if (previousIndex > 0) {
            previousName = QString::fromLocal8Bit(
                        thePresentation->GetSlideSystem().GetSlideName(
                            qt3ds::runtime::SSlideKey(*theComponent, QT3DSU32(previousIndex))));
        } else {
            previousName.clear();
        }
        return true;
    } else {
        qCCritical(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::GetSlideInfo: Supplied element is not a component " << element;
    }
    return false;
}

void CQmlEngineImpl::GotoTime(const char *component, const Q3DStudio::FLOAT time)
{
    TElement *theTarget = getTarget(component);
    if (theTarget && (theTarget->GetActive() || theTarget->AboutToActivate())) {
        UVariant theArg1;
        UVariant theArg2;

        IPresentation *thePresentation = theTarget->GetBelongedPresentation();

        theArg1.m_INT32 = static_cast<INT32>(time * 1000);

        TElement *theParentTarget = &theTarget->GetComponentParent();

        thePresentation->FireCommand(COMMAND_GOTOTIME, theParentTarget, &theArg1, &theArg2);
    }
}

bool CQmlEngineImpl::RegisterCallback(Q3DStudio::UINT32 callbackType,
                                      const TScriptCallback inCallback, void *inUserData)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    return m_ScriptCallbacks.RegisterCallback(callbackType, inCallback, inUserData);
}

void CQmlEngineImpl::ProcessFrameCallbacks(IPresentation *)
{
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    m_ScriptCallbacks.ProcessCallbacks();

    for (Q3DSQmlScript *script : m_scripts)
        script->update();
}

void CQmlEngineImpl::ProcessSignal(IPresentation *inPresentation,
                                   const SEventCommand &inCommand)
{
    using qt3ds::runtime::TIdValuePair;
    QML_ENGINE_MULTITHREAD_PROTECT_METHOD;
    TElement *theElement = inCommand.m_Target; // the element that is a behavior

    CPresentation *thePresentation = (CPresentation *)inPresentation;
    // IParametersSystem& theParametersManager = thePresentation->GetParametersSystem();
    qt3ds::foundation::IStringTable &theStrTable(thePresentation->GetStringTable());

    CRegisteredString theSignal = theStrTable.HandleToStr(inCommand.m_Arg1.m_INT32);

    if (!theSignal.IsValid() || m_EmitSignalDataList.size() > CQmlEngineImpl::MAX_ACTION_QUEUE_SIZE)
        return;

    TEmitSignalPtr theTemp = QT3DS_NEW(m_Foundation.getAllocator(), SEmitSignalData)(
        m_Foundation.getAllocator(), inCommand.m_Arg1.m_INT32, theElement);
    m_EmitSignalDataList.push_back(theTemp);
}


//==============================================================================
/**
*   Handle custom actions
*   @param inCommand                carrier of command and parameter information
*/
void CQmlEngineImpl::ProcessCustomActions(IPresentation *presentation,
                                          const SEventCommand &command)
{
    TElement *element = command.m_Target;
    for (Q3DSQmlScript *script : m_scripts) {
        if (script->hasBehavior(element)) {
            IParametersSystem &parametersManager
                    = static_cast<CPresentation *>(presentation)->GetParametersSystem();
            INT32 groupId = command.m_Arg1.m_INT32;
            UINT32 numParams = parametersManager.GetNumParameters(groupId);
            if (numParams == 0) {
                QT3DS_ASSERT(false);
                return;
            }

            qt3ds::runtime::TIdValuePair tempData = parametersManager.GetParameter(groupId, 0);
            if (tempData.first != CHash::HashString("string")) {
                QT3DS_ASSERT(false);
                return;
            }

            CRegisteredString functionName = presentation->GetStringTable().HandleToStr(
                        tempData.second.m_StringHandle);
            script->call(functionName.c_str());
            return;
        }
    }
}

bool CQmlEngineImpl::PeekSignal(TElement *&outElement, char *&outName)
{
    if (m_EmitSignalDataList.empty())
        return false;

    NVScopedRefCounted<SEmitSignalData> theAction = m_EmitSignalDataList.front();
    m_EmitSignalDataList.pop_front();

    outElement = theAction->m_TargetElement;
    if (outElement) {
        IPresentation *thePresentation = outElement->GetBelongedPresentation();
        qt3ds::foundation::IStringTable &theStrTable(thePresentation->GetStringTable());
        CRegisteredString theSignal = theStrTable.HandleToStr(theAction->m_SignalNameHandler);
        outName = (char *)theSignal.c_str();
    } else {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "CQmlEngineImpl::PeekCustomAction: Unable to find element: EmitSignal queue error";
        outName = NULL;
    }

    return true;
}

void CQmlEngineImpl::createComponent(QQmlComponent *component, TElement *element)
{
    TElement *parent = element->GetParent();
    if (!parent)
        return;

    QQmlContext *context = new QQmlContext(m_engine.data(), m_engine.data());
    QObject *obj = component->beginCreate(context);
    if (!obj) {
        context->deleteLater();
        return;
    }
    Q3DSQmlBehavior *behaviorObject = qobject_cast<Q3DSQmlBehavior *>(obj);
    if (behaviorObject == nullptr) {
        obj->deleteLater();
        context->deleteLater();
        return;
    }

    auto script = new Q3DSQmlScript(*this, *behaviorObject, *element, *parent);
    component->completeCreate();
    behaviorObject->setScript(script);

    script->setParent(component);
    obj->setParent(script);
    m_scripts.push_back(script);
}

TElement *CQmlEngineImpl::getTarget(const QString &component) {
    TElement *target = NULL;
    int delimIndex = component.indexOf(":");
    if (delimIndex > 0) {
        target = CQmlElementHelper::GetElement(
                    *m_Application,
                    m_Application->LoadAndGetPresentationById(component.left(delimIndex)),
                    component.right(delimIndex + 1), NULL);
    } else {
        target = CQmlElementHelper::GetElement(
                    *m_Application,
                    m_Application->GetPrimaryPresentation(),
                    component, NULL);
    }
    return target;
}

void CQmlEngineImpl::listAllElements(TElement *root, QList<TElement *> &elements)
{
    elements.append(root);
    const auto children(root->children());
    for (auto nextChild : children)
        listAllElements(nextChild, elements);
}

// Initializes datainput bindings in the presentation starting by default from the root element.
// If inElements is specified, only parses the specified elements.
void CQmlEngineImpl::initializeDataInputsInPresentation(CPresentation &presentation,
                                                        bool isPrimary, bool isDynamicAdd,
                                                        QList<TElement *> inElements)
{
    QList<TElement *> elements;
    if (!inElements.empty()) {
        elements = inElements;
    } else {
        TElement *parent = presentation.GetRoot();
        listAllElements(parent, elements);
    }
    qt3ds::runtime::DataInputMap &diMap = m_Application->dataInputMap();

// #TODO: Remove below once QT3DS-3510 has been implemented in the editor
    qt3ds::runtime::DataOutputMap &doMap = m_Application->dataOutputMap();
// #TODO: Remove above once QT3DS-3510 has been implemented in the editor

    qt3ds::foundation::IStringTable &strTable(presentation.GetStringTable());
    QHash<CRegisteredString, qt3ds::runtime::DataOutputDef> elementPathToDataOutputDefMap;
    for (TElement *element : qAsConst(elements)) {
        Option<QT3DSU32> ctrlIndex = element->FindPropertyIndex(ATTRIBUTE_CONTROLLEDPROPERTY);
        if (ctrlIndex.hasValue()) {
            Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> propertyInfo =
                    element->GetPropertyByIndex(*ctrlIndex);
            UVariant *valuePtr = propertyInfo->second;
            QString valueStr =
                    QString::fromUtf8(strTable.HandleToStr(valuePtr->m_StringHandle));
            if (!valueStr.isEmpty()) {
                QStringList splitValues = valueStr.split(QChar(' '));
                for (int i = 1; i < splitValues.size(); i += 2) {
                    QString controllerName = splitValues[i - 1];
                    // remove datainput name prefix "$"
                    controllerName.remove(0, 1);
                    if (diMap.contains(controllerName)) {
                        qt3ds::runtime::DataInOutAttribute ctrlElem;
                        if (!isPrimary) {
                            // Prepend presentation id to element path
                            ctrlElem.elementPath = presentation.GetName();
                            ctrlElem.elementPath.append(QByteArrayLiteral(":"));
                        }
                        ctrlElem.attributeName.append(splitValues[i].toUtf8());
                        if (ctrlElem.attributeName.first() == QByteArrayLiteral("@timeline")) {
                            ctrlElem.propertyType = ATTRIBUTETYPE_DATAINPUT_TIMELINE;
                            TElement *component = &element->GetComponentParent();
                            ctrlElem.elementPath.append(component->path());
                        } else if (ctrlElem.attributeName.first() == QByteArrayLiteral("@slide")) {
                            ctrlElem.propertyType = ATTRIBUTETYPE_DATAINPUT_SLIDE;
                            TElement *component = &element->GetComponentParent();
                            ctrlElem.elementPath.append(component->path());
                        } else if (diMap[controllerName].type
                                   == qt3ds::runtime::DataInOutTypeVector4) {
                            // special handling for vector datatype to handle
                            // expansion from <propertyname> to <propertyname>.x .y .z .w
                            QVector<QByteArray> attVec;
                            bool success = getAttributeVector4(
                                attVec, ctrlElem.attributeName.first().constData(),
                                element);
                            if (!attVec.empty() && success) {
                                ctrlElem.attributeName = attVec;
                                ctrlElem.elementPath.append(element->path());
                                ctrlElem.propertyType = ATTRIBUTETYPE_FLOAT4;
                            } else {
                                qWarning() << __FUNCTION__ << "Property "
                                           << ctrlElem.attributeName.first()
                                           << " was not expanded to vector";
                                ctrlElem.propertyType = ATTRIBUTETYPE_NONE;
                            }
                        } else if (diMap[controllerName].type
                                   == qt3ds::runtime::DataInOutTypeVector3) {
                            // special handling for vector datatype to handle
                            // expansion from <propertyname> to <propertyname>.x .y .z
                            QVector<QByteArray> attVec;
                            bool success = getAttributeVector3(
                                attVec, ctrlElem.attributeName.first().constData(),
                                element);
                            if (!attVec.empty() && success) {
                                ctrlElem.attributeName = attVec;
                                ctrlElem.elementPath.append(element->path());
                                ctrlElem.propertyType = ATTRIBUTETYPE_FLOAT3;
                            } else {
                                qWarning() << __FUNCTION__ << "Property "
                                           << ctrlElem.attributeName.first()
                                           << " was not expanded to vector";
                                ctrlElem.propertyType = ATTRIBUTETYPE_NONE;
                            }
                        } else if (diMap[controllerName].type
                                   == qt3ds::runtime::DataInOutTypeVector2) {
                            // special handling for vector datatype to handle
                            // expansion from <propertyname> to <propertyname>.x .y
                            QVector<QByteArray> attVec;
                            bool success = getAttributeVector2(
                                attVec, ctrlElem.attributeName.first().constData(),
                                element);
                            if (!attVec.empty() && success) {
                                ctrlElem.attributeName = attVec;
                                ctrlElem.elementPath.append(element->path());
                                ctrlElem.propertyType = ATTRIBUTETYPE_FLOAT2;
                            } else {
                                qWarning() << __FUNCTION__ << "Property "
                                    << ctrlElem.attributeName.first()
                                    << " was not expanded to vector";
                                ctrlElem.propertyType = ATTRIBUTETYPE_NONE;
                            }
                        } else if (diMap[controllerName].type
                                   == qt3ds::runtime::DataInOutTypeEvaluator) {
                            diMap[controllerName].evalFunc
                                = buildJSFunc(diMap[controllerName].evaluator);
                            auto referencedDIs = resolveDependentDatainputs(
                                        diMap[controllerName].evaluator, controllerName);
                            // add this evaluator datainput to the dependent list
                            // for those datainputs that are used in the expression
                            // for this evaluator
                            for (auto ref : referencedDIs)
                                diMap[ref].dependents.append(controllerName);

                            ctrlElem.elementPath.append(element->path());
                            TStringHash attHash = CHash::HashAttribute(
                                QString(ctrlElem.attributeName.first()));
                            Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> attInfo
                                = element->FindProperty(attHash);
                            if (attInfo.hasValue())
                                ctrlElem.propertyType = attInfo->first.type();
                        } else {
                            // all other scalar datatypes
                            ctrlElem.elementPath.append(element->path());
                            TStringHash attHash = CHash::HashAttribute(
                                QString(ctrlElem.attributeName.first()));
                            Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> attInfo
                                = element->FindProperty(attHash);
                            if (attInfo.hasValue()) {
                                ctrlElem.propertyType = attInfo->first.type();
                            } else {
                                ctrlElem.propertyType = ATTRIBUTETYPE_NONE;
                                qWarning() << __FUNCTION__ << "Property "
                                           << ctrlElem.attributeName.first() << " not existing!";
                            }
                        }
                        qt3ds::runtime::DataInputDef &diDef = diMap[controllerName];
                        diDef.controlledAttributes.append(ctrlElem);

// #TODO: Remove below once QT3DS-3510 has been implemented in the editor
                        // Skip data output modification when adding dynamic elements,
                        // as that would break the previous binding
                        if (!isDynamicAdd) {
                            // Note, in this temp implementation only the LAST of multiple attrs
                            // will be notified from the object under the DataInput name..
                            qt3ds::runtime::DataInputDef inDef   = diMap[controllerName];
                            qt3ds::runtime::DataOutputDef &doDef = doMap[controllerName];
                            doDef.observedAttribute = ctrlElem;
                            doDef.type = inDef.type;
                            doDef.min  = inDef.min;
                            doDef.max  = inDef.max;

                            if (ctrlElem.propertyType == ATTRIBUTETYPE_DATAINPUT_TIMELINE) {
                                // Find the TElement for the @timeline attrib
                                TElement *target = nullptr;
                                QStringList split
                                        = QString(ctrlElem.elementPath).split(QLatin1Char(':'));
                                if (split.size() > 1) {
                                    target = CQmlElementHelper::GetElement(
                                                *m_Application,
                                                m_Application->LoadAndGetPresentationById(
                                                    split.at(0).toStdString().c_str()),
                                                split.at(1).toStdString().c_str(), nullptr);
                                } else {
                                    target = CQmlElementHelper::GetElement(
                                                *m_Application,
                                                m_Application->GetPrimaryPresentation(),
                                                split.at(0).toStdString().c_str(), nullptr);
                                }

                                doDef.timelineComponent = static_cast<TComponent *>(target);
                            } else if (ctrlElem.propertyType == ATTRIBUTETYPE_DATAINPUT_SLIDE) {
                                // Slide notifications are already done with separate signal
                                // No need to process
                            } else {
                                // Other than slide or timeline are handled by CPresentation
                                CRegisteredString rString = strTable.RegisterStr(
                                            ctrlElem.elementPath);
                                elementPathToDataOutputDefMap.insertMulti(rString, doDef);
                            }
                        }
// #TODO: Remove above once QT3DS-3510 has been implemented in the editor
                    }
                }
            }
        }
    }

// #TODO: Remove below once QT3DS-3510 has been implemented in the editor
    if (!isDynamicAdd)
        presentation.AddToDataOutputMap(elementPathToDataOutputDefMap);
// #TODO: Remove above once QT3DS-3510 has been implemented in the editor
}

void CQmlEngineImpl::initializeDataOutputsInPresentation(CPresentation &presentation,
                                                         bool isPrimary)
{
    TElement *parent = presentation.GetRoot();
    QList<TElement *> elements;
    listAllElements(parent, elements);
    qt3ds::runtime::DataOutputMap &doMap = m_Application->dataOutputMap();

    qt3ds::foundation::IStringTable &strTable(presentation.GetStringTable());
    QHash<CRegisteredString, qt3ds::runtime::DataOutputDef> elementPathToDataOutputDefMap;
    for (TElement *element : qAsConst(elements)) {
        Option<QT3DSU32> ctrlIndex = element->FindPropertyIndex(ATTRIBUTE_OBSERVEDPROPERTY);
        if (ctrlIndex.hasValue()) {
            Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> propertyInfo =
                    element->GetPropertyByIndex(*ctrlIndex);
            UVariant *valuePtr = propertyInfo->second;
            QString valueStr =
                    QString::fromUtf8(strTable.HandleToStr(valuePtr->m_StringHandle));

            if (!valueStr.isEmpty()) {
                QStringList splitValues = valueStr.split(QLatin1Char(' '));

                // Single value pair expected for DataOutputs
                QString observerName = splitValues[0];
                QString observedAttribute = splitValues[1];
                // Remove DataOutput name prefix "$"
                observerName.remove(0, 1);
                qt3ds::runtime::DataOutputDef &doDef = doMap[observerName];

                if (doMap.contains(observerName)) {
                    qt3ds::runtime::DataInOutAttribute obsElem;
                    if (!isPrimary) {
                        // Prepend presentation id to element path
                        obsElem.elementPath = presentation.GetName();
                        obsElem.elementPath.append(QByteArrayLiteral(":"));
                    }

                    obsElem.attributeName.append(observedAttribute.toUtf8());

                    if (obsElem.attributeName.first() == QByteArrayLiteral("@timeline")) {
                        // Timeline requires special additional handling
                        obsElem.propertyType = ATTRIBUTETYPE_DATAINPUT_TIMELINE;
                        TElement *component = &element->GetComponentParent();
                        obsElem.elementPath.append(component->path());

                        // Find the TElement for the @timeline attrib
                        TElement *target = nullptr;
                        QStringList split = QString(obsElem.elementPath).split(QLatin1Char(':'));
                        if (split.size() > 1) {
                            target = CQmlElementHelper::GetElement(
                                        *m_Application,
                                        m_Application->LoadAndGetPresentationById(
                                            split.at(0).toStdString().c_str()),
                                        split.at(1).toStdString().c_str(), nullptr);
                        } else {
                            target = CQmlElementHelper::GetElement(
                                        *m_Application,
                                        m_Application->GetPrimaryPresentation(),
                                        split.at(0).toStdString().c_str(), nullptr);
                        }
                        doDef.timelineComponent = static_cast<TComponent *>(target);
                    } else if (obsElem.attributeName.first() == QByteArrayLiteral("@slide")) {
                        // Slides are ignored if set as we have separate signal in the API for
                        // slide transitions
                    } else {
                        // Every other type is handled by CPresentation
                        obsElem.elementPath.append(element->path());
                        TStringHash attHash = CHash::HashAttribute(
                                    obsElem.attributeName.first().constData());
                        Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> attInfo
                                = element->FindProperty(attHash);
                        if (attInfo.hasValue()) {
                            obsElem.propertyType = attInfo->first.type();
                        } else {
                            obsElem.propertyType = ATTRIBUTETYPE_NONE;
                            qWarning() << __FUNCTION__ << "Property"
                                       << obsElem.attributeName.first() << "not existing!";
                        }

                        doDef.observedAttribute = obsElem;
                        CRegisteredString rString = strTable.RegisterStr(obsElem.elementPath);
                        elementPathToDataOutputDefMap.insertMulti(rString, doDef);
                    }
                }
            }
        }
    }

    // Inform the presentation of the ready data output defs
    presentation.AddToDataOutputMap(elementPathToDataOutputDefMap);
}

// Remove datainput control from listed elements and update internal control map. Should be used
// when elements having a datainput controller are removed in order to avoid invalid entries.
void CQmlEngineImpl::removeDataInputControl(const QVector<TElement *> &inElements)
{
    const qt3ds::runtime::DataInputMap diMap(m_Application->dataInputMap());

    // Have to do nasty triple-nested search loop as the datainput map is organized around
    // datainputs for fast lookups in setDataInputValue, not around control targets.
    QVector<QPair<QString, qt3ds::runtime::DataInOutAttribute>> elemAttrsToRemove;
    QMapIterator<QString, qt3ds::runtime::DataInputDef> diMapIter(diMap);

    for (const auto &elem : inElements) {
        while (diMapIter.hasNext()) {
            diMapIter.next();
            for (const auto &inOutAttr : qAsConst(diMapIter.value().controlledAttributes)) {
                if (inOutAttr.elementPath == QByteArray(elem->path()))
                    elemAttrsToRemove.append({diMapIter.key(), inOutAttr});
            }
        }
        diMapIter.toFront();
    }

    for (const auto &attr : qAsConst(elemAttrsToRemove))
        m_Application->dataInputMap()[attr.first].controlledAttributes.removeAll(attr.second);
}

// Bit clumsy way of getting from "position" to "position .x .y .z" and enabling datainput
// support for vectorized types. UIP parser has already thrown away all vector
// type attributes and at this point we are operating with scalar components only.
// We check if this element has a property attName.x or attName.r to find out it
// we should expand property attName to XYZ or RGB vector
bool CQmlEngineImpl::getAttributeVector4(QVector<QByteArray> &outAttVec,
                                         const QByteArray &attName,
                                         TElement *elem)
{
    auto hashName = Q3DStudio::CHash::HashAttribute(QString(attName + ".x"));

    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".x");
        outAttVec.append(attName + ".y");
        outAttVec.append(attName + ".z");
        outAttVec.append(attName + ".w");
        return true;
    }
    hashName = Q3DStudio::CHash::HashAttribute(QString(attName + ".r"));
    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".r");
        outAttVec.append(attName + ".g");
        outAttVec.append(attName + ".b");
        outAttVec.append(attName + ".a");
        return true;
    }
    return false;
}

bool CQmlEngineImpl::getAttributeVector3(QVector<QByteArray> &outAttVec,
                                         const QByteArray &attName,
                                         TElement *elem)
{
    auto hashName = Q3DStudio::CHash::HashAttribute(QString(attName + ".x"));

    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".x");
        outAttVec.append(attName + ".y");
        outAttVec.append(attName + ".z");
        return true;
    }
    hashName = Q3DStudio::CHash::HashAttribute(QString(attName + ".r"));
    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".r");
        outAttVec.append(attName + ".g");
        outAttVec.append(attName + ".b");
        return true;
    }
    return false;
}

bool CQmlEngineImpl::getAttributeVector2(QVector<QByteArray> &outAttVec,
                                         const QByteArray &attName,
                                         TElement *elem)
{
    auto hashName = Q3DStudio::CHash::HashAttribute(QString(attName + ".x"));

    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".x");
        outAttVec.append(attName + ".y");
        return true;
    }

    hashName = Q3DStudio::CHash::HashAttribute(QString(attName + ".u"));
    if (!elem->FindProperty(hashName).isEmpty()) {
        outAttVec.append(attName + ".u");
        outAttVec.append(attName + ".v");
        return true;
    }
    return false;
}

QJSValue CQmlEngineImpl::buildJSFunc(const QString &userFunc)
{
    auto res = this->m_engine->evaluate(userFunc);
    if (res.isError()) {
        qWarning() << __FUNCTION__
            << "Uncaught exception during datainput evaluation. Evaluator function" << userFunc;
    }
    return res;
}

QVariant CQmlEngineImpl::callJSFunc(const QString &controllerName,
                                    qt3ds::runtime::DataInputDef &diDef,
                                    const QVariant::Type type)
{
    qt3ds::runtime::DataInputMap &diMap = m_Application->dataInputMap();
    QJSValueList args;
    QVector<QString> sourceDIs = resolveDependentDatainputs(diDef.evaluator, controllerName);

    // get the most recent set values for datainput sources (arguments) in the expression
    for (auto diVal : sourceDIs)
        args << this->m_engine->toScriptValue(diMap[diVal].value);

    if (diDef.evalFunc.isCallable()) {
        QJSValue res = diDef.evalFunc.call(args);
        if (res.isError()) {
            qWarning() << __FUNCTION__ << "Error during datainput" << controllerName
                       << "evaluator call:" << res.toString() << "\nEvaluator function"
                       << diDef.evaluator;
            return QVariant::Invalid;
        }

        QVariant ret = res.toVariant();
        if (ret.isValid() && isMatchingDatatype(ret.type(), type)) {
            // further check if the result is valid number
            if (ret.type() == QVariant::Double && qIsNaN(res.toNumber())) {
                qWarning() << __FUNCTION__ << "Datainput" << controllerName << "evaluator"
                           << "result not a number (NaN)."
                           << "\nEvaluator function" << diDef.evaluator;
                return QVariant::Invalid;
            } else {
                return ret;
            }
        } else {
            qWarning() << __FUNCTION__ << "Datainput" << controllerName << "evaluator"
                       << "result not valid or matching with target attribute type. Result type"
                       << QVariant::typeToName(ret.type()) << " target attribute type "
                       << QVariant::typeToName(type) << "\nEvaluator function" << diDef.evaluator;
        }
    } else {
        qWarning() << __FUNCTION__ << "Datainput" << controllerName << "evaluator"
                   << diDef.evaluator << " not valid callable";
    }
    return QVariant::Invalid;
}

bool CQmlEngineImpl::isMatchingDatatype(QVariant::Type resultType, QVariant::Type propertyType)
{
    if (resultType == propertyType)
        return true;
    // Allow binding from numeric datainput to string target
    if ((resultType == QVariant::Double || resultType == QVariant::Int
         || resultType == QVariant::LongLong)
        && (propertyType == QVariant::Double || propertyType == QVariant::Int
            || propertyType == QVariant::LongLong || propertyType == QVariant::String)) {
        return true;
    }
    return false;
}

QVector<QString> CQmlEngineImpl::resolveDependentDatainputs(const QString &expression,
                                                            const QString &controllerName)
{
    QVector<QString> ret;
    qt3ds::runtime::DataInputMap &diMap = m_Application->dataInputMap();
    if (!expression.contains("function", Qt::CaseInsensitive)) {
        qWarning() << __FUNCTION__ << "Function keyword not found in datainput"
                   << controllerName << "evaluator";
        return QVector<QString>();
    }

    int argListStart = expression.indexOf("function(") + 9;
    int argListStop = expression.indexOf(')', argListStart);
    QString argstr = expression.mid(argListStart , argListStop - argListStart);
    QStringList args = argstr.split(',');

    for (auto di : args) {
        auto diTrim = di.trimmed();
        if (diMap.contains(diTrim)) {
            if (diMap[diTrim].type == qt3ds::runtime::DataInOutTypeEvaluator
                && diTrim != controllerName) {
                qWarning() << __FUNCTION__ << "Invalid evaluator function in" << controllerName
                           << ". Another evaluator is used as source data.";
            } else {
                ret.append(diTrim);
            }
        } else {
            qWarning() << __FUNCTION__ << "Evaluator in" << controllerName << "evaluator"
                       << "is using unknown datainput" << diTrim << " as input argument name";
        }
    }
    return ret;
}

void CQmlEngineImpl::addStringAttribute(IStringTable &strTable,
                                        CQmlEngineImpl::TPropertyDescAndValueList &list,
                                        const QString &inAttName, const QString &inValue)
{
    QByteArray valueBa = inValue.toUtf8();
    qt3ds::foundation::CStringHandle strHandle = strTable.GetHandle(valueBa.constData());
    UVariant theValue;
    theValue.m_StringHandle = strHandle.handle();
    const CRegisteredString attStr = strTable.RegisterStr(inAttName);
    list.push_back(eastl::make_pair(TPropertyDesc(attStr, ATTRIBUTETYPE_STRING), theValue));
}

void CQmlEngineImpl::addIntAttribute(IStringTable &strTable,
                                     CQmlEngineImpl::TPropertyDescAndValueList &list,
                                     const QString &inAttName, int inValue)
{
    UVariant theValue;
    theValue.m_INT32 = inValue;
    const CRegisteredString attStr = strTable.RegisterStr(inAttName);
    list.push_back(eastl::make_pair(TPropertyDesc(attStr, ATTRIBUTETYPE_INT32), theValue));
}

void CQmlEngineImpl::addBoolAttribute(IStringTable &strTable,
                                      CQmlEngineImpl::TPropertyDescAndValueList &list,
                                      const QString &inAttName, bool inValue)
{
    UVariant theValue;
    theValue.m_INT32 = int(inValue);
    const CRegisteredString attStr = strTable.RegisterStr(inAttName);
    list.push_back(eastl::make_pair(TPropertyDesc(attStr, ATTRIBUTETYPE_BOOL), theValue));
}

void CQmlEngineImpl::addFloatAttribute(IStringTable &strTable,
                                       CQmlEngineImpl::TPropertyDescAndValueList &list,
                                       const QString &inAttName, float inValue)
{
    UVariant theValue;
    theValue.m_FLOAT = inValue;
    const CRegisteredString attStr = strTable.RegisterStr(inAttName);
    list.push_back(eastl::make_pair(TPropertyDesc(attStr, ATTRIBUTETYPE_FLOAT), theValue));
}

void CQmlEngineImpl::addFloat2Attribute(IStringTable &strTable,
                                        CQmlEngineImpl::TPropertyDescAndValueList &list,
                                        const QStringList &inAttNames, const QVector2D &inValue)
{
    for (int i = 0; i < 2; ++i) {
        UVariant theValue;
        theValue.m_FLOAT = inValue[i];
        const CRegisteredString attStr = strTable.RegisterStr(inAttNames.at(i));
        list.push_back(eastl::make_pair(TPropertyDesc(attStr, ATTRIBUTETYPE_FLOAT), theValue));
    }
}

void CQmlEngineImpl::addFloat3Attribute(IStringTable &strTable,
                                        CQmlEngineImpl::TPropertyDescAndValueList &list,
                                        const QStringList &inAttNames, const QVector3D &inValue)
{
    for (int i = 0; i < 3; ++i) {
        UVariant theValue;
        theValue.m_FLOAT = inValue[i];
        const CRegisteredString attStr = strTable.RegisterStr(inAttNames.at(i));
        list.push_back(eastl::make_pair(TPropertyDesc(attStr, ATTRIBUTETYPE_FLOAT), theValue));
    }
}

void CQmlEngineImpl::addFloat4Attribute(IStringTable &strTable,
                                        CQmlEngineImpl::TPropertyDescAndValueList &list,
                                        const QStringList &inAttNames, const QVector4D &inValue)
{
    for (int i = 0; i < 4; ++i) {
        UVariant theValue;
        theValue.m_FLOAT = inValue[i];
        const CRegisteredString attStr = strTable.RegisterStr(inAttNames.at(i));
        list.push_back(eastl::make_pair(TPropertyDesc(attStr, ATTRIBUTETYPE_FLOAT), theValue));
    }
}

void CQmlEngineImpl::addElementRefAttribute(IStringTable &strTable,
                                            CQmlEngineImpl::TPropertyDescAndValueList &list,
                                            const QString &inAttName, TElement *element)
{
    UVariant theValue;
    if (element) {
        theValue.m_ElementHandle = element->GetHandle();
        QT3DS_ASSERT(theValue.m_ElementHandle);
    } else {
        theValue.m_ElementHandle = 0;
    }
    const CRegisteredString attStr = strTable.RegisterStr(inAttName);
    list.push_back(eastl::make_pair(TPropertyDesc(attStr, ATTRIBUTETYPE_ELEMENTREF), theValue));
}

QVector2D CQmlEngineImpl::parseFloat2Property(const QString &propValue)
{
    QVector<QStringRef> values = propValue.splitRef(QLatin1Char(' '));
    QVector2D retVal;
    for (int i = 0; i < values.size() && i < 2; ++i)
        retVal[i] = values[i].toFloat();
    return retVal;
}

QVector3D CQmlEngineImpl::parseFloat3Property(const QString &propValue)
{
    QVector<QStringRef> values = propValue.splitRef(QLatin1Char(' '));
    QVector3D retVal;
    for (int i = 0; i < values.size() && i < 3; ++i)
        retVal[i] = values[i].toFloat();
    return retVal;
}

QVector4D CQmlEngineImpl::parseFloat4Property(const QString &propValue)
{
    QVector<QStringRef> values = propValue.splitRef(QLatin1Char(' '));
    QVector4D retVal;
    for (int i = 0; i < values.size() && i < 4; ++i)
        retVal[i] = values[i].toFloat();
    return retVal;
}

void CQmlEngineImpl::notifyElementCreation(const QStringList &elementNames, const QString &error)
{
    // Notify presentation asynchronously to give renderer time to initialize the elements properly
    if (!error.isEmpty()) {
        qWarning() << "Warning: Element creation failed:" << error;
        QT3DS_ASSERT(false);
    }
    QTimer::singleShot(0, [this, elementNames, error]() {
        m_Application->GetPrimaryPresentation()->signalProxy()
                ->SigElementsCreated(elementNames, error);
    });
}

void CQmlEngineImpl::notifyMaterialCreation(const QStringList &materialNames, const QString &error)
{
    // Notify presentation asynchronously to give renderer time to initialize the materials properly
    if (!error.isEmpty()) {
        qWarning() << "Warning: Material creation failed:" << materialNames << error;
        QT3DS_ASSERT(false);
    }
    QTimer::singleShot(0, [this, materialNames, error]() {
        m_Application->GetPrimaryPresentation()->signalProxy()
                ->SigMaterialsCreated(materialNames, error);
    });
}

void CQmlEngineImpl::deleteElements(const QVector<TElement *> &elements,
                                    IQt3DSRenderer *renderer)
{
    TElement *lastParent = nullptr;
    IPresentation *presentation = nullptr;
    TElement *component = nullptr;
    ISlideSystem *slideSystem = nullptr;
    QSet<qt3ds::render::SNode *> parentNodes;
    for (auto element : elements) {
        TElement *parentElement = element->GetParent();
        if (parentElement != lastParent) {
            lastParent = parentElement;
            presentation = element->GetBelongedPresentation();
            component = &element->GetComponentParent();
            slideSystem = &presentation->GetSlideSystem();
        }
        // Remove element recursively from slide system
        slideSystem->removeElement(*component, *element);

        Q_ASSERT(parentElement);

        NVAllocatorCallback &allocator = presentation->GetScene()->allocator();

        // Recursive deleter for translators and graph objects
        std::function<void(TElement *)> deleteRenderObjects;
        deleteRenderObjects = [&](TElement *elem)  {
            if (m_elementIdMap.contains(elem))
                releaseId(m_elementIdMap.take(elem));

            const auto children(elem->children());
            for (auto child : children)
                deleteRenderObjects(child);

            auto translator = static_cast<qt3ds::render::Qt3DSTranslator *>(elem->GetAssociation());
            if (translator) {
                qt3ds::render::GraphObjectTypes::Enum type = translator->GetUIPType();
                if (type == qt3ds::render::GraphObjectTypes::Model) {
                    auto model = static_cast<qt3ds::render::SModel *>(&translator->RenderObject());
                    // Delete material
                    if (model->m_FirstMaterial) {
                        auto material = static_cast<qt3ds::render::SReferencedMaterial *>(
                                    model->m_FirstMaterial);
                        QT3DS_FREE(allocator, material);
                    }
                } else if (type == qt3ds::render::GraphObjectTypes::Image) {
                    auto image = static_cast<qt3ds::render::SImage *>(&translator->RenderObject());
                    if (image->m_LoadedTextureData)
                        image->m_LoadedTextureData->m_callbacks.removeOne(image);
                }
                QT3DS_FREE(allocator, &translator->RenderObject());
                QT3DS_FREE(allocator, translator);
            }
        };

        qt3ds::render::SNode *node = nullptr;
        qt3ds::render::SNode *parentNode = nullptr;
        auto translator = static_cast<qt3ds::render::Qt3DSTranslator *>(element->GetAssociation());

        if (translator) {
            if (qt3ds::render::GraphObjectTypes::IsNodeType(translator->GetUIPType())) {
                node = &static_cast<qt3ds::render::SNode &>(translator->RenderObject());
                auto parentTranslator = static_cast<qt3ds::render::Qt3DSTranslator *>(
                            parentElement->GetAssociation());
                if (parentTranslator) {
                    parentNode = &static_cast<qt3ds::render::SNode &>(
                                parentTranslator->RenderObject());
                    parentNode->RemoveChild(*node);
                    parentNodes.insert(parentNode);
                }
            }
            // Release child element graph objects/translators
            deleteRenderObjects(element);
        }

        // Remove element recursively
        m_Application->GetElementAllocator().ReleaseElement(*element, true);
    }
    for (auto parentNode : qAsConst(parentNodes))
        renderer->ChildrenUpdated(*parentNode);

    removeDataInputControl(elements);
}

int CQmlEngineImpl::getAvailableId()
{
    static int _idCounter = 0;

    int id = -1;
    if (m_availableIds.isEmpty()) {
        id = ++_idCounter;
    } else {
        auto first = m_availableIds.begin();
        id = *first;
        m_availableIds.erase(first);
    }

    return id;
}

void CQmlEngineImpl::releaseId(int id)
{
    m_availableIds.insert(id);
}

template<typename TDataType>
void CQmlEngineImpl::setDynamicObjectProperty(qt3ds::render::SDynamicObject &material,
                                              const dynamic::SPropertyDefinition &propDesc,
                                              const TDataType &propValue)
{
    memCopy(material.GetDataSectionBegin() + propDesc.m_Offset, &propValue, sizeof(TDataType));
}

/**
* @brief Create QML engine
*
* @param[in] inFoundation		Pointer to foundation
* @param[in] inTimeProvider		Pointer to time provider
*
* @return  no return
*/
CQmlEngine *CQmlEngine::Create(qt3ds::NVFoundationBase &inFoundation, ITimeProvider &inTimeProvider)
{
    return QT3DS_NEW(inFoundation.getAllocator(), CQmlEngineImpl)(inFoundation, inTimeProvider);
}

}
