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

#pragma once

#include "Qt3DSTimer.h"
#include "foundation/Qt3DSRefCounted.h"

namespace qt3ds {
namespace render {
    class IInputStreamFactory;
    class IQt3DSRenderContext;
    class IQt3DSRenderContextCore;
}
}

namespace qt3ds {
namespace runtime {
    class IApplication;
}
}

namespace qt3ds {
namespace evt {
    class IEventSystem;
}
}

namespace qt3ds {
class NVFoundationBase;
namespace foundation {
    class IStringTable;
    class IPerfTimer;
}
}

namespace Q3DStudio {

class IScene;
class CRenderEngine;
class ISceneManager;
class IRender;
class CInputEngine;
class IScriptBridge;
class CPresentation;
class IPresentation;
class ITimeProvider;
class ISceneBinaryLoader;

// All of the interfaces available without opengl initialized.
class IRuntimeFactoryCore : public qt3ds::foundation::NVRefCounted
{
public:
    virtual ISceneBinaryLoader &GetSceneLoader() = 0;
    virtual IScriptBridge &GetScriptEngineQml() = 0;
    virtual qt3ds::render::IQt3DSRenderContextCore &GetRenderContextCore() = 0;
    virtual qt3ds::render::IInputStreamFactory &GetInputStreamFactory() = 0;
    virtual qt3ds::evt::IEventSystem &GetEventSystem() = 0;
    virtual ITimeProvider &GetTimeProvider() = 0;
    virtual qt3ds::NVFoundationBase &GetFoundation() = 0;
    virtual qt3ds::foundation::IPerfTimer &GetPerfTimer() = 0;
    virtual qt3ds::foundation::IStringTable &GetStringTable() = 0;
    virtual void AddSearchPath(const char8_t *inFile) = 0;
    virtual void SetDllDir(const char *inDir) = 0;
    virtual qt3ds::runtime::IApplication *GetApplicationCore() = 0;
    virtual void SetApplicationCore(qt3ds::runtime::IApplication *app) = 0;
};

class IRuntimeFactory : public IRuntimeFactoryCore
{
protected:
    virtual ~IRuntimeFactory() {}

public:
    virtual ISceneManager &GetSceneManager() = 0;
    virtual qt3ds::render::IQt3DSRenderContext &GetQt3DSRenderContext() = 0;
    virtual qt3ds::runtime::IApplication *GetApplication() = 0;
    virtual void SetApplication(qt3ds::runtime::IApplication *app) = 0;
};

} // namespace Q3DStudio
