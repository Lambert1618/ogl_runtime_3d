/****************************************************************************
**
** Copyright (C) 2015 NVIDIA Corporation.
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

#include "render/Qt3DSRenderPathSpecification.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderContext.h"

namespace qt3ds {
namespace render {

    NVRenderPathSpecification::NVRenderPathSpecification(NVRenderContextImpl &context,
                                                         NVFoundationBase &fnd)
        : m_Context(context)
        , m_Foundation(fnd)
        , m_Backend(context.GetBackend())
        , mRefCount(0)
        , m_PathCommands(fnd.getAllocator(), "m_PathCommands")
        , m_PathCoords(fnd.getAllocator(), "m_PathCoords")
    {
    }

    NVRenderPathSpecification::~NVRenderPathSpecification() {}

    void NVRenderPathSpecification::Reset()
    {
        m_PathCommands.clear();
        m_PathCoords.clear();
    }

    void NVRenderPathSpecification::P(QT3DSVec2 inData)
    {
        m_PathCoords.push_back(inData.x);
        m_PathCoords.push_back(inData.y);
    }

    void NVRenderPathSpecification::MoveTo(QT3DSVec2 inPoint)
    {
        // we should actually query the backend for command converesion
        // but will we support any other pather render system than nv path?
        StaticAssert<NVRenderPathCommands::MoveTo == 0x02>::valid_expression();

        m_PathCommands.push_back(NVRenderPathCommands::MoveTo);
        P(inPoint);
    }

    void NVRenderPathSpecification::CubicCurveTo(QT3DSVec2 inC1, QT3DSVec2 inC2, QT3DSVec2 inDest)
    {
        // we should actually query the backend for command converesion
        // but will we support any other pather render system than nv path?
        StaticAssert<NVRenderPathCommands::CubicCurveTo == 0x0C>::valid_expression();

        m_PathCommands.push_back(NVRenderPathCommands::CubicCurveTo);
        P(inC1);
        P(inC2);
        P(inDest);
    }

    void NVRenderPathSpecification::ClosePath()
    {
        // we should actually query the backend for command converesion
        // but will we support any other pather render system than nv path?
        StaticAssert<NVRenderPathCommands::Close == 0x00>::valid_expression();

        m_PathCommands.push_back(NVRenderPathCommands::Close);
    }

    NVRenderPathSpecification *
    NVRenderPathSpecification::CreatePathSpecification(NVRenderContextImpl &context)
    {
        QT3DS_ASSERT(context.IsPathRenderingSupported());

        return QT3DS_NEW(context.GetFoundation().getAllocator(),
                      NVRenderPathSpecification)(context, context.GetFoundation());
    }
}
}
