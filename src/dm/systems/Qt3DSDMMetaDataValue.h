/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
#pragma once
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMDataTypes.h"
#include "foundation/Qt3DSDataRef.h"
#include "Qt3DSDMValue.h"

#include <QStringList>

namespace qt3dsdm {

struct AdditionalMetaDataType {

  enum Value {
    None,
    StringList,
    Range,
    Image,
    Color,
    Rotation,
    Font,
    FontSize,
    MultiLine,
    ObjectRef,
    Mesh,
    Import,
    Texture,
    Renderable,
    PathBuffer,
    ShadowMapResolution,
    String,
  };


  Q_ENUM(Value)
  Q_GADGET
};

// List type metadata
typedef eastl::vector<TCharStr> TMetaDataStringList;

// Float type metadata
struct SMetaDataRange
{
    SMetaDataRange() {}

    SMetaDataRange(float min, float max, int decimals = -1)
        : m_min(min)
        , m_max(max)
        , m_decimals(decimals)
    {
    }

    bool operator==(const SMetaDataRange &other) const
    {
        // no need to check m_decimals for quality as it is not significant to the range value
        return m_min == other.m_min && m_max == other.m_max;
    }

    float m_min = 0.0f;
    float m_max = 0.0f;
    int m_decimals = -1; // num decimals to show, -1: dynamically calculated
};
}

namespace qt3ds {
namespace foundation {

    template <>
    struct DestructTraits<qt3dsdm::SMetaDataRange>
    {
        void destruct(qt3dsdm::SMetaDataRange &) {}
    };
}
}

namespace qt3dsdm {

template <typename TDataType>
struct SMetaDataTypeToEnumMap
{
};
template <>
struct SMetaDataTypeToEnumMap<eastl::vector<TCharStr>>
{
    static AdditionalMetaDataType::Value getType() { return AdditionalMetaDataType::StringList; }
};
template <>
struct SMetaDataTypeToEnumMap<SMetaDataRange>
{
    static AdditionalMetaDataType::Value getType() { return AdditionalMetaDataType::Range; }
};

struct SMetaDataValueTraits
{
    typedef AdditionalMetaDataType::Value TIdType;

    enum {
        TBufferSize = sizeof(TMetaDataStringList),
    };

    static TIdType getNoDataId() { return AdditionalMetaDataType::None; }

    template <typename TDataType>
    static TIdType getType()
    {
        return SMetaDataTypeToEnumMap<TDataType>::getType();
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(char *inData, AdditionalMetaDataType::Value inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case AdditionalMetaDataType::StringList:
            return inVisitor(*reinterpret_cast<TMetaDataStringList *>(inData));
        case AdditionalMetaDataType::Range:
            return inVisitor(*reinterpret_cast<SMetaDataRange *>(inData));
        default:
            QT3DS_ASSERT(false);
        case AdditionalMetaDataType::None:
            return inVisitor();
        }
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(const char *inData, AdditionalMetaDataType::Value inType,
                          TVisitorType inVisitor)
    {
        switch (inType) {
        case AdditionalMetaDataType::StringList:
            return inVisitor(*reinterpret_cast<const TMetaDataStringList *>(inData));
        case AdditionalMetaDataType::Range:
            return inVisitor(*reinterpret_cast<const SMetaDataRange *>(inData));
        default:
            QT3DS_ASSERT(false);
        case AdditionalMetaDataType::None:
            return inVisitor();
        }
    }
};

struct SMetaDataData
    : public qt3ds::foundation::
          DiscriminatedUnion<qt3ds::foundation::
                                 DiscriminatedUnionGenericBase<SMetaDataValueTraits,
                                                               Qt3DSDMDataTypeUnionTraits::
                                                                   TBufferSize>,
                             SMetaDataValueTraits::TBufferSize>
{
    typedef qt3ds::foundation::
        DiscriminatedUnion<qt3ds::foundation::DiscriminatedUnionGenericBase<SMetaDataValueTraits,
                                                                         Qt3DSDMDataTypeUnionTraits::
                                                                             TBufferSize>,
                           SMetaDataValueTraits::TBufferSize>
            TBase;
    SMetaDataData() {}
    SMetaDataData(const SMetaDataData &inOther)
        : TBase(static_cast<const TBase &>(inOther))
    {
    }
    SMetaDataData &operator=(const SMetaDataData &inOther)
    {
        TBase::operator=(static_cast<const TBase &>(inOther));
        return *this;
    }
    SMetaDataData(const TMetaDataStringList &inData)
        : TBase(inData)
    {
    }
    SMetaDataData(const SMetaDataRange &inData)
        : TBase(inData)
    {
    }
    SMetaDataData &operator=(const TMetaDataStringList &inData)
    {
        TBase::operator=(inData);
        return *this;
    }
    SMetaDataData &operator=(const SMetaDataRange &inData)
    {
        TBase::operator=(inData);
        return *this;
    }
    bool empty() const { return getType() == AdditionalMetaDataType::None; }
};

template <>
struct Qt3DSDMGetter<SMetaDataData>
{
    template <typename TRetType>
    TRetType doGet(const SMetaDataData &inValue)
    {
        return inValue.getData<TRetType>();
    }
};

template <>
struct Qt3DSDMValueTyper<SMetaDataData>
{
    AdditionalMetaDataType::Value Get(const SMetaDataData &inValue) { return inValue.getType(); }
};

typedef SMetaDataData TMetaDataData;


template <>
inline QStringList get<QStringList>(const qt3dsdm::SMetaDataData &inType)
{
    QStringList result;
    if (inType.getType() == qt3dsdm::AdditionalMetaDataType::None)
        return result;

    auto list = qt3dsdm::get<qt3dsdm::TMetaDataStringList>(inType);
    std::transform(list.begin(), list.end(), std::back_inserter(result), [](const Qt3DSDMStr &s) {
        return QString::fromWCharArray(s.wide_str());
    });
    return result;
}
}

Q_DECLARE_METATYPE(qt3dsdm::AdditionalMetaDataType)

