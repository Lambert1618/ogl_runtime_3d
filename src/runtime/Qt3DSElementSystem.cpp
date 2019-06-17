/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "RuntimePrefix.h"
#include "Qt3DSElementSystem.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "EASTL/sort.h"
#include "foundation/Qt3DSIntrinsics.h"
#include "Qt3DSHash.h"
#include "Qt3DSActivationManager.h"
#include "Qt3DSIPresentation.h"
#include "Qt3DSPresentationFrameData.h"
#include "Qt3DSAttributeHashes.h"
#include "foundation/SerializationTypes.h"
#include "foundation/IOStreams.h"
#include "foundation/Qt3DSIndexableLinkedList.h"

using namespace qt3ds::runtime::element;
using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace qt3ds::intrinsics;
namespace eastl {
template <>
struct hash<STypeDesc>
{
    size_t operator()(const STypeDesc &inDesc) const { return inDesc.HashCode(); }
};
}

namespace {
typedef IndexableLinkedList<SPropertyValueGroup, Q3DStudio::UVariant,
                            SPropertyValueGroup::NumValues>
    TElementPropertyList;
struct SPropertyDescSorter
{
    bool operator()(const TPropertyDescAndValue &lhs, const TPropertyDescAndValue &rhs) const
    {
        return strcmp(lhs.first.m_Name.c_str(), rhs.first.m_Name.c_str()) < 0;
    }
};
QT3DSU32 GetNumValueAllocations(const STypeDesc &inTypeDesc)
{
    return ((inTypeDesc.m_Properties.size() + SPropertyValueGroup::NumValues - 1)
            / SPropertyValueGroup::NumValues);
}
struct SElementAllocator : public qt3ds::runtime::IElementAllocator
{
    typedef Pool<SElement, ForwardingAllocator> TElementPool;
    typedef Pool<SComponent, ForwardingAllocator> TComponentPool;
    typedef TElementPropertyList::TPoolType TValuePool;
    typedef nvhash_map<QT3DSU32, SElement *> THandleElementMap;
    typedef nvhash_map<SElement *, QT3DSU32> TElementOffsetMap;
    typedef nvhash_set<STypeDesc> TTypeDescSet;
    typedef nvhash_map<const STypeDesc *, QT3DSU32> TTypeDescOffsetMap;
    typedef nvhash_map<QT3DSU32, const STypeDesc *> TOffsetTypeDescMap;
    typedef nvvector<NVDataRef<QT3DSU8>> TLoadBufferList;

    NVFoundationBase &m_Foundation;
    IStringTable &m_StringTable;
    TElementPool m_Elements;
    TComponentPool m_Components;
    TValuePool m_Values;
    THandleElementMap m_HandleToElements;
    TElementOffsetMap m_ElementOffsets;
    TTypeDescOffsetMap m_TypeDescOffsets;
    TOffsetTypeDescMap m_OffsetsToTypeDescs;
    TTypeDescSet m_TypeDescriptors;
    nvvector<TPropertyDescAndValue> m_TempPropertyDescsAndValues;
    nvvector<SPropertyDesc> m_TempPropertyDescs;
    nvvector<CRegisteredString> m_IgnoredProperties;
    // Upon load we allocate a set of buffers
    nvvector<QT3DSU8> m_LoadBuffer;
    TLoadBufferList m_AllocatedBuffers;
    SSAutoDeallocatorAllocator m_AutoAllocator;
    nvvector<SElement *> m_LoadElements;

    QT3DSU32 m_NextElementHandle;

    eastl::string m_Workspace;
    eastl::string m_WorkspaceExt;

    QT3DSI32 m_RefCount;

    SElementAllocator(NVFoundationBase &inFoundation, IStringTable &inStrTable)
        : m_Foundation(inFoundation)
        , m_StringTable(inStrTable)
        , m_Elements(ForwardingAllocator(m_Foundation.getAllocator(), "m_Elements"))
        , m_Components(ForwardingAllocator(m_Foundation.getAllocator(), "m_Components"))
        , m_Values(ForwardingAllocator(m_Foundation.getAllocator(), "m_Values"))
        , m_HandleToElements(m_Foundation.getAllocator(), "m_HandleToElements")
        , m_ElementOffsets(m_Foundation.getAllocator(), "m_ElementOffsets")
        , m_TypeDescOffsets(m_Foundation.getAllocator(), "m_TypeDescOffsets")
        , m_OffsetsToTypeDescs(m_Foundation.getAllocator(), "m_OffsetsToTypeDescs")
        , m_TypeDescriptors(m_Foundation.getAllocator(), "m_TypeDescriptors")
        , m_TempPropertyDescsAndValues(m_Foundation.getAllocator(), "m_TempPropertyDescsAndValues")
        , m_TempPropertyDescs(m_Foundation.getAllocator(), "m_TempPropertyDescs")
        , m_IgnoredProperties(m_Foundation.getAllocator(), "m_IgnoredProperties")
        , m_LoadBuffer(m_Foundation.getAllocator(), "m_LoadBuffer")
        , m_AllocatedBuffers(m_Foundation.getAllocator(), "m_AllocatedBuffers")
        , m_AutoAllocator(inFoundation)
        , m_LoadElements(inFoundation.getAllocator(), "m_LoadElements")
        , m_NextElementHandle(1)
        , m_RefCount(0)
    {
    }

    ~SElementAllocator() {}

    void GetIgnoredProperties()
    {
        if (m_IgnoredProperties.empty())
            m_IgnoredProperties.push_back(m_StringTable.RegisterStr("name"));
    }

    void addRef() override { atomicIncrement(&m_RefCount); }

    void release() override
    {
        atomicDecrement(&m_RefCount);
        if (m_RefCount <= 0) {
            NVAllocatorCallback &alloc(m_Foundation.getAllocator());
            NVDelete(alloc, this);
        }
    }

    SElement &CreateElement(CRegisteredString inName, CRegisteredString inType,
                                    CRegisteredString inSubType,
                                    NVConstDataRef<TPropertyDescAndValue> inPropertyDescriptions,
                                    Q3DStudio::IPresentation *inPresentation, SElement *inParent,
                                    bool inIsComponent) override
    {
        m_TempPropertyDescsAndValues.clear();
        m_TempPropertyDescs.clear();
        bool participatesInTimeGraph = false;
        GetIgnoredProperties();
        for (QT3DSU32 idx = 0, end = inPropertyDescriptions.size(); idx < end; ++idx) {
            QT3DSU32 nameHash = inPropertyDescriptions[idx].first.GetNameHash();
            if (nameHash == Q3DStudio::ATTRIBUTE_STARTTIME
                || nameHash == Q3DStudio::ATTRIBUTE_ENDTIME)
                participatesInTimeGraph = true;
            if (eastl::find(m_IgnoredProperties.begin(), m_IgnoredProperties.end(),
                            inPropertyDescriptions[idx].first.m_Name)
                == m_IgnoredProperties.end()) {
                m_TempPropertyDescsAndValues.push_back(inPropertyDescriptions[idx]);
            }
        }
        eastl::sort(m_TempPropertyDescsAndValues.begin(), m_TempPropertyDescsAndValues.end(),
                    SPropertyDescSorter());

        for (QT3DSU32 idx = 0, end = m_TempPropertyDescsAndValues.size(); idx < end; ++idx) {
            m_TempPropertyDescs.push_back(m_TempPropertyDescsAndValues[idx].first);
        }

        STypeDesc theDesc;
        theDesc.m_TypeName = inType;
        theDesc.m_SubtypeName = inSubType;
        theDesc.m_Properties = m_TempPropertyDescs;
        theDesc.SetHashValue();
        eastl::pair<TTypeDescSet::iterator, bool> inserter = m_TypeDescriptors.insert(theDesc);

        const STypeDesc &theTypeDesc = *inserter.first;
        if (inserter.second) {
            size_t allocSize = theDesc.m_Properties.size() * sizeof(SPropertyDesc);
            if (allocSize) {
                SPropertyDesc *newProps = (SPropertyDesc *)m_AutoAllocator.allocate(
                    allocSize, "TypeDescData", __FILE__, __LINE__, 0);
                memCopy(newProps, theDesc.m_Properties.begin(), (QT3DSU32)allocSize);
                // Note that this does not change the hash value.
                const_cast<STypeDesc &>(theTypeDesc).m_Properties =
                    toConstDataRef(newProps, theDesc.m_Properties.size());
            }
        }

        SElement *retval = inIsComponent ? m_Components.construct(theTypeDesc, __FILE__, __LINE__)
                                         : m_Elements.construct(theTypeDesc, __FILE__, __LINE__);
        retval->m_BelongedPresentation = inPresentation;
        retval->m_Name = inName;
        // children
        if (inParent) {
            retval->m_Parent = inParent;
            if (inParent->m_Child == NULL) {
                inParent->m_Child = retval;
            } else {
                SElement *lastChild = inParent->m_Child;
                while (lastChild->m_Sibling)
                    lastChild = lastChild->m_Sibling;
                lastChild->m_Sibling = retval;
            }
        }
        while (FindElementByHandle(m_NextElementHandle)) {
            ++m_NextElementHandle;
            if (!m_NextElementHandle)
                ++m_NextElementHandle;
        }

        // required flags
        retval->m_Handle = m_NextElementHandle;
        if (inIsComponent)
            retval->Flags().clearOrSet(true, Q3DStudio::ELEMENTFLAG_COMPONENT);

        if (participatesInTimeGraph)
            retval->Flags().clearOrSet(true, Q3DStudio::ELEMENTFLAG_TIMELINE);

        retval->Flags().clearOrSet(true, Q3DStudio::ELEMENTFLAG_EXPLICITACTIVE);

        retval->SetDepth();

        retval->SetDirty();

        m_HandleToElements.insert(eastl::make_pair(retval->m_Handle, retval));

        // property values;
        QT3DSU32 propIdx = 0;
        TElementPropertyList::CreateAll(retval->m_PropertyValues, theTypeDesc.m_Properties.size(),
                                        m_Values);
        for (TElementPropertyList::iterator
                 iter = TElementPropertyList::begin(retval->m_PropertyValues,
                                                    theTypeDesc.m_Properties.size()),
                 end = TElementPropertyList::end(retval->m_PropertyValues,
                                                 theTypeDesc.m_Properties.size());
             iter != end; ++iter, ++propIdx) {
            (*iter) = m_TempPropertyDescsAndValues[propIdx].second;
        }

        return *retval;
    }

    void ReleaseElement(SElement &inElement, bool inRecurse) override
    {
        if (inRecurse) {
            while (inElement.m_Child)
                ReleaseElement(*inElement.m_Child, true);
        }
        // Trim out the element.
        if (inElement.m_Parent) {
            SElement *theParent = inElement.m_Parent;
            if (theParent->m_Child == &inElement)
                theParent->m_Child = inElement.m_Sibling;
            else {
                SElement *thePreviousChild = NULL;
                // Empty loop to find the previous child
                for (thePreviousChild = theParent->m_Child;
                     thePreviousChild->m_Sibling != &inElement && thePreviousChild;
                     thePreviousChild = thePreviousChild->m_Sibling) {
                }
                if (thePreviousChild)
                    thePreviousChild->m_Sibling = inElement.m_Sibling;
            }
        }

        m_HandleToElements.erase(inElement.m_Handle);

        QT3DSU8 *elemAddr = reinterpret_cast<QT3DSU8 *>(&inElement);
        for (QT3DSU32 idx = 0, end = m_AllocatedBuffers.size(); idx < end; ++idx) {
            NVDataRef<QT3DSU8> theBuffer(m_AllocatedBuffers[idx]);
            // Preloaded element, do not release back to element pools
            if (elemAddr >= theBuffer.begin() && elemAddr < theBuffer.end())
                return;
        }

        SPropertyValueGroup *theValueGroup = inElement.m_PropertyValues;
        while (theValueGroup) {
            SPropertyValueGroup *currentGroup = theValueGroup;
            theValueGroup = theValueGroup->m_NextNode;
            m_Values.deallocate(currentGroup);
        }

        if (inElement.IsComponent())
            m_Components.deallocate(&inElement);
        else
            m_Elements.deallocate(&inElement);
    }

    QT3DSI32 GetExtensionIndex(eastl::string &inExt)
    {
        QT3DSI32 retVal = 0;

        if (!inExt.compare(".x") || !inExt.compare(".r"))
            retVal = 0;
        else if (!inExt.compare(".y") || !inExt.compare(".g"))
            retVal = 1;
        if (!inExt.compare(".z") || !inExt.compare(".b"))
            retVal = 2;

        return retVal;
    }

    virtual Option<TPropertyDescAndValuePtr>
    CreateDynamicProperty(Q3DStudio::IRuntimeMetaData &theMetaData, SElement &element,
                          CRegisteredString inName) override
    {
        SPropertyDesc *newProp = NULL;
        m_TempPropertyDescsAndValues.clear();
        m_TempPropertyDescs.clear();

        // create dynamic descriptor
        if (!element.m_DynamicTypeDescription) {
            STypeDesc *theDesc;
            theDesc = (STypeDesc *)m_AutoAllocator.allocate(sizeof(STypeDesc), "TypeDescData",
                                                            __FILE__, __LINE__, 0);
            theDesc->m_TypeName = element.m_TypeDescription->m_TypeName;
            theDesc->m_SubtypeName = element.m_TypeDescription->m_SubtypeName;
            theDesc->m_Properties = m_TempPropertyDescs;
            theDesc->SetHashValue();
            // we need this palcement new here
            ::new (theDesc) STypeDesc();
            element.m_DynamicTypeDescription = const_cast<STypeDesc *>(theDesc);
        }

        STypeDesc *theTypeDesc = element.m_DynamicTypeDescription;

        // remove the extension that we can find the property
        QT3DSI32 extIndex = 0;
        m_Workspace.assign(inName.c_str());
        eastl::string::size_type theCharPos = m_Workspace.rfind('.');
        if (theCharPos != eastl::string::npos) {
            m_WorkspaceExt.assign(m_Workspace, theCharPos, m_Workspace.length());
            m_Workspace.resize(theCharPos);
            extIndex = GetExtensionIndex(m_WorkspaceExt);
        }

        CRegisteredString theWorkSpaceString = m_StringTable.RegisterStr(m_Workspace.c_str());
        Q3DStudio::ERuntimeDataModelDataType thePropertyType =
            theMetaData.GetPropertyType(element.m_TypeDescription->m_TypeName, theWorkSpaceString,
                                        element.m_TypeDescription->m_SubtypeName);

        // using separate properties
        bool separateProperties = theCharPos != eastl::string::npos;

        // single property
        size_t allocSize = sizeof(SPropertyDesc);
        if (allocSize) {
            newProp = (SPropertyDesc *)m_AutoAllocator.allocate(allocSize, "SPropertyDesc",
                                                                __FILE__, __LINE__, 0);

            newProp->m_Name = inName;

            // convert to appropriate type
            if (thePropertyType == Q3DStudio::ERuntimeDataModelDataTypeFloat) {
                float value = theMetaData.GetPropertyValueFloat(
                    element.m_TypeDescription->m_TypeName, theWorkSpaceString,
                    element.m_TypeDescription->m_SubtypeName);

                newProp->m_Type = Q3DStudio::ATTRIBUTETYPE_FLOAT;

                Q3DStudio::UVariant theValue;
                theValue.m_FLOAT = value;
                m_TempPropertyDescsAndValues.push_back(eastl::make_pair(
                    SPropertyDesc(theWorkSpaceString, Q3DStudio::ATTRIBUTETYPE_FLOAT), theValue));

                theTypeDesc->m_DynamicProperties.push_back(*newProp);
            } else if (thePropertyType == Q3DStudio::ERuntimeDataModelDataTypeFloat2) {
                qt3ds::QT3DSVec2 value = theMetaData.GetPropertyValueVector2(
                    element.m_TypeDescription->m_TypeName, theWorkSpaceString,
                    element.m_TypeDescription->m_SubtypeName);
                newProp->m_Type = Q3DStudio::ATTRIBUTETYPE_FLOAT;

                Q3DStudio::UVariant theVarValue;
                theVarValue.m_FLOAT = value[extIndex];
                m_TempPropertyDescsAndValues.push_back(eastl::make_pair(
                    SPropertyDesc(theWorkSpaceString, Q3DStudio::ATTRIBUTETYPE_FLOAT),
                    theVarValue));

                theTypeDesc->m_DynamicProperties.push_back(*newProp);
            } else if (thePropertyType == Q3DStudio::ERuntimeDataModelDataTypeFloat3) {
                qt3ds::QT3DSVec3 value = theMetaData.GetPropertyValueVector3(
                    element.m_TypeDescription->m_TypeName, theWorkSpaceString,
                    element.m_TypeDescription->m_SubtypeName);

                if (separateProperties) {
                    newProp->m_Type = Q3DStudio::ATTRIBUTETYPE_FLOAT;

                    Q3DStudio::UVariant theVarValue;
                    theVarValue.m_FLOAT = value[extIndex];
                    m_TempPropertyDescsAndValues.push_back(eastl::make_pair(
                        SPropertyDesc(theWorkSpaceString, Q3DStudio::ATTRIBUTETYPE_FLOAT),
                        theVarValue));
                } else {
                    newProp->m_Type = Q3DStudio::ATTRIBUTETYPE_FLOAT3;

                    Q3DStudio::UVariant theVarValue;
                    theVarValue.m_FLOAT3[0] = value[0];
                    theVarValue.m_FLOAT3[1] = value[1];
                    theVarValue.m_FLOAT3[2] = value[2];
                    m_TempPropertyDescsAndValues.push_back(eastl::make_pair(
                        SPropertyDesc(theWorkSpaceString, Q3DStudio::ATTRIBUTETYPE_FLOAT3),
                        theVarValue));
                }

                theTypeDesc->m_DynamicProperties.push_back(*newProp);
            } else if (thePropertyType == Q3DStudio::ERuntimeDataModelDataTypeLong) {
                QT3DSI32 value = (QT3DSI32)theMetaData.GetPropertyValueLong(
                    element.m_TypeDescription->m_TypeName, theWorkSpaceString,
                    element.m_TypeDescription->m_SubtypeName);

                newProp->m_Type = Q3DStudio::ATTRIBUTETYPE_INT32;

                Q3DStudio::UVariant theVarValue;
                theVarValue.m_INT32 = value;
                m_TempPropertyDescsAndValues.push_back(eastl::make_pair(
                    SPropertyDesc(theWorkSpaceString, Q3DStudio::ATTRIBUTETYPE_INT32),
                    theVarValue));

                theTypeDesc->m_DynamicProperties.push_back(*newProp);
            } else if (thePropertyType == Q3DStudio::ERuntimeDataModelDataTypeBool) {
                bool value = theMetaData.GetPropertyValueBool(
                    element.m_TypeDescription->m_TypeName, theWorkSpaceString,
                    element.m_TypeDescription->m_SubtypeName);

                newProp->m_Type = Q3DStudio::ATTRIBUTETYPE_BOOL;

                Q3DStudio::UVariant theVarValue;
                theVarValue.m_INT32 = value;
                m_TempPropertyDescsAndValues.push_back(eastl::make_pair(
                    SPropertyDesc(theWorkSpaceString, Q3DStudio::ATTRIBUTETYPE_BOOL), theVarValue));

                theTypeDesc->m_DynamicProperties.push_back(*newProp);
            } else if (thePropertyType == Q3DStudio::ERuntimeDataModelDataTypeString) {
                Option<eastl::string> theRuntimeStr = theMetaData.GetPropertyValueString(
                    element.m_TypeDescription->m_TypeName, theWorkSpaceString,
                    element.m_TypeDescription->m_SubtypeName);

                newProp->m_Type = Q3DStudio::ATTRIBUTETYPE_STRING;

                CStringHandle theString = m_StringTable.GetHandle(theRuntimeStr->c_str());
                Q3DStudio::UVariant theVarValue;
                theVarValue.m_StringHandle = theString.handle();
                m_TempPropertyDescsAndValues.push_back(eastl::make_pair(
                    SPropertyDesc(theWorkSpaceString, Q3DStudio::ATTRIBUTETYPE_STRING),
                    theVarValue));

                theTypeDesc->m_DynamicProperties.push_back(*newProp);
            } else {
                newProp->m_Type = Q3DStudio::ATTRIBUTETYPE_FLOAT;

                theTypeDesc->m_DynamicProperties.push_back(*newProp);
            }
        }

        // property values;
        QT3DSU32 newListSize = theTypeDesc->m_DynamicProperties.size() - 1;
        TElementPropertyList::Create(element.m_DynamicPropertyValues, newListSize, m_Values);

        Q3DStudio::UVariant &propertyValue =
            TElementPropertyList::GetObjAtIdx(element.m_DynamicPropertyValues, newListSize - 1);
        propertyValue = m_TempPropertyDescsAndValues[0].second;

        return eastl::make_pair(
            element.m_DynamicTypeDescription->m_DynamicProperties[newListSize - 1],
            &TElementPropertyList::GetObjAtIdx(element.m_DynamicPropertyValues, newListSize - 1));
    }

    SElement *FindElementByHandle(QT3DSU32 inElementHandle) override
    {
        THandleElementMap::iterator theFind = m_HandleToElements.find(inElementHandle);
        if (theFind != m_HandleToElements.end())
            return theFind->second;
        return NULL;
    }

    // Returns an element pointer that when added to the return value of load will be a valid
    // element.
    SElement *GetRemappedElementAddress(SElement *inElement) const override
    {
        if (inElement == NULL)
            return NULL;
        TElementOffsetMap::const_iterator iter = m_ElementOffsets.find(inElement);
        if (iter != m_ElementOffsets.end())
            return reinterpret_cast<SElement *>(iter->second);
        return NULL;
    }

    const STypeDesc *GetRemappedTypeDescAddress(const STypeDesc *inTypeDesc) const override
    {
        TTypeDescOffsetMap::const_iterator iter = m_TypeDescOffsets.find((STypeDesc *)inTypeDesc);
        if (iter != m_TypeDescOffsets.end())
            return reinterpret_cast<STypeDesc *>(iter->second);
        return NULL;
    }
    const STypeDesc *RemapTypeDesc(const STypeDesc *inOffset)
    {
        size_t memoryOffset = reinterpret_cast<size_t>(inOffset);
        TOffsetTypeDescMap::iterator iter =
            m_OffsetsToTypeDescs.find(static_cast<QT3DSU32>(memoryOffset));
        if (iter != m_OffsetsToTypeDescs.end())
            return iter->second;
        return NULL;
    }

    SElement *RemapElement(SElement *inElem, size_t memoryOffset)
    {
        if (inElem)
            return reinterpret_cast<SElement *>(reinterpret_cast<size_t>(inElem) + memoryOffset);
        return NULL;
    }
};
}

bool STypeDesc::operator==(const STypeDesc &inOther) const
{
    if (m_TypeName == inOther.m_TypeName && m_SubtypeName == inOther.m_SubtypeName
        && m_Properties.size() == inOther.m_Properties.size()) {
        for (QT3DSU32 idx = 0, end = m_Properties.size(); idx < end; ++idx) {
            const SPropertyDesc &lhs = m_Properties[idx];
            const SPropertyDesc &rhs = inOther.m_Properties[idx];
            if (lhs.m_Name != rhs.m_Name || lhs.m_Type != rhs.m_Type)
                return false;
        }
        return true;
    }
    return false;
}

Option<QT3DSU32> STypeDesc::FindProperty(QT3DSU32 inNameHash) const
{
    for (QT3DSU32 idx = 0, end = m_Properties.size(); idx < end; ++idx) {
        const SPropertyDesc &theDesc = m_Properties[idx];
        if (Q3DStudio::CHash::HashAttribute(theDesc.m_Name) == inNameHash)
            return idx;
    }
    return Empty();
}

Option<QT3DSU32> STypeDesc::FindProperty(CRegisteredString inName) const
{
    for (QT3DSU32 idx = 0, end = m_Properties.size(); idx < end; ++idx) {
        const SPropertyDesc &theDesc = m_Properties[idx];
        if (theDesc.m_Name == inName)
            return idx;
    }
    return Empty();
}

Option<QT3DSU32> STypeDesc::FindDynamicProperty(QT3DSU32 inNameHash) const
{
    for (QT3DSU32 idx = 0, end = m_DynamicProperties.size(); idx < end; ++idx) {
        const SPropertyDesc &theDesc = m_DynamicProperties[idx];
        if (Q3DStudio::CHash::HashAttribute(theDesc.m_Name) == inNameHash)
            return idx;
    }
    return Empty();
}

void STypeDesc::SetHashValue()
{
    size_t typeDescHash = m_TypeName.hash() ^ m_SubtypeName.hash();
    for (QT3DSU32 idx = 0, end = m_Properties.size(); idx < end; ++idx) {
        typeDescHash = typeDescHash ^ m_Properties[idx].m_Name.hash();
        typeDescHash = typeDescHash ^ eastl::hash<QT3DSU32>()(m_Properties[idx].m_Type);
    }
    // TODO check 64 bit compatibility
    m_HashValue = (QT3DSU32)typeDescHash;
}

QT3DSU32 SPropertyDesc::GetNameHash() const
{
    return Q3DStudio::CHash::HashAttribute(m_Name.c_str());
}

Q3DStudio::SAttributeKey SPropertyDesc::GetAttributeKey() const
{
    Q3DStudio::SAttributeKey retval;
    memZero(&retval, sizeof(retval));
    retval.m_Type = m_Type;
    retval.m_Hash = GetNameHash();
    return retval;
}

bool SElement::IsUserActive() const
{
    return m_Flags.IsExplicitActive();
}

bool SElement::IsIndependent() const
{
    return IsComponent();
}

bool SElement::IsGlobalActive() const
{
    return GetActive();
}
// This may require a mutex to be held.
void SElement::SetGlobalActive(bool active)
{
    SetActive(active);
}

bool SElement::GetAttribute(QT3DSU32 inHashName, Q3DStudio::UVariant &outValue) const
{
    if (inHashName == Q3DStudio::ATTRIBUTE_NAME) {
        outValue.m_StringHandle = m_BelongedPresentation->GetStringTable().GetHandle(m_Name);
        return true;
    }
    if (inHashName == Q3DStudio::CHash::HashAttribute("path")) {
        outValue.m_StringHandle = m_BelongedPresentation->GetStringTable().GetHandle(m_Path);
        return true;
    }
    const Q3DStudio::UVariant *valPtr = FindPropertyValue(inHashName);
    if (valPtr) {
        outValue = *valPtr;
        return true;
    }
    return false;
}

void SElement::SetAttribute(const Q3DStudio::TAttributeHash inKey,
                            const Q3DStudio::UVariant inValue)
{
    Option<TPropertyDescAndValuePtr> existing = FindProperty(inKey);
    if (existing.hasValue() == false) {
        if (Q3DStudio::ATTRIBUTE_EYEBALL == inKey)
            SetFlag(Q3DStudio::ELEMENTFLAG_EXPLICITACTIVE, inValue.m_INT32 ? true : false);
        return;
    }
    SetAttribute(*existing, inValue);
}

void SElement::SetAttribute(TPropertyDescAndValuePtr inKey, const Q3DStudio::UVariant inValue,
                            bool forceSet)
{
    Q3DStudio::EAttributeType theType = inKey.first.m_Type;
    Q3DStudio::UVariant *currentValue = inKey.second;
    QT3DSU32 attHash = inKey.first.GetNameHash();
    if (!forceSet) {
        switch (theType) {
        case Q3DStudio::ATTRIBUTETYPE_FLOAT: // Early return
            if (fabs(currentValue->m_FLOAT - inValue.m_FLOAT) < SmallestDifference())
                return;
            break;
        case Q3DStudio::ATTRIBUTETYPE_FLOAT3: // Early return
            if (fabs(currentValue->m_FLOAT3[0] - inValue.m_FLOAT3[0]) < SmallestDifference()
                && fabs(currentValue->m_FLOAT3[1] - inValue.m_FLOAT3[1]) < SmallestDifference()
                && fabs(currentValue->m_FLOAT3[2] - inValue.m_FLOAT3[2]) < SmallestDifference()) {
                return;
            }
            break;
        case Q3DStudio::ATTRIBUTETYPE_STRING:
            if (currentValue->m_StringHandle == inValue.m_StringHandle)
                return;
            break;
        default: // Early return
            if (Q3DStudio::ATTRIBUTE_EYEBALL != attHash && currentValue->m_INT32 == inValue.m_INT32)
                return;
            break;
        }
    }
    *currentValue = inValue;

    if (Q3DStudio::ATTRIBUTE_EYEBALL == attHash)
        SetFlag(Q3DStudio::ELEMENTFLAG_EXPLICITACTIVE, inValue.m_INT32 ? true : false);

    if (Q3DStudio::ATTRIBUTE_STARTTIME == attHash || Q3DStudio::ATTRIBUTE_ENDTIME == attHash)
        GetActivityZone().UpdateItemInfo(*this);

    /*
    if ( inElement->m_Flags & ELEMENTFLAG_REGISTEREDFORATTRIBUTECHANGE )
            m_AttributeChangeCallbacks.FireCallbacks( inElement, theAttribute->m_Key,
    thePreviousValue, inValue );
            */
    SetDirty();
}

// SElement implementation
QT3DSU32 SElement::GetNameHash() const
{
    return Q3DStudio::CHash::HashString(m_Name.c_str());
}

// Q3DStudio::CHash::HashAttribute
Option<QT3DSU32> SElement::FindPropertyIndex(QT3DSU32 inNameHash) const
{
    return m_TypeDescription->FindProperty(inNameHash);
}

Option<QT3DSU32> SElement::FindPropertyIndex(CRegisteredString inName) const
{
    return m_TypeDescription->FindProperty(inName);
}

Option<QT3DSU32> SElement::FindDynamicPropertyIndex(QT3DSU32 inNameHash) const
{
    if (m_DynamicTypeDescription)
        return m_DynamicTypeDescription->FindDynamicProperty(inNameHash);
    else
        return Empty();
}

QT3DSU32 SElement::GetNumProperties() const
{
    return m_TypeDescription->m_Properties.size();
}

QT3DSU32 SElement::GetNumDynamicProperties() const
{
    if (m_DynamicTypeDescription)
        return m_DynamicTypeDescription->m_DynamicProperties.size();
    else
        return 0;
}

Option<TPropertyDescAndValuePtr> SElement::GetPropertyByIndex(QT3DSU32 inIdx)
{
    if (inIdx < this->m_TypeDescription->m_Properties.size())
        return eastl::make_pair(m_TypeDescription->m_Properties[inIdx],
                                &TElementPropertyList::GetObjAtIdx(m_PropertyValues, inIdx));

    return Empty();
}

Option<TPropertyDescAndValuePtr> SElement::GetDynamicPropertyByIndex(QT3DSU32 inIdx)
{
    if (inIdx < this->m_DynamicTypeDescription->m_DynamicProperties.size())
        return eastl::make_pair(m_DynamicTypeDescription->m_DynamicProperties[inIdx],
                                &TElementPropertyList::GetObjAtIdx(m_DynamicPropertyValues, inIdx));

    return Empty();
}

Option<TPropertyDescAndConstValuePtr> SElement::GetPropertyByIndex(QT3DSU32 inIdx) const
{
    Option<TPropertyDescAndValuePtr> retval =
        const_cast<SElement &>(*this).GetPropertyByIndex(inIdx);
    if (retval.hasValue())
        return TPropertyDescAndConstValuePtr(*retval);
    return Empty();
}

void SElement::SetDirty()
{
    if (!m_Flags.IsDirty()) {
        m_Flags.SetDirty(true);
        // The element is dirty, so place it on the frame data dirty list
        m_BelongedPresentation->GetFrameData().GetDirtyList().Push(this);
        m_Flags.SetDirty(true);
    }
}

void SElement::SetFlag(Q3DStudio::EElementFlag inFlag, bool inValue)
{
    bool existing = m_Flags & inFlag;
    if (existing != inValue && HasActivityZone()) {
        m_Flags.clearOrSet(inValue, inFlag);
        if (inFlag == Q3DStudio::ELEMENTFLAG_EXPLICITACTIVE) {
            GetActivityZone().UpdateItemInfo(*this);
            if (IsComponent()) {
                SElement *parent = m_Parent;
                // Get out parent component
                while (parent && parent->m_Parent && !parent->IsComponent())
                    parent = parent->m_Parent;
                if (parent)
                    parent->GetActivityZone().UpdateItemInfo(*parent);
            }
        } else if (inFlag == Q3DStudio::ELEMENTFLAG_SCRIPTCALLBACKS) {
            GetActivityZone().UpdateItemScriptStatus(*this);
        }
        SetDirty();
    }
}

bool SElement::HasActivityZone() const
{
    return m_BelongedPresentation->GetActivityZone() != NULL;
}

qt3ds::runtime::IActivityZone &SElement::GetActivityZone() const
{
    return *m_BelongedPresentation->GetActivityZone();
}

SElement &SElement::GetComponentParent()
{
    if (IsComponent())
        return *this;

    SElement *theParent = GetActivityZone().GetItemTimeParent(*this);
    if (theParent)
        return *theParent;
    QT3DS_ASSERT(false);
    return *this;
}

const SElement &SElement::GetComponentParent() const
{
    return const_cast<SElement *>(this)->GetComponentParent();
}

SElement *SElement::FindChild(QT3DSU32 inNameHash)
{
    for (SElement *theChild = m_Child; theChild; theChild = theChild->m_Sibling) {
        if (theChild->GetNameHash() == inNameHash)
            return theChild;
    }
    return NULL;
}

Q3DStudio::TTimeUnit SElement::GetInnerTime() const
{
    return GetActivityZone().GetItemComponentTime(const_cast<SElement &>(*this));
}
// Get the time I am animating at.
Q3DStudio::TTimeUnit SElement::GetOuterTime() const
{
    return GetActivityZone().GetItemLocalTime(const_cast<SElement &>(*this));
}

bool SElement::IsDescendent(SElement &inPossibleParent) const
{
    if (m_Parent == NULL)
        return false;
    if (m_Parent == &inPossibleParent)
        return true;
    return m_Parent->IsDescendent(inPossibleParent);
}

bool SComponent::GetPaused() const
{
    return GetTimePolicy().GetPaused();
}

bool SComponent::GetPlayBackDirection() const
{
    return GetTimePolicy().GetPlayBackDirection();
}

Q3DStudio::CTimePolicy &SComponent::GetTimePolicy()
{
    return *GetActivityZone().GetOwnedTimePolicy(*this);
}

const Q3DStudio::CTimePolicy &SComponent::GetTimePolicy() const
{
    return const_cast<SComponent *>(this)->GetTimePolicy();
}

qt3ds::runtime::IElementAllocator &
qt3ds::runtime::IElementAllocator::CreateElementAllocator(NVFoundationBase &inFoundation,
                                                        IStringTable &inStringTable)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SElementAllocator)(inFoundation, inStringTable);
}
