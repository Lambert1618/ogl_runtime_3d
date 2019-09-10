/****************************************************************************
**
** Copyright (c) 2016 NVIDIA CORPORATION.
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

#include "q3dsrenderer_p.h"
#include "Qt3DSViewerApp.h"
#include "Qt3DSAudioPlayerImpl.h"
#include "q3dspresentationitem_p.h"
#include "q3dsruntimeInitializerthread_p.h"

#include <QtStudio3D/private/q3dscommandqueue_p.h>
#include <QtStudio3D/private/q3dsviewersettings_p.h>
#include <QtStudio3D/private/q3dspresentation_p.h>
#include <QtStudio3D/private/studioutils_p.h>
#include <QtStudio3D/private/q3dsdatainput_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qthread.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopenglcontext.h>
#include <QtQuick/qquickwindow.h>

using namespace Q3DSViewer;

QT_BEGIN_NAMESPACE

Q3DSRenderer::Q3DSRenderer(bool visibleFlag, qt3ds::Qt3DSAssetVisitor *assetVisitor,
                           QElapsedTimer *startupTimer, QSurface *asyncInitSurface)
    : m_visibleFlag(visibleFlag)
    , m_initElements(false)
    , m_runtime(nullptr)
    , m_window(nullptr)
    , m_initialized(false)
    , m_initializationFailure(false)
    , m_visitor(assetVisitor)
    , m_settings(new Q3DSViewerSettings(this))
    , m_presentation(new Q3DSPresentation(this))
    , m_startupTimer(startupTimer)
    , m_asyncInitSurface(asyncInitSurface)
{

}

Q3DSRenderer::~Q3DSRenderer()
{
    releaseRuntime();
}

QOpenGLFramebufferObject *Q3DSRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat theFormat;
    theFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    QOpenGLFramebufferObject *frameBuffer =
            new QOpenGLFramebufferObject(size, theFormat);
    if (m_runtime && m_runtime->IsInitialised())
        m_runtime->setOffscreenId(int(frameBuffer->handle()));
    return frameBuffer;
}

/** Pull pending commands from the plugin.
 *  Invoked automatically by the QML scene graph.
 *
 *  This is the only place where it is valid for the Q3DSStudio3D plugin and render to communicate.
 */
void Q3DSRenderer::synchronize(QQuickFramebufferObject *inView)
{
    // Passing m_InitElements here is a bit of a hack to easily set the flag on the plugin.
    static_cast<Q3DSStudio3D *>(inView)->getCommands(m_initElements, m_commands);

    if (m_initializationFailure)
        static_cast<Q3DSStudio3D *>(inView)->setError(m_error);

    if (m_commands.m_sourceChanged || m_commands.m_variantListChanged) {
        releaseRuntime();
        // Need to update source and variant list here rather than
        // processCommands, as both are needed for init
        m_presentation->setVariantList(m_commands.m_variantList);
        m_presentation->setSource(m_commands.m_source);
        m_presentation->setDelayedLoading(m_commands.m_delayedLoading);
        m_presentation->setShaderCacheFile(m_commands.m_shaderCacheFile);
        m_commands.m_sourceChanged = false;
        m_commands.m_variantListChanged = false;
        m_commands.m_delayedLoadingChanged = false;
        m_commands.m_shaderCacheFileChanged = false;
        m_initialized = false;
        m_initializationFailure = false;
        m_error.clear();
        static_cast<Q3DSStudio3D *>(inView)->setError(QString());
    }

    m_initElements = false;

    // We need a handle to the window to be able to reset the GL state inside of Draw().
    // See https://bugreports.qt.io/browse/QTBUG-47213
    if (!m_window)
        m_window = inView->window();
}

void Q3DSRenderer::releaseRuntime()
{
    m_settings->d_ptr->setViewerApp(nullptr);
    m_presentation->d_ptr->setViewerApp(nullptr);

    if (m_runtime) {
        if (m_asyncInitSurface && m_runtimeInitializerThread) {
            m_runtimeInitializerThread->quit();
            m_runtimeInitializerThread->wait();
            delete m_runtimeInitializerThread;
            m_runtimeInitializerThread = nullptr;
        }
        m_runtime->Release();
        m_runtime = nullptr;
    }
}

void Q3DSRenderer::registerCallbacks()
{
    m_runtime->RegisterScriptCallback(Q3DSViewer::ViewerCallbackType::Enum::CALLBACK_ON_INIT,
                                      reinterpret_cast<qml_Function>(Q3DSRenderer::onInitHandler),
                                      this);
    m_runtime->RegisterScriptCallback(Q3DSViewer::ViewerCallbackType::Enum::CALLBACK_ON_UPDATE,
                                      reinterpret_cast<qml_Function>(Q3DSRenderer::onUpdateHandler),
                                      this);
}

class RuntimeInitializer : public QRunnable
{
public:
    RuntimeInitializer(Q3DSRenderer *r)
        : m_renderer(r)
    { }
    void run() override
    {
        if (!m_renderer->m_initialized && !m_renderer->m_initializationFailure) {
            m_renderer->m_initialized
                    = m_renderer->initializeRuntime(m_renderer->framebufferObject());
            m_renderer->m_initializationFailure = !m_renderer->m_initialized;
            if (m_renderer->m_initializationFailure)
                m_renderer->m_commands.clear(true);
        }
    }

private:
    Q3DSRenderer *m_renderer;
};

/** Invoked by the QML scene graph indicating that it's time to render.
 *  Calls `draw()` if the plugin is visible, or `processCommands()` otherwise.
 *
 *  Note that this will still render if the plugin is opacity:0. To avoid this,
 *  add a line to your QML to hide the object when opacity is zero, like:
 *
 *      visible: opacity != 0
 */
void Q3DSRenderer::render()
{
    // We may start in a non visible state but we still need
    // to init the runtime otherwise the commands are never processed
    if (!m_initialized && !m_initializationFailure) {
        if (m_asyncInitSurface) {
            if (!m_runtimeInitializerThread)
                initializeRuntime(framebufferObject());
        } else {
            auto *ri = new RuntimeInitializer(this);
            // Initialize runtime after the first frame has been drawn
            m_window->scheduleRenderJob(ri, QQuickWindow::AfterSwapStage);
        }
    }

    // Don't render if the plugin is hidden; however, if hidden, but sure
    // to process pending commands so we can be shown again.
    if (m_initialized) {
        if (m_visibleFlag)
            draw();
        else
            processCommands();
        update(); // mark as dirty to ensure update again
    }
}

/** Cause Qt3DS runtime to render content.
 *  Initializes GL and the runtime when called the first time.
 */
void Q3DSRenderer::draw()
{
    if (m_runtime && m_runtime->IsInitialised() && m_window) {
        if (m_initialized)
            m_runtime->RestoreState();
        m_runtime->Render();
        m_runtime->SaveState();

        m_window->resetOpenGLState();
    }
}

bool Q3DSRenderer::initializeRuntime(QOpenGLFramebufferObject *inFbo)
{
    m_runtime = &Q3DSViewerApp::Create(nullptr, new Qt3DSAudioPlayerImpl(), m_startupTimer);
    Q_ASSERT(m_runtime);

    // Connect presentation ready signal before initializing the app
    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigPresentationReady,
            this, &Q3DSRenderer::presentationReady);
    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigPresentationLoaded,
            this, &Q3DSRenderer::presentationLoaded);

    int theWidth = inFbo->width();
    int theHeight = inFbo->height();

    const QString localSource = Q3DSUtils::urlToLocalFileOrQrc(m_presentation->source());

    if (m_asyncInitSurface) {
        auto currentContext = QOpenGLContext::currentContext();
        auto context = new QOpenGLContext();
        context->setFormat(currentContext->format());
        context->setShareContext(currentContext);
        context->create();
        m_runtimeInitializerThread = new Q3DSRuntimeInitializerThread(
                    m_runtime, theWidth, theHeight, QOpenGLContext::currentContext()->format(),
                    int(inFbo->handle()), localSource, m_presentation->variantList(),
                    m_presentation->delayedLoading(), m_visitor, context,
                    m_asyncInitSurface, m_presentation->d_ptr->loadShaderCache());
        connect(m_runtimeInitializerThread, &Q3DSRuntimeInitializerThread::initDone,
                this, &Q3DSRenderer::handleRuntimeInitializedAsync, Qt::QueuedConnection);
        context->moveToThread(m_runtimeInitializerThread);
        m_runtimeInitializerThread->start();
    } else {
        if (!m_runtime->InitializeApp(theWidth, theHeight,
                                      QOpenGLContext::currentContext()->format(),
                                      int(inFbo->handle()), localSource,
                                      m_presentation->variantList(),
                                      m_presentation->delayedLoading(), true,
                                      m_presentation->d_ptr->loadShaderCache(),
                                      m_visitor)) {
            m_error = m_runtime->error();
            releaseRuntime();
            return false;
        }
        m_settings->d_ptr->setViewerApp(m_runtime);
        m_presentation->d_ptr->setViewerApp(m_runtime, false);
        registerCallbacks();
    }

    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigSlideEntered,
            this, &Q3DSRenderer::enterSlide);
    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigSlideExited,
            this, &Q3DSRenderer::exitSlide);
    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigCustomSignal,
            this, &Q3DSRenderer::customSignalEmitted);
    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigElementsCreated,
            this, &Q3DSRenderer::elementsCreated);
    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigMaterialsCreated,
            this, &Q3DSRenderer::materialsCreated);
    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigMeshesCreated,
            this, &Q3DSRenderer::meshesCreated);
    connect(m_runtime, &Q3DSViewer::Q3DSViewerApp::SigDataOutputValueUpdated,
            this, &Q3DSRenderer::dataOutputValueUpdated);

    return true;
}

/** Accept user commands (e.g. setAttribute) needed to initialize the code.
 *
 *  If we attempt to run Qt3DS methods like setAttribute() before the runtime
 *  has been initialized, they will be lost. This method is the correct place
 *  to accept user commands.
 *
 *  Currently this method just sets a flag needed to pass a flag to the
 *  plugin next time syncronize() is called, which eventually gets the plugin
 *  to emit an `runningChanged` signal the next time `tick()` is called.
 *  As a result, code specified in an `onRunningChanged` handler may be run
 *  after one or more frames have already rendered.
 */
void Q3DSRenderer::onInitHandler(void *userData)
{
    Q3DSRenderer *theRenderer = static_cast<Q3DSRenderer *>(userData);
    theRenderer->setInitElements(true);
}

/** Accept the latest pending user commands (e.g. setAttribute).
 *
 *  This method just calls `ProcessCommands` to avoid unnecessary
 *  pointer dereferencing and accessor methods (or public member variables).
 */
void Q3DSRenderer::onUpdateHandler(void *userData)
{
    Q3DSRenderer *theRenderer = static_cast<Q3DSRenderer *>(userData);
    theRenderer->processCommands();
}

/** Apply commands queued up by the user (e.g. setAttribute).
 *
 *  Note that these commands are executed even if the plugin is not visible,
 *  in part to allow changes to the visible flag to be noticed, but also
 *  to allow specialty code to continue to be queued up even when not rendering.
 */
void Q3DSRenderer::processCommands()
{
    if (!m_runtime) {
        m_commands.clear(true);
        return;
    }

    if (m_commands.m_visibleChanged)
        m_visibleFlag = m_commands.m_visible;

    if (QOpenGLFramebufferObject *inFbo = this->framebufferObject()) {
        if (inFbo->isValid() && (inFbo->width() != m_runtime->GetWindowWidth()
                                 || inFbo->height() != m_runtime->GetWindowHeight())) {
            m_runtime->Resize(inFbo->width(), inFbo->height());
        }
    }

    if (m_commands.m_scaleModeChanged)
        m_settings->setScaleMode(m_commands.m_scaleMode);
    if (m_commands.m_stereoModeChanged)
        m_settings->setStereoMode(m_commands.m_stereoMode);
    if (m_commands.m_stereoEyeSeparationChanged)
        m_settings->setStereoEyeSeparation(m_commands.m_stereoEyeSeparation);
    if (m_commands.m_shadeModeChanged)
        m_settings->setShadeMode(m_commands.m_shadeMode);
    if (m_commands.m_matteColorChanged)
        m_settings->setMatteColor(m_commands.m_matteColor);
    if (m_commands.m_showRenderStatsChanged)
        m_settings->setShowRenderStats(m_commands.m_showRenderStats);
    if (m_commands.m_delayedLoadingChanged)
        this->m_runtime->setDelayedLoading(m_commands.m_delayedLoading);
    if (m_commands.m_matteEnabledChanged)
        this->m_runtime->setMatteEnabled(m_commands.m_matteEnabled);

    if (m_commands.m_globalAnimationTimeChanged)
        m_presentation->setGlobalAnimationTime(m_commands.m_globalAnimationTime);

    // Send scene graph changes over to Q3DS
    for (int i = 0; i < m_commands.size(); i++) {
        const ElementCommand &cmd = m_commands.constCommandAt(i);
        switch (cmd.m_commandType) {
        case CommandType_SetAttribute:
            m_presentation->setAttribute(cmd.m_elementPath, cmd.m_stringValue, cmd.m_variantValue);
            break;
        case CommandType_SetPresentationActive:
            m_presentation->setPresentationActive(cmd.m_elementPath, cmd.m_boolValue);
            break;
        case CommandType_GoToTime:
            m_presentation->goToTime(cmd.m_elementPath, cmd.m_floatValue);
            break;
        case CommandType_GoToSlide:
            m_presentation->goToSlide(cmd.m_elementPath, quint32(cmd.m_intValues[0]));
            break;
        case CommandType_GoToSlideByName:
            m_presentation->goToSlide(cmd.m_elementPath, cmd.m_stringValue);
            break;
        case CommandType_GoToSlideRelative:
            m_presentation->goToSlide(cmd.m_elementPath, bool(cmd.m_intValues[0]),
                    bool(cmd.m_intValues[1]));
            break;
        case CommandType_FireEvent:
            m_presentation->fireEvent(cmd.m_elementPath, cmd.m_stringValue);
            break;
        case CommandType_MousePress:
            m_runtime->HandleMousePress(cmd.m_intValues[0],
                    cmd.m_intValues[1], cmd.m_intValues[2], true);
            break;
        case CommandType_MouseRelease:
            m_runtime->HandleMousePress(cmd.m_intValues[0],
                    cmd.m_intValues[1], cmd.m_intValues[2], false);
            break;
        case CommandType_MouseMove:
            m_runtime->HandleMouseMove(cmd.m_intValues[0], cmd.m_intValues[1], true);
            break;
        case CommandType_MouseWheel:
            m_runtime->HandleMouseWheel(cmd.m_intValues[0], cmd.m_intValues[1],
                    bool(cmd.m_intValues[2]), cmd.m_intValues[3]);
            break;
        case CommandType_KeyPress:
            m_runtime->HandleKeyInput(Q3DStudio::EKeyCode(cmd.m_intValues[0]), true);
            break;
        case CommandType_KeyRelease:
            m_runtime->HandleKeyInput(Q3DStudio::EKeyCode(cmd.m_intValues[0]), false);
            break;
        case CommandType_SetDataInputValue:
            m_runtime->SetDataInputValue(
                        cmd.m_stringValue, cmd.m_variantValue,
                        static_cast<qt3ds::runtime::DataInputValueRole>(cmd.m_intValues[0]));
            break;
        case CommandType_SetDataInputBatch: {
            const QVector<QPair<QString, QVariant>> diValues
                    = *static_cast<QVector<QPair<QString, QVariant>> *>(cmd.m_data);

            for (const auto &di : diValues) {
                m_runtime->SetDataInputValue(di.first, di.second,
                                             qt3ds::runtime::DataInputValueRole::Value);
            }
            // No need for data after this, delete.
            auto &command = m_commands.commandAt(i);
            delete static_cast<QVector<QPair<QString, QVariant>> *>(command.m_data);
            break;
        }
        case CommandType_CreateElements: {
            m_runtime->createElements(
                        cmd.m_elementPath, cmd.m_stringValue,
                        *static_cast<QVector<QHash<QString, QVariant>> *>(cmd.m_data));
            // Runtime makes copy of the data in its own format, so we can delete it now
            auto &command = m_commands.commandAt(i);
            delete reinterpret_cast<QVector<QHash<QString, QVariant>> *>(command.m_data);
            command.m_data = nullptr;
            break;
        }
        case CommandType_DeleteElements: {
            m_runtime->deleteElements(*static_cast<QStringList *>(cmd.m_data));
            // Runtime makes copy of the data in its own format, so we can delete it now
            auto &command = m_commands.commandAt(i);
            delete reinterpret_cast<QStringList *>(command.m_data);
            command.m_data = nullptr;
            break;
        }
        case CommandType_CreateMaterials: {
            m_runtime->createMaterials(cmd.m_elementPath, *static_cast<QStringList *>(cmd.m_data));
            // Runtime makes copy of the data in its own format, so we can delete it now
            auto &command = m_commands.commandAt(i);
            delete reinterpret_cast<QStringList *>(command.m_data);
            command.m_data = nullptr;
            break;
        }
        case CommandType_DeleteMaterials: {
            m_runtime->deleteMaterials(*static_cast<QStringList *>(cmd.m_data));
            // Runtime makes copy of the data in its own format, so we can delete it now
            auto &command = m_commands.commandAt(i);
            delete reinterpret_cast<QStringList *>(command.m_data);
            command.m_data = nullptr;
            break;
        }
        case CommandType_CreateMeshes: {
            m_runtime->createMeshes(*static_cast<QHash<QString, Q3DSViewer::MeshData> *>(
                                        cmd.m_data));
            // Runtime makes copy of the data, so we can delete it now
            auto &command = m_commands.commandAt(i);
            auto meshData = reinterpret_cast<QHash<QString, Q3DSViewer::MeshData> *>(
                        command.m_data);
            delete meshData;
            command.m_data = nullptr;
            break;
        }
        case CommandType_DeleteMeshes: {
            m_runtime->deleteMeshes(*static_cast<QStringList *>(cmd.m_data));
            // Runtime makes copy of the data in its own format, so we can delete it now
            auto &command = m_commands.commandAt(i);
            delete reinterpret_cast<QStringList *>(command.m_data);
            command.m_data = nullptr;
            break;
        }
        case CommandType_PreloadSlide:
            m_runtime->preloadSlide(cmd.m_elementPath);
            break;
        case CommandType_UnloadSlide:
            m_runtime->unloadSlide(cmd.m_elementPath);
            break;
        case CommandType_AddImageProvider:
            m_runtime->addImageProvider(cmd.m_elementPath,
                                        static_cast<QQmlImageProviderBase *>(cmd.m_data));
            break;
        case CommandType_RequestSlideInfo: {
            int current = 0;
            int previous = 0;
            QString currentName;
            QString previousName;
            const QByteArray path(cmd.m_elementPath.toUtf8());
            m_runtime->GetSlideInfo(path, current, previous, currentName, previousName);
            QVariantList *requestData = new QVariantList();
            requestData->append(QVariant(current));
            requestData->append(QVariant(previous));
            requestData->append(QVariant(currentName));
            requestData->append(QVariant(previousName));

            Q_EMIT requestResponse(cmd.m_elementPath, cmd.m_commandType, requestData);
            break;
        }
        case CommandType_RequestDataInputs: {
            QVariantList *requestData = new QVariantList();
            if (m_runtime) {
                const auto diList = m_runtime->dataInputs();

                for (const auto &it : diList) {
                    Q3DSDataInput *newIt = new Q3DSDataInput(it, nullptr);
                    newIt->d_ptr->m_max = m_runtime->dataInputMax(it);
                    newIt->d_ptr->m_min = m_runtime->dataInputMin(it);
                    newIt->d_ptr->m_metadata = m_runtime->dataInputMetadata(it);
                    requestData->append(QVariant::fromValue(newIt));
                }
            }

            Q_EMIT requestResponse(cmd.m_elementPath, cmd.m_commandType, requestData);
            break;
        }
        case CommandType_RequestDataOutputs: {
            QVariantList *requestData = new QVariantList();
            if (m_presentation) {
                const auto diList = m_presentation->dataOutputs();

                for (const auto &it : diList)
                    requestData->append(QVariant::fromValue(it->name()));
            }

            Q_EMIT requestResponse(cmd.m_elementPath, cmd.m_commandType, requestData);
            break;
        }
        case CommandType_RequestExportShaderCache: {
            QByteArray shaderCache = m_runtime->exportShaderCache(cmd.m_boolValue);
            QVariantList *requestData = new QVariantList();
            requestData->append(QVariant(shaderCache));

            Q_EMIT requestResponse({}, cmd.m_commandType, requestData);
            break;
        }

        default:
            qWarning() << __FUNCTION__ << "Unrecognized CommandType in command list!";
        }
    }

    m_commands.clear(false);
}

void Q3DSRenderer::handleRuntimeInitializedAsync()
{
    m_initialized = m_runtimeInitializerThread->wasSuccess();
    m_initializationFailure = !m_initialized;

    if (m_initializationFailure) {
        m_commands.clear(true);
        m_error = m_runtime->error();
        releaseRuntime();
    } else {
        m_runtime->finishAsyncInit();
        registerCallbacks();
        m_settings->d_ptr->setViewerApp(m_runtime);
        m_presentation->d_ptr->setViewerApp(m_runtime, false);

        connect(m_runtimeInitializerThread, &QThread::finished, [this]() {
            m_runtimeInitializerThread->deleteLater();
            m_runtimeInitializerThread = nullptr;
        });
        m_runtimeInitializerThread->quit();
    }
}

QT_END_NAMESPACE
