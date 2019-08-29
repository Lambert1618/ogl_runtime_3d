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
#ifndef QT3DS_RENDER_IMPL_RENDERABLE_OBJECTS_H
#define QT3DS_RENDER_IMPL_RENDERABLE_OBJECTS_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSFlags.h"
#include "Qt3DSRenderModel.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSRenderDefaultMaterial.h"
#include "Qt3DSRenderCustomMaterial.h"
#include "Qt3DSRenderText.h"
#include "Qt3DSRenderMesh.h"
#include "Qt3DSRenderShaderKeys.h"
#include "Qt3DSRenderShaderCache.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "Qt3DSRenderableImage.h"
#include "Qt3DSDistanceFieldRenderer.h"

namespace qt3ds {
namespace render {

    struct RenderPreparationResultFlagValues
    {
        enum Enum {
            HasTransparency = 1 << 0,
            CompletelyTransparent = 1 << 1,
            Dirty = 1 << 2,
            Pickable = 1 << 3,
            DefaultMaterialMeshSubset = 1 << 4,
            Text = 1 << 5,
            Custom = 1 << 6,
            CustomMaterialMeshSubset = 1 << 7,
            HasRefraction = 1 << 8,
            Path = 1 << 9,
            ShadowCaster = 1 << 10,
            DistanceField = 1 << 11,
            HasAlphaTest = 1 << 12,
        };
    };

    struct SRenderableObjectFlags : public NVFlags<RenderPreparationResultFlagValues::Enum, QT3DSU32>
    {
        void ClearOrSet(bool value, RenderPreparationResultFlagValues::Enum enumVal)
        {
            if (value)
                *this |= enumVal;
            else
                clear(enumVal);
        }

        void SetHasTransparency(bool inHasTransparency)
        {
            ClearOrSet(inHasTransparency, RenderPreparationResultFlagValues::HasTransparency);
        }
        bool HasTransparency() const
        {
            return *this & RenderPreparationResultFlagValues::HasTransparency;
        }
        bool HasRefraction() const
        {
            return *this & RenderPreparationResultFlagValues::HasRefraction;
        }
        void SetCompletelyTransparent(bool inTransparent)
        {
            ClearOrSet(inTransparent, RenderPreparationResultFlagValues::CompletelyTransparent);
        }
        bool IsCompletelyTransparent() const
        {
            return *this & RenderPreparationResultFlagValues::CompletelyTransparent;
        }
        void SetDirty(bool inDirty)
        {
            ClearOrSet(inDirty, RenderPreparationResultFlagValues::Dirty);
        }
        bool IsDirty() const { return *this & RenderPreparationResultFlagValues::Dirty; }
        void SetPickable(bool inPickable)
        {
            ClearOrSet(inPickable, RenderPreparationResultFlagValues::Pickable);
        }
        bool GetPickable() const
        {
            return *this & RenderPreparationResultFlagValues::Pickable;
        }

        // Mutually exclusive values
        void SetDefaultMaterialMeshSubset(bool inMeshSubset)
        {
            ClearOrSet(inMeshSubset, RenderPreparationResultFlagValues::DefaultMaterialMeshSubset);
        }
        bool IsDefaultMaterialMeshSubset() const
        {
            return *this & RenderPreparationResultFlagValues::DefaultMaterialMeshSubset;
        }

        void SetCustomMaterialMeshSubset(bool inMeshSubset)
        {
            ClearOrSet(inMeshSubset, RenderPreparationResultFlagValues::CustomMaterialMeshSubset);
        }
        bool IsCustomMaterialMeshSubset() const
        {
            return *this & RenderPreparationResultFlagValues::CustomMaterialMeshSubset;
        }

        void SetText(bool inText) { ClearOrSet(inText, RenderPreparationResultFlagValues::Text); }
        bool IsText() const { return *this & RenderPreparationResultFlagValues::Text; }

        void setDistanceField(bool inText)
        {
            ClearOrSet(inText, RenderPreparationResultFlagValues::DistanceField);
        }

        bool isDistanceField() const
        {
            return *this & RenderPreparationResultFlagValues::DistanceField;
        }

        void SetCustom(bool inCustom)
        {
            ClearOrSet(inCustom, RenderPreparationResultFlagValues::Custom);
        }
        bool IsCustom() const { return *this & RenderPreparationResultFlagValues::Custom; }

        void SetPath(bool inPath) { ClearOrSet(inPath, RenderPreparationResultFlagValues::Path); }
        bool IsPath() const { return *this & RenderPreparationResultFlagValues::Path; }

        void SetShadowCaster(bool inCaster)
        {
            ClearOrSet(inCaster, RenderPreparationResultFlagValues::ShadowCaster);
        }
        bool IsShadowCaster() const
        {
            return *this & RenderPreparationResultFlagValues::ShadowCaster;
        }
        void setAlphaTest(bool set)
        {
            ClearOrSet(set, RenderPreparationResultFlagValues::HasAlphaTest);
        }
        bool hasAlphaTest() const
        {
            return *this & RenderPreparationResultFlagValues::HasAlphaTest;
        }
    };

    struct SNodeLightEntry
    {
        SLight *m_Light;
        QT3DSU32 m_LightIndex;
        SNodeLightEntry *m_NextNode;
        SNodeLightEntry()
            : m_Light(NULL)
            , m_NextNode(NULL)
        {
        }
        SNodeLightEntry(SLight *inLight, QT3DSU32 inLightIndex)
            : m_Light(inLight)
            , m_LightIndex(inLightIndex)
            , m_NextNode(NULL)
        {
        }
    };

    DEFINE_INVASIVE_SINGLE_LIST(NodeLightEntry);

    IMPLEMENT_INVASIVE_SINGLE_LIST(NodeLightEntry, m_NextNode);

    struct SRenderableObject;

    typedef void (*TRenderFunction)(SRenderableObject &inObject, const QT3DSVec2 &inCameraProperties);

    struct SRenderableObject
    {
        // Variables used for picking
        const QT3DSMat44 &m_GlobalTransform;
        const NVBounds3 &m_Bounds;
        SRenderableObjectFlags m_RenderableFlags;
        // For rough sorting for transparency and for depth
        QT3DSVec3 m_WorldCenterPoint;
        QT3DSF32 m_CameraDistanceSq;
        TessModeValues::Enum m_TessellationMode;
        bool m_ShadowCaster;
        // For custom renderable objects the render function must be defined
        TRenderFunction m_RenderFunction;
        TNodeLightEntryList m_ScopedLights;
        SRenderableObject(SRenderableObjectFlags inFlags, QT3DSVec3 inWorldCenterPt,
                          const QT3DSMat44 &inGlobalTransform, const NVBounds3 &inBounds,
                          TessModeValues::Enum inTessMode = TessModeValues::NoTess,
                          bool inShadowCaster = true, TRenderFunction inFunction = nullptr)

            : m_GlobalTransform(inGlobalTransform)
            , m_Bounds(inBounds)
            , m_RenderableFlags(inFlags)
            , m_WorldCenterPoint(inWorldCenterPt)
            , m_CameraDistanceSq(0)
            , m_TessellationMode(inTessMode)
            , m_ShadowCaster(inShadowCaster)
            , m_RenderFunction(inFunction)
        {
        }
        bool operator<(SRenderableObject *inOther) const
        {
            return m_CameraDistanceSq < inOther->m_CameraDistanceSq;
        }
    };

    typedef nvvector<SRenderableObject *> TRenderableObjectList;

    // Different subsets from the same model will get the same
    // model context so we can generate the MVP and normal matrix once
    // and only once per subset.
    struct SModelContext
    {
        const SModel &m_Model;
        QT3DSMat44 m_ModelViewProjection;
        QT3DSMat33 m_NormalMatrix;

        SModelContext(const SModel &inModel, const QT3DSMat44 &inViewProjection)
            : m_Model(inModel)
        {
            m_Model.CalculateMVPAndNormalMatrix(inViewProjection, m_ModelViewProjection,
                                                m_NormalMatrix);
        }
        SModelContext(const SModelContext &inOther)
            : m_Model(inOther.m_Model)
        {
            // The default copy constructor for these objects is pretty darn slow.
            memCopy(&m_ModelViewProjection, &inOther.m_ModelViewProjection,
                    sizeof(m_ModelViewProjection));
            memCopy(&m_NormalMatrix, &inOther.m_NormalMatrix, sizeof(m_NormalMatrix));
        }
    };

    typedef nvvector<SModelContext *> TModelContextPtrList;

    class Qt3DSRendererImpl;
    struct SLayerRenderData;
    struct SShadowMapEntry;

    struct SSubsetRenderableBase : public SRenderableObject
    {
        Qt3DSRendererImpl &m_Generator;
        const SModelContext &m_ModelContext;
        SRenderSubset m_Subset;
        QT3DSF32 m_Opacity;

        SSubsetRenderableBase(SRenderableObjectFlags inFlags, QT3DSVec3 inWorldCenterPt,
                              Qt3DSRendererImpl &gen, const SRenderSubset &subset,
                              const SModelContext &modelContext, QT3DSF32 inOpacity)

            : SRenderableObject(inFlags, inWorldCenterPt, modelContext.m_Model.m_GlobalTransform,
                                m_Subset.m_Bounds)
            , m_Generator(gen)
            , m_ModelContext(modelContext)
            , m_Subset(subset)
            , m_Opacity(inOpacity)
        {
        }
        void RenderShadowMapPass(const QT3DSVec2 &inCameraVec, const SLight *inLight,
                                 const SCamera &inCamera, SShadowMapEntry *inShadowMapEntry);

        void RenderDepthPass(const QT3DSVec2 &inCameraVec, SRenderableImage *inDisplacementImage,
                             float inDisplacementAmount);
    };

    /**
     *	A renderable that corresponds to a subset (a part of a model).
     *	These are created per subset per layer and are responsible for actually
     *	rendering this type of object.
     */
    struct SSubsetRenderable : public SSubsetRenderableBase
    {
        const SDefaultMaterial &m_Material;
        SRenderableImage *m_FirstImage;
        SShaderDefaultMaterialKey m_ShaderDescription;
        NVConstDataRef<QT3DSMat44> m_Bones;

        SSubsetRenderable(SRenderableObjectFlags inFlags, QT3DSVec3 inWorldCenterPt,
                          Qt3DSRendererImpl &gen, const SRenderSubset &subset,
                          const SDefaultMaterial &mat, const SModelContext &modelContext,
                          QT3DSF32 inOpacity, SRenderableImage *inFirstImage,
                          SShaderDefaultMaterialKey inShaderKey,
                          NVConstDataRef<QT3DSMat44> inBoneGlobals)

            : SSubsetRenderableBase(inFlags, inWorldCenterPt, gen, subset, modelContext, inOpacity)
            , m_Material(mat)
            , m_FirstImage(inFirstImage)
            , m_ShaderDescription(inShaderKey)
            , m_Bones(inBoneGlobals)
        {
            m_RenderableFlags.SetDefaultMaterialMeshSubset(true);
            m_RenderableFlags.SetCustom(false);
            m_RenderableFlags.SetText(false);
            m_RenderableFlags.setDistanceField(false);
        }

        void Render(const QT3DSVec2 &inCameraVec, TShaderFeatureSet inFeatureSet, bool depth);
        void RenderShadow(const QT3DSVec2 &inCameraVec, TShaderFeatureSet inFeatureSet,
                          const SLight *inLight, const SCamera &inCamera,
                          SShadowMapEntry *inShadowMapEntry);
        void RenderDepthPass(const QT3DSVec2 &inCameraVec);

        DefaultMaterialBlendMode::Enum getBlendingMode()
        {
            return m_Material.m_BlendMode;
        }
    };

    struct SCustomMaterialRenderable : public SSubsetRenderableBase
    {
        const SCustomMaterial &m_Material;
        SRenderableImage *m_FirstImage;
        SShaderDefaultMaterialKey m_ShaderDescription;

        SCustomMaterialRenderable(SRenderableObjectFlags inFlags, QT3DSVec3 inWorldCenterPt,
                                  Qt3DSRendererImpl &gen, const SRenderSubset &subset,
                                  const SCustomMaterial &mat, const SModelContext &modelContext,
                                  QT3DSF32 inOpacity, SRenderableImage *inFirstImage,
                                  SShaderDefaultMaterialKey inShaderKey)
            : SSubsetRenderableBase(inFlags, inWorldCenterPt, gen, subset, modelContext, inOpacity)
            , m_Material(mat)
            , m_FirstImage(inFirstImage)
            , m_ShaderDescription(inShaderKey)
        {
            m_RenderableFlags.SetCustomMaterialMeshSubset(true);
        }

        void Render(const QT3DSVec2 &inCameraVec, const SLayerRenderData &inLayerData,
                    const SLayer &inLayer, NVDataRef<SLight *> inLights, const SCamera &inCamera,
                    const NVRenderTexture2D *inDepthTexture, const NVRenderTexture2D *inSsaoTexture,
                    TShaderFeatureSet inFeatureSet);

        void RenderDepthPass(const QT3DSVec2 &inCameraVec, const SLayer &inLayer,
                             NVConstDataRef<SLight *> inLights, const SCamera &inCamera,
                             const NVRenderTexture2D *inDepthTexture);
    };

    struct STextScaleAndOffset
    {
        QT3DSVec2 m_TextOffset;
        QT3DSVec2 m_TextScale;
        STextScaleAndOffset(const QT3DSVec2 &inTextOffset, const QT3DSVec2 &inTextScale)
            : m_TextOffset(inTextOffset)
            , m_TextScale(inTextScale)
        {
        }
        STextScaleAndOffset(NVRenderTexture2D &inTexture, const STextTextureDetails &inTextDetails,
                            const STextRenderInfo &inInfo);
    };

    struct STextRenderable : public SRenderableObject, public STextScaleAndOffset
    {
        Qt3DSRendererImpl &m_Generator;
        const SText &m_Text;
        NVRenderTexture2D &m_Texture;
        QT3DSMat44 m_ModelViewProjection;
        QT3DSMat44 m_ViewProjection;

        STextRenderable(SRenderableObjectFlags inFlags, QT3DSVec3 inWorldCenterPt,
                        Qt3DSRendererImpl &gen, const SText &inText, const NVBounds3 &inBounds,
                        const QT3DSMat44 &inModelViewProjection, const QT3DSMat44 &inViewProjection,
                        NVRenderTexture2D &inTextTexture, const QT3DSVec2 &inTextOffset,
                        const QT3DSVec2 &inTextScale)
            : SRenderableObject(inFlags, inWorldCenterPt, inText.m_GlobalTransform, inBounds)
            , STextScaleAndOffset(inTextOffset, inTextScale)
            , m_Generator(gen)
            , m_Text(inText)
            , m_Texture(inTextTexture)
            , m_ModelViewProjection(inModelViewProjection)
            , m_ViewProjection(inViewProjection)
        {
            m_RenderableFlags.SetDefaultMaterialMeshSubset(false);
            m_RenderableFlags.SetCustom(false);
            m_RenderableFlags.SetText(true);
            m_RenderableFlags.setDistanceField(false);
        }

        void Render(const QT3DSVec2 &inCameraVec);
        void RenderDepthPass(const QT3DSVec2 &inCameraVec);
    };

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
    struct SDistanceFieldRenderable : public SRenderableObject
    {
        Q3DSDistanceFieldRenderer &m_distanceFieldText;
        QT3DSMat44 m_mvp;
        SText &m_text;

        SDistanceFieldRenderable(SRenderableObjectFlags flags, QT3DSVec3 worldCenterPt,
                                 SText &text, const NVBounds3 &bounds, const QT3DSMat44 &mvp,
                                 Q3DSDistanceFieldRenderer &distanceFieldText)
            : SRenderableObject(flags, worldCenterPt, text.m_GlobalTransform, bounds)
            , m_distanceFieldText(distanceFieldText)
            , m_mvp(mvp)
            , m_text(text)
        {
            m_RenderableFlags.SetDefaultMaterialMeshSubset(false);
            m_RenderableFlags.SetCustom(false);
            m_RenderableFlags.SetText(false);
            m_RenderableFlags.setDistanceField(true);
            m_distanceFieldText.checkAndAddRenderedTexts(text);
        }

        void Render(const QT3DSVec2 &inCameraVec);
        void RenderDepthPass(const QT3DSVec2 &inCameraVec);
    };
#endif

    struct SPathRenderable : public SRenderableObject
    {
        Qt3DSRendererImpl &m_Generator;
        SPath &m_Path;
        NVBounds3 m_Bounds;
        QT3DSMat44 m_ModelViewProjection;
        QT3DSMat33 m_NormalMatrix;
        const SGraphObject &m_Material;
        QT3DSF32 m_Opacity;
        SRenderableImage *m_FirstImage;
        SShaderDefaultMaterialKey m_ShaderDescription;
        bool m_IsStroke;

        SPathRenderable(SRenderableObjectFlags inFlags, QT3DSVec3 inWorldCenterPt,
                        Qt3DSRendererImpl &gen, const QT3DSMat44 &inGlobalTransform,
                        NVBounds3 &inBounds, SPath &inPath, const QT3DSMat44 &inModelViewProjection,
                        const QT3DSMat33 inNormalMat, const SGraphObject &inMaterial, QT3DSF32 inOpacity,
                        SShaderDefaultMaterialKey inShaderKey, bool inIsStroke)

            : SRenderableObject(inFlags, inWorldCenterPt, inGlobalTransform, m_Bounds)
            , m_Generator(gen)
            , m_Path(inPath)
            , m_Bounds(inBounds)
            , m_ModelViewProjection(inModelViewProjection)
            , m_NormalMatrix(inNormalMat)
            , m_Material(inMaterial)
            , m_Opacity(inOpacity)
            , m_FirstImage(NULL)
            , m_ShaderDescription(inShaderKey)
            , m_IsStroke(inIsStroke)
        {
            m_RenderableFlags.SetPath(true);
        }
        void Render(const QT3DSVec2 &inCameraVec, const SLayer &inLayer,
                    NVConstDataRef<SLight *> inLights, const SCamera &inCamera,
                    const NVRenderTexture2D *inDepthTexture, const NVRenderTexture2D *inSsaoTexture,
                    TShaderFeatureSet inFeatureSet);

        void RenderDepthPass(const QT3DSVec2 &inCameraVec, const SLayer &inLayer,
                             NVConstDataRef<SLight *> inLights, const SCamera &inCamera,
                             const NVRenderTexture2D *inDepthTexture);

        void RenderShadowMapPass(const QT3DSVec2 &inCameraVec, const SLight *inLight,
                                 const SCamera &inCamera, SShadowMapEntry *inShadowMapEntry);
    };
}
}

#endif
