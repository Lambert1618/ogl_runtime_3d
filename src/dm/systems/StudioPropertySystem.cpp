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

#include "Qt3DSDMPrefix.h"
#include "StudioPropertySystem.h"
#include "StudioAnimationSystem.h"
#include "SignalsImpl.h"

using namespace std;

namespace qt3dsdm {

CStudioPropertySystem::CStudioPropertySystem(std::shared_ptr<IMetaData> inMetaData,
                                             TDataCorePtr inDataCore, TSlideSystemPtr inSlideSystem,
                                             TStudioAnimationSystemPtr inStudioAnimationSystem)
    : m_MetaData(inMetaData)
    , m_DataCore(inDataCore)
    , m_SlideSystem(inSlideSystem)
    , m_StudioAnimationSystem(inStudioAnimationSystem)
{
    m_PropertyCoreSignaller = CreatePropertyCoreSignaller();
    m_ImmediateModePropertyCoreSignaller = CreatePropertyCoreSignaller();
}

DataModelDataType::Value CStudioPropertySystem::GetDataType(Qt3DSDMPropertyHandle inProperty) const
{
    if (m_DataCore->IsProperty(inProperty))
        return m_DataCore->GetProperty(inProperty).m_Type;
    return DataModelDataType::None;
}

TCharStr CStudioPropertySystem::GetName(Qt3DSDMPropertyHandle inProperty) const
{
    if (m_DataCore->IsProperty(inProperty))
        return m_DataCore->GetProperty(inProperty).m_Name;
    return TCharStr();
}

TCharStr CStudioPropertySystem::GetFormalName(Qt3DSDMInstanceHandle inInstance,
                                              Qt3DSDMPropertyHandle inProperty) const
{
    if (inInstance.Valid() && inProperty.Valid())
        return m_MetaData->GetFormalName(inInstance, inProperty);
    return TCharStr();
}

AdditionalMetaDataType::Value
CStudioPropertySystem::GetAdditionalMetaDataType(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty) const
{
    return m_MetaData->GetAdditionalMetaDataType(inInstance, inProperty);
}

TMetaDataData
CStudioPropertySystem::GetAdditionalMetaDataData(Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty) const
{
    return m_MetaData->GetAdditionalMetaDataData(inInstance, inProperty);
}

Qt3DSDMInstanceHandle CStudioPropertySystem::GetPropertyOwner(Qt3DSDMPropertyHandle inProperty) const
{

    qt3dsdm::Qt3DSDMPropertyDefinition thePropDef = m_DataCore->GetProperty(inProperty);
    return thePropDef.m_Instance;
}

Qt3DSDMPropertyHandle
CStudioPropertySystem::GetAggregateInstancePropertyByName(Qt3DSDMInstanceHandle inInstance,
                                                          const TCharStr &inStr) const
{
    return m_DataCore->GetAggregateInstancePropertyByName(inInstance, inStr);
}

void CStudioPropertySystem::GetAggregateInstanceProperties(Qt3DSDMInstanceHandle inInstance,
                                                           TPropertyHandleList &outProperties) const
{
    m_DataCore->GetAggregateInstanceProperties(inInstance, outProperties);
}

bool CStudioPropertySystem::HasAggregateInstanceProperty(Qt3DSDMInstanceHandle inInstance,
                                                         Qt3DSDMPropertyHandle inProperty) const
{
    return m_DataCore->HasAggregateInstanceProperty(inInstance, inProperty);
}

bool ApplyValueAndReturnTrue(const SValue &inValue, SValue &outValue)
{
    outValue = inValue;
    return true;
}

bool CStudioPropertySystem::GetInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                     Qt3DSDMPropertyHandle inProperty,
                                                     SValue &outValue) const
{
    if (!m_DataCore->IsInstance(inInstance))
        return false;
    if (!m_DataCore->IsProperty(inProperty))
        return false;

    Qt3DSDMSlideHandle theAnimationSlide;
    theAnimationSlide = m_SlideSystem->GetApplicableSlide(inInstance, inProperty);
    SValue theTemp;
    bool retval = SetDefault(GetDataType(inProperty), theTemp);
    if (retval && theAnimationSlide.Valid()
        && m_StudioAnimationSystem->GetAnimatedInstancePropertyValue(theAnimationSlide, inInstance,
                                                                     inProperty, theTemp))
        return ApplyValueAndReturnTrue(theTemp, outValue);
    if (theAnimationSlide.Valid()
        && m_SlideSystem->GetInstancePropertyValue(theAnimationSlide, inInstance, inProperty,
                                                   theTemp))
        return ApplyValueAndReturnTrue(theTemp, outValue);
    return m_DataCore->GetInstancePropertyValue(inInstance, inProperty, outValue);
}

void CStudioPropertySystem::SetInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                     Qt3DSDMPropertyHandle inProperty,
                                                     const SValue &inValue)
{
    m_DataCore->CheckValue(inInstance, inProperty, inValue);

    Qt3DSDMSlideHandle theApplicableSlide;
    theApplicableSlide = m_SlideSystem->GetApplicableSlide(inInstance, inProperty);
    if (theApplicableSlide.Valid()) {
        if (!m_StudioAnimationSystem->SetAnimatedInstancePropertyValue(
                theApplicableSlide, inInstance, inProperty, inValue.toOldSkool())) {
            // Force set on slide where item exists as it doesn't exist on the root.
            m_SlideSystem->ForceSetInstancePropertyValue(theApplicableSlide, inInstance, inProperty,
                                                         inValue.toOldSkool());
        }
    } else {
        m_DataCore->SetInstancePropertyValue(inInstance, inProperty, inValue);
    }
}

Qt3DSDMInstanceHandle CStudioPropertySystem::CreateInstance()
{
    return m_DataCore->CreateInstance();
}

void CStudioPropertySystem::GetInstances(TInstanceHandleList &outInstances) const
{
    m_DataCore->GetInstances(outInstances);
}

void CStudioPropertySystem::DeleteInstance(Qt3DSDMInstanceHandle inHandle)
{
    m_DataCore->DeleteInstance(inHandle);
}

void CStudioPropertySystem::DeriveInstance(Qt3DSDMInstanceHandle inInstance,
                                           Qt3DSDMInstanceHandle inParent)
{
    m_DataCore->DeriveInstance(inInstance, inParent);
}

bool CStudioPropertySystem::IsInstanceOrDerivedFrom(Qt3DSDMInstanceHandle inInstance,
                                                    Qt3DSDMInstanceHandle inParent) const
{
    return m_DataCore->IsInstanceOrDerivedFrom(inInstance, inParent);
}

// Vector of group name and group properties pairs
// Vector used instead of map/hash to preserve the order
QVector<QPair<QString, QVector<Qt3DSDMPropertyHandle>>>
CStudioPropertySystem::GetControllableProperties(Qt3DSDMInstanceHandle inInst) const
{
    vector<Qt3DSDMMetaDataPropertyHandle> propList;
    QVector<QPair<QString, QVector<Qt3DSDMPropertyHandle>>> outVec;
    QHash<QString, int> groupIndexMap;
    m_MetaData->GetMetaDataProperties(inInst, propList);

    int groupIndex = 0;
    for (const auto &it : qAsConst(propList)) {
        auto metadata = m_MetaData->GetMetaDataPropertyInfo(it).getValue();
        const QString groupName = QString::fromStdWString(metadata.m_GroupName.wide_str());
        if ((metadata.m_Controllable
             || (metadata.m_Animatable
                 && m_StudioAnimationSystem->IsPropertyAnimatable(inInst, metadata.m_Property)))
            && !metadata.m_IsHidden) {
            if (!groupIndexMap.contains(groupName)) {
                groupIndexMap.insert(groupName, groupIndex++);
                outVec.append({groupName, {}});
            }
            outVec[groupIndexMap[groupName]].second.append(metadata.m_Property);
        }
    }
    return outVec;
}

Qt3DSDMPropertyHandle CStudioPropertySystem::AddProperty(Qt3DSDMInstanceHandle inInstance,
                                                        TCharPtr inName,
                                                        DataModelDataType::Value inPropType)
{
    return m_DataCore->AddProperty(inInstance, inName, inPropType);
}

bool CStudioPropertySystem::HandleValid(int inHandle) const
{
    return m_DataCore->HandleValid(inHandle);
}

bool CStudioPropertySystem::GetCanonicalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                              Qt3DSDMPropertyHandle inProperty,
                                                              SValue &outValue) const
{
    SValue theTempValue;
    if (m_SlideSystem->GetCanonicalInstancePropertyValue(inInstance, inProperty, theTempValue)) {
        outValue = SValue(theTempValue);
        return true;
    }
    return m_DataCore->GetInstancePropertyValue(inInstance, inProperty, outValue);
}
}
