/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include "Qt3DSFontDatabase_p.h"

#include <QtCore/qdir.h>
#include "qloggingcategory.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)

QT_BEGIN_NAMESPACE

Q3DSFontDatabase::Q3DSFontDatabase()
{
}

void Q3DSFontDatabase::registerFonts(const QStringList &directories)
{
    const QStringList nameFilters = { QStringLiteral("*.ttf"), QStringLiteral("*.otf") };
    for (const QString &directory : directories) {
        QDir fontDir(directory);
        if (!fontDir.exists()) {
            qWarning("Attempted to register invalid font directory: %s",
                     qPrintable(directory));
            continue;
        }

        const QFileInfoList entryInfoList = fontDir.entryInfoList(nameFilters);
        for (const QFileInfo &entryInfo : entryInfoList) {
            QRawFont font(entryInfo.absoluteFilePath(), 16);
            if (!font.isValid()) {
                qWarning("Invalid font file: %s", qPrintable(entryInfo.absoluteFilePath()));
                continue;
            }

            // ### Only support scalable fonts

            QString fontId = entryInfo.baseName();
            if (std::find_if(m_fonts.constBegin(), m_fonts.constEnd(),
                             [fontId](const Font &f) { return f.fontId == fontId; })
                    != m_fonts.constEnd()) {
                // already registered
                continue;
            }

            m_fonts.append(Font(fontId, entryInfo.absoluteFilePath()));
        }
    }
}

void Q3DSFontDatabase::unregisterFonts(const QStringList &directories)
{
    for (const QString &directory : directories) {
        QDir dir(directory);
        QVector<Font>::iterator it = m_fonts.begin();
        while (it != m_fonts.end()) {
            if (dir == QDir(QFileInfo(it->filePath).absolutePath()))
                it = m_fonts.erase(it);
            else
                ++it;
        }
    }
}

QRawFont Q3DSFontDatabase::findFont(const QString &fontId)
{
    for (Font &font : m_fonts) {
        if (font.fontId == fontId) {
            if (!font.rawFont.isValid())
                font.rawFont = QRawFont(font.filePath, 16);
            return font.rawFont;
        }
    }

    return QRawFont::fromFont(QFont());
}

QT_END_NAMESPACE

#endif // QT_VERSION >= QT_VERSION_CHECK(5,12,2)
