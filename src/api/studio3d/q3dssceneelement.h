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

#ifndef Q3DSSCENEELEMENT_H
#define Q3DSSCENEELEMENT_H

#include <QtStudio3D/q3dselement.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class Q3DSSceneElementPrivate;

class Q_STUDIO3D_EXPORT Q3DSSceneElement : public Q3DSElement
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DSSceneElement)

    Q_PROPERTY(int currentSlideIndex READ currentSlideIndex WRITE setCurrentSlideIndex NOTIFY currentSlideIndexChanged)
    Q_PROPERTY(int previousSlideIndex READ previousSlideIndex NOTIFY previousSlideIndexChanged)
    Q_PROPERTY(QString currentSlideName READ currentSlideName WRITE setCurrentSlideName NOTIFY currentSlideNameChanged)
    Q_PROPERTY(QString previousSlideName READ previousSlideName NOTIFY previousSlideNameChanged)

public:
    explicit Q3DSSceneElement(QObject *parent = nullptr);
    explicit Q3DSSceneElement(const QString &elementPath, QObject *parent = nullptr);
    explicit Q3DSSceneElement(Q3DSPresentation *presentation, const QString &elementPath,
                              QObject *parent = nullptr);
    ~Q3DSSceneElement();

    int currentSlideIndex() const;
    int previousSlideIndex() const;
    QString currentSlideName() const;
    QString previousSlideName() const;

public Q_SLOTS:
    void setCurrentSlideIndex(int currentSlideIndex);
    void setCurrentSlideName(const QString &currentSlideName);
    void goToSlide(bool next, bool wrap);
    void goToTime(float time);

Q_SIGNALS:
    void currentSlideIndexChanged(int currentSlideIndex);
    void previousSlideIndexChanged(int previousSlideIndex);
    void currentSlideNameChanged(const QString &currentSlideName);
    void previousSlideNameChanged(const QString &previousSlideName);

private:
    Q_DISABLE_COPY(Q3DSSceneElement)

    friend class Q3DSPresentationPrivate;
    friend class Q3DSStudio3D;
};

QT_END_NAMESPACE

#endif // Q3DSSCENEELEMENT_H
