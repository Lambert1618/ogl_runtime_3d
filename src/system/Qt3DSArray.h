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
#include "Qt3DSMemorySettings.h"
#include "Qt3DSMemory.h"
#include "Qt3DSPlatformSpecific.h"

#ifdef WIN32
#pragma warning(disable : 4625) // copy constructor could not be generated because a base class copy
                                // constructor is inaccessible
#pragma warning(disable : 4626) // assignment operator could not be generated because a base class
                                // assignment operator is inaccessible
#endif

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
#define NVARRAY_NOTFOUND -1

//==============================================================================
/**
 *	Ordered Collection also knows as a vector.
 *
 *	Use Reserve or the constructor parameter if you know how many slots
 *	will be needed to prevent frequent reallocations during growth.
 *
 *	The array grows by DEFAULT_GROWFACTOR aka 20% whenever it overflows. By
 *	setting the factor to 0 it will still clamp to 1 and instead grow the
 *	array by one slot at a time.  This is not ideal but given a pooled memory
 *	manager you will not thrash memory too much since you will grow inside
 *	each pool chunk and only reallocate when switching pools.
 *
 *	Naming of CArray instances are used to enable memory tracking of arrays
 *	that have gone berzerk or less frequently used instances that could be
 *	released after usage.
 *
 *	The FOR_ARRAY macro is the preferred way to process all array slots.
 */
template <class T>
class CArray
{
//==============================================================================
//	Constants
//==============================================================================
#define DEFAULT_GROWFACTOR 1.2f

    //==============================================================================
    //	Types
    //==============================================================================
public:
    typedef T TType; ///< Easy access to template type

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    CHAR m_Name[32]; ///< Allows easy ID using memory reporting
    T *m_Data; ///< Allocated memory containing array data
    INT32 m_Capacity; ///< Max number of slots possible
    INT32 m_Count; ///< Current number of slots in array
    FLOAT m_GrowFactor; ///< Factor by which array grows when out of space

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    inline CArray(const INT32 inCapacity = 0, const FLOAT inGrowFactor = DEFAULT_GROWFACTOR,
                  const CHAR *inName = NULL);
    inline CArray(const CArray<T> &);
    inline ~CArray();

public: // Management
    inline void SetName(const CHAR *inName = NULL);
    inline INT32 GetCount() const;
    inline INT32 GetCapacity() const;
    inline void Clear(const BOOL inRelease = true);
    inline void Reserve(const INT32 inCapacity, const BOOL inExpand = false);

public: // Stack access
    void Push(const T &inValue);
    inline const T &Pop();
    inline const T &Top() const;

public: // Random access
    inline INT32 GetIndex(const T &inValue) const;
    inline T &operator[](const INT32 inIndex) const;
    inline void Remove(const INT32 inIndex);

public: // FOR_ARRAY iteration
    inline T *Begin() const;
    inline T *End() const;

private: // Disabled Copy Construction
    inline CArray<T> &operator=(const CArray<T> &);
};

} // namespace Q3DStudio

//==============================================================================
//	Template code
//==============================================================================
#include "Qt3DSArray.inl"
