/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#ifndef QT3DS_STATE_H
#define QT3DS_STATE_H

namespace qt3ds {
class NVAllocatorCallback;
class NVFoundationBase;
namespace foundation {
    class CRegisteredString;
    class IStringTable;
    class IOutStream;
    class IInStream;
    class IDOMFactory;
    struct SDOMAttribute;
    struct SDOMElement;
    struct SNamespacePairNode;
    class SocketStream;
}

namespace intrinsics {
}
}

namespace qt3ds {
namespace state {

    using namespace qt3ds;
    using namespace qt3ds::foundation;
    using namespace qt3ds::intrinsics;
    using qt3ds::foundation::CRegisteredString;
    using qt3ds::foundation::IStringTable;
    class IStateContext;
    class IStateInterpreter;
    class IStateLogger;
    class IExecutionContext;
    class IScriptContext;
    struct SSCXML;
    struct SState;
    struct STransition;
    struct SParallel;
    struct SFinal;
    struct SHistory;
    struct SOnEntry;
    struct SOnExit;
    struct SSend;
    struct SRaise;
    struct SIf;
    struct SElseIf;
    struct SElse;
    struct SLog;
    struct SAssign;
    struct SScript;
    struct SDataModel;
    struct SData;
    struct SStateNode;
    struct SCancel;

    namespace editor {
        class IEditor;
        class IEditorObject;
    }
}
}

#endif
