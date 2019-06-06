/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#ifndef QT3DS_RENDER_PLUGIN_H
#define QT3DS_RENDER_PLUGIN_H
#include "Qt3DSRender.h"
#include "Qt3DSRenderPluginCInterface.h"
#include "Qt3DSOffscreenRenderManager.h"
#include "EASTL/utility.h"

namespace qt3ds {
namespace render {

    // UICRenderPluginPropertyValue.h
    struct SRenderPropertyValueUpdate;

    class IRenderPluginInstance : public IOffscreenRenderer
    {
    protected:
        virtual ~IRenderPluginInstance() {}
    public:
        static const char *IRenderPluginOffscreenRendererType() { return "IRenderPluginInstance"; }
        // If this render plugin has an instance ptr, get it.
        virtual TRenderPluginInstancePtr GetRenderPluginInstance() = 0;
        virtual void Update(NVConstDataRef<SRenderPropertyValueUpdate> updateBuffer) = 0;
        virtual IRenderPluginClass &GetPluginClass() = 0;
        virtual void CreateScriptProxy(script_State *state) = 0;
    };
    struct RenderPluginPropertyValueTypes
    {
        enum Enum {
            NoRenderPluginPropertyValue = 0,
            Boolean,
            Long,
            Float,
            String,
        };
    };

    struct SRenderPluginPropertyTypes
    {
        enum Enum {
            UnknownRenderPluginPropertyType = 0,
            Float,
            Vector3,
            Vector2,
            Color,
            Boolean,
            Long,
            String,
        };
    };

    struct SRenderPluginPropertyDeclaration
    {
        CRegisteredString m_Name;
        SRenderPluginPropertyTypes::Enum m_Type;
        // Filled in by the class, ignored if set on registered property
        QT3DSU32 m_StartOffset;
        SRenderPluginPropertyDeclaration()
            : m_Type(SRenderPluginPropertyTypes::UnknownRenderPluginPropertyType)
        {
        }
        SRenderPluginPropertyDeclaration(CRegisteredString n, SRenderPluginPropertyTypes::Enum t)
            : m_Name(n)
            , m_Type(t)
            , m_StartOffset(0)
        {
        }
    };

    class IRenderPluginClass : public NVRefCounted
    {
    protected:
        virtual ~IRenderPluginClass() {}
    public:
        virtual NVScopedRefCounted<IRenderPluginInstance> CreateInstance() = 0;
        virtual void RegisterProperty(const SRenderPluginPropertyDeclaration &dec) = 0;
        virtual NVConstDataRef<SRenderPluginPropertyDeclaration> GetRegisteredProperties() = 0;
        // The declaration contains an offset
        virtual SRenderPluginPropertyDeclaration
        GetPropertyDeclaration(CRegisteredString inPropName) = 0;
        // From which you can get the property name breakdown
        virtual eastl::pair<CRegisteredString, RenderPluginPropertyValueTypes::Enum>
        GetPropertyValueInfo(QT3DSU32 inIndex) = 0;
    };

    class IRenderPluginManager;

    class IRenderPluginManagerCore : public NVRefCounted
    {
    public:
        virtual void SetDllDir(const char *inDllDir) = 0;
        virtual void Load(NVDataRef<QT3DSU8> inData, CStrTableOrDataRef inStrDataBlock,
                          const char8_t *inProjectDir) = 0;
        virtual IRenderPluginManager &GetRenderPluginManager(NVRenderContext &rc) = 0;

        static IRenderPluginManagerCore &Create(NVFoundationBase &inFoundation,
                                                IStringTable &strTable,
                                                IInputStreamFactory &inFactory);
    };

    class IRenderPluginManager : public NVRefCounted
    {
    public:
        virtual IRenderPluginClass *GetRenderPlugin(CRegisteredString inRelativePath) = 0;
        virtual IRenderPluginClass *GetOrCreateRenderPlugin(CRegisteredString inRelativePath) = 0;
        // Map a render plugin instance to this key.  The instance's lifetime is managed by the
        // manager so a client does not
        // need to manage it.
        virtual IRenderPluginInstance *
        GetOrCreateRenderPluginInstance(CRegisteredString inRelativePath, void *inKey) = 0;

        virtual void Save(qt3ds::render::SWriteBuffer &ioBuffer,
                          const qt3ds::render::SStrRemapMap &inRemapMap,
                          const char8_t *inProjectDir) const = 0;
    };
}
}

#endif
