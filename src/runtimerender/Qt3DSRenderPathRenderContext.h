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
#ifndef QT3DS_RENDER_PATH_RENDER_CONTEXT_H
#define QT3DS_RENDER_PATH_RENDER_CONTEXT_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/StringTable.h"
#include "Qt3DSRenderShaderCache.h" //TShaderFeatureSet
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSBounds3.h"
#include "Qt3DSRenderShaderKeys.h"
#include "Qt3DSRenderableImage.h"

namespace qt3ds {
namespace render {

    struct SPathRenderContext
    {
        // The lights and camera will not change per layer,
        // so that information can be set once for all the shaders.
        NVConstDataRef<SLight *> m_Lights;
        const SCamera &m_Camera;

        // Per-object information.
        const SPath &m_Path;
        const QT3DSMat44 &m_ModelViewProjection;
        const QT3DSMat44 &m_ModelMatrix; ///< model to world transformation
        const QT3DSMat33 &m_NormalMatrix;

        QT3DSF32 m_Opacity;
        const SGraphObject &m_Material;
        SShaderDefaultMaterialKey m_MaterialKey;
        SRenderableImage *m_FirstImage;
        QT3DSVec2 m_CameraVec;

        bool m_EnableWireframe;
        bool m_HasTransparency;
        bool m_IsStroke;

        SPathRenderContext(NVConstDataRef<SLight *> lights, const SCamera &cam, const SPath &p,
                           const QT3DSMat44 &mvp, const QT3DSMat44 &world, const QT3DSMat33 &nm,
                           QT3DSF32 inOpacity, const SGraphObject &inMaterial,
                           SShaderDefaultMaterialKey inMaterialKey, SRenderableImage *inFirstImage,
                           bool inWireframe, QT3DSVec2 inCameraVec, bool inHasTransparency,
                           bool inIsStroke)

            : m_Lights(lights)
            , m_Camera(cam)
            , m_Path(p)
            , m_ModelViewProjection(mvp)
            , m_ModelMatrix(world)
            , m_NormalMatrix(nm)
            , m_Opacity(inOpacity)
            , m_Material(inMaterial)
            , m_MaterialKey(inMaterialKey)
            , m_FirstImage(inFirstImage)
            , m_CameraVec(inCameraVec)
            , m_EnableWireframe(inWireframe)
            , m_HasTransparency(inHasTransparency)
            , m_IsStroke(inIsStroke)
        {
        }
    };
}
}
#endif