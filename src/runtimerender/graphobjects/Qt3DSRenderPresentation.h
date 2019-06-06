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
#pragma once
#ifndef QT3DS_RENDER_PRESENTATION_H
#define QT3DS_RENDER_PRESENTATION_H

#include "foundation/StringTable.h"
#include "Qt3DSRenderGraphObject.h"
#include "Qt3DSRenderScene.h"
#include "foundation/Qt3DSVec2.h"

namespace qt3ds {
namespace render {

    struct RenderRotationValues
    {
        enum Enum {
            NoRotation = 0,
            Clockwise90,
            Clockwise180,
            Clockwise270,
        };
    };

    struct SPresentation : public SGraphObject
    {
        QT3DSVec2 m_PresentationDimensions;
        RenderRotationValues::Enum m_PresentationRotation;
        bool m_preferKTX;
        SScene *m_Scene;

        CRegisteredString m_PresentationDirectory;

        SPresentation()
            : SGraphObject(GraphObjectTypes::Presentation)
            , m_PresentationDimensions(800, 400)
            , m_PresentationRotation(RenderRotationValues::NoRotation)
            , m_preferKTX(false)
            , m_Scene(NULL)
        {
        }

        SPresentation(QT3DSF32 w, QT3DSF32 h, bool preferKTX, CRegisteredString presDir)
            : SGraphObject(GraphObjectTypes::Presentation)
            , m_PresentationDimensions(w, h)
            , m_PresentationRotation(RenderRotationValues::NoRotation)
            , m_preferKTX(preferKTX)
            , m_Scene(NULL)
            , m_PresentationDirectory(presDir)
        {
        }
        // Generic method used during serialization
        // to remap string and object pointers
        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SGraphObject::Remap(inRemapper);
            inRemapper.Remap(m_Scene);
            inRemapper.Remap(m_PresentationDirectory);
        }

        void Render(IQt3DSRenderContext &inContext);
    };
}
}

#endif
