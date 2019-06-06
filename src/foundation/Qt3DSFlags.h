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

#ifndef QT3DS_FOUNDATION_QT3DS_FLAGS_H
#define QT3DS_FOUNDATION_QT3DS_FLAGS_H

/** \addtogroup foundation
  @{
*/

#include "foundation/Qt3DS.h"

#ifndef QT3DS_DOXYGEN
namespace qt3ds {
#endif
/**
\brief Container for bitfield flag variables associated with a specific enum type.

This allows for type safe manipulation for bitfields.

<h3>Example</h3>
        // enum that defines each bit...
        struct MyEnum
        {
                enum Enum
                {
                        eMAN  = 1,
                        eBEAR = 2,
                        ePIG  = 4,
                };
        };

        // implements some convenient global operators.
        QT3DS_FLAGS_OPERATORS(MyEnum::Enum, QT3DSU8);

        NVFlags<MyEnum::Enum, QT3DSU8> myFlags;
        myFlags |= MyEnum::eMAN;
        myFlags |= MyEnum::eBEAR | MyEnum::ePIG;
        if(myFlags & MyEnum::eBEAR)
        {
                doSomething();
        }
*/
template <typename enumtype, typename storagetype = QT3DSU32>
class NVFlags
{
public:
    QT3DS_INLINE explicit NVFlags(const NVEmpty &) {}
    QT3DS_INLINE NVFlags(void);
    QT3DS_INLINE NVFlags(enumtype e);
    QT3DS_INLINE NVFlags(const NVFlags<enumtype, storagetype> &f);
    QT3DS_INLINE explicit NVFlags(storagetype b);

    QT3DS_INLINE bool operator==(enumtype e) const;
    QT3DS_INLINE bool operator==(const NVFlags<enumtype, storagetype> &f) const;
    QT3DS_INLINE bool operator==(bool b) const;
    QT3DS_INLINE bool operator!=(enumtype e) const;
    QT3DS_INLINE bool operator!=(const NVFlags<enumtype, storagetype> &f) const;

    QT3DS_INLINE NVFlags<enumtype, storagetype> &operator=(enumtype e);

    QT3DS_INLINE NVFlags<enumtype, storagetype> &operator|=(enumtype e);
    QT3DS_INLINE NVFlags<enumtype, storagetype> &operator|=(const NVFlags<enumtype, storagetype> &f);
    QT3DS_INLINE NVFlags<enumtype, storagetype> operator|(enumtype e) const;
    QT3DS_INLINE NVFlags<enumtype, storagetype>
    operator|(const NVFlags<enumtype, storagetype> &f) const;

    QT3DS_INLINE NVFlags<enumtype, storagetype> &operator&=(enumtype e);
    QT3DS_INLINE NVFlags<enumtype, storagetype> &operator&=(const NVFlags<enumtype, storagetype> &f);
    QT3DS_INLINE NVFlags<enumtype, storagetype> operator&(enumtype e) const;
    QT3DS_INLINE NVFlags<enumtype, storagetype>
    operator&(const NVFlags<enumtype, storagetype> &f) const;

    QT3DS_INLINE NVFlags<enumtype, storagetype> &operator^=(enumtype e);
    QT3DS_INLINE NVFlags<enumtype, storagetype> &operator^=(const NVFlags<enumtype, storagetype> &f);
    QT3DS_INLINE NVFlags<enumtype, storagetype> operator^(enumtype e) const;
    QT3DS_INLINE NVFlags<enumtype, storagetype>
    operator^(const NVFlags<enumtype, storagetype> &f) const;

    QT3DS_INLINE NVFlags<enumtype, storagetype> operator~(void)const;

    QT3DS_INLINE operator bool(void) const;
    QT3DS_INLINE operator QT3DSU8(void) const;
    QT3DS_INLINE operator QT3DSU16(void) const;
    QT3DS_INLINE operator QT3DSU32(void) const;

    QT3DS_INLINE void clear(enumtype e);

    QT3DS_INLINE void clearOrSet(bool value, enumtype enumVal);

public:
    friend QT3DS_INLINE NVFlags<enumtype, storagetype> operator&(enumtype a,
                                                                 NVFlags<enumtype, storagetype> &b)
    {
        NVFlags<enumtype, storagetype> out;
        out.mBits = a & b.mBits;
        return out;
    }

private:
    storagetype mBits;
};

#define QT3DS_FLAGS_OPERATORS(enumtype, storagetype)                                                  \
    QT3DS_INLINE NVFlags<enumtype, storagetype> operator|(enumtype a, enumtype b)                     \
{                                                                                              \
    NVFlags<enumtype, storagetype> r(a);                                                       \
    r |= b;                                                                                    \
    return r;                                                                                  \
}                                                                                              \
    QT3DS_INLINE NVFlags<enumtype, storagetype> operator&(enumtype a, enumtype b)                     \
{                                                                                              \
    NVFlags<enumtype, storagetype> r(a);                                                       \
    r &= b;                                                                                    \
    return r;                                                                                  \
}                                                                                              \
    QT3DS_INLINE NVFlags<enumtype, storagetype> operator~(enumtype a)                                 \
{                                                                                              \
    return ~NVFlags<enumtype, storagetype>(a);                                                 \
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype>::NVFlags(void)
{
    mBits = 0;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype>::NVFlags(enumtype e)
{
    mBits = static_cast<storagetype>(e);
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype>::NVFlags(const NVFlags<enumtype, storagetype> &f)
{
    mBits = f.mBits;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype>::NVFlags(storagetype b)
{
    mBits = b;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE bool NVFlags<enumtype, storagetype>::operator==(enumtype e) const
{
    return mBits == static_cast<storagetype>(e);
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE bool NVFlags<enumtype, storagetype>::
operator==(const NVFlags<enumtype, storagetype> &f) const
{
    return mBits == f.mBits;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE bool NVFlags<enumtype, storagetype>::operator==(bool b) const
{
    return ((bool)*this) == b;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE bool NVFlags<enumtype, storagetype>::operator!=(enumtype e) const
{
    return mBits != static_cast<storagetype>(e);
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE bool NVFlags<enumtype, storagetype>::
operator!=(const NVFlags<enumtype, storagetype> &f) const
{
    return mBits != f.mBits;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> &NVFlags<enumtype, storagetype>::operator=(enumtype e)
{
    mBits = static_cast<storagetype>(e);
    return *this;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> &NVFlags<enumtype, storagetype>::operator|=(enumtype e)
{
    mBits |= static_cast<storagetype>(e);
    return *this;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> &NVFlags<enumtype, storagetype>::
operator|=(const NVFlags<enumtype, storagetype> &f)
{
    mBits |= f.mBits;
    return *this;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> NVFlags<enumtype, storagetype>::operator|(enumtype e) const
{
    NVFlags<enumtype, storagetype> out(*this);
    out |= e;
    return out;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> NVFlags<enumtype, storagetype>::
operator|(const NVFlags<enumtype, storagetype> &f) const
{
    NVFlags<enumtype, storagetype> out(*this);
    out |= f;
    return out;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> &NVFlags<enumtype, storagetype>::operator&=(enumtype e)
{
    mBits &= static_cast<storagetype>(e);
    return *this;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> &NVFlags<enumtype, storagetype>::
operator&=(const NVFlags<enumtype, storagetype> &f)
{
    mBits &= f.mBits;
    return *this;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> NVFlags<enumtype, storagetype>::operator&(enumtype e) const
{
    NVFlags<enumtype, storagetype> out = *this;
    out.mBits &= static_cast<storagetype>(e);
    return out;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> NVFlags<enumtype, storagetype>::
operator&(const NVFlags<enumtype, storagetype> &f) const
{
    NVFlags<enumtype, storagetype> out = *this;
    out.mBits &= f.mBits;
    return out;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> &NVFlags<enumtype, storagetype>::operator^=(enumtype e)
{
    mBits ^= static_cast<storagetype>(e);
    return *this;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> &NVFlags<enumtype, storagetype>::
operator^=(const NVFlags<enumtype, storagetype> &f)
{
    mBits ^= f.mBits;
    return *this;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> NVFlags<enumtype, storagetype>::operator^(enumtype e) const
{
    NVFlags<enumtype, storagetype> out = *this;
    out.mBits ^= static_cast<storagetype>(e);
    return out;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> NVFlags<enumtype, storagetype>::
operator^(const NVFlags<enumtype, storagetype> &f) const
{
    NVFlags<enumtype, storagetype> out = *this;
    out.mBits ^= f.mBits;
    return out;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype> NVFlags<enumtype, storagetype>::operator~(void)const
{
    NVFlags<enumtype, storagetype> out;
    out.mBits = ~mBits;
    return out;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype>::operator bool(void) const
{
    return mBits ? true : false;
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype>::operator QT3DSU8(void) const
{
    return static_cast<QT3DSU8>(mBits);
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype>::operator QT3DSU16(void) const
{
    return static_cast<QT3DSU16>(mBits);
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE NVFlags<enumtype, storagetype>::operator QT3DSU32(void) const
{
    return static_cast<QT3DSU32>(mBits);
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE void NVFlags<enumtype, storagetype>::clear(enumtype e)
{
    mBits &= ~static_cast<storagetype>(e);
}

template <typename enumtype, typename storagetype>
QT3DS_INLINE void NVFlags<enumtype, storagetype>::clearOrSet(bool value, enumtype enumVal)
{
    if (value)
        this->operator|=(enumVal);
    else
        clear(enumVal);
}

#ifndef QT3DS_DOXYGEN
} // namespace qt3ds
#endif

/** @} */
#endif // #ifndef QT3DS_FOUNDATION_QT3DS_FLAGS_H
