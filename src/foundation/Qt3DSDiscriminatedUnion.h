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

#ifndef QT3DS_FOUNDATION_DISCRIMINATED_UNION_H
#define QT3DS_FOUNDATION_DISCRIMINATED_UNION_H
#include "Qt3DSAssert.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/Qt3DSUnionCast.h"

#ifdef WIN32
#pragma warning(disable : 4100)
#endif

namespace qt3ds {
namespace foundation {

    template <typename TUnionTraits, int TBufferSize>
    class DiscriminatedUnion
    {
    public:
        typedef DiscriminatedUnion<TUnionTraits, TBufferSize> TThisType;
        typedef TUnionTraits TTraits;
        typedef typename TTraits::TIdType TIdType;

    protected:
        char m_Data[TBufferSize];
        // Id type must include a no-data type.
        TIdType m_DataType;

    public:
        DiscriminatedUnion() { TTraits::defaultConstruct(m_Data, m_DataType); }

        DiscriminatedUnion(const TThisType &inOther)
            : m_DataType(inOther.m_DataType)
        {
            TTraits::copyConstruct(m_Data, inOther.m_Data, m_DataType);
        }

        template <typename TDataType>
        DiscriminatedUnion(const TDataType &inType)
        {
            TTraits::copyConstruct(m_Data, m_DataType, inType);
        }

        ~DiscriminatedUnion() { TTraits::destruct(m_Data, m_DataType); }

        TThisType &operator=(const TThisType &inType)
        {
            if (this != &inType) {
                TTraits::destruct(m_Data, m_DataType);
                m_DataType = inType.m_DataType;
                TTraits::copyConstruct(m_Data, inType.m_Data, inType.m_DataType);
            }
            return *this;
        }

        typename TTraits::TIdType getType() const { return m_DataType; }

        template <typename TDataType>
        const TDataType *getDataPtr() const
        {
            return TTraits::template getDataPtr<TDataType>(m_Data, m_DataType);
        }

        template <typename TDataType>
        TDataType *getDataPtr()
        {
            return TTraits::template getDataPtr<TDataType>(m_Data, m_DataType);
        }

        template <typename TDataType>
        TDataType getData() const
        {
            const TDataType *dataPtr = getDataPtr<TDataType>();
            if (dataPtr)
                return *dataPtr;
            QT3DS_ASSERT(false);
            return TDataType();
        }

        bool operator==(const TThisType &inOther) const
        {
            return m_DataType == inOther.m_DataType
                && TTraits::areEqual(m_Data, inOther.m_Data, m_DataType);
        }

        bool operator!=(const TThisType &inOther) const
        {
            return m_DataType != inOther.m_DataType
                || TTraits::areEqual(m_Data, inOther.m_Data, m_DataType) == false;
        }

        template <typename TRetType, typename TVisitorType>
        TRetType visit(TVisitorType inVisitor)
        {
            return TTraits::template visit<TRetType>(m_Data, m_DataType, inVisitor);
        }

        template <typename TRetType, typename TVisitorType>
        TRetType visit(TVisitorType inVisitor) const
        {
            return TTraits::template visit<TRetType>(m_Data, m_DataType, inVisitor);
        }
    };

    // Helper system to enable quicker and correct construction of union traits types

    struct CopyConstructVisitor
    {
        const char *m_Src;
        CopyConstructVisitor(const char *inSrc)
            : m_Src(inSrc)
        {
        }

        template <typename TDataType>
        void operator()(TDataType &inDst)
        {
            new (&inDst) TDataType(*NVUnionCast<const TDataType *>(m_Src));
        }
        void operator()() { QT3DS_ASSERT(false); }
    };

    template <typename TDataType>
    struct DestructTraits
    {
        void destruct(TDataType &inType) { inType.~TDataType(); }
    };

    // Until compilers improve a bit, you need this for POD types else you get
    // unused parameter warnings.
    template <>
    struct DestructTraits<QT3DSU8>
    {
        void destruct(QT3DSU8 &) {}
    };
    template <>
    struct DestructTraits<QT3DSI8>
    {
        void destruct(QT3DSI8 &) {}
    };
    template <>
    struct DestructTraits<QT3DSU16>
    {
        void destruct(QT3DSU16 &) {}
    };
    template <>
    struct DestructTraits<QT3DSI16>
    {
        void destruct(QT3DSI16 &) {}
    };
    template <>
    struct DestructTraits<QT3DSU32>
    {
        void destruct(QT3DSU32 &) {}
    };
    template <>
    struct DestructTraits<QT3DSI32>
    {
        void destruct(QT3DSI32 &) {}
    };
    template <>
    struct DestructTraits<QT3DSU64>
    {
        void destruct(QT3DSU64 &) {}
    };
    template <>
    struct DestructTraits<QT3DSI64>
    {
        void destruct(QT3DSI64 &) {}
    };
    template <>
    struct DestructTraits<QT3DSF32>
    {
        void destruct(QT3DSF32 &) {}
    };
    template <>
    struct DestructTraits<QT3DSF64>
    {
        void destruct(QT3DSF64 &) {}
    };
    template <>
    struct DestructTraits<bool>
    {
        void destruct(bool &) {}
    };
    template <>
    struct DestructTraits<void *>
    {
        void destruct(void *&) {}
    };
#ifdef __INTEGRITY
    template <>
    struct DestructTraits<QT3DSVec2>
    {
        void destruct(QT3DSVec2 &) {}
    };
    template <>
    struct DestructTraits<QT3DSVec3>
    {
        void destruct(QT3DSVec3 &) {}
    };
#endif

    struct DestructVisitor
    {
        template <typename TDataType>
        void operator()(TDataType &inDst)
        {
            DestructTraits<TDataType>().destruct(inDst);
        }
        void operator()() { QT3DS_ASSERT(false); }
    };

    template <typename TDataType>
    struct EqualVisitorTraits
    {
        bool operator()(const TDataType &lhs, const TDataType &rhs) { return lhs == rhs; }
    };

    struct EqualVisitor
    {
        const char *m_Rhs;
        EqualVisitor(const char *rhs)
            : m_Rhs(rhs)
        {
        }
        template <typename TDataType>
        bool operator()(const TDataType &lhs)
        {
            const TDataType &rhs(*NVUnionCast<const TDataType *>(m_Rhs));
            return EqualVisitorTraits<TDataType>()(lhs, rhs);
        }
        bool operator()()
        {
            QT3DS_ASSERT(false);
            return true;
        }
    };

    template <typename TBase, QT3DSU32 TBufferSize>
    struct DiscriminatedUnionGenericBase : public TBase
    {
        typedef typename TBase::TIdType TIdType;

        static void zeroBuf(char *outDst) { qt3ds::intrinsics::memZero(outDst, TBufferSize); }

        static void defaultConstruct(char *outDst, TIdType &outType)
        {
            zeroBuf(outDst);
            outType = TBase::getNoDataId();
        }

        template <typename TDataType>
        static void copyConstruct(char *outDst, TIdType &outType, const TDataType &inSrc)
        {
            zeroBuf(outDst);
            StaticAssert<sizeof(TDataType) <= TBufferSize>::valid_expression();
            outType = TBase::template getType<TDataType>();
            new (outDst) TDataType(inSrc);
        }

        static void copyConstruct(char *inDst, const char *inSrc, TIdType inType)
        {
            if (inType == TBase::getNoDataId())
                zeroBuf(inDst);
            else
                TBase::template visit<void>(inDst, inType, CopyConstructVisitor(inSrc));
        }

        static void destruct(char *inDst, TIdType inType)
        {
            if (inType != TBase::getNoDataId())
                TBase::template visit<void>(inDst, inType, DestructVisitor());
            zeroBuf(inDst);
        }

        template <typename TDataType>
        static const TDataType *getDataPtr(const char *inData, const TIdType &inType)
        {
            if (TBase::template getType<TDataType>() == inType)
                return NVUnionCast<const TDataType *>(inData);
            QT3DS_ASSERT(false);
            return NULL;
        }

        template <typename TDataType>
        static TDataType *getDataPtr(char *inData, const TIdType &inType)
        {
            if (TBase::template getType<TDataType>() == inType)
                return NVUnionCast<TDataType *>(inData);
            QT3DS_ASSERT(false);
            return NULL;
        }

        static bool areEqual(const char *inLhs, const char *inRhs, TIdType inType)
        {
            if (inType != TBase::getNoDataId())
                return TBase::template visit<bool>(inLhs, inType, EqualVisitor(inRhs));
            else
                return true;
        }
    };
}
}

#endif
