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

#include "RuntimePrefix.h"
#include "Qt3DSIPresentation.h"
#include "Qt3DSPresentationFrameData.h"
#include "Qt3DSAnimationSystem.h"
#include "Qt3DSCircularArray.h"
#include "Qt3DSEventCallbacks.h"
#include "Qt3DSTimePolicy.h"
#include "EASTL/hash_map.h"
#include "foundation/StringTable.h"
#include "Qt3DSComponentManager.h"

#include <QObject>

class QPresentationSignalProxy : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void SigSlideEntered(const QString &elementPath, unsigned int index, const QString &name);
    void SigSlideExited(const QString &elementPath, unsigned int index, const QString &name);
    void SigCustomSignal(const QString &elementPath, const QString &name);
    void SigPresentationReady();
    void SigElementsCreated(const QStringList &elementPaths, const QString &error);
    void SigMaterialsCreated(const QStringList &materialNames, const QString &error);
    void SigDataOutputValueUpdated(const QString &name, const QVariant &value);
};

namespace qt3ds {
namespace runtime {
    class IApplication;
    class IActivityZone;
    struct DataOutputDef;
}
}

namespace qt3ds {
namespace render {
    class ILoadedBuffer;
}
}

namespace Q3DStudio {

/**
 *	Intelligent representation of a Studio presentation.
 */
class CPresentation : public IPresentation
{
protected:
    QString m_Name; // Name of this presentation
    QString m_FilePath; // Absolute path to this presentation
    QString m_projectPath; // Absolute path to the project root
    qt3ds::runtime::IApplication *m_Application; // Runtime object
    IScene *m_Scene; // Connection to the associated scene (render) for this presentation
    qt3ds::runtime::IActivityZone *m_ActivityZone; // Controls element active status
    TElement *m_RootElement;

    CPresentationFrameData m_FrameData; // Storage of data of the current frame
    CCircularArray<SEventCommand> m_EventCommandQueue; // The Event/Command integrated queue
    bool m_IsProcessingEventCommandQueue;
    CEventCallbacks m_EventCallbacks; // Handles event callbacks on registered elements
    SPresentationSize m_Size; // Native width, height and mode exported from Studio

    qt3ds::foundation::NVScopedRefCounted<qt3ds::render::ILoadedBuffer>
        m_LoadedBuffer; // Reference to loaded data when loading from binary

    CComponentManager m_ComponentManager;
    qt3ds::foundation::NVScopedRefCounted<ISlideSystem>
        m_SlideSystem; // Container and factory of all slides
    qt3ds::foundation::NVScopedRefCounted<ILogicSystem>
        m_LogicSystem; // Container and factory of all logics
    qt3ds::foundation::NVScopedRefCounted<IAnimationSystem>
        m_AnimationSystem; // Container and factory of all animation tracks
    qt3ds::foundation::NVScopedRefCounted<IParametersSystem>
        m_ParametersSystem; // Container and factory of all custom actions

    TTimeUnit m_Offset;
    TTimeUnit m_LocalTime;
    TTimeUnit m_PreviousGlobalTime;
    bool m_Paused;
    bool m_OffsetInvalid;
    bool m_Active;
    bool m_presentationReady = false;

    typedef eastl::hash_map<TElement *, qt3ds::foundation::CRegisteredString> TElemStringMap;
    TElemStringMap m_ElementPathMap;

public: // Construction
    CPresentation(const QString &inName, const QString &projectPath,
                  qt3ds::runtime::IApplication *inRuntime);
    virtual ~CPresentation() override;

public: // Execution
    void Initialize();
    // Clear dirty elements
    void ClearDirtyList() override;
    // Run events
    void PreUpdate(const TTimeUnit inGlobalTime) override;
    // update element graph
    void BeginUpdate() override;
    // end update element graph
    void EndUpdate() override;
    // Run behaviors.
    void PostUpdate(const TTimeUnit inGlobalTime) override;
    // Notify DataUpdates if any are registered to this presentation
    void NotifyDataOutputs();

public: // Bridge Control
    IScene *GetScene() const override;
    IScriptBridge *GetScriptBridgeQml() override;
    void SetScene(IScene *inScene) override;

    void SetActivityZone(qt3ds::runtime::IActivityZone *inZone);
    qt3ds::runtime::IActivityZone *GetActivityZone() override { return m_ActivityZone; }
    void SetActive(bool inValue);
    bool GetActive() const;
    TElement *GetRoot() override { return m_RootElement; }
    void SetRoot(TElement &inRoot) override { m_RootElement = &inRoot; }

public: // Commands and Events
    void FireEvent(const SEventCommand &inEvent);
    void FireEvent(const TEventCommandHash inEventType, TElement *inTarget,
                   const UVariant *inArg1 = nullptr, const UVariant *inArg2 = nullptr,
                   const EAttributeType inType1 = ATTRIBUTETYPE_NONE,
                   const EAttributeType inType2 = ATTRIBUTETYPE_NONE) override;
    void FireCommand(const TEventCommandHash inCommandType, TElement *inTarget,
                     const UVariant *inArg1 = nullptr, const UVariant *inArg2 = nullptr,
                     const EAttributeType inType1 = ATTRIBUTETYPE_NONE,
                     const EAttributeType inType2 = ATTRIBUTETYPE_NONE) override;
    void FlushEventCommandQueue(void) override;
    void ProcessEvent(SEventCommand &inEvent) override;

    QPresentationSignalProxy *signalProxy() { return &m_SignalProxy; }

public: // Data Output
    void AddToDataOutputMap(const QHash<qt3ds::foundation::CRegisteredString,
                            qt3ds::runtime::DataOutputDef> &doMap);

public: // Event Callbacks
    void RegisterEventCallback(TElement *inElement, const TEventCommandHash inEventHash,
                               const TEventCallback inCallback, void *inContextData) override;
    BOOL UnregisterEventCallback(TElement *inElement, const TEventCommandHash inEventHash,
                                 const TEventCallback inCallback, void *inContextData) override;

protected: // Execution Helper Methods
    void ProcessAllCallbacks();
    BOOL ProcessEventCommandQueue();
    void ProcessEvent(SEventCommand &ioEvent, INT32 &ioEventCount);
    void ProcessEventBubbling(SEventCommand &ioEvent, INT32 &ioEventCount);
    void ProcessCommand(const SEventCommand &inCommand);

public: // Managers
    IComponentManager &GetComponentManager() override;
    ISlideSystem &GetSlideSystem() override;
    IAnimationSystem &GetAnimationSystem() override;
    ILogicSystem &GetLogicSystem() override;
    IParametersSystem &GetParametersSystem() override;
    qt3ds::foundation::IStringTable &GetStringTable() override;
    qt3ds::runtime::IApplication &GetApplication() override
    {
        QT3DS_ASSERT(m_Application);
        return *m_Application;
    }
    void SetLoadedBuffer(qt3ds::render::ILoadedBuffer &inBuffer);

    void SetElementPath(TElement &inElement, const char8_t *inPath) override;
    qt3ds::foundation::CRegisteredString GetElementPath(TElement &inElement) override;

public: // Helpers
    CPresentationFrameData &GetFrameData() override;
    TTimeUnit GetTime() { return m_LocalTime; }

public: // Hooks and callbacks
    void OnPresentationLoaded() override;

public: // Configuration access
    SPresentationSize GetSize() const override;
    BOOL GetPause() const;
    const QByteArray GetName() const;

    void SetSize(const SPresentationSize &inSize) override;
    void SetPause(const BOOL inPause);
    void SetHide(const BOOL inHide);
    void SetUpdateLock(const BOOL inLockUpdate);
    void SetVCAA(const BOOL inVCAA);

public: // Full file paths
    void SetFilePath(const CHAR *inPath) override;
    QString GetFilePath() const override;
    QString getProjectPath() const override;

private: // Disabled Copy Construction
    CPresentation(CPresentation &);
    CPresentation &operator=(const CPresentation &);
private:
    QPresentationSignalProxy m_SignalProxy;
    QHash<qt3ds::foundation::CRegisteredString, qt3ds::runtime::DataOutputDef> m_pathToDataOutMap;
};

} // namespace Q3DStudio
