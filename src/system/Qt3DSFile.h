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
#include "Qt3DSTypes.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Single overridable exit point for all low level file calls.
 */
class CFile
{
    //==============================================================================
    //	Typedefs
    //==============================================================================
public:
    typedef enum _E_SEEK {
        E_SEEK_SET = SEEK_SET,
        E_SEEK_CUR = SEEK_CUR,
        E_SEEK_END = SEEK_END
    } E_SEEK;

    typedef TFile *(*TOpen)(const char *inFileName, const char *inMode);
    typedef int (*TClose)(TFile *inFile);
    typedef TFileSize (*TRead)(void *inBuffer, TFileSize inSize, TFileSize inCount, TFile *inFile);
    typedef TFileSize (*TWrite)(const void *inBuffer, TFileSize inSize, TFileSize inCount,
                                TFile *inFile);
    typedef long (*TTell)(TFile *inFile);
    typedef int (*TSeek)(TFile *inFile, long inOffset, int inOrigin);

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    static TOpen s_Open; ///< function pointer to fopen operation
    static TClose s_Close; ///< function pointer to fclose operation
    static TRead s_Read; ///< function pointer to fread operation
    static TWrite s_Write; ///< function pointer to fwrite operation
    static TTell s_Tell; ///< function pointer to ftell operation
    static TSeek s_Seek; ///< function pointer to fseek operation

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Memory override
    static void SetFileFunctions(const TOpen inOpen, const TClose inClose, const TRead inRead,
                                 const TWrite inWrite, const TTell inTell, const TSeek inSeek);

public: // Function access
    static TOpen Open() { return s_Open; }
    static TClose Close() { return s_Close; }
    static TRead Read() { return s_Read; }
    static TWrite Write() { return s_Write; }
    static TTell Tell() { return s_Tell; }
    static TSeek Seek() { return s_Seek; }
};

} // namespace Q3DStudio
