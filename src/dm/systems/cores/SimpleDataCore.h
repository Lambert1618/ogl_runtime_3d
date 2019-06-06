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
#ifndef DATACOREH
#define DATACOREH

#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMSignals.h"
#include "HandleSystemBase.h"
#include "Qt3DSDMPropertyDefinition.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMStringTable.h"
#include <unordered_set>
#include "Qt3DSDMValue.h"

namespace qt3dsdm {

struct PropertyValueFlags
{
    enum Enum {
        // Property values that are set via event propagation should
        // not be serialized to a file.  The auto-propagation step
        // is meant to make looking up property values quicker because
        // the the derivation chain doesn't need to be followed to get
        // the latest default value.
        SetViaAutoPropagate = 1,
    };

    int m_Flags;

    PropertyValueFlags()
        : m_Flags(0)
    {
    }
    explicit PropertyValueFlags(int inFlags)
        : m_Flags(inFlags)
    {
    }

    bool IsSetViaAutoPropagation() const { return (m_Flags & SetViaAutoPropagate) > 0; }
};

// Pair of property value flags and property value.  The property value flags allow
// us to track metadata that is
typedef std::pair<PropertyValueFlags, SInternValue> TPropertyValuePair;
typedef std::unordered_map<int, TPropertyValuePair> TPropertyPairHash;
// Property pairs are referred to by property handle.
typedef std::pair<int, TPropertyValuePair> TPropertyPair;
typedef std::vector<TPropertyPair> TPropertyPairList;

class CDataModelPropertyDefinitionObject : public CHandleObject
{
public:
    Qt3DSDMPropertyDefinition m_Definition;

    CDataModelPropertyDefinitionObject(int inHandle, const Qt3DSDMPropertyDefinition &inDefinition)
        : CHandleObject(inHandle)
        , m_Definition(inDefinition)
    {
    }
    CDataModelPropertyDefinitionObject() {}

    static const EHandleObjectType s_Type =
        CHandleObject::EHandleObjectTypeCDataModelPropertyDefinitionObject;
    EHandleObjectType GetType() override { return s_Type; }
};

typedef std::unordered_map<int, SInternValue> TIntStringVariantList;

class CDataModelInstance : public CHandleObject
{
public:
    typedef std::pair<int, std::shared_ptr<CDataModelInstance>> TInstancePair;
    typedef std::unordered_map<int, std::shared_ptr<CDataModelInstance>> TInstancePairList;
    typedef std::unordered_map<int, bool> TInstanceParentMap;

    TInstancePairList m_Parents;
    mutable TInstanceParentMap m_CachedParents;
    // Properties specific to this class
    TIntList m_Properties;
    TPropertyPairHash m_PropertyValues;

    CDataModelInstance() {}
    CDataModelInstance(int inHandle)
        : CHandleObject(inHandle)
    {
    }

    void ToPropertyPairList(TPropertyPairList &outList) const
    {
        for (TPropertyPairHash::const_iterator iter = m_PropertyValues.begin(),
                                               end = m_PropertyValues.end();
             iter != end; ++iter)
            outList.push_back(*iter);
    }

    void FromPropertyPairList(const TPropertyPairList &inList)
    {
        m_PropertyValues.clear();
        for (TPropertyPairList::const_iterator iter = inList.begin(), end = inList.end();
             iter != end; ++iter)
            m_PropertyValues.insert(*iter);
    }

    void ClearParentCache() const { m_CachedParents.clear(); }

    bool IsDerivedFrom(Qt3DSDMInstanceHandle inParent) const
    {
        std::pair<TInstanceParentMap::iterator, bool> theQueryResult =
            m_CachedParents.insert(std::make_pair(inParent.GetHandleValue(), false));
        // If the insert failed, returned what the hashtable already had in it
        if (theQueryResult.second == false)
            return theQueryResult.first->second;

        // Else find a valid answer
        if (m_Parents.find(inParent.GetHandleValue()) != m_Parents.end()) {
            theQueryResult.first->second = true;
        } else {
            for (TInstancePairList::const_iterator iter = m_Parents.begin(), end = m_Parents.end();
                 iter != end; ++iter) {
                if (iter->second->IsDerivedFrom(inParent)) {
                    theQueryResult.first->second = true;
                    break;
                }
            }
        }

        // Note that we inserted false to begin with.  This means that
        // we can return the insert result here safely as if it wasn't
        // supposed to be false, we would have set it above.
        return theQueryResult.first->second;
    }

    void RemoveCachedValues()
    {
        vector<int> theCachedProperties;

        for (TPropertyPairHash::iterator theProperty = m_PropertyValues.begin(),
                                         end = m_PropertyValues.end();
             theProperty != end; ++theProperty) {
            if (theProperty->second.first.IsSetViaAutoPropagation())
                theCachedProperties.push_back(theProperty->first);
        }

        for (size_t idx = 0, end = theCachedProperties.size(); idx < end; ++idx)
            m_PropertyValues.erase(theCachedProperties[idx]);
    }

    // CHandleObject
    static const EHandleObjectType s_Type = CHandleObject::EHandleObjectTypeCDataModelInstance;
    EHandleObjectType GetType() override { return s_Type; }
};

typedef std::shared_ptr<CDataModelInstance> TDataModelInstancePtr;

class CSimpleDataCore : public CHandleBase, public IDataCore
{
    mutable TStringTablePtr m_StringTable;

public:
    CSimpleDataCore(TStringTablePtr strTable)
        : m_StringTable(strTable)
    {
    }
    CSimpleDataCore(const CSimpleDataCore &inOther)
        : CHandleBase(inOther)
        , m_StringTable(inOther.m_StringTable)
    {
    }

    CSimpleDataCore &operator=(const CSimpleDataCore &inOther)
    {
        CHandleBase::operator=(inOther);
        return *this;
    }

    IStringTable &GetStringTable() const override { return *m_StringTable.get(); }
    TStringTablePtr GetStringTablePtr() const override { return m_StringTable; }

    // IHandleBase
    bool HandleValid(int inHandle) const override
    {
        return m_Objects.find(inHandle) != m_Objects.end();
    }

    // IInstancePropertyCore
    Qt3DSDMPropertyHandle GetAggregateInstancePropertyByName(Qt3DSDMInstanceHandle inInstance,
                                                                    const TCharStr &inStr) const override;
    void GetAggregateInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                                TPropertyHandleList &outProperties) const override;
    void GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inHandle,
                                                   TPropertyHandleValuePairList &outValues) override;
    bool HasAggregateInstanceProperty(Qt3DSDMInstanceHandle inInstance,
                                              Qt3DSDMPropertyHandle inProperty) const override;
    void CheckValue(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                            const SValue &inValue) const override;
    bool GetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty, SValue &outValue) const override;
    void SetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty, const SValue &inValue) override;

    // IDataCore
    //===============================================================
    Qt3DSDMInstanceHandle CreateInstance(Qt3DSDMInstanceHandle hdl = Qt3DSDMInstanceHandle()) override;
    void DeleteInstance(Qt3DSDMInstanceHandle inHandle) override;
    void GetInstances(TInstanceHandleList &outInstances) const override;
    void GetInstancesDerivedFrom(TInstanceHandleList &outInstances,
                                         Qt3DSDMInstanceHandle inParentHandle) const override;

    void DeriveInstance(Qt3DSDMInstanceHandle inInstance, Qt3DSDMInstanceHandle inParent) override;
    void GetInstanceParents(Qt3DSDMInstanceHandle inHandle,
                                    TInstanceHandleList &outParents) const override;
    // Returns true if inParent == inInstance || inInstance is derived from inParent somehow.
    // Recursive.
    bool IsInstanceOrDerivedFrom(Qt3DSDMInstanceHandle inInstance,
                                         Qt3DSDMInstanceHandle inParent) const override;

    Qt3DSDMPropertyHandle AddProperty(Qt3DSDMInstanceHandle inClass, TCharPtr inName,
                                             DataModelDataType::Value inPropType) override;
    const Qt3DSDMPropertyDefinition &GetProperty(Qt3DSDMPropertyHandle inProperty) const override;
    void GetInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                       TPropertyHandleList &outProperties) const override;
    void RemoveProperty(Qt3DSDMPropertyHandle inProperty) override;
    void CopyInstanceProperties(Qt3DSDMInstanceHandle inSrcInstance,
                                        Qt3DSDMInstanceHandle inDestInstance) override;
    void RemoveCachedValues(Qt3DSDMInstanceHandle inInstance) override;

    bool IsInstance(int inHandle) const override;
    bool IsProperty(int inHandle) const override;
    //===============================================================

    bool GetSpecificInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, SValue &outValue) const;
    void GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inInstance,
                                           TPropertyPairHash &outValues) const;

    static CDataModelPropertyDefinitionObject *GetPropertyDefinitionNF(int inHandle,
                                                                       THandleObjectMap &inObjects)
    {
        return const_cast<CDataModelPropertyDefinitionObject *>(
            GetPropertyDefinitionNF(inHandle, static_cast<const THandleObjectMap &>(inObjects)));
    }

    static const CDataModelPropertyDefinitionObject *
    GetPropertyDefinitionNF(int inHandle, const THandleObjectMap &inObjects)
    {
        const CDataModelPropertyDefinitionObject *theClass =
            GetHandleObject<CDataModelPropertyDefinitionObject>(inHandle, inObjects);
        if (theClass)
            return theClass;
        throw PropertyNotFound(L"");
    }

    static TDataModelInstancePtr GetInstance(int inHandle, const THandleObjectMap &inObjects)
    {
        THandleObjectMap::const_iterator theFind(inObjects.find(inHandle));
        // dynamic cast isn't allowed in runtime code as the runtime doesn't enable rtti.
        if (theFind != inObjects.end())
            return std::static_pointer_cast<CDataModelInstance>(theFind->second);
        return TDataModelInstancePtr();
    }

    static TDataModelInstancePtr GetInstanceNF(int inHandle, const THandleObjectMap &inObjects)
    {
        TDataModelInstancePtr retval = GetInstance(inHandle, inObjects);
        if (retval)
            return retval;
        throw InstanceNotFound(L"");
    }

    static inline bool InstancePairMatches(int inHandle,
                                           const CDataModelInstance::TInstancePair &inInstancePair)
    {
        return inHandle == inInstancePair.first;
    }

    // if inInstance doesn't exists in inObjects, it will push back NULL instead of throwing
    // exception
    static inline void UncheckAddInstancePair(CDataModelInstance::TInstancePairList &outInstances,
                                              Qt3DSDMInstanceHandle inInstance,
                                              THandleObjectMap &inObjects)
    {
        outInstances.insert(
            std::make_pair(inInstance, CSimpleDataCore::GetInstance(inInstance, inObjects)));
    }

    static inline bool InstancePropertyMatches(int inProp, const TPropertyPair &inPropPair)
    {
        return inProp == inPropPair.first;
    }

    static bool CheckParentPropertyExistenceWithFail(const TDataModelInstancePtr inInstance,
                                                     int inProperty,
                                                     const THandleObjectMap &inObjects);

    static void CheckPropertyExistence(const TDataModelInstancePtr inInstance, int inProperty,
                                       const THandleObjectMap &inObjects);

    static inline void FindRelatedItemsForDelete(int inInstance, TIntList &outProperties,
                                                 TIntList &outChildInstances,
                                                 const std::pair<int, THandleObjectPtr> &inPair)
    {

        using namespace std;
        if (inPair.second->GetType() == CHandleObject::EHandleObjectTypeCDataModelInstance) {
            TDataModelInstancePtr theInstance =
                static_pointer_cast<CDataModelInstance>(inPair.second);
            // No longer a parent class.
            if (find_if(theInstance->m_Parents.begin(), theInstance->m_Parents.end(),
                        std::bind(InstancePairMatches, inInstance, std::placeholders::_1))
                != theInstance->m_Parents.end())
                outChildInstances.push_back(inPair.first);
        } else if (inPair.second->GetType()
                   == CHandleObject::EHandleObjectTypeCDataModelPropertyDefinitionObject) {
            const CDataModelPropertyDefinitionObject *theProperty =
                static_cast<const CDataModelPropertyDefinitionObject *>(inPair.second.get());
            if (theProperty->m_Definition.m_Instance.GetHandleValue() == inInstance)
                outProperties.push_back(inPair.first);
        }
    }

protected:
    bool IsInstanceOrDerivedFromHelper(const TDataModelInstancePtr inInstance,
                                       Qt3DSDMInstanceHandle inParent) const;
    void AddInstanceIfDerivedFrom(const std::pair<int, THandleObjectPtr> &inItem,
                                  TInstanceHandleList &outInstances,
                                  Qt3DSDMInstanceHandle inParentHandle) const;

    bool GetInstancePropertyValueHelper(const TDataModelInstancePtr inInstance,
                                        Qt3DSDMPropertyHandle inProperty, SValue &outValue) const;

    // Create an instance *at* this specific handle position
    // This is used for special cases of serialization
    Qt3DSDMInstanceHandle CreateInstanceWithHandle(int inHandle);
    Qt3DSDMPropertyHandle AddPropertyWithHandle(int inHandle, Qt3DSDMInstanceHandle inClass,
                                               TCharPtr inName, DataModelDataType::Value inPropType);

    void UncheckedSetSpecificInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMPropertyHandle inProperty,
                                                   const SValue &inValue,
                                                   PropertyValueFlags inIsUserSet);
};

typedef std::shared_ptr<CSimpleDataCore> TSimpleDataCorePtr;
}

#endif
