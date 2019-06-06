/****************************************************************************
**
** Copyright (C) 2013 - 2016 NVIDIA Corporation.
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

#include "Qt3DSAudioPlayerImpl.h"
#include "foundation/Qt3DSLogging.h"
#include <QDebug>

#ifdef PLATFORM_HAS_QT_MULTIMEDIA_LIB
#include <QSound>
#endif // PLATFORM_HAS_QT_MULTIMEDIA_LIB

namespace Q3DSViewer {

Qt3DSAudioPlayerImpl::Qt3DSAudioPlayerImpl()
{
}

Qt3DSAudioPlayerImpl::~Qt3DSAudioPlayerImpl()
{
}

bool Qt3DSAudioPlayerImpl::PlaySoundFile(const char *inFilePath)
{
#ifdef PLATFORM_HAS_QT_MULTIMEDIA_LIB
    QSound::play(QString(inFilePath));
    return true;
#else
    Q_UNUSED(inFilePath)
    qCWarning(qt3ds::WARNING, "Qt3DSAudioPlayerImpl::PlaySoundFile: no QT multimedia lib on this platform");
    return false;
#endif // PLATFORM_HAS_QT_MULTIMEDIA_LIB
}
}
