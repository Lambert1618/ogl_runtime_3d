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
#ifndef QT3DS_RENDER_PATH_MANAGER_H
#define QT3DS_RENDER_PATH_MANAGER_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/StringTable.h"
#include "Qt3DSRenderShaderCache.h" //TShaderFeatureSet
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSBounds3.h"
//#include "Qt3DSRenderDefaultMaterialShaderGenerator.h" //SLayerGlobalRenderProperties

namespace qt3ds {
namespace render {

    struct SLayerGlobalRenderProperties;

    struct SPathAnchorPoint
    {
        QT3DSVec2 m_Position;
        QT3DSF32 m_IncomingAngle;
        QT3DSF32 m_OutgoingAngle;
        QT3DSF32 m_IncomingDistance;
        QT3DSF32 m_OutgoingDistance;
        SPathAnchorPoint() {}
        SPathAnchorPoint(QT3DSVec2 inPos, QT3DSF32 inAngle, QT3DSF32 outAngle, QT3DSF32 inDis, QT3DSF32 outDis)
            : m_Position(inPos)
            , m_IncomingAngle(inAngle)
            , m_OutgoingAngle(outAngle)
            , m_IncomingDistance(inDis)
            , m_OutgoingDistance(outDis)
        {
        }
    };

    class IPathManagerCore : public NVRefCounted
    {
    public:
        // returns the path buffer id
        //!! Note this call is made from multiple threads simultaneously during binary load.
        //!! - see UICRenderGraphObjectSerializer.cpp
        virtual void
        SetPathSubPathData(const SPathSubPath &inPathSubPath,
                           NVConstDataRef<SPathAnchorPoint> inPathSubPathAnchorPoints) = 0;

        virtual NVDataRef<SPathAnchorPoint>
        GetPathSubPathBuffer(const SPathSubPath &inPathSubPath) = 0;
        // Marks the PathSubPath anchor points as dirty.  This will mean rebuilding any PathSubPath
        // context required to render the PathSubPath.
        virtual NVDataRef<SPathAnchorPoint>
        ResizePathSubPathBuffer(const SPathSubPath &inPathSubPath, QT3DSU32 inNumAnchors) = 0;
        virtual NVBounds3 GetBounds(const SPath &inPath) = 0;

        // Helper functions used in various locations
        // Angles here are in degrees because that is how they are represented in the data.
        static QT3DSVec2 GetControlPointFromAngleDistance(QT3DSVec2 inPosition, float inAngle,
                                                       float inDistance);

        // Returns angle in x, distance in y.
        static QT3DSVec2 GetAngleDistanceFromControlPoint(QT3DSVec2 inPosition, QT3DSVec2 inControlPoint);

        virtual IPathManager &OnRenderSystemInitialize(IQt3DSRenderContext &context) = 0;

        static IPathManagerCore &CreatePathManagerCore(IQt3DSRenderContextCore &inContext);
    };

    struct SPathRenderContext; // UICRenderPathRenderContext.h

    class IPathManager : public IPathManagerCore
    {
    public:
        // The path segments are next expected to change after this call; changes will be ignored.
        virtual bool PrepareForRender(const SPath &inPath) = 0;

        virtual void RenderDepthPrepass(SPathRenderContext &inRenderContext,
                                        SLayerGlobalRenderProperties inRenderProperties,
                                        TShaderFeatureSet inFeatureSet) = 0;

        virtual void RenderShadowMapPass(SPathRenderContext &inRenderContext,
                                         SLayerGlobalRenderProperties inRenderProperties,
                                         TShaderFeatureSet inFeatureSet) = 0;

        virtual void RenderCubeFaceShadowPass(SPathRenderContext &inRenderContext,
                                              SLayerGlobalRenderProperties inRenderProperties,
                                              TShaderFeatureSet inFeatureSet) = 0;

        virtual void RenderPath(SPathRenderContext &inRenderContext,
                                SLayerGlobalRenderProperties inRenderProperties,
                                TShaderFeatureSet inFeatureSet) = 0;
    };
}
}
#endif