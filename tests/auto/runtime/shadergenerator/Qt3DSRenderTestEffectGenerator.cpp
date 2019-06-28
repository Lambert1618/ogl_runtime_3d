/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "Qt3DSRenderTestEffectGenerator.h"
#include "../Qt3DSRenderTestMathUtil.h"
#include "render/Qt3DSRenderShaderProgram.h"
#include "render/Qt3DSRenderContext.h"
#include "Qt3DSRenderCustomMaterialSystem.h"
#include "Qt3DSRenderCustomMaterialRenderContext.h"
#include "Qt3DSRenderCustomMaterialShaderGenerator.h"
#include "Qt3DSRenderDynamicObjectSystem.h"
#include "Qt3DSRenderDynamicObjectSystemCommands.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSTypes.h"
#include "Qt3DSRenderRuntimeBinding.h"
#include "Qt3DSApplication.h"
#include "Qt3DSInputEngine.h"
#include "foundation/FileTools.h"
#include "Qt3DSWindowSystem.h"
#include "Qt3DSRenderShaderCache.h"
#include "rendererimpl/Qt3DSRendererImpl.h"
#include "Qt3DSRenderLight.h"
#include "Qt3DSRenderUIPLoader.h"
#include "Qt3DSDMMetaDataTypes.h"

#include <QTime>
#include <QString>
#include <QStringList>

#include <string>

using namespace qt3ds::render;

namespace qt3ds {
namespace render {

Qt3DSRenderTestEffectGenerator::Qt3DSRenderTestEffectGenerator()
{

}

Qt3DSRenderTestEffectGenerator::~Qt3DSRenderTestEffectGenerator()
{

}

bool Qt3DSRenderTestEffectGenerator::isSupported(NVRenderContext *context)
{
    Q_UNUSED(context);
    return true;
}

bool Qt3DSRenderTestEffectGenerator::runPerformance(NVRenderContext *context,
                                                    userContextData *pContextData)
{
    Q_UNUSED(context);
    Q_UNUSED(pContextData);
    return false;
}

void Qt3DSRenderTestEffectGenerator::cleanup(NVRenderContext *context,
                                             userContextData *pUserData)
{
    Q_UNUSED(context);
    Q_UNUSED(pUserData);
}

bool GenShader(IQt3DSRenderContext &qt3dsContext, SEffect &effect, qt3dsdm::SMetaDataEffect *metaEffect)
{
    bool success = true;
    for (int i = 0; i < metaEffect->m_EffectCommands.size(); i++) {
        dynamic::SCommand &command = *metaEffect->m_EffectCommands[i];
        if (command.m_Type == dynamic::CommandTypes::Enum::BindShader) {
            dynamic::SBindShader *bindShader = static_cast<dynamic::SBindShader *>(&command);
            NVRenderShaderProgram *theProgram =
                qt3dsContext.GetDynamicObjectSystem()
                    .GetShaderProgram(bindShader->m_ShaderPath, bindShader->m_ShaderDefine,
                                      TShaderFeatureSet(), dynamic::SDynamicShaderProgramFlags())
                    .first;
            if (!theProgram)
                success = false;
        }
    }
    return success;
}

bool Qt3DSRenderTestEffectGenerator::run(NVRenderContext *context, userContextData *pUserData)
{
    Q_UNUSED(pUserData);
    bool success = true;

    QStringList effectFiles;
    effectFiles.append("Desaturate.effect");
    effectFiles.append("Gaussian Blur.effect");
    effectFiles.append("Sepia.effect");
    effectFiles.append("Bloom.effect");

    for (QString effectName : effectFiles) {
        QString qfile = "qrc:/";
        qfile.append(effectName);
        QByteArray data = qfile.toLatin1();
        const char *cname = data.data();
        CRegisteredString name = context->GetStringTable().RegisterStr(cname);

        metadata()->LoadEffectXMLFile("Effect", "", effectName.toLatin1().data(), cname);
        Option<qt3dsdm::SMetaDataEffect> metaEffect =
                            metadata()->GetEffectMetaDataBySourcePath(cname);

        if (metaEffect.hasValue()) {
            qt3ds::render::IUIPLoader::CreateEffectClassFromMetaEffect(
                name, context->GetFoundation(),
                qt3dsRenderer()->GetQt3DSContext().GetEffectSystem(), *metaEffect,
                context->GetStringTable());

             SEffect *effect = qt3dsRenderer()->GetQt3DSContext().GetEffectSystem()
                    .CreateEffectInstance(name, qt3dsRenderer()->GetContext().GetAllocator());

            success &= GenShader(qt3dsRenderer()->GetQt3DSContext(), *effect, &metaEffect.getValue());
            if (!success)
                qDebug () << "failed effect: " << effectName;
            delete effect;
        }
    }

    return success;
}

} // render
} // qt3ds
