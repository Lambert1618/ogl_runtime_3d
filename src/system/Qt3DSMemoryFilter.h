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
 *	Aggregation interface for legacy calls that don't remember allocation size.
 *
 *	CMemoryManager and its pools require that the size must be sent along with
 *	any call, including the Free call.  Normal memory calls do not require this
 *	information and this filter class is used to intercept these calls and
 *	add a header to each call before forwarded to the manager.  This thin class
 *	allows old school calls such as malloc and free to still use the advantages
 *	of a pooled memory system without impacting the cleanliness and memory
 *	footprint of the modern system.
 */
class CMemoryFilter
{
    //==============================================================================
    //	Constants
    //==============================================================================
protected:
    const static INT32 FILTER_DOGTAG = 0x1337f1d0;

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Allocation Deallocation
    static void *Allocate(const INT32 inSize, const CHAR *inType, const CHAR *inFile,
                          const INT32 inLine, const BOOL inClear);
    static void Free(void *inPointer);
    static void *Reallocate(void *inPointer, const INT32 inNewSize, const CHAR *inNewType,
                            const CHAR *inFile, const INT32 inLine);
};

} // namespace Q3DStudio
