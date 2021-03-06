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
#include "Qt3DSRenderEffect.h"
#include "Qt3DSRenderEffectSystem.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "StringTools.h"
#include "foundation/FileTools.h"

using namespace qt3ds::render;

void SEffect::Initialize()
{
    m_Layer = NULL;
    m_NextEffect = NULL;
    m_Context = NULL;
    m_imageMaps = nullptr;
    m_error = CRegisteredString();
}

void SEffect::SetActive(bool inActive, IEffectSystem &inManager)
{
    if (m_Flags.IsActive() != inActive) {
        m_Flags.SetActive(inActive);
        if (m_Context)
            inManager.ResetEffectFrameData(*m_Context);
        m_Flags.SetDirty(true);
        if (inActive)
            inManager.SetEffectRequiresCompilation(m_ClassName, true);
    }
}

void SEffect::Reset(IEffectSystem &inSystem)
{
    if (m_Context)
        inSystem.ResetEffectFrameData(*m_Context);
    m_Flags.SetDirty(true);
}

CRegisteredString SEffect::GetError() const
{
    return m_error;
}

void SEffect::SetError(const CRegisteredString &error)
{
    m_error = error;
}
