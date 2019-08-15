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

#include "q3dscommandqueue_p.h"
#include "q3dspresentation.h"
#include "Qt3DSViewerApp.h"

#include <QtCore/qstringlist.h>

ElementCommand::ElementCommand()
    : m_commandType(CommandType_Invalid)
{
}

QString ElementCommand::toString() const
{
    QString ret = QStringLiteral("ElementCommand - Type: %1 Path: '%2' StrVal: '%3' VarVal: '%4'");
    return ret.arg(m_commandType).arg(m_elementPath)
            .arg(m_stringValue).arg(m_variantValue.toString());
}

CommandQueue::CommandQueue()
    : m_visibleChanged(false)
    , m_scaleModeChanged(false)
    , m_stereoModeChanged(false)
    , m_stereoEyeSeparationChanged(false)
    , m_shadeModeChanged(false)
    , m_showRenderStatsChanged(false)
    , m_matteColorChanged(false)
    , m_sourceChanged(false)
    , m_variantListChanged(false)
    , m_globalAnimationTimeChanged(false)
    , m_delayedLoadingChanged(false)
    , m_visible(false)
    , m_scaleMode(Q3DSViewerSettings::ScaleModeCenter)
    , m_stereoMode(Q3DSViewerSettings::StereoModeMono)
    , m_stereoEyeSeparation(0.4)
    , m_shadeMode(Q3DSViewerSettings::ShadeModeShaded)
    , m_showRenderStats(false)
    , m_matteColor(Qt::black)
    , m_delayedLoading(false)
    , m_matteEnabled(false)
    , m_size(0)
{
    qRegisterMetaType<CommandType>();
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath,
                                           CommandType commandType,
                                           const QString &attributeName,
                                           const QVariant &value)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;
    cmd.m_stringValue = attributeName;
    cmd.m_variantValue = value;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath,
                                           CommandType commandType,
                                           const QString &attributeName,
                                           const QVariant &value,
                                           int intValue)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;
    cmd.m_stringValue = attributeName;
    cmd.m_variantValue = value;
    cmd.m_intValues[0] = intValue;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath, CommandType commandType,
                                           const QString &value)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;
    cmd.m_stringValue = value;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath,
                                           CommandType commandType, bool value)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;
    cmd.m_boolValue = value;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath,
                                           CommandType commandType, float value)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;
    cmd.m_floatValue = value;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath,
                                           CommandType commandType, int value0, int value1,
                                           int value2, int value3)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;
    cmd.m_intValues[0] = value0;
    cmd.m_intValues[1] = value1;
    cmd.m_intValues[2] = value2;
    cmd.m_intValues[3] = value3;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath, CommandType commandType,
                                           const QString &stringValue, void *commandData)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;
    cmd.m_stringValue = stringValue;
    cmd.m_data = commandData;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(CommandType commandType, void *commandData)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_data = commandData;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath, CommandType commandType)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;

    return cmd;
}

ElementCommand &CommandQueue::queueCommand(const QString &elementPath, CommandType commandType,
                                           void *commandData)
{
    ElementCommand &cmd = nextFreeCommand();

    cmd.m_commandType = commandType;
    cmd.m_elementPath = elementPath;
    cmd.m_data = commandData;

    return cmd;
}

void CommandQueue::copyCommands(CommandQueue &fromQueue)
{
    m_visibleChanged = m_visibleChanged || fromQueue.m_visibleChanged;
    m_scaleModeChanged = m_scaleModeChanged || fromQueue.m_scaleModeChanged;
    m_stereoModeChanged = m_stereoModeChanged || fromQueue.m_stereoModeChanged;
    m_stereoEyeSeparationChanged
            = m_stereoEyeSeparationChanged || fromQueue.m_stereoEyeSeparationChanged;
    m_shadeModeChanged = m_shadeModeChanged || fromQueue.m_shadeModeChanged;
    m_showRenderStatsChanged = m_showRenderStatsChanged || fromQueue.m_showRenderStatsChanged;
    m_matteColorChanged = m_matteColorChanged || fromQueue.m_matteColorChanged;
    m_sourceChanged = m_sourceChanged || fromQueue.m_sourceChanged;
    m_variantListChanged = m_variantListChanged || fromQueue.m_variantListChanged;
    m_globalAnimationTimeChanged
            = m_globalAnimationTimeChanged || fromQueue.m_globalAnimationTimeChanged;
    m_delayedLoadingChanged = m_delayedLoadingChanged || fromQueue.m_delayedLoadingChanged;
    m_matteEnabledChanged = m_matteEnabledChanged || fromQueue.m_matteEnabledChanged;

    if (fromQueue.m_visibleChanged)
       m_visible = fromQueue.m_visible;
    if (fromQueue.m_scaleModeChanged)
       m_scaleMode = fromQueue.m_scaleMode;
    if (fromQueue.m_stereoModeChanged)
       m_stereoMode = fromQueue.m_stereoMode;
    if (fromQueue.m_stereoEyeSeparationChanged)
       m_stereoEyeSeparation = fromQueue.m_stereoEyeSeparation;
    if (fromQueue.m_shadeModeChanged)
       m_shadeMode = fromQueue.m_shadeMode;
    if (fromQueue.m_showRenderStatsChanged)
       m_showRenderStats = fromQueue.m_showRenderStats;
    if (fromQueue.m_matteColorChanged)
       m_matteColor = fromQueue.m_matteColor;
    if (fromQueue.m_sourceChanged)
       m_source = fromQueue.m_source;
    if (fromQueue.m_variantListChanged)
       m_variantList = fromQueue.m_variantList;
    if (fromQueue.m_globalAnimationTimeChanged)
        m_globalAnimationTime = fromQueue.m_globalAnimationTime;
    if (fromQueue.m_delayedLoadingChanged)
        m_delayedLoading = fromQueue.m_delayedLoading;
    if (fromQueue.m_matteEnabledChanged)
        m_matteEnabled = fromQueue.m_matteEnabled;

    // Pending queue may be synchronized multiple times between queue processing, so let's append
    // to the existing queue rather than clearing it.
    for (int i = 0; i < fromQueue.m_size; i++) {
        const ElementCommand &source = fromQueue.constCommandAt(i);
        switch (source.m_commandType) {
        case CommandType_SetDataInputValue:
            queueCommand(source.m_elementPath, source.m_commandType, source.m_stringValue,
                         source.m_variantValue, source.m_intValues[0]);
            break;
        case CommandType_SetDataInputBatch:
            queueCommand(source.m_elementPath, source.m_commandType, source.m_stringValue,
                         source.m_data);
            fromQueue.commandAt(i).m_data = nullptr; // This queue takes ownership of data
            break;
        case CommandType_SetAttribute:
            queueCommand(source.m_elementPath, source.m_commandType, source.m_stringValue,
                         source.m_variantValue);
            break;
        case CommandType_GoToSlideByName:
        case CommandType_FireEvent:
            queueCommand(source.m_elementPath, source.m_commandType, source.m_stringValue);
            break;
        case CommandType_SetPresentationActive:
            queueCommand(source.m_elementPath, source.m_commandType, source.m_boolValue);
            break;
        case CommandType_GoToTime:
            queueCommand(source.m_elementPath, source.m_commandType, source.m_floatValue);
            break;
        case CommandType_GoToSlide:
        case CommandType_GoToSlideRelative:
        case CommandType_MousePress:
        case CommandType_MouseRelease:
        case CommandType_MouseMove:
        case CommandType_MouseWheel:
        case CommandType_KeyPress:
        case CommandType_KeyRelease:
            queueCommand(source.m_elementPath, source.m_commandType,
                         source.m_intValues[0], source.m_intValues[1],
                         source.m_intValues[2], source.m_intValues[3]);
            break;
        case CommandType_CreateElements:
            queueCommand(source.m_elementPath, source.m_commandType, source.m_stringValue,
                         source.m_data);
            fromQueue.commandAt(i).m_data = nullptr; // This queue takes ownership of data
            break;
        case CommandType_DeleteElements:
        case CommandType_DeleteMaterials:
        case CommandType_DeleteMeshes:
        case CommandType_CreateMeshes:
            queueCommand(source.m_commandType, source.m_data);
            fromQueue.commandAt(i).m_data = nullptr; // This queue takes ownership of data
            break;
        case CommandType_CreateMaterials:
            queueCommand(source.m_elementPath, source.m_commandType, source.m_data);
            fromQueue.commandAt(i).m_data = nullptr; // This queue takes ownership of data
            break;
        case CommandType_RequestSlideInfo:
        case CommandType_UnloadSlide:
        case CommandType_PreloadSlide:
        case CommandType_RequestDataInputs:
        case CommandType_RequestDataOutputs:
            queueCommand(source.m_elementPath, source.m_commandType);
            break;
        default:
            queueCommand(QString(), CommandType_Invalid, false);
            break;
        }
    }
}

// Clears changed states and empties the queue
void CommandQueue::clear(bool deleteCommandData)
{
    m_visibleChanged = false;
    m_scaleModeChanged = false;
    m_stereoModeChanged = false;
    m_stereoEyeSeparationChanged = false;
    m_shadeModeChanged = false;
    m_showRenderStatsChanged = false;
    m_matteColorChanged = false;
    m_sourceChanged = false;
    m_variantListChanged = false;
    m_globalAnimationTimeChanged = false;
    m_delayedLoadingChanged = false;
    m_matteEnabledChanged = false;

    if (deleteCommandData) {
        for (int i = 0; i < m_size; ++i) {
            ElementCommand &cmd = m_elementCommands[i];
            if (cmd.m_data) {
                switch (cmd.m_commandType) {
                case CommandType_CreateElements:
                    delete static_cast<QVector<QHash<QString, QVariant>> *>(cmd.m_data);
                    break;
                case CommandType_DeleteElements:
                case CommandType_CreateMaterials:
                case CommandType_DeleteMaterials:
                case CommandType_DeleteMeshes:
                    delete static_cast<QStringList *>(cmd.m_data);
                    break;
                case CommandType_CreateMeshes: {
                    delete static_cast<QHash<QString, Q3DSViewer::MeshData> *>(cmd.m_data);
                    break;
                }
                case CommandType_SetDataInputBatch: {
                    delete static_cast<QVector<QPair<QString, QVariant>> *>(cmd.m_data);
                    break;
                }
                default:
                    Q_ASSERT(false); // Should never come here
                    break;
                }
                cmd.m_data = nullptr;
            }
        }
    }

    // We do not clear the actual queued commands, those will be reused the next frame
    // To avoid a lot of unnecessary reallocations.
    m_size = 0;
}

ElementCommand &CommandQueue::nextFreeCommand()
{
    m_size++;
    if (m_size > m_elementCommands.size())
        m_elementCommands.append(ElementCommand());
    return m_elementCommands[m_size - 1];
}
