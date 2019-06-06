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
#ifndef QT3DS_RENDER_RENDER_LIST_H
#define QT3DS_RENDER_RENDER_LIST_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "render/Qt3DSRenderBaseTypes.h"

namespace qt3ds {
namespace render {

    class IRenderTask
    {
    public:
        virtual ~IRenderTask() {}
        virtual void Run() = 0;
        virtual bool persistent() const
        {
            return false;
        }
    };

    /**
     * The render list exists so that dependencies of the main render target can render themselves
     * completely before the main render target renders.  From Maxwell GPU's on, we have a tiled
     * architecture.  This tiling mechanism is sensitive to switching the render target, so we would
     * really like to render completely to a single render target and then switch.  With our layered
     * render architecture, this is not very feasible unless dependencies render themselves before
     * the
     * main set of layers render themselves.  Furthermore it benefits the overall software
     * architecture
     * to have a distinct split between the prepare for render step and the render step and using
     * this
     * render list allows us to avoid some level of repeated tree traversal at the cost of some
     * minimal
     * per frame allocation.  The rules for using the render list are that you need to add yourself
     * before
     * your dependencies do; the list is iterated in reverse during RunRenderTasks.  So a layer adds
     * itself
     * (if it is going to render offscreen) before it runs through its renderable list to prepare
     * each object
     * because it is during the renderable prepare traversale that subpresentations will get added
     * by
     * the offscreen render manager.
     */
    class IRenderList : public NVRefCounted
    {
    public:
        // Called by the render context, do not call this.
        virtual void BeginFrame() = 0;

        // Next tell all sub render target rendering systems to add themselves to the render list.
        // At this point
        // we agree to *not* have rendered anything, no clears or anything so if you are caching
        // render state and you detect nothing has changed it may not be necessary to swap egl
        // buffers.
        virtual QT3DSU32 AddRenderTask(IRenderTask &inTask) = 0;
        virtual void DiscardRenderTask(QT3DSU32 inTaskId) = 0;
        // This runs through the added tasks in reverse order.  This is used to render dependencies
        // before rendering to the main render target.
        virtual void RunRenderTasks() = 0;

        // We used to use GL state to pass information down the callstack.
        // I have replaced those calls with this state here because that information
        // controls how layers size themselves (which is quite a complicated process).
        virtual void SetScissorTestEnabled(bool enabled) = 0;
        virtual void SetScissorRect(NVRenderRect rect) = 0;
        virtual void SetViewport(NVRenderRect rect) = 0;
        virtual bool IsScissorTestEnabled() const = 0;
        virtual NVRenderRect GetScissor() const = 0;
        virtual NVRenderRect GetViewport() const = 0;

        static IRenderList &CreateRenderList(NVFoundationBase &inFnd);
    };

    // Now for scoped property access.
    template <typename TDataType>
    struct SRenderListScopedProperty
        : public qt3ds::render::NVRenderGenericScopedProperty<IRenderList, TDataType>
    {
        typedef qt3ds::render::NVRenderGenericScopedProperty<IRenderList, TDataType> TBaseType;
        typedef typename TBaseType::TGetter TGetter;
        typedef typename TBaseType::TSetter TSetter;
        SRenderListScopedProperty(IRenderList &ctx, TGetter getter, TSetter setter)
            : TBaseType(ctx, getter, setter)
        {
        }
        SRenderListScopedProperty(IRenderList &ctx, TGetter getter, TSetter setter,
                                  const TDataType &inNewValue)
            : TBaseType(ctx, getter, setter, inNewValue)
        {
        }
    };
}
}

#endif
