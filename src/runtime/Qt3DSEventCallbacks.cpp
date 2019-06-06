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

//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSEventCallbacks.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 */
CEventCallbacks::CEventCallbacks()
    : m_EventCallbacksList(0, 0, "EventCallbacksList")
    , m_CallbackList(0, 0, "CallbackList")
{
}

//==============================================================================
/**
 *	Destructor
 */
CEventCallbacks::~CEventCallbacks()
{
    UnregisterAllCallbacks();
}

//==============================================================================
/**
 *	Registers a callback to be triggered when a specific event is fired on an
 *	element.
 *	@param inElement		element to monitor
 *	@param inEventHash		event hash to monitor
 *	@param inCallback		static callback function
 *	@param inContextData	arbitary data pointer
 */
void CEventCallbacks::RegisterCallback(TElement *inElement, const TEventCommandHash inEventHash,
                                       const TEventCallback inCallback, void *inContextData)
{
    TEventCallbacksList *theEventCallbackList = NULL;

    // Look for the list of callbacks for this element
    FOR_ARRAY(SElementCallbackEntry *, theEntry, m_EventCallbacksList)
    {
        if (inElement == (*theEntry)->m_Element) {
            theEventCallbackList = &(*theEntry)->m_EventEntries;
            break;
        }
    }

    // Create a new list of callbacks if it does not already exist
    if (!theEventCallbackList) {
        SElementCallbackEntry *theElementCallbackEntry =
            Q3DStudio_new(SElementCallbackEntry) SElementCallbackEntry();
        theElementCallbackEntry->m_Element = inElement;
        m_EventCallbacksList.Push(theElementCallbackEntry);
        theEventCallbackList = &m_EventCallbacksList.Top()->m_EventEntries;
    }

    TCallbackList *theElementCallbackList = NULL;

    // Look for the list of callbacks for this event
    FOR_ARRAY(SEventCallbackEntry *, theEntry, (*theEventCallbackList))
    {
        if (inEventHash == (*theEntry)->m_EventHash) {
            theElementCallbackList = &(*theEntry)->m_Callbacks;
            break;
        }
    }

    // Create a new list of callbacks if it does not already exist
    if (!theElementCallbackList) {
        SEventCallbackEntry *theEventCallbackEntry =
            Q3DStudio_new(SEventCallbackEntry) SEventCallbackEntry();
        theEventCallbackEntry->m_EventHash = inEventHash;
        theEventCallbackList->Push(theEventCallbackEntry);
        theElementCallbackList = &theEventCallbackList->Top()->m_Callbacks;
    }

    // Insert callback
    SCallbackEntry theCallbackEntry = { inCallback, inContextData };
    theElementCallbackList->Push(theCallbackEntry);
}

//==============================================================================
/**
 *	Unregisters an event callback.
 *	@param inElement		element to monitor
 *	@param inEventHash		event hash to monitor
 *	@param inCallback		static callback function
 *	@param inContextData	arbitary data pointer
 *	@param outLast			indicates if element no longer has callbacks registered
 *	@return BOOL true if callback is unregistered successfully
 */
BOOL CEventCallbacks::UnregisterCallback(TElement *inElement, const TEventCommandHash inEventHash,
                                         const TEventCallback inCallback, void *inContextData,
                                         BOOL &outLast)
{
    outLast = false;

    TEventCallbacksList *theEventCallbackList = NULL;

    INT32 theEventListIndex = 0;
    SElementCallbackEntry *theFoundEntry = NULL;
    FOR_ARRAY(SElementCallbackEntry *, theEntry, m_EventCallbacksList)
    {
        if (inElement == (*theEntry)->m_Element) {
            theFoundEntry = *theEntry;
            theEventCallbackList = &(*theEntry)->m_EventEntries;
            break;
        }
        ++theEventListIndex;
    }

    if (!theEventCallbackList)
        return false;

    TCallbackList *theElementCallbackList = NULL;

    INT32 theElementListIndex = 0;
    SEventCallbackEntry *theEventCallbackEntry = NULL;
    FOR_ARRAY(SEventCallbackEntry *, theEntry, (*theEventCallbackList))
    {
        if (inEventHash == (*theEntry)->m_EventHash) {
            theEventCallbackEntry = *theEntry;
            theElementCallbackList = &(*theEntry)->m_Callbacks;
            break;
        }
        ++theElementListIndex;
    }

    if (!theElementCallbackList)
        return false;

    INT32 theCallbackIndex = 0;
    FOR_ARRAY(SCallbackEntry, theEntry, (*theElementCallbackList))
    {
        if (inCallback == theEntry->m_Function && inContextData == theEntry->m_ContextData) {
            theElementCallbackList->Remove(theCallbackIndex);

            if (theElementCallbackList->GetCount() == 0) {
                Q3DStudio_delete(theEventCallbackEntry, SEventCallbackEntry);
                theEventCallbackList->Remove(theElementListIndex);
            }

            if (theEventCallbackList->GetCount() == 0) {
                Q3DStudio_delete(theFoundEntry, SElementCallbackEntry);
                m_EventCallbacksList.Remove(theEventListIndex);
                outLast = true;
            }

            return true;
        }
        ++theCallbackIndex;
    }

    return false;
}

//==============================================================================
/**
 *	Unregisters all callbacks.
 */
void CEventCallbacks::UnregisterAllCallbacks()
{
    FOR_ARRAY(SElementCallbackEntry *, theEntry, m_EventCallbacksList)
    Q3DStudio_delete(*theEntry, SElementCallbackEntry);
    m_EventCallbacksList.Clear();
}

//==============================================================================
/**
 *	Fires event callbacks
 *	@param ioEvent			the event that was fired
 */
void CEventCallbacks::FireCallbacks(SEventCommand &ioEvent)
{
    TEventCallbacksList *theEventCallbackList = NULL;

    // Look for the list of callbacks for this element
    FOR_ARRAY(SElementCallbackEntry *, theEntry, m_EventCallbacksList)
    {
        if (ioEvent.m_Target == (*theEntry)->m_Element) {
            theEventCallbackList = &(*theEntry)->m_EventEntries;
            break;
        }
    }

    if (!theEventCallbackList)
        return;

    // Look for the list of callbacks for this event
    FOR_ARRAY(SEventCallbackEntry *, theEntry, (*theEventCallbackList))
    {
        if (ioEvent.m_Type == (*theEntry)->m_EventHash) {
            PerformCallbacks((*theEntry)->m_Callbacks, ioEvent);
            break;
        }
    }
}

//==============================================================================
/**
 *	Executing a list of callbacks
 *	@param inCallbackList	the list of callbacks to invoke
 *	@param ioEvent			the event to pass to the callbacks
 */
void CEventCallbacks::PerformCallbacks(TCallbackList &inCallbackList, SEventCommand &ioEvent)
{
    // As the callbacks can potentially do a register/unregister that would alter the call list,
    // it is safer to get a clean copy of the call list to iterate through.

    // Clumsy way to do a m_CallbackList = inCallbackList
    m_CallbackList.Clear(false);
    m_CallbackList.Reserve(inCallbackList.GetCount());
    FOR_ARRAY(SCallbackEntry, theEntry, inCallbackList)
    m_CallbackList.Push(*theEntry);

    // Invoking the callbacks
    FOR_ARRAY(SCallbackEntry, theEntry, m_CallbackList)
    theEntry->m_Function(theEntry->m_ContextData, ioEvent);
}

} // namespace Q3DStudio
