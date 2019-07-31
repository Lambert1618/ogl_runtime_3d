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

#ifndef QT3DS_RENDER_RUNTIME_BINDING_IMPL_H
#define QT3DS_RENDER_RUNTIME_BINDING_IMPL_H

#include "Qt3DSRenderRuntimeBinding.h"
#include "foundation/TrackingAllocator.h"
#include "foundation/Utils.h"
#include "foundation/Qt3DSRefCounted.h"
#include "Qt3DSRender.h"
#include "Qt3DSRenderContextCore.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Qt3DSAtomic.h"
#include "Qt3DSIPresentation.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "Qt3DSRenderPresentation.h"
#include "Qt3DSRenderScene.h"
#include "Qt3DSRenderLayer.h"
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderCamera.h"
#include "Qt3DSRenderModel.h"
#include "Qt3DSRenderDefaultMaterial.h"
#include "Qt3DSRenderImage.h"
#include "Qt3DSUIPParser.h"
#include "Qt3DSRenderEffect.h"
#include "Qt3DSRenderCustomMaterial.h"
#include "Qt3DSRenderEffectSystem.h"
#include "foundation/Qt3DSInvasiveSet.h"
#include "Qt3DSIPresentation.h"
#include "Qt3DSTextRenderer.h"
#include "Qt3DSRenderText.h"
#include "foundation/StrConvertUTF.h"
#include "Qt3DSMetadata.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSWindowSystem.h"
#include "Qt3DSRenderPluginGraphObject.h"
#include "Qt3DSRenderReferencedMaterial.h"
#include "Qt3DSRenderText.h"
#include "foundation/Qt3DSMutex.h"

#ifdef EA_PLATFORM_WINDOWS
#pragma warning(disable : 4355)
#else
#define stricmp strcasecmp
#endif

using namespace qt3ds::render;
using eastl::make_pair;
using eastl::pair;

namespace qt3ds {
namespace render {

    extern qt3ds::foundation::MallocAllocator g_BaseAllocator;
    // Small core object shared between the larger objects.
    // creates the object stack needed by the rest of the system.
    struct SBindingCore : public NVRefCounted
    {
        qt3ds::render::CAllocator m_Allocator;
        QT3DSU8 *m_FlowData;
        NVScopedRefCounted<NVFoundation> m_Foundation;
        NVScopedRefCounted<IStringTable> m_StringTable;
        NVScopedRefCounted<IQt3DSRenderContextCore> m_CoreContext;
        NVScopedRefCounted<NVRenderContext> m_RenderContext;
        NVScopedRefCounted<IQt3DSRenderContext> m_Context;
        QSize m_WindowDimensions;
        eastl::string m_PrimitivePath;
        bool m_RenderRotationsEnabled;
        bool m_WriteOutShaderCache;
        volatile QT3DSI32 mRefCount;
        Q3DStudio::IWindowSystem &m_WindowSystem;
        Q3DStudio::ITimeProvider &m_TimeProvider;

        SBindingCore(const char8_t *inPrimitivePath, Q3DStudio::IWindowSystem &inWindowSystem,
                     Q3DStudio::ITimeProvider &inTimeProvider)
            : m_Allocator()
            , m_FlowData(NULL)
            , m_Foundation(NVCreateFoundation(QT3DS_FOUNDATION_VERSION, m_Allocator))
            , m_StringTable(IStringTable::CreateStringTable(m_Foundation->getAllocator()))
            , m_CoreContext(IQt3DSRenderContextCore::Create(*m_Foundation, *m_StringTable))
            , m_PrimitivePath(inPrimitivePath)
            , m_RenderRotationsEnabled(false)
            , m_WriteOutShaderCache(false)
            , mRefCount(0)
            , m_WindowSystem(inWindowSystem)
            , m_TimeProvider(inTimeProvider)
        {
            if (!IQt3DSRenderContextCore::distanceFieldEnabled()) {
                m_CoreContext->SetTextRendererCore(
                            ITextRendererCore::CreateQtTextRenderer(*m_Foundation, *m_StringTable));
            }
#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)
            else {
                m_CoreContext->setDistanceFieldRenderer(
                            ITextRendererCore::createDistanceFieldRenderer(*m_Foundation));
            }
#endif

            m_CoreContext->SetOnscreenTextRendererCore(
                ITextRendererCore::CreateOnscreenTextRenderer(*m_Foundation));
        }
        virtual ~SBindingCore()
        {
            m_CoreContext = NULL;
            m_Context = NULL;
            m_RenderContext = NULL;
            if (m_FlowData)
                m_Allocator.deallocate(m_FlowData);
            m_FlowData = NULL;
        }

        void CreateRenderContext(qt3ds::render::IRuntimeFactoryRenderFactory &inContextFactory,
                                 bool delayedLoading)
        {
            m_RenderContext = inContextFactory.CreateRenderContext(*m_Foundation, *m_StringTable);
            if (m_RenderContext) {
                m_Context =
                    m_CoreContext->CreateRenderContext(*m_RenderContext, m_PrimitivePath.c_str(),
                                                       delayedLoading);
            }
        }

        QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(g_BaseAllocator)

        NVAllocatorCallback &GetAllocator() { return m_Allocator; }

        Q3DStudio::ITegraApplicationRenderEngine &CreateRenderer();
    };
    struct STranslatorContext
    {
    };

    ////////////////////////////////////////
    //	Translators are the set of objects that translate
    //	changes in the elements to changes in the scene graph.
    //  TODO - get rid of the virtual functions and just do dispatch
    //	on the m_RenderObject.m_Type.
    ////////////////////////////////////////
    class Qt3DSTranslator
    {
    public:
        QT3DSU32 m_DirtyIndex;
        Q3DStudio::TElement *m_Element;
        SGraphObject *m_RenderObject;
        STranslatorContext *m_TranslatorContext;

        Qt3DSTranslator(Q3DStudio::TElement &inElement, SGraphObject &inRenderObject);
        GraphObjectTypes::Enum GetUIPType() const { return m_RenderObject->m_Type; }
        Q3DStudio::TElement &Element() { return *m_Element; }
        SGraphObject &RenderObject() { return *m_RenderObject; }

        // This function is done via a dispatch mechanism on the graph object type.
        void OnElementChanged(SPresentation &inPresentation, IQt3DSRenderContext &inRenderContext,
                              Q3DStudio::IPresentation &inStudioPresentation);

        void Save(SWriteBuffer &inWriteBuffer, QT3DSU32 inGraphObjectOffset);
        // Most translators don't need an extra allocation but effects due because the mapping from
        // effect property
        // to runtime property is too complex to do quickly.
        static Qt3DSTranslator *LoadTranslator(SDataReader &inReader, size_t inElemOffset,
                                              NVDataRef<QT3DSU8> inSGSection,
                                              NVAllocatorCallback &inAllocator);

        static Qt3DSTranslator *GetTranslatorFromGraphNode(SGraphObject &inObject);
        static Qt3DSTranslator *CreateTranslatorForElement(Q3DStudio::TElement &inElement,
                                                           SGraphObject &inGraphObject,
                                                           NVAllocatorCallback &inAlloc);
        static Q3DStudio::IPresentation *
        GetPresentationFromPresentation(SPresentation &inPresentation);
        static void InitializePointerTags(IStringTable &inTable);
        static void AssignUserData(Q3DStudio::IPresentation &inPresentation,
                                   SPresentation &inGraphPresentation);
    };

    struct STranslatorGetOp
    {
        QT3DSU32 operator()(const Qt3DSTranslator &translator) { return translator.m_DirtyIndex; }
    };
    struct STranslatorSetOp
    {
        void operator()(Qt3DSTranslator &translator, QT3DSU32 value)
        {
            translator.m_DirtyIndex = value;
        }
    };

    typedef InvasiveSet<Qt3DSTranslator, STranslatorGetOp, STranslatorSetOp> TTranslatorDirytSet;
}
}

#endif
