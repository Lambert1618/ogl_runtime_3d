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

#ifndef QT3DS_QML_ELEMENT_HELPER_H
#define QT3DS_QML_ELEMENT_HELPER_H

#include "Qt3DSApplication.h"
#include "Qt3DSPresentation.h"
#include "Qt3DSKernelTypes.h"

namespace Q3DStudio {

class CQmlElementHelper
{
private:
    CQmlElementHelper();
    virtual ~CQmlElementHelper();

public:
    static TElement *GetElement(qt3ds::runtime::IApplication &inApplication,
                                IPresentation *inDefaultPresentation, const QString &inPath,
                                TElement *inStartElement = NULL);

    static bool SetAttribute(TElement *inElement, const char *inAttribute, const void *value,
                             TAttributeHash attrHash);
    static bool EnsureAttribute(TElement *inElement, const char *inAttribute,
                                TAttributeHash attrHash);
    static bool GetAttribute(TElement *inElement, const char *inAttribute, void *value);

    struct TypedAttributeAndValue {
        SAttributeKey attribute;
        UVariant value;
    };

    static TypedAttributeAndValue getTypedAttributeAndValue(TElement *inElement,
                                                            const char *inAttribute,
                                                            const void *value,
                                                            TAttributeHash attrHash);
};
}

#endif // QT3DS_QML_ELEMENT_HELPER_H
