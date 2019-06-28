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

#ifndef Q3DSVIEWERSETTINGS_H
#define Q3DSVIEWERSETTINGS_H

#include <QtStudio3D/qstudio3dglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class Q3DSViewerSettingsPrivate;

class Q_STUDIO3D_EXPORT Q3DSViewerSettings : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DSViewerSettings)
    Q_ENUMS(ShadeMode)
    Q_ENUMS(ScaleMode)

    Q_PROPERTY(bool matteEnabled READ matteEnabled WRITE setMatteEnabled NOTIFY matteEnabledChanged)
    Q_PROPERTY(QColor matteColor READ matteColor WRITE setMatteColor NOTIFY matteColorChanged)
    Q_PROPERTY(bool showRenderStats READ isShowRenderStats WRITE setShowRenderStats NOTIFY showRenderStatsChanged)
    Q_PROPERTY(ScaleMode scaleMode READ scaleMode WRITE setScaleMode NOTIFY scaleModeChanged)

public:
    enum ShadeMode {
        ShadeModeShaded,
        ShadeModeShadedWireframe
    };

    enum ScaleMode {
        ScaleModeFit,
        ScaleModeFill,
        ScaleModeCenter
    };

    explicit Q3DSViewerSettings(QObject *parent = nullptr);
    ~Q3DSViewerSettings();

    bool matteEnabled() const;
    QColor matteColor() const;
    bool isShowRenderStats() const;
    ScaleMode scaleMode() const;

    Q_INVOKABLE void save(const QString &group, const QString &organization = QString(),
                          const QString &application = QString());
    Q_INVOKABLE void load(const QString &group, const QString &organization = QString(),
                          const QString &application = QString());

public Q_SLOTS:
    void setMatteEnabled(bool enabled);
    void setMatteColor(const QColor &color);
    void setShowRenderStats(bool show);
    void setScaleMode(ScaleMode mode);

Q_SIGNALS:
    void matteEnabledChanged(bool enabled);
    void matteColorChanged(const QColor &color);
    void showRenderStatsChanged(bool show);
    void shadeModeChanged(ShadeMode mode);
    void scaleModeChanged(ScaleMode mode);

private:
    Q_DISABLE_COPY(Q3DSViewerSettings)
    Q3DSViewerSettingsPrivate *d_ptr;

    ShadeMode shadeMode() const;
    void setShadeMode(ShadeMode mode);

    friend class Q3DSSurfaceViewerPrivate;
    friend class Q3DSRenderer;
    friend class Q3DSStudio3D;
};

QT_END_NAMESPACE

#endif // Q3DSVIEWERSETTINGS_H
