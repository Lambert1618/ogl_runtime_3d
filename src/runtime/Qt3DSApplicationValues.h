/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
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
#include "Qt3DSApplication.h"
#include "foundation/Qt3DSDiscriminatedUnion.h"
#include "foundation/StringTable.h"

namespace qt3ds {

class Qt3DSAssetVisitor
{
public:
    virtual void visit(const char *path) = 0;
    virtual void visit(const char *type, const char *id, const char *src, const char *args) = 0;
};

namespace runtime {

    using qt3ds::foundation::CRegisteredString;

    struct AssetValueTypes
    {
        enum Enum {
            NoAssetValue = 0,
            Presentation,
            SCXML,
            RenderPlugin,
            Behavior,
            QmlPresentation,
        };
    };
    struct SAssetBase
    {
        CRegisteredString m_Id;
        CRegisteredString m_Src;
        CRegisteredString m_Args;
        SAssetBase(CRegisteredString inId = CRegisteredString(),
                   CRegisteredString inSrc = CRegisteredString(),
                   CRegisteredString inArgs = CRegisteredString())
            : m_Id(inId)
            , m_Src(inSrc)
            , m_Args(inArgs)
        {
        }
        bool operator==(const SAssetBase &inOther) const
        {
            return m_Id == inOther.m_Id && m_Src == inOther.m_Src
                    && m_Args == inOther.m_Args;
        }
        template <typename TRemapper>
        void Remap(TRemapper &item)
        {
            item.Remap(m_Id);
            item.Remap(m_Src);
            item.Remap(m_Args);
        }
        virtual const char *Type() const
        {
            return nullptr;
        }
    };

    struct SPresentationAsset : public SAssetBase
    {
        Q3DStudio::CPresentation *m_Presentation;
        bool m_Active;

        SPresentationAsset(CRegisteredString inId = CRegisteredString(),
                           CRegisteredString inRelPath = CRegisteredString(),
                           Q3DStudio::CPresentation *inPresentation = NULL)
            : SAssetBase(inId, inRelPath, CRegisteredString())
            , m_Presentation(inPresentation)
            , m_Active(true)
        {
        }

        bool operator==(const SPresentationAsset &inOther) const
        {
            return SAssetBase::operator==(inOther) && m_Presentation == inOther.m_Presentation
                && m_Active == inOther.m_Active;
        }

        template <typename TRemapper>
        void Remap(TRemapper &item)
        {
            m_Presentation = NULL;
            SAssetBase::Remap(item);
        }
        const char *Type() const override
        {
            return "presentation";
        }
    };

    struct SSCXMLAsset : public SAssetBase
    {
        CRegisteredString m_Datamodel;
        SSCXMLAsset(CRegisteredString inId = CRegisteredString(),
                    CRegisteredString inRelPath = CRegisteredString(),
                    CRegisteredString inDatamodel = CRegisteredString())
            : SAssetBase(inId, inRelPath, CRegisteredString())
            , m_Datamodel(inDatamodel)
        {
        }
        bool operator==(const SSCXMLAsset &inOther) const
        {
            return SAssetBase::operator==(inOther) && m_Datamodel == inOther.m_Datamodel;
        }
        template <typename TRemapper>
        void Remap(TRemapper &item)
        {
            SAssetBase::Remap(item);
            item.Remap(m_Datamodel);
        }
        const char *Type() const override
        {
            return "scxml";
        }
    };

    struct SRenderPluginAsset : public SAssetBase
    {
        SRenderPluginAsset(CRegisteredString inId = CRegisteredString(),
                           CRegisteredString inRelPath = CRegisteredString(),
                           CRegisteredString inArgs = CRegisteredString())
            : SAssetBase(inId, inRelPath, inArgs)
        {
        }
        const char *Type() const override
        {
            return "renderplugin";
        }
    };

    struct SQmlPresentationAsset : public SAssetBase
    {
        SQmlPresentationAsset(CRegisteredString inId = CRegisteredString(),
                           CRegisteredString inRelPath = CRegisteredString(),
                           CRegisteredString inArgs = CRegisteredString())
            : SAssetBase(inId, inRelPath, inArgs)
        {
        }
        const char *Type() const override
        {
            return "presentation-qml";
        }
    };

    struct SBehaviorAsset : public SAssetBase
    {
        Q3DStudio::INT32 m_Handle;
        SBehaviorAsset(CRegisteredString inId = CRegisteredString(),
                       CRegisteredString inRelPath = CRegisteredString(),
                       Q3DStudio::INT32 inHandle = 0)
            : SAssetBase(inId, inRelPath, CRegisteredString())
            , m_Handle(inHandle)
        {
        }
        bool operator==(const SBehaviorAsset &inOther) const
        {
            return SAssetBase::operator==(inOther) && m_Handle == inOther.m_Handle;
        }
        template <typename TRemapper>
        void Remap(TRemapper &item)
        {
            m_Handle = -1;
            SAssetBase::Remap(item);
        }
        const char *Type() const override
        {
            return "behaviour";
        }
    };
}
}

namespace qt3ds {
namespace foundation {
    template <>
    struct DestructTraits<qt3ds::runtime::SPresentationAsset>
    {
        void destruct(qt3ds::runtime::SPresentationAsset &) {}
    };
    template <>
    struct DestructTraits<qt3ds::runtime::SSCXMLAsset>
    {
        void destruct(qt3ds::runtime::SSCXMLAsset &) {}
    };
    template <>
    struct DestructTraits<qt3ds::runtime::SRenderPluginAsset>
    {
        void destruct(qt3ds::runtime::SRenderPluginAsset &) {}
    };
    template <>
    struct DestructTraits<qt3ds::runtime::SBehaviorAsset>
    {
        void destruct(qt3ds::runtime::SBehaviorAsset &) {}
    };
}
}

namespace qt3ds {
namespace runtime {

    // Force compile error if unsupported datatype requested
    template <typename TDataType>
    struct SAssetValueTypeMap
    {
    };

    template <>
    struct SAssetValueTypeMap<SPresentationAsset>
    {
        static AssetValueTypes::Enum GetType() { return AssetValueTypes::Presentation; }
    };
    template <>
    struct SAssetValueTypeMap<SSCXMLAsset>
    {
        static AssetValueTypes::Enum GetType() { return AssetValueTypes::SCXML; }
    };
    template <>
    struct SAssetValueTypeMap<SRenderPluginAsset>
    {
        static AssetValueTypes::Enum GetType() { return AssetValueTypes::RenderPlugin; }
    };
    template <>
    struct SAssetValueTypeMap<SQmlPresentationAsset>
    {
        static AssetValueTypes::Enum GetType() { return AssetValueTypes::QmlPresentation; }
    };
    template <>
    struct SAssetValueTypeMap<SBehaviorAsset>
    {
        static AssetValueTypes::Enum GetType() { return AssetValueTypes::Behavior; }
    };

    struct SAssetValueUnionTraits
    {
        typedef AssetValueTypes::Enum TIdType;
        enum {
            TBufferSize = sizeof(SPresentationAsset),
        };

        static TIdType getNoDataId() { return AssetValueTypes::NoAssetValue; }

        template <typename TDataType>
        static TIdType getType()
        {
            return SAssetValueTypeMap<TDataType>().GetType();
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case AssetValueTypes::Presentation:
                return inVisitor(*NVUnionCast<SPresentationAsset *>(inData));
            case AssetValueTypes::SCXML:
                return inVisitor(*NVUnionCast<SSCXMLAsset *>(inData));
            case AssetValueTypes::RenderPlugin:
                return inVisitor(*NVUnionCast<SRenderPluginAsset *>(inData));
            case AssetValueTypes::Behavior:
                return inVisitor(*NVUnionCast<SBehaviorAsset *>(inData));
            case AssetValueTypes::QmlPresentation:
                return inVisitor(*NVUnionCast<SQmlPresentationAsset*>(inData));
            default:
                QT3DS_ASSERT(false);
            case AssetValueTypes::NoAssetValue:
                return inVisitor();
            }
        }

        template <typename TRetType, typename TVisitorType>
        static TRetType visit(const char *inData, TIdType inType, TVisitorType inVisitor)
        {
            switch (inType) {
            case AssetValueTypes::Presentation:
                return inVisitor(*NVUnionCast<const SPresentationAsset *>(inData));
            case AssetValueTypes::SCXML:
                return inVisitor(*NVUnionCast<const SSCXMLAsset *>(inData));
            case AssetValueTypes::RenderPlugin:
                return inVisitor(*NVUnionCast<const SRenderPluginAsset *>(inData));
            case AssetValueTypes::Behavior:
                return inVisitor(*NVUnionCast<const SBehaviorAsset *>(inData));
            case AssetValueTypes::QmlPresentation:
                return inVisitor(*NVUnionCast<const SQmlPresentationAsset *>(inData));
            default:
                QT3DS_ASSERT(false);
            case AssetValueTypes::NoAssetValue:
                return inVisitor();
            }
        }
    };

    typedef qt3ds::foundation::
        DiscriminatedUnion<qt3ds::foundation::
                               DiscriminatedUnionGenericBase<SAssetValueUnionTraits,
                                                             SAssetValueUnionTraits::TBufferSize>,
                           SAssetValueUnionTraits::TBufferSize>
            TAssetValueUnionType;

    struct SAssetValue : public TAssetValueUnionType
    {
        SAssetValue() {}

        SAssetValue(const SAssetValue &inOther)
            : TAssetValueUnionType(static_cast<const TAssetValueUnionType &>(inOther))
        {
        }

        template <typename TDataType>
        SAssetValue(const TDataType &inDt)
            : TAssetValueUnionType(inDt)
        {
        }

        SAssetValue &operator=(const SAssetValue &inOther)
        {
            TAssetValueUnionType::operator=(inOther);
            return *this;
        }

        bool operator==(const SAssetValue &inOther) const
        {
            return TAssetValueUnionType::operator==(inOther);
        }
        bool operator!=(const SAssetValue &inOther) const
        {
            return TAssetValueUnionType::operator!=(inOther);
        }

        bool empty() const { return getType() == AssetValueTypes::NoAssetValue; }

        CRegisteredString GetSource()
        {
            if (empty())
                return CRegisteredString();
            return reinterpret_cast<SAssetBase *>(m_Data)->m_Src;
        }
    };
}
}
