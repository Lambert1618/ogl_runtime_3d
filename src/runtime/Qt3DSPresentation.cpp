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

#include "Qt3DSPresentation.h"
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSIScriptBridge.h"
#include "Qt3DSInputEventTypes.h"
#include "Qt3DSDataLogger.h"
#include "Qt3DSApplication.h"
#include "Qt3DSRuntimeFactory.h"
#include "Qt3DSCommandHelper.h"
#include "Qt3DSActivationManager.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSComponentManager.h"
#include "Qt3DSRenderBufferLoader.h"
#include "Qt3DSSlideSystem.h"
#include "Qt3DSLogicSystem.h"
#include "Qt3DSParametersSystem.h"
#include "Qt3DSApplication.h"

#include <QtCore/qfileinfo.h>
#include <QtGui/qvector4d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector2d.h>

namespace Q3DStudio {

// Maximum number of Event/Command that can be queued in an Update cycle
const INT32 Q3DStudio_EVENTCOMMANDQUEUECAPACITY = 512;

// Limit to prevent infinite loop during queue processing
const INT32 Q3DStudio_MAXEVENTCOMMANDLOOPCOUNT = Q3DStudio_EVENTCOMMANDQUEUECAPACITY * 10;

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4355)
#endif

CPresentation::CPresentation(const QString &inName, const QString &projectPath,
                             qt3ds::runtime::IApplication *inApplication)
    : m_Name(inName)
    , m_Application(inApplication)
    , m_Scene(nullptr)
    , m_ActivityZone(nullptr)
    , m_RootElement(nullptr)
    , m_EventCommandQueue(Q3DStudio_EVENTCOMMANDQUEUECAPACITY, "EventCommandQueue")
    , m_IsProcessingEventCommandQueue(false)
    , m_ComponentManager(*this)
    , m_Offset(0)
    , m_LocalTime(0)
    , m_PreviousGlobalTime(-1)
    , m_Paused(false)
    , m_OffsetInvalid(true)
    , m_Active(true)
{
    m_projectPath = QFileInfo(projectPath).absoluteFilePath();
    m_Size.m_Width = 0;
    m_Size.m_Height = 0;
    m_Size.m_ScaleMode = SCALEMODE_UNKNOWN;
    m_AnimationSystem = IAnimationSystem::CreateAnimationSystem(
        inApplication->GetRuntimeFactoryCore().GetFoundation());
    m_SlideSystem =
        ISlideSystem::CreateSlideSystem(inApplication->GetRuntimeFactoryCore().GetFoundation(),
                                        inApplication->GetRuntimeFactoryCore().GetStringTable(),
                                        inApplication->GetElementAllocator());
    m_LogicSystem =
        ILogicSystem::CreateLogicSystem(inApplication->GetRuntimeFactoryCore().GetFoundation());
    m_ParametersSystem = IParametersSystem::CreateParametersSystem(
        inApplication->GetRuntimeFactoryCore().GetFoundation());

    // Signal proxy thread affinity is set later when signals are connected to ensure it is correct
    m_SignalProxy.moveToThread(nullptr);
}
#ifdef _WIN32
#pragma warning(pop)
#endif

CPresentation::~CPresentation()
{
}

/**
 *	Registers an element for notification when events fired on it.
 *	@param inElement		target element to monitor
 *	@param inEventHash		event hash to register for
 *	@param inCallback		static callback function
 *	@param inContextData	arbitrary data pointer
 */
void CPresentation::RegisterEventCallback(TElement *inElement, const TEventCommandHash inEventHash,
                                          const TEventCallback inCallback, void *inContextData)
{
    m_EventCallbacks.RegisterCallback(inElement, inEventHash, inCallback, inContextData);
    inElement->SetFlag(ELEMENTFLAG_REGISTEREDFOREVENTCALLBACK, true);

    if (qt3ds::runtime::IApplication::isPickingEvent(inEventHash))
        inElement->SetFlag(ELEMENTFLAG_PICKENABLED, true);
}

/**
 *	Unregisters a previously registered event callback.
 *	@param inElement		target element to monitor
 *	@param inEventHash		event hash to register for
 *	@param inCallback		static callback function
 *	@param inContextData	arbitrary data pointer
 */
BOOL CPresentation::UnregisterEventCallback(TElement *inElement,
                                            const TEventCommandHash inEventHash,
                                            const TEventCallback inCallback, void *inContextData)
{
    BOOL theLast = false;
    BOOL theResult = m_EventCallbacks.UnregisterCallback(inElement, inEventHash, inCallback,
                                                         inContextData, theLast);

    // Unflag element if there are no longer any callbacks on it
    if (theLast)
        inElement->SetFlag(ELEMENTFLAG_REGISTEREDFOREVENTCALLBACK, false);

    if (qt3ds::runtime::IApplication::isPickingEvent(inEventHash))
        inElement->SetFlag(ELEMENTFLAG_PICKENABLED, false);

    return theResult;
}

void CPresentation::ClearDirtyList()
{
    FOR_ARRAY(TElement *, theElement, GetFrameData().GetDirtyList())
    {
        (*theElement)->Flags().SetDirty(false);
    }
    GetFrameData().Reset();
}

inline void ConvertActivityZoneBufferToElementList(qt3ds::runtime::TActivityItemBuffer inSource,
                                                   TElementList &outResult)
{
    for (qt3ds::QT3DSU32 idx = 0, end = inSource.size(); idx < end; ++idx)
        outResult.Push(inSource[idx].first);
}

void CPresentation::PreUpdate(const TTimeUnit inGlobalTime)
{
    if (m_OffsetInvalid || m_Paused) {
        m_OffsetInvalid = false;
        m_Offset = m_LocalTime - inGlobalTime;
    } else
        m_LocalTime = inGlobalTime + m_Offset;

    // Event/Command Processing Stage
    ProcessEventCommandQueue();
}

/**
 *	Update the presentation to the current time. This will start triggering the
 *	various stages of the presentation frame rhythm
 *	@param	inGlobalTime		time at which to update each presentation
 *	@return true if there were events to be processed.
 */
void CPresentation::BeginUpdate()
{
    // Active Scan Stage
    if (m_ActivityZone) {
        m_ActivityZone->BeginUpdate(
            m_LocalTime, m_Application->GetRuntimeFactory().GetPerfTimer(),
            m_Application->GetRuntimeFactory().GetQt3DSRenderContext().GetThreadPool());
    }
}

void CPresentation::EndUpdate()
{
    if (m_ActivityZone) {
        m_ActivityZone->EndUpdate();
        CPresentationFrameData &theFrameData = GetFrameData();
        ConvertActivityZoneBufferToElementList(m_ActivityZone->GetActivatedItems(),
                                               theFrameData.GetActivationList());
        ConvertActivityZoneBufferToElementList(m_ActivityZone->GetDeactivatedItems(),
                                               theFrameData.GetDeactivationList());
        ConvertActivityZoneBufferToElementList(m_ActivityZone->GetScriptItems(),
                                               theFrameData.GetScriptsList());
    }

    if (!m_Paused) {
        // Animation Track Evaluation Stage
        m_AnimationSystem->Update();
    }
    // Presentation is considered ready to accept external commands when the first frame property
    // updates are done.
    if (!m_presentationReady) {
        m_SignalProxy.SigPresentationReady();
        m_presentationReady = true;
    }
}

void CPresentation::PostUpdate(const TTimeUnit inGlobalTime)
{
    if (!m_Paused) {
        // Callback Stage
        if (m_Application && m_PreviousGlobalTime != inGlobalTime)
            m_Application->GetRuntimeFactoryCore().GetScriptEngineQml().ProcessFrameCallbacks(this);
    }

    m_PreviousGlobalTime = inGlobalTime;
}

void CPresentation::NotifyDataOutputs()
{
    if (m_pathToDataOutMap.size() == 0)
        return;

    // Based on the dirty list, check if we need to fire DataOutput notifications
    Q3DStudio::TElementList &dirtyList = GetFrameData().GetDirtyList();
    for (int idx = 0, end = dirtyList.GetCount(); idx < end; ++idx) {
        Q3DStudio::TElement &element = *dirtyList[idx];
        if (m_pathToDataOutMap.contains(element.path())) {
            auto outDefIter = m_pathToDataOutMap.find(element.path());

            while (outDefIter != m_pathToDataOutMap.end() && outDefIter.key() == element.path()) {
                qt3ds::runtime::DataOutputDef &outDef = outDefIter.value();

                // Get current value
                Q3DStudio::UVariant value;
                qt3ds::QT3DSU32 attribHash = CHash::HashAttribute(
                            outDef.observedAttribute.attributeName[0].constData());
                element.GetAttribute(attribHash, value);
                QVariant qvar;
                switch (outDef.observedAttribute.propertyType) {
                case ATTRIBUTETYPE_INT32:
                    qvar.setValue(value.m_INT32);
                    break;
                case ATTRIBUTETYPE_FLOAT:
                    qvar.setValue(value.m_FLOAT);
                    break;
                case ATTRIBUTETYPE_BOOL:
                    qvar.setValue(value.m_INT32);
                    break;
                case ATTRIBUTETYPE_STRING:
                    qvar.setValue(QString::fromUtf8(
                                      GetStringTable().HandleToStr(value.m_StringHandle).c_str()));
                    break;
                case ATTRIBUTETYPE_FLOAT4: {
                    QVector4D qvalue(value.m_FLOAT4[0], value.m_FLOAT4[1],
                                     value.m_FLOAT4[2], value.m_FLOAT4[3]);
                    qvar.setValue(qvalue);
                }
                    break;
                case ATTRIBUTETYPE_FLOAT3: {
                    QVector3D qvalue(value.m_FLOAT3[0], value.m_FLOAT3[1], value.m_FLOAT3[2]);
                    qvar.setValue(qvalue);
                }
                    break;
                case ATTRIBUTETYPE_FLOAT2: {
                    QVector2D qvalue(value.m_FLOAT3[0], value.m_FLOAT3[1]);
                    qvar.setValue(qvalue);
                }
                    break;
                default:
                    break;
                }

                if (qvar.isValid() && (outDef.value != qvar)) {
                    outDef.value.setValue(qvar);
                    m_SignalProxy.SigDataOutputValueUpdated(outDef.name, outDef.value);
                }

                ++outDefIter;
            }
        }
    }
}

void CPresentation::AddToDataOutputMap(const QHash<qt3ds::foundation::CRegisteredString,
                                       qt3ds::runtime::DataOutputDef> &doMap)
{
    if (doMap.size() > 0)
        m_pathToDataOutMap.unite(doMap);
}

/**
 *	Process the Event/Command queue completely
 *	PostEventCommand is the method that will add new Event/Command to the queue.
 *
 *	The contract for Event/Command processing:
 *	1. If an Event or Command is posted during the processing stage, it will be added
 *	to the end of the queue and is guaranteed to be processed in the current cycle.
 *	For example, if an Event (which is processed) triggers a Command to set a model to red,
 *	the model will turn red in the current frame.
 *
 *	2. If an Event or Command is posted after the processing stage, such as during the
 *	Callback stage, it will only be processed on the next Update cycle.
 *	For example, if the "turn mode to red" Command is posted in a script callback, the
 *	model will turn red in the next frame.
 *
 *	3. If an Event or Command is posted before the processing stage, for example an
 *	external Event posted just before the Update cycle, it will be processed in the current
 *cycle.
 *	For example, if the "turn mode to red" Command is posted from the game engine prior to
 *	the Update cycle, the model will turn red in the current frame.
 *
 *	@see PostEventCommand
 *	@return true if there were events that were processed.
 */
BOOL CPresentation::ProcessEventCommandQueue()
{
    PerfLogGeneralEvent2(DATALOGGER_PROCESSEVENTS);

    m_IsProcessingEventCommandQueue = true;

    BOOL theResult = !m_EventCommandQueue.IsEmpty();
    INT32 theEventProcessedCount = 0;
    while (!m_EventCommandQueue.IsEmpty()) {
        SEventCommand &theEventCommand = m_EventCommandQueue.Top();
        if (theEventCommand.m_IsEvent)
            ProcessEvent(theEventCommand, theEventProcessedCount);
        else
            ProcessCommand(theEventCommand);

        // Infinite-loop check
        if (theEventProcessedCount > Q3DStudio_MAXEVENTCOMMANDLOOPCOUNT) {
            // Breakout policy: Dump remaining Event/Commands
            qCCritical(qt3ds::INVALID_OPERATION)
                    << "ProcessEventCommandQueue exceeded maximum loop count"
                    << "Remaining Event/Commands will be cleared. Event count: "
                    << theEventProcessedCount << "Limit: " << Q3DStudio_MAXEVENTCOMMANDLOOPCOUNT;
            m_EventCommandQueue.Clear();
        } else {
            m_EventCommandQueue.Pop();
        }
    }

    m_IsProcessingEventCommandQueue = false;

    return theResult;
}

/**
 *	Pass incoming event to event consumers immediately
 *	@param ioEvent		the incoming event
 */
void CPresentation::ProcessEvent(SEventCommand &ioEvent, INT32 &ioEventCount)
{
    if (ioEventCount < Q3DStudio_MAXEVENTCOMMANDLOOPCOUNT) {
        ++ioEventCount;
        PerfLogPresentationEvent1(DATALOGGER_PROCESSEVENT);

        // Callbacks can change ioEvent's bubbling flags
        if (ioEvent.m_Target->GetFlag(ELEMENTFLAG_REGISTEREDFOREVENTCALLBACK)) {
            m_EventCallbacks.FireCallbacks(ioEvent);
        }

        // Do an early return if callback do a "stopImmediatePropagation"
        if (ioEvent.m_Done)
            return;

        if (ioEvent.m_Target) {
            // Logic should not be able to change ioEvent's bubbling flags
            // ...or can it?
            m_LogicSystem->OnEvent(ioEvent.m_Type, *ioEvent.m_Target, *this);
        }

        ProcessEventBubbling(ioEvent, ioEventCount);
    }
}

/**
 *	Handle event bubbling
 *	@param ioEvent		the incoming event
 */
void CPresentation::ProcessEventBubbling(SEventCommand &ioEvent, INT32 &ioEventCount)
{
    PerfLogPresentationEvent1(DATALOGGER_PROCESSEVENTBUBBLING);

    // Check for onGroupedMouseOver/Out
    // arg1 = the original onMouseOut model and arg2 = the original onMouseOver model
    if (ioEvent.m_Type == ON_MOUSEOUT) {
        // If original onMouseOver model is nullptr or not a descendent, fire onGroupedMouseOut
        TElement *theMouseOverModel = static_cast<TElement *>(ioEvent.m_Arg2.m_VoidPointer);
        if (!theMouseOverModel || !ioEvent.m_Target->IsDescendent(*theMouseOverModel)) {
            SEventCommand theEvent = ioEvent;
            theEvent.m_Type = ON_GROUPEDMOUSEOUT;
            theEvent.m_BubbleUp = 0; // no bubbling
            theEvent.m_BubbleDown = 0;
            FireEvent(theEvent);
        }
        // set the original onMouseOver model to the target to make IsDescendent queries less
        // expensive
        else if (theMouseOverModel) {
            ioEvent.m_Arg2.m_VoidPointer = ioEvent.m_Target;
        }
    } else if (ioEvent.m_Type == ON_MOUSEOVER) {
        // If original onMouseOut model is nullptr or not a descendent, fire onGroupedMouseOver
        TElement *theMouseOutModel = static_cast<TElement *>(ioEvent.m_Arg1.m_VoidPointer);
        if (!theMouseOutModel || !ioEvent.m_Target->IsDescendent(*theMouseOutModel)) {
            SEventCommand theEvent = ioEvent;
            theEvent.m_Type = ON_GROUPEDMOUSEOVER;
            theEvent.m_BubbleUp = 0; // no bubbling
            theEvent.m_BubbleDown = 0;
            FireEvent(theEvent);
        }
        // set the original onMouseOut model to the target to make IsDescendent queries less
        // expensive
        else if (theMouseOutModel) {
            ioEvent.m_Arg1.m_VoidPointer = ioEvent.m_Target;
        }
    }

    // Do NOT bubble up onSlideEnter and onSlideExit events from current component to its parent
    // scene
    // since each component has its own slides.
    if (ioEvent.m_BubbleUp && (ioEvent.m_Target->Flags() & ELEMENTFLAG_COMPONENT)
        && (ioEvent.m_Type == EVENT_ONSLIDEENTER || ioEvent.m_Type == EVENT_ONSLIDEEXIT)) {
        ioEvent.m_BubbleUp = FALSE;
    }

    // Event bubbling
    if (ioEvent.m_BubbleUp) {
        TElement *theParent = ioEvent.m_Target->GetParent();
        if (theParent) {
            ioEvent.m_Target = theParent;
            ProcessEvent(ioEvent, ioEventCount);
        }
    }
}

/**
 *	Execute command immediately
 *	@param inCommand		incoming command structure
 */
void CPresentation::ProcessCommand(const SEventCommand &inCommand)
{
    PerfLogPresentationEvent1(DATALOGGER_PROCESSCOMMAND);

    // Attributes	(Arg1 = key, Arg2 = value)
    if (inCommand.m_Type == COMMAND_SETPROPERTY) {
        SAttributeKey theAttributeKey;
        UINT32 theHash = static_cast<UINT32>(inCommand.m_Arg1.m_Hash);
        theAttributeKey.Convert(
            theHash); // Need this conversion to prevent problems arising due to endianess
        inCommand.m_Target->SetAttribute(theAttributeKey.m_Hash, inCommand.m_Arg2);

    // Events		(Arg1 = hashed event name)
    } else if (inCommand.m_Type == COMMAND_FIREEVENT) {
        FireEvent(inCommand.m_Arg1.m_Hash, inCommand.m_Target);

    // Time			(Arg1 = time)
    } else if (inCommand.m_Type == COMMAND_PLAY) {
        GetComponentManager().SetPause(inCommand.m_Target, false);
    } else if (inCommand.m_Type == COMMAND_PAUSE) {
        GetComponentManager().SetPause(inCommand.m_Target, true);
    } else if (inCommand.m_Type == COMMAND_GOTOTIME) {
        GetComponentManager().GoToTime(inCommand.m_Target, inCommand.m_Arg1.m_INT32);

    // Slide		(Arg1 = slide index or slide name)
    } else if (inCommand.m_Type == COMMAND_GOTOSLIDE) {
        // Goto slide commands are handled differently.
        IComponentManager &theManager(GetComponentManager());
        Q3DStudio::SComponentGotoSlideData theGotoSlideData =
            theManager.GetComponentGotoSlideCommand(inCommand.m_Target);
        if (theGotoSlideData.m_Slide > 0)
            theManager.GotoSlideIndex(inCommand.m_Target, theGotoSlideData);

        theManager.ReleaseComponentGotoSlideCommand(inCommand.m_Target);
    } else if (inCommand.m_Type == COMMAND_GOTOSLIDENAME) {
        GetComponentManager().GotoSlideName(inCommand.m_Target, inCommand.m_Arg1.m_Hash);
    } else if (inCommand.m_Type == COMMAND_GOTONEXTSLIDE) {
        GetComponentManager().GoToNextSlide(inCommand.m_Target);
    } else if (inCommand.m_Type == COMMAND_GOTOPREVIOUSSLIDE) {
        GetComponentManager().GoToPreviousSlide(inCommand.m_Target);
    } else if (inCommand.m_Type == COMMAND_BACKSLIDE) {
        GetComponentManager().GoToBackSlide(inCommand.m_Target);

    // Behavior
    } else if (inCommand.m_Type == COMMAND_CUSTOMACTION) {
        m_Application->GetRuntimeFactoryCore().GetScriptEngineQml()
                                              .ProcessCustomActions(this, inCommand);
    } else if (inCommand.m_Type == COMMAND_CUSTOMCALLBACK) {
        m_Application->GetRuntimeFactoryCore().GetScriptEngineQml().ProcessCustomCallback(
                    this, inCommand);
    } else if (inCommand.m_Type == COMMAND_PLAYSOUND) {
        CRegisteredString theSoundPathReg = GetStringTable().HandleToStr(inCommand.m_Arg1.m_INT32);
        if (theSoundPathReg.IsValid()) {
            const char *theSoundPath = theSoundPathReg.c_str();
            if (strlen(theSoundPath) > 0) {
                m_Application->GetRuntimeFactoryCore().GetScriptEngineQml().PlaySoundFile(
                            theSoundPath);
            }
        }
    } else if (inCommand.m_Type == COMMAND_EMITSIGNAL) {
        CRegisteredString nameStr = GetStringTable().HandleToStr(inCommand.m_Arg1.m_INT32);
        m_Application->GetRuntimeFactoryCore().GetScriptEngineQml().ProcessSignal(this, inCommand);
        QString path = QString::fromLatin1(inCommand.m_Target->path().c_str());
        QString name = QString::fromLatin1(nameStr.c_str());
        signalProxy()->SigCustomSignal(path, name);
    } else {
        qCCritical(qt3ds::INVALID_OPERATION) << "Command not implemented: " << inCommand.m_Type;
    }
}

/**
 *	Put an Event in the queue to be processed later during the Event/Command
 *	processing stage in Update
 *	This method is used by Event-bubbling during ProcessEvent
 *
 *	@param inEvent		the incoming event
 *	@see PostEvent
 */
void CPresentation::FireEvent(const SEventCommand &inEvent)
{
    SEventCommand &theEventCommand = m_EventCommandQueue.NewEntry();

    theEventCommand = inEvent;

    theEventCommand.m_IsEvent = true;
}

/**
 *	Put an Event in the queue to be processed later during the Event/Command
 *	processing stage in Update See ProcessEventCommandQueue for more
 *	information on the contract for Event/Command processing.
 *
 *	@param inEventType		the incoming event type
 *	@param inTarget			the target for the event
 *	@param inArg1			optional argument #1
 *	@param inArg2			optional argument #2
 *	@param inType1			optional type for argument #1
 *	@param inType2			optional type for argument #2
 *	@see ProcessEventCommandQueue
 *	@see PostEventCommand
 */
void CPresentation::FireEvent(const TEventCommandHash inEventType, TElement *inTarget,
                              const UVariant *inArg1, const UVariant *inArg2,
                              const EAttributeType inType1, const EAttributeType inType2)
{

    SEventCommand theEvent = { inTarget, inEventType };

    theEvent.m_IsEvent = true;
    theEvent.m_BubbleUp = true;

    theEvent.m_Arg1Type = static_cast<UINT8>(inType1);
    theEvent.m_Arg2Type = static_cast<UINT8>(inType2);

    if (inArg1)
        theEvent.m_Arg1 = *inArg1;
    if (inArg2)
        theEvent.m_Arg2 = *inArg2;

    m_EventCommandQueue.NewEntry() = theEvent;
}

/**
 *	Put a Command in the queue to be processed later during the Event/Command
 *	processing stage in Update. See ProcessEventCommandQueue for more information
 *	on the contract for Event/Command processing.
 *
 *	For cases where the Commands need to be synchronized with the Frame-Rhythm
 *	An example would be calling "SetSlide" in scripts.
 *
 *	@see ProcessEventCommandQueue
 *	@see PostEventCommand
 *	@param inEventType		the incoming command type
 *	@param inTarget			the target for the command
 *	@param inArg1			optional argument #1
 *	@param inArg2			optional argument #2
 *	@param inType1			optional type for argument #1
 *	@param inType2			optional type for argument #2
 */
void CPresentation::FireCommand(const TEventCommandHash inEventType, TElement *inTarget,
                                const UVariant *inArg1, const UVariant *inArg2,
                                const EAttributeType inType1, const EAttributeType inType2)
{
    // Pre-filter gotoslidename commands.
    if (inEventType == COMMAND_GOTOSLIDENAME) {
        int theSlideHashName = inArg1->m_INT32;
        TComponent *theComponent = GetComponentManager().GetComponent(inTarget);
        UINT8 theSlideIndex = GetSlideSystem().FindSlide(*theComponent, theSlideHashName);
        // Translate into a slide index command.
        CCommandHelper::SetupGotoSlideCommand(*inTarget, theSlideIndex,
                                              SScriptEngineGotoSlideArgs());
    } else {
        SEventCommand theCommand = { inTarget, inEventType };

        theCommand.m_Arg1Type = static_cast<UINT8>(inType1);
        theCommand.m_Arg2Type = static_cast<UINT8>(inType2);

        if (inArg1)
            theCommand.m_Arg1 = *inArg1;
        if (inArg2)
            theCommand.m_Arg2 = *inArg2;

        m_EventCommandQueue.NewEntry() = theCommand;
    }
}

void CPresentation::FlushEventCommandQueue(void)
{
    if (!m_IsProcessingEventCommandQueue)
        ProcessEventCommandQueue();
}

void CPresentation::ProcessEvent(SEventCommand &inEvent)
{
    INT32 theEventProcessedCount = 0;
    ProcessEvent(inEvent, theEventProcessedCount);
}

/**
 *	This method is triggered after the presentation is streamed in. At this point,
 *	all the stores will be loaded up.
 */
void CPresentation::OnPresentationLoaded()
{
    m_FrameData.Reserve(1000 /*m_ElementManager.GetElementCount( )*/);
}

/**
 *	Set the full path for this presentation. This can be used by anyone who
 *	knows the presentation to form relative paths.
 *	@param inPath		the path to which to set.
 */
void CPresentation::SetFilePath(const CHAR *inPath)
{
    m_FilePath = QFileInfo(inPath).absoluteFilePath();
}

/**
 *	Gets the full file path for this presentation. This can be used by anyone who
 *	knows the presentation to form relative correct paths.
 */
QString CPresentation::GetFilePath() const
{
    return m_FilePath;
}

/**
 *  Gets the absolute file path for the project that owns this presentation.
 */
QString CPresentation::getProjectPath() const
{
    return m_projectPath;
}

/**
 *	Gets the pause state
 *	@return true if the presentation is paused, false if otherwise
 */
BOOL CPresentation::GetPause() const
{
    return m_Paused;
}

/**
 *	Sets the pause state
 *	@param inPause		set true to pause, set false if otherwise
 */
void CPresentation::SetPause(const BOOL inPause)
{
    m_Paused = inPause ? true : false;
}

/**
 *	Simple manager access: Scene
 */
void CPresentation::SetScene(IScene *inScene)
{
    m_Scene = inScene;
}

void CPresentation::SetActivityZone(qt3ds::runtime::IActivityZone *inZone)
{
    m_ActivityZone = inZone;
    m_ActivityZone->SetZoneActive(m_Active);
    TElement *theSceneElement = m_RootElement;
    qt3ds::runtime::IActivityZone &theZone(*GetActivityZone());
    // The activity zone requires elements described to it in breadth first search order.
    theZone.AddActivityItems(*theSceneElement);
}

void CPresentation::SetActive(bool inValue)
{
    m_Active = inValue;
    m_ActivityZone->SetZoneActive(m_Active);
}

bool CPresentation::GetActive() const
{
    return m_Active;
}

/**
 *	Simple manager access: Scene
 */
IScene *CPresentation::GetScene() const
{
    return m_Scene;
}

/**
*	Simple manager access: Script Bridge Qml
*/
IScriptBridge *CPresentation::GetScriptBridgeQml()
{
    if (m_Application)
        return &m_Application->GetRuntimeFactoryCore().GetScriptEngineQml();

    return nullptr;
}

/**
 *	Simple manager access: Component Manager
 */
IComponentManager &CPresentation::GetComponentManager()
{
    return m_ComponentManager;
}

/**
 *	Simple manager access: Slide Manager
 */
ISlideSystem &CPresentation::GetSlideSystem()
{
    return *m_SlideSystem;
}

/**
 *	Simple manager access: Animation Manager
 */
qt3ds::runtime::IAnimationSystem &CPresentation::GetAnimationSystem()
{
    return *m_AnimationSystem;
}

/**
 *	Simple manager access: Logic Manager
 */
ILogicSystem &CPresentation::GetLogicSystem()
{
    return *m_LogicSystem;
}

/**
 *	Simple manager access: Params Manager
 */
IParametersSystem &CPresentation::GetParametersSystem()
{
    return *m_ParametersSystem;
}

void CPresentation::SetElementPath(TElement &inElement, const char8_t *inPath)
{
    CRegisteredString str;
    if (m_Application)
        str = m_Application->GetRuntimeFactoryCore().GetStringTable().RegisterStr(
            qt3ds::foundation::nonNull(inPath));

    if (str.IsValid())
        m_ElementPathMap.insert(eastl::make_pair(&inElement, str));
}

qt3ds::foundation::CRegisteredString CPresentation::GetElementPath(TElement &inElement)
{
    TElemStringMap::iterator iter = m_ElementPathMap.find(&inElement);
    if (iter != m_ElementPathMap.end())
        return iter->second;

    return qt3ds::foundation::CRegisteredString();
}

qt3ds::foundation::IStringTable &CPresentation::GetStringTable()
{
    return GetApplication().GetRuntimeFactoryCore().GetStringTable();
}

/**
 *	Current frame data stores traversal lists for use later
 */
CPresentationFrameData &CPresentation::GetFrameData()
{
    return m_FrameData;
}

void CPresentation::SetLoadedBuffer(qt3ds::render::ILoadedBuffer &inBuffer)
{
    m_LoadedBuffer = inBuffer;
}

/**
 *	Retrieve the name of the presentation. This is actually the file path.
 *	@return the name of this presentation
 */
const QByteArray CPresentation::GetName() const
{
    return m_Name.toLatin1();
}

/**
 *	Retrieve the size of the presentation in Studio
 *	@return the size of this presentation
 */
SPresentationSize CPresentation::GetSize() const
{
    return m_Size;
}

/**
 *	Set the size of the presentation as reflected in Studio
 *	@param inSize		size of presentation ( width, height, scale mode ) in Studio
 */
void CPresentation::SetSize(const SPresentationSize &inSize)
{
    m_Size = inSize;
}

} // namespace Q3DStudio
