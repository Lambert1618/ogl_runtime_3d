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
#ifndef QT3DS_RENDER_GRAPH_OBJECT_PICK_QUERY_H
#define QT3DS_RENDER_GRAPH_OBJECT_PICK_QUERY_H

#include "Qt3DSRender.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSMat44.h"
#include "Qt3DSRenderImage.h"

namespace qt3ds {
namespace render {

    class IOffscreenRenderer;

    struct Qt3DSRenderPickSubResult
    {
        IOffscreenRenderer *m_SubRenderer;
        QT3DSMat44 m_TextureMatrix;
        NVRenderTextureCoordOp::Enum m_HorizontalTilingMode;
        NVRenderTextureCoordOp::Enum m_VerticalTilingMode;
        QT3DSU32 m_ViewportWidth;
        QT3DSU32 m_ViewportHeight;
        Qt3DSRenderPickSubResult *m_NextSibling;

        Qt3DSRenderPickSubResult()
            : m_SubRenderer(NULL)
            , m_NextSibling(NULL)
        {
        }
        Qt3DSRenderPickSubResult(IOffscreenRenderer &inSubRenderer, QT3DSMat44 inTextureMatrix,
                                NVRenderTextureCoordOp::Enum inHorizontalTilingMode,
                                NVRenderTextureCoordOp::Enum inVerticalTilingMode, QT3DSU32 width,
                                QT3DSU32 height)
            : m_SubRenderer(&inSubRenderer)
            , m_TextureMatrix(inTextureMatrix)
            , m_HorizontalTilingMode(inHorizontalTilingMode)
            , m_VerticalTilingMode(inVerticalTilingMode)
            , m_ViewportWidth(width)
            , m_ViewportHeight(height)
            , m_NextSibling(NULL)
        {
        }
    };

    struct Qt3DSRenderPickResult
    {
        const SGraphObject *m_HitObject;
        QT3DSF32 m_CameraDistanceSq;
        // The local coordinates in X,Y UV space where the hit occured
        QT3DSVec2 m_LocalUVCoords;
        // The local mouse coordinates will be the same on all of the sub objects.
        Qt3DSRenderPickSubResult *m_FirstSubObject;
        // The offscreen renderer that was used to render the scene graph this result was produced
        // from.
        IOffscreenRenderer *m_OffscreenRenderer;

        Qt3DSRenderPickResult(const SGraphObject &inHitObject, QT3DSF32 inCameraDistance,
                             const QT3DSVec2 &inLocalUVCoords)
            : m_HitObject(&inHitObject)
            , m_CameraDistanceSq(inCameraDistance)
            , m_LocalUVCoords(inLocalUVCoords)
            , m_FirstSubObject(NULL)
            , m_OffscreenRenderer(NULL)
        {
        }
        Qt3DSRenderPickResult()
            : m_HitObject(NULL)
            , m_CameraDistanceSq(QT3DS_MAX_F32)
            , m_LocalUVCoords(0, 0)
            , m_FirstSubObject(NULL)
            , m_OffscreenRenderer(NULL)
        {
        }
    };

    class IGraphObjectPickQuery
    {
    protected:
        virtual ~IGraphObjectPickQuery() {}

    public:
        // Implementors have the option of batching the results to allow fewer virtual calls
        // or returning one item each pick.
        // Results are guaranteed to be returned nearest to furthest
        // If the return value has size of zero then we assume nothing more can be picked and the
        // pick
        // is finished.
        virtual Qt3DSRenderPickResult Pick(const QT3DSVec2 &inMouseCoords,
                                          const QT3DSVec2 &inViewportDimensions,
                                          bool inPickEverything) = 0;
    };
}
}
#endif
