/****************************************************************************
**
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

#include "Qt3DSRenderTestBase.h"

#include "Qt3DSDMPrefix.h"
#include "EASTL/allocator.h"

namespace eastl
{
void AssertionFailure(const char* pExpression)
{
    Q_UNUSED(pExpression);
}
EmptyString gEmptyString;
void* gpEmptyBucketArray[2] = { nullptr, (void*)uintptr_t(~0) };
}

namespace qt3ds {

void Qt3DSAssert(const char *exp, const char *file, int line, bool *ignore)
{
    Q_UNUSED(ignore)
    qFatal("Assertion thrown %s(%d): %s", file, line, exp);
}

namespace render {
SEndlType Endl;
}
}

void *operator new[](size_t size, const char *, int, unsigned, const char *, int)
{
    return malloc(size);
}

void *operator new[](size_t size, size_t, size_t, const char *, int, unsigned, const char *, int)
{
    return malloc(size);
}

using namespace qt3ds::render;

namespace qt3ds {
namespace render {

NVRenderTestBase::~NVRenderTestBase()
{
    delete m_renderImpl;
}

bool NVRenderTestBase::initializeQt3DSRenderer(QSurfaceFormat format)
{
    m_coreFactory = qt3ds::render::IQt3DSRenderFactoryCore::CreateRenderFactoryCore("", m_windowSystem,
                                                                                m_timeProvider);
    m_factory = m_coreFactory->CreateRenderFactory(format, false);
    m_rc = m_factory->GetQt3DSRenderContext();
    m_renderImpl = new qt3ds::render::Qt3DSRendererImpl(*m_rc);

    return true;
}

Qt3DSRendererImpl *NVRenderTestBase::qt3dsRenderer()
{
    return m_renderImpl;
}

Q3DStudio::IRuntimeMetaData *NVRenderTestBase::metadata()
{
    if (m_metaData.mPtr == NULL)
        m_metaData = &Q3DStudio::IRuntimeMetaData::Create(m_coreFactory->GetInputStreamFactory());
    return m_metaData.mPtr;
}

}
}
