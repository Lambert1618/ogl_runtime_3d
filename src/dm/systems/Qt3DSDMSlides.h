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
#ifndef QT3DSDM_SLIDES_H
#define QT3DSDM_SLIDES_H
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSlideCore.h"

namespace qt3dsdm {
/**
 * Some properties point to instances which act like extended properties. An example of this would
 * be the images on materials. If a material has an image in one of its slots, that image should be
 * duplicated upon property unlink and deleted upon property link. It (and its animations) should
 * be duplicated upon create slide and deleted upon slide destruction such that the concept that
 * the image is really a property on the material stays sound.
 */
class IPropertyInstanceInfo
{
public:
    virtual ~IPropertyInstanceInfo() {}

    /**
     * Return the instance that relates to this property
     */
    virtual Qt3DSDMInstanceHandle GetInstanceForProperty(const SValue &inValue) = 0;

    /**
     * Duplicate this instance and whichever properties and animations you desire,
     * returning a new data model value that will be set on the newly created property.
     * Don't forget to associate the instance with a the dest slide.  I doubt things
     * will work correctly if you do not.
     */
    virtual SValue CreateInstanceForProperty(Qt3DSDMSlideHandle inSourceSlide,
                                             Qt3DSDMSlideHandle inDestSlide,
                                             Qt3DSDMInstanceHandle inInstance) = 0;
};

typedef std::shared_ptr<IPropertyInstanceInfo> TPropertyInstanceInfoPtr;

struct SInstanceSlideInformation
{
    // The slide associated with the instance
    Qt3DSDMSlideHandle m_AssociatedSlide;
    // The master slide of the component.
    Qt3DSDMSlideHandle m_MasterSlide;
    // The active slide of the component.
    Qt3DSDMSlideHandle m_ActiveSlide;
    // The current time of the active slide on the component.
    long m_ComponentTime;

    SInstanceSlideInformation()
        : m_ComponentTime(0)
    {
    }
    SInstanceSlideInformation(Qt3DSDMSlideHandle inAssoc, Qt3DSDMSlideHandle inMaster,
                              Qt3DSDMSlideHandle inActive, long inTime)
        : m_AssociatedSlide(inAssoc)
        , m_MasterSlide(inMaster)
        , m_ActiveSlide(inActive)
        , m_ComponentTime(inTime)
    {
    }
};

class ISlideSystem
{
public:
    virtual ~ISlideSystem() {}
    virtual Qt3DSDMSlideHandle CreateMasterSlide() = 0;
    virtual Qt3DSDMSlideHandle CreateSlide(Qt3DSDMSlideHandle inMaster, int inIndex = -1) = 0;
    // Duplicate this slide and put it into the master's child lists at index.
    virtual Qt3DSDMSlideHandle DuplicateSlide(Qt3DSDMSlideHandle inSourceSlide, int inDestIndex) = 0;
    virtual Qt3DSDMSlideHandle GetMasterSlide(Qt3DSDMSlideHandle inSlide) const = 0;
    virtual bool IsMasterSlide(Qt3DSDMSlideHandle inSlide) const = 0;
    virtual Qt3DSDMSlideHandle GetMasterSlideByComponentGuid(SLong4 inGuid) const = 0;
    // Indexes are 1 based.  Index 0 refers to the master slide; you can't delete this.
    virtual void DeleteSlideByIndex(Qt3DSDMSlideHandle inMaster, size_t inIndex) = 0;
    virtual void GetSlideReferencedInstances(Qt3DSDMSlideHandle inMaster, size_t inIndex,
                                             TInstanceHandleList &outReferencedInstances) = 0;
    virtual Qt3DSDMSlideHandle GetSlideByIndex(Qt3DSDMSlideHandle inMaster, size_t inIndex) const = 0;
    virtual void SetActiveSlide(Qt3DSDMSlideHandle inMaster, size_t inIndex) = 0;
    virtual void RearrangeSlide(Qt3DSDMSlideHandle inMaster, size_t inOldIndex,
                                size_t inNewIndex) = 0;
    /**
     * Set the current component time. This object will figure out the component from the slide.
     */
    virtual void SetComponentTime(Qt3DSDMSlideHandle slide, long time) = 0;
    virtual long GetComponentTime(Qt3DSDMSlideHandle inSlide) const = 0;

    // The fastest way possible, get all of the slide information for this instance.
    virtual SInstanceSlideInformation
    GetInstanceSlideInformation(Qt3DSDMInstanceHandle inInstance) const = 0;
    /**
     * Slide count includes the master slide;
     */
    virtual size_t GetSlideCount(Qt3DSDMSlideHandle inMaster) const = 0;
    /**
     * Use the instance for storing information such as name, or the GUID of the object
     * this slide links to.
     */
    virtual Qt3DSDMInstanceHandle GetSlideInstance(Qt3DSDMSlideHandle inSlide) const = 0;
    /**
     * Reverse lookup into the slide system so you can match slides to instances.
     */
    virtual Qt3DSDMSlideHandle GetSlideByInstance(Qt3DSDMInstanceHandle inInstance) const = 0;
    /**
     * Slide may be either a master slide
     */
    virtual void AssociateInstanceWithSlide(Qt3DSDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance) = 0;
    virtual Qt3DSDMSlideHandle GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance) const = 0;
    virtual void GetAssociatedInstances(
        Qt3DSDMSlideHandle inMaster,
        std::vector<std::pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>> &outAssociations) const = 0;
    virtual void GetAssociatedInstances(Qt3DSDMSlideHandle inSlide,
                                        TInstanceHandleList &outAssociations) const = 0;
    virtual void LinkProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) = 0;
    virtual void UnlinkProperty(Qt3DSDMInstanceHandle inInstance,
                                Qt3DSDMPropertyHandle inProperty) = 0;
    virtual bool IsPropertyLinked(Qt3DSDMInstanceHandle inInstance,
                                  Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual bool CanPropertyBeLinked(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty) const = 0;
    virtual void GetUnionOfProperties(Qt3DSDMSlideHandle inSlide1, Qt3DSDMSlideHandle inSlide,
                                      TInstancePropertyPairList &outProperties) const = 0;

    virtual bool GetSlidePropertyValue(size_t inSlide, Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, SValue &outValue) = 0;

    virtual void SetActiveSlide(Qt3DSDMSlideHandle inSlide) = 0;
    virtual Qt3DSDMSlideHandle GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty) const = 0;

    virtual bool SlideValid(Qt3DSDMSlideHandle inSlide) const = 0;
    virtual int GetSlideIndex(Qt3DSDMSlideHandle inSlide) const = 0;
    virtual int GetActiveSlideIndex(Qt3DSDMSlideHandle inMaster) const = 0;
    virtual Qt3DSDMSlideHandle GetActiveSlide(Qt3DSDMSlideHandle inMaster) const = 0;
    virtual Qt3DSDMInstanceHandle GetSlideSelectedInstance(Qt3DSDMSlideHandle inSlide) const = 0;
    virtual void SetSlideSelectedInstance(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance) = 0;

    virtual Qt3DSDMSlideHandle GetApplicableSlide(Qt3DSDMInstanceHandle inHandle) = 0;
    virtual Qt3DSDMSlideHandle GetApplicableSlide(Qt3DSDMInstanceHandle inHandle,
                                                 Qt3DSDMPropertyHandle inProperty) = 0;

    virtual bool GetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty,
                                          SValue &outValue) const = 0;
    virtual bool GetCanonicalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMPropertyHandle inProperty,
                                                   SValue &outValue) const = 0;
    virtual void ForceSetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                               Qt3DSDMInstanceHandle inInstance,
                                               Qt3DSDMPropertyHandle inProperty,
                                               const SValue &inValue) = 0;

    /**
     * Let this object know that this property will sometimes reference another property
     * and give an outside entity the chance to create new objects and properties when
     * the property is unlinked and linked.
     */
    virtual void RegisterPropertyInstance(Qt3DSDMPropertyHandle inPropertyHandle,
                                          TPropertyInstanceInfoPtr inPropertyInfo) = 0;
};

typedef std::shared_ptr<ISlideSystem> TSlideSystemPtr;
}

#endif
