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
#pragma once
#ifndef QT3DS_RENDER_BUFFER_MANAGER_H
#define QT3DS_RENDER_BUFFER_MANAGER_H
#include "Qt3DSRender.h"
#include "EASTL/utility.h" //pair
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/StringTable.h"
#include "Qt3DSRenderImageTextureData.h"
#include "foundation/Qt3DSBounds3.h"

namespace qt3dsimp {
    struct Mesh;
}

namespace qt3ds {
namespace render {

    class IBufferManager : public NVRefCounted
    {
    protected:
        virtual ~IBufferManager() {}

    public:
        // Path manipulation used to get the final path form a base path plus relative extension
        virtual CRegisteredString CombineBaseAndRelative(const char8_t *inBase,
                                                         const char8_t *inRelative) = 0;
        virtual void SetImageHasTransparency(CRegisteredString inSourcePath,
                                             bool inHasTransparency, bool alsoOpaque) = 0;
        virtual bool GetImageHasTransparency(CRegisteredString inSourcePath) const = 0;
        virtual bool GetImageHasOpaquePixels(CRegisteredString inSourcePath) const = 0;
        virtual void SetImageTransparencyToFalseIfNotSet(CRegisteredString inSourcePath) = 0;
        virtual void SetInvertImageUVCoords(CRegisteredString inSourcePath,
                                            bool inShouldInvertCoords) = 0;

        // Returns true if this image has been loaded into memory
        // This call is threadsafe.  Nothing else on this object is guaranteed to be.
        virtual bool IsImageLoaded(CRegisteredString inSourcePath) = 0;

        // Alias one image path with another image path.  Optionally this object will ignore the
        // call if
        // the source path is already loaded.  Aliasing is currently used to allow a default image
        // to be shown
        // in place of an image that is loading offline.
        // Returns true if the image was aliased, false otherwise.
        virtual bool AliasImagePath(CRegisteredString inSourcePath, CRegisteredString inAliasPath,
                                    bool inIgnoreIfLoaded) = 0;
        virtual void UnaliasImagePath(CRegisteredString inSourcePath) = 0;

        // Returns the given source path unless the source path is aliased; in which case returns
        // the aliased path.
        virtual CRegisteredString GetImagePath(CRegisteredString inSourcePath) = 0;
        // Returns a texture and a boolean indicating if this texture has transparency in it or not.
        // Can't name this LoadImage because that gets mangled by windows to LoadImageA (uggh)
        // In some cases we need to only scan particular images for transparency.
        virtual SImageTextureData LoadRenderImage(CRegisteredString inImagePath,
                                                  SLoadedTexture &inTexture,
                                                  bool inForceScanForTransparency = false,
                                                  bool inBsdfMipmaps = false) = 0;
        virtual SImageTextureData LoadRenderImage(CRegisteredString inSourcePath,
                                                  bool inForceScanForTransparency = false,
                                                  bool inBsdfMipmaps = false) = 0;

        virtual ReloadableTexturePtr CreateReloadableImage(CRegisteredString inSourcePath,
                                                           bool inForceScanForTransparency = false,
                                                           bool inBsdfMipmaps = false) = 0;
        virtual void enableReloadableResources(bool enable) = 0;
        virtual bool isReloadableResourcesEnabled() const = 0;

        virtual void loadSet(const QSet<QString> &imageSet) = 0;
        virtual void unloadSet(const QSet<QString> &imageSet) = 0;

        virtual void loadCustomMesh(const QString &name, qt3dsimp::Mesh *mesh) = 0;
        virtual SRenderMesh *LoadMesh(CRegisteredString inSourcePath) = 0;

        virtual SRenderMesh *CreateMesh(const char *inSourcePath, QT3DSU8 *inVertData,
                                        QT3DSU32 inNumVerts, QT3DSU32 inVertStride, QT3DSU32 *inIndexData,
                                        QT3DSU32 inIndexCount, qt3ds::NVBounds3 inBounds) = 0;

        // Remove *all* buffers from the buffer manager;
        virtual void Clear() = 0;
        virtual void InvalidateBuffer(CRegisteredString inSourcePath) = 0;
        virtual IStringTable &GetStringTable() = 0;

        static IBufferManager &Create(NVRenderContext &inRenderContext, IStringTable &inStrTable,
                                      IInputStreamFactory &inInputStreamFactory,
                                      IPerfTimer &inTimer);
    };
}
}

#endif
