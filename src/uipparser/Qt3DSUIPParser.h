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
#include "foundation/StringTable.h"
#include "Qt3DSKernelTypes.h"
#include <EASTL/string.h>

namespace qt3ds {
namespace state {
    struct SSetAttribute;
    namespace debugger {
        class ISceneGraphRuntimeDebugger;
    }
}
}

namespace qt3ds {
namespace render {
    class IInputStreamFactory;
}
}

//==============================================================================
//	Forwards
//==============================================================================
namespace qt3dsdm {
class IDOMReader;
class IStringTable;
}

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {
using namespace qt3ds::foundation;
//==============================================================================
//	Forwards
//==============================================================================
class IRuntimeMetaData;
class IPresentation;

struct UIPElementTypes
{
    enum Enum {
        Unknown = 0,
        Scene = 1,
        Node,
        Layer,
        Component,
        Group,
        Behavior,
        Model,
        Light,
        Camera,
        Image,
        Material,
        Text,
        Effect,
        RenderPlugin,
        CustomMaterial,
        ReferencedMaterial,
        Path,
        PathAnchorPoint,
        PathSubPath,
    };
};

struct SElementAndType
{
    UIPElementTypes::Enum m_Type;
    TElement *m_Element;
    SElementAndType(UIPElementTypes::Enum inType, TElement *elem)
        : m_Type(inType)
        , m_Element(elem)
    {
    }
};

// Ensure these elements exist and if the attribute isn't empty ensure the attribute is referenced.
struct SElementAttributeReference
{
    eastl::string m_Path;
    eastl::string m_Attribute;
    SElementAttributeReference(const eastl::string &p, const eastl::string &a)
        : m_Path(p)
        , m_Attribute(a)
    {
    }
};
//==============================================================================
/**
 *	@class	CUIPParser
 *	@brief	Class for parsing UIP file
 */

class IUIPParser : public NVReleasable
{
protected:
    virtual ~IUIPParser() {}
public: // Parse UIP file
    virtual BOOL Load(IPresentation &inPresentation,
                      NVConstDataRef<SElementAttributeReference> inStateReferences) = 0;
    virtual qt3dsdm::IDOMReader &GetDOMReader() = 0;
    virtual IRuntimeMetaData &GetMetaData() = 0;
    // Mapping back from file id to element id, needed to hook elements up to their respective
    // parsed
    // counterparts.
    virtual SElementAndType GetElementForID(const char *inStringId) = 0;
    virtual eastl::string ResolveReference(const char *inStringId, const char *inReferance) = 0;
    // The rendering system needs to know every sourcepath found during parse of the UIP file
    // so that it can do things like register images as opaque/transparent as well as preload
    // mesh files (and possibly font files).
    virtual NVConstDataRef<eastl::string> GetSourcePaths() const = 0;
    virtual bool isIblImage(const eastl::string &sourcepath) const = 0;

    virtual QVector<QString> GetSlideSourcePaths() const = 0;

    // Creation function
    static IUIPParser &Create(const QString &inFileName, IRuntimeMetaData &inMetaData,
                              qt3ds::render::IInputStreamFactory &inStreamFactory,
                              qt3ds::foundation::IStringTable &inStrTable);
};

} // namespace Q3DStudio
