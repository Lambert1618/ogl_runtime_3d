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
#ifndef QT3DS_RENDER_PIXEL_GRAPHICS_TYPES_H
#define QT3DS_RENDER_PIXEL_GRAPHICS_TYPES_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec4.h"
#include "foundation/Qt3DSMat33.h"
#include "foundation/Qt3DSOption.h"

namespace qt3ds {
namespace render {

    // Vector graphics with no scaling are pixel aligned with 0,0 being the bottom,left of the
    // screen
    // with coordinates increasing to the right and up.  This is opposite most window systems but it
    // preserves the normal openGL assumptions about viewports and positive Y going up in general.
    struct SGTypes
    {
        enum Enum {
            UnknownVGType = 0,
            Layer,
            Rect,
            VertLine,
            HorzLine,
        };
    };

    struct SPGGraphObject
    {
        SGTypes::Enum m_Type;
        SPGGraphObject(SGTypes::Enum inType);
    };

    struct SPGRect : public SPGGraphObject
    {
        QT3DSF32 m_Left;
        QT3DSF32 m_Top;
        QT3DSF32 m_Right;
        QT3DSF32 m_Bottom;

        QT3DSVec4 m_FillColor;

        SPGRect();
    };

    struct SPGVertLine : public SPGGraphObject
    {
        QT3DSF32 m_X;
        QT3DSF32 m_Top;
        QT3DSF32 m_Bottom;
        QT3DSVec4 m_LineColor;
        void SetPosition(QT3DSF32 val) { m_X = val; }
        void SetStart(QT3DSF32 val) { m_Bottom = val; }
        void SetStop(QT3DSF32 val) { m_Top = val; }

        SPGVertLine();
    };

    struct SPGHorzLine : public SPGGraphObject
    {
        QT3DSF32 m_Y;
        QT3DSF32 m_Left;
        QT3DSF32 m_Right;
        QT3DSVec4 m_LineColor;
        void SetPosition(QT3DSF32 val) { m_Y = val; }
        void SetStart(QT3DSF32 val) { m_Left = val; }
        void SetStop(QT3DSF32 val) { m_Right = val; }

        SPGHorzLine();
    };
}
}

#endif