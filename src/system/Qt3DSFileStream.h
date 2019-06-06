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
#include "Qt3DSIFileStream.h"
#include "Qt3DSArray.h"
#include "Qt3DSEndian.h"

#include <QtCore/QString>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	File stream base class.
 */
class CFileStream : public IFileStream
{
    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CFileStream(const CHAR *inFilePath = NULL, const CHAR *inMode = "rb",
                BOOL inKeepTrying = false);
    virtual ~CFileStream();

public: // Serializing
    INT32 ReadRawCopy(void *inDestination, const UINT32 inByteCount) override;
    INT32 ReadRaw(const void *&inPtr, const UINT32 inByteCount) override;
    INT32 WriteRaw(const void *inSource, const UINT32 inByteCount) override;

    // Seeking
    void Offset(const INT32 inOffset) override;
    TStreamPosition Current() override;
    void Set(const TStreamPosition &inPosition) override;

public: // File Operation
    void Open(const CHAR *inFilePath, const CHAR *inMode) override;
    void Close() override;
    BOOL IsOpen() const override;
    const CHAR *GetFilePath() const override;

public: // Endianess
    void SetEndian(const BOOL inEndian);
    BOOL GetEndian();

    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    TFile *m_FileStream;
    QString m_FilePath;
    CArray<UINT8> m_TempBuffer;
    BOOL m_Endian;
    BOOL m_KeepTrying;
    CHAR m_Unused[2]; ///< (padding)
};

} // namespace Q3DStudio
