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

#include "q3dsmaterialdefinitionparser.h"

#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#include <QtCore/qxmlstream.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>

namespace Q3DStudio {

void Q3DSMaterialDefinitionParser::getMaterialInfo(const QString &materialDefinition,
        const QString &projPath, const QString &presPath,
        QString &outName, QMap<QString, QString> &outValues,
        QMap<QString, QMap<QString, QString>> &outTextureValues)
{
    // materialDefinition can be the .materialdef file name or its content
    QString matDef;
    if (materialDefinition.endsWith(QLatin1String(".materialdef"), Qt::CaseInsensitive)) {
        QFile file(materialDefinition);
        if (!file.open(QFile::Text | QFile::ReadOnly)) {
            qWarning() << __FUNCTION__ << "Failed to open material definition file:"
                       << file.errorString();
            return;
        }
        QTextStream stream(&file);
        matDef = stream.readAll();
    } else {
        matDef = materialDefinition;
    }

    QXmlStreamReader reader(matDef);
    reader.setNamespaceProcessing(false);

    const QDir docDir(presPath);
    const QDir projDir(projPath);
    QString textureName;
    QString propertyName;
    QString propertyType;
    QString propertyValue;
    QMap<QString, QString> textureProperties;

    const QString matDataVersion = QStringLiteral("1.0");
    const QString sourcePathStr = QStringLiteral("sourcepath");
    const QString nameStr = QStringLiteral("name");
    const QString typeStr = QStringLiteral("type");
    const QString versionStr = QStringLiteral("version");

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("TextureData")) {
                textureName = reader.attributes().value(nameStr).toString();
                textureProperties.clear();
            } else if (reader.name() == QLatin1String("Property")) {
                propertyName = reader.attributes().value(nameStr).toString();
                propertyType = reader.attributes().value(typeStr).toString();
                propertyValue.clear();
            } else if (reader.name() == QLatin1String("MaterialData")) {
                QString version = reader.attributes().value(versionStr).toString();
                if (version != matDataVersion) {
                    qWarning() << __FUNCTION__ << "Invalid MaterialData version; expected '"
                               << matDataVersion << "', got '" << version << "'";
                    outName.clear();
                    outValues.clear();
                    outTextureValues.clear();
                    return;
                }
            }
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            if (reader.name() == QLatin1String("TextureData")) {
                outTextureValues[textureName] = textureProperties;
                textureName.clear();
            } else if (reader.name() == QLatin1String("Property")) {
                if (textureName.isEmpty()) {
                    outValues[propertyName] = propertyValue;
                    if (propertyName == nameStr)
                        outName = propertyValue;
                } else {
                    textureProperties[propertyName] = propertyValue;
                }
                propertyName.clear();
                propertyType.clear();
            }
        } else if (reader.tokenType() == QXmlStreamReader::Characters) {
            if (!propertyName.isEmpty()) {
                propertyValue = reader.text().toString();
                if (propertyName == sourcePathStr) {
                    // Check that the referenced file exists
                    const auto absSourcePath = projDir.absoluteFilePath(propertyValue);
                        if (!QFileInfo(absSourcePath).exists()) {
                        qWarning() << __FUNCTION__ << "Referenced file doesn't exist:"
                                   << propertyValue;
                        outName.clear();
                        outValues.clear();
                        outTextureValues.clear();
                        return;
                    }
                }
                if (!propertyValue.isEmpty() && (propertyType == QLatin1String("Texture")
                                                 || propertyName == sourcePathStr)) {
                    propertyValue = docDir.relativeFilePath(
                                projDir.absoluteFilePath(propertyValue));
                }
            }
        }
    }
}

}
