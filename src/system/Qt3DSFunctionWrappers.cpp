/****************************************************************************
**
** Copyright (C) 1993-2010 NVIDIA Corporation.
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

//==============================================================================
// Includes
//==============================================================================
#include "SystemPrefix.h"
#include "Qt3DSFunctionWrappers.h"

using namespace Q3DStudio;

//==============================================================================
/**
* CTOR
*/
IFunctionWrapper::IFunctionWrapper()
    : m_Cost(0)
{
}

//==============================================================================
/**
* Retrieve the cost associated with this function.
* Should be zero or greater
*/
INT32 IFunctionWrapper::GetCost()
{
    return m_Cost;
}

//==============================================================================
/**
* Sets the cost associated with this function.
*/
void IFunctionWrapper::SetCost(const INT32 inCost)
{
    m_Cost = inCost;
}