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
#ifndef QT3DS_LINUX_DYNAMIC_LIB_LOADER_H
#define QT3DS_LINUX_DYNAMIC_LIB_LOADER_H
#include <dlfcn.h>
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"

namespace qt3ds {
namespace render {
    using namespace qt3ds;
    using namespace qt3ds::render;

    class CLoadedDynamicLibrary
    {
        void *m_DLLHandle;
        CLoadedDynamicLibrary(void *hdl)
            : m_DLLHandle(hdl)
        {
        }
        CLoadedDynamicLibrary(const CLoadedDynamicLibrary &);
        CLoadedDynamicLibrary &operator=(const CLoadedDynamicLibrary &);

    public:
        ~CLoadedDynamicLibrary()
        {
            if (m_DLLHandle)
            {
#ifndef _INTEGRITYPLATFORM
                ::dlclose(m_DLLHandle);
#endif
            }
            m_DLLHandle = 0;
        }
        void *FindFunction(const char *name)
        {
#ifndef _INTEGRITYPLATFORM
            return ::dlsym(m_DLLHandle, name);
#else
            qWarning() << "CLoadedDynamicLibrary::FindFunction returns NULL!";
            return NULL;
#endif
        }
        static CLoadedDynamicLibrary *Create(const char *inFullDllPath, NVFoundationBase &fnd)
        {
#ifndef _INTEGRITYPLATFORM
            void *hdl = ::dlopen(inFullDllPath, RTLD_NOW);
            if (hdl == 0) {
                const char *error = ::dlerror();
                qCCritical(INVALID_OPERATION, "Failed to load dynamic library %s: %s",
                    inFullDllPath, error);
                return NULL;
            }
            return QT3DS_NEW(fnd.getAllocator(), CLoadedDynamicLibrary)(hdl);
#else
            qWarning() << "CLoadedDynamicLibrary::Create returns NULL!";
            return NULL;
#endif
        }
    };
}
}

#endif
