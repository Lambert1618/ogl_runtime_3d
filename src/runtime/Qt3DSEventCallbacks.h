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

#include "Qt3DSEvent.h"
#include "Qt3DSMemory.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Typedefs
//==============================================================================
typedef void (*TEventCallback)(void *inContextData, SEventCommand &ioEvent);

//==============================================================================
/**
 *	Manages the various events callbacks in the system. It fires the
 *	registered callbacks when an event was triggered.
 */
class CEventCallbacks
{
    //==============================================================================
    //	Structs
    //==============================================================================
protected:
    struct SCallbackEntry
    {
        TEventCallback m_Function; ///< Callback function pointer
        void *m_ContextData; ///< User data
    };
    typedef CArray<SCallbackEntry> TCallbackList; ///< Array of callbacks regisgtered

    struct SEventCallbackEntry
    {
        SEventCallbackEntry()
            : m_EventHash(0)
        {
        }
        ~SEventCallbackEntry() {}

        TEventCommandHash m_EventHash; ///< The event of interest
        TCallbackList m_Callbacks; ///< List of callbacks listening to this event

    private: // Disabled Copy Construction
        SEventCallbackEntry(const SEventCallbackEntry &);
        SEventCallbackEntry &operator=(const SEventCallbackEntry &);
    };
    typedef CArray<SEventCallbackEntry *> TEventCallbacksList; ///< Array of events with callback

    struct SElementCallbackEntry
    {
        SElementCallbackEntry()
            : m_Element(NULL)
        {
        }
        ~SElementCallbackEntry()
        {
            FOR_ARRAY(SEventCallbackEntry *, theEntry, m_EventEntries)
            Q3DStudio_delete(*theEntry, SEventCallbackEntry);
        }

        TElement *m_Element; ///< Element to monitor for event
        TEventCallbacksList m_EventEntries; ///< List of events listened on this element

    private: // Disabled Copy Construction
        SElementCallbackEntry(const SElementCallbackEntry &);
        SElementCallbackEntry &operator=(const SElementCallbackEntry &);
    };
    typedef CArray<SElementCallbackEntry *>
        TElementCallbacksList; ///< Array of elements with callbacks

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    TElementCallbacksList m_EventCallbacksList; ///< List of event callbacks
    TCallbackList m_CallbackList;

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CEventCallbacks();
    ~CEventCallbacks();

public: // Registration
    void RegisterCallback(TElement *inElement, const TEventCommandHash inEventHash,
                          const TEventCallback inCallback, void *inContextData);
    BOOL UnregisterCallback(TElement *inElement, const TEventCommandHash inEventHash,
                            const TEventCallback inCallback, void *inContextData, BOOL &outLast);
    void UnregisterAllCallbacks();

public: // Operation
    void FireCallbacks(SEventCommand &ioEvent);

protected:
    void PerformCallbacks(TCallbackList &inCallbackList, SEventCommand &ioEvent);

private: // Disabled Copy Construction
    CEventCallbacks(const CEventCallbacks &);
    CEventCallbacks &operator=(const CEventCallbacks &);
};

} // namespace Q3DStudio
