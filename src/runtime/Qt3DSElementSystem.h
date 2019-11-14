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

#pragma once
#ifndef QT3DS_ELEMENT_SYSTEM_H
#define QT3DS_ELEMENT_SYSTEM_H
#include "foundation/Qt3DSRefCounted.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Utils.h"
#include "foundation/StringTable.h"
#include "Qt3DSKernelTypes.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSFlags.h"
#include "foundation/Qt3DSOption.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSInvasiveSet.h"
#include "Qt3DSMetadata.h"

// The qt3ds runtime element system contains the element graph and the data in the element
// graph.

namespace Q3DStudio {
// class CPresentation;
class IPresentation;
class CTimePolicy;
}

namespace qt3ds {
namespace foundation {
    class IInStream;
    class IOutStream;
}
}

namespace qt3ds {
namespace runtime {
    using namespace qt3ds::foundation;
    using namespace qt3ds;
    class IActivityZone;
    namespace element {
        struct SPropertyDesc
        {
            CRegisteredString m_Name;
            Q3DStudio::EAttributeType m_Type;
            SPropertyDesc()
                : m_Type(Q3DStudio::ATTRIBUTETYPE_NONE)
            {
            }
            SPropertyDesc(CRegisteredString inStr, Q3DStudio::EAttributeType inType)
                : m_Name(inStr)
                , m_Type(inType)
            {
            }
            QT3DSU32 GetNameHash() const; // CHash::HashAttribute
            Q3DStudio::SAttributeKey GetAttributeKey() const;
        };

        struct STypeDesc
        {
            CRegisteredString m_TypeName;
            CRegisteredString m_SubtypeName;
            // Properties are sorted so that we can quickly create new type descriptions
            // when necessary.
            NVConstDataRef<SPropertyDesc> m_Properties;
            eastl::vector<SPropertyDesc> m_DynamicProperties;

        private:
            QT3DSU32 m_HashValue; // Cached hash value for this type descriptor
        public:
            STypeDesc()
                : m_HashValue(0)
            {
            }
            bool operator==(const STypeDesc &inOther) const;
            // Q3DStudio::CHash::HashAttribute
            Option<QT3DSU32> FindProperty(QT3DSU32 inNameHash) const;
            Option<QT3DSU32> FindProperty(CRegisteredString inName) const;
            Option<QT3DSU32> FindDynamicProperty(QT3DSU32 inNameHash) const;
            void SetHashValue();
            QT3DSU32 HashCode() const { return m_HashValue; }
        };

        struct SActivationManagerNodeFlagValues
        {
            enum Enum {
                TimeActive = 1 << 1,
                TempTimeActive = 1 << 4,
                ParticipatesInTimeGraph = 1 << 5,
                Script = 1 << 6,
                ChildDirty = 1 << 7,
            };
        };

        struct SActivationManagerNodeFlags : public NVFlags<SActivationManagerNodeFlagValues::Enum>
        {
            bool IsTimeActive() const
            {
                return operator&(SActivationManagerNodeFlagValues::TimeActive);
            }
            void SetTimeActive(bool active)
            {
                clearOrSet(active, SActivationManagerNodeFlagValues::TimeActive);
            }

            bool IsTempTimeActive() const
            {
                return operator&(SActivationManagerNodeFlagValues::TempTimeActive);
            }
            void SetTempTimeActive(bool value)
            {
                clearOrSet(value, SActivationManagerNodeFlagValues::TempTimeActive);
            }

            // Is this item in the activation manager's scripts list.
            bool HasScript() const { return operator&(SActivationManagerNodeFlagValues::Script); }
            void SetHasScript(bool value)
            {
                clearOrSet(value, SActivationManagerNodeFlagValues::Script);
            }

            bool IsChildDirty() const
            {
                return operator&(SActivationManagerNodeFlagValues::ChildDirty);
            }
            void SetChildDirty() { clearOrSet(true, SActivationManagerNodeFlagValues::ChildDirty); }
            void ClearChildDirty()
            {
                clearOrSet(false, SActivationManagerNodeFlagValues::ChildDirty);
            }
        };

        struct SElement;

        struct SActivationManagerNode
        {
            // IT may seem wasteful to always have start,end time on the node.  We need to do this
            // however
            // in order to transmit information down the timeline at times.  Furthermore the
            // activation manager
            // is free to mangle these times as it sees fit; unlike the attribute start,end time.
            QT3DSU32 m_StartTime;
            QT3DSU32 m_StopTime;
            // Used by the activation manager specifically.
            QT3DSU32 m_DirtyIndex;

            // If m_Flags.IsIndependent(), first child is implicitly zero and instead
            // this records the time context index assocated with this element node.
            // Else this records the first child index.
            // These could perhaps be moved to the element flags except it breaks some isolation and
            // it just doesn't seem worth it.
            SActivationManagerNodeFlags m_Flags;

            SActivationManagerNode()
                : m_StartTime(QT3DS_MAX_U32)
                , m_StopTime(QT3DS_MAX_U32)
                , m_DirtyIndex(QT3DS_MAX_U32)
            {
            }
        };

        struct SElementFlag : public NVFlags<Q3DStudio::EElementFlag, QT3DSU16>
        {
            void SetDirty(bool inDirty) { clearOrSet(inDirty, Q3DStudio::ELEMENTFLAG_DIRTY); }
            bool IsDirty() const { return this->operator&(Q3DStudio::ELEMENTFLAG_DIRTY); }

            bool IsComponent() const { return this->operator&(Q3DStudio::ELEMENTFLAG_COMPONENT); }

            void SetExplicitActive(bool inValue)
            {
                clearOrSet(inValue, Q3DStudio::ELEMENTFLAG_EXPLICITACTIVE);
            }
            bool IsExplicitActive() const
            {
                return this->operator&(Q3DStudio::ELEMENTFLAG_EXPLICITACTIVE);
            }
            bool IsControlledActive() const
            {
                return this->operator&(Q3DStudio::ELEMENTFLAG_CONTROLLED_ACTIVE);
            }
            void SetActive(bool inValue)
            {
                clearOrSet(inValue, Q3DStudio::ELEMENTFLAG_GLOBALACTIVE);
            }
            bool IsActive() const { return this->operator&(Q3DStudio::ELEMENTFLAG_GLOBALACTIVE); }

            void SetPickEnabled(bool inValue)
            {
                clearOrSet(inValue, Q3DStudio::ELEMENTFLAG_PICKENABLED);
            }
            bool IsPickEnabled() const
            {
                return this->operator&(Q3DStudio::ELEMENTFLAG_PICKENABLED);
            }

            void SetPlayThrough(bool inValue)
            {
                clearOrSet(inValue, Q3DStudio::ELEMENTFLAG_PLAYTHROUGH);
            }
            bool IsPlayThrough() const
            {
                return this->operator&(Q3DStudio::ELEMENTFLAG_PLAYTHROUGH);
            }

            void SetScriptCallbacks(bool inValue)
            {
                clearOrSet(inValue, Q3DStudio::ELEMENTFLAG_SCRIPTCALLBACKS);
            }
            bool HasScriptCallbacks() const
            {
                return this->operator&(Q3DStudio::ELEMENTFLAG_SCRIPTCALLBACKS);
            }

            bool HasStartEndTime() const
            {
                return this->operator&(Q3DStudio::ELEMENTFLAG_TIMELINE);
            }
        };

        typedef eastl::pair<SPropertyDesc, Q3DStudio::UVariant *> TPropertyDescAndValuePtr;
        typedef eastl::pair<SPropertyDesc, const Q3DStudio::UVariant *>
            TPropertyDescAndConstValuePtr;
        typedef eastl::pair<SPropertyDesc, Q3DStudio::UVariant> TPropertyDescAndValue;
        // There is probably some balance here between number of values in a group
        // and number of groups.  A heuristic of a large application would tell you
        // the optimum number of values to groups.
        struct SPropertyValueGroup
        {
            enum {
                NumValues = 4,
            };
            Q3DStudio::UVariant m_Data[4];
            SPropertyValueGroup *m_NextNode;
            SPropertyValueGroup()
                : m_NextNode(NULL)
            {
                for (QT3DSU32 idx = 0; idx < NumValues; ++idx)
                    m_Data[idx].m_INT32 = 0;
            }
        };

        struct SElement
        {
            CRegisteredString m_Name; // const, do not set after creation.
            CRegisteredString m_Path;
            const STypeDesc *m_TypeDescription; ///< static information created on load time
            // The property values are in order described in the type description.
            SPropertyValueGroup *m_PropertyValues;
            // dynamic section
            STypeDesc *m_DynamicTypeDescription; ///< we may create this on the fly
            SPropertyValueGroup *m_DynamicPropertyValues;

        protected:
            SElementFlag m_Flags; // Setting these changes things.
        public:
            QT3DSU32 m_Handle;
            QT3DSU32 m_ScriptID; ///< Superfluous, could use handle to link to script representation
            QT3DSU32 m_Depth; ///< Distance from this node to the root of the graph.
            SElement *m_Parent; ///< Parent element in activity graph
            SElement *m_Sibling; ///< Next sibling element in activity graph
            SElement *m_Child; ///< First child element in activity graph
            bool m_OnMaster = false;
            void *m_Association; ///< Link to associated asset in scene
            Q3DStudio::IPresentation *m_BelongedPresentation;
            SActivationManagerNode m_ActivationManagerNode;

            SElement(const STypeDesc &inDesc)
                : m_TypeDescription(&inDesc)
                , m_PropertyValues(NULL)
                , m_DynamicTypeDescription(NULL)
                , m_DynamicPropertyValues(NULL)
                , m_Handle(0)
                , m_ScriptID(0)
                , m_Depth(0)
                , m_Parent(NULL)
                , m_Sibling(NULL)
                , m_Child(NULL)
                , m_Association(NULL)
                , m_BelongedPresentation(NULL)
            {
            }
            QT3DSU32 GetHandle() const { return m_Handle; }
            const STypeDesc &GetTypeDescription() const { return *m_TypeDescription; }
            void SetTypeDescription(const STypeDesc *inDesc) { m_TypeDescription = inDesc; }
            // Q3DStudio::CHash::HashString
            QT3DSU32 GetNameHash() const;
            const SPropertyValueGroup *GetFirstPropertyGroup() const { return m_PropertyValues; }
            void SetFirstPropertyGroup(SPropertyValueGroup *inGroup) { m_PropertyValues = inGroup; }

            SPropertyValueGroup *&UnsafeGetFirstPropertyGroup() { return m_PropertyValues; }

            // In general, use these accessors for attributes, especially setting values.
            //  The other accessors are fast paths that do not:
            //  1.  Set dirty flags
            //  2.  Notify the activation zone of changes if setting start,end time.
            //  3.  Notify any other subsystems.
            bool GetAttribute(QT3DSU32 inHashName, Q3DStudio::UVariant &outValue) const;
            // Triggers updating various subcomponents based on attribute key.
            void SetAttribute(const Q3DStudio::TAttributeHash inKey,
                              const Q3DStudio::UVariant inValue);
            // Triggers updating various subcomponents without requiring a new property lookup.
            void SetAttribute(TPropertyDescAndValuePtr inKey, const Q3DStudio::UVariant inNewValue,
                              bool forceSet = false);

            // Q3DStudio::CHash::HashAttribute
            Option<QT3DSU32> FindPropertyIndex(QT3DSU32 inNameHash) const;
            Option<QT3DSU32> FindPropertyIndex(CRegisteredString inName) const;

            Option<QT3DSU32> FindDynamicPropertyIndex(QT3DSU32 inNameHash) const;

            QT3DSU32 GetNumProperties() const;
            QT3DSU32 GetAttributeCount() const { return GetNumProperties(); }

            QT3DSU32 GetNumDynamicProperties() const;
            QT3DSU32 GetDynamicAttributeCount() const { return GetNumDynamicProperties(); }

            // Note that if you set a value then this object is dirty.
            // Bypasses special checks in setAttribute.  In general, use setAttribute.
            Option<TPropertyDescAndValuePtr> GetPropertyByIndex(QT3DSU32 inIdx);
            Option<TPropertyDescAndConstValuePtr> GetPropertyByIndex(QT3DSU32 inIdx) const;

            Option<TPropertyDescAndValuePtr> GetDynamicPropertyByIndex(QT3DSU32 inIdx);

            Q3DStudio::UVariant *FindPropertyValue(QT3DSU32 inNameHash)
            {
                Option<QT3DSU32> idx = FindPropertyIndex(inNameHash);
                if (idx.hasValue()) {
                    Option<TPropertyDescAndValuePtr> theVal = GetPropertyByIndex(idx);
                    if (theVal.hasValue())
                        return theVal->second;
                }
                return NULL;
            }
            const Q3DStudio::UVariant *FindPropertyValue(QT3DSU32 inNameHash) const
            {
                Option<QT3DSU32> idx = FindPropertyIndex(inNameHash);
                if (idx.hasValue()) {
                    Option<TPropertyDescAndConstValuePtr> theVal = GetPropertyByIndex(idx);
                    if (theVal.hasValue())
                        return theVal->second;
                }
                return NULL;
            }
            Q3DStudio::UVariant *FindPropertyValue(CRegisteredString inNameHash)
            {
                Option<QT3DSU32> idx = FindPropertyIndex(inNameHash);
                if (idx.hasValue()) {
                    Option<TPropertyDescAndValuePtr> theVal = GetPropertyByIndex(idx);
                    if (theVal.hasValue())
                        return theVal->second;
                }
                return NULL;
            }

            const Q3DStudio::UVariant *FindPropertyValue(CRegisteredString inNameHash) const
            {
                Option<QT3DSU32> idx = FindPropertyIndex(inNameHash);
                if (idx.hasValue()) {
                    Option<TPropertyDescAndConstValuePtr> theVal = GetPropertyByIndex(idx);
                    if (theVal.hasValue())
                        return theVal->second;
                }
                return NULL;
            }

            Option<TPropertyDescAndValuePtr> FindProperty(QT3DSU32 inNameHash)
            {
                Option<QT3DSU32> idx = FindPropertyIndex(inNameHash);
                if (idx.hasValue())
                    return GetPropertyByIndex(*idx);
                return Empty();
            }

            Option<TPropertyDescAndValuePtr> FindDynamicProperty(QT3DSU32 inNameHash)
            {
                Option<QT3DSU32> idx = FindDynamicPropertyIndex(inNameHash);
                if (idx.hasValue())
                    return GetDynamicPropertyByIndex(*idx);

                return Empty();
            }

            SElement *GetParent() { return m_Parent; }
            const SElement *GetParent() const { return m_Parent; }
            SElement *GetSibling() { return m_Sibling; }
            const SElement *GetSibling() const { return m_Sibling; }
            SElement *GetChild() { return m_Child; }
            const SElement *GetChild() const { return m_Child; }
            CRegisteredString GetType() const { return m_TypeDescription->m_TypeName; }
            bool IsComponent() const { return m_Flags.IsComponent(); }

            Q3DStudio::IPresentation *GetBelongedPresentation() { return m_BelongedPresentation; }
            void SetBelongedPresentation(Q3DStudio::IPresentation &inPresentation)
            {
                m_BelongedPresentation = &inPresentation;
            }

            // Set flags bypassing the dirty system *and* updates to the activity zone.
            // do not do this unless you are sure you do not want any side effects.
            SElementFlag &Flags() { return m_Flags; }

            // User flag that is set and is persistent
            void SetExplicitActive(bool inValue)
            {
                SetFlag(Q3DStudio::ELEMENTFLAG_EXPLICITACTIVE, inValue);
            }
            bool IsExplicitActive() const { return m_Flags.IsExplicitActive(); }

            void SetControlledActive(bool inValue)
            {
                SetFlag(Q3DStudio::ELEMENTFLAG_CONTROLLED_ACTIVE, inValue);
            }

            bool IsControlledActive() const { return m_Flags.IsControlledActive(); }

            // Flag set by the activity manager.
            void SetActive(bool inValue) { SetFlag(Q3DStudio::ELEMENTFLAG_GLOBALACTIVE, inValue); }
            bool GetActive() const { return m_Flags.IsActive(); }

            // Return true if we are about to activate, but have not yet been activated
            bool AboutToActivate() const
            {
                if (!IsDirty())
                    return false;
                return IsExplicitActive();
            }

            void SetPickEnabled(bool inValue)
            {
                SetFlag(Q3DStudio::ELEMENTFLAG_PICKENABLED, inValue);
            }
            bool IsPickEnabled() const { return m_Flags.IsPickEnabled(); }

            void SetPlayThrough(bool inValue)
            {
                SetFlag(Q3DStudio::ELEMENTFLAG_PLAYTHROUGH, inValue);
            }
            bool IsPlayThrough() const { return m_Flags.IsPlayThrough(); }

            void SetScriptCallbacks(bool inValue)
            {
                SetFlag(Q3DStudio::ELEMENTFLAG_SCRIPTCALLBACKS, inValue);
            }
            bool HasScriptCallbacks() const { return m_Flags.HasScriptCallbacks(); }

            bool IsDirty() const { return m_Flags.IsDirty(); }

            bool GetFlag(Q3DStudio::EElementFlag inFlag) const { return m_Flags.operator&(inFlag); }

            void SetDirty();
            void SetFlag(Q3DStudio::EElementFlag inFlag, bool inValue);
            void SetFlag(Q3DStudio::EElementFlag inFlag, int inValue)
            {
                SetFlag(inFlag, inValue ? true : false);
            }
            qt3ds::runtime::IActivityZone &GetActivityZone() const;
            bool HasActivityZone() const;
            // Floating point number differences above this trigger a setDirty call.
            static float SmallestDifference() { return 0.000001f; }
            SElement &GetComponentParent();
            const SElement &GetComponentParent() const;
            SElement *FindChild(QT3DSU32 inNameHash);
            // Get the time of my first noncomponent child or
            // my time if I am not a component.
            Q3DStudio::TTimeUnit GetInnerTime() const;
            // Get the time I am animating at.
            Q3DStudio::TTimeUnit GetOuterTime() const;
            bool IsDescendent(SElement &inPossibleParent) const;
            void *GetAssociation() const { return m_Association; }
            void SetAssociation(void *inAssoc) { m_Association = inAssoc; }
            QT3DSU32 GetUncachedDepth() const
            {
                if (m_Parent)
                    return m_Parent->GetUncachedDepth() + 1;
                return 0;
            }
            void SetDepth() { m_Depth = GetUncachedDepth(); }
            QT3DSU32 Depth() const { return m_Depth; }

            bool IsUserActive() const;

            bool IsIndependent() const;

            bool IsGlobalActive() const;
            // This may require a mutex to be held.
            void SetGlobalActive(bool active);

            bool IsTimeActive() const
            {
                if (m_Parent != NULL)
                    return m_ActivationManagerNode.m_Flags.IsTimeActive();
                return true;
            }

            bool DoesParticipateInTimeGraph() const { return m_Flags.HasStartEndTime(); }

            bool IsGlobalActive(bool inParentActive) const
            {
                bool ta = IsTimeActive();
                bool ua = IsUserActive();

                return ta && ua && inParentActive;
            }
            void findComponents(QVector<SElement *> &components)
            {
                if (IsComponent())
                    components.push_back(this);
                SElement *child = m_Child;
                while (child) {
                    child->findComponents(components);
                    child = child->m_Sibling;
                }
            }

            // Sets animation on property to inactive.
            void stopAnimations(QT3DSU32 propHash);
        };

        struct SGetElementNodeDirtyIndex
        {
            QT3DSU32 operator()(const SElement &inNode)
            {
                return inNode.m_ActivationManagerNode.m_DirtyIndex;
            }
        };
        struct SSetElementNodeDirtyIndex
        {
            void operator()(SElement &inNode, QT3DSU32 val)
            {
                inNode.m_ActivationManagerNode.m_DirtyIndex = val;
            }
        };

        struct SActivationManagerNodeDirtyList
            : public InvasiveSet<SElement, SGetElementNodeDirtyIndex, SSetElementNodeDirtyIndex>
        {
            typedef InvasiveSet<SElement, SGetElementNodeDirtyIndex, SSetElementNodeDirtyIndex>
                TBaseType;
            SActivationManagerNodeDirtyList(NVAllocatorCallback &callback)
                : TBaseType(callback, "SActivationManagerNodeDirtyList")
            {
            }
        };

        struct SComponent : public SElement
        {
            Q3DStudio::SAlignedTimeUnit m_BeginTime;
            Q3DStudio::SAlignedTimeUnit m_Duration;

            // Slide related
            QT3DSU8 m_SlideCount; ///< Number of slides starting from base index
            QT3DSU8 m_CurrentSlide; ///< Current slide number
            QT3DSU8 m_PreviousSlide; ///< Previous slide number
            SComponent(const STypeDesc &inDesc)
                : SElement(inDesc)
                , m_SlideCount(0)
                , m_CurrentSlide(0)
                , m_PreviousSlide(0)
            {
            }

            QT3DSU8 GetCurrentSlide() const { return m_CurrentSlide; }
            void SetCurrentSlide(QT3DSU8 inSlide)
            {
                m_PreviousSlide = m_CurrentSlide;
                m_CurrentSlide = inSlide;
            }
            QT3DSU8 GetSlideCount() const { return m_SlideCount; }
            QT3DSU8 GetPreviousSlide() const { return m_PreviousSlide; }
            bool GetPaused() const;
            bool GetPlayBackDirection() const;
            Q3DStudio::CTimePolicy &GetTimePolicy();
            const Q3DStudio::CTimePolicy &GetTimePolicy() const;
        };
    }
    // Global store of all elements, values.
    class IElementAllocator : public NVRefCounted
    {
    public:
        // Performs coalesing of element types so that there are the least number of type
        // descriptions
        // Certain properties, like name, eyeball, are never in the property description.
        virtual element::SElement &
        CreateElement(CRegisteredString inName, CRegisteredString inType,
                      CRegisteredString inSubType,
                      NVConstDataRef<element::TPropertyDescAndValue> inPropertyDescriptions,
                      Q3DStudio::IPresentation *inPresentation, element::SElement *inParent,
                      bool inIsComponent) = 0;

        virtual void ReleaseElement(element::SElement &inElement, bool inRecurse) = 0;

        virtual Option<element::TPropertyDescAndValuePtr>
        CreateDynamicProperty(Q3DStudio::IRuntimeMetaData &theMetaData, element::SElement &element,
                              CRegisteredString inName) = 0;

        virtual element::SElement *FindElementByHandle(QT3DSU32 inElementHandle) = 0;
        // Returns an element pointer that when added to the return value of load will be a valid
        // element.
        virtual element::SElement *
        GetRemappedElementAddress(element::SElement *inElement) const = 0;
        virtual const element::STypeDesc *
        GetRemappedTypeDescAddress(const element::STypeDesc *inTypeDesc) const = 0;
        // If load is successful, then returns an address that can be added to elements and the root
        // element.

        static IElementAllocator &CreateElementAllocator(NVFoundationBase &inFoundation,
                                                         IStringTable &inStringTable);
    };
}
}

#endif
