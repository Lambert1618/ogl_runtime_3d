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
#ifndef FOUNDATION_LINUX_SOCKET_IMPL_H
#define FOUNDATION_LINUX_SOCKET_IMPL_H
#pragma once

/*=========================================================================*\
* BSD include files
\*=========================================================================*/
/* error codes */
#include <errno.h>
/* close function */
#include <unistd.h>
/* fnctnl function and associated constants */
#include <fcntl.h>
/* struct sockaddr */
#include <sys/types.h>
/* socket function */
#include <sys/socket.h>
/* struct timeval */
#include <sys/time.h>
/* gethostbyname and gethostbyaddr functions */
#include <netdb.h>
/* sigpipe handling */
#include <signal.h>
/* IP stuff*/
#include <netinet/in.h>
#include <arpa/inet.h>
/* TCP options (nagle algorithm disable) */
#include <netinet/tcp.h>

namespace qt3ds {
namespace foundation {
    namespace socketimpl {
        // Functions take from lua socket implementation.  Note that it has an MIT license.

        enum {
            IO_DONE = 0, /* operation completed successfully */
            IO_TIMEOUT = -1, /* operation timed out */
            IO_CLOSED = -2, /* the connection has been closed */
            IO_UNKNOWN = -3
        };

        /*-------------------------------------------------------------------------*\
        * I/O error strings
        \*-------------------------------------------------------------------------*/
        const char *io_strerror(int err)
        {
            switch (err) {
            case IO_DONE:
                return NULL;
            case IO_CLOSED:
                return "closed";
            case IO_TIMEOUT:
                return "timeout";
            default:
                return "unknown error";
            }
        }

        typedef int t_socket;
        typedef t_socket *p_socket;
        typedef struct sockaddr SA;

#define SOCKET_INVALID (-1)

#define WAITFD_R 1
#define WAITFD_W 2
#define WAITFD_C (WAITFD_R | WAITFD_W)

        int socket_waitfd(p_socket ps, int sw, QT3DSU32 tm)
        {
            int ret;
            fd_set rfds, wfds, *rp, *wp;
            struct timeval tv, *tp = NULL;
            if (tm == 0)
                return IO_TIMEOUT; /* optimize timeout == 0 case */
            if (tm < QT3DS_MAX_U32) {
                // shoule consider max timeout specially by setting tp = NULL
                // or you get invalid argument errno = 22
                tv.tv_sec = (int)(tm / 1000);
                QT3DSU32 leftover = tm % 1000;
                tv.tv_usec = (int)(leftover * 100000);
                tp = &tv;
            }
            do {
                /* must set bits within loop, because select may have modifed them */
                rp = wp = NULL;
                if (sw & WAITFD_R) {
                    FD_ZERO(&rfds);
                    FD_SET(*ps, &rfds);
                    rp = &rfds;
                }
                if (sw & WAITFD_W) {
                    FD_ZERO(&wfds);
                    FD_SET(*ps, &wfds);
                    wp = &wfds;
                }
                ret = select(*ps + 1, rp, wp, NULL, tp);
            } while (ret == -1 && errno == EINTR);
            if (ret == -1)
                return errno;
            if (ret == 0)
                return IO_TIMEOUT;
            if (sw == WAITFD_C && FD_ISSET(*ps, &rfds))
                return IO_CLOSED;
            return IO_DONE;
        }

        /*-------------------------------------------------------------------------*\
        * Initializes module
        \*-------------------------------------------------------------------------*/
        int socket_open(void)
        {
            /* instals a handler to ignore sigpipe or it will crash us */
            signal(SIGPIPE, SIG_IGN);
            return 1;
        }

        /*-------------------------------------------------------------------------*\
        * Close module
        \*-------------------------------------------------------------------------*/
        int socket_close(void) { return 1; }

        /*-------------------------------------------------------------------------*\
        * Put socket into blocking mode
        \*-------------------------------------------------------------------------*/
        void socket_setblocking(p_socket ps)
        {
            int flags = fcntl(*ps, F_GETFL, 0);
            flags &= (~(O_NONBLOCK));
            fcntl(*ps, F_SETFL, flags);
        }

        /*-------------------------------------------------------------------------*\
        * Put socket into non-blocking mode
        \*-------------------------------------------------------------------------*/
        void socket_setnonblocking(p_socket ps)
        {
            int flags = fcntl(*ps, F_GETFL, 0);
            flags |= O_NONBLOCK;
            fcntl(*ps, F_SETFL, flags);
        }

        /*-------------------------------------------------------------------------*\
        * Close and inutilize socket
        \*-------------------------------------------------------------------------*/
        void socket_destroy(p_socket ps)
        {
            if (*ps != SOCKET_INVALID) {
                socket_setblocking(ps);
                close(*ps);
                *ps = SOCKET_INVALID;
            }
        }
        /*-------------------------------------------------------------------------*\
        *
        \*-------------------------------------------------------------------------*/
        void socket_shutdown(p_socket ps, int how)
        {
            socket_setblocking(ps);
            shutdown(*ps, how);
            socket_setnonblocking(ps);
        }

        /*-------------------------------------------------------------------------*\
        * Creates and sets up a socket
        \*-------------------------------------------------------------------------*/
        int socket_create(p_socket ps, int domain, int type, int protocol)
        {
            *ps = socket(domain, type, protocol);
            if (*ps != SOCKET_INVALID)
                return IO_DONE;
            else
                return errno;
        }

        /*-------------------------------------------------------------------------*\
        *
        \*-------------------------------------------------------------------------*/
        int socket_listen(p_socket ps, int backlog)
        {
            int err = IO_DONE;
            socket_setblocking(ps);
            if (listen(*ps, backlog))
                err = errno;
            socket_setnonblocking(ps);
            return err;
        }

        /*-------------------------------------------------------------------------*\
        * Binds or returns error message
        \*-------------------------------------------------------------------------*/
        int socket_bind(p_socket ps, SA *addr, socklen_t len)
        {
            int err = IO_DONE;
            socket_setblocking(ps);
            if (bind(*ps, addr, len) < 0)
                err = errno;
            socket_setnonblocking(ps);
            return err;
        }

        /*-------------------------------------------------------------------------*\
        * Connects or returns error message
        \*-------------------------------------------------------------------------*/
        int socket_connect(p_socket ps, SA *addr, socklen_t len, QT3DSU32 tm)
        {
            int err;
            /* avoid calling on closed sockets */
            if (*ps == SOCKET_INVALID)
                return IO_CLOSED;
            /* call connect until done or failed without being interrupted */
            do
                if (connect(*ps, addr, len) == 0)
                    return IO_DONE;
            while ((err = errno) == EINTR);
            /* if connection failed immediately, return error code */
            if (err != EINPROGRESS && err != EAGAIN)
                return err;
            /* zero timeout case optimization */
            if (tm == 0)
                return IO_TIMEOUT;
            /* wait until we have the result of the connection attempt or timeout */
            err = socket_waitfd(ps, WAITFD_C, tm);
            if (err == IO_CLOSED) {
                if (recv(*ps, (char *)&err, 0, 0) == 0)
                    return IO_DONE;
                else
                    return errno;
            } else
                return err;
        }

        /*-------------------------------------------------------------------------*\
        * Accept with timeout
        \*-------------------------------------------------------------------------*/
        int socket_accept(p_socket ps, p_socket pa, SA *addr, socklen_t *len, QT3DSU32 tm)
        {
            SA daddr;
            socklen_t dlen = sizeof(daddr);
            if (*ps == SOCKET_INVALID)
                return IO_CLOSED;
            if (!addr)
                addr = &daddr;
            if (!len)
                len = &dlen;
            for (;;) {
                int err;
                if ((*pa = accept(*ps, addr, len)) != SOCKET_INVALID)
                    return IO_DONE;
                err = errno;
                if (err == EINTR)
                    continue;
                if (err != EAGAIN && err != ECONNABORTED)
                    return err;
                if ((err = socket_waitfd(ps, WAITFD_R, tm)) != IO_DONE)
                    return err;
            }
            /* can't reach here */
            return IO_UNKNOWN;
        }

        /*-------------------------------------------------------------------------*\
        * Send with timeout
        \*-------------------------------------------------------------------------*/
        int socket_send(p_socket ps, const char *data, size_t count, size_t *sent, QT3DSU32 tm)
        {
            int err;
            *sent = 0;
            /* avoid making system calls on closed sockets */
            if (*ps == SOCKET_INVALID)
                return IO_CLOSED;
            /* loop until we send something or we give up on error */
            for (;;) {
                long put = (long)send(*ps, data, count, 0);
                /* if we sent anything, we are done */
                if (put > 0) {
                    *sent = put;
                    return IO_DONE;
                }
                err = errno;
                /* send can't really return 0, but EPIPE means the connection was closed */
                if (put == 0 || err == EPIPE)
                    return IO_CLOSED;
                /* we call was interrupted, just try again */
                if (err == EINTR)
                    continue;
                /* if failed fatal reason, report error */
                if (err != EAGAIN)
                    return err;
                /* wait until we can send something or we timeout */
                if ((err = socket_waitfd(ps, WAITFD_W, tm)) != IO_DONE)
                    return err;
            }
            /* can't reach here */
            return IO_UNKNOWN;
        }

        /*-------------------------------------------------------------------------*\
        * Receive with timeout
        \*-------------------------------------------------------------------------*/
        int socket_recv(p_socket ps, char *data, size_t count, size_t *got, QT3DSU32 tm)
        {
            int err;
            *got = 0;
            if (*ps == SOCKET_INVALID)
                return IO_CLOSED;
            for (;;) {
                long taken = (long)recv(*ps, data, count, 0);
                if (taken > 0) {
                    *got = taken;
                    return IO_DONE;
                }
                err = errno;
                if (taken == 0)
                    return IO_CLOSED;
                if (err == EINTR)
                    continue;
                if (err != EAGAIN)
                    return err;
                if ((err = socket_waitfd(ps, WAITFD_R, tm)) != IO_DONE)
                    return err;
            }
            return IO_UNKNOWN;
        }

        int socket_gethostbyaddr(const char *addr, socklen_t len, struct hostent **hp)
        {
            *hp = gethostbyaddr(addr, len, AF_INET);
            if (*hp)
                return IO_DONE;
            else if (h_errno)
                return h_errno;
            else if (errno)
                return errno;
            else
                return IO_UNKNOWN;
        }

        int socket_gethostbyname(const char *addr, struct hostent **hp)
        {
            *hp = gethostbyname(addr);
            if (*hp)
                return IO_DONE;
            else if (h_errno)
                return h_errno;
            else if (errno)
                return errno;
            else
                return IO_UNKNOWN;
        }

        /*-------------------------------------------------------------------------*\
        * Error translation functions
        * Make sure important error messages are standard
        \*-------------------------------------------------------------------------*/
        const char *socket_hoststrerror(int err)
        {
            if (err <= 0)
                return io_strerror(err);
            switch (err) {
            case HOST_NOT_FOUND:
                return "host not found";
            default:
                return hstrerror(err);
            }
        }

        const char *socket_strerror(int err)
        {
            if (err <= 0)
                return io_strerror(err);
            switch (err) {
            case EADDRINUSE:
                return "address already in use";
            case EISCONN:
                return "already connected";
            case EACCES:
                return "permission denied";
            case ECONNREFUSED:
                return "connection refused";
            case ECONNABORTED:
                return "closed";
            case ECONNRESET:
                return "closed";
            case ETIMEDOUT:
                return "timeout";
            default:
                return strerror(errno);
            }
        }
    }
}
}

#endif
