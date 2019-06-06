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
#include "Qt3DSRenderResourceManager.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "render/Qt3DSRenderRenderBuffer.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "render/Qt3DSRenderTexture2DArray.h"
#include "render/Qt3DSRenderTextureCube.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSContainers.h"

using namespace qt3ds::render;

namespace {

struct SResourceManager : public IResourceManager
{
    NVScopedRefCounted<NVRenderContext> m_RenderContext;
    // Complete list of all allocated objects
    nvvector<NVScopedRefCounted<NVRefCounted>> m_AllocatedObjects;

    nvvector<NVRenderFrameBuffer *> m_FreeFrameBuffers;
    nvvector<NVRenderRenderBuffer *> m_FreeRenderBuffers;
    nvvector<NVRenderTexture2D *> m_FreeTextures;
    nvvector<NVRenderTexture2DArray *> m_FreeTexArrays;
    nvvector<NVRenderTextureCube *> m_FreeTexCubes;
    nvvector<NVRenderImage2D *> m_FreeImages;

    volatile QT3DSI32 mRefCount;

    SResourceManager(NVRenderContext &ctx)
        : m_RenderContext(ctx)
        , m_AllocatedObjects(ctx.GetAllocator(), "SResourceManager::m_FrameBuffers")
        , m_FreeFrameBuffers(ctx.GetAllocator(), "SResourceManager::m_FreeFrameBuffers")
        , m_FreeRenderBuffers(ctx.GetAllocator(), "SResourceManager::m_FreeRenderBuffers")
        , m_FreeTextures(ctx.GetAllocator(), "SResourceManager::m_FreeTextures")
        , m_FreeTexArrays(ctx.GetAllocator(), "SResourceManager::m_FreeTexArrays")
        , m_FreeTexCubes(ctx.GetAllocator(), "SResourceManager::m_FreeTexCubes")
        , m_FreeImages(ctx.GetAllocator(), "SResourceManager::m_FreeImages")
        , mRefCount(0)
    {
    }
    virtual ~SResourceManager() {}

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_RenderContext->GetAllocator())

    NVRenderFrameBuffer *AllocateFrameBuffer() override
    {
        if (m_FreeFrameBuffers.empty() == true) {
            NVRenderFrameBuffer *newBuffer = m_RenderContext->CreateFrameBuffer();
            m_AllocatedObjects.push_back(newBuffer);
            m_FreeFrameBuffers.push_back(newBuffer);
        }
        NVRenderFrameBuffer *retval = m_FreeFrameBuffers.back();
        m_FreeFrameBuffers.pop_back();
        return retval;
    }
    void Release(NVRenderFrameBuffer &inBuffer) override
    {
        if (inBuffer.HasAnyAttachment()) {
            // Ensure the framebuffer has no attachments.
            inBuffer.Attach(NVRenderFrameBufferAttachments::Color0,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Color1,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Color2,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Color3,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Color4,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Color5,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Color6,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Color7,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Depth,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            inBuffer.Attach(NVRenderFrameBufferAttachments::Stencil,
                            qt3ds::render::NVRenderTextureOrRenderBuffer());
            if (m_RenderContext->IsDepthStencilSupported())
                inBuffer.Attach(NVRenderFrameBufferAttachments::DepthStencil,
                                qt3ds::render::NVRenderTextureOrRenderBuffer());
        }
#ifdef _DEBUG
        nvvector<NVRenderFrameBuffer *>::iterator theFind =
            eastl::find(m_FreeFrameBuffers.begin(), m_FreeFrameBuffers.end(), &inBuffer);
        QT3DS_ASSERT(theFind == m_FreeFrameBuffers.end());
#endif
        m_FreeFrameBuffers.push_back(&inBuffer);
    }

    virtual NVRenderRenderBuffer *
    AllocateRenderBuffer(QT3DSU32 inWidth, QT3DSU32 inHeight,
                         NVRenderRenderBufferFormats::Enum inBufferFormat) override
    {
        // Look for one of this specific size and format.
        QT3DSU32 existingMatchIdx = m_FreeRenderBuffers.size();
        for (QT3DSU32 idx = 0, end = existingMatchIdx; idx < end; ++idx) {
            NVRenderRenderBuffer *theBuffer = m_FreeRenderBuffers[idx];
            qt3ds::render::NVRenderRenderBufferDimensions theDims = theBuffer->GetDimensions();
            NVRenderRenderBufferFormats::Enum theFormat = theBuffer->GetStorageFormat();
            if (theDims.m_Width == inWidth && theDims.m_Height == inHeight
                && theFormat == inBufferFormat) {
                // Replace idx with last for efficient erasure (that reorders the vector).
                m_FreeRenderBuffers.replace_with_last(idx);
                return theBuffer;
            } else if (theFormat == inBufferFormat)
                existingMatchIdx = idx;
        }
        // If a specific exact match couldn't be found, just use the buffer with
        // the same format and resize it.
        if (existingMatchIdx < m_FreeRenderBuffers.size()) {
            NVRenderRenderBuffer *theBuffer = m_FreeRenderBuffers[existingMatchIdx];
            m_FreeRenderBuffers.replace_with_last(existingMatchIdx);
            theBuffer->SetDimensions(qt3ds::render::NVRenderRenderBufferDimensions(inWidth, inHeight));
            return theBuffer;
        }

        NVRenderRenderBuffer *theBuffer =
            m_RenderContext->CreateRenderBuffer(inBufferFormat, inWidth, inHeight);
        m_AllocatedObjects.push_back(theBuffer);
        return theBuffer;
    }
    void Release(NVRenderRenderBuffer &inBuffer) override
    {
#ifdef _DEBUG
        nvvector<NVRenderRenderBuffer *>::iterator theFind =
            eastl::find(m_FreeRenderBuffers.begin(), m_FreeRenderBuffers.end(), &inBuffer);
        QT3DS_ASSERT(theFind == m_FreeRenderBuffers.end());
#endif
        m_FreeRenderBuffers.push_back(&inBuffer);
    }
    NVRenderTexture2D *SetupAllocatedTexture(NVRenderTexture2D &inTexture)
    {
        inTexture.SetMinFilter(NVRenderTextureMinifyingOp::Linear);
        inTexture.SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
        return &inTexture;
    }
    NVRenderTexture2D *AllocateTexture2D(QT3DSU32 inWidth, QT3DSU32 inHeight,
                                                 NVRenderTextureFormats::Enum inTextureFormat,
                                                 QT3DSU32 inSampleCount, bool immutable) override
    {
        bool inMultisample =
            inSampleCount > 1 && m_RenderContext->AreMultisampleTexturesSupported();
        for (QT3DSU32 idx = 0, end = m_FreeTextures.size(); idx < end; ++idx) {
            NVRenderTexture2D *theTexture = m_FreeTextures[idx];
            STextureDetails theDetails = theTexture->GetTextureDetails();
            if (theDetails.m_Width == inWidth && theDetails.m_Height == inHeight
                && inTextureFormat == theDetails.m_Format
                && theTexture->GetSampleCount() == inSampleCount) {
                m_FreeTextures.replace_with_last(idx);
                return SetupAllocatedTexture(*theTexture);
            }
        }
        // else resize an existing texture.  This is very expensive
        // note that MSAA textures are not resizable ( in GLES )
        /*
        if ( !m_FreeTextures.empty() && !inMultisample )
        {
                NVRenderTexture2D* theTexture = m_FreeTextures.back();
                m_FreeTextures.pop_back();

                // note we could re-use a former MSAA texture
                // this causes a entiere destroy of the previous texture object
                theTexture->SetTextureData( NVDataRef<QT3DSU8>(), 0, inWidth, inHeight, inTextureFormat
        );

                return SetupAllocatedTexture( *theTexture );
        }*/
        // else create a new texture.
        NVRenderTexture2D *theTexture = m_RenderContext->CreateTexture2D();

        if (inMultisample)
            theTexture->SetTextureDataMultisample(inSampleCount, inWidth, inHeight,
                                                  inTextureFormat);
        else if (immutable)
            theTexture->SetTextureStorage(1, inWidth, inHeight, inTextureFormat);
        else
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, inWidth, inHeight, inTextureFormat);

        m_AllocatedObjects.push_back(theTexture);
        return SetupAllocatedTexture(*theTexture);
    }
    void Release(NVRenderTexture2D &inBuffer) override
    {
#ifdef _DEBUG
        nvvector<NVRenderTexture2D *>::iterator theFind =
            eastl::find(m_FreeTextures.begin(), m_FreeTextures.end(), &inBuffer);
        QT3DS_ASSERT(theFind == m_FreeTextures.end());
#endif
        m_FreeTextures.push_back(&inBuffer);
    }

    NVRenderTexture2DArray *AllocateTexture2DArray(QT3DSU32 inWidth, QT3DSU32 inHeight, QT3DSU32 inSlices,
                                                   NVRenderTextureFormats::Enum inTextureFormat,
                                                   QT3DSU32 inSampleCount) override
    {
        bool inMultisample =
            inSampleCount > 1 && m_RenderContext->AreMultisampleTexturesSupported();
        for (QT3DSU32 idx = 0, end = m_FreeTexArrays.size(); idx < end; ++idx) {
            NVRenderTexture2DArray *theTexture = m_FreeTexArrays[idx];
            STextureDetails theDetails = theTexture->GetTextureDetails();
            if (theDetails.m_Width == inWidth && theDetails.m_Height == inHeight
                && theDetails.m_Depth == inSlices && inTextureFormat == theDetails.m_Format
                && theTexture->GetSampleCount() == inSampleCount) {
                m_FreeTexArrays.replace_with_last(idx);
                theTexture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
                theTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
                return theTexture;
            }
        }

        // else resize an existing texture.  This should be fairly quick at the driver level.
        // note that MSAA textures are not resizable ( in GLES )
        if (!m_FreeTexArrays.empty() && !inMultisample) {
            NVRenderTexture2DArray *theTexture = m_FreeTexArrays.back();
            m_FreeTexArrays.pop_back();

            // note we could re-use a former MSAA texture
            // this causes a entiere destroy of the previous texture object
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, inWidth, inHeight, inSlices,
                                       inTextureFormat);
            theTexture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
            theTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
            return theTexture;
        }

        // else create a new texture.
        NVRenderTexture2DArray *theTexture = NULL;

        if (!inMultisample) {
            theTexture = m_RenderContext->CreateTexture2DArray();
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, inWidth, inHeight, inSlices,
                                       inTextureFormat);
        } else {
            // Not supported yet
            return NULL;
        }

        m_AllocatedObjects.push_back(theTexture);
        theTexture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
        theTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
        return theTexture;
    }

    void Release(NVRenderTexture2DArray &inBuffer) override
    {
#ifdef _DEBUG
        nvvector<NVRenderTexture2DArray *>::iterator theFind =
            eastl::find(m_FreeTexArrays.begin(), m_FreeTexArrays.end(), &inBuffer);
        QT3DS_ASSERT(theFind == m_FreeTexArrays.end());
#endif
        m_FreeTexArrays.push_back(&inBuffer);
    }

    NVRenderTextureCube *AllocateTextureCube(QT3DSU32 inWidth, QT3DSU32 inHeight,
                                             NVRenderTextureFormats::Enum inTextureFormat,
                                             QT3DSU32 inSampleCount) override
    {
        bool inMultisample =
            inSampleCount > 1 && m_RenderContext->AreMultisampleTexturesSupported();
        for (QT3DSU32 idx = 0, end = m_FreeTexCubes.size(); idx < end; ++idx) {
            NVRenderTextureCube *theTexture = m_FreeTexCubes[idx];
            STextureDetails theDetails = theTexture->GetTextureDetails();
            if (theDetails.m_Width == inWidth && theDetails.m_Height == inHeight
                && inTextureFormat == theDetails.m_Format
                && theTexture->GetSampleCount() == inSampleCount) {
                m_FreeTexCubes.replace_with_last(idx);

                theTexture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
                theTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
                return theTexture;
            }
        }

        // else resize an existing texture.  This should be fairly quick at the driver level.
        // note that MSAA textures are not resizable ( in GLES )
        if (!m_FreeTexCubes.empty() && !inMultisample) {
            NVRenderTextureCube *theTexture = m_FreeTexCubes.back();
            m_FreeTexCubes.pop_back();

            // note we could re-use a former MSAA texture
            // this causes a entire destroy of the previous texture object
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubePosX,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubeNegX,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubePosY,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubeNegY,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubePosZ,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubeNegZ,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
            theTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
            return theTexture;
        }

        // else create a new texture.
        NVRenderTextureCube *theTexture = NULL;

        if (!inMultisample) {
            theTexture = m_RenderContext->CreateTextureCube();
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubePosX,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubeNegX,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubePosY,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubeNegY,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubePosZ,
                                       inWidth, inHeight, inTextureFormat);
            theTexture->SetTextureData(NVDataRef<QT3DSU8>(), 0, NVRenderTextureCubeFaces::CubeNegZ,
                                       inWidth, inHeight, inTextureFormat);
        } else {
            // Not supported yet
            return NULL;
        }

        m_AllocatedObjects.push_back(theTexture);
        theTexture->SetMinFilter(NVRenderTextureMinifyingOp::Linear);
        theTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
        return theTexture;
    }

    void Release(NVRenderTextureCube &inBuffer) override
    {
#ifdef _DEBUG
        nvvector<NVRenderTextureCube *>::iterator theFind =
            eastl::find(m_FreeTexCubes.begin(), m_FreeTexCubes.end(), &inBuffer);
        QT3DS_ASSERT(theFind == m_FreeTexCubes.end());
#endif
        m_FreeTexCubes.push_back(&inBuffer);
    }

    NVRenderImage2D *AllocateImage2D(NVRenderTexture2D *inTexture,
                                     NVRenderImageAccessType::Enum inAccess) override
    {
        if (m_FreeImages.empty() == true) {
            NVRenderImage2D *newImage = m_RenderContext->CreateImage2D(inTexture, inAccess);
            if (newImage) {
                m_AllocatedObjects.push_back(newImage);
                m_FreeImages.push_back(newImage);
            }
        }

        NVRenderImage2D *retval = m_FreeImages.back();
        m_FreeImages.pop_back();

        return retval;
    }

    void Release(NVRenderImage2D &inBuffer) override
    {
#ifdef _DEBUG
        nvvector<NVRenderImage2D *>::iterator theFind =
            eastl::find(m_FreeImages.begin(), m_FreeImages.end(), &inBuffer);
        QT3DS_ASSERT(theFind == m_FreeImages.end());
#endif
        m_FreeImages.push_back(&inBuffer);
    }

    NVRenderContext &GetRenderContext() override { return *m_RenderContext; }

    void RemoveObjectAllocation(NVRefCounted *obj) {
        for (QT3DSU32 idx = 0, end = m_AllocatedObjects.size(); idx < end; ++idx) {
            if (obj == m_AllocatedObjects[idx]) {
                m_AllocatedObjects.replace_with_last(idx);
                break;
            }
        }
    }

    void DestroyFreeSizedResources()
    {
        for (int idx = m_FreeRenderBuffers.size() - 1; idx >= 0; --idx) {
            NVRenderRenderBuffer *obj = m_FreeRenderBuffers[idx];
            m_FreeRenderBuffers.replace_with_last(idx);
            RemoveObjectAllocation(obj);
        }
        for (int idx = m_FreeTextures.size() - 1; idx >= 0; --idx) {
            NVRenderTexture2D *obj = m_FreeTextures[idx];
            m_FreeTextures.replace_with_last(idx);
            RemoveObjectAllocation(obj);
        }
        for (int idx = m_FreeTexArrays.size() - 1; idx >= 0; --idx) {
            NVRenderTexture2DArray *obj = m_FreeTexArrays[idx];
            m_FreeTexArrays.replace_with_last(idx);
            RemoveObjectAllocation(obj);
        }
        for (int idx = m_FreeTexCubes.size() - 1; idx >= 0; --idx) {
            NVRenderTextureCube *obj = m_FreeTexCubes[idx];
            m_FreeTexCubes.replace_with_last(idx);
            RemoveObjectAllocation(obj);
        }
    }
};
}

IResourceManager &IResourceManager::CreateResourceManager(NVRenderContext &inContext)
{
    return *QT3DS_NEW(inContext.GetAllocator(), SResourceManager)(inContext);
}
