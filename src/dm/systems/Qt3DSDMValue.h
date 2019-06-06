/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#ifndef QT3DSDM_VALUE_H
#define QT3DSDM_VALUE_H
#include "Qt3DSDMDataTypes.h"

#include <QColor>
#include <QMetaType>
#include <QVariant>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

namespace qt3dsdm {

template <typename TDataType>
struct SDataTypeToEnumMap
{
};

template <DataModelDataType::Value TEnumVal>
struct SEnumToDataTypeMap
{
};

#define DEFINE_QT3DSDM_DATA_TYPE_MAP(dtype, enumName)                                                \
    template <>                                                                                    \
    struct SDataTypeToEnumMap<dtype>                                                               \
    {                                                                                              \
        static DataModelDataType::Value getType() { return enumName; }                                   \
    };                                                                                             \
    template <>                                                                                    \
    struct SEnumToDataTypeMap<enumName>                                                            \
    {                                                                                              \
        typedef dtype TDataType;                                                                   \
    };

#define ITERATE_QT3DSDM_DATA_TYPES                                                                   \
    HANDLE_QT3DSDM_DATA_TYPE(float, DataModelDataType::Float);                                        \
    HANDLE_QT3DSDM_DATA_TYPE(SFloat2, DataModelDataType::Float2);                                     \
    HANDLE_QT3DSDM_DATA_TYPE(SFloat3, DataModelDataType::Float3);                                     \
    HANDLE_QT3DSDM_DATA_TYPE(SFloat4, DataModelDataType::Float4);                                     \
    HANDLE_QT3DSDM_DATA_TYPE(qt3ds::QT3DSI32, DataModelDataType::Long);                                          \
    HANDLE_QT3DSDM_DATA_TYPE(TDataStrPtr, DataModelDataType::String);                                 \
    HANDLE_QT3DSDM_DATA_TYPE(bool, DataModelDataType::Bool);                                          \
    HANDLE_QT3DSDM_DATA_TYPE(SLong4, DataModelDataType::Long4);                                       \
    HANDLE_QT3DSDM_DATA_TYPE(SStringRef, DataModelDataType::StringRef);                               \
    HANDLE_QT3DSDM_DATA_TYPE(SObjectRefType, DataModelDataType::ObjectRef);                           \
    HANDLE_QT3DSDM_DATA_TYPE(SStringOrInt, DataModelDataType::StringOrInt);                           \
    HANDLE_QT3DSDM_DATA_TYPE(TFloatList, DataModelDataType::FloatList);

#define HANDLE_QT3DSDM_DATA_TYPE(a, b) DEFINE_QT3DSDM_DATA_TYPE_MAP(a, b)
ITERATE_QT3DSDM_DATA_TYPES
#undef HANDLE_QT3DSDM_DATA_TYPE

struct Qt3DSDMDataTypeUnionTraits
{
    typedef DataModelDataType::Value TIdType;

    enum {
        TBufferSize = sizeof(SObjectRefType),
    };

    static TIdType getNoDataId() { return DataModelDataType::None; }

    template <typename TDataType>
    static TIdType getType()
    {
        return SDataTypeToEnumMap<TDataType>::getType();
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(char *inData, DataModelDataType::Value inType, TVisitorType inVisitor)
    {
        switch (inType) {
#define HANDLE_QT3DSDM_DATA_TYPE(dtype, enumType)                                                    \
    case enumType:                                                                                 \
        return inVisitor(*NVUnionCast<dtype *>(inData));
            ITERATE_QT3DSDM_DATA_TYPES
#undef HANDLE_QT3DSDM_DATA_TYPE

        default:
            QT3DS_ASSERT(false);
        case DataModelDataType::None:
            return inVisitor();
        }
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(const char *inData, DataModelDataType::Value inType, TVisitorType inVisitor)
    {
        switch (inType) {
#define HANDLE_QT3DSDM_DATA_TYPE(dtype, enumType)                                                    \
    case enumType:                                                                                 \
        return inVisitor(*NVUnionCast<const dtype *>(inData));
            ITERATE_QT3DSDM_DATA_TYPES
#undef HANDLE_QT3DSDM_DATA_TYPE

        default:
            QT3DS_ASSERT(false);
        case DataModelDataType::None:
            return inVisitor();
        }
    }
};

typedef qt3ds::foundation::
    DiscriminatedUnion<qt3ds::foundation::
                           DiscriminatedUnionGenericBase<Qt3DSDMDataTypeUnionTraits,
                                                         Qt3DSDMDataTypeUnionTraits::TBufferSize>,
                       Qt3DSDMDataTypeUnionTraits::TBufferSize>
        TValue;

struct SValue : public TValue
{
    SValue() {}
    SValue(const SValue &inOther)
        : TValue(static_cast<const TValue &>(inOther))
    {
    }
    SValue(const QVariant &inData);

    template <typename TDataType>
    SValue(const TDataType &inData)
        : TValue(inData)
    {
    }
    SValue &operator=(const SValue &inOther)
    {
        TValue::operator=(inOther);
        return *this;
    }
    bool operator==(const SValue &inOther) const { return TValue::operator==(inOther); }
    bool operator!=(const SValue &inOther) const { return TValue::operator!=(inOther); }
    bool empty() const { return getType() == DataModelDataType::None; }

    // Conversion from this data type into a data model value
    const SValue &toOldSkool() const { return *this; }

    QVariant toQVariant() const;
};

typedef std::shared_ptr<SValue> SValuePtr;

template <>
struct Qt3DSDMGetter<SValue>
{
    template <typename TRetType>
    TRetType doGet(const SValue &inValue)
    {
        return inValue.getData<TRetType>();
    }
};

template <>
struct Qt3DSDMValueTyper<SValue>
{
    DataModelDataType::Value Get(const SValue &inValue) { return inValue.getType(); }
};

inline bool CheckValueType(DataModelDataType::Value inType, const SValue &inValue)
{
    bool retval = inType == inValue.getType();
    if (!retval)
        throw ValueTypeError(L"");
    return retval;
}

inline bool Equals(const SValue &lhs, const SValue &rhs)
{
    return lhs == rhs;
}

class SInternValue
{
    SValue m_Value;

public:
    SInternValue(const SValue &inValue, IStringTable &inTable);
    SInternValue() {}
    SInternValue(const SInternValue &inOther);
    SInternValue &operator=(const SInternValue &inOther)
    {
        m_Value = inOther.m_Value;
        return *this;
    }

    static SInternValue ISwearThisHasAlreadyBeenInternalized(const SValue &inValue)
    {
        SInternValue retval;
        retval.m_Value = inValue;
        return retval;
    }

    const SValue &GetValue() const { return m_Value; }
    operator const SValue &() const { return m_Value; }
};

template <>
struct SDefaulter<SValue>
{
    inline bool SetDefault(DataModelDataType::Value inDataType, SValue &outValue)
    {
        switch (inDataType) {
        case DataModelDataType::Float:
            outValue = SValue(0.f);
            break;
        case DataModelDataType::Float2:
            outValue = SValue(SFloat2());
            break;
        case DataModelDataType::Float3:
            outValue = SValue(SFloat3());
            break;
        case DataModelDataType::Float4:
            outValue = SValue(SFloat4());
            break;
        case DataModelDataType::Long:
            outValue = SValue(0);
            break;
        case DataModelDataType::String:
            outValue = SValue(TDataStrPtr(new CDataStr(L"")));
            break;
        case DataModelDataType::Bool:
            outValue = SValue(false);
            break;
        case DataModelDataType::Long4:
            outValue = SValue(SLong4());
            break;
        case DataModelDataType::StringRef:
            outValue = SValue(SStringRef());
            break;
        case DataModelDataType::ObjectRef:
            outValue = SValue(SObjectRefType());
            break;
        case DataModelDataType::FloatList:
            outValue = SValue(TFloatList());
            break;
        case DataModelDataType::StringOrInt:
        default:
            outValue = SValue(SStringOrInt());
            return false;
        }
        return true;
    }
};

inline SObjectRefType ConvertToObjectRef(const SValue &inValue)
{
    using namespace std;
    switch (GetValueType(inValue)) {
    case DataModelDataType::StringRef:
        return SObjectRefType(make_shared<CDataStr>(get<SStringRef>(inValue).m_Id));
    case DataModelDataType::String:
        return SObjectRefType(get<TDataStrPtr>(inValue));
    case DataModelDataType::Long4:
        return SObjectRefType(get<SLong4>(inValue));
    case DataModelDataType::ObjectRef:
        return get<SObjectRefType>(inValue);
    default:
        break;
    }
    return SObjectRefType();
}


template <>
inline QColor get<QColor>(const SValue &inType)
{
    auto f = get<qt3dsdm::SFloat4>(inType);
    qreal r = qBound<qreal>(0.0, f.m_Floats[0], 1.0);
    qreal g = qBound<qreal>(0.0, f.m_Floats[1], 1.0);
    qreal b = qBound<qreal>(0.0, f.m_Floats[2], 1.0);
    qreal a = qBound<qreal>(0.0, f.m_Floats[3], 1.0);
    return QColor::fromRgbF(r, g, b, a);
}

template <>
inline QString get<QString>(const qt3dsdm::SValue &inType)
{
    return QString::fromWCharArray(qt3dsdm::get<qt3dsdm::TDataStrPtr>(inType)->GetData());
}

template <>
inline QVector2D get<QVector2D>(const qt3dsdm::SValue &inType)
{
    auto f = get<qt3dsdm::SFloat2>(inType);
    return QVector2D(f.m_Floats[0], f.m_Floats[1]);
}

template <>
inline QVector3D get<QVector3D>(const qt3dsdm::SValue &inType)
{
    auto f = get<qt3dsdm::SFloat3>(inType);
    return QVector3D(f.m_Floats[0], f.m_Floats[1], f.m_Floats[2]);
}

template <>
inline QVector<float> get<QVector<float> >(const qt3dsdm::SValue &inType)
{
    auto f = get<qt3dsdm::SFloat4>(inType);
    return {f.m_Floats[0], f.m_Floats[1], f.m_Floats[2], f.m_Floats[3]};
}

// KDAB_TODO Shortcut to not define our own 4 member long structure
template <>
inline QVector<qt3ds::QT3DSU32> get<QVector<qt3ds::QT3DSU32> >(const qt3dsdm::SValue &inType)
{
    auto f = get<qt3dsdm::SLong4>(inType);
    return {f.m_Longs[0], f.m_Longs[1], f.m_Longs[2], f.m_Longs[3]};
}

}
Q_DECLARE_METATYPE(qt3dsdm::DataModelDataType)
#endif
