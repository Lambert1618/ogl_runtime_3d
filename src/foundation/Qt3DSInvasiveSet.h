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

#ifndef QT3DS_FOUNDATION_INVASIVE_SET_H
#define QT3DS_FOUNDATION_INVASIVE_SET_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/Qt3DSContainers.h"

namespace qt3ds {
namespace foundation {

    template <typename TObjectType, typename TGetSetIndexOp, typename TSetSetIndexOp>
    class InvasiveSet
    {
        nvvector<TObjectType *> mSet;

        InvasiveSet(const InvasiveSet &other);
        InvasiveSet &operator=(const InvasiveSet &other);

    public:
        InvasiveSet(NVAllocatorCallback &callback, const char *allocName)
            : mSet(callback, allocName)
        {
        }

        bool insert(TObjectType &inObject)
        {
            QT3DSU32 currentIdx = TGetSetIndexOp()(inObject);
            if (currentIdx == QT3DS_MAX_U32) {
                TSetSetIndexOp()(inObject, mSet.size());
                mSet.push_back(&inObject);
                return true;
            }
            return false;
        }

        bool remove(TObjectType &inObject)
        {
            QT3DSU32 currentIdx = TGetSetIndexOp()(inObject);
            if (currentIdx != QT3DS_MAX_U32) {
                TObjectType *theEnd = mSet.back();
                TObjectType *theObj = &inObject;
                if (theEnd != theObj) {
                    TSetSetIndexOp()(*theEnd, currentIdx);
                    mSet[currentIdx] = theEnd;
                }
                mSet.pop_back();
                TSetSetIndexOp()(inObject, QT3DS_MAX_U32);
                return true;
            }
            return false;
        }

        bool contains(TObjectType &inObject) { return TGetSetIndexOp()(inObject) != QT3DS_MAX_U32; }

        void clear()
        {
            for (QT3DSU32 idx = 0; idx < mSet.size(); ++idx)
                TSetSetIndexOp()(*(mSet[idx]), QT3DS_MAX_U32);
            mSet.clear();
        }

        TObjectType *operator[](QT3DSU32 idx) { return mSet[idx]; }
        const TObjectType *operator[](QT3DSU32 idx) const { return mSet[idx]; }
        QT3DSU32 size() const { return mSet.size(); }
        TObjectType **begin() { return mSet.begin(); }
        TObjectType **end() { return mSet.end(); }
        const TObjectType **begin() const { return mSet.begin(); }
        const TObjectType **end() const { return mSet.end(); }
        const TObjectType *back() const { return mSet.back(); }
        TObjectType *back() { return mSet.back(); }
    };
}
}
#endif