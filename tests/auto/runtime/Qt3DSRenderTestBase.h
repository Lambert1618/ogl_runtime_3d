/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QT3DS_RENDER_TEST_BASE_H
#define QT3DS_RENDER_TEST_BASE_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSWindowSystem.h"
#include "Qt3DSTypes.h"
#include "Qt3DSTimer.h"
#include "Qt3DSRenderRuntimeBinding.h"
#include "Qt3DSRenderRuntimeBindingImpl.h"
#include "Qt3DSRenderContextCore.h"
#include "rendererimpl/Qt3DSRendererImpl.h"
#include "Qt3DSMetadata.h"

namespace qt3ds {
namespace render {
class IQt3DSRenderer;
class Qt3DSRendererImpl;
class IQt3DSRenderFactoryCore;
class IQt3DSRenderFactory;
}
}

namespace qt3ds {
namespace render {

    class IQt3DSRenderFactoryCore;
    class IQt3DSRenderFactory;
    class Qt3DSRenderer;

    typedef struct SUSER_CONTEXT_DATA
    {
        unsigned int winWidth;
        unsigned int winHeight;
    } userContextData;

    struct SNullTimeProvider : public Q3DStudio::ITimeProvider
    {
        virtual Q3DStudio::INT64 GetCurrentTimeMicroSeconds() { return 0; }
    };

    struct SNullWindowSystem : public Q3DStudio::IWindowSystem
    {
        virtual QSize GetWindowDimensions() { return QSize(); }

        virtual void SetWindowDimensions(const QSize &) {}
        // For platforms that support it, we get the egl info for render plugins
        // Feel free to return NULL.
        virtual Q3DStudio::SEGLInfo *GetEGLInfo() { return NULL; }

        // on some systems we allow our default render target to be a offscreen buffer
        // otherwise return 0;
        virtual int GetDefaultRenderTargetID() { return 0; }
        // returns the depth buffer bit count for the render window
        // does not really matter here
        virtual int GetDepthBitCount() { return 16; }
    };

    /// this is the base class for all tests
    class NVRenderTestBase
    {
    public:
        /// constructor
        NVRenderTestBase(){}
        /// destructor
        virtual ~NVRenderTestBase();

        /// Checks if this test is supported
        ///
        /// @return true if supported
        virtual bool isSupported(NVRenderContext *context) = 0;

        /// This runs the test
        ///
        /// @param context (in) Pointer to a NVRenderContext context
        /// @param pUserData (in) Pointer to pUserData
        ///
        /// @return false if failed
        virtual bool run(NVRenderContext *context, userContextData *pUserData) = 0;

        /// This cleans up state after the test if needed
        ///
        /// @param context (in) Pointer to a NVRenderContext context
        /// @param pUserData (in) Pointer to pUserData
        ///
        /// @return false if failed
        virtual void cleanup(NVRenderContext *context, userContextData *pUserData) = 0;

        /// This runs the performance test
        ///
        /// @param context (in) Pointer to a NVRenderContext context
        /// @param pUserData (in) Pointer to pUserData
        ///
        /// @return false if failed
        virtual bool runPerformance(NVRenderContext *context, userContextData *pContextData) = 0;

        /// This is a query to determine if we run on a ES context
        ///
        /// @param context (in) Pointer to a NVRenderContext context
        ///
        /// @return false if failed
        virtual const bool isGLESContext(NVRenderContext *context)
        {
            NVRenderContextType ctxType = context->GetRenderContextType();

            // Need minimum of GL3 or GLES3
            if (ctxType == NVRenderContextValues::GLES2 || ctxType == NVRenderContextValues::GLES3
                || ctxType == NVRenderContextValues::GLES3PLUS) {
                return true;
            }

            return false;
        }

        bool initializeQt3DSRenderer(QSurfaceFormat format);
        qt3ds::render::Qt3DSRendererImpl *qt3dsRenderer();
        Q3DStudio::IRuntimeMetaData *metadata();

    private:

        NVScopedRefCounted<qt3ds::render::IQt3DSRenderFactoryCore> m_coreFactory;
        NVScopedRefCounted<qt3ds::render::IQt3DSRenderFactory> m_factory;
        NVScopedRefCounted<qt3ds::render::IQt3DSRenderContext> m_rc;
        qt3ds::foundation::NVScopedReleasable<Q3DStudio::IRuntimeMetaData> m_metaData;
        qt3ds::render::Qt3DSRendererImpl *m_renderImpl;

        SNullTimeProvider m_timeProvider;
        SNullWindowSystem m_windowSystem;
    };
}
}

#endif // QT3DS_RENDER_TEST_BASE_H
