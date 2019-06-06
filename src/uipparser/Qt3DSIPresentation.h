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

#pragma once

#include "Qt3DSEventCallbacks.h"

namespace qt3ds {
namespace runtime {
    class IApplication;
    class IActivityZone;
    class IAnimationSystem;
    class ISlideSystem;
    class ILogicSystem;
    class IParametersSystem;
}
}

namespace qt3ds {
namespace foundation {
    class IStringTable;
}
}

namespace Q3DStudio {

class IComponentManager;
class IEventManager;
class IScene;
class IScriptBridge;
class CPresentationFrameData;
class CTimePolicy;
using qt3ds::runtime::ISlideSystem;
using qt3ds::runtime::IAnimationSystem;
using qt3ds::runtime::ILogicSystem;
using qt3ds::runtime::IParametersSystem;

// Method used to translate from presentation size to window size
enum EScaleMode {
    SCALEMODE_FREE, // Free scaling mode
    SCALEMODE_EXACT, // Fixed presentation size
    SCALEMODE_ASPECT, // Maintain aspect ratio
    SCALEMODE_UNKNOWN, // ERROR! - Uninitialized scale mode
};

struct SPresentationSize
{
    SPresentationSize()
        : m_Width(0)
        , m_Height(0)
        , m_ScaleMode(0)
    {
    }
    UINT16 m_Width; // Native width of the presentation
    UINT16 m_Height; // Native height of the presentation
    UINT8 m_ScaleMode; // Presentation to window scale method
    UINT8 m_Padding[3];
};

/**
 *	@interface	IPresentation
 *	Base interface of the presentation.
 */
class IPresentation
{
public: // Construction
    IPresentation() {}
    virtual ~IPresentation() {}

public: // Execution
    virtual void ClearDirtyList() = 0;
    virtual void PreUpdate(const TTimeUnit inGlobalTime) = 0;
    virtual void BeginUpdate() = 0;
    virtual void EndUpdate() = 0;
    virtual void PostUpdate(const TTimeUnit inGlobalTime) = 0;

public: // Bridge Control
    virtual void SetScene(IScene *inScene) = 0;
    virtual IScene *GetScene() const = 0;
    virtual IScriptBridge *GetScriptBridgeQml() = 0;

public: // Commands and Events
    virtual void FireEvent(const TEventCommandHash inEventType, TElement *inTarget,
                           const UVariant *inArg1 = nullptr, const UVariant *inArg2 = nullptr,
                           const EAttributeType inType1 = ATTRIBUTETYPE_NONE,
                           const EAttributeType inType2 = ATTRIBUTETYPE_NONE) = 0;
    virtual void FireCommand(const TEventCommandHash inEventType, TElement *inTarget,
                             const UVariant *inArg1 = nullptr, const UVariant *inArg2 = nullptr,
                             const EAttributeType inType1 = ATTRIBUTETYPE_NONE,
                             const EAttributeType inType2 = ATTRIBUTETYPE_NONE) = 0;
    virtual void FlushEventCommandQueue(void) = 0;
    virtual void ProcessEvent(SEventCommand &) = 0;

public: // Manager Access
    virtual IComponentManager &GetComponentManager() = 0;
    virtual ISlideSystem &GetSlideSystem() = 0;
    virtual IAnimationSystem &GetAnimationSystem() = 0;
    virtual ILogicSystem &GetLogicSystem() = 0;
    virtual IParametersSystem &GetParametersSystem() = 0;

    virtual qt3ds::foundation::IStringTable &GetStringTable() = 0;
    virtual qt3ds::runtime::IApplication &GetApplication() = 0;
    virtual qt3ds::runtime::IActivityZone *GetActivityZone() = 0;

public: // Hooks and callbacks
    virtual void OnPresentationLoaded() = 0;

public: // Full file path
    virtual void SetFilePath(const CHAR *inPath) = 0;
    virtual QString GetFilePath() const = 0;
    virtual QString getProjectPath() const = 0;
    virtual TElement *GetRoot() = 0;
    virtual void SetRoot(TElement &inRoot) = 0;

public: // Configuration access
    virtual SPresentationSize GetSize() const = 0;
    virtual void SetSize(const SPresentationSize &inSize) = 0;
    virtual void SetElementPath(TElement &inElement, const char8_t *inPath) = 0;
    virtual qt3ds::foundation::CRegisteredString GetElementPath(TElement &inElement) = 0;

public: // Event Callbacks
    virtual void RegisterEventCallback(TElement *inElement, const TEventCommandHash inEventHash,
                                       const TEventCallback inCallback, void *inContextData) = 0;
    virtual BOOL UnregisterEventCallback(TElement *inElement, const TEventCommandHash inEventHash,
                                         const TEventCallback inCallback, void *inContextData) = 0;

public: // FrameData access
    virtual CPresentationFrameData &GetFrameData() = 0;
};

} // namespace Q3DStudio
