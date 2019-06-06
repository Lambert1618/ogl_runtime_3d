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

#include "Qt3DSQmlElementHelper.h"
#include "Qt3DSCommandEventTypes.h"
#include "Qt3DSHash.h"
#include "Qt3DSAttributeHashes.h"
#include "Qt3DSElementSystem.h"
#include "Qt3DSRuntimeFactory.h"
#include "qmath.h"

using namespace Q3DStudio;
using namespace qt3ds;

//==============================================================================
//  Constants
//==============================================================================
const char PRESENTATION_DELIMITER = ':';
const char NODE_DELIMITER = '.';
const TStringHash RESERVED_THIS = CHash::HashString("this");
const TStringHash RESERVED_PARENT = CHash::HashString("parent");
const TStringHash RESERVED_SCENE = CHash::HashString("Scene");

//==============================================================================
/**
*   Constructor
*/
CQmlElementHelper::CQmlElementHelper()
{
}

//==============================================================================
/**
*   Destructor
*/
CQmlElementHelper::~CQmlElementHelper()
{
}

TElement *CQmlElementHelper::GetElement(qt3ds::runtime::IApplication &inApplication,
                                        IPresentation *inDefaultPresentation, const char *inPath,
                                        TElement *inStartElement)
{
    if (inPath == nullptr || *inPath == 0)
        return nullptr;
    const char *thePath(inPath);
    const char *theSubPath = nullptr;
    IPresentation *thePresentation = nullptr;
    size_t thePathLength = ::strlen(thePath) + 1;
    char *theToken =
        Q3DStudio_allocate_desc(CHAR, thePathLength, "Token:TempPath"); // Temporary token storage
    // Try to get the specified presentation
    theSubPath = ::strchr(thePath, PRESENTATION_DELIMITER);
    TElement *theElement = inStartElement;
    if (theSubPath != nullptr) {
        UINT32 theSubPathLength = static_cast<UINT32>(theSubPath - thePath);

        ::strncpy(theToken, thePath, theSubPathLength);
        theToken[theSubPathLength] = '\0';

        thePath = theSubPath + 1;

        const CHAR *thePresentationName = theToken;

        thePresentation = inApplication.GetPresentationById(thePresentationName);
    }
    if (thePresentation == nullptr)
        thePresentation = inDefaultPresentation;

    // Return nil if the inStartElement is not in the specified presentation
    if (theElement != nullptr
        && (theSubPath == nullptr && theElement->GetBelongedPresentation() != thePresentation)) {
        thePresentation = theElement->GetBelongedPresentation();
    }

    if (thePresentation == nullptr)
        return nullptr;

    TStringHash theName;
    INT32 theParseCounter = 0;

    while (thePath != nullptr && thePath[0] != '\0') {
        ++theParseCounter;

        // Do some strtok() work here
        theSubPath = ::strchr(thePath, NODE_DELIMITER);
        if (theSubPath) {
            UINT32 theSubPathLength = static_cast<UINT32>(theSubPath - thePath);
            Q3DStudio_ASSERT(theSubPathLength < thePathLength);

            ::strncpy(theToken, thePath, theSubPathLength);
            theToken[theSubPathLength] = '\0';

            thePath = theSubPath + 1;
        } else {
            ::strcpy(theToken, thePath);
            thePath = nullptr;
        }

        // Hash the token and do some element searching
        theName = CHash::HashString(theToken);

        if (theName == RESERVED_PARENT) {
            if (theElement)
                theElement = theElement->GetParent();
        } else if (theName == RESERVED_THIS) {
            //Keep the original element if "this" wanted
        } else {
            if (theName == RESERVED_SCENE && theParseCounter == 1)
                theElement =
                    thePresentation->GetRoot(); // theElement is nullptr, so using absolute path

            else if (theElement)
                theElement = theElement->FindChild(theName); // Using relative path
        }

        if (!theElement)
            thePath = nullptr;
    } // while

    Q3DStudio_free(theToken, CHAR, thePathLength);
    return theElement;
}

bool CQmlElementHelper::SetAttribute(TElement *theElement, const char *theAttName,
                                     const void *value, bool inDelay)
{
    SAttributeKey theAttributeKey;
    theAttributeKey.m_Hash = CHash::HashAttribute(theAttName);

    // Early out for our single 'read only' attribute
    if (ATTRIBUTE_URI == theAttributeKey.m_Hash) {
        // we didn't push anything onto the  stack
        return false;
    }

    // first search if it is a static property
    Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> thePropertyInfo =
        theElement->FindProperty(theAttributeKey.m_Hash);

    if (!thePropertyInfo.hasValue()) {
        // if not search in the dynamic properties
        thePropertyInfo = theElement->FindDynamicProperty(theAttributeKey.m_Hash);
        if (!thePropertyInfo.hasValue()) {
            // create a new dynamic porperty
            qt3ds::runtime::IElementAllocator &theElemAllocator(
                theElement->GetBelongedPresentation()->GetApplication().GetElementAllocator());
            qt3ds::foundation::IStringTable &theStringTable(
                theElement->GetBelongedPresentation()->GetStringTable());
            IRuntimeMetaData &theMetaData =
                theElement->GetBelongedPresentation()->GetApplication().GetMetaData();
            thePropertyInfo = theElemAllocator.CreateDynamicProperty(
                theMetaData, *theElement, theStringTable.RegisterStr(theAttName));
        }
    }

    if (thePropertyInfo.hasValue()) {
        UVariant theNewValue;
        QString name(thePropertyInfo->first.m_Name.c_str());
        EAttributeType theAttributeType = thePropertyInfo->first.m_Type;
        switch (theAttributeType) {
        case ATTRIBUTETYPE_INT32:
        case ATTRIBUTETYPE_HASH:
            theNewValue.m_INT32 = *(INT32 *)value;
            break;

        case ATTRIBUTETYPE_FLOAT:
            theNewValue.m_FLOAT = *(FLOAT *)value;
            if (name.startsWith(QLatin1String("rotation.")))
                theNewValue.m_FLOAT = qDegreesToRadians(theNewValue.m_FLOAT);
            break;

        case ATTRIBUTETYPE_BOOL:
            theNewValue.m_INT32 = *(INT32 *)value;
            break;

        case ATTRIBUTETYPE_STRING:
            theNewValue.m_StringHandle =
                theElement->GetBelongedPresentation()->GetStringTable().GetHandle(
                    (const char *)value);
            break;

        case ATTRIBUTETYPE_POINTER:
            qCCritical(INVALID_OPERATION, "setAttribute: pointer attributes not handled.");
            return false;
            break;

        case ATTRIBUTETYPE_ELEMENTREF:
            qCCritical(INVALID_OPERATION, "setAttribute: ElementRef attributes are read only.");
            return false;
            break;

        case ATTRIBUTETYPE_FLOAT3: {
            FLOAT *vec3 = (FLOAT *)value;
            theNewValue.m_FLOAT3[0] = vec3[0];
            theNewValue.m_FLOAT3[1] = vec3[1];
            theNewValue.m_FLOAT3[2] = vec3[2];
            if (name == QLatin1String("rotation")) {
                theNewValue.m_FLOAT3[0]= qDegreesToRadians(theNewValue.m_FLOAT3[0]);
                theNewValue.m_FLOAT3[1]= qDegreesToRadians(theNewValue.m_FLOAT3[1]);
                theNewValue.m_FLOAT3[2]= qDegreesToRadians(theNewValue.m_FLOAT3[2]);
            }
        } break;

        case ATTRIBUTETYPE_NONE:
        case ATTRIBUTETYPE_DATADRIVEN_PARENT:
        case ATTRIBUTETYPE_DATADRIVEN_CHILD:
        case ATTRIBUTETYPECOUNT:
        default:
            qCCritical(INVALID_OPERATION, "setAttribute: Attribute has no type!");
            return false;
            break;
        }

        if (inDelay) {
            UVariant arg1;
            arg1.m_INT32 = theAttributeKey.m_Hash;
            theElement->GetBelongedPresentation()->FireCommand(
                COMMAND_SETPROPERTY, theElement, &arg1, &theNewValue, ATTRIBUTETYPE_HASH,
                theAttributeType);
        } else {
            theElement->SetAttribute(*thePropertyInfo, theNewValue);
        }
    } else {
        return false;
    }
    return true;
}

bool CQmlElementHelper::GetAttribute(TElement *inElement, const char *inAttribute, void *value)
{
    SAttributeKey theAttributeKey;
    theAttributeKey.m_Hash = CHash::HashAttribute(inAttribute);

    // first search if it is a static property
    Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> thePropertyInfo =
        inElement->FindProperty(theAttributeKey.m_Hash);

    if (!thePropertyInfo.hasValue())
        thePropertyInfo = inElement->FindDynamicProperty(theAttributeKey.m_Hash);

    if (thePropertyInfo.hasValue()) {
        UVariant *theValuePtr = thePropertyInfo->second;
        EAttributeType theAttributeType = thePropertyInfo->first.m_Type;
        switch (theAttributeType) {
        case ATTRIBUTETYPE_INT32:
        case ATTRIBUTETYPE_HASH:
            *(INT32 *)value = theValuePtr->m_INT32;
            break;

        case ATTRIBUTETYPE_FLOAT:
            *(FLOAT *)value = theValuePtr->m_FLOAT;
            break;

        case ATTRIBUTETYPE_BOOL:
            *(INT32 *)value = (theValuePtr->m_INT32 != 0);
            break;

        case ATTRIBUTETYPE_STRING:
            *(char *)value = *inElement->GetBelongedPresentation()
                                  ->GetStringTable()
                                  .HandleToStr(theValuePtr->m_StringHandle)
                                  .c_str();
            break;

        case ATTRIBUTETYPE_POINTER:
            qCCritical(INVALID_OPERATION, "getAttribute: pointer attributes not handled.");
            return false;
            break;

        case ATTRIBUTETYPE_ELEMENTREF:
            qCCritical(INVALID_OPERATION, "getAttribute: ElementRef attributes are read only.");
            return false;
            break;

        case ATTRIBUTETYPE_FLOAT3: {
            FLOAT *vec3 = (FLOAT *)value;
            vec3[0] = theValuePtr->m_FLOAT3[0];
            vec3[1] = theValuePtr->m_FLOAT3[1];
            vec3[2] = theValuePtr->m_FLOAT3[2];
        } break;

        case ATTRIBUTETYPE_NONE:
        case ATTRIBUTETYPE_DATADRIVEN_PARENT:
        case ATTRIBUTETYPE_DATADRIVEN_CHILD:
        case ATTRIBUTETYPECOUNT:
        default:
            qCCritical(INVALID_OPERATION, "getAttribute: Attribute has no type!");
            return false;
            break;
        }
    } else {
        return false;
    }
    return true;
}
