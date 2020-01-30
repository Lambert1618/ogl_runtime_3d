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
#include "Qt3DSSlideSystem.h"
#include "Qt3DSActivationManager.h"
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
                                        IPresentation *inDefaultPresentation, const QString &inPath,
                                        TElement *inStartElement)
{
    if (inPath.isNull())
        return nullptr;

    int delimIndex = inPath.indexOf(QLatin1Char(PRESENTATION_DELIMITER));
    IPresentation *thePresentation = nullptr;
    QString path = inPath;

    TElement *theElement = inStartElement;
    if (delimIndex > 0) {
        thePresentation = inApplication.LoadAndGetPresentationById(path.left(delimIndex));
        path = path.right(path.length() - delimIndex - 1);
    }

    if (thePresentation == nullptr)
        thePresentation = inDefaultPresentation;

    // Return nil if the inStartElement is not in the specified presentation
    if (theElement != nullptr
        && (delimIndex < 0 && theElement->GetBelongedPresentation() != thePresentation)) {
        thePresentation = theElement->GetBelongedPresentation();
    }

    if (thePresentation == nullptr)
        return nullptr;

    int curIdx = 0;
    int nodeDelim;
    int theParseCounter = 0;
    bool last = false;

    do {
        nodeDelim = path.indexOf(QLatin1Char(NODE_DELIMITER), curIdx);
        if (nodeDelim < 0) {
            nodeDelim = path.length();
            last = true;
        }

        ++theParseCounter;

        auto name = CHash::HashString(path, curIdx, nodeDelim);
        if (name == RESERVED_PARENT) {
            if (theElement)
                theElement = theElement->GetParent();
        } else if (RESERVED_THIS == name) {
            //Keep the original element if "this" wanted
        } else {
            if (name == RESERVED_SCENE && theParseCounter == 1)
                theElement = thePresentation->GetRoot();
            else if (theElement)
                theElement = theElement->FindChild(name);
        }
        if (!theElement)
            break;

        curIdx = nodeDelim + 1;
    } while (!last);

    return theElement;
}

CQmlElementHelper::TypedAttributeAndValue CQmlElementHelper::getTypedAttributeAndValue(
        TElement *theElement, const char *theAttName, const void *value, TAttributeHash attrHash)
{
    TypedAttributeAndValue retVal = { SAttributeKey(), UVariant() };
    retVal.attribute.m_Hash = 0;

    SAttributeKey theAttributeKey;
    theAttributeKey.m_Hash = attrHash;
    if (!attrHash)
        theAttributeKey.m_Hash = CHash::HashAttribute(theAttName);

    // Early out for our single 'read only' attribute
    if (ATTRIBUTE_URI == theAttributeKey.m_Hash) {
        // we didn't push anything onto the stack
        return retVal;
    }

    // first search if it is a static property
    Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> thePropertyInfo
            = theElement->FindProperty(theAttributeKey.m_Hash);

    if (!thePropertyInfo.hasValue() && theAttributeKey.m_Hash != Q3DStudio::ATTRIBUTE_EYEBALL) {
        // if not search in the dynamic properties
        thePropertyInfo = theElement->FindDynamicProperty(theAttributeKey.m_Hash);
        if (!thePropertyInfo.hasValue())
            return retVal;
    }

    if (theAttributeKey.m_Hash == Q3DStudio::ATTRIBUTE_EYEBALL) {
        UVariant theNewValue;
        theNewValue.m_INT32 = *(INT32 *)value;
        retVal.attribute = theAttributeKey;
        retVal.value = theNewValue;
    } else if (thePropertyInfo.hasValue()) {
        UVariant theNewValue;
        QString name(thePropertyInfo->first.name().c_str());
        EAttributeType theAttributeType = thePropertyInfo->first.type();
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
            theNewValue.m_StringHandle
                    = theElement->GetBelongedPresentation()->GetStringTable().getDynamicHandle(
                        QByteArray(static_cast<const char *>(value)));
            break;
        case ATTRIBUTETYPE_POINTER:
            qCCritical(INVALID_OPERATION, "getTypedAttributeAndValue: "
                                          "pointer attributes not handled.");
            return retVal;
        case ATTRIBUTETYPE_ELEMENTREF:
            qCCritical(INVALID_OPERATION, "getTypedAttributeAndValue: "
                                          "ElementRef attributes are read only.");
            return retVal;
        case ATTRIBUTETYPE_FLOAT3: {
            FLOAT *vec3 = (FLOAT *)value;
            theNewValue.m_FLOAT3[0] = vec3[0];
            theNewValue.m_FLOAT3[1] = vec3[1];
            theNewValue.m_FLOAT3[2] = vec3[2];
            if (name == QLatin1String("rotation")) {
                theNewValue.m_FLOAT3[0] = qDegreesToRadians(theNewValue.m_FLOAT3[0]);
                theNewValue.m_FLOAT3[1] = qDegreesToRadians(theNewValue.m_FLOAT3[1]);
                theNewValue.m_FLOAT3[2] = qDegreesToRadians(theNewValue.m_FLOAT3[2]);
            }
        } break;
        case ATTRIBUTETYPE_FLOAT4: {
            FLOAT *vec4 = (FLOAT *)value;
            theNewValue.m_FLOAT4[0] = vec4[0];
            theNewValue.m_FLOAT4[1] = vec4[1];
            theNewValue.m_FLOAT4[2] = vec4[2];
            theNewValue.m_FLOAT4[3] = vec4[3];
        } break;
        case ATTRIBUTETYPE_NONE:
        case ATTRIBUTETYPE_DATADRIVEN_PARENT:
        case ATTRIBUTETYPE_DATADRIVEN_CHILD:
        case ATTRIBUTETYPECOUNT:
        default:
            qCCritical(INVALID_OPERATION, "getTypedAttributeAndValue: Attribute has no type!");
            return retVal;
        }

        retVal.attribute = theAttributeKey;
        retVal.value = theNewValue;
    }

    return retVal;
}

bool CQmlElementHelper::EnsureAttribute(TElement *theElement, const char *theAttName,
                                        TAttributeHash attrHash)
{
    SAttributeKey theAttributeKey;
    theAttributeKey.m_Hash = attrHash;
    if (!attrHash)
        theAttributeKey.m_Hash = CHash::HashAttribute(theAttName);

    if (theAttributeKey.m_Hash == ATTRIBUTE_EYEBALL || theAttributeKey.m_Hash == ATTRIBUTE_URI)
        return false;

    // first search if it is a static property
    Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> thePropertyInfo =
        theElement->FindProperty(theAttributeKey.m_Hash);

    // Do not create property for eyeball, it uses the explicit activity flag
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
    return true;
}

bool CQmlElementHelper::SetAttribute(TElement *theElement, const char *theAttName,
                                     const void *value, TAttributeHash attrHash)
{
    SAttributeKey theAttributeKey;
    theAttributeKey.m_Hash = attrHash;
    if (!attrHash)
        theAttributeKey.m_Hash = CHash::HashAttribute(theAttName);
    bool force = false;

    // Fail if trying to change the activation state of an object in another slide
    if (theAttributeKey.m_Hash == Q3DStudio::ATTRIBUTE_EYEBALL
            && theElement->GetBelongedPresentation()->GetActivityZone()) {
        CPresentation *presentation = static_cast<CPresentation *>(
                    theElement->GetBelongedPresentation());
        ISlideSystem &slideSystem = presentation->GetSlideSystem();
        TElement *componentElement = theElement->GetActivityZone().GetItemTimeParent(*theElement);
        TComponent *component = presentation->GetComponentManager().GetComponent(componentElement);
        if (!slideSystem.isElementInSlide(*theElement, *componentElement,
                                          component->GetCurrentSlide())) {
            return false;
        }
    }

    // Early out for our single 'read only' attribute
    if (ATTRIBUTE_URI == theAttributeKey.m_Hash) {
        // we didn't push anything onto the  stack
        return false;
    }

    // first search if it is a static property
    Option<qt3ds::runtime::element::TPropertyDescAndValuePtr> thePropertyInfo =
        theElement->FindProperty(theAttributeKey.m_Hash);

    // Do not create property for eyeball, it uses the explicit activity flag
    if (!thePropertyInfo.hasValue() && theAttributeKey.m_Hash != Q3DStudio::ATTRIBUTE_EYEBALL) {
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
            force = true;
        }
    }

    TypedAttributeAndValue attributeAndValue
            = getTypedAttributeAndValue(theElement, theAttName, value, theAttributeKey.m_Hash);

    if (attributeAndValue.attribute.m_Hash == 0)
        return false;

    if (attributeAndValue.attribute.m_Hash == Q3DStudio::ATTRIBUTE_EYEBALL)
        theElement->SetAttribute(attributeAndValue.attribute.m_Hash, attributeAndValue.value);
    else if (thePropertyInfo.hasValue())
        theElement->SetAttribute(thePropertyInfo, attributeAndValue.value, force);
    else
        return false;

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

    if (theAttributeKey.m_Hash == Q3DStudio::ATTRIBUTE_EYEBALL) {
        INT32 val = inElement->IsExplicitActive() ? 1 : 0;
        *(INT32 *)value = val;
    } else if (thePropertyInfo.hasValue()) {
        UVariant *theValuePtr = thePropertyInfo->second;
        EAttributeType theAttributeType = thePropertyInfo->first.type();
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
        case ATTRIBUTETYPE_FLOAT4: {
            FLOAT *vec4 = (FLOAT *)value;
            vec4[0] = theValuePtr->m_FLOAT4[0];
            vec4[1] = theValuePtr->m_FLOAT4[1];
            vec4[2] = theValuePtr->m_FLOAT4[2];
            vec4[3] = theValuePtr->m_FLOAT4[3];
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
