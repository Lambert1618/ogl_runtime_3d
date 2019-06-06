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
#include "Qt3DSMemoryProbe.h"
#include "Qt3DSMemoryStatistics.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor - records its size with the statistics class
 */
CMemoryProbe::CMemoryProbe()
{
    CMemoryStatistics::Overhead() += sizeof(CMemoryProbe);
}

//==============================================================================
/**
 *	Destructor
 */
CMemoryProbe::~CMemoryProbe()
{
    CMemoryStatistics::Overhead() -= sizeof(CMemoryProbe);
}

//==============================================================================
/**
 *	Track the number related to incrementing memory.
 *	@param inByteAmount		amount of bytes being allocated
 */
void CMemoryProbe::Allocate(const INT32 inByteAmount)
{
    if (inByteAmount > 0) {
        m_Calls[MEMSCOPE_RESET].Add(1);
        m_Bytes[MEMSCOPE_RESET].Add(inByteAmount);

        m_Calls[MEMSCOPE_GLOBAL].Add(1);
        m_Bytes[MEMSCOPE_GLOBAL].Add(inByteAmount);
    }
}

//==============================================================================
/**
 *	Track the number related to incrementing memory.
 *	@param inByteAmount		amount of bytes being deallocted
 */
void CMemoryProbe::Free(const INT32 inByteAmount)
{
    Q3DStudio_ASSERT(inByteAmount >= 0);
    if (inByteAmount > 0) {
        m_Calls[MEMSCOPE_RESET].Delete(1);
        m_Bytes[MEMSCOPE_RESET].Delete(inByteAmount);

        m_Calls[MEMSCOPE_GLOBAL].Delete(1);
        m_Bytes[MEMSCOPE_GLOBAL].Delete(inByteAmount);
    }
}

//==============================================================================
/**
 *	Reset all local statistics. Global values are not reset.
 */
void CMemoryProbe::Reset()
{
    m_Calls[MEMSCOPE_RESET].Reset();
    m_Bytes[MEMSCOPE_RESET].Reset();
}

//==============================================================================
/**
 *	Helper method for adding statistics used by manager to add pool usage.
 *	@param inProbe is the stats being added
 */
void CMemoryProbe::Combine(const CMemoryProbe &inProbe)
{
    m_Calls[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_ADDS] +=
        inProbe.m_Calls[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_ADDS];
    m_Calls[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_DELETES] +=
        inProbe.m_Calls[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_DELETES];
    m_Calls[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_PEAK] +=
        inProbe.m_Calls[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_PEAK];
    m_Calls[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_CURRENT] +=
        inProbe.m_Calls[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_CURRENT];

    m_Bytes[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_ADDS] +=
        inProbe.m_Bytes[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_ADDS];
    m_Bytes[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_DELETES] +=
        inProbe.m_Bytes[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_DELETES];
    m_Bytes[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_PEAK] +=
        inProbe.m_Bytes[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_PEAK];
    m_Bytes[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_CURRENT] +=
        inProbe.m_Bytes[MEMSCOPE_GLOBAL].m_Value[MEMVALUE_CURRENT];

    m_Calls[MEMSCOPE_RESET].m_Value[MEMVALUE_ADDS] +=
        inProbe.m_Calls[MEMSCOPE_RESET].m_Value[MEMVALUE_ADDS];
    m_Calls[MEMSCOPE_RESET].m_Value[MEMVALUE_DELETES] +=
        inProbe.m_Calls[MEMSCOPE_RESET].m_Value[MEMVALUE_DELETES];
    m_Calls[MEMSCOPE_RESET].m_Value[MEMVALUE_PEAK] +=
        inProbe.m_Calls[MEMSCOPE_RESET].m_Value[MEMVALUE_PEAK];
    m_Calls[MEMSCOPE_RESET].m_Value[MEMVALUE_CURRENT] +=
        inProbe.m_Calls[MEMSCOPE_RESET].m_Value[MEMVALUE_CURRENT];

    m_Bytes[MEMSCOPE_RESET].m_Value[MEMVALUE_ADDS] +=
        inProbe.m_Bytes[MEMSCOPE_RESET].m_Value[MEMVALUE_ADDS];
    m_Bytes[MEMSCOPE_RESET].m_Value[MEMVALUE_DELETES] +=
        inProbe.m_Bytes[MEMSCOPE_RESET].m_Value[MEMVALUE_DELETES];
    m_Bytes[MEMSCOPE_RESET].m_Value[MEMVALUE_PEAK] +=
        inProbe.m_Bytes[MEMSCOPE_RESET].m_Value[MEMVALUE_PEAK];
    m_Bytes[MEMSCOPE_RESET].m_Value[MEMVALUE_CURRENT] +=
        inProbe.m_Bytes[MEMSCOPE_RESET].m_Value[MEMVALUE_CURRENT];
}

} // namespace Q3DStudio
