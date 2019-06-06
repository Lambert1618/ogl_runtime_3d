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

#include "q3dsviewersettings_p.h"
#include "Qt3DSViewerApp.h"
#include "q3dscommandqueue_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ViewerSettings
    \instantiates Q3DSViewerSettings
    \inqmlmodule Qt3DStudio
    \ingroup OpenGLRuntime
    \brief Qt 3D Studio presentation viewer settings.

    This type provides properties to define presentation independent viewer settings.

    \note ViewerSettings are only applicable when \l Studio3D is used in the
    default mode, showing the final, composed image from the Qt 3D Studio
    renderer.

    \sa Studio3D
*/

/*!
    \class Q3DSViewerSettings
    \inmodule OpenGLRuntime
    \since Qt 3D Studio 2.0

    \brief Qt 3D Studio presentation viewer settings.

    Q3DSViewerSettings provides properties to define presentation independent
    viewer settings.

    \note This class should not be instantiated directly when working with the
    C++ APIs. Q3DSSurfaceViewer and Q3DSWidget create a Q3DSViewerSettings
    instance implicitly. This can be queried via Q3DSSurfaceViewer::settings()
    or Q3DSWidget::settings().
 */

/*!
 * \internal
 */
Q3DSViewerSettings::Q3DSViewerSettings(QObject *parent)
    : QObject(parent)
    , d_ptr(new Q3DSViewerSettingsPrivate(this))
{
}

/*!
 * \internal
 */
Q3DSViewerSettings::~Q3DSViewerSettings()
{
}

/*!
    \enum Q3DSViewerSettings::ScaleMode

    This enumeration specifies the possible scaling modes.

    \value ScaleModeFit Scales the presentation to fit the output area.
    \value ScaleModeFill Scales the presentation to completely fill the output area.
    This is the default.
    \value ScaleModeCenter Centers the presentation in the output area without scaling it.
*/

/*!
    \qmlproperty QColor ViewerSettings::matteColor

    Specifies the matte color.
 */

/*!
    \property Q3DSViewerSettings::matteColor

    Specifies the matte color.
 */
QColor Q3DSViewerSettings::matteColor() const
{
    return d_ptr->m_matteColor;
}

void Q3DSViewerSettings::setMatteColor(const QColor &color)
{
    if (d_ptr->m_matteColor != color) {
        d_ptr->setMatteColor(color);
        Q_EMIT matteColorChanged(color);
    }
}

/*!
    \qmlproperty bool ViewerSettings::showRenderStats

    If this property is set to \c true, the simple profile
    view is displayed in-scene, on top of the 3D content.

    \note this feature can be disabled at build time, in which case this
    property has no effect.

    The default value is \c{false}.
*/

/*!
    \property Q3DSViewerSettings::showRenderStats

    When this property is \c{true}, the simple profile
    view is displayed in-scene, on top of the 3D content.

    \note This feature can be disabled at build time, in which case this
    property has no effect.

    Default value is \c{false}.
*/
bool Q3DSViewerSettings::isShowRenderStats() const
{
    return d_ptr->m_showRenderStats;
}

void Q3DSViewerSettings::setShowRenderStats(bool show)
{
    if (d_ptr->m_showRenderStats != show) {
        d_ptr->setShowRenderStats(show);
        Q_EMIT showRenderStatsChanged(show);
    }
}

/*!
    \qmlproperty ViewerSettings::shadeMode
 */

/*!
    \property Q3DSViewerSettings::shadeMode
 */
Q3DSViewerSettings::ShadeMode Q3DSViewerSettings::shadeMode() const
{
    return d_ptr->m_shadeMode;
}

void Q3DSViewerSettings::setShadeMode(Q3DSViewerSettings::ShadeMode mode)
{
    if (d_ptr->m_shadeMode != mode) {
        d_ptr->setShadeMode(mode);
        Q_EMIT shadeModeChanged(mode);
    }
}

/*!
    \qmlproperty enumeration ViewerSettings::scaleMode

    Specifies the scaling mode. The default value \c is ScaleModeFill where the
    size of the presentation on-screen follows and fills the size of the output
    area (the window, the screen, or the area occupied by the Studio3D
    element).

    During the design phase it can be valuable to see the presentation with
    some other scaling approach. For example, the Qt 3D Studio Viewer
    application uses ScaleModeCenter by default.

    \value ScaleModeFit Scales the presentation to fit the output area.
    \value ScaleModeFill Scales the presentation to completely fill the output area.
    \value ScaleModeCenter Centers the presentation in the output area without scaling it.

    The default value is \c{ScaleModeFill}.
*/
/*!
    \property Q3DSViewerSettings::scaleMode

    Specifies the scaling mode. The default value \c is ScaleModeFill where the
    size of the presentation on-screen follows and fills the size of the output
    area (the window, the screen, or the area occupied by the \l Studio3D
    element).

    During the design phase it can be valuable to see the presentation with
    some other scaling approach. For example, the Qt 3D Studio Viewer
    application uses ScaleModeCenter by default.

    \value ScaleModeFit Scales the presentation to fit the output area.
    \value ScaleModeFill Scales the presentation to completely fill the output area.
    \value ScaleModeCenter Centers the presentation in the output area without scaling it.

    The default value is \c{ScaleModeFill}.
 */
Q3DSViewerSettings::ScaleMode Q3DSViewerSettings::scaleMode() const
{
    return d_ptr->m_scaleMode;
}

void Q3DSViewerSettings::setScaleMode(Q3DSViewerSettings::ScaleMode mode)
{
    if (d_ptr->m_scaleMode != mode) {
        d_ptr->setScaleMode(mode);
        Q_EMIT scaleModeChanged(mode);
    }
}

/*!
    \qmmlmethod ViewerSettings::save
    Persistently saves the viewer \l{QSettings}{settings} using \a group, \a organization and
    \a application.
    \param group
    \param organization
    \param application
 */
/*!
 * \brief Q3DSViewerSettings::save  Persistently saves the viewer \l{QSettings}{settings}
    Persistently saves the viewer \l{QSettings}{settings} using \a group, \a organization and
    \a application.
    \param group
    \param organization
    \param application
 */
void Q3DSViewerSettings::save(const QString &group, const QString &organization,
                                      const QString &application)
{
    d_ptr->save(group, organization, application);
}

/*!
    \qmlmethod ViewerSettings::load
    Loads previously saved viewer \l{QSettings}{settings} using \a group, \a organization and
    \a application.
    \param group
    \param organization
    \param application
 */
/*!
 * \brief Q3DSViewerSettings::load Loads previously saved viewer \l{QSettings}{settings}
    Loads previously saved viewer \l{QSettings}{settings} using \a group, \a organization and
    \a application.
    \param group
    \param organization
    \param application
 */
void Q3DSViewerSettings::load(const QString &group, const QString &organization,
                                      const QString &application)
{
    d_ptr->load(group, organization, application);
}

Q3DSViewerSettingsPrivate::Q3DSViewerSettingsPrivate(Q3DSViewerSettings *q)
    : QObject(q)
    , q_ptr(q)
    , m_viewerApp(nullptr)
    , m_commandQueue(nullptr)
    , m_matteColor(Qt::black)
    , m_showRenderStats(false)
    , m_shadeMode(Q3DSViewerSettings::ShadeModeShaded)
    , m_scaleMode(Q3DSViewerSettings::ScaleModeCenter)
    , m_savedSettings(nullptr)
{
}

Q3DSViewerSettingsPrivate::~Q3DSViewerSettingsPrivate()
{
}

void Q3DSViewerSettingsPrivate::setViewerApp(Q3DSViewer::Q3DSViewerApp *app)
{
    m_viewerApp = app;
    if (m_viewerApp) {
        setMatteColor(m_matteColor);
        setShowRenderStats(m_showRenderStats);
        setShadeMode(m_shadeMode);
        setScaleMode(m_scaleMode);
    }
}

void Q3DSViewerSettingsPrivate::setCommandQueue(CommandQueue *queue)
{
    m_commandQueue = queue;
    if (m_commandQueue) {
        setMatteColor(m_matteColor);
        setShowRenderStats(m_showRenderStats);
        setShadeMode(m_shadeMode);
        setScaleMode(m_scaleMode);
    }
}

void Q3DSViewerSettingsPrivate::save(const QString &group, const QString &organization,
                                             const QString &application)
{
    initSettingsStore(group, organization, application);

    m_savedSettings->setValue(QStringLiteral("matteColor"), m_matteColor);
    m_savedSettings->setValue(QStringLiteral("showRenderStats"), m_showRenderStats);
    m_savedSettings->setValue(QStringLiteral("shadeMode"), m_shadeMode);
    m_savedSettings->setValue(QStringLiteral("scaleMode"), m_scaleMode);
}

void Q3DSViewerSettingsPrivate::load(const QString &group, const QString &organization,
                                             const QString &application)
{
    initSettingsStore(group, organization, application);

    q_ptr->setMatteColor(m_savedSettings->value(QStringLiteral("matteColor")).value<QColor>());
    q_ptr->setShowRenderStats(m_savedSettings->value(QStringLiteral("showRenderStats")).toBool());
    q_ptr->setShadeMode(Q3DSViewerSettings::ShadeMode(
                            m_savedSettings->value(QStringLiteral("shadeMode")).toInt()));
    q_ptr->setScaleMode(Q3DSViewerSettings::ScaleMode(
                            m_savedSettings->value(QStringLiteral("scaleMode")).toInt()));
}

void Q3DSViewerSettingsPrivate::setMatteColor(const QColor &color)
{
    m_matteColor = color;
    if (m_viewerApp) {
        m_viewerApp->setMatteColor(color);
    } else if (m_commandQueue) {
        m_commandQueue->m_matteColor = color;
        m_commandQueue->m_matteColorChanged = true;
    }
}

void Q3DSViewerSettingsPrivate::setShowRenderStats(bool show)
{
    m_showRenderStats = show;
    if (m_viewerApp) {
        m_viewerApp->setShowOnScreenStats(show);
    } else if (m_commandQueue) {
        m_commandQueue->m_showRenderStats = show;
        m_commandQueue->m_showRenderStatsChanged = true;
    }
}

void Q3DSViewerSettingsPrivate::setShadeMode(Q3DSViewerSettings::ShadeMode mode)
{
    m_shadeMode = mode;
    if (m_viewerApp) {
        if (mode == Q3DSViewerSettings::ShadeModeShaded)
            m_viewerApp->SetShadeMode(Q3DSViewer::ViewerShadeModes::Shaded);
        else
            m_viewerApp->SetShadeMode(Q3DSViewer::ViewerShadeModes::ShadedWireframe);
    } else if (m_commandQueue) {
        m_commandQueue->m_shadeMode = mode;
        m_commandQueue->m_shadeModeChanged = true;
    }
}

void Q3DSViewerSettingsPrivate::setScaleMode(Q3DSViewerSettings::ScaleMode mode)
{
    m_scaleMode = mode;
    if (m_viewerApp) {
        if (mode == Q3DSViewerSettings::ScaleModeFit)
            m_viewerApp->SetScaleMode(Q3DSViewer::ViewerScaleModes::ScaleToFit);
        else if (mode == Q3DSViewerSettings::ScaleModeFill)
            m_viewerApp->SetScaleMode(Q3DSViewer::ViewerScaleModes::ScaleToFill);
        else
            m_viewerApp->SetScaleMode(Q3DSViewer::ViewerScaleModes::ExactSize);
    } else if (m_commandQueue) {
        m_commandQueue->m_scaleMode = mode;
        m_commandQueue->m_scaleModeChanged = true;
    }
}

void Q3DSViewerSettingsPrivate::initSettingsStore(const QString &group, const QString &organization,
                                                  const QString &application)
{
    if (!m_savedSettings) {
        QString org = organization.isEmpty() ? QCoreApplication::instance()->organizationName()
                                             : organization;
        QString app = application.isEmpty() ? QCoreApplication::instance()->applicationName()
                                            : application;

        m_savedSettings = new QSettings(org, app, this);
        m_savedSettings->beginGroup(group);
    }
}

QT_END_NAMESPACE
