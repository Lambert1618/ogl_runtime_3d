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
#include "Qt3DSConfig.h"
#include <qstring.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Utility class transforming a string into a hash value.
 */
class CHash
{
    //==============================================================================
    //	Methods
    //==============================================================================
public: // Static utility
    //==============================================================================
    /**
     *	We use the hashing algorithm called "sdbm".
     *
     *	This algorithm was created from sdbm (a public-domain reimplementation
     *	of ndbm) database library. It was found to do well in scrambling bits,
     *	causing better distribution of the keys and fewer splits. it also happens
     *	to be a good general hashing function with good distribution. the actual
     *	function is hash(i) = hash(i - 1) * 65599 + str[i]; what is included below
     *	is the faster version used in gawk. [there is even a faster, duff-device
     *	version] the magic constant 65599 was picked out of thin air while
     *	experimenting with different constants, and turns out to be a prime. this
     *	is one of the algorithms used in berkeley db (see sleepycat) and elsewhere.
     *
     *	@param inString		the string to hash
     *	@return the hash value
     */
    static TStringHash HashString(const CHAR *inString)
    {
        TStringHash theHash = 0;
        INT32 theChar = *inString;
        INT32 theCount = 0;
        while (theChar && (theCount < HASH_LIMIT)) {
            theChar = *inString++;
            theHash = theChar + (theHash << 6) + (theHash << 16) - theHash;
            ++theCount;
        }

        return theHash;
    }

    static TStringHash HashString(const QString &inString)
    {
        TStringHash theHash = 0;
        INT32 theCount = 0;
        for (auto theChar : inString) {
            if (theCount == HASH_LIMIT)
                break;
            theHash = theChar.toLatin1() + (theHash << 6) + (theHash << 16) - theHash;
            ++theCount;
        }
        theHash = (theHash << 6) + (theHash << 16) - theHash;

        return theHash;
    }

    static TStringHash HashString(const QString &inString, int begin, int end)
    {
        TStringHash theHash = 0;
        INT32 theCount = 0;
        for (int i = begin; i < end; ++i) {
            if (theCount == HASH_LIMIT)
                break;
            theHash = inString.at(i).toLatin1() + (theHash << 6) + (theHash << 16) - theHash;
            ++theCount;
        }
        theHash = (theHash << 6) + (theHash << 16) - theHash;

        return theHash;
    }


    //==============================================================================
    /**
     *	31-bit hash with MSB set to 0
     *
     *	@param inString		the string to hash
     *	@return the 31-bit hash value
     */
    static TEventCommandHash HashEventCommand(const CHAR *inString)
    {
        return HashString(inString) & 0x7fffffff;
    }

    //==============================================================================
    /**
     *	26-bit hash with MSBs all set to 0
     *
     *	@param inString		the string to hash
     *	@return the 26-bit hash value
     */
    static TAttributeHash HashAttribute(const CHAR *inString)
    {
        return HashString(inString) & 0x03ffffff;
    }

    static TAttributeHash HashAttribute(const QString &inString)
    {
        return HashString(inString) & 0x03ffffff;
    }
};

} // namespace Q3DStudio
