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

#include "q3dsqmlstreamerserviceimpl.h"

Q3DSQmlStreamServiceImpl::Q3DSQmlStreamServiceImpl()
{

}

IQ3DSQmlStreamRenderer *Q3DSQmlStreamServiceImpl::getRenderer(const char *id)
{
    if (m_producers[QString(id)])
        return m_producers[QString(id)]->getRenderer();
    return nullptr;
}

bool Q3DSQmlStreamServiceImpl::registerProducer(IQ3DSQmlStreamProducer *producer, const char *id)
{
    m_producers[QString(id)] = producer;
    return true;
}

void Q3DSQmlStreamServiceImpl::unregisterProducer(IQ3DSQmlStreamProducer *producer)
{
    auto iter = m_producers.begin();
    for (; iter != m_producers.end(); ++iter) {
        if (*iter == producer) {
            m_producers.erase(iter);
            return;
        }
    }
}

static Q3DSQmlStreamServiceImpl *service = nullptr;
IQ3DSQmlStreamService *IQ3DSQmlStreamService::getQmlStreamService()
{
    if (!service)
        service = new Q3DSQmlStreamServiceImpl();
    return service;
}
