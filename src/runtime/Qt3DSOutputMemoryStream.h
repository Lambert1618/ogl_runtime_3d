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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSHash.h"
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4820) // X bytes padding added after data member
#pragma warning(                                                                                   \
    disable : 4061) // enumerator X in switch of enum Y is not explicitly handled by a case label
#pragma warning(disable : 4062) // enumerator X in switch of enum Y is not handled
#pragma warning(disable : 4548) // xlocale warnings
#pragma warning(                                                                                   \
    disable : 4738) // storing 32-bit float result in memory, possible loss of performance
#endif
#include <vector>
#include <map>
#include <set>
#include <string>

#ifdef _WIN32
#pragma warning(pop)
#endif

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Class
//==============================================================================
class COutputMemoryStream
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    std::vector<UINT8> m_Data;

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    COutputMemoryStream();
    virtual ~COutputMemoryStream();

public: // Access
    void Reset();
    UINT32 GetSize() const;
    const UINT8 *GetData() const;

public: // Output
    void WriteStringHash(const CHAR *inItem);
    void WriteEventCommandHash(const CHAR *inItem);

public: // Operators
    COutputMemoryStream &operator<<(const COutputMemoryStream &inStream);
    COutputMemoryStream &operator<<(const std::string &inString);
    COutputMemoryStream &operator<<(const EAttributeType inType);

    //==============================================================================
    //	Template Methods
    //==============================================================================
public:
    template <typename T>
    COutputMemoryStream &operator<<(const T &inItem)
    {
        size_t theSize = sizeof(T);
        const UINT8 *thePtr = reinterpret_cast<const UINT8 *>(&inItem);
        for (size_t theCounter = 0; theCounter < theSize; ++theCounter) {
            m_Data.push_back(*thePtr);
            ++thePtr;
        }
        return *this;
    }

    template <typename T>
    void SetData(const T &inItem, UINT32 inByteOffset)
    {
        UINT8 *theSourcePtr = &m_Data[inByteOffset];
        size_t theSize = sizeof(T);
        const UINT8 *thePtr = reinterpret_cast<const UINT8 *>(&inItem);
        for (size_t theCounter = 0; theCounter < theSize; ++theCounter) {
            *theSourcePtr = *thePtr;
            ++thePtr;
            ++theSourcePtr;
        }
    }
};

} // namespace Q3DStudio
