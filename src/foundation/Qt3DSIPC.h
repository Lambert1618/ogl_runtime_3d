/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QT3DS_IPC_H
#define QT3DS_IPC_H

#include "foundation/Qt3DS.h"

namespace qt3ds {
namespace foundation {

    class NVIPC
    {
    public:
        enum ConnectionType {
            CT_CLIENT, // start up as a client, will succeed even if the server has not yet been
                       // found.
            CT_CLIENT_REQUIRE_SERVER, // start up as a client, but only if the server already
                                      // exists.
            CT_SERVER, // will start up as a server, will fail if an existing server is already
                       // open.
            CT_CLIENT_OR_SERVER, // connect as either a client or server, don't care who is created
                                 // first.
            CT_LAST
        };

        enum ErrorCode {
            EC_OK, // no error.
            EC_FAIL, // generic failure.
            EC_SERVER_ALREADY_EXISTS, // couldn't create a server, because the server already
                                      // exists.
            EC_CLIENT_ALREADY_EXISTS, // couldn't create a client, because an existing client is
                                      // already registered.
            EC_CLIENT_SERVER_ALREADY_EXISTS, // both the client and server channels are already used
            EC_SERVER_NOT_FOUND, // client opened with a required server, which was not found.
            EC_BUFFER_MISSMATCH, // the reserved buffers for client/server do not match up.
            EC_MAPFILE_CREATE, // failed to create the shared memory map file.
            EC_MAPFILE_VIEW, // failed to map the memory view of he
            // communications errors.
            EC_SEND_DATA_EXCEEDS_MAX_BUFFER, // trying to send more data than can even fit in the
                                             // sednd buffe.
            EC_SEND_DATA_TOO_LARGE, // the data we tried to send exceeds the available room int the
                                    // output ring buffer.
            EC_SEND_BUFFER_FULL, // the send buffer is completely full.
            EC_SEND_FROM_WRONG_THREAD, // Tried to do a send from a different thread
            EC_RECEIVE_FROM_WRONG_THREAD, // Tried to do a recieve from a different thread
            EC_NO_RECEIVE_PENDING, // tried to acknowledge a receive but none was pending.
        };

        virtual bool pumpPendingSends(void) = 0; // give up a time slice to pending sends; returns
                                                 // true if there are still pends sending.
        virtual ErrorCode sendData(const void *data, QT3DSU32 data_len, bool bufferIfFull) = 0;
        virtual const void *receiveData(QT3DSU32 &data_len) = 0;
        virtual ErrorCode receiveAcknowledge(void) = 0; // acknowledge that we have processed the
                                                        // incoming message and can advance the read
                                                        // buffer.

        virtual bool isServer(void) const = 0; // returns true if we are opened as a server.

        virtual bool haveConnection(void) const = 0;

        virtual bool canSend(QT3DSU32 len) = 0; // return true if we can send a message of this size.

        virtual void release(void) = 0;

    protected:
        virtual ~NVIPC(void){}
    };
}; // end of namespace
}; // end of namespace

#endif
