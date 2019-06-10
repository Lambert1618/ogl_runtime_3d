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

#include "Qt3DSRenderTestClear.h"
#include "foundation/Qt3DSVec4.h"

using namespace qt3ds;
using namespace qt3ds::render;

NVRenderTestClear::NVRenderTestClear()
{
}

NVRenderTestClear::~NVRenderTestClear()
{
}

bool NVRenderTestClear::isSupported(NVRenderContext *context)
{
    return true;
}

////////////////////////////////
// test for functionality
////////////////////////////////

static bool checkColor(int width, int height, unsigned char *pixels, const QT3DSVec3 &color)
{
    unsigned char *pSrc = pixels;

    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            if (pSrc[0] != (unsigned char)color.x || pSrc[1] != (unsigned char)color.y
                || pSrc[2] != (unsigned char)color.z) {
                return false;
            }

            pSrc += 3;
        }
    }

    return true;
}

bool NVRenderTestClear::run(NVRenderContext *context, userContextData *pUserData)
{
    bool success = true;

    success &= testColorClear(context, pUserData);

    // if successful draw green otherwise a red
    if (success) {
        // set clear color to green
        context->SetClearColor(QT3DSVec4(0.0f, 1.0f, .0f, 1.f));
        context->Clear(NVRenderClearFlags(NVRenderClearValues::Color));
    } else {
        // set clear color to green
        context->SetClearColor(QT3DSVec4(1.0f, .0f, .0f, 1.f));
        context->Clear(NVRenderClearFlags(NVRenderClearValues::Color));
    }

    return success;
}

bool NVRenderTestClear::testColorClear(NVRenderContext *context, userContextData *pUserData)
{
    // allocate buffer for readback
    NVAllocatorCallback &alloc(context->GetFoundation().getAllocator());
    QT3DSU32 size = pUserData->winHeight * pUserData->winHeight * 3 * sizeof(QT3DSU8);
    QT3DSU8 *pixels = (QT3DSU8 *)alloc.allocate(size, "testColorClear color clear", __FILE__, __LINE__);

    if (!pixels)
        return false;

    // set clear color to yellow
    context->SetClearColor(QT3DSVec4(1.0f, 1.0f, .0f, 0.f));
    context->Clear(NVRenderClearFlags(NVRenderClearValues::Color));

    // read back pixels
    context->ReadPixels(NVRenderRect(0, 0, pUserData->winHeight, pUserData->winHeight),
                        NVRenderReadPixelFormats::RGB8, NVDataRef<QT3DSU8>(pixels, size));
    // check color
    bool passed =
        checkColor(pUserData->winHeight, pUserData->winHeight, pixels, QT3DSVec3(255.0f, 255.0f, .0f));

    alloc.deallocate(pixels);

    return passed;
}

////////////////////////////////
// performance test
////////////////////////////////
bool NVRenderTestClear::runPerformance(NVRenderContext *context, userContextData *pUserData)
{
    return true;
}

////////////////////////////////
// test cleanup
////////////////////////////////
void NVRenderTestClear::cleanup(NVRenderContext *context, userContextData *pUserData)
{
    context->SetClearColor(QT3DSVec4(0.0f, .0f, .0f, 0.f));
}
