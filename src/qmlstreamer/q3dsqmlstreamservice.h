/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef Q3DS_QMLSTREAM_SERVICE_H
#define Q3DS_QMLSTREAM_SERVICE_H

#include <qglobal.h>
#include <q3dsqmlstreamer_global.h>

QT_BEGIN_NAMESPACE
class QOpenGLContext;
class QSurface;
class QSize;
QT_END_NAMESPACE

enum E_TEXTURE_FORMAT
{
    E_TEXTURE_RGBA8,
    E_TEXTURE_UNKNOWN
};

class Q3DS_QMLSTREAMERSHARED_EXPORT IQ3DSQmlStreamRenderer
{
public:
    virtual bool initialize(QT_PREPEND_NAMESPACE(QOpenGLContext) *context, QSurface *surface) = 0;

    virtual QSize getDesiredSize() = 0;
    virtual E_TEXTURE_FORMAT getDesiredFormat() = 0;
    virtual bool isUpdateRequested() = 0;

    virtual void render() = 0;

    virtual void uninitialize() = 0;
};

class Q3DS_QMLSTREAMERSHARED_EXPORT IQ3DSQmlStreamProducer
{
public:
    virtual IQ3DSQmlStreamRenderer *getRenderer() = 0;
};

class Q3DS_QMLSTREAMERSHARED_EXPORT IQ3DSQmlStreamService
{
public:
    virtual IQ3DSQmlStreamRenderer *getRenderer(const char *id) = 0;
    virtual bool registerProducer(IQ3DSQmlStreamProducer *producer, const char *id) = 0;
    virtual void unregisterProducer(IQ3DSQmlStreamProducer *producer) = 0;

    static IQ3DSQmlStreamService *getQmlStreamService();
};

#endif // Q3DS_QMLSTREAM_SERVICE_H


