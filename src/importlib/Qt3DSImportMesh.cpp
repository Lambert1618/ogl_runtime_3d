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
#ifdef WIN32
#pragma warning(disable : 4100)
#endif
#include "Qt3DSImportLibPrecompile.h"
#include "Qt3DSImportMesh.h"

using namespace qt3dsimp;

#ifdef QT3DS_X86

// Ensure our objects are of expected sizes.  This keeps us honest
// And ensures that we can load the datastructures we expect to by simply
// mapping the memory.
QT3DS_COMPILE_TIME_ASSERT(sizeof(NVRenderComponentTypes::Enum) == 4);
QT3DS_COMPILE_TIME_ASSERT(sizeof(NVRenderDrawMode::Enum) == 4);
QT3DS_COMPILE_TIME_ASSERT(sizeof(NVRenderVertexBufferEntry) == 20);
QT3DS_COMPILE_TIME_ASSERT(sizeof(VertexBuffer) == 20);
QT3DS_COMPILE_TIME_ASSERT(sizeof(IndexBuffer) == 12);
QT3DS_COMPILE_TIME_ASSERT(sizeof(MeshSubset) == 40);
QT3DS_COMPILE_TIME_ASSERT(sizeof(MeshDataHeader) == 12);
QT3DS_COMPILE_TIME_ASSERT(sizeof(Mesh) == 56);

#endif

#define QT3DSIMP_FOREACH(idxnm, val)                                                                  \
    for (QT3DSU32 idxnm = 0, __numItems = (QT3DSU32)val; idxnm < __numItems; ++idxnm)

namespace {

struct MeshSubsetV1
{
    // See description of a logical vertex buffer below
    QT3DSU32 m_LogicalVbufIndex;
    // QT3DS_MAX_U32 means use all available items
    QT3DSU32 m_Count;
    // Offset is in item size, not bytes.
    QT3DSU32 m_Offset;
    // Bounds of this subset.  This is filled in by the builder
    // see AddMeshSubset
    NVBounds3 m_Bounds;
};

struct LogicalVertexBuffer
{
    QT3DSU32 m_ByteOffset;
    QT3DSU32 m_ByteSize;
    LogicalVertexBuffer(QT3DSU32 byteOff, QT3DSU32 byteSize)
        : m_ByteOffset(byteOff)
        , m_ByteSize(byteSize)
    {
    }
    LogicalVertexBuffer()
        : m_ByteOffset(0)
        , m_ByteSize(0)
    {
    }
};

struct MeshV1
{
    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;
    SOffsetDataRef<LogicalVertexBuffer> m_LogicalVertexBuffers; // may be empty
    SOffsetDataRef<MeshSubsetV1> m_Subsets;
    NVRenderDrawMode::Enum m_DrawMode;
    NVRenderWinding::Enum m_Winding;
    typedef MeshSubsetV1 TSubsetType;
};

template <typename TSerializer>
void Serialize(TSerializer &serializer, MeshV1 &mesh)
{
    QT3DSU8 *baseAddr = reinterpret_cast<QT3DSU8 *>(&mesh);
    serializer.streamify(mesh.m_VertexBuffer.m_Entries);
    serializer.align();
    QT3DSIMP_FOREACH(entry, mesh.m_VertexBuffer.m_Entries.size())
    {
        MeshVertexBufferEntry &entryData = const_cast<MeshVertexBufferEntry &>(
            mesh.m_VertexBuffer.m_Entries.index(baseAddr, entry));
        serializer.streamifyCharPointerOffset(entryData.m_NameOffset);
        serializer.align();
    }
    serializer.streamify(mesh.m_VertexBuffer.m_Data);
    serializer.align();
    serializer.streamify(mesh.m_IndexBuffer.m_Data);
    serializer.align();
    serializer.streamify(mesh.m_LogicalVertexBuffers);
    serializer.align();
    serializer.streamify(mesh.m_Subsets);
    serializer.align();
}

struct MeshSubsetV2
{
    QT3DSU32 m_LogicalVbufIndex;
    QT3DSU32 m_Count;
    QT3DSU32 m_Offset;
    NVBounds3 m_Bounds;
    SOffsetDataRef<char16_t> m_Name;
};

struct MeshV2
{
    static const char16_t *s_DefaultName;

    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;
    SOffsetDataRef<LogicalVertexBuffer> m_LogicalVertexBuffers; // may be empty
    SOffsetDataRef<MeshSubsetV2> m_Subsets;
    NVRenderDrawMode::Enum m_DrawMode;
    NVRenderWinding::Enum m_Winding;
    typedef MeshSubsetV2 TSubsetType;
};

template <typename TSerializer>
void Serialize(TSerializer &serializer, MeshV2 &mesh)
{
    QT3DSU8 *baseAddr = reinterpret_cast<QT3DSU8 *>(&mesh);
    serializer.streamify(mesh.m_VertexBuffer.m_Entries);
    serializer.align();
    QT3DSIMP_FOREACH(entry, mesh.m_VertexBuffer.m_Entries.size())
    {
        MeshVertexBufferEntry &entryData = const_cast<MeshVertexBufferEntry &>(
            mesh.m_VertexBuffer.m_Entries.index(baseAddr, entry));
        serializer.streamifyCharPointerOffset(entryData.m_NameOffset);
        serializer.align();
    }
    serializer.streamify(mesh.m_VertexBuffer.m_Data);
    serializer.align();
    serializer.streamify(mesh.m_IndexBuffer.m_Data);
    serializer.align();
    serializer.streamify(mesh.m_LogicalVertexBuffers);
    serializer.align();
    serializer.streamify(mesh.m_Subsets);
    serializer.align();
    QT3DSIMP_FOREACH(entry, mesh.m_Subsets.size())
    {
        MeshSubsetV2 &theSubset = const_cast<MeshSubsetV2 &>(mesh.m_Subsets.index(baseAddr, entry));
        serializer.streamify(theSubset.m_Name);
        serializer.align();
    }
}

// Localize the knowledge required to read/write a mesh into one function
// written in such a way that you can both read and write by passing
// in one serializer type or another.
// This function needs to be careful to request alignment after every write of a
// buffer that may leave us unaligned.  The easiest way to be correct is to request
// alignment a lot.  The hardest way is to use knowledge of the datatypes and
// only request alignment when necessary.
template <typename TSerializer>
void Serialize(TSerializer &serializer, Mesh &mesh)
{
    QT3DSU8 *baseAddr = reinterpret_cast<QT3DSU8 *>(&mesh);
    serializer.streamify(mesh.m_VertexBuffer.m_Entries);
    serializer.align();
    QT3DSIMP_FOREACH(entry, mesh.m_VertexBuffer.m_Entries.size())
    {
        MeshVertexBufferEntry &entryData = mesh.m_VertexBuffer.m_Entries.index(baseAddr, entry);
        serializer.streamifyCharPointerOffset(entryData.m_NameOffset);
        serializer.align();
    }
    serializer.streamify(mesh.m_VertexBuffer.m_Data);
    serializer.align();
    serializer.streamify(mesh.m_IndexBuffer.m_Data);
    serializer.align();
    serializer.streamify(mesh.m_Subsets);
    serializer.align();
    QT3DSIMP_FOREACH(entry, mesh.m_Subsets.size())
    {
        MeshSubset &theSubset = const_cast<MeshSubset &>(mesh.m_Subsets.index(baseAddr, entry));
        serializer.streamify(theSubset.m_Name);
        serializer.align();
    }
    serializer.streamify(mesh.m_Joints);
    serializer.align();
}

struct TotallingSerializer
{
    QT3DSU32 m_NumBytes;
    QT3DSU8 *m_BaseAddress;
    TotallingSerializer(QT3DSU8 *inBaseAddr)
        : m_NumBytes(0)
        , m_BaseAddress(inBaseAddr)
    {
    }
    template <typename TDataType>
    void streamify(const SOffsetDataRef<TDataType> &data)
    {
        m_NumBytes += data.size() * sizeof(TDataType);
    }
    void streamify(const char *data)
    {
        if (data == NULL)
            data = "";
        QT3DSU32 len = (QT3DSU32)strlen(data) + 1;
        m_NumBytes += 4;
        m_NumBytes += len;
    }
    void streamifyCharPointerOffset(QT3DSU32 inOffset)
    {
        if (inOffset) {
            const char *dataPtr = (const char *)(inOffset + m_BaseAddress);
            streamify(dataPtr);
        } else
            streamify("");
    }
    bool needsAlignment() const { return getAlignmentAmount() > 0; }
    QT3DSU32 getAlignmentAmount() const { return 4 - (m_NumBytes % 4); }
    void align()
    {
        if (needsAlignment())
            m_NumBytes += getAlignmentAmount();
    }
};

struct ByteWritingSerializer
{
    IOutStream &m_Stream;
    TotallingSerializer m_ByteCounter;
    QT3DSU8 *m_BaseAddress;
    ByteWritingSerializer(IOutStream &str, QT3DSU8 *inBaseAddress)
        : m_Stream(str)
        , m_ByteCounter(inBaseAddress)
        , m_BaseAddress(inBaseAddress)
    {
    }

    template <typename TDataType>
    void streamify(const SOffsetDataRef<TDataType> &data)
    {
        m_ByteCounter.streamify(data);
        m_Stream.Write(data.begin(m_BaseAddress), data.size());
    }
    void streamify(const char *data)
    {
        m_ByteCounter.streamify(data);
        if (data == NULL)
            data = "";
        QT3DSU32 len = (QT3DSU32)strlen(data) + 1;
        m_Stream.Write(len);
        m_Stream.Write(data, len);
    }
    void streamifyCharPointerOffset(QT3DSU32 inOffset)
    {
        const char *dataPtr = (const char *)(inOffset + m_BaseAddress);
        streamify(dataPtr);
    }

    void align()
    {
        if (m_ByteCounter.needsAlignment()) {
            QT3DSU8 buffer[] = { 0, 0, 0, 0 };
            m_Stream.Write(buffer, m_ByteCounter.getAlignmentAmount());
            m_ByteCounter.align();
        }
    }
};

struct MemoryAssigningSerializer
{
    QT3DSU8 *m_Memory;
    QT3DSU8 *m_BaseAddress;
    QT3DSU32 m_Size;
    TotallingSerializer m_ByteCounter;
    bool m_Failure;
    MemoryAssigningSerializer(QT3DSU8 *data, QT3DSU32 size, QT3DSU32 startOffset)
        : m_Memory(data + startOffset)
        , m_BaseAddress(data)
        , m_Size(size)
        , m_ByteCounter(data)
        , m_Failure(false)
    {
        // We expect 4 byte aligned memory to begin with
        QT3DS_ASSERT((((size_t)m_Memory) % 4) == 0);
    }

    template <typename TDataType>
    void streamify(const SOffsetDataRef<TDataType> &_data)
    {
        SOffsetDataRef<TDataType> &data = const_cast<SOffsetDataRef<TDataType> &>(_data);
        if (m_Failure) {
            data.m_Size = 0;
            data.m_Offset = 0;
            return;
        }
        QT3DSU32 current = m_ByteCounter.m_NumBytes;
        m_ByteCounter.streamify(_data);
        if (m_ByteCounter.m_NumBytes > m_Size) {
            data.m_Size = 0;
            data.m_Offset = 0;
            m_Failure = true;
            return;
        }
        QT3DSU32 numBytes = m_ByteCounter.m_NumBytes - current;
        if (numBytes) {
            data.m_Offset = (QT3DSU32)(m_Memory - m_BaseAddress);
            updateMemoryBuffer(numBytes);
        } else {
            data.m_Offset = 0;
            data.m_Size = 0;
        }
    }
    void streamify(const char *&_data)
    {
        QT3DSU32 len;
        m_ByteCounter.m_NumBytes += 4;
        if (m_ByteCounter.m_NumBytes > m_Size) {
            _data = "";
            m_Failure = true;
            return;
        }
        qt3ds::intrinsics::memCopy(&len, m_Memory, 4);
        updateMemoryBuffer(4);
        m_ByteCounter.m_NumBytes += len;
        if (m_ByteCounter.m_NumBytes > m_Size) {
            _data = "";
            m_Failure = true;
            return;
        }
        _data = (const char *)m_Memory;
        updateMemoryBuffer(len);
    }
    void streamifyCharPointerOffset(QT3DSU32 &inOffset)
    {
        const char *dataPtr;
        streamify(dataPtr);
        inOffset = (QT3DSU32)(dataPtr - (const char *)m_BaseAddress);
    }
    void align()
    {
        if (m_ByteCounter.needsAlignment()) {
            QT3DSU32 numBytes = m_ByteCounter.getAlignmentAmount();
            m_ByteCounter.align();
            updateMemoryBuffer(numBytes);
        }
    }
    void updateMemoryBuffer(QT3DSU32 numBytes) { m_Memory += numBytes; }
};

inline QT3DSU32 GetMeshDataSize(Mesh &mesh)
{
    TotallingSerializer s(reinterpret_cast<QT3DSU8 *>(&mesh));
    Serialize(s, mesh);
    return s.m_NumBytes;
}

template <typename TDataType>
QT3DSU32 NextIndex(const QT3DSU8 *inBaseAddress, const SOffsetDataRef<QT3DSU8> data, QT3DSU32 idx)
{
    QT3DSU32 numItems = data.size() / sizeof(TDataType);
    if (idx < numItems) {
        const TDataType *dataPtr(reinterpret_cast<const TDataType *>(data.begin(inBaseAddress)));
        return dataPtr[idx];
    } else {
        QT3DS_ASSERT(false);
        return 0;
    }
}

template <typename TDataType>
QT3DSU32 NextIndex(NVConstDataRef<QT3DSU8> data, QT3DSU32 idx)
{
    QT3DSU32 numItems = data.size() / sizeof(TDataType);
    if (idx < numItems) {
        const TDataType *dataPtr(reinterpret_cast<const TDataType *>(data.begin()));
        return dataPtr[idx];
    } else {
        QT3DS_ASSERT(false);
        return 0;
    }
}

inline QT3DSU32 NextIndex(NVConstDataRef<QT3DSU8> inData,
                       qt3ds::render::NVRenderComponentTypes::Enum inCompType, QT3DSU32 idx)
{
    if (inData.size() == 0)
        return idx;
    switch (inCompType) {
    case NVRenderComponentTypes::QT3DSU8:
        return NextIndex<QT3DSU8>(inData, idx);
    case NVRenderComponentTypes::QT3DSI8:
        return NextIndex<QT3DSI8>(inData, idx);
    case NVRenderComponentTypes::QT3DSU16:
        return NextIndex<QT3DSU16>(inData, idx);
    case NVRenderComponentTypes::QT3DSI16:
        return NextIndex<QT3DSI16>(inData, idx);
    case NVRenderComponentTypes::QT3DSU32:
        return NextIndex<QT3DSU32>(inData, idx);
    case NVRenderComponentTypes::QT3DSI32:
        return NextIndex<QT3DSI32>(inData, idx);
    default:
        break;
    }

    // Invalid index buffer index type.
    QT3DS_ASSERT(false);
    return 0;
}

template <typename TMeshType>
// Not exposed to the outside world
TMeshType *DoInitialize(MeshBufHeaderFlags /*meshFlags*/, NVDataRef<QT3DSU8> data)
{
    QT3DSU8 *newMem = data.begin();
    QT3DSU32 amountLeft = data.size() - sizeof(TMeshType);
    MemoryAssigningSerializer s(newMem, amountLeft, sizeof(TMeshType));
    TMeshType *retval = (TMeshType *)newMem;
    Serialize(s, *retval);
    if (s.m_Failure)
        return NULL;
    return retval;
}
}

NVBounds3 Mesh::CalculateSubsetBounds(const NVRenderVertexBufferEntry &inEntry,
                                      NVConstDataRef<QT3DSU8> inVertxData, QT3DSU32 inStride,
                                      NVConstDataRef<QT3DSU8> inIndexData,
                                      qt3ds::render::NVRenderComponentTypes::Enum inIndexCompType,
                                      QT3DSU32 inSubsetCount, QT3DSU32 inSubsetOffset)
{
    NVBounds3 retval(NVBounds3::empty());
    const NVRenderVertexBufferEntry &entry(inEntry);
    if (entry.m_ComponentType != NVRenderComponentTypes::QT3DSF32 || entry.m_NumComponents != 3) {
        QT3DS_ASSERT(false);
        return retval;
    }

    const QT3DSU8 *beginPtr = inVertxData.begin();
    QT3DSU32 numBytes = inVertxData.size();
    QT3DSU32 dataStride = inStride;
    QT3DSU32 posOffset = entry.m_FirstItemOffset;
    // The loop below could be template specialized *if* we wanted to do this.
    // and the perf of the existing loop was determined to be a problem.
    // Else I would rather stay way from the template specialization.
    QT3DSIMP_FOREACH(idx, inSubsetCount)
    {
        QT3DSU32 dataIdx = NextIndex(inIndexData, inIndexCompType, idx + inSubsetOffset);
        QT3DSU32 finalOffset = (dataIdx * dataStride) + posOffset;
        if (finalOffset + sizeof(QT3DSVec3) <= numBytes) {
            const QT3DSU8 *dataPtr = beginPtr + finalOffset;
            retval.include(*reinterpret_cast<const QT3DSVec3 *>(dataPtr));
        } else {
            QT3DS_ASSERT(false);
        }
    }

    return retval;
}

void Mesh::Save(IOutStream &outStream) const
{
    Mesh &mesh(const_cast<Mesh &>(*this));
    QT3DSU8 *baseAddress = reinterpret_cast<QT3DSU8 *>(&mesh);
    QT3DSU32 numBytes = sizeof(Mesh) + GetMeshDataSize(mesh);
    MeshDataHeader header(numBytes);
    outStream.Write(header);
    outStream.Write(*this);
    ByteWritingSerializer writer(outStream, baseAddress);
    Serialize(writer, mesh);
}

wchar_t g_DefaultName[] = { 0 };

const wchar_t *Mesh::s_DefaultName = g_DefaultName;

template <typename TMeshType>
struct SubsetNameHandler
{
};

template <>
struct SubsetNameHandler<MeshV1>
{
    void AssignName(const QT3DSU8 * /*v1BaseAddress*/, const MeshSubsetV1 & /*mesh*/,
                    QT3DSU8 * /*baseAddress*/, QT3DSU8 *& /*nameBuffer*/, MeshSubset &outDest)
    {
        outDest.m_Name = SOffsetDataRef<char16_t>();
    }
    QT3DSU32 NameLength(const MeshSubsetV1 &) { return 0; }
};

using qt3ds::intrinsics::memCopy;

template <>
struct SubsetNameHandler<MeshV2>
{
    void AssignName(const QT3DSU8 *v2BaseAddress, const MeshSubsetV2 &mesh, QT3DSU8 *baseAddress,
                    QT3DSU8 *&nameBuffer, MeshSubset &outDest)
    {
        outDest.m_Name.m_Size = mesh.m_Name.m_Size;
        outDest.m_Name.m_Offset = (QT3DSU32)(nameBuffer - baseAddress);
        QT3DSU32 dtypeSize = mesh.m_Name.m_Size * 2;
        memCopy(nameBuffer, mesh.m_Name.begin(v2BaseAddress), dtypeSize);
        nameBuffer += dtypeSize;
    }
    QT3DSU32 NameLength(const MeshSubsetV2 &mesh) { return (mesh.m_Name.size() + 1) * 2; }
};

QT3DSU32 GetAlignedOffset(QT3DSU32 offset, QT3DSU32 align)
{
    QT3DSU32 leftover = offset % align;
    if (leftover)
        return offset + (align - leftover);
    return offset;
}

template <typename TPreviousMeshType>
Mesh *CreateMeshFromPreviousMesh(TPreviousMeshType *temp, NVAllocatorCallback &alloc)
{
    QT3DSU32 newMeshSize = sizeof(Mesh);
    QT3DSU8 *tempBaseAddress = reinterpret_cast<QT3DSU8 *>(temp);
    QT3DSU32 alignment = sizeof(void *);

    QT3DSU32 vertBufferSize = GetAlignedOffset(temp->m_VertexBuffer.m_Data.size(), alignment);
    newMeshSize += vertBufferSize;
    QT3DSU32 entryDataSize = temp->m_VertexBuffer.m_Entries.size() * sizeof(MeshVertexBufferEntry);
    newMeshSize += entryDataSize;
    QT3DSU32 indexBufferSize = GetAlignedOffset(temp->m_IndexBuffer.m_Data.size(), alignment);
    newMeshSize += indexBufferSize;
    QT3DSU32 entryNameSize = 0;
    for (QT3DSU32 entryIdx = 0, entryEnd = temp->m_VertexBuffer.m_Entries.size(); entryIdx < entryEnd;
         ++entryIdx) {
        const qt3ds::render::NVRenderVertexBufferEntry theEntry =
            temp->m_VertexBuffer.m_Entries.index(tempBaseAddress, entryIdx)
                .ToVertexBufferEntry(tempBaseAddress);
        const char *namePtr = theEntry.m_Name;
        if (namePtr == NULL)
            namePtr = "";

        entryNameSize += (QT3DSU32)strlen(theEntry.m_Name) + 1;
    }
    entryNameSize = GetAlignedOffset(entryNameSize, alignment);

    newMeshSize += entryNameSize;
    QT3DSU32 subsetBufferSize = temp->m_Subsets.size() * sizeof(MeshSubset);
    newMeshSize += subsetBufferSize;
    QT3DSU32 nameLength = 0;
    for (QT3DSU32 subsetIdx = 0, subsetEnd = temp->m_Subsets.size(); subsetIdx < subsetEnd;
         ++subsetIdx) {
        nameLength += SubsetNameHandler<TPreviousMeshType>().NameLength(
            temp->m_Subsets.index(tempBaseAddress, subsetIdx));
    }
    nameLength = GetAlignedOffset(nameLength, alignment);

    newMeshSize += nameLength;

    Mesh *retval = (Mesh *)alloc.allocate(newMeshSize, "TempData", __FILE__, __LINE__);
    new (retval) Mesh();
    QT3DSU8 *baseOffset = reinterpret_cast<QT3DSU8 *>(retval);
    QT3DSU8 *vertBufferData = baseOffset + sizeof(Mesh);
    QT3DSU8 *entryBufferData = vertBufferData + vertBufferSize;
    QT3DSU8 *entryNameBuffer = entryBufferData + entryDataSize;
    QT3DSU8 *indexBufferData = entryNameBuffer + entryNameSize;
    QT3DSU8 *subsetBufferData = indexBufferData + indexBufferSize;
    QT3DSU8 *nameData = subsetBufferData + subsetBufferSize;

    retval->m_DrawMode = temp->m_DrawMode;
    retval->m_Winding = temp->m_Winding;
    retval->m_VertexBuffer = temp->m_VertexBuffer;
    retval->m_VertexBuffer.m_Data.m_Offset = (QT3DSU32)(vertBufferData - baseOffset);
    retval->m_VertexBuffer.m_Entries.m_Offset = (QT3DSU32)(entryBufferData - baseOffset);
    memCopy(vertBufferData, temp->m_VertexBuffer.m_Data.begin(tempBaseAddress),
            temp->m_VertexBuffer.m_Data.size());
    memCopy(entryBufferData, temp->m_VertexBuffer.m_Entries.begin(tempBaseAddress), entryDataSize);
    QT3DSIMP_FOREACH(idx, temp->m_VertexBuffer.m_Entries.size())
    {
        const MeshVertexBufferEntry &src =
            temp->m_VertexBuffer.m_Entries.index(tempBaseAddress, idx);
        MeshVertexBufferEntry &dest = retval->m_VertexBuffer.m_Entries.index(baseOffset, idx);

        const char *targetName = reinterpret_cast<const char *>(src.m_NameOffset + tempBaseAddress);
        if (src.m_NameOffset == 0)
            targetName = "";
        QT3DSU32 nameLen = (QT3DSU32)strlen(targetName) + 1;
        dest.m_NameOffset = (QT3DSU32)(entryNameBuffer - baseOffset);
        memCopy(entryNameBuffer, targetName, nameLen);
        entryNameBuffer += nameLen;
    }

    retval->m_IndexBuffer = temp->m_IndexBuffer;
    retval->m_IndexBuffer.m_Data.m_Offset = (QT3DSU32)(indexBufferData - baseOffset);
    memCopy(indexBufferData, temp->m_IndexBuffer.m_Data.begin(tempBaseAddress),
            temp->m_IndexBuffer.m_Data.size());

    retval->m_Subsets.m_Size = temp->m_Subsets.m_Size;
    retval->m_Subsets.m_Offset = (QT3DSU32)(subsetBufferData - baseOffset);
    QT3DSIMP_FOREACH(idx, temp->m_Subsets.size())
    {
        MeshSubset &dest = const_cast<MeshSubset &>(retval->m_Subsets.index(baseOffset, idx));
        const typename TPreviousMeshType::TSubsetType &src =
            temp->m_Subsets.index(tempBaseAddress, idx);
        dest.m_Count = src.m_Count;
        dest.m_Offset = src.m_Offset;
        dest.m_Bounds = src.m_Bounds;
        SubsetNameHandler<TPreviousMeshType>().AssignName(tempBaseAddress, src, baseOffset,
                                                          nameData, dest);
    }
    alloc.deallocate(temp);
    return retval;
}

Mesh *Mesh::Load(NVAllocatorCallback &alloc, IInStream &inStream)
{
    MeshDataHeader header;
    inStream.Read(header);
    QT3DS_ASSERT(header.m_FileId == MeshDataHeader::GetFileId());
    if (header.m_FileId != MeshDataHeader::GetFileId())
        return NULL;
    if (header.m_FileVersion < 1 || header.m_FileVersion > MeshDataHeader::GetCurrentFileVersion())
        return NULL;
    if (header.m_SizeInBytes < sizeof(Mesh))
        return NULL;
    QT3DSU8 *newMem = (QT3DSU8 *)alloc.allocate(header.m_SizeInBytes, "Mesh", __FILE__, __LINE__);
    QT3DSU32 amountRead = inStream.Read(NVDataRef<QT3DSU8>(newMem, header.m_SizeInBytes));
    if (amountRead != header.m_SizeInBytes)
        goto failure;

    if (header.m_FileVersion == 1) {
        MeshV1 *temp = DoInitialize<MeshV1>(header.m_HeaderFlags,
                                            NVDataRef<QT3DSU8>(newMem, header.m_SizeInBytes));
        if (temp == NULL)
            goto failure;
        return CreateMeshFromPreviousMesh(temp, alloc);

    } else if (header.m_FileVersion == 2) {
        MeshV2 *temp = DoInitialize<MeshV2>(header.m_HeaderFlags,
                                            NVDataRef<QT3DSU8>(newMem, header.m_SizeInBytes));
        if (temp == NULL)
            goto failure;
        return CreateMeshFromPreviousMesh(temp, alloc);
    } else {
        Mesh *retval = Initialize(header.m_FileVersion, header.m_HeaderFlags,
                                  NVDataRef<QT3DSU8>(newMem, header.m_SizeInBytes));
        if (retval == NULL)
            goto failure;
        return retval;
    }

failure:
    QT3DS_ASSERT(false);
    alloc.deallocate(newMem);
    return NULL;
}

Mesh *Mesh::Initialize(QT3DSU16 meshVersion, MeshBufHeaderFlags meshFlags, NVDataRef<QT3DSU8> data)
{
    if (meshVersion != MeshDataHeader::GetCurrentFileVersion())
        return NULL;
    return DoInitialize<Mesh>(meshFlags, data);
}

// Multimesh support where you have multiple meshes in a single file.
// Save multi where you have overridden the allocator.
QT3DSU32 Mesh::SaveMulti(NVAllocatorCallback &alloc, ISeekableIOStream &inStream, QT3DSU32 inId) const
{
    QT3DSU32 nextId = 1;
    MeshMultiHeader tempHeader;
    MeshMultiHeader *theHeader = NULL;
    MeshMultiHeader *theWriteHeader = NULL;

    QT3DSI64 newMeshStartPos = 0;
    if (inStream.GetLength() != 0) {
        theHeader = LoadMultiHeader(alloc, inStream);
        if (theHeader == NULL) {
            QT3DS_ASSERT(false);
            return 0;
        }
        QT3DSU8 *headerBaseAddr = reinterpret_cast<QT3DSU8 *>(theHeader);
        for (QT3DSU32 idx = 0, end = theHeader->m_Entries.size(); idx < end; ++idx) {
            if (inId != 0) {
                QT3DS_ASSERT(inId != theHeader->m_Entries.index(headerBaseAddr, idx).m_MeshId);
            }
            nextId = qMax(nextId, theHeader->m_Entries.index(headerBaseAddr, idx).m_MeshId + 1);
        }
        newMeshStartPos =
            sizeof(MeshMultiHeader) + theHeader->m_Entries.size() * sizeof(MeshMultiEntry);
        theWriteHeader = theHeader;
    } else
        theWriteHeader = &tempHeader;

    inStream.SetPosition(-newMeshStartPos, SeekPosition::End);
    QT3DSI64 meshOffset = inStream.GetPosition();

    Save(inStream);

    if (inId != 0)
        nextId = inId;
    QT3DSU8 *theWriteBaseAddr = reinterpret_cast<QT3DSU8 *>(theWriteHeader);
    // Now write a new header out.
    inStream.Write(theWriteHeader->m_Entries.begin(theWriteBaseAddr),
                   theWriteHeader->m_Entries.size());
    MeshMultiEntry newEntry(static_cast<QT3DSI64>(meshOffset), nextId);
    inStream.Write(newEntry);
    theWriteHeader->m_Entries.m_Size++;
    inStream.Write(*theWriteHeader);

    if (theHeader != NULL) {
        alloc.deallocate(theHeader);
    }
    return static_cast<QT3DSU32>(nextId);
}

// Load a single mesh directly from a multi file with the provided overridden items
SMultiLoadResult Mesh::LoadMulti(NVAllocatorCallback &alloc, ISeekableIOStream &inStream,
                                 QT3DSU32 inId)
{
    MeshMultiHeader *theHeader(LoadMultiHeader(alloc, inStream));
    if (theHeader == NULL) {
        return SMultiLoadResult();
    }
    QT3DSU64 fileOffset = (QT3DSU64)-1;
    QT3DSU32 theId = inId;
    QT3DSU8 *theHeaderBaseAddr = reinterpret_cast<QT3DSU8 *>(theHeader);
    bool foundMesh = false;
    for (QT3DSU32 idx = 0, end = theHeader->m_Entries.size(); idx < end && !foundMesh; ++idx) {
        const MeshMultiEntry &theEntry(theHeader->m_Entries.index(theHeaderBaseAddr, idx));
        if (theEntry.m_MeshId == inId || (inId == 0 && theEntry.m_MeshId > theId)) {
            if (theEntry.m_MeshId == inId)
                foundMesh = true;
            theId = qMax(theId, (QT3DSU32)theEntry.m_MeshId);
            fileOffset = theEntry.m_MeshOffset;
        }
    }
    Mesh *retval = NULL;
    if (fileOffset == (QT3DSU64)-1) {
        goto endFunction;
    }

    inStream.SetPosition(static_cast<QT3DSI64>(fileOffset), SeekPosition::Begin);
    retval = Load(alloc, inStream);
endFunction:
    if (theHeader != NULL)
        alloc.deallocate(theHeader);
    return SMultiLoadResult(retval, theId);
}

// Returns true if this is a multimesh (several meshes in one file).
bool Mesh::IsMulti(ISeekableIOStream &inStream)
{
    MeshMultiHeader theHeader;
    inStream.SetPosition(-((QT3DSI64)(sizeof(MeshMultiHeader))), SeekPosition::End);
    QT3DSU32 numBytes = inStream.Read(theHeader);
    if (numBytes != sizeof(MeshMultiHeader))
        return false;
    return theHeader.m_Version == MeshMultiHeader::GetMultiStaticFileId();
}
// Load a multi header from a stream.
MeshMultiHeader *Mesh::LoadMultiHeader(NVAllocatorCallback &alloc, ISeekableIOStream &inStream)
{
    MeshMultiHeader theHeader;
    inStream.SetPosition(-((QT3DSI64)sizeof(MeshMultiHeader)), SeekPosition::End);
    QT3DSU32 numBytes = inStream.Read(theHeader);
    if (numBytes != sizeof(MeshMultiHeader)
        || theHeader.m_FileId != MeshMultiHeader::GetMultiStaticFileId()
        || theHeader.m_Version > MeshMultiHeader::GetMultiStaticVersion()) {
        return NULL;
    }
    size_t allocSize =
        sizeof(MeshMultiHeader) + theHeader.m_Entries.m_Size * sizeof(MeshMultiEntry);
    MeshMultiHeader *retval =
        (MeshMultiHeader *)alloc.allocate(allocSize, "MeshMultiHeader", __FILE__, __LINE__);
    if (retval == NULL) {
        QT3DS_ASSERT(false);
        return NULL;
    }
    QT3DSU8 *baseAddr = reinterpret_cast<QT3DSU8 *>(retval);
    QT3DSU8 *entryData = baseAddr + sizeof(MeshMultiHeader);
    *retval = theHeader;
    retval->m_Entries.m_Offset = (QT3DSU32)(entryData - baseAddr);
    inStream.SetPosition(-((QT3DSI64)allocSize), SeekPosition::End);

    numBytes =
        inStream.Read(reinterpret_cast<MeshMultiEntry *>(entryData), retval->m_Entries.m_Size);
    if (numBytes != retval->m_Entries.m_Size * sizeof(MeshMultiEntry)) {
        QT3DS_ASSERT(false);
        alloc.deallocate(retval);
        retval = NULL;
    }
    return retval;
}

QT3DSU32 GetHighestId(NVAllocatorCallback &inAlloc, MeshMultiHeader *inHeader)
{
    if (inHeader == NULL) {
        QT3DS_ASSERT(false);
        return 0;
    }
    QT3DSU8 *baseHeaderAddr = reinterpret_cast<QT3DSU8 *>(inHeader);
    QT3DSU32 highestId = 0;
    for (QT3DSU32 idx = 0, end = inHeader->m_Entries.size(); idx < end; ++idx)
        highestId = qMax(highestId, inHeader->m_Entries.index(baseHeaderAddr, idx).m_MeshId);
    inAlloc.deallocate(inHeader);
    return highestId;
}

QT3DSU32 Mesh::GetHighestMultiVersion(NVAllocatorCallback &alloc, ISeekableIOStream &inStream)
{
    return GetHighestId(alloc, LoadMultiHeader(alloc, inStream));
}
