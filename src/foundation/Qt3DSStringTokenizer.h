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

#ifndef QT3DS_FOUNDATION_STRING_TOKENIZER_H
#define QT3DS_FOUNDATION_STRING_TOKENIZER_H

#include "EASTL/string.h"

namespace qt3ds {
namespace foundation {

    template <typename TStrType>
    class QT3DS_FOUNDATION_API StringTokenizer
    {
        typedef typename eastl::basic_string<TStrType>::size_type size_t;

    public:
        StringTokenizer(const eastl::basic_string<TStrType> &inString,
                        const eastl::basic_string<TStrType> &inToken)
        {
            eastl::basic_string<TStrType> mStr = inString;
            eastl::basic_string<TStrType> theTempString = inString.substr(0, inToken.length());

            while (inToken.length() && theTempString.compare(inToken) == 0) {
                mStr = mStr.substr(inToken.length());
                theTempString = mStr.substr(0, inToken.length());
            }

            theTempString = mStr.substr(mStr.length() - 1 - inToken.length());
            while (theTempString.compare(inToken) == 0) {
                mStr = mStr.substr(0, mStr.length() - inToken.length());
                theTempString = mStr.substr(mStr.length() - 1 - inToken.length());
            }

            m_OriginalString = mStr;
            m_Token = inToken;
            m_Index = 0;
        }

        eastl::basic_string<TStrType> GetCurrentPartition()
        {
            eastl::basic_string<TStrType> theReturnString;
            if (m_Index != eastl::basic_string<TStrType>::npos) {
                size_t theCurrentTokenIndex = m_OriginalString.find(m_Token, m_Index);
                if (theCurrentTokenIndex == eastl::basic_string<TStrType>::npos) {
                    theReturnString = m_OriginalString.substr(m_Index);
                } else {
                    theReturnString =
                        m_OriginalString.substr(m_Index, theCurrentTokenIndex - m_Index);
                }
            }

            return theReturnString;
        }

        bool HasNextPartition() { return m_Index != eastl::basic_string<TStrType>::npos; }

        void operator++()
        {
            if (m_Index != eastl::basic_string<TStrType>::npos) {
                size_t theCurrentTokenIndex = m_OriginalString.find(m_Token, m_Index);
                if (theCurrentTokenIndex == eastl::basic_string<TStrType>::npos) {
                    m_Index = eastl::basic_string<TStrType>::npos;
                } else {
                    m_Index = theCurrentTokenIndex + m_Token.length();
                    if (m_Index > m_OriginalString.length())
                        m_Index = eastl::basic_string<TStrType>::npos;
                }
            }
        }

    private:
        size_t m_Index;
        eastl::basic_string<TStrType> m_Token;
        eastl::basic_string<TStrType> m_OriginalString;
    };

} // namespace foundation
} // namespace qt3ds

#endif
