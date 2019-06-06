/****************************************************************************
**
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

#ifndef QT3DS_RENDER_LIGHT_CONSTANT_PROPERTIES
#define QT3DS_RENDER_LIGHT_CONSTANT_PROPERTIES

#include "render/Qt3DSRenderShaderProgram.h"

namespace qt3ds {
namespace render {

static const QStringList lconstantnames = {
    QStringLiteral("position"),
    QStringLiteral("direction"),
    QStringLiteral("up"),
    QStringLiteral("right"),
    QStringLiteral("diffuse"),
    QStringLiteral("ambient"),
    QStringLiteral("specular"),
    QStringLiteral("spotExponent"),
    QStringLiteral("spotCutoff"),
    QStringLiteral("constantAttenuation"),
    QStringLiteral("linearAttenuation"),
    QStringLiteral("quadraticAttenuation"),
    QStringLiteral("range"),
    QStringLiteral("width"),
    QStringLiteral("height"),
    QStringLiteral("shadowControls"),
    QStringLiteral("shadowView"),
    QStringLiteral("shadowIdx"),
    QStringLiteral("attenuation")
};

#define LCSEED QStringLiteral("%1%2")

template <typename GeneratedShader>
struct SLightConstantProperties
{
    struct LightConstants
    {
        NVRenderCachedShaderProperty<QT3DSVec4> m_position;
        NVRenderCachedShaderProperty<QT3DSVec4> m_direction;
        NVRenderCachedShaderProperty<QT3DSVec4> m_up;
        NVRenderCachedShaderProperty<QT3DSVec4> m_right;
        NVRenderCachedShaderProperty<QT3DSVec4> m_diffuse;
        NVRenderCachedShaderProperty<QT3DSVec4> m_ambient;
        NVRenderCachedShaderProperty<QT3DSVec4> m_specular;
        NVRenderCachedShaderProperty<QT3DSF32> m_spotExponent;
        NVRenderCachedShaderProperty<QT3DSF32> m_spotCutoff;
        NVRenderCachedShaderProperty<QT3DSF32> m_constantAttenuation;
        NVRenderCachedShaderProperty<QT3DSF32> m_linearAttenuation;
        NVRenderCachedShaderProperty<QT3DSF32> m_quadraticAttenuation;
        NVRenderCachedShaderProperty<QT3DSF32> m_range;
        NVRenderCachedShaderProperty<QT3DSF32> m_width;
        NVRenderCachedShaderProperty<QT3DSF32> m_height;
        NVRenderCachedShaderProperty<QT3DSVec4> m_shadowControls;
        NVRenderCachedShaderProperty<QT3DSMat44> m_shadowView;
        NVRenderCachedShaderProperty<QT3DSI32> m_shadowIdx;
        NVRenderCachedShaderProperty<QT3DSVec3> m_attenuation;

        LightConstants(const QString &lightRef, render::NVRenderShaderProgram &shader)
            : m_position(LCSEED.arg(lightRef, lconstantnames[0]), shader)
            , m_direction(LCSEED.arg(lightRef).arg(lconstantnames[1]), shader)
            , m_up(LCSEED.arg(lightRef, lconstantnames[2]), shader)
            , m_right(LCSEED.arg(lightRef, lconstantnames[3]), shader)
            , m_diffuse(LCSEED.arg(lightRef, lconstantnames[4]), shader)
            , m_ambient(LCSEED.arg(lightRef, lconstantnames[5]), shader)
            , m_specular(LCSEED.arg(lightRef, lconstantnames[6]), shader)
            , m_spotExponent(LCSEED.arg(lightRef, lconstantnames[7]), shader)
            , m_spotCutoff(LCSEED.arg(lightRef, lconstantnames[8]), shader)
            , m_constantAttenuation(LCSEED.arg(lightRef, lconstantnames[9]), shader)
            , m_linearAttenuation(LCSEED.arg(lightRef, lconstantnames[10]), shader)
            , m_quadraticAttenuation(LCSEED.arg(lightRef, lconstantnames[11]), shader)
            , m_range(LCSEED.arg(lightRef, lconstantnames[12]), shader)
            , m_width(LCSEED.arg(lightRef, lconstantnames[13]), shader)
            , m_height(LCSEED.arg(lightRef, lconstantnames[14]), shader)
            , m_shadowControls(LCSEED.arg(lightRef, lconstantnames[15]), shader)
            , m_shadowView(LCSEED.arg(lightRef, lconstantnames[16]), shader)
            , m_shadowIdx(LCSEED.arg(lightRef, lconstantnames[17]), shader)
            , m_attenuation(LCSEED.arg(lightRef, lconstantnames[18]), shader)
        {

        }

        template <typename LightProps>
        void updateLights(LightProps &props)
        {
            m_position.Set(props.m_position);
            m_direction.Set(props.m_direction);
            m_up.Set(props.m_up);
            m_right.Set(props.m_right);
            m_diffuse.Set(props.m_diffuse);
            m_ambient.Set(props.m_ambient);
            m_specular.Set(props.m_specular);
            m_spotExponent.Set(props.m_spotExponent);
            m_spotCutoff.Set(props.m_spotCutoff);
            m_constantAttenuation.Set(props.m_constantAttenuation);
            m_linearAttenuation.Set(props.m_linearAttenuation);
            m_quadraticAttenuation.Set(props.m_quadraticAttenuation);
            m_range.Set(props.m_range);
            m_width.Set(props.m_width);
            m_height.Set(props.m_height);
            m_shadowControls.Set(props.m_shadowControls);
            m_shadowView.Set(props.m_shadowView);
            m_shadowIdx.Set(props.m_shadowIdx);
            m_attenuation.Set(QT3DSVec3(props.m_constantAttenuation,
                                        props.m_linearAttenuation,
                                        props.m_quadraticAttenuation));
        }
    };

    SLightConstantProperties(GeneratedShader &shader, bool packed)
        : m_lightCount("uNumLights", shader.m_Shader)
    {
        m_constants.resize(shader.m_Lights.size());
        for (unsigned int i = 0; i < shader.m_Lights.size(); ++i) {
            QString lref;
            if (packed)
                lref = QStringLiteral("light_%1_");
            else
                lref = QStringLiteral("lights[%1].");
            lref = lref.arg(i);
            m_constants[i] = new LightConstants(lref, shader.m_Shader);
        }
        m_lightCount.Set(shader.m_Lights.size());
        m_lightCountInt = shader.m_Lights.size();
    }

    SLightConstantProperties(const QString &lseed, const QString &lcount,
                             GeneratedShader &shader, bool packed, int count)
        : m_lightCount(lcount, shader.m_Shader)
    {
        m_constants.resize(count);
        for (int i = 0; i < count; ++i) {
            QString lref;
            if (packed)
                lref = lseed + QStringLiteral("_%1_");
            else
                lref = lseed + QStringLiteral("[%1].");
            lref = lref.arg(i);
            m_constants[i] = new LightConstants(lref, shader.m_Shader);
        }
        m_lightCount.Set(count);
        m_lightCountInt = count;
    }

    ~SLightConstantProperties()
    {
        qDeleteAll(m_constants);
    }

    void updateLights(GeneratedShader &shader)
    {
        for (int i = 0; i < m_constants.size(); ++i)
            m_constants[i]->updateLights(shader.m_Lights[i].m_LightData);
    }
    template <typename LightProps>
    void updateLights(const QVector<LightProps*> &props)
    {
        for (int i = 0; i < m_constants.size(); ++i)
            m_constants[i]->updateLights(props[i]->m_LightData);
    }

    QVector<LightConstants *> m_constants;
    NVRenderCachedShaderProperty<QT3DSI32> m_lightCount;
    int m_lightCountInt;
};

}
}

#endif
