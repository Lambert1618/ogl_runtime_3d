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

#include "RuntimePrefix.h"
#ifdef EA_PLATFORM_WINDOWS
#pragma warning(disable : 4396) // specializer warning nonsense
#endif

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSUIPParserActionHelper.h"
#include "Qt3DSUIPParserObjectRefHelper.h"
#include "Qt3DSIPresentation.h"
#include "Qt3DSCommandEventTypes.h"

#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "Qt3DSMetadata.h"
#include "Qt3DSEulerAngles.h"
#include "Qt3DSLogicSystem.h"
#include "Qt3DSParametersSystem.h"

using namespace qt3dsdm;

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 * Constructor
 */
CUIPParserActionHelper::CUIPParserActionHelper(CUIPParserImpl *inUIPParser,
                                               CUIPParserObjectRefHelper *inObjRefHelper,
                                               IRuntimeMetaData &inMetaData)
    : m_UIPParser(inUIPParser)
    , m_ObjRefHelper(inObjRefHelper)
    , m_MetaData(inMetaData)
{
}

//==============================================================================
/**
 * Destructor
 */
CUIPParserActionHelper::~CUIPParserActionHelper()
{
}

void CUIPParserActionHelper::CacheAction(qt3dsdm::IDOMReader &inReader,
                                         const char8_t *inActionOwnerId)
{
    if (IsTrivial(inActionOwnerId))
        return;

    IDOMReader::Scope __childScope(inReader);

    const char *theRef;
    if (inReader.Att("ref", theRef)) {
        // Ignore "ref" because we are only interested in new actions
        return;
    }

    // Parse the action
    SActionInfo theAction;
    inReader.Att("id", theAction.m_Id);
    theAction.m_Owner = inActionOwnerId;

    const char *tempStr;
    if (inReader.Att("triggerObject", tempStr))
        theAction.m_TriggerObject = m_UIPParser->ParseObjectRefId(tempStr, inActionOwnerId);
    inReader.Att("event", theAction.m_Event);
    if (inReader.Att("targetObject", tempStr))
        theAction.m_TargetObject = m_UIPParser->ParseObjectRefId(tempStr, inActionOwnerId);
    if (inReader.Att("handler", tempStr)) {
        theAction.m_Handler = tempStr;
        CacheHandlerArguments(inReader, theAction);
    }

    // Only proceeds if we can find the elements
    if (theAction.m_TargetObject.empty() || theAction.m_TriggerObject.empty()
        || theAction.m_Owner.empty()) {
        qCWarning(qt3ds::INVALID_OPERATION) << "Action invalid, id: " << theAction.m_Id.c_str();
        return;
    }

    m_ActionMap[theAction.m_Id] = theAction;

    // Build up logic tables to be processed by BuildAction later
    // There are 2 tables involved:
    // 1. The Listener-To-Events tables - multimap of each listener to all events it is listening to
    // 2. The Listener-Event-To-Actions table - multimap of each listener-event pair to all actions
    // it triggers
    // note: The struct for Action is a pair of EventAction node and its owner
    AddListenerEventPair(theAction.m_TriggerObject, theAction.m_Event);

    SListenerEventNamePair theListenerEventPair(theAction.m_TriggerObject, theAction.m_Event);
    TEventActionOwnerPair theLogicOwnerPair(theAction.m_Id, theAction.m_Owner);
    m_ListenerEventActionsMap.insert(eastl::make_pair(theListenerEventPair, theLogicOwnerPair));
}

void CUIPParserActionHelper::AddListenerEventPair(eastl::string inListener,
                                                  const eastl::string &inEventName)
{
    // Check for dupes
    TListenerEventsNameMap::iterator theStartItr = m_ListenerEventsNameMap.lower_bound(inListener);
    TListenerEventsNameMap::iterator theEndItr = m_ListenerEventsNameMap.upper_bound(inListener);
    for (; theStartItr != theEndItr; ++theStartItr) {
        if (theStartItr->second == inEventName)
            return;
    }

    m_ListenerEventsNameMap.insert(eastl::make_pair(inListener, inEventName));
}

void CUIPParserActionHelper::CacheHandlerArguments(qt3dsdm::IDOMReader &inReader,
                                                   SActionInfo &inAction)
{
    IDOMReader::Scope __handlerArgScope(inReader);
    for (bool success = inReader.MoveToFirstChild(); success;
         success = inReader.MoveToNextSibling()) {
        SHandlerArgumentInfo theArg;
        inReader.Att("name", theArg.m_Name);
        inReader.Att("value", theArg.m_Value);
        inAction.m_HandlerArgs.push_back(theArg);
    }
    if (inAction.m_Handler == "Set Property") {
        SHandlerArgumentInfo theHandlerArg1 = inAction.m_HandlerArgs[0];
        SElementData *targetData = m_UIPParser->GetElementData(inAction.m_TargetObject.c_str());
        if (targetData && !theHandlerArg1.m_Value.empty()) {
            m_UIPParser->m_ParseElementManager.MarkAttributeAsReferenced(
                *targetData, theHandlerArg1.m_Value.c_str());
        }
    }
}

SElement *CUIPParserActionHelper::GetElement(const eastl::string &inElementName)
{
    return m_UIPParser->GetElement(inElementName.c_str());
}

void CUIPParserActionHelper::GetActionSectionCount(
    CUIPParserImpl::SActionSectionCount &outActionCount)
{
    // There are 2 tables involved:
    // 1. The Listener-To-Events tables - multimap of each listener to all events it is listening to
    // 2. The Listener-Event-To-Actions table - multimap of each listener-event pair to all actions
    // it triggers
    // note: The struct for Action is a pair of EventAction node and its owner

    // Get the count to reserve
    // Iterate through each listener
    for (TListenerEventsNameMap::iterator theListenerItr = m_ListenerEventsNameMap.begin();
         theListenerItr != m_ListenerEventsNameMap.end();) {
        ++outActionCount.m_TriggerElementCount;

        INT32 theEventCount =
            static_cast<INT32>(m_ListenerEventsNameMap.count(theListenerItr->first));
        for (INT32 theEventCounter = 0; theEventCounter < theEventCount;
             ++theEventCounter, ++theListenerItr) {
            ++outActionCount.m_TriggerEventCount;

            // iterate through each listener-event pair
            SListenerEventNamePair thePair(theListenerItr->first, theListenerItr->second);
            TListenerEventActionsMap::iterator theActionsItr =
                m_ListenerEventActionsMap.lower_bound(thePair);
            INT32 theActionCount = static_cast<INT32>(m_ListenerEventActionsMap.count(thePair));
            for (INT32 theActionCounter = 0; theActionCounter < theActionCount;
                 ++theActionCounter) {
                eastl::string theActionId = theActionsItr->second.first;
                outActionCount.m_ActionCount +=
                    GetActionCount(theActionId, outActionCount.m_StringAttrCount,
                                   outActionCount.m_CustomParamCount);
                ++theActionsItr;
            }
        }
    }
}

void CUIPParserActionHelper::BuildActions(IPresentation &inPresentation)
{
    // Now we start building
    // Iterate through each listener
    for (TListenerEventsNameMap::iterator theListenerItr = m_ListenerEventsNameMap.begin();
         theListenerItr != m_ListenerEventsNameMap.end();) {
        TElement *theElement = GetElement(theListenerItr->first);

        INT32 theEventCount =
            static_cast<INT32>(m_ListenerEventsNameMap.count(theListenerItr->first));
        for (INT32 theEventCounter = 0; theEventCounter < theEventCount;
             ++theEventCounter, ++theListenerItr) {
            UINT32 theEventName =
                CHash::HashEventCommand(theListenerItr->second.c_str()); // UINT32: HashedName

            // iterate through each listener-event pair
            SListenerEventNamePair thePair(theListenerItr->first, theListenerItr->second);
            TListenerEventActionsMap::iterator theActionsItr =
                m_ListenerEventActionsMap.lower_bound(thePair);
            INT32 theActionCount = static_cast<INT32>(m_ListenerEventActionsMap.count(thePair));
            for (INT32 theActionCounter = 0; theActionCounter < theActionCount;
                 ++theActionCounter) {
                eastl::string theActionId = theActionsItr->second.first;
                if (theElement)
                    BuildAction(*theElement, theEventName, inPresentation, theActionId);
                ++theActionsItr;
            }
        }
    }
}

union UTypeConverter {
    SAttributeKey m_Key;
    INT32 m_IntData;
};

struct SActionAdder
{
    ILogicSystem &m_LogicSystem;
    TElement &m_EventElement;
    UINT32 m_EventName;
    TElement *m_Owner;
    TElement *m_Target;

    SActionAdder(ILogicSystem &inLogicSystem, TElement &inEventElement, UINT32 inEventName,
                 TElement *inOwner, TElement *inTarget)
        : m_LogicSystem(inLogicSystem)
        , m_EventElement(inEventElement)
        , m_EventName(inEventName)
        , m_Owner(inOwner)
        , m_Target(inTarget)
    {
    }

    INT32 AddAction(TEventCommandHash inCommand, UINT32 inArg1 = 0, UINT32 inArg2 = 0)
    {
        UVariant arg1Var;
        UVariant arg2Var;
        arg1Var.m_INT32 = (INT32)inArg1;
        arg2Var.m_INT32 = (INT32)inArg2;
        return m_LogicSystem.AddAction(m_EventElement, m_EventName, m_Target, m_Owner, inCommand,
                                       arg1Var, arg2Var, true);
    }
};

void CUIPParserActionHelper::BuildAction(TElement &inElement, UINT32 inEventName,
                                         IPresentation &inPresentation,
                                         const eastl::string &inActionId)
{
    ILogicSystem &theLogicBuilder = inPresentation.GetLogicSystem();
    IParametersSystem &theParamSystem = inPresentation.GetParametersSystem();
    TActionMap::iterator theIter = m_ActionMap.find(inActionId);
    if (theIter == m_ActionMap.end()) {
        Q3DStudio_ASSERT(false);
        qCWarning(qt3ds::INVALID_OPERATION) << "Can't find action, id: " << inActionId.c_str();
        return;
    }

    SActionInfo theAction = theIter->second;
    INT32 theAddActionIndex = -1; // the index of newly added action. If there are more than 1
                                  // actions, the index points to the firstly added action.
    SActionAdder theAdder(theLogicBuilder, inElement, inEventName, GetElement(theAction.m_Owner),
                          GetElement(theAction.m_TargetObject));

    // The list of applicable handler is loaded from metadata xml file (if it is non-custom handler)
    bool theIsCustomHandler = IsCustomHandler(theAction);

    if (theIsCustomHandler) {
        // Argument 1 - param table start index
        INT32 paramGroup = theParamSystem.CreateParameterGroup();

        // First param is the handler name we want to invoke for the custom action.
        ExportCustomActionParameters(inPresentation, theParamSystem, paramGroup,
                                     theAction.m_Handler.c_str(), ERuntimeDataModelDataTypeString,
                                     ERuntimeAdditionalMetaDataTypeNone);

        int theHandlerArgumentCount = theAction.m_HandlerArgs.size();
        ERuntimeDataModelDataType theType;
        ERuntimeAdditionalMetaDataType theAdditionalType;
        // Actual handler parameters to pass in.
        for (int theIndex = 0; theIndex < theHandlerArgumentCount; ++theIndex) {
            GetHandlerArgumentType(theAction, theIndex, theType, theAdditionalType);
            ExportCustomActionParameters(inPresentation, theParamSystem, paramGroup,
                                         theAction.m_HandlerArgs[theIndex].m_Value.c_str(), theType,
                                         theAdditionalType);
        }
        theAddActionIndex = theAdder.AddAction(COMMAND_CUSTOMACTION, (UINT32)paramGroup);
    } else if (theAction.m_Handler == "Set Property") // Plain set action
    {
        // ArgumentOne - HashPropertyName
        // ArgumentTwo - Value
        SHandlerArgumentInfo theHandlerArg1 = theAction.m_HandlerArgs[0];
        SHandlerArgumentInfo theHandlerArg2 = theAction.m_HandlerArgs[1];

        eastl::string thePropertyName = theHandlerArg1.m_Value;
        SElementData *theTargetObject =
            m_UIPParser->GetElementData(theAction.m_TargetObject.c_str());
        if (theTargetObject) {
            SElementPropertyInfo *theProperty = m_UIPParser->m_ParseElementManager.FindProperty(
                *theTargetObject, thePropertyName.c_str());
            if (theProperty) {
                TPropertyDescAndValueList theProperties;
                ERuntimeDataModelDataType theType = theProperty->m_DataType;
                ERuntimeAdditionalMetaDataType theAdditionalType = theProperty->m_AdditionalType;

                m_UIPParser->GetAttributeList(inPresentation, theProperties, theType,
                                              theAdditionalType, thePropertyName.c_str(),
                                              theHandlerArg2.m_Value.c_str(),
                                              theProperty->m_PropertyNames);

                UINT32 theActionCount = theProperties.size();
                Q3DStudio_ASSERT(theActionCount > 0);
                for (UINT32 theActionIndex = 0; theActionIndex < theActionCount; ++theActionIndex) {
                    INT32 actionId = theAdder.AddAction(
                        COMMAND_SETPROPERTY, theProperties[theActionIndex].first.nameHash(),
                        theProperties[theActionIndex].second.m_INT32);
                    if (theActionIndex == 0)
                        theAddActionIndex = actionId;
                }
            }
        }
    } else if (theAction.m_Handler == "Fire Event") // Fire the selected event on the element
    {
        theAddActionIndex = theAdder.AddAction(
            COMMAND_FIREEVENT, CHash::HashEventCommand(theAction.m_HandlerArgs[0].m_Value.c_str()));
    } else if (theAction.m_Handler == "Go to Slide") // Switch slides
    {
        theAddActionIndex = theAdder.AddAction(
            COMMAND_GOTOSLIDENAME, CHash::HashString(theAction.m_HandlerArgs[0].m_Value.c_str()));
    } else if (theAction.m_Handler == "Next Slide") // Switch slides
    {
        theAddActionIndex = theAdder.AddAction(COMMAND_GOTONEXTSLIDE);
    } else if (theAction.m_Handler == "Previous Slide") // Switch slides
    {
        theAddActionIndex = theAdder.AddAction(COMMAND_GOTOPREVIOUSSLIDE);
    } else if (theAction.m_Handler == "Preceding Slide") // Switch slides
    {
        theAddActionIndex = theAdder.AddAction(COMMAND_BACKSLIDE);
    } else if (theAction.m_Handler == "Play") // Play
    {
        theAddActionIndex = theAdder.AddAction(COMMAND_PLAY);
    } else if (theAction.m_Handler == "Pause") // Pause
    {
        theAddActionIndex = theAdder.AddAction(COMMAND_PAUSE);
    } else if (theAction.m_Handler == "Go to Time") // Goto Time
    {
        float theTime = 0;
        const char *theValue = theAction.m_HandlerArgs[0].m_Value.c_str();
        if (!IsTrivial(theValue))
            WStrOps<float>().StrTo(theValue, theTime);

        theAddActionIndex =
            theAdder.AddAction(COMMAND_GOTOTIME, static_cast<UINT32>(theTime * 1000.0f));

        bool thePause = false;
        theValue = theAction.m_HandlerArgs[1].m_Value.c_str();
        if (!IsTrivial(theValue))
            WStrOps<bool>().StrTo(theValue, thePause);
        theAdder.AddAction(thePause ? COMMAND_PAUSE : COMMAND_PLAY);
    } else if (theAction.m_Handler == "Emit Signal") // Emit a signal
    {
        qt3ds::foundation::CStringHandle theStringHandle;
        if (!theAction.m_HandlerArgs[0].m_Value.empty()) {
            const char *theValue = theAction.m_HandlerArgs[0].m_Value.c_str();
            theStringHandle = inPresentation.GetStringTable().GetHandle(theValue);
        }
        theAddActionIndex = theAdder.AddAction(COMMAND_EMITSIGNAL, theStringHandle.handle());
    } else {
        Q3DStudio_ASSERT(false); // what sort of crazy action is this?
    }

    if (theAddActionIndex > -1)
        m_ActionIdentifierMap[theAction.m_Id] = theAddActionIndex;
}

INT32 CUIPParserActionHelper::GetActionCount(const eastl::string &inActionId)
{
    INT32 theStringAttrCount = 0;
    INT32 theCustomParamCount = 0;
    return GetActionCount(inActionId, theStringAttrCount, theCustomParamCount);
}

INT32 CUIPParserActionHelper::GetActionCount(const eastl::string &inActionId,
                                             INT32 &outStringAttrCount, INT32 &outCustomParamCount)
{
    INT32 theActionCount = 0;

    TActionMap::iterator theIter = m_ActionMap.find(inActionId);
    if (theIter == m_ActionMap.end()) {
        Q3DStudio_ASSERT(false);
        qCWarning(qt3ds::INVALID_OPERATION) << "Can't find action, id: " << inActionId.c_str();
        return theActionCount;
    }

    SActionInfo theAction = theIter->second;

    // The list of applicable handler is loaded from metadata xml file (if it is non-custom handler)
    bool theIsCustomHandler = IsCustomHandler(theAction);

    if (theIsCustomHandler) {
        theActionCount = 1;

        // First param is the handler name we want to invoke for the custom action.
        ++outStringAttrCount;
        ++outCustomParamCount;

        int theHandlerArgumentCount = theAction.m_HandlerArgs.size();
        ERuntimeDataModelDataType theType;
        ERuntimeAdditionalMetaDataType theAdditionalType;
        // Actual handler parameters to pass in.
        for (int theIndex = 0; theIndex < theHandlerArgumentCount; ++theIndex) {
            GetHandlerArgumentType(theAction, theIndex, theType, theAdditionalType);
            GetCustomActionParametersCount(theType, theAdditionalType, outStringAttrCount,
                                           outCustomParamCount);
        }
    }

    else if (theAction.m_Handler == "Set Property") // Plain set action
    {
        SHandlerArgumentInfo theHandlerArg1 = theAction.m_HandlerArgs[0];

        eastl::string thePropertyName = theHandlerArg1.m_Value;
        SElementData *theData = m_UIPParser->GetElementData(theAction.m_TargetObject.c_str());
        if (theData) {
            SElementPropertyInfo *thePropertyInfo =
                m_UIPParser->m_ParseElementManager.GetOrCreateProperty(*theData,
                                                                       thePropertyName.c_str());
            if (thePropertyInfo) {
                theActionCount = thePropertyInfo->m_Arity;
                if (m_UIPParser->IsStringType(thePropertyInfo->m_DataType))
                    ++outStringAttrCount;
            }
        }
    }

    else if (theAction.m_Handler == "Fire Event" // Fire the selected event on the element
             || theAction.m_Handler == "Go to Slide" // Switch slides
             || theAction.m_Handler == "Next Slide" // Switch slides
             || theAction.m_Handler == "Previous Slide" // Switch slides
             || theAction.m_Handler == "Preceding Slide" // Switch slides
             || theAction.m_Handler == "Play" // Play
             || theAction.m_Handler == "Pause" // Pause
             ) {
        theActionCount = 1;
    }

    else if (theAction.m_Handler == "Go to Time") // Goto Time
    {
        theActionCount = 2;
    } else if (theAction.m_Handler == "Play Sound") // Play Sound
    {
        theActionCount = 1;
        ++outStringAttrCount;
    } else if (theAction.m_Handler == "Emit Signal") // Emit Signal
    {
        theActionCount = 1;
        ++outStringAttrCount;
    } else {
        Q3DStudio_ASSERT(false); // what sort of crazy action is this?
    }

    return theActionCount;
}

INT32 CUIPParserActionHelper::GetActionIndex(const eastl::string &inActionId)
{
    TActionIdentifierMap::iterator theFind = m_ActionIdentifierMap.find(inActionId);
    if (theFind != m_ActionIdentifierMap.end())
        return theFind->second;
    Q3DStudio_ASSERT(false);
    return -1;
}

bool CUIPParserActionHelper::IsCustomHandler(const SActionInfo &inAction)
{
    const char *theClass = m_ObjRefHelper->GetClass(inAction.m_TargetObject);
    if (!IsTrivial(theClass))
        return m_MetaData.IsCustomHandler(theClass, inAction.m_Handler.c_str());
    return false;
}

void CUIPParserActionHelper::GetHandlerArgumentType(
    const SActionInfo &inAction, int inArgumentIndex, ERuntimeDataModelDataType &outType,
    ERuntimeAdditionalMetaDataType &outAdditionalType)
{
    const char *theClass = m_ObjRefHelper->GetClass(inAction.m_TargetObject);
    return m_MetaData.GetHandlerArgumentType(theClass, inAction.m_Handler.c_str(),
                                             inAction.m_HandlerArgs[inArgumentIndex].m_Name.c_str(),
                                             outType, outAdditionalType);
}

void CUIPParserActionHelper::GetCustomActionParametersCount(
    ERuntimeDataModelDataType inDataType, ERuntimeAdditionalMetaDataType inAdditionalType,
    INT32 &outStringAttrCount, INT32 &outCustomParamCount)
{
    // This function should match ExportCustomActionParameters
    switch (inDataType) {
    case ERuntimeDataModelDataTypeFloat:
    case ERuntimeDataModelDataTypeLong:
    case ERuntimeDataModelDataTypeBool: {
        outCustomParamCount += 1;
        break;
    }
    case ERuntimeDataModelDataTypeFloat2: {
        outCustomParamCount += 2;
        break;
    }
    case ERuntimeDataModelDataTypeFloat3: {
        if (inAdditionalType == ERuntimeAdditionalMetaDataTypeColor) {
            // Append rgba
            outCustomParamCount += 4;
        } else {
            // Append xyz
            outCustomParamCount += 3;
        }
        break;
    }
    // default to string
    default: {
        outCustomParamCount += 1;
        outStringAttrCount += 1;
    }
    }
}

namespace {

    using qt3ds::foundation::IStringTable;

    struct SCustomParameterParseInfoFloat
    {
        typedef float TParseType;
        static const char8_t *Name() { return "float"; }
        static TParseType Default() { return 0; }
        static Q3DStudio::UVariant ToVariant(const TParseType &inParseType)
        {
            Q3DStudio::UVariant retval;
            retval.m_FLOAT = inParseType;
            return retval;
        }
    };

    struct SCustomParameterParseInfoLong
    {
        typedef QT3DSI32 TParseType;
        static const char8_t *Name() { return "long"; }
        static TParseType Default() { return 0; }
        static Q3DStudio::UVariant ToVariant(const TParseType &inParseType)
        {
            Q3DStudio::UVariant retval;
            retval.m_FLOAT = static_cast<float>(inParseType);
            return retval;
        }
    };

    struct SCustomParameterParseInfoBool
    {
        typedef bool TParseType;
        static const char8_t *Name() { return "bool"; }
        static TParseType Default() { return false; }
        static Q3DStudio::UVariant ToVariant(const TParseType &inParseType)
        {
            Q3DStudio::UVariant retval;
            retval.m_FLOAT = static_cast<float>(inParseType);
            return retval;
        }
    };

    struct SCustomParameterParseInfoFloat2
    {
        typedef SFloat2 TParseType;
        static const char8_t *Name() { return "float2"; }
        static TParseType Default() { return SFloat2(0, 0); }
        static QT3DSU32 NumParameters() { return 2; }
        static Q3DStudio::UVariant ToVariant(QT3DSU32 inIdx, const TParseType &inParseType)
        {
            Q3DStudio::UVariant retval;
            retval.m_FLOAT = static_cast<float>(inParseType[inIdx]);
            return retval;
        }
    };

    struct SCustomParameterParseInfoFloat3
    {
        typedef SFloat3 TParseType;
        static const char8_t *Name() { return "float3"; }
        static const char8_t **Extensions()
        {
            static const char8_t *retval[] = { ".x", ".y", ".z", NULL };
            return retval;
        }
        static TParseType Default() { return SFloat3(0, 0, 0); }
        static QT3DSU32 NumParameters() { return 3; }
        static Q3DStudio::UVariant ToVariant(QT3DSU32 inIdx, const TParseType &inParseType)
        {
            Q3DStudio::UVariant retval;
            retval.m_FLOAT = static_cast<float>(inParseType[inIdx]);
            return retval;
        }
    };

    struct SCustomParameterParseInfoColor
    {
        typedef SFloat3 TParseType;
        static const char8_t *Name() { return "color4"; }
        static TParseType Default() { return SFloat3(0, 0, 0); }
        static QT3DSU32 NumParameters() { return 4; }
        static Q3DStudio::UVariant ToVariant(QT3DSU32 inIdx, const TParseType &inParseType)
        {
            if (inIdx < 3) {
                Q3DStudio::UVariant retval;
                retval.m_FLOAT = static_cast<float>(inParseType[inIdx]) * 255.0f;
                return retval;
            } else {
                Q3DStudio::UVariant retval;
                retval.m_FLOAT = 255.0f;
                return retval;
            }
        }
    };

    template <typename TParseInfo>
    struct ParseParameter
    {
        void operator()(WCharTReader &inReader, const char *inValue, IParametersSystem &inSystem,
                        QT3DSI32 inGroupId)
        {
            typename TParseInfo::TParseType theValue = TParseInfo::Default();
            if (!IsTrivial(inValue))
                inReader.Read(theValue);
            Q3DStudio::UVariant theVarValue = TParseInfo::ToVariant(theValue);
            inSystem.AddParameter(inGroupId, CHash::HashString(TParseInfo::Name()), theVarValue);
        }
    };

    template <typename TParseInfo>
    struct ParseAggregateParameter
    {
        void operator()(WCharTReader &inReader, const char *inValue, IParametersSystem &inSystem,
                        QT3DSI32 inGroupId)
        {
            typename TParseInfo::TParseType theValue = TParseInfo::Default();
            if (!IsTrivial(inValue))
                inReader.ReadRef(NVDataRef<float>(&theValue[0], TParseInfo::NumParameters()));

            for (QT3DSU32 idx = 0, end = TParseInfo::NumParameters(); idx < end; ++idx) {
                Q3DStudio::UVariant theVarValue = TParseInfo::ToVariant(idx, theValue);
                inSystem.AddParameter(inGroupId, CHash::HashString(TParseInfo::Name()),
                                      theVarValue);
            }
        }
    };
}

void CUIPParserActionHelper::ExportCustomActionParameters(
    IPresentation &inPresentation, IParametersSystem &inParamSystem, QT3DSI32 inParamGroupId,
    const char *inValue, ERuntimeDataModelDataType inDataType,
    ERuntimeAdditionalMetaDataType inAdditionalType)
{
    // Create a destructible value
    m_UIPParser->m_ValueBuffer.clear();
    m_UIPParser->m_ValueBuffer.write(inValue, (QT3DSU32)strlen(inValue) + 1);
    // Clear the destination buffer
    m_UIPParser->m_TempBuffer.clear();
    WCharTReader theReader((char8_t *)m_UIPParser->m_ValueBuffer.begin(), m_UIPParser->m_TempBuffer,
                           *m_UIPParser->GetDOMReader().GetStringTable());

    // The function should match CUIPParserActionHelper::GetCustomActionParametersCount
    switch (inDataType) {
    case ERuntimeDataModelDataTypeFloat: {
        ParseParameter<SCustomParameterParseInfoFloat>()(theReader, inValue, inParamSystem,
                                                         inParamGroupId);
        break;
    }
    case ERuntimeDataModelDataTypeLong: {
        ParseParameter<SCustomParameterParseInfoLong>()(theReader, inValue, inParamSystem,
                                                        inParamGroupId);
        break;
    }
    case ERuntimeDataModelDataTypeBool: {
        ParseParameter<SCustomParameterParseInfoBool>()(theReader, inValue, inParamSystem,
                                                        inParamGroupId);
        break;
    }
    case ERuntimeDataModelDataTypeFloat2: {
        ParseAggregateParameter<SCustomParameterParseInfoFloat2>()(theReader, inValue,
                                                                   inParamSystem, inParamGroupId);
        break;
    }
    case ERuntimeDataModelDataTypeFloat3: {
        if (inAdditionalType == ERuntimeAdditionalMetaDataTypeColor) {
            ParseAggregateParameter<SCustomParameterParseInfoColor>()(
                theReader, inValue, inParamSystem, inParamGroupId);
        } else {
            ParseAggregateParameter<SCustomParameterParseInfoFloat3>()(
                theReader, inValue, inParamSystem, inParamGroupId);
        }
        break;
    }
    // default to string
    default: {
        UVariant theValue;
        theValue.m_StringHandle = inPresentation.GetStringTable().GetHandle(inValue);
        inParamSystem.AddParameter(inParamGroupId, CHash::HashString("string"), theValue);
        break;
    }
    }
}
}
