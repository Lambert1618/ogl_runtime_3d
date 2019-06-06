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

#include "Qt3DSElementHelper.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSApplication.h"
#include "Qt3DSCommandEventTypes.h"

using namespace qt3ds;

namespace Q3DStudio {

const char PRESENTATION_DELIMITER = ':';
const char NODE_DELIMITER = '.';
const TStringHash RESERVED_THIS = CHash::HashString("this");
const TStringHash RESERVED_PARENT = CHash::HashString("parent");
const TStringHash RESERVED_SCENE = CHash::HashString("Scene");

CElementHelper::CElementHelper()
{
}

CElementHelper::~CElementHelper()
{
}

TElement *CElementHelper::GetElement(qt3ds::runtime::IApplication &inApplication,
                                     IPresentation *inDefaultPresentation, const char *inPath,
                                     TElement *inStartElement)
{
    if (!inPath || *inPath == 0)
        return nullptr;
    const char *thePath(inPath);
    const char *theSubPath = nullptr;
    IPresentation *thePresentation = nullptr;
    size_t thePathLength = ::strlen(thePath) + 1;
    char *theToken = Q3DStudio_allocate_desc(CHAR, thePathLength, "Token:TempPath");
    // Try to get the specified presentation
    theSubPath = ::strchr(thePath, PRESENTATION_DELIMITER);
    TElement *theElement = inStartElement;
    if (theSubPath) {
        UINT32 theSubPathLength = static_cast<UINT32>(theSubPath - thePath);

        ::strncpy(theToken, thePath, theSubPathLength);
        theToken[theSubPathLength] = '\0';

        thePath = theSubPath + 1;

        const CHAR *thePresentationName = theToken;

        thePresentation = inApplication.GetPresentationById(thePresentationName);
    }

    if (!thePresentation)
        thePresentation = inDefaultPresentation;

    // Return nil if the inStartElement is not in the specified presentation
    if (theElement
            && (!theSubPath && theElement->GetBelongedPresentation() != thePresentation)) {
        thePresentation = theElement->GetBelongedPresentation();
    }

    if (!thePresentation)
        return nullptr;

    TStringHash theName;
    INT32 theParseCounter = 0;

    while (thePath && thePath[0] != '\0') {
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
            ;
        } else {
            if (theName == RESERVED_SCENE && theParseCounter == 1) {
                // theElement is nullptr, so using absolute path
                theElement = thePresentation->GetRoot();
            } else if (theElement) {
                // Using relative path
                theElement = theElement->FindChild(theName);
            }
        }

        if (!theElement)
            thePath = nullptr;
    } // while

    Q3DStudio_free(theToken, CHAR, thePathLength);
    return theElement;
}
} // namespace Q3DStudio
