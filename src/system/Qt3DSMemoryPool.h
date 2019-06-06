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
//	Includes
//==============================================================================
#include "Qt3DSMemoryProbe.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Managed memory allocation using fixed-size chunks.
 *
 *	The pool class implements a generic algorithm that favors reusing recently
 *	used memory chunks instead of returning memory continously.  Yet, a fresh
 *	pool will always return chunks positioned side by side.
 *
 *	If a chunk is freed it will be the first to be returned next time a fresh
 *	chunk is new'ed.  This makes the cache lines happier.  Deleted chunks are
 *	maintained by linking to each other instead of being stored in a separate
 *	memory consuming array.
 */
class CMemoryPool
{
    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    INT32 m_ChunkSize; ///< size in bytes of each chunk
    INT32 m_ChunkCount; ///< number of chunks in pool

    INT8 *m_Ceiling; ///< ceiling memory pointer
    INT8 *m_Memory; ///< base memory pointer, INT8 for easy pointer arithemtic
    INT8 *m_Top; ///< edge of free continous memory
    void *m_Hole; ///< first dis-continous memory chunk if any

    CMemoryProbe m_Probe; ///< call and byte data on pool usage
    CHAR m_Description[32]; ///< short description of this pool

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CMemoryPool();
    ~CMemoryPool();

public: // Initialization
    void Initialize(const CHAR *inName, const INT32 inChunkSize, const INT32 inChunkCount);
    void Release();

public: // Allocation and Deallocation
    void *Allocate();
    void Free(void *inChunk);
    BOOL IsFull() const;
    BOOL OwnsChunk(const void *inChunk) const;

protected: // Implementation
    void *UseTop();
    void PushHole(void *inChunk);
    void *PopHole();

public: // Statistics and reports
    INT32 GetFreeHoles() const;
    INT32 GetFreeTops() const;
    INT32 GetFreeChunks() const;
    INT32 GetChunkSize() const { return m_ChunkSize; }
    INT32 GetChunkCount() const { return m_ChunkCount; }
    CMemoryProbe &GetProbe() { return m_Probe; }
};

} // namespace Q3DStudio
