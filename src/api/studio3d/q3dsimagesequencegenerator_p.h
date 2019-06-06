/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef Q3DSIMAGE_SEQUENCE_GENERATOR_H
#define Q3DSIMAGE_SEQUENCE_GENERATOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtStudio3D API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtStudio3D/qstudio3dglobal.h>
#include <QtCore/qobject.h>
#ifndef Q_OS_ANDROID
QT_BEGIN_NAMESPACE

class Q_STUDIO3D_EXPORT Q3DSImageSequenceGenerator : public QObject
{
    Q_OBJECT
public:
    Q3DSImageSequenceGenerator() {}
    virtual ~Q3DSImageSequenceGenerator() {}

    bool generateImageSequence(const QString &presentation, qreal start, qreal end,
                               qreal fps, qreal frameInterval, int width, int height,
                               const QString &outPath, const QString &outFile);
Q_SIGNALS:
    void progress(int totalFrames, int frameNumber);
    void finished(bool success, const QString &details);
};

QT_END_NAMESPACE

#endif // Q3DSIMAGE_SEQUENCE_GENERATOR_H
#endif
