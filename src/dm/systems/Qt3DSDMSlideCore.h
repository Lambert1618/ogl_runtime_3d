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
#ifndef QT3DSDM_SLIDE_CORE_H
#define QT3DSDM_SLIDE_CORE_H
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMDataTypes.h"
#include "HandleSystemBase.h"
#include "Qt3DSDMStringTable.h"
#include "Qt3DSDMDataCore.h"

namespace qt3dsdm {
typedef std::pair<Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle> TInstancePropertyPair;
typedef std::vector<TInstancePropertyPair> TInstancePropertyPairList;

// instance,property,value
typedef std::tuple<Qt3DSDMInstanceHandle, Qt3DSDMPropertyHandle, SValue> TSlideEntry;
typedef std::vector<TSlideEntry> TSlideEntryList;

class ISlideCore : public IHandleBase
{
public:
    virtual ~ISlideCore() {}
    virtual TStringTablePtr GetStringTablePtr() const = 0;
    virtual IStringTable &GetStringTable() const = 0;
    //===============================================================
    // Slide lifetime management
    //===============================================================
    /**
     *	Create a new slide.  Slides must have an instance allocated for them.
     */
    virtual Qt3DSDMSlideHandle CreateSlide(Qt3DSDMInstanceHandle inInstance) = 0;
    /**
     *	Return the instance that was allocated for this slide.
     */
    virtual Qt3DSDMInstanceHandle GetSlideInstance(Qt3DSDMSlideHandle inSlide) const = 0;
    /**
     *	Reverse lookup into the slide system so you can match slides to instances.
     */
    virtual Qt3DSDMSlideHandle GetSlideByInstance(Qt3DSDMInstanceHandle inSlide) const = 0;
    /**
     *	Delete a given slide.  This recursively deletes all children.  All associated instances
     *	are returned in the outInstances list so the caller can further delete them.
     *	The deleted object is removed from the parent's list of children.  Child objects are
     *	also deleted.
     */
    virtual void DeleteSlide(Qt3DSDMSlideHandle inSlide, TInstanceHandleList &outInstances) = 0;
    /**
     *	Return all of the slides in the core.
     */
    virtual void GetSlides(TSlideHandleList &outSlides) const = 0;

    virtual float GetSlideTime(Qt3DSDMSlideHandle inSlide) const = 0;

    virtual void SetSlideTime(Qt3DSDMSlideHandle inSlide, float inNewTime) = 0;

    //===============================================================
    // Slide derivation
    //===============================================================
    /**
     *	Derive a slide from another slide.  This simply provides an alternate path for property
     *lookup
     *	if it isn't found on this slide.  OK for parent to be an invalid handle.  It is also fine
     *	for the slide to have a valid parent; all involved parties are notified and updated.
     */
    virtual void DeriveSlide(Qt3DSDMSlideHandle inSlide, Qt3DSDMSlideHandle inParent,
                             int inIndex = -1) = 0;
    /**
     *	Return the parent slide for this slide.  Invalid handle if no parent.
     */
    virtual Qt3DSDMSlideHandle GetParentSlide(Qt3DSDMSlideHandle inSlide) const = 0;
    /**
     *	Return a list of derived slides for this slide.
     */
    virtual void GetChildSlides(Qt3DSDMSlideHandle inSlide, TSlideHandleList &outChildren) const = 0;

    /**
     *	Return the index of this child.  Exception if child isn't found
     */
    virtual int GetChildIndex(Qt3DSDMSlideHandle inParent, Qt3DSDMSlideHandle inChild) const = 0;
    //===============================================================

    //===============================================================
    //	Instance Property Access
    //===============================================================
    /**
     *	Get this property from this instance in this slide.  Includes lookup into any parent slides.
     */
    virtual bool GetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty,
                                          SValue &outValue) const = 0;

    /**
     *	Set this property.  If this slide doesn't already contain an entry
     *(PushPropertyValueToChildren)
     *	then this will attempt to set it on the parent slide.  If it does not have a parent slide or
     *if
     *	it already has a property entry then it sets the value locally.
     */
    virtual void SetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty,
                                          const SValue &inValue) = 0;

    /**
     *	Set this property on this slide; do not try to set on parent slide under any circumstances.
     *Always adds entry to this slide.
     */
    virtual void ForceSetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                               Qt3DSDMInstanceHandle inInstance,
                                               Qt3DSDMPropertyHandle inProperty,
                                               const SValue &inValue) = 0;

    /**
     * Set a property value on an instance on all slides.
     */
    virtual void forceSetInstancePropertyValueOnAllSlides(Qt3DSDMInstanceHandle inInstance,
                                                          Qt3DSDMPropertyHandle inProperty,
                                                          const SValue &inValue) = 0;

    /**
     *	Return the value for this property if it exists on this slide.
     */
    virtual bool GetSpecificInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                                  Qt3DSDMInstanceHandle inInstance,
                                                  Qt3DSDMPropertyHandle inProperty,
                                                  SValue &outValue) const = 0;

    /**
     *	Return all of the properties for this instance stored on this slide.
     */
    virtual void GetSpecificInstancePropertyValues(Qt3DSDMSlideHandle inSlide,
                                                   Qt3DSDMInstanceHandle inInstance,
                                                   TPropertyHandleValuePairList &outValues) = 0;

    /**
     *	Get all of the property entries for the slide.
     */
    virtual void GetSlidePropertyEntries(Qt3DSDMSlideHandle inSlide,
                                         TSlideEntryList &outEntries) const = 0;

    /**
     *	Set this property value on all my child slides.
     */
    virtual void PushPropertyValueToChildren(Qt3DSDMSlideHandle inParent,
                                             Qt3DSDMInstanceHandle inInstance,
                                             Qt3DSDMPropertyHandle inProperty,
                                             const SValue &inValue) = 0;

    /**
     *	If any property entries in slide1 intersect (have matching instance and property handles)
     *with slide 2,
     *	add said property entry from slide1 to outEntries;
     */
    virtual void GetIntersectingProperties(Qt3DSDMSlideHandle inSlide1, Qt3DSDMSlideHandle inSlide2,
                                           TSlideEntryList &outEntries) const = 0;

    /**
    *	Take the intersection of the properties of slide1 and slide2 and push values from slide1 to
    *destination.
    *	Used for new slide operations.
    */
    virtual void PushIntersectingProperties(Qt3DSDMSlideHandle inSlide1, Qt3DSDMSlideHandle inSlide2,
                                            Qt3DSDMSlideHandle inDestination) = 0;

    /**
     *	Ensure the children of this parent item do not contain entries for a given property.
     */
    virtual void ClearChildrenPropertyValues(Qt3DSDMSlideHandle inParent,
                                             Qt3DSDMInstanceHandle inHandle,
                                             Qt3DSDMPropertyHandle inProperty) = 0;

    /**
     *	Delete all property entries related to this instance.
     */
    virtual void DeleteAllInstanceEntries(Qt3DSDMInstanceHandle inHandle) = 0;

    /**
     *	Delete all of the property entries related to this property
     */
    virtual void DeleteAllPropertyEntries(Qt3DSDMPropertyHandle inHandle) = 0;

    /**
     *	Delete all property entries that have an instance in the instances list *and*
     *	a property in the properties list.
     */
    virtual void DeleteAllInstancePropertyEntries(const TInstanceHandleList &inInstances,
                                                  const TPropertyHandleList &inProperties) = 0;

    /**
     *	Does this slide contain this property?
     */
    virtual bool ContainsProperty(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inHandle,
                                  Qt3DSDMPropertyHandle inProperty) const = 0;

    /**
     *	Copy the properties from the source slide and source instance to the destination slide and
     *destination
     *	instance.
     */
    virtual void CopyProperties(Qt3DSDMSlideHandle inSourceSlide,
                                Qt3DSDMInstanceHandle inSourceInstance,
                                Qt3DSDMSlideHandle inDestSlide,
                                Qt3DSDMInstanceHandle inDestInstance) = 0;

    virtual bool IsSlide(Qt3DSDMSlideHandle inSlide) const = 0;
};

typedef std::shared_ptr<ISlideCore> TSlideCorePtr;
}

#endif
