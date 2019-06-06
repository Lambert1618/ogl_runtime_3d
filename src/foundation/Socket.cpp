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

/*
LuaSocket 3.0 license
Copyright � 2004-2013 Diego Nehab

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "foundation/Socket.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSMutex.h"
#include "foundation/Qt3DSSync.h"
#include "foundation/Qt3DSMemoryBuffer.h"

#if defined QT3DS_WINDOWS || defined QT3DS_WIN8ARM
#include "windows/SocketImpl.h"
#else
#include "linux/SocketImpl.h"
#endif

using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace qt3ds::foundation::socketimpl;

namespace {

#if defined QT3DS_WINDOWS || defined QT3DS_WIN8ARM
/*-------------------------------------------------------------------------*\
* Some systems do not provide this so that we provide our own. It's not
* marvelously fast, but it works just fine.
\*-------------------------------------------------------------------------*/
int inet_aton(const char *cp, struct in_addr *inp)
{
    unsigned int a = 0, b = 0, c = 0, d = 0;
    int n = 0, r;
    unsigned long int addr = 0;
    r = sscanf(cp, "%u.%u.%u.%u%n", &a, &b, &c, &d, &n);
    if (r == 0 || n == 0)
        return 0;
    cp += n;
    if (*cp)
        return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255)
        return 0;
    if (inp) {
        addr += a;
        addr <<= 8;
        addr += b;
        addr <<= 8;
        addr += c;
        addr <<= 8;
        addr += d;
        inp->s_addr = htonl(addr);
    }
    return 1;
}
#endif

/*-------------------------------------------------------------------------*\
* Tries to connect to remote address (address, port)
\*-------------------------------------------------------------------------*/
int inet_tryconnect(p_socket ps, const char *address, unsigned short port, QT3DSU32 tm, SA *remoteAddr)
{
    struct sockaddr_in remote;
    int err;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    if (strcmp(address, "*")) {
        if (!inet_aton(address, &remote.sin_addr)) {
            struct hostent *hp = NULL;
            struct in_addr **addr;
            err = socket_gethostbyname(address, &hp);
            if (err != IO_DONE)
                return err;
            addr = (struct in_addr **)hp->h_addr_list;
            memcpy(&remote.sin_addr, *addr, sizeof(struct in_addr));
        }
    } else
        remote.sin_family = AF_UNSPEC;
    if (remoteAddr)
        memcpy(remoteAddr, &remote, sizeof(remote));
    err = socket_connect(ps, (SA *)&remote, sizeof(remote), tm);
    return err;
}

/*-------------------------------------------------------------------------*\
* Tries to bind socket to (address, port)
\*-------------------------------------------------------------------------*/
int inet_trybind(p_socket ps, const char *address, unsigned short port)
{
    struct sockaddr_in local;
    int err;
    memset(&local, 0, sizeof(local));
    /* address is either wildcard or a valid ip address */
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    local.sin_family = AF_INET;
    if (strcmp(address, "*") && !inet_aton(address, &local.sin_addr)) {
        struct hostent *hp = NULL;
        struct in_addr **addr;
        err = socket_gethostbyname(address, &hp);
        if (err != IO_DONE)
            return err;
        addr = (struct in_addr **)hp->h_addr_list;
        memcpy(&local.sin_addr, *addr, sizeof(struct in_addr));
    }
    err = socket_bind(ps, (SA *)&local, sizeof(local));
    if (err != IO_DONE)
        socket_destroy(ps);
    return err;
}

static bool is_socket_error(int errcode)
{
    return errcode != IO_DONE && errcode != IO_TIMEOUT;
}

const char *generalized_strerror(int err)
{
    if (err <= 0)
        return io_strerror(err);
    else
        return socket_strerror(err);
}

struct SocketSystemCore : public NVRefCounted
{
    NVFoundationBase &m_Foundation;
    QT3DSI32 mRefCount;
    SocketSystemCore(NVFoundationBase &fnd)
        : m_Foundation(fnd)
        , mRefCount(0)
    {
    }
    ~SocketSystemCore() { socket_close(); }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    static NVScopedRefCounted<SocketSystemCore> Create(NVFoundationBase &fnd)
    {
        int success = socket_open();
        if (!success) {
            qCCritical(INVALID_OPERATION, "Failed to initialize network subsystem");
            return NVScopedRefCounted<SocketSystemCore>();
        }
        return QT3DS_NEW(fnd.getAllocator(), SocketSystemCore)(fnd);
    }
};

struct SocketStreamImpl : public SocketStream
{
    NVScopedRefCounted<SocketSystemCore> m_SocketCore;
    NVFoundationBase &m_Foundation;
    t_socket m_Socket;
    SA m_Destination;
    bool m_Connected;
    QT3DSU32 m_Timeout;
    QT3DSI32 mRefCount;
    SocketStreamImpl(SocketSystemCore &core, NVFoundationBase &fnd, t_socket s, SA dest)
        : m_SocketCore(core)
        , m_Foundation(fnd)
        , m_Socket(s)
        , m_Destination(dest)
        , m_Connected(true)
        , m_Timeout(10000)
        , mRefCount(0)
    {
        // We use wait functions in order to block.
        socket_setnonblocking(&m_Socket);
    }

    ~SocketStreamImpl()
    {
        shutdown();
        socket_destroy(&m_Socket);
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    void setTimeout(QT3DSU32 milliseconds) override { m_Timeout = milliseconds; }

    bool Write(NVConstDataRef<QT3DSU8> data) override
    {
        if (m_Connected == false)
            return false;

        size_t totalSent = 0;
        const char *writePtr(reinterpret_cast<const char *>(data.begin()));
        size_t amountLeft = data.size();
        do {

            size_t amountSent = 0;
            int errcode =
                socket_send(&m_Socket, writePtr + totalSent, amountLeft, &amountSent, m_Timeout);

            if (is_socket_error(errcode)) {
                m_Connected = false;
                qCWarning(WARNING, "Networking error during send: %s", generalized_strerror(errcode));
                return false;
            }
            totalSent += amountSent;
            amountLeft -= amountSent;
        } while (amountLeft);
        return true;
    }

    QT3DSU32 Read(NVDataRef<QT3DSU8> data) override
    {
        if (m_Connected == false)
            return 0;
        size_t amountReceived = 0;
        int errcode = socket_recv(&m_Socket, reinterpret_cast<char *>(data.begin()), data.size(),
                                  &amountReceived, m_Timeout);
        if (is_socket_error(errcode)) {
            m_Connected = false;
            qCWarning(WARNING, "Networking error during receive: %s", generalized_strerror(errcode));
            return false;
        }
        return static_cast<QT3DSU32>(amountReceived);
    }

    QT3DSU32 nonBlockingRead(NVDataRef<QT3DSU8> data) override
    {
        if (m_Connected == false)
            return 0;
        size_t amountReceived = 0;
        int errcode = socket_recv(&m_Socket, reinterpret_cast<char *>(data.begin()), data.size(),
                                  &amountReceived, 0);
        if (is_socket_error(errcode)) {
            m_Connected = false;
            qCWarning(WARNING, "Networking error during receive: %s",
                               generalized_strerror(errcode));
            return false;
        }
        return static_cast<QT3DSU32>(amountReceived);
    }

    bool connected() override { return m_Connected; }

    void shutdown() override
    {
        if (m_Connected) {
            socket_shutdown(&m_Socket, 2);
            m_Connected = false;
        }
    }
};

struct FileSocketStreamImpl : public SocketStream
{
    NVFoundationBase &m_Foundation;
    CFileSeekableIOStream m_Stream;
    FileOpenFlags m_FileFlags;
    bool m_Connected;
    QT3DSI32 mRefCount;

    FileSocketStreamImpl(NVFoundationBase &fnd, const char *fname, FileOpenFlags fileFlags)
        : m_Foundation(fnd)
        , m_Stream(fname, fileFlags)
        , m_FileFlags(fileFlags)
        , m_Connected(false)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    bool IsOpen() { return m_Connected && m_Stream.IsOpen(); }

    QT3DSU32 Read(NVDataRef<QT3DSU8> data) override
    {
        if (IsOpen()) {
            QT3DSU32 retval = m_Stream.Read(data);
            if (retval < data.size())
                m_Connected = false;
            return retval;
        }

        return 0;
    }

    bool Write(NVConstDataRef<QT3DSU8> data) override
    {
        bool canWrite = m_FileFlags & FileOpenFlagValues::Write;
        if (IsOpen() && canWrite) {
            m_Stream.Write(data);
            return true;
        }
        return false;
    }

    void setTimeout(QT3DSU32) override {}

    QT3DSU32 nonBlockingRead(NVDataRef<QT3DSU8> data) override { return Read(data); }

    bool connected() override { return m_Connected; }

    void shutdown() override { m_Connected = false; }
};

struct SocketServerImpl : public SocketServer
{
    NVScopedRefCounted<SocketSystemCore> m_SocketCore;
    NVFoundationBase &m_Foundation;
    t_socket m_Socket;
    int m_Port;
    QT3DSU32 m_Timeout;
    QT3DSI32 mRefCount;
    SocketServerImpl(SocketSystemCore &core, NVFoundationBase &fnd, t_socket s, int port)
        : m_SocketCore(core)
        , m_Foundation(fnd)
        , m_Socket(s)
        , m_Port(port)
        , m_Timeout(10000)
        , mRefCount(0)
    {
    }

    ~SocketServerImpl() { socket_destroy(&m_Socket); }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    int port() override { return m_Port; }

    void setTimeout(QT3DSU32 milliseconds) override { m_Timeout = milliseconds; }

    NVScopedRefCounted<SocketStream> nextClient() override
    {
        SA daddr;
        memset(&daddr, 0, sizeof(daddr));
        t_socket new_socket;
        int errcode = socket_accept(&m_Socket, &new_socket, NULL, NULL, m_Timeout);

        if (is_socket_error(errcode)) {
            return NVScopedRefCounted<SocketStream>();
        } else if (errcode == IO_DONE) {
            return QT3DS_NEW(m_Foundation.getAllocator(),
                          SocketStreamImpl)(*m_SocketCore, m_Foundation, new_socket, daddr);
        }
        return NVScopedRefCounted<SocketStream>();
    }
};

struct SocketSystemImpl : public SocketSystem
{
    NVScopedRefCounted<SocketSystemCore> m_SocketCore;
    NVFoundationBase &m_Foundation;
    QT3DSI32 mRefCount;
    SocketSystemImpl(SocketSystemCore &core, NVFoundationBase &fnd)
        : m_SocketCore(core)
        , m_Foundation(fnd)
        , mRefCount(0)

    {
    }
    ~SocketSystemImpl() {}

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    NVScopedRefCounted<SocketStream> createStream(const char *host, QT3DSI32 port,
                                                          QT3DSU32 timeoutMilliseconds) override
    {
        t_socket newSocket(SOCKET_INVALID);
        int errcode = socket_create(&newSocket, PF_INET, SOCK_STREAM, 0);
        if (is_socket_error(errcode)) {
            qCWarning(WARNING, "Error during connect: %s", generalized_strerror(errcode));
            return NVScopedRefCounted<SocketStream>();
        }
        SA remoteAddr;
        memset(&remoteAddr, 0, sizeof(SA));
        errcode = inet_tryconnect(&newSocket, host, (unsigned short)port, timeoutMilliseconds,
                                  &remoteAddr);
        if (is_socket_error(errcode)) {
            qCWarning(WARNING, "Error during connect: %s", generalized_strerror(errcode));
            return NVScopedRefCounted<SocketStream>();
        } else if (errcode == IO_DONE) {
            return QT3DS_NEW(m_Foundation.getAllocator(),
                          SocketStreamImpl)(*m_SocketCore, m_Foundation, newSocket, remoteAddr);
        }
        return NVScopedRefCounted<SocketStream>();
    }

    NVScopedRefCounted<SocketServer> createServer(QT3DSI32 port) override
    {
        t_socket newSocket(SOCKET_INVALID);
        int errcode = socket_create(&newSocket, PF_INET, SOCK_STREAM, 0);
        if (is_socket_error(errcode)) {
            qCWarning(WARNING, "Error during create server create socket: %s",
                generalized_strerror(errcode));
            return NVScopedRefCounted<SocketServer>();
        }
        errcode = inet_trybind(&newSocket, "*", (unsigned short)port);
        if (is_socket_error(errcode)) {
            qCWarning(WARNING, "Error during create server bind: %s",
                generalized_strerror(errcode));
            return NVScopedRefCounted<SocketServer>();
        } else if (errcode == IO_DONE) {
            errcode = socket_listen(&newSocket, 10);
            if (errcode == IO_DONE) {
                return QT3DS_NEW(m_Foundation.getAllocator(),
                              SocketServerImpl)(*m_SocketCore, m_Foundation, newSocket, port);
            } else
                qCWarning(WARNING, "Error during create server listen: %s",
                    generalized_strerror(errcode));
        }
        return NVScopedRefCounted<SocketServer>();
    }
};
}

NVScopedRefCounted<SocketStream>
SocketStream::CreateFileStream(NVFoundationBase &fnd, const char *fname, FileOpenFlags flags)
{
    return QT3DS_NEW(fnd.getAllocator(), FileSocketStreamImpl)(fnd, fname, flags);
}

NVScopedRefCounted<SocketSystem> SocketSystem::createSocketSystem(NVFoundationBase &fnd)
{
    NVScopedRefCounted<SocketSystemCore> theCore = SocketSystemCore::Create(fnd);
    if (theCore) {
        return QT3DS_NEW(fnd.getAllocator(), SocketSystemImpl)(*theCore, fnd);
    }
    return NVScopedRefCounted<SocketSystem>();
}
