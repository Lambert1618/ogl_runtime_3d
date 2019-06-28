/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#ifndef QT3DS_RENDER_TEST_PRIMITIVES_H
#define QT3DS_RENDER_TEST_PRIMITIVES_H

#include "../Qt3DSRenderTestBase.h"

namespace qt3ds {
namespace render {

    struct Vertex
    {
        QT3DSVec3 positions;
    };

    /// This class tests the creation of all kinds of primitives
    class NVRenderTestPrimitives : public NVRenderTestBase
    {
    public:
        NVRenderTestPrimitives();
        ~NVRenderTestPrimitives();

        bool isSupported(NVRenderContext *context);
        bool run(NVRenderContext *context, userContextData *pUserData);
        bool runPerformance(NVRenderContext *context, userContextData *pContextData);
        void cleanup(NVRenderContext *context, userContextData *pUserData);

    private:
        bool triangles(NVRenderContext *context, userContextData *pUserData);
        bool triangleStrip(NVRenderContext *context, userContextData *pUserData);
        bool lines(NVRenderContext *context, userContextData *pUserData);
        bool lineStrip(NVRenderContext *context, userContextData *pContextData);

        bool trianglesIndexed(NVRenderContext *context, userContextData *pUserData);
        bool triangleStripIndexed(NVRenderContext *context, userContextData *pUserData);
        bool linesIndexed(NVRenderContext *context, userContextData *pUserData);
        bool lineStripIndexed(NVRenderContext *context, userContextData *pContextData);

        bool renderPrimitive(NVRenderContext *context, userContextData *pContextData,
                             const Vertex *pVertexData, unsigned int vertexCount,
                             const unsigned short *pIndexData, unsigned int indexCount,
                             NVRenderDrawMode::Enum primType);

        unsigned int _curTest;
        unsigned int _cellSize;
        unsigned int _maxColumn;
    };
}
}

#endif // QT3DS_RENDER_TEST_PRIMITIVES_H
