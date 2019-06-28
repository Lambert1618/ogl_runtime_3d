/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qfile.h>
#include <QtCore/qrandom.h>
#include <QtCore/qmath.h>
#include <QtCore/qdebug.h>
#include <array>
#include "demo.h"

Demo::Demo(Q3DSPresentation *presentation, QObject *parent)
    : QObject(parent),
      m_presentation(presentation)
{
    m_elementData.resize(200);
}

void Demo::setup()
{
    createMaterials();

    QObject::connect(m_presentation, &Q3DSPresentation::materialsCreated,
                     [&](const QStringList &materialNames, const QString &error) {
        if (!error.isEmpty())
            qDebug() << "createMaterials error: " << error;
        else
            qDebug() << "materialNames: " << materialNames;

        createMeshes();
    });

    QObject::connect(m_presentation, &Q3DSPresentation::meshesCreated,
                     [&](const QStringList &meshNames, const QString &error) {
        if (!error.isEmpty())
            qDebug() << "createMeshes error: " << error;
        else
            qDebug() << "meshNames: " << meshNames;

        createElements();
    });

    QObject::connect(m_presentation, &Q3DSPresentation::elementsCreated,
                     [&](const QStringList &elementNames, const QString &error) {
        if (!error.isEmpty())
            qDebug() << "createElements error: " << error;
        else
            qDebug() << "elementNames: " << elementNames;

        setupDone = true;
    });
}

void Demo::createMaterials()
{
    m_presentation->createMaterials({QStringLiteral(":/presentation/materials/Copper.materialdef"),
                                     QStringLiteral(":/presentation/materials/Red.materialdef")});
}

void Demo::createMeshes()
{
    Q3DSGeometry obj;

    if (!parseObj(QStringLiteral(":/demo.obj"), obj))
        qWarning("Failed to parse object");

    QHash<QString, const Q3DSGeometry *> meshData;

    meshData.insert(QStringLiteral("Suzanne"), &obj);

    m_presentation->createMeshes(meshData);
}

void Demo::createElements()
{
    QRandomGenerator *rng = QRandomGenerator::system();
    QVector< QHash<QString, QVariant> > properties;

    for (int i = 0; i < m_elementData.size(); ++i) {
        double theta = rng->bounded(2.0 * M_PI);

        m_elementData[i].position = QVector3D(float(500.0 * qCos(theta)),
                                              float(500.0 * qSin(theta)),
                                              float(rng->bounded(-500, 10000)));
        m_elementData[i].rotation = QVector3D(float(rng->bounded(360.0)),
                                              float(rng->bounded(360.0)),
                                              float(rng->bounded(360.0)));
        m_elementData[i].delta = QVector3D(float(-1.0 + 2.0 * rng->generateDouble()),
                                           float(-1.0 + 2.0 * rng->generateDouble()),
                                           float(-1.0 + 2.0 * rng->generateDouble()));

        QHash<QString, QVariant> data;

        data.insert(QStringLiteral("name"), QStringLiteral("MyElement_%1").arg(i));
        data.insert(QStringLiteral("sourcepath"), QStringLiteral("Suzanne"));
        data.insert(QStringLiteral("material"),
                    i % 2 == 0 ? QStringLiteral("materials/Red") :
                                 QStringLiteral("materials/Copper"));
        data.insert(QStringLiteral("position"),
                    QVariant::fromValue<QVector3D>(m_elementData[i].position));
        data.insert(QStringLiteral("rotation"),
                    QVariant::fromValue<QVector3D>(m_elementData[i].rotation));

        properties.push_back(data);
    }

    m_presentation->createElements(QStringLiteral("Scene.Layer"),
                                   QStringLiteral("Slide1"), properties);
}

void Demo::animate()
{
    if (!setupDone)
        return;

    QRandomGenerator *rng = QRandomGenerator::system();

    for (int i = 0; i < m_elementData.size(); ++i) {
        QString e = QStringLiteral("Scene.Layer.MyElement_%1").arg(i);

        float z = m_elementData[i].position.z() - 10;

        if (z < -600) {
            double theta = rng->bounded(2.0 * M_PI);

            m_elementData[i].position = QVector3D(float(500.0 * qCos(theta)),
                                                  float(500.0 * qSin(theta)),
                                                  10000.0);
        } else {
            m_elementData[i].position.setZ(z);
        }

        m_presentation->setAttribute(e, QStringLiteral("position.x"), m_elementData[i].position.x());
        m_presentation->setAttribute(e, QStringLiteral("position.y"), m_elementData[i].position.y());
        m_presentation->setAttribute(e, QStringLiteral("position.z"), m_elementData[i].position.z());

        m_elementData[i].rotation.setX(m_elementData[i].rotation.x() + m_elementData[i].delta.x());
        m_elementData[i].rotation.setY(m_elementData[i].rotation.y() + m_elementData[i].delta.y());
        m_elementData[i].rotation.setZ(m_elementData[i].rotation.z() + m_elementData[i].delta.z());

        m_presentation->setAttribute(e, QStringLiteral("rotation.x"), m_elementData[i].rotation.x());
        m_presentation->setAttribute(e, QStringLiteral("rotation.y"), m_elementData[i].rotation.y());
        m_presentation->setAttribute(e, QStringLiteral("rotation.z"), m_elementData[i].rotation.z());
    }
}

bool Demo::parseObj(const QString &path, Q3DSGeometry &obj)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open %s", qPrintable(path));
        return false;
    }

    QTextStream in(&file);

    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<QVector2D> texCoords;

    struct Index
    {
        int v;
        int vt;
        int vn;
    };

    QVector< std::array<Index, 3> > faces;

    vertices.clear();
    normals.clear();
    texCoords.clear();
    faces.clear();

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(QLatin1Char(' '));
        if (fields[0] == QLatin1String("v")) {
            vertices.push_back(QVector3D(fields[1].toFloat(),
                               fields[2].toFloat(),
                    fields[3].toFloat()));
        } else if (fields[0] == QLatin1String("vn")) {
            normals.push_back(QVector3D(fields[1].toFloat(),
                              fields[2].toFloat(),
                    fields[3].toFloat()));
        } else if (fields[0] == QLatin1String("vt")) {
            texCoords.push_back(QVector2D(fields[1].toFloat(),
                                fields[2].toFloat()));
        }
        else if (fields[0] == QLatin1String("f")) {
            if (fields.size() != 4) {
                qWarning("Only triangle meshes supported");
                return false;
            }

            auto parseIndex = [&](QStringList v) -> Index {
                int vertex = v[0].toInt() - 1;
                int texture = v[1].toInt() - 1;
                int normal = v[2].toInt() - 1;
                return Index({ vertex, texture, normal });
            };

            Index i = parseIndex(fields[1].split(QLatin1Char('/')));
            Index j = parseIndex(fields[2].split(QLatin1Char('/')));
            Index k = parseIndex(fields[3].split(QLatin1Char('/')));

            faces.push_back({i, j, k});
        }
    }

    struct Vertex
    {
        QVector3D position;
        QVector3D normal;
        QVector2D texture;
        QVector3D textan;
        QVector3D binorm;
    };

    QVector<QVector3D> textans;
    QVector<QVector3D> binorms;

    for (int n = 0; n < faces.size(); ++n) {
        QVector3D v0 = vertices[faces[n][0].v];
        QVector3D v1 = vertices[faces[n][1].v];
        QVector3D v2 = vertices[faces[n][2].v];

        QVector2D uv0 = texCoords[faces[n][0].vt];
        QVector2D uv1 = texCoords[faces[n][1].vt];
        QVector2D uv2 = texCoords[faces[n][2].vt];

        QVector3D deltaPos1 = v1 - v0;
        QVector3D deltaPos2 = v2 - v0;

        QVector2D deltaUV1 = uv1 - uv0;
        QVector2D deltaUV2 = uv2 - uv0;

        float r = 1.0f / (deltaUV1.x() * deltaUV2.y() - deltaUV1.y() * deltaUV2.x());
        QVector3D tangent = (deltaPos1 * deltaUV2.y() - deltaPos2 * deltaUV1.y()) * r;
        QVector3D binorm = (deltaPos2 * deltaUV1.x() - deltaPos1 * deltaUV2.x()) * r;

        textans.append(tangent);
        binorms.append(binorm);
    }

    QVector<Vertex> vertexData;

    for (int n = 0; n < faces.size(); ++n) {
        for (int m = 0; m < 3; ++m) {
            int v = faces[n][m].v;
            int vt = faces[n][m].vt;
            int vn = faces[n][m].vn;

            vertexData.push_back({ vertices[v], normals[vn],
                                   vt != -1 ? texCoords[vt] : QVector2D(0, 0),
                                   textans[n], binorms[n] });
        }
    }

    QByteArray vertexBuffer(reinterpret_cast<const char *>(vertexData.constData()),
                            vertexData.size() * int(sizeof(Vertex)));

    obj.clear();
    obj.setVertexData(vertexBuffer);
    obj.addAttribute(Q3DSGeometry::Attribute::PositionSemantic);
    obj.addAttribute(Q3DSGeometry::Attribute::NormalSemantic);
    obj.addAttribute(Q3DSGeometry::Attribute::TexCoordSemantic);
    obj.addAttribute(Q3DSGeometry::Attribute::TangentSemantic);
    obj.addAttribute(Q3DSGeometry::Attribute::BinormalSemantic);

    return true;
}
