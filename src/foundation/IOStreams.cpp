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
#include "foundation/IOStreams.h"
#include "foundation/FileTools.h"
#include "foundation/StrConvertUTF.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSThread.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSMemoryBuffer.h"

using namespace qt3ds::foundation;

#ifndef _WIN32

inline int _fseeki64(FILE *inFile, int64_t pos, int seekFlags)
{
    return fseek(inFile, (int32_t)pos, seekFlags);
}

inline int64_t _ftelli64(FILE *inFile)
{
    return ftell(inFile);
}

#endif

CFileSeekableIOStream::CFileSeekableIOStream(const char *inFileName, FileOpenFlags inFlags)
{
    openFile(QString(inFileName), inFlags);
}

#ifdef WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T

CFileSeekableIOStream::CFileSeekableIOStream(const wchar_t *inFileName, FileOpenFlags inFlags)
{
    openFile(QString::fromWCharArray(inFileName), inFlags);
}

#endif

CFileSeekableIOStream::CFileSeekableIOStream(const char16_t *inWideName, FileOpenFlags inFlags)
{
    openFile(QString::fromUtf16(inWideName), inFlags);
}

CFileSeekableIOStream::CFileSeekableIOStream(const QString &inFIle, FileOpenFlags inFlags)
{
    openFile(inFIle, inFlags);
}

void CFileSeekableIOStream::openFile(const QString &path, FileOpenFlags inFlags)
{
    if (path.isEmpty())
        return;

    QIODevice::OpenMode fileFlags = QIODevice::ReadOnly;
    if (inFlags & FileOpenFlagValues::Write)
        fileFlags = QIODevice::ReadWrite;
    if (inFlags & FileOpenFlagValues::Truncate)
        fileFlags |= QIODevice::Truncate;

    m_File.setFileName(CFileTools::NormalizePathForQtUsage(path));
    if (!m_File.open(fileFlags)) {
        qCCritical(INTERNAL_ERROR) << "failed to open file"
            << path << "with error" << m_File.errorString();
        QT3DS_ASSERT(false);
    }
}

CFileSeekableIOStream::~CFileSeekableIOStream()
{
    m_File.close();
}

bool CFileSeekableIOStream::IsOpen()
{
    return m_File.isOpen();
}

void CFileSeekableIOStream::SetPosition(QT3DSI64 inOffset, SeekPosition::Enum inEnum)
{
    if (inOffset > QT3DS_MAX_I32 || inOffset < QT3DS_MIN_I32) {
        qCCritical(INVALID_OPERATION, "Attempt to seek further than platform allows");
        QT3DS_ASSERT(false);
        return;
    } else {
        CFileTools::SetStreamPosition(m_File, inOffset, inEnum);
    }
}

QT3DSI64 CFileSeekableIOStream::GetPosition() const
{
    return m_File.pos();
}

QT3DSU32 CFileSeekableIOStream::Read(NVDataRef<QT3DSU8> data)
{
    return m_File.read((char *)data.begin(), data.size());
}

bool CFileSeekableIOStream::Write(NVConstDataRef<QT3DSU8> data)
{
    if (!m_File.isOpen()) {
        QT3DS_ASSERT(false);
        return 0;
    }
    qint64 numBytes = m_File.write((char*)data.begin(), data.size());
    return numBytes == data.size();
}

CMemorySeekableIOStream::CMemorySeekableIOStream(NVAllocatorCallback &inAlloc,
                                                 const char *inAllocName)
    : m_Allocator(inAlloc)
    , m_AllocationName(inAllocName)
    , m_Data(NULL)
    , m_Size(0)
    , m_Offset(0)
    , m_Capacity(0)
{
}

CMemorySeekableIOStream::~CMemorySeekableIOStream()
{
    if (m_Data)
        m_Allocator.deallocate(m_Data);
    m_Data = NULL;
    m_Size = 0;
    m_Capacity = 0;
    m_Offset = 0;
}

void CMemorySeekableIOStream::SetPosition(QT3DSI64 inOffset, SeekPosition::Enum inEnum)
{
    QT3DSI64 startPos = 0;

    switch (inEnum) {
    case SeekPosition::Begin:
        startPos = 0;
        break;
    case SeekPosition::Current:
        startPos = m_Offset;
        break;
    case SeekPosition::End:
        startPos = m_Size;
        break;
    default:
        QT3DS_ASSERT(false);
        break;
    }

    startPos += inOffset;
    if (m_Size == 0 && inOffset != 0) {
        QT3DS_ASSERT(false);
        return;
    }
    if (startPos < 0) {
        QT3DS_ASSERT(false);
        startPos = 0;
    }
    if (startPos >= m_Size && startPos != 0) {
        QT3DS_ASSERT(false);
        startPos = m_Size - 1;
    }
    m_Offset = static_cast<QT3DSU32>(startPos);
}

QT3DSU32 CMemorySeekableIOStream::Read(NVDataRef<QT3DSU8> data)
{
    if (m_Data == NULL)
        return 0;
    QT3DSU32 amountLeft = m_Size - m_Offset;
    QT3DSU32 numBytes = NVMin(amountLeft, data.size());
    intrinsics::memCopy(data.begin(), m_Data + m_Offset, numBytes);
    m_Offset += numBytes;
    return numBytes;
}

bool CMemorySeekableIOStream::Write(NVConstDataRef<QT3DSU8> data)
{
    reserve(data.size() + m_Offset);
    intrinsics::memCopy(m_Data + m_Offset, data.begin(), data.size());
    m_Offset += data.size();
    m_Size = NVMax(m_Size, m_Offset);
    return true;
}

void CMemorySeekableIOStream::reserve(QT3DSU32 inNewSize)
{
    if (inNewSize > m_Capacity) {
        if (inNewSize < 100000)
            inNewSize *= 2;
        QT3DSU8 *newData =
            (QT3DSU8 *)m_Allocator.allocate(inNewSize, m_AllocationName, __FILE__, __LINE__);
        if (m_Size) {
            intrinsics::memCopy(newData, m_Data, m_Size);
            m_Allocator.deallocate(m_Data);
        }
        m_Data = newData;
        m_Capacity = inNewSize;
    }
}

namespace {

struct WriteBufferedStreamImpl;
struct WriteBufferThread : public Thread
{
    // When a buffer is available
    WriteBufferedStreamImpl &m_Impl;
    WriteBufferThread(NVFoundationBase &fnd, WriteBufferedStreamImpl &i)
        : Thread(fnd)
        , m_Impl(i)
    {
        setName("WriteBufferThread");
    }
    void execute() override;
};

#ifdef _WIN32
#pragma warning(disable : 4355)
#endif
/*	Double buffered stream implementation with a sending thread constantly
 *	pulling data from the main thread and writing it out to socket.
 */
struct WriteBufferedStreamImpl : public WriteBufferedOutStream
{
    NVFoundationBase &m_Foundation;
    IOutStream &m_Stream;
    MemoryBuffer<> m_Buf1;
    MemoryBuffer<> m_Buf2;
    MemoryBuffer<> *m_CurrentBuffer;
    MemoryBuffer<> *m_WriteBuffer;
    QT3DSU32 m_BufferSize;
    volatile bool m_StreamValid;
    Mutex m_BufferLock;
    Sync m_DataAvailable;
    Sync m_WriteFinished;
    WriteBufferThread m_Thread;
    QT3DSI32 mRefCount;

    WriteBufferedStreamImpl(NVFoundationBase &fnd, QT3DSU32 totalBufSize, IOutStream &s)
        : m_Foundation(fnd)
        , m_Stream(s)
        , m_Buf1(ForwardingAllocator(fnd.getAllocator(), "WriteBufferedStreamImpl::buffer"))
        , m_Buf2(ForwardingAllocator(fnd.getAllocator(), "WriteBufferedStreamImpl::buffer"))
        , m_CurrentBuffer(&m_Buf1)
        , m_WriteBuffer(NULL)
        , m_BufferSize(totalBufSize / 2)
        , m_StreamValid(true)
        , m_BufferLock(fnd.getAllocator())
        , m_DataAvailable(fnd.getAllocator())
        , m_WriteFinished(fnd.getAllocator())
        , m_Thread(fnd, *this)
        , mRefCount(0)
    {
        m_Buf1.reserve(m_BufferSize);
        m_Buf2.reserve(m_BufferSize);
    }
    ~WriteBufferedStreamImpl()
    {
        m_Thread.signalQuit();
        m_DataAvailable.set();
        m_Thread.waitForQuit();
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    bool Write(NVConstDataRef<QT3DSU8> data) override
    {
        while (data.size() && m_StreamValid) {
            QT3DSU32 currentBufferSize;
            QT3DSU32 amountCanWrite;
            {
                Mutex::ScopedLock locker(m_BufferLock);
                currentBufferSize = m_CurrentBuffer->size();
                amountCanWrite = NVMin(data.size(), m_BufferSize - currentBufferSize);
                m_CurrentBuffer->write(data.begin(), amountCanWrite);
                currentBufferSize += amountCanWrite;
            }
            m_DataAvailable.set();
            if (currentBufferSize == m_BufferSize) {
                m_WriteFinished.wait();
                m_WriteFinished.reset();
                // Blocking call if we are already sending data.
                data = NVConstDataRef<QT3DSU8>(data.begin() + amountCanWrite,
                                            data.size() - amountCanWrite);
            }
        }
        return m_StreamValid;
    }

    IOutStream &wrappedStream() override { return m_Stream; }

    QT3DSU32 getTotalBufferSize()
    {
        Mutex::ScopedLock locker(m_BufferLock);
        QT3DSU32 retval = m_CurrentBuffer->size();
        if (m_WriteBuffer)
            retval += m_WriteBuffer->size();
        return retval;
    }

    QT3DSU32 getWriteBufferSize()
    {
        Mutex::ScopedLock locker(m_BufferLock);
        if (m_WriteBuffer)
            return m_WriteBuffer->size();
        return 0;
    }

    void flush() override
    {
        while (getTotalBufferSize()) {
            m_WriteFinished.wait();
            m_WriteFinished.reset();
        }
    }
};

void WriteBufferThread::execute()
{
    while (!quitIsSignalled()) {
        m_Impl.m_DataAvailable.wait();

        if (!quitIsSignalled() && m_Impl.m_StreamValid) {
            m_Impl.m_DataAvailable.reset();
            {
                Mutex::ScopedLock locker(m_Impl.m_BufferLock);
                m_Impl.m_WriteBuffer = m_Impl.m_CurrentBuffer;
                m_Impl.m_CurrentBuffer =
                    m_Impl.m_CurrentBuffer == &m_Impl.m_Buf1 ? &m_Impl.m_Buf2 : &m_Impl.m_Buf1;
                QT3DS_ASSERT(m_Impl.m_WriteBuffer != m_Impl.m_CurrentBuffer);
            }
            NVConstDataRef<QT3DSU8> dataBuffer(*m_Impl.m_WriteBuffer);
            if (dataBuffer.size()) {
                m_Impl.m_StreamValid = m_Impl.m_Stream.Write(dataBuffer);
                {
                    Mutex::ScopedLock locker(m_Impl.m_BufferLock);
                    m_Impl.m_WriteBuffer->clear();
                    m_Impl.m_WriteBuffer = NULL;
                }
            }
            m_Impl.m_WriteFinished.set();
        }
    }
    quit();
}
}

NVScopedRefCounted<WriteBufferedOutStream>
WriteBufferedOutStreamCreate(NVFoundationBase &fnd, IOutStream &stream, QT3DSU32 totalBufferSize)
{
    return QT3DS_NEW(fnd.getAllocator(), WriteBufferedStreamImpl)(fnd, totalBufferSize, stream);
}
