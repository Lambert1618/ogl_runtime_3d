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
#include "Qt3DSRenderEffectSystem.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSContainers.h"
#include "StringTools.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSRenderEffect.h"
#include "Qt3DSRenderResourceManager.h"
#include "Qt3DSRenderDynamicObjectSystemCommands.h"
#include "render/Qt3DSRenderFrameBuffer.h"
#include "render/Qt3DSRenderShaderConstant.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderer.h"
#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "foundation/PreAllocatedAllocator.h"
#include "foundation/SerializationTypes.h"
#include "Qt3DSRenderShaderCache.h"
#include "foundation/FileTools.h"
#include "Qt3DSOffscreenRenderKey.h"
#include "Qt3DSRenderDynamicObjectSystemUtil.h"

using namespace qt3ds::render;
using namespace qt3ds::render::dynamic;
using qt3ds::render::NVRenderContextScopedProperty;
using qt3ds::render::NVRenderCachedShaderProperty;
using qt3ds::render::NVRenderCachedShaderBuffer;

// None of this code will work if the size of void* changes because that would mean that
// the alignment of some of the objects isn't 4 bytes but would be 8 bytes.

typedef eastl::pair<CRegisteredString, CRegisteredString> TStrStrPair;

namespace eastl {
template <>
struct hash<TStrStrPair>
{
    size_t operator()(const TStrStrPair &item) const
    {
        return hash<CRegisteredString>()(item.first) ^ hash<CRegisteredString>()(item.second);
    }
};
}

namespace {

/*
                ApplyBufferValue,
                //Apply the depth buffer as an input texture.
                ApplyDepthValue,
                Render, //Render to current FBO
                */

struct SEffectClass
{
    NVAllocatorCallback *m_Allocator;
    IDynamicObjectClass *m_DynamicClass;
    volatile QT3DSI32 mRefCount;

    SEffectClass(NVAllocatorCallback &inFnd, IDynamicObjectClass &dynClass)
        : m_Allocator(&inFnd)
        , m_DynamicClass(&dynClass)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(*m_Allocator)

    void SetupThisObjectFromMemory(NVAllocatorCallback &inAlloc, IDynamicObjectClass &inClass)
    {
        m_Allocator = &inAlloc;
        m_DynamicClass = &inClass;
        mRefCount = 0;
    }
};

struct SAllocatedBufferEntry
{
    CRegisteredString m_Name;
    NVScopedRefCounted<NVRenderFrameBuffer> m_FrameBuffer;
    NVScopedRefCounted<NVRenderTexture2D> m_Texture;
    SAllocateBufferFlags m_Flags;
    bool m_NeedsClear;

    SAllocatedBufferEntry(CRegisteredString inName, NVRenderFrameBuffer &inFb,
                          NVRenderTexture2D &inTexture, SAllocateBufferFlags inFlags)
        : m_Name(inName)
        , m_FrameBuffer(&inFb)
        , m_Texture(&inTexture)
        , m_Flags(inFlags)
        , m_NeedsClear(true)
    {
    }
    SAllocatedBufferEntry() {}
};

struct SAllocatedImageEntry
{
    CRegisteredString m_Name;
    NVScopedRefCounted<NVRenderImage2D> m_Image;
    NVScopedRefCounted<NVRenderTexture2D> m_Texture;
    SAllocateBufferFlags m_Flags;

    SAllocatedImageEntry(CRegisteredString inName, NVRenderImage2D &inImage,
                         NVRenderTexture2D &inTexture, SAllocateBufferFlags inFlags)
        : m_Name(inName)
        , m_Image(&inImage)
        , m_Texture(&inTexture)
        , m_Flags(inFlags)
    {
    }
    SAllocatedImageEntry() {}
};

struct SImageEntry
{
    NVScopedRefCounted<NVRenderShaderProgram> m_Shader;
    NVRenderCachedShaderProperty<NVRenderImage2D *> m_Image;
    volatile QT3DSI32 mRefCount;

    SImageEntry(NVRenderShaderProgram &inShader, const char *inImageName)
        : m_Shader(inShader)
        , m_Image(inImageName, inShader)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader->GetRenderContext().GetAllocator())

    void Set(NVRenderImage2D *inImage) { m_Image.Set(inImage); }

    static SImageEntry CreateImageEntry(NVRenderShaderProgram &inShader, const char *inStem)
    {
        return SImageEntry(inShader, inStem);
    }
};

struct SAllocatedDataBufferEntry
{
    CRegisteredString m_Name;
    NVScopedRefCounted<qt3ds::render::NVRenderDataBuffer> m_DataBuffer;
    NVRenderBufferBindValues::Enum m_BufferType;
    NVDataRef<QT3DSU8> m_BufferData;
    SAllocateBufferFlags m_Flags;
    bool m_NeedsClear;

    SAllocatedDataBufferEntry(CRegisteredString inName,
                              qt3ds::render::NVRenderDataBuffer &inDataBuffer,
                              NVRenderBufferBindValues::Enum inType, NVDataRef<QT3DSU8> data,
                              SAllocateBufferFlags inFlags)
        : m_Name(inName)
        , m_DataBuffer(&inDataBuffer)
        , m_BufferType(inType)
        , m_BufferData(data)
        , m_Flags(inFlags)
        , m_NeedsClear(false)
    {
    }
    SAllocatedDataBufferEntry() {}
};

struct SDataBufferEntry
{
    NVScopedRefCounted<NVRenderShaderProgram> m_Shader;
    NVRenderCachedShaderBuffer<qt3ds::render::NVRenderShaderBufferBase *> m_DataBuffer;
    volatile QT3DSI32 mRefCount;

    SDataBufferEntry(NVRenderShaderProgram &inShader, const char *inBufferName)
        : m_Shader(inShader)
        , m_DataBuffer(inBufferName, inShader)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader->GetRenderContext().GetAllocator())

    void Set(qt3ds::render::NVRenderDataBuffer *inBuffer)
    {
        if (inBuffer)
            inBuffer->Bind();

        m_DataBuffer.Set();
    }

    static SDataBufferEntry CreateDataBufferEntry(NVRenderShaderProgram &inShader,
                                                  const char *inStem)
    {
        return SDataBufferEntry(inShader, inStem);
    }
};

struct SEffectTextureData
{
    NVRenderTexture2D *m_Texture;
    bool m_NeedsAlphaMultiply;
    SEffectTextureData(NVRenderTexture2D *inTexture, bool inNeedsMultiply)
        : m_Texture(inTexture)
        , m_NeedsAlphaMultiply(inNeedsMultiply)
    {
    }
    SEffectTextureData()
        : m_Texture(NULL)
        , m_NeedsAlphaMultiply(false)
    {
    }
};

struct STextureEntry
{
    NVScopedRefCounted<NVRenderShaderProgram> m_Shader;
    NVRenderCachedShaderProperty<NVRenderTexture2D *> m_Texture;
    NVRenderCachedShaderProperty<QT3DSVec4> m_TextureData;
    NVRenderCachedShaderProperty<QT3DSI32> m_TextureFlags;
    volatile QT3DSI32 mRefCount;

    STextureEntry(NVRenderShaderProgram &inShader, const char *inTexName, const char *inDataName,
                  const char *inFlagName)
        : m_Shader(inShader)
        , m_Texture(inTexName, inShader)
        , m_TextureData(inDataName, inShader)
        , m_TextureFlags(inFlagName, inShader)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader->GetRenderContext().GetAllocator())

    void Set(NVRenderTexture2D *inTexture, bool inNeedsAlphaMultiply,
             const SPropertyDefinition *inDefinition)
    {
        QT3DSF32 theMixValue(inNeedsAlphaMultiply ? 0.0f : 1.0f);
        if (inTexture && inDefinition) {
            inTexture->SetMagFilter(inDefinition->m_MagFilterOp);
            inTexture->SetMinFilter(
                static_cast<NVRenderTextureMinifyingOp::Enum>(inDefinition->m_MagFilterOp));
            inTexture->SetTextureWrapS(inDefinition->m_CoordOp);
            inTexture->SetTextureWrapT(inDefinition->m_CoordOp);
        }
        m_Texture.Set(inTexture);
        if (inTexture) {
            STextureDetails theDetails(inTexture->GetTextureDetails());
            m_TextureData.Set(
                QT3DSVec4((QT3DSF32)theDetails.m_Width, (QT3DSF32)theDetails.m_Height, theMixValue, 0.0f));
            // I have no idea what these flags do.
            m_TextureFlags.Set(1);
        } else
            m_TextureFlags.Set(0);
    }

    static STextureEntry CreateTextureEntry(NVRenderShaderProgram &inShader, const char *inStem,
                                            Qt3DSString &inBuilder, Qt3DSString &inBuilder2)
    {
        inBuilder.assign(inStem);
        inBuilder.append("Info");
        inBuilder2.assign("flag");
        inBuilder2.append(inStem);
        return STextureEntry(inShader, inStem, inBuilder.c_str(), inBuilder2.c_str());
    }
};

typedef eastl::pair<CRegisteredString, NVScopedRefCounted<STextureEntry>> TNamedTextureEntry;
typedef eastl::pair<CRegisteredString, NVScopedRefCounted<SImageEntry>> TNamedImageEntry;
typedef eastl::pair<CRegisteredString, NVScopedRefCounted<SDataBufferEntry>> TNamedDataBufferEntry;
}

namespace qt3ds {
namespace render {

    struct SEffectContext
    {
        CRegisteredString m_ClassName;
        IQt3DSRenderContext &m_Context;
        IResourceManager *m_ResourceManager;
        nvvector<SAllocatedBufferEntry> m_AllocatedBuffers;
        nvvector<SAllocatedImageEntry> m_AllocatedImages;
        nvvector<SAllocatedDataBufferEntry> m_AllocatedDataBuffers;
        nvvector<TNamedTextureEntry> m_TextureEntries;
        nvvector<TNamedImageEntry> m_ImageEntries;
        nvvector<TNamedDataBufferEntry> m_DataBufferEntries;

        SEffectContext(CRegisteredString inName, IQt3DSRenderContext &ctx, IResourceManager *inManager)
            : m_ClassName(inName)
            , m_Context(ctx)
            , m_ResourceManager(inManager)
            , m_AllocatedBuffers(ctx.GetAllocator(), "SEffectContext::m_AllocatedBuffers")
            , m_AllocatedImages(ctx.GetAllocator(), "SEffectContext::m_AllocatedImages")
            , m_AllocatedDataBuffers(ctx.GetAllocator(), "SEffectContext::m_AllocatedDataBuffers")
            , m_TextureEntries(ctx.GetAllocator(), "SEffectContext::m_TextureEntries")
            , m_ImageEntries(ctx.GetAllocator(), "SEffectContext::m_ImageEntries")
            , m_DataBufferEntries(ctx.GetAllocator(), "SEffectContext::m_DataBufferEntries")
        {
        }

        ~SEffectContext()
        {
            while (m_AllocatedBuffers.size())
                ReleaseBuffer(0);

            while (m_AllocatedImages.size())
                ReleaseImage(0);

            while (m_AllocatedDataBuffers.size())
                ReleaseDataBuffer(0);
        }

        void ReleaseBuffer(QT3DSU32 inIdx)
        {
            SAllocatedBufferEntry &theEntry(m_AllocatedBuffers[inIdx]);
            theEntry.m_FrameBuffer->Attach(NVRenderFrameBufferAttachments::Color0,
                                           NVRenderTextureOrRenderBuffer());
            m_ResourceManager->Release(*theEntry.m_FrameBuffer);
            m_ResourceManager->Release(*theEntry.m_Texture);
            m_AllocatedBuffers.replace_with_last(inIdx);
        }

        void ReleaseImage(QT3DSU32 inIdx)
        {
            SAllocatedImageEntry &theEntry(m_AllocatedImages[inIdx]);
            m_ResourceManager->Release(*theEntry.m_Image);
            m_ResourceManager->Release(*theEntry.m_Texture);
            m_AllocatedImages.replace_with_last(inIdx);
        }

        void ReleaseDataBuffer(QT3DSU32 inIdx)
        {
            SAllocatedDataBufferEntry &theEntry(m_AllocatedDataBuffers[inIdx]);
            m_Context.GetAllocator().deallocate(theEntry.m_BufferData.begin());

            m_AllocatedDataBuffers.replace_with_last(inIdx);
        }

        QT3DSU32 FindBuffer(CRegisteredString inName)
        {
            for (QT3DSU32 idx = 0, end = m_AllocatedBuffers.size(); idx < end; ++idx)
                if (m_AllocatedBuffers[idx].m_Name == inName)
                    return idx;
            return m_AllocatedBuffers.size();
        }

        QT3DSU32 FindImage(CRegisteredString inName)
        {
            for (QT3DSU32 idx = 0, end = m_AllocatedImages.size(); idx < end; ++idx)
                if (m_AllocatedImages[idx].m_Name == inName)
                    return idx;

            return m_AllocatedImages.size();
        }

        QT3DSU32 FindDataBuffer(CRegisteredString inName)
        {
            for (QT3DSU32 idx = 0, end = m_AllocatedDataBuffers.size(); idx < end; ++idx) {
                if (m_AllocatedDataBuffers[idx].m_Name == inName)
                    return idx;
            }

            return m_AllocatedDataBuffers.size();
        }

        void SetTexture(NVRenderShaderProgram &inShader, CRegisteredString inPropName,
                        NVRenderTexture2D *inTexture, bool inNeedsMultiply,
                        Qt3DSString &inStringBuilder, Qt3DSString &inStringBuilder2,
                        const SPropertyDefinition *inPropDec = NULL)
        {
            STextureEntry *theTextureEntry(NULL);
            for (QT3DSU32 idx = 0, end = m_TextureEntries.size(); idx < end && theTextureEntry == NULL;
                 ++idx) {
                if (m_TextureEntries[idx].first == inPropName
                    && m_TextureEntries[idx].second->m_Shader.mPtr == &inShader)
                    theTextureEntry = m_TextureEntries[idx].second;
            }
            if (theTextureEntry == NULL) {
                NVScopedRefCounted<STextureEntry> theNewEntry = QT3DS_NEW(
                    m_Context.GetAllocator(), STextureEntry)(STextureEntry::CreateTextureEntry(
                    inShader, inPropName, inStringBuilder, inStringBuilder2));
                m_TextureEntries.push_back(eastl::make_pair(inPropName, theNewEntry));
                theTextureEntry = theNewEntry.mPtr;
            }
            theTextureEntry->Set(inTexture, inNeedsMultiply, inPropDec);
        }

        void SetImage(NVRenderShaderProgram &inShader, CRegisteredString inPropName,
                      NVRenderImage2D *inImage)
        {
            SImageEntry *theImageEntry(NULL);
            for (QT3DSU32 idx = 0, end = m_ImageEntries.size(); idx < end && theImageEntry == NULL;
                 ++idx) {
                if (m_ImageEntries[idx].first == inPropName
                    && m_ImageEntries[idx].second->m_Shader.mPtr == &inShader)
                    theImageEntry = m_ImageEntries[idx].second;
            }
            if (theImageEntry == NULL) {
                NVScopedRefCounted<SImageEntry> theNewEntry =
                    QT3DS_NEW(m_Context.GetAllocator(),
                           SImageEntry)(SImageEntry::CreateImageEntry(inShader, inPropName));
                m_ImageEntries.push_back(eastl::make_pair(inPropName, theNewEntry));
                theImageEntry = theNewEntry.mPtr;
            }

            theImageEntry->Set(inImage);
        }

        void SetDataBuffer(NVRenderShaderProgram &inShader, CRegisteredString inPropName,
                           qt3ds::render::NVRenderDataBuffer *inBuffer)
        {
            SDataBufferEntry *theDataBufferEntry(NULL);
            for (QT3DSU32 idx = 0, end = m_DataBufferEntries.size();
                 idx < end && theDataBufferEntry == NULL; ++idx) {
                if (m_DataBufferEntries[idx].first == inPropName
                    && m_DataBufferEntries[idx].second->m_Shader.mPtr == &inShader)
                    theDataBufferEntry = m_DataBufferEntries[idx].second;
            }
            if (theDataBufferEntry == NULL) {
                NVScopedRefCounted<SDataBufferEntry> theNewEntry =
                    QT3DS_NEW(m_Context.GetAllocator(), SDataBufferEntry)(
                        SDataBufferEntry::CreateDataBufferEntry(inShader, inPropName));
                m_DataBufferEntries.push_back(eastl::make_pair(inPropName, theNewEntry));
                theDataBufferEntry = theNewEntry.mPtr;
            }

            theDataBufferEntry->Set(inBuffer);
        }
    };
}
}

namespace {

using qt3ds::render::NVRenderCachedShaderProperty;
/* We setup some shared state on the effect shaders */
struct SEffectShader
{
    NVScopedRefCounted<NVRenderShaderProgram> m_Shader;
    NVRenderCachedShaderProperty<QT3DSMat44> m_MVP;
    NVRenderCachedShaderProperty<QT3DSVec2> m_FragColorAlphaSettings;
    NVRenderCachedShaderProperty<QT3DSVec2> m_DestSize;
    NVRenderCachedShaderProperty<QT3DSF32> m_AppFrame;
    NVRenderCachedShaderProperty<QT3DSF32> m_FPS;
    NVRenderCachedShaderProperty<QT3DSVec2> m_CameraClipRange;
    STextureEntry m_TextureEntry;
    volatile QT3DSI32 mRefCount;
    SEffectShader(NVRenderShaderProgram &inShader)
        : m_Shader(inShader)
        , m_MVP("ModelViewProjectionMatrix", inShader)
        , m_FragColorAlphaSettings("FragColorAlphaSettings", inShader)
        , m_DestSize("DestSize", inShader)
        , m_AppFrame("AppFrame", inShader)
        , m_FPS("FPS", inShader)
        , m_CameraClipRange("CameraClipRange", inShader)
        , m_TextureEntry(inShader, "Texture0", "Texture0Info", "Texture0Flags")
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Shader->GetRenderContext().GetAllocator())
};

struct SEffectSystem : public IEffectSystem
{
    typedef nvhash_map<CRegisteredString, char8_t *> TPathDataMap;
    typedef nvhash_set<CRegisteredString> TPathSet;
    typedef nvhash_map<CRegisteredString, NVScopedRefCounted<SEffectClass>> TEffectClassMap;
    typedef nvhash_map<TStrStrPair, NVScopedRefCounted<SEffectShader>> TShaderMap;
    typedef nvvector<SEffectContext *> TContextList;
    typedef eastl::pair<CRegisteredString, SImage *> TAllocatedImageEntry;

    IQt3DSRenderContextCore &m_CoreContext;
    IQt3DSRenderContext *m_Context;
    NVScopedRefCounted<IResourceManager> m_ResourceManager;
    mutable qt3ds::render::SPreAllocatedAllocator m_Allocator;
    // Keep from dual-including headers.
    TEffectClassMap m_EffectClasses;
    nvvector<CRegisteredString> m_EffectList;
    TContextList m_Contexts;
    Qt3DSString m_TextureStringBuilder;
    Qt3DSString m_TextureStringBuilder2;
    TShaderMap m_ShaderMap;
    NVScopedRefCounted<NVRenderDepthStencilState> m_DefaultStencilState;
    nvvector<NVScopedRefCounted<NVRenderDepthStencilState>> m_DepthStencilStates;
    nvvector<TAllocatedImageEntry> m_AllocatedImages;
    nvvector<SEffect::TImageMapHash *> m_effectImageMaps;

    volatile QT3DSI32 mRefCount;

    SEffectSystem(IQt3DSRenderContextCore &inContext)
        : m_CoreContext(inContext)
        , m_Context(NULL)
        , m_Allocator(inContext.GetAllocator())
        , m_EffectClasses(inContext.GetAllocator(), "SEffectSystem::m_EffectClasses")
        , m_EffectList(inContext.GetAllocator(), "SEffectSystem::m_EffectList")
        , m_Contexts(inContext.GetAllocator(), "SEffectSystem::m_Contexts")
        , m_ShaderMap(inContext.GetAllocator(), "SEffectSystem::m_ShaderMap")
        , m_DepthStencilStates(inContext.GetAllocator(), "SEffectSystem::m_DepthStencilStates")
        , m_AllocatedImages(inContext.GetAllocator(), "SEffectSystem::m_AllocatedImages")
        , m_effectImageMaps(inContext.GetAllocator(), "SEffectSystem::m_effectImageMaps")
        , mRefCount(0)
    {
    }

    ~SEffectSystem() override
    {
        clearCaches();
    }

    void clearCaches() override
    {
        for (QT3DSU32 idx = 0, end = m_Contexts.size(); idx < end; ++idx)
            NVDelete(m_Allocator, m_Contexts[idx]);
        m_Contexts.clear();

        for (QT3DSU32 idx = 0; idx < m_AllocatedImages.size(); ++idx) {
            SImage *pImage = m_AllocatedImages[idx].second;
            QT3DS_FREE(m_CoreContext.GetAllocator(), pImage);
        }
        for (QT3DSU32 idx = 0; idx < m_effectImageMaps.size(); ++idx) {
            auto *effmap = m_effectImageMaps[idx];
            effmap->~nvhash_map();
            QT3DS_FREE(m_CoreContext.GetAllocator(), effmap);
        }
        m_effectImageMaps.clear();
        m_AllocatedImages.clear();
    }

    SEffectContext &GetEffectContext(SEffect &inEffect)
    {
        if (inEffect.m_Context == NULL) {
            inEffect.m_Context =
                QT3DS_NEW(m_Allocator, SEffectContext)(inEffect.m_ClassName,
                                                       *m_Context, m_ResourceManager);
            m_Contexts.push_back(inEffect.m_Context);
        }
        return *inEffect.m_Context;
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_CoreContext.GetAllocator());

    SEffectClass *GetEffectClass(CRegisteredString inStr)
    {
        TEffectClassMap::iterator theIter = m_EffectClasses.find(inStr);
        if (theIter != m_EffectClasses.end())
            return theIter->second;
        return NULL;
    }
    const SEffectClass *GetEffectClass(CRegisteredString inStr) const
    {
        return const_cast<SEffectSystem *>(this)->GetEffectClass(inStr);
    }

    bool IsEffectRegistered(CRegisteredString inStr) override
    {
        return GetEffectClass(inStr) != NULL;
    }
    NVConstDataRef<CRegisteredString> GetRegisteredEffects() override
    {
        m_EffectList.clear();
        for (TEffectClassMap::iterator theIter = m_EffectClasses.begin(),
                                       theEnd = m_EffectClasses.end();
             theIter != theEnd; ++theIter)
            m_EffectList.push_back(theIter->first);
        return m_EffectList;
    }

    // Registers an effect that runs via a single GLSL file.
    bool RegisterGLSLEffect(CRegisteredString inName, const char8_t *inPathToEffect,
                                    NVConstDataRef<SPropertyDeclaration> inProperties) override
    {
        if (IsEffectRegistered(inName))
            return false;

        m_CoreContext.GetDynamicObjectSystemCore().Register(inName, inProperties, sizeof(SEffect),
                                                            GraphObjectTypes::Effect);
        IDynamicObjectClass &theClass =
            *m_CoreContext.GetDynamicObjectSystemCore().GetDynamicObjectClass(inName);

        SEffectClass *theEffect = QT3DS_NEW(m_Allocator, SEffectClass)(m_Allocator, theClass);
        m_EffectClasses.insert(eastl::make_pair(inName, theEffect));

        // Setup the commands required to run this effect
        StaticAssert<(sizeof(SBindShader) % 4 == 0)>::valid_expression();
        StaticAssert<(sizeof(SApplyInstanceValue) % 4 == 0)>::valid_expression();
        StaticAssert<(sizeof(SRender) % 4 == 0)>::valid_expression();

        QT3DSU32 commandAllocationSize = sizeof(SBindTarget);
        commandAllocationSize += sizeof(SBindShader);
        commandAllocationSize += sizeof(SApplyInstanceValue) * inProperties.size();
        commandAllocationSize += sizeof(SRender);

        QT3DSU32 commandCount = 3 + inProperties.size();
        QT3DSU32 commandPtrAllocationSize = commandCount * sizeof(SCommand *);
        QT3DSU32 allocationSize = Align8(commandAllocationSize) + commandPtrAllocationSize;
        QT3DSU8 *startBuffer =
            (QT3DSU8 *)m_Allocator.allocate(allocationSize, "dynamic::SCommand", __FILE__, __LINE__);
        QT3DSU8 *dataBuffer = startBuffer;
        // Setup the command buffer such that the ptrs to the commands and the commands themselves
        // are
        // all in the same block of memory.  This enables quicker iteration (trivially quicker, most
        // likely)
        // but it also enables simpler memory management (single free).
        // Furthermore, for a single glsl file the effect properties map precisely into the
        // glsl file itself.
        SCommand **theFirstCommandPtr =
            (reinterpret_cast<SCommand **>(dataBuffer + Align8(commandAllocationSize)));
        SCommand **theCommandPtr = theFirstCommandPtr;
        memZero(dataBuffer, commandAllocationSize);

        new (dataBuffer) SBindTarget();
        *theCommandPtr = (SCommand *)dataBuffer;
        ++theCommandPtr;
        dataBuffer += sizeof(SBindTarget);

        new (dataBuffer) SBindShader(m_CoreContext.GetStringTable().RegisterStr(inPathToEffect));
        *theCommandPtr = (SCommand *)dataBuffer;
        ++theCommandPtr;
        dataBuffer += sizeof(SBindShader);

        for (QT3DSU32 idx = 0, end = inProperties.size(); idx < end; ++idx) {
            const SPropertyDefinition &theDefinition(
                theEffect->m_DynamicClass->GetProperties()[idx]);
            new (dataBuffer) SApplyInstanceValue(theDefinition.m_Name, theDefinition.m_DataType,
                                                 theDefinition.m_Offset);
            *theCommandPtr = (SCommand *)dataBuffer;
            ++theCommandPtr;
            dataBuffer += sizeof(SApplyInstanceValue);
        }
        new (dataBuffer) SRender(false);
        *theCommandPtr = (SCommand *)dataBuffer;
        ++theCommandPtr;
        dataBuffer += sizeof(SRender);
        // Ensure we end up *exactly* where we expected to.
        QT3DS_ASSERT(dataBuffer == startBuffer + commandAllocationSize);
        QT3DS_ASSERT(theCommandPtr - theFirstCommandPtr == (int)inProperties.size() + 3);
        m_CoreContext.GetDynamicObjectSystemCore().SetRenderCommands(
            inName, NVConstDataRef<SCommand *>(theFirstCommandPtr, commandCount));
        m_Allocator.deallocate(startBuffer);
        return true;
    }

    void SetEffectPropertyDefaultValue(CRegisteredString inName,
                                               CRegisteredString inPropName,
                                               NVConstDataRef<QT3DSU8> inDefaultData) override
    {
        m_CoreContext.GetDynamicObjectSystemCore().SetPropertyDefaultValue(inName, inPropName,
                                                                           inDefaultData);
    }

    void SetEffectPropertyEnumNames(CRegisteredString inName, CRegisteredString inPropName,
                                            NVConstDataRef<CRegisteredString> inNames) override
    {
        m_CoreContext.GetDynamicObjectSystemCore().SetPropertyEnumNames(inName, inPropName,
                                                                        inNames);
    }

    bool RegisterEffect(CRegisteredString inName,
                                NVConstDataRef<SPropertyDeclaration> inProperties) override
    {
        if (IsEffectRegistered(inName))
            return false;
        m_CoreContext.GetDynamicObjectSystemCore().Register(inName, inProperties, sizeof(SEffect),
                                                            GraphObjectTypes::Effect);
        IDynamicObjectClass &theClass =
            *m_CoreContext.GetDynamicObjectSystemCore().GetDynamicObjectClass(inName);
        SEffectClass *theEffect = QT3DS_NEW(m_Allocator, SEffectClass)(m_Allocator, theClass);
        m_EffectClasses.insert(eastl::make_pair(inName, theEffect));
        return true;
    }

    bool UnregisterEffect(CRegisteredString inName) override
    {
        if (!IsEffectRegistered(inName))
            return false;

        m_CoreContext.GetDynamicObjectSystemCore().Unregister(inName);

        TEffectClassMap::iterator iter = m_EffectClasses.find(inName);
        if (iter != m_EffectClasses.end())
            m_EffectClasses.erase(iter);

        for (QT3DSU32 idx = 0, end = m_Contexts.size(); idx < end; ++idx) {
            if (m_Contexts[idx]->m_ClassName == inName)
                ReleaseEffectContext(m_Contexts[idx]);
        }
        return true;
    }

    virtual NVConstDataRef<CRegisteredString>
    GetEffectPropertyEnumNames(CRegisteredString inName, CRegisteredString inPropName) const override
    {
        const SEffectClass *theClass = GetEffectClass(inName);
        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            NVConstDataRef<CRegisteredString>();
        }
        const SPropertyDefinition *theDefinitionPtr =
            theClass->m_DynamicClass->FindPropertyByName(inPropName);
        if (theDefinitionPtr)
            return theDefinitionPtr->m_EnumValueNames;
        return NVConstDataRef<CRegisteredString>();
    }

    virtual NVConstDataRef<SPropertyDefinition>
    GetEffectProperties(CRegisteredString inEffectName) const override
    {
        const SEffectClass *theClass = GetEffectClass(inEffectName);
        if (theClass)
            return theClass->m_DynamicClass->GetProperties();
        return NVConstDataRef<SPropertyDefinition>();
    }

    void SetEffectPropertyTextureSettings(CRegisteredString inName,
                                                  CRegisteredString inPropName,
                                                  CRegisteredString inPropPath,
                                                  NVRenderTextureTypeValue::Enum inTexType,
                                                  NVRenderTextureCoordOp::Enum inCoordOp,
                                                  NVRenderTextureMagnifyingOp::Enum inMagFilterOp,
                                                  NVRenderTextureMinifyingOp::Enum inMinFilterOp) override
    {
        m_CoreContext.GetDynamicObjectSystemCore().SetPropertyTextureSettings(
            inName, inPropName, inPropPath, inTexType, inCoordOp, inMagFilterOp, inMinFilterOp);
    }

    void SetEffectRequiresDepthTexture(CRegisteredString inEffectName, bool inValue) override
    {
        SEffectClass *theClass = GetEffectClass(inEffectName);
        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        theClass->m_DynamicClass->SetRequiresDepthTexture(inValue);
    }

    bool DoesEffectRequireDepthTexture(CRegisteredString inEffectName) const override
    {
        const SEffectClass *theClass = GetEffectClass(inEffectName);
        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return false;
        }
        return theClass->m_DynamicClass->RequiresDepthTexture();
    }

    void SetEffectRequiresCompilation(CRegisteredString inEffectName, bool inValue) override
    {
        SEffectClass *theClass = GetEffectClass(inEffectName);
        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return;
        }
        theClass->m_DynamicClass->SetRequiresCompilation(inValue);
    }

    bool DoesEffectRequireCompilation(CRegisteredString inEffectName) const override
    {
        const SEffectClass *theClass = GetEffectClass(inEffectName);
        if (theClass == NULL) {
            QT3DS_ASSERT(false);
            return false;
        }
        return theClass->m_DynamicClass->RequiresCompilation();
    }

    void SetEffectCommands(CRegisteredString inEffectName,
                                   NVConstDataRef<dynamic::SCommand *> inCommands) override
    {
        m_CoreContext.GetDynamicObjectSystemCore().SetRenderCommands(inEffectName, inCommands);
    }

    virtual NVConstDataRef<dynamic::SCommand *>
    GetEffectCommands(CRegisteredString inEffectName) const override
    {
        return m_CoreContext.GetDynamicObjectSystemCore().GetRenderCommands(inEffectName);
    }

    SEffect *CreateEffectInstance(CRegisteredString inEffectName,
                                          NVAllocatorCallback &inSceneGraphAllocator) override
    {
        SEffectClass *theClass = GetEffectClass(inEffectName);
        if (theClass == NULL)
            return NULL;
        StaticAssert<(sizeof(SEffect) % 4 == 0)>::valid_expression();

        SEffect *theEffect = (SEffect *)m_CoreContext.GetDynamicObjectSystemCore().CreateInstance(
            inEffectName, inSceneGraphAllocator);
        theEffect->Initialize();
        return theEffect;
    }

    void AllocateBuffer(SEffect &inEffect, const SAllocateBuffer &inCommand, QT3DSU32 inFinalWidth,
                        QT3DSU32 inFinalHeight, NVRenderTextureFormats::Enum inSourceTextureFormat)
    {
        // Check to see if it is already allocated and if it is, is it the correct size. If both of
        // these assumptions hold, then we are good.
        NVRenderTexture2D *theBufferTexture = NULL;
        QT3DSU32 theWidth =
            ITextRenderer::NextMultipleOf4((QT3DSU32)(inFinalWidth * inCommand.m_SizeMultiplier));
        QT3DSU32 theHeight =
            ITextRenderer::NextMultipleOf4((QT3DSU32)(inFinalHeight * inCommand.m_SizeMultiplier));
        NVRenderTextureFormats::Enum resultFormat = inCommand.m_Format;
        if (resultFormat == NVRenderTextureFormats::Unknown)
            resultFormat = inSourceTextureFormat;

        if (inEffect.m_Context) {
            SEffectContext &theContext(*inEffect.m_Context);
            // size intentionally requiried every loop;
            QT3DSU32 bufferIdx = theContext.FindBuffer(inCommand.m_Name);
            if (bufferIdx < theContext.m_AllocatedBuffers.size()) {
                SAllocatedBufferEntry &theEntry(theContext.m_AllocatedBuffers[bufferIdx]);
                STextureDetails theDetails = theEntry.m_Texture->GetTextureDetails();
                if (theDetails.m_Width == theWidth && theDetails.m_Height == theHeight
                    && theDetails.m_Format == resultFormat) {
                    theBufferTexture = theEntry.m_Texture;
                } else {
                    theContext.ReleaseBuffer(bufferIdx);
                }
            }
        }
        if (theBufferTexture == NULL) {
            SEffectContext &theContext(GetEffectContext(inEffect));
            NVRenderFrameBuffer *theFB(m_ResourceManager->AllocateFrameBuffer());
            NVRenderTexture2D *theTexture(
                m_ResourceManager->AllocateTexture2D(theWidth, theHeight, resultFormat));
            theTexture->SetMagFilter(inCommand.m_FilterOp);
            theTexture->SetMinFilter(
                static_cast<NVRenderTextureMinifyingOp::Enum>(inCommand.m_FilterOp));
            theTexture->SetTextureWrapS(inCommand.m_TexCoordOp);
            theTexture->SetTextureWrapT(inCommand.m_TexCoordOp);
            theFB->Attach(NVRenderFrameBufferAttachments::Color0, *theTexture);
            theContext.m_AllocatedBuffers.push_back(SAllocatedBufferEntry(
                inCommand.m_Name, *theFB, *theTexture, inCommand.m_BufferFlags));
            theBufferTexture = theTexture;
        }
    }

    void AllocateImage(SEffect &inEffect, const SAllocateImage &inCommand, QT3DSU32 inFinalWidth,
                       QT3DSU32 inFinalHeight)
    {
        NVRenderImage2D *theImage = NULL;
        QT3DSU32 theWidth =
            ITextRenderer::NextMultipleOf4((QT3DSU32)(inFinalWidth * inCommand.m_SizeMultiplier));
        QT3DSU32 theHeight =
            ITextRenderer::NextMultipleOf4((QT3DSU32)(inFinalHeight * inCommand.m_SizeMultiplier));

        QT3DS_ASSERT(inCommand.m_Format != NVRenderTextureFormats::Unknown);

        if (inEffect.m_Context) {
            SEffectContext &theContext(*inEffect.m_Context);
            // size intentionally requiried every loop;
            QT3DSU32 imageIdx = theContext.FindImage(inCommand.m_Name);
            if (imageIdx < theContext.m_AllocatedImages.size()) {
                SAllocatedImageEntry &theEntry(theContext.m_AllocatedImages[imageIdx]);
                STextureDetails theDetails = theEntry.m_Texture->GetTextureDetails();
                if (theDetails.m_Width == theWidth && theDetails.m_Height == theHeight
                    && theDetails.m_Format == inCommand.m_Format) {
                    theImage = theEntry.m_Image;
                } else {
                    theContext.ReleaseImage(imageIdx);
                }
            }
        }

        if (theImage == NULL) {
            SEffectContext &theContext(GetEffectContext(inEffect));
            // allocate an immutable texture
            NVRenderTexture2D *theTexture(m_ResourceManager->AllocateTexture2D(
                theWidth, theHeight, inCommand.m_Format, 1, true));
            theTexture->SetMagFilter(inCommand.m_FilterOp);
            theTexture->SetMinFilter(
                static_cast<NVRenderTextureMinifyingOp::Enum>(inCommand.m_FilterOp));
            theTexture->SetTextureWrapS(inCommand.m_TexCoordOp);
            theTexture->SetTextureWrapT(inCommand.m_TexCoordOp);
            NVRenderImage2D *theImage =
                (m_ResourceManager->AllocateImage2D(theTexture, inCommand.m_Access));
            theContext.m_AllocatedImages.push_back(SAllocatedImageEntry(
                inCommand.m_Name, *theImage, *theTexture, inCommand.m_BufferFlags));
        }
    }

    void AllocateDataBuffer(SEffect &inEffect, const SAllocateDataBuffer &inCommand)
    {
        QT3DSU32 theBufferSize = (QT3DSU32)inCommand.m_Size;
        QT3DS_ASSERT(theBufferSize);
        qt3ds::render::NVRenderDataBuffer *theDataBuffer = NULL;
        qt3ds::render::NVRenderDataBuffer *theDataWrapBuffer = NULL;

        if (inEffect.m_Context) {
            SEffectContext &theContext(*inEffect.m_Context);
            // size intentionally requiried every loop;
            QT3DSU32 bufferIdx = theContext.FindDataBuffer(inCommand.m_Name);
            if (bufferIdx < theContext.m_AllocatedDataBuffers.size()) {
                SAllocatedDataBufferEntry &theEntry(theContext.m_AllocatedDataBuffers[bufferIdx]);
                if (theEntry.m_BufferType == inCommand.m_DataBufferType
                    && theEntry.m_BufferData.size() == theBufferSize) {
                    theDataBuffer = theEntry.m_DataBuffer;
                } else {
                    // if type and size don't match something is wrong
                    QT3DS_ASSERT(false);
                }
            }
        }

        if (theDataBuffer == NULL) {
            SEffectContext &theContext(GetEffectContext(inEffect));
            NVRenderContext &theRenderContext(m_Context->GetRenderContext());
            QT3DSU8 *initialData = (QT3DSU8 *)theContext.m_Context.GetAllocator().allocate(
                theBufferSize, "SEffectContext::AllocateDataBuffer", __FILE__, __LINE__);
            NVDataRef<QT3DSU8> data((QT3DSU8 *)initialData, theBufferSize);
            memset(initialData, 0x0L, theBufferSize);
            if (inCommand.m_DataBufferType == NVRenderBufferBindValues::Storage) {
                theDataBuffer = theRenderContext.CreateStorageBuffer(
                    inCommand.m_Name, qt3ds::render::NVRenderBufferUsageType::Dynamic, theBufferSize,
                    data, NULL);
            } else if (inCommand.m_DataBufferType == NVRenderBufferBindValues::Draw_Indirect) {
                QT3DS_ASSERT(theBufferSize == sizeof(qt3ds::render::DrawArraysIndirectCommand));
                // init a draw call
                QT3DSU32 *pIndirectDrawCall = (QT3DSU32 *)initialData;
                // vertex count we draw points right now only
                // the rest we fill in by GPU
                pIndirectDrawCall[0] = 1;
                theDataBuffer = theRenderContext.CreateDrawIndirectBuffer(
                    qt3ds::render::NVRenderBufferUsageType::Dynamic, theBufferSize, data);
            } else
                QT3DS_ASSERT(false);

            theContext.m_AllocatedDataBuffers.push_back(SAllocatedDataBufferEntry(
                inCommand.m_Name, *theDataBuffer, inCommand.m_DataBufferType, data,
                inCommand.m_BufferFlags));

            // create wrapper buffer
            if (inCommand.m_DataBufferWrapType == NVRenderBufferBindValues::Storage
                && inCommand.m_WrapName && theDataBuffer) {
                theDataWrapBuffer = theRenderContext.CreateStorageBuffer(
                    inCommand.m_WrapName, qt3ds::render::NVRenderBufferUsageType::Dynamic,
                    theBufferSize, data, theDataBuffer);
                theContext.m_AllocatedDataBuffers.push_back(SAllocatedDataBufferEntry(
                    inCommand.m_WrapName, *theDataWrapBuffer, inCommand.m_DataBufferWrapType,
                    NVDataRef<QT3DSU8>(), inCommand.m_BufferFlags));
            }
        }
    }

    NVRenderTexture2D *FindTexture(SEffect &inEffect, CRegisteredString inName)
    {
        if (inEffect.m_Context) {
            SEffectContext &theContext(*inEffect.m_Context);
            QT3DSU32 bufferIdx = theContext.FindBuffer(inName);
            if (bufferIdx < theContext.m_AllocatedBuffers.size()) {
                return theContext.m_AllocatedBuffers[bufferIdx].m_Texture;
            }
        }
        QT3DS_ASSERT(false);
        return NULL;
    }

    NVRenderFrameBuffer *BindBuffer(SEffect &inEffect, const SBindBuffer &inCommand,
                                    QT3DSMat44 &outMVP, QT3DSVec2 &outDestSize)
    {
        NVRenderFrameBuffer *theBuffer = NULL;
        NVRenderTexture2D *theTexture = NULL;
        if (inEffect.m_Context) {
            SEffectContext &theContext(*inEffect.m_Context);
            QT3DSU32 bufferIdx = theContext.FindBuffer(inCommand.m_BufferName);
            if (bufferIdx < theContext.m_AllocatedBuffers.size()) {
                theBuffer = theContext.m_AllocatedBuffers[bufferIdx].m_FrameBuffer;
                theTexture = theContext.m_AllocatedBuffers[bufferIdx].m_Texture;
                theContext.m_AllocatedBuffers[bufferIdx].m_NeedsClear = false;
            }
        }
        if (theBuffer == NULL) {
            qCCritical(INVALID_OPERATION, "Effect %s: Failed to find buffer %s for bind",
                       inEffect.m_ClassName.c_str(), inCommand.m_BufferName.c_str());
            QString errorMsg = QObject::tr("Failed to compile \"%1\" effect.\nConsider"
                                           " removing it from the presentation.")
                    .arg(inEffect.m_ClassName.c_str());
            QT3DS_ALWAYS_ASSERT_MESSAGE(errorMsg.toUtf8());
            outMVP = QT3DSMat44::createIdentity();
            return NULL;
        }

        if (theTexture) {
            SCamera::SetupOrthographicCameraForOffscreenRender(*theTexture, outMVP);
            STextureDetails theDetails(theTexture->GetTextureDetails());
            m_Context->GetRenderContext().SetViewport(
                NVRenderRect(0, 0, (QT3DSU32)theDetails.m_Width, (QT3DSU32)theDetails.m_Height));
            outDestSize = QT3DSVec2((QT3DSF32)theDetails.m_Width, (QT3DSF32)theDetails.m_Height);
        }

        return theBuffer;
    }

    SEffectShader *BindShader(CRegisteredString &inEffectId, const SBindShader &inCommand,
                              QString &errors)
    {
        SEffectClass *theClass = GetEffectClass(inEffectId);
        if (!theClass) {
            QT3DS_ASSERT(false);
            return NULL;
        }

        bool forceCompilation = theClass->m_DynamicClass->RequiresCompilation();

        eastl::pair<const TStrStrPair, NVScopedRefCounted<SEffectShader>> theInserter(
            TStrStrPair(inCommand.m_ShaderPath, inCommand.m_ShaderDefine),
            NVScopedRefCounted<SEffectShader>());
        eastl::pair<TShaderMap::iterator, bool> theInsertResult(m_ShaderMap.insert(theInserter));

        if (theInsertResult.second || forceCompilation) {
            NVRenderShaderProgram *theProgram =
                m_Context->GetDynamicObjectSystem()
                    .GetShaderProgram(inCommand.m_ShaderPath, inCommand.m_ShaderDefine,
                                      TShaderFeatureSet(), SDynamicShaderProgramFlags(), errors,
                                      forceCompilation).first;
            if (theProgram)
                theInsertResult.first->second = QT3DS_NEW(m_Allocator, SEffectShader)(*theProgram);
        }
        if (theInsertResult.first->second) {
            NVRenderContext &theContext(m_Context->GetRenderContext());
            theContext.SetActiveShader(theInsertResult.first->second->m_Shader);
        }

        return theInsertResult.first->second;
    }

    void DoApplyInstanceValue(SEffect &inEffect, QT3DSU8 *inDataPtr, CRegisteredString inPropertyName,
                              NVRenderShaderDataTypes::Enum inPropertyType,
                              NVRenderShaderProgram &inShader,
                              const SPropertyDefinition &inDefinition)
    {
        qt3ds::render::NVRenderShaderConstantBase *theConstant =
            inShader.GetShaderConstant(inPropertyName);
        using namespace qt3ds::render;
        if (theConstant) {
            if (theConstant->GetShaderConstantType() == inPropertyType) {
                if (inPropertyType == NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                    StaticAssert<sizeof(CRegisteredString)
                                 == sizeof(NVRenderTexture2DPtr)>::valid_expression();
                    CRegisteredString *theStrPtr = reinterpret_cast<CRegisteredString *>(inDataPtr);
                    bool needsAlphaMultiply = true;
                    NVRenderTexture2D *theTexture = nullptr;
                    if (theStrPtr->IsValid() && inEffect.m_imageMaps) {
                        SImage *image = (*inEffect.m_imageMaps)[inPropertyName];
                        if (image) {
                            if (image->m_ImagePath != *theStrPtr) {
                                image->m_ImagePath = *theStrPtr;
                                image->m_Flags.SetDirty(true);
                            } else {
                                if (image->m_OffscreenRendererId.IsValid()) {
                                    IOffscreenRenderManager &theOffscreenRenderer(
                                                            m_Context->GetOffscreenRenderManager());
                                    SOffscreenRenderResult theResult
                                            = theOffscreenRenderer.GetRenderedItem(*theStrPtr);
                                    needsAlphaMultiply = false;
                                    theTexture = theResult.m_Texture;
                                } else {
                                    needsAlphaMultiply = true;
                                    theTexture = image->m_LoadedTextureData->m_Texture;
                                }
                            }
                        }
                    }
                    GetEffectContext(inEffect).SetTexture(
                        inShader, inPropertyName, theTexture, needsAlphaMultiply,
                        m_TextureStringBuilder, m_TextureStringBuilder2, &inDefinition);
                } else if (inPropertyType == NVRenderShaderDataTypes::NVRenderImage2DPtr) {
                    StaticAssert<sizeof(CRegisteredString)
                                 == sizeof(NVRenderTexture2DPtr)>::valid_expression();
                    NVRenderImage2D *theImage = NULL;
                    GetEffectContext(inEffect).SetImage(inShader, inPropertyName, theImage);
                } else if (inPropertyType == NVRenderShaderDataTypes::NVRenderDataBufferPtr) {
                    // we don't handle this here
                } else {
                    switch (inPropertyType) {
#define HANDLE_QT3DS_SHADER_DATA_TYPE(type)                                                           \
    case NVRenderShaderDataTypes::type:                                                            \
        inShader.SetPropertyValue(theConstant, *(reinterpret_cast<type *>(inDataPtr)));            \
        break;
                        ITERATE_QT3DS_SHADER_DATA_TYPES
#undef HANDLE_QT3DS_SHADER_DATA_TYPE
                    default:
                        QT3DS_ASSERT(false);
                        break;
                    }
                }
            } else {
                qCCritical(INVALID_OPERATION,
                    "Effect ApplyInstanceValue command datatype "
                    "and shader datatypes differ for property %s",
                    inPropertyName.c_str());
                QT3DS_ASSERT(false);
            }
        }
    }

    void ApplyInstanceValue(SEffect &inEffect, SEffectClass &inClass,
                            NVRenderShaderProgram &inShader, const SApplyInstanceValue &inCommand)
    {
        // sanity check
        if (inCommand.m_PropertyName.IsValid()) {
            bool canGetData =
                inCommand.m_ValueOffset + getSizeofShaderDataType(inCommand.m_ValueType)
                <= inEffect.m_DataSectionByteSize;
            if (canGetData == false) {
                QT3DS_ASSERT(false);
                return;
            }
            QT3DSU8 *dataPtr = inEffect.GetDataSectionBegin() + inCommand.m_ValueOffset;
            const SPropertyDefinition *theDefinition =
                inClass.m_DynamicClass->FindPropertyByName(inCommand.m_PropertyName);
            if (theDefinition)
                DoApplyInstanceValue(inEffect, dataPtr, inCommand.m_PropertyName,
                                     inCommand.m_ValueType, inShader, *theDefinition);
        } else {
            NVConstDataRef<SPropertyDefinition> theDefs = inClass.m_DynamicClass->GetProperties();
            for (QT3DSU32 idx = 0, end = theDefs.size(); idx < end; ++idx) {
                const SPropertyDefinition &theDefinition(theDefs[idx]);
                qt3ds::render::NVRenderShaderConstantBase *theConstant =
                    inShader.GetShaderConstant(theDefinition.m_Name);

                // This is fine, the property wasn't found and we continue, no problem.
                if (!theConstant)
                    continue;
                QT3DSU8 *dataPtr = inEffect.GetDataSectionBegin() + theDefinition.m_Offset;
                DoApplyInstanceValue(inEffect, dataPtr, theDefinition.m_Name,
                                     theDefinition.m_DataType, inShader, theDefinition);
            }
        }
    }

    void ApplyValue(SEffect &inEffect, SEffectClass &inClass, NVRenderShaderProgram &inShader,
                    const SApplyValue &inCommand)
    {
        if (inCommand.m_PropertyName.IsValid()) {
            QT3DSU8 *dataPtr = inCommand.m_Value.mData;
            const SPropertyDefinition *theDefinition =
                inClass.m_DynamicClass->FindPropertyByName(inCommand.m_PropertyName);
            if (theDefinition)
                DoApplyInstanceValue(inEffect, dataPtr, inCommand.m_PropertyName,
                                     inCommand.m_ValueType, inShader, *theDefinition);
        }
    }

    bool ApplyBlending(const SApplyBlending &inCommand)
    {
        NVRenderContext &theContext(m_Context->GetRenderContext());

        theContext.SetBlendingEnabled(true);

        qt3ds::render::NVRenderBlendFunctionArgument blendFunc =
            qt3ds::render::NVRenderBlendFunctionArgument(
                inCommand.m_SrcBlendFunc, inCommand.m_DstBlendFunc, inCommand.m_SrcBlendFunc,
                inCommand.m_DstBlendFunc);

        qt3ds::render::NVRenderBlendEquationArgument blendEqu(NVRenderBlendEquation::Add,
                                                           NVRenderBlendEquation::Add);

        theContext.SetBlendFunction(blendFunc);
        theContext.SetBlendEquation(blendEqu);

        return true;
    }

    // This has the potential to change the source texture for the current render pass
    SEffectTextureData ApplyBufferValue(SEffect &inEffect, NVRenderShaderProgram &inShader,
                                        const SApplyBufferValue &inCommand,
                                        NVRenderTexture2D &inSourceTexture,
                                        SEffectTextureData inCurrentSourceTexture)
    {
        SEffectTextureData theTextureToBind;
        if (inCommand.m_BufferName.IsValid()) {
            if (inEffect.m_Context) {
                SEffectContext &theContext(*inEffect.m_Context);
                QT3DSU32 bufferIdx = theContext.FindBuffer(inCommand.m_BufferName);
                if (bufferIdx < theContext.m_AllocatedBuffers.size()) {
                    SAllocatedBufferEntry &theEntry(theContext.m_AllocatedBuffers[bufferIdx]);
                    if (theEntry.m_NeedsClear) {
                        NVRenderContext &theRenderContext(m_Context->GetRenderContext());

                        theRenderContext.SetRenderTarget(theEntry.m_FrameBuffer);
                        // Note that depth/stencil buffers need an explicit clear in their bind
                        // commands in order to ensure
                        // we clear the least amount of information possible.

                        if (theEntry.m_Texture) {
                            NVRenderTextureFormats::Enum theTextureFormat =
                                theEntry.m_Texture->GetTextureDetails().m_Format;
                            if (theTextureFormat != NVRenderTextureFormats::Depth16
                                && theTextureFormat != NVRenderTextureFormats::Depth24
                                && theTextureFormat != NVRenderTextureFormats::Depth32
                                && theTextureFormat != NVRenderTextureFormats::Depth24Stencil8) {
                                NVRenderContextScopedProperty<QT3DSVec4> __clearColor(
                                    theRenderContext, &NVRenderContext::GetClearColor,
                                    &NVRenderContext::SetClearColor, QT3DSVec4(0.0f));
                                theRenderContext.Clear(qt3ds::render::NVRenderClearValues::Color);
                            }
                        }
                        theEntry.m_NeedsClear = false;
                    }
                    theTextureToBind = SEffectTextureData(theEntry.m_Texture, false);
                }
            }
            if (theTextureToBind.m_Texture == NULL) {
                QT3DS_ASSERT(false);
                qCCritical(INVALID_OPERATION, "Effect %s: Failed to find buffer %s for bind",
                    inEffect.m_ClassName.c_str(), inCommand.m_BufferName.c_str());
                QT3DS_ASSERT(false);
            }
        } else // no name means bind the source
            theTextureToBind = SEffectTextureData(&inSourceTexture, false);

        if (inCommand.m_ParamName.IsValid()) {
            qt3ds::render::NVRenderShaderConstantBase *theConstant =
                inShader.GetShaderConstant(inCommand.m_ParamName);

            if (theConstant) {
                if (theConstant->GetShaderConstantType()
                    != NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                    qCCritical(INVALID_OPERATION,
                        "Effect %s: Binding buffer to parameter %s that is not a texture",
                        inEffect.m_ClassName.c_str(), inCommand.m_ParamName.c_str());
                    QT3DS_ASSERT(false);
                } else {
                    GetEffectContext(inEffect).SetTexture(
                        inShader, inCommand.m_ParamName, theTextureToBind.m_Texture,
                        theTextureToBind.m_NeedsAlphaMultiply, m_TextureStringBuilder,
                        m_TextureStringBuilder2);
                }
            }
            return inCurrentSourceTexture;
        } else {
            return theTextureToBind;
        }
    }

    void ApplyDepthValue(SEffect &inEffect, NVRenderShaderProgram &inShader,
                         const SApplyDepthValue &inCommand, NVRenderTexture2D *inTexture)
    {
        qt3ds::render::NVRenderShaderConstantBase *theConstant =
            inShader.GetShaderConstant(inCommand.m_ParamName);

        if (theConstant) {
            if (theConstant->GetShaderConstantType()
                != NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                qCCritical(INVALID_OPERATION,
                    "Effect %s: Binding buffer to parameter %s that is not a texture",
                    inEffect.m_ClassName.c_str(), inCommand.m_ParamName.c_str());
                QT3DS_ASSERT(false);
            } else {
                GetEffectContext(inEffect).SetTexture(inShader, inCommand.m_ParamName, inTexture,
                                                      false, m_TextureStringBuilder,
                                                      m_TextureStringBuilder2);
            }
        }
    }

    void ApplyImageValue(SEffect &inEffect, NVRenderShaderProgram &inShader,
                         const SApplyImageValue &inCommand)
    {
        SAllocatedImageEntry theImageToBind;
        if (inCommand.m_ImageName.IsValid()) {
            if (inEffect.m_Context) {
                SEffectContext &theContext(*inEffect.m_Context);
                QT3DSU32 bufferIdx = theContext.FindImage(inCommand.m_ImageName);
                if (bufferIdx < theContext.m_AllocatedImages.size()) {
                    theImageToBind = SAllocatedImageEntry(theContext.m_AllocatedImages[bufferIdx]);
                }
            }
        }

        if (theImageToBind.m_Image == NULL) {
            qCCritical(INVALID_OPERATION, "Effect %s: Failed to find image %s for bind",
                inEffect.m_ClassName.c_str(), inCommand.m_ImageName.c_str());
            QT3DS_ASSERT(false);
        }

        if (inCommand.m_ParamName.IsValid()) {
            qt3ds::render::NVRenderShaderConstantBase *theConstant =
                inShader.GetShaderConstant(inCommand.m_ParamName);

            if (theConstant) {
                if (inCommand.m_NeedSync) {
                    NVRenderBufferBarrierFlags flags(
                        qt3ds::render::NVRenderBufferBarrierValues::TextureFetch
                        | qt3ds::render::NVRenderBufferBarrierValues::TextureUpdate);
                    inShader.GetRenderContext().SetMemoryBarrier(flags);
                }

                if (theConstant->GetShaderConstantType()
                        == NVRenderShaderDataTypes::NVRenderImage2DPtr
                    && !inCommand.m_BindAsTexture) {
                    GetEffectContext(inEffect).SetImage(inShader, inCommand.m_ParamName,
                                                        theImageToBind.m_Image);
                } else if (theConstant->GetShaderConstantType()
                               == NVRenderShaderDataTypes::NVRenderTexture2DPtr
                           && inCommand.m_BindAsTexture) {
                    GetEffectContext(inEffect).SetTexture(
                        inShader, inCommand.m_ParamName, theImageToBind.m_Texture, false,
                        m_TextureStringBuilder, m_TextureStringBuilder2);
                } else {
                    qCCritical(INVALID_OPERATION,
                        "Effect %s: Binding buffer to parameter %s that is not a texture",
                        inEffect.m_ClassName.c_str(), inCommand.m_ParamName.c_str());
                    QT3DS_ASSERT(false);
                }
            }
        }
    }

    void ApplyDataBufferValue(SEffect &inEffect, NVRenderShaderProgram &inShader,
                              const SApplyDataBufferValue &inCommand)
    {
        SAllocatedDataBufferEntry theBufferToBind;
        if (inCommand.m_ParamName.IsValid()) {
            if (inEffect.m_Context) {
                SEffectContext &theContext(*inEffect.m_Context);
                QT3DSU32 bufferIdx = theContext.FindDataBuffer(inCommand.m_ParamName);
                if (bufferIdx < theContext.m_AllocatedDataBuffers.size()) {
                    theBufferToBind =
                        SAllocatedDataBufferEntry(theContext.m_AllocatedDataBuffers[bufferIdx]);
                    if (theBufferToBind.m_NeedsClear) {
                        NVDataRef<QT3DSU8> pData = theBufferToBind.m_DataBuffer->MapBuffer();
                        memset(pData.begin(), 0x0L, theBufferToBind.m_BufferData.size());
                        theBufferToBind.m_DataBuffer->UnmapBuffer();
                        theBufferToBind.m_NeedsClear = false;
                    }
                }
            }

            if (theBufferToBind.m_DataBuffer == NULL) {
                qCCritical(INVALID_OPERATION, "Effect %s: Failed to find buffer %s for bind",
                    inEffect.m_ClassName.c_str(), inCommand.m_ParamName.c_str());
                QT3DS_ASSERT(false);
            }

            qt3ds::render::NVRenderShaderBufferBase *theConstant =
                inShader.GetShaderBuffer(inCommand.m_ParamName);

            if (theConstant) {
                GetEffectContext(inEffect).SetDataBuffer(inShader, inCommand.m_ParamName,
                                                         theBufferToBind.m_DataBuffer);
            } else if (theBufferToBind.m_BufferType
                       == qt3ds::render::NVRenderBufferBindValues::Draw_Indirect) {
                // since we filled part of this buffer on the GPU we need a sync before usage
                NVRenderBufferBarrierFlags flags(
                    qt3ds::render::NVRenderBufferBarrierValues::CommandBuffer);
                inShader.GetRenderContext().SetMemoryBarrier(flags);
            }
        }
    }

    void ApplyRenderStateValue(NVRenderFrameBuffer *inTarget,
                               NVRenderTexture2D *inDepthStencilTexture,
                               const SApplyRenderState &theCommand)
    {
        NVRenderContext &theContext(m_Context->GetRenderContext());
        QT3DSU32 inState = (QT3DSU32)theCommand.m_RenderState;
        bool inEnable = theCommand.m_Enabled;

        switch (inState) {
        case NVRenderState::StencilTest: {
            if (inEnable && inTarget) {
                inTarget->Attach(NVRenderFrameBufferAttachments::DepthStencil,
                                 *inDepthStencilTexture);
            } else if (inTarget) {
                inTarget->Attach(NVRenderFrameBufferAttachments::DepthStencil,
                                 NVRenderTextureOrRenderBuffer());
            }

            theContext.SetStencilTestEnabled(inEnable);
        } break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
    }

    static bool CompareDepthStencilState(NVRenderDepthStencilState &inState,
                                         SDepthStencil &inStencil)
    {
        qt3ds::render::NVRenderStencilFunctionArgument theFunction =
            inState.GetStencilFunc(qt3ds::render::NVRenderFaces::Front);
        qt3ds::render::NVRenderStencilOperationArgument theOperation =
            inState.GetStencilOp(qt3ds::render::NVRenderFaces::Front);

        return theFunction.m_Function == inStencil.m_StencilFunction
            && theFunction.m_Mask == inStencil.m_Mask
            && theFunction.m_ReferenceValue == inStencil.m_Reference
            && theOperation.m_StencilFail == inStencil.m_StencilFailOperation
            && theOperation.m_DepthFail == inStencil.m_DepthFailOperation
            && theOperation.m_DepthPass == inStencil.m_DepthPassOperation;
    }

    void RenderPass(SEffectShader &inShader, const QT3DSMat44 &inMVP,
                    SEffectTextureData inSourceTexture, NVRenderFrameBuffer *inFrameBuffer,
                    QT3DSVec2 &inDestSize, const QT3DSVec2 &inCameraClipRange,
                    NVRenderTexture2D *inDepthStencil, Option<SDepthStencil> inDepthStencilCommand,
                    bool drawIndirect)
    {
        NVRenderContext &theContext(m_Context->GetRenderContext());
        theContext.SetRenderTarget(inFrameBuffer);
        if (inDepthStencil && inFrameBuffer) {
            inFrameBuffer->Attach(NVRenderFrameBufferAttachments::DepthStencil, *inDepthStencil);
            if (inDepthStencilCommand.hasValue()) {
                SDepthStencil &theDepthStencil(*inDepthStencilCommand);
                QT3DSU32 clearFlags = 0;
                if (theDepthStencil.m_Flags.HasClearStencil())
                    clearFlags |= NVRenderClearValues::Stencil;
                if (theDepthStencil.m_Flags.HasClearDepth())
                    clearFlags |= NVRenderClearValues::Depth;

                if (clearFlags)
                    theContext.Clear(qt3ds::render::NVRenderClearFlags(clearFlags));

                NVRenderDepthStencilState *targetState = NULL;
                for (QT3DSU32 idx = 0, end = m_DepthStencilStates.size();
                     idx < end && targetState == NULL; ++idx) {
                    NVRenderDepthStencilState &theState = *m_DepthStencilStates[idx];
                    if (CompareDepthStencilState(theState, theDepthStencil))
                        targetState = &theState;
                }
                if (targetState == NULL) {
                    qt3ds::render::NVRenderStencilFunctionArgument theFunctionArg(
                        theDepthStencil.m_StencilFunction, theDepthStencil.m_Reference,
                        theDepthStencil.m_Mask);
                    qt3ds::render::NVRenderStencilOperationArgument theOpArg(
                        theDepthStencil.m_StencilFailOperation,
                        theDepthStencil.m_DepthFailOperation, theDepthStencil.m_DepthPassOperation);
                    targetState = theContext.CreateDepthStencilState(
                        theContext.IsDepthTestEnabled(), theContext.IsDepthWriteEnabled(),
                        theContext.GetDepthFunction(), true, theFunctionArg, theFunctionArg,
                        theOpArg, theOpArg);
                    m_DepthStencilStates.push_back(targetState);
                }
                theContext.SetDepthStencilState(targetState);
            }
        }

        theContext.SetActiveShader(inShader.m_Shader);
        inShader.m_MVP.Set(inMVP);
        if (inSourceTexture.m_Texture) {
            inShader.m_TextureEntry.Set(inSourceTexture.m_Texture,
                                        inSourceTexture.m_NeedsAlphaMultiply, NULL);
        } else {
            qCCritical(INTERNAL_ERROR, "Failed to setup pass due to null source texture");
            QT3DS_ASSERT(false);
        }
        inShader.m_FragColorAlphaSettings.Set(QT3DSVec2(1.0f, 0.0f));
        inShader.m_DestSize.Set(inDestSize);
        if (inShader.m_AppFrame.IsValid())
            inShader.m_AppFrame.Set((QT3DSF32)m_Context->GetFrameCount());
        if (inShader.m_FPS.IsValid())
            inShader.m_FPS.Set((QT3DSF32)m_Context->GetFPS().first);
        if (inShader.m_CameraClipRange.IsValid())
            inShader.m_CameraClipRange.Set(inCameraClipRange);

        if (!drawIndirect)
            m_Context->GetRenderer().RenderQuad();
        else
            m_Context->GetRenderer().RenderPointsIndirect();

        if (inDepthStencil && inFrameBuffer) {
            inFrameBuffer->Attach(NVRenderFrameBufferAttachments::DepthStencil,
                                  NVRenderTextureOrRenderBuffer());
            theContext.SetDepthStencilState(m_DefaultStencilState);
        }
    }

    void DoRenderEffect(SEffect &inEffect, SEffectClass &inClass,
                        NVRenderTexture2D &inSourceTexture, QT3DSMat44 &inMVP,
                        NVRenderFrameBuffer *inTarget, bool inEnableBlendWhenRenderToTarget,
                        NVRenderTexture2D *inDepthTexture, NVRenderTexture2D *inDepthStencilTexture,
                        const QT3DSVec2 inCameraClipRange)
    {
        // Run through the effect commands and render the effect.
        // NVRenderTexture2D* theCurrentTexture(&inSourceTexture);
        NVRenderContext &theContext = m_Context->GetRenderContext();

        // Context variables that are updated during the course of a pass.
        SEffectTextureData theCurrentSourceTexture(&inSourceTexture, false);
        NVRenderTexture2D *theCurrentDepthStencilTexture = NULL;
        NVRenderFrameBuffer *theCurrentRenderTarget(inTarget);
        SEffectShader *theCurrentShader(NULL);
        NVRenderRect theOriginalViewport(theContext.GetViewport());
        bool wasScissorEnabled = theContext.IsScissorTestEnabled();
        bool wasBlendingEnabled = theContext.IsBlendingEnabled();
        // save current blending setup
        qt3ds::render::NVRenderBlendFunctionArgument theBlendFunc = theContext.GetBlendFunction();
        qt3ds::render::NVRenderBlendEquationArgument theBlendEqu = theContext.GetBlendEquation();
        bool intermediateBlendingEnabled = false;
        STextureDetails theDetails(inSourceTexture.GetTextureDetails());
        QT3DSU32 theFinalWidth = (QT3DSU32)(theDetails.m_Width);
        QT3DSU32 theFinalHeight = (QT3DSU32)(theDetails.m_Height);
        QT3DSVec2 theDestSize;
        {
            // Ensure no matter the command run goes we replace the rendering system to some
            // semblance of the approprate
            // setting.
            NVRenderContextScopedProperty<NVRenderFrameBuffer *> __framebuffer(
                theContext, &NVRenderContext::GetRenderTarget, &NVRenderContext::SetRenderTarget);
            NVRenderContextScopedProperty<NVRenderRect> __viewport(
                theContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport);
            NVRenderContextScopedProperty<bool> __scissorEnabled(
                theContext, &NVRenderContext::IsScissorTestEnabled,
                &NVRenderContext::SetScissorTestEnabled);
            NVRenderContextScopedProperty<bool> __stencilTest(
                theContext, &NVRenderContext::IsStencilTestEnabled,
                &NVRenderContext::SetStencilTestEnabled);
            NVRenderContextScopedProperty<qt3ds::render::NVRenderBoolOp::Enum> __depthFunction(
                theContext, &NVRenderContext::GetDepthFunction, &NVRenderContext::SetDepthFunction);
            Option<SDepthStencil> theCurrentDepthStencil;

            theContext.SetScissorTestEnabled(false);
            theContext.SetBlendingEnabled(false);
            theContext.SetCullingEnabled(false);
            theContext.SetDepthTestEnabled(false);
            theContext.SetDepthWriteEnabled(false);

            QT3DSMat44 theMVP(QT3DSMat44::createIdentity());
            QString errors;
            NVConstDataRef<dynamic::SCommand *> theCommands =
                inClass.m_DynamicClass->GetRenderCommands();
            for (QT3DSU32 commandIdx = 0, commandEnd = theCommands.size(); commandIdx < commandEnd;
                 ++commandIdx) {
                const SCommand &theCommand(*theCommands[commandIdx]);
                switch (theCommand.m_Type) {
                case CommandTypes::AllocateBuffer:
                    AllocateBuffer(inEffect, static_cast<const SAllocateBuffer &>(theCommand),
                                   theFinalWidth, theFinalHeight, theDetails.m_Format);
                    break;

                case CommandTypes::AllocateImage:
                    AllocateImage(inEffect, static_cast<const SAllocateImage &>(theCommand),
                                  theFinalWidth, theFinalHeight);
                    break;

                case CommandTypes::AllocateDataBuffer:
                    AllocateDataBuffer(inEffect,
                                       static_cast<const SAllocateDataBuffer &>(theCommand));
                    break;

                case CommandTypes::BindBuffer:
                    theCurrentRenderTarget =
                        BindBuffer(inEffect, static_cast<const SBindBuffer &>(theCommand), theMVP,
                                   theDestSize);
                    break;

                case CommandTypes::BindTarget: {
                    m_Context->GetRenderContext().SetRenderTarget(inTarget);
                    theCurrentRenderTarget = inTarget;
                    theMVP = inMVP;
                    theContext.SetViewport(theOriginalViewport);
                    theDestSize = QT3DSVec2((QT3DSF32)theFinalWidth, (QT3DSF32)theFinalHeight);
                    // This isn't necessary if we are rendering to an offscreen buffer and not
                    // compositing
                    // with other objects.
                    if (inEnableBlendWhenRenderToTarget) {
                        theContext.SetBlendingEnabled(wasBlendingEnabled);
                        theContext.SetScissorTestEnabled(wasScissorEnabled);
                        // The blending setup was done before we apply the effect
                        theContext.SetBlendFunction(theBlendFunc);
                        theContext.SetBlendEquation(theBlendEqu);
                    }
                } break;
                case CommandTypes::BindShader:
                    theCurrentShader = BindShader(inEffect.m_ClassName,
                                                  static_cast<const SBindShader &>(theCommand),
                                                  errors);
                    if (!errors.isEmpty())
                        inEffect.SetError(m_CoreContext.GetStringTable().RegisterStr(errors));
                case CommandTypes::ApplyInstanceValue:
                    if (theCurrentShader)
                        ApplyInstanceValue(inEffect, inClass, *theCurrentShader->m_Shader,
                                           static_cast<const SApplyInstanceValue &>(theCommand));
                    break;
                case CommandTypes::ApplyValue:
                    if (theCurrentShader)
                        ApplyValue(inEffect, inClass, *theCurrentShader->m_Shader,
                                   static_cast<const SApplyValue &>(theCommand));
                    break;
                case CommandTypes::ApplyBlending:
                    intermediateBlendingEnabled =
                        ApplyBlending(static_cast<const SApplyBlending &>(theCommand));
                    break;
                case CommandTypes::ApplyBufferValue:
                    if (theCurrentShader)
                        theCurrentSourceTexture =
                            ApplyBufferValue(inEffect, *theCurrentShader->m_Shader,
                                             static_cast<const SApplyBufferValue &>(theCommand),
                                             inSourceTexture, theCurrentSourceTexture);
                    break;
                case CommandTypes::ApplyDepthValue:
                    if (theCurrentShader)
                        ApplyDepthValue(inEffect, *theCurrentShader->m_Shader,
                                        static_cast<const SApplyDepthValue &>(theCommand),
                                        inDepthTexture);
                    if (!inDepthTexture) {
                        qCCritical(INVALID_OPERATION,
                            "Depth value command detected but no "
                            "depth buffer provided for effect %s",
                            inEffect.m_ClassName.c_str());
                        QT3DS_ASSERT(false);
                    }
                    break;
                case CommandTypes::ApplyImageValue:
                    if (theCurrentShader)
                        ApplyImageValue(inEffect, *theCurrentShader->m_Shader,
                                        static_cast<const SApplyImageValue &>(theCommand));
                    break;
                case CommandTypes::ApplyDataBufferValue:
                    if (theCurrentShader)
                        ApplyDataBufferValue(
                            inEffect, *theCurrentShader->m_Shader,
                            static_cast<const SApplyDataBufferValue &>(theCommand));
                    break;
                case CommandTypes::DepthStencil: {
                    const SDepthStencil &theDepthStencil =
                        static_cast<const SDepthStencil &>(theCommand);
                    theCurrentDepthStencilTexture =
                        FindTexture(inEffect, theDepthStencil.m_BufferName);
                    if (theCurrentDepthStencilTexture)
                        theCurrentDepthStencil = theDepthStencil;
                } break;
                case CommandTypes::Render:
                    if (theCurrentShader && theCurrentSourceTexture.m_Texture) {
                        RenderPass(*theCurrentShader, theMVP, theCurrentSourceTexture,
                                   theCurrentRenderTarget, theDestSize, inCameraClipRange,
                                   theCurrentDepthStencilTexture, theCurrentDepthStencil,
                                   static_cast<const SRender &>(theCommand).m_DrawIndirect);
                    }
                    // Reset the source texture regardless
                    theCurrentSourceTexture = SEffectTextureData(&inSourceTexture, false);
                    theCurrentDepthStencilTexture = NULL;
                    theCurrentDepthStencil = Option<SDepthStencil>();
                    // reset intermediate blending state
                    if (intermediateBlendingEnabled) {
                        theContext.SetBlendingEnabled(false);
                        intermediateBlendingEnabled = false;
                    }
                    break;
                case CommandTypes::ApplyRenderState:
                    ApplyRenderStateValue(theCurrentRenderTarget, inDepthStencilTexture,
                                          static_cast<const SApplyRenderState &>(theCommand));
                    break;
                default:
                    QT3DS_ASSERT(false);
                    break;
                }
            }

            SetEffectRequiresCompilation(inEffect.m_ClassName, false);

            // reset to default stencil state
            if (inDepthStencilTexture) {
                theContext.SetDepthStencilState(m_DefaultStencilState);
            }

            // Release any per-frame buffers
            if (inEffect.m_Context) {
                SEffectContext &theContext(*inEffect.m_Context);
                // Query for size on every loop intentional
                for (QT3DSU32 idx = 0; idx < theContext.m_AllocatedBuffers.size(); ++idx) {
                    if (theContext.m_AllocatedBuffers[idx].m_Flags.IsSceneLifetime() == false) {
                        theContext.ReleaseBuffer(idx);
                        --idx;
                    }
                }
                for (QT3DSU32 idx = 0; idx < theContext.m_AllocatedImages.size(); ++idx) {
                    if (theContext.m_AllocatedImages[idx].m_Flags.IsSceneLifetime() == false) {
                        theContext.ReleaseImage(idx);
                        --idx;
                    }
                }
            }
        }
    }

    NVRenderTexture2D *RenderEffect(SEffectRenderArgument inRenderArgument) override
    {
        SEffectClass *theClass = GetEffectClass(inRenderArgument.m_Effect.m_ClassName);
        if (!theClass) {
            QT3DS_ASSERT(false);
            return NULL;
        }
        QT3DSMat44 theMVP;
        SCamera::SetupOrthographicCameraForOffscreenRender(inRenderArgument.m_ColorBuffer, theMVP);
        // setup a render target
        NVRenderContext &theContext(m_Context->GetRenderContext());
        IResourceManager &theManager(m_Context->GetResourceManager());
        NVRenderContextScopedProperty<NVRenderFrameBuffer *> __framebuffer(
            theContext, &NVRenderContext::GetRenderTarget, &NVRenderContext::SetRenderTarget);
        STextureDetails theDetails(inRenderArgument.m_ColorBuffer.GetTextureDetails());
        QT3DSU32 theFinalWidth = ITextRenderer::NextMultipleOf4((QT3DSU32)(theDetails.m_Width));
        QT3DSU32 theFinalHeight = ITextRenderer::NextMultipleOf4((QT3DSU32)(theDetails.m_Height));
        NVRenderFrameBuffer *theBuffer = theManager.AllocateFrameBuffer();
        NVRenderTexture2D *theTargetTexture = inRenderArgument.m_targetTexture;
        if (theTargetTexture == nullptr) {
            // Some Effects may need to run before HDR tonemap. This means we need to keep the
            // input format
            NVRenderTextureFormats::Enum theOutputFormat = NVRenderTextureFormats::RGBA8;
            if (theClass->m_DynamicClass->GetOutputTextureFormat()
                    == NVRenderTextureFormats::Unknown) {
                theOutputFormat = theDetails.m_Format;
            }
            theTargetTexture = theManager.AllocateTexture2D(theFinalWidth, theFinalHeight,
                                                            theOutputFormat);
        }
        theBuffer->Attach(NVRenderFrameBufferAttachments::Color0, *theTargetTexture);
        theContext.SetRenderTarget(theBuffer);
        NVRenderContextScopedProperty<NVRenderRect> __viewport(
            theContext, &NVRenderContext::GetViewport, &NVRenderContext::SetViewport,
            NVRenderRect(0, 0, theFinalWidth, theFinalHeight));

        NVRenderContextScopedProperty<bool> __scissorEnable(
            theContext, &NVRenderContext::IsScissorTestEnabled,
            &NVRenderContext::SetScissorTestEnabled, false);

        DoRenderEffect(inRenderArgument.m_Effect, *theClass, inRenderArgument.m_ColorBuffer, theMVP,
                       m_Context->GetRenderContext().GetRenderTarget(), false,
                       inRenderArgument.m_DepthTexture, inRenderArgument.m_DepthStencilBuffer,
                       inRenderArgument.m_CameraClipRange);

        theBuffer->Attach(NVRenderFrameBufferAttachments::Color0, NVRenderTextureOrRenderBuffer());
        theManager.Release(*theBuffer);
        return theTargetTexture;
    }

    // Render the effect to the currently bound render target using this MVP
    bool RenderEffect(SEffectRenderArgument inRenderArgument, QT3DSMat44 &inMVP,
                      bool inEnableBlendWhenRenderToTarget) override
    {
        SEffectClass *theClass = GetEffectClass(inRenderArgument.m_Effect.m_ClassName);
        if (!theClass) {
            QT3DS_ASSERT(false);
            return false;
        }

        DoRenderEffect(inRenderArgument.m_Effect, *theClass, inRenderArgument.m_ColorBuffer, inMVP,
                       m_Context->GetRenderContext().GetRenderTarget(),
                       inEnableBlendWhenRenderToTarget, inRenderArgument.m_DepthTexture,
                       inRenderArgument.m_DepthStencilBuffer, inRenderArgument.m_CameraClipRange);
        return true;
    }

    void ReleaseEffectContext(SEffectContext *inContext) override
    {
        if (inContext == NULL)
            return;
        for (QT3DSU32 idx = 0, end = m_Contexts.size(); idx < end; ++idx) {
            if (m_Contexts[idx] == inContext) {
                m_Contexts.replace_with_last(idx);
                NVDelete(m_Allocator, inContext);
            }
        }
    }

    void ResetEffectFrameData(SEffectContext &inContext) override
    { // Query for size on every loop intentional
        for (QT3DSU32 idx = 0; idx < inContext.m_AllocatedBuffers.size(); ++idx) {
            SAllocatedBufferEntry &theBuffer(inContext.m_AllocatedBuffers[idx]);
            if (theBuffer.m_Flags.IsSceneLifetime() == true)
                theBuffer.m_NeedsClear = true;
        }
        for (QT3DSU32 idx = 0; idx < inContext.m_AllocatedDataBuffers.size(); ++idx) {
            SAllocatedDataBufferEntry &theDataBuffer(inContext.m_AllocatedDataBuffers[idx]);
            if (theDataBuffer.m_Flags.IsSceneLifetime() == true)
                theDataBuffer.m_NeedsClear = true;
        }
    }

    void SetShaderData(CRegisteredString path, const char8_t *data,
                               const char8_t *inShaderType, const char8_t *inShaderVersion,
                               bool inHasGeomShader, bool inIsComputeShader) override
    {
        m_CoreContext.GetDynamicObjectSystemCore().SetShaderData(
            path, data, inShaderType, inShaderVersion, inHasGeomShader, inIsComputeShader);
    }

    void Save(qt3ds::render::SWriteBuffer &ioBuffer,
                      const qt3ds::render::SStrRemapMap &inRemapMap, const char8_t *inProjectDir) const override
    {
        ioBuffer.write((QT3DSU32)m_EffectClasses.size());
        SStringSaveRemapper theRemapper(m_Allocator, inRemapMap, inProjectDir,
                                        m_CoreContext.GetStringTable());
        for (TEffectClassMap::const_iterator theIter = m_EffectClasses.begin(),
                                             end = m_EffectClasses.end();
             theIter != end; ++theIter) {
            const SEffectClass &theClass = *theIter->second;
            CRegisteredString theClassName = theClass.m_DynamicClass->GetId();
            theClassName.Remap(inRemapMap);
            ioBuffer.write(theClassName);
            // Effect classes do not store any additional data from the dynamic object class.
            ioBuffer.write(theClass);
        }
    }

    void Load(NVDataRef<QT3DSU8> inData, CStrTableOrDataRef inStrDataBlock,
                      const char8_t *inProjectDir) override
    {
        m_Allocator.m_PreAllocatedBlock = inData;
        m_Allocator.m_OwnsMemory = false;
        qt3ds::render::SDataReader theReader(inData.begin(), inData.end());
        QT3DSU32 numEffectClasses = theReader.LoadRef<QT3DSU32>();
        SStringLoadRemapper theRemapper(m_Allocator, inStrDataBlock, inProjectDir,
                                        m_CoreContext.GetStringTable());
        for (QT3DSU32 idx = 0, end = numEffectClasses; idx < end; ++idx) {
            CRegisteredString theClassName = theReader.LoadRef<CRegisteredString>();
            theClassName.Remap(inStrDataBlock);
            IDynamicObjectClass *theBaseClass =
                m_CoreContext.GetDynamicObjectSystemCore().GetDynamicObjectClass(theClassName);
            if (theBaseClass == NULL) {
                QT3DS_ASSERT(false);
                return;
            }
            SEffectClass *theClass = theReader.Load<SEffectClass>();
            theClass->SetupThisObjectFromMemory(m_Allocator, *theBaseClass);
            NVScopedRefCounted<SEffectClass> theClassPtr(theClass);
            m_EffectClasses.insert(eastl::make_pair(theBaseClass->GetId(), theClassPtr));
        }
    }

    IEffectSystem &GetEffectSystem(IQt3DSRenderContext &context) override
    {
        m_Context = &context;

        NVRenderContext &theContext(m_Context->GetRenderContext());

        m_ResourceManager = &IResourceManager::CreateResourceManager(theContext);

        // create default stencil state
        qt3ds::render::NVRenderStencilFunctionArgument stencilDefaultFunc(
            qt3ds::render::NVRenderBoolOp::AlwaysTrue, 0x0, 0xFF);
        qt3ds::render::NVRenderStencilOperationArgument stencilDefaultOp(
            qt3ds::render::NVRenderStencilOp::Keep, qt3ds::render::NVRenderStencilOp::Keep,
            qt3ds::render::NVRenderStencilOp::Keep);
        m_DefaultStencilState = theContext.CreateDepthStencilState(
            theContext.IsDepthTestEnabled(), theContext.IsDepthWriteEnabled(),
            theContext.GetDepthFunction(), theContext.IsStencilTestEnabled(), stencilDefaultFunc,
            stencilDefaultFunc, stencilDefaultOp, stencilDefaultOp);

        return *this;
    }

    IResourceManager &GetResourceManager() override
    {
        return *m_ResourceManager;
    }

    QT3DSU32 FindAllocatedImage(CRegisteredString inName)
    {
        for (QT3DSU32 idx = 0, end = m_AllocatedImages.size(); idx < end; ++idx) {
            if (m_AllocatedImages[idx].first == inName)
                return idx;
        }
        return QT3DSU32(-1);
    }

    SEffect::TImageMapHash *newImageMap(NVAllocatorCallback &allocator)
    {
        const char *name = "SEffect::TImageMapHash";
        SEffect::TImageMapHash *ret
                = new (QT3DS_ALLOC(allocator, sizeof(SEffect::TImageMapHash), name))
                        SEffect::TImageMapHash(allocator, name);
        m_effectImageMaps.push_back(ret);
        return ret;
    }

    void prepareEffectForRender(SEffect &inEffect) override
    {
        SEffectClass *theClass = GetEffectClass(inEffect.m_ClassName);
        if (!theClass)
            return;

        NVConstDataRef<SPropertyDefinition> theDefs = theClass->m_DynamicClass->GetProperties();
        for (QT3DSU32 idx = 0, end = theDefs.size(); idx < end; ++idx) {
            const SPropertyDefinition &theDefinition(theDefs[idx]);
            if (theDefinition.m_DataType == NVRenderShaderDataTypes::NVRenderTexture2DPtr) {
                SImage *pImage = nullptr;

                QT3DSU8 *dataPtr = inEffect.GetDataSectionBegin() + theDefinition.m_Offset;
                StaticAssert<sizeof(CRegisteredString)
                             == sizeof(NVRenderTexture2DPtr)>::valid_expression();
                CRegisteredString *theStrPtr = reinterpret_cast<CRegisteredString *>(dataPtr);
                if (theStrPtr->IsValid()) {
                    IOffscreenRenderManager &theOffscreenRenderer(
                        m_Context->GetOffscreenRenderManager());

                    if (theOffscreenRenderer.HasOffscreenRenderer(*theStrPtr)) {
                        theOffscreenRenderer.GetRenderedItem(*theStrPtr);
                    } else {
                        QT3DSU32 index = FindAllocatedImage(*theStrPtr);
                        if (index == QT3DSU32(-1)) {
                            pImage = QT3DS_NEW(m_CoreContext.GetAllocator(), SImage)();
                            m_AllocatedImages.push_back(
                                eastl::make_pair(*theStrPtr, pImage));
                        } else {
                            pImage = m_AllocatedImages[index].second;
                        }

                        if (!inEffect.m_imageMaps)
                            inEffect.m_imageMaps = newImageMap(m_CoreContext.GetAllocator());

                        if ((*inEffect.m_imageMaps)[theDefs[idx].m_Name] != pImage) {
                            (*inEffect.m_imageMaps)[theDefs[idx].m_Name] = pImage;
                            pImage->m_ImagePath = theDefs[idx].m_ImagePath;
                            pImage->m_ImageShaderName = theDefs[idx].m_Name;
                            pImage->m_VerticalTilingMode = theDefs[idx].m_CoordOp;
                            pImage->m_HorizontalTilingMode = theDefs[idx].m_CoordOp;
                        }
                    }
                } else {
                    if (!inEffect.m_imageMaps)
                        inEffect.m_imageMaps = newImageMap(m_CoreContext.GetAllocator());
                    (*inEffect.m_imageMaps)[theDefs[idx].m_Name] = nullptr;
                }
            }
        }
    }
};
}

IEffectSystemCore &IEffectSystemCore::CreateEffectSystemCore(IQt3DSRenderContextCore &inContext)
{
    return *QT3DS_NEW(inContext.GetAllocator(), SEffectSystem)(inContext);
}
