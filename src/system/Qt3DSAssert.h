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

#include "Qt3DSTypes.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Static gateway for all asserts.
 *
 *	By setting your own assert function you can redirect all Runtime asserts.
 *	By default it uses the internal Default method that uses CLog for output
 *	and displays a dialog on Windows PCs.
 */
class CAssert
{
    //==============================================================================
    //	Typedefs
    //==============================================================================
public:
    typedef void (*TFunction)(const Q3DStudio::CHAR *inAssert, const Q3DStudio::CHAR *inFile,
                              Q3DStudio::INT32 inLine, const Q3DStudio::CHAR *inFunction);

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    static TFunction s_AssertFunction; ///< Function pointer to active assert function

    //==============================================================================
    //	Methods
    //==============================================================================
private: // Hidden Constructor
    CAssert();

public: // Static Usage
    static void Default(const Q3DStudio::CHAR *inAssert, const Q3DStudio::CHAR *inFile,
                        Q3DStudio::INT32 inLine, const Q3DStudio::CHAR *inFunction);

public: // Static Configuration
    static void SetFunction(TFunction inAssert);
    static TFunction GetFunction();
};

} // namespace Q3DStudio
