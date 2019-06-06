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
#ifndef QT3DSDM_ERRORS_H
#define QT3DSDM_ERRORS_H
#include "Qt3DSDMWindowsCompatibility.h"

namespace qt3dsdm {
class Qt3DSDMError : public std::exception
{
public:
    Qt3DSDMError(const wchar_t *inMessage)
        : std::exception()
    {
        wcscpy_s(m_Message, inMessage);
    }
    wchar_t m_Message[1024];
};

class HandleExists : public Qt3DSDMError
{
public:
    HandleExists(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class PropertyNotFound : public Qt3DSDMError
{
public:
    PropertyNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class PropertyExists : public Qt3DSDMError
{
public:
    PropertyExists(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class DuplicateInstanceName : public Qt3DSDMError
{
public:
    DuplicateInstanceName(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class InstanceNotFound : public Qt3DSDMError
{
public:
    InstanceNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class ValueTypeError : public Qt3DSDMError
{
public:
    ValueTypeError(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class SerializationError : public Qt3DSDMError
{
public:
    SerializationError(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class SlideNotFound : public Qt3DSDMError
{
public:
    SlideNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class SlideExists : public Qt3DSDMError
{
public:
    SlideExists(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class SlideDerivationError : public Qt3DSDMError
{
public:
    SlideDerivationError(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class SlideChildNotFoundError : public Qt3DSDMError
{
public:
    SlideChildNotFoundError(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class SlideGraphNotFound : public Qt3DSDMError
{
public:
    SlideGraphNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class SlideGraphExists : public Qt3DSDMError
{
public:
    SlideGraphExists(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class PropertyLinkError : public Qt3DSDMError
{
public:
    PropertyLinkError(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class RearrangeSlideArgumentsMustNotBeZero : public Qt3DSDMError
{
public:
    RearrangeSlideArgumentsMustNotBeZero(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class AnimationNotFound : public Qt3DSDMError
{
public:
    AnimationNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class AnimationExists : public Qt3DSDMError
{
public:
    AnimationExists(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class AnimationKeyframeTypeError : public Qt3DSDMError
{
public:
    AnimationKeyframeTypeError(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class AnimationKeyframeNotFound : public Qt3DSDMError
{
public:
    AnimationKeyframeNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class AnimationEvaluationError : public Qt3DSDMError
{
public:
    AnimationEvaluationError(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class ActionNotFound : public Qt3DSDMError
{
public:
    ActionNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class ActionExists : public Qt3DSDMError
{
public:
    ActionExists(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class HandlerArgumentNotFound : public Qt3DSDMError
{
public:
    HandlerArgumentNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class HandlerArgumentExists : public Qt3DSDMError
{
public:
    HandlerArgumentExists(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class CustomPropertyNotFound : public Qt3DSDMError
{
public:
    CustomPropertyNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class CustomEventNotFound : public Qt3DSDMError
{
public:
    CustomEventNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class CustomHandlerNotFound : public Qt3DSDMError
{
public:
    CustomHandlerNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class CustomHandlerParamNotFound : public Qt3DSDMError
{
public:
    CustomHandlerParamNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};

class AttributeTypeNotFound : public Qt3DSDMError
{
public:
    AttributeTypeNotFound(const wchar_t *inMessage)
        : Qt3DSDMError(inMessage)
    {
    }
};
}

#endif
