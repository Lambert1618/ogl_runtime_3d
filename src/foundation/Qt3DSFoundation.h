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

#ifndef QT3DS_FOUNDATION_QT3DS_FOUNDATION_H
#define QT3DS_FOUNDATION_QT3DS_FOUNDATION_H

/** \addtogroup foundation
  @{
*/

#include <stdarg.h>
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSVersionNumber.h"
#include "foundation/Qt3DSLogging.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif

class NVAllocatorCallback;
class NVProfilingZone;
class NVBroadcastingAllocator;

class NVFoundationBase
{
public:
    /**
    retrieves the current allocator.
    */
    virtual NVBroadcastingAllocator &getAllocator() const = 0;
};

namespace foundation {
template <typename TObjType>
inline void NVDelete(NVFoundationBase &alloc, TObjType *item)
{
    NVDelete(alloc.getAllocator(), item);
}
}

/**
\brief Foundation SDK singleton class.

You need to have an instance of this class to instance the higher level SDKs.
*/
class QT3DS_FOUNDATION_API NVFoundation : public NVFoundationBase
{
public:
    virtual void addRef() = 0;
    /**
    \brief Destroys the instance it is called on.

    The operation will fail, if there are still modules referencing the foundation object. Release
    all dependent modules prior
    to calling this method.

    @see NVCreateFoundation()
    */
    virtual void release() = 0;

    /**
    Retrieves the allocator this object was created with.
    */
    virtual NVAllocatorCallback &getAllocatorCallback() const = 0;

protected:
    virtual ~NVFoundation() {}
};

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/**
\brief Creates an instance of the foundation class

\param version Version number we are expecting (should be QT3DS_FOUNDATION_VERSION)
\param allocator User supplied interface for allocating memory(see #NVAllocatorCallback)
\return Foundation instance on success, NULL if operation failed

@see NVFoundation
*/

#ifdef QT3DS_FOUNDATION_NO_EXPORTS
QT3DS_AUTOTEST_EXPORT
#else
QT3DS_FOUNDATION_API
#endif
qt3ds::NVFoundation *QT3DS_CALL_CONV NVCreateFoundation(
        qt3ds::QT3DSU32 version, qt3ds::NVAllocatorCallback &allocator);

/** @} */
#endif // QT3DS_FOUNDATION_QT3DS_FOUNDATION_H
