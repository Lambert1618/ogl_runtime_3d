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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSArray.h"

namespace qt3ds {
namespace runtime {
    namespace element {
        struct SElement;
        struct SComponent;
    }
}
}

namespace Q3DStudio {
typedef qt3ds::runtime::element::SElement TElement;
typedef qt3ds::runtime::element::SComponent TComponent;
}
//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Forwards
//==============================================================================
class CString;

//==============================================================================
//	Typedefs
//==============================================================================
typedef CArray<TElement *> TElementList; ///< Dynamic list of CElements
typedef CArray<const CString *> TEventList; ///< Dynamic list of CStrings
typedef CArray<INT32> TAssociationList; ///< Dynamic list of IDs used for the association process

typedef UINT32 TEventCommandHash; ///< Value from CHash::Hash31
typedef UINT32 TStringHash; ///< Value from CHash::Hash
typedef UINT32 TAttributeHash; ///< Value from CHash::Hash27
typedef UINT32 THashValue; ///< 32 bit value indicating a hash

//==============================================================================
//	Enumerations
//==============================================================================

/// Each attribute knows the type of its value and uses 4 bits for this enum
enum EAttributeType {
    ATTRIBUTETYPE_NONE = 0,
    ATTRIBUTETYPE_INT32,
    ATTRIBUTETYPE_HASH,
    ATTRIBUTETYPE_FLOAT,
    ATTRIBUTETYPE_BOOL,
    ATTRIBUTETYPE_STRING,
    ATTRIBUTETYPE_POINTER,
    ATTRIBUTETYPE_ELEMENTREF,
    ATTRIBUTETYPE_DATADRIVEN_PARENT,
    ATTRIBUTETYPE_DATADRIVEN_CHILD,
    ATTRIBUTETYPE_FLOAT4,
    ATTRIBUTETYPE_FLOAT3,
    ATTRIBUTETYPE_FLOAT2,
    ATTRIBUTETYPE_DATAINPUT_TIMELINE,
    ATTRIBUTETYPE_DATAINPUT_SLIDE,
    ATTRIBUTETYPECOUNT
};

/// Elements represent various types of objects
enum EElementType {
    ELEMENTTYPE_UNKNOWN = 0,
    ELEMENTTYPE_NODE,
    ELEMENTTYPE_CAMERA,
    ELEMENTTYPE_LIGHT,
    ELEMENTTYPE_TEXT,
    ELEMENTTYPE_MATERIAL,
    ELEMENTTYPE_TEXTURE,
    ELEMENTTYPE_COMPONENT,
    ELEMENTTYPE_BEHAVIOR,
    ELEMENTTYPE_PATH,
    ELEMENTTYPE_PATHANCHORPOINT,
    ELEMENTTYPE_SUBPATH,
    ELEMENTTYPECOUNT
};

//==============================================================================
//	Enumerations
//==============================================================================

/// Various bit flags packed into a single 16bit space.
enum EElementFlag {
    // Persistent flags (set only at creation)
    ELEMENTFLAG_COMPONENT = 1, ///< Element is a component
    ELEMENTFLAG_TIMELINE = 1 << 1, ///< Element has start time and duration attributes
    ELEMENTFLAG_CLONE = 1 << 2, ///< Element is a clone
    ELEMENTFLAG_ATTRIBUTELOCK =
        1 << 3, ///< No more attributes can be added (always true during runtime)

    // Configuration flags (seldom changed)
    ELEMENTFLAG_SCRIPTCALLBACKS = 1
        << 4, ///< At least one script has requested notify on onActivate, onDeactivate, onUpdate
    ELEMENTFLAG_SCRIPTINITIALIZE = 1
        << 5, ///< At least one script has requested notify on onInitialize
    ELEMENTFLAG_REGISTEREDFORATTRIBUTECHANGE = 1 << 6, ///< Element monitored for attribute change
    ELEMENTFLAG_REGISTEREDFOREVENTCALLBACK = 1 << 7, ///< Element registered for event callbacks
    ELEMENTFLAG_PICKENABLED = 1 << 8, ///< At least one script or action has registered for mouse
                                      ///events based on this element

    // Runtime flags
    ELEMENTFLAG_GLOBALACTIVE =
        1 << 9, ///< Combination active flag based on explicit, time and parent active
    ELEMENTFLAG_EXPLICITACTIVE = 1 << 10, ///< Explicit On/Off switch on each element
    ELEMENTFLAG_PLAYTHROUGH = 1 << 11, ///< Playthrough slides enabled if this is a component
    ELEMENTFLAG_DIRTY = 1 << 12, ///< Attributes or user visible flags have changed

    // Flags used by the activation manager
    ELEMENTFLAG_AMGR_TIMEACTIVE = 1 << 13, ///< Is the element alive according to time information
    ELEMENTFLAG_CONTROLLED_ACTIVE = 1 << 14 ///< Is the element alive based on datainput control
};

// Four byte aligned time unit structure.
struct SAlignedTimeUnit
{
    UINT32 m_LowWord;
    UINT32 m_HighWord;

    SAlignedTimeUnit() {}
    SAlignedTimeUnit(const TTimeUnit &inUnit);
    SAlignedTimeUnit(const SAlignedTimeUnit &inOther)
        : m_LowWord(inOther.m_LowWord)
        , m_HighWord(inOther.m_HighWord)
    {
    }
    SAlignedTimeUnit &operator=(const SAlignedTimeUnit &inOther)
    {
        m_LowWord = inOther.m_LowWord;
        m_HighWord = inOther.m_HighWord;
        return *this;
    }
    operator TTimeUnit() const;

    void operator-=(const TTimeUnit &inTime);
    void operator%=(const TTimeUnit &inTime);
};

//==============================================================================
//	Defines
//==============================================================================
#define Q3DStudio_ATTRIBUTEKEYBITS 26 /* Number of bits to store attributes index */
#define Q3DStudio_ATTRIBUTEKEYMASK 0x03ffffff /* Bit mask covering 26 bits */

//==============================================================================
//	Structs
//==============================================================================

/// Each element has a number of attributes associated with it - This is the key.
struct SAttributeKey
{
    TAttributeHash m_Hash : Q3DStudio_ATTRIBUTEKEYBITS; ///< hash of attribute name
    TAttributeHash m_Type : 4; ///< Attribute type such as INT32, FLOAT, STRING
    TAttributeHash m_Dirty : 1; ///< Dirty bit signalling that the attribute value has been modified
    TAttributeHash
        m_DontOptimize : 1; ///< Used by the exporter/optimizer to flag attributes as un-optimizable

    void Convert(const UINT32 &inKey)
    {
        m_Hash = inKey & Q3DStudio_ATTRIBUTEKEYMASK;
        m_Type = (inKey & 0x3c000000) >> Q3DStudio_ATTRIBUTEKEYBITS;
        m_Dirty = (inKey & 0x40000000) >> (Q3DStudio_ATTRIBUTEKEYBITS + 4);
        m_DontOptimize = (inKey & 0x80000000) >> (Q3DStudio_ATTRIBUTEKEYBITS + 5);
    }
};

/// The structure to store a memory pool for clones.
/// The actual data pool will start at the address after these 8 bytes.
struct SClone
{
    SClone *m_Next; ///< Pointer to the next block of memory
    UINT32 m_Size; ///< Size of the current block of memory
};

//==============================================================================
//	Unions
//==============================================================================

/// Each element has a number of attributes associated with it - This is the value.
union UVariant {
    INT32 m_INT32;          // Integer representation
    FLOAT m_FLOAT;          // Float representation
    THashValue m_Hash;      // Explicit hash representation
    UINT32 m_StringHandle;  // Handle into the IStringTable member of the presentation
    void *m_VoidPointer;    // Generic data Pointer
    UINT32 m_ElementHandle; // Element handle.  Resolve using IApplication object.
    FLOAT m_FLOAT3[3];      // Vector 3 representation
    FLOAT m_FLOAT4[4];      // Vector 4 representation
};

} // namespace Q3DStudio
