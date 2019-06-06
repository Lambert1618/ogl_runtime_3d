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

#ifndef Q3DSGEOMETRY_H
#define Q3DSGEOMETRY_H

#include <QtStudio3D/qstudio3dglobal.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class Q3DSGeometryPrivate;

class Q_STUDIO3D_EXPORT Q3DSGeometry
{
public:
    Q_DECLARE_PRIVATE(Q3DSGeometry)

    // All enums must match the ones defined by MeshData::Attribute struct in Qt3DSViewerApp.h
    enum PrimitiveType {
        UnknownType = 0,
        Points,
        LineStrip,
        LineLoop,
        Lines,
        TriangleStrip,
        TriangleFan,
        Triangles, // Default primitive type
        Patches
    };

    struct Attribute {
        enum Semantic {
            UnknownSemantic = 0,
            IndexSemantic,
            PositionSemantic, // attr_pos
            NormalSemantic,   // attr_norm
            TexCoordSemantic, // attr_uv0
            TangentSemantic,  // attr_textan
            BinormalSemantic  // attr_binormal
        };
        enum ComponentType {
            DefaultType = 0,
            U8Type,
            I8Type,
            U16Type,
            I16Type,
            U32Type, // Default for IndexSemantic
            I32Type,
            U64Type,
            I64Type,
            F16Type,
            F32Type, // Default for other semantics
            F64Type
        };
        Semantic semantic = PositionSemantic;
        ComponentType componentType = DefaultType;
    };

    explicit Q3DSGeometry();
    virtual ~Q3DSGeometry();

    void setVertexData(const QByteArray &data);
    void setIndexData(const QByteArray &data);

    const QByteArray &vertexBuffer() const;
    QByteArray &vertexBuffer();
    const QByteArray &indexBuffer() const;
    QByteArray &indexBuffer();

    int attributeCount() const;
    void addAttribute(Attribute::Semantic semantic,
                      Attribute::ComponentType componentType = Attribute::DefaultType);
    void addAttribute(const Attribute &att);
    Attribute attribute(int idx) const;

    PrimitiveType primitiveType() const;
    void setPrimitiveType(PrimitiveType type);

    void clear();

protected:
    Q_DISABLE_COPY(Q3DSGeometry)
    Q3DSGeometryPrivate *d_ptr;

    friend class Q3DSPresentation;
};

QT_END_NAMESPACE

#endif // Q3DSGEOMETRY_H
