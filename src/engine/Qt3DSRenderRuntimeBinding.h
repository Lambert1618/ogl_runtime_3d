/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

#ifndef QT3DS_RENDER_RUNTIME_BINDING_H
#define QT3DS_RENDER_RUNTIME_BINDING_H

#include "Qt3DSRuntimeFactory.h"
#include "EABase/eabase.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderContext.h"

#include <QSurfaceFormat>

namespace Q3DStudio {
class ITegraApplicationRenderEngine;
class ITimeProvider;
class IWindowSystem;
}

namespace qt3ds {
namespace render {

    class ITextRenderer;
    class IRuntimeFactoryRenderFactory
    {
    protected:
        virtual ~IRuntimeFactoryRenderFactory() {}
    public:
        virtual qt3ds::render::NVRenderContext *
        CreateRenderContext(qt3ds::NVFoundationBase &foundat, qt3ds::foundation::IStringTable &strt) = 0;
    };

    class IQt3DSRenderFactory;

    class QT3DS_AUTOTEST_EXPORT IQt3DSRenderFactoryCore : public Q3DStudio::IRuntimeFactoryCore
    {
    public:
        virtual IQt3DSRenderFactory &
        CreateRenderFactory(const QSurfaceFormat &format, bool delayedLoading) = 0;

        static IQt3DSRenderFactoryCore &
        CreateRenderFactoryCore(const char8_t *inApplicationDirectory,
                                Q3DStudio::IWindowSystem &inWindowSystem,
                                Q3DStudio::ITimeProvider &inTimeProvider);
    };

    class IQt3DSRenderFactory : public Q3DStudio::IRuntimeFactory
    {
    public:
        virtual Q3DStudio::ITegraApplicationRenderEngine &CreateRenderEngine() = 0;
    };
}
}

#endif
