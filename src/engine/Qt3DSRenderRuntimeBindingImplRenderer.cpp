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
#include "EnginePrefix.h"
#include "Qt3DSRenderRuntimeBindingImpl.h"
#include "Qt3DSRuntimeView.h"
#include "Qt3DSWindowSystem.h"
#include "Qt3DSRenderLoadedTexture.h"

struct SRenderer;
struct SRendererRSM : public Q3DStudio::ITegraRenderStateManager
{
    SRenderer &m_Renderer;

public:
    SRendererRSM(SRenderer &renderer)
        : m_Renderer(renderer)
    {
    }

    void SetViewport(Q3DStudio::INT32 inX, Q3DStudio::INT32 inY, Q3DStudio::INT32 inWidth,
                             Q3DStudio::INT32 inHeight) override;
    void PushState() override;
    void PopState() override;
    void SetScissorTestEnabled(bool inValue) override;
    void SaveAllState() override;
    void RestoreAllState() override;
};

struct SRenderer : public Q3DStudio::ITegraApplicationRenderEngine
{
    NVScopedRefCounted<SBindingCore> m_BindingCore;
    NVScopedRefCounted<IQt3DSRenderContext> m_Context;
    NVRenderRect m_Viewport;
    nvvector<NVRenderRect> m_StateStack;
    QSize m_PresentationDimensions;
    SRendererRSM m_RSM;
    Q3DStudio::IWindowSystem &m_WindowSystem;

    SRenderer(SBindingCore &inCore, Q3DStudio::IWindowSystem &inWindowSystem)
        : m_BindingCore(inCore)
        , m_Context(*inCore.m_Context)
        , m_StateStack(inCore.m_Context->GetAllocator(), "SRenderer::m_StateStack")
        , m_RSM(*this)
        , m_WindowSystem(inWindowSystem)
    {
        m_Context->SetSceneColor(QT3DSVec4(0, 0, 0, 0.0f));
        if (m_BindingCore->m_RenderContext) {
            m_BindingCore->m_RenderContext->SetDefaultRenderTarget(
                m_WindowSystem.GetDefaultRenderTargetID());
            m_BindingCore->m_RenderContext->SetDefaultDepthBufferBitCount(
                m_WindowSystem.GetDepthBitCount());
        }
    }

    void ensureRenderTarget() override
    {
        m_BindingCore->m_RenderContext->SetDefaultRenderTarget(
            m_WindowSystem.GetDefaultRenderTargetID());
    }

    void CheckResize(bool, Q3DStudio::IPresentation & /*inPresentation*/) override
    {
        QSize theWindowDims(m_WindowSystem.GetWindowDimensions());
        m_BindingCore->m_WindowDimensions = theWindowDims;
        m_BindingCore->m_Context->SetWindowDimensions(m_BindingCore->m_WindowDimensions);
    }
    QByteArray exportShaderCache(bool binaryShaders) override
    {
        return m_BindingCore->m_Context->GetShaderCache().exportShaderCache(binaryShaders);
    }

    void AbandonLoadingImages(Q3DStudio::IScene & /*inScene*/) override {}

    Q3DStudio::BOOL IsPickValid(Q3DStudio::FLOAT &outX, Q3DStudio::FLOAT &outY,
                                        const Q3DStudio::IPresentation & /*inPresentation*/) const override
    {
        Q3DStudio::FLOAT theX = outX;
        Q3DStudio::FLOAT theY = outY;
        theX = theX / static_cast<Q3DStudio::FLOAT>(m_BindingCore->m_WindowDimensions.width());
        theY = theY / static_cast<Q3DStudio::FLOAT>(m_BindingCore->m_WindowDimensions.height());
        Q3DStudio::BOOL theValid = false;

        if ((theX >= 0.0f) && (theY >= 0.0f) && (theX <= 1.0f) && (theY <= 1.0f))
            theValid = true;
        return theValid;
    }

    void SetScaleMode(Q3DStudio::TegraRenderScaleModes::Enum inScale) override
    {
        if (m_BindingCore && m_BindingCore->m_Context)
            m_BindingCore->m_Context->SetScaleMode(static_cast<ScaleModes::Enum>(inScale));
    }
    Q3DStudio::TegraRenderScaleModes::Enum GetScaleMode() const override
    {
        if (m_BindingCore && m_BindingCore->m_Context)
            return static_cast<Q3DStudio::TegraRenderScaleModes::Enum>(
                const_cast<SRenderer &>(*this).m_BindingCore->m_Context->GetScaleMode());

        QT3DS_ASSERT(false);
        return Q3DStudio::TegraRenderScaleModes::ExactSize;
    }

    void SetStereoMode(Q3DStudio::TegraRenderStereoModes::Enum inMode) override
    {
        if (m_BindingCore && m_BindingCore->m_Context)
            m_BindingCore->m_Context->SetStereoMode(static_cast<StereoModes::Enum>(inMode));
    }
    Q3DStudio::TegraRenderStereoModes::Enum GetStereoMode() const override
    {
        if (m_BindingCore && m_BindingCore->m_Context) {
            return static_cast<Q3DStudio::TegraRenderStereoModes::Enum>(
                const_cast<SRenderer &>(*this).m_BindingCore->m_Context->GetStereoMode());
        }
        QT3DS_ASSERT(false);
        return Q3DStudio::TegraRenderStereoModes::Mono;
    }

    void SetStereoEyeSeparation(double separation) override
    {
        if (m_BindingCore && m_BindingCore->m_Context)
            m_BindingCore->m_Context->SetStereoEyeSeparation(separation);
    }
    double GetStereoEyeSeparation() const override
    {
        if (m_BindingCore && m_BindingCore->m_Context) {
            return const_cast<SRenderer &>(*this).m_BindingCore->
                    m_Context->GetStereoEyeSeparation();
        }
        QT3DS_ASSERT(false);
        return 0;
    }

    void SetStereoProgressiveEnabled(bool enabled) override
    {
        if (m_BindingCore && m_BindingCore->m_Context)
            m_BindingCore->m_Context->SetStereoProgressiveEnabled(enabled);
    }
    bool GetStereoProgressiveEnabled() const override
    {
        if (m_BindingCore && m_BindingCore->m_Context) {
            return const_cast<SRenderer &>(*this).m_BindingCore->
                    m_Context->GetStereoProgressiveEnabled();
        }
        QT3DS_ASSERT(false);
        return false;
    }

    void SetShadeMode(Q3DStudio::TegraRenderShadeModes::Enum inShade) override
    {
        if (m_BindingCore && m_BindingCore->m_Context) {
            m_BindingCore->m_Context->SetWireframeMode(
                (inShade == Q3DStudio::TegraRenderShadeModes::Shaded) ? false : true);
        }
    }

    void EnableRenderRotation(bool inEnable) override
    {
        m_BindingCore->m_RenderRotationsEnabled = inEnable;
    }

    Q3DStudio::ITegraRenderStateManager &GetTegraRenderStateManager() override { return m_RSM; }
    qt3ds::render::NVRenderContext &GetRenderContext() override
    {
        return *m_BindingCore->m_RenderContext;
    }

    void Release() override
    {
        // Ensure the core doesn't die until after we do.
        NVScopedRefCounted<SBindingCore> theContext(m_BindingCore);
        NVDelete(m_Context->GetAllocator(), this);
    }

    void SetViewport(Q3DStudio::INT32 inX, Q3DStudio::INT32 inY, Q3DStudio::INT32 inWidth,
                             Q3DStudio::INT32 inHeight) override
    {
        m_Viewport = NVRenderRect(inX, inY, inWidth, inHeight);
        m_BindingCore->m_RenderContext->SetViewport(m_Viewport);
    }
    void SetApplicationViewport(const qt3ds::render::NVRenderRect &inViewport) override
    {
        m_BindingCore->m_Context->SetViewport(inViewport);
    }

    void SetMatteColor(Option<QT3DSVec4> inColor) override { m_Context->SetMatteColor(inColor); }
    void setMatteEnabled(bool enabled) override { m_Context->setMatteEnabled(enabled); };
    virtual void PushState() { m_StateStack.push_back(m_Viewport); }
    virtual void PopState()
    {
        m_Viewport = m_StateStack.back();
        m_StateStack.pop_back();
        SetViewport(m_Viewport.m_X, m_Viewport.m_Y, m_Viewport.m_Width, m_Viewport.m_Height);
    }

    void RenderText2D(Q3DStudio::FLOAT x, Q3DStudio::FLOAT y,
                              qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor, const char *text) override
    {
        m_Context->RenderText2D(x, y, inColor, text);
    }

    void RenderGpuProfilerStats(Q3DStudio::FLOAT x, Q3DStudio::FLOAT y,
                                        qt3ds::foundation::Option<qt3ds::QT3DSVec3> inColor) override
    {
        m_Context->RenderGpuProfilerStats(x, y, inColor);
    }
};

void SRendererRSM::SetViewport(Q3DStudio::INT32 inX, Q3DStudio::INT32 inY, Q3DStudio::INT32 inWidth,
                               Q3DStudio::INT32 inHeight)
{
    m_Renderer.SetViewport(inX, inY, inWidth, inHeight);
}
void SRendererRSM::PushState()
{
    m_Renderer.PushState();
}
void SRendererRSM::PopState()
{
    m_Renderer.PopState();
}
void SRendererRSM::SetScissorTestEnabled(bool inValue)
{
    m_Renderer.m_BindingCore->m_RenderContext->SetScissorTestEnabled(inValue);
}
void SRendererRSM::SaveAllState()
{
    m_Renderer.m_BindingCore->m_RenderContext->PushPropertySet();
}
void SRendererRSM::RestoreAllState()
{
    m_Renderer.m_BindingCore->m_RenderContext->PopPropertySet(true);
}

Q3DStudio::ITegraApplicationRenderEngine &SBindingCore::CreateRenderer()
{
    SRenderer *retval = NULL;
    if (m_Context)
        retval = QT3DS_NEW(m_Context->GetAllocator(), SRenderer)(*this, this->m_WindowSystem);

    return *retval;
}
