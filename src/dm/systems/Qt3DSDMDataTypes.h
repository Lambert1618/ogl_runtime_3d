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
#ifndef QT3DSDM_DATA_H
#define QT3DSDM_DATA_H

#include <memory>
#include <functional>

#include "Qt3DSDMErrors.h"
#include "Qt3DSDMStringTable.h"
#include "Qt3DSDMWindowsCompatibility.h"
#include <stdexcept>
#include <vector>
#include <map>
#include <set>
#include <EABase/eabase.h> //char8_t, etc.
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include "foundation/StrConvertUTF.h"
#include "foundation/Qt3DSDiscriminatedUnion.h"

#include <QObject>

namespace qt3dsdm {

typedef const wchar_t *TCharPtr;
struct Qt3DSDMStr : public eastl::basic_string<qt3ds::foundation::TWCharEASTLConverter::TCharType>
{
    typedef qt3ds::foundation::TWCharEASTLConverter::TCharType TCharType;
    typedef eastl::basic_string<qt3ds::foundation::TWCharEASTLConverter::TCharType> TStrType;
    Qt3DSDMStr() {}
    Qt3DSDMStr(const wchar_t *inStr)
        : TStrType(qt3ds::NVUnionCast<const TCharType *>(inStr))
    {
    }
    Qt3DSDMStr(const TStrType &inOther)
        : TStrType(inOther)
    {
    }

    const wchar_t *wide_str() const { return reinterpret_cast<const wchar_t *>(c_str()); }
    bool operator==(const Qt3DSDMStr &inOther) const { return TStrType::compare(inOther) == 0; }
    bool operator!=(const Qt3DSDMStr &inOther) const { return TStrType::compare(inOther) != 0; }
    bool operator==(const wchar_t *inOther) const
    {
        return TStrType::compare(reinterpret_cast<const TCharType *>(inOther)) == 0;
    }
    bool operator!=(const wchar_t *inOther) const
    {
        return TStrType::compare(reinterpret_cast<const TCharType *>(inOther)) != 0;
    }

    TStrType::size_type find_first_of(const wchar_t *p, size_type position = 0) const
    {
        return TStrType::find_first_of(reinterpret_cast<const TCharType *>(p), position);
    }
    TStrType::size_type find_first_not_of(const wchar_t *p, size_type position = 0) const
    {
        return TStrType::find_first_not_of(reinterpret_cast<const TCharType *>(p), position);
    }
    TStrType::size_type find_last_not_of(const wchar_t *p,
                                         size_type position = TStrType::npos) const
    {
        return TStrType::find_last_not_of(reinterpret_cast<const TCharType *>(p), position);
    }
    TStrType::size_type find(const wchar_t *p, size_type position = 0) const
    {
        return TStrType::find(reinterpret_cast<const TCharType *>(p), position);
    }
    TStrType::size_type find(wchar_t p, size_type position = 0) const
    {
        return TStrType::find(static_cast<TCharType>(p), position);
    }
    TStrType::size_type rfind(const wchar_t *p, size_type position = TStrType::npos) const
    {
        return TStrType::rfind(reinterpret_cast<const TCharType *>(p), position);
    }
    TStrType::size_type rfind(wchar_t p, size_type position = TStrType::npos) const
    {
        return TStrType::rfind(p, position);
    }
    Qt3DSDMStr replace(size_type position, size_type length, const wchar_t *inNewStr)
    {
        return TStrType::replace(position, length, reinterpret_cast<const TCharType *>(inNewStr));
    }

    void append(const wchar_t *p) { TStrType::append(reinterpret_cast<const TCharType *>(p)); }
    void append(const wchar_t *p, size_type position, size_type n)
    {
        TStrType::append(reinterpret_cast<const TCharType *>(p), position, n);
    }
    void append(const Qt3DSDMStr &p) { TStrType::append(p); }
    void assign(const wchar_t *p) { TStrType::assign(reinterpret_cast<const TCharType *>(p)); }
    void assign(const Qt3DSDMStr &p) { TStrType::assign(p); }
};
typedef Qt3DSDMStr TCharStr;
typedef std::vector<wchar_t> TCharList;
typedef eastl::vector<TCharStr> TCharStrList;
typedef std::map<TCharStr, int> TCharStrIntMap;
typedef std::map<int, TCharStr> TIntTCharStrMap;
typedef std::vector<int> TIntList;
typedef std::set<int> TIntSet;
typedef eastl::vector<float> TFloatList;

template <typename TDataType>
class CImmutableVector : private std::vector<TDataType>
{
public:
    typedef std::vector<TDataType> base;

    CImmutableVector(const std::vector<TDataType> &inSrc)
        : base(inSrc)
    {
    }
    CImmutableVector(const CImmutableVector &inSrc)
        : base(inSrc)
    {
    }

    CImmutableVector &operator=(const CImmutableVector &inSrc)
    {
        base::operator=(inSrc);
        return *this;
    }

    typename std::vector<TDataType>::const_iterator begin() const { return base::begin(); }
    typename std::vector<TDataType>::const_iterator end() const { return base::end(); }

    size_t size() const { return base::size(); }

    const TDataType &at(int inIndex) const { return base::at(inIndex); }
    const TDataType &operator[](int inIndex) const { return base::at(inIndex); }
};

typedef CImmutableVector<TCharStr> TImmutableCharStrList;
typedef std::shared_ptr<TImmutableCharStrList> TImmutableCharStrListPtr;

class CDataStr
{
    TCharList m_Data;

public:
    CDataStr() {}
    CDataStr(TCharList &inData) { m_Data.swap(inData); }
    CDataStr(const wchar_t *inData)
    {
        if (inData && *inData)
            m_Data.insert(m_Data.begin(), inData, inData + wcslen(inData));
        m_Data.push_back(0);
    }
    CDataStr(const wchar_t *inData, size_t inLen)
    {
        if (inData && *inData)
            m_Data.insert(m_Data.begin(), inData, inData + inLen);
        if (m_Data.size() > 0 && m_Data.back() != 0)
            m_Data.push_back(0);
    }
    CDataStr(const CDataStr &inOther)
        : m_Data(inOther.m_Data)
    {
    }
    virtual ~CDataStr() {}

    CDataStr &operator=(const CDataStr &inOther)
    {
        m_Data = inOther.m_Data;
        return *this;
    }
    const wchar_t *GetData() const { return m_Data.size() <= 1 ? L"" : &(*m_Data.begin()); }
    size_t GetLength() const { return m_Data.size() > 1 ? m_Data.size() - 1 : 0; }
    bool operator==(const CDataStr &inOther) const { return m_Data == inOther.m_Data; }
    QString toQString() const { return QString::fromWCharArray(GetData()); }
};

typedef std::shared_ptr<CDataStr> TDataStrPtr;

struct SFloat2
{
    float m_Floats[2];
    SFloat2(float f1 = 0, float f2 = 0)
    {
        m_Floats[0] = f1;
        m_Floats[1] = f2;
    }
    SFloat2(const SFloat2 &inOther) { operator=(inOther); }
    SFloat2 &operator=(const SFloat2 &inOther)
    {
        m_Floats[0] = inOther.m_Floats[0];
        m_Floats[1] = inOther.m_Floats[1];
        return *this;
    }
    bool operator==(const SFloat2 &other) const
    {
        return m_Floats[0] == other.m_Floats[0] && m_Floats[1] == other.m_Floats[1];
    }
    float &operator[](size_t inIndex)
    {
        switch (inIndex) {
        default:
            throw std::out_of_range("");
        case 0:
        case 1:
            return m_Floats[inIndex];
        }
    }
    const float &operator[](size_t inIndex) const
    {
        switch (inIndex) {
        default:
            throw std::out_of_range("");
        case 0:
        case 1:
            return m_Floats[inIndex];
        }
    }
};

struct SFloat3
{
    float m_Floats[3];

    SFloat3(float f1 = 0, float f2 = 0, float f3 = 0)
    {
        m_Floats[0] = f1;
        m_Floats[1] = f2;
        m_Floats[2] = f3;
    }
    SFloat3(const SFloat3 &inOther) { operator=(inOther); }
    SFloat3 &operator=(const SFloat3 &inOther)
    {
        m_Floats[0] = inOther.m_Floats[0];
        m_Floats[1] = inOther.m_Floats[1];
        m_Floats[2] = inOther.m_Floats[2];
        return *this;
    }

    bool operator==(const SFloat3 &other) const
    {
        return m_Floats[0] == other.m_Floats[0] && m_Floats[1] == other.m_Floats[1]
            && m_Floats[2] == other.m_Floats[2];
    }

    float &operator[](size_t inIndex)
    {
        switch (inIndex) {
        default:
            throw std::out_of_range("");
        case 0:
        case 1:
        case 2:
            return m_Floats[inIndex];
        }
    }
    const float &operator[](size_t inIndex) const
    {
        switch (inIndex) {
        default:
            throw std::out_of_range("");
        case 0:
        case 1:
        case 2:
            return m_Floats[inIndex];
        }
    }
};

struct SFloat4
{
    float m_Floats[4];

    SFloat4(float f1 = 0, float f2 = 0, float f3 = 0, float f4 = 1)
    {
        m_Floats[0] = f1;
        m_Floats[1] = f2;
        m_Floats[2] = f3;
        m_Floats[3] = f4;
    }
    SFloat4(const SFloat4 &inOther) { operator=(inOther); }
    SFloat4 &operator=(const SFloat4 &inOther)
    {
        m_Floats[0] = inOther.m_Floats[0];
        m_Floats[1] = inOther.m_Floats[1];
        m_Floats[2] = inOther.m_Floats[2];
        m_Floats[3] = inOther.m_Floats[3];
        return *this;
    }

    bool operator==(const SFloat4 &other) const
    {
        return m_Floats[0] == other.m_Floats[0] && m_Floats[1] == other.m_Floats[1]
               && m_Floats[2] == other.m_Floats[2] && m_Floats[3] == other.m_Floats[3];
    }

    float &operator[](size_t inIndex)
    {
        if (inIndex < 4)
            return m_Floats[inIndex];

        throw std::out_of_range("");
    }
    const float &operator[](size_t inIndex) const
    {
        if (inIndex < 4)
            return m_Floats[inIndex];

        throw std::out_of_range("");
    }
};

struct SLong4
{
    qt3ds::QT3DSU32 m_Longs[4];

    SLong4(qt3ds::QT3DSU32 l1 = 0,
           qt3ds::QT3DSU32 l2 = 0,
           qt3ds::QT3DSU32 l3 = 0,
           qt3ds::QT3DSU32 l4 = 0)
    {
        m_Longs[0] = l1;
        m_Longs[1] = l2;
        m_Longs[2] = l3;
        m_Longs[3] = l4;
    }
    SLong4(const SLong4 &inOther) { operator=(inOther); }
    SLong4 &operator=(const SLong4 &inOther)
    {
        m_Longs[0] = inOther.m_Longs[0];
        m_Longs[1] = inOther.m_Longs[1];
        m_Longs[2] = inOther.m_Longs[2];
        m_Longs[3] = inOther.m_Longs[3];
        return *this;
    }

    bool operator==(const SLong4 &other) const
    {
        return m_Longs[0] == other.m_Longs[0] && m_Longs[1] == other.m_Longs[1]
            && m_Longs[2] == other.m_Longs[2] && m_Longs[3] == other.m_Longs[3];
    }

    bool operator<(const SLong4 &other) const
    {
        if (m_Longs[0] < other.m_Longs[0])
            return true;
        else if (m_Longs[0] == other.m_Longs[0]) {
            if (m_Longs[1] < other.m_Longs[1])
                return true;
            else if (m_Longs[1] == other.m_Longs[1]) {
                if (m_Longs[2] < other.m_Longs[2])
                    return true;
                else if (m_Longs[3] < other.m_Longs[3])
                    return true;
            }
        }
        return false;
    }

    bool Valid() const
    {
        bool retval = false;
        for (int idx = 0; idx < 4; ++idx)
            retval = retval || m_Longs[0] > 0;
        return retval;
    }
};

typedef std::vector<SLong4> TLong4Vec;

struct SSizet4
{
    size_t m_Longs[4];

    SSizet4(size_t l1 = 0, size_t l2 = 0, size_t l3 = 0, size_t l4 = 0)
    {
        m_Longs[0] = l1;
        m_Longs[1] = l2;
        m_Longs[2] = l3;
        m_Longs[3] = l4;
    }
    SSizet4(const SSizet4 &inOther) { operator=(inOther); }
    SSizet4 &operator=(const SSizet4 &inOther)
    {
        m_Longs[0] = inOther.m_Longs[0];
        m_Longs[1] = inOther.m_Longs[1];
        m_Longs[2] = inOther.m_Longs[2];
        m_Longs[3] = inOther.m_Longs[3];
        return *this;
    }

    bool operator==(const SSizet4 &other) const
    {
        return m_Longs[0] == other.m_Longs[0] && m_Longs[1] == other.m_Longs[1]
            && m_Longs[2] == other.m_Longs[2] && m_Longs[3] == other.m_Longs[3];
    }

    bool operator<(const SSizet4 &other) const
    {
        if (m_Longs[0] < other.m_Longs[0])
            return true;
        else if (m_Longs[0] == other.m_Longs[0]) {
            if (m_Longs[1] < other.m_Longs[1])
                return true;
            else if (m_Longs[1] == other.m_Longs[1]) {
                if (m_Longs[2] < other.m_Longs[2])
                    return true;
                else if (m_Longs[3] < other.m_Longs[3])
                    return true;
            }
        }
        return false;
    }

    bool Valid() const
    {
        bool retval = false;
        for (int idx = 0; idx < 4; ++idx)
            retval = retval || m_Longs[0] > 0;
        return retval;
    }
};

inline bool IsTrivial(const wchar_t *inStr)
{
    return inStr == NULL || *inStr == 0;
}
inline const wchar_t *NonNull(const wchar_t *inStr)
{
    return inStr ? inStr : L"";
}
inline bool AreEqual(const wchar_t *lhs, const wchar_t *rhs)
{
    return wcscmp(NonNull(lhs), NonNull(rhs)) == 0;
}

inline bool IsTrivial(const char8_t *inStr)
{
    return inStr == NULL || *inStr == 0;
}
inline const char8_t *NonNull(const char8_t *inStr)
{
    return inStr ? inStr : "";
}
inline bool AreEqual(const char8_t *lhs, const char8_t *rhs)
{
    return strcmp(NonNull(lhs), NonNull(rhs)) == 0;
}

inline bool AreEqualCaseless(const char8_t *lhs, const char8_t *rhs)
{
    if (IsTrivial(lhs) && IsTrivial(rhs))
        return true;
    if (IsTrivial(lhs) || IsTrivial(rhs))
        return false;
    for (; *lhs && *rhs; ++lhs, ++rhs)
        if (tolower(*lhs) != tolower(*rhs))
            return false;
    if (*lhs || *rhs)
        return false;
    return true;
}

struct SStringRef
{
    const wchar_t *m_Id;

    SStringRef(const wchar_t *id = L"")
        : m_Id(NonNull(id))
    {
    }

    operator const wchar_t *() const { return m_Id; }
    bool operator==(const SStringRef &inOther) const { return AreEqual(m_Id, inOther.m_Id); }
};
}

// Traits specializations have to be in the same namespace as they were first declared.
namespace qt3ds {
namespace foundation {

    template <>
    struct DestructTraits<qt3dsdm::SFloat2>
    {
        void destruct(qt3dsdm::SFloat2 &) {}
    };
    template <>
    struct DestructTraits<qt3dsdm::SFloat3>
    {
        void destruct(qt3dsdm::SFloat3 &) {}
    };
    template <>
    struct DestructTraits<qt3dsdm::SFloat4>
    {
        void destruct(qt3dsdm::SFloat4 &) {}
    };
    template <>
    struct DestructTraits<qt3dsdm::SLong4>
    {
        void destruct(qt3dsdm::SLong4 &) {}
    };
    template <>
    struct DestructTraits<qt3dsdm::SStringRef>
    {
        void destruct(qt3dsdm::SStringRef &) {}
    };

    template <>
    struct EqualVisitorTraits<qt3dsdm::TDataStrPtr>
    {
        bool operator()(const qt3dsdm::TDataStrPtr &lhs, const qt3dsdm::TDataStrPtr &rhs)
        {
            if (lhs && rhs)
                return *lhs == *rhs;
            if (lhs || rhs)
                return false;
            return true;
        }
    };
}
}

namespace qt3dsdm {

using qt3ds::NVUnionCast;

struct ObjectReferenceType
{
    enum Enum {
        Unknown = 0,
        Absolute = 1,
        Relative = 2,
    };
};

template <typename TDataType>
struct SObjectRefTypeTraits
{
};

template <>
struct SObjectRefTypeTraits<SLong4>
{
    ObjectReferenceType::Enum getType() { return ObjectReferenceType::Absolute; }
};
template <>
struct SObjectRefTypeTraits<TDataStrPtr>
{
    ObjectReferenceType::Enum getType() { return ObjectReferenceType::Relative; }
};

struct SObjectRefUnionTraits
{
    typedef ObjectReferenceType::Enum TIdType;
    enum {
        TBufferSize = sizeof(SSizet4),
    };

    static ObjectReferenceType::Enum getNoDataId() { return ObjectReferenceType::Unknown; }

    template <typename TDataType>
    static TIdType getType()
    {
        return SObjectRefTypeTraits<TDataType>().getType();
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case ObjectReferenceType::Absolute:
            return inVisitor(*NVUnionCast<SLong4 *>(inData));
        case ObjectReferenceType::Relative:
            return inVisitor(*NVUnionCast<TDataStrPtr *>(inData));
        default:
            QT3DS_ASSERT(false);
        case ObjectReferenceType::Unknown:
            return inVisitor();
        }
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case ObjectReferenceType::Absolute:
            return inVisitor(*NVUnionCast<const SLong4 *>(inData));
        case ObjectReferenceType::Relative:
            return inVisitor(*NVUnionCast<const TDataStrPtr *>(inData));
        default:
            QT3DS_ASSERT(false);
        case ObjectReferenceType::Unknown:
            return inVisitor();
        }
    }
};

typedef qt3ds::foundation::
    DiscriminatedUnion<qt3ds::foundation::
                           DiscriminatedUnionGenericBase<SObjectRefUnionTraits,
                                                         SObjectRefUnionTraits::TBufferSize>,
                       SObjectRefUnionTraits::TBufferSize>
        TObjRefUnionType;

template <typename TDataType>
struct Qt3DSDMGetter
{
};

template <typename TRetType, typename TDataType>
TRetType get(const TDataType &inType)
{
    return Qt3DSDMGetter<TDataType>().template doGet<TRetType>(inType);
}

template <>
struct Qt3DSDMGetter<TObjRefUnionType>
{
    template <typename TRetType>
    TRetType doGet(const TObjRefUnionType &inValue)
    {
        return inValue.getData<TRetType>();
    }
};

// Either an absolute reference (SLong4) or a relative reference (string)
struct SObjectRefType
{
    TObjRefUnionType m_Value;

    SObjectRefType(const TDataStrPtr &inValue)
        : m_Value(inValue)
    {
    }
    SObjectRefType(const SLong4 &inValue)
        : m_Value(inValue)
    {
    }
    SObjectRefType(const SObjectRefType &inOther)
        : m_Value(inOther.m_Value)
    {
    }
    SObjectRefType() {}
    SObjectRefType &operator=(const SObjectRefType &inOther)
    {
        m_Value = inOther.m_Value;
        return *this;
    }
    ObjectReferenceType::Enum GetReferenceType() const { return m_Value.getType(); }
    bool operator==(const SObjectRefType &inOther) const { return m_Value == inOther.m_Value; }
};

inline bool Equals(const SObjectRefType &lhs, const SObjectRefType &rhs)
{
    return lhs == rhs;
}

struct SStringOrIntTypes
{
    enum Enum {
        Unknown = 0,
        Int,
        String,
    };
};

template <typename TDataType>
struct SStringOrIntTypeTraits
{
};

template <>
struct SStringOrIntTypeTraits<long>
{
    SStringOrIntTypes::Enum getType() { return SStringOrIntTypes::Int; }
};
template <>
struct SStringOrIntTypeTraits<int>
{
    SStringOrIntTypes::Enum getType() { return SStringOrIntTypes::Int; }
};
template <>
struct SStringOrIntTypeTraits<TDataStrPtr>
{
    SStringOrIntTypes::Enum getType() { return SStringOrIntTypes::String; }
};

struct SStringOrIntUnionTraits
{
    typedef SStringOrIntTypes::Enum TIdType;
    enum {
        TBufferSize = sizeof(TDataStrPtr),
    };

    static SStringOrIntTypes::Enum getNoDataId() { return SStringOrIntTypes::Unknown; }

    template <typename TDataType>
    static TIdType getType()
    {
        return SStringOrIntTypeTraits<TDataType>().getType();
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case SStringOrIntTypes::Int:
            return inVisitor(*NVUnionCast<long *>(inData));
        case SStringOrIntTypes::String:
            return inVisitor(*NVUnionCast<TDataStrPtr *>(inData));
        default:
            QT3DS_ASSERT(false);
        case SStringOrIntTypes::Unknown:
            return inVisitor();
        }
    }

    template <typename TRetType, typename TVisitorType>
    static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
    {
        switch (inType) {
        case SStringOrIntTypes::Int:
            return inVisitor(*NVUnionCast<const qt3ds::QT3DSI32 *>(inData));
        case SStringOrIntTypes::String:
            return inVisitor(*NVUnionCast<const TDataStrPtr *>(inData));
        default:
            QT3DS_ASSERT(false);
        case SStringOrIntTypes::Unknown:
            return inVisitor();
        }
    }
};

typedef qt3ds::foundation::
    DiscriminatedUnion<qt3ds::foundation::
                           DiscriminatedUnionGenericBase<SStringOrIntUnionTraits,
                                                         SStringOrIntUnionTraits::TBufferSize>,
                       SStringOrIntUnionTraits::TBufferSize>
        TStringOrIntUnionType;

template <>
struct Qt3DSDMGetter<TStringOrIntUnionType>
{
    template <typename TRetType>
    TRetType doGet(const TStringOrIntUnionType &inValue)
    {
        return inValue.getData<TRetType>();
    }
};

struct SStringOrInt
{
    TStringOrIntUnionType m_Value;

    SStringOrInt() {}
    SStringOrInt(int inValue)
        : m_Value(inValue)
    {
    }
    SStringOrInt(TDataStrPtr inValue)
        : m_Value(inValue)
    {
    }
    SStringOrInt(const SStringOrInt &inOther)
        : m_Value(inOther.m_Value)
    {
    }

    SStringOrInt &operator=(const SStringOrInt &inOther)
    {
        m_Value = inOther.m_Value;
        return *this;
    }

    SStringOrIntTypes::Enum GetType() const { return m_Value.getType(); }

    bool operator==(const SStringOrInt &inOther) const { return m_Value == inOther.m_Value; }
};

inline bool Equals(const SStringOrInt &lhs, const SStringOrInt &rhs)
{
    return lhs == rhs;
}

//comes from broken X.h
#ifdef None
#undef None
#endif

#ifdef Bool
#undef Bool
#endif

struct DataModelDataType {

  enum Value {
      None,
      Float,
      Float2,
      Float3,
      Float4,
      Long,
      String,
      Bool,
      Long4,
      StringRef,
      ObjectRef,
      StringOrInt,
      FloatList,
      RangedNumber // datainput-specific type for editor only
  };


  Q_ENUM(Value)
  Q_GADGET
};


class IStringTable;

template <typename TDataType>
struct Qt3DSDMValueTyper
{
};

template <typename TDataType>
inline DataModelDataType::Value GetValueType(const TDataType &inValue)
{
    return Qt3DSDMValueTyper<TDataType>().Get(inValue);
}

template <typename TDataType>
struct SDefaulter
{
};

template <typename TDataType>
inline bool SetDefault(DataModelDataType::Value inDataType, TDataType &outValue)
{
    return SDefaulter<TDataType>().SetDefault(inDataType, outValue);
}

typedef void (*Qt3DSDMDebugLogFunction)(const char *message);
// UICDMSimpleDataCore.cpp
extern Qt3DSDMDebugLogFunction g_DataModelDebugLogger;

#define QT3DSDM_DEBUG_LOG(msg)                                                                       \
    {                                                                                              \
        if (g_DataModelDebugLogger) {                                                                  \
            g_DataModelDebugLogger(msg);                                                               \
        }                                                                                          \
    }
struct Qt3DSDMLogScope
{
    const char *m_EndMessage;
    Qt3DSDMLogScope(const char *inStartMessage, const char *inEndMessage)
        : m_EndMessage(inEndMessage)
    {
        QT3DSDM_DEBUG_LOG(inStartMessage);
    }
    ~Qt3DSDMLogScope() { QT3DSDM_DEBUG_LOG(m_EndMessage); }
};

#define QT3DSDM_LOG_FUNCTION(fnname) Qt3DSDMLogScope __fn_scope(fnname " Enter", fnname " Leave");
}

#endif
