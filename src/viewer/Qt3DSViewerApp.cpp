/****************************************************************************
**
** Copyright (C) 2013 - 2016 NVIDIA Corporation.
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

#include <algorithm>

#include "EnginePrefix.h"
#include "Qt3DSRuntimeView.h"
#include "Qt3DSViewerApp.h"
#include "Qt3DSTegraInputEngine.h"
#include "Qt3DSInputFrame.h" // keyboard mapping
#include "foundation/Qt3DSTime.h"
#include "Qt3DSFNDTimer.h"
#include "Qt3DSAudioPlayer.h"
#include "Qt3DSImportMesh.h"

#include <QList>
#include <QFileInfo>
#include <QDebug>

#ifdef _MACOSX
#include <mach-o/dyld.h>
#endif

#ifndef EASTL_DEBUG_BREAK
void EASTL_DEBUG_BREAK()
{
    qFatal("EASTL_DEBUG_BREAK: Assertion blown");
}

#endif

#ifdef _WIN32
#include <Xinput.h>
#include <ShellAPI.h>
#include "Qt3DSInputFrame.h"

void HandleController(Q3DStudio::IRuntimeView &inView)
{
    static const class XInputLibrary
    {
    private:
        typedef DWORD(__stdcall *GetStateFunction)(DWORD, XINPUT_STATE *);

    public:
        XInputLibrary()
            : m_module(NULL)
            , GetState(NULL)
        {
            m_module = LoadLibraryA("XINPUT9_1_0.DLL");
            if (m_module != NULL) {
                GetState = reinterpret_cast<GetStateFunction>(GetProcAddress(m_module,
                                                                             "XInputGetState"));
            }
        }

        ~XInputLibrary()
        {
            if (m_module != NULL)
                FreeLibrary(m_module);
        }

        GetStateFunction GetState;

    private:
        HMODULE m_module;
    } library;

    static const DWORD MAX_USERS = 4;
    static const Q3DStudio::FLOAT STICK_RANGE = 32767;
    static const Q3DStudio::FLOAT TRIGGER_RANGE = 255;

    static const struct ButtonEntry
    {
        WORD XBoxButton;
        Q3DStudio::INT32 button;
    } buttonMap[] = {
    { XINPUT_GAMEPAD_START, Q3DStudio::BUTTON_START },
    { XINPUT_GAMEPAD_BACK, Q3DStudio::BUTTON_SELECT },
    { XINPUT_GAMEPAD_LEFT_THUMB, Q3DStudio::BUTTON_THUMBL },
    { XINPUT_GAMEPAD_RIGHT_THUMB, Q3DStudio::BUTTON_THUMBR },
    { XINPUT_GAMEPAD_LEFT_SHOULDER, Q3DStudio::BUTTON_L1 },
    { XINPUT_GAMEPAD_RIGHT_SHOULDER, Q3DStudio::BUTTON_R1 },
    { XINPUT_GAMEPAD_A, Q3DStudio::BUTTON_A },
    { XINPUT_GAMEPAD_B, Q3DStudio::BUTTON_B },
    { XINPUT_GAMEPAD_X, Q3DStudio::BUTTON_X },
    { XINPUT_GAMEPAD_Y, Q3DStudio::BUTTON_Y } };

    static DWORD userLastPacketNumber[MAX_USERS] = { 0 };
    static WORD userLastButtons[MAX_USERS] = { 0 };

    Q3DStudio::CInputEngine *input = inView.GetInputEngine();
    if (input != NULL && library.GetState) {
        // for each controller
        for (DWORD userIndex = 0; userIndex < MAX_USERS; ++userIndex) {
            DWORD &lastPacketNumber = userLastPacketNumber[userIndex];
            WORD &lastButtons = userLastButtons[userIndex];

            // get the state
            XINPUT_STATE state = { 0 };
            if (library.GetState(userIndex, &state) == ERROR_SUCCESS
                    && state.dwPacketNumber != lastPacketNumber) {
                using namespace Q3DStudio;
                const XINPUT_GAMEPAD &gamepad = state.Gamepad;
                const WORD &buttons = gamepad.wButtons;

                // handle button changes
                DWORD buttonChanges = buttons ^ lastButtons;
                for (size_t buttonMapIndex = 0;
                     buttonMapIndex < sizeof(buttonMap) / sizeof(buttonMap[0]); ++buttonMapIndex) {
                    const ButtonEntry &buttonEntry = buttonMap[buttonMapIndex];
                    if (buttonEntry.XBoxButton & buttonChanges) {
                        input->HandleButton(buttonEntry.button,
                                            buttons & buttonEntry.XBoxButton);
                    }
                }

                // handle gamepad
                if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
                    input->HandleAxis(AXIS_HAT_X, -1);
                else if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
                    input->HandleAxis(AXIS_HAT_X, 1);
                else
                    input->HandleAxis(AXIS_HAT_X, 0);
                if (buttons & XINPUT_GAMEPAD_DPAD_UP)
                    input->HandleAxis(AXIS_HAT_Y, -1);
                else if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
                    input->HandleAxis(AXIS_HAT_Y, 1);
                else
                    input->HandleAxis(AXIS_HAT_Y, 0);

                // handle sticks and triggers
                input->HandleAxis(AXIS_X, gamepad.sThumbLX / STICK_RANGE);
                input->HandleAxis(AXIS_Y, gamepad.sThumbLY / -STICK_RANGE);
                input->HandleAxis(AXIS_Z, gamepad.sThumbRX / STICK_RANGE);
                input->HandleAxis(AXIS_RZ, gamepad.sThumbRY / -STICK_RANGE);
                input->HandleAxis(AXIS_LTRIGGER, gamepad.bLeftTrigger / TRIGGER_RANGE);
                input->HandleAxis(AXIS_RTRIGGER, gamepad.bRightTrigger / TRIGGER_RANGE);

                // save the state
                lastButtons = buttons;
                lastPacketNumber = state.dwPacketNumber;
            }
        }
    }
}
#endif

using namespace Q3DStudio;

struct WindowRect
{
    int x;
    int y;
    int width;
    int height;
};

void PerfLogSetStringData(const char *)
{
} // Dummy defination when TCP perf logging isnt used

Q3DStudio::Qt3DSFNDTimer g_GlobalTimeProvider;

qint64 GetTimeUST()
{
    // this needs to be nano seconds
    Q3DStudio::ITimeProvider &theTimer = g_GlobalTimeProvider;
    return theTimer.GetCurrentTimeMicroSeconds() * 1000;
}

void initResource() {
    // init runtime static resources
    Q_INIT_RESOURCE(res);
}

namespace Q3DSViewer {

struct SWindowSystemImpl : public Q3DStudio::IWindowSystem
{
    QSize m_size;
    int m_OffscreenID;
    int m_DepthBitCount;

    QSize GetWindowDimensions() override { return m_size; }
    void SetWindowDimensions(const QSize &inSize) override
    {
        m_size = inSize;
    }
    // For platforms that support it, we get the egl info for render plugins
    // Feel free to return NULL.
    SEGLInfo *GetEGLInfo() override { return nullptr; }
    // on some systems we allow our default render target to be a offscreen buffer
    // otherwise return 0;
    int GetDefaultRenderTargetID() override { return m_OffscreenID; }
    // returns the depth buffer bit count for the render window
    int GetDepthBitCount() override { return m_DepthBitCount; }
};

class Q3DSViewerAppImpl
{
public:
    Q3DSViewerAppImpl(Q3DStudio::IAudioPlayer *inAudioPlayer)
        : m_view(nullptr)
        , m_appInitSuccessful(false)
        , m_AudioPlayer(inAudioPlayer)
    {
#ifndef EMBEDDED_LINUX
        initResource();
#endif
    }
    Q3DStudio::IRuntimeView *m_view;
    bool m_appInitSuccessful; ///< true if m_view is initialized successful

    std::vector<int> m_mouseButtons;
    Q3DStudio::IWindowSystem *m_WindowSystem;

    IAudioPlayer *m_AudioPlayer;
    QVector<QMouseEvent *> m_pendingEvents;

    QString m_error;
    qt3dsimp::MeshBuilder *m_meshBuilder = nullptr;

    void queueMouseEvent(int button, int pressed, int x, int y)
    {
        QMouseEvent *e = new QMouseEvent(pressed ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease,
                                         QPointF(x, y), (Qt::MouseButton)button,
                                         (Qt::MouseButtons)button, 0);
        e->setTimestamp(static_cast<ulong>(GetTimeUST()));
        m_pendingEvents.append(e);
    }

    qt3dsimp::Mesh *buildMesh(const Q3DSViewer::MeshData &meshData, QString &error)
    {
        using namespace qt3dsimp;
        using namespace qt3ds::render;

        // Do some basic validation of the meshData
        if (meshData.m_vertexBuffer.size() == 0) {
            error = QObject::tr("Vertex buffer empty");
            return nullptr;
        }
        if (meshData.m_attributeCount == 0) {
            error = QObject::tr("No attributes defined");
            return nullptr;
        }

        if (!m_meshBuilder)
            m_meshBuilder = &MeshBuilder::CreateMeshBuilder();
        m_meshBuilder->Reset();
        m_meshBuilder->SetDrawParameters(
                    static_cast<NVRenderDrawMode::Enum>(meshData.m_primitiveType),
                    NVRenderWinding::CounterClockwise);

        // The expectation is that the vertex buffer included in meshData is already properly
        // formatted and doesn't need further processing.

        // Validate attributes
        QVector<NVRenderVertexBufferEntry> vBufEntries;
        NVRenderComponentTypes::Enum indexBufferComponentType = NVRenderComponentTypes::Unknown;
        int indexBufferTypeSize = 0;
        for (int i = 0; i < meshData.m_attributeCount; ++i) {
            const Q3DSViewer::MeshData::Attribute &att = meshData.m_attributes[i];
            auto componentType = static_cast<NVRenderComponentTypes::Enum>(att.componentType);
            if (att.semantic == Q3DSViewer::MeshData::Attribute::IndexSemantic) {
                indexBufferComponentType = componentType;
                indexBufferTypeSize = att.typeSize();
            } else {
                const char *name = nullptr;
                switch (att.semantic) {
                case MeshData::Attribute::PositionSemantic:
                    name = Mesh::GetPositionAttrName();
                    break;
                case MeshData::Attribute::NormalSemantic:
                    name = Mesh::GetNormalAttrName();
                    break;
                case MeshData::Attribute::TexCoordSemantic:
                    name = Mesh::GetUVAttrName();
                    break;
                case MeshData::Attribute::TangentSemantic:
                    name = Mesh::GetTexTanAttrName();
                    break;
                case MeshData::Attribute::BinormalSemantic:
                    name = Mesh::GetTexBinormalAttrName();
                    break;
                default:
                    error = QObject::tr("Warning: Invalid attribute semantic: %1")
                            .arg(att.semantic);
                    return nullptr;
                }
                vBufEntries << NVRenderVertexBufferEntry(name, componentType,
                                                         QT3DSU32(att.componentCount()),
                                                         QT3DSU32(att.offset));
            }
        }
        m_meshBuilder->SetVertexBuffer(
                    toConstDataRef(reinterpret_cast<const NVRenderVertexBufferEntry *>(
                                       vBufEntries.constData()), QT3DSU32(vBufEntries.count())),
                    QT3DSU32(meshData.m_stride),
                    toConstDataRef(const_cast<QT3DSU8 *>(reinterpret_cast<const QT3DSU8 *>(
                                       meshData.m_vertexBuffer.constData())),
                                   QT3DSU32(meshData.m_vertexBuffer.size())));

        int vertexCount = 0;
        if (indexBufferComponentType != NVRenderComponentTypes::Unknown) {
            m_meshBuilder->SetIndexBuffer(
                        toConstDataRef(const_cast<QT3DSU8 *>(reinterpret_cast<const QT3DSU8 *>(
                                           meshData.m_indexBuffer.constData())),
                                       QT3DSU32(meshData.m_indexBuffer.size())),
                        indexBufferComponentType);
            vertexCount = meshData.m_indexBuffer.size() / indexBufferTypeSize;
        } else {
            vertexCount = meshData.m_vertexBuffer.size() / meshData.m_stride;
        }

        m_meshBuilder->AddMeshSubset(Mesh::s_DefaultName, QT3DSU32(vertexCount));

        return &m_meshBuilder->GetMesh();
    }
};

///< @brief contructor
Q3DSViewerApp::Q3DSViewerApp(void *glContext, Q3DStudio::IAudioPlayer *inAudioPlayer,
                             QElapsedTimer *startupTimer)
    : m_Impl(*new Q3DSViewerAppImpl(inAudioPlayer))
    , m_startupTimer(startupTimer)
{
    Q_UNUSED(glContext)
    m_Impl.m_WindowSystem = new SWindowSystemImpl();
}

///< @brief destructor
Q3DSViewerApp::~Q3DSViewerApp()
{
    qDeleteAll(m_Impl.m_pendingEvents);

    if (m_Impl.m_meshBuilder)
        m_Impl.m_meshBuilder->Release();

    delete m_Impl.m_AudioPlayer;

    if (m_Impl.m_view) {
        disconnect(m_Impl.m_view->signalProxy(), nullptr);

        m_Impl.m_view->Cleanup();
        m_Impl.m_view->release();

        if (GetMemoryManager().GetLineTracker())
            GetMemoryManager().GetLineTracker()->Report();

        GetMemoryManager().Release();
    }
    delete static_cast<SWindowSystemImpl *>(m_Impl.m_WindowSystem);
}

void Q3DSViewerApp::setOffscreenId(int offscreenID)
{
    static_cast<SWindowSystemImpl *>(m_Impl.m_WindowSystem)->m_OffscreenID
            = offscreenID;
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine())
        m_Impl.m_view->GetTegraRenderEngine()->ensureRenderTarget();
}

bool Q3DSViewerApp::InitializeApp(int winWidth, int winHeight, const QSurfaceFormat &format,
                                  int offscreenID, const QString &source,
                                  const QStringList &variantList,
                                  bool delayedLoading, bool initInRenderThread,
                                  const QByteArray &shaderCache,
                                  qt3ds::Qt3DSAssetVisitor *assetVisitor)
{
    bool hasValidPresentationFile = !source.isEmpty();

    QFileInfo info(source);
    if (!info.exists()) {
        m_Impl.m_error
                = QObject::tr("Failed to initialize viewer, presentation doesn't exist");
        qCritical() << m_Impl.m_error;
        return false;
    }

    m_Impl.m_WindowSystem->SetWindowDimensions(QSize(winWidth, winHeight));

    // create our internal application
    if (hasValidPresentationFile && !m_Impl.m_view) {
        static_cast<SWindowSystemImpl *>(m_Impl.m_WindowSystem)->m_OffscreenID = offscreenID;
        static_cast<SWindowSystemImpl *>(m_Impl.m_WindowSystem)->m_DepthBitCount
                = format.depthBufferSize();

        // create internal app
        m_Impl.m_view = &IRuntimeView::Create(g_GlobalTimeProvider, *m_Impl.m_WindowSystem,
                                          m_Impl.m_AudioPlayer, m_startupTimer);

        if (assetVisitor)
            m_Impl.m_view->setAssetVisitor(assetVisitor);

        m_Impl.m_appInitSuccessful = m_Impl.m_view->BeginLoad(source, variantList);

        if (m_Impl.m_appInitSuccessful == false) {
            m_Impl.m_error = QObject::tr("Viewer launch failure! Failed to load: '%1'").arg(source);
            m_Impl.m_error.append("\n");
            qCritical() << m_Impl.m_error;
            return false;
        }
        QString errors;
        bool success = m_Impl.m_view->InitializeGraphics(format, delayedLoading,
                                                         initInRenderThread,
                                                         shaderCache, errors);
        if (!errors.isEmpty())
            Q_EMIT SigLoadShaderCacheErrors(errors);

        if (!success) {
            m_Impl.m_error = QObject::tr("Viewer launch failure! Failed to load: '%1'").arg(source);
            m_Impl.m_error.append("\n");
            qCritical() << m_Impl.m_error;
            return false;
        }

        if (initInRenderThread)
            this->connectSignals();

        Resize(winWidth, winHeight);

        Q_EMIT SigPresentationLoaded();
    }
    return true;
}

void Q3DSViewerApp::connectSignals()
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->connectSignals();

    connect(m_Impl.m_view->signalProxy(),
            &QRuntimeViewSignalProxy::SigSlideEntered, this, &Q3DSViewerApp::SigSlideEntered);
    connect(m_Impl.m_view->signalProxy(),
            &QRuntimeViewSignalProxy::SigSlideExited, this, &Q3DSViewerApp::SigSlideExited);
    connect(m_Impl.m_view->signalProxy(),
            &QRuntimeViewSignalProxy::SigCustomSignal, this, &Q3DSViewerApp::SigCustomSignal);
    connect(m_Impl.m_view->signalProxy(), &QRuntimeViewSignalProxy::SigDataOutputValueUpdated,
            this, &Q3DSViewerApp::SigDataOutputValueUpdated);
    QMetaObject::Connection *presReadyconn = new QMetaObject::Connection();
    *presReadyconn = connect(m_Impl.m_view->signalProxy(),
                             &QRuntimeViewSignalProxy::SigPresentationReady, [&, presReadyconn]{
        // We receive presentation ready signal from runtime when animations and properties
        // have been updated.
        Q_EMIT SigPresentationReady();
        disconnect(*presReadyconn);
        delete presReadyconn;
    });
    connect(m_Impl.m_view->signalProxy(), &QRuntimeViewSignalProxy::SigElementsCreated, this,
            &Q3DSViewerApp::SigElementsCreated);
    connect(m_Impl.m_view->signalProxy(), &QRuntimeViewSignalProxy::SigMaterialsCreated, this,
            &Q3DSViewerApp::SigMaterialsCreated);
}

void Q3DSViewerApp::finishAsyncInit()
{
    connectSignals();
    m_Impl.m_view->finishAsyncInit();
}

bool Q3DSViewerApp::IsInitialised(void)
{
    return m_Impl.m_view != nullptr && m_Impl.m_appInitSuccessful;
}

int Q3DSViewerApp::GetWindowHeight()
{
    return m_Impl.m_WindowSystem->GetWindowDimensions().width();
}

int Q3DSViewerApp::GetWindowWidth()
{
    return m_Impl.m_WindowSystem->GetWindowDimensions().height();
}

void Q3DSViewerApp::setupSearchPath(std::vector<std::string> &cmdLineArgs)
{
    // setup some additional search path
    // index 0 contains the module directory
#ifdef _MACOSX
    char buf[2056];
    uint32_t bufsize = 2056;
    _NSGetExecutablePath(buf, &bufsize);
    std::string theModuleDirectory = buf;
#else
    std::string theModuleDirectory = cmdLineArgs[0];
#endif
    std::string::size_type pos = theModuleDirectory.rfind('\\');
    if (pos == std::string::npos)
        pos = theModuleDirectory.rfind('/');

    // Include the slash
    theModuleDirectory = theModuleDirectory.substr(0, pos + 1);

    // TODO: Search path needs to be set up properly
    //NvFAppendSearchPath(theModuleDirectory.c_str());
}

bool Q3DSViewerApp::Render()
{
    bool ret = true;
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine()) {
        if (m_Impl.m_appInitSuccessful) {
            for (QEvent *e : m_Impl.m_pendingEvents) {
                m_Impl.m_view->HandleMessage(e);
                delete e;
            }
            m_Impl.m_pendingEvents.clear();
#ifdef WIN32
            HandleController(*m_Impl.m_view);
#endif

            ret = m_Impl.m_view->Render();
        }
    }
    return ret;
}

void Q3DSViewerApp::SaveState()
{
    if (!m_Impl.m_view)
        return;

    if (m_Impl.m_view->GetTegraRenderEngine()) {
        ITegraRenderStateManager &manager =
                m_Impl.m_view->GetTegraRenderEngine()->GetTegraRenderStateManager();

        manager.SaveAllState();
    }
}

void Q3DSViewerApp::RestoreState()
{
    if (!m_Impl.m_view)
        return;

    if (m_Impl.m_view->GetTegraRenderEngine()) {
        ITegraRenderStateManager &manager =
                m_Impl.m_view->GetTegraRenderEngine()->GetTegraRenderStateManager();

        manager.RestoreAllState();
    }
}

bool Q3DSViewerApp::WasLastFrameDirty()
{
    if (m_Impl.m_view)
        return m_Impl.m_view->WasLastFrameDirty();
    return false;
}

QString Q3DSViewerApp::error()
{
    QString error = m_Impl.m_error;
    m_Impl.m_error.clear();
    return error;
}

void Q3DSViewerApp::setDelayedLoading(bool enable)
{
    if (!m_Impl.m_view)
        return;
    m_Impl.m_view->setDelayedLoading(enable);
}

void Q3DSViewerApp::preloadSlide(const QString &slide)
{
    if (!m_Impl.m_view)
        return;
    m_Impl.m_view->preloadSlide(slide);
}

void Q3DSViewerApp::unloadSlide(const QString &slide)
{
    if (!m_Impl.m_view)
        return;
    m_Impl.m_view->unloadSlide(slide);
}

void Q3DSViewerApp::Resize(int width, int height)
{
    QSize oldSize = m_Impl.m_WindowSystem->GetWindowDimensions();
    QSize newSize(width, height);
    m_Impl.m_WindowSystem->SetWindowDimensions(newSize);

    if (m_Impl.m_appInitSuccessful && m_Impl.m_view
            && m_Impl.m_view->GetTegraRenderEngine()) {
        QResizeEvent event = QResizeEvent(newSize, oldSize);
        m_Impl.m_view->HandleMessage(&event);
    }
}

void Q3DSViewerApp::HandleKeyInput(Q3DStudio::EKeyCode inKeyCode, bool isPressed)
{
    if (!m_Impl.m_view || inKeyCode == Q3DStudio::KEY_NOKEY)
        return;

    CInputEngine *input = m_Impl.m_view->GetInputEngine();
    if (input)
        input->HandleKeyboard(inKeyCode, isPressed);
}

void Q3DSViewerApp::HandleMouseMove(int x, int y, bool isPressed)
{
    if (!m_Impl.m_view)
        return;

    CInputEngine *input = m_Impl.m_view->GetInputEngine();
    if (input) {
        input->BeginPickInput();
        input->EndPickInput();
        input->SetPickInput(static_cast<Q3DStudio::FLOAT>(x), static_cast<Q3DStudio::FLOAT>(y),
                            isPressed);
    }

}

void Q3DSViewerApp::HandleMousePress(int x, int y, int mouseButton, bool isPressed)
{
    if (!m_Impl.m_view)
        return;

    bool hasButton
            = std::find(m_Impl.m_mouseButtons.begin(), m_Impl.m_mouseButtons.end(), mouseButton)
            != m_Impl.m_mouseButtons.end();

    if ((mouseButton == 1 || mouseButton == 2)) {
        // We keep track of the mouse presses because there are situations where the application
        // will not give us a mouse
        // up for each mouse down.  This ameleorates the effects of those slightly misbehaving
        // applications.
        if (isPressed == hasButton) {
            bool localIsPressed = !isPressed;
            // Slightly recursive call in order to handle situations where we aren't getting
            // the mouse up with the corresponding mouse down or vice versa.
            HandleMousePress(x, y, mouseButton, localIsPressed);
            HandleMousePress(x, y, mouseButton, isPressed);
        } else {
            if (isPressed) {
                m_Impl.m_mouseButtons.push_back(mouseButton);
                qCInfo(qt3ds::TRACE_INFO)
                        << "ViewerApp: Mouse down of frame "
                        << m_Impl.m_view->GetFrameCount();
            } else {
                m_Impl.m_mouseButtons.erase(std::remove(m_Impl.m_mouseButtons.begin(),
                                                        m_Impl.m_mouseButtons.end(), mouseButton),
                                            m_Impl.m_mouseButtons.end());
            }

            CInputEngine *input = m_Impl.m_view->GetInputEngine();

            if (input) {
                input->BeginPickInput();
                input->SetPickInput(static_cast<Q3DStudio::FLOAT>(x),
                                    static_cast<Q3DStudio::FLOAT>(y), isPressed);
                input->EndPickInput();

                m_Impl.queueMouseEvent(mouseButton, isPressed ? 1 : 0, x, y);
            }
        }
    }
}

void Q3DSViewerApp::HandleMouseWheel(int x, int y, int orientation, int numSteps)
{
    if (!m_Impl.m_view)
        return;

    CInputEngine *input = m_Impl.m_view->GetInputEngine();
    if (input) {
        input->SetPickInput(static_cast<Q3DStudio::FLOAT>(x), static_cast<Q3DStudio::FLOAT>(y), 0);
        input->SetScrollValue(orientation == 0 ? VSCROLLWHEEL : HSCROLLWHEEL, numSteps);
    }
}

void Q3DSViewerApp::GoToSlideByName(const char *elementPath, const char *slideName)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->GoToSlideByName(elementPath, slideName);
}

void Q3DSViewerApp::GoToSlideByIndex(const char *elementPath, const int slideIndex)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->GoToSlideByIndex(elementPath, slideIndex);
}

void Q3DSViewerApp::GoToSlideRelative(const char *elementPath, const bool next, const bool wrap)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->GoToSlideRelative(elementPath, next, wrap);
}

bool Q3DSViewerApp::GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                                QString &currentName, QString &previousName)
{
    if (!m_Impl.m_view)
        return false;

    return m_Impl.m_view->GetSlideInfo(elementPath, currentIndex, previousIndex,
                                           currentName, previousName);
}

void Q3DSViewerApp::SetPresentationActive(const char *presId, const bool active)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->SetPresentationAttribute(presId, nullptr, active ? "True" : "False");
}

void Q3DSViewerApp::GoToTime(const char *elementPath, const float time)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->GoToTime(elementPath, time);
}

void Q3DSViewerApp::PlaySoundFile(const char *soundPath)
{
    if (!m_Impl.m_AudioPlayer || !soundPath)
        return;

    m_Impl.m_AudioPlayer->PlaySoundFile(soundPath);
}

void Q3DSViewerApp::SetAttribute(const char *elementPath, const char *attributeName,
                                const char *value)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->SetAttribute(elementPath, attributeName, value);
}

bool Q3DSViewerApp::GetAttribute(const char *elementPath, const char *attributeName, void *value)
{
    if (!m_Impl.m_view)
        return false;

    return m_Impl.m_view->GetAttribute(elementPath, attributeName, value);
}

void Q3DSViewerApp::FireEvent(const char *elementPath, const char *evtName)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->FireEvent(elementPath, evtName);
}

bool Q3DSViewerApp::PeekCustomAction(std::string &outElementPath, std::string &outActionName)
{
    if (!m_Impl.m_view)
        return false;

    char *theElementPath = nullptr;
    char *theActioName = nullptr;
    bool retVal = m_Impl.m_view->PeekCustomAction(theElementPath, theActioName);

    if (theElementPath)
        outElementPath = theElementPath;
    if (theActioName)
        outActionName = theActioName;

    return retVal;
}

bool Q3DSViewerApp::RegisterScriptCallback(ViewerCallbackType::Enum inCallbackType,
                                          const qml_Function inCallback, void *inUserData)
{
    if (!m_Impl.m_view)
        return false;

    // convert to int
    int callbackType = 0;
    if (inCallbackType == Q3DSViewer::ViewerCallbackType::CALLBACK_ON_INIT)
        callbackType = 1;
    else if (inCallbackType == Q3DSViewer::ViewerCallbackType::CALLBACK_ON_UPDATE)
        callbackType = 2;
    else
        return false;

    bool retVal = m_Impl.m_view->RegisterScriptCallback(callbackType, inCallback, inUserData);

    return retVal;
}

void Q3DSViewerApp::SetScaleMode(ViewerScaleModes::Enum inScale)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine()) {
        m_Impl.m_view->GetTegraRenderEngine()->SetScaleMode(
                    static_cast<TegraRenderScaleModes::Enum>(inScale));
    }
}

ViewerScaleModes::Enum Q3DSViewerApp::GetScaleMode()
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine()) {
        return static_cast<ViewerScaleModes::Enum>(
                    m_Impl.m_view->GetTegraRenderEngine()->GetScaleMode());
    }

    return ViewerScaleModes::ExactSize;
}

void Q3DSViewerApp::SetStereoMode(ViewerStereoModes::Enum inMode)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine()) {
        m_Impl.m_view->GetTegraRenderEngine()->SetStereoMode(
                    static_cast<TegraRenderStereoModes::Enum>(inMode));
    }
}

ViewerStereoModes::Enum Q3DSViewerApp::GetStereoMode() const
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine()) {
        return static_cast<ViewerStereoModes::Enum>(
                    m_Impl.m_view->GetTegraRenderEngine()->GetStereoMode());
    }

    return ViewerStereoModes::Mono;
}

void Q3DSViewerApp::SetStereoEyeSeparation(double separation)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine())
        m_Impl.m_view->GetTegraRenderEngine()->SetStereoEyeSeparation(separation);
}

double Q3DSViewerApp::GetStereoEyeSeparation() const
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine())
        return m_Impl.m_view->GetTegraRenderEngine()->GetStereoEyeSeparation();

    return 0;
}

void Q3DSViewerApp::SetStereoProgressiveEnabled(bool enabled)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine())
        m_Impl.m_view->GetTegraRenderEngine()->SetStereoProgressiveEnabled(enabled);
}

bool Q3DSViewerApp::GetStereoProgressiveEnabled() const
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine())
        return m_Impl.m_view->GetTegraRenderEngine()->GetStereoProgressiveEnabled();

    return 0;
}

void Q3DSViewerApp::setMatteColor(const QColor &color)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine()) {
        m_Impl.m_view->GetTegraRenderEngine()->SetMatteColor(
                    qt3ds::QT3DSVec4(color.redF(), color.greenF(),
                                     color.blueF(), color.alphaF()));
    }
}

void Q3DSViewerApp::setMatteEnabled(bool enable)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine())
        m_Impl.m_view->GetTegraRenderEngine()->setMatteEnabled(enable);
}

QByteArray Q3DSViewerApp::exportShaderCache(bool binaryShaders)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine())
        return m_Impl.m_view->GetTegraRenderEngine()->exportShaderCache(binaryShaders);

    return {};
}

void Q3DSViewerApp::setShowOnScreenStats(bool inShow)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine())
        m_Impl.m_view->showOnScreenStats(inShow);
}

void Q3DSViewerApp::SetShadeMode(ViewerShadeModes::Enum inShadeMode)
{
    if (m_Impl.m_view && m_Impl.m_view->GetTegraRenderEngine()) {
        StaticAssert<ViewerShadeModes::Shaded == TegraRenderShadeModes::Shaded>::valid_expression();
        StaticAssert<ViewerShadeModes::ShadedWireframe
                == TegraRenderShadeModes::ShadedWireframe>::valid_expression();
        StaticAssert<ViewerShadeModes::Wireframe
                == TegraRenderShadeModes::Wireframe>::valid_expression();

        m_Impl.m_view->GetTegraRenderEngine()->SetShadeMode(
                    static_cast<TegraRenderShadeModes::Enum>(inShadeMode));
    }
}

void Q3DSViewerApp::SetGlobalAnimationTime(qint64 inMilliSecs)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->SetGlobalAnimationTime(inMilliSecs);
}

void Q3DSViewerApp::SetDataInputValue(
        const QString &name, const QVariant &value, qt3ds::runtime::DataInputValueRole valueRole)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->SetDataInputValue(name, value, int(valueRole));
}

void Q3DSViewerApp::setPresentationId(const QString &id)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->setPresentationId(id);
}

QList<QString> Q3DSViewerApp::dataInputs() const
{
    if (!m_Impl.m_view)
        return {};

    return m_Impl.m_view->dataInputs();
}

QList<QString> Q3DSViewerApp::dataOutputs() const
{
    if (!m_Impl.m_view)
        return {};

    return m_Impl.m_view->dataOutputs();
}

float Q3DSViewerApp::dataInputMax(const QString &name) const
{
    if (!m_Impl.m_view)
        return 0.0f;

    return m_Impl.m_view->dataInputMax(name);
}

float Q3DSViewerApp::dataInputMin(const QString &name) const
{
    if (!m_Impl.m_view)
        return 0.0f;

    return m_Impl.m_view->dataInputMin(name);
}

QHash<QString, QString> Q3DSViewerApp::dataInputMetadata(const QString &name) const
{
    if (!m_Impl.m_view)
        return {};

    return m_Impl.m_view->dataInputMetadata(name);
}

void Q3DSViewerApp::createElements(const QString &parentElementPath, const QString &slideName,
                                   const QVector<QHash<QString, QVariant>> &properties)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->createElements(parentElementPath, slideName, properties);
}

void Q3DSViewerApp::deleteElements(const QStringList &elementPaths)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->deleteElements(elementPaths);
}

void Q3DSViewerApp::createMaterials(const QString &subPresId,
                                    const QStringList &materialDefinitions)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->createMaterials(subPresId, materialDefinitions);
}

void Q3DSViewerApp::deleteMaterials(const QStringList &materialNames)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->deleteMaterials(materialNames);
}

void Q3DSViewerApp::createMeshes(const QHash<QString, Q3DSViewer::MeshData> &meshData)
{
    if (!m_Impl.m_view)
        return;

    QString error;
    QHashIterator<QString, Q3DSViewer::MeshData> dataIter(meshData);
    while (dataIter.hasNext()) {
        dataIter.next();
        // Mesh creation needs to be done one by one, as Mesh pointers are not valid
        // after builder is reused
        qt3dsimp::Mesh *mesh = m_Impl.buildMesh(dataIter.value(), error);
        if (mesh) {
            m_Impl.m_view->createMesh(dataIter.key(), mesh);
        } else {
            QString errorMsg = QObject::tr("Creating mesh '%1' failed: %2")
                    .arg(dataIter.key()).arg(error);
            qWarning() << __FUNCTION__ << errorMsg;
            SigMeshesCreated(meshData.keys(), error);
            return;
        }
    }
    m_Impl.m_meshBuilder->Reset();
    SigMeshesCreated(meshData.keys(), error);
}

void Q3DSViewerApp::deleteMeshes(const QStringList &meshNames)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->deleteMeshes(meshNames);
}

void Q3DSViewerApp::addImageProvider(const QString &providerId, QQmlImageProviderBase *provider)
{
    if (!m_Impl.m_view)
        return;

    m_Impl.m_view->addImageProvider(providerId, provider);
}

uint Q3DSViewerApp::textureId(const QString &elementPath)
{
    if (!m_Impl.m_view)
        return 0;

    return m_Impl.m_view->textureId(elementPath);
}

uint Q3DSViewerApp::textureId(const QString &elementPath, QSize &size, GLenum &format)
{
    if (!m_Impl.m_view) {
        size = {};
        format = GL_INVALID_ENUM;
        return 0;
    }
    return m_Impl.m_view->textureId(elementPath, size, format);
}

Q3DSViewerApp &Q3DSViewerApp::Create(void *glContext, Q3DStudio::IAudioPlayer *inAudioPlayer,
                                     QElapsedTimer *startupTimer)
{
    return *Q3DStudio_virtual_new(Q3DSViewerApp) Q3DSViewerApp(glContext, inAudioPlayer,
                                                               startupTimer);
}

void Q3DSViewerApp::Release()
{
    Q3DStudio_virtual_delete(this, Q3DSViewerApp);
}

} // end namespace
