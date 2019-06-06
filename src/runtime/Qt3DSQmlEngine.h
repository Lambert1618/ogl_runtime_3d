/****************************************************************************
**
** Copyright (C) 1993-20016 NVIDIA Corporation.
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
#ifndef QT3DS_QML_ENGINE_H
#define QT3DS_QML_ENGINE_H

#include "Qt3DSIScriptBridge.h"
#include "Qt3DSIComponentManager.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSMutex.h"
#include "Qt3DSTimer.h"

//==============================================================================
//	Namespace
//==============================================================================

namespace Q3DStudio {
//==============================================================================
//	Typedefs
//==============================================================================
typedef void (*TScriptCallback)(void *inUserData);

//==============================================================================
//	Defines
//==============================================================================
#define SCRIPT_ON_INITIALIZE 1
#define SCRIPT_ON_UPDATE 2

//==============================================================================
//	Forwards
//==============================================================================
struct SEventCommand;
class IPresentation;

//==============================================================================
/**
*	Manages the various events callbacks in the system. It fires the
*	registered callbacks when an event was triggered.
*/
class CScriptCallbacks
{
    //==============================================================================
    //	Structs
    //==============================================================================
protected:
    struct SScriptCallbackEntry
    {
        SScriptCallbackEntry() {}
        ~SScriptCallbackEntry() {}

        TScriptCallback m_Function; ///< Callback function pointer
        void *m_UserData; ///< User data
        Q3DStudio::UINT32
            m_CallbackType; ///< callback type. determines when and/or how often it is called
        bool m_Processed; ///< Only used if the callback is of

    private: // Disabled Copy Construction
        SScriptCallbackEntry(const SScriptCallbackEntry &);
        SScriptCallbackEntry &operator=(const SScriptCallbackEntry &);
    };
    typedef CArray<SScriptCallbackEntry *> TCallbackList; ///< Array of callbacks regisgtered

    struct SFrameCallbackEntry
    {
        SFrameCallbackEntry() {}
        ~SFrameCallbackEntry()
        {
            FOR_ARRAY(SScriptCallbackEntry *, theEntry, m_Callbacks)
            Q3DStudio_delete(*theEntry, SScriptCallbackEntry);
        }

        TCallbackList m_Callbacks; ///< List of callbacks listening to this event

    private: // Disabled Copy Construction
        SFrameCallbackEntry(const SFrameCallbackEntry &);
        SFrameCallbackEntry &operator=(const SFrameCallbackEntry &);
    };

    typedef CArray<SFrameCallbackEntry *> TFrameCallbacksList; ///< Array of frame callbacks

public: // Construction
    CScriptCallbacks();
    ~CScriptCallbacks();

    bool RegisterCallback(Q3DStudio::UINT32 callbackType, const TScriptCallback inCallback,
                          void *inUserData);
    void UnregisterCallback(Q3DStudio::UINT32 callbackType, const TScriptCallback inCallback);

    void ProcessCallbacks();

private:
    TFrameCallbacksList m_CallbackList;

private: // Disabled Copy Construction
    CScriptCallbacks(const CScriptCallbacks &);
    CScriptCallbacks &operator=(const CScriptCallbacks &);
};

class CQmlEngine : public IScriptBridge
{
    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    virtual ~CQmlEngine() {}

public: // Public functions but not functions on the script bridge
    /**
    * @brief Peek a custom action from the queue
    *
    * @param[out] outElement		Target Element
    * @param[out] outSignal			Signal Name
    *
    * @return  true if signal available
    */
    virtual bool PeekSignal(TElement *&outElement, char *&outName) = 0;

    /**
    * @brief Select a slide by index
    *
    * @param[in] component		Component Name
    * @param[in] slideIndex		Slide index
    * @param[in] inArgs			Arguemnts for slide switch
    *
    * @return  none
    */
    virtual void GotoSlideIndex(const char *component, const Q3DStudio::INT32 slideIndex,
                                const SScriptEngineGotoSlideArgs &inArgs) = 0;

    virtual bool GetSlideInfo(const char *elementPath, int &currentIndex, int &previousIndex,
                              QString &currentName, QString &previousName) = 0;

    /**
    * @brief Go to specified time
    *
    * @param[in] component		Component Name
    * @param[in] time			New time
    *
    * @return none
    */
    virtual void GotoTime(const char *component, const Q3DStudio::FLOAT time) = 0;

    /**
    * @brief Return values of an attribute
    *
    * @param[in] element		Element Name
    * @param[in] attName		Attribute name
    * @param[out] value		    Attribute value
    *
    * @return none
    */
    virtual bool GetAttribute(const char *element, const char *attName, char *value) = 0;
    virtual bool GetAttribute(TElement *target, const char *attName, char *value) = 0;

    /**
    * @brief Register a callback
    *
    * @param[in] callbackType		callback type
* @param[in] inCallback		    pointer to callback
* @param[in] inUserData		    pointer to user data
    *
    * @return  true on success
    */
    virtual bool RegisterCallback(Q3DStudio::UINT32 callbackType, const TScriptCallback inCallback,
                                  void *inUserData) = 0;

    /**
    * @brief Shutdown QML engine
    *
    * @param[in] inFoundation		Pointer to foundation
    *
    * @return  no return
    */
    virtual void Shutdown(qt3ds::NVFoundationBase &inFoundation) = 0;

    virtual qt3ds::runtime::IApplication *GetApplication() = 0;

    virtual void Initialize() = 0;

public:
    /**
    * @brief Create QML engine
    *
    * @param[in] inFoundation		Pointer to foundation
    * @param[in] inTimeProvider		Pointer to time provider
    *
    * @return  no return
    */
    static CQmlEngine *Create(qt3ds::NVFoundationBase &inFoundation, ITimeProvider &inTimeProvider);
};

} // namespace Q3DStudio

#endif
