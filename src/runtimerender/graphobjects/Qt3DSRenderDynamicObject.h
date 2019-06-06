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
#ifndef QT3DS_RENDER_DYNAMIC_OBJECT_H
#define QT3DS_RENDER_DYNAMIC_OBJECT_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderGraphObject.h"
#include "Qt3DSRenderNode.h"
#include "EASTL/string.h"
#include "StringTools.h"

namespace qt3ds {
namespace render {

    namespace dynamic {
        struct SPropertyDefinition;
    }

    // Dynamic objects are objects that have variable number of properties during runtime.
    struct SDynamicObject : public SGraphObject
    {
        CRegisteredString m_ClassName;
        NodeFlags m_Flags;
        QT3DSU32 m_DataSectionByteSize;
        QT3DSU32 m_ThisObjectSize;

        SDynamicObject(GraphObjectTypes::Enum inType, CRegisteredString inClassName,
                       QT3DSU32 inDSByteSize, QT3DSU32 thisObjSize);

        QT3DSU8 *GetDataSectionBegin()
        {
            QT3DSU8 *thisObjectStart = reinterpret_cast<QT3DSU8 *>(this);
            QT3DSU8 *retval = thisObjectStart + m_ThisObjectSize;
            QT3DS_ASSERT((reinterpret_cast<size_t>(retval) % 4 == 0));
            return retval;
        }

        const QT3DSU8 *GetDataSectionBegin() const
        {
            return const_cast<SDynamicObject *>(this)->GetDataSectionBegin();
        }

        QT3DSU8 *GetDataSectionEnd() { return GetDataSectionBegin() + m_DataSectionByteSize; }

        template <typename TDataType>
        void SetPropertyValueT(const dynamic::SPropertyDefinition &inDefinition,
                               const TDataType &inType);
        template <typename TStrType>
        void SetStrPropertyValueT(dynamic::SPropertyDefinition &inDefinition,
                                  const char8_t *inValue, const char8_t *inProjectDir,
                                  TStrType &ioWorkspace, IStringTable &inStrTable);

        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition, bool inValue);
        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition, QT3DSF32 inValue);
        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition, QT3DSF32 inValue,
                              QT3DSU32 inOffset);
        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                              const QT3DSVec2 &inValue);
        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                              const QT3DSVec3 &inValue);
        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                              const QT3DSVec4 &inValue);
        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition, QT3DSI32 inValue);
        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                              CRegisteredString inValue);

        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                              const char8_t *inValue, const char8_t *inProjectDir,
                              Qt3DSString &ioWorkspace, IStringTable &inStrTable);

        void SetPropertyValue(const dynamic::SPropertyDefinition &inDefinition,
                              const char8_t *inValue, const char8_t *inProjectDir,
                              eastl::string &ioWorkspace, IStringTable &inStrTable);

        // Generic method used during serialization
        // to remap string and object pointers
        template <typename TRemapperType>
        void Remap(TRemapperType &inRemapper)
        {
            SGraphObject::Remap(inRemapper);
            inRemapper.Remap(m_ClassName);
        }
    };
}
}
#endif
