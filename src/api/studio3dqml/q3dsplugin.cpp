/****************************************************************************
**
** Copyright (c) 2016 NVIDIA CORPORATION.
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

#include "q3dsplugin.h"

#include <QtQml/qqml.h>

#include <QtStudio3D/private/q3dsviewersettings_p.h>

#include "q3dsstudio3d_p.h"
#include "q3dspresentationitem_p.h"
#include "q3dsqmlstream.h"
#include "q3dsqmlsubpresentationsettings.h"
#include "q3dssceneelement.h"
#include "q3dsdatainput.h"
#include "qtstudio3dversion.h"

QT_BEGIN_NAMESPACE

void Q3DSPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("QtStudio3D.OpenGL"));

    // @uri QtStudio3D.OpenGL
    qmlRegisterType<Q3DSStudio3D>(uri, 2, 4, "Studio3D");
    qmlRegisterType<Q3DSViewerSettings>(uri, 2, 4, "ViewerSettings");
    qmlRegisterType<Q3DSPresentationItem>(uri, 2, 4, "Presentation");
    qmlRegisterType<Q3DSSceneElement>(uri, 2, 4, "SceneElement");
    qmlRegisterType<Q3DSElement>(uri, 2, 4, "Element");
    qmlRegisterType<Q3DSQmlStream>(uri, 2, 4, "QmlStream");
    qmlRegisterType<Q3DSSubPresentationSettings>(uri, 2, 4, "SubPresentationSettings");
    qmlRegisterType<Q3DSDataInput>(uri, 2, 4, "DataInput");
    qmlRegisterType<Q3DSDataOutput>(uri, 2, 4, "DataOutput");

    // 2.5
    qmlRegisterType<Q3DSStudio3D, 1>(uri, 2, 5, "Studio3D");
    qmlRegisterType<Q3DSViewerSettings, 1>(uri, 2, 5, "ViewerSettings");
    qmlRegisterType<Q3DSPresentationItem, 1>(uri, 2, 5, "Presentation");
    qmlRegisterRevision<Q3DSPresentation, 1>(uri, 2, 5);

    // 2.7
    qmlRegisterType<Q3DSPresentationItem, 2>(uri, 2, 7, "Presentation");
    qmlRegisterRevision<Q3DSPresentation, 2>(uri, 2, 7);

    // Automatically register the latest version
    qmlRegisterModule(uri, ((QTSTUDIO3D_VERSION >> 16) & 0xff), ((QTSTUDIO3D_VERSION >> 8) & 0xff));
}

QT_END_NAMESPACE
