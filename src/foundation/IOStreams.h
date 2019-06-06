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
#ifndef QT3DS_FOUNDATION_IO_STREAMS_H
#define QT3DS_FOUNDATION_IO_STREAMS_H

#include "EABase/eabase.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSAssert.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "foundation/TrackingAllocator.h"
#include "foundation/Qt3DSMath.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/Utils.h"
#include "foundation/Qt3DSRefCounted.h"

#include <QFile>
#include <QString>

namespace qt3ds {
class NVFoundationBase;

namespace foundation {

    class IOutStream
    {
    protected:
        virtual ~IOutStream() {}
    public:
        virtual bool Write(NVConstDataRef<QT3DSU8> data) = 0;
        void WriteWithLen(NVConstDataRef<QT3DSU8> data)
        {
            Write(data.size());
            Write(data);
        }
        template <typename TDataType>
        void Write(const TDataType &type)
        {
            Write(toU8ConstDataRef(type));
        }
        template <typename TDataType>
        void Write(const TDataType *data, QT3DSU32 numItems)
        {
            Write(toU8ConstDataRef(data, numItems));
        }
        void Write(const wchar_t *data)
        {
            if (data == NULL)
                data = L"";
            // Write the null character at the end of the string.
            // This just makes reading and debugging a lot less error prone.
            // at the expense of 2 bytes.
            Write(data, (QT3DSU32)StrLen(data) + 1);
        }
        template <typename TDataType>
        void WriteWithLen(const TDataType *data, QT3DSU32 numItems)
        {
            WriteWithLen(toU8ConstDataRef(data, numItems));
        }
    };

    class IInStream
    {
    protected:
        virtual ~IInStream() {}
    public:
        // Semantics are precisely that you return the amount read
        virtual QT3DSU32 Read(NVDataRef<QT3DSU8> data) = 0;

        QT3DSU32 SafeRead(NVDataRef<QT3DSU8> data)
        {
            QT3DSU32 amountRead = Read(data);
            QT3DS_ASSERT(amountRead == data.size());
            if (amountRead < data.size())
                intrinsics::memZero(data.begin() + amountRead, data.size() - amountRead);
            return amountRead;
        }
        QT3DSU32 ReadWithLen(NVDataRef<QT3DSU8> data)
        {
            QT3DSU32 len = 0;
            Read(len);
            return SafeRead(toDataRef(data.begin(), len));
        }
        template <typename TDataType>
        QT3DSU32 Read(TDataType &type)
        {
            return SafeRead(toU8DataRef(type));
        }
        template <typename TDataType>
        QT3DSU32 Read(TDataType *data, QT3DSU32 numItems)
        {
            return SafeRead(toU8DataRef(data, numItems));
        }
    };

    struct SeekPosition
    {
        enum Enum {
            Unknown,
            Begin,
            Current,
            End,
        };
    };

    class ISeekable
    {
    protected:
        virtual ~ISeekable() {}
    public:
        virtual void SetPosition(QT3DSI64 inOffset, SeekPosition::Enum inEnum) = 0;
        virtual QT3DSI64 GetPosition() const = 0;
        virtual QT3DSI64 GetLength() const
        {
            ISeekable &seekable(const_cast<ISeekable &>(*this));
            QT3DSI64 currentPos(GetPosition());
            seekable.SetPosition(0, SeekPosition::End);
            QT3DSI64 retval(GetPosition());
            seekable.SetPosition(currentPos, SeekPosition::Begin);
            return retval;
        }
    };

    class ISeekableIOStream : public IInStream, public IOutStream, public ISeekable
    {
    };

    struct FileOpenFlagValues
    {
        enum Enum {
            Open = 1, // Without this flag, function fails if file exists
            Truncate = 1 << 1, // Truncate the file so an immediate close will empty it.
            Create = 1 << 2,
            Write = 1 << 3,
        };
    };

    typedef NVFlags<FileOpenFlagValues::Enum, int> FileOpenFlags;

    static inline FileOpenFlags FileReadFlags() { return FileOpenFlags(FileOpenFlagValues::Open); }

    static inline FileOpenFlags FileWriteFlags()
    {
        return FileOpenFlags(FileOpenFlagValues::Create | FileOpenFlagValues::Open
                             | FileOpenFlagValues::Write | FileOpenFlagValues::Truncate);
    }
    static inline FileOpenFlags FileAppendFlags()
    {
        return FileOpenFlags(FileOpenFlagValues::Create | FileOpenFlagValues::Open
                             | FileOpenFlagValues::Write);
    }

    class CFileSeekableIOStream : public ISeekableIOStream
    {
    protected:
        QFile m_File;

    public:
        // Enabling append also enables reading from the file while being able to
        // write to it.
        CFileSeekableIOStream(const char *inFile, FileOpenFlags inFlags);
#ifdef WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T
        CFileSeekableIOStream(const wchar_t *inFile, FileOpenFlags inFlags);
#endif

        CFileSeekableIOStream(const char16_t *inFile, FileOpenFlags inFlags);
        CFileSeekableIOStream(const QString &inFIle, FileOpenFlags inFlags);
        virtual ~CFileSeekableIOStream();
        virtual bool IsOpen();
        void SetPosition(QT3DSI64 inOffset, SeekPosition::Enum inEnum) override;
        QT3DSI64 GetPosition() const override;
        QT3DSU32 Read(NVDataRef<QT3DSU8> data) override;
        bool Write(NVConstDataRef<QT3DSU8> data) override;

    private:
        void openFile(const QString &path, FileOpenFlags inFlags);
    };

    class CMemorySeekableIOStream : public ISeekableIOStream
    {
    protected:
        NVAllocatorCallback &m_Allocator;
        const char *m_AllocationName;
        QT3DSU8 *m_Data;
        QT3DSU32 m_Size;
        QT3DSU32 m_Offset;
        QT3DSU32 m_Capacity;

    public:
        CMemorySeekableIOStream(NVAllocatorCallback &inAlloc, const char *inAllocName);
        virtual ~CMemorySeekableIOStream();
        void SetPosition(QT3DSI64 inOffset, SeekPosition::Enum inEnum) override;
        QT3DSI64 GetPosition() const override { return m_Offset; }
        QT3DSI64 GetLength() const override { return m_Size; }
        QT3DSU32 Read(NVDataRef<QT3DSU8> data) override;
        bool Write(NVConstDataRef<QT3DSU8> data) override;

        // Add in the standard container functions
        QT3DSU8 *begin() { return m_Data; }
        QT3DSU8 *end() { return m_Data + m_Size; }
        const QT3DSU8 *begin() const { return m_Data; }
        const QT3DSU8 *end() const { return m_Data + m_Size; }
        void clear()
        {
            m_Offset = 0;
            m_Size = 0;
        }
        QT3DSU32 size() const { return m_Size; }
        void reserve(QT3DSU32 inNewSize);
        void resize(QT3DSU32 inNewSize)
        {
            reserve(inNewSize);
            m_Size = inNewSize;
        }
    };

    // Simple, one way input stream.
    struct SMemoryInStream : public IInStream
    {
        const QT3DSU8 *m_Begin;
        const QT3DSU8 *m_End;
        SMemoryInStream(const QT3DSU8 *b, const QT3DSU8 *e)
            : m_Begin(b)
            , m_End(e)
        {
        }

        QT3DSU32 Read(NVDataRef<QT3DSU8> data) override
        {
            size_t available = m_End - m_Begin;
            size_t requested = data.size();
            size_t amount = NVMin(available, requested);
            qt3ds::intrinsics::memCopy(data.mData, m_Begin, (QT3DSU32)amount);
            m_Begin += amount;
            return (QT3DSU32)amount;
        }
    };

    class WriteBufferedOutStream : public IOutStream, public NVRefCounted
    {
    public:
        virtual IOutStream &wrappedStream() = 0;
        virtual void flush() = 0;

        static NVScopedRefCounted<WriteBufferedOutStream>
        Create(NVFoundationBase &fnd, IOutStream &stream, QT3DSU32 totalBufferSize = 128 * 1024);
    };
}
}

#endif
