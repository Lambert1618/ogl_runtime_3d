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

#ifndef MIPMAPBSDF_H
#define MIPMAPBSDF_H
#include "foundation/Qt3DSVec3.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/Qt3DSPerfTimer.h"
#include "foundation/Qt3DSAtomic.h"
#include "render/Qt3DSRenderBaseTypes.h"
#include "render/backends/Qt3DSRenderBackend.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "render/backends/gl/Qt3DSOpenGLUtil.h"

#include "Qt3DSRenderLoadedTexture.h"

#include "Qt3DSTypes.h"
#ifdef _LINUXPLATFORM
#define __declspec(dllexport)
#define __cdecl

#endif
using namespace qt3ds::render;

class BSDFMipMap : public NVRefCounted
{
public:
    BSDFMipMap(NVRenderContext *inNVRenderContext, int inWidth, int inHeight,
               NVRenderTexture2D &inTexture, NVRenderTextureFormats::Enum inDestFormat,
               qt3ds::NVFoundationBase &inFnd);
    virtual ~BSDFMipMap();

    virtual void Build(void *inTextureData, int inTextureDataSize,
                       NVRenderBackend::NVRenderBackendTextureObject inTextureHandle,
                       NVRenderTextureFormats::Enum inFormat) = 0;
    static BSDFMipMap *Create(NVRenderContext *inNVRenderContext, int inWidth, int inHeight,
                              NVRenderTexture2D &inTexture,
                              NVRenderTextureFormats::Enum inDestFormat,
                              qt3ds::NVFoundationBase &inFnd);

protected:
    volatile QT3DSI32 mRefCount; ///< reference count
    NVFoundationBase &m_Foundation; ///< Foundation class for allocations and other base things

    NVRenderTexture2D &m_Texture2D;
    NVRenderTextureFormats::Enum m_InternalFormat;
    NVRenderTextureFormats::Enum m_DestinationFormat;
    int m_Width;
    int m_Height;
    int m_MaxMipMapLevel;
    int m_SizeOfFormat;
    int m_SizeOfInternalFormat;
    int m_InternalNoOfComponent;
    int m_NoOfComponent;
    NVRenderContext *m_NVRenderContext;
};

class BasicBSDFMipMap : public BSDFMipMap
{
public:
    BasicBSDFMipMap(NVRenderContext *inNVRenderContext, int inWidth, int inHeight,
                    NVRenderTexture2D &inTexture, NVRenderTextureFormats::Enum inDestFormat,
                    qt3ds::NVFoundationBase &inFnd);

    void Build(void *inTextureData, int inTextureDataSize,
               NVRenderBackend::NVRenderBackendTextureObject inTextureHandle,
               NVRenderTextureFormats::Enum inFormat);

    STextureData CreateBsdfMipLevel(STextureData &inCurMipLevel, STextureData &inPrevMipLevel,
                                    int width, int height); //, IPerfTimer& inPerfTimer );
    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation);

    int wrapMod(int a, int base);
    void getWrappedCoords(int &sX, int &sY, int width, int height);
    NVRenderBackend::NVRenderBackendTextureObject m_TextureHandle;
};

#endif
