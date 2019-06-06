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
#include <EASTL/list.h>
//==============================================================================
// Includes
//==============================================================================
#include "Qt3DSInputEngine.h"
#include "Qt3DSSceneManager.h"
#include "Qt3DSInputEventTypes.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSApplication.h"
#include "Qt3DSRuntimeFactory.h"
#include "EventPollingSystem.h"
#include "foundation/Qt3DSAssert.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

class CInputEventProvider : public qt3ds::evt::IEventProvider
{
    int m_RefCount;

public:
    CInputEventProvider()
        : m_RefCount(0)
    {
    }
    ~CInputEventProvider()
    {
        while (!m_Events.empty()) {
            delete m_Events.front();
            m_Events.pop_front();
        }
    }
    struct SEvent
    {
        enum EType { TYPE_NONE = 0, TYPE_KEYBOARD, TYPE_BUTTON, TYPE_COUNT };
        SEvent(EType inType)
            : m_Type(inType)
        {
        }
        EType m_Type;

        virtual SEvent *Clone() const = 0;
        virtual Qt3DSEventSystemEvent *GenEventSystemEvent(qt3ds::evt::IEventFactory &inFactory) = 0;
    };
    struct SKeyboardEvent : public SEvent
    {
        SKeyboardEvent()
            : SEvent(TYPE_KEYBOARD)
            , m_KeyCode(KEY_NOKEY)
            , m_KeyEvent(0)
        {
        }
        SKeyboardEvent(INT32 inKeyCode, UINT32 inKeyEvent)
            : SEvent(TYPE_KEYBOARD)
            , m_KeyCode(inKeyCode)
            , m_KeyEvent(inKeyEvent)
        {
        }
        SEvent *Clone() const override
        {
            return new SKeyboardEvent(this->m_KeyCode, this->m_KeyEvent);
        }
        Qt3DSEventSystemEvent *GenEventSystemEvent(qt3ds::evt::IEventFactory &inFactory) override
        {
            if (m_KeyEvent == ON_KEYDOWN || m_KeyEvent == ON_KEYUP || m_KeyEvent == ON_KEYREPEAT) {
                Qt3DSEventSystemEvent &theEvent = inFactory.CreateEvent(3);
                int theIndex = 0;
                theEvent.m_Data[theIndex].m_Name = inFactory.RegisterStr("category");
                theEvent.m_Data[theIndex].m_Value.m_Type = QT3DSEventSystemEventTypesString;
                theEvent.m_Data[theIndex].m_Value.m_String = inFactory.AllocateStr("kb");
                ++theIndex;
                theEvent.m_Data[theIndex].m_Name = inFactory.RegisterStr("code");
                theEvent.m_Data[theIndex].m_Value.m_Type = QT3DSEventSystemEventTypesNumber;
                theEvent.m_Data[theIndex].m_Value.m_Number = m_KeyCode;
                ++theIndex;
                theEvent.m_Data[theIndex].m_Name = inFactory.RegisterStr("action");
                theEvent.m_Data[theIndex].m_Value.m_Type = QT3DSEventSystemEventTypesString;
                if (m_KeyEvent == ON_KEYDOWN)
                    theEvent.m_Data[theIndex].m_Value.m_String = inFactory.AllocateStr("down");
                else if (m_KeyEvent == ON_KEYUP)
                    theEvent.m_Data[theIndex].m_Value.m_String = inFactory.AllocateStr("up");
                else if (m_KeyEvent == ON_KEYREPEAT)
                    theEvent.m_Data[theIndex].m_Value.m_String = inFactory.AllocateStr("repeat");
                ++theIndex;
                return &theEvent;
            }
            QT3DS_ASSERT(false);
            return 0;
        }
        INT32 m_KeyCode;
        UINT32 m_KeyEvent;
    };
    struct SButtonEvent : public SEvent
    {
        SButtonEvent()
            : SEvent(TYPE_BUTTON)
            , m_ButtonCode(BUTTON_TOTAL_COUNT)
            , m_ButtonEvent(0)
        {
        }
        SButtonEvent(INT32 inButtonCode, UINT32 inButtonEvent)
            : SEvent(TYPE_BUTTON)
            , m_ButtonCode(inButtonCode)
            , m_ButtonEvent(inButtonEvent)
        {
        }
        SEvent *Clone() const override
        {
            return new SKeyboardEvent(this->m_ButtonCode, this->m_ButtonEvent);
        }
        Qt3DSEventSystemEvent *GenEventSystemEvent(qt3ds::evt::IEventFactory &inFactory) override
        {
            if (m_ButtonEvent == ON_BUTTONDOWN || m_ButtonEvent == ON_BUTTONUP
                || m_ButtonEvent == ON_BUTTONREPEAT) {
                Qt3DSEventSystemEvent &theEvent = inFactory.CreateEvent(3);
                int theIndex = 0;
                theEvent.m_Data[theIndex].m_Name = inFactory.RegisterStr("category");
                theEvent.m_Data[theIndex].m_Value.m_Type = QT3DSEventSystemEventTypesString;
                theEvent.m_Data[theIndex].m_Value.m_String = inFactory.AllocateStr("button");
                ++theIndex;
                theEvent.m_Data[theIndex].m_Name = inFactory.RegisterStr("code");
                theEvent.m_Data[theIndex].m_Value.m_Type = QT3DSEventSystemEventTypesNumber;
                theEvent.m_Data[theIndex].m_Value.m_Number = m_ButtonCode;
                ++theIndex;
                theEvent.m_Data[theIndex].m_Name = inFactory.RegisterStr("action");
                theEvent.m_Data[theIndex].m_Value.m_Type = QT3DSEventSystemEventTypesString;
                if (m_ButtonEvent == ON_BUTTONDOWN)
                    theEvent.m_Data[theIndex].m_Value.m_String = inFactory.AllocateStr("down");
                else if (m_ButtonEvent == ON_BUTTONUP)
                    theEvent.m_Data[theIndex].m_Value.m_String = inFactory.AllocateStr("up");
                else if (m_ButtonEvent == ON_BUTTONREPEAT)
                    theEvent.m_Data[theIndex].m_Value.m_String = inFactory.AllocateStr("repeat");
                ++theIndex;
                return &theEvent;
            }
            QT3DS_ASSERT(false);
            return 0;
        }
        INT32 m_ButtonCode;
        UINT32 m_ButtonEvent;
    };
    size_t GetNextEvents(qt3ds::evt::IEventFactory &inFactory, Qt3DSEventSystemEvent **outBuffer,
                         size_t inBufLenInEvent) override
    {
        size_t theEventCount = 0;
        while (!m_Events.empty() && inBufLenInEvent--) {
            SEvent *theBufferedEvent = m_Events.front();
            Qt3DSEventSystemEvent *theEventSystemEvent =
                theBufferedEvent->GenEventSystemEvent(inFactory);
            if (theEventSystemEvent)
                outBuffer[theEventCount++] = theEventSystemEvent;
            m_Events.pop_front();
        }
        return theEventCount;
    }

    void addRef() { ++m_RefCount; }

    void release()
    {
        --m_RefCount;
        if (m_RefCount <= 0)
            delete this;
    }

    void Release() override { release(); }

    void AddEvent(const SEvent &inEvent)
    {
        SEvent *theEvent = inEvent.Clone();
        if (theEvent)
            m_Events.push_back(theEvent);
        else
            QT3DS_ASSERT(false);
    }

private:
    eastl::list<SEvent *> m_Events;
};

//==============================================================================
/**
 *	Constructor
 */
CInputEngine::CInputEngine()
    : m_BeginPickInput(false)
    , m_Application(NULL)
    , m_InputEventProvider(NULL)
{
    m_InputFrame.m_PickValid = false;
    m_InputFrame.m_PickX = 0;
    m_InputFrame.m_PickY = 0;

    // TODO: SK - To quickly restore functionality, but this is should be in tegra-specific project.
    m_InputFrame.m_DeltaX = 0;
    m_InputFrame.m_DeltaY = 0;
}

//==============================================================================
/**
 *	Destructor
 */
CInputEngine::~CInputEngine()
{
}

//==============================================================================
/**
 *	Set the top-level interface of the Runtime object
 *	@param inRuntime	the top-level interface of the Runtime object
 */
void CInputEngine::SetApplication(qt3ds::runtime::IApplication *inApplication)
{
    m_Application = inApplication;
    // Create this here.  If we die before the application does, then the system will crash unless
    // the application event system is reponsibe for the keyboard provider.  Since the provider
    // never
    // access back to this object, this is the most sensible relationship.  Furthermore if we have
    // never been given an application, it doesn't make sense to have a keyboard event provider.
    // In reality, this entire system should be ref counted.
    m_InputEventProvider = new CInputEventProvider();

    // Add a ref for the event system because it does not add its own ref.
    m_InputEventProvider->addRef();
    m_Application->GetRuntimeFactory().GetEventSystem().AddProvider(*m_InputEventProvider);
    MarkApplicationDirty();
}

//==============================================================================
/**
 *	Get the input information for the current frame
 *	@return the SInputFrame that defines the current frame input information
 */
SInputFrame &CInputEngine::GetInputFrame()
{
    return m_InputFrame;
}

//==============================================================================
/**
 *	Return the key input portion of the input frame.
 */
SKeyInputFrame &CInputEngine::GetKeyInputFrame()
{
    return m_InputFrame.m_KeyInputFrame;
}

void CInputEngine::BeginPickInput()
{
    m_PickInput.clear();
    m_BeginPickInput = true;
}

//==============================================================================
/**
 *	Set the pick information the the input frame structure
 *	@param inX			the X value of the pick position
 *	@param inY			the Y value of the pick position
 *	@param inValid		whether the X and Y value are valid
 */
void CInputEngine::SetPickInput(FLOAT inX, FLOAT inY, BOOL inValid /*= true*/)
{
    m_InputFrame.m_PickValid = inValid;
    m_InputFrame.m_PickX = inX;
    m_InputFrame.m_PickY = inY;
    m_InputFrame.m_MouseFlags = 0;
    MarkApplicationDirty();
    if (m_BeginPickInput && m_PickInput.size() < 1000)
        m_PickInput.push_back(eastl::make_pair(inX, inY));
}

void CInputEngine::EndPickInput()
{
    m_BeginPickInput = false;
}

CInputEngine::TPickInputList CInputEngine::GetPickInput() const
{
    return qt3ds::foundation::toDataRef(m_PickInput.data(), m_PickInput.size());
}

//==============================================================================
/**
 *	Set the mouse button state flag
 *	@param inFlags		the current mouse state
 */
void CInputEngine::SetPickFlags(INT32 inFlags)
{
    m_InputFrame.m_MouseFlags |= inFlags;
}

//==============================================================================
/**
 *	Set the keyboard status
 *	@param inKeyCode		the identifier of the key
 *	@param inDown			identify whether the key is down
 */
void CInputEngine::SetKeyState(INT32 inKeyCode, BOOL inDown)
{
    if (!inDown)
        m_InputFrame.m_KeyInputFrame.m_KeyStates.erase(inKeyCode);
    else {
        SKeyInputFrame::MAP_KEYCODE_COUNT::iterator theIter =
            m_InputFrame.m_KeyInputFrame.m_KeyStates.find(inKeyCode);
        if (theIter == m_InputFrame.m_KeyInputFrame.m_KeyStates.end())
            m_InputFrame.m_KeyInputFrame.m_KeyStates.insert(
                eastl::make_pair<INT32, UINT16>(inKeyCode, 1));
        else if (theIter->second < 255)
            ++theIter->second;
    }
    MarkApplicationDirty();
}

//==============================================================================
/**
 *	Set the button status
 *	@param inButtonCode		the identifier of the button
 *	@param inDown			identify whether the button is down
 */
void CInputEngine::SetButtonState(INT32 inButtonCode, BOOL inDown)
{
    if (!inDown)
        m_InputFrame.m_ButtonStates.erase(inButtonCode);
    else {
        SInputFrame::MAP_BUTTONCODE_COUNT::iterator theIter =
            m_InputFrame.m_ButtonStates.find(inButtonCode);
        if (theIter == m_InputFrame.m_ButtonStates.end())
            m_InputFrame.m_ButtonStates.insert(eastl::make_pair<INT32, UINT16>(inButtonCode, 1));
        else if (theIter->second < 255)
            ++theIter->second;
    }
    MarkApplicationDirty();
}

//==============================================================================
/**
 *	Set the scroll value
 *	@param inMouseFlag		flag to indicate whether this is a vertical or horizontal
 *scroll
 *	@param inValue			value of scroll performed
 */
void CInputEngine::SetScrollValue(INT32 inMouseFlag, INT16 inValue)
{
    m_InputFrame.m_MouseFlags = inMouseFlag;
    m_InputFrame.m_ScrollValue = inValue;
    MarkApplicationDirty();
}
//==============================================================================
/**
 *	Set whether the keyboard is modified
 *	@param inFlag	true if the keyboard status is modified
 */
void CInputEngine::SetModifierFlag(INT32 inFlag)
{
    m_InputFrame.m_KeyInputFrame.m_ModifierFlags |= inFlag;
}

// Currently unused
//==============================================================================
/**
 *	Converts KeyCode in index format to ASCII format.
 *	This will translate upper/lower-case letters too.
 *	@param inKeyCode		the index format keycode
 *	@param inShift			whether the shift button is pressed
 *	@return the ASCII format keycode
 */
// INT32 CInputEngine::ConvertKeyCode( INT32 inKeyCode, BOOL inShift )
//{
//	static INT32 s_ASCIILowerCase[] =
//	{
//		0,//KEY_NOKEY           = 0x00,
//		27,//KEY_ESCAPE,
//		'1',//KEY_1,
//		'2',//KEY_2,
//		'3',//KEY_3,
//		'4',//KEY_4,
//		'5',//KEY_5,
//		'6',//KEY_6,
//		'7',//KEY_7,
//		'8',//KEY_8,
//		'9',//KEY_9,
//		'0',//KEY_0,
//		'-',//KEY_MINUS,		/* - on main keyboard */
//		'=',//KEY_EQUALS,
//		8,//KEY_BACK,			/* backspace */
//		'\t',//KEY_TAB,
//		'q',//KEY_Q,
//		'w',//KEY_W,
//		'e',//KEY_E,
//		'r',//KEY_R,
//		't',//KEY_T,
//		'y',//KEY_Y,
//		'u',//KEY_U,
//		'i',//KEY_I,
//		'o',//KEY_O,
//		'p',//KEY_P,
//		'[',//KEY_LBRACKET,
//		']',//KEY_RBRACKET,
//		'\n',//KEY_RETURN,		/* Enter on main keyboard */
//		0,//KEY_LCONTROL,
//		'a',//KEY_A,
//		's',//KEY_S,
//		'd',//KEY_D,
//		'f',//KEY_F,
//		'g',//KEY_G,
//		'h',//KEY_H,
//		'j',//KEY_J,
//		'k',//KEY_K,
//		'l',//KEY_L,
//		';',//KEY_SEMICOLON,
//		'\'',//KEY_APOSTROPHE,
//		'`',//KEY_GRAVE,		/* accent grave */
//		0,//KEY_LSHIFT,
//		'\\',//KEY_BACKSLASH,
//		'z',//KEY_Z,
//		'x',//KEY_X,
//		'c',//KEY_C,
//		'v',//KEY_V,
//		'b',//KEY_B,
//		'n',//KEY_N,
//		'm',//KEY_M,
//		',',//KEY_COMMA,
//		'.',//KEY_PERIOD,		/* . on main keyboard */
//		'/',//KEY_SLASH,		/* / on main keyboard */
//		0,//KEY_RSHIFT,
//		'*',//KEY_MULTIPLY,		/* * on numeric keypad */
//		0,//KEY_LMENU,			/* left Alt */
//		' ',//KEY_SPACE,
//		0,//KEY_CAPITAL,
//		0,//KEY_F1,
//		0,//KEY_F2,
//		0,//KEY_F3,
//		0,//KEY_F4,
//		0,//KEY_F5,
//		0,//KEY_F6,
//		0,//KEY_F7,
//		0,//KEY_F8,
//		0,//KEY_F9,
//		0,//KEY_F10,
//		0,//KEY_NUMLOCK,
//		0,//KEY_SCROLL,			/* Scroll Lock */
//		'7',//KEY_NUMPAD7,
//		'8',//KEY_NUMPAD8,
//		'9',//KEY_NUMPAD9,
//		'-',//KEY_SUBTRACT,		/* - on numeric keypad */
//		'4',//KEY_NUMPAD4,
//		'5',//KEY_NUMPAD5,
//		'6',//KEY_NUMPAD6,
//		'+',//KEY_ADD,			/* + on numeric keypad */
//		'1',//KEY_NUMPAD1,
//		'2',//KEY_NUMPAD2,
//		'3',//KEY_NUMPAD3,
//		'0',//KEY_NUMPAD0,
//		'.',//KEY_DECIMAL,		/* . on numeric keypad */
//		0,//KEY_OEM_102,		/* <> or \| on RT 102-key keyboard (Non-U.S.) */
//		0,//KEY_F11,
//		0,//KEY_F12,
//		0,//KEY_F13,			/*                     (NEC PC98) */
//		0,//KEY_F14,            /*                     (NEC PC98) */
//		0,//KEY_F15,            /*                     (NEC PC98) */
//		0,//KEY_KANA,           /* (Japanese keyboard)            */
//		0,//KEY_ABNT_C1,        /* /? on Brazilian keyboard */
//		0,//KEY_CONVERT,        /* (Japanese keyboard)            */
//		0,//KEY_NOCONVERT,      /* (Japanese keyboard)            */
//		0,//KEY_YEN,            /* (Japanese keyboard)            */
//		0,//KEY_ABNT_C2,        /* Numpad . on Brazilian keyboard */
//		0,//KEY_NUMPADEQUALS,   /* = on numeric keypad (NEC PC98) */
//		0,//KEY_PREVTRACK,      /* Previous Track (DIK_CIRCUMFLEX on Japanese keyboard) */
//		0,//KEY_AT,             /*                     (NEC PC98) */
//		0,//KEY_COLON,          /*                     (NEC PC98) */
//		0,//KEY_UNDERLINE,      /*                     (NEC PC98) */
//		0,//KEY_KANJI,          /* (Japanese keyboard)            */
//		0,//KEY_STOP,           /*                     (NEC PC98) */
//		0,//KEY_AX,             /*                     (Japan AX) */
//		0,//KEY_UNLABELED,      /*                        (J3100) */
//		0,//KEY_NEXTTRACK,      /* Next Track */
//		0,//KEY_NUMPADENTER,    /* Enter on numeric keypad */
//		0,//KEY_RCONTROL,
//		0,//KEY_MUTE,           /* Mute */
//		0,//KEY_CALCULATOR,     /* Calculator */
//		0,//KEY_PLAYPAUSE,      /* Play / Pause */
//		0,//KEY_MEDIASTOP,      /* Media Stop */
//		0,//KEY_VOLUMEDOWN,     /* Volume - */
//		0,//KEY_VOLUMEUP,       /* Volume + */
//		0,//KEY_WEBHOME,        /* Web home */
//		0,//KEY_NUMPADCOMMA,    /* , on numeric keypad (NEC PC98) */
//		0,//KEY_DIVIDE,         /* / on numeric keypad */
//		0,//KEY_SYSRQ,
//		0,//KEY_RMENU,          /* right Alt */
//		0,//KEY_PAUSE,          /* Pause */
//		0,//KEY_HOME,           /* Home on arrow keypad */
//		0,//KEY_UP,             /* UpArrow on arrow keypad */
//		0,//KEY_PRIOR,          /* PgUp on arrow keypad */
//		0,//KEY_LEFT,           /* LeftArrow on arrow keypad */
//		0,//KEY_RIGHT,          /* RightArrow on arrow keypad */
//		0,//KEY_END,            /* End on arrow keypad */
//		0,//KEY_DOWN,           /* DownArrow on arrow keypad */
//		0,//KEY_NEXT,           /* PgDn on arrow keypad */
//		0,//KEY_INSERT,         /* Insert on arrow keypad */
//		127,//KEY_DELETE,		/* Delete on arrow keypad */
//		0,//KEY_LWIN,           /* Left Windows key */
//		0,//KEY_RWIN,           /* Right Windows key */
//		0,//KEY_APPS,           /* AppMenu key */
//		0,//KEY_POWER,          /* System Power */
//		0,//KEY_SLEEP,          /* System Sleep */
//		0,//KEY_WAKE,           /* System Wake */
//		0,//KEY_WEBSEARCH,      /* Web Search */
//		0,//KEY_WEBFAVORITES,   /* Web Favorites */
//		0,//KEY_WEBREFRESH,     /* Web Refresh */
//		0,//KEY_WEBSTOP,        /* Web Stop */
//		0,//KEY_WEBFORWARD,     /* Web Forward */
//		0,//KEY_WEBBACK,        /* Web Back */
//		0,//KEY_MYCOMPUTER,     /* My Computer */
//		0,//KEY_MAIL,           /* Mail */
//		0//KEY_MEDIASELECT,		/* Media Select */
//	};
//
//	static INT32 s_ASCIIUpperCase[] =
//	{
//		0,//KEY_NOKEY           = 0x00,
//		27,//KEY_ESCAPE,
//		'!',//KEY_1,
//		'@',//KEY_2,
//		'#',//KEY_3,
//		'$',//KEY_4,
//		'%',//KEY_5,
//		'^',//KEY_6,
//		'&',//KEY_7,
//		'*',//KEY_8,
//		'(',//KEY_9,
//		')',//KEY_0,
//		'_',//KEY_MINUS,		/* - on main keyboard */
//		'+',//KEY_EQUALS,
//		8,//KEY_BACK,			/* backspace */
//		'\t',//KEY_TAB,
//		'Q',//KEY_Q,
//		'W',//KEY_W,
//		'E',//KEY_E,
//		'R',//KEY_R,
//		'T',//KEY_T,
//		'Y',//KEY_Y,
//		'U',//KEY_U,
//		'I',//KEY_I,
//		'O',//KEY_O,
//		'P',//KEY_P,
//		'{',//KEY_LBRACKET,
//		'}',//KEY_RBRACKET,
//		'\n',//KEY_RETURN,		/* Enter on main keyboard */
//		0,//KEY_LCONTROL,
//		'A',//KEY_A,
//		'S',//KEY_S,
//		'D',//KEY_D,
//		'F',//KEY_F,
//		'G',//KEY_G,
//		'H',//KEY_H,
//		'J',//KEY_J,
//		'K',//KEY_K,
//		'L',//KEY_L,
//		':',//KEY_SEMICOLON,
//		'\"',//KEY_APOSTROPHE,
//		'~',//KEY_GRAVE,		/* accent grave */
//		0,//KEY_LSHIFT,
//		'|',//KEY_BACKSLASH,
//		'Z',//KEY_Z,
//		'X',//KEY_X,
//		'C',//KEY_C,
//		'V',//KEY_V,
//		'B',//KEY_B,
//		'N',//KEY_N,
//		'M',//KEY_M,
//		'<',//KEY_COMMA,
//		'>',//KEY_PERIOD,		/* . on main keyboard */
//		'?',//KEY_SLASH,		/* / on main keyboard */
//		0,//KEY_RSHIFT,
//		'*',//KEY_MULTIPLY,		/* * on numeric keypad */
//		0,//KEY_LMENU,			/* left Alt */
//		' ',//KEY_SPACE,
//		0,//KEY_CAPITAL,
//		0,//KEY_F1,
//		0,//KEY_F2,
//		0,//KEY_F3,
//		0,//KEY_F4,
//		0,//KEY_F5,
//		0,//KEY_F6,
//		0,//KEY_F7,
//		0,//KEY_F8,
//		0,//KEY_F9,
//		0,//KEY_F10,
//		0,//KEY_NUMLOCK,
//		0,//KEY_SCROLL,			/* Scroll Lock */
//		'7',//KEY_NUMPAD7,
//		'8',//KEY_NUMPAD8,
//		'9',//KEY_NUMPAD9,
//		'-',//KEY_SUBTRACT,		/* - on numeric keypad */
//		'4',//KEY_NUMPAD4,
//		'5',//KEY_NUMPAD5,
//		'6',//KEY_NUMPAD6,
//		'+',//KEY_ADD,			/* + on numeric keypad */
//		'1',//KEY_NUMPAD1,
//		'2',//KEY_NUMPAD2,
//		'3',//KEY_NUMPAD3,
//		'0',//KEY_NUMPAD0,
//		'.',//KEY_DECIMAL,		/* . on numeric keypad */
//		0,//KEY_OEM_102,        /* <> or \| on RT 102-key keyboard (Non-U.S.) */
//		0,//KEY_F11,
//		0,//KEY_F12,
//		0,//KEY_F13,            /*                     (NEC PC98) */
//		0,//KEY_F14,            /*                     (NEC PC98) */
//		0,//KEY_F15,            /*                     (NEC PC98) */
//		0,//KEY_KANA,           /* (Japanese keyboard)            */
//		0,//KEY_ABNT_C1,        /* /? on Brazilian keyboard */
//		0,//KEY_CONVERT,        /* (Japanese keyboard)            */
//		0,//KEY_NOCONVERT,      /* (Japanese keyboard)            */
//		0,//KEY_YEN,            /* (Japanese keyboard)            */
//		0,//KEY_ABNT_C2,        /* Numpad . on Brazilian keyboard */
//		0,//KEY_NUMPADEQUALS,   /* = on numeric keypad (NEC PC98) */
//		0,//KEY_PREVTRACK,      /* Previous Track (DIK_CIRCUMFLEX on Japanese keyboard) */
//		0,//KEY_AT,             /*                     (NEC PC98) */
//		0,//KEY_COLON,          /*                     (NEC PC98) */
//		0,//KEY_UNDERLINE,      /*                     (NEC PC98) */
//		0,//KEY_KANJI,          /* (Japanese keyboard)            */
//		0,//KEY_STOP,           /*                     (NEC PC98) */
//		0,//KEY_AX,             /*                     (Japan AX) */
//		0,//KEY_UNLABELED,      /*                        (J3100) */
//		0,//KEY_NEXTTRACK,      /* Next Track */
//		0,//KEY_NUMPADENTER,    /* Enter on numeric keypad */
//		0,//KEY_RCONTROL,
//		0,//KEY_MUTE,           /* Mute */
//		0,//KEY_CALCULATOR,     /* Calculator */
//		0,//KEY_PLAYPAUSE,      /* Play / Pause */
//		0,//KEY_MEDIASTOP,      /* Media Stop */
//		0,//KEY_VOLUMEDOWN,     /* Volume - */
//		0,//KEY_VOLUMEUP,       /* Volume + */
//		0,//KEY_WEBHOME,        /* Web home */
//		0,//KEY_NUMPADCOMMA,    /* , on numeric keypad (NEC PC98) */
//		0,//KEY_DIVIDE,         /* / on numeric keypad */
//		0,//KEY_SYSRQ,
//		0,//KEY_RMENU,          /* right Alt */
//		0,//KEY_PAUSE,          /* Pause */
//		0,//KEY_HOME,           /* Home on arrow keypad */
//		0,//KEY_UP,             /* UpArrow on arrow keypad */
//		0,//KEY_PRIOR,          /* PgUp on arrow keypad */
//		0,//KEY_LEFT,           /* LeftArrow on arrow keypad */
//		0,//KEY_RIGHT,          /* RightArrow on arrow keypad */
//		0,//KEY_END,            /* End on arrow keypad */
//		0,//KEY_DOWN,           /* DownArrow on arrow keypad */
//		0,//KEY_NEXT,           /* PgDn on arrow keypad */
//		0,//KEY_INSERT,         /* Insert on arrow keypad */
//		127,//KEY_DELETE,		/* Delete on arrow keypad */
//		0,//KEY_LWIN,           /* Left Windows key */
//		0,//KEY_RWIN,           /* Right Windows key */
//		0,//KEY_APPS,           /* AppMenu key */
//		0,//KEY_POWER,          /* System Power */
//		0,//KEY_SLEEP,          /* System Sleep */
//		0,//KEY_WAKE,           /* System Wake */
//		0,//KEY_WEBSEARCH,      /* Web Search */
//		0,//KEY_WEBFAVORITES,   /* Web Favorites */
//		0,//KEY_WEBREFRESH,     /* Web Refresh */
//		0,//KEY_WEBSTOP,        /* Web Stop */
//		0,//KEY_WEBFORWARD,     /* Web Forward */
//		0,//KEY_WEBBACK,        /* Web Back */
//		0,//KEY_MYCOMPUTER,     /* My Computer */
//		0,//KEY_MAIL,           /* Mail */
//		0//KEY_MEDIASELECT,		/* Media Select */
//	};
//
//	if ( inKeyCode < 0 || inKeyCode > KEY_TOTAL_COUNT )
//		return 0;
//
//	if ( inShift )
//		return s_ASCIIUpperCase[inKeyCode];
//
//	return s_ASCIILowerCase[inKeyCode];
//}

//==============================================================================
/**
 *	Clear the current input frame data, typically after the event has been processed.
 */
void CInputEngine::ClearInputFrame()
{
    // TODO: SK - To quickly restore functionality, but this is should be in tegra-specific project.
    m_InputFrame.m_PickValid = false;
}

//==============================================================================
/**
 *	Can be more appropriately named and perhaps moved to a COutputEngine class.
 *	to translate event triggered from script back to a "device" event
 *	e.g to simulate clicking on Home button.
 *
 *	Base class does nothing.
 */
BOOL CInputEngine::TranslateEvent(const CHAR *inEvent)
{
    Q3DStudio_UNREFERENCED_PARAMETER(inEvent);
    return false;
}

//==============================================================================
/**
 *	Handle firing of keyboard events when a scancode is received.
 */
void CInputEngine::HandleKeyboard(INT32 inKeyCode, INT32 inPressed, INT32 inRepeat)
{
    if (inKeyCode >= KEY_TOTAL_COUNT || inKeyCode <= KEY_NOKEY) {
        qCDebug(qt3ds::TRACE_INFO)
                << "Input: Key input out of range. key code: " << inKeyCode
                << " pressed: " << inPressed << " repeat: " << inRepeat;
    }
    if (m_Application == NULL)
        return;
    MarkApplicationDirty();

    SetKeyState(inKeyCode, inPressed ? true : false);

    CPresentation *theActivePresentation(m_Application->GetPrimaryPresentation());
    if (theActivePresentation) {
        TElement *theScene = theActivePresentation->GetRoot();
        UVariant theKeycode;
        theKeycode.m_INT32 = inKeyCode;

        TEventCommandHash theEvent;
        if (inPressed) {
            UINT16 theKeyCount = m_InputFrame.m_KeyInputFrame.GetKeyCount(inKeyCode);
            QT3DS_ASSERT(theKeyCount >= 1);
            if (theKeyCount == 1 && inRepeat == 0)
                theEvent = ON_KEYDOWN;
            else
                theEvent = ON_KEYREPEAT;
        } else {
            theEvent = ON_KEYUP;
        }

        theActivePresentation->FireEvent(theEvent, theScene, &theKeycode, NULL, ATTRIBUTETYPE_INT32,
                                         ATTRIBUTETYPE_NONE);
        if (m_InputEventProvider)
            m_InputEventProvider->AddEvent(
                CInputEventProvider::SKeyboardEvent(theKeycode.m_INT32, theEvent));
    }
}

//==============================================================================
/**
 *	Handle button events.
 */
void CInputEngine::HandleButton(INT32 inButtonCode, INT32 inPressed, INT32 inRepeat)
{
    if (inButtonCode >= BUTTON_TOTAL_COUNT || inButtonCode < 0) {
        qCDebug(qt3ds::TRACE_INFO)
                << "Input: Key input out of range. key code: " << inButtonCode
                << " pressed: " << inPressed << " repeat: " << inRepeat;
    }
    if (m_Application == NULL)
        return;
    MarkApplicationDirty();

    SetButtonState(inButtonCode, inPressed ? true : false);

    CPresentation *theActivePresentation(m_Application->GetPrimaryPresentation());
    if (theActivePresentation) {
        TElement *theScene = theActivePresentation->GetRoot();
        UVariant theButtonCode;
        theButtonCode.m_INT32 = inButtonCode;

        TEventCommandHash theEvent;
        if (inPressed) {
            if (inRepeat == 0)
                theEvent = ON_BUTTONDOWN;
            else
                theEvent = ON_BUTTONREPEAT;
        } else {
            theEvent = ON_BUTTONUP;
        }

        theActivePresentation->FireEvent(theEvent, theScene, &theButtonCode, NULL,
                                         ATTRIBUTETYPE_INT32, ATTRIBUTETYPE_NONE);
        if (m_InputEventProvider)
            m_InputEventProvider->AddEvent(
                CInputEventProvider::SButtonEvent(theButtonCode.m_INT32, theEvent));
    }
}

//==============================================================================
/**
 *	Handle joystick events.
 */
void CInputEngine::HandleAxis(INT32 inAxisCode, FLOAT inValue)
{
    if (m_Application == NULL)
        return;
    MarkApplicationDirty();
    if (m_InputFrame.m_AxisStates[inAxisCode] != inValue) {
        m_InputFrame.m_AxisStates[inAxisCode] = inValue;

        CPresentation *theActivePresentation(m_Application->GetPrimaryPresentation());
        if (theActivePresentation) {
            TElement *theScene = theActivePresentation->GetRoot();
            UVariant theAxisCode;
            theAxisCode.m_INT32 = inAxisCode;
            UVariant theValue;
            theValue.m_FLOAT = inValue;

            theActivePresentation->FireEvent(ON_AXISMOVED, theScene, &theAxisCode, &theValue,
                                             ATTRIBUTETYPE_INT32, ATTRIBUTETYPE_FLOAT);
        }
    }
}

void CInputEngine::MarkApplicationDirty()
{
    if (m_Application)
        m_Application->MarkApplicationDirty();
}

} // namespace Q3DStudio
