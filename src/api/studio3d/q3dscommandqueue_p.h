/****************************************************************************
**
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

#ifndef Q3DS_COMMAND_QUEUE_H
#define Q3DS_COMMAND_QUEUE_H

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

#include "q3dsviewersettings.h"

#include <QtGui/qcolor.h>
#include <QtCore/qsize.h>
#include <QtCore/qvector.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

enum CommandType {
    CommandType_Invalid = 0,
    CommandType_SetAttribute,
    CommandType_SetPresentationActive,
    CommandType_GoToSlideByName,
    CommandType_GoToSlide,
    CommandType_GoToSlideRelative,
    CommandType_GoToTime,
    CommandType_FireEvent,
    CommandType_MousePress,
    CommandType_MouseRelease,
    CommandType_MouseMove,
    CommandType_MouseWheel,
    CommandType_KeyPress,
    CommandType_KeyRelease,
    CommandType_SetGlobalAnimationTime,
    CommandType_SetDataInputValue,
    CommandType_SetDataInputBatch,
    CommandType_CreateElements,
    CommandType_DeleteElements,
    CommandType_CreateMaterials,
    CommandType_DeleteMaterials,
    CommandType_CreateMeshes,
    CommandType_DeleteMeshes,

    // Requests
    CommandType_RequestSlideInfo,
    CommandType_RequestDataInputs,
    CommandType_PreloadSlide,
    CommandType_UnloadSlide,
    CommandType_RequestDataOutputs
};

class Q_STUDIO3D_EXPORT ElementCommand
{
public:
    ElementCommand();

    CommandType m_commandType;
    QString m_elementPath;
    QString m_stringValue;
    QVariant m_variantValue;
    void *m_data = nullptr; // Data is owned by the queue and is deleted once command is handled
    union {
        bool m_boolValue;
        float m_floatValue;
        long m_longValue;
        int m_intValues[4];
        qint64 m_int64Value;
    };

    QString toString() const;
};

typedef QVector<ElementCommand> CommandList;

class Q_STUDIO3D_EXPORT CommandQueue
{
public:
    CommandQueue();

    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType,
                                 const QString &attributeName, const QVariant &value);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType,
                                 const QString &attributeName, const QVariant &value,
                                 int intValue);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType,
                                 const QString &value);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType,
                                 bool value);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType, float value);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType, long value);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType,
                                 int value0, int value1 = 0,
                                 int value2 = 0, int value3 = 0);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType,
                                 const QString &stringValue, void *commandData);
    ElementCommand &queueCommand(CommandType commandType, void *commandData);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType);
    ElementCommand &queueCommand(const QString &elementPath, CommandType commandType,
                                 void *commandData);

    void copyCommands(CommandQueue &fromQueue);

    bool m_visibleChanged;
    bool m_scaleModeChanged;
    bool m_stereoModeChanged;
    bool m_stereoEyeSeparationChanged;
    bool m_shadeModeChanged;
    bool m_showRenderStatsChanged;
    bool m_matteColorChanged;
    bool m_sourceChanged;
    bool m_variantListChanged;
    bool m_globalAnimationTimeChanged;
    bool m_delayedLoadingChanged;
    bool m_matteEnabledChanged;

    bool m_visible;
    Q3DSViewerSettings::ScaleMode m_scaleMode;
    Q3DSViewerSettings::StereoMode m_stereoMode;
    double m_stereoEyeSeparation;
    Q3DSViewerSettings::ShadeMode m_shadeMode;
    bool m_showRenderStats;
    QColor m_matteColor;
    QUrl m_source;
    QStringList m_variantList;
    qint64 m_globalAnimationTime;
    bool m_delayedLoading;
    bool m_matteEnabled;

    void clear(bool deleteCommandData);
    int size() const { return m_size; }
    const ElementCommand &constCommandAt(int index) const { return m_elementCommands.at(index); }
    ElementCommand &commandAt(int index) { return m_elementCommands[index]; }

private:
    ElementCommand &nextFreeCommand();

    CommandList m_elementCommands;
    int m_size;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(CommandType)

#endif // Q3DS_COMMAND_QUEUE_H
