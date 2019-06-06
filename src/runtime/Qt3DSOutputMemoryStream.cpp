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

#include "RuntimePrefix.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSOutputMemoryStream.h"
#include "Qt3DSHash.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	XXX
 */
COutputMemoryStream::COutputMemoryStream()
{
}

//==============================================================================
/**
 *	XXX
 */
COutputMemoryStream::~COutputMemoryStream()
{
}

//==============================================================================
/**
 *	XXX
 */
void COutputMemoryStream::Reset()
{
    m_Data.clear();
}

//==============================================================================
/**
 *	XXX
 */
COutputMemoryStream &COutputMemoryStream::operator<<(const COutputMemoryStream &inStream)
{
    size_t theSize = inStream.m_Data.size();
    for (size_t theCount = 0; theCount < theSize; ++theCount)
        m_Data.push_back(inStream.m_Data[theCount]);
    return *this;
}

//==============================================================================
/**
 *	XXX
 */
void COutputMemoryStream::WriteStringHash(const CHAR *inItem)
{
    TStringHash theHash = CHash::HashString(inItem);
    // DEBUG
    //{FILE*fp=fopen("hashed.txt","a");if(fp){fprintf(fp,"%ul %s\n",theHash,inItem);fclose(fp);}}
    operator<<(theHash);
}

//==============================================================================
/**
 *	XXX
 */
void COutputMemoryStream::WriteEventCommandHash(const CHAR *inItem)
{
    TStringHash theHash = CHash::HashEventCommand(inItem);
    // DEBUG
    //{FILE*fp=fopen("hashed.txt","a");if(fp){fprintf(fp,"%ul %s\n",theHash,inItem);fclose(fp);}}
    operator<<(theHash);
}

//==============================================================================
/**
 *	XXX
 */
COutputMemoryStream &COutputMemoryStream::operator<<(const std::string &inString)
{
    const UINT8 *thePtr = reinterpret_cast<const UINT8 *>(inString.c_str());
    for (size_t theCounter = 0; theCounter < inString.size(); ++theCounter) {
        m_Data.push_back(*thePtr);
        ++thePtr;
    }

    return *this;
}

//==============================================================================
/**
 *	XXX
 */
COutputMemoryStream &COutputMemoryStream::operator<<(const EAttributeType inType)
{
    UINT8 theType = static_cast<UINT8>(inType);
    m_Data.push_back(theType);

    return *this;
}

//==============================================================================
/**
 *	XXX
 */
UINT32 COutputMemoryStream::GetSize() const
{
    return static_cast<UINT32>(m_Data.size());
}

//==============================================================================
/**
 *	XXX
 */
const UINT8 *COutputMemoryStream::GetData() const
{
    return &m_Data[0];
}

} // namespace Q3DStudio
