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

#include "render/Qt3DSRenderBaseTypes.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSBounds3.h"
#include "Qt3DSBoundingBox.h"
namespace qt3ds {
namespace render {
    class IImageLoadListener;
}
    class NVAllocatorCallback;
}

namespace Q3DStudio {

class IPresentation;
class RuntimeMatrix;
struct SPickFrame;

/**
*	@interface	IScene
*
*	Runtime interface to the renderer's representation of a presentation.  Scenes are
*	created by the SceneManager and rendered via a lower level rendering system.
*/
struct STextSizes
{
    INT32 m_Width;
    INT32 m_Height;
    STextSizes()
        : m_Width(0)
        , m_Height(0)
    {
    }
    STextSizes(INT32 w, INT32 h)
        : m_Width(w)
        , m_Height(h)
    {
    }
};

struct SMousePosition
{
    INT32 m_X;
    INT32 m_Y;
    SMousePosition(INT32 x, INT32 y)
        : m_X(x)
        , m_Y(y)
    {
    }
    SMousePosition()
        : m_X(0)
        , m_Y(0)
    {
    }
};

struct SCameraRect
{
    float m_Left;
    float m_Top;
    float m_Right;
    float m_Bottom;
    SCameraRect(float l = 0.0f, float t = 0.0f, float r = 0.0f, float b = 0.0f)
        : m_Left(l)
        , m_Top(t)
        , m_Right(r)
        , m_Bottom(b)
    {
    }

    bool IsValid() const { return fabs(m_Right - m_Left) > 0.0f; }
};

class IScene
{
protected:
    virtual ~IScene() {}

public: // Base Interface
    virtual IPresentation &GetPresentation() = 0;

    virtual void SetUserData(void *inUserData) = 0;
    virtual void *GetUserData() = 0;

    virtual void CalculateGlobalTransform(TElement *inElement, RuntimeMatrix &outTransform) = 0;
    virtual void SetLocalTransformMatrix(TElement *inElement, const RuntimeMatrix &inTransform) = 0;
    // Get bounding box in global space
    virtual CBoundingBox GetBoundingBox(TElement *inElement, bool inSelfOnly) = 0;
    // Get bounding box in local space.
    virtual CBoundingBox GetLocalBoundingBox(TElement *inElement, bool inSelfOnly) = 0;

    // The final argument, inHasTransparency has 3 possible values,
    // 0 for no transparency, 1 for hasTransparency, -1 for unknown
    virtual void SetTextureData(TElement *inElement, const unsigned char *inBuffer,
                                INT32 inBufferLength, INT32 inWidth, INT32 inHeight,
                                qt3ds::render::NVRenderTextureFormats::Enum inFormat,
                                INT32 inHasTransparency = -1) = 0;

    virtual bool CreateOrSetMeshData(const char *inPathStr, unsigned char *vertData,
                                     unsigned int numVerts, unsigned int vertStride,
                                     unsigned int *indexData, unsigned int numIndices,
                                     qt3ds::NVBounds3 &objBounds) = 0;

    virtual STextSizes MeasureText(TElement *inElement, const char *inTextStr) = 0;

    virtual STextSizes GetPresentationDesignDimensions() = 0;
    // If the rect's right - left == 0.0, this method failed.  Possibly because the layer is just
    // direct-rendering a sub-presentation.
    virtual SCameraRect GetCameraBounds(TElement &inElement) = 0;

    virtual void PositionToScreen(TElement &inElement, qt3ds::QT3DSVec3 &inPos,
                                  qt3ds::QT3DSVec3 &outScreen) = 0;
    virtual void ScreenToPosition(TElement &inElement, qt3ds::QT3DSVec3 &inScreen,
                                  qt3ds::QT3DSVec3 &outPos) = 0;

    virtual qt3ds::foundation::CRegisteredString RegisterStr(const char *inStr) = 0;

    virtual SMousePosition WindowToPresentation(const SMousePosition &inWindowPos) = 0;

    virtual void RegisterOffscreenRenderer(const char *inKey) = 0;

    virtual void Release() = 0;

    virtual bool preferKtx() const = 0;

    virtual qt3ds::NVAllocatorCallback &allocator() = 0;
};

} // namespace Q3DStudio
