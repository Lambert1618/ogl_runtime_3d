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
#ifndef QT3DS_FOUNDATION_SOCKET_H
#define QT3DS_FOUNDATION_SOCKET_H
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSMemoryBuffer.h"

namespace qt3ds {

class NVFoundationBase;
namespace foundation {

    class SocketStream : public IInStream, public IOutStream, public NVRefCounted
    {
    public:
        // set timeout for write and blocking read operations
        // Setting to zero makes all operations nonblocking.
        virtual void setTimeout(QT3DSU32 milliseconds) = 0;
        virtual QT3DSU32 nonBlockingRead(NVDataRef<QT3DSU8> ioBuffer) = 0;
        virtual bool connected() = 0;
        virtual void shutdown() = 0;

        // Useful to testing systems without going into the issues that a network connection can
        // cause.
        static NVScopedRefCounted<SocketStream>
        CreateFileStream(NVFoundationBase &fnd, const char *fname, FileOpenFlags flags);
    };

    class SocketServer : public NVRefCounted
    {
    public:
        virtual int port() = 0;
        virtual void setTimeout(QT3DSU32 milliseconds) = 0;
        virtual NVScopedRefCounted<SocketStream> nextClient() = 0;
    };

    class SocketSystem : public NVRefCounted
    {
    public:
        // timeout of 0 means wait forever
        virtual NVScopedRefCounted<SocketStream>
        createStream(const char *host, QT3DSI32 port, QT3DSU32 timeoutMilliseconds = 10000) = 0;
        virtual NVScopedRefCounted<SocketServer> createServer(QT3DSI32 port) = 0;

        static NVScopedRefCounted<SocketSystem> createSocketSystem(NVFoundationBase &fnd);
    };
}
}

#endif