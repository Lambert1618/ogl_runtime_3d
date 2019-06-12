/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "q3dsgeometry_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class Q3DSGeometry
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.4

    \brief Represents a mesh geometry.

    This class describes the mesh geometry for dynamic mesh creation.
    The geometry consists of a vertex buffer and an optional index buffer.
    Geometry attributes are used to define how the data in these buffers should be interpreted.

    For example, create a simple textured pyramid geometry:

    \badcode
    // A vertex in vertex buffer consists of position, normal, and texture coordinates
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D uv;
    };

    // The vertex buffer
    QVector<Vertex> vertices;

    // Creates a triangle into the vertex buffer
    auto createTriangle = [&](const QVector3D &xyz1, const QVector2D &uv1,
                              const QVector3D &xyz2, const QVector2D &uv2,
                              const QVector3D &xyz3, const QVector2D &uv3) {
        QVector3D n = QVector3D::crossProduct(xyz2 - xyz1, xyz3 - xyz1).normalized();
        vertices.append({xyz1, n, uv1});
        vertices.append({xyz2, n, uv2});
        vertices.append({xyz3, n, uv3});
    };

    // Pyramid corner coordinates in local space
    QVector3D xyz[5] = {{0, 0, 50}, {50, 50, -50}, {50, -50, -50}, {-50, -50, -50}, {-50, 50, -50}};

    // Possible texture coordinates for triangle corners
    QVector2D uv[4] = {{1, 1}, {1, 0}, {0, 0}, {0, 1}};

    // Pyramid consists of four side triangles and a bottom quad made of two triangles
    createTriangle(xyz[0], uv[0], xyz[1], uv[1], xyz[2], uv[2]);
    createTriangle(xyz[0], uv[0], xyz[2], uv[1], xyz[3], uv[2]);
    createTriangle(xyz[0], uv[0], xyz[3], uv[1], xyz[4], uv[2]);
    createTriangle(xyz[0], uv[0], xyz[4], uv[1], xyz[1], uv[2]);
    createTriangle(xyz[1], uv[0], xyz[4], uv[2], xyz[3], uv[1]);
    createTriangle(xyz[1], uv[0], xyz[3], uv[3], xyz[2], uv[2]);

    // Make a byte array out of the vertex buffer
    QByteArray vertexBuffer(reinterpret_cast<const char *>(vertices.constData()),
                            vertices.size() * int(sizeof(Vertex)));

    // Create the geometry. Triangle is the default primitive type, so we don't specify it.
    // The order of the added attributes must match the order of the attribute data in the
    // vertex buffer.
    Q3DSGeometry pyramid;
    pyramid.setVertexData(vertexBuffer);
    pyramid.addAttribute(Q3DSGeometry::Attribute::PositionSemantic);
    pyramid.addAttribute(Q3DSGeometry::Attribute::NormalSemantic);
    pyramid.addAttribute(Q3DSGeometry::Attribute::TexCoordSemantic);
    \endcode

    \sa Q3DSPresentation::createMesh
 */

/*!
    \enum Q3DSGeometry::PrimitiveType

    This enumeration specifies the possible rendering primitives for the geometry.
    For more information about rendering primitives and how they affect the vertex data,
    see OpenGL documentation.

    \value UnknownType Primitive type is unknown.
    \value Points Geometry uses point primitives.
    \value LineStrip Geometry uses line strip primitives.
    \value LineLoop Geometry uses line loop primitives.
    \value Lines Geometry uses line primitives.
    \value TriangleStrip Geometry uses triangle strip primitives.
    \value TriangleFan Geometry uses triangle fan primitives.
    \value Triangles Geometry uses triangle primitives. This is the default primitive type.
    \value Patches Geometry uses patch primitives.
*/

/*!
    \enum Q3DSGeometry::Attribute::Semantic

    This enumeration specifies the possible attribute semantics for the geometry.
    The attribute semantic indicates the purpose of the attribute.

    \value UnknownSemantic Attribute semantic is unknown.
    \value IndexSemantic Attribute specifies index buffer data type.
    \value PositionSemantic Attribute specifies vertex position attribute
                            (\c{attr_pos} in shaders).
                            Attribute has three components.
    \value NormalSemantic Attribute specifies vertex normal attribute
                            (\c{attr_norm} in shaders).
                            Attribute has three components.
    \value TexCoordSemantic Attribute specifies vertex texture coordinate attribute
                            (\c{attr_uv0} in shaders).
                            Attribute has two components.
    \value TangentSemantic Attribute specifies vertex tangent attribute
                            (\c{attr_textan} in shaders).
                            Attribute has three components.
    \value BinormalSemantic Attribute specifies vertex binormal attribute
                            (\c{attr_binormal} in shaders).
                            Attribute has three components.
*/

/*!
    \enum Q3DSGeometry::Attribute::ComponentType

    This enumeration specifies the possible attribute component types for the geometry.
    The attribute component type indicates how the attribute component data should be interpreted.

    \value DefaultType Use the default type for the attribute.
    \value U8Type Component data is unsigned 8 bit integer.
    \value I8Type Component data is signed 8 bit integer.
    \value U16Type Component data is unsigned 16 bit integer.
    \value I16Type Component data is signed 16 bit integer.
    \value U32Type Component data is unsigned 32 bit integer.
           Default component type for attributes with IndexSemantic.
    \value I32Type Component data is signed 32 bit integer.
    \value U64Type Component data is unsigned 64 bit integer.
    \value I64Type Component data is signed 64 bit integer.
    \value F16Type Component data is 16 bit float.
    \value F32Type Component data is 32 bit float.
           Default component type for attributes with a semantic other than IndexSemantic.
    \value F64Type Component data is 64 bit float.
*/

/*!
    Constructs a new Q3DSGeometry instance.
 */
Q3DSGeometry::Q3DSGeometry()
    : d_ptr(new Q3DSGeometryPrivate(this))
{
}

/*!
    Destructor.
 */
Q3DSGeometry::~Q3DSGeometry()
{
    delete d_ptr;
}

/*!
    Sets the vertex buffer to \a data. The \a data must contain all attribute data in interleaved
    format. You must also add attributes to the geometry to specify how the vertex buffer
    data should be interpreted.

    \sa addAttribute
    \sa vertexBuffer
 */
void Q3DSGeometry::setVertexData(const QByteArray &data)
{
    d_ptr->m_meshData.m_vertexBuffer = data;
}

/*!
    Sets the index buffer to \a data.
    You must also add an attribute with \c IndexSemantic to the geometry.

    \sa addAttribute
    \sa indexBuffer
    \sa Attribute::Semantic
 */
void Q3DSGeometry::setIndexData(const QByteArray &data)
{
    d_ptr->m_meshData.m_indexBuffer = data;
}

/*!
    Returns the currently set vertex buffer data.

    \sa setVertexData
 */
const QByteArray &Q3DSGeometry::vertexBuffer() const
{
    return d_ptr->m_meshData.m_vertexBuffer;
}

/*!
    This is an overloaded function.
 */
QByteArray &Q3DSGeometry::vertexBuffer()
{
    return d_ptr->m_meshData.m_vertexBuffer;
}

/*!
    Returns the currently set index buffer data.

    \sa setIndexData
 */
const QByteArray &Q3DSGeometry::indexBuffer() const
{
    return d_ptr->m_meshData.m_indexBuffer;
}

/*!
    This is an overloaded function.
 */
QByteArray &Q3DSGeometry::indexBuffer()
{
    return d_ptr->m_meshData.m_indexBuffer;
}

/*!
    Returns the number of attributes set to this geometry.

    \sa addAttribute
 */
int Q3DSGeometry::attributeCount() const
{
    return d_ptr->m_meshData.m_attributeCount;
}

/*!
    Sets an attribute to this geometry. The geometry attributes specify how the data in vertex and
    index buffers should be interpreted. Each attribute is composed of a \a semantic, which
    indicates which vertex attribute this attribute refers to and \a componentType, which indicates
    the data type of each component of the attribute data. The \a semantic also determines
    the component count in vertex buffer for the attribute. The component count is two for
    TexCoordSemantic and three for other vertex buffer semantics.
    Component count for index buffer is always one.

    For example, PositionSemantic specifies the vertex position in local space, so it is composed
    of three components: x, y, and z-coordinates.

    The order of addAttribute calls must match the order of the attributes in vertex data.
    The order is relevant as it is used to calculate the offset and stride of each attribute.
 */
void Q3DSGeometry::addAttribute(Q3DSGeometry::Attribute::Semantic semantic,
                                Q3DSGeometry::Attribute::ComponentType componentType)
{
    Q_ASSERT(d_ptr->m_meshData.m_attributeCount < Q3DSViewer::MeshData::MAX_ATTRIBUTES);

    Q3DSViewer::MeshData::Attribute &att
            = d_ptr->m_meshData.m_attributes[d_ptr->m_meshData.m_attributeCount];

    Q3DSGeometry::Attribute::ComponentType theCompType = componentType;
    if (theCompType == Attribute::DefaultType) {
        theCompType = semantic == Attribute::IndexSemantic ? Attribute::U32Type
                                                           : Attribute::F32Type;
    }
    att.semantic = static_cast<Q3DSViewer::MeshData::Attribute::Semantic>(semantic);
    att.componentType = static_cast<Q3DSViewer::MeshData::Attribute::ComponentType>(theCompType);
    if (semantic != Q3DSGeometry::Attribute::IndexSemantic)
        att.offset = d_ptr->getNextAttributeOffset();

    d_ptr->m_meshData.m_attributeCount++;
    d_ptr->m_meshData.m_stride = d_ptr->getNextAttributeOffset();
}

/*!
    This is an overloaded function.
 */
void Q3DSGeometry::addAttribute(const Q3DSGeometry::Attribute &att)
{
    addAttribute(att.semantic, att.componentType);
}

/*!
    Returns an added attribute with index \a idx.

    \sa addAttribute
    \sa attributeCount
 */
Q3DSGeometry::Attribute Q3DSGeometry::attribute(int idx) const
{
    Attribute att;
    att.semantic = static_cast<Q3DSGeometry::Attribute::Semantic>(
                d_ptr->m_meshData.m_attributes[idx].semantic);
    att.componentType = static_cast<Q3DSGeometry::Attribute::ComponentType>(
                d_ptr->m_meshData.m_attributes[idx].componentType);
    return att;
}

/*!
    Returns the primitive type of this geometry.

    \sa setPrimitiveType
 */
Q3DSGeometry::PrimitiveType Q3DSGeometry::primitiveType() const
{
    return static_cast<Q3DSGeometry::PrimitiveType>(d_ptr->m_meshData.m_primitiveType);
}

/*!
    Sets the primitive type of this geometry to \a type.

    \sa primitiveType
 */
void Q3DSGeometry::setPrimitiveType(Q3DSGeometry::PrimitiveType type)
{
    d_ptr->m_meshData.m_primitiveType = static_cast<Q3DSViewer::MeshData::PrimitiveType>(type);
}

/*!
    Removes all added attributes and buffers and resets the geometry to an uninitialized state.

    \sa primitiveType
 */
void Q3DSGeometry::clear()
{
    d_ptr->m_meshData.clear();
}

Q3DSGeometryPrivate::Q3DSGeometryPrivate(Q3DSGeometry *parent)
    : q_ptr(parent)
{
}

Q3DSGeometryPrivate::~Q3DSGeometryPrivate()
{
}

int Q3DSGeometryPrivate::getNextAttributeOffset() const
{
    int retval = 0;
    for (int i = 0; i < m_meshData.m_attributeCount; ++i) {
        if (m_meshData.m_attributes[i].semantic != Q3DSViewer::MeshData::Attribute::IndexSemantic) {
            retval += m_meshData.m_attributes[i].typeSize()
                    * m_meshData.m_attributes[i].componentCount();
        }
    }
    return retval;
}

QT_END_NAMESPACE
