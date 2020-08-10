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
#ifndef SLIDECOREH
#define SLIDECOREH
#include "HandleSystemBase.h"
#include "Qt3DSDMSlideCore.h"
#include "SimpleDataCore.h"
#include "Qt3DSDMStringTable.h"
#include <unordered_map>
#include <QtCore/qdebug.h>

namespace qt3dsdm {

typedef TInstancePropertyPair TSlideInstancePropertyPair;
typedef std::unordered_map<TSlideInstancePropertyPair, SInternValue > TSlideEntryHash;

// Abstract access to these objects a little bit because in the future we are going to
// reorganize the data such that getting a defined set of properties for a single instance is
// very fast.
struct SSlide : public CHandleObject
{
    SSlide()
        : m_Instance(0)
        , m_Parent(0)
    {
    }
    SSlide(int inHandle, int inInstance)
        : CHandleObject(inHandle)
        , m_Instance(inInstance)
        , m_Parent(0)
        , m_Time(0)
    {
    }
    int m_Instance;
    int m_Parent;
    TIntList m_Children;
    TSlideEntryHash m_Properties;
    long m_Time;

    // Returns true if it was inserted, false if the property value was set.
    bool SetInstancePropertyValue(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                  const SInternValue &inValue)
    {
        TSlideInstancePropertyPair theKey(inInstance.GetHandleValue(), inProperty.GetHandleValue());
        std::pair<TSlideEntryHash::iterator, bool> insertResult =
            m_Properties.insert(std::make_pair(theKey, inValue));

        if (insertResult.second == false)
            insertResult.first->second = inValue;
        return insertResult.second;
    }
    // Returns true if the property was deleted
    bool RemoveInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty)
    {
        TSlideInstancePropertyPair theKey(inInstance.GetHandleValue(), inProperty.GetHandleValue());
        TSlideEntryHash::iterator find(m_Properties.find(theKey));
        if (find != m_Properties.end()) {
            m_Properties.erase(find);
            return true;
        }
        return false;
    }
    // Return a pointer to out property value.  This allows quicker checks for isset and such
    SInternValue *GetInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                           Qt3DSDMPropertyHandle inProperty) const
    {
        const TSlideInstancePropertyPair theKey(inInstance.GetHandleValue(), inProperty.GetHandleValue());
        TSlideEntryHash::const_iterator find(m_Properties.find(theKey));
        if (find != m_Properties.end())
            return const_cast<SInternValue *>(&find->second);
        return NULL;
    }

    void GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inInstance,
                                           TPropertyHandleValuePairList &outProperties) const
    {
        for (TSlideEntryHash::const_iterator theIter = m_Properties.begin(),
                                             theEnd = m_Properties.end();
             theIter != theEnd; ++theIter) {
            if (theIter->first.first == inInstance)
                outProperties.push_back(
                    std::make_pair(theIter->first.second, theIter->second.GetValue()));
        }
    }

    bool HasProperty(std::function<bool(const TSlideEntry &)> inPredicate) const
    {
        for (TSlideEntryHash::const_iterator theIter = m_Properties.begin(),
                                             theEnd = m_Properties.end();
             theIter != theEnd; ++theIter) {
            if (inPredicate(TSlideEntry(theIter->first.first, theIter->first.second,
                                        theIter->second.GetValue())))
                return true;
        }
        return false;
    }

    void DeleteSlideEntries(TSlideEntryList &outList,
                            std::function<bool(const TSlideEntry &)> inPredicate)
    {
        for (TSlideEntryHash::const_iterator theIter = m_Properties.begin(),
                                             theEnd = m_Properties.end();
             theIter != theEnd; ++theIter) {
            if (inPredicate(TSlideEntry(theIter->first.first, theIter->first.second,
                                        theIter->second.GetValue()))) {
                outList.push_back(TSlideEntry(theIter->first.first, theIter->first.second,
                                              theIter->second.GetValue()));
            }
        }
        DeleteEntriesFromList(outList);
    }

    void DeleteEntriesFromList(const TSlideEntryList &inList)
    {
        for (size_t idx = 0, end = inList.size(); idx < end; ++idx)
            m_Properties.erase(
                TSlideInstancePropertyPair(std::get<0>(inList[idx]), std::get<1>(inList[idx])));
    }

    void InsertSlideEntries(const TSlideEntryList &inList, IStringTable &inStringTable)
    {
        for (size_t idx = 0, end = inList.size(); idx < end; ++idx) {
            TSlideInstancePropertyPair theKey(std::get<0>(inList[idx]), std::get<1>(inList[idx]));

            if (m_Properties.find(theKey) != m_Properties.end()) {
                // The only known case when this condition happens is when DnD a sub-presentation to
                // the scene as a texture rect then undoing.
                qWarning() << __FUNCTION__ << ": Instance/Property Pair" << theKey
                                           << "already exists, erasing it.";
                m_Properties.erase(theKey);
            }

            m_Properties.insert(
                std::make_pair(theKey, SInternValue(std::get<2>(inList[idx]), inStringTable)));
        }
    }

    // Convert to the older, straight forward representation of the
    // data in this slide.
    void ToSlideEntryList(TSlideEntryList &outList) const
    {
        for (TSlideEntryHash::const_iterator theIter = m_Properties.begin(),
                                             theEnd = m_Properties.end();
             theIter != theEnd; ++theIter)
            outList.push_back(TSlideEntry(theIter->first.first, theIter->first.second,
                                          theIter->second.GetValue()));
    }

    void FromSlideEntryList(const TSlideEntryList &inList, IStringTable &inStringTable)
    {
        using namespace std;
        m_Properties.clear();
        for (TSlideEntryList::const_iterator theIter = inList.begin(), theEnd = inList.end();
             theIter != theEnd; ++theIter)
            SetInstancePropertyValue(get<0>(*theIter), get<1>(*theIter),
                                     SInternValue(get<2>(*theIter), inStringTable));
    }

    // result is the instance, property, myvalue, othervalue
    void IntersectProperties(const SSlide &inOther,
                             std::function<void(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle,
                                                  SInternValue, SInternValue)>
                                 inResult) const
    {
        for (TSlideEntryHash::const_iterator theIter = m_Properties.begin(),
                                             theEnd = m_Properties.end();
             theIter != theEnd; ++theIter) {
            SInternValue *otherValue =
                inOther.GetInstancePropertyValue(theIter->first.first, theIter->first.second);
            if (otherValue)
                inResult(theIter->first.first, theIter->first.second, theIter->second, *otherValue);
        }
    }

    // Call the predicate, if it returns true set the property for all properties.
    // This allows a third party to manipulate the property values during the process.
    void SetPropertyValuesIf(
        IStringTable &inStringTable,
        std::function<bool(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, SValue &)> inPredIntercept)
    {
        for (TSlideEntryHash::iterator theIter = m_Properties.begin(), theEnd = m_Properties.end();
             theIter != theEnd; ++theIter) {
            SValue theValue(theIter->second.GetValue());
            if (inPredIntercept(theIter->first.first, theIter->first.second, theValue))
                theIter->second = SInternValue(theValue, inStringTable);
        }
    }

    void
    ClearPropertiesIf(std::function<bool(Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle)> inPredicate)
    {
        bool foundOne;
        do {
            foundOne = false;
            for (TSlideEntryHash::iterator theIter = m_Properties.begin(),
                                           theEnd = m_Properties.end();
                 theIter != theEnd; ++theIter) {
                if (inPredicate(theIter->first.first, theIter->first.second)) {
                    foundOne = true;
                    m_Properties.erase(theIter->first);
                    break;
                }
            }

        } while (foundOne);
    }

    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeSSlide;
    EHandleObjectType GetType() override { return s_Type; }
};

class CSimpleSlideCore : public CHandleBase, public ISlideCore
{
    TStringTablePtr m_StringTable;

public: // use
    CSimpleSlideCore(TStringTablePtr inStrTable)
        : m_StringTable(inStrTable)
    {
    }

    TStringTablePtr GetStringTablePtr() const override { return m_StringTable; }
    IStringTable &GetStringTable() const override { return *m_StringTable.get(); }
    Qt3DSDMSlideHandle CreateSlide(Qt3DSDMInstanceHandle inInstance) override;
    Qt3DSDMInstanceHandle GetSlideInstance(Qt3DSDMSlideHandle inSlide) const override;
    Qt3DSDMSlideHandle GetSlideByInstance(Qt3DSDMInstanceHandle inInstance) const override;
    void DeleteSlide(Qt3DSDMSlideHandle inSlide, TInstanceHandleList &outInstances) override;
    void GetSlides(TSlideHandleList &outSlides) const override;

    long GetSlideTime(Qt3DSDMSlideHandle inSlide) const override;
    void SetSlideTime(Qt3DSDMSlideHandle inSlide, long inNewTime) override;

    void DeriveSlide(Qt3DSDMSlideHandle inSlide, Qt3DSDMSlideHandle inParent, int inIndex = -1) override;
    Qt3DSDMSlideHandle GetParentSlide(Qt3DSDMSlideHandle inSlide) const override;
    void GetChildSlides(Qt3DSDMSlideHandle inSlide, TSlideHandleList &outChildren) const override;
    int GetChildIndex(Qt3DSDMSlideHandle inParent, Qt3DSDMSlideHandle inChild) const override;

    bool GetInstancePropertyValue(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inHandle,
                                  Qt3DSDMPropertyHandle inProperty, SValue &outValue) const override;
    void SetInstancePropertyValue(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inHandle,
                                  Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override;

    // Return the slide this property should be set on, along with the previous value if any.
    // Set the value on the slide.
    std::pair<SSlide *, SInternValue *>
    ResolveSetInstancePropertyValue(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inHandle,
                                    Qt3DSDMPropertyHandle inProperty);
    void ForceSetInstancePropertyValue(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inHandle,
                                       Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override;
    void forceSetInstancePropertyValueOnAllSlides(Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty,
                                                  const SValue &inValue) override;

    bool GetSpecificInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, SValue &outValue) const override;

    void GetSpecificInstancePropertyValues(Qt3DSDMSlideHandle inSlide,
                                           Qt3DSDMInstanceHandle inInstance,
                                           TPropertyHandleValuePairList &outValues) override;

    void GetSlidePropertyEntries(Qt3DSDMSlideHandle inSlide, TSlideEntryList &outEntries) const override;

    void PushPropertyValueToChildren(Qt3DSDMSlideHandle inParent, Qt3DSDMInstanceHandle inHandle,
                                     Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override;

    void GetIntersectingProperties(Qt3DSDMSlideHandle inSlide1, Qt3DSDMSlideHandle inSlide2,
                                   TSlideEntryList &outEntries) const override;
    void PushIntersectingProperties(Qt3DSDMSlideHandle inSlide1, Qt3DSDMSlideHandle inSlide2,
                                    Qt3DSDMSlideHandle inDestination) override;

    void ClearChildrenPropertyValues(Qt3DSDMSlideHandle inParent, Qt3DSDMInstanceHandle inHandle,
                                     Qt3DSDMPropertyHandle inProperty) override;

    void DeleteAllInstanceEntries(Qt3DSDMInstanceHandle inHandle) override;
    void DeleteAllPropertyEntries(Qt3DSDMPropertyHandle inHandle) override;
    void DeleteAllInstancePropertyEntries(const TInstanceHandleList &inInstances,
                                          const TPropertyHandleList &inProperties) override;

    bool ContainsProperty(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inHandle,
                          Qt3DSDMPropertyHandle inProperty) const override;
    bool HandleValid(int inHandle) const override { return CHandleBase::HandleValid(inHandle); }

    Qt3DSDMSlideHandle CreateSlideWithHandle(int inHandle, Qt3DSDMInstanceHandle inInstance);
    void GetSlideProperties(Qt3DSDMSlideHandle inSlide, TSlideEntryList &outProperties) const;

    bool IsSlide(Qt3DSDMSlideHandle inSlide) const override;

    // Possibly alter every slide in the database
    void ForEachSlide(std::function<void(SSlide *)> inFunction);
    void ForEachChild(Qt3DSDMSlideHandle inSlide, std::function<void(SSlide *)> inFunction);

    // Only implemented at the producer level, not at this lower level.
    void CopyProperties(Qt3DSDMSlideHandle /*inSourceSlide*/,
                        Qt3DSDMInstanceHandle /*inSourceInstance*/,
                        Qt3DSDMSlideHandle /*inDestSlide*/, Qt3DSDMInstanceHandle /*inDestInstance*/) override
    {
        throw SlideNotFound(L"");
    }

    static SSlide *GetSlideNF(int inHandle, THandleObjectMap &inObjects)
    {
        return const_cast<SSlide *>(
            GetSlideNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const SSlide *GetSlideNF(int inHandle, const THandleObjectMap &inObjects)
    {
        const SSlide *theSlide = GetHandleObject<SSlide>(inHandle, inObjects);
        if (theSlide)
            return theSlide;
        throw SlideNotFound(L"");
    }

    static inline bool PropertyFound(int inInstance, int inProperty, const TSlideEntry &inEntry)
    {
        if (inInstance == std::get<0>(inEntry) && inProperty == std::get<1>(inEntry))
            return true;
        return false;
    }

    static void ForceSetPropertyValue(IStringTable &inStringTable, THandleObjectMap &inObjects,
                                      Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inHandle,
                                      Qt3DSDMPropertyHandle inProperty, const SValue &inValue);

    static void ClearPropertyValue(THandleObjectMap &inObjects, Qt3DSDMSlideHandle inSlide,
                                   Qt3DSDMInstanceHandle inInstance,
                                   Qt3DSDMPropertyHandle inProperty);

    static inline bool SlideEntryInstanceMatches(const TSlideEntry &inEntry,
                                                 Qt3DSDMInstanceHandle inHandle)
    {
        using namespace std;
        if (inHandle.GetHandleValue() == get<0>(inEntry))
            return true;
        return false;
    }

    static inline bool SlideEntryPropertyMatches(const TSlideEntry &inEntry,
                                                 Qt3DSDMPropertyHandle inProperty)
    {
        using namespace std;
        return inProperty.GetHandleValue() == get<1>(inEntry);
    }

    static inline bool SlideEntryInstancePropertyMatches(const TSlideEntry &inEntry,
                                                         const TInstanceHandleList &inInstances,
                                                         const TPropertyHandleList &inProperties)
    {
        using namespace std;
        return exists(inInstances, std::bind(equal_to<int>(), get<0>(inEntry),
                                             std::placeholders::_1))
            && exists(inProperties, std::bind(equal_to<int>(), get<1>(inEntry),
                                              std::placeholders::_1));
    }
};

typedef std::shared_ptr<CSimpleSlideCore> TSimpleSlideCorePtr;
}

#endif
