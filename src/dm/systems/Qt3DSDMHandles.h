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
#ifndef QT3DSDM_HANDLES_H
#define QT3DSDM_HANDLES_H

#include <QMetaType>

namespace qt3dsdm {

class IDataCore;

class CDataModelHandle
{
    int m_Handle;

public:
    inline CDataModelHandle(int inHandle = 0);
    inline CDataModelHandle(const CDataModelHandle &other);
    virtual ~CDataModelHandle();

    // Operators
    inline CDataModelHandle &operator=(const CDataModelHandle &other);
    inline bool operator==(const CDataModelHandle &other) const;
    inline bool operator<(const CDataModelHandle &other) const;
    inline operator int() const { return GetHandleValue(); }

    // Use
    inline bool Valid() const { return m_Handle != 0; }
    int GetHandleValue() const { return m_Handle; }
};

inline CDataModelHandle::CDataModelHandle(int inHandle)
    : m_Handle(inHandle)
{
}
inline CDataModelHandle::CDataModelHandle(const CDataModelHandle &other)
    : m_Handle(other.m_Handle)
{
}
inline CDataModelHandle::~CDataModelHandle()
{
}

inline CDataModelHandle &CDataModelHandle::operator=(const CDataModelHandle &other)
{
    if (this != &other) {
        m_Handle = other.m_Handle;
    }
    return *this;
}

inline bool CDataModelHandle::operator==(const CDataModelHandle &other) const
{
    return m_Handle == other.m_Handle;
}

inline bool CDataModelHandle::operator<(const CDataModelHandle &other) const
{
    return m_Handle < other.m_Handle;
}

class Qt3DSDMInstanceHandle : public CDataModelHandle
{
public:
    Qt3DSDMInstanceHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMInstanceHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMInstanceHandle(const Qt3DSDMInstanceHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMInstanceHandle &operator=(const Qt3DSDMInstanceHandle &inOther)
    {
        return static_cast<Qt3DSDMInstanceHandle &>(CDataModelHandle::operator=(inOther));
    }

    inline bool operator==(const Qt3DSDMInstanceHandle &other) const
    {
        return CDataModelHandle::operator==(other);
    }
};

typedef std::vector<Qt3DSDMInstanceHandle> TInstanceHandleList;

class Qt3DSDMPropertyHandle : public CDataModelHandle
{
public:
    Qt3DSDMPropertyHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMPropertyHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMPropertyHandle(const Qt3DSDMInstanceHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMPropertyHandle &operator=(const Qt3DSDMPropertyHandle &inOther)
    {
        return static_cast<Qt3DSDMPropertyHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMPropertyHandle> TPropertyHandleList;

class Qt3DSDMSlideHandle : public CDataModelHandle
{
public:
    Qt3DSDMSlideHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMSlideHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMSlideHandle(const Qt3DSDMSlideHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMSlideHandle &operator=(const Qt3DSDMSlideHandle &inOther)
    {
        return static_cast<Qt3DSDMSlideHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMSlideHandle> TSlideHandleList;

class Qt3DSDMSlideGraphHandle : public CDataModelHandle
{
public:
    Qt3DSDMSlideGraphHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMSlideGraphHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMSlideGraphHandle(const Qt3DSDMSlideGraphHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMSlideGraphHandle &operator=(const Qt3DSDMSlideGraphHandle &inOther)
    {
        return static_cast<Qt3DSDMSlideGraphHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMSlideGraphHandle> TSlideGraphHandleList;

class Qt3DSDMAnimationHandle : public CDataModelHandle
{
public:
    Qt3DSDMAnimationHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMAnimationHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMAnimationHandle(const Qt3DSDMAnimationHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMAnimationHandle &operator=(const Qt3DSDMAnimationHandle &inOther)
    {
        return static_cast<Qt3DSDMAnimationHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMAnimationHandle> TAnimationHandleList;

class Qt3DSDMKeyframeHandle : public CDataModelHandle
{
public:
    Qt3DSDMKeyframeHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMKeyframeHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMKeyframeHandle(const Qt3DSDMKeyframeHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMKeyframeHandle &operator=(const Qt3DSDMKeyframeHandle &inOther)
    {
        return static_cast<Qt3DSDMKeyframeHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMKeyframeHandle> TKeyframeHandleList;

class Qt3DSDMActionHandle : public CDataModelHandle
{
public:
    Qt3DSDMActionHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMActionHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMActionHandle(const Qt3DSDMActionHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMActionHandle &operator=(const Qt3DSDMActionHandle &inOther)
    {
        return static_cast<Qt3DSDMActionHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMActionHandle> TActionHandleList;

class Qt3DSDMHandlerArgHandle : public CDataModelHandle
{
public:
    Qt3DSDMHandlerArgHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMHandlerArgHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMHandlerArgHandle(const Qt3DSDMHandlerArgHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }
    Qt3DSDMHandlerArgHandle &operator=(const Qt3DSDMHandlerArgHandle &inOther)
    {
        return static_cast<Qt3DSDMHandlerArgHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMHandlerArgHandle> THandlerArgHandleList;

class Qt3DSDMEventHandle : public CDataModelHandle
{
public:
    Qt3DSDMEventHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMEventHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMEventHandle(const Qt3DSDMEventHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMEventHandle &operator=(const Qt3DSDMEventHandle &inOther)
    {
        return static_cast<Qt3DSDMEventHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMEventHandle> TEventHandleList;

class Qt3DSDMHandlerHandle : public CDataModelHandle
{
public:
    Qt3DSDMHandlerHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMHandlerHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMHandlerHandle(const Qt3DSDMHandlerHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMHandlerHandle &operator=(const Qt3DSDMHandlerHandle &inOther)
    {
        return static_cast<Qt3DSDMHandlerHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMHandlerHandle> THandlerHandleList;

class Qt3DSDMHandlerParamHandle : public CDataModelHandle
{
public:
    Qt3DSDMHandlerParamHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMHandlerParamHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMHandlerParamHandle(const Qt3DSDMHandlerParamHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMHandlerParamHandle &operator=(const Qt3DSDMHandlerParamHandle &inOther)
    {
        return static_cast<Qt3DSDMHandlerParamHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMHandlerParamHandle> THandlerParamHandleList;

class Qt3DSDMMetaDataPropertyHandle : public CDataModelHandle
{
public:
    Qt3DSDMMetaDataPropertyHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMMetaDataPropertyHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMMetaDataPropertyHandle(const Qt3DSDMMetaDataPropertyHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMMetaDataPropertyHandle &operator=(const Qt3DSDMMetaDataPropertyHandle &inOther)
    {
        return static_cast<Qt3DSDMMetaDataPropertyHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMMetaDataPropertyHandle> TMetaDataPropertyHandleList;

class Qt3DSDMCategoryHandle : public CDataModelHandle
{
public:
    Qt3DSDMCategoryHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMCategoryHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMCategoryHandle(const Qt3DSDMCategoryHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }
    Qt3DSDMCategoryHandle &operator=(const Qt3DSDMCategoryHandle &inOther)
    {
        return static_cast<Qt3DSDMCategoryHandle &>(CDataModelHandle::operator=(inOther));
    }
};

typedef std::vector<Qt3DSDMCategoryHandle> TCategoryHandleList;

class Qt3DSDMGuideHandle : public CDataModelHandle
{
public:
    Qt3DSDMGuideHandle(int inHandle = 0)
        : CDataModelHandle(inHandle)
    {
    }

    Qt3DSDMGuideHandle(const CDataModelHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }

    Qt3DSDMGuideHandle(const Qt3DSDMCategoryHandle &inOther)
        : CDataModelHandle(inOther)
    {
    }
    Qt3DSDMGuideHandle &operator=(const Qt3DSDMGuideHandle &inOther)
    {
        CDataModelHandle::operator=(inOther);
        return *this;
    }
};

typedef std::vector<Qt3DSDMGuideHandle> TGuideHandleList;
}

Q_DECLARE_METATYPE(qt3dsdm::Qt3DSDMSlideHandle);

#endif
