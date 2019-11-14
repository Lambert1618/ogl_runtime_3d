/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef Q3DSDATAINPUT_P_P_H
#define Q3DSDATAINPUT_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtStudio3D API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "q3dsdatainput.h"
#include "q3dscommandqueue_p.h"
#include "Qt3DSViewerApp.h"

QT_BEGIN_NAMESPACE

class Q3DSPresentationPrivate;

class Q_STUDIO3D_EXPORT Q3DSDataInputPrivate
{
    Q_DECLARE_PUBLIC(Q3DSDataInput)
public:
    explicit Q3DSDataInputPrivate(Q3DSDataInput *parent);
    virtual ~Q3DSDataInputPrivate();

    void setValue(const QVariant &value, bool force = false);
    void setViewerApp(Q3DSViewer::Q3DSViewerApp *app);
    void setCommandQueue(CommandQueue *queue);
    void setPresentation(Q3DSPresentation *presentation);
    void setDirty(bool dirty);
    // Used by presentation to indicate which is the latest committed (not cached locally) value.
    void setCommittedValue(const QVariant &value);

protected:
    Q3DSDataInput *q_ptr;
    Q3DSViewer::Q3DSViewerApp *m_viewerApp = nullptr; // Not owned
    CommandQueue *m_commandQueue = nullptr; // Not owned
    Q3DSPresentation *m_presentation = nullptr; // Not owned
    QString m_name;
    // Local cached values, used only when synchronous getter from runtime engine
    // is not available (for QML -side that is behind command queue).
    QVariant m_value;
    float m_max = 0;
    float m_min = 0;
    // Used to order API calls
    int m_callIdx = 0;

    bool m_dirty = false;
    bool m_forced = false;

    QVariant m_committedValue; // Latest value that was committed to runtime engine

    // Note: Qt3d Runtime allows metadata to be both read and set, therefore requiring
    // internal representation of both keys and values to be QVariant as per API convention.
    // OpenGL Runtime, in contrast, only allows metadata to be read. As metadata now can only
    // come from UIA file where it is stored as string, we use QStrings in API as well.
    QHash<QString, QString> m_metadata;

    friend class Q3DSPresentationPrivate;
    friend class Q3DSPresentation;
    friend class Q3DSRenderer;
};

QT_END_NAMESPACE

#endif // Q3DSDATAINPUT_P_P_H
