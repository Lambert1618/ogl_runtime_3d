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
#include "Qt3DSOffscreenRenderManager.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/StringTable.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "Qt3DSRenderResourceManager.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSFoundation.h"
#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSOffscreenRenderKey.h"
#include "foundation/FastAllocator.h"
#include "Qt3DSRenderRenderList.h"
#include "Qt3DSRenderResourceTexture2D.h"
#include "Qt3DSRenderResourceBufferObjects.h"
#include "Qt3DSRendererUtil.h"

using namespace qt3ds::render;

namespace eastl {
template <>
struct hash<SOffscreenRendererKey>
{
    size_t operator()(const SOffscreenRendererKey &key) const
    {
        switch (key.getType()) {
        case OffscreenRendererKeyTypes::RegisteredString:
            return hash<CRegisteredString>()(key.getData<CRegisteredString>());
        case OffscreenRendererKeyTypes::VoidPtr:
            return hash<size_t>()(reinterpret_cast<size_t>(key.getData<void *>()));
        default:
            break;
        }
        QT3DS_ASSERT(false);
        return 0;
    }
    bool operator()(const SOffscreenRendererKey &lhs, const SOffscreenRendererKey &rhs) const
    {
        return lhs == rhs;
    }
};
}

namespace {

using eastl::pair;
using eastl::make_pair;

struct SRendererData : SOffscreenRenderResult
{
    NVAllocatorCallback &m_Allocator;
    IResourceManager &m_ResourceManager;
    QT3DSU32 m_FrameCount;
    bool m_Rendering;

    SRendererData(NVAllocatorCallback &inAllocator, IResourceManager &inResourceManager)
        : m_Allocator(inAllocator)
        , m_ResourceManager(inResourceManager)
        , m_FrameCount(QT3DS_MAX_U32)
        , m_Rendering(false)
    {
    }
    ~SRendererData()
    {
        if (m_Texture)
            m_ResourceManager.Release(*m_Texture);
        m_Texture = NULL;
    }
    void Release() { NVDelete(m_Allocator, this); }
};

struct SScopedRenderDataRenderMarker
{
    SRendererData &m_Data;
    SScopedRenderDataRenderMarker(SRendererData &d)
        : m_Data(d)
    {
        QT3DS_ASSERT(m_Data.m_Rendering == false);
        m_Data.m_Rendering = true;
    }
    ~SScopedRenderDataRenderMarker() { m_Data.m_Rendering = false; }
};

struct SRenderDataReleaser
{
    SRendererData *mPtr;
    SRenderDataReleaser(SRendererData *inItem)
        : mPtr(inItem)
    {
    }
    // Transfer ownership
    SRenderDataReleaser(const SRenderDataReleaser &inOther)
        : mPtr(inOther.mPtr)
    {
        const_cast<SRenderDataReleaser &>(inOther).mPtr = NULL;
    }

    ~SRenderDataReleaser()
    {
        if (mPtr)
            mPtr->Release();
    }
};
struct SOffscreenRenderManager;

struct SOffscreenRunnable : public IRenderTask
{
    SOffscreenRenderManager &m_RenderManager;
    SRendererData &m_Data;
    SOffscreenRendererEnvironment m_DesiredEnvironment;
    SOffscreenRunnable(SOffscreenRenderManager &rm, SRendererData &data,
                       SOffscreenRendererEnvironment env)
        : m_RenderManager(rm)
        , m_Data(data)
        , m_DesiredEnvironment(env)
    {
    }
    void Run() override;
};

struct SOffscreenRenderManager : public IOffscreenRenderManager
{
    typedef nvhash_map<SOffscreenRendererKey, SRenderDataReleaser> TRendererMap;
    IQt3DSRenderContext &m_Context;
    NVAllocatorCallback &m_Allocator;
    NVScopedRefCounted<IStringTable> m_StringTable;
    NVScopedRefCounted<IResourceManager> m_ResourceManager;
    TRendererMap m_Renderers;
    SFastAllocator<> m_PerFrameAllocator;
    QT3DSU32 m_FrameCount; // cheap per-

    volatile QT3DSI32 mRefCount;

    SOffscreenRenderManager(NVAllocatorCallback &inCallback, IStringTable &inStringTable,
                            IResourceManager &inManager, IQt3DSRenderContext &inContext)
        : m_Context(inContext)
        , m_Allocator(inCallback)
        , m_StringTable(inStringTable)
        , m_ResourceManager(inManager)
        , m_Renderers(inCallback, "SOffscreenRenderManager::m_Renderers")
        , m_PerFrameAllocator(inCallback, "m_PerFrameAllocator")
        , m_FrameCount(0)
        , mRefCount(0)
    {
    }

    virtual ~SOffscreenRenderManager() {}

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Allocator)

    Option<bool> MaybeRegisterOffscreenRenderer(const SOffscreenRendererKey &inKey,
                                                        IOffscreenRenderer &inRenderer) override
    {
        TRendererMap::iterator theIter = m_Renderers.find(inKey);
        if (theIter != m_Renderers.end()) {
            SRendererData &theData = *(theIter->second.mPtr);
            if (theData.m_Renderer != &inRenderer) {
                if (inKey.getType() == OffscreenRendererKeyTypes::RegisteredString) {
                    qCCritical(INVALID_OPERATION, "Different renderers registered under same key: %s",
                        inKey.getData<CRegisteredString>().c_str());
                }
                QT3DS_ASSERT(false);
                return Empty();
            }
            return false;
        }
        RegisterOffscreenRenderer(inKey, inRenderer);
        return true;
    }

    void RegisterOffscreenRenderer(const SOffscreenRendererKey &inKey,
                                           IOffscreenRenderer &inRenderer) override
    {
        pair<TRendererMap::iterator, bool> theInsert = m_Renderers.insert(
            make_pair(inKey, QT3DS_NEW(m_Allocator, SRendererData)(m_Allocator, *m_ResourceManager)));
        QT3DS_ASSERT(theInsert.second);
        SRendererData &theData = *(theInsert.first->second.mPtr);
        theData.m_Renderer = &inRenderer;
    }

    bool HasOffscreenRenderer(const SOffscreenRendererKey &inKey) override
    {
        return m_Renderers.find(inKey) != m_Renderers.end();
    }

    IOffscreenRenderer *GetOffscreenRenderer(const SOffscreenRendererKey &inKey) override
    {
        TRendererMap::iterator theRenderer = m_Renderers.find(inKey);
        if (theRenderer != m_Renderers.end()) {
            SRendererData &theData = *theRenderer->second.mPtr;
            return theData.m_Renderer;
        }
        return NULL;
    }
    void ReleaseOffscreenRenderer(const SOffscreenRendererKey &inKey) override
    {
        m_Renderers.erase(inKey);
    }

    void RenderItem(SRendererData &theData, SOffscreenRendererEnvironment theDesiredEnvironment)
    {
        NVRenderContext &theContext = m_ResourceManager->GetRenderContext();
        QT3DSVec2 thePresScaleFactor = m_Context.GetPresentationScaleFactor();
        SOffscreenRendererEnvironment theOriginalDesiredEnvironment(theDesiredEnvironment);
        // Ensure that our overall render context comes back no matter what the client does.
        qt3ds::render::NVRenderContextScopedProperty<QT3DSVec4> __clearColor(
            theContext, &NVRenderContext::GetClearColor, &NVRenderContext::SetClearColor,
            QT3DSVec4(0, 0, 0, 0));
        qt3ds::render::NVRenderContextScopedProperty<bool> __scissorEnabled(
            theContext, &NVRenderContext::IsScissorTestEnabled,
            &NVRenderContext::SetScissorTestEnabled, false);
        qt3ds::render::NVRenderContextScopedProperty<NVRenderRect> __scissorRect(
            theContext, &NVRenderContext::GetScissorRect, &NVRenderContext::SetScissorRect);
        qt3ds::render::NVRenderContextScopedProperty<NVRenderRect> __viewportRect(
            theContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport);
        qt3ds::render::NVRenderContextScopedProperty<bool> __depthWrite(
            theContext, &NVRenderContext::IsDepthWriteEnabled,
            &NVRenderContext::SetDepthWriteEnabled, false);
        qt3ds::render::NVRenderContextScopedProperty<qt3ds::render::NVRenderBoolOp::Enum> __depthFunction(
            theContext, &NVRenderContext::GetDepthFunction, &NVRenderContext::SetDepthFunction,
            qt3ds::render::NVRenderBoolOp::Less);
        qt3ds::render::NVRenderContextScopedProperty<bool> __blendEnabled(
            theContext, &NVRenderContext::IsBlendingEnabled, &NVRenderContext::SetBlendingEnabled,
            false);
        qt3ds::render::NVRenderContextScopedProperty<qt3ds::render::NVRenderBlendFunctionArgument>
            __blendFunction(theContext, &NVRenderContext::GetBlendFunction,
                            &NVRenderContext::SetBlendFunction,
                            qt3ds::render::NVRenderBlendFunctionArgument());
        qt3ds::render::NVRenderContextScopedProperty<qt3ds::render::NVRenderBlendEquationArgument>
            __blendEquation(theContext, &NVRenderContext::GetBlendEquation,
                            &NVRenderContext::SetBlendEquation,
                            qt3ds::render::NVRenderBlendEquationArgument());
        qt3ds::render::NVRenderContextScopedProperty<qt3ds::render::NVRenderFrameBufferPtr>
            __rendertarget(theContext, &NVRenderContext::GetRenderTarget,
                           &NVRenderContext::SetRenderTarget);

        QT3DSU32 theSampleCount = 1;
        bool isMultisamplePass = false;
        if (theDesiredEnvironment.m_MSAAMode != AAModeValues::NoAA) {
            switch (theDesiredEnvironment.m_MSAAMode) {
            case AAModeValues::SSAA:
                theSampleCount = 1;
                isMultisamplePass = true;
                break;
            case AAModeValues::X2:
                theSampleCount = 2;
                isMultisamplePass = true;
                break;
            case AAModeValues::X4:
                theSampleCount = 4;
                isMultisamplePass = true;
                break;
            case AAModeValues::X8:
                theSampleCount = 8;
                isMultisamplePass = true;
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            };

            // adjust render size for SSAA
            if (theDesiredEnvironment.m_MSAAMode == AAModeValues::SSAA) {
                CRendererUtil::GetSSAARenderSize(
                    theOriginalDesiredEnvironment.m_Width, theOriginalDesiredEnvironment.m_Height,
                    theDesiredEnvironment.m_Width, theDesiredEnvironment.m_Height);
            }
        }
        CResourceFrameBuffer theFrameBuffer(*m_ResourceManager);
        theFrameBuffer.EnsureFrameBuffer();
        NVRenderTexture2D *renderTargetTexture(theData.m_Texture);
        qt3ds::render::NVRenderTextureTargetType::Enum fboAttachmentType =
            qt3ds::render::NVRenderTextureTargetType::Texture2D;
        if (isMultisamplePass) {
            renderTargetTexture = NULL;
            if (theSampleCount > 1)
                fboAttachmentType = qt3ds::render::NVRenderTextureTargetType::Texture2D_MS;
        }

        CResourceTexture2D renderColorTexture(*m_ResourceManager, renderTargetTexture);

        CResourceTexture2D renderDepthStencilTexture(*m_ResourceManager);

        if (theSampleCount > 1)
            m_Context.GetRenderContext().SetMultisampleEnabled(true);

        qt3ds::render::NVRenderClearFlags theClearFlags;
        NVRenderTextureFormats::Enum theDepthStencilTextureFormat(NVRenderTextureFormats::Unknown);
        NVRenderFrameBufferAttachments::Enum theAttachmentLocation(
            NVRenderFrameBufferAttachments::Unknown);
        if (theDesiredEnvironment.m_Stencil) {
            theDepthStencilTextureFormat = NVRenderTextureFormats::Depth24Stencil8;
            theAttachmentLocation = NVRenderFrameBufferAttachments::DepthStencil;
        } else if (theDesiredEnvironment.m_Depth != OffscreenRendererDepthValues::NoDepthBuffer) {
            theAttachmentLocation = NVRenderFrameBufferAttachments::Depth;
            switch (theDesiredEnvironment.m_Depth) {
            case OffscreenRendererDepthValues::Depth16:
                theDepthStencilTextureFormat = NVRenderTextureFormats::Depth16;
                break;
            case OffscreenRendererDepthValues::Depth24:
                theDepthStencilTextureFormat = NVRenderTextureFormats::Depth24;
                break;
            case OffscreenRendererDepthValues::Depth32:
                theDepthStencilTextureFormat = NVRenderTextureFormats::Depth32;
                break;
            default:
                theAttachmentLocation = NVRenderFrameBufferAttachments::Unknown;
                theDepthStencilTextureFormat = NVRenderTextureFormats::Unknown;
                break;
            }
        }
        renderColorTexture.EnsureTexture(theDesiredEnvironment.m_Width,
                                         theDesiredEnvironment.m_Height,
                                         theDesiredEnvironment.m_Format, theSampleCount);
        theFrameBuffer->Attach(NVRenderFrameBufferAttachments::Color0, *renderColorTexture,
                               fboAttachmentType);

        if (theDepthStencilTextureFormat != NVRenderTextureFormats::Unknown) {
            renderDepthStencilTexture.EnsureTexture(theDesiredEnvironment.m_Width,
                                                    theDesiredEnvironment.m_Height,
                                                    theDepthStencilTextureFormat, theSampleCount);
            theFrameBuffer->Attach(theAttachmentLocation, *renderDepthStencilTexture,
                                   fboAttachmentType);
        }
        // IsComplete check takes a really long time so I am not going to worry about it for now.

        theContext.SetRenderTarget(theFrameBuffer);
        theContext.SetViewport(
            NVRenderRect(0, 0, theDesiredEnvironment.m_Width, theDesiredEnvironment.m_Height));
        theContext.SetScissorTestEnabled(false);

        theContext.SetBlendingEnabled(false);
        theData.m_Renderer->Render(theDesiredEnvironment, theContext, thePresScaleFactor,
                                   SScene::AlwaysClear, this);

        if (theSampleCount > 1) {
            CResourceTexture2D theResult(*m_ResourceManager, theData.m_Texture);

            if (theDesiredEnvironment.m_MSAAMode != AAModeValues::SSAA) {
                // Have to downsample the FBO.
                CRendererUtil::ResolveMutisampleFBOColorOnly(
                    *m_ResourceManager, theResult, m_Context.GetRenderContext(),
                    theDesiredEnvironment.m_Width, theDesiredEnvironment.m_Height,
                    theDesiredEnvironment.m_Format, *theFrameBuffer);

                m_Context.GetRenderContext().SetMultisampleEnabled(false);
            } else {
                // Resolve the FBO to the layer texture
                CRendererUtil::ResolveSSAAFBOColorOnly(
                    *m_ResourceManager, theResult, theOriginalDesiredEnvironment.m_Width,
                    theOriginalDesiredEnvironment.m_Height, m_Context.GetRenderContext(),
                    theDesiredEnvironment.m_Width, theDesiredEnvironment.m_Height,
                    theDesiredEnvironment.m_Format, *theFrameBuffer);
            }

            QT3DS_ASSERT(theData.m_Texture == theResult.GetTexture());
            theResult.ForgetTexture();
        } else {
            renderColorTexture.ForgetTexture();
        }
        theFrameBuffer->Attach(NVRenderFrameBufferAttachments::Color0,
                               NVRenderTextureOrRenderBuffer(), fboAttachmentType);
        if (theAttachmentLocation != NVRenderFrameBufferAttachments::Unknown)
            theFrameBuffer->Attach(theAttachmentLocation, NVRenderTextureOrRenderBuffer(),
                                   fboAttachmentType);
    }

    SOffscreenRenderResult GetRenderedItem(const SOffscreenRendererKey &inKey) override
    {
        TRendererMap::iterator theRenderer = m_Renderers.find(inKey);
        QT3DSVec2 thePresScaleFactor = m_Context.GetPresentationScaleFactor();
        if (theRenderer != m_Renderers.end() && theRenderer->second.mPtr->m_Rendering == false) {
            SRendererData &theData = *theRenderer->second.mPtr;
            SScopedRenderDataRenderMarker __renderMarker(theData);

            bool renderedThisFrame = theData.m_Texture && theData.m_FrameCount == m_FrameCount;
            theData.m_FrameCount = m_FrameCount;
            // Two different quick-out pathways.
            if (renderedThisFrame)
                return theData;

            SOffscreenRendererEnvironment theDesiredEnvironment =
                theData.m_Renderer->GetDesiredEnvironment(thePresScaleFactor);
            // Ensure we get a valid width and height
            theDesiredEnvironment.m_Width =
                ITextRenderer::NextMultipleOf4(theDesiredEnvironment.m_Width);
            theDesiredEnvironment.m_Height =
                ITextRenderer::NextMultipleOf4(theDesiredEnvironment.m_Height);
            if (theDesiredEnvironment.m_Width == 0 || theDesiredEnvironment.m_Height == 0) {
                return SOffscreenRenderResult();
            }

            NVRenderRect theViewport(0, 0, theDesiredEnvironment.m_Width,
                                     theDesiredEnvironment.m_Height);
            IRenderList &theRenderList(m_Context.GetRenderList());
            NVRenderContext &theContext(m_Context.GetRenderContext());
            // This happens here because if there are any fancy render steps
            SRenderListScopedProperty<bool> _scissor(theRenderList,
                                                     &IRenderList::IsScissorTestEnabled,
                                                     &IRenderList::SetScissorTestEnabled, false);
            SRenderListScopedProperty<NVRenderRect> _viewport(
                theRenderList, &IRenderList::GetViewport, &IRenderList::SetViewport, theViewport);
            // Some plugins don't use the render list so they need the actual gl context setup.
            qt3ds::render::NVRenderContextScopedProperty<bool> __scissorEnabled(
                theContext, &NVRenderContext::IsScissorTestEnabled,
                &NVRenderContext::SetScissorTestEnabled, false);
            qt3ds::render::NVRenderContextScopedProperty<NVRenderRect> __viewportRect(
                theContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport,
                theViewport);

            QT3DSU32 taskId = m_Context.GetRenderList().AddRenderTask(
                *QT3DS_NEW(m_Context.GetPerFrameAllocator(),
                        SOffscreenRunnable)(*this, theData, theDesiredEnvironment));

            SOffscreenRenderFlags theFlags =
                theData.m_Renderer->NeedsRender(theDesiredEnvironment, thePresScaleFactor, this);
            theData.m_HasTransparency = theFlags.m_HasTransparency;
            theData.m_HasChangedSinceLastFrame = theFlags.m_HasChangedSinceLastFrame;
            if (theData.m_Texture) {
                // Quick-out if the renderer doesn't need to render itself.
                if (theData.m_HasChangedSinceLastFrame == false) {
                    m_Context.GetRenderList().DiscardRenderTask(taskId);
                    return theData;
                }
            } else
                theData.m_HasChangedSinceLastFrame = true;

            // Release existing texture if it doesn't match latest environment request.
            if (theData.m_Texture) {
                STextureDetails theDetails = theData.m_Texture->GetTextureDetails();
                if (theDesiredEnvironment.m_Width != theDetails.m_Width
                    || theDesiredEnvironment.m_Height != theDetails.m_Height
                    || theDesiredEnvironment.m_Format != theDetails.m_Format) {
                    m_ResourceManager->Release(*theData.m_Texture);
                    theData.m_Texture = NULL;
                }
            }

            if (theData.m_Texture == NULL)
                theData.m_Texture = m_ResourceManager->AllocateTexture2D(
                    theDesiredEnvironment.m_Width, theDesiredEnvironment.m_Height,
                    theDesiredEnvironment.m_Format);

            // Add the node to the graph and get on with it.

            return theData;
        }
        return SOffscreenRenderResult();
    }

    void BeginFrame() override { m_PerFrameAllocator.reset(); }
    void EndFrame() override { ++m_FrameCount; }
};

void SOffscreenRunnable::Run()
{
    m_RenderManager.RenderItem(m_Data, m_DesiredEnvironment);
}
}

IOffscreenRenderManager &IOffscreenRenderManager::CreateOffscreenRenderManager(
    NVAllocatorCallback &inCallback, IStringTable &inStringTable, IResourceManager &inManager,
    IQt3DSRenderContext &inContext)
{
    return *QT3DS_NEW(inCallback, SOffscreenRenderManager)(inCallback, inStringTable, inManager,
                                                        inContext);
}
