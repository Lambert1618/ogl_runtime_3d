/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#ifndef DOCUMENTRESOURCEMANAGERCUSTOMMATERIALPARSERH
#define DOCUMENTRESOURCEMANAGERCUSTOMMATERIALPARSERH

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMWStrOpsImpl.h"
#include "foundation/StrConvertUTF.h"
#include <EASTL/string.h>
#include "Qt3DSRenderInputStreamFactory.h"

namespace Q3DStudio {
struct CCustomMaterialParser
{

    static bool NavigateToMetadata(std::shared_ptr<qt3dsdm::IDOMReader> inReader)
    {
        return inReader->MoveToFirstChild("MetaData");
    }

    static std::shared_ptr<qt3dsdm::IDOMReader>
    ParseFile(std::shared_ptr<qt3dsdm::IDOMFactory> inFactory,
              std::shared_ptr<qt3dsdm::IStringTable> inStringTable, const char8_t *inFileData,
              qt3dsdm::CXmlErrorHandler &inErrorHandler,
              qt3ds::render::IInputStreamFactory &inStreamFactory)
    {
        using namespace qt3ds;
        using namespace qt3ds::foundation;

        NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> theStream(
            inStreamFactory.GetStreamForFile(inFileData));
        if (!theStream) {
            QT3DS_ASSERT(0);
            return std::shared_ptr<qt3dsdm::IDOMReader>();
        }

        qt3dsdm::SDOMElement *theElem(
            qt3dsdm::CDOMSerializer::Read(*inFactory, *theStream, &inErrorHandler));

        if (theElem == NULL) {
            return std::shared_ptr<qt3dsdm::IDOMReader>();
        } else
            return qt3dsdm::IDOMReader::CreateDOMReader(*theElem, inStringTable, inFactory);
    }
};
}

#endif
