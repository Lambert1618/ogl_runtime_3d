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

#include "foundation/windows/Qt3DSWindowsInclude.h"
#include "foundation/Qt3DSSemaphore.h"
#include "foundation/Qt3DSAllocator.h"

namespace qt3ds {
namespace foundation {

    class SemaphoreImpl
    {
    public:
        SemaphoreImpl(NVAllocatorCallback &alloc)
            : mAllocator(alloc)
        {
        }
        NVAllocatorCallback &mAllocator;
        HANDLE handle;
    };

    Semaphore::Semaphore(NVAllocatorCallback &alloc, QT3DSU32 initialCount, QT3DSU32 maxCount)
    {
        mImpl = QT3DS_NEW(alloc, SemaphoreImpl)(alloc);
        mImpl->handle = CreateSemaphore(0, initialCount, maxCount, (LPCTSTR)NULL);
    }

    Semaphore::~Semaphore()
    {
        CloseHandle(mImpl->handle);
        QT3DS_FREE(mImpl->mAllocator, mImpl);
    }

    bool Semaphore::wait(QT3DSU32 milliseconds)
    {
        if (milliseconds == -1)
            milliseconds = INFINITE;

        return WaitForSingleObject(mImpl->handle, milliseconds) == WAIT_OBJECT_0 ? true : false;
    }

    void Semaphore::post() { ReleaseSemaphore(mImpl->handle, 1, (LPLONG)NULL); }

} // namespace foundation
} // namespace qt3ds
