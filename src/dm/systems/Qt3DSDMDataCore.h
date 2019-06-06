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
#ifndef QT3DSDM_DATACORE_H
#define QT3DSDM_DATACORE_H
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMPropertyDefinition.h"
#include "HandleSystemBase.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMValue.h"

namespace qt3dsdm {

typedef std::pair<Qt3DSDMPropertyHandle, SValue> TPropertyHandleValuePair;
typedef std::vector<TPropertyHandleValuePair> TPropertyHandleValuePairList;

class IInstancePropertyCore
{
public:
    virtual ~IInstancePropertyCore() {}

    /**
     *	Find a given instance property by name. May return an invalid handle.
     */
    virtual Qt3DSDMPropertyHandle
    GetAggregateInstancePropertyByName(Qt3DSDMInstanceHandle inInstance,
                                       const TCharStr &inStr) const = 0;
    /**
     *	Get the entire list of instance properties;
     */
    virtual void GetAggregateInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                                TPropertyHandleList &outProperties) const = 0;
    // Get the properties that are stored only on this instance, not on any parents.
    virtual void GetSpecificInstancePropertyValues(Qt3DSDMInstanceHandle inHandle,
                                                   TPropertyHandleValuePairList &outValues) = 0;
    /**
     *	Return true if this property is on this instance or one of its parents.
     */
    virtual bool HasAggregateInstanceProperty(Qt3DSDMInstanceHandle inInstance,
                                              Qt3DSDMPropertyHandle inProperty) const = 0;

    /**
     *	Does this value match the approperty property type.
     *	error - PropertyNotFound
     *	error - ValueTypeError
     */
    virtual void CheckValue(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                            const SValue &inValue) const = 0;
    /**
     *	Get this property from this instance.   This includes looking up the default value.  If the
     *instance doesn't have the property
     *	and it has no default the return value is false.
     *	error - InstanceNotFound
     *	error - PropertyNotFound if this property doesn't exist on this instance or doesn't exist.
     */
    virtual bool GetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty,
                                          SValue &outValue) const = 0;
    /**
     *	Set this property on this instance.
     *	error - InstanceNotFound
     *	error - PropertyNotFound if this property doesn't exist on this instance or doesn't exist.
     *	error - ValueTypeError if this value is of the wrong type
     */
    virtual void SetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty,
                                          const SValue &inValue) = 0;
};

typedef std::shared_ptr<IInstancePropertyCore> TInstancePropertyCorePtr;

class IStringTable;

/**
 *	Primary interface to data model system.  This allows you to define 'classes', which are
 *really just
 *	collections of properties, and instances of those classes.  A zero handle value is regarded
 *as empty.
 */
class IDataCore : public IHandleBase, public IInstancePropertyCore
{
public:
    virtual ~IDataCore() {}

    // Bookkeeping
    virtual IStringTable &GetStringTable() const = 0;
    virtual std::shared_ptr<IStringTable> GetStringTablePtr() const = 0;

    //===============================================================
    //	Instances
    //===============================================================
    /**
     *	Create a new instance of a class.  Instances can be named, in which case
     *	they are sometimes treated differently when serialized.  If the name is null
     *	or has no length then it is ignored.  If the name is non-null, then a check is
     *	run as named instances must have unique names.  When a named instance is a parent
     *	of another instance, only its name is serialized out; not its handle thus you can safely
     *	use names as inter-application instance handles.
     *	May use a target id to specify what the ideal id is; id must not exist.
     *	- error DuplicateInstanceName
     */
    virtual Qt3DSDMInstanceHandle
    CreateInstance(Qt3DSDMInstanceHandle inTargetId = Qt3DSDMInstanceHandle()) = 0;
    /**
     *	Delete an instance.
     *	error - InstanceNotFound
     */
    virtual void DeleteInstance(Qt3DSDMInstanceHandle inHandle) = 0;
    /**
     *	Return all the instances in the system.
     */
    virtual void GetInstances(TInstanceHandleList &outInstances) const = 0;
    /**
     *	Return all the instances in the system that is derived from parent.
     */
    virtual void GetInstancesDerivedFrom(TInstanceHandleList &outInstances,
                                         Qt3DSDMInstanceHandle inParentHandle) const = 0;

    //===============================================================
    //	Instance Derivation
    //===============================================================
    /**
     *	Derive an instance from another instance.  This implies a rebuild of
     *	the aggregate properties of a given instance.
     *	error - InstanceNotFound if either instance or parent aren't found
     */
    virtual void DeriveInstance(Qt3DSDMInstanceHandle inInstance, Qt3DSDMInstanceHandle inParent) = 0;
    /**
     *	Return a count of the parent's of a given instance.
     *	error - InstanceNotFound
     */
    virtual void GetInstanceParents(Qt3DSDMInstanceHandle inHandle,
                                    TInstanceHandleList &outParents) const = 0;

    /**
     *	Returns true if the instance is derived from the parent somehow.
     */
    virtual bool IsInstanceOrDerivedFrom(Qt3DSDMInstanceHandle inInstance,
                                         Qt3DSDMInstanceHandle inParent) const = 0;
    //===============================================================

    //===============================================================
    //	Class Properties
    //===============================================================
    /**
     *	Add a new property to a given instances.  You can use this to overshadow properties
     *	on base instances but you can't add two properties of the same name to a given
     *	instance.
     *	error - InstanceNotFound
     *	error - PropertyExists if the property already exists on this class
     *	error - ValueTypeError if the default value exists and is if a different type than
     *inPropType
     */
    virtual Qt3DSDMPropertyHandle AddProperty(Qt3DSDMInstanceHandle inInstance, TCharPtr inName,
                                             DataModelDataType::Value inPropType) = 0;
    /**
     *	Return the property definition that corresponds to a given property value.
     */
    virtual const Qt3DSDMPropertyDefinition &GetProperty(Qt3DSDMPropertyHandle inProperty) const = 0;
    /**
     *	Return the properties specific to this instance, not including properties
     *	gained by derivation
     */
    virtual void GetInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                       TPropertyHandleList &outProperties) const = 0;
    /**
     *	Remove a property from an instance.  Instances will no long have this property accessible
     *nor will
     *	child instances.
     */
    virtual void RemoveProperty(Qt3DSDMPropertyHandle inProperty) = 0;
    /**
     *	Copy properties (definitions & values) specific to one instance, not including properties
     *	gained by derivation, to another instance.
     */
    virtual void CopyInstanceProperties(Qt3DSDMInstanceHandle inSrcInstance,
                                        Qt3DSDMInstanceHandle inDestInstance) = 0;
    /**
     *	Removed cached intermediate values from the instance.  DataModel pull properties from the
     *inheritance hierarchy chain
     *  up to the current instance when someone requests a property in order to make the next
     *request quicker.  This breaks
     *	some forms of updating where a parent's default value gets changed and the instance won't
     *reflect it (until you
     *	save/load the file).
     */
    //===============================================================
    virtual void RemoveCachedValues(Qt3DSDMInstanceHandle inInstance) = 0;

    //===============================================================
    //	Handle validation
    //===============================================================
    /**
     *	Is valid and is instances
     */
    virtual bool IsInstance(int inHandle) const = 0;
    /**
     * is valid and is property
     */
    virtual bool IsProperty(int inHandle) const = 0;
};

typedef std::shared_ptr<IDataCore> TDataCorePtr;

class IPropertySystem : public IHandleBase
{
public:
    virtual ~IPropertySystem() {}

    virtual Qt3DSDMInstanceHandle CreateInstance() = 0;
    virtual void DeleteInstance(Qt3DSDMInstanceHandle inHandle) = 0;
    virtual void DeriveInstance(Qt3DSDMInstanceHandle inInstance, Qt3DSDMInstanceHandle inParent) = 0;

    virtual void GetInstances(TInstanceHandleList &outInstances) const = 0;

    virtual Qt3DSDMPropertyHandle AddProperty(Qt3DSDMInstanceHandle inInstance, TCharPtr inName,
                                             DataModelDataType::Value inPropType) = 0;

    virtual Qt3DSDMPropertyHandle
    GetAggregateInstancePropertyByName(Qt3DSDMInstanceHandle inInstance,
                                       const TCharStr &inStr) const = 0;
    virtual void GetAggregateInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                                TPropertyHandleList &outProperties) const = 0;
    virtual bool HasAggregateInstanceProperty(Qt3DSDMInstanceHandle inInstance,
                                              Qt3DSDMPropertyHandle inProperty) const = 0;

    virtual bool GetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty,
                                          SValue &outValue) const = 0;
    virtual void SetInstancePropertyValue(Qt3DSDMInstanceHandle inHandle,
                                          Qt3DSDMPropertyHandle inProperty,
                                          const SValue &inValue) = 0;

    virtual bool IsInstanceOrDerivedFrom(Qt3DSDMInstanceHandle inInstance,
                                         Qt3DSDMInstanceHandle inParent) const = 0;

    virtual DataModelDataType::Value GetDataType(Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual TCharStr GetName(Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual TCharStr GetFormalName(Qt3DSDMInstanceHandle inInstance,
                                   Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual AdditionalMetaDataType::Value
    GetAdditionalMetaDataType(Qt3DSDMInstanceHandle inInstance,
                              Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual TMetaDataData GetAdditionalMetaDataData(Qt3DSDMInstanceHandle inInstance,
                                                    Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual Qt3DSDMInstanceHandle GetPropertyOwner(Qt3DSDMPropertyHandle inProperty) const = 0;

    virtual QVector<Qt3DSDMPropertyHandle> GetControllableProperties(
            Qt3DSDMInstanceHandle inInst) const = 0;

};
typedef std::shared_ptr<IPropertySystem> TPropertySystemPtr;

template <typename TCoreType>
inline SValue GetInstancePropertyValue(TCoreType inCore, Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty)
{
    SValue retval;
    if (!inCore->GetInstancePropertyValue(inInstance, inProperty, retval))
        throw PropertyNotFound(L"");
    return retval;
}

template <typename TDataType, typename TCoreType>
inline TDataType GetSpecificInstancePropertyValue(TCoreType inDataCore,
                                                  Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty)
{
    return qt3dsdm::get<TDataType>(GetInstancePropertyValue(inDataCore, inInstance, inProperty));
}

template <typename TDataType, typename TCoreType>
inline TDataType GetNamedInstancePropertyValue(TCoreType inDataCore,
                                               Qt3DSDMInstanceHandle inInstance,
                                               const TCharStr &inName)
{
    Qt3DSDMPropertyHandle theProperty =
        inDataCore->GetAggregateInstancePropertyByName(inInstance, inName);
    return GetSpecificInstancePropertyValue<TDataType, TCoreType>(inDataCore, inInstance,
                                                                  theProperty);
}

inline Qt3DSDMPropertyHandle AddPropertyWithValue(IPropertySystem &inPropertySystem,
                                                 Qt3DSDMInstanceHandle inInstance, TCharPtr inName,
                                                 DataModelDataType::Value inDataType,
                                                 const SValue &inValue)
{
    Qt3DSDMPropertyHandle theProperty(inPropertySystem.AddProperty(inInstance, inName, inDataType));
    inPropertySystem.SetInstancePropertyValue(inInstance, theProperty, inValue);
    return theProperty;
}
}

#endif
