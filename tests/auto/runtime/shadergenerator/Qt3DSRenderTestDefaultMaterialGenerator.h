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

#ifndef QT3DS_RENDER_TEST_DEFAULT_MATERIAL_GENERATOR_H
#define QT3DS_RENDER_TEST_DEFAULT_MATERIAL_GENERATOR_H

#include "../Qt3DSRenderTestBase.h"


namespace qt3ds {
namespace render {

class Qt3DSRenderTestDefaultMaterialGenerator : public NVRenderTestBase
{
public:
    Qt3DSRenderTestDefaultMaterialGenerator();
    ~Qt3DSRenderTestDefaultMaterialGenerator();

    bool isSupported(NVRenderContext *context);
    bool run(NVRenderContext *context, userContextData *pUserData);
    bool runPerformance(NVRenderContext *context, userContextData *pContextData);
    void cleanup(NVRenderContext *context, userContextData *pUserData);

private:

};

} // render
} // qt3ds

#endif
