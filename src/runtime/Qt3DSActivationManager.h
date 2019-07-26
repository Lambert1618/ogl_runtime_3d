/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef QT3DS_ACTIVATION_MANAGER_H
#define QT3DS_ACTIVATION_MANAGER_H
#include "foundation/Qt3DSRefCounted.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Utils.h"
#include "Qt3DSKernelTypes.h"
#include "Qt3DSMetadata.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSFoundation.h"
#include "Qt3DSTimePolicy.h"

namespace Q3DStudio {
struct IComponentTimeOverrideFinishedCallback;
struct SComponentTimePolicyOverride;
class CPresentation;
}

namespace qt3ds {
namespace foundation {
    class IPerfTimer;
}
}

namespace qt3ds {
namespace render {
    class IThreadPool;
}
}

namespace qt3ds {
namespace runtime {
    using namespace qt3ds::foundation;
    using namespace qt3ds;
    using Q3DStudio::CTimePolicy;
    using Q3DStudio::SComponentTimePolicyOverride;
    using Q3DStudio::IComponentTimeOverrideFinishedCallback;
    using Q3DStudio::CPresentation;
    using Q3DStudio::TTimeUnit;
    using qt3ds::render::IThreadPool;

    class IActivationManager;

    typedef element::SElement &TActivityItem;
    typedef element::SElement *TActivityItemPtr;

    typedef eastl::pair<element::SElement *, QT3DSU32> TElementAndSortKey;

    typedef NVConstDataRef<TElementAndSortKey> TActivityItemBuffer;

    // Items can have user active and time active.  Time active is controlled by this object
    // but user active can be set.  An item is only active if it both actives are true.
    // Inactive items have completely inactive children.
    class IActivityZone
    {
    protected:
        virtual ~IActivityZone() {}
    public:
        virtual Q3DStudio::CPresentation &GetPresentation() = 0;

        // Items activated this cycle
        virtual TActivityItemBuffer GetActivatedItems() = 0;
        // Items deactivated this cycle
        virtual TActivityItemBuffer GetDeactivatedItems() = 0;
        // All active items that are script enabled.
        virtual TActivityItemBuffer GetScriptItems() = 0;

        // Inactive zones don't update their item lists when the manager updates.
        virtual void SetZoneActive(bool inActive) = 0;
        virtual bool IsZoneActive() = 0;

        virtual void AddActivityItems(TActivityItem root) = 0;

        // Get the time policy that is owned by this element.
        virtual CTimePolicy *GetOwnedTimePolicy(TActivityItem item) = 0;
        // Get the local time for this element.  This is the time the animation system should see
        virtual TTimeUnit GetItemLocalTime(TActivityItem item) = 0;
        // Get the local time if this isn't independent, else get the time context time for the
        // component.
        virtual TTimeUnit GetItemComponentTime(TActivityItem item) = 0;

        virtual SComponentTimePolicyOverride *
        GetOrCreateItemComponentOverride(TActivityItem item, float inMultiplier,
                                         TTimeUnit inEndTime,
                                         IComponentTimeOverrideFinishedCallback *inCallback) = 0;

        // If this item is independent, return this item.
        // else get time parent of my parent.
        virtual TActivityItemPtr GetItemTimeParent(TActivityItem item) = 0;

        bool IsIndependent(TActivityItem item) { return GetOwnedTimePolicy(item) != NULL; }

        // Any time the item has script flag changes after its initial creation.
        // Note that this flag cannot change due to slide changes, it can only change due
        // to something that had a script error and thus is disabled.
        virtual void UpdateItemScriptStatus(TActivityItem item) = 0;

        // If the start, end, or explicit active flags change *outside* of a slide change.
        virtual void UpdateItemInfo(TActivityItem item) = 0;
        virtual bool GetItemUserActive(TActivityItem item) = 0;

        // Callback when the slide changes, the time context needs to be rebuilt in this case.
        virtual void OnSlideChange(TActivityItem item) = 0;

        virtual bool GetItemTimeActive(TActivityItem item) = 0;

        virtual void BeginUpdate(QT3DSI64 inGlobalTime, IPerfTimer &inPerfTimer,
                                 IThreadPool &inThreadPool) = 0;
        virtual bool IsUpdating() = 0;
        virtual void EndUpdate() = 0;

        virtual void GoToTime(TActivityItem item, TTimeUnit inTime) = 0;

        virtual void setControlled(TActivityItem item) = 0;
    };

    class IActivityZoneManager : public NVRefCounted
    {
    protected:
        virtual ~IActivityZoneManager() {}
    public:
        virtual IActivityZone &CreateActivityZone(Q3DStudio::CPresentation &inPresentation) = 0;

        static IActivityZoneManager &CreateActivityZoneManager(NVFoundationBase &inFoundation,
                                                               IStringTable &inStrTable);
    };
}
}
#endif
