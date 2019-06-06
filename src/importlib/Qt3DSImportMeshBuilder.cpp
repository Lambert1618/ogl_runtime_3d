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
#include "Qt3DSImportMesh.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "Qt3DSImportContainers.h"

using namespace qt3dsimp;

// Disable mesh optimization. TODO: After removing NvTriStrip need
// to implement mesh optimization.
//#define DISABLE_MESH_OPTIMIZATION 1

// It turns out we can't enable vertex remapping because it would break
// mesh morphing.
#define DISABLE_VERTEX_REMAP 1

namespace {
template <typename TDataType>
NVConstDataRef<TDataType> toRefBuf(QT3DSU8 *bufData, QT3DSU32 off, QT3DSU32 size)
{
    QT3DS_ASSERT(size % sizeof(TDataType) == 0);
    if (size)
        return NVConstDataRef<TDataType>((TDataType *)(bufData + off), size / sizeof(TDataType));
    return NVConstDataRef<TDataType>();
}
struct DynamicVBuf
{
    QT3DSU32 m_Stride;
    ImportArray<NVRenderVertexBufferEntry> m_VertexBufferEntries;
    MemoryBuffer<RawAllocator> m_VertexData;

    void clear()
    {
        m_Stride = 0;
        m_VertexBufferEntries.clear();
        m_VertexData.clear();
    }
};
struct DynamicIndexBuf
{
    NVRenderComponentTypes::Enum m_CompType;
    MemoryBuffer<RawAllocator> m_IndexData;
    DynamicIndexBuf() {}

    void clear() { m_IndexData.clear(); }
};

struct SubsetDesc
{
    QT3DSU32 m_Count;
    QT3DSU32 m_Offset;

    NVBounds3 m_Bounds;
    QString m_Name;
    SubsetDesc(QT3DSU32 c, QT3DSU32 off)
        : m_Count(c)
        , m_Offset(off)
    {
    }
    SubsetDesc()
        : m_Count(0)
        , m_Offset(0)
    {
    }
};

QT3DSU32 GetAlignedOffset(QT3DSU32 offset, QT3DSU32 align)
{
    QT3DSU32 leftover = offset % align;
    if (leftover)
        return offset + (align - leftover);
    return offset;
}

class MeshBuilderImpl : public MeshBuilder
{
    DynamicVBuf m_VertexBuffer;
    DynamicIndexBuf m_IndexBuffer;
    ImportArray<Joint> m_Joints;
    ImportArray<SubsetDesc> m_MeshSubsetDescs;
    NVRenderDrawMode::Enum m_DrawMode;
    NVRenderWinding::Enum m_Winding;
    MemoryBuffer<RawAllocator> m_RemappedVertexData;
    MemoryBuffer<RawAllocator> m_NewIndexBuffer;
    ImportArray<QT3DSU8> m_MeshBuffer;

public:
    MeshBuilderImpl() { Reset(); }
    virtual ~MeshBuilderImpl() { Reset(); }
    void Release() override { delete this; }
    void Reset() override
    {
        m_VertexBuffer.clear();
        m_IndexBuffer.clear();
        m_Joints.clear();
        m_MeshSubsetDescs.clear();
        m_DrawMode = NVRenderDrawMode::Triangles;
        m_Winding = NVRenderWinding::CounterClockwise;
        m_MeshBuffer.clear();
    }

    void SetDrawParameters(NVRenderDrawMode::Enum drawMode, NVRenderWinding::Enum winding) override
    {
        m_DrawMode = drawMode;
        m_Winding = winding;
    }

    // Somewhat burly method to interleave the data as tightly as possible
    // while taking alignment into account.
    bool SetVertexBuffer(NVConstDataRef<MeshBuilderVBufEntry> entries) override
    {
        QT3DSU32 currentOffset = 0;
        QT3DSU32 bufferAlignment = 0;
        QT3DSU32 numItems = 0;
        bool retval = true;
        QT3DSIMP_FOREACH(idx, entries.size())
        {
            const MeshBuilderVBufEntry &entry(entries[idx]);
            // Ignore entries with no data.
            if (entry.m_Data.begin() == NULL || entry.m_Data.size() == 0)
                continue;

            QT3DSU32 alignment = NVRenderComponentTypes::getSizeofType(entry.m_ComponentType);
            bufferAlignment = qMax(bufferAlignment, alignment);
            QT3DSU32 byteSize = alignment * entry.m_NumComponents;

            if (entry.m_Data.size() % alignment != 0) {
                QT3DS_ASSERT(false);
                retval = false;
            }

            QT3DSU32 localNumItems = entry.m_Data.size() / byteSize;
            if (numItems == 0) {
                numItems = localNumItems;
            } else if (numItems != localNumItems) {
                QT3DS_ASSERT(false);
                retval = false;
                numItems = qMin(numItems, localNumItems);
            }
            // Lots of platforms can't handle non-aligned data.
            // so ensure we are aligned.
            currentOffset = GetAlignedOffset(currentOffset, alignment);
            NVRenderVertexBufferEntry vbufEntry(entry.m_Name, entry.m_ComponentType,
                                                entry.m_NumComponents, currentOffset);
            m_VertexBuffer.m_VertexBufferEntries.push_back(vbufEntry);
            currentOffset += byteSize;
        }
        m_VertexBuffer.m_Stride = GetAlignedOffset(currentOffset, bufferAlignment);

        // Packed interleave the data
        QT3DSIMP_FOREACH(idx, numItems)
        {
            QT3DSU32 dataOffset = 0;
            QT3DSIMP_FOREACH(entryIdx, entries.size())
            {
                const MeshBuilderVBufEntry &entry(entries[entryIdx]);
                // Ignore entries with no data.
                if (entry.m_Data.begin() == NULL || entry.m_Data.size() == 0)
                    continue;

                QT3DSU32 alignment = NVRenderComponentTypes::getSizeofType(entry.m_ComponentType);
                QT3DSU32 byteSize = alignment * entry.m_NumComponents;
                QT3DSU32 offset = byteSize * idx;
                QT3DSU32 newOffset = GetAlignedOffset(dataOffset, alignment);
                if (newOffset != dataOffset)
                    m_VertexBuffer.m_VertexData.writeZeros(newOffset - dataOffset);
                m_VertexBuffer.m_VertexData.write(entry.m_Data.begin() + offset, byteSize);
                dataOffset = newOffset + byteSize;
            }
            QT3DS_ASSERT(dataOffset == m_VertexBuffer.m_Stride);
        }
        return retval;
    }

    void SetVertexBuffer(NVConstDataRef<NVRenderVertexBufferEntry> entries, QT3DSU32 stride,
                                 NVConstDataRef<QT3DSU8> data) override
    {
        QT3DSIMP_FOREACH(idx, (QT3DSU32)entries.size())
        {
            m_VertexBuffer.m_VertexBufferEntries.push_back(entries[idx]);
        }
        m_VertexBuffer.m_VertexData.write(data.begin(), data.size());
        if (stride == 0) {
            // Calculate the stride of the buffer using the vbuf entries
            QT3DSIMP_FOREACH(idx, entries.size())
            {
                const NVRenderVertexBufferEntry &entry(entries[idx]);
                stride = qMax(stride, entry.m_FirstItemOffset
                               + (entry.m_NumComponents * NVRenderComponentTypes::getSizeofType(
                                      entry.m_ComponentType)));
            }
        }
        m_VertexBuffer.m_Stride = stride;
    }

    void SetIndexBuffer(NVConstDataRef<QT3DSU8> data, NVRenderComponentTypes::Enum comp) override
    {
        m_IndexBuffer.m_CompType = comp;
        m_IndexBuffer.m_IndexData.write(data.begin(), data.size());
    }

    void AddJoint(QT3DSI32 jointID, QT3DSI32 parentID, const QT3DSF32 *invBindPose,
                          const QT3DSF32 *localToGlobalBoneSpace) override
    {
        m_Joints.push_back(Joint(jointID, parentID, invBindPose, localToGlobalBoneSpace));
    }

    SubsetDesc CreateSubset(const wchar_t *inName, QT3DSU32 count, QT3DSU32 offset)
    {
        if (inName == NULL)
            inName = L"";
        SubsetDesc retval(count, offset);
        retval.m_Name = QString::fromWCharArray(inName);
        return retval;
    }

    // indexBuffer QT3DS_MAX_U32 means no index buffer.
    // count of QT3DS_MAX_U32 means use all available items
    // offset means exactly what you would think.  Offset is in item size, not bytes.
    void AddMeshSubset(const wchar_t *inName, QT3DSU32 count, QT3DSU32 offset,
                               QT3DSU32 boundsPositionEntryIndex) override
    {
        SubsetDesc retval = CreateSubset(inName, count, offset);
        if (boundsPositionEntryIndex != QT3DS_MAX_U32) {
            retval.m_Bounds = Mesh::CalculateSubsetBounds(
                        m_VertexBuffer.m_VertexBufferEntries[boundsPositionEntryIndex],
                        m_VertexBuffer.m_VertexData, m_VertexBuffer.m_Stride, m_IndexBuffer.m_IndexData,
                        m_IndexBuffer.m_CompType, count, offset);
        }
        m_MeshSubsetDescs.push_back(retval);
    }

    void AddMeshSubset(const wchar_t *inName, QT3DSU32 count, QT3DSU32 offset,
                               const NVBounds3 &inBounds) override
    {
        SubsetDesc retval = CreateSubset(inName, count, offset);
        retval.m_Bounds = inBounds;
        m_MeshSubsetDescs.push_back(retval);
    }
#ifndef DISABLE_MESH_OPTIMIZATION
    void DeletePrimitiveGroup(PrimitiveGroup *&inGroup)
    {
        if (inGroup)
            delete[] inGroup;
        inGroup = NULL;
    }
#endif
    // We connect sub meshes which habe the same material
    void ConnectSubMeshes() override
    {
        if (m_MeshSubsetDescs.size() < 2) {
            // nothing to do
            return;
        }

        QT3DSU32 matDuplicates = 0;

        // as a pre-step we check if we have duplicate material at all
        for (QT3DSU32 i = 0, subsetEnd = m_MeshSubsetDescs.size(); i < subsetEnd && !matDuplicates;
             ++i) {
            SubsetDesc &currentSubset = m_MeshSubsetDescs[i];

            for (QT3DSU32 j = 0, subsetEnd = m_MeshSubsetDescs.size(); j < subsetEnd; ++j) {
                SubsetDesc &theSubset = m_MeshSubsetDescs[j];

                if (i == j)
                    continue;

                if (currentSubset.m_Name == theSubset.m_Name) {
                    matDuplicates++;
                    break; // found a duplicate bail out
                }
            }
        }

        // did we find some duplicates?
        if (matDuplicates) {
            ImportArray<SubsetDesc> newMeshSubsetDescs;
            ImportArray<SubsetDesc>::iterator theIter;
            QString curMatName;
            m_NewIndexBuffer.clear();

            for (theIter = m_MeshSubsetDescs.begin(); theIter != m_MeshSubsetDescs.end();
                 ++theIter) {
                bool bProcessed = false;

                for (ImportArray<SubsetDesc>::iterator iter = newMeshSubsetDescs.begin();
                     iter != newMeshSubsetDescs.end(); ++iter) {
                    if (theIter->m_Name == iter->m_Name) {
                        bProcessed = true;
                        break;
                    }
                }

                if (bProcessed)
                    continue;

                curMatName = theIter->m_Name;

                QT3DSU32 theIndexCompSize =
                        NVRenderComponentTypes::getSizeofType(m_IndexBuffer.m_CompType);
                // get pointer to indices
                QT3DSU8 *theIndices =
                        (m_IndexBuffer.m_IndexData.begin()) + (theIter->m_Offset * theIndexCompSize);
                // write new offset
                theIter->m_Offset = m_NewIndexBuffer.size() / theIndexCompSize;
                // store indices
                m_NewIndexBuffer.write(theIndices, theIter->m_Count * theIndexCompSize);

                for (QT3DSU32 j = 0, subsetEnd = m_MeshSubsetDescs.size(); j < subsetEnd; ++j) {
                    if (theIter == &m_MeshSubsetDescs[j])
                        continue;

                    SubsetDesc &theSubset = m_MeshSubsetDescs[j];

                    if (curMatName == theSubset.m_Name) {
                        // get pointer to indices
                        QT3DSU8 *theIndices = (m_IndexBuffer.m_IndexData.begin())
                                + (theSubset.m_Offset * theIndexCompSize);
                        // store indices
                        m_NewIndexBuffer.write(theIndices, theSubset.m_Count * theIndexCompSize);
                        // increment indices count
                        theIter->m_Count += theSubset.m_Count;
                    }
                }

                newMeshSubsetDescs.push_back(*theIter);
            }

            m_MeshSubsetDescs.clear();
            m_MeshSubsetDescs = newMeshSubsetDescs;
            m_IndexBuffer.m_IndexData.clear();
            m_IndexBuffer.m_IndexData.write(m_NewIndexBuffer.begin(), m_NewIndexBuffer.size());

            // compute new bounding box
            for (theIter = m_MeshSubsetDescs.begin(); theIter != m_MeshSubsetDescs.end();
                 ++theIter) {
                theIter->m_Bounds = Mesh::CalculateSubsetBounds(
                            m_VertexBuffer.m_VertexBufferEntries[0], m_VertexBuffer.m_VertexData,
                        m_VertexBuffer.m_Stride, m_IndexBuffer.m_IndexData, m_IndexBuffer.m_CompType,
                        theIter->m_Count, theIter->m_Offset);
            }
        }
    }

    // Here is the NVTriStrip magic.
    void OptimizeMesh() override
    {
        if (NVRenderComponentTypes::getSizeofType(m_IndexBuffer.m_CompType) != 2) {
            // we currently re-arrange unsigned int indices.
            // this is because NvTriStrip only supports short indices
            QT3DS_ASSERT(NVRenderComponentTypes::getSizeofType(m_IndexBuffer.m_CompType) == 4);
            return;
        }
#ifndef DISABLE_MESH_OPTIMIZATION
        SetCacheSize(CACHESIZE_GEFORCE3);
        SetStitchStrips(true);
        SetMinStripSize(0);
        // Create the optimized list indices
        SetListsOnly(true);
        // Optimize the indices using NvTriStrip

        // First, nv-tri-strip all of the indexes.  They shouldn't be interleaved, meaning
        // there is an assumption here that mesh subset1 doesn't uses indexes from mesh subset 2.
        // They may share vertexes, however, which means that we need to be careful when remapping
        // them.
        // Have to go subset by subset in order for the tri-strip to avoid stepping on subsets.
        m_NewIndexBuffer.clear();
        for (QT3DSU32 subsetIdx = 0, subsetEnd = m_MeshSubsetDescs.size(); subsetIdx < subsetEnd;
             ++subsetIdx) {
            SubsetDesc &theSubset = m_MeshSubsetDescs[subsetIdx];
            QT3DSU16 *theIndices =
                    reinterpret_cast<QT3DSU16 *>(m_IndexBuffer.m_IndexData.begin()) + theSubset.m_Offset;
            QT3DSU32 theIndexCount = theSubset.m_Count;
            QT3DSU16 theNumGroups = 0;
            PrimitiveGroup *thePrimitiveGroupsList = NULL;
            theSubset.m_Offset = m_NewIndexBuffer.size() / sizeof(QT3DSU16);
            theSubset.m_Count = 0;

            // We don't support larger vertex buffers.  That requires splitting the buffer,
            // an operation we haven't implemented (yet).
            if (GenerateStrips(theIndices, theIndexCount, &thePrimitiveGroupsList, &theNumGroups)) {
                if (theNumGroups) {
                    QT3DS_ASSERT(theNumGroups == 1);
                    PrimitiveGroup &srcGroup(thePrimitiveGroupsList[0]);
                    QT3DS_ASSERT(srcGroup.type == PT_LIST);
                    m_NewIndexBuffer.write(srcGroup.indices, srcGroup.numIndices);
                    theSubset.m_Count = srcGroup.numIndices;
                }
            }

            DeletePrimitiveGroup(thePrimitiveGroupsList);
        }
        m_IndexBuffer.m_IndexData.clear();
        m_IndexBuffer.m_IndexData.write(m_NewIndexBuffer.begin(), m_NewIndexBuffer.size());

#ifndef DISABLE_VERTEX_REMAP
        // This operation does not go subset by subset
        // by rather remaps the entire vertex buffer in one shot
        // once all of the index buffers are setup.
        QT3DSU16 *theIndices = reinterpret_cast<QT3DSU16 *>(m_IndexBuffer.m_IndexData.begin());
        QT3DSU32 theIndexCount = m_IndexBuffer.m_IndexData.size() / sizeof(QT3DSU16);

        // Setup input to the remap system
        QT3DSU16 theNumGroups = 1;
        PrimitiveGroup thePrimitiveGroup;
        thePrimitiveGroup.type = PT_LIST;
        thePrimitiveGroup.numIndices = theIndexCount;
        thePrimitiveGroup.indices = new QT3DSU16[theIndexCount];
        memCopy(thePrimitiveGroup.indices, theIndices, theIndexCount * sizeof(QT3DSU16));

        PrimitiveGroup *theRemappedGroup = NULL;
        QT3DSU32 vertBufByteSize = m_VertexBuffer.m_VertexData.size();
        QT3DSU32 numVertexIndices = vertBufByteSize / m_VertexBuffer.m_Stride;
        QT3DS_ASSERT(numVertexIndices < QT3DS_MAX_U16);
        QT3DSU32 vertBufStride = m_VertexBuffer.m_Stride;
        // This remaps the vertexes
        RemapIndices(&thePrimitiveGroup, theNumGroups, static_cast<QT3DSU16>(numVertexIndices),
                     &theRemappedGroup);
        m_RemappedVertexData.reserve(vertBufByteSize);
        const QT3DSU8 *srcVertexPtr(m_VertexBuffer.m_VertexData.begin());
        QT3DSU8 *dstVertexPtr(m_RemappedVertexData.begin());
        QT3DS_ASSERT(theNumGroups == 1);
        for (QT3DSU32 theGroupIdx = 0; theGroupIdx < 1; ++theGroupIdx) {
            PrimitiveGroup &srcGroup(thePrimitiveGroup);
            PrimitiveGroup &dstGroup(theRemappedGroup[theGroupIdx]);
            QT3DS_ASSERT(srcGroup.type == PT_LIST);

            for (QT3DSU32 theIndexIdx = 0; theIndexIdx < srcGroup.numIndices; ++theIndexIdx) {
                QT3DSU16 srcIndex = srcGroup.indices[theIndexIdx];
                QT3DSU16 dstIndex = dstGroup.indices[theIndexIdx];
                QT3DS_ASSERT(dstIndex * m_VertexBuffer.m_Stride < vertBufByteSize);
                QT3DS_ASSERT(srcIndex * m_VertexBuffer.m_Stride < vertBufByteSize);
                // Maybe add in a check to see if we possibly already copied this information
                // That would of course be only an optimization.
                memCopy(dstVertexPtr + dstIndex * vertBufStride,
                        srcVertexPtr + srcIndex * vertBufStride, vertBufStride);
                theIndices[theIndexIdx] = dstIndex;
            }
            memCopy(m_VertexBuffer.m_VertexData.begin(), m_RemappedVertexData.begin(),
                    vertBufByteSize);
        }

        DeletePrimitiveGroup(theRemappedGroup);
#endif // DISABLE_VERTEX_REMAP
#endif // DISABLE_MESH_OPTIMIZATION
    }

    template <typename TDataType>
    static void Assign(QT3DSU8 *inBaseAddress, QT3DSU8 *inDataAddress,
                       SOffsetDataRef<TDataType> &inBuffer, const TDataType *inDestData,
                       QT3DSU32 inDestSize)
    {
        inBuffer.m_Offset = (QT3DSU32)(inDataAddress - inBaseAddress);
        inBuffer.m_Size = inDestSize;
        memCopy(inDataAddress, inDestData, inDestSize * sizeof(TDataType));
    }
    template <typename TDataType>
    static void Assign(QT3DSU8 *inBaseAddress, QT3DSU8 *inDataAddress,
                       SOffsetDataRef<TDataType> &inBuffer, QT3DSU32 inDestSize)
    {
        inBuffer.m_Offset = (QT3DSU32)(inDataAddress - inBaseAddress);
        inBuffer.m_Size = inDestSize;
    }
    // Return the current mesh.  This is only good for this function call, item may change or be
    // released
    // due to any further function calls.
    Mesh &GetMesh() override
    {
        QT3DSU32 meshSize = sizeof(Mesh);
        QT3DSU32 alignment = sizeof(void *);
        QT3DSU32 vertDataSize = GetAlignedOffset(m_VertexBuffer.m_VertexData.size(), alignment);
        meshSize += vertDataSize;
        QT3DSU32 entrySize = m_VertexBuffer.m_VertexBufferEntries.size()
                * sizeof(qt3ds::render::NVRenderVertexBufferEntry);
        meshSize += entrySize;
        QT3DSU32 entryNameSize = 0;
        for (QT3DSU32 idx = 0, end = m_VertexBuffer.m_VertexBufferEntries.size(); idx < end; ++idx) {
            const qt3ds::render::NVRenderVertexBufferEntry &theEntry(
                        m_VertexBuffer.m_VertexBufferEntries[idx]);
            const char *entryName = theEntry.m_Name;
            if (entryName == NULL)
                entryName = "";
            entryNameSize += (QT3DSU32)(strlen(theEntry.m_Name)) + 1;
        }
        entryNameSize = GetAlignedOffset(entryNameSize, alignment);
        meshSize += entryNameSize;
        QT3DSU32 indexBufferSize = GetAlignedOffset(m_IndexBuffer.m_IndexData.size(), alignment);
        meshSize += indexBufferSize;
        QT3DSU32 subsetSize = m_MeshSubsetDescs.size() * sizeof(MeshSubset);
        QT3DSU32 nameSize = 0;
        for (QT3DSU32 idx = 0, end = m_MeshSubsetDescs.size(); idx < end; ++idx) {
            if (!m_MeshSubsetDescs[idx].m_Name.isEmpty())
                nameSize += m_MeshSubsetDescs[idx].m_Name.size() + 1;
        }
        nameSize *= sizeof(char16_t);
        nameSize = GetAlignedOffset(nameSize, alignment);

        meshSize += subsetSize + nameSize;
        QT3DSU32 jointsSize = m_Joints.size() * sizeof(qt3dsimp::Joint);
        meshSize += jointsSize;
        m_MeshBuffer.resize(meshSize);
        QT3DSU8 *baseAddress = m_MeshBuffer.data();
        Mesh *retval = reinterpret_cast<Mesh *>(baseAddress);
        retval->m_DrawMode = m_DrawMode;
        retval->m_Winding = m_Winding;
        QT3DSU8 *vertBufferData = baseAddress + sizeof(Mesh);
        QT3DSU8 *vertEntryData = vertBufferData + vertDataSize;
        QT3DSU8 *vertEntryNameData = vertEntryData + entrySize;
        QT3DSU8 *indexBufferData = vertEntryNameData + entryNameSize;
        QT3DSU8 *subsetBufferData = indexBufferData + indexBufferSize;
        QT3DSU8 *nameBufferData = subsetBufferData + subsetSize;
        QT3DSU8 *jointBufferData = nameBufferData + nameSize;

        retval->m_VertexBuffer.m_Stride = m_VertexBuffer.m_Stride;
        Assign(baseAddress, vertBufferData, retval->m_VertexBuffer.m_Data,
               m_VertexBuffer.m_VertexData.begin(), m_VertexBuffer.m_VertexData.size());
        retval->m_VertexBuffer.m_Entries.m_Size = m_VertexBuffer.m_VertexBufferEntries.size();
        retval->m_VertexBuffer.m_Entries.m_Offset = (QT3DSU32)(vertEntryData - baseAddress);
        for (QT3DSU32 idx = 0, end = m_VertexBuffer.m_VertexBufferEntries.size(); idx < end; ++idx) {
            const qt3ds::render::NVRenderVertexBufferEntry &theEntry(
                        m_VertexBuffer.m_VertexBufferEntries[idx]);
            MeshVertexBufferEntry &theDestEntry(
                        retval->m_VertexBuffer.m_Entries.index(baseAddress, idx));
            theDestEntry.m_ComponentType = theEntry.m_ComponentType;
            theDestEntry.m_FirstItemOffset = theEntry.m_FirstItemOffset;
            theDestEntry.m_NumComponents = theEntry.m_NumComponents;
            const char *targetName = theEntry.m_Name;
            if (targetName == NULL)
                targetName = "";

            QT3DSU32 entryNameLen = (QT3DSU32)(strlen(targetName)) + 1;
            theDestEntry.m_NameOffset = (QT3DSU32)(vertEntryNameData - baseAddress);
            memCopy(vertEntryNameData, theEntry.m_Name, entryNameLen);
            vertEntryNameData += entryNameLen;
        }

        retval->m_IndexBuffer.m_ComponentType = m_IndexBuffer.m_CompType;
        Assign(baseAddress, indexBufferData, retval->m_IndexBuffer.m_Data,
               m_IndexBuffer.m_IndexData.begin(), m_IndexBuffer.m_IndexData.size());
        Assign(baseAddress, subsetBufferData, retval->m_Subsets, m_MeshSubsetDescs.size());
        for (QT3DSU32 idx = 0, end = m_MeshSubsetDescs.size(); idx < end; ++idx) {
            SubsetDesc &theDesc = m_MeshSubsetDescs[idx];
            MeshSubset &theSubset = reinterpret_cast<MeshSubset *>(subsetBufferData)[idx];
            theSubset.m_Bounds = theDesc.m_Bounds;
            theSubset.m_Count = theDesc.m_Count;
            theSubset.m_Offset = theDesc.m_Offset;
            if (!theDesc.m_Name.isEmpty()) {
                theSubset.m_Name.m_Size = theDesc.m_Name.size() + 1;
                theSubset.m_Name.m_Offset = (QT3DSU32)(nameBufferData - baseAddress);
                std::transform(theDesc.m_Name.begin(), theDesc.m_Name.end(),
                               reinterpret_cast<char16_t *>(nameBufferData),
                               [](QChar c) { return static_cast<char16_t>(c.unicode()); });
                reinterpret_cast<char16_t *>(nameBufferData)[theDesc.m_Name.size()] = 0;
                nameBufferData += (theDesc.m_Name.size() + 1) * sizeof(char16_t);
            } else {
                theSubset.m_Name.m_Size = 0;
                theSubset.m_Name.m_Offset = 0;
            }
        }
        Assign(baseAddress, jointBufferData, retval->m_Joints, m_Joints.data(), m_Joints.size());
        return *retval;
    }
};
}

// Uses new/delete.
MeshBuilder &MeshBuilder::CreateMeshBuilder()
{
    return *(new MeshBuilderImpl());
}
