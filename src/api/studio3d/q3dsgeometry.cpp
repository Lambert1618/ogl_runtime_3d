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

Q3DSGeometry::Q3DSGeometry()
    : d_ptr(new Q3DSGeometryPrivate(this))
{
}

Q3DSGeometry::~Q3DSGeometry()
{
    delete d_ptr;
}

void Q3DSGeometry::setVertexData(const QByteArray &data)
{
    d_ptr->m_meshData.m_vertexBuffer = data;
}

void Q3DSGeometry::setIndexData(const QByteArray &data)
{
    d_ptr->m_meshData.m_indexBuffer = data;
}

const QByteArray &Q3DSGeometry::vertexBuffer() const
{
    return d_ptr->m_meshData.m_vertexBuffer;
}

QByteArray &Q3DSGeometry::vertexBuffer()
{
    return d_ptr->m_meshData.m_vertexBuffer;
}

const QByteArray &Q3DSGeometry::indexBuffer() const
{
    return d_ptr->m_meshData.m_indexBuffer;
}

QByteArray &Q3DSGeometry::indexBuffer()
{
    return d_ptr->m_meshData.m_indexBuffer;
}

int Q3DSGeometry::attributeCount() const
{
    return d_ptr->m_meshData.m_attributeCount;
}

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

void Q3DSGeometry::addAttribute(const Q3DSGeometry::Attribute &att)
{
    addAttribute(att.semantic, att.componentType);
}

Q3DSGeometry::Attribute Q3DSGeometry::attribute(int idx) const
{
    Attribute att;
    att.semantic = static_cast<Q3DSGeometry::Attribute::Semantic>(
                d_ptr->m_meshData.m_attributes[idx].semantic);
    att.componentType = static_cast<Q3DSGeometry::Attribute::ComponentType>(
                d_ptr->m_meshData.m_attributes[idx].componentType);
    return att;
}

Q3DSGeometry::PrimitiveType Q3DSGeometry::primitiveType() const
{
    return static_cast<Q3DSGeometry::PrimitiveType>(d_ptr->m_meshData.m_primitiveType);
}

void Q3DSGeometry::setPrimitiveType(Q3DSGeometry::PrimitiveType type)
{
    d_ptr->m_meshData.m_primitiveType = static_cast<Q3DSViewer::MeshData::PrimitiveType>(type);
}

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
