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
#ifndef QT3DS_RENDER_SHADER_KEY_H
#define QT3DS_RENDER_SHADER_KEY_H
#include "Qt3DSRender.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/Qt3DSDataRef.h"
#include "EASTL/string.h"
#include "foundation/StringConversionImpl.h"
#include "Qt3DSRenderDefaultMaterial.h"
#include "Qt3DSRenderTessModeValues.h"
#include "render/Qt3DSRenderBaseTypes.h"

namespace qt3ds {
namespace render {
    // We have an ever expanding set of properties we like to hash into one or more 32 bit
    // quantities.
    // Furthermore we would like this set of properties to be convertable to string
    // So the shader cache file itself is somewhat human readable/diagnosable.
    // To do this we create a set of objects that act as properties to the master shader key.
    // These objects are tallied in order to figure out their actual offset into the shader key's
    // data store.  They are also run through in order to create the string shader cache key.

    struct SShaderKeyPropertyBase
    {
        const char *m_Name;
        QT3DSU32 m_Offset;
        SShaderKeyPropertyBase(const char *name = "")
            : m_Name(name)
            , m_Offset(0)
        {
        }
        QT3DSU32 GetOffset() const { return m_Offset; }
        void SetOffset(QT3DSU32 of) { m_Offset = of; }

        template <QT3DSU32 TBitWidth>
        QT3DSU32 GetMaskTemplate() const
        {
            QT3DSU32 bit = m_Offset % 32;
            QT3DSU32 startValue = (1 << TBitWidth) - 1;
            QT3DSU32 mask = startValue << bit;
            return mask;
        }

        QT3DSU32 GetIdx() const { return m_Offset / 32; }
    protected:
        void InternalToString(eastl::string &ioStr, const char *inBuffer) const
        {
            ioStr.append(m_Name);
            ioStr.append("=");
            ioStr.append(inBuffer);
        }

        static void InternalToString(eastl::string &ioStr, const char *name, bool inValue)
        {
            if (inValue) {
                ioStr.append(name);
                ioStr.append("=");
                ioStr.append(inValue ? "true" : "false");
            }
        }
    };

    struct SShaderKeyBoolean : public SShaderKeyPropertyBase
    {
        enum {
            BitWidth = 1,
        };

        SShaderKeyBoolean(const char *name = "")
            : SShaderKeyPropertyBase(name)
        {
        }

        QT3DSU32 GetMask() const { return GetMaskTemplate<BitWidth>(); }
        void SetValue(NVDataRef<QT3DSU32> inDataStore, bool inValue) const
        {
            QT3DSU32 idx = GetIdx();
            QT3DS_ASSERT(inDataStore.size() > idx);
            QT3DSU32 mask = GetMask();
            QT3DSU32 &target = inDataStore[idx];
            if (inValue == true) {
                target = target | mask;
            } else {
                mask = ~mask;
                target = target & mask;
            }
        }

        bool GetValue(NVConstDataRef<QT3DSU32> inDataStore) const
        {
            QT3DSU32 idx = GetIdx();
            QT3DSU32 mask = GetMask();
            const QT3DSU32 &target = inDataStore[idx];
            return (target & mask) ? true : false;
        }

        void ToString(eastl::string &ioStr, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            bool isHigh = GetValue(inKeySet);
            InternalToString(ioStr, m_Name, isHigh);
        }
    };

    template <QT3DSU32 TBitWidth>
    struct SShaderKeyUnsigned : public SShaderKeyPropertyBase
    {
        enum {
            BitWidth = TBitWidth,
        };
        SShaderKeyUnsigned(const char *name = "")
            : SShaderKeyPropertyBase(name)
        {
        }
        QT3DSU32 GetMask() const { return GetMaskTemplate<BitWidth>(); }
        void SetValue(NVDataRef<QT3DSU32> inDataStore, QT3DSU32 inValue) const
        {
            QT3DSU32 startValue = (1 << TBitWidth) - 1;
            // Ensure inValue is within range of bit width.
            inValue = inValue & startValue;
            QT3DSU32 bit = m_Offset % 32;
            QT3DSU32 mask = GetMask();
            QT3DSU32 idx = GetIdx();
            inValue = inValue << bit;
            QT3DSU32 &target = inDataStore[idx];
            // Get rid of existing value
            QT3DSU32 inverseMask = ~mask;
            target = target & inverseMask;
            target = target | inValue;
        }

        QT3DSU32 GetValue(NVConstDataRef<QT3DSU32> inDataStore) const
        {
            QT3DSU32 idx = GetIdx();
            QT3DSU32 bit = m_Offset % 32;
            QT3DSU32 mask = GetMask();
            const QT3DSU32 &target = inDataStore[idx];

            QT3DSU32 retval = target & mask;
            retval = retval >> bit;
            return retval;
        }

        void ToString(eastl::string &ioStr, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            QT3DSU32 value = GetValue(inKeySet);
            char buf[64];
            StringConversion<QT3DSU32>().ToStr(value, toDataRef(buf, 64));
            InternalToString(ioStr, buf);
        }
    };

    struct SShaderKeyTessellation : public SShaderKeyUnsigned<4>
    {
        enum TessellationBits {
            noTessellation = 1 << 0,
            linearTessellation = 1 << 1,
            phongTessellation = 1 << 2,
            npatchTessellation = 1 << 3
        };

        SShaderKeyTessellation(const char *name = "")
            : SShaderKeyUnsigned<4>(name)
        {
        }

        bool GetBitValue(TessellationBits swizzleBit, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return (GetValue(inKeySet) & swizzleBit) ? true : false;
        }

        void SetBitValue(TessellationBits swizzleBit, bool inValue, NVDataRef<QT3DSU32> inKeySet)
        {
            QT3DSU32 theValue = GetValue(inKeySet);
            QT3DSU32 mask = swizzleBit;
            if (inValue) {
                theValue = theValue | mask;
            } else {
                mask = ~mask;
                theValue = theValue & mask;
            }
            SetValue(inKeySet, theValue);
        }

        void SetTessellationMode(NVDataRef<QT3DSU32> inKeySet, TessModeValues::Enum tessellationMode,
                                 bool val)
        {
            switch (tessellationMode) {
            case TessModeValues::NoTess:
                SetBitValue(noTessellation, val, inKeySet);
                break;
            case TessModeValues::TessLinear:
                SetBitValue(linearTessellation, val, inKeySet);
                break;
            case TessModeValues::TessNPatch:
                SetBitValue(npatchTessellation, val, inKeySet);
                break;
            case TessModeValues::TessPhong:
                SetBitValue(phongTessellation, val, inKeySet);
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }

        bool IsNoTessellation(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(noTessellation, inKeySet);
        }
        void SetNoTessellation(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(noTessellation, val, inKeySet);
        }

        bool IsLinearTessellation(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(linearTessellation, inKeySet);
        }
        void SetLinearTessellation(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(linearTessellation, val, inKeySet);
        }

        bool IsNPatchTessellation(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(npatchTessellation, inKeySet);
        }
        void SetNPatchTessellation(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(npatchTessellation, val, inKeySet);
        }

        bool IsPhongTessellation(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(phongTessellation, inKeySet);
        }
        void SetPhongTessellation(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(phongTessellation, val, inKeySet);
        }

        void ToString(eastl::string &ioStr, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            ioStr.append(m_Name);
            ioStr.append("={");
            InternalToString(ioStr, "noTessellation", IsNoTessellation(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "linearTessellation", IsLinearTessellation(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "npatchTessellation", IsNPatchTessellation(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "phongTessellation", IsPhongTessellation(inKeySet));
            ioStr.append("}");
        }
    };

    struct SShaderKeyTextureSwizzle : public SShaderKeyUnsigned<5>
    {
        enum TextureSwizzleBits {
            noSwizzle = 1 << 0,
            L8toR8 = 1 << 1,
            A8toR8 = 1 << 2,
            L8A8toRG8 = 1 << 3,
            L16toR16 = 1 << 4
        };

        SShaderKeyTextureSwizzle(const char *name = "")
            : SShaderKeyUnsigned<5>(name)
        {
        }

        bool GetBitValue(TextureSwizzleBits swizzleBit, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return (GetValue(inKeySet) & swizzleBit) ? true : false;
        }

        void SetBitValue(TextureSwizzleBits swizzleBit, bool inValue, NVDataRef<QT3DSU32> inKeySet)
        {
            QT3DSU32 theValue = GetValue(inKeySet);
            QT3DSU32 mask = swizzleBit;
            if (inValue) {
                theValue = theValue | mask;
            } else {
                mask = ~mask;
                theValue = theValue & mask;
            }
            SetValue(inKeySet, theValue);
        }

        void SetSwizzleMode(NVDataRef<QT3DSU32> inKeySet, NVRenderTextureSwizzleMode::Enum swizzleMode,
                            bool val)
        {
            switch (swizzleMode) {
            case NVRenderTextureSwizzleMode::NoSwizzle:
                SetBitValue(noSwizzle, val, inKeySet);
                break;
            case NVRenderTextureSwizzleMode::L8toR8:
                SetBitValue(L8toR8, val, inKeySet);
                break;
            case NVRenderTextureSwizzleMode::A8toR8:
                SetBitValue(A8toR8, val, inKeySet);
                break;
            case NVRenderTextureSwizzleMode::L8A8toRG8:
                SetBitValue(L8A8toRG8, val, inKeySet);
                break;
            case NVRenderTextureSwizzleMode::L16toR16:
                SetBitValue(L16toR16, val, inKeySet);
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }

        bool IsNoSwizzled(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(noSwizzle, inKeySet);
        }
        void SetNoSwizzled(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(noSwizzle, val, inKeySet);
        }

        bool IsL8Swizzled(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(L8toR8, inKeySet);
        }
        void SetL8Swizzled(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(L8toR8, val, inKeySet);
        }

        bool IsA8Swizzled(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(A8toR8, inKeySet);
        }
        void SetA8Swizzled(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(A8toR8, val, inKeySet);
        }

        bool IsL8A8Swizzled(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(L8A8toRG8, inKeySet);
        }
        void SetL8A8Swizzled(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(L8A8toRG8, val, inKeySet);
        }

        bool IsL16Swizzled(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(L16toR16, inKeySet);
        }
        void SetL16Swizzled(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(L16toR16, val, inKeySet);
        }

        void ToString(eastl::string &ioStr, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            ioStr.append(m_Name);
            ioStr.append("={");
            InternalToString(ioStr, "noswizzle", IsNoSwizzled(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "l8swizzle", IsL8Swizzled(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "a8swizzle", IsA8Swizzled(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "l8a8swizzle", IsL8A8Swizzled(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "l16swizzle", IsL16Swizzled(inKeySet));
            ioStr.append("}");
        }
    };

    struct SShaderKeyImageMap : public SShaderKeyUnsigned<5>
    {
        enum ImageMapBits {
            Enabled = 1 << 0,
            EnvMap = 1 << 1,
            LightProbe = 1 << 2,
            InvertUV = 1 << 3,
            Premultiplied = 1 << 4,
        };

        SShaderKeyImageMap(const char *name = "")
            : SShaderKeyUnsigned<5>(name)
        {
        }

        bool GetBitValue(ImageMapBits imageBit, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return (GetValue(inKeySet) & imageBit) ? true : false;
        }

        void SetBitValue(ImageMapBits imageBit, bool inValue, NVDataRef<QT3DSU32> inKeySet)
        {
            QT3DSU32 theValue = GetValue(inKeySet);
            QT3DSU32 mask = imageBit;
            if (inValue) {
                theValue = theValue | mask;
            } else {
                mask = ~mask;
                theValue = theValue & mask;
            }
            SetValue(inKeySet, theValue);
        }

        bool IsEnabled(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(Enabled, inKeySet);
        }
        void SetEnabled(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(Enabled, val, inKeySet);
        }

        bool IsEnvMap(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(EnvMap, inKeySet);
        }
        void SetEnvMap(NVDataRef<QT3DSU32> inKeySet, bool val) { SetBitValue(EnvMap, val, inKeySet); }

        bool IsLightProbe(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(LightProbe, inKeySet);
        }
        void SetLightProbe(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(LightProbe, val, inKeySet);
        }

        bool IsInvertUVMap(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(InvertUV, inKeySet);
        }
        void SetInvertUVMap(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(InvertUV, val, inKeySet);
        }

        bool IsPremultiplied(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return GetBitValue(Premultiplied, inKeySet);
        }
        void SetPremultiplied(NVDataRef<QT3DSU32> inKeySet, bool val)
        {
            SetBitValue(Premultiplied, val, inKeySet);
        }

        void ToString(eastl::string &ioStr, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            ioStr.append(m_Name);
            ioStr.append("={");
            InternalToString(ioStr, "enabled", IsEnabled(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "envMap", IsEnvMap(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "lightProbe", IsLightProbe(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "invertUV", IsInvertUVMap(inKeySet));
            ioStr.append(";");
            InternalToString(ioStr, "premultiplied", IsPremultiplied(inKeySet));
            ioStr.append("}");
        }
    };

    struct SShaderKeySpecularModel : SShaderKeyUnsigned<2>
    {
        SShaderKeySpecularModel(const char *name = "")
            : SShaderKeyUnsigned<2>(name)
        {
        }

        void SetSpecularModel(NVDataRef<QT3DSU32> inKeySet,
                              qt3ds::render::DefaultMaterialSpecularModel::Enum inModel)
        {
            SetValue(inKeySet, (QT3DSU32)inModel);
        }

        qt3ds::render::DefaultMaterialSpecularModel::Enum
        GetSpecularModel(NVConstDataRef<QT3DSU32> inKeySet) const
        {
            return static_cast<qt3ds::render::DefaultMaterialSpecularModel::Enum>(GetValue(inKeySet));
        }

        void ToString(eastl::string &ioStr, NVConstDataRef<QT3DSU32> inKeySet) const
        {
            ioStr.append(m_Name);
            ioStr.append("=");
            switch (GetSpecularModel(inKeySet)) {
            case DefaultMaterialSpecularModel::KGGX:
                ioStr.append("KGGX");
                break;
            case DefaultMaterialSpecularModel::KWard:
                ioStr.append("KWard");
                break;
            case DefaultMaterialSpecularModel::Default:
                ioStr.append("Default");
                break;
            }
            ioStr.append(";");
        }
    };

    struct SShaderDefaultMaterialKeyProperties
    {
        enum {
            LightCount = 7,
        };
        enum ImageMapNames {
            DiffuseMap0 = 0,
            DiffuseMap1,
            DiffuseMap2,
            EmissiveMap,
            EmissiveMap2,
            SpecularMap,
            OpacityMap,
            BumpMap,
            SpecularAmountMap,
            NormalMap,
            DisplacementMap,
            TranslucencyMap,
            LightmapIndirect,
            LightmapRadiosity,
            LightmapShadow,
            RoughnessMap,
            ImageMapCount
        };

        enum KeyMode
        {
            DefaultKey,
            DepthKey,
        };

        SShaderKeyBoolean m_HasLighting;
        SShaderKeyBoolean m_HasIbl;
        SShaderKeyUnsigned<3> m_LightCount;
        SShaderKeyBoolean m_LightFlags[LightCount];
        SShaderKeyBoolean m_LightAreaFlags[LightCount];
        SShaderKeyBoolean m_LightShadowFlags[LightCount];
        SShaderKeyBoolean m_SpecularEnabled;
        SShaderKeyBoolean m_FresnelEnabled;
        SShaderKeyBoolean m_AlphaTestEnabled;
        SShaderKeyBoolean m_VertexColorsEnabled;
        SShaderKeySpecularModel m_SpecularModel;
        SShaderKeyImageMap m_ImageMaps[ImageMapCount];
        SShaderKeyTextureSwizzle m_TextureSwizzle[ImageMapCount];
        SShaderKeyTessellation m_TessellationMode;
        SShaderKeyBoolean m_HasSkinning;
        SShaderKeyBoolean m_WireframeMode;

        SShaderDefaultMaterialKeyProperties()
            : m_HasLighting("hasLighting")
            , m_HasIbl("hasIbl")
            , m_LightCount("lightCount")
            , m_SpecularEnabled("specularEnabled")
            , m_FresnelEnabled("fresnelEnabled")
            , m_AlphaTestEnabled("alphaTestEnabled")
            , m_VertexColorsEnabled("vertexColorsEnabled")
            , m_SpecularModel("specularModel")
            , m_TessellationMode("tessellationMode")
            , m_HasSkinning("hasSkinning")
            , m_WireframeMode("wireframeMode")
        {
            m_LightFlags[0].m_Name = "light0HasPosition";
            m_LightFlags[1].m_Name = "light1HasPosition";
            m_LightFlags[2].m_Name = "light2HasPosition";
            m_LightFlags[3].m_Name = "light3HasPosition";
            m_LightFlags[4].m_Name = "light4HasPosition";
            m_LightFlags[5].m_Name = "light5HasPosition";
            m_LightFlags[6].m_Name = "light6HasPosition";
            m_LightAreaFlags[0].m_Name = "light0HasArea";
            m_LightAreaFlags[1].m_Name = "light1HasArea";
            m_LightAreaFlags[2].m_Name = "light2HasArea";
            m_LightAreaFlags[3].m_Name = "light3HasArea";
            m_LightAreaFlags[4].m_Name = "light4HasArea";
            m_LightAreaFlags[5].m_Name = "light5HasArea";
            m_LightAreaFlags[6].m_Name = "light6HasArea";
            m_LightShadowFlags[0].m_Name = "light0HasShadow";
            m_LightShadowFlags[1].m_Name = "light1HasShadow";
            m_LightShadowFlags[2].m_Name = "light2HasShadow";
            m_LightShadowFlags[3].m_Name = "light3HasShadow";
            m_LightShadowFlags[4].m_Name = "light4HasShadow";
            m_LightShadowFlags[5].m_Name = "light5HasShadow";
            m_LightShadowFlags[6].m_Name = "light6HasShadow";
            m_ImageMaps[0].m_Name = "diffuseMap0";
            m_ImageMaps[1].m_Name = "diffuseMap1";
            m_ImageMaps[2].m_Name = "diffuseMap2";
            m_ImageMaps[3].m_Name = "emissiveMap";
            m_ImageMaps[4].m_Name = "emissiveMap2";
            m_ImageMaps[5].m_Name = "specularMap";
            m_ImageMaps[6].m_Name = "opacityMap";
            m_ImageMaps[7].m_Name = "bumpMap";
            m_ImageMaps[8].m_Name = "specularAmountMap";
            m_ImageMaps[9].m_Name = "normalMap";
            m_ImageMaps[10].m_Name = "displacementMap";
            m_ImageMaps[11].m_Name = "translucencyMap";
            m_ImageMaps[12].m_Name = "lightmapIndirect";
            m_ImageMaps[13].m_Name = "lightmapRadiosity";
            m_ImageMaps[14].m_Name = "lightmapShadow";
            m_ImageMaps[15].m_Name = "roughnessMap";
            m_TextureSwizzle[0].m_Name = "diffuseMap0_swizzle";
            m_TextureSwizzle[1].m_Name = "diffuseMap1_swizzle";
            m_TextureSwizzle[2].m_Name = "diffuseMap2_swizzle";
            m_TextureSwizzle[3].m_Name = "emissiveMap_swizzle";
            m_TextureSwizzle[4].m_Name = "emissiveMap2_swizzle";
            m_TextureSwizzle[5].m_Name = "specularMap_swizzle";
            m_TextureSwizzle[6].m_Name = "opacityMap_swizzle";
            m_TextureSwizzle[7].m_Name = "bumpMap_swizzle";
            m_TextureSwizzle[8].m_Name = "specularAmountMap_swizzle";
            m_TextureSwizzle[9].m_Name = "normalMap_swizzle";
            m_TextureSwizzle[10].m_Name = "displacementMap_swizzle";
            m_TextureSwizzle[11].m_Name = "translucencyMap_swizzle";
            m_TextureSwizzle[12].m_Name = "lightmapIndirect_swizzle";
            m_TextureSwizzle[13].m_Name = "lightmapRadiosity_swizzle";
            m_TextureSwizzle[14].m_Name = "lightmapShadow_swizzle";
            m_TextureSwizzle[15].m_Name = "roughnessMap_swizzle";
            SetPropertyOffsets();
        }

        bool isDepthKeyImage(QT3DSU32 idx) const
        {
            if (idx == DiffuseMap0 || idx == DiffuseMap1 || idx == DiffuseMap2 || idx == SpecularMap
                    || idx == OpacityMap || idx == LightmapShadow) {
                return true;
            }
            return false;
        }

        template <typename TVisitor>
        void VisitProperties(TVisitor &inVisitor, KeyMode mode)
        {
            if (mode == DefaultKey) {
                inVisitor.Visit(m_HasLighting);
                inVisitor.Visit(m_HasIbl);
                inVisitor.Visit(m_LightCount);

                for (QT3DSU32 idx = 0, end = LightCount; idx < end; ++idx)
                    inVisitor.Visit(m_LightFlags[idx]);

                for (QT3DSU32 idx = 0, end = LightCount; idx < end; ++idx)
                    inVisitor.Visit(m_LightAreaFlags[idx]);

                for (QT3DSU32 idx = 0, end = LightCount; idx < end; ++idx)
                    inVisitor.Visit(m_LightShadowFlags[idx]);

                inVisitor.Visit(m_SpecularEnabled);
                inVisitor.Visit(m_FresnelEnabled);
                inVisitor.Visit(m_VertexColorsEnabled);
                inVisitor.Visit(m_SpecularModel);
            }
            inVisitor.Visit(m_AlphaTestEnabled);

            for (QT3DSU32 idx = 0, end = ImageMapCount; idx < end; ++idx) {
                if (mode == DefaultKey || isDepthKeyImage(idx)) {
                    inVisitor.Visit(m_ImageMaps[idx]);
                    inVisitor.Visit(m_TextureSwizzle[idx]);
                }
            }
            if (mode == DefaultKey) {
                inVisitor.Visit(m_TessellationMode);
                inVisitor.Visit(m_HasSkinning);
                inVisitor.Visit(m_WireframeMode);
            }
        }

        struct SOffsetVisitor
        {
            QT3DSU32 m_Offset;
            SOffsetVisitor()
                : m_Offset(0)
            {
            }
            template <typename TPropType>
            void Visit(TPropType &inProp)
            {
                // if we cross the 32 bit border we just move
                // to the next dword.
                // This cost a few extra bits but prevents tedious errors like
                // loosing shader key bits because they got moved beyond the 32 border
                QT3DSU32 bit = m_Offset % 32;
                if (bit + TPropType::BitWidth > 31) {
                    m_Offset += 32 - bit;
                }

                inProp.SetOffset(m_Offset);
                m_Offset += TPropType::BitWidth;
            }
        };

        void SetPropertyOffsets()
        {
            SOffsetVisitor visitor;
            VisitProperties(visitor, DefaultKey);
            // If this assert fires, then the default material key needs more bits.
            QT3DS_ASSERT(visitor.m_Offset < 224);
        }
    };

    struct SShaderDefaultMaterialKey
    {
        enum {
            DataBufferSize = 7,
        };
        QT3DSU32 m_DataBuffer[DataBufferSize];
        size_t m_FeatureSetHash;

        SShaderDefaultMaterialKey(size_t inFeatureSetHash)
            : m_FeatureSetHash(inFeatureSetHash)
        {
            for (size_t idx = 0; idx < DataBufferSize; ++idx)
                m_DataBuffer[idx] = 0;
        }

        SShaderDefaultMaterialKey()
            : m_FeatureSetHash(0)
        {
            for (size_t idx = 0; idx < DataBufferSize; ++idx)
                m_DataBuffer[idx] = 0;
        }

        size_t hash() const
        {
            size_t retval = 0;
            for (size_t idx = 0; idx < DataBufferSize; ++idx)
                retval = retval ^ eastl::hash<QT3DSU32>()(m_DataBuffer[idx]);
            return retval ^ m_FeatureSetHash;
        }

        bool operator==(const SShaderDefaultMaterialKey &other) const
        {
            bool retval = true;
            for (size_t idx = 0; idx < DataBufferSize && retval; ++idx)
                retval = m_DataBuffer[idx] == other.m_DataBuffer[idx];
            return retval && m_FeatureSetHash == other.m_FeatureSetHash;
        }

        // Cast operators to make getting properties easier.
        operator NVDataRef<QT3DSU32>() { return toDataRef(m_DataBuffer, DataBufferSize); }

        operator NVConstDataRef<QT3DSU32>() const
        {
            return toConstDataRef(m_DataBuffer, DataBufferSize);
        }

        struct SStringVisitor
        {
            eastl::string &m_Str;
            NVConstDataRef<QT3DSU32> m_KeyStore;
            SStringVisitor(eastl::string &s, NVConstDataRef<QT3DSU32> ks)
                : m_Str(s)
                , m_KeyStore(ks)
            {
            }
            template <typename TPropType>
            void Visit(const TPropType &prop)
            {
                QT3DSU32 originalSize = m_Str.size();
                if (m_Str.size())
                    m_Str.append(";");
                prop.ToString(m_Str, m_KeyStore);
                // if the only thing we added was the semicolon
                // then nuke the semicolon
                if (originalSize && m_Str.size() == originalSize + 1)
                    m_Str.resize(originalSize);
            }
        };

        void ToString(eastl::string &ioString,
                      SShaderDefaultMaterialKeyProperties &inProperties,
                      SShaderDefaultMaterialKeyProperties::KeyMode mode) const
        {
            SStringVisitor theVisitor(ioString, *this);
            inProperties.VisitProperties(theVisitor, mode);
        }
    };
}
}

namespace eastl {
template <>
struct hash<qt3ds::render::SShaderDefaultMaterialKey>
{
    size_t operator()(const qt3ds::render::SShaderDefaultMaterialKey &key) const
    {
        return key.hash();
    }
};
}

#endif
