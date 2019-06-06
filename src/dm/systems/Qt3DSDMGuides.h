/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DSDM_GUIDES_H
#define QT3DSDM_GUIDES_H
#include "Qt3DSDMDataTypes.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "Qt3DSDMSignals.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMTransactions.h"
#include <functional>

namespace qt3dsdm {
struct GuideDirections
{
    enum Enum {
        UnknownDirection = 0,
        Vertical,
        Horizontal,
    };
};

struct SGuideInfo
{
    qt3ds::QT3DSF32 m_Position;
    GuideDirections::Enum m_Direction;
    qt3ds::QT3DSI32 m_Width;
    SGuideInfo(qt3ds::QT3DSF32 pos = 0.0f, GuideDirections::Enum dir = GuideDirections::UnknownDirection,
               qt3ds::QT3DSI32 width = 1)
        : m_Position(pos)
        , m_Direction(dir)
        , m_Width(width)
    {
    }
};

class IGuideSystem : public ITransactionProducer
{
protected:
    virtual ~IGuideSystem() {}
public:
    friend class std::shared_ptr<IGuideSystem>;

    virtual Qt3DSDMGuideHandle CreateGuide() = 0;
    virtual void SetGuideInfo(Qt3DSDMGuideHandle inGuideHandle, const SGuideInfo &info) = 0;
    virtual SGuideInfo GetGuideInfo(Qt3DSDMGuideHandle inGuideHandle) const = 0;
    virtual void DeleteGuide(Qt3DSDMGuideHandle inGuideHandle) = 0;
    virtual TGuideHandleList GetAllGuides() const = 0;
    virtual bool IsGuideValid(Qt3DSDMGuideHandle inGuideHandle) const = 0;
    // No real effect on datamodel because you can still create guides when they are locked.
    // Just used in the UI.
    virtual bool AreGuidesEditable() const = 0;
    virtual void SetGuidesEditable(bool val) = 0;

    // Undo/Redo
    void SetConsumer(std::shared_ptr<ITransactionConsumer> inConsumer) override = 0;

    // These are events coming from undo/redo operations, not events coming directly from the
    // modification of the guides
    virtual TSignalConnectionPtr
    ConnectGuideCreated(const std::function<void(Qt3DSDMGuideHandle, SGuideInfo)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectGuideDestroyed(
        const std::function<void(Qt3DSDMGuideHandle, SGuideInfo)> &inCallback) = 0;
    virtual TSignalConnectionPtr ConnectGuideModified(
        const std::function<void(Qt3DSDMGuideHandle, SGuideInfo)> &inCallback) = 0;

    // Signal happens immediately instead of on undo/redo, used for live-update of the inspector
    // palette
    virtual TSignalConnectionPtr ConnectGuideModifiedImmediate(
        const std::function<void(Qt3DSDMGuideHandle, SGuideInfo)> &inCallback) = 0;

    static std::shared_ptr<IGuideSystem> CreateGuideSystem();
};
}

#endif
