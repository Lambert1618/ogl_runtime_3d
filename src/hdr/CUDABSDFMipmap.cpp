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

#ifdef PLATFORM_HAS_CUDA

#include "CUDABSDFMipmap.h"
#include "cuda.h"
#include "cuda_runtime.h"
#include "cuda_gl_interop.h"
#include "render/backends/Qt3DSRenderBackend.h"
#include "render/Qt3DSRenderTexture2D.h"
#include "foundation/Qt3DSRefCounted.h"
#include "nv_log.h"

using namespace qt3ds;
using namespace qt3ds::render;
using namespace qt3ds::foundation;

__host__ void jerror1(cudaError error);
#ifdef _DEBUG
#define CHECK_AND_HANDLE_CUDA_ERROR(func)                                                          \
    func;                                                                                          \
    {                                                                                              \
        cudaError error = cudaGetLastError();                                                      \
        if (error != cudaSuccess) {                                                                \
            printf("%s\n", cudaGetErrorString(error));                                             \
            jerror1(error);                                                                        \
            QT3DS_ASSERT(false);                                                                      \
        }                                                                                          \
    }
#else
#define CHECK_AND_HANDLE_CUDA_ERROR(func) func;
#endif

CUDABSDFMipMap::CUDABSDFMipMap(NVRenderContext *inNVRenderContext, int inWidth, int inHeight,
                               NVRenderTexture2D &inTexture2D,
                               NVRenderTextureFormats::Enum inDestFormat, NVFoundationBase &inFnd)
    : BSDFMipMap(inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat, inFnd)
    , m_TextureBinded(false)
{

    // CHECK_AND_HANDLE_CUDA_ERROR( cudaFree( 0 ); )
    m_Pitches = (size_t *)QT3DS_ALLOC(m_Foundation.getAllocator(),
                                   sizeof(size_t) * m_MaxMipMapLevel + 1, "BSDF MipMap pitches");
    md_MipMapsData = (void **)QT3DS_ALLOC(m_Foundation.getAllocator(),
                                       sizeof(void *) * m_MaxMipMapLevel + 1, "BSDF MipMap data");
    // CHECK_AND_HANDLE_CUDA_ERROR();
    size_t imagePitch;
    int width = m_Width;
    int height = m_Height;

    for (int i = 0; i <= m_MaxMipMapLevel; ++i) {
        imagePitch = m_SizeOfFormat * width;
        // checkCudaErrors(cudaMalloc((void **)&cuda_dest_resource[mip], size_tex_data));
        CHECK_AND_HANDLE_CUDA_ERROR(
            cudaMallocPitch((void **)&md_MipMapsData[i], &m_Pitches[i], imagePitch, height);)
        CHECK_AND_HANDLE_CUDA_ERROR(cudaMemset(md_MipMapsData[i], -1, m_Pitches[i] * height);)

        width = width > 2 ? width >> 1 : 1;
        height = height > 2 ? height >> 1 : 1;
    }
}

CUDABSDFMipMap::~CUDABSDFMipMap()
{
    //	CHECK_AND_HANDLE_CUDA_ERROR( cudaDeviceReset(); )
    CHECK_AND_HANDLE_CUDA_ERROR(cudaDeviceSynchronize();)
    for (int i = 0; i <= m_MaxMipMapLevel; ++i) {
        CHECK_AND_HANDLE_CUDA_ERROR(cudaFree(md_MipMapsData[i]);)
    }
    QT3DS_FREE(m_Foundation.getAllocator(), md_MipMapsData);
    QT3DS_FREE(m_Foundation.getAllocator(), m_Pitches);
}

void CUDABSDFMipMap::BindTexture()
{
    if (!m_TextureBinded) {
        m_TextureBinded = true;

        int width = m_Width;
        int height = m_Height;
        for (int i = 0; i <= m_MaxMipMapLevel; ++i) {
            // if you wwant to change some texture filter settings use m_Texture2D object
            m_Texture2D.SetTextureData(NVDataRef<QT3DSU8>(), (QT3DSU8)i, width, height,
                                       NVRenderTextureFormats::RGBA16F,
                                       NVRenderTextureFormats::RGBA16F);

            width = width > 2 ? width >> 1 : 1;
            height = height > 2 ? height >> 1 : 1;
        }
        // CHECK_AND_HANDLE_CUDA_ERROR( cudaGraphicsGLRegisterImage( &m_CudaMipMapResource,
        // (GLuint)m_TextureHandle, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard |
        // cudaGraphicsRegisterFlagsTextureGather) )
        CHECK_AND_HANDLE_CUDA_ERROR(cudaGraphicsGLRegisterImage(
            &m_CudaMipMapResource, (GLuint)m_TextureHandle, GL_TEXTURE_2D,
            cudaGraphicsRegisterFlagsWriteDiscard | cudaGraphicsRegisterFlagsTextureGather))
    }
}

void CUDABSDFMipMap::TransferTexture() // after cuda generation
{
    cudaArray *texturePtr;
    CHECK_AND_HANDLE_CUDA_ERROR(cudaGraphicsMapResources(1, &m_CudaMipMapResource, 0))
    int width = m_Width;
    int height = m_Height;
    for (int i = 0; i <= m_MaxMipMapLevel; ++i) {
        CHECK_AND_HANDLE_CUDA_ERROR(
            cudaGraphicsSubResourceGetMappedArray(&texturePtr, m_CudaMipMapResource, 0, i))
        CHECK_AND_HANDLE_CUDA_ERROR(cudaMemcpy2DToArray(texturePtr, 0, 0, md_MipMapsData[i],
                                                        m_Pitches[i], width * m_SizeOfFormat,
                                                        height, cudaMemcpyDeviceToDevice));

        width = width > 2 ? width >> 1 : 1;
        height = height > 2 ? height >> 1 : 1;
    }
    CHECK_AND_HANDLE_CUDA_ERROR(cudaGraphicsUnmapResources(1, &m_CudaMipMapResource, 0))
}

#endif
