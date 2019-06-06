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
#ifndef HANDLESYSTEMIMPLH
#define HANDLESYSTEMIMPLH

#include <unordered_map>

namespace qt3dsdm {

class CHandleObject
{
public:
    // Type of CHandleObject
    // Each subclass needs to specify what type it is
    // This is used to avoid dynamic_cast (RTTI) which is expensive
    enum EHandleObjectType {
        EHandleObjectTypeUnknown = 0,
        EHandleObjectTypeCDataModelInstance,
        EHandleObjectTypeCDataModelPropertyDefinitionObject,
        EHandleObjectTypeSAnimationTrack,
        EHandleObjectTypeSKeyframe,
        EHandleObjectTypeSSlide,
        EHandleObjectTypeSSlideGraph,
        EHandleObjectTypeAction,
        EHandleObjectTypeActionHandlerArgument,
        EHandleObjectTypeCustomProperty,
        EHandleObjectTypeEvent,
        EHandleObjectTypeCustomHandler,
        EHandleObjectTypeHandlerParam,
        EHandleObjectTypeEnd,
    };

    CHandleObject(int inHandle = 0)
        : m_Handle(inHandle)
    {
    }
    virtual ~CHandleObject() {}

    virtual EHandleObjectType GetType() = 0;

    int m_Handle;

private: //noncopyable
    CHandleObject(const CHandleObject&) = delete;
    CHandleObject& operator=(const CHandleObject&) = delete;

};

typedef std::shared_ptr<CHandleObject> THandleObjectPtr;

// Note that maps don't need to copy their objects.
typedef std::unordered_map<int, THandleObjectPtr> THandleObjectMap;
typedef std::pair<int, THandleObjectPtr> THandleObjectPair;

class IHandleBase
{
public:
    /**
     *	Check whether a given handle exists
     */
    virtual bool HandleValid(int inHandle) const = 0;
};

struct CHandleBase : public IHandleBase
{
    THandleObjectMap m_Objects;
    int m_NextId;

    CHandleBase()
        : m_NextId(1)
    {
    }
    CHandleBase(const CHandleBase &inOther)
        : m_Objects(inOther.m_Objects)
        , m_NextId(inOther.m_NextId)
    {
    }

    CHandleBase &operator=(const CHandleBase &inOther)
    {
        m_Objects = inOther.m_Objects;
        m_NextId = inOther.m_NextId;
        return *this;
    }

    // IHandleBase
    bool HandleValid(int inHandle) const override
    {
        return m_Objects.find(inHandle) != m_Objects.end();
    }

    template <typename T>
    static inline bool HandleObjectValid(int inHandle, const THandleObjectMap &inMap)
    {
        THandleObjectMap::const_iterator theIter = inMap.find(inHandle);
        if (theIter != inMap.end()) {
            if (theIter->second->GetType() == T::s_Type)
                return true;
        }
        return false;
    }

    template <typename T>
    static inline const T *GetHandleObject(int inHandle, const THandleObjectMap &inMap)
    {
        THandleObjectMap::const_iterator theIter = inMap.find(inHandle);
        if (theIter != inMap.end()) {
            if (theIter->second->GetType() == T::s_Type)
                return static_cast<const T *>(theIter->second.get());
        }
        return NULL;
    }

    template <typename ObjectType, typename HandleType>
    static inline void MaybeAddObject(const std::pair<int, THandleObjectPtr> &inItem,
                                      std::vector<HandleType> &outHandles)
    {
        if (inItem.second->GetType() == ObjectType::s_Type)
            outHandles.push_back(HandleType(inItem.first));
    }

    static void EraseHandle(int inHandle, THandleObjectMap &inObjects)
    {
        THandleObjectMap::iterator theFind = inObjects.find(inHandle);
        if (theFind != inObjects.end())
            inObjects.erase(theFind);
    }

    // Return the next unused id.  There are no guarantees whether positive or negative; what this
    // will
    // guarantee is that it isn't currently used in the object map and it is non-zero.
    int GetNextId()
    {
        do {
            ++m_NextId;
        } while (m_Objects.find(m_NextId) != m_Objects.end());
        return m_NextId;
    }
};
}

#endif
