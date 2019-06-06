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
#ifndef SLIDESYSTEMH
#define SLIDESYSTEMH
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMSlideCore.h"
#include "Qt3DSDMSlideGraphCore.h"
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMAnimation.h"
#include "StudioPropertySystem.h"
#include "SignalsImpl.h"

/**
 *	The systems aggregate the various cores and provide further information or
 *	integrity checking taking the various cores into account.
 */

namespace qt3dsdm {

typedef std::pair<Qt3DSDMPropertyHandle, TPropertyInstanceInfoPtr> TPropertyHandlePropertyInfoPair;
typedef std::vector<TPropertyHandlePropertyInfoPair> TPropertyHandlePropertyInfoPairList;
/**
 *	Provides more thorough checking and will return an appropriate
 *	slide graph when calling GetSlideGraph
 */
struct SSlideSystem : public ISlideSystem
{
    Q_DISABLE_COPY(SSlideSystem)

    TDataCorePtr m_DataCore; // TODO: We might want to throw this away and use the PropertySystem,
                             // unless we have a clean seperate of IPropertySystem and IDataCore
    TPropertySystemPtr m_PropertySystem;
    TSlideCorePtr m_SlideCore;
    TSlideGraphCorePtr m_SlideGraphCore;
    TAnimationCorePtr m_AnimationCore;
    Qt3DSDMInstanceHandle m_SlideInstance;
    Qt3DSDMPropertyHandle m_ComponentGuid;
    std::shared_ptr<ISignalItem> m_Signaller;
    TPropertyHandlePropertyInfoPairList m_PropertyInfoPairList;
    typedef std::unordered_map<int, int> TIntIntMap;
    TIntIntMap m_SlideSelectedInstances;

    SSlideSystem(TDataCorePtr inDataCore, TSlideCorePtr inSlideCore,
                 TSlideGraphCorePtr inSlideGraphCore, TAnimationCorePtr inAnimationCore,
                 Qt3DSDMInstanceHandle inSlideInstance,
                 Qt3DSDMPropertyHandle inComponentGuidProperty);

    void SetPropertySystem(TPropertySystemPtr inPropertySystem);

    Qt3DSDMSlideHandle CreateMasterSlide() override;
    Qt3DSDMSlideHandle CreateSlide(Qt3DSDMSlideHandle inMaster, int inIndex = -1) override;
    Qt3DSDMSlideHandle DuplicateSlide(Qt3DSDMSlideHandle inSourceSlide, int inDestIndex = -1) override;
    Qt3DSDMSlideHandle GetMasterSlide(Qt3DSDMSlideHandle inSlide) const override;
    bool IsMasterSlide(Qt3DSDMSlideHandle inSlide) const override;
    Qt3DSDMSlideHandle GetMasterSlideByComponentGuid(SLong4 inGuid) const override;
    // Indexes are 1 based.  Index 0 refers to the master slide; you can't delete this.
    void DeleteSlideByIndex(Qt3DSDMSlideHandle inMaster, size_t inIndex) override;
    void GetSlideReferencedInstances(Qt3DSDMSlideHandle inMaster, size_t inIndex,
                                             TInstanceHandleList &outReferencedInstances) override;
    Qt3DSDMSlideHandle GetSlideByIndex(Qt3DSDMSlideHandle inMaster, size_t inIndex) const override;
    void SetActiveSlide(Qt3DSDMSlideHandle inMaster, size_t inIndex) override;
    size_t GetSlideCount(Qt3DSDMSlideHandle inMaster) const override;
    void RearrangeSlide(Qt3DSDMSlideHandle inMaster, size_t inOldIndex, size_t inNewIndex) override;

    void SetComponentSeconds(Qt3DSDMSlideHandle inSlide, float inSeconds) override;
    float GetComponentSeconds(Qt3DSDMSlideHandle inSlide) const override;
    long GetComponentSecondsLong(Qt3DSDMSlideHandle inSlide) const override;
    // For any given instance, find the current seconds via backtracking to the graph, finding the
    // active
    // slide, and return.
    long GetComponentSecondsLong(Qt3DSDMInstanceHandle inInstance) const override;
    virtual SInstanceSlideInformation
    GetInstanceSlideInformation(Qt3DSDMInstanceHandle inInstance) const override;
    /**
     * Use the instance for storing information such as name, or the GUID of the object
     * this slide links to.
     */
    Qt3DSDMInstanceHandle GetSlideInstance(Qt3DSDMSlideHandle inInstance) const override;
    /**
     *	Reverse lookup into the slide system so you can match slides to instances.
     */
    Qt3DSDMSlideHandle GetSlideByInstance(Qt3DSDMInstanceHandle inInstance) const override;

    /**
     *	Slide may be either a master slide or a normal slide.  This will associate this instance
     *	with this set of slides.  Property lookups (using the above IInstancePropertyCore interface)
     *	will now run through the slide set before hitting the main data core database.
     */
    void AssociateInstanceWithSlide(Qt3DSDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance) override;
    Qt3DSDMSlideHandle GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance) const override;
    void GetAssociatedInstances(
        Qt3DSDMSlideHandle inMaster,
        std::vector<std::pair<Qt3DSDMSlideHandle, Qt3DSDMInstanceHandle>> &outAssociations) const override;
    void GetAssociatedInstances(Qt3DSDMSlideHandle inSlide,
                                        TInstanceHandleList &outAssociations) const override;
    void LinkProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) override;
    void UnlinkProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty) override;
    bool IsPropertyLinked(Qt3DSDMInstanceHandle inInstance,
                                  Qt3DSDMPropertyHandle inProperty) const override;
    bool CanPropertyBeLinked(Qt3DSDMInstanceHandle inInstance,
                                     Qt3DSDMPropertyHandle inProperty) const override;
    bool GetSlidePropertyValue(size_t inSlide, Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty, SValue &outValue) override;
    void GetUnionOfProperties(Qt3DSDMSlideHandle inSlide1, Qt3DSDMSlideHandle inSlide,
                                      TInstancePropertyPairList &outProperties) const override;

    void SetActiveSlide(Qt3DSDMSlideHandle inSlide) override;
    Qt3DSDMSlideHandle GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty) const override;

    bool SlideValid(Qt3DSDMSlideHandle inSlide) const override;
    int GetSlideIndex(Qt3DSDMSlideHandle inSlide) const override;
    int GetActiveSlideIndex(Qt3DSDMSlideHandle inMaster) const override;
    Qt3DSDMSlideHandle GetActiveSlide(Qt3DSDMSlideHandle inMaster) const override;
    Qt3DSDMInstanceHandle GetSlideSelectedInstance(Qt3DSDMSlideHandle inSlide) const override;
    void SetSlideSelectedInstance(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance) override;

    Qt3DSDMSlideHandle GetApplicableSlide(Qt3DSDMInstanceHandle inHandle) override;
    Qt3DSDMSlideHandle GetApplicableSlide(Qt3DSDMInstanceHandle inHandle,
                                                 Qt3DSDMPropertyHandle inProperty) override;

    bool GetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                          Qt3DSDMInstanceHandle inInstance,
                                          Qt3DSDMPropertyHandle inProperty, SValue &outValue) const override;
    bool GetCanonicalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMPropertyHandle inProperty,
                                                   SValue &outValue) const override;
    void ForceSetInstancePropertyValue(Qt3DSDMSlideHandle inSlide,
                                               Qt3DSDMInstanceHandle inInstance,
                                               Qt3DSDMPropertyHandle inProperty,
                                               const SValue &inValue) override;

    void RegisterPropertyInstance(Qt3DSDMPropertyHandle inPropertyHandle,
                                          TPropertyInstanceInfoPtr inPropertyInfo) override;

    virtual ISlideSystemSignalProvider *GetSignalProvider();

private:
    virtual ISlideSystemSignalSender *GetSignalSender();
    // helper method
    void InsertEntryAndPropertyInstance(const TSlideEntry &inEntry,
                                        TInstanceHandleList &inInstances,
                                        Qt3DSDMSlideHandle inSlide);
    void DeleteReferencedInstances(Qt3DSDMSlideHandle inSlide);
    void GetReferencedInstances(Qt3DSDMSlideHandle inSlide,
                                TInstanceHandleList &outReferencedInstances);
};
}

#endif
