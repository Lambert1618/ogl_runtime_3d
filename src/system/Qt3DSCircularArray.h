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
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Circular array.  Not growable for now.
 *	The "streamlined" interface acts like a resource pool where construction of
 *	item T is minimized.
 */
template <class T>
class CCircularArray
{
    //==============================================================================
    //	Types
    //==============================================================================
public:
    typedef T TType; ///< Easy access to template type

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    CHAR m_Name[32]; ///< Allows easy ID if it grows too much
    T *m_Data; ///< Allocated memory containing array data
    INT32 m_Capacity; ///< Max number of elements possible
    INT32 m_Count; ///< Current number of elements in array

    INT32 m_Begin; ///< circular list indices
    INT32 m_End; ///< circular list indices

    // For 32-bit alignment
    BOOL m_Full; ///< flag to indicate circular list is full
    BOOL m_Paddings[3]; ///< Padding

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    inline CCircularArray(const INT32 inCapacity = 0, const CHAR *inName = NULL);
    inline ~CCircularArray();

public: // Management
    inline INT32 GetCount() const;
    inline INT32 GetCapacity() const;
    inline void Clear(const BOOL inRelease = false);
    inline void Reserve(const INT32 inCapacity);

public: // Stack access
    inline T &NewEntry();
    inline void Pop();
    inline T &Top();

public: // Status
    BOOL IsEmpty() const;
    BOOL IsFull() const;

protected: // Internal methods
    void Increment(INT32 &outIndex);
    void Decrement(INT32 &outIndex);
    void UpdateFullStatus();
};

} // namespace Q3DStudio

//==============================================================================
//	Template code
//==============================================================================
#include "Qt3DSCircularArray.inl"
