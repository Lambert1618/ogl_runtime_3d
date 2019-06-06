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
#ifndef QT3DS_IMPORT_MESH_H
#define QT3DS_IMPORT_MESH_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSDataRef.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSBounds3.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/IOStreams.h"

namespace qt3dsimp {
using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace qt3ds::render;
template <typename TDataType>
struct SOffsetDataRef
{
    QT3DSU32 m_Offset;
    QT3DSU32 m_Size;
    SOffsetDataRef()
        : m_Offset(0)
        , m_Size(0)
    {
    }
    TDataType *begin(QT3DSU8 *inBase) { return reinterpret_cast<TDataType *>(inBase + m_Offset); }
    TDataType *end(QT3DSU8 *inBase) { return begin(inBase) + m_Size; }
    const TDataType *begin(const QT3DSU8 *inBase) const
    {
        return reinterpret_cast<const TDataType *>(inBase + m_Offset);
    }
    const TDataType *end(const QT3DSU8 *inBase) const { return begin(inBase) + m_Size; }
    QT3DSU32 size() const { return m_Size; }
    bool empty() const { return m_Size == 0; }
    TDataType &index(QT3DSU8 *inBase, QT3DSU32 idx)
    {
        QT3DS_ASSERT(idx < m_Size);
        return begin(inBase)[idx];
    }
    const TDataType &index(const QT3DSU8 *inBase, QT3DSU32 idx) const
    {
        QT3DS_ASSERT(idx < m_Size);
        return begin(inBase)[idx];
    }
};

struct MeshVertexBufferEntry
{
    QT3DSU32 m_NameOffset;
    /** Datatype of the this entry points to in the buffer */
    NVRenderComponentTypes::Enum m_ComponentType;
    /** Number of components of each data member. 1,2,3, or 4.  Don't be stupid.*/
    QT3DSU32 m_NumComponents;
    /** Offset from the beginning of the buffer of the first item */
    QT3DSU32 m_FirstItemOffset;
    MeshVertexBufferEntry()
        : m_NameOffset(0)
        , m_ComponentType(NVRenderComponentTypes::QT3DSF32)
        , m_NumComponents(3)
        , m_FirstItemOffset(0)
    {
    }
    NVRenderVertexBufferEntry ToVertexBufferEntry(QT3DSU8 *inBaseAddress)
    {
        const char *nameBuffer = "";
        if (m_NameOffset)
            nameBuffer = reinterpret_cast<const char *>(inBaseAddress + m_NameOffset);
        return NVRenderVertexBufferEntry(nameBuffer, m_ComponentType, m_NumComponents,
                                         m_FirstItemOffset);
    }
};

struct VertexBuffer
{
    SOffsetDataRef<MeshVertexBufferEntry> m_Entries;
    QT3DSU32 m_Stride;
    SOffsetDataRef<QT3DSU8> m_Data;
    VertexBuffer(SOffsetDataRef<MeshVertexBufferEntry> entries, QT3DSU32 stride,
                 SOffsetDataRef<QT3DSU8> data)
        : m_Entries(entries)
        , m_Stride(stride)
        , m_Data(data)
    {
    }
    VertexBuffer()
        : m_Stride(0)
    {
    }
};

struct IndexBuffer
{
    // Component types must be either QT3DSU16 or QT3DSU8 in order for the
    // graphics hardware to deal with the buffer correctly.
    NVRenderComponentTypes::Enum m_ComponentType;
    SOffsetDataRef<QT3DSU8> m_Data;
    // Either QT3DSU8 or QT3DSU16 component types are allowed by the underlying rendering
    // system, so you would be wise to stick with those.
    IndexBuffer(NVRenderComponentTypes::Enum compType, SOffsetDataRef<QT3DSU8> data)
        : m_ComponentType(compType)
        , m_Data(data)
    {
    }
    IndexBuffer()
        : m_ComponentType(NVRenderComponentTypes::Unknown)
    {
    }
};

template <QT3DSU32 TNumBytes>
struct MeshPadding
{
    QT3DSU8 m_Padding[TNumBytes];
    MeshPadding() { memZero(m_Padding, TNumBytes); }
};

struct MeshSubset
{
    // QT3DS_MAX_U32 means use all available items
    QT3DSU32 m_Count;
    // Offset is in item size, not bytes.
    QT3DSU32 m_Offset;
    // Bounds of this subset.  This is filled in by the builder
    // see AddMeshSubset
    NVBounds3 m_Bounds;

    // Subsets have to be named else artists will be unable to use
    // a mesh with multiple subsets as they won't have any idea
    // while part of the model a given mesh actually maps to.
    SOffsetDataRef<char16_t> m_Name;

    MeshSubset(QT3DSU32 count, QT3DSU32 off, const NVBounds3 &bounds, SOffsetDataRef<char16_t> inName)
        : m_Count(count)
        , m_Offset(off)
        , m_Bounds(bounds)
        , m_Name(inName)
    {
    }
    MeshSubset()
        : m_Count((QT3DSU32)-1)
        , m_Offset(0)
        , m_Bounds(NVBounds3::empty())
    {
    }
    bool HasCount() const { return m_Count != QT3DS_MAX_U32; }
};

// Placeholder for bitflags in the mesh header.
struct MeshBufHeaderFlagValues
{
    enum Enum {};
};

struct MeshBufHeaderFlags : NVFlags<MeshBufHeaderFlagValues::Enum, QT3DSU16>
{
    MeshBufHeaderFlags() {}
    MeshBufHeaderFlags(QT3DSU16 value)
        : NVFlags<MeshBufHeaderFlagValues::Enum, QT3DSU16>(value)
    {
    }
    // Binary or creates an integer.
    MeshBufHeaderFlags(int value)
        : NVFlags<MeshBufHeaderFlagValues::Enum, QT3DSU16>((QT3DSU16)value)
    {
    }
};

struct MeshDataHeader
{
    static QT3DSU32 GetFileId() { return (QT3DSU32)-929005747; }
    static QT3DSU16 GetCurrentFileVersion() { return 3; }
    QT3DSU32 m_FileId;
    QT3DSU16 m_FileVersion;
    MeshBufHeaderFlags m_HeaderFlags;
    QT3DSU32 m_SizeInBytes;
    MeshDataHeader(QT3DSU32 size = 0)
        : m_FileId(GetFileId())
        , m_FileVersion(GetCurrentFileVersion())
        , m_SizeInBytes(size)
    {
    }
};

struct Joint
{
    QT3DSI32 m_JointID;
    QT3DSI32 m_ParentID;
    QT3DSF32 m_invBindPose[16];
    QT3DSF32 m_localToGlobalBoneSpace[16];

    Joint(QT3DSI32 jointID, QT3DSI32 parentID, const QT3DSF32 *invBindPose,
          const QT3DSF32 *localToGlobalBoneSpace)
        : m_JointID(jointID)
        , m_ParentID(parentID)
    {
        ::memcpy(m_invBindPose, invBindPose, sizeof(float) * 16);
        ::memcpy(m_localToGlobalBoneSpace, localToGlobalBoneSpace, sizeof(float) * 16);
    }
    Joint()
        : m_JointID(-1)
        , m_ParentID(-1)
    {
        ::memset(m_invBindPose, 0, sizeof(float) * 16);
        ::memset(m_localToGlobalBoneSpace, 0, sizeof(float) * 16);
    }
};

// Tells us what offset a mesh with this ID starts.
struct MeshMultiEntry
{
    QT3DSU64 m_MeshOffset;
    QT3DSU32 m_MeshId;
    QT3DSU32 m_Padding;
    MeshMultiEntry()
        : m_MeshOffset(0)
        , m_MeshId(0)
        , m_Padding(0)
    {
    }
    MeshMultiEntry(QT3DSU64 mo, QT3DSU32 meshId)
        : m_MeshOffset(mo)
        , m_MeshId(meshId)
        , m_Padding(0)
    {
    }
};

// The multi headers are actually saved at the end of the file.
// Thus when you append to the file we overwrite the last header
// then write out a new header structure.
// The last 8 bytes of the file contain the multi header.
// The previous N*8 bytes contain the mesh entries.
struct MeshMultiHeader
{
    QT3DSU32 m_FileId;
    QT3DSU32 m_Version;
    SOffsetDataRef<MeshMultiEntry> m_Entries;
    static QT3DSU32 GetMultiStaticFileId() { return (QT3DSU32)555777497; }
    static QT3DSU32 GetMultiStaticVersion() { return 1; }

    MeshMultiHeader()
        : m_FileId(GetMultiStaticFileId())
        , m_Version(GetMultiStaticVersion())
    {
    }
};

struct Mesh;

// Result of a multi-load operation.  This returns both the mesh
// and the id of the mesh that was loaded.
struct SMultiLoadResult
{
    Mesh *m_Mesh;
    QT3DSU32 m_Id;
    SMultiLoadResult(Mesh *inMesh, QT3DSU32 inId)
        : m_Mesh(inMesh)
        , m_Id(inId)
    {
    }
    SMultiLoadResult()
        : m_Mesh(NULL)
        , m_Id(0)
    {
    }
    operator Mesh *() { return m_Mesh; }
};

/**
*	A Mesh defines one vertex buffer layout, one or more logical vertex buffers
*	one index buffer, and a set of defined draw calls (subsets).
*
*	The vertex buffer data is held together continguously and the layout cannot
*	change.  There can be several actual vertex buffers on the card, however, in order
*	to facilitate using index buffer components of smaller component sizes than there
*	are actual vertex buffer data entries.  For instance, you may have a very large vertex
*	buffer, larger than 64 K but openglES has a restriction on the size of the index buffer
*component
*	that it cannot be larger than two bytes per entry.  So you would need to split the vertex
*buffer
*	into multiple logical vertex buffers and adjust your indexes such that you indexed into
*	only one logical vertex buffer per draw call.
*
*	No logical vertex buffers means that the vertex buffer index on a mesh subset will be
*ignored.
*/
struct Mesh
{
    static const wchar_t *s_DefaultName;

    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;
    SOffsetDataRef<MeshSubset> m_Subsets;
    SOffsetDataRef<Joint> m_Joints;
    NVRenderDrawMode::Enum m_DrawMode;
    NVRenderWinding::Enum m_Winding;

    Mesh()
        : m_DrawMode(NVRenderDrawMode::Triangles)
        , m_Winding(NVRenderWinding::CounterClockwise)
    {
    }
    Mesh(VertexBuffer vbuf, IndexBuffer ibuf, const SOffsetDataRef<MeshSubset> &insts,
         const SOffsetDataRef<Joint> &joints,
         NVRenderDrawMode::Enum drawMode = NVRenderDrawMode::Triangles,
         NVRenderWinding::Enum winding = NVRenderWinding::CounterClockwise)
        : m_VertexBuffer(vbuf)
        , m_IndexBuffer(ibuf)
        , m_Subsets(insts)
        , m_Joints(joints)
        , m_DrawMode(drawMode)
        , m_Winding(winding)
    {
    }

    QT3DSU8 *GetBaseAddress() { return reinterpret_cast<QT3DSU8 *>(this); }
    const QT3DSU8 *GetBaseAddress() const { return reinterpret_cast<const QT3DSU8 *>(this); }

    static const char *GetPositionAttrName() { return "attr_pos"; }
    static const char *GetNormalAttrName() { return "attr_norm"; }
    static const char *GetUVAttrName() { return "attr_uv0"; }
    static const char *GetUV2AttrName() { return "attr_uv1"; }
    static const char *GetTexTanAttrName() { return "attr_textan"; }
    static const char *GetTexBinormalAttrName() { return "attr_binormal"; }
    static const char *GetWeightAttrName() { return "attr_weight"; }
    static const char *GetBoneIndexAttrName() { return "attr_boneid"; }
    static const char *GetColorAttrName() { return "attr_color"; }

    // Run through the vertex buffer items indicated by subset
    // Assume vbuf entry[posEntryIndex] is the position entry
    // This entry has to be QT3DSF32 and 3 components.
    // Using this entry and the (possibly empty) index buffer
    // along with the (possibly emtpy) logical vbuf data
    // return a bounds of the given vertex buffer.
    static NVBounds3 CalculateSubsetBounds(const NVRenderVertexBufferEntry &inEntry,
                                           NVConstDataRef<QT3DSU8> inVertxData, QT3DSU32 inStride,
                                           NVConstDataRef<QT3DSU8> inIndexData,
                                           qt3ds::render::NVRenderComponentTypes::Enum inIndexCompType,
                                           QT3DSU32 inSubsetCount, QT3DSU32 inSubsetOffset);

    // Format is:
    // MeshDataHeader
    // mesh data.
    void Save(IOutStream &outStream) const;

    // Save a mesh using fopen and fwrite
    bool Save(const char *inFilePath) const;

    // read the header, then read the object.
    // Object data is written in LE format for now.
    // Free the new mesh by calling:
    // alloc.deallocate( mesh );
    // All the memory is allocated once and then pointers are back
    // filled, so this will work.
    static Mesh *Load(NVAllocatorCallback &alloc, IInStream &inStream);

    // Load a mesh using fopen and fread
    // Mesh needs to be freed by the caller using free
    static Mesh *Load(const char *inFilePath);

    // Create a mesh given this header, and that data.  data.size() must match
    // header.SizeInBytes.  The mesh returned starts a data[0], so however data
    // was allocated is how the mesh should be deallocated.
    static Mesh *Initialize(QT3DSU16 meshVersion, MeshBufHeaderFlags meshFlags, NVDataRef<QT3DSU8> data);

    // Multimesh support where you have multiple meshes in a single file.
    // Save multi where you have overridden the allocator.
    QT3DSU32 SaveMulti(NVAllocatorCallback &alloc, ISeekableIOStream &inStream, QT3DSU32 inId = 0) const;
    // You can save multiple meshes in a file.  Each mesh returns an incrementing
    // integer for the multi file.  The original meshes aren't changed, and the file
    // is appended to.
    QT3DSU32 SaveMulti(const char *inFilePath) const;

    // Load a single mesh directly from a multi file with the provided overridden items
    // Loading a multimesh with id == 0 indicates to just load the mesh with the highest id.
    static SMultiLoadResult LoadMulti(NVAllocatorCallback &alloc, ISeekableIOStream &inStream,
                                      QT3DSU32 inId = 0);
    // Load a single mesh using c file API and malloc/free.
    static SMultiLoadResult LoadMulti(const char *inFilePath, QT3DSU32 inId);
    // Returns true if this is a multimesh (several meshes in one file).
    static bool IsMulti(ISeekableIOStream &inStream);
    // Load a multi header from a stream.
    static MeshMultiHeader *LoadMultiHeader(NVAllocatorCallback &alloc,
                                            ISeekableIOStream &inStream);
    // Load a multi header from a file using malloc.  Header needs to be freed using free.
    static MeshMultiHeader *LoadMultiHeader(const char *inFilePath);

    // Get the highest mesh version from a stream.
    static QT3DSU32 GetHighestMultiVersion(NVAllocatorCallback &alloc, ISeekableIOStream &inStream);
    // Get the highest mesh version from a file.
    static QT3DSU32 GetHighestMultiVersion(const char *inFilePath);
};

struct ScopedMesh
{
    Mesh *m_Mesh;
    NVAllocatorCallback *m_Callback;
    ScopedMesh(Mesh *m, NVAllocatorCallback *cback = NULL)
        : m_Mesh(m)
        , m_Callback(cback)
    {
    }
    ~ScopedMesh()
    {
        if (m_Mesh) {
            if (m_Callback)
                m_Callback->deallocate(m_Mesh);
            else
                free(m_Mesh);
        }
    }
    Mesh *operator->() { return m_Mesh; }
    operator Mesh *() { return m_Mesh; }
};

struct MeshBuilderVBufEntry
{
    const char *m_Name;
    NVConstDataRef<QT3DSU8> m_Data;
    NVRenderComponentTypes::Enum m_ComponentType;
    QT3DSU32 m_NumComponents;
    MeshBuilderVBufEntry()
        : m_Name(NULL)
        , m_ComponentType(NVRenderComponentTypes::Unknown)
        , m_NumComponents(0)
    {
    }
    MeshBuilderVBufEntry(const char *name, NVConstDataRef<QT3DSU8> data,
                         NVRenderComponentTypes::Enum componentType, QT3DSU32 numComponents)
        : m_Name(name)
        , m_Data(data)
        , m_ComponentType(componentType)
        , m_NumComponents(numComponents)
    {
    }
};

// Useful class to build up a mesh.  Necessary since meshes don't include that
// sort of utility.
class MeshBuilder
{
protected:
    virtual ~MeshBuilder() {}
public:
    virtual void Release() = 0;
    virtual void Reset() = 0;
    // Set the draw parameters for any subsets.  Defaults to triangles and counter clockwise
    virtual void SetDrawParameters(NVRenderDrawMode::Enum drawMode,
                                   NVRenderWinding::Enum winding) = 0;
    // Set the vertex buffer and have the mesh builder interleave the data for you
    virtual bool SetVertexBuffer(NVConstDataRef<MeshBuilderVBufEntry> entries) = 0;
    // Set the vertex buffer from interleaved data.
    virtual void SetVertexBuffer(NVConstDataRef<NVRenderVertexBufferEntry> entries, QT3DSU32 stride,
                                 NVConstDataRef<QT3DSU8> data) = 0;
    // The builder (and the majority of the rest of the product) only supports unsigned 16 bit
    // indexes
    virtual void SetIndexBuffer(NVConstDataRef<QT3DSU8> data, NVRenderComponentTypes::Enum comp) = 0;
    // Assets if the supplied parameters are out of range.
    virtual void AddJoint(QT3DSI32 jointID, QT3DSI32 parentID, const QT3DSF32 *invBindPose,
                          const QT3DSF32 *localToGlobalBoneSpace) = 0;
    /**
    *	Add a subset, which equates roughly to a draw call.
    *	A logical vertex buffer allows you to have more that 64K vertexes but still
    *	use u16 index buffers.  In any case, if the mesh has an index buffer then this subset
    *	refers to that index buffer, else it is assumed to index into the vertex buffer.
    *	count and offset do exactly what they seem to do, while boundsPositionEntryIndex, if set to
    *	something other than QT3DS_MAX_U32, drives the calculation of the aa-bounds of the subset
    *	using mesh::CalculateSubsetBounds
    */
    virtual void AddMeshSubset(const wchar_t *inSubsetName = Mesh::s_DefaultName,
                               QT3DSU32 count = QT3DS_MAX_U32, QT3DSU32 offset = 0,
                               QT3DSU32 boundsPositionEntryIndex = QT3DS_MAX_U32) = 0;

    virtual void AddMeshSubset(const wchar_t *inSubsetName, QT3DSU32 count, QT3DSU32 offset,
                               const NVBounds3 &inBounds) = 0;

    // Call to optimize the index and vertex buffers.  This doesn't change the subset information,
    // each triangle is rendered precisely the same.
    // It just orders the vertex data so we iterate through it as linearly as possible.
    // This *only* works if the *entire* builder is using triangles as the draw mode.  This will be
    // a disaster if that
    // condition is not met.
    virtual void OptimizeMesh() = 0;

    /**
    * @brief This functions stitches together sub-meshes with the same material.
    *		 This re-writes the index buffer
    *
    * @return no return.
    */
    virtual void ConnectSubMeshes() = 0;

    // Return the current mesh.  This is only good for this function call, item may change or be
    // released
    // due to any further function calls.
    virtual Mesh &GetMesh() = 0;

    // Uses new/delete.
    static MeshBuilder &CreateMeshBuilder();
};
}

#endif
