/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

#include "EnginePrefix.h"
#include "Qt3DSRenderRuntimeBindingImpl.h"
#include "Qt3DSSceneManager.h"
#include "Qt3DSIScene.h"
#include "Qt3DSRuntimeView.h"
#include "Qt3DSQmlEngine.h"
#include "Qt3DSRenderUIPLoader.h"
#include "Qt3DSPresentationFrameData.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "Qt3DSRenderSubpresentation.h"
#include "Qt3DSIScriptBridge.h"
#include "Qt3DSFileStream.h"
#include "Qt3DSDMPrefix.h"
#include "foundation/IOStreams.h"
#include "Qt3DSDMStringTable.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSRenderBufferManager.h"
#include "foundation/SerializationTypes.h"
#include "Qt3DSRenderGraphObjectSerializer.h"
#include "Qt3DSRenderShaderCache.h"
#include "Qt3DSRenderImageBatchLoader.h"
#include "Qt3DSPresentation.h"

#include "Qt3DSDLLManager.h"
#include "Qt3DSBasicPluginDLL.h"
#include "Qt3DSPluginDLL.h"
#include "Qt3DSRenderPlugin.h"
#include "foundation/FileTools.h"
#include "Qt3DSStateVisualBindingContextCommands.h"
#include "Qt3DSStateScriptContext.h"
#include "EventPollingSystem.h"
#include "EventSystem.h"
#include "Qt3DSApplication.h"
#include "Qt3DSMatrix.h"
#include "Qt3DSWindowSystem.h"
#if !defined (Q_OS_MACOS)
#include "Qt3DSEGLInfo.h"
#endif
#include "Qt3DSOffscreenRenderKey.h"
#include "Qt3DSOldNBustedRenderPlugin.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "Qt3DSRenderBufferLoader.h"
#include "Qt3DSRenderRenderList.h"
#include "Qt3DSRenderPrefilterTexture.h"
#include "foundation/PoolingAllocator.h"
#include "q3dsqmlrender.h"
#include "Qt3DSHash.h"

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(disable : 4355)
#endif

using namespace qt3ds::render;
using eastl::make_pair;
using eastl::pair;
using qt3ds::runtime::IApplication;

#ifndef _WIN32
#define stricmp strcasecmp
#endif
namespace qt3ds {
namespace render {
    qt3ds::foundation::MallocAllocator g_BaseAllocator;
}
}
namespace {
struct Qt3DSRenderScene;

struct Qt3DSRenderSceneSubPresRenderer : public CSubPresentationRenderer
{
    Qt3DSRenderScene &m_Scene;
    Qt3DSRenderSceneSubPresRenderer(Qt3DSRenderScene &inScene, IQt3DSRenderContext &inRenderContext,
                                  SPresentation &inPresentation)
        : CSubPresentationRenderer(inRenderContext, inPresentation)
        , m_Scene(inScene)
    {
    }
    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_RenderContext.GetAllocator())
    SOffscreenRenderFlags NeedsRender(const SOffscreenRendererEnvironment &inEnvironment,
                                      QT3DSVec2 inPresScale,
                                      const SRenderInstanceId instanceId) override;
    void Render(const SOffscreenRendererEnvironment &inEnvironment,
                NVRenderContext &inRenderContext, QT3DSVec2 inPresScale,
                SScene::RenderClearCommand inClearBuffer,
                const SRenderInstanceId instanceId) override;
    void RenderWithClear(const SOffscreenRendererEnvironment &inEnvironment,
                         NVRenderContext &inRenderContext, QT3DSVec2 inPresScale,
                         SScene::RenderClearCommand inClearBuffer, QT3DSVec4 inClearColor,
                         const SRenderInstanceId instanceId) override;
};

struct SSceneLoadData
{
    NVAllocatorCallback &m_Allocator;
    NVScopedRefCounted<ILoadedBuffer> m_Data;
    SPresentation *m_Presentation;
    NVDataRef<QT3DSU8> m_TranslatorData;
    NVDataRef<QT3DSU8> m_SceneGraphData;
    eastl::vector<Qt3DSTranslator *> m_Translators;
    SPoolingAllocator m_AutoAllocator;
    Q3DStudio::IPresentation *m_RuntimePresentation;
    QT3DSI32 mRefCount;

    SSceneLoadData(NVAllocatorCallback &alloc)
        : m_Allocator(alloc)
        , m_Presentation(nullptr)
        , m_AutoAllocator(alloc)
        , m_RuntimePresentation(nullptr)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator)
};

struct Qt3DSRenderScene : public Q3DStudio::IScene
{
    SBindingCore &m_BindingCore;
    NVScopedRefCounted<IQt3DSRenderContext> m_Context;
    NVScopedRefCounted<SSceneLoadData> m_LoadData;
    // The scene graph gets allocated kind of on its own
    // So do the smaller translation objects that translate TElement properties
    // into the graph.
    SPresentation *m_Presentation;
    Q3DStudio::IPresentation *m_RuntimePresentation;
    void *m_UserData;
    TTranslatorDirytSet m_DirtySet;
    CRegisteredString m_OffscreenRendererId;
    IOffscreenRenderer *m_OffscreenRenderer;
    CRegisteredString m_SubPresentationType;
    nvvector<SGraphObject *> m_GraphObjectList;
    bool m_LoggedPickLastFrame;
    NVRenderRect m_LastRenderViewport;
    CRegisteredString m_PathSubPathType;

    Qt3DSRenderScene(SBindingCore &inBindingCore, IQt3DSRenderContext &inContext,
                    SSceneLoadData &inLoadData)
        : m_BindingCore(inBindingCore)
        , m_Context(inContext)
        , m_LoadData(inLoadData)
        , m_Presentation(inLoadData.m_Presentation)
        , m_RuntimePresentation(inLoadData.m_RuntimePresentation)
        , m_UserData(nullptr)
        , m_DirtySet(inContext.GetAllocator(), "Qt3DSRenderScene::m_DirtySet")
        , m_OffscreenRenderer(nullptr)
        , m_SubPresentationType(
              inContext.GetStringTable().RegisterStr(CSubPresentationRenderer::GetRendererName()))
        , m_GraphObjectList(inContext.GetAllocator(), "Qt3DSDSRenderScene::m_GraphObjectList")
        , m_LoggedPickLastFrame(false)
    {
        for (QT3DSU32 idx = 0, end = inLoadData.m_Translators.size(); idx < end; ++idx) {
            m_DirtySet.insert(*inLoadData.m_Translators[idx]);
        }
        m_PathSubPathType = inContext.GetStringTable().RegisterStr("PathAnchorPoint");
    }

    virtual ~Qt3DSRenderScene() override
    {
        if (m_OffscreenRenderer)
            m_Context->GetOffscreenRenderManager().ReleaseOffscreenRenderer(m_OffscreenRendererId);
        m_OffscreenRenderer = nullptr;
        if (m_Presentation && m_Presentation->m_Scene) {
            for (SLayer *theLayer = m_Presentation->m_Scene->m_FirstChild; theLayer;
                 theLayer = static_cast<SLayer *>(theLayer->m_NextSibling)) {
                m_Context->GetRenderer().ReleaseLayerRenderResources(*theLayer, nullptr);
            }
        }
    }

    qt3ds::NVAllocatorCallback &allocator() override { return m_LoadData->m_AutoAllocator; }
    Q3DStudio::IPresentation &GetPresentation() override { return *m_RuntimePresentation; }

    bool preferKtx() const override
    {
        return m_Presentation->m_preferKTX;
    }

    // Update really just adds objects to the dirty set
    bool Update()
    {
        Q3DStudio::TElementList &theDirtyList =
            m_RuntimePresentation->GetFrameData().GetDirtyList();
        for (int idx = 0, end = theDirtyList.GetCount(); idx < end; ++idx) {
            Q3DStudio::TElement &theElement = *theDirtyList[idx];
            Qt3DSTranslator *theTranslator =
                reinterpret_cast<Qt3DSTranslator *>(theElement.GetAssociation());
            if (!theTranslator && theElement.GetType() == m_PathSubPathType) {
                Q3DStudio::TElement *theParent = theElement.GetParent();
                if (theParent) {
                    theTranslator = reinterpret_cast<Qt3DSTranslator *>(theParent->GetAssociation());
                    // The path translator responds to anchor point changes as well as its own data
                    // changes.
                    if (theTranslator
                        && theTranslator->RenderObject().m_Type != GraphObjectTypes::PathSubPath)
                        theTranslator = nullptr;
                }
            }
            if (theTranslator)
                m_DirtySet.insert(*theTranslator);
        }
        return m_DirtySet.size() > 0;
    }

    void GetActiveSubPresentations(QVector<qt3ds::foundation::CRegisteredString> &subs) override
    {
        using namespace Q3DStudio;
        Q3DStudio::TElementList &theDirtyList
                = m_RuntimePresentation->GetFrameData().GetDirtyList();
        for (int idx = 0, end = theDirtyList.GetCount(); idx < end; ++idx) {
            Q3DStudio::TElement &theElement = *theDirtyList[idx];
            Qt3DSTranslator *theTranslator
                    = reinterpret_cast<Qt3DSTranslator *>(theElement.GetAssociation());
            if (theTranslator && theTranslator->GetUIPType() == GraphObjectTypes::Layer) {
                Q3DStudio::UVariant value;
                if (theElement.GetAttribute(ATTRIBUTE_SOURCEPATH, value))
                    subs.push_back(m_Context->GetStringTable().HandleToStr(value.m_StringHandle));
            } else if (theTranslator && theTranslator->GetUIPType() == GraphObjectTypes::Image) {
                Q3DStudio::UVariant value;
                if (theElement.GetAttribute(ATTRIBUTE_SUBPRESENTATION, value))
                    subs.push_back(m_Context->GetStringTable().HandleToStr(value.m_StringHandle));
            }
        }
    }

    void TransferDirtyProperties()
    {
        if (m_Presentation) {
            for (QT3DSU32 idx = 0, end = m_DirtySet.size(); idx < end; ++idx) {
                QT3DS_ASSERT(m_DirtySet.size() == end);
                m_DirtySet[idx]->OnElementChanged(*m_Presentation, *m_Context,
                                                  *m_RuntimePresentation);
            }
            m_DirtySet.clear();
        }
    }

    bool PrepareForRender()
    {
        TransferDirtyProperties();
        m_LastRenderViewport = m_Context->GetRenderList().GetViewport();
        if (m_Presentation && m_Presentation->m_Scene) {
            NVRenderRect theViewportSize(m_LastRenderViewport);
            return m_Presentation->m_Scene->PrepareForRender(
                QT3DSVec2(QT3DSF32(theViewportSize.m_Width), QT3DSF32(theViewportSize.m_Height)),
                *m_Context);
        }
        return false;
    }

    void Render()
    {
        if (m_Presentation && m_Presentation->m_Scene) {
            NVRenderRect theViewportSize(m_LastRenderViewport);
            m_Presentation->m_Scene->Render(
                QT3DSVec2(QT3DSF32(theViewportSize.m_Width), QT3DSF32(theViewportSize.m_Height)),
                        *m_Context, SScene::DoNotClear);
        }
    }

    // Note that we do not need to call WindowToPresentation on the mouse coordinates because they
    // are specifically
    // supposed to be the return values from getMousePosition which applies that transformation.  We
    // do, however need
    // to reverse part of this transformation; whatever part happens after
    // m_Context->GetMousePickMouseCoords
    Q3DStudio::TElement *UserPick(float mouseX, float mouseY)
    {
        // Note that the pick code below only calls GetMousePickMouseCoords
        // while windowToPresentation subtracts the window positional offset from
        // the mouse position.
        // Thus we have to add it back.
        QT3DSVec2 mousePos(mouseX + m_LastRenderViewport.m_X, mouseY + m_LastRenderViewport.m_Y);

        Qt3DSRenderPickResult thePickResult = m_Context->GetRenderer().Pick(
            *m_Presentation->m_Scene->m_FirstChild, m_Context->GetMousePickViewport()
            // GetMousePickMouseCoords is called by the renderer to setup the pick frame.
            // This is so that the presentation's lastMouseX and lastMouseY variables are correctly
            // setup.
            ,
            mousePos, true, true);

        if (thePickResult.m_HitObject != nullptr) {
            SModel *theHitModel =
                static_cast<SModel *>(const_cast<SGraphObject *>(thePickResult.m_HitObject));
            return &Qt3DSTranslator::GetTranslatorFromGraphNode(*theHitModel)->Element();
        }
        return nullptr;
    }

    virtual Option<QT3DSVec2> FacePosition(Q3DStudio::TElement &inElement, float mouseX, float mouseY,
                                        NVDataRef<Q3DStudio::TElement *> inMapperElements,
                                        Q3DStudio::FacePositionPlanes::Enum inPlane)
    {
        if (inElement.GetBelongedPresentation() != this->m_RuntimePresentation
            && inMapperElements.size() == 0) {
            return Empty();
        }
        Qt3DSTranslator *theTranslator =
            reinterpret_cast<Qt3DSTranslator *>(inElement.GetAssociation());
        if (theTranslator == nullptr)
            return Empty();
        bool isValidPickObject =
            GraphObjectTypes::IsNodeType(theTranslator->m_RenderObject->m_Type);
        if (isValidPickObject == false)
            return Empty();
        SNode &theNode = static_cast<SNode &>(*theTranslator->m_RenderObject);
        NVBounds3 theBounds = GetNodeLocalBoundingBox(&inElement, false);
        // See commens in UserPick
        QT3DSVec2 mousePos(mouseX + m_LastRenderViewport.m_X, mouseY + m_LastRenderViewport.m_Y);
        eastl::vector<SGraphObject *> theMapperObjects(inMapperElements.size());
        for (QT3DSU32 idx = 0, end = inMapperElements.size(); idx < end; ++idx) {
            Qt3DSTranslator *theMapperTranslator =
                reinterpret_cast<Qt3DSTranslator *>(inMapperElements[idx]->GetAssociation());
            SGraphObject *theMapperObject = nullptr;
            if (theMapperTranslator) {
                theMapperObject = theMapperTranslator->m_RenderObject;
            }
            if (theMapperObject == nullptr) {
                QT3DS_ASSERT(false);
            } else {
                theMapperObjects[idx] = theMapperObject;
            }
        }
        qt3ds::render::SBasisPlanes::Enum thePlane(qt3ds::render::SBasisPlanes::XY);
        switch (inPlane) {
        case Q3DStudio::FacePositionPlanes::XY:
            thePlane = qt3ds::render::SBasisPlanes::XY;
            break;
        case Q3DStudio::FacePositionPlanes::YZ:
            thePlane = qt3ds::render::SBasisPlanes::YZ;
            break;
        case Q3DStudio::FacePositionPlanes::XZ:
            thePlane = qt3ds::render::SBasisPlanes::XZ;
            break;
        }
        if (theBounds.isEmpty() == false) {
            return m_Context->GetRenderer().FacePosition(
                theNode, theBounds, theNode.m_GlobalTransform, m_Context->GetMousePickViewport(),
                mousePos, NVDataRef<SGraphObject *>(theMapperObjects.data(),
                                                    QT3DSU32(theMapperObjects.size())), thePlane);
        }
        return Empty();
    }

    void Pick(Q3DStudio::SPickFrame &ioPickFrame)
    {
        // Presentation's m_Hide can disable rendering
        bool wasPickLoggedLastFrame = m_LoggedPickLastFrame;
        m_LoggedPickLastFrame = false;
        if (ioPickFrame.m_InputFrame.m_PickValid) {
            // If we have not already found a valid pick on a previous layer
            if (!ioPickFrame.m_ResultValid && m_Presentation && m_Presentation->m_Scene
                && m_Presentation->m_Scene->m_FirstChild) {
                Qt3DSRenderPickResult thePickResult = m_Context->GetRenderer().Pick(
                    *m_Presentation->m_Scene->m_FirstChild, m_Context->GetMousePickViewport()
                    // GetMousePickMouseCoords is called by the renderer to setup the pick frame.
                    // This is so that the presentation's lastMouseX and lastMouseY variables are
                    // correctly setup.
                    ,
                    m_Context->GetMousePickMouseCoords(
                        QT3DSVec2(ioPickFrame.m_InputFrame.m_PickX, ioPickFrame.m_InputFrame.m_PickY)),
                    true);
                if (thePickResult.m_HitObject != nullptr) {
                    SModel *theHitModel = static_cast<SModel *>(
                        const_cast<SGraphObject *>(thePickResult.m_HitObject));
                    ioPickFrame.m_Model =
                        &Qt3DSTranslator::GetTranslatorFromGraphNode(*theHitModel)->Element();
                    // I don't think local hit is used any more, but not sure.  If they are used,
                    // then the code below is probably wrong.
                    ioPickFrame.m_LocalHit[0] = thePickResult.m_LocalUVCoords.x;
                    ioPickFrame.m_LocalHit[1] = thePickResult.m_LocalUVCoords.y;
                    ioPickFrame.m_SquaredDistance = thePickResult.m_CameraDistanceSq;
                    ioPickFrame.m_ResultValid = true;
                    if (wasPickLoggedLastFrame == false) {
                        m_LoggedPickLastFrame = true;
                        Q3DStudio::IPresentation *thePresentation =
                            ioPickFrame.m_Model->GetBelongedPresentation();
                        IApplication &theRuntime = thePresentation->GetApplication();
                        // We are picking against the previous frame of information.
                        qCInfo(TRACE_INFO, "Model Picked: %s on frame %d",
                            theHitModel->m_Id.c_str(),
                            theRuntime.GetFrameCount() - 1);
                    }
                } else {
                    // The scene is always picked if it is pickable; nothing else really makes
                    // sense.
                    Qt3DSTranslator *theTranslator =
                        Qt3DSTranslator::GetTranslatorFromGraphNode(*m_Presentation->m_Scene);

                    if (theTranslator) {
                        ioPickFrame.m_Model = &theTranslator->Element();
                        // I don't think local hit is used any more, but not sure.  If they are
                        // used, then the code below is probably wrong.
                        ioPickFrame.m_LocalHit[0] = ioPickFrame.m_InputFrame.m_PickX
                            / m_Presentation->m_PresentationDimensions.x;
                        ioPickFrame.m_LocalHit[1] = 1.0f
                            - ioPickFrame.m_InputFrame.m_PickY
                                / m_Presentation->m_PresentationDimensions.y;
                        ioPickFrame.m_SquaredDistance = 0;
                        ioPickFrame.m_ResultValid = true;
                    }
                }
            }
        }
    }

    void SetUserData(void *inUserData) override { m_UserData = inUserData; }
    void *GetUserData() override { return m_UserData; }

    // Unfortunately, you should expect the node to be dirty at this point so we may need to force
    // an update
    void CalculateGlobalTransform(Q3DStudio::TElement *inElement,
                                          Q3DStudio::RuntimeMatrix &outTransform) override
    {
        if (inElement == nullptr) {
            QT3DS_ASSERT(false);
            return;
        }
        Qt3DSTranslator *theTranslator =
            reinterpret_cast<Qt3DSTranslator *>(inElement->GetAssociation());
        if (theTranslator && GraphObjectTypes::IsNodeType(theTranslator->GetUIPType())) {
            Update();
            TransferDirtyProperties();
            SNode *theNode = static_cast<SNode *>(&theTranslator->RenderObject());
            // Ensure the node stays dirty
            bool wasDirty = theNode->m_Flags.IsDirty();
            theNode->CalculateGlobalVariables();
            theNode->m_Flags.SetDirty(wasDirty);
            memCopy(outTransform.m_Data, theNode->m_GlobalTransform.front(), sizeof(QT3DSMat44));
        } else {
            qCCritical(INVALID_OPERATION, "Calculate global transform called on invalide object");
            QT3DS_ASSERT(false);
        }
    }

    void SetLocalTransformMatrix(Q3DStudio::TElement *inElement,
                                         const Q3DStudio::RuntimeMatrix &inTransform) override
    {
        if (inElement == nullptr) {
            QT3DS_ASSERT(false);
            return;
        }

        Qt3DSTranslator *theTranslator =
            reinterpret_cast<Qt3DSTranslator *>(inElement->GetAssociation());
        if (theTranslator && GraphObjectTypes::IsNodeType(theTranslator->GetUIPType())) {
            SNode *theNode = static_cast<SNode *>(&theTranslator->RenderObject());
            QT3DSMat44 transform;
            memCopy(transform.front(), inTransform.m_Data, sizeof(QT3DSMat44));
            theNode->SetLocalTransformFromMatrix(transform);
            theNode->MarkDirty(NodeTransformDirtyFlag::TransformIsDirty);
        }
    }

    void EnsureNodeIsUpToDate(SNode &inNode, bool inIncludeChildren)
    {
        bool wasDirty = inNode.m_Flags.IsDirty();
        if (wasDirty) {
            inNode.CalculateGlobalVariables();
            inNode.m_Flags.SetDirty(wasDirty);
            if (inIncludeChildren) {
                for (SNode *theChild = inNode.m_FirstChild; theChild;
                     theChild = theChild->m_NextSibling)
                    EnsureNodeIsUpToDate(*theChild, inIncludeChildren);
            }
        }
    }

    NVBounds3 GetNodeLocalBoundingBox(Q3DStudio::TElement *inElement, bool inSelfOnly)
    {
        NVBounds3 retval(NVBounds3::empty());
        if (inElement == nullptr) {
            QT3DS_ASSERT(false);
            return retval;
        }
        Qt3DSTranslator *theTranslator =
            reinterpret_cast<Qt3DSTranslator *>(inElement->GetAssociation());
        if (theTranslator && GraphObjectTypes::IsNodeType(theTranslator->GetUIPType())) {
            Update();
            TransferDirtyProperties();
            SNode *theNode = static_cast<SNode *>(&theTranslator->RenderObject());
            bool theIncludeChildren = !inSelfOnly;
            EnsureNodeIsUpToDate(*theNode, theIncludeChildren);
            IBufferManager &theBufferManager(m_Context->GetBufferManager());
            return theNode->GetBounds(theBufferManager, m_Context->GetPathManager(),
                                      theIncludeChildren);
        } else {
            qCCritical(INVALID_OPERATION, "GetBoundingBox called on invalid object");
            QT3DS_ASSERT(false);
        }
        return retval;
    }

    static void Assign(Q3DStudio::CBoundingBox &lhs, NVBounds3 &rhs)
    {
        lhs.m_Min.m_X = rhs.minimum.x;
        lhs.m_Min.m_Y = rhs.minimum.y;
        lhs.m_Min.m_Z = rhs.minimum.z;

        lhs.m_Max.m_X = rhs.maximum.x;
        lhs.m_Max.m_Y = rhs.maximum.y;
        lhs.m_Max.m_Z = rhs.maximum.z;
    }

    Q3DStudio::CBoundingBox GetBoundingBox(Q3DStudio::TElement *inElement, bool inSelfOnly) override
    {
        Q3DStudio::CBoundingBox retval;
        retval.SetEmpty();
        NVBounds3 theLocalBox = GetNodeLocalBoundingBox(inElement, inSelfOnly);
        if (theLocalBox.isEmpty() == false) {
            Qt3DSTranslator *theTranslator =
                reinterpret_cast<Qt3DSTranslator *>(inElement->GetAssociation());
            if (theTranslator && GraphObjectTypes::IsNodeType(theTranslator->GetUIPType())) {
                SNode *theNode = static_cast<SNode *>(&theTranslator->RenderObject());
                theLocalBox.transform(theNode->m_GlobalTransform);
                Assign(retval, theLocalBox);
                // Left handed nodes need to return values in their space, not in the global space
                // This is a hack fix due to a bug.
                if (theNode->m_Flags.IsLeftHanded()) {
                    eastl::swap(retval.m_Min.m_Z, retval.m_Max.m_Z);
                    retval.m_Min.m_Z *= -1;
                    retval.m_Max.m_Z *= -1;
                }
            }
        }
        return retval;
    }

    Q3DStudio::CBoundingBox GetLocalBoundingBox(Q3DStudio::TElement *inElement,
                                                        bool inSelfOnly) override
    {
        Q3DStudio::CBoundingBox retval;
        retval.SetEmpty();
        if (inElement == nullptr) {
            QT3DS_ASSERT(false);
            return retval;
        }
        NVBounds3 theLocalBounds = GetNodeLocalBoundingBox(inElement, inSelfOnly);
        if (theLocalBounds.isEmpty() == false)
            Assign(retval, theLocalBounds);

        return retval;
    }

    Q3DStudio::SCameraRect GetCameraBounds(Q3DStudio::TElement &inElem) override
    {
        Qt3DSTranslator *theTranslator = reinterpret_cast<Qt3DSTranslator *>(inElem.GetAssociation());
        if (theTranslator && theTranslator->m_RenderObject) {
            Option<qt3ds::render::SCuboidRect> theRectOpt =
                m_Context->GetRenderer().GetCameraBounds(*theTranslator->m_RenderObject);
            if (theRectOpt.hasValue()) {
                qt3ds::render::SCuboidRect theRect(*theRectOpt);
                return Q3DStudio::SCameraRect(theRect.m_Left, theRect.m_Top, theRect.m_Right,
                                              theRect.m_Bottom);
            }
        }
        return Q3DStudio::SCameraRect();
    }

    void PositionToScreen(Q3DStudio::TElement &inElement, QT3DSVec3 &inPos, QT3DSVec3 &outScreen) override
    {
        Qt3DSTranslator *theTranslator =
            reinterpret_cast<Qt3DSTranslator *>(inElement.GetAssociation());
        if (theTranslator && theTranslator->m_RenderObject) {
            SNode *theNode = reinterpret_cast<SNode *>(theTranslator->m_RenderObject);
            QT3DSVec3 thePos = theNode->m_GlobalTransform.transform(inPos);
            outScreen = m_Context->GetRenderer().ProjectPosition(*theNode, thePos);
        }
    }

    void ScreenToPosition(Q3DStudio::TElement &inElement, QT3DSVec3 &inScreen, QT3DSVec3 &outPos) override
    {
        Qt3DSTranslator *theTranslator =
            reinterpret_cast<Qt3DSTranslator *>(inElement.GetAssociation());
        if (theTranslator && theTranslator->m_RenderObject) {
            SNode *theNode = reinterpret_cast<SNode *>(theTranslator->m_RenderObject);
            QT3DSVec3 objPos = theNode->GetGlobalPos();
            outPos = m_Context->GetRenderer().UnprojectWithDepth(*(theNode), objPos, inScreen);
        }
    }

    Q3DStudio::STextSizes MeasureText(Q3DStudio::TElement *inElement, const char *inTextStr) override
    {
        Qt3DSTranslator *theTranslator =
            reinterpret_cast<Qt3DSTranslator *>(inElement->GetAssociation());
        Q3DStudio::STextSizes retval;
        if (theTranslator && theTranslator->GetUIPType() == GraphObjectTypes::Text) {
            if (inElement->IsDirty()) {
                theTranslator->OnElementChanged(*m_Presentation, *m_Context,
                                                *m_RuntimePresentation);
            }
            SText *theText = static_cast<SText *>(&theTranslator->RenderObject());
            if (theText) {

                STextDimensions theDimensions =
                    m_Context->GetTextRenderer()->MeasureText(*theText, 1.0f, inTextStr);

                retval = Q3DStudio::STextSizes(Q3DStudio::INT32(theDimensions.m_TextWidth),
                                               Q3DStudio::INT32(theDimensions.m_TextHeight));
            }
        } else {
            qCCritical(INVALID_OPERATION, "MeasureText called on object that is not text");
            QT3DS_ASSERT(false);

        }
        return retval;
    }

    Q3DStudio::STextSizes GetPresentationDesignDimensions() override
    {
        if (m_Presentation) {
            return Q3DStudio::STextSizes(
                static_cast<Q3DStudio::INT32>(m_Presentation->m_PresentationDimensions.x),
                static_cast<Q3DStudio::INT32>(m_Presentation->m_PresentationDimensions.y));
        }
        QT3DS_ASSERT(false);
        return Q3DStudio::STextSizes();
    }

    virtual Q3DStudio::SMousePosition
    WindowToPresentation(const Q3DStudio::SMousePosition &inWindowCoords) override
    {
        // If there aren't any rotations, then account for the difference in width/height of the
        // presentation and the window
        QT3DSVec2 theCoords = m_Context->GetMousePickMouseCoords(
            QT3DSVec2(QT3DSF32(inWindowCoords.m_X), QT3DSF32(inWindowCoords.m_Y)));
        theCoords.x -= m_LastRenderViewport.m_X;
        // Note that the mouse Y is reversed.  Thus a positive offset of the viewport will reduce
        // the mouse value.
        theCoords.y -= m_LastRenderViewport.m_Y;
        return Q3DStudio::SMousePosition(theCoords.x, theCoords.y);
    }

    qt3ds::foundation::CRegisteredString RegisterStr(const char *inStr) override
    {
        return m_Context->GetStringTable().RegisterStr(inStr);
    }

    void RegisterOffscreenRenderer(const char *inKey) override
    {
        m_OffscreenRenderer = QT3DS_NEW(m_Context->GetAllocator(), Qt3DSRenderSceneSubPresRenderer)(
            *this, *m_Context, *m_Presentation);
        m_OffscreenRendererId = m_Context->GetStringTable().RegisterStr(inKey);
        m_Context->GetOffscreenRenderManager().RegisterOffscreenRenderer(m_OffscreenRendererId,
                                                                         *m_OffscreenRenderer);
    }

    template <typename T, typename C>
    void forAllObjects(nvvector<SGraphObject *> &vec, GraphObjectTypes::Enum type, C callable)
    {
        nvvector<SGraphObject *>::iterator it = vec.begin();
        nvvector<SGraphObject *>::iterator end = vec.end();
        while (it != end) {
            if ((*it)->m_Type == type)
                callable(static_cast<T*>(*it));
            ++it;
        }
    }

    void PostLoadStep()
    {
        IBufferManager &mgr = m_Context->GetBufferManager();
        forAllObjects<SImage>(m_GraphObjectList, GraphObjectTypes::Image, [&mgr](SImage *image){
            if (image->m_ImagePath.IsValid() && qt3ds::runtime::isImagePath(
                        image->m_ImagePath.c_str())) {
                const bool ibl = image->m_MappingMode == ImageMappingModes::LightProbe;
                image->m_LoadedTextureData = mgr.CreateReloadableImage(image->m_ImagePath,
                                                                       false, ibl);
                image->m_LoadedTextureData->m_callbacks.push_back(image);
            }
        });
    }

    void Release() override { NVDelete(m_Context->GetAllocator(), this); }
};

SOffscreenRenderFlags
Qt3DSRenderSceneSubPresRenderer::NeedsRender(const SOffscreenRendererEnvironment &inEnvironment,
                                             QT3DSVec2 inPresScale,
                                             const SRenderInstanceId instanceId)
{
    m_Scene.TransferDirtyProperties();
    return CSubPresentationRenderer::NeedsRender(inEnvironment, inPresScale, instanceId);
}

void Qt3DSRenderSceneSubPresRenderer::Render(const SOffscreenRendererEnvironment &inEnvironment,
                                             NVRenderContext &inRenderContext,
                                             QT3DSVec2 inPresScale,
                                             SScene::RenderClearCommand inClearBuffer,
                                             const SRenderInstanceId instanceId)
{
    CSubPresentationRenderer::Render(inEnvironment, inRenderContext, inPresScale, inClearBuffer,
                                     instanceId);
}

void Qt3DSRenderSceneSubPresRenderer::RenderWithClear(
        const SOffscreenRendererEnvironment &inEnvironment,
        NVRenderContext &inRenderContext, QT3DSVec2 inPresScale,
        SScene::RenderClearCommand inClearBuffer, QT3DSVec4 inClearColor,
        const SRenderInstanceId id)
{
    CSubPresentationRenderer::RenderWithClear(inEnvironment, inRenderContext,
                                              inPresScale, inClearBuffer,
                                              inClearColor, id);
}

//////////////////////////////////////////////////////////////////
// Scene Manager
//////////////////////////////////////////////////////////////////

struct Qt3DSRenderSceneManager : public Q3DStudio::ISceneManager,
                                public Q3DStudio::ISceneBinaryLoader
{
    typedef nvhash_map<CRegisteredString, bool> TStrBoolMap;

    NVScopedRefCounted<SBindingCore> m_Context;
    nvvector<pair<Q3DStudio::IPresentation *, Qt3DSRenderScene *>> m_Scenes;
    nvvector<pair<qt3ds::foundation::CRegisteredString, long>> m_RenderPlugins;
    Q3DStudio::INT32 m_ViewWidth;
    Q3DStudio::INT32 m_ViewHeight;
    Q3DStudio::SPickFrame m_PickFrame;
    NVDataRef<QT3DSU8> m_StrTableData;
    // The boolean is to mark transparent images and ibl images
    nvvector<eastl::pair<CRegisteredString, eastl::pair<bool, bool>>> m_SourcePaths;
    eastl::hash_set<CRegisteredString> m_SourcePathSet;

    Qt3DSRenderScene *m_LastRenderedScene;
    Q3DStudio::IWindowSystem &m_WindowSystem;
    Mutex m_LoadingScenesMutex;
    eastl::vector<NVScopedRefCounted<SSceneLoadData>> m_LoadingScenes;
    CRegisteredString m_ProjectDir;
    CRegisteredString m_BinaryDir;
    bool m_ProjectInitialized;
    QT3DSI32 mRefCount;

    Qt3DSRenderSceneManager(SBindingCore &ctx, Q3DStudio::IWindowSystem &inWindowSystem)
        : m_Context(ctx)
        , m_Scenes(ctx.GetAllocator(), "Qt3DSRenderSceneManager::m_Scenes")
        , m_RenderPlugins(ctx.GetAllocator(), "Qt3DSRenderSceneManager::m_RenderPlugins")
        , m_ViewWidth(0)
        , m_ViewHeight(0)
        , m_SourcePaths(ctx.GetAllocator(), "Qt3DSRenderSceneManager::m_SourcePaths")
        , m_LastRenderedScene(nullptr)
        , m_WindowSystem(inWindowSystem)
        , m_LoadingScenesMutex(ctx.GetAllocator())
        , m_ProjectInitialized(false)
        , mRefCount(0)
    {
        memZero(&m_PickFrame, sizeof(m_PickFrame));
    }
    virtual ~Qt3DSRenderSceneManager() override
    {
        for (QT3DSU32 idx = 0, end = m_Scenes.size(); idx < end; ++idx)
            m_Scenes[idx].second->Release();
        m_Scenes.clear();
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Context->GetAllocator())

    static QT3DSU32 GetFileTag()
    {
        const char *fileTag = "Qt3DSS";
        const QT3DSU32 *theTagPtr = reinterpret_cast<const QT3DSU32 *>(fileTag);
        return *theTagPtr;
    }

    void FinalizeScene(Q3DStudio::IPresentation &inPresentation, Qt3DSRenderScene &inScene)
    {
        inPresentation.SetScene(&inScene);
        if (m_ProjectInitialized == false) {
            m_ProjectInitialized = true;
            // For QT3DS-3353 assume project fonts are in a subdirectory relative to presentation.
            QString projectFontDir = inPresentation.getProjectPath() + QStringLiteral("/fonts");
            if (m_Context->m_Context->GetTextRenderer()) {
                m_Context->m_Context->GetTextRenderer()->AddProjectFontDirectory(
                    projectFontDir.toUtf8().data());
            }
            if (m_Context->m_Context->getDistanceFieldRenderer()) {
                m_Context->m_Context->getDistanceFieldRenderer()->AddProjectFontDirectory(
                    projectFontDir.toUtf8().data());
            }
            eastl::string theBinaryPath(inPresentation.GetFilePath().toLatin1().constData());
            qt3ds::foundation::CFileTools::AppendDirectoryInPathToFile(theBinaryPath, "binary");
            eastl::string theBinaryDir(theBinaryPath);
            qt3ds::foundation::CFileTools::GetDirectory(theBinaryDir);
            if (m_Context->m_WriteOutShaderCache)
                qt3ds::foundation::CFileTools::CreateDir(theBinaryDir.c_str());
        }
        inScene.m_RuntimePresentation = &inPresentation;
        m_Scenes.push_back(make_pair(&inPresentation, &inScene));
        Qt3DSTranslator::AssignUserData(inPresentation, *inScene.m_Presentation);
    }

    static const char *GetBinaryExtension() { return "uibsg"; }

    struct SPluginInstanceTableProvider : public Q3DStudio::IScriptTableProvider
    {
        IRenderPluginInstance &m_Instance;
        SPluginInstanceTableProvider(IRenderPluginInstance &ins)
            : m_Instance(ins)
        {
        }
        void CreateTable(script_State *inState) override { m_Instance.CreateScriptProxy(inState); }
    };

    void InitializeTranslator(Qt3DSTranslator &inTranslator, Q3DStudio::IScriptBridge &inBridge)
    {
        if (inTranslator.m_RenderObject->m_Type == GraphObjectTypes::RenderPlugin) {
            SRenderPlugin &theRenderPlugin =
                static_cast<SRenderPlugin &>(*inTranslator.m_RenderObject);
            IRenderPluginInstance *thePluginInstance =
                m_Context->m_Context->GetRenderPluginManager().GetOrCreateRenderPluginInstance(
                    theRenderPlugin.m_PluginPath, &theRenderPlugin);
            if (thePluginInstance) {
                SPluginInstanceTableProvider theProvider(*thePluginInstance);
                inBridge.SetTableForElement(*inTranslator.m_Element, theProvider);
            }
        }
    }

    struct SResolver : public qt3ds::render::IUIPReferenceResolver
    {
        IStringTable &m_StringTable;
        Q3DStudio::IUIPParser &m_Parser;
        SResolver(IStringTable &strt, Q3DStudio::IUIPParser &p)
            : m_StringTable(strt)
            , m_Parser(p)
        {
        }

        CRegisteredString ResolveReference(CRegisteredString inStart,
                                                   const char *inReference) override
        {
            eastl::string theResolvedId = m_Parser.ResolveReference(inStart, inReference);
            if (theResolvedId.size())
                return m_StringTable.RegisterStr(theResolvedId.c_str());
            return CRegisteredString();
        }
    };

    Q3DStudio::IScene *LoadScene(Q3DStudio::IPresentation *inPresentation,
                                 Q3DStudio::IUIPParser *inParser,
                                 Q3DStudio::IScriptBridge &inBridge,
                                 const qt3ds::Q3DSVariantConfig &variantConfig) override
    {
        // We have to initialize the tags late so that we can load flow data before adding anything
        // to the string table.
        Qt3DSTranslator::InitializePointerTags(m_Context->m_RenderContext->GetStringTable());
        NVScopedRefCounted<SSceneLoadData> theScene =
            QT3DS_NEW(m_Context->GetAllocator(), SSceneLoadData)(m_Context->GetAllocator());
        Qt3DSRenderScene *theIScene = nullptr;
        if (inParser) {
            QString thePath(inPresentation->GetFilePath());
            QFileInfo fileInfo(thePath);
            TIdObjectMap theObjMap(m_Context->GetAllocator(), "LoadScene::theObjMap");
            SResolver theResolver(m_Context->m_CoreContext->GetStringTable(), *inParser);

            theScene->m_Presentation = IUIPLoader::LoadUIPFile(
                inParser->GetDOMReader(),
                fileInfo.absoluteFilePath().toLatin1().constData(),
                inParser->GetMetaData(),
                m_Context->m_CoreContext->GetStringTable(),
                m_Context->m_RenderContext->GetFoundation(),
                theScene->m_AutoAllocator, theObjMap,
                m_Context->m_Context->GetBufferManager(),
                m_Context->m_Context->GetEffectSystem(),
                fileInfo.path().toLatin1().constData(),
                m_Context->m_Context->GetRenderPluginManager(),
                m_Context->m_Context->GetCustomMaterialSystem(),
                m_Context->m_Context->GetDynamicObjectSystem(),
                m_Context->m_Context->GetPathManager(), &theResolver,
                variantConfig, false);
            if (!theScene->m_Presentation) {
                QT3DS_ASSERT(false);
                return nullptr;
            }

            NVConstDataRef<eastl::string> theSourcePathData(inParser->GetSourcePaths());
            const QVector<QString> slideSourcePaths = inParser->GetSlideSourcePaths();
            IBufferManager &theManager(m_Context->m_Context->GetBufferManager());
            // List of image paths to be loaded in parallel at the end.
            eastl::vector<CRegisteredString> theSourcePathList;
            eastl::vector<CRegisteredString> iblList;
            for (QT3DSU32 idx = 0, end = theSourcePathData.size(); idx < end; ++idx) {
                const eastl::string &theValue = theSourcePathData[idx];
                CRegisteredString theSourcePath =
                    m_Context->m_CoreContext->GetStringTable().RegisterStr(theValue.c_str());
                size_t theValueSize = theValue.size();
                if (theValueSize > 3) {
                    CRegisteredString theObjectPath = theSourcePath;
                    if (qt3ds::runtime::isImagePath(theValue.c_str())) {
                        // load only images not on any slide
                        if (!theManager.isReloadableResourcesEnabled() ||
                                !slideSourcePaths.contains(QString::fromLatin1(theValue.c_str()))) {
                            theManager.SetImageTransparencyToFalseIfNotSet(theObjectPath);
                            bool ibl = inParser->isIblImage(theObjectPath.c_str());
                            bool transparent = theManager.GetImageHasTransparency(theObjectPath);
                            if (m_SourcePathSet.insert(theSourcePath).second) {

                                m_SourcePaths.push_back(eastl::make_pair(theSourcePath,
                                                        eastl::make_pair(transparent, ibl)));
                            }
                            if (ibl)
                                iblList.push_back(theObjectPath);
                            else
                                theSourcePathList.push_back(theObjectPath);

                        }
                    } else if (theValue.find(".mesh") != eastl::string::npos) {
                        theManager.LoadMesh(theObjectPath);
                    }
                }
            }

            // Fire off parallel loading of the source paths
            QT3DSU64 imageBatchId = m_Context->m_Context->GetImageBatchLoader().LoadImageBatch(
                toConstDataRef(theSourcePathList.data(), theSourcePathList.size()),
                CRegisteredString(), nullptr, m_Context->m_Context->GetRenderContext()
                                                .GetRenderContextType(),
                        theScene->m_Presentation->m_preferKTX, false);
            QT3DSU64 iblImageBatchId = m_Context->m_Context->GetImageBatchLoader().LoadImageBatch(
                toConstDataRef(iblList.data(), iblList.size()),
                CRegisteredString(), nullptr, m_Context->m_Context->GetRenderContext()
                                                .GetRenderContextType(),
                        theScene->m_Presentation->m_preferKTX, true);
            m_Context->m_Context->GetImageBatchLoader().BlockUntilLoaded(
                static_cast<TImageBatchId>(imageBatchId));
            m_Context->m_Context->GetImageBatchLoader().BlockUntilLoaded(
                static_cast<TImageBatchId>(iblImageBatchId));

            theIScene = QT3DS_NEW(m_Context->GetAllocator(),
                               Qt3DSRenderScene)(*m_Context, *m_Context->m_Context, *theScene);
            // Now we need to associate the presentation with everything else.
            NVAllocatorCallback &translatorAllocator = theScene->m_AutoAllocator;
            for (TIdObjectMap::iterator iter = theObjMap.begin(), end = theObjMap.end();
                 iter != end; ++iter) {
                theIScene->m_GraphObjectList.push_back(iter->second);
                Q3DStudio::SElementAndType theElement =
                    inParser->GetElementForID(iter->first.c_str());
                if (theElement.m_Element
                    && theElement.m_Type != Q3DStudio::UIPElementTypes::Unknown) {
                    Qt3DSTranslator *theTranslator = Qt3DSTranslator::CreateTranslatorForElement(
                        *theElement.m_Element, *iter->second, translatorAllocator);
                    if (theTranslator) {
                        theIScene->m_DirtySet.insert(*theTranslator);
                        InitializeTranslator(*theTranslator, inBridge);
                    }
                }
            }
            theIScene->PostLoadStep();
        } else {
            // Binary load path is quite different than normal load path and
            // nothing else will load here.
            QT3DS_ASSERT(false);
        }
        if (inPresentation && theIScene)
            FinalizeScene(*inPresentation, *theIScene);
        return theIScene;
    }

    // threadsafe
    // Can be called from any thread
    bool GetBinaryLoadFileName(eastl::string &inPresentationFilename,
                                       eastl::string &outResult) override
    {
        eastl::string theBinaryPath(inPresentationFilename.c_str());
        qt3ds::foundation::CFileTools::AppendDirectoryInPathToFile(theBinaryPath, "binary");
        qt3ds::foundation::CFileTools::SetExtension(
            theBinaryPath, GetBinaryExtension()); // uibb: short for ui binary binding
        outResult = theBinaryPath;
        return true;
    }

    // threadsafe
    // returns a handle to the loaded object. Return value of zero means error.
    qt3ds::QT3DSU32 LoadSceneStage1(CRegisteredString inPresentationDirectory,
                                      qt3ds::render::ILoadedBuffer &inData) override
    {
        SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                    "Load Scene Graph Stage 1");
        NVDataRef<QT3DSU8> theLoadedData(inData.Data());
        SDataReader theReader(theLoadedData.begin(), theLoadedData.end());

        QT3DSU32 theFileSig = theReader.LoadRef<QT3DSU32>();
        QT3DSU32 theBinaryVersion = theReader.LoadRef<QT3DSU32>();
        QT3DSU32 theDataSectionSize = QT3DSU32(theReader.m_EndPtr - theReader.m_CurrentPtr);

        if (theFileSig != GetFileTag()
            || theBinaryVersion != SGraphObject::GetSceneGraphBinaryVersion()) {
            QT3DS_ASSERT(false);
            return 0;
        }
        QT3DSU32 theLoadingSceneIndex = 0;
        SSceneLoadData *theScene;
        {
            Mutex::ScopedLock __locker(m_LoadingScenesMutex);
            theLoadingSceneIndex = QT3DSU32(m_LoadingScenes.size()) + 1;
            m_LoadingScenes.push_back(
                QT3DS_NEW(m_Context->GetAllocator(), SSceneLoadData)(m_Context->GetAllocator()));
            theScene = m_LoadingScenes.back();
        }
        // preserve the data buffer because we run directly from it; there isn't a memcopy.
        theScene->m_Data = inData;

        QT3DSU32 theTranslatorOffset = theReader.LoadRef<QT3DSU32>();

        NVDataRef<QT3DSU8> theSGData = NVDataRef<QT3DSU8>(theReader.m_CurrentPtr, theTranslatorOffset);
        NVDataRef<QT3DSU8> theTranslatorData = NVDataRef<QT3DSU8>(
            theReader.m_CurrentPtr + theTranslatorOffset, theDataSectionSize - theTranslatorOffset);

        CStrTableOrDataRef theStrTableData(m_Context->m_CoreContext->GetStringTable());
        if (m_StrTableData.size())
            theStrTableData = CStrTableOrDataRef(m_StrTableData);

        theScene->m_Presentation = SGraphObjectSerializer::Load(
            theSGData, m_StrTableData, m_Context->m_CoreContext->GetDynamicObjectSystemCore(),
            m_Context->m_CoreContext->GetPathManagerCore(), m_Context->GetAllocator(),
            inPresentationDirectory);
        if (theScene->m_Presentation)
            theScene->m_Presentation->m_PresentationDirectory = inPresentationDirectory;

        theScene->m_TranslatorData = theTranslatorData;
        theScene->m_SceneGraphData = theSGData;

        {
            Mutex::ScopedLock __locker(m_LoadingScenesMutex);
            m_LoadingScenes[theLoadingSceneIndex - 1] = theScene;
        }
        return theLoadingSceneIndex;
    }

    // threadsafe
    // still does not require openGL context but has dependency on a few other things.
    void LoadSceneStage2(qt3ds::QT3DSU32 inSceneHandle, Q3DStudio::IPresentation &inPresentation,
                                 size_t inElementMemoryOffset, Q3DStudio::IScriptBridge &inBridge) override
    {
        SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                    "Load Scene Graph Stage 2");
        QT3DSU32 theSceneIndex = QT3DS_MAX_U32;
        SSceneLoadData *theScene;
        {
            Mutex::ScopedLock __locker(m_LoadingScenesMutex);
            QT3DSU32 numLoadingScenes = QT3DSU32(m_LoadingScenes.size());
            if (inSceneHandle && inSceneHandle <= numLoadingScenes) {
                theSceneIndex = inSceneHandle - 1;
                theScene = m_LoadingScenes[theSceneIndex];
            } else {
                QT3DS_ASSERT(false);
                return;
            }
        }
        SDataReader theReader(theScene->m_TranslatorData.begin(), theScene->m_TranslatorData.end());
        QT3DSU32 theNumTranslators = theReader.LoadRef<QT3DSU32>();
        theScene->m_Translators.resize(theNumTranslators);
        theScene->m_RuntimePresentation = &inPresentation;
        for (QT3DSU32 idx = 0, end = theNumTranslators; idx < end; ++idx) {
            Qt3DSTranslator *theTranslator = Qt3DSTranslator::LoadTranslator(
                theReader, inElementMemoryOffset, theScene->m_SceneGraphData,
                theScene->m_AutoAllocator);
            if (theTranslator) {
                InitializeTranslator(*theTranslator, inBridge);
            }
            theScene->m_Translators[idx] = theTranslator;
        }
    }

    void OnGraphicsInitialized()
    {
        QT3DS_ASSERT(m_Context->m_Context.mPtr);
        // this means graphics have been initialized
        eastl::string theSourcePathStr;
        IBufferManager &theManager(m_Context->m_Context->GetBufferManager());
        nvvector<CRegisteredString> imagePathList(m_Context->GetAllocator(),
                                                      "imagePathList");
        nvvector<CRegisteredString> iblImagePathList(m_Context->GetAllocator(),
                                                      "iblImagePathList");
        for (QT3DSU32 idx = 0, end = m_SourcePaths.size(); idx < end; ++idx) {
            theSourcePathStr.assign(m_SourcePaths[idx].first);
            bool hasTransparency = m_SourcePaths[idx].second.first;
            bool isIbl = m_SourcePaths[idx].second.second;
            if (theSourcePathStr.size() > 4) {
                CRegisteredString theObjectPath = m_SourcePaths[idx].first;
                if (qt3ds::runtime::isImagePath(theSourcePathStr.c_str())) {
                    theManager.SetImageHasTransparency(theObjectPath, hasTransparency);
                    if (isIbl)
                        iblImagePathList.push_back(theObjectPath);
                    else
                        imagePathList.push_back(theObjectPath);
                } else {
                    if (theSourcePathStr.find(".mesh") != eastl::string::npos)
                        theManager.LoadMesh(theObjectPath);
                }
            }
        }

        bool pktx = false;
        for (unsigned int i = 0; i < m_Scenes.size(); ++i) {
            if (m_Scenes[i].second->m_Presentation->m_preferKTX) {
                pktx = true;
                break;
            }
        }

        {
            SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                        "Initial Batch Image Load");

            m_Context->m_Context->GetImageBatchLoader().LoadImageBatch(
                toConstDataRef(imagePathList.data(), imagePathList.size()),
                CRegisteredString(), nullptr, m_Context->m_Context->GetRenderContext()
                        .GetRenderContextType(), pktx, false);
            m_Context->m_Context->GetImageBatchLoader().LoadImageBatch(
                toConstDataRef(iblImagePathList.data(), iblImagePathList.size()),
                CRegisteredString(), nullptr, m_Context->m_Context->GetRenderContext()
                        .GetRenderContextType(), pktx, true);
        }

        {

            SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                        "Initialize Scenes");
            for (QT3DSU32 idx = 0, end = m_LoadingScenes.size(); idx < end; ++idx) {
                SSceneLoadData &theScene = *m_LoadingScenes[idx];
                // m_Context->m_Foundation->error( QT3DS_WARN, "Finalizing scene %d", (int)idx+1 );
                if (theScene.m_RuntimePresentation) {
                    Qt3DSRenderScene *theIScene = QT3DS_NEW(m_Context->GetAllocator(), Qt3DSRenderScene)(
                        *m_Context, *m_Context->m_Context, theScene);
                    FinalizeScene(*theScene.m_RuntimePresentation, *theIScene);
                    theIScene->PostLoadStep();
                } else {
                    qCWarning(WARNING, "Failed to finalize scene %d", int(idx + 1));
                }
            }
        }
    }

    void LoadRenderPlugin(const char *inAssetIDString, const char *inPath,
                                  const char *inArgs) override
    {
        Q3DStudio::CDLLManager &theDLLManager = Q3DStudio::CDLLManager::GetDLLManager();
        long theHandle = theDLLManager.LoadLibrary(inPath, EDLLTYPE_RENDERABLE_PLUGIN);
        if (theHandle >= 0) {
            qt3ds::render::IOffscreenRenderer *theOffscreenRenderer =
                QT3DS_NEW(m_Context->GetAllocator(),
                       qt3ds::render::COldNBustedPluginRenderer)(*m_Context->m_Context, theHandle);
            qt3ds::foundation::CRegisteredString theAssetString =
                m_Context->m_CoreContext->GetStringTable().RegisterStr(inAssetIDString);

            m_Context->m_Context->GetOffscreenRenderManager().RegisterOffscreenRenderer(
                theAssetString, *theOffscreenRenderer);

            PROC_Initialize theInitializeProc =
                reinterpret_cast<PROC_Initialize>(theDLLManager.GetProc("Initialize", theHandle));
            Q3DStudio_ASSERT(theInitializeProc);
#if !defined (Q_OS_MACOS)
            PROC_SetEGLInfo theSetEGLInfoProc =
                reinterpret_cast<PROC_SetEGLInfo>(theDLLManager.GetProc("SetEGLInfo", theHandle));
            // Set EGL parameters used for optional context creation
            if (theSetEGLInfoProc) {
                Q3DStudio::SEGLInfo *theInfo = m_WindowSystem.GetEGLInfo();
                if (theInfo)
                    theSetEGLInfoProc(theInfo->display, theInfo->context, theInfo->surface,
                                      theInfo->config);
            }
#endif
            if (theInitializeProc && theInitializeProc(inArgs) == EDLLSTATUS_OK)
                m_RenderPlugins.push_back(make_pair(theAssetString, theHandle));
            else
                qCWarning(qt3ds::INVALID_OPERATION) << "Unable to load plugin " << inAssetIDString;
        } else
            qCWarning(qt3ds::INVALID_OPERATION) << "Unable to load plugin " << inAssetIDString;

        return;
    }

    void LoadQmlStreamerPlugin(const char *inAssetIDString) override
    {
        qt3ds::render::IOffscreenRenderer *theOffscreenRenderer =
            QT3DS_NEW(m_Context->GetAllocator(),
                   Q3DSQmlRender)(*m_Context->m_Context, inAssetIDString);
        if (theOffscreenRenderer) {
            qt3ds::foundation::CRegisteredString theAssetString =
                m_Context->m_CoreContext->GetStringTable().RegisterStr(inAssetIDString);
            m_Context->m_Context->GetOffscreenRenderManager().RegisterOffscreenRenderer(
                SOffscreenRendererKey(theAssetString), *theOffscreenRenderer);

            m_RenderPlugins.push_back(make_pair(theAssetString, 0));
        }
    }

    void BinarySave(Q3DStudio::IScene &inScene) override
    {
        Qt3DSRenderScene &theScene = static_cast<Qt3DSRenderScene &>(inScene);
        qt3ds::render::SWriteBuffer theWriteBuffer(m_Context->GetAllocator(), "BinarySaveBuffer");
        qt3ds::render::SPtrOffsetMap theSGOffsetMap(m_Context->GetAllocator(), "PointerOffsetMap");
        // Start with some versioning and sanity checks.
        theWriteBuffer.write(GetFileTag());
        theWriteBuffer.write(SGraphObject::GetSceneGraphBinaryVersion());
        QT3DSU32 theTranslatorOffsetAddress = theWriteBuffer.size();
        // Now the data section starts.  Offsets should be relative to here, not the first
        // 8 bytes.
        theWriteBuffer.writeZeros(4); // offset where the translator data starts;
        QT3DSU32 theDataSectionStart = theWriteBuffer.size();

        // These offsets are after we have read in the data section
        SGraphObjectSerializer::Save(
            m_Context->m_RenderContext->GetFoundation(), *theScene.m_Presentation, theWriteBuffer,
            m_Context->m_Context->GetDynamicObjectSystem(),
            m_Context->m_Context->GetPathManager(), theSGOffsetMap,
            m_Context->m_CoreContext->GetStringTable(), theScene.m_GraphObjectList);

        theWriteBuffer.align(sizeof(void *));
        QT3DSU32 theTranslatorCountAddress = theWriteBuffer.size();
        QT3DSU32 theTranslatorOffset = theTranslatorCountAddress - theDataSectionStart;

        theWriteBuffer.writeZeros(4);
        // Now write out the translators verbatim.  We get an adjustment parameter on save that
        // allows a translation
        // from old element ptr->new element ptr.
        QT3DSU32 theTranslatorCount = 0;
        for (QT3DSU32 idx = 0, end = theScene.m_GraphObjectList.size(); idx < end; ++idx) {
            Qt3DSTranslator *theTranslator =
                Qt3DSTranslator::GetTranslatorFromGraphNode(*theScene.m_GraphObjectList[idx]);
            // Presentation nodes don't have translator
            if (theTranslator) {
                qt3ds::render::SPtrOffsetMap::iterator theIter =
                    theSGOffsetMap.find(theScene.m_GraphObjectList[idx]);
                if (theIter != theSGOffsetMap.end()) {
                    QT3DSU32 theOffset = theIter->second;
                    theTranslator->Save(theWriteBuffer, theOffset);
                    ++theTranslatorCount;
                } else {
                    QT3DS_ASSERT(false);
                }
            }
        }
        QT3DSU32 *theTranslatorCountPtr =
            reinterpret_cast<QT3DSU32 *>(theWriteBuffer.begin() + theTranslatorCountAddress);
        *theTranslatorCountPtr = theTranslatorCount;
        QT3DSU32 *theTranslatorOffsetPtr =
            reinterpret_cast<QT3DSU32 *>(theWriteBuffer.begin() + theTranslatorOffsetAddress);
        *theTranslatorOffsetPtr = theTranslatorOffset;

        Q3DStudio::IPresentation &thePresentation = *theScene.m_RuntimePresentation;
        eastl::string theBinaryPath(thePresentation.GetFilePath().toLatin1().constData());
        qt3ds::foundation::CFileTools::AppendDirectoryInPathToFile(theBinaryPath, "binary");
        eastl::string theBinaryDir(theBinaryPath);
        qt3ds::foundation::CFileTools::GetDirectory(theBinaryDir);
        qt3ds::foundation::CFileTools::SetExtension(
            theBinaryPath, GetBinaryExtension()); // uibb: short for ui binary binding

        Q3DStudio::CFileStream theStream(theBinaryPath.c_str(), "wb");
        if (theStream.IsOpen() == false) {
            QT3DS_ASSERT(false);
        }

        theStream.WriteRaw(theWriteBuffer.begin(), theWriteBuffer.size());
        theStream.Close();
    }

    // We save in the reverse order that we load because the effect system may add strings
    // to the string table when it is writing its data out, this the string table needs to come
    // last.
    // Loading, obviously, the string table needs to be the first object loaded.
    void BinarySaveManagerData(qt3ds::foundation::IOutStream &inStream,
                                       const char *inBinaryDir) override
    {
        qt3ds::render::SWriteBuffer theWriteBuffer(m_Context->GetAllocator(), "BinarySaveBuffer");
        IStringTable &theStrTable = m_Context->m_CoreContext->GetStringTable();
        eastl::string theProjectDir(inBinaryDir);
        qt3ds::foundation::CFileTools::GetDirectory(theProjectDir);

        // We save everything before the string table because often times saving something creates
        // new strings.
        theWriteBuffer.writeZeros(4); // Total data size
        // Dynamic object system
        theWriteBuffer.writeZeros(4); // Effect system offset
        // effect system
        theWriteBuffer.writeZeros(4); // Material system offset
        // material system
        theWriteBuffer.writeZeros(4); // Binary path offset
        // binary path data
        theWriteBuffer.writeZeros(4); // Plugin manager offset
        // plugin manager
        theWriteBuffer.writeZeros(4); // String system offset
        // string system last.
        QT3DSU32 theOffsetStart = theWriteBuffer.size();
        m_Context->m_Context->GetDynamicObjectSystem().Save(
            theWriteBuffer, theStrTable.GetRemapMap(), theProjectDir.c_str());
        theWriteBuffer.align(sizeof(void *));
        QT3DSU32 theEffectSystemOffset = theWriteBuffer.size() - theOffsetStart;
        m_Context->m_Context->GetEffectSystem().Save(theWriteBuffer, theStrTable.GetRemapMap(),
                                                        theProjectDir.c_str());
        theWriteBuffer.align(sizeof(void *));
        QT3DSU32 theMaterialSystemOffset = theWriteBuffer.size() - theOffsetStart;
        m_Context->m_Context->GetCustomMaterialSystem().Save(
            theWriteBuffer, theStrTable.GetRemapMap(), theProjectDir.c_str());
        QT3DSU32 theBinaryPathOffset = theWriteBuffer.size() - theOffsetStart;

        theWriteBuffer.write(QT3DSU32(m_SourcePaths.size()));
        for (nvvector<pair<CRegisteredString, eastl::pair<bool, bool>>>::iterator iter
             = m_SourcePaths.begin(), end = m_SourcePaths.end();
             iter != end; ++iter) {
            CRegisteredString theStr(iter->first);
            theStr.Remap(theStrTable.GetRemapMap());
            theWriteBuffer.write(size_t(theStr.c_str()));
            QT3DSU32 theSourcePathFlags = iter->second.first ? 1 : 0;
            theSourcePathFlags |= iter->second.second ? 2 : 0;
            theWriteBuffer.write(theSourcePathFlags);
        }

        QT3DSU32 thePluginManagerOffset = theWriteBuffer.size() - theOffsetStart;

        m_Context->m_Context->GetRenderPluginManager().Save(
            theWriteBuffer, theStrTable.GetRemapMap(), theProjectDir.c_str());

        QT3DSU32 theStringTableOffset = theWriteBuffer.size() - theOffsetStart;

        theStrTable.Save(theWriteBuffer);

        QT3DSU32 *theSizePtr = reinterpret_cast<QT3DSU32 *>(theWriteBuffer.begin());

        theSizePtr[0] = theWriteBuffer.size() - 4; // overall size
        theSizePtr[1] = theEffectSystemOffset;
        theSizePtr[2] = theMaterialSystemOffset;
        theSizePtr[3] = theBinaryPathOffset; // thePathOffset
        theSizePtr[4] = thePluginManagerOffset;
        theSizePtr[5] = theStringTableOffset;

        inStream.Write(theWriteBuffer.begin(), theWriteBuffer.size());
    }

    NVDataRef<qt3ds::QT3DSU8> BinaryLoadManagerData(qt3ds::foundation::IInStream &inStream,
                                                      const char *inBinaryDir) override
    {
        SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                    "Load UIAB - String Table + Render Objects");
        QT3DS_ASSERT(m_Context->m_FlowData == nullptr);
        QT3DSU32 dataSize = 0;
        inStream.Read(dataSize);
        m_Context->m_FlowData = (QT3DSU8 *)m_Context->m_CoreContext->GetAllocator().allocate(
            dataSize, "SceneManager::BinaryFlowData", __FILE__, __LINE__);

        {
            SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                        "Load UIAB - Initial Data Load");
            inStream.Read(m_Context->m_FlowData, dataSize);
        }
        SDataReader theReader(m_Context->m_FlowData, m_Context->m_FlowData + dataSize);
        QT3DSU32 theEffectSystemOffset = theReader.LoadRef<QT3DSU32>();
        QT3DSU32 theMaterialSystemOffset = theReader.LoadRef<QT3DSU32>();
        QT3DSU32 theBinaryPathOffset = theReader.LoadRef<QT3DSU32>();
        QT3DSU32 thePluginManagerOffset = theReader.LoadRef<QT3DSU32>();
        QT3DSU32 theStringTableOffset = theReader.LoadRef<QT3DSU32>();
        QT3DSU8 *theStartOffset = theReader.m_CurrentPtr;
        IStringTable &theStrTable = m_Context->m_CoreContext->GetStringTable();

        // Load string table.
        {
            SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                        "Load UIAB - Load String Table");
            m_StrTableData = toDataRef(theReader.m_CurrentPtr + theStringTableOffset,
                                       dataSize - theStringTableOffset);
            theStrTable.Load(m_StrTableData);
        }

        // Load source paths to preload heavy data
        theReader.m_CurrentPtr = theStartOffset + theBinaryPathOffset;
        QT3DSU32 theNumSourcePaths = theReader.LoadRef<QT3DSU32>();
        eastl::string theSourcePathStr;
        eastl::string theProjectDir(inBinaryDir);
        eastl::string theShaderCacheDir(inBinaryDir);

        // Up one moves to the project directory.
        qt3ds::foundation::CFileTools::GetDirectory(theProjectDir);
        const char8_t *theBasePath(theProjectDir.c_str());

        m_ProjectDir = theStrTable.RegisterStr(theProjectDir.c_str());
        m_BinaryDir = theStrTable.RegisterStr(theShaderCacheDir.c_str());

        // Preload the heavy buffers
        m_SourcePaths.resize(theNumSourcePaths);
        for (QT3DSU32 idx = 0, end = theNumSourcePaths; idx < end; ++idx) {
            CRegisteredString thePath = theReader.LoadRef<CRegisteredString>();
            QT3DSU32 theFlags = theReader.LoadRef<QT3DSU32>();
            thePath.Remap(m_StrTableData);
            bool theBoolFlagValue = theFlags ? true : false;
            m_SourcePaths[idx] = eastl::make_pair(thePath, theBoolFlagValue);
        }

        {
            SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                        "Load UIAB - Base Dynamic System");
            // Load effect system.
            NVDataRef<QT3DSU8> theDynamicSystemData(theStartOffset, theEffectSystemOffset);
            m_Context->m_CoreContext->GetDynamicObjectSystemCore().Load(
                theDynamicSystemData, m_StrTableData, theBasePath);
        }

        {
            SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                        "Load UIAB - Effect System");
            NVDataRef<QT3DSU8> theEffectSystemData(theStartOffset + theEffectSystemOffset,
                                                theMaterialSystemOffset - theEffectSystemOffset);
            m_Context->m_CoreContext->GetEffectSystemCore().Load(theEffectSystemData,
                                                                 m_StrTableData, theBasePath);
        }

        {
            SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                        "Load UIAB - Material System");
            NVDataRef<QT3DSU8> theMaterialSystemData(theStartOffset + theMaterialSystemOffset,
                                                  thePluginManagerOffset - theMaterialSystemOffset);
            m_Context->m_CoreContext->GetMaterialSystemCore().Load(theMaterialSystemData,
                                                                   m_StrTableData, theBasePath);
        }

        {
            SStackPerfTimer __perfTimer(m_Context->m_CoreContext->GetPerfTimer(),
                                        "Load UIAB - Plugin Manager Data");
            NVDataRef<QT3DSU8> thePluginManagerData(theStartOffset + thePluginManagerOffset,
                                                 theStringTableOffset - thePluginManagerOffset);
            m_Context->m_CoreContext->GetRenderPluginCore().Load(thePluginManagerData,
                                                                 m_StrTableData, theBasePath);
        }

        return m_StrTableData;
    }

    virtual void DeleteScene(Q3DStudio::IPresentation *inPresentation)
    {
        QT3DSU32 idx;
        QT3DSU32 end;
        for (idx = 0, end = m_Scenes.size(); idx < end; ++idx) {
            if (m_Scenes[idx].first == inPresentation)
                break;
        }
        if (idx < m_Scenes.size()) {
            m_Scenes[idx].second->Release();
            m_Scenes.erase(m_Scenes.begin() + idx);
        }
    }

    Q3DStudio::BOOL Update() override
    {
        bool theResult = false;
        long theSceneCount = m_Scenes.size();
        for (size_t theSceneIndex = 0; theSceneIndex < theSceneCount; ++theSceneIndex) {
            Qt3DSRenderScene *theScene = m_Scenes[theSceneIndex].second;
            theResult |= theScene->Update();
        }
        return theResult;
    }

    virtual void GetActiveSubPresentations(QVector<qt3ds::foundation::CRegisteredString> &subs)
    {
        size_t theSceneCount = m_Scenes.size();
        for (size_t sid = 0; sid < theSceneCount; ++sid) {
            Qt3DSRenderScene *theScene = m_Scenes[sid].second;
            theScene->GetActiveSubPresentations(subs);
        }
    }

    Q3DStudio::BOOL RenderPresentation(Q3DStudio::IPresentation *inPresentation,
                                       bool firstFrame) override
    {
        Qt3DSRenderScene *theFirstScene = nullptr;
        for (QT3DSU32 idx = 0, end = m_Scenes.size(); idx < end && theFirstScene == nullptr; ++idx)
            if (m_Scenes[idx].second->m_RuntimePresentation == inPresentation)
                theFirstScene = m_Scenes[idx].second;

        if (theFirstScene && theFirstScene->m_Presentation) {
            m_LastRenderedScene = theFirstScene;
            if (theFirstScene->m_Presentation->m_Scene
                && theFirstScene->m_Presentation->m_Scene->m_UseClearColor) {
                m_Context->m_Context->SetSceneColor(
                    theFirstScene->m_Presentation->m_Scene->m_ClearColor);
            } else
                m_Context->m_Context->SetSceneColor(QT3DSVec4(0.0f, 0.0f, 0.0f, 0.0f));

            // Setup the render rotation *before* rendering so that the magic can happen on begin
            // render.
            if (m_Context->m_RenderRotationsEnabled)
                m_Context->m_Context->SetRenderRotation(
                    theFirstScene->m_Presentation->m_PresentationRotation);
            else
                m_Context->m_Context->SetRenderRotation(RenderRotationValues::NoRotation);

            m_Context->m_Context->SetPresentationDimensions(QSize(
                int(theFirstScene->m_Presentation->m_PresentationDimensions.x),
                int(theFirstScene->m_Presentation->m_PresentationDimensions.y)));
        }

        m_Context->m_Context->BeginFrame(firstFrame);
        m_Context->m_RenderContext->ResetBlendState();

        // How exactly does this work, I have no idea.
        // Should we only render the first scene and not every scene, perhaps?
        bool wasDirty = false;
        if (theFirstScene)
            wasDirty = theFirstScene->PrepareForRender();
        else {
            m_Context->m_RenderContext->SetClearColor(QT3DSVec4(0, 0, 0, 0));
            m_Context->m_RenderContext->Clear(qt3ds::render::NVRenderClearFlags(
                NVRenderClearValues::Color | NVRenderClearValues::Depth));
        }
        m_Context->m_Context->RunRenderTasks();
        if (theFirstScene)
            theFirstScene->Render();

        m_Context->m_Context->EndFrame();

        return wasDirty;
    }
    // I think render::check resize is called so this isn't necessary
    void OnViewResize(Q3DStudio::INT32 inViewWidth, Q3DStudio::INT32 inViewHeight) override
    {
        m_ViewWidth = inViewWidth;
        m_ViewHeight = inViewHeight;
    }
    void GetViewSize(Q3DStudio::INT32 &outWidth, Q3DStudio::INT32 &outHeight) override
    {
        outWidth = m_ViewWidth;
        outHeight = m_ViewHeight;
    }

    Q3DStudio::STextSizes GetDisplayDimensions(Q3DStudio::IPresentation *inPresentation) override
    {
        Qt3DSRenderScene *theFirstScene = nullptr;
        for (QT3DSU32 idx = 0, end = m_Scenes.size(); idx < end && theFirstScene == nullptr; ++idx)
            if (m_Scenes[idx].second->m_RuntimePresentation == inPresentation)
                theFirstScene = m_Scenes[idx].second;
        if (theFirstScene) {
            m_Context->m_Context->SetPresentationDimensions(QSize(
                int(theFirstScene->m_Presentation->m_PresentationDimensions.x),
                int(theFirstScene->m_Presentation->m_PresentationDimensions.y)));
            render::NVRenderRectF theDisplayViewport =
                m_Context->m_Context->GetDisplayViewport();
            return Q3DStudio::STextSizes(
                static_cast<Q3DStudio::INT32>(theDisplayViewport.m_Width),
                static_cast<Q3DStudio::INT32>(theDisplayViewport.m_Height));
        }
        return Q3DStudio::STextSizes(static_cast<Q3DStudio::INT32>(0),
                                     static_cast<Q3DStudio::INT32>(0));
    }

    Q3DStudio::TElement *UserPick(float mouseX, float mouseY) override
    {
        if (m_LastRenderedScene) {
            return m_LastRenderedScene->UserPick(mouseX, mouseY);
        }
        return nullptr;
    }

    Option<QT3DSVec2> FacePosition(Q3DStudio::TElement &inElement, float mouseX, float mouseY,
                                        NVDataRef<Q3DStudio::TElement *> inElements,
                                        Q3DStudio::FacePositionPlanes::Enum inPlane) override
    {
        if (m_LastRenderedScene) {

            return m_LastRenderedScene->FacePosition(inElement, mouseX, mouseY, inElements,
                                                     inPlane);
        }
        return Empty();
    }

    Q3DStudio::SPickFrame AdvancePickFrame(const Q3DStudio::SInputFrame &inInputFrame) override
    {
        // We now have a new input frame, and our results are invalid but ready to be filled
        m_PickFrame.m_InputFrame = inInputFrame;
        m_PickFrame.m_Model = nullptr;
        m_PickFrame.m_ResultValid = false;
        if (m_LastRenderedScene) {
            if (m_PickFrame.m_InputFrame.m_PickValid)
                m_LastRenderedScene->Pick(m_PickFrame);
        }
        return m_PickFrame;
    }

    void Release() override
    {
        long theRenderPluginSize = m_RenderPlugins.size();
        Q3DStudio::CDLLManager &theDLLManager = Q3DStudio::CDLLManager::GetDLLManager();
        for (int theRenderPluginIndex = 0; theRenderPluginIndex < theRenderPluginSize;
             ++theRenderPluginIndex) {
            long theDLLHandle = m_RenderPlugins[theRenderPluginIndex].second;
            PROC_Uninitialize theUninitializeProc = reinterpret_cast<PROC_Uninitialize>(
                theDLLManager.GetProc("Uninitialize", theDLLHandle));
            Q3DStudio_ASSERT(theUninitializeProc);
            theUninitializeProc &&theUninitializeProc();

            theDLLManager.UnloadLibrary(theDLLHandle);
        }

        // Ensure the binding core doesn't get released until after we get released.
        NVScopedRefCounted<SBindingCore> theContext(m_Context);
        NVDelete(m_Context->GetAllocator(), this);
    }
};

struct SRenderFactory;

struct SRenderFactory : public IQt3DSRenderFactoryCore, public IQt3DSRenderFactory
{
    NVScopedRefCounted<SBindingCore> m_Context;

    NVScopedRefCounted<Q3DStudio::CQmlEngine> m_ScriptBridgeQml;
    NVScopedRefCounted<Qt3DSRenderSceneManager> m_SceneManager;
    NVScopedRefCounted<qt3ds::evt::IEventSystem> m_EventSystem;
    qt3ds::runtime::IApplication *m_Application;
    QT3DSI32 m_RefCount;

    SRenderFactory(SBindingCore &inCore)
        : m_Context(inCore)
        , m_ScriptBridgeQml(nullptr)
        , m_SceneManager(nullptr)
        , m_Application(nullptr)
        , m_RefCount(0)
    {
    }

    ~SRenderFactory()
    {
        using namespace Q3DStudio;
        // Release the event system, it must be released before script engine
        m_EventSystem = nullptr;
        m_ScriptBridgeQml->Shutdown(*m_Context->m_Foundation);
    }

    void addRef() override { atomicIncrement(&m_RefCount); }

    void release() override
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVScopedRefCounted<SBindingCore> theContext(m_Context);
            NVDelete(m_Context->GetAllocator(), this);
        }
    }

    qt3ds::render::IQt3DSRenderContextCore &GetRenderContextCore() override
    {
        return *m_Context->m_CoreContext;
    }

    qt3ds::runtime::IApplication *GetApplicationCore() override { return m_Application; }
    void SetApplicationCore(qt3ds::runtime::IApplication *app) override
    {
        m_Application = app;
    }

    Q3DStudio::ISceneBinaryLoader &GetSceneLoader() override
    {
        if (m_SceneManager == nullptr)
            m_SceneManager = QT3DS_NEW(m_Context->GetAllocator(),
                                    Qt3DSRenderSceneManager)(*m_Context, m_Context->m_WindowSystem);
        return *m_SceneManager;
    }

    Q3DStudio::ITegraApplicationRenderEngine &CreateRenderEngine() override
    {
        return m_Context->CreateRenderer();
    }
    Q3DStudio::ISceneManager &GetSceneManager() override
    {
        if (m_SceneManager == nullptr)
            m_SceneManager = QT3DS_NEW(m_Context->GetAllocator(),
                                    Qt3DSRenderSceneManager)(*m_Context, m_Context->m_WindowSystem);
        return *m_SceneManager;
    }
    Q3DStudio::IScriptBridge &GetScriptEngineQml() override
    {
        if (m_ScriptBridgeQml == nullptr) {
            m_ScriptBridgeQml =
                Q3DStudio::CQmlEngine::Create(*m_Context->m_Foundation, m_Context->m_TimeProvider);
        }

        return *m_ScriptBridgeQml;
    }
    qt3ds::render::IInputStreamFactory &GetInputStreamFactory() override
    {
        return m_Context->m_CoreContext->GetInputStreamFactory();
    }

    qt3ds::render::IQt3DSRenderContext &GetQt3DSRenderContext() override
    {
        return *m_Context->m_Context;
    }

    qt3ds::evt::IEventSystem &GetEventSystem() override
    {
        if (!m_EventSystem) {
            m_EventSystem = qt3ds::evt::IEventSystem::Create(*m_Context->m_Foundation);
        }
        return *m_EventSystem;
    }
    Q3DStudio::ITimeProvider &GetTimeProvider() override { return m_Context->m_TimeProvider; }
    qt3ds::foundation::IStringTable &GetStringTable() override
    {
        return m_Context->m_CoreContext->GetStringTable();
    }
    qt3ds::NVFoundationBase &GetFoundation() override { return *m_Context->m_Foundation.mPtr; }
    qt3ds::foundation::IPerfTimer &GetPerfTimer() override
    {
        return m_Context->m_CoreContext->GetPerfTimer();
    }
    qt3ds::runtime::IApplication *GetApplication() override { return m_Application; }
    void SetApplication(qt3ds::runtime::IApplication *app) override
    {
        m_Application = app;
        if (app) {
            // QML engine
            GetScriptEngineQml();
            m_ScriptBridgeQml->SetApplication(*app);
            m_ScriptBridgeQml->Initialize();
        }
    }
    void SetDllDir(const char *dllDir) override
    {
        m_Context->m_CoreContext->GetRenderPluginCore().SetDllDir(dllDir);
    }

    void AddSearchPath(const char8_t *inFile) override
    {
        m_Context->m_CoreContext->GetInputStreamFactory().AddSearchDirectory(inFile);
    }
    virtual void Release() { NVDelete(m_Context->GetAllocator(), this); }

    struct SContextTypeRenderFactory : public IRuntimeFactoryRenderFactory
    {
        QSurfaceFormat format;
        SContextTypeRenderFactory(const QSurfaceFormat &fmt)
            : format(fmt)
        {
        }

        qt3ds::render::NVRenderContext *CreateRenderContext(qt3ds::NVFoundationBase &foundat,
                                                                 IStringTable &strt) override
        {
#ifndef Qt3DS_NO_RENDER_SYMBOLS
            qt3ds::render::NVRenderContext &retval = NVRenderContext::CreateGL(foundat, strt, format);
            return &retval;
#else
            qt3ds::render::NVRenderContext &retval = NVRenderContext::Createnullptr(foundat, strt);
            return &retval;
#endif
        }
    };

    IQt3DSRenderFactory &CreateRenderFactory(const QSurfaceFormat& format,
                                             bool delayedLoading) override
    {

        SContextTypeRenderFactory theContextFactory(format);
        m_Context->CreateRenderContext(theContextFactory, delayedLoading);

        GetSceneLoader();
        {
            SStackPerfTimer __loadTimer(GetPerfTimer(), "SceneManager OnGraphicsInitialized");
            m_SceneManager->OnGraphicsInitialized();
        }
        return *this;
    }
};

}

IQt3DSRenderFactoryCore &IQt3DSRenderFactoryCore::CreateRenderFactoryCore(
        const char8_t *inApplicationDirectory,
        Q3DStudio::IWindowSystem &inWindowSystem,
        Q3DStudio::ITimeProvider &inTimeProvider)
{
    SBindingCore *theCore = reinterpret_cast<SBindingCore *>(malloc(sizeof(SBindingCore)));
    new (theCore) SBindingCore(inApplicationDirectory, inWindowSystem, inTimeProvider);
    return *QT3DS_NEW(theCore->GetAllocator(), SRenderFactory)(*theCore);
}
