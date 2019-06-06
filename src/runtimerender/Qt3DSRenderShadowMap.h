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
#ifndef QT3DS_RENDER_SHADOW_MAP_H
#define QT3DS_RENDER_SHADOW_MAP_H
#include "Qt3DSRenderContextCore.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSMat44.h"
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/StringTable.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderTexture2D.h"
#ifdef _INTEGRITYPLATFORM
#include "render/Qt3DSRenderTextureCube.h"
#endif

namespace qt3ds {
namespace render {

    struct SLayerRenderData;

    struct ShadowMapModes
    {
        enum Enum {
            SSM, ///< standard shadow mapping
            VSM, ///< variance shadow mapping
            CUBE, ///< cubemap omnidirectional shadows
        };
    };

    struct ShadowFilterValues
    {
        enum Enum {
            NONE = 1 << 0, ///< hard shadows
            PCF = 1 << 1, ///< Percentage close filtering
            BLUR = 1 << 2, ///< Gausian Blur
        };
    };

    struct SShadowMapEntry
    {
        SShadowMapEntry()
            : m_LightIndex(QT3DS_MAX_U32)
            , m_ShadowMapMode(ShadowMapModes::SSM)
            , m_ShadowFilterFlags(ShadowFilterValues::NONE)
        {
        }

        SShadowMapEntry(QT3DSU32 index, ShadowMapModes::Enum mode, ShadowFilterValues::Enum filter,
                        NVRenderTexture2D &depthMap, NVRenderTexture2D &depthCopy,
                        NVRenderTexture2D &depthTemp)
            : m_LightIndex(index)
            , m_ShadowMapMode(mode)
            , m_ShadowFilterFlags(filter)
            , m_DepthMap(depthMap)
            , m_DepthCopy(depthCopy)
            , m_DepthCube(NULL)
            , m_CubeCopy(NULL)
            , m_DepthRender(depthTemp)
        {
        }

        SShadowMapEntry(QT3DSU32 index, ShadowMapModes::Enum mode, ShadowFilterValues::Enum filter,
                        NVRenderTextureCube &depthCube, NVRenderTextureCube &cubeTmp,
                        NVRenderTexture2D &depthTemp)
            : m_LightIndex(index)
            , m_ShadowMapMode(mode)
            , m_ShadowFilterFlags(filter)
            , m_DepthMap(NULL)
            , m_DepthCopy(NULL)
            , m_DepthCube(depthCube)
            , m_CubeCopy(cubeTmp)
            , m_DepthRender(depthTemp)
        {
        }

        QT3DSU32 m_LightIndex; ///< the light index it belongs to
        ShadowMapModes::Enum m_ShadowMapMode; ///< shadow map method
        ShadowFilterValues::Enum m_ShadowFilterFlags; ///< shadow filter mode

        // PKC : Adding the DepthRender buffer allows us to have a depth+stencil format when filling
        // the shadow maps (depth+stencil is necessary), but use a more compact format for the
        // actual
        // shadow map used at shade time.  See if it's worth adding.
        NVScopedRefCounted<NVRenderTexture2D> m_DepthMap; ///< shadow map texture
        NVScopedRefCounted<NVRenderTexture2D>
            m_DepthCopy; ///< shadow map buffer used during blur passes
        NVScopedRefCounted<NVRenderTextureCube> m_DepthCube; ///< shadow cube map
        NVScopedRefCounted<NVRenderTextureCube>
            m_CubeCopy; ///< cube map buffer used during the blur passes
        NVScopedRefCounted<NVRenderTexture2D>
            m_DepthRender; ///< shadow depth+stencil map used during rendering

        QT3DSMat44 m_LightVP; ///< light view projection matrix
        QT3DSMat44 m_LightCubeView[6]; ///< light cubemap view matrices
        QT3DSMat44 m_LightView; ///< light view transform
    };

    class Qt3DSShadowMap : public NVRefCounted
    {
        typedef nvvector<SShadowMapEntry> TShadowMapEntryList;

    public:
        IQt3DSRenderContext &m_Context;
        volatile QT3DSI32 mRefCount;

    public:
        Qt3DSShadowMap(IQt3DSRenderContext &inContext);
        ~Qt3DSShadowMap();

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Context.GetAllocator())

        /*
         * @brief Add a shadow map entry
         *		  This creates a new shadow map if it does not exist or changed
         *
         * @param[in] index		shadow map entry index
         * @param[in] width		shadow map width
         * @param[in] height	shadow map height
         * @param[in] format	shadow map format
         * @param[in] samples	shadow map sample count
         * @param[in] mode		shadow map mode like SSM, VCM
         * @param[in] filter	soft shadow map mode filter like PCF
         *
         * @ return no return
         */
        void AddShadowMapEntry(QT3DSU32 index, QT3DSU32 width, QT3DSU32 height,
                               NVRenderTextureFormats::Enum format, QT3DSU32 samples,
                               ShadowMapModes::Enum mode, ShadowFilterValues::Enum filter);

        /*
         * @brief Get a shadow map entry
         *
         * @param[in] index		shadow map entry index
         *
         * @ return shadow map entry or NULL
         */
        SShadowMapEntry *GetShadowMapEntry(QT3DSU32 index);

        /*
         * @brief Get shadow map entry count
         *
         * @ return count of shadow map entries
         */
        QT3DSU32 GetShadowMapEntryCount() { return m_ShadowMapList.size(); }

        static Qt3DSShadowMap *Create(IQt3DSRenderContext &inContext);

    private:
        TShadowMapEntryList m_ShadowMapList; ///< List of shadow map entries
    };
}
}

#endif
