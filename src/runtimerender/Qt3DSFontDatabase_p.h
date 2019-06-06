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

#ifndef Q3DSFONTDATABASE_P_H
#define Q3DSFONTDATABASE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qvector.h>
#include <QtGui/qrawfont.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)

QT_BEGIN_NAMESPACE

class Q3DSSceneManager;
class Q3DSFontDatabase
{
public:
    Q3DSFontDatabase();

    void registerFonts(const QStringList &directories);
    void unregisterFonts(const QStringList &directories);

    QRawFont findFont(const QString &fontId);

private:
    struct Font {
        Font() = default;
        Font(const QString &id, const QString &path)
            : fontId(id)
            , filePath(path)
        {}

        QString fontId;
        QString filePath;
        QRawFont rawFont;
    };

    QVector<Font> m_fonts;
};

QT_END_NAMESPACE

#endif // QT_VERSION >= QT_VERSION_CHECK(5,12,2)

#endif // Q3DSFONTDATABASE_P_H
