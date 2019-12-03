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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSRuntimeView.h"
#include "Qt3DSTegraInputEngine.h"
#include "Qt3DSDataLogger.h"
#include "Qt3DSFileStream.h"
#include "Qt3DSArray.h"
#include "Qt3DSApplication.h"
#include "foundation/FileTools.h"
#include "Qt3DSIPresentation.h"
#include "Qt3DSPresentation.h"
#include "EASTL/string.h"
#include "Qt3DSMemory.h"
#include "Qt3DSKernelTypes.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderer.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSRenderRuntimeBindingImpl.h"
#include "Qt3DSImportMesh.h"

#include "Qt3DSDLLManager.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/TrackingAllocator.h"
// For perf log timestamp
#include <time.h>
#include "Qt3DSArray.h"
// For perf log timestamp
#include <time.h>

#ifdef _LINUXPLATFORM
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#include <QCoreApplication>
#include <QSurfaceFormat>

//==============================================================================
//	Namespace
//==============================================================================
namespace qt3ds {
namespace render {
extern qt3ds::foundation::MallocAllocator g_BaseAllocator;
}
}

namespace Q3DStudio {

namespace {

bool CaselessEqual(const char *lhs, const char *rhs)
{
    if (lhs == nullptr)
        lhs = "";
    if (rhs == nullptr)
        rhs = "";
    return Q3DStudio_stricmp(lhs, rhs) == 0;
}

CInputEngine *CreateInputEngine()
{
    return Q3DStudio_virtual_new(CTegraInputEngine) CTegraInputEngine();
}

static eastl::string *theAppDir = nullptr;
const eastl::string &GetAppDir(const eastl::string &inAppExe)
{
    if (!theAppDir)
        theAppDir = new eastl::string;
#ifndef __ANDROID__
    theAppDir->assign(inAppExe.empty() == false ? inAppExe : "");
#ifdef Qt3DS_OS_LINUX
    char theBuf[1024] = { 0 };
    int rc = readlink("/proc/self/exe", theBuf, sizeof(theBuf));
    if (rc > 0)
        theAppDir->assign(theBuf);
#endif
#ifdef Qt3DS_OS_QNX
    char theBuf[1024] = { 0 };
    FILE *exefile = fopen("/proc/self/exefile", "r");
    if (exefile != nullptr) {
        fgets(theBuf, sizeof(theBuf), exefile);
        fclose(exefile);
        theAppDir->assign(theBuf);
    }
#endif
    eastl::string::size_type pos = theAppDir->find_last_of("\\/");
    if (pos != eastl::string::npos)
        *theAppDir = theAppDir->substr(0, pos);
    theAppDir->append("\\");
#endif
    return *theAppDir;
}
}

using namespace qt3ds;
using namespace qt3ds::foundation;

class CRuntimeView : public IRuntimeView
{
    //==============================================================================
    //	Fields
    //==============================================================================
private:
    ITegraApplicationRenderEngine *m_RenderEngine; ///< Handles all rendering functions
    CTegraInputEngine *m_InputEngine; ///< Handles all user input events
    // Pre graphics init objects
    NVScopedRefCounted<qt3ds::render::IQt3DSRenderFactoryCore> m_RuntimeFactoryCore;
    ///< Base application before graphics
    NVScopedRefCounted<qt3ds::runtime::IApplication> m_ApplicationCore;

    // Post graphics init objects
    NVScopedRefCounted<qt3ds::render::IQt3DSRenderFactory> m_RuntimeFactory;
    NVScopedRefCounted<qt3ds::runtime::IApplication> m_Application; ///< Application after graphics
    ///< Currently loaded presentation, this should be removed in the future
    CPresentation *m_Presentation;

    CPausingTimeProvider m_TimeProvider;
    IWindowSystem &m_WindowSystem;
    IAudioPlayer *m_AudioPlayer;

    volatile QT3DSI32 mRefCount;

    qt3ds::Qt3DSAssetVisitor *m_visitor;
    bool m_showOnScreenStats;
    QElapsedTimer *m_startupTimer;
    qint64 m_startupTime;

public:
    CRuntimeView(ITimeProvider &inTimeProvider, IWindowSystem &inWindowSystem,
                 IAudioPlayer *inAudioPlayer, QElapsedTimer *startupTimer);
    ~CRuntimeView() override;

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(qt3ds::render::g_BaseAllocator)

    bool BeginLoad(const QString &sourcePath, const QStringList &variantList) override;
    bool HasOfflineLoadingCompleted() override;
    bool InitializeGraphics(const QSurfaceFormat &format, bool delayedLoading,
                            bool initInRenderThread, const QByteArray &shaderCache) override;
    void connectSignals() override;
    void finishAsyncInit() override;

    void Cleanup() override;

    bool CanRender() override;
    void Render() override;
    bool WasLastFrameDirty() override;

    bool HandleMessage(const QEvent *inEvent) override;

    void Pause() override;
    void UnPause() override;
    bool IsPaused() override;
    void setAssetVisitor(qt3ds::Qt3DSAssetVisitor *) override;

    INT32 GetFrameCount() override;
    void showOnScreenStats(bool) override;

    CInputEngine *GetInputEngine() override;
    // Only valid after InitializeGraphics
    ITegraApplicationRenderEngine *GetTegraRenderEngine() override { return m_RenderEngine; }

    void GoToSlideByName(const char *elementPath, const char *slideName) override;
    void GoToSlideByIndex(const char *elementPath, const int slideIndex) override;
    void GoToSlideRelative(const char *elementPath, const bool next, const bool wrap) override;
    bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                      QString &currentName, QString &previousName) override;
    void SetPresentationAttribute(const char *presId, const char *, const char *value) override;
    void GoToTime(const char *elementPath, const float time) override;
    void SetGlobalAnimationTime(qint64 inMilliSecs) override;
    void SetDataInputValue(const QString &name, const QVariant &value,
                           int property) override;

    void setPresentationId(const QString &id) override
    {
        m_Application->setPresentationId(id);
    }

    QList<QString> dataInputs() const override;
    QList<QString> dataOutputs() const override;
    float dataInputMax(const QString &name) const override;
    float dataInputMin(const QString &name) const override;
    QHash<QString, QString> dataInputMetadata(const QString &name) const override;

    void createElements(const QString &parentElementPath, const QString &slideName,
                        const QVector<QHash<QString, QVariant>> &properties) override;
    void deleteElements(const QStringList &elementPaths) override;
    void createMaterials(const QString &subPresId,
                         const QStringList &materialDefinitions) override;
    void deleteMaterials(const QStringList &materialNames) override;
    void createMesh(const QString &name, qt3dsimp::Mesh *mesh) override;
    void deleteMeshes(const QStringList &meshNames) override;
    void addImageProvider(const QString &providerId, QQmlImageProviderBase *provider) override;
    uint textureId(const QString &elementPath) override;
    void SetAttribute(const char *elementPath, const char *attributeName,
                      const char *value) override;
    bool GetAttribute(const char *elementPath, const char *attributeName, void *value) override;
    void FireEvent(const char *element, const char *evtName) override;
    bool PeekCustomAction(char *&outElementPath, char *&outActionName) override;
    bool RegisterScriptCallback(int callbackType, qml_Function func, void *inUserData) override;
    void FireEvent(const TEventCommandHash inEventType, eastl::string inArgument) override;
    qt3ds::foundation::Option<SPresentationSize> GetPresentationSize() override;
    void preloadSlide(const QString &slide) override;
    void unloadSlide(const QString &slide) override;
    void setDelayedLoading(bool enable) override;
    void BootupPreGraphicsInitObjects();
    QByteArray exportShaderCache(bool binaryShaders);
};

CRuntimeView::CRuntimeView(ITimeProvider &inTimeProvider, IWindowSystem &inWindowSystem,
                           IAudioPlayer *inAudioPlayer, QElapsedTimer *startupTimer)
    : m_RenderEngine(nullptr)
    , m_InputEngine(nullptr)
    , m_Application(nullptr)
    , m_Presentation(nullptr)
    , m_TimeProvider(inTimeProvider)
    , m_WindowSystem(inWindowSystem)
    , m_AudioPlayer(inAudioPlayer)
    , mRefCount(0)
    , m_visitor(nullptr)
    , m_showOnScreenStats(false)
    , m_startupTimer(startupTimer)
    , m_startupTime(-1)
{
    // Signal proxy thread affinity is set later when signals are connected to ensure it is correct
    signalProxy()->moveToThread(nullptr);
}

CRuntimeView::~CRuntimeView()
{
}

bool CRuntimeView::BeginLoad(const QString &sourcePath, const QStringList &variantList)
{
    bool theResult = false;
    m_startupTime = -1;

    // boot up the application
    BootupPreGraphicsInitObjects();

    // If there was a presentation file then we have to load it or something failed.
    if (m_ApplicationCore->BeginLoad(sourcePath.toUtf8(), variantList))
        theResult = true;
    else
        theResult = false;

    // If Initialize wasn't successful - this means the CShaderFactory failed to initialize
    // or, the presentation failed to load.
    //
    // NOTE: if no presentation was passed, this is 'ok'
    if (!theResult)
        Cleanup();

    return theResult;
}

bool CRuntimeView::HasOfflineLoadingCompleted()
{
    if (m_Application.mPtr == nullptr) {
        if (m_ApplicationCore)
            return m_ApplicationCore->HasCompletedLoading();
        else
            return false;
    }
    return true;
}

bool CRuntimeView::InitializeGraphics(const QSurfaceFormat &format, bool delayedLoading,
                                      bool initInRenderThread, const QByteArray &shaderCache)
{
    m_ApplicationCore->EndLoad();
    // Next call will initialize the render portion of the scenes.  This *must* have a loaded
    // application to go further as it will bind scene graph data to application data.
    m_RuntimeFactory = m_RuntimeFactoryCore->CreateRenderFactory(format, delayedLoading);
    m_Application
            = m_ApplicationCore->CreateApplication(*m_InputEngine, m_AudioPlayer,
                                                   *m_RuntimeFactory, shaderCache,
                                                   initInRenderThread);
    if (!m_Application->createSuccessful())
        return false;

    m_Application->ResetTime();
    m_RenderEngine = &m_RuntimeFactory->CreateRenderEngine();
    m_Presentation = m_Application->GetPrimaryPresentation();

    m_TimeProvider.Reset();
    return true;
}

void CRuntimeView::connectSignals()
{
    m_Presentation->signalProxy()->moveToThread(QThread::currentThread());
    signalProxy()->moveToThread(QThread::currentThread());

    QObject::connect(m_Presentation->signalProxy(), &QPresentationSignalProxy::SigSlideEntered,
                     signalProxy(), &QRuntimeViewSignalProxy::SigSlideEntered);
    QObject::connect(m_Presentation->signalProxy(), &QPresentationSignalProxy::SigSlideExited,
                     signalProxy(), &QRuntimeViewSignalProxy::SigSlideExited);
    QObject::connect(m_Presentation->signalProxy(), &QPresentationSignalProxy::SigCustomSignal,
                     signalProxy(), &QRuntimeViewSignalProxy::SigCustomSignal);
    QObject::connect(m_Presentation->signalProxy(),
                     &QPresentationSignalProxy::SigPresentationReady,
                     signalProxy(), &QRuntimeViewSignalProxy::SigPresentationReady);
    QObject::connect(m_Presentation->signalProxy(), &QPresentationSignalProxy::SigElementsCreated,
                     signalProxy(), &QRuntimeViewSignalProxy::SigElementsCreated);
    QObject::connect(m_Presentation->signalProxy(), &QPresentationSignalProxy::SigMaterialsCreated,
                     signalProxy(), &QRuntimeViewSignalProxy::SigMaterialsCreated);
    QObject::connect(m_Presentation->signalProxy(),
                     &QPresentationSignalProxy::SigDataOutputValueUpdated,
                     signalProxy(),
                     &QRuntimeViewSignalProxy::SigDataOutputValueUpdated);
}

void CRuntimeView::finishAsyncInit()
{
    Q3DStudio::CQmlEngine &bridgeEngine
            = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
    bridgeEngine.loadDeferredScripts();
}

void CRuntimeView::Cleanup()
{
    // Q3DStudio_virtual_delete( m_Timer, CTimer );
    // Q3DStudio_virtual_delete( m_PerfFileStream, CFileStream );
    m_Application = nullptr;
    Q3DStudio_virtual_delete(m_InputEngine, CTegraInputEngine);
    if (m_RenderEngine) {
        m_RenderEngine->Release();
        m_RenderEngine = nullptr;
    }

    CDLLManager &theDLLManager = CDLLManager::GetDLLManager();
    theDLLManager.Cleanup();
    if (m_Presentation)
        QObject::disconnect(m_Presentation->signalProxy(), 0, signalProxy(), 0);

    m_InputEngine = nullptr;
    m_RenderEngine = nullptr;
    m_Presentation = nullptr;
}

bool CRuntimeView::CanRender()
{
    return m_Application.mPtr != nullptr;
}

//==============================================================================
/**
 *	nv_main APP-SPECIFIC rendering call
 *  returns KD_TRUE to call egl_render and swap properly, KD_FALSE if there has been no scene update
 *or redraw.
 */
void CRuntimeView::Render()
{
    if (m_Application.mPtr == nullptr) {
        // InitializeGraphics has not been called
        QT3DS_ASSERT(false);
    }

    PerfLogGeneralEvent1(DATALOGGER_FRAME);

    m_Application->UpdateAndRender();

    if (m_startupTime < 0 && m_startupTimer && m_startupTimer->isValid()) {
        m_startupTime = m_startupTimer->elapsed();
        m_startupTimer->invalidate();
        qCDebug(PERF_INFO, "RuntimeView: First frame at - %dms", m_startupTime);
    }

    if (m_showOnScreenStats) {
        ITegraRenderStateManager &manager
                = GetTegraRenderEngine()->GetTegraRenderStateManager();
        manager.PushState();

        QSize dim = m_WindowSystem.GetWindowDimensions();
        manager.SetScissorTestEnabled(false);
        manager.SetViewport(0, 0, dim.width(), dim.height());

        QPair<QT3DSF32, QT3DSF32> fps
                = m_RuntimeFactory->GetQt3DSRenderContext().GetFPS();
        const QVector<QT3DSF32> times = m_RuntimeFactory->GetQt3DSRenderContext().GetFrameTimes();

        QString text;
        QTextStream stream(&text);
        stream << QStringLiteral("Render Statistics: ");
        stream << QString::number(fps.first, 'f', 2);
        stream << " fps, frame count ";
        stream << QString::number(fps.second);
        stream << ", frame time ";
        stream << QString::number(times[0], 'f', 2);
        stream << " ms";
        if (m_startupTime) {
            stream << ", Init time: ";
            stream << QString::number(m_startupTime);
            stream << " ms";
        }

        // bottom left coordinates
        GetTegraRenderEngine()->RenderText2D(
                    dim.width() / 4, dim.height() - 25, qt3ds::QT3DSVec3(0.0, 1.0, 0.0),
                    text.toLatin1().constData());
        GetTegraRenderEngine()->RenderGpuProfilerStats(
                    20.0, dim.height() - 80, qt3ds::QT3DSVec3(0.0, 1.0, 0.0));

        manager.PopState();
    }
}

bool CRuntimeView::WasLastFrameDirty()
{
    if (m_Application)
        return m_Application->IsApplicationDirty();
    return false;
}

//==============================================================================
/**
 *	nv_main APP-SPECIFIC message call
 *	HandleMessage
 */
bool CRuntimeView::HandleMessage(const QEvent *inEvent)
{
    if (m_Application.mPtr == nullptr || m_RenderEngine == nullptr)
        return 0;

    bool ret = false;
    switch (inEvent->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        {
            const QMouseEvent *event = static_cast<const QMouseEvent *>(inEvent);
            m_InputEngine->SetPickInput(static_cast<FLOAT>(event->x()),
                                        static_cast<FLOAT>(event->y()), true);
            m_InputEngine->SetPickFlags(inEvent->type() == QEvent::MouseButtonPress
                                        ? LMOUSE_DOWN : LMOUSE_UP);
            ret = true;
        }
        break;
    case QEvent::Resize:
        {
            if (m_Application->GetPrimaryPresentation())
                m_RenderEngine->CheckResize(true, *m_Application->GetPrimaryPresentation());
            ret = true;
        }
        break;
    default:
        break;
    }

    m_InputEngine->HandleMessage(inEvent, *m_RenderEngine, m_Application->GetPrimaryPresentation());
    return ret ? 1 : 0;
}

void CRuntimeView::Pause()
{
    m_TimeProvider.Pause();
}

void CRuntimeView::UnPause()
{
    m_TimeProvider.UnPause();
}

bool CRuntimeView::IsPaused()
{
    return m_TimeProvider.IsPaused();
}

INT32 CRuntimeView::GetFrameCount()
{
    return m_Application->GetFrameCount();
}

void CRuntimeView::showOnScreenStats(bool show)
{
    m_showOnScreenStats = show;
    m_RuntimeFactory->GetQt3DSRenderContext().GetRenderer().EnableLayerGpuProfiling(show);
}

CInputEngine *CRuntimeView::GetInputEngine()
{
    return m_InputEngine;
}

//==============================================================================
/**
 *	Generates an event in the presentation.
 */
void CRuntimeView::GoToSlideByName(const char *elementPath, const char *slideName)
{
    if (m_Application) {
        if (!elementPath || !slideName)
            return;

        CPresentation *thePresentation = m_Application->GetPrimaryPresentation();
        IScriptBridge *theBridge = thePresentation->GetScriptBridgeQml();

        if (!theBridge)
            return;

        theBridge->GotoSlide(elementPath, slideName, SScriptEngineGotoSlideArgs());
    }
}

void CRuntimeView::GoToSlideByIndex(const char *elementPath, const int slideIndex)
{
    if (m_Application) {
        if (!elementPath || slideIndex < 0)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.GotoSlideIndex(elementPath, slideIndex, SScriptEngineGotoSlideArgs());
    }
}

void CRuntimeView::GoToSlideRelative(const char *elementPath, const bool next, const bool wrap)
{
    if (m_Application) {
        if (!elementPath)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.GotoSlideRelative(elementPath, next, wrap, SScriptEngineGotoSlideArgs());
    }
}

bool CRuntimeView::GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                            QString &currentName, QString &previousName)
{
    if (m_Application && elementPath) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        return theBridgeEngine.GetSlideInfo(elementPath, currentIndex, previousIndex,
                                            currentName, previousName);
    }
    return false;
}

void CRuntimeView::SetPresentationAttribute(const char *presId, const char *, const char *value)
{
    if (m_Application) {
        if (!presId || !value)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.SetPresentationAttribute(presId, nullptr, value);
    }
}

bool CRuntimeView::RegisterScriptCallback(int callbackType, qml_Function func, void *inUserData)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        return theBridgeEngine.RegisterCallback(callbackType, func, inUserData);
    }

    return false;
}

void CRuntimeView::GoToTime(const char *elementPath, const float time)
{
    if (m_Application) {
        if (!elementPath || time < 0.0)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.GotoTime(elementPath, time);
    }
}

void CRuntimeView::SetGlobalAnimationTime(qint64 inMilliSecs)
{
    if (m_Application)
        m_Application->SetTimeMilliSecs(inMilliSecs);
}

void CRuntimeView::SetDataInputValue(
        const QString &name, const QVariant &value,
        int property = 0)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
    theBridgeEngine.SetDataInputValue(name, value, (qt3ds::runtime::DataInputValueRole)property);
    }
}

QList<QString> CRuntimeView::dataInputs() const
{
    if (m_Application)
        return m_Application->dataInputs();

    return {};
}

QList<QString> CRuntimeView::dataOutputs() const
{
    if (m_Application)
        return m_Application->dataOutputs();

    return {};
}

float CRuntimeView::dataInputMax(const QString &name) const
{
    if (m_Application)
        return m_Application->dataInputMax(name);

    return 0;
}

float CRuntimeView::dataInputMin(const QString &name) const
{
    if (m_Application)
        return m_Application->dataInputMin(name);

    return 0;
}

QHash<QString, QString> CRuntimeView::dataInputMetadata(const QString &name) const
{
    if (m_Application)
        return m_Application->dataInputMetadata(name);

    return {};
}

void CRuntimeView::createElements(const QString &parentElementPath, const QString &slideName,
                                  const QVector<QHash<QString, QVariant>> &properties)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
        theBridgeEngine.createElements(parentElementPath, slideName, properties,
                                       &m_RuntimeFactory->GetQt3DSRenderContext().GetRenderer());
    }
}

void CRuntimeView::deleteElements(const QStringList &elementPaths)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
        theBridgeEngine.deleteElements(elementPaths,
                                       &m_RuntimeFactory->GetQt3DSRenderContext().GetRenderer());
    }
}

void CRuntimeView::createMaterials(const QString &subPresId,
                                   const QStringList &materialDefinitions)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
        theBridgeEngine.createMaterials(
                    subPresId, materialDefinitions,
                    &m_RuntimeFactory->GetQt3DSRenderContext().GetCustomMaterialSystem(),
                    &m_RuntimeFactory->GetQt3DSRenderContext().GetDynamicObjectSystem(),
                    &m_RuntimeFactory->GetQt3DSRenderContext().GetRenderer());
    }
}

void CRuntimeView::deleteMaterials(const QStringList &materialNames)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
        theBridgeEngine.deleteMaterials(materialNames,
                                        &m_RuntimeFactory->GetQt3DSRenderContext().GetRenderer());
    }
}

void CRuntimeView::createMesh(const QString &name, qt3dsimp::Mesh *mesh)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
        theBridgeEngine.createMesh(
                    name, mesh, &m_RuntimeFactory->GetQt3DSRenderContext().GetBufferManager());
    }
}

void CRuntimeView::deleteMeshes(const QStringList &meshNames)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
        theBridgeEngine.deleteMeshes(
                    meshNames, &m_RuntimeFactory->GetQt3DSRenderContext().GetBufferManager());
    }
}

void CRuntimeView::addImageProvider(const QString &providerId, QQmlImageProviderBase *provider)
{
    if (m_Application) {
        IBufferManager &bufferManager
                = m_RuntimeFactory->GetQt3DSRenderContext().GetBufferManager();
        bufferManager.addImageProvider(providerId, provider);
    }
}

uint CRuntimeView::textureId(const QString &elementPath)
{
    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());
        return theBridgeEngine.textureId(elementPath,
                                         &m_RuntimeFactory->GetQt3DSRenderContext().GetRenderer());
    }
    return 0;
}

void CRuntimeView::SetAttribute(const char *elementPath, const char *attributeName,
                                const char *value)
{
    if (m_Application) {
        if (!elementPath || !attributeName || !value)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.SetAttribute(elementPath, attributeName, value);
    }
}

bool CRuntimeView::GetAttribute(const char *elementPath, const char *attributeName, void *value)
{
    if (m_Application) {
        if (!elementPath || !attributeName || !value)
            return false;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        return theBridgeEngine.GetAttribute(elementPath, attributeName, (char *)value);
    }

    return false;
}

void CRuntimeView::FireEvent(const char *element, const char *evtName)
{
    if (m_Application) {
        if (!element || !evtName)
            return;

        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        theBridgeEngine.FireEvent(element, evtName);
    }
}

bool CRuntimeView::PeekCustomAction(char *&outElementPath, char *&outActionName)
{
    bool actionAvailable = true;

    if (m_Application) {
        Q3DStudio::CQmlEngine &theBridgeEngine
                = static_cast<Q3DStudio::CQmlEngine &>(m_RuntimeFactoryCore->GetScriptEngineQml());

        Q3DStudio::TElement *theElement = nullptr;
        actionAvailable = theBridgeEngine.PeekSignal(theElement, outActionName);
        if (actionAvailable && theElement)
            outElementPath = (char *)theElement->m_Path.c_str();
    }

    return actionAvailable;
}

void CRuntimeView::FireEvent(const TEventCommandHash inEventType, eastl::string inArgument)
{
    if (m_Application) {
        CPresentation *thePresentation = m_Application->GetPrimaryPresentation();
        TElement *theScene = thePresentation->GetRoot();
        if (inArgument.empty()) {
            thePresentation->FireEvent(inEventType, theScene, nullptr, nullptr, ATTRIBUTETYPE_NONE,
                                       ATTRIBUTETYPE_NONE);
        } else {
            UVariant inArg;
            inArg.m_StringHandle = thePresentation->GetStringTable().GetHandle(inArgument.c_str());
            thePresentation->FireEvent(inEventType, theScene, &inArg, nullptr, ATTRIBUTETYPE_STRING,
                                       ATTRIBUTETYPE_NONE);
        }
    }
}

void CRuntimeView::preloadSlide(const QString &slide)
{
    if (m_Application)
        m_Application->preloadSlide(slide);
}

void CRuntimeView::unloadSlide(const QString &slide)
{
    if (m_Application)
        m_Application->unloadSlide(slide);
}

void CRuntimeView::setDelayedLoading(bool enable)
{
    if (m_Application)
        m_Application->setDelayedLoading(enable);
}

qt3ds::foundation::Option<SPresentationSize> CRuntimeView::GetPresentationSize()
{
    if (m_Application) {
        CPresentation *thePresentation = m_Application->GetPrimaryPresentation();
        if (thePresentation)
            return thePresentation->GetSize();
    }
    return qt3ds::foundation::Empty();
}

//==============================================================================
/**
 *	Perform the initialization steps prior to loading any presentation.
 */
void CRuntimeView::BootupPreGraphicsInitObjects()
{
    qCInfo(TRACE_INFO) << "CNDDView::BootupPreGraphicsInitObjects: DoInitialize";
    // Create engines and runtime
    const eastl::string &theAppDir = QCoreApplication::applicationDirPath().toLatin1().constData();

    m_RuntimeFactoryCore = qt3ds::render::IQt3DSRenderFactoryCore::CreateRenderFactoryCore(
                theAppDir.c_str(), m_WindowSystem, m_TimeProvider);
    m_ApplicationCore = qt3ds::runtime::IApplication::CreateApplicationCore(*m_RuntimeFactoryCore,
                                                                            theAppDir.c_str());

    if (m_ApplicationCore && m_visitor)
        m_ApplicationCore->setAssetVisitor(m_visitor);

    m_InputEngine = static_cast<CTegraInputEngine *>(CreateInputEngine());
    Q3DStudio_ASSERT(m_InputEngine != nullptr);

    qCInfo(TRACE_INFO) << "CNDDView::DoInitialize: Successfully initialized!";
}

void CRuntimeView::setAssetVisitor(qt3ds::Qt3DSAssetVisitor *v)
{
    m_visitor = v;
    if (m_ApplicationCore)
        m_ApplicationCore->setAssetVisitor(v);
}

IRuntimeView &IRuntimeView::Create(ITimeProvider &inProvider, IWindowSystem &inWindowSystem,
                                   IAudioPlayer *inAudioPlayer, QElapsedTimer *startupTimer)
{
    return *QT3DS_NEW(qt3ds::render::g_BaseAllocator, CRuntimeView)(inProvider, inWindowSystem,
                                                                    inAudioPlayer, startupTimer);
}

QRuntimeViewSignalProxy *IRuntimeView::signalProxy()
{
    return &m_SignalProxy;
}

} // namespace Q3DStudio

