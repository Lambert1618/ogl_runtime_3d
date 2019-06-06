/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DSDM_PROPERTY_DEFINITION_H
#define QT3DSDM_PROPERTY_DEFINITION_H

#include "Qt3DSDMDataTypes.h"

namespace qt3dsdm {

struct Qt3DSDMPropertyDefinition
{
    TCharStr m_Name;
    Qt3DSDMInstanceHandle m_Instance;
    DataModelDataType::Value m_Type;

    Qt3DSDMPropertyDefinition()
        : m_Type(DataModelDataType::None)
    {
    }
    Qt3DSDMPropertyDefinition(Qt3DSDMInstanceHandle inInstanceHandle, TCharPtr inName,
                             DataModelDataType::Value inType)
        : m_Name(inName)
        , m_Instance(inInstanceHandle)
        , m_Type(inType)
    {
    }
    Qt3DSDMPropertyDefinition(const Qt3DSDMPropertyDefinition &inOther)
        : m_Name(inOther.m_Name)
        , m_Instance(inOther.m_Instance)
        , m_Type(inOther.m_Type)
    {
    }

    Qt3DSDMPropertyDefinition &operator=(const Qt3DSDMPropertyDefinition &inOther)
    {
        if (this != &inOther) {
            m_Name = inOther.m_Name;
            m_Instance = inOther.m_Instance;
            m_Type = inOther.m_Type;
        }
        return *this;
    }
};

typedef std::vector<Qt3DSDMPropertyDefinition> TPropertyDefinitionList;
}

#endif
