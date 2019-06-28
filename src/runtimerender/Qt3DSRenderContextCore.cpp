/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#include "Qt3DSRender.h"
#include "EABase/eabase.h" //char16_t definition
#include "Qt3DSRenderContextCore.h"
#include "foundation/StringTable.h"
#include "Qt3DSRenderNode.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSRenderer.h"
#include "Qt3DSRenderResourceManager.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSRenderEffectSystem.h"
#include "Qt3DSRenderShaderCache.h"
#include "foundation/Qt3DSFoundation.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "render/Qt3DSRenderRenderBuffer.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "Qt3DSRenderCamera.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSRenderThreadPool.h"
#include "Qt3DSRenderImageBatchLoader.h"
#include "Qt3DSRenderTextTextureCache.h"
#include "Qt3DSRenderTextTextureAtlas.h"
#include "Qt3DSRenderPlugin.h"
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderPixelGraphicsRenderer.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "Qt3DSRenderBufferLoader.h"
#include "foundation/FastAllocator.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "Qt3DSRenderRenderList.h"
#include "Qt3DSRenderPathManager.h"
#include "Qt3DSRenderShaderCodeGeneratorV2.h"
#include "Qt3DSRenderDefaultMaterialShaderGenerator.h"
#include "Qt3DSRenderCustomMaterialShaderGenerator.h"
#include "Qt3DSDistanceFieldRenderer.h"

using namespace qt3ds::render;

namespace {

struct SRenderContextCore : public IQt3DSRenderContextCore
{
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<IStringTable> m_StringTable;
    NVScopedRefCounted<IPerfTimer> m_PerfTimer;
    NVScopedRefCounted<IInputStreamFactory> m_InputStreamFactory;
    NVScopedRefCounted<IThreadPool> m_ThreadPool;
    NVScopedRefCounted<IDynamicObjectSystemCore> m_DynamicObjectSystem;
    NVScopedRefCounted<ICustomMaterialSystemCore> m_MaterialSystem;
    NVScopedRefCounted<IEffectSystemCore> m_EffectSystem;
    NVScopedRefCounted<IBufferLoader> m_BufferLoader;
    NVScopedRefCounted<IRenderPluginManagerCore> m_RenderPluginManagerCore;
    NVScopedRefCounted<ITextRendererCore> m_TextRenderer;
    NVScopedRefCounted<ITextRendererCore> m_OnscreenTexRenderer;
    NVScopedRefCounted<IPathManagerCore> m_PathManagerCore;
    NVScopedRefCounted<ITextRendererCore> m_distanceFieldRenderer;

    QT3DSI32 mRefCount;
    SRenderContextCore(NVFoundationBase &fnd, IStringTable &strTable)
        : m_Foundation(fnd)
        , m_StringTable(strTable)
        , m_PerfTimer(IPerfTimer::CreatePerfTimer(fnd))
        , m_InputStreamFactory(IInputStreamFactory::Create(fnd))
        , m_ThreadPool(IThreadPool::CreateThreadPool(fnd, 4))
        , mRefCount(0)
    {
        m_DynamicObjectSystem = IDynamicObjectSystemCore::CreateDynamicSystemCore(*this);
        m_MaterialSystem = ICustomMaterialSystemCore::CreateCustomMaterialSystemCore(*this);
        m_EffectSystem = IEffectSystemCore::CreateEffectSystemCore(*this);
        m_RenderPluginManagerCore =
            IRenderPluginManagerCore::Create(fnd, strTable, *m_InputStreamFactory);
        m_BufferLoader = IBufferLoader::Create(m_Foundation, *m_InputStreamFactory, *m_ThreadPool);
        m_PathManagerCore = IPathManagerCore::CreatePathManagerCore(*this);
    }

    virtual ~SRenderContextCore() {}

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    IStringTable &GetStringTable() override { return *m_StringTable; }
    NVFoundationBase &GetFoundation() override { return m_Foundation; }
    NVAllocatorCallback &GetAllocator() override { return m_Foundation.getAllocator(); }
    IInputStreamFactory &GetInputStreamFactory() override { return *m_InputStreamFactory; }
    IThreadPool &GetThreadPool() override { return *m_ThreadPool; }
    IDynamicObjectSystemCore &GetDynamicObjectSystemCore() override
    {
        return *m_DynamicObjectSystem;
    }
    ICustomMaterialSystemCore &GetMaterialSystemCore() override { return *m_MaterialSystem; }
    IEffectSystemCore &GetEffectSystemCore() override { return *m_EffectSystem; }
    IPerfTimer &GetPerfTimer() override { return *m_PerfTimer; }
    IBufferLoader &GetBufferLoader() override { return *m_BufferLoader; }
    IRenderPluginManagerCore &GetRenderPluginCore() override { return *m_RenderPluginManagerCore; }
    IPathManagerCore &GetPathManagerCore() override { return *m_PathManagerCore; }
    IQt3DSRenderContext &CreateRenderContext(NVRenderContext &inContext,
                                             const char8_t *inPrimitivesDirectory,
                                             bool delayedLoading) override;
    void SetTextRendererCore(ITextRendererCore &inRenderer) override { m_TextRenderer = inRenderer; }
    ITextRendererCore *GetTextRendererCore() override { return m_TextRenderer.mPtr; }
    void setDistanceFieldRenderer(ITextRendererCore &inRenderer) override
    {
        m_distanceFieldRenderer = inRenderer;
    }
    ITextRendererCore *getDistanceFieldRenderer() override { return m_distanceFieldRenderer.mPtr; }
    void SetOnscreenTextRendererCore(ITextRendererCore &inRenderer) override
    {
        m_OnscreenTexRenderer = inRenderer;
    }
    ITextRendererCore *GetOnscreenTextRendererCore() override { return m_OnscreenTexRenderer.mPtr; }
};

inline float Clamp(float val, float inMin = 0.0f, float inMax = 1.0f)
{
    if (val < inMin)
        return inMin;
    if (val > inMax)
        return inMax;
    return val;
}

struct SPerFrameAllocator : public NVAllocatorCallback
{
    SFastAllocator<> m_FastAllocator;
    SSAutoDeallocatorAllocator m_LargeAllocator;

    SPerFrameAllocator(NVAllocatorCallback &baseAllocator)
        : m_FastAllocator(baseAllocator, "PerFrameAllocation")
        , m_LargeAllocator(baseAllocator)
    {
    }

    inline void *allocate(size_t inSize, const char *inFile, int inLine)
    {
        if (inSize < 8192)
            return m_FastAllocator.allocate(inSize, "PerFrameAllocation", inFile, inLine, 0);
        else
            return m_LargeAllocator.allocate(inSize, "PerFrameAllocation", inFile, inLine, 0);
    }

    inline void *allocate(size_t inSize, const char *inFile, int inLine, int, int)
    {
        if (inSize < 8192)
            return m_FastAllocator.allocate(inSize, "PerFrameAllocation", inFile, inLine, 0);
        else
            return m_LargeAllocator.allocate(inSize, "PerFrameAllocation", inFile, inLine, 0);
    }

    inline void deallocate(void *, size_t) {}

    void reset()
    {
        m_FastAllocator.reset();
        m_LargeAllocator.deallocateAllAllocations();
    }

    void *allocate(size_t inSize, const char *typeName, const char *inFile, int inLine,
                           int flags = 0) override
    {
        if (inSize < SFastAllocator<>::SlabSize)
            return m_FastAllocator.allocate(inSize, typeName, inFile, inLine, flags);
        else
            return m_LargeAllocator.allocate(inSize, typeName, inFile, inLine, flags);
    }

    void *allocate(size_t inSize, const char *typeName, const char *inFile, int inLine,
                           size_t alignment, size_t alignmentOffset) override
    {
        if (inSize < SFastAllocator<>::SlabSize)
            return m_FastAllocator.allocate(inSize, typeName, inFile, inLine, alignment,
                                            alignmentOffset);
        else
            return m_LargeAllocator.allocate(inSize, typeName, inFile, inLine, alignment,
                                             alignmentOffset);
    }

    void deallocate(void *) override {}
};

struct SRenderContext : public IQt3DSRenderContext
{
    NVScopedRefCounted<NVRenderContext> m_RenderContext;
    NVScopedRefCounted<IQt3DSRenderContextCore> m_CoreContext;
    NVScopedRefCounted<IStringTable> m_StringTable;
    NVScopedRefCounted<IPerfTimer> m_PerfTimer;
    NVScopedRefCounted<IInputStreamFactory> m_InputStreamFactory;
    NVScopedRefCounted<IBufferManager> m_BufferManager;
    NVScopedRefCounted<IResourceManager> m_ResourceManager;
    NVScopedRefCounted<IOffscreenRenderManager> m_OffscreenRenderManager;
    NVScopedRefCounted<IQt3DSRenderer> m_Renderer;
    NVScopedRefCounted<ITextRenderer> m_TextRenderer;
    NVScopedRefCounted<ITextRenderer> m_distanceFieldRenderer;
    NVScopedRefCounted<ITextRenderer> m_OnscreenTextRenderer;
    NVScopedRefCounted<ITextTextureCache> m_TextTextureCache;
    NVScopedRefCounted<ITextTextureAtlas> m_TextTextureAtlas;
    NVScopedRefCounted<IDynamicObjectSystem> m_DynamicObjectSystem;
    NVScopedRefCounted<IEffectSystem> m_EffectSystem;
    NVScopedRefCounted<IShaderCache> m_ShaderCache;
    NVScopedRefCounted<IThreadPool> m_ThreadPool;
    NVScopedRefCounted<IImageBatchLoader> m_ImageBatchLoader;
    NVScopedRefCounted<IRenderPluginManager> m_RenderPluginManager;
    NVScopedRefCounted<ICustomMaterialSystem> m_CustomMaterialSystem;
    NVScopedRefCounted<IPixelGraphicsRenderer> m_PixelGraphicsRenderer;
    NVScopedRefCounted<IPathManager> m_PathManager;
    NVScopedRefCounted<IShaderProgramGenerator> m_ShaderProgramGenerator;
    NVScopedRefCounted<IDefaultMaterialShaderGenerator> m_DefaultMaterialShaderGenerator;
    NVScopedRefCounted<ICustomMaterialShaderGenerator> m_CustomMaterialShaderGenerator;
    SPerFrameAllocator m_PerFrameAllocator;
    NVScopedRefCounted<IRenderList> m_RenderList;
    QT3DSU32 m_FrameCount;
    volatile QT3DSI32 mRefCount;
    // Viewport that this render context should use
    Option<NVRenderRect> m_Viewport;
    QSize m_WindowDimensions;
    ScaleModes::Enum m_ScaleMode;
    bool m_WireframeMode;
    bool m_IsInSubPresentation;
    Option<QT3DSVec4> m_SceneColor;
    Option<QT3DSVec4> m_MatteColor;
    bool m_matteEnabled;
    RenderRotationValues::Enum m_Rotation;
    NVScopedRefCounted<NVRenderFrameBuffer> m_RotationFBO;
    NVScopedRefCounted<NVRenderTexture2D> m_RotationTexture;
    NVScopedRefCounted<NVRenderRenderBuffer> m_RotationDepthBuffer;
    NVRenderFrameBuffer *m_ContextRenderTarget;
    NVRenderRect m_PresentationViewport;
    QSize m_PresentationDimensions;
    QSize m_RenderPresentationDimensions;
    QSize m_PreRenderPresentationDimensions;
    QT3DSVec2 m_PresentationScale;
    NVRenderRect m_VirtualViewport;
    QPair<QT3DSF32, int> m_FPS;
    bool m_AuthoringMode;
    QVector<QT3DSF32> m_frameTimes;

    SRenderContext(NVRenderContext &ctx, IQt3DSRenderContextCore &inCore,
                   const char8_t *inApplicationDirectory, bool delayedLoading)
        : m_RenderContext(ctx)
        , m_CoreContext(inCore)
        , m_StringTable(ctx.GetStringTable())
        , m_PerfTimer(inCore.GetPerfTimer())
        , m_InputStreamFactory(inCore.GetInputStreamFactory())
        , m_BufferManager(
              IBufferManager::Create(ctx, *m_StringTable, *m_InputStreamFactory, *m_PerfTimer))
        , m_ResourceManager(IResourceManager::CreateResourceManager(ctx))
        , m_ShaderCache(IShaderCache::CreateShaderCache(ctx, *m_InputStreamFactory, *m_PerfTimer))
        , m_ThreadPool(inCore.GetThreadPool())
        , m_PerFrameAllocator(ctx.GetAllocator())
        , m_RenderList(IRenderList::CreateRenderList(ctx.GetFoundation()))
        , m_FrameCount(0)
        , mRefCount(0)
        , m_WindowDimensions(800, 480)
        , m_ScaleMode(ScaleModes::ExactSize)
        , m_WireframeMode(false)
        , m_IsInSubPresentation(false)
        , m_matteEnabled(false)
        , m_Rotation(RenderRotationValues::NoRotation)
        , m_ContextRenderTarget(NULL)
        , m_PresentationScale(0, 0)
        , m_FPS(qMakePair(0.0, 0))
        , m_AuthoringMode(false)
    {
        m_BufferManager->enableReloadableResources(delayedLoading);
        m_OffscreenRenderManager = IOffscreenRenderManager::CreateOffscreenRenderManager(
            ctx.GetAllocator(), *m_StringTable, *m_ResourceManager, *this);
        m_Renderer = IQt3DSRenderer::CreateRenderer(*this);
        if (inApplicationDirectory && *inApplicationDirectory)
            m_InputStreamFactory->AddSearchDirectory(inApplicationDirectory);

        m_ImageBatchLoader =
            IImageBatchLoader::CreateBatchLoader(ctx.GetFoundation(), *m_InputStreamFactory,
                                                 *m_BufferManager, *m_ThreadPool, *m_PerfTimer);
        m_RenderPluginManager = inCore.GetRenderPluginCore().GetRenderPluginManager(ctx);
        m_DynamicObjectSystem = inCore.GetDynamicObjectSystemCore().CreateDynamicSystem(*this);
        m_EffectSystem = inCore.GetEffectSystemCore().GetEffectSystem(*this);
        m_CustomMaterialSystem = inCore.GetMaterialSystemCore().GetCustomMaterialSystem(*this);
        // as does the custom material system
        m_PixelGraphicsRenderer = IPixelGraphicsRenderer::CreateRenderer(*this, *m_StringTable);
        ITextRendererCore *theTextCore = inCore.GetTextRendererCore();
        m_ShaderProgramGenerator = IShaderProgramGenerator::CreateProgramGenerator(*this);
        m_DefaultMaterialShaderGenerator =
            IDefaultMaterialShaderGenerator::CreateDefaultMaterialShaderGenerator(*this);
        m_CustomMaterialShaderGenerator =
            ICustomMaterialShaderGenerator::CreateCustomMaterialShaderGenerator(*this);
        if (theTextCore) {
            m_TextRenderer = theTextCore->GetTextRenderer(ctx);
            m_TextTextureCache = ITextTextureCache::CreateTextureCache(
                m_RenderContext->GetFoundation(), *m_TextRenderer, *m_RenderContext);
        }

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
        ITextRendererCore *distanceFieldRenderer = inCore.getDistanceFieldRenderer();
        if (distanceFieldRenderer) {
            m_distanceFieldRenderer = distanceFieldRenderer->GetTextRenderer(ctx);
            static_cast<Q3DSDistanceFieldRenderer *>(m_distanceFieldRenderer.mPtr)
                    ->setContext(*this);
        }
#endif

        ITextRendererCore *theOnscreenTextCore = inCore.GetOnscreenTextRendererCore();
        if (theOnscreenTextCore) {
            m_OnscreenTextRenderer = theOnscreenTextCore->GetTextRenderer(ctx);
            m_TextTextureAtlas = ITextTextureAtlas::CreateTextureAtlas(
                m_RenderContext->GetFoundation(), *m_OnscreenTextRenderer, *m_RenderContext);
        }
        m_PathManager = inCore.GetPathManagerCore().OnRenderSystemInitialize(*this);

#if defined (QT3DS_SHADER_PLATFORM_LIBRARY_DIR)
        const QString platformDirectory;
#if defined(_WIN32)
        platformDirectory = QStringLiteral("res/platform/win");
#elif defined(_LINUX)
        platformDirectory = QStringLiteral("res/platform/linux");
#elif defined(_MACOSX)
        platformDirectory = QStringLiteral("res/platform/macos");
#endif
        GetDynamicObjectSystem().setShaderCodeLibraryPlatformDirectory(platformDirectory);
#endif
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_RenderContext->GetAllocator());

    IStringTable &GetStringTable() override { return *m_StringTable; }
    NVFoundationBase &GetFoundation() override { return m_RenderContext->GetFoundation(); }
    NVAllocatorCallback &GetAllocator() override { return m_RenderContext->GetAllocator(); }
    IQt3DSRenderer &GetRenderer() override { return *m_Renderer; }
    IBufferManager &GetBufferManager() override { return *m_BufferManager; }
    IResourceManager &GetResourceManager() override { return *m_ResourceManager; }
    NVRenderContext &GetRenderContext() override { return *m_RenderContext; }
    IOffscreenRenderManager &GetOffscreenRenderManager() override
    {
        return *m_OffscreenRenderManager;
    }
    IInputStreamFactory &GetInputStreamFactory() override { return *m_InputStreamFactory; }
    IEffectSystem &GetEffectSystem() override { return *m_EffectSystem; }
    IShaderCache &GetShaderCache() override { return *m_ShaderCache; }
    IThreadPool &GetThreadPool() override { return *m_ThreadPool; }
    IImageBatchLoader &GetImageBatchLoader() override { return *m_ImageBatchLoader; }
    ITextTextureCache *GetTextureCache() override { return m_TextTextureCache.mPtr; }
    ITextTextureAtlas *GetTextureAtlas() override { return m_TextTextureAtlas.mPtr; }
    IRenderPluginManager &GetRenderPluginManager() override { return *m_RenderPluginManager; }
    IDynamicObjectSystem &GetDynamicObjectSystem() override { return *m_DynamicObjectSystem; }
    ICustomMaterialSystem &GetCustomMaterialSystem() override { return *m_CustomMaterialSystem; }
    IPixelGraphicsRenderer &GetPixelGraphicsRenderer() override { return *m_PixelGraphicsRenderer; }
    IPerfTimer &GetPerfTimer() override { return *m_PerfTimer; }
    IRenderList &GetRenderList() override { return *m_RenderList; }
    IPathManager &GetPathManager() override { return *m_PathManager; }
    IShaderProgramGenerator &GetShaderProgramGenerator() override
    {
        return *m_ShaderProgramGenerator;
    }
    IDefaultMaterialShaderGenerator &GetDefaultMaterialShaderGenerator() override
    {
        return *m_DefaultMaterialShaderGenerator;
    }
    ICustomMaterialShaderGenerator &GetCustomMaterialShaderGenerator() override
    {
        return *m_CustomMaterialShaderGenerator;
    }
    NVAllocatorCallback &GetPerFrameAllocator() override { return m_PerFrameAllocator; }

    QT3DSU32 GetFrameCount() override { return m_FrameCount; }
    void SetFPS(QPair<QT3DSF32, int> inFPS) override { m_FPS = inFPS; }
    QPair<QT3DSF32, int> GetFPS(void) override { return m_FPS; }

    void SetFrameTime(QT3DSF32 time) override
    {
        m_frameTimes.push_front(time);
        // Store only one value for now. This can be increased once we have proper graph for
        // the frame times.
        if (m_frameTimes.size() > 1)
            m_frameTimes.pop_back();
    }
    QVector<QT3DSF32> GetFrameTimes() const override
    {
        return m_frameTimes;
    }

    bool IsAuthoringMode() override { return m_AuthoringMode; }
    void SetAuthoringMode(bool inMode) override { m_AuthoringMode = inMode; }

    bool IsInSubPresentation() override { return m_IsInSubPresentation; }
    void SetInSubPresentation(bool inValue) override { m_IsInSubPresentation = inValue; }

    ITextRenderer *GetTextRenderer() override { return m_TextRenderer; }

    ITextRenderer *getDistanceFieldRenderer() override { return m_distanceFieldRenderer; }

    ITextRenderer *GetOnscreenTextRenderer() override { return m_OnscreenTextRenderer; }

    void SetSceneColor(Option<QT3DSVec4> inSceneColor) override { m_SceneColor = inSceneColor; }
    void SetMatteColor(Option<QT3DSVec4> inMatteColor) override { m_MatteColor = inMatteColor; }
    void setMatteEnabled(bool enable) override
    {
        m_matteEnabled = enable;
    }

    void SetWindowDimensions(const QSize &inWindowDimensions) override
    {
        m_WindowDimensions = inWindowDimensions;
    }

    QSize GetWindowDimensions() override { return m_WindowDimensions; }

    void SetScaleMode(ScaleModes::Enum inMode) override { m_ScaleMode = inMode; }

    ScaleModes::Enum GetScaleMode() override { return m_ScaleMode; }

    void SetWireframeMode(bool inEnable) override { m_WireframeMode = inEnable; }

    bool GetWireframeMode() override { return m_WireframeMode; }

    void SetViewport(Option<NVRenderRect> inViewport) override { m_Viewport = inViewport; }
    Option<NVRenderRect> GetViewport() const override { return m_Viewport; }

    IRenderWidgetContext &GetRenderWidgetContext() override
    {
        return m_Renderer->GetRenderWidgetContext();
    }

    eastl::pair<NVRenderRect, NVRenderRect> GetPresentationViewportAndOuterViewport() const
    {
        QSize thePresentationDimensions(m_PresentationDimensions);
        NVRenderRect theOuterViewport(GetContextViewport());
        if (m_Rotation == RenderRotationValues::Clockwise90
            || m_Rotation == RenderRotationValues::Clockwise270) {
            eastl::swap(theOuterViewport.m_Width, theOuterViewport.m_Height);
            eastl::swap(theOuterViewport.m_X, theOuterViewport.m_Y);
        }
        // Calculate the presentation viewport perhaps with the window width and height swapped.
        return eastl::make_pair(
            GetPresentationViewport(theOuterViewport, m_ScaleMode, thePresentationDimensions),
            theOuterViewport);
    }

    NVRenderRectF GetDisplayViewport() const override
    {
        return GetPresentationViewportAndOuterViewport().first;
    }

    void SetPresentationDimensions(const QSize &inPresentationDimensions) override
    {
        m_PresentationDimensions = inPresentationDimensions;
    }
    QSize GetCurrentPresentationDimensions() const override
    {
        return m_PresentationDimensions;
    }

    void SetRenderRotation(RenderRotationValues::Enum inRotation) override
    {
        m_Rotation = inRotation;
    }

    RenderRotationValues::Enum GetRenderRotation() const override { return m_Rotation; }
    QT3DSVec2 GetMousePickViewport() const override
    {
        bool renderOffscreen = m_Rotation != RenderRotationValues::NoRotation;
        if (renderOffscreen)
            return QT3DSVec2((QT3DSF32)m_PresentationViewport.m_Width,
                          (QT3DSF32)m_PresentationViewport.m_Height);
        else
            return QT3DSVec2((QT3DSF32)m_WindowDimensions.width(), (QT3DSF32)m_WindowDimensions.height());
    }
    NVRenderRect GetContextViewport() const override
    {
        NVRenderRect retval;
        if (m_Viewport.hasValue())
            retval = *m_Viewport;
        else
            retval = NVRenderRect(0, 0, m_WindowDimensions.width(), m_WindowDimensions.height());

        return retval;
    }

    QT3DSVec2 GetMousePickMouseCoords(const QT3DSVec2 &inMouseCoords) const override
    {
        bool renderOffscreen = m_Rotation != RenderRotationValues::NoRotation;
        if (renderOffscreen) {
            QSize thePresentationDimensions(m_RenderPresentationDimensions);
            NVRenderRect theViewport(GetContextViewport());
            // Calculate the presentation viewport perhaps with the presentation width and height
            // swapped.
            NVRenderRect thePresentationViewport =
                GetPresentationViewport(theViewport, m_ScaleMode, thePresentationDimensions);
            // Translate pick into presentation space without rotations or anything else.
            QT3DSF32 YHeightDiff = (QT3DSF32)((QT3DSF32)m_WindowDimensions.height()
                                        - (QT3DSF32)thePresentationViewport.m_Height);
            QT3DSVec2 theLocalMouse((inMouseCoords.x - thePresentationViewport.m_X),
                                 (inMouseCoords.y - YHeightDiff + thePresentationViewport.m_Y));
            switch (m_Rotation) {
            default:
            case RenderRotationValues::NoRotation:
                QT3DS_ASSERT(false);
                break;
            case RenderRotationValues::Clockwise90:
                eastl::swap(theLocalMouse.x, theLocalMouse.y);
                theLocalMouse.y = thePresentationViewport.m_Width - theLocalMouse.y;
                break;
            case RenderRotationValues::Clockwise180:
                theLocalMouse.y = thePresentationViewport.m_Height - theLocalMouse.y;
                theLocalMouse.x = thePresentationViewport.m_Width - theLocalMouse.x;
                break;
            case RenderRotationValues::Clockwise270:
                eastl::swap(theLocalMouse.x, theLocalMouse.y);
                theLocalMouse.x = thePresentationViewport.m_Height - theLocalMouse.x;
                break;
            }
            return theLocalMouse;
        }
        return inMouseCoords;
    }

    NVRenderRect GetPresentationViewport(const NVRenderRect &inViewerViewport,
                                         ScaleModes::Enum inScaleToFit,
                                         const QSize &inPresDimensions) const
    {
        NVRenderRect retval;
        QT3DSI32 theWidth = inViewerViewport.m_Width;
        QT3DSI32 theHeight = inViewerViewport.m_Height;
        if (inPresDimensions.width() == 0 || inPresDimensions.height() == 0)
            return NVRenderRect(0, 0, 0, 0);
        // Setup presentation viewport.  This may or may not match the physical viewport that we
        // want to setup.
        // Avoiding scaling keeps things as sharp as possible.
        if (inScaleToFit == ScaleModes::ExactSize) {
            retval.m_Width = inPresDimensions.width();
            retval.m_Height = inPresDimensions.height();
            retval.m_X = (theWidth - (QT3DSI32)inPresDimensions.width()) / 2;
            retval.m_Y = (theHeight - (QT3DSI32)inPresDimensions.height()) / 2;
        } else if (inScaleToFit == ScaleModes::ScaleToFit
                   || inScaleToFit == ScaleModes::FitSelected) {
            // Scale down in such a way to preserve aspect ratio.
            float screenAspect = (float)theWidth / (float)theHeight;
            float thePresentationAspect =
                (float)inPresDimensions.width() / (float)inPresDimensions.height();
            if (screenAspect >= thePresentationAspect) {
                // if the screen height is the limiting factor
                retval.m_Y = 0;
                retval.m_Height = theHeight;
                retval.m_Width = (QT3DSI32)(thePresentationAspect * retval.m_Height);
                retval.m_X = (theWidth - retval.m_Width) / 2;
            } else {
                retval.m_X = 0;
                retval.m_Width = theWidth;
                retval.m_Height = (QT3DSI32)(retval.m_Width / thePresentationAspect);
                retval.m_Y = (theHeight - retval.m_Height) / 2;
            }
        } else {
            // Setup the viewport for everything and let the presentations figure it out.
            retval.m_X = 0;
            retval.m_Y = 0;
            retval.m_Width = theWidth;
            retval.m_Height = theHeight;
        }
        retval.m_X += inViewerViewport.m_X;
        retval.m_Y += inViewerViewport.m_Y;
        return retval;
    }

    void RenderText2D(QT3DSF32 x, QT3DSF32 y, qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor,
                              const char *text) override
    {
        m_Renderer->RenderText2D(x, y, inColor, text);
    }

    void RenderGpuProfilerStats(QT3DSF32 x, QT3DSF32 y,
                                        qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor) override
    {
        m_Renderer->RenderGpuProfilerStats(x, y, inColor);
    }

    NVRenderRect GetPresentationViewport() const override { return m_PresentationViewport; }
    struct SBeginFrameResult
    {
        bool m_RenderOffscreen;
        QSize m_PresentationDimensions;
        bool m_ScissorTestEnabled;
        NVRenderRect m_ScissorRect;
        NVRenderRect m_Viewport;
        QSize m_FBODimensions;
        SBeginFrameResult(bool ro, QSize presDims, bool scissorEnabled,
                          NVRenderRect scissorRect, NVRenderRect viewport,
                          QSize fboDims)
            : m_RenderOffscreen(ro)
            , m_PresentationDimensions(presDims)
            , m_ScissorTestEnabled(scissorEnabled)
            , m_ScissorRect(scissorRect)
            , m_Viewport(viewport)
            , m_FBODimensions(fboDims)
        {
        }
        SBeginFrameResult() {}
    };

    // Calculated values passed from beginframe to setupRenderTarget.
    // Trying to avoid duplicate code as much as possible.
    SBeginFrameResult m_BeginFrameResult;

    void BeginFrame(bool firstFrame) override
    {
        m_PreRenderPresentationDimensions = m_PresentationDimensions;
        QSize thePresentationDimensions(m_PreRenderPresentationDimensions);
        NVRenderRect theContextViewport(GetContextViewport());
        m_PerFrameAllocator.reset();
        IRenderList &theRenderList(*m_RenderList);
        theRenderList.BeginFrame();
        if (m_Viewport.hasValue()) {
            theRenderList.SetScissorTestEnabled(true);
            theRenderList.SetScissorRect(theContextViewport);
        } else {
            theRenderList.SetScissorTestEnabled(false);
        }
        bool renderOffscreen = m_Rotation != RenderRotationValues::NoRotation;
        eastl::pair<NVRenderRect, NVRenderRect> thePresViewportAndOuterViewport =
            GetPresentationViewportAndOuterViewport();
        NVRenderRect theOuterViewport = thePresViewportAndOuterViewport.second;
        // Calculate the presentation viewport perhaps with the window width and height swapped.
        NVRenderRect thePresentationViewport = thePresViewportAndOuterViewport.first;
        m_PresentationViewport = thePresentationViewport;
        m_PresentationScale = QT3DSVec2(
            (QT3DSF32)thePresentationViewport.m_Width / (QT3DSF32)thePresentationDimensions.width(),
            (QT3DSF32)thePresentationViewport.m_Height / (QT3DSF32)thePresentationDimensions.height());
        QSize fboDimensions;
        if (thePresentationViewport.m_Width > 0 && thePresentationViewport.m_Height > 0) {
            if (renderOffscreen == false) {
                m_PresentationDimensions = QSize(thePresentationViewport.m_Width,
                                                             thePresentationViewport.m_Height);
                m_RenderList->SetViewport(thePresentationViewport);
                if (thePresentationViewport.m_X || thePresentationViewport.m_Y
                    || thePresentationViewport.m_Width != (QT3DSI32)theOuterViewport.m_Width
                    || thePresentationViewport.m_Height != (QT3DSI32)theOuterViewport.m_Height) {
                    m_RenderList->SetScissorRect(thePresentationViewport);
                    m_RenderList->SetScissorTestEnabled(true);
                }
            } else {
                QT3DSU32 imageWidth = ITextRenderer::NextMultipleOf4(thePresentationViewport.m_Width);
                QT3DSU32 imageHeight =
                    ITextRenderer::NextMultipleOf4(thePresentationViewport.m_Height);
                fboDimensions = QSize(imageWidth, imageHeight);
                m_PresentationDimensions = QSize(thePresentationViewport.m_Width,
                                                             thePresentationViewport.m_Height);
                NVRenderRect theSceneViewport = NVRenderRect(0, 0, imageWidth, imageHeight);
                m_RenderList->SetScissorTestEnabled(false);
                m_RenderList->SetViewport(theSceneViewport);
            }
        }

        m_BeginFrameResult = SBeginFrameResult(
            renderOffscreen, m_PresentationDimensions, m_RenderList->IsScissorTestEnabled(),
            m_RenderList->GetScissor(), m_RenderList->GetViewport(), fboDimensions);

        m_Renderer->BeginFrame();
        m_OffscreenRenderManager->BeginFrame();
        if (m_TextRenderer)
            m_TextRenderer->BeginFrame();
        if (m_TextTextureCache)
            m_TextTextureCache->BeginFrame();
        m_ImageBatchLoader->BeginFrame(firstFrame);
    }

    QT3DSVec2 GetPresentationScaleFactor() const override { return m_PresentationScale; }

    virtual void SetupRenderTarget()
    {
        NVRenderRect theContextViewport(GetContextViewport());
        if (m_Viewport.hasValue()) {
            m_RenderContext->SetScissorTestEnabled(true);
            m_RenderContext->SetScissorRect(theContextViewport);
        } else {
            m_RenderContext->SetScissorTestEnabled(false);
        }
        {
            QT3DSVec4 theClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            if (m_MatteColor.hasValue() && m_matteEnabled)
                theClearColor = m_MatteColor;
            else if (m_SceneColor.hasValue())
                theClearColor = m_SceneColor;
            //premultiply the clear color
            if (theClearColor.w < 1.0f) {
                theClearColor.x *= theClearColor.w;
                theClearColor.y *= theClearColor.w;
                theClearColor.z *= theClearColor.w;
            }
            m_RenderContext->SetClearColor(theClearColor);
            m_RenderContext->Clear(qt3ds::render::NVRenderClearValues::Color);
        }
        bool renderOffscreen = m_BeginFrameResult.m_RenderOffscreen;
        m_RenderContext->SetViewport(m_BeginFrameResult.m_Viewport);
        m_RenderContext->SetScissorRect(m_BeginFrameResult.m_ScissorRect);
        m_RenderContext->SetScissorTestEnabled(m_BeginFrameResult.m_ScissorTestEnabled);

        if (m_PresentationViewport.m_Width > 0 && m_PresentationViewport.m_Height > 0) {
            if (renderOffscreen == false) {
                if (m_RotationFBO != NULL) {
                    m_ResourceManager->Release(*m_RotationFBO);
                    m_ResourceManager->Release(*m_RotationTexture);
                    m_ResourceManager->Release(*m_RotationDepthBuffer);
                    m_RotationFBO = NULL;
                    m_RotationTexture = NULL;
                    m_RotationDepthBuffer = NULL;
                }
                if (m_SceneColor.hasValue() && m_SceneColor.getValue().w != 0.0f) {
                    QT3DSVec4 theClearColor = m_SceneColor;
                    if (theClearColor.w < 1.0f) {
                        theClearColor.x *= theClearColor.w;
                        theClearColor.y *= theClearColor.w;
                        theClearColor.z *= theClearColor.w;
                    }
                    m_RenderContext->SetClearColor(theClearColor);
                    m_RenderContext->Clear(qt3ds::render::NVRenderClearValues::Color);
                }
            } else {
                QT3DSU32 imageWidth = m_BeginFrameResult.m_FBODimensions.width();
                QT3DSU32 imageHeight = m_BeginFrameResult.m_FBODimensions.height();
                NVRenderTextureFormats::Enum theColorBufferFormat = NVRenderTextureFormats::RGBA8;
                NVRenderRenderBufferFormats::Enum theDepthBufferFormat =
                    NVRenderRenderBufferFormats::Depth16;
                m_ContextRenderTarget = m_RenderContext->GetRenderTarget();
                if (m_RotationFBO == NULL) {
                    m_RotationFBO = m_ResourceManager->AllocateFrameBuffer();
                    m_RotationTexture = m_ResourceManager->AllocateTexture2D(
                        imageWidth, imageHeight, theColorBufferFormat);
                    m_RotationDepthBuffer = m_ResourceManager->AllocateRenderBuffer(
                        imageWidth, imageHeight, theDepthBufferFormat);
                    m_RotationFBO->Attach(NVRenderFrameBufferAttachments::Color0,
                                          *m_RotationTexture);
                    m_RotationFBO->Attach(NVRenderFrameBufferAttachments::Depth,
                                          *m_RotationDepthBuffer);
                } else {
                    STextureDetails theDetails = m_RotationTexture->GetTextureDetails();
                    if (theDetails.m_Width != imageWidth || theDetails.m_Height != imageHeight) {
                        m_RotationTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, imageWidth,
                                                          imageHeight, theColorBufferFormat);
                        m_RotationDepthBuffer->SetDimensions(
                            qt3ds::render::NVRenderRenderBufferDimensions(imageWidth, imageHeight));
                    }
                }
                m_RenderContext->SetRenderTarget(m_RotationFBO);
                if (m_SceneColor.hasValue()) {
                    QT3DSVec4 theClearColor = m_SceneColor;
                    if (theClearColor.w < 1.0f) {
                        theClearColor.x *= theClearColor.w;
                        theClearColor.y *= theClearColor.w;
                        theClearColor.z *= theClearColor.w;
                    }
                    m_RenderContext->SetClearColor(theClearColor);
                    m_RenderContext->Clear(qt3ds::render::NVRenderClearValues::Color);
                }
            }
        }
    }

    void RunRenderTasks() override
    {
        m_RenderList->RunRenderTasks();
        SetupRenderTarget();
    }

    // Note this runs before EndFrame
    virtual void TeardownRenderTarget()
    {
        if (m_RotationFBO) {
            ScaleModes::Enum theScaleToFit = m_ScaleMode;
            NVRenderRect theOuterViewport(GetContextViewport());
            m_RenderContext->SetRenderTarget(m_ContextRenderTarget);
            QSize thePresentationDimensions = GetCurrentPresentationDimensions();
            if (m_Rotation == RenderRotationValues::Clockwise90
                || m_Rotation == RenderRotationValues::Clockwise270) {
                thePresentationDimensions = QSize(thePresentationDimensions.height(),
                                                  thePresentationDimensions.width());
            }
            m_RenderPresentationDimensions = thePresentationDimensions;
            // Calculate the presentation viewport perhaps with the presentation width and height
            // swapped.
            NVRenderRect thePresentationViewport =
                GetPresentationViewport(theOuterViewport, theScaleToFit, thePresentationDimensions);
            SCamera theCamera;
            switch (m_Rotation) {
            default:
                QT3DS_ASSERT(false);
                break;
            case RenderRotationValues::Clockwise90:
                theCamera.m_Rotation.z = 90;
                break;
            case RenderRotationValues::Clockwise180:
                theCamera.m_Rotation.z = 180;
                break;
            case RenderRotationValues::Clockwise270:
                theCamera.m_Rotation.z = 270;
                break;
            }
            TORAD(theCamera.m_Rotation.z);
            theCamera.MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
            theCamera.m_Flags.SetOrthographic(true);
            m_RenderContext->SetViewport(thePresentationViewport);
            QT3DSVec2 theCameraDimensions((QT3DSF32)thePresentationViewport.m_Width,
                                       (QT3DSF32)thePresentationViewport.m_Height);
            theCamera.CalculateGlobalVariables(
                NVRenderRect(0, 0, (QT3DSU32)thePresentationViewport.m_Width,
                             (QT3DSU32)thePresentationViewport.m_Height),
                theCameraDimensions);
            QT3DSMat44 theVP;
            theCamera.CalculateViewProjectionMatrix(theVP);
            SNode theTempNode;
            theTempNode.CalculateGlobalVariables();
            QT3DSMat44 theMVP;
            QT3DSMat33 theNormalMat;
            theTempNode.CalculateMVPAndNormalMatrix(theVP, theMVP, theNormalMat);
            m_RenderContext->SetCullingEnabled(false);
            m_RenderContext->SetBlendingEnabled(false);
            m_RenderContext->SetDepthTestEnabled(false);
            m_Renderer->RenderQuad(QT3DSVec2((QT3DSF32)m_PresentationViewport.m_Width,
                                          (QT3DSF32)m_PresentationViewport.m_Height),
                                   theMVP, *m_RotationTexture);
        }
    }

    void EndFrame() override
    {
        TeardownRenderTarget();
        m_ImageBatchLoader->EndFrame();
        if (m_TextTextureCache)
            m_TextTextureCache->EndFrame();
        if (m_TextRenderer)
            m_TextRenderer->EndFrame();
        if (m_distanceFieldRenderer)
            m_distanceFieldRenderer->EndFrame();
        m_OffscreenRenderManager->EndFrame();
        m_Renderer->EndFrame();
        m_CustomMaterialSystem->EndFrame();
        m_PresentationDimensions = m_PreRenderPresentationDimensions;
        ++m_FrameCount;
    }
};

IQt3DSRenderContext &SRenderContextCore::CreateRenderContext(NVRenderContext &inContext,
                                                             const char8_t *inPrimitivesDirectory,
                                                             bool delayedLoading)
{
    return *QT3DS_NEW(m_Foundation.getAllocator(), SRenderContext)(inContext, *this,
                                                                   inPrimitivesDirectory,
                                                                   delayedLoading);
}
}

IQt3DSRenderContextCore &IQt3DSRenderContextCore::Create(NVFoundationBase &fnd, IStringTable &strt)
{
    return *QT3DS_NEW(fnd.getAllocator(), SRenderContextCore)(fnd, strt);
}
