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

#include "SystemPrefix.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSFile.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	OS level memory routines
//==============================================================================
#if defined(_XENONPLATFORM) || defined(_PCPLATFORM) || defined(_PS3PLATFORM)                       \
    || defined(_TEGRAPLATFORM) || defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
CFile::TOpen CFile::s_Open = &fopen;
CFile::TClose CFile::s_Close = &fclose;
CFile::TRead CFile::s_Read = &fread;
CFile::TWrite CFile::s_Write = &fwrite;
CFile::TTell CFile::s_Tell = &ftell;
CFile::TSeek CFile::s_Seek = &fseek;
#else
#error "A platform must be defined"
#endif

//==============================================================================
/**
 *	Overrides basic memory allocation/deallocation routines
 *	@param	inOpen	fopen replacement method
 *	@param	inClose	fclose replacement method
 *	@param	inRead	fread replacement method
 *	@param	inWrite	fwrite replacement method
 */
void CFile::SetFileFunctions(const TOpen inOpen, const TClose inClose, const TRead inRead,
                             const TWrite inWrite, const TTell inTell, const TSeek inSeek)
{
    s_Open = inOpen;
    s_Close = inClose;
    s_Read = inRead;
    s_Write = inWrite;
    s_Tell = inTell;
    s_Seek = inSeek;
}

} // namespace Q3DStudio
