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

#include "Qt3DSElementSystem.h"

namespace Q3DStudio {
class ILogicManager;
}

namespace qt3ds {
namespace runtime {
    class ILogicSystem;
    class IAnimationSystem;

    struct SSlideKey
    {
        element::SElement *m_Component;
        QT3DSU32 m_Index;
        SSlideKey()
            : m_Component(NULL)
            , m_Index(0)
        {
        }
        SSlideKey(element::SElement &inElem, QT3DSU32 inIdx)
            : m_Component(&inElem)
            , m_Index(inIdx)
        {
        }

        bool IsValid() const { return m_Component != NULL; }

        bool operator==(const SSlideKey &inOther) const
        {
            return m_Component == inOther.m_Component && m_Index == inOther.m_Index;
        }
    };

    struct PlayMode
    {
        enum Enum {
            Looping = 0,
            StopAtEnd,
            PingPong,
            Ping,
            PlayThroughTo,
        };
    };

    struct SSlidePlayInformation
    {
        PlayMode::Enum m_PlayMode;
        bool m_Paused;
        QT3DSU8 m_PlayThroughTo;

        SSlidePlayInformation()
            : m_PlayMode(PlayMode::Looping)
            , m_Paused(false)
            , m_PlayThroughTo(0xFF)
        {
        }
    };

    struct SSlideAnimAction
    {
        QT3DSI32 m_Id;
        bool m_Active;
        bool m_IsAnimation;
        SSlideAnimAction()
            : m_Id(0)
            , m_Active(false)
            , m_IsAnimation(false)
        {
        }
        SSlideAnimAction(QT3DSI32 inId, bool inActive, bool inAnimation)
            : m_Id(inId)
            , m_Active(inActive)
            , m_IsAnimation(inAnimation)
        {
        }
    };

    // Throughout this object, a slide index of 0xFF means invalid slide index.
    class ISlideSystem : public NVRefCounted
    {
    public:
        static QT3DSU8 InvalidSlideIndex() { return 0xFF; }
        // Building out a dataset.
        // Returns the index
        // This slide is now the active cursor.  The rest of the building functions will implicitly
        // refer
        // to the last added slide
        // Playthroughto is either an index of the new slide or -1 meaning no playthough value.
        virtual QT3DSU32 AddSlide(element::SElement &inComponent, const char8_t *inName,
                               PlayMode::Enum inPlayMode = PlayMode::Looping, bool inPaused = false,
                               QT3DSU8 inPlaythroughTo = InvalidSlideIndex(), QT3DSU32 inMinTime = 0,
                               QT3DSU32 inMaxTime = 0) = 0;

        virtual void SetSlideMaxTime(QT3DSU32 inMaxTime) = 0;
        virtual void AddSlideElement(element::SElement &inElement, bool inActive) = 0;
        virtual bool addSlideElement(element::SElement &inComponent, int slideIndex,
                                     element::SElement &inElement, bool eyeBall) = 0;
        virtual void removeElement(element::SElement &inComponent,
                                   element::SElement &inElement) = 0;
        virtual void AddSlideAttribute(Q3DStudio::SAttributeKey inKey,
                                       Q3DStudio::UVariant inValue) = 0;
        virtual SSlideAnimAction *AddSlideAnimAction(bool inAnimation, QT3DSI32 inIndex,
                                                     bool inActive) = 0;
        virtual void AddSourcePath(const char8_t *path) = 0;
        virtual void AddSourcePath(SSlideKey key, const char8_t *path) = 0;
        virtual void AddSubPresentation(const char8_t *subpresentationId) = 0;
        virtual QVector<QString> GetSourcePaths(SSlideKey inKey) = 0;
        virtual QVector<QString> GetSubPresentations(SSlideKey inKey) = 0;
        virtual void setIsActiveSlide(SSlideKey inKey, bool active) = 0;
        virtual bool isActiveSlide(SSlideKey inKey) const = 0;
        virtual void setUnloadSlide(SSlideKey inKey, bool unload) = 0;
        virtual bool isUnloadSlideSet(SSlideKey inKey) const = 0;

        // Using the dataset
        virtual void InitializeDynamicKeys(SSlideKey inKey,
                                           IAnimationSystem &inAnimationSystem) const = 0;
        virtual void ExecuteSlide(SSlideKey inKey, IAnimationSystem &inAnimationSystem,
                                  ILogicSystem &inLogicManager) = 0;
        virtual void RollbackSlide(SSlideKey inKey, IAnimationSystem &inAnimationSystem,
                                   ILogicSystem &inLogicManager) = 0;
        virtual QT3DSU8 FindSlide(element::SElement &inComponent,
                               const char8_t *inSlideName) const = 0;
        virtual QT3DSU8 FindSlide(element::SElement &inComponent, QT3DSU32 inSlideHashName) const = 0;
        virtual const char8_t *GetSlideName(SSlideKey inKey) const = 0;
        virtual QT3DSU8 GetPlaythroughToSlideIndex(SSlideKey inKey) const = 0;
        virtual bool isElementInSlide(const element::SElement &element,
                                      element::SElement &component,
                                      int slideIndex) const = 0;

        static ISlideSystem &CreateSlideSystem(NVFoundationBase &inFnd, IStringTable &inStrTable,
                                               IElementAllocator &inElemAllocator);
    };
}
}
