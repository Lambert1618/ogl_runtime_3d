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

#include "q3dsqmlrender.h"
#include "q3dsqmlstreamservice.h"
#include "render/Qt3DSRenderContext.h"

#include <QSize>
#include <QOpenGLContext>

Q3DSQmlRender::Q3DSQmlRender(IQt3DSRenderContext &inRenderContext, const char *asset)
    : m_RenderContext(inRenderContext)
    , m_qmlStreamRenderer(nullptr)
    , m_offscreenRenderType(inRenderContext.GetStringTable().RegisterStr(GetRendererName()))
    , m_assetString(inRenderContext.GetStringTable().RegisterStr(asset))
    , m_callback(nullptr)
    , mRefCount(0)
{

}

Q3DSQmlRender::~Q3DSQmlRender()
{
    m_qmlStreamRenderer =
            IQ3DSQmlStreamService::getQmlStreamService()->getRenderer(m_assetString.c_str());
    if (m_qmlStreamRenderer)
        m_qmlStreamRenderer->uninitialize();
}

CRegisteredString Q3DSQmlRender::GetOffscreenRendererType()
{
    return m_offscreenRenderType;
}

NVRenderTextureFormats::Enum convertTextureFormat(E_TEXTURE_FORMAT fmt)
{
    NVRenderTextureFormats::Enum ret;

    switch (fmt) {
    case E_TEXTURE_RGBA8:
        ret = NVRenderTextureFormats::RGBA8;
        break;
    default:
        ret = NVRenderTextureFormats::Unknown;
        break;
    }

    return ret;
}

SOffscreenRendererEnvironment Q3DSQmlRender::GetDesiredEnvironment(QT3DSVec2 inPresScale)
{
    QSize size(0, 0);
    E_TEXTURE_FORMAT format = E_TEXTURE_UNKNOWN;

    if (!m_qmlStreamRenderer)
        initializeRenderer();

    if (m_qmlStreamRenderer) {
        size = m_qmlStreamRenderer->getDesiredSize();
        format = m_qmlStreamRenderer->getDesiredFormat();
    }
    return SOffscreenRendererEnvironment(
        (QT3DSU32)(size.width() * inPresScale.x), (QT3DSU32)(size.height() * inPresScale.y),
        convertTextureFormat(format), OffscreenRendererDepthValues::Depth24, false,
        AAModeValues::NoAA);
}

SOffscreenRenderFlags Q3DSQmlRender::NeedsRender(const SOffscreenRendererEnvironment &inEnvironment,
                                                 QT3DSVec2 inPresentationScaleFactor,
                                                 const SRenderInstanceId instanceId)
{
    Q_UNUSED(inEnvironment);
    Q_UNUSED(inPresentationScaleFactor);
    Q_UNUSED(instanceId);
    bool render = false;
    if (!m_qmlStreamRenderer)
        initializeRenderer();
    if (m_qmlStreamRenderer)
        render = m_qmlStreamRenderer->isUpdateRequested();
    return SOffscreenRenderFlags(true, render);
}

void Q3DSQmlRender::Render(const SOffscreenRendererEnvironment &inEnvironment,
                           NVRenderContext &inRenderContext, QT3DSVec2 inPresentationScaleFactor,
                           SScene::RenderClearCommand inColorBufferNeedsClear,
                           const SRenderInstanceId instanceId)
{
    Q_UNUSED(inEnvironment)
    Q_UNUSED(inPresentationScaleFactor)
    Q_UNUSED(inColorBufferNeedsClear)
    Q_UNUSED(instanceId)
    if (m_qmlStreamRenderer) {
        inRenderContext.PushPropertySet();

        m_qmlStreamRenderer->render();

        inRenderContext.PopPropertySet(true);

        if (m_callback)
            m_callback->onOffscreenRendererFrame(QString(m_assetString.c_str()));
    }
}

void Q3DSQmlRender::initializeRenderer()
{
    m_qmlStreamRenderer
            = IQ3DSQmlStreamService::getQmlStreamService()->getRenderer(m_assetString.c_str());
    if (m_qmlStreamRenderer) {
        if (!m_qmlStreamRenderer->initialize(
                    QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext(),
                    QT_PREPEND_NAMESPACE(QOpenGLContext)::currentContext()->surface())) {
            m_qmlStreamRenderer = nullptr;
        } else if (m_callback) {
            m_callback->onOffscreenRendererInitialized(QString(m_assetString.c_str()));
        }
    }
}
