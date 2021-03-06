/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#include "Qt3DSRenderPresentation.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSRenderContextCore.h"

using namespace qt3ds::render;

void SPresentation::Render(IQt3DSRenderContext &inContext)
{
    if (m_Scene) {
        NVRenderRect theViewportSize(inContext.GetRenderContext().GetViewport());
        m_Scene->Render(QT3DSVec2((QT3DSF32)theViewportSize.m_Width, (QT3DSF32)theViewportSize.m_Height),
                        inContext);
    }
}
