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

#include "foundation/Qt3DSMat44.h"
#include "render/Qt3DSRenderContext.h"
#include "foundation/Utils.h"
#include "EASTL/set.h"
#include "EASTL/utility.h"
#include "render/Qt3DSRenderShaderProgram.h"

using namespace qt3ds;
using namespace qt3ds::render;
using namespace eastl;

namespace qt3ds {
namespace render {

    NVRenderContext &NVRenderContext::CreateGL(NVFoundationBase &foundation,
                                               IStringTable &inStringTable,
                                               const QSurfaceFormat &format)
    {
        NVRenderContext *retval = NULL;

        QT3DS_ASSERT(format.majorVersion() >= 2);

        // create backend
        NVScopedRefCounted<IStringTable> theStringTable(inStringTable);
        NVScopedRefCounted<NVRenderBackend> theBackend;
        bool isES = format.renderableType() == QSurfaceFormat::OpenGLES;
        if (isES && (format.majorVersion() == 2
                     || (format.majorVersion() == 3 && format.minorVersion() == 0))) {
            theBackend = QT3DS_NEW(foundation.getAllocator(), NVRenderBackendGLES2Impl)(foundation,
                                                                                   *theStringTable,
                                                                                   format);
        } else if (format.majorVersion() == 3 && format.minorVersion() >= 1 && !isES) {
            theBackend = QT3DS_NEW(foundation.getAllocator(), NVRenderBackendGL3Impl)(foundation,
                                                                                   *theStringTable,
                                                                                   format);
        } else if (format.majorVersion() == 4
            || (isES && format.majorVersion() == 3 && format.minorVersion() >= 1)) {
#ifdef Q_OS_MACOS
            // TODO: macOS crashes with glTextStorage2DMultisample, so fall back to OpenGL3
            // for now (QT3DS-590)
            theBackend = QT3DS_NEW(foundation.getAllocator(), NVRenderBackendGL3Impl)(foundation,
                                                                                   *theStringTable,
                                                                                   format);
#else
            theBackend = QT3DS_NEW(foundation.getAllocator(), NVRenderBackendGL4Impl)(foundation,
                                                                                   *theStringTable,
                                                                                   format);
#endif
        } else {
            QT3DS_ASSERT(false);
            qCCritical(INTERNAL_ERROR) << "Can't find a suitable OpenGL version for" << format;
        }


        retval = QT3DS_NEW(foundation.getAllocator(), NVRenderContextImpl)(foundation, *theBackend,
                                                                        *theStringTable);

        return *retval;
    }
}
}
