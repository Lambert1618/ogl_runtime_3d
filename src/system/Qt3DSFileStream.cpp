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

//==============================================================================
// Includes
//==============================================================================
#include "SystemPrefix.h"
#include "Qt3DSFile.h"
#include "Qt3DSFileStream.h"
#include "foundation/Qt3DSLogging.h"

#include <QtCore/qthread.h>

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 *	@param inFilename	the file to open
 *	@param inMode		the file mode to use
 */
CFileStream::CFileStream(const CHAR *inFilePath, const CHAR *inMode, BOOL inKeepTrying)
    : m_FileStream(NULL)
    , m_Endian(QT3DS_LITTLE_ENDIAN)
    , m_KeepTrying(inKeepTrying)
{
    m_TempBuffer.Reserve(sizeof(MATRIX16)); // Preallocated some space
    if (inFilePath)
        Open(inFilePath, inMode);
}

//==============================================================================
/**
 *	Destructor
 */
CFileStream::~CFileStream()
{
    Close();
}

//==============================================================================
/**
 *	Open file.
 *	@param	inFilename	the file to open
 *	@param	inMode		the file mode ( read/write ) to use
 */
void CFileStream::Open(const CHAR *inFilePath, const CHAR *inMode)
{
    Close();
    m_FileStream = CFile::Open()(inFilePath, inMode);
    while (m_KeepTrying && !m_FileStream) {
        QThread::msleep(10);
        m_FileStream = CFile::Open()(inFilePath, inMode);
    }

    if (m_FileStream)
        m_FilePath = inFilePath;
}

//==============================================================================
/**
 *	Close file.
 */
void CFileStream::Close()
{
    if (m_FileStream) {
        CFile::Close()(m_FileStream);
        m_FileStream = NULL;
    }
}

//==============================================================================
/**
 *	Check if there is an opened file.
 *	@return TRUE if opened, FALSE otherwise.
 */
BOOL CFileStream::IsOpen() const
{
    return m_FileStream != NULL;
}

//==============================================================================
/**
 *	Access the source path of a file stream.
 *	@return an empty string if the stream is not open
 */
const CHAR *CFileStream::GetFilePath() const
{
    return m_FilePath.toLatin1().constData();
}

//==============================================================================
/**
 *	Read from stream into memory.
 *	@param	outMemory		the destination of the read
 *	@param	inByteCount		the amount to read
 *	@return the number of bytes actually read
 */
INT32 CFileStream::ReadRawCopy(void *inDestination, const UINT32 inByteCount)
{
    INT32 lastRead = 0;
    INT32 totalRead = 0;
    do {
        if (totalRead) {
            qCWarning(qt3ds::INVALID_OPERATION)
                    << "Failed to read expected amount, retrying, expected "
                    << inByteCount << " bytes, got "<< totalRead <<" bytes";
        }
        lastRead = static_cast<INT32>(CFile::Read()(((char *)inDestination) + totalRead, 1,
                                                    inByteCount - totalRead, m_FileStream));
        totalRead += lastRead;
    } while (lastRead && totalRead < (INT32)inByteCount);

    if (totalRead != (INT32)inByteCount) {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "Failed to read expected amount, retrying, expected "
                << inByteCount << " bytes, got "<< totalRead <<" bytes";

        // Zero out remaining destination buffer.
        // This can, in some cases, avoid a crash and allow the system to survive a partial
        // read.
        INT32 amountLeft = inByteCount - totalRead;
        memset(((char *)inDestination) + totalRead, 0, amountLeft);
    }
    return totalRead;
}

//==============================================================================
/**
 *	Reads data into internal buffer and return the data pointer for it.
 *	@param	outMemory		pointer to internal data buffer
 *	@param	inByteCount		the amount to read
 *	@return the number of bytes actually read
 */
INT32 CFileStream::ReadRaw(const void *&inPtr, const UINT32 inByteCount)
{
    m_TempBuffer.Reserve(static_cast<INT32>(inByteCount));
    INT32 theReadSize = ReadRawCopy(m_TempBuffer.Begin(), inByteCount);
    if (theReadSize != (INT32)inByteCount) {
        qCWarning(qt3ds::INVALID_OPERATION)
                << "Failed to read expected amount, retrying, expected "<< inByteCount
                << " bytes, got "<< theReadSize <<" bytes";
    }
    inPtr = m_TempBuffer.Begin();
    return theReadSize;
}

//==============================================================================
/**
 *	Write from memory to stream.
 *	@param	inMemory		the buffer to write from
 *	@param	inByteCount		the amount to write
 *	@return the number of bytes actually written
 */
INT32 CFileStream::WriteRaw(const void *inSource, const UINT32 inByteCount)
{
    return static_cast<INT32>(CFile::Write()(inSource, 1, inByteCount, m_FileStream));
}

//==============================================================================
/**
 *	Offsets the file position by the specified number of bytes
 */
void CFileStream::Offset(const INT32 inByteCount)
{
    CFile::Seek()(m_FileStream, inByteCount, CFile::E_SEEK_CUR);
}

//==============================================================================
/**
 *	Returns the current position of the file stream
 */
IStream::TStreamPosition CFileStream::Current()
{
    return static_cast<INT32>(CFile::Tell()(m_FileStream));
}

//==============================================================================
/**
 *	Sets the stream to the specified position within the file
 */
void CFileStream::Set(const TStreamPosition &inPosition)
{
    CFile::Seek()(m_FileStream, inPosition, CFile::E_SEEK_SET);
}

//==============================================================================
/**
 *	Set the endianess of the file.
 *	@warning endian support not implemented
 */
void CFileStream::SetEndian(const BOOL inEndian)
{
    m_Endian = inEndian;
}

//==============================================================================
/**
 *	Get the endianess of the file.
 *	@warning endian support not implemented
 */
BOOL CFileStream::GetEndian()
{
    return m_Endian;
}

} // namespace Q3DStudio
