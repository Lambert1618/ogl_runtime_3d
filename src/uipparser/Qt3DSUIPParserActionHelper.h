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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSUIPParserImpl.h"

namespace qt3ds {
namespace runtime {
    class IParametersSystem;
}
}
//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {
using qt3ds::runtime::IParametersSystem;
//==============================================================================
//	Forwards
//==============================================================================

//==============================================================================
/**
 *	@class	CUIPParserActionHelper
 *	@brief	Class for parsing UIP file - Action
 */
class CUIPParserActionHelper
{
    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    CUIPParserImpl *m_UIPParser;
    CUIPParserObjectRefHelper *m_ObjRefHelper;
    IRuntimeMetaData &m_MetaData; ///< Reference to Metadata

    typedef eastl::map<eastl::string, INT32> TActionIdentifierMap; ///< Mapping of the string that
                                                                   ///uniquely identify the action
                                                                   ///to the index created
    TActionIdentifierMap m_ActionIdentifierMap;

    struct SHandlerArgumentInfo
    {
        eastl::string m_Name;
        eastl::string m_Value;

        SHandlerArgumentInfo() {}
    };
    typedef eastl::vector<SHandlerArgumentInfo> THandlerArguments; ///< List of handler arguments
    struct SActionInfo
    {
        eastl::string m_Id;

        // Where the action is added to
        eastl::string m_Owner; // Absolute path of owner object

        // Trigger object
        eastl::string m_TriggerObject; // Absolute path of trigger object
        eastl::string m_Event;

        // Target object
        eastl::string m_TargetObject; // Absolute path of target object
        eastl::string m_Handler;
        THandlerArguments m_HandlerArgs;

        SActionInfo() {}
    };
    typedef eastl::map<eastl::string, SActionInfo> TActionMap; ///< Map of action id and action info

    TActionMap m_ActionMap;

    // There are 2 tables involved:
    // 1. The Listener-To-Events tables - multimap of each listener to all events it is listening to
    // 2. The Listener-Event-To-Actions table - multimap of each listener-event pair to all actions
    // it triggers
    // note: The struct for Action is a pair of EventAction node and its owner
    struct SListenerEventNamePair
    {
        eastl::string m_Listener;
        eastl::string m_Event;

        SListenerEventNamePair(eastl::string inListener, const eastl::string &inEvent)
            : m_Listener(inListener)
            , m_Event(inEvent)
        {
        }

        bool operator<(const SListenerEventNamePair &inListenerEventPair) const
        {
            if (m_Listener == inListenerEventPair.m_Listener)
                return m_Event < inListenerEventPair.m_Event;
            return m_Listener < inListenerEventPair.m_Listener;
        }
    };

    typedef eastl::pair<eastl::string, eastl::string>
        TEventActionOwnerPair; ///< One listener(source) can listen to multiple events
    typedef eastl::multimap<eastl::string, eastl::string>
        TListenerEventsNameMap; ///< One listener(source)-event pair can trigger multiple actions
    typedef eastl::multimap<SListenerEventNamePair, TEventActionOwnerPair> TListenerEventActionsMap;

    TListenerEventsNameMap m_ListenerEventsNameMap;
    TListenerEventActionsMap m_ListenerEventActionsMap;

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CUIPParserActionHelper(CUIPParserImpl *inUIPParser, CUIPParserObjectRefHelper *inObjRefHelper,
                           IRuntimeMetaData &inMetaData);
    virtual ~CUIPParserActionHelper();

public:
    void CacheAction(qt3dsdm::IDOMReader &inReader, const char8_t *inOwnerId);
    void BuildActions(IPresentation &inPresentation);
    void GetActionSectionCount(CUIPParserImpl::SActionSectionCount &outActionCount);
    INT32 GetActionCount(const eastl::string &inActionId);
    INT32 GetActionIndex(const eastl::string &inActionId);

protected: // Action helper
    void AddListenerEventPair(eastl::string inListener, const eastl::string &inEventName);
    void CacheHandlerArguments(qt3dsdm::IDOMReader &inReader, SActionInfo &inAction);
    void BuildAction(TElement &inElement, UINT32 inEventName, IPresentation &inPresentation,
                     const eastl::string &inActionId);
    INT32 GetActionCount(const eastl::string &inActionId, INT32 &outStringAttrCount,
                         INT32 &outCustomParamCount);
    bool IsCustomHandler(const SActionInfo &inAction);
    void GetHandlerArgumentType(const SActionInfo &inAction, int inArgumentIndex,
                                ERuntimeDataModelDataType &outType,
                                ERuntimeAdditionalMetaDataType &outAdditionalType);
    void GetCustomActionParametersCount(ERuntimeDataModelDataType inDataType,
                                        ERuntimeAdditionalMetaDataType inAdditionalType,
                                        INT32 &outStringAttrCount, INT32 &outCustomParamCount);
    void ExportCustomActionParameters(IPresentation &inPresentation,
                                      IParametersSystem &inParamSystem, QT3DSI32 inParamGroup,
                                      const char *inDataValue, ERuntimeDataModelDataType inDataType,
                                      ERuntimeAdditionalMetaDataType inAdditionalType);

    inline SElement *GetElement(const eastl::string &inElementName);
};

} // namespace Q3DStudio
