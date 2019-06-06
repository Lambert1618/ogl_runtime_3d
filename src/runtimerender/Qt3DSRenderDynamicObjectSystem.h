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
#ifndef QT3DS_RENDER_DYNAMIC_OBJECT_SYSTEM_H
#define QT3DS_RENDER_DYNAMIC_OBJECT_SYSTEM_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/Qt3DSRefCounted.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSVec2.h"
#include "Qt3DSRenderShaderCache.h"
#include "Qt3DSRenderTessModeValues.h"
#include "Qt3DSRenderGraphObjectTypes.h"
#include "EASTL/utility.h"

#include <QtCore/qstring.h>

namespace qt3ds {
namespace render {
    struct SDynamicObject;

    namespace dynamic {

        struct SCommand;

        struct SPropertyDeclaration
        {
            const char8_t *m_Name;
            // The datatypes map directly to the obvious types *except*
            // for NVRenderTexture2DPtr.  This type will be interpreted as a
            // CRegisteredString (they are the same binary size)
            // and will be used to lookup the texture from the buffer manager.
            NVRenderShaderDataTypes::Enum m_DataType;

            SPropertyDeclaration(const char8_t *inName, NVRenderShaderDataTypes::Enum inDtype)
                : m_Name(inName)
                , m_DataType(inDtype)
            {
            }
            SPropertyDeclaration()
                : m_Name("")
                , m_DataType(NVRenderShaderDataTypes::Unknown)
            {
            }
        };

        struct SPropertyDefinition
        {
            CRegisteredString m_Name;

            //*not* relative to the presentation directory
            CRegisteredString m_ImagePath;
            // The datatypes map directly to the obvious types *except*
            // for NVRenderTexture2DPtr.  This type will be interpreted as a
            // CRegisteredString and will be used to lookup the texture
            // from the buffer manager.
            NVRenderShaderDataTypes::Enum m_DataType;
            // All offsets are relative to the beginning of the SEffect
            // and are aligned to 4 byte boundaries.
            QT3DSU32 m_Offset;
            // Sizeof this datatype.
            QT3DSU32 m_ByteSize;
            NVConstDataRef<CRegisteredString> m_EnumValueNames;

            NVRenderTextureTypeValue::Enum
                m_TexUsageType; ///< texture usage type like diffuse, specular, ...
            // Applies to both s,t
            NVRenderTextureCoordOp::Enum m_CoordOp;
            // Set mag Filter
            NVRenderTextureMagnifyingOp::Enum m_MagFilterOp;
            // Set min Filter
            NVRenderTextureMinifyingOp::Enum m_MinFilterOp;
            bool m_IsEnumProperty;
            SPropertyDefinition()
                : m_DataType(NVRenderShaderDataTypes::Unknown)
                , m_Offset(0)
                , m_ByteSize(0)
                , m_TexUsageType(NVRenderTextureTypeValue::Unknown)
                , m_CoordOp(NVRenderTextureCoordOp::ClampToEdge)
                , m_MagFilterOp(NVRenderTextureMagnifyingOp::Linear)
                , m_MinFilterOp(NVRenderTextureMinifyingOp::Linear)
                , m_IsEnumProperty(false)
            {
            }
            SPropertyDefinition(CRegisteredString inName, NVRenderShaderDataTypes::Enum inType,
                                QT3DSU32 inOffset, QT3DSU32 inByteSize)
                : m_Name(inName)
                , m_DataType(inType)
                , m_Offset(inOffset)
                , m_ByteSize(inByteSize)
                , m_TexUsageType(NVRenderTextureTypeValue::Unknown)
                , m_CoordOp(NVRenderTextureCoordOp::ClampToEdge)
                , m_MagFilterOp(NVRenderTextureMagnifyingOp::Linear)
                , m_MinFilterOp(NVRenderTextureMinifyingOp::Linear)
                , m_IsEnumProperty(false)
            {
            }
        };

        struct SDynamicShaderProgramFlags : public SShaderCacheProgramFlags
        {
            TessModeValues::Enum m_TessMode;
            bool m_WireframeMode;

            SDynamicShaderProgramFlags()
                : m_TessMode(TessModeValues::NoTess)
                , m_WireframeMode(false)
            {
            }

            SDynamicShaderProgramFlags(TessModeValues::Enum inTessMode, bool inWireframeMode)
                : m_TessMode(inTessMode)
                , m_WireframeMode(inWireframeMode)
            {
            }

            static const char *wireframeToString(bool inEnable)
            {
                if (inEnable)
                    return "wireframeMode:true";
                else
                    return "wireframeMode:false";
            }
        };
    }

    class IDynamicObjectClass
    {
    protected:
        virtual ~IDynamicObjectClass() {}
    public:
        virtual CRegisteredString GetId() const = 0;
        virtual NVConstDataRef<dynamic::SPropertyDefinition> GetProperties() const = 0;
        virtual QT3DSU32 GetPropertySectionByteSize() const = 0;
        virtual const QT3DSU8 *GetDefaultValueBuffer() const = 0;
        virtual QT3DSU32 GetBaseObjectSize() const = 0;
        virtual GraphObjectTypes::Enum GraphObjectType() const = 0;
        virtual const dynamic::SPropertyDefinition *
        FindPropertyByName(CRegisteredString inName) const = 0;
        virtual NVConstDataRef<dynamic::SCommand *> GetRenderCommands() const = 0;
        virtual bool RequiresDepthTexture() const = 0;
        virtual void SetRequiresDepthTexture(bool inRequires) = 0;
        virtual bool RequiresCompilation() const = 0;
        virtual void SetRequiresCompilation(bool inRequires) = 0;
        virtual NVRenderTextureFormats::Enum GetOutputTextureFormat() const = 0;
    };

    class IDynamicObjectSystemCore : public NVRefCounted
    {
    protected:
        virtual ~IDynamicObjectSystemCore() {}
    public:
        virtual bool IsRegistered(CRegisteredString inStr) = 0;

        virtual bool Register(CRegisteredString inName,
                              NVConstDataRef<dynamic::SPropertyDeclaration> inProperties,
                              QT3DSU32 inBaseObjectSize, GraphObjectTypes::Enum inGraphObjectType) = 0;

        virtual bool Unregister(CRegisteredString inName) = 0;

        // Set the default value.  THis is unnecessary if the default is zero as that is what it is
        // assumed to be.
        virtual void SetPropertyDefaultValue(CRegisteredString inName, CRegisteredString inPropName,
                                             NVConstDataRef<QT3DSU8> inDefaultData) = 0;

        virtual void SetPropertyEnumNames(CRegisteredString inName, CRegisteredString inPropName,
                                          NVConstDataRef<CRegisteredString> inNames) = 0;

        virtual NVConstDataRef<CRegisteredString>
        GetPropertyEnumNames(CRegisteredString inName, CRegisteredString inPropName) const = 0;

        virtual NVConstDataRef<dynamic::SPropertyDefinition>
        GetProperties(CRegisteredString inName) const = 0;

        virtual void SetPropertyTextureSettings(CRegisteredString inName,
                                                CRegisteredString inPropName,
                                                CRegisteredString inPropPath,
                                                NVRenderTextureTypeValue::Enum inTexType,
                                                NVRenderTextureCoordOp::Enum inCoordOp,
                                                NVRenderTextureMagnifyingOp::Enum inMagFilterOp,
                                                NVRenderTextureMinifyingOp::Enum inMinFilterOp) = 0;

        virtual IDynamicObjectClass *GetDynamicObjectClass(CRegisteredString inName) = 0;

        // The effect commands are the actual commands that run for a given effect.  The tell the
        // system exactly
        // explicitly things like bind this shader, bind this render target, apply this property,
        // run this shader
        // See UICRenderEffectCommands.h for the list of commands.
        // These commands are copied into the effect.
        virtual void SetRenderCommands(CRegisteredString inClassName,
                                       NVConstDataRef<dynamic::SCommand *> inCommands) = 0;
        virtual NVConstDataRef<dynamic::SCommand *>
        GetRenderCommands(CRegisteredString inClassName) const = 0;

        virtual SDynamicObject *CreateInstance(CRegisteredString inClassName,
                                               NVAllocatorCallback &inSceneGraphAllocator) = 0;

        // scan shader for #includes and insert any found"
        virtual void InsertShaderHeaderInformation(Qt3DSString &inShader,
                                                   const char *inLogPath) = 0;

        // Set the shader data for a given path.  Used when a path doesn't correspond to a file but
        // the data has been
        // auto-generated.  The system will look for data under this path key during the BindShader
        // effect command.
        virtual void SetShaderData(CRegisteredString inPath, const char8_t *inData,
                                   const char8_t *inShaderType = NULL,
                                   const char8_t *inShaderVersion = NULL,
                                   bool inHasGeomShader = false,
                                   bool inIsComputeShader = false) = 0;

        // Overall save functions for saving the class information out to the binary file.
        virtual void Save(qt3ds::render::SWriteBuffer &ioBuffer,
                          const qt3ds::render::SStrRemapMap &inRemapMap,
                          const char8_t *inProjectDir) const = 0;
        virtual void Load(NVDataRef<QT3DSU8> inData, CStrTableOrDataRef inStrDataBlock,
                          const char8_t *inProjectDir) = 0;

        virtual IDynamicObjectSystem &CreateDynamicSystem(IQt3DSRenderContext &rc) = 0;

        static IDynamicObjectSystemCore &CreateDynamicSystemCore(IQt3DSRenderContextCore &rc);
    };

    typedef eastl::pair<NVScopedRefCounted<NVRenderShaderProgram>,
                        dynamic::SDynamicShaderProgramFlags>
        TShaderAndFlags;

    class IDynamicObjectSystem : public IDynamicObjectSystemCore
    {
    protected:
        virtual ~IDynamicObjectSystem() {}

    public:
        virtual TShaderAndFlags
        GetShaderProgram(CRegisteredString inPath, CRegisteredString inProgramMacro,
                         TShaderFeatureSet inFeatureSet,
                         const dynamic::SDynamicShaderProgramFlags &inFlags,
                         bool inForceCompilation = false) = 0;

        virtual void GetShaderSource(CRegisteredString inPath, Qt3DSString &outSource) = 0;

        // Will return null in the case where a custom prepass shader isn't needed for this object
        // If no geom shader, then no depth prepass shader.
        virtual TShaderAndFlags GetDepthPrepassShader(CRegisteredString inPath,
                                                      CRegisteredString inProgramMacro,
                                                      TShaderFeatureSet inFeatureSet) = 0;

        virtual void setShaderCodeLibraryPlatformDirectory(const QString &directory) = 0;
        virtual QString shaderCodeLibraryPlatformDirectory() = 0;

        static QString GetShaderCodeLibraryDirectory() { return QStringLiteral("res/effectlib"); }
    };
}
}

#endif
