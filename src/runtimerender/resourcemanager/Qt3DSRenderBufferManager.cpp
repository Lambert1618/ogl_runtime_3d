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
#ifdef _WIN32
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#endif
#include "Qt3DSRender.h"
#include "Qt3DSRenderBufferManager.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSAllocator.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSAtomic.h"
#include "EASTL/hash_map.h"
#include "foundation/FileTools.h"
#include "Qt3DSImportMesh.h"
#include "Qt3DSRenderMesh.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "Qt3DSRenderLoadedTexture.h"
#include "foundation/Qt3DSFoundation.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSRenderImageScaler.h"
#include "Qt3DSRenderImage.h"
#include "Qt3DSTextRenderer.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "foundation/Qt3DSMutex.h"
#include "Qt3DSRenderPrefilterTexture.h"
#include <QtCore/qdir.h>

using namespace qt3ds::render;

namespace {

using eastl::hash;
using eastl::pair;
using eastl::make_pair;
typedef eastl::basic_string<char8_t, ForwardingAllocator> TStr;
struct StrHasher
{
    size_t operator()(const TStr &str) const
    {
        return hash<const char8_t *>()((const char8_t *)str.c_str());
    }
};

struct StrEq
{
    bool operator()(const TStr &lhs, const TStr &rhs) const { return lhs == rhs; }
};

struct SImageEntry : public SImageTextureData
{
    bool m_Loaded;
    SImageEntry()
        : SImageTextureData(), m_Loaded(false)
    {
    }
    SImageEntry(const SImageEntry &entry)
        : SImageTextureData(entry), m_Loaded(entry.m_Loaded)
    {

    }
};

struct SPrimitiveEntry
{
    // Name of the primitive as it will be in the UIP file
    CRegisteredString m_PrimitiveName;
    // Name of the primitive file on the filesystem
    CRegisteredString m_FileName;
};

struct SBufferManager : public IBufferManager
{
    typedef eastl::hash_set<CRegisteredString, eastl::hash<CRegisteredString>,
                            eastl::equal_to<CRegisteredString>, ForwardingAllocator>
        TStringSet;
    typedef nvhash_map<CRegisteredString, SImageEntry> TImageMap;
    typedef nvhash_map<CRegisteredString, SRenderMesh *> TMeshMap;
    typedef nvhash_map<CRegisteredString, CRegisteredString> TAliasImageMap;

    NVScopedRefCounted<NVRenderContext> m_Context;
    NVScopedRefCounted<IStringTable> m_StrTable;
    NVScopedRefCounted<IInputStreamFactory> m_InputStreamFactory;
    IPerfTimer &m_PerfTimer;
    volatile QT3DSI32 mRefCount;
    TStr m_PathBuilder;
    TImageMap m_ImageMap;
    Mutex m_LoadedImageSetMutex;
    TStringSet m_LoadedImageSet;
    TAliasImageMap m_AliasImageMap;
    TMeshMap m_MeshMap;
    SPrimitiveEntry m_PrimitiveNames[5];
    nvvector<qt3ds::render::NVRenderVertexBufferEntry> m_EntryBuffer;
    bool m_GPUSupportsCompressedTextures;
    bool m_reloadableResources;
    QHash<QString, QSharedPointer<QQmlImageProviderBase> > m_imageProviders;

    QHash<QString, ReloadableTexturePtr> m_reloadableTextures;

    static const char8_t *GetPrimitivesDirectory() { return "res//primitives"; }

    SBufferManager(NVRenderContext &ctx, IStringTable &strTable,
                   IInputStreamFactory &inInputStreamFactory, IPerfTimer &inTimer)
        : m_Context(ctx)
        , m_StrTable(strTable)
        , m_InputStreamFactory(inInputStreamFactory)
        , m_PerfTimer(inTimer)
        , mRefCount(0)
        , m_PathBuilder(ForwardingAllocator(ctx.GetAllocator(), "SBufferManager::m_PathBuilder"))
        , m_ImageMap(ctx.GetAllocator(), "SBufferManager::m_ImageMap")
        , m_LoadedImageSetMutex(ctx.GetAllocator())
        , m_LoadedImageSet(
              ForwardingAllocator(ctx.GetAllocator(), "SBufferManager::m_LoadedImageSet"))
        , m_AliasImageMap(ctx.GetAllocator(), "SBufferManager::m_AliasImageMap")
        , m_MeshMap(ctx.GetAllocator(), "SBufferManager::m_MeshMap")
        , m_EntryBuffer(ctx.GetAllocator(), "SBufferManager::m_EntryBuffer")
        , m_GPUSupportsCompressedTextures(ctx.AreCompressedTexturesSupported())
        , m_reloadableResources(false)
    {
    }
    virtual ~SBufferManager() { Clear(); }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Context->GetAllocator())

    CRegisteredString CombineBaseAndRelative(const char8_t *inBase,
                                                     const char8_t *inRelative) override
    {
        CFileTools::CombineBaseAndRelative(inBase, inRelative, m_PathBuilder);
        return m_StrTable->RegisterStr(m_PathBuilder.c_str());
    }

    void SetImageHasTransparency(CRegisteredString inImagePath, bool inHasTransparency,
                                 bool hasOpaque) override
    {
        pair<TImageMap::iterator, bool> theImage =
            m_ImageMap.insert(make_pair(inImagePath, SImageEntry()));
        theImage.first->second.m_TextureFlags.SetHasTransparency(inHasTransparency);
        theImage.first->second.m_TextureFlags.setHasOpaquePixels(hasOpaque);
    }

    bool GetImageHasTransparency(CRegisteredString inSourcePath) const override
    {
        TImageMap::const_iterator theIter = m_ImageMap.find(inSourcePath);
        if (theIter != m_ImageMap.end())
            return theIter->second.m_TextureFlags.HasTransparency();
        return false;
    }

    bool GetImageHasOpaquePixels(CRegisteredString inSourcePath) const override
    {
        TImageMap::const_iterator theIter = m_ImageMap.find(inSourcePath);
        if (theIter != m_ImageMap.end())
            return theIter->second.m_TextureFlags.HasOpaquePixels();
        return false;
    }

    void SetImageTransparencyToFalseIfNotSet(CRegisteredString inSourcePath) override
    {
        pair<TImageMap::iterator, bool> theImage =
            m_ImageMap.insert(make_pair(inSourcePath, SImageEntry()));
        // If we did actually insert something
        if (theImage.second)
            theImage.first->second.m_TextureFlags.SetHasTransparency(false);
    }

    void SetInvertImageUVCoords(CRegisteredString inImagePath, bool inShouldInvertCoords) override
    {
        pair<TImageMap::iterator, bool> theImage =
            m_ImageMap.insert(make_pair(inImagePath, SImageEntry()));
        theImage.first->second.m_TextureFlags.SetInvertUVCoords(inShouldInvertCoords);
    }

    bool IsImageLoaded(CRegisteredString inSourcePath) override
    {
        Mutex::ScopedLock __locker(m_LoadedImageSetMutex);
        return m_LoadedImageSet.find(inSourcePath) != m_LoadedImageSet.end();
    }

    bool AliasImagePath(CRegisteredString inSourcePath, CRegisteredString inAliasPath,
                                bool inIgnoreIfLoaded) override
    {
        if (inSourcePath.IsValid() == false || inAliasPath.IsValid() == false)
            return false;
        // If the image is loaded then we ignore this call in some cases.
        if (inIgnoreIfLoaded && IsImageLoaded(inSourcePath))
            return false;
        m_AliasImageMap.insert(eastl::make_pair(inSourcePath, inAliasPath));
        return true;
    }

    void UnaliasImagePath(CRegisteredString inSourcePath) override
    {
        m_AliasImageMap.erase(inSourcePath);
    }

    CRegisteredString GetImagePath(CRegisteredString inSourcePath) override
    {
        TAliasImageMap::iterator theAliasIter = m_AliasImageMap.find(inSourcePath);
        if (theAliasIter != m_AliasImageMap.end())
            return theAliasIter->second;
        return inSourcePath;
    }

    CRegisteredString getImagePath(const QString &path)
    {
        TAliasImageMap::iterator theAliasIter
                = m_AliasImageMap.find(m_StrTable->RegisterStr(qPrintable(path)));
        if (theAliasIter != m_AliasImageMap.end())
            return theAliasIter->second;
        return m_StrTable->RegisterStr(qPrintable(path));
    }

    static inline int wrapMod(int a, int base)
    {
        int ret = a % base;
        if (ret < 0)
            ret += base;
        return ret;
    }

    static inline void getWrappedCoords(int &sX, int &sY, int width, int height)
    {
        if (sY < 0) {
            sX -= width >> 1;
            sY = -sY;
        }
        if (sY >= height) {
            sX += width >> 1;
            sY = height - sY;
        }
        sX = wrapMod(sX, width);
        sY = wrapMod(sY, height);
    }

    template <typename V, typename C>
    void iterateAll(const V &vv, C c)
    {
        for (const auto x : vv)
            c(x);
    }

    void loadTextureImage(SReloadableImageTextureData &data, bool flipCompressed = false)
    {
        CRegisteredString imagePath = getImagePath(data.m_path);
        TImageMap::iterator theIter = m_ImageMap.find(imagePath);
        if ((theIter == m_ImageMap.end() || theIter->second.m_Loaded == false)
                && imagePath.IsValid()) {
            NVScopedReleasable<SLoadedTexture> theLoadedImage;
            SImageTextureData textureData;

            doImageLoad(imagePath, theLoadedImage, flipCompressed);

            if (theLoadedImage) {
                textureData = LoadRenderImage(imagePath, *theLoadedImage, data.m_scanTransparency,
                                              data.m_bsdfMipmap);
                data.m_Texture = textureData.m_Texture;
                data.m_TextureFlags = textureData.m_TextureFlags;
                data.m_BSDFMipMap = textureData.m_BSDFMipMap;
                data.m_loaded = true;
                iterateAll(data.m_callbacks, [](SImage *img){ img->m_Flags.SetDirty(true); });
            } else {
                // We want to make sure that bad path fails once and doesn't fail over and over
                // again which could slow down the system quite a bit.
                pair<TImageMap::iterator, bool> theImage =
                        m_ImageMap.insert(make_pair(imagePath, SImageEntry()));
                theImage.first->second.m_Loaded = true;
                qCWarning(WARNING, "Failed to load image: %s", imagePath.c_str());
                theIter = theImage.first;
            }
        } else {
            SImageEntry textureData = theIter->second;
            if (textureData.m_Loaded) {
                data.m_Texture = textureData.m_Texture;
                data.m_TextureFlags = textureData.m_TextureFlags;
                data.m_BSDFMipMap = textureData.m_BSDFMipMap;
                data.m_loaded = true;
                iterateAll(data.m_callbacks, [](SImage *img){ img->m_Flags.SetDirty(true); });
            }
        }
    }

    void unloadTextureImage(SReloadableImageTextureData &data)
    {
        CRegisteredString r = m_StrTable->RegisterStr(qPrintable(data.m_path));
        data.m_loaded = false;
        data.m_Texture = nullptr;
        data.m_BSDFMipMap = nullptr;
        data.m_TextureFlags = {};
        iterateAll(data.m_callbacks, [](SImage *img){ img->m_Flags.SetDirty(true); });
        InvalidateBuffer(r);
    }

    void loadSet(const QSet<QString> &imageSet, bool flipCompressed) override
    {
        for (const auto &x : imageSet) {
            if (!m_reloadableTextures.contains(x)) {
                auto img = CreateReloadableImage(m_StrTable->RegisterStr(qPrintable(x)), false,
                                                 false, flipCompressed);
                img->m_initialized = false;
                loadTextureImage(*m_reloadableTextures[x]);
            } else if (!m_reloadableTextures[x]->m_loaded) {
                loadTextureImage(*m_reloadableTextures[x]);
            }
        }
    }

    void unloadSet(const QSet<QString> &imageSet) override
    {
        for (const auto &x : imageSet) {
            if (m_reloadableTextures.contains(x)) {
                if (m_reloadableTextures[x]->m_loaded)
                    unloadTextureImage(*m_reloadableTextures[x]);
            }
        }
    }

    void reloadAll(bool flipCompressed) override
    {
        for (const auto &tx : qAsConst(m_reloadableTextures)) {
            if (tx->m_loaded) {
                unloadTextureImage(*tx.data());
                loadTextureImage(*tx.data(), flipCompressed);
            }
        }
    }

    virtual ReloadableTexturePtr CreateReloadableImage(CRegisteredString inSourcePath,
                                                       bool inForceScanForTransparency,
                                                       bool inBsdfMipmaps,
                                                       bool flipCompressed) override
    {
        QString path = QString::fromLatin1(inSourcePath.c_str());
        const bool inserted = m_reloadableTextures.contains(path);
        if (!inserted || (inserted && m_reloadableTextures[path]->m_initialized == false)) {
            if (!inserted)
                m_reloadableTextures.insert(path, ReloadableTexturePtr::create());
            m_reloadableTextures[path]->m_path = path;
            m_reloadableTextures[path]->m_scanTransparency = inForceScanForTransparency;
            m_reloadableTextures[path]->m_bsdfMipmap = inBsdfMipmaps;
            m_reloadableTextures[path]->m_initialized = true;


#ifndef LEGACY_ASTC_LOADING
            if (!m_reloadableResources)
#endif
                loadTextureImage(*m_reloadableTextures[path], flipCompressed);

            CRegisteredString imagePath = getImagePath(path);
            TImageMap::iterator theIter = m_ImageMap.find(imagePath);
            if (theIter != m_ImageMap.end()) {
                SImageEntry textureData = theIter->second;
                if (textureData.m_Loaded) {
                    m_reloadableTextures[path]->m_Texture = textureData.m_Texture;
                    m_reloadableTextures[path]->m_TextureFlags = textureData.m_TextureFlags;
                    m_reloadableTextures[path]->m_BSDFMipMap = textureData.m_BSDFMipMap;
                    m_reloadableTextures[path]->m_loaded = true;
                }
            }
        }
        return m_reloadableTextures[path];
    }

    void doImageLoad(CRegisteredString inImagePath,
                     NVScopedReleasable<SLoadedTexture> &theLoadedImage,
                     bool inFlipCompressed = false)
    {
        QT3DS_PERF_SCOPED_TIMER(m_PerfTimer, "BufferManager: Image Decompression")
        theLoadedImage = SLoadedTexture::Load(
                    inImagePath.c_str(), m_Context->GetFoundation(), *m_InputStreamFactory,
                    true, inFlipCompressed, m_Context->GetRenderContextType(), false, this);
        // Hackish solution to custom materials not finding their textures if they are used
        // in sub-presentations.
        if (!theLoadedImage) {
            if (QDir(inImagePath.c_str()).isRelative()) {
                QString searchPath = inImagePath.c_str();

                // Trying relative to search directories.
                if (searchPath.startsWith(QLatin1String("../"))) {
                    auto searchPathRel = searchPath.right(searchPath.length() - 3);
                    theLoadedImage = SLoadedTexture::Load(
                                searchPathRel.toUtf8(), m_Context->GetFoundation(),
                                *m_InputStreamFactory, true, false,
                                m_Context->GetRenderContextType(), false, this);
                }

                if (!theLoadedImage) {
                    if (searchPath.startsWith(QLatin1String("./")))
                        searchPath.prepend(QLatin1Char('.'));

                    int loops = 0;
                    while (!theLoadedImage && ++loops <= 3) {
                        theLoadedImage = SLoadedTexture::Load(
                                    searchPath.toUtf8(), m_Context->GetFoundation(),
                                    *m_InputStreamFactory, true, false,
                                    m_Context->GetRenderContextType(), false, this);
                        searchPath.prepend(QLatin1String("../"));
                    }
                }


            } else {
                // Some textures, for example environment maps for custom materials,
                // have absolute path at this point. It points to the wrong place with
                // the new project structure, so we need to split it up and construct
                // the new absolute path here.
                QString wholePath = inImagePath.c_str();
                QStringList splitPath = wholePath.split(QLatin1String("../"));
                if (splitPath.size() > 1) {
                    QString searchPath = splitPath.at(0) + splitPath.at(1);
                    int loops = 0;
                    while (!theLoadedImage && ++loops <= 3) {
                        theLoadedImage = SLoadedTexture::Load(
                                    searchPath.toUtf8(), m_Context->GetFoundation(),
                                    *m_InputStreamFactory, true, false,
                                    m_Context->GetRenderContextType(), false, this);
                        searchPath = splitPath.at(0);
                        for (int i = 0; i < loops; i++)
                            searchPath.append(QLatin1String("../"));
                        searchPath.append(splitPath.at(1));
                    }
                }
            }
        }
    }

    void enableReloadableResources(bool enable) override
    {
        m_reloadableResources = enable;
    }

    bool isReloadableResourcesEnabled() const override
    {
        return m_reloadableResources;
    }

    SImageTextureData LoadRenderImage(CRegisteredString inImagePath,
                                      SLoadedTexture &inLoadedImage,
                                      bool inForceScanForTransparency, bool inBsdfMipmaps) override
    {
        QT3DS_PERF_SCOPED_TIMER(m_PerfTimer, "BufferManager: Image Upload")
        {
            Mutex::ScopedLock __mapLocker(m_LoadedImageSetMutex);
            m_LoadedImageSet.insert(inImagePath);
        }
        pair<TImageMap::iterator, bool> theImage =
            m_ImageMap.insert(make_pair(inImagePath, SImageEntry()));
        bool wasInserted = theImage.second;
        theImage.first->second.m_Loaded = true;
        // inLoadedImage.EnsureMultiplerOfFour( m_Context->GetFoundation(), inImagePath.c_str() );

        NVRenderTexture2D *theTexture = m_Context->CreateTexture2D();
        if (inLoadedImage.data) {
            qt3ds::render::NVRenderTextureFormats::Enum destFormat = inLoadedImage.format;
            if (inBsdfMipmaps) {
                if (inLoadedImage.format != NVRenderTextureFormats::RGBE8) {
                    if (m_Context->GetRenderContextType() == render::NVRenderContextValues::GLES2)
                        destFormat = qt3ds::render::NVRenderTextureFormats::RGBA8;
                    else
                        destFormat = qt3ds::render::NVRenderTextureFormats::RGBA16F;
                }
            }
            else {
                theTexture->SetTextureData(
                    NVDataRef<QT3DSU8>((QT3DSU8 *)inLoadedImage.data, inLoadedImage.dataSizeInBytes), 0,
                    inLoadedImage.width, inLoadedImage.height, inLoadedImage.format, destFormat);
                {
                    static int enable = qEnvironmentVariableIntValue("QT3DS_GENERATE_MIPMAPS");
                    if (enable) {
                        theTexture->SetMinFilter(NVRenderTextureMinifyingOp::LinearMipmapLinear);
                        theTexture->SetMagFilter(NVRenderTextureMagnifyingOp::Linear);
                        theTexture->GenerateMipmaps();
                    }
                }
            }

            if (inBsdfMipmaps
                && NVRenderTextureFormats::isUncompressedTextureFormat(inLoadedImage.format)) {
                theTexture->SetMinFilter(NVRenderTextureMinifyingOp::LinearMipmapLinear);
                Qt3DSRenderPrefilterTexture *theBSDFMipMap = theImage.first->second.m_BSDFMipMap;
                if (theBSDFMipMap == NULL) {
                    theBSDFMipMap = Qt3DSRenderPrefilterTexture::Create(
                        m_Context, inLoadedImage.width, inLoadedImage.height, *theTexture,
                        destFormat, m_Context->GetFoundation());
                    theImage.first->second.m_BSDFMipMap = theBSDFMipMap;
                }

                if (theBSDFMipMap) {
                    theBSDFMipMap->Build(inLoadedImage.data, inLoadedImage.dataSizeInBytes,
                                         inLoadedImage.format);
                }
            }
        } else if (inLoadedImage.dds) {
            theImage.first->second.m_Texture = theTexture;
            bool supportsCompressedTextures = m_GPUSupportsCompressedTextures;
            bool isACompressedTexture
                    = NVRenderTextureFormats::isCompressedTextureFormat(inLoadedImage.format);
            bool requiresDecompression
                    = (supportsCompressedTextures == false && isACompressedTexture) || false;
            // test code for DXT decompression
            // if ( isDXT ) requiresDecompression = true;
            if (requiresDecompression) {
                qCWarning(WARNING, PERF_INFO,
                    "Image %s is compressed format which is unsupported by "
                    "the graphics subsystem, decompressing in CPU",
                    inImagePath.c_str());
            }
            STextureData theDecompressedImage;
            for (int idx = 0; idx < inLoadedImage.dds->numMipmaps; ++idx) {
                if (inLoadedImage.dds->mipwidth[idx] && inLoadedImage.dds->mipheight[idx]) {
                    if (requiresDecompression == false) {
                        theTexture->SetTextureData(
                            toU8DataRef((char *)inLoadedImage.dds->data[idx],
                                        (QT3DSU32)inLoadedImage.dds->size[idx]),
                            (QT3DSU8)idx, (QT3DSU32)inLoadedImage.dds->mipwidth[idx],
                            (QT3DSU32)inLoadedImage.dds->mipheight[idx], inLoadedImage.format);
                    } else {
                        theDecompressedImage =
                            inLoadedImage.DecompressDXTImage(idx, &theDecompressedImage);

                        if (theDecompressedImage.data) {
                            theTexture->SetTextureData(
                                toU8DataRef((char *)theDecompressedImage.data,
                                            (QT3DSU32)theDecompressedImage.dataSizeInBytes),
                                (QT3DSU8)idx, (QT3DSU32)inLoadedImage.dds->mipwidth[idx],
                                (QT3DSU32)inLoadedImage.dds->mipheight[idx],
                                theDecompressedImage.format);
                        }
                    }
                }
            }
            if (theDecompressedImage.data)
                inLoadedImage.ReleaseDecompressedTexture(theDecompressedImage);
        }
        if (wasInserted || inForceScanForTransparency) {
            auto &flags = theImage.first->second.m_TextureFlags;
            bool alsoOpaquePixels = false;
            flags.SetHasTransparency(inLoadedImage.ScanForTransparency(alsoOpaquePixels));
            flags.setHasOpaquePixels(alsoOpaquePixels);
        }
        theImage.first->second.m_Texture = theTexture;
        return theImage.first->second;
    }

    SImageTextureData LoadRenderImage(CRegisteredString inImagePath,
                                      bool inForceScanForTransparency, bool inBsdfMipmaps) override
    {
        inImagePath = GetImagePath(inImagePath);

        if (!inImagePath.IsValid())
            return SImageEntry();

        TImageMap::iterator theIter = m_ImageMap.find(inImagePath);
        if (theIter == m_ImageMap.end() && inImagePath.IsValid()) {
            NVScopedReleasable<SLoadedTexture> theLoadedImage;

            doImageLoad(inImagePath, theLoadedImage);

            if (theLoadedImage) {
                return LoadRenderImage(inImagePath, *theLoadedImage, inForceScanForTransparency,
                                       inBsdfMipmaps);
            } else {
                // We want to make sure that bad path fails once and doesn't fail over and over
                // again
                // which could slow down the system quite a bit.
                pair<TImageMap::iterator, bool> theImage =
                        m_ImageMap.insert(make_pair(inImagePath, SImageEntry()));
                theImage.first->second.m_Loaded = true;
                qCWarning(WARNING, "Failed to load image: %s", inImagePath.c_str());
                theIter = theImage.first;
            }
        }
        return theIter->second;
    }

    qt3dsimp::SMultiLoadResult LoadPrimitive(const char8_t *inRelativePath)
    {
        CRegisteredString theName(m_StrTable->RegisterStr(inRelativePath));
        if (m_PrimitiveNames[0].m_PrimitiveName.IsValid() == false) {
            IStringTable &strTable(m_Context->GetStringTable());
            m_PrimitiveNames[0].m_PrimitiveName = strTable.RegisterStr("#Rectangle");
            m_PrimitiveNames[0].m_FileName = strTable.RegisterStr("Rectangle.mesh");
            m_PrimitiveNames[1].m_PrimitiveName = strTable.RegisterStr("#Sphere");
            m_PrimitiveNames[1].m_FileName = strTable.RegisterStr("Sphere.mesh");
            m_PrimitiveNames[2].m_PrimitiveName = strTable.RegisterStr("#Cube");
            m_PrimitiveNames[2].m_FileName = strTable.RegisterStr("Cube.mesh");
            m_PrimitiveNames[3].m_PrimitiveName = strTable.RegisterStr("#Cone");
            m_PrimitiveNames[3].m_FileName = strTable.RegisterStr("Cone.mesh");
            m_PrimitiveNames[4].m_PrimitiveName = strTable.RegisterStr("#Cylinder");
            m_PrimitiveNames[4].m_FileName = strTable.RegisterStr("Cylinder.mesh");
        }
        for (size_t idx = 0; idx < 5; ++idx) {
            if (m_PrimitiveNames[idx].m_PrimitiveName == theName) {
                CFileTools::CombineBaseAndRelative(GetPrimitivesDirectory(),
                                                   m_PrimitiveNames[idx].m_FileName, m_PathBuilder);
                QT3DSU32 id = 1;
                NVScopedRefCounted<IRefCountedInputStream> theInStream(
                    m_InputStreamFactory->GetStreamForFile(m_PathBuilder.c_str()));
                if (theInStream)
                    return qt3dsimp::Mesh::LoadMulti(m_Context->GetAllocator(), *theInStream, id);
                else {
                    qCCritical(INTERNAL_ERROR, "Unable to find mesh primitive %s",
                        m_PathBuilder.c_str());
                    return qt3dsimp::SMultiLoadResult();
                }
            }
        }
        return qt3dsimp::SMultiLoadResult();
    }

    virtual NVConstDataRef<QT3DSU8> CreatePackedPositionDataArray(
            const qt3dsimp::SMultiLoadResult &inResult)
    {
        // we assume a position consists of 3 floats
        QT3DSU32 vertexCount = inResult.m_Mesh->m_VertexBuffer.m_Data.size()
            / inResult.m_Mesh->m_VertexBuffer.m_Stride;
        QT3DSU32 dataSize = vertexCount * 3 * sizeof(QT3DSF32);
        QT3DSF32 *posData = static_cast<QT3DSF32 *>(
                    QT3DS_ALLOC(m_Context->GetAllocator(), dataSize,
                                "SRenderMesh::CreatePackedPositionDataArray"));
        QT3DSU8 *baseOffset = reinterpret_cast<QT3DSU8 *>(inResult.m_Mesh);
        // copy position data
        if (posData) {
            QT3DSF32 *srcData = reinterpret_cast<QT3DSF32 *>(
                        inResult.m_Mesh->m_VertexBuffer.m_Data.begin(baseOffset));
            QT3DSU32 srcStride = inResult.m_Mesh->m_VertexBuffer.m_Stride / sizeof(QT3DSF32);
            QT3DSF32 *dstData = posData;
            QT3DSU32 dstStride = 3;

            for (QT3DSU32 i = 0; i < vertexCount; ++i) {
                dstData[0] = srcData[0];
                dstData[1] = srcData[1];
                dstData[2] = srcData[2];

                dstData += dstStride;
                srcData += srcStride;
            }

            return toConstDataRef(reinterpret_cast<const qt3ds::QT3DSU8 *>(posData), dataSize);
        }

        return NVConstDataRef<QT3DSU8>();
    }

    SRenderMesh *createRenderMesh(const qt3dsimp::SMultiLoadResult &result)
    {
        SRenderMesh *theNewMesh = QT3DS_NEW(m_Context->GetAllocator(), SRenderMesh)(
            qt3ds::render::NVRenderDrawMode::Triangles,
            qt3ds::render::NVRenderWinding::CounterClockwise, result.m_Id,
            m_Context->GetAllocator());
        QT3DSU8 *baseAddress = reinterpret_cast<QT3DSU8 *>(result.m_Mesh);
        NVConstDataRef<QT3DSU8> theVBufData(
            result.m_Mesh->m_VertexBuffer.m_Data.begin(baseAddress),
            result.m_Mesh->m_VertexBuffer.m_Data.size());

        NVRenderVertexBuffer *theVertexBuffer = m_Context->CreateVertexBuffer(
            qt3ds::render::NVRenderBufferUsageType::Static,
            result.m_Mesh->m_VertexBuffer.m_Data.m_Size,
            result.m_Mesh->m_VertexBuffer.m_Stride, theVBufData);

        // create a tight packed position data VBO
        // this should improve our depth pre pass rendering
        NVRenderVertexBuffer *thePosVertexBuffer = nullptr;
        NVConstDataRef<QT3DSU8> posData = CreatePackedPositionDataArray(result);
        if (posData.size()) {
            thePosVertexBuffer
                = m_Context->CreateVertexBuffer(qt3ds::render::NVRenderBufferUsageType::Static,
                                                posData.size(), 3 * sizeof(QT3DSF32), posData);
        }

        NVRenderIndexBuffer *theIndexBuffer = nullptr;
        if (result.m_Mesh->m_IndexBuffer.m_Data.size()) {
            using qt3ds::render::NVRenderComponentTypes;
            QT3DSU32 theIndexBufferSize = result.m_Mesh->m_IndexBuffer.m_Data.size();
            NVRenderComponentTypes::Enum bufComponentType =
                result.m_Mesh->m_IndexBuffer.m_ComponentType;
            QT3DSU32 sizeofType
                = qt3ds::render::NVRenderComponentTypes::getSizeofType(bufComponentType);

            if (sizeofType == 2 || sizeofType == 4) {
                // Ensure type is unsigned; else things will fail in rendering pipeline.
                if (bufComponentType == NVRenderComponentTypes::QT3DSI16)
                    bufComponentType = NVRenderComponentTypes::QT3DSU16;
                if (bufComponentType == NVRenderComponentTypes::QT3DSI32)
                    bufComponentType = NVRenderComponentTypes::QT3DSU32;

                NVConstDataRef<QT3DSU8> theIBufData(
                    result.m_Mesh->m_IndexBuffer.m_Data.begin(baseAddress),
                    result.m_Mesh->m_IndexBuffer.m_Data.size());
                theIndexBuffer = m_Context->CreateIndexBuffer(
                    qt3ds::render::NVRenderBufferUsageType::Static, bufComponentType,
                    theIndexBufferSize, theIBufData);
            } else {
                QT3DS_ASSERT(false);
            }
        }
        nvvector<qt3ds::render::NVRenderVertexBufferEntry> &theEntryBuffer(m_EntryBuffer);
        theEntryBuffer.resize(result.m_Mesh->m_VertexBuffer.m_Entries.size());
        for (QT3DSU32 entryIdx = 0,
             entryEnd = result.m_Mesh->m_VertexBuffer.m_Entries.size();
             entryIdx < entryEnd; ++entryIdx) {
            theEntryBuffer[entryIdx]
                = result.m_Mesh->m_VertexBuffer.m_Entries.index(baseAddress, entryIdx)
                    .ToVertexBufferEntry(baseAddress);
        }
        // create our attribute layout
        NVRenderAttribLayout *theAttribLayout
            = m_Context->CreateAttributeLayout(theEntryBuffer);
        // create our attribute layout for depth pass
        qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
            qt3ds::render::NVRenderVertexBufferEntry(
                "attr_pos", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
        };
        NVRenderAttribLayout *theAttribLayoutDepth
            = m_Context->CreateAttributeLayout(toConstDataRef(theEntries, 1));

        // create input assembler object
        QT3DSU32 strides = result.m_Mesh->m_VertexBuffer.m_Stride;
        QT3DSU32 offsets = 0;
        NVRenderInputAssembler *theInputAssembler = m_Context->CreateInputAssembler(
            theAttribLayout, toConstDataRef(&theVertexBuffer, 1), theIndexBuffer,
            toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1),
            result.m_Mesh->m_DrawMode);

        // create depth input assembler object
        QT3DSU32 posStrides = thePosVertexBuffer ? 3 * sizeof(QT3DSF32) : strides;
        NVRenderInputAssembler *theInputAssemblerDepth = m_Context->CreateInputAssembler(
            theAttribLayoutDepth,
            toConstDataRef(thePosVertexBuffer ? &thePosVertexBuffer : &theVertexBuffer, 1),
                    theIndexBuffer, toConstDataRef(&posStrides, 1), toConstDataRef(&offsets, 1),
            result.m_Mesh->m_DrawMode);

        NVRenderInputAssembler *theInputAssemblerPoints = m_Context->CreateInputAssembler(
            theAttribLayoutDepth,
            toConstDataRef(thePosVertexBuffer ? &thePosVertexBuffer : &theVertexBuffer, 1),
                    nullptr, toConstDataRef(&posStrides, 1), toConstDataRef(&offsets, 1),
            NVRenderDrawMode::Points);

        if (!theInputAssembler || !theInputAssemblerDepth || !theInputAssemblerPoints) {
            QT3DS_ASSERT(false);
            return nullptr;
        }
        theNewMesh->m_Joints.resize(result.m_Mesh->m_Joints.size());
        for (QT3DSU32 jointIdx = 0, jointEnd = result.m_Mesh->m_Joints.size();
             jointIdx < jointEnd; ++jointIdx) {
            const qt3dsimp::Joint &theImportJoint(
                result.m_Mesh->m_Joints.index(baseAddress, jointIdx));
            SRenderJoint &theNewJoint(theNewMesh->m_Joints[jointIdx]);
            theNewJoint.m_JointID = theImportJoint.m_JointID;
            theNewJoint.m_ParentID = theImportJoint.m_ParentID;
            memCopy(theNewJoint.m_invBindPose, theImportJoint.m_invBindPose,
                    16 * sizeof(QT3DSF32));
            memCopy(theNewJoint.m_localToGlobalBoneSpace,
                    theImportJoint.m_localToGlobalBoneSpace, 16 * sizeof(QT3DSF32));
        }

        for (QT3DSU32 subsetIdx = 0, subsetEnd = result.m_Mesh->m_Subsets.size();
             subsetIdx < subsetEnd; ++subsetIdx) {
            SRenderSubset theSubset(m_Context->GetAllocator());
            const qt3dsimp::MeshSubset &source(
                result.m_Mesh->m_Subsets.index(baseAddress, subsetIdx));
            theSubset.m_Bounds = source.m_Bounds;
            theSubset.m_Count = source.m_Count;
            theSubset.m_Offset = source.m_Offset;
            theSubset.m_Joints = theNewMesh->m_Joints;
            theSubset.m_Name = m_StrTable->RegisterStr(source.m_Name.begin(baseAddress));
            theVertexBuffer->addRef();
            theSubset.m_VertexBuffer = theVertexBuffer;
            if (thePosVertexBuffer) {
                thePosVertexBuffer->addRef();
                theSubset.m_PosVertexBuffer = thePosVertexBuffer;
            }
            if (theIndexBuffer) {
                theIndexBuffer->addRef();
                theSubset.m_IndexBuffer = theIndexBuffer;
            }
            theSubset.m_InputAssembler = theInputAssembler;
            theSubset.m_InputAssemblerDepth = theInputAssemblerDepth;
            theSubset.m_InputAssemblerPoints = theInputAssemblerPoints;
            theSubset.m_PrimitiveType = result.m_Mesh->m_DrawMode;
            theInputAssembler->addRef();
            theInputAssemblerDepth->addRef();
            theSubset.m_InputAssemblerPoints->addRef();
            theNewMesh->m_Subsets.push_back(theSubset);
        }
        // If we want to, we can break up models into sub-subsets.
        // These are assumed to use the same material as the outer subset but have fewer triangles
        // and should have a more exact bounding box. This sort of thing helps with using the
        // frustum culling system but it is really done incorrectly.
        // It should be done via some sort of oct-tree mechanism and so that the sub-subsets
        // are spatially sorted and it should only be done upon save-to-binary with the
        // results saved out to disk. As you can see, doing it properly requires some real
        // engineering effort so it is somewhat unlikely it will ever happen.
        // Or it could be done on import if someone really wants to change the mesh buffer
        // format. Either way it isn't going to happen here and it isn't going to happen this way
        // but this is a working example of using the technique.
#ifdef QT3DS_RENDER_GENERATE_SUB_SUBSETS
        Option<qt3ds::render::NVRenderVertexBufferEntry> thePosAttrOpt
            = theVertexBuffer->GetEntryByName("attr_pos");
        bool hasPosAttr = thePosAttrOpt.hasValue()
            && thePosAttrOpt->m_ComponentType == qt3ds::render::NVRenderComponentTypes::QT3DSF32
            && thePosAttrOpt->m_NumComponents == 3;

        for (size_t subsetIdx = 0, subsetEnd = theNewMesh->m_Subsets.size();
             subsetIdx < subsetEnd; ++subsetIdx) {
            SRenderSubset &theOuterSubset = theNewMesh->m_Subsets[subsetIdx];
            if (theOuterSubset.m_Count && theIndexBuffer
                && theIndexBuffer->GetComponentType()
                    == qt3ds::render::NVRenderComponentTypes::QT3DSU16
                && theNewMesh->m_DrawMode == NVRenderDrawMode::Triangles && hasPosAttr) {
                // Num tris in a sub subset.
                QT3DSU32 theSubsetSize = 3334 * 3; // divisible by three.
                size_t theNumSubSubsets = ((theOuterSubset.m_Count - 1) / theSubsetSize) + 1;
                QT3DSU32 thePosAttrOffset = thePosAttrOpt->m_FirstItemOffset;
                const QT3DSU8 *theVertData = result.m_Mesh->m_VertexBuffer.m_Data.begin();
                const QT3DSU8 *theIdxData = result.m_Mesh->m_IndexBuffer.m_Data.begin();
                QT3DSU32 theVertStride = result.m_Mesh->m_VertexBuffer.m_Stride;
                QT3DSU32 theOffset = theOuterSubset.m_Offset;
                QT3DSU32 theCount = theOuterSubset.m_Count;
                for (size_t subSubsetIdx = 0, subSubsetEnd = theNumSubSubsets;
                     subSubsetIdx < subSubsetEnd; ++subSubsetIdx) {
                    SRenderSubsetBase theBase;
                    theBase.m_Offset = theOffset;
                    theBase.m_Count = NVMin(theSubsetSize, theCount);
                    theBase.m_Bounds.setEmpty();
                    theCount -= theBase.m_Count;
                    theOffset += theBase.m_Count;
                    // Create new bounds.
                    // Offset is in item size, not bytes.
                    const QT3DSU16 *theSubsetIdxData
                        = reinterpret_cast<const QT3DSU16 *>(theIdxData + theBase.m_Offset * 2);
                    for (size_t theIdxIdx = 0, theIdxEnd = theBase.m_Count;
                         theIdxIdx < theIdxEnd; ++theIdxIdx) {
                        QT3DSU32 theVertOffset = theSubsetIdxData[theIdxIdx] * theVertStride;
                        theVertOffset += thePosAttrOffset;
                        QT3DSVec3 thePos = *(
                            reinterpret_cast<const QT3DSVec3 *>(theVertData + theVertOffset));
                        theBase.m_Bounds.include(thePos);
                    }
                    theOuterSubset.m_SubSubsets.push_back(theBase);
                }
            } else {
                SRenderSubsetBase theBase;
                theBase.m_Bounds = theOuterSubset.m_Bounds;
                theBase.m_Count = theOuterSubset.m_Count;
                theBase.m_Offset = theOuterSubset.m_Offset;
                theOuterSubset.m_SubSubsets.push_back(theBase);
            }
        }
#endif
        if (posData.size()) {
            m_Context->GetAllocator().deallocate(
                        static_cast<void *>(const_cast<qt3ds::QT3DSU8 *>(posData.begin())));
        }

        return theNewMesh;
    }

    void loadCustomMesh(const QString &name, qt3dsimp::Mesh *mesh) override
    {
        if (!name.isEmpty() && mesh) {
            CRegisteredString meshName = m_StrTable->RegisterStr(name);
            pair<TMeshMap::iterator, bool> theMesh
                = m_MeshMap.insert({ meshName, static_cast<SRenderMesh *>(nullptr) });
            // Only create the mesh if it doesn't yet exist
            if (theMesh.second) {
                qt3dsimp::SMultiLoadResult result;
                result.m_Mesh = mesh;
                theMesh.first->second = createRenderMesh(result);
            }
        }
    }

    SRenderMesh *LoadMesh(CRegisteredString inMeshPath) override
    {
        if (inMeshPath.IsValid() == false)
            return nullptr;
        pair<TMeshMap::iterator, bool> theMesh =
            m_MeshMap.insert(make_pair(inMeshPath, static_cast<SRenderMesh *>(nullptr)));
        if (theMesh.second) {
            // Check to see if this is primitive
            qt3dsimp::SMultiLoadResult theResult = LoadPrimitive(inMeshPath);

            // Attempt a load from the filesystem if this mesh isn't a primitive.
            if (!theResult.m_Mesh) {
                m_PathBuilder = inMeshPath;
                TStr::size_type pound = m_PathBuilder.rfind('#');
                QT3DSU32 id = 0;
                if (pound != TStr::npos) {
                    id = QT3DSU32(atoi(m_PathBuilder.c_str() + pound + 1));
                    m_PathBuilder.erase(m_PathBuilder.begin() + pound, m_PathBuilder.end());
                }
                NVScopedRefCounted<IRefCountedInputStream> theStream(
                    m_InputStreamFactory->GetStreamForFile(m_PathBuilder.c_str()));
                if (theStream) {
                    theResult = qt3dsimp::Mesh::LoadMulti(
                                m_Context->GetAllocator(), *theStream, id);
                }
                if (!theResult.m_Mesh)
                    qCWarning(WARNING, "Failed to load mesh: %s", m_PathBuilder.c_str());
            }

            if (theResult.m_Mesh) {
                theMesh.first->second = createRenderMesh(theResult);
                m_Context->GetAllocator().deallocate(theResult.m_Mesh);
            }
        }
        return theMesh.first->second;
    }

    SRenderMesh *CreateMesh(Qt3DSBCharPtr inSourcePath, QT3DSU8 *inVertData, QT3DSU32 inNumVerts,
                            QT3DSU32 inVertStride, QT3DSU32 *inIndexData, QT3DSU32 inIndexCount,
                            qt3ds::NVBounds3 inBounds) override
    {
        CRegisteredString sourcePath = m_StrTable->RegisterStr(inSourcePath);

        // eastl::pair<CRegisteredString, SRenderMesh*> thePair(sourcePath, (SRenderMesh*)NULL);
        pair<TMeshMap::iterator, bool> theMesh;
        // Make sure there isn't already a buffer entry for this mesh.
        if (m_MeshMap.contains(sourcePath)) {
            theMesh = make_pair<TMeshMap::iterator, bool>(m_MeshMap.find(sourcePath), true);
        } else {
            theMesh = m_MeshMap.insert(make_pair(sourcePath, (SRenderMesh *)NULL));
        }

        if (theMesh.second == true) {
            SRenderMesh *theNewMesh = QT3DS_NEW(m_Context->GetAllocator(), SRenderMesh)(
                qt3ds::render::NVRenderDrawMode::Triangles,
                qt3ds::render::NVRenderWinding::CounterClockwise, 0, m_Context->GetAllocator());

            // If we failed to create the RenderMesh, return a failure.
            if (!theNewMesh) {
                QT3DS_ASSERT(false);
                return NULL;
            }

            // Get rid of any old mesh that was sitting here and fill it with a new one.
            // NOTE : This is assuming that the source of our mesh data doesn't do its own memory
            // management and always returns new buffer pointers every time.
            // Don't know for sure if that's what we'll get from our intended sources, but that's
            // easily
            // adjustable by looking for matching pointers in the Subsets.
            if (theNewMesh && theMesh.first->second != NULL) {
                delete theMesh.first->second;
                theMesh.first->second = NULL;
            }

            theMesh.first->second = theNewMesh;
            QT3DSU32 vertDataSize = inNumVerts * inVertStride;
            NVConstDataRef<QT3DSU8> theVBufData(inVertData, vertDataSize);
            // NVConstDataRef<QT3DSU8> theVBufData( theResult.m_Mesh->m_VertexBuffer.m_Data.begin(
            // baseAddress )
            //		, theResult.m_Mesh->m_VertexBuffer.m_Data.size() );

            NVRenderVertexBuffer *theVertexBuffer =
                m_Context->CreateVertexBuffer(qt3ds::render::NVRenderBufferUsageType::Static,
                                              vertDataSize, inVertStride, theVBufData);
            NVRenderIndexBuffer *theIndexBuffer = NULL;
            if (inIndexData != NULL && inIndexCount > 3) {
                NVConstDataRef<QT3DSU8> theIBufData((QT3DSU8 *)inIndexData, inIndexCount * sizeof(QT3DSU32));
                theIndexBuffer =
                    m_Context->CreateIndexBuffer(qt3ds::render::NVRenderBufferUsageType::Static,
                                                 qt3ds::render::NVRenderComponentTypes::QT3DSU32,
                                                 inIndexCount * sizeof(QT3DSU32), theIBufData);
            }

            // WARNING
            // Making an assumption here about the contents of the stream
            // PKC TODO : We may have to consider some other format.
            qt3ds::render::NVRenderVertexBufferEntry theEntries[] = {
                qt3ds::render::NVRenderVertexBufferEntry("attr_pos",
                                                      qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3),
                qt3ds::render::NVRenderVertexBufferEntry(
                    "attr_uv", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 2, 12),
                qt3ds::render::NVRenderVertexBufferEntry(
                    "attr_norm", qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3, 18),
            };

            // create our attribute layout
            NVRenderAttribLayout *theAttribLayout =
                m_Context->CreateAttributeLayout(toConstDataRef(theEntries, 3));
            /*
            // create our attribute layout for depth pass
            qt3ds::render::NVRenderVertexBufferEntry theEntriesDepth[] = {
                    qt3ds::render::NVRenderVertexBufferEntry( "attr_pos",
            qt3ds::render::NVRenderComponentTypes::QT3DSF32, 3 ),
            };
            NVRenderAttribLayout* theAttribLayoutDepth = m_Context->CreateAttributeLayout(
            toConstDataRef( theEntriesDepth, 1 ) );
            */
            // create input assembler object
            QT3DSU32 strides = inVertStride;
            QT3DSU32 offsets = 0;
            NVRenderInputAssembler *theInputAssembler = m_Context->CreateInputAssembler(
                theAttribLayout, toConstDataRef(&theVertexBuffer, 1), theIndexBuffer,
                toConstDataRef(&strides, 1), toConstDataRef(&offsets, 1),
                qt3ds::render::NVRenderDrawMode::Triangles);

            if (!theInputAssembler) {
                QT3DS_ASSERT(false);
                return NULL;
            }

            // Pull out just the mesh object name from the total path
            eastl::string fullName(inSourcePath);
            eastl::string subName(inSourcePath);
            if (fullName.rfind("#") != eastl::string::npos) {
                subName = fullName.substr(fullName.rfind("#"), eastl::string::npos);
            }

            theNewMesh->m_Joints.clear();
            SRenderSubset theSubset(m_Context->GetAllocator());
            theSubset.m_Bounds = inBounds;
            theSubset.m_Count = inIndexCount;
            theSubset.m_Offset = 0;
            theSubset.m_Joints = theNewMesh->m_Joints;
            theSubset.m_Name = m_StrTable->RegisterStr(subName.c_str());
            theVertexBuffer->addRef();
            theSubset.m_VertexBuffer = theVertexBuffer;
            theSubset.m_PosVertexBuffer = NULL;
            if (theIndexBuffer)
                theIndexBuffer->addRef();
            theSubset.m_IndexBuffer = theIndexBuffer;
            theSubset.m_InputAssembler = theInputAssembler;
            theSubset.m_InputAssemblerDepth = theInputAssembler;
            theSubset.m_InputAssemblerPoints = theInputAssembler;
            theSubset.m_PrimitiveType = qt3ds::render::NVRenderDrawMode::Triangles;
            theSubset.m_InputAssembler->addRef();
            theSubset.m_InputAssemblerDepth->addRef();
            theSubset.m_InputAssemblerPoints->addRef();
            theNewMesh->m_Subsets.push_back(theSubset);
        }

        return theMesh.first->second;
    }

    void addImageProvider(const QString &providerId, QQmlImageProviderBase *provider) override
    {
        QString providerIdLower = providerId.toLower();
        QSharedPointer<QQmlImageProviderBase> sp(provider);
        m_imageProviders.insert(std::move(providerIdLower), std::move(sp));
    }

    QQmlImageProviderBase *imageProvider(const QString &providerId) override
    {
        const QString providerIdLower = providerId.toLower();
        return m_imageProviders.value(providerIdLower).data();
    }

    void ReleaseMesh(SRenderMesh &inMesh)
    {
        for (QT3DSU32 subsetIdx = 0, subsetEnd = inMesh.m_Subsets.size(); subsetIdx < subsetEnd;
             ++subsetIdx) {
            inMesh.m_Subsets[subsetIdx].m_VertexBuffer->release();
            if (inMesh.m_Subsets[subsetIdx].m_PosVertexBuffer) // can be NULL
                inMesh.m_Subsets[subsetIdx].m_PosVertexBuffer->release();
            if (inMesh.m_Subsets[subsetIdx].m_IndexBuffer) // can be NULL
                inMesh.m_Subsets[subsetIdx].m_IndexBuffer->release();
            inMesh.m_Subsets[subsetIdx].m_InputAssembler->release();
            inMesh.m_Subsets[subsetIdx].m_InputAssemblerDepth->release();
            if (inMesh.m_Subsets[subsetIdx].m_InputAssemblerPoints)
                inMesh.m_Subsets[subsetIdx].m_InputAssemblerPoints->release();
        }
        NVDelete(m_Context->GetAllocator(), &inMesh);
    }
    void ReleaseTexture(SImageEntry &inEntry)
    {
        if (inEntry.m_Texture)
            inEntry.m_Texture->release();
        if (inEntry.m_BSDFMipMap)
            inEntry.m_BSDFMipMap->release();
    }
    void Clear() override
    {
        m_reloadableTextures.clear();
        for (TMeshMap::iterator iter = m_MeshMap.begin(), end = m_MeshMap.end(); iter != end;
             ++iter) {
            SRenderMesh *theMesh = iter->second;
            if (theMesh)
                ReleaseMesh(*theMesh);
        }
        m_MeshMap.clear();
        for (TImageMap::iterator iter = m_ImageMap.begin(), end = m_ImageMap.end(); iter != end;
             ++iter) {
            SImageEntry &theEntry = iter->second;
            ReleaseTexture(theEntry);
        }
        m_ImageMap.clear();
        m_AliasImageMap.clear();
        {
            Mutex::ScopedLock __locker(m_LoadedImageSetMutex);
            m_LoadedImageSet.clear();
        }
        m_imageProviders.clear();
    }
    void InvalidateBuffer(CRegisteredString inSourcePath) override
    {
        {
            TMeshMap::iterator iter = m_MeshMap.find(inSourcePath);
            if (iter != m_MeshMap.end()) {
                if (iter->second)
                    ReleaseMesh(*iter->second);
                m_MeshMap.erase(iter);
                return;
            }
        }
        {
            TImageMap::iterator iter = m_ImageMap.find(inSourcePath);
            if (iter != m_ImageMap.end()) {
                SImageEntry &theEntry = iter->second;
                ReleaseTexture(theEntry);
                m_ImageMap.erase(inSourcePath);
                {
                    Mutex::ScopedLock __locker(m_LoadedImageSetMutex);
                    m_LoadedImageSet.erase(inSourcePath);
                }
            }
        }
    }
    IStringTable &GetStringTable() override { return *m_StrTable; }
};
}

IBufferManager &IBufferManager::Create(NVRenderContext &inRenderContext, IStringTable &inStrTable,
                                       IInputStreamFactory &inFactory, IPerfTimer &inPerfTimer)
{
    return *QT3DS_NEW(inRenderContext.GetAllocator(), SBufferManager)(inRenderContext, inStrTable,
                                                                      inFactory, inPerfTimer);
}
