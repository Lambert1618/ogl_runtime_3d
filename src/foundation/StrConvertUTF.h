/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#ifndef QT3DS_RENDER_STR_CONVERT_UTF_H
#define QT3DS_RENDER_STR_CONVERT_UTF_H
#include <EASTL/string.h>
#include "foundation/ConvertUTF.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSSimpleTypes.h"

namespace qt3ds {
namespace foundation {

    using namespace qt3ds;
    template <typename TSrcType>
    struct ConverterType
    {
    };
    template <>
    struct ConverterType<char8_t>
    {
        typedef UTF8 TConverterType;
    };
    template <>
    struct ConverterType<char16_t>
    {
        typedef UTF16 TConverterType;
    };
    template <>
    struct ConverterType<char32_t>
    {
        typedef UTF32 TConverterType;
    };

    template <QT3DSU32 TWCharSize>
    struct WCharType
    {
    };

    template <>
    struct WCharType<2>
    {
        typedef char16_t TCharType;
    };

    template <>
    struct WCharType<4>
    {
        typedef char32_t TCharType;
    };

    typedef WCharType<sizeof(wchar_t)> TWCharEASTLConverter;

    template <typename TSrcType, typename TDestType>
    struct UTFConversionSelector
    {
    };

    template <>
    struct UTFConversionSelector<char8_t, char16_t>
    {
        static ConversionResult Convert(const UTF8 **sourceStart, const UTF8 *sourceEnd,
                                        UTF16 **targetStart, UTF16 *targetEnd,
                                        ConversionFlags flags)
        {
            return Q3DSConvertUTF8toUTF16(sourceStart, sourceEnd, targetStart, targetEnd, flags);
        }
    };

    template <>
    struct UTFConversionSelector<char8_t, char32_t>
    {
        static ConversionResult Convert(const UTF8 **sourceStart, const UTF8 *sourceEnd,
                                        UTF32 **targetStart, UTF32 *targetEnd,
                                        ConversionFlags flags)
        {
            return Q3DSConvertUTF8toUTF32(sourceStart, sourceEnd, targetStart, targetEnd, flags);
        }
    };
    template <>
    struct UTFConversionSelector<char16_t, char8_t>
    {
        static ConversionResult Convert(const UTF16 **sourceStart, const UTF16 *sourceEnd,
                                        UTF8 **targetStart, UTF8 *targetEnd, ConversionFlags flags)
        {
            return Q3DSConvertUTF16toUTF8(sourceStart, sourceEnd, targetStart, targetEnd, flags);
        }
    };
    template <>
    struct UTFConversionSelector<char16_t, char32_t>
    {
        static ConversionResult Convert(const UTF16 **sourceStart, const UTF16 *sourceEnd,
                                        UTF32 **targetStart, UTF32 *targetEnd,
                                        ConversionFlags flags)
        {
            return Q3DSConvertUTF16toUTF32(sourceStart, sourceEnd, targetStart, targetEnd, flags);
        }
    };
    template <>
    struct UTFConversionSelector<char32_t, char8_t>
    {
        static ConversionResult Convert(const UTF32 **sourceStart, const UTF32 *sourceEnd,
                                        UTF8 **targetStart, UTF8 *targetEnd, ConversionFlags flags)
        {
            return Q3DSConvertUTF32toUTF8(sourceStart, sourceEnd, targetStart, targetEnd, flags);
        }
    };
    template <>
    struct UTFConversionSelector<char32_t, char16_t>
    {
        static ConversionResult Convert(const UTF32 **sourceStart, const UTF32 *sourceEnd,
                                        UTF16 **targetStart, UTF16 *targetEnd,
                                        ConversionFlags flags)
        {
            return Q3DSConvertUTF32toUTF16(sourceStart, sourceEnd, targetStart, targetEnd, flags);
        }
    };

    // Convert into an EASTL string type.
    // inSrcLen may be zero in which case we analyze the string looking for the zero element.
    template <typename TSrcType, typename TDestType, typename TAllocType>
    bool ConvertUTF(const TSrcType *inSrc, size_t inSrcLen,
                    eastl::basic_string<TDestType, TAllocType> &outString)
    {
        typedef typename ConverterType<TDestType>::TConverterType TDestUTFType;
        typedef typename ConverterType<TSrcType>::TConverterType TSrcUTFType;
        if (inSrc == 0 || *inSrc == 0) {
            outString.clear();
            return true;
        }

        if (inSrcLen == 0) {
            // empty loop intentional.
            for (const TSrcType *ptr = inSrc; ptr && *ptr; ++ptr, ++inSrcLen) {
            }
        }

        typename eastl::basic_string<TDestType, TAllocType>::size_type capacity =
            outString.capacity();
        if (capacity == 0)
            outString.resize((QT3DSU32)inSrcLen * 2);
        else
            outString.resize(capacity);

        ConversionResult theConversionResult(conversionOK);
        TDestUTFType *writePtr = NULL;
        do {
            writePtr = reinterpret_cast<TDestUTFType *>(const_cast<TDestType *>(outString.data()));
            TDestUTFType *writeEnd(writePtr + outString.size());
            const TSrcUTFType *readPtr(reinterpret_cast<const TSrcUTFType *>(inSrc));
            const TSrcUTFType *readEnd = readPtr + inSrcLen;
            theConversionResult = UTFConversionSelector<TSrcType, TDestType>::Convert(
                &readPtr, readEnd, &writePtr, writeEnd, lenientConversion);
            if (theConversionResult == targetExhausted) {
                capacity = outString.capacity() * 2;
                outString.resize(capacity);
            }
        } while (theConversionResult == targetExhausted);

        if (theConversionResult == conversionOK) {
            TDestUTFType *writeStart =
                reinterpret_cast<TDestUTFType *>(const_cast<TDestType *>(outString.data()));
            outString.resize((QT3DSU32)(writePtr - writeStart));
            return true;
        } else {
            outString.clear();
            QT3DS_ASSERT(false);
            return false;
        }
    }

#ifdef WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T

    template <typename TDestType, typename TAllocType>
    bool ConvertWideUTF(const wchar_t *inSrc, size_t inSrcLen,
                        eastl::basic_string<TDestType, TAllocType> &outString)
    {
        return ConvertUTF((const TWCharEASTLConverter::TCharType *)inSrc, inSrcLen, outString);
    }

#endif
}
}

#endif
