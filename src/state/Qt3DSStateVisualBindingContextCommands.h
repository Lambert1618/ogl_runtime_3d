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
#ifndef QT3DS_STATE_VISUAL_BINDING_CONTEXT_COMMANDS_H
#define QT3DS_STATE_VISUAL_BINDING_CONTEXT_COMMANDS_H
#include "Qt3DSState.h"
#include "foundation/StringTable.h"

namespace qt3ds {
namespace state {

    struct VisualStateCommandTypes
    {
        enum Enum {
            NoVisualStateCommand = 0,
            GotoSlide,
            CallFunction,
            SetAttribute,
            GotoSlideRelative,
            FireEvent,
            PresentationAttribute,
            PlaySound,
        };
    };

    struct SlidePlaybackModes
    {
        enum Enum {
            StopAtEnd = 0,
            Looping,
            PingPong,
            Ping,
            PlaythroughTo,
        };
    };

    struct SGotoSlideData
    {
        Option<SlidePlaybackModes::Enum> m_Mode;
        Option<CRegisteredString> m_PlaythroughTo;
        Option<QT3DSU32> m_StartTime;
        QT3DSF32 m_Rate;
        bool m_Reverse;
        Option<bool> m_Paused;
        SGotoSlideData()
            : m_Rate(1.0f)
            , m_Reverse(false)
        {
        }
    };

    // All the element references in this file need to be absolute, meaning they
    // must begin with a presentationid:.  The is because the state machine exists at the
    // application level where all presentations are pretty much equal and must be referenced
    // by id.

    // Go to a particular slide.

    /*
    a. If the slide attribute references a slide name not present on the time context an error must
    be logged each time the executable would have been run, and the executable ignored.
    b. If the time context is already on the slide referenced, no change is made to the time
    context.
                    The slide does not restart. */
    struct SGotoSlide
    {
        CRegisteredString m_Component;
        CRegisteredString m_Slide;
        SGotoSlideData m_GotoSlideData;
        SGotoSlide(CRegisteredString cmd = CRegisteredString(),
                   CRegisteredString slide = CRegisteredString())
            : m_Component(cmd)
            , m_Slide(slide)
        {
        }
        bool operator==(const SGotoSlide &inOther) const
        {
            return m_Component == inOther.m_Component && m_Slide == inOther.m_Slide;
        }
        template <typename TRemapper>
        void Remap(TRemapper &inRemapper)
        {
            inRemapper.Remap(m_Component);
            inRemapper.Remap(m_Slide);
        }
    };

    /*
    1. A <call.../> executable must have a element="..." attribute that references a behavior
    element in one of the presentation assets for the application.
            a. If the element attribute is missing, or references an element that cannot be found,
    an error must be logged each time the executable would have been run, and the executable
    ignored.
            b. If the element attribute references an element that is not a behavior then an error
    must be logged each time the executable would have been run, and the executable ignored.
    2. A <call.../> executable must have a handler="..." attribute that references a function value
    in the table associated with the behavior element.
            a. If the handler attribute is missing, or references a value that is not a function
    value, an error must be logged each time the executable would have been run, and the executable
    ignored.
    3. A <call.../> executable may have an args="..." attribute that specifies an expression to
    evaluate.
            a. If the result of evaluating this expression is a single value, it is passed as the
    first argument to the behavior's function.
            b. If the result of evaluating this expression is a table, it is treated as a list that
    is unpack'd, the numeric properties sent as arguments in order.
            c. If the result of evaluating this expression is an error, an error message must be
    logged and the executable ignored.
                    The function is not invoked with no parameters.
    */
    struct SCallFunction
    {
        CRegisteredString m_Behavior;
        CRegisteredString m_Handler;
        CRegisteredString m_Arguments;
        SCallFunction(CRegisteredString behavior = CRegisteredString(),
                      CRegisteredString hdler = CRegisteredString(),
                      CRegisteredString args = CRegisteredString())
            : m_Behavior(behavior)
            , m_Handler(hdler)
            , m_Arguments(args)
        {
        }
        bool operator==(const SCallFunction &inOther) const
        {
            return m_Behavior == inOther.m_Behavior && m_Handler == inOther.m_Handler
                && m_Arguments == inOther.m_Arguments;
        }
        template <typename TRemapper>
        void Remap(TRemapper &inRemapper)
        {
            inRemapper.Remap(m_Behavior);
            inRemapper.Remap(m_Handler);
            inRemapper.Remap(m_Arguments);
        }
    };

    /*
    1. A <set-attribute.../> executable must have an element="..." attribute that references an
    element in one of the presentation assets for the application.
            a. If the element attribute is missing, or references an element that cannot be found,
    an error must be logged each time the executable would have been run, and the executable
    ignored.
    2. A <set-attribute.../> executable must have an attribute="..." attribute that references an
    attribute on the referenced element.
            a. If the attribute attribute is missing, or references an attribute not present on the
    element, an error must be logged each time the executable would have been run, and the
    executable ignored.
    3. A <set-attribute.../> executable must have an value="..." attribute that describes the value
    to set.
            a. The contents of this attribute are evaluated as an expression and the result used
    to set the attribute.
                    i. If the result of evaluating this expression does not match the type of the
    attribute on the element then an error must be logged and the executable ignored.
    4. If a single visual state has both <goto-slide/> and <set-attribute/> executables, and the
    slide change affects the same attribute as the set-attribute executable, then the set-attribute
    executable must take effect (be applied after the slide change occurs).
            In the future we may wish to have the order of this interaction controllable by the
    ordering of the executables.
    */
    struct SSetAttribute
    {
        CRegisteredString m_Element;
        CRegisteredString m_Attribute;
        CRegisteredString m_Value;
        SSetAttribute(CRegisteredString elem = CRegisteredString(),
                      CRegisteredString att = CRegisteredString(),
                      CRegisteredString val = CRegisteredString())
            : m_Element(elem)
            , m_Attribute(att)
            , m_Value(val)
        {
        }
        bool operator==(const SSetAttribute &inOther) const
        {
            return m_Element == inOther.m_Element && m_Attribute == inOther.m_Attribute
                && m_Value == inOther.m_Value;
        }

        template <typename TRemapper>
        void Remap(TRemapper &inRemapper)
        {
            inRemapper.Remap(m_Element);
            inRemapper.Remap(m_Attribute);
            inRemapper.Remap(m_Value);
        }
    };

    /*
    4. A rel="..." attribute must have either the value next or prev.
            a. If the rel attribute has a different value an error must be logged each time the
    executable would have been run, and the executable ignored.
            b. A value of next causes the time context to go to the next slide.
                    i. If the time context is at the last slide, and there is no wrap attribute, or
    the wrap attribute does not have a value of true, then no change occurs to the time context.
                    The slide does not restart.
                    ii. If the time context is at the last slide and there exists a wrap attribute
    with a value of true then the time context is taken to the first slide.
            c. A value of prev causes the time context to go to the previous slide.
                    i. If the time context is at the first slide, and there is no wrap attribute, or
    the wrap attribute does not have a value of true, then no change occurs to the time context.
                            The slide does not restart.
                    ii. If the time context is at the last first and there exists a wrap attribute
    with a value of true then the time context is taken to the last slide.
    */
    struct SGotoSlideRelative
    {
        CRegisteredString m_Component;
        enum Enum {
            Next = 0,
            Previous,
            Error,
        };
        Enum m_Direction;
        bool m_Wrap;
        SGotoSlideData m_GotoSlideData;
        SGotoSlideRelative(CRegisteredString comp = CRegisteredString(), Enum dir = Next,
                           bool wrap = false)
            : m_Component(comp)
            , m_Direction(dir)
            , m_Wrap(wrap)
        {
        }
        bool operator==(const SGotoSlideRelative &inOther) const
        {
            return m_Component == inOther.m_Component && m_Direction == inOther.m_Direction
                && m_Wrap == inOther.m_Wrap;
        }
        template <typename TRemapper>
        void Remap(TRemapper &inRemapper)
        {
            inRemapper.Remap(m_Component);
        }
    };

    /*
    1. A <fire-event.../> executable must have an element="..." attribute that references an element
    in one of the presentation assets for the application.
            a. If the element attribute is missing, or references an element that cannot be found,
    an error must be logged each time the executable would have been run, and the executable
    ignored.
    2. A <fire-event.../> executable must have an event="..." attribute that describes the event
    name to fire.
            a. If the event attribute is missing an error must be logged each time the executable
    would have been run, and the executable ignored.
    */
    struct SFireEvent
    {
        CRegisteredString m_Element;
        CRegisteredString m_Event;
        SFireEvent(CRegisteredString elem, CRegisteredString evt)
            : m_Element(elem)
            , m_Event(evt)
        {
        }
        SFireEvent() {}
        bool operator==(const SFireEvent &inOther) const
        {
            return m_Element == inOther.m_Element && m_Event == inOther.m_Event;
        }

        template <typename TRemapper>
        void Remap(TRemapper &inRemapper)
        {
            inRemapper.Remap(m_Element);
            inRemapper.Remap(m_Event);
        }
    };

    struct SPresentationAttribute
    {
        CRegisteredString m_Presentation;
        CRegisteredString m_Attribute;
        CRegisteredString m_Value;
        SPresentationAttribute(CRegisteredString pres, CRegisteredString att, CRegisteredString val)
            : m_Presentation(pres)
            , m_Attribute(att)
            , m_Value(val)
        {
        }
        SPresentationAttribute() {}
        bool operator==(const SPresentationAttribute &inOther) const
        {
            return m_Presentation == inOther.m_Presentation && m_Attribute == inOther.m_Attribute
                && m_Value == inOther.m_Value;
        }

        template <typename TRemapper>
        void Remap(TRemapper &inRemapper)
        {
            inRemapper.Remap(m_Presentation);
            inRemapper.Remap(m_Attribute);
            inRemapper.Remap(m_Value);
        }
    };

    struct SPlaySound
    {
        CRegisteredString m_SoundFilePath;
        SPlaySound(CRegisteredString inSoundFilePath)
            : m_SoundFilePath(inSoundFilePath)
        {
        }
        SPlaySound() {}
        bool operator==(const SPlaySound &inOther) const
        {
            return m_SoundFilePath == inOther.m_SoundFilePath;
        }

        template <typename TRemapper>
        void Remap(TRemapper &inRemapper)
        {
            inRemapper.Remap(m_SoundFilePath);
        }
    };

    // defined in UICStateVisualBindingContextValues.h
    struct SVisualStateCommand;
}
}
#endif
