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
#ifndef QT3DSDM_OPS_H
#define QT3DSDM_OPS_H

namespace qt3dsdm {
inline const TCharStr &GetPropertyName(const IDataCore &inModel, Qt3DSDMPropertyHandle inProperty)
{
    return inModel.GetProperty(inProperty).m_Name;
}

inline bool PropertyNameMatches(const IDataCore &inModel, const TCharStr &inName,
                                Qt3DSDMPropertyHandle inProperty)
{
    return inName == GetPropertyName(inModel, inProperty);
}

inline const Qt3DSDMPropertyHandle
GetPropertyByName(const IDataCore &inModel, Qt3DSDMInstanceHandle inInstance, const TCharStr &inName)
{
    using namespace std;
    TPropertyHandleList properties;
    inModel.GetAggregateInstanceProperties(inInstance, properties);
    TPropertyHandleList::iterator theProp =
        std::find_if(properties.begin(), properties.end(),
                     std::bind(PropertyNameMatches, std::ref(inModel), inName,
                               std::placeholders::_1));
    if (theProp != properties.end())
        return *theProp;
    return 0;
}
}

#endif
