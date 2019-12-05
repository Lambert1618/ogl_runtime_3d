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
#include "Qt3DSRenderImageBatchLoader.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/StringTable.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSRenderBufferManager.h"
#include "Qt3DSRenderThreadPool.h"
#include "Qt3DSRenderImageScaler.h"
#include "Qt3DSRenderLoadedTexture.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSPerfTimer.h"

using namespace qt3ds::render;

namespace {

struct SImageLoaderBatch;
typedef Mutex::ScopedLock TScopedLock;

struct SLoadingImage
{
    SImageLoaderBatch *m_Batch;
    CRegisteredString m_SourcePath;
    QT3DSU64 m_TaskId;
    SLoadingImage *m_Tail;

    // Called from main thread
    SLoadingImage(CRegisteredString inSourcePath)
        : m_Batch(NULL)
        , m_SourcePath(inSourcePath)
        , m_TaskId(0)
        , m_Tail(NULL)
    {
    }
    SLoadingImage()
        : m_Batch(NULL)
        , m_TaskId(0)
        , m_Tail(NULL)
    {
    }
    // Called from main thread
    void Setup(SImageLoaderBatch &inBatch);

    // Called from loader thread
    static void LoadImage(void *inImg);

    // Potentially called from loader thread
    static void TaskCancelled(void *inImg);
};

struct SLoadingImageTailOp
{
    SLoadingImage *get(SLoadingImage &inImg) { return inImg.m_Tail; }
    void set(SLoadingImage &inImg, SLoadingImage *inItem) { inImg.m_Tail = inItem; }
};

typedef InvasiveSingleLinkedList<SLoadingImage, SLoadingImageTailOp> TLoadingImageList;

struct SBatchLoader;

struct SImageLoaderBatch
{
    // All variables setup in main thread and constant from then on except
    // loaded image count.
    SBatchLoader &m_Loader;
    NVScopedRefCounted<IImageLoadListener> m_LoadListener;
    Sync m_LoadEvent;
    Mutex m_LoadMutex;
    TLoadingImageList m_Images;

    TImageBatchId m_BatchId;
    // Incremented in main thread
    QT3DSU32 m_LoadedOrCanceledImageCount;
    QT3DSU32 m_FinalizedImageCount;
    QT3DSU32 m_NumImages;
    NVRenderContextType m_contextType;
    bool m_preferKTX;
    bool m_ibl;

    // Called from main thread
    static SImageLoaderBatch *CreateLoaderBatch(SBatchLoader &inLoader, TImageBatchId inBatchId,
                                                NVConstDataRef<CRegisteredString> inSourcePaths,
                                                CRegisteredString inImageTillLoaded,
                                                IImageLoadListener *inListener,
                                                NVRenderContextType contextType,
                                                bool preferKTX, bool ibl);

    // Called from main thread
    SImageLoaderBatch(SBatchLoader &inLoader, IImageLoadListener *inLoadListener,
                      const TLoadingImageList &inImageList, TImageBatchId inBatchId,
                      QT3DSU32 inImageCount, NVRenderContextType contextType,
                      bool preferKTX, bool ibl);

    // Called from main thread
    ~SImageLoaderBatch();

    // Called from main thread
    bool IsLoadingFinished()
    {
        Mutex::ScopedLock __locker(m_LoadMutex);
        return m_LoadedOrCanceledImageCount >= m_NumImages;
    }

    bool IsFinalizedFinished()
    {
        Mutex::ScopedLock __locker(m_LoadMutex);
        return m_FinalizedImageCount >= m_NumImages;
    }

    void IncrementLoadedImageCount()
    {
        Mutex::ScopedLock __locker(m_LoadMutex);
        ++m_LoadedOrCanceledImageCount;
    }
    void IncrementFinalizedImageCount()
    {
        Mutex::ScopedLock __locker(m_LoadMutex);
        ++m_FinalizedImageCount;
    }
    // Called from main thread
    void Cancel();
    void Cancel(CRegisteredString inSourcePath);
};

struct SBatchLoadedImage
{
    CRegisteredString m_SourcePath;
    SLoadedTexture *m_Texture;
    SImageLoaderBatch *m_Batch;
    SBatchLoadedImage()
        : m_Texture(NULL)
        , m_Batch(NULL)
    {
    }

    // Called from loading thread
    SBatchLoadedImage(CRegisteredString inSourcePath, SLoadedTexture *inTexture,
                      SImageLoaderBatch &inBatch)
        : m_SourcePath(inSourcePath)
        , m_Texture(inTexture)
        , m_Batch(&inBatch)
    {
    }

    // Called from main thread
    bool Finalize(IBufferManager &inMgr);
};

struct SBatchLoader : public IImageBatchLoader
{
    typedef nvhash_map<TImageBatchId, SImageLoaderBatch *> TImageLoaderBatchMap;
    typedef nvhash_map<CRegisteredString, TImageBatchId> TSourcePathToBatchMap;
    typedef Pool<SLoadingImage, ForwardingAllocator> TLoadingImagePool;
    typedef Pool<SImageLoaderBatch, ForwardingAllocator> TBatchPool;

    // Accessed from loader thread
    NVFoundationBase &m_Foundation;
    volatile QT3DSI32 mRefCount;
    // Accessed from loader thread
    IInputStreamFactory &m_InputStreamFactory;
    //!!Not threadsafe!  accessed only from main thread
    IBufferManager &m_BufferManager;
    // Accessed from main thread
    IThreadPool &m_ThreadPool;
    // Accessed from both threads
    IPerfTimer &m_PerfTimer;
    // main thread
    TImageBatchId m_NextBatchId;
    // main thread
    TImageLoaderBatchMap m_Batches;
    // main thread
    Mutex m_LoaderMutex;

    // Both loader and main threads
    nvvector<SBatchLoadedImage> m_LoadedImages;
    // main thread
    nvvector<TImageBatchId> m_FinishedBatches;
    // main thread
    TSourcePathToBatchMap m_SourcePathToBatches;
    // main thread
    nvvector<SLoadingImage> m_LoaderBuilderWorkspace;
    TLoadingImagePool m_LoadingImagePool;
    TBatchPool m_BatchPool;

    SBatchLoader(NVFoundationBase &inFoundation, IInputStreamFactory &inFactory,
                 IBufferManager &inBufferManager, IThreadPool &inThreadPool, IPerfTimer &inTimer)
        : m_Foundation(inFoundation)
        , mRefCount(0)
        , m_InputStreamFactory(inFactory)
        , m_BufferManager(inBufferManager)
        , m_ThreadPool(inThreadPool)
        , m_PerfTimer(inTimer)
        , m_NextBatchId(1)
        , m_Batches(inFoundation.getAllocator(), "SBatchLoader::m_Batches")
        , m_LoaderMutex(inFoundation.getAllocator())
        , m_LoadedImages(inFoundation.getAllocator(), "SBatchLoader::m_LoadedImages")
        , m_FinishedBatches(inFoundation.getAllocator(), "SBatchLoader::m_FinishedBatches")
        , m_SourcePathToBatches(inFoundation.getAllocator(), "SBatchLoader::m_SourcePathToBatches")
        , m_LoaderBuilderWorkspace(inFoundation.getAllocator(),
                                   "SBatchLoader::m_LoaderBuilderWorkspace")
        , m_LoadingImagePool(
              ForwardingAllocator(inFoundation.getAllocator(), "SBatchLoader::m_LoadingImagePool"))
        , m_BatchPool(ForwardingAllocator(inFoundation.getAllocator(), "SBatchLoader::m_BatchPool"))
    {
    }

    virtual ~SBatchLoader()
    {
        nvvector<TImageBatchId> theCancelledBatches(m_Foundation.getAllocator(), "~SBatchLoader");
        for (TImageLoaderBatchMap::iterator theIter = m_Batches.begin(), theEnd = m_Batches.end();
             theIter != theEnd; ++theIter) {
            theIter->second->Cancel();
            theCancelledBatches.push_back(theIter->second->m_BatchId);
        }
        for (QT3DSU32 idx = 0, end = theCancelledBatches.size(); idx < end; ++idx)
            BlockUntilLoaded(theCancelledBatches[idx]);

        QT3DS_ASSERT(m_Batches.size() == 0);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    // Returns an ID to the load request.  Request a block of images to be loaded.
    // Also takes an image that the buffer system will return when requested for the given source
    // paths
    // until said path is loaded.
    // An optional listener can be passed in to get callbacks about the batch.
    TImageBatchId LoadImageBatch(NVConstDataRef<CRegisteredString> inSourcePaths,
                                 CRegisteredString inImageTillLoaded,
                                 IImageLoadListener *inListener,
                                 NVRenderContextType contextType,
                                 bool preferKTX, bool iblImages) override
    {
        if (inSourcePaths.size() == 0)
            return 0;

        TScopedLock __loaderLock(m_LoaderMutex);

        TImageBatchId theBatchId = 0;

        // Empty loop intentional to find an unused batch id.
        for (theBatchId = m_NextBatchId; m_Batches.find(theBatchId) != m_Batches.end();
             ++m_NextBatchId, theBatchId = m_NextBatchId) {
        }

        SImageLoaderBatch *theBatch(SImageLoaderBatch::CreateLoaderBatch(
            *this, theBatchId, inSourcePaths, inImageTillLoaded, inListener, contextType,
            preferKTX, iblImages));
        if (theBatch) {
            m_Batches.insert(eastl::make_pair(theBatchId, theBatch));
            return theBatchId;
        }
        return 0;
    }

    void CancelImageBatchLoading(TImageBatchId inBatchId) override
    {
        SImageLoaderBatch *theBatch(GetBatch(inBatchId));
        if (theBatch)
            theBatch->Cancel();
    }

    // Blocks if the image is currently in-flight
    void CancelImageLoading(CRegisteredString inSourcePath) override
    {
        TScopedLock __loaderLock(m_LoaderMutex);
        TSourcePathToBatchMap::iterator theIter = m_SourcePathToBatches.find(inSourcePath);
        if (theIter != m_SourcePathToBatches.end()) {
            TImageBatchId theBatchId = theIter->second;
            TImageLoaderBatchMap::iterator theBatchIter = m_Batches.find(theBatchId);
            if (theBatchIter != m_Batches.end())
                theBatchIter->second->Cancel(inSourcePath);
        }
    }

    SImageLoaderBatch *GetBatch(TImageBatchId inId)
    {
        TScopedLock __loaderLock(m_LoaderMutex);
        TImageLoaderBatchMap::iterator theIter = m_Batches.find(inId);
        if (theIter != m_Batches.end())
            return theIter->second;
        return NULL;
    }

    void BlockUntilLoaded(TImageBatchId inId) override
    {
        for (SImageLoaderBatch *theBatch = GetBatch(inId); theBatch; theBatch = GetBatch(inId)) {
            // Only need to block if images aren't loaded.  Don't need to block if they aren't
            // finalized.
            if (!theBatch->IsLoadingFinished()) {
                theBatch->m_LoadEvent.wait(200);
                theBatch->m_LoadEvent.reset();
            }
            BeginFrame(true);
        }
    }
    void ImageLoaded(SLoadingImage &inImage, SLoadedTexture *inTexture)
    {
        TScopedLock __loaderLock(m_LoaderMutex);
        m_LoadedImages.push_back(
            SBatchLoadedImage(inImage.m_SourcePath, inTexture, *inImage.m_Batch));
        inImage.m_Batch->IncrementLoadedImageCount();
        inImage.m_Batch->m_LoadEvent.set();
    }
    // These are called by the render context, users don't need to call this.
    void BeginFrame(bool firstFrame) override
    {
        TScopedLock __loaderLock(m_LoaderMutex);
        // Pass 1 - send out all image loaded signals
        for (QT3DSU32 idx = 0, end = m_LoadedImages.size(); idx < end; ++idx) {

            m_SourcePathToBatches.erase(m_LoadedImages[idx].m_SourcePath);
            m_LoadedImages[idx].Finalize(m_BufferManager);
            m_LoadedImages[idx].m_Batch->IncrementFinalizedImageCount();
            if (m_LoadedImages[idx].m_Batch->IsFinalizedFinished())
                m_FinishedBatches.push_back(m_LoadedImages[idx].m_Batch->m_BatchId);
            if (!firstFrame)
                break;
        }
        if (firstFrame)
            m_LoadedImages.clear();
        else if (m_LoadedImages.size())
            m_LoadedImages.erase(m_LoadedImages.begin());
        // pass 2 - clean up any existing batches.
        for (QT3DSU32 idx = 0, end = m_FinishedBatches.size(); idx < end; ++idx) {
            TImageLoaderBatchMap::iterator theIter = m_Batches.find(m_FinishedBatches[idx]);
            if (theIter != m_Batches.end()) {
                SImageLoaderBatch *theBatch = theIter->second;
                if (theBatch->m_LoadListener)
                    theBatch->m_LoadListener->OnImageBatchComplete(theBatch->m_BatchId);
                m_Batches.erase(m_FinishedBatches[idx]);
                theBatch->~SImageLoaderBatch();
                m_BatchPool.deallocate(theBatch);
            }
        }
        m_FinishedBatches.clear();
    }

    void EndFrame() override {}
};

void SLoadingImage::Setup(SImageLoaderBatch &inBatch)
{
    m_Batch = &inBatch;
    m_TaskId = inBatch.m_Loader.m_ThreadPool.AddTask(this, LoadImage, TaskCancelled);
}

void SLoadingImage::LoadImage(void *inImg)
{
    SLoadingImage *theThis = reinterpret_cast<SLoadingImage *>(inImg);
    QT3DS_PERF_SCOPED_TIMER(theThis->m_Batch->m_Loader.m_PerfTimer, "BatchLoader: Image Decompression")
    if (theThis->m_Batch->m_Loader.m_BufferManager.IsImageLoaded(theThis->m_SourcePath) == false) {
        SLoadedTexture *theTexture = SLoadedTexture::Load(
            theThis->m_SourcePath.c_str(), theThis->m_Batch->m_Loader.m_Foundation,
            theThis->m_Batch->m_Loader.m_InputStreamFactory, true,
            theThis->m_Batch->m_contextType,
            theThis->m_Batch->m_preferKTX,
            &theThis->m_Batch->m_Loader.m_BufferManager);
        // if ( theTexture )
        //	theTexture->EnsureMultiplerOfFour( theThis->m_Batch->m_Loader.m_Foundation,
        //theThis->m_SourcePath.c_str() );

        theThis->m_Batch->m_Loader.ImageLoaded(*theThis, theTexture);
    } else {
        theThis->m_Batch->m_Loader.ImageLoaded(*theThis, NULL);
    }
}

void SLoadingImage::TaskCancelled(void *inImg)
{
    SLoadingImage *theThis = reinterpret_cast<SLoadingImage *>(inImg);
    theThis->m_Batch->m_Loader.ImageLoaded(*theThis, NULL);
}

bool SBatchLoadedImage::Finalize(IBufferManager &inMgr)
{
    if (m_Texture) {
        eastl::string thepath(m_SourcePath);
        bool isIBL = this->m_Batch->m_ibl;
        inMgr.LoadRenderImage(m_SourcePath, *m_Texture, false, isIBL);
        inMgr.UnaliasImagePath(m_SourcePath);
    }
    if (m_Batch->m_LoadListener)
        m_Batch->m_LoadListener->OnImageLoadComplete(
            m_SourcePath, m_Texture ? ImageLoadResult::Succeeded : ImageLoadResult::Failed);

    if (m_Texture) {
        m_Texture->release();
        return true;
    }

    return false;
}

SImageLoaderBatch *
SImageLoaderBatch::CreateLoaderBatch(SBatchLoader &inLoader, TImageBatchId inBatchId,
                                     NVConstDataRef<CRegisteredString> inSourcePaths,
                                     CRegisteredString inImageTillLoaded,
                                     IImageLoadListener *inListener,
                                     NVRenderContextType contextType,
                                     bool preferKTX, bool iblImages)
{
    TLoadingImageList theImages;
    QT3DSU32 theLoadingImageCount = 0;
    for (QT3DSU32 idx = 0, end = inSourcePaths.size(); idx < end; ++idx) {
        CRegisteredString theSourcePath(inSourcePaths[idx]);

        if (theSourcePath.IsValid() == false)
            continue;

        if (inLoader.m_BufferManager.IsImageLoaded(theSourcePath))
            continue;

        eastl::pair<SBatchLoader::TSourcePathToBatchMap::iterator, bool> theInserter =
            inLoader.m_SourcePathToBatches.insert(eastl::make_pair(inSourcePaths[idx], inBatchId));

        // If the loader has already seen this image.
        if (theInserter.second == false)
            continue;

        if (inImageTillLoaded.IsValid()) {
            // Alias the image so any further requests for this source path will result in
            // the default images (image till loaded).
            bool aliasSuccess =
                inLoader.m_BufferManager.AliasImagePath(theSourcePath, inImageTillLoaded, true);
            (void)aliasSuccess;
            QT3DS_ASSERT(aliasSuccess);
        }

        theImages.push_front(
            *inLoader.m_LoadingImagePool.construct(theSourcePath, __FILE__, __LINE__));
        ++theLoadingImageCount;
    }
    if (theImages.empty() == false) {
        SImageLoaderBatch *theBatch =
            (SImageLoaderBatch *)inLoader.m_BatchPool.allocate(__FILE__, __LINE__);
        new (theBatch)
            SImageLoaderBatch(inLoader, inListener, theImages, inBatchId, theLoadingImageCount,
                              contextType, preferKTX, iblImages);
        return theBatch;
    }
    return NULL;
}

SImageLoaderBatch::SImageLoaderBatch(SBatchLoader &inLoader, IImageLoadListener *inLoadListener,
                                     const TLoadingImageList &inImageList, TImageBatchId inBatchId,
                                     QT3DSU32 inImageCount, NVRenderContextType contextType,
                                     bool preferKTX, bool ibl)
    : m_Loader(inLoader)
    , m_LoadListener(inLoadListener)
    , m_LoadEvent(inLoader.m_Foundation.getAllocator())
    , m_LoadMutex(inLoader.m_Foundation.getAllocator())
    , m_Images(inImageList)
    , m_BatchId(inBatchId)
    , m_LoadedOrCanceledImageCount(0)
    , m_FinalizedImageCount(0)
    , m_NumImages(inImageCount)
    , m_contextType(contextType)
    , m_preferKTX(preferKTX)
    , m_ibl(ibl)
{
    for (TLoadingImageList::iterator iter = m_Images.begin(), end = m_Images.end(); iter != end;
         ++iter) {
        iter->Setup(*this);
    }
}

SImageLoaderBatch::~SImageLoaderBatch()
{
    for (TLoadingImageList::iterator iter = m_Images.begin(), end = m_Images.end(); iter != end;
         ++iter) {
        TLoadingImageList::iterator temp(iter);
        ++iter;
        m_Loader.m_LoadingImagePool.deallocate(temp.m_Obj);
    }
}

void SImageLoaderBatch::Cancel()
{
    for (TLoadingImageList::iterator iter = m_Images.begin(), end = m_Images.end(); iter != end;
         ++iter)
        m_Loader.m_ThreadPool.CancelTask(iter->m_TaskId);
}

void SImageLoaderBatch::Cancel(CRegisteredString inSourcePath)
{
    for (TLoadingImageList::iterator iter = m_Images.begin(), end = m_Images.end(); iter != end;
         ++iter) {
        if (iter->m_SourcePath == inSourcePath) {
            m_Loader.m_ThreadPool.CancelTask(iter->m_TaskId);
            break;
        }
    }
}
}

IImageBatchLoader &IImageBatchLoader::CreateBatchLoader(NVFoundationBase &inFoundation,
                                                        IInputStreamFactory &inFactory,
                                                        IBufferManager &inBufferManager,
                                                        IThreadPool &inThreadPool,
                                                        IPerfTimer &inTimer)
{
    return *QT3DS_NEW(inFoundation.getAllocator(),
                   SBatchLoader)(inFoundation, inFactory, inBufferManager, inThreadPool, inTimer);
}
