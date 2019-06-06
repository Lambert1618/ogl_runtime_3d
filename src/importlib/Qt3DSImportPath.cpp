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
#include "Qt3DSImportLibPrecompile.h"
#include "Qt3DSImportPath.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"

using namespace qt3dsimp;

void SPathBuffer::Save(IOutStream &outStream) const
{
    outStream.Write(GetFileTag());
    outStream.Write(GetFileVersion());
    outStream.Write((QT3DSU32)m_Commands.size());
    outStream.Write((QT3DSU32)m_Data.size());
    outStream.Write(toU8ConstDataRef((PathCommand::Enum *)m_Commands.begin(), m_Commands.size()));
    outStream.Write(toU8ConstDataRef((QT3DSF32 *)m_Data.begin(), m_Data.size()));
}

SPathBuffer *SPathBuffer::Load(IInStream &inStream, NVFoundationBase &inFoundation)
{
    QT3DSU64 fileTag;
    QT3DSU32 version, numCommands, numData;
    inStream.Read(fileTag);
    inStream.Read(version);
    inStream.Read(numCommands);
    inStream.Read(numData);
    if (fileTag != GetFileTag()) {
        qCCritical(INVALID_OPERATION, "Invalid file, not a path file");
        return NULL;
    }
    if (version > GetFileVersion()) {
        qCCritical(INVALID_OPERATION, "Version number out of range.");
        return NULL;
    }
    QT3DSU32 commandSize = numCommands * sizeof(QT3DSU32);
    QT3DSU32 dataSize = numData * sizeof(QT3DSF32);
    QT3DSU32 objectSize = sizeof(SPathBuffer);
    QT3DSU32 allocSize = objectSize + commandSize + dataSize;
    QT3DSU8 *rawData =
        (QT3DSU8 *)inFoundation.getAllocator().allocate(allocSize, "SPathBuffer", __FILE__, __LINE__);
    SPathBuffer *retval = new (rawData) SPathBuffer();
    QT3DSU8 *commandBuffer = rawData + sizeof(SPathBuffer);
    QT3DSU8 *dataBuffer = commandBuffer + commandSize;
    inStream.Read(commandBuffer, commandSize);
    inStream.Read(dataBuffer, dataSize);
    retval->m_Commands = toDataRef((PathCommand::Enum *)commandBuffer, numCommands);
    retval->m_Data = toDataRef((QT3DSF32 *)dataBuffer, numData);
    return retval;
}

void SPathBuffer::Free(NVAllocatorCallback &inAllocator)
{
    inAllocator.deallocate(this);
}

namespace {
struct SBuilder : public IPathBufferBuilder
{
    NVFoundationBase &m_Foundation;
    nvvector<PathCommand::Enum> m_Commands;
    nvvector<QT3DSF32> m_Data;
    QT3DSI32 m_RefCount;

    SBuilder(NVFoundationBase &inFoundation)
        : m_Foundation(inFoundation)
        , m_Commands(inFoundation.getAllocator(), "m_Commands")
        , m_Data(inFoundation.getAllocator(), "m_Data")
        , m_RefCount(0)
    {
    }

    void addRef() override { atomicIncrement(&m_RefCount); }

    void release() override
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVAllocatorCallback &theAlloc(m_Foundation.getAllocator());
            NVDelete(theAlloc, this);
        }
    }

    void Clear() override
    {
        m_Commands.clear();
        m_Data.clear();
    }

    void Push(const QT3DSVec2 inPos)
    {
        m_Data.push_back(inPos.x);
        m_Data.push_back(inPos.y);
    }

    void MoveTo(const QT3DSVec2 &inPos) override
    {
        m_Commands.push_back(PathCommand::MoveTo);
        Push(inPos);
    }

    void CubicCurveTo(const QT3DSVec2 &inC1, const QT3DSVec2 &inC2, const QT3DSVec2 &inP2) override
    {
        m_Commands.push_back(PathCommand::CubicCurveTo);
        Push(inC1);
        Push(inC2);
        Push(inP2);
    }

    void Close() override { m_Commands.push_back(PathCommand::Close); }

    // Points back to internal data structures, must use or copy.
    SPathBuffer GetPathBuffer() override
    {
        SPathBuffer retval;
        retval.m_Data = m_Data;
        retval.m_Commands = m_Commands;
        return retval;
    }
};
}

IPathBufferBuilder &IPathBufferBuilder::CreateBuilder(NVFoundationBase &inFoundation)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SBuilder)(inFoundation);
}
