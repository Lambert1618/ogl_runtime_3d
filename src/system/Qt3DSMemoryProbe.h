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
#include "Qt3DSMacros.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Memory Tracking Enums
//==============================================================================

/// Track a value - remembering peak and add/del count
enum EMemoryValue {
    MEMVALUE_ADDS = 0, ///< For example: allocation call count or bytes allocated
    MEMVALUE_DELETES, ///< For example: free call count or bytes deallocated
    MEMVALUE_PEAK, ///< Peak of the current value since the last reset
    MEMVALUE_CURRENT, ///< Current count
    MEMVALUECOUNT
};

/// Differentiate between full session (global) or since last reset (local)
enum EMemoryScope {
    MEMSCOPE_RESET = 0, ///< Since last reset
    MEMSCOPE_GLOBAL, ///< Since process start
    MEMSCOPECOUNT
};

//==============================================================================
/**
 *	Memory allocation counter tracking bytes or calls.
 *
 *	A probe instance can be placed to track either number of allocations
 *	or the bytes of each allocation.  CMemoryManager and CMemoryHeap both
 *	record statistics using this class.  64 bytes.
 */
class CMemoryProbe
{
    //==============================================================================
    //	Structs
    //==============================================================================
public:
    //==============================================================================
    /**
     *	Intelligent number that tracks current, peak, incr. and decr.
     *
     *	This value remembers how the amount added, the amount deleted, the highest
     *	value, while keeping a current value.  These are used by CMemoryProbe when
     *	tracking memory allocations.
     */
    struct SValue
    {
        //	Fields
        INT32 m_Value[MEMVALUECOUNT]; ///< Four aspects (add,del,current,peak) we are tracking

        //	Methods
        SValue() { Reset(); }
        void Reset()
        {
            m_Value[MEMVALUE_ADDS] = 0;
            m_Value[MEMVALUE_DELETES] = 0;
            m_Value[MEMVALUE_PEAK] = 0;
            m_Value[MEMVALUE_CURRENT] = 0;
        }

        void Delete(INT32 inAmount)
        {
            Q3DStudio_ASSERT(inAmount >= 0);
            m_Value[MEMVALUE_DELETES] += inAmount;
            m_Value[MEMVALUE_CURRENT] -= inAmount;
        }

        void Add(INT32 inAmount)
        {
            Q3DStudio_ASSERT(inAmount >= 0);
            m_Value[MEMVALUE_ADDS] += inAmount;
            m_Value[MEMVALUE_CURRENT] += inAmount;
            m_Value[MEMVALUE_PEAK] =
                Q3DStudio_max(m_Value[MEMVALUE_PEAK], m_Value[MEMVALUE_CURRENT]);
        }
    };

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    SValue m_Calls[MEMSCOPECOUNT]; ///< Memory call count
    SValue m_Bytes[MEMSCOPECOUNT]; ///< Memory allocation amount

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Constructor
    CMemoryProbe();
    ~CMemoryProbe();

public: // Operations
    void Reset();
    void Allocate(const INT32 inByteAmount);
    void Free(const INT32 inByteAmount);
    void Combine(const CMemoryProbe &inProbe);

public: // Accessors
    INT32 GetCalls(const EMemoryScope inScope, const EMemoryValue inValue) const
    {
        return m_Calls[inScope].m_Value[inValue];
    }
    INT32 GetBytes(const EMemoryScope inScope, const EMemoryValue inValue) const
    {
        return m_Bytes[inScope].m_Value[inValue];
    }
};

} // namespace Q3DStudio
