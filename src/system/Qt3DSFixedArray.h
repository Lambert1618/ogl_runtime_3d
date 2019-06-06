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

#include <QtCore/QDebug>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Fixed size Ordered Collection that doesn't support growing.
 */
template <class T, int Size>
class CFixedArray
{
    //==============================================================================
    //	Typedefs
    //==============================================================================
public:
    typedef T TType; ///< Easy access to template type

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    T m_Data[Size]; ///< Allocated memory containing array data
    INT32 m_Capacity; ///< Max number of elements possible
    INT32 m_Count; ///< Current number of elements in array

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    inline CFixedArray();
    inline ~CFixedArray();

public: // Management
    inline void Clear();
    inline INT32 GetCount() const;
    inline INT32 GetCapacity() const;

public: // Stack access
    inline void Push(const T &inValue);
    inline const T &Pop();

public: // Array access
    inline T &operator[](const INT32 inIndex);
    inline void Remove(const INT32 inIndex);
};

} // namespace Q3DStudio

//==============================================================================
//	Template code
//==============================================================================
#include "Qt3DSFixedArray.inl"
