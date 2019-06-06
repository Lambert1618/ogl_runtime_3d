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
#ifndef QT3DS_IMPORT_PATH_H
#define QT3DS_IMPORT_PATH_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSDataRef.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSBounds3.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSVec2.h"

namespace qt3dsimp {
using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace qt3ds::render;
struct PathCommand
{
    enum Enum {
        Noner = 0,
        MoveTo, // 2 floats
        CubicCurveTo, // 6 floats, c1, c2, p2.  p1 is existing location
        Close, // 0 floats
    };
};

struct SPathBuffer
{
    // 64 bit random number to uniquely identify this file type.
    static QT3DSU64 GetFileTag() { return 0x7b1a41633c43a6afULL; }
    static QT3DSU32 GetFileVersion() { return 1; }
    NVConstDataRef<PathCommand::Enum> m_Commands;
    NVConstDataRef<QT3DSF32> m_Data;
    SPathBuffer() {}
    void Save(IOutStream &outStream) const;
    static SPathBuffer *Load(IInStream &inStream, NVFoundationBase &inFoundation);

    // Object is unused after this call.  Anything created with Load must use this function.
    void Free(NVAllocatorCallback &inAllocator);
    SPathBuffer *Copy(NVAllocatorCallback &inAllocator) const;
};

class IPathBufferBuilder : public NVRefCounted
{
public:
    virtual void Clear() = 0;

    virtual void MoveTo(const QT3DSVec2 &inPos) = 0;
    virtual void CubicCurveTo(const QT3DSVec2 &inC1, const QT3DSVec2 &inC2, const QT3DSVec2 &inP2) = 0;
    virtual void Close() = 0;
    // Points back to internal data structures, must use or copy.
    virtual SPathBuffer GetPathBuffer() = 0;

    static IPathBufferBuilder &CreateBuilder(NVFoundationBase &inFoundation);
};
}

#endif