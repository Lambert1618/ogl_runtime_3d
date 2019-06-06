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

#include "SystemPrefix.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSAssert.h"
#include "foundation/Qt3DSLogging.h"
//==============================================================================
//	Platform Specific Includes
//==============================================================================
#if defined(_PCPLATFORM)
#pragma warning(push, 3)
#include <Windows.h>
#include <crtdbg.h>
#pragma warning(pop)
#endif // #if defined (_PCPLATFORM)

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Global Variables
//==============================================================================
CAssert::TFunction CAssert::s_AssertFunction = CAssert::Default; ///< Default assert function

//==============================================================================
/**
 *	Default method for asserts
 */
void CAssert::Default(const CHAR *inAssert, const CHAR *inFile, INT32 inLine,
                      const CHAR *inFunction)
{
    // Use logger to report the string first
    qCCritical (qt3ds::INTERNAL_ERROR) << "Assertion failed: " << inFile << " " << inLine
                                       << " " << inAssert << " " << inFunction;

#if defined(_DEBUG) && defined(_PCPLATFORM) && defined(QT3DS_VC)
    if (_CrtDbgReport(_CRT_ASSERT, inFile, inLine, inFunction, "Assertion Failed") == 1)
        ::DebugBreak();
#endif // _DEBUG & _PCPLATFORM
}

//==============================================================================
/**
 *	Method for setting the assert function
 */
void CAssert::SetFunction(TFunction inAssert)
{
    s_AssertFunction = inAssert;
}

//==============================================================================
/**
 *	Method for getting the assert function
 */
CAssert::TFunction CAssert::GetFunction()
{
    return s_AssertFunction;
}

} // namespace Q3DStudio
