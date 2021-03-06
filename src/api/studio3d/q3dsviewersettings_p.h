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

#ifndef Q3DSVIEWERSETTINGS_P_H
#define Q3DSVIEWERSETTINGS_P_H

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

#include "q3dsviewersettings.h"
#include "Qt3DSViewerApp.h"

QT_BEGIN_NAMESPACE

class QSettings;
class CommandQueue;

class Q_STUDIO3D_EXPORT Q3DSViewerSettingsPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Q3DSViewerSettings)
public:
    explicit Q3DSViewerSettingsPrivate(Q3DSViewerSettings *parent);
    ~Q3DSViewerSettingsPrivate();

    void setViewerApp(Q3DSViewer::Q3DSViewerApp *app);
    void setCommandQueue(CommandQueue *queue);
    void save(const QString &group, const QString &organization, const QString &application);
    void load(const QString &group, const QString &organization, const QString &application);

    void setMatteEnabled(bool enabled);
    void setMatteColor(const QColor &color);
    void setShowRenderStats(bool show);
    void setShadeMode(Q3DSViewerSettings::ShadeMode mode);
    void setScaleMode(Q3DSViewerSettings::ScaleMode mode);
    void setStereoMode(Q3DSViewerSettings::StereoMode mode);
    void setStereoEyeSeparation(double separation);
    void setStereoProgressiveEnabled(bool enabled);
    void setSkipFramesInterval(int interval);

public:
    Q3DSViewerSettings *q_ptr;

private:
    void initSettingsStore(const QString &group, const QString &organization,
                           const QString &application);

    Q3DSViewer::Q3DSViewerApp *m_viewerApp; // Not owned
    CommandQueue *m_commandQueue; // Not owned
    QColor m_matteColor;
    bool m_showRenderStats;
    bool m_matteEnabled;
    Q3DSViewerSettings::ShadeMode m_shadeMode;
    Q3DSViewerSettings::ScaleMode m_scaleMode;
    Q3DSViewerSettings::StereoMode m_stereoMode;
    double m_stereoEyeSeparation;
    bool m_stereoProgressiveEnabled;
    int m_skipFramesInterval;
    QSettings *m_savedSettings;
};

QT_END_NAMESPACE

#endif // Q3DSVIEWERSETTINGS_P_H
