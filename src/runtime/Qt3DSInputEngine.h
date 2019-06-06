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
#include "Qt3DSKernelTypes.h"
#include "Qt3DSPickFrame.h"
#include "foundation/Qt3DSRefCounted.h"
#include "EASTL/vector.h"
#include "foundation/Qt3DSDataRef.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace qt3ds {
namespace runtime {
    class IApplication;
}
}
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class CRuntime;
union UVariant;
class CInputEventProvider;

//==============================================================================
/**
*	@class	CInputEngine
*	@brief	The basic input engine
*/
class CInputEngine
{
    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    SInputFrame m_InputFrame; ///< The data describing the input state
    eastl::vector<eastl::pair<float, float>> m_PickInput;
    bool m_BeginPickInput;
    qt3ds::runtime::IApplication *m_Application; ///< The Runtime object
    qt3ds::foundation::NVScopedRefCounted<CInputEventProvider>
        m_InputEventProvider; ///< The event provider for keyboard

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    typedef qt3ds::foundation::NVConstDataRef<const eastl::pair<float, float>> TPickInputList;
    CInputEngine();
    virtual ~CInputEngine();

public: //	Runtime interfaces
    void SetApplication(qt3ds::runtime::IApplication *inApplication);

public:
    // Access
    virtual SInputFrame &GetInputFrame();
    virtual SKeyInputFrame &GetKeyInputFrame();

    // Setters
    // Multitouch is handled by using BeginPickInput, SetPickInput (several times), and EndPickInput
    // pairs.
    // The assumption here is that the pick input is static until changed.  So to clear it you need
    // a
    // begin, end pair without any other pieces.
    virtual void BeginPickInput();
    virtual void SetPickInput(FLOAT inX, FLOAT inY, BOOL inValid = true);
    virtual void EndPickInput();
    virtual TPickInputList GetPickInput() const;
    virtual void SetPickFlags(INT32 inFlags);
    virtual void SetKeyState(INT32 inKeyCode, BOOL inDown);
    virtual void SetButtonState(INT32 inButtonCode, BOOL inDown);
    virtual void SetScrollValue(INT32 inFlags, INT16 value);
    virtual void SetModifierFlag(INT32 inFlag);

    // Keyboard hook
    virtual void HandleKeyboard(INT32 inKeyCode, INT32 inPressed, INT32 inRepeat = 0);

    // Other input
    virtual void HandleButton(INT32 inButtonCode, INT32 inPressed, INT32 inRepeat = 0);
    virtual void HandleAxis(INT32 inAxisCode, FLOAT inValue);

    // Helpers
    // virtual INT32				ConvertKeyCode( INT32 inKeyCode, BOOL inShift );
    // TODO: SK - To quickly restore functionality, but this is should be in tegra-specific project.
    virtual void ClearInputFrame();
    virtual BOOL TranslateEvent(const CHAR *inEvent);

    void MarkApplicationDirty();
};

} // namespace Q3DStudio
