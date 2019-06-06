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

#if defined (_PLATFORM_USE_EGL)
#include <GLES31/gl31.h>
#include <GLES31/gl2ext.h>
#endif

#include "CUDABSDFMipmap.h"
#include "cuda.h"
#include "cuda_runtime.h"
#include "cuda_gl_interop.h"
#include <iostream>

using namespace nv;
using namespace nv::render;
__host__ void jerror1(cudaError error)
{
   static  int i = 0;
   ++i;
}
#ifdef _DEBUG
#define CHECK_AND_HANDLE_CUDA_ERROR(func)													\
		func;																			\
		{																				\
			cudaError error = cudaGetLastError();												\
			if ( error != cudaSuccess )													\
			{																			\
				printf("%s\n", cudaGetErrorString(error));								\
                jerror1(error);\
				NV_ASSERT( false );														\
			}																			\
		}
#else
#define CHECK_AND_HANDLE_CUDA_ERROR(func)													\
func;
#endif

__device__ inline int wrapMod( int a, int base )
{
	int ret = a % base;
	if (ret < 0 ) ret += base;
	return ret;
}

__device__ inline void getWrappedCoords( int &sX, int &sY, int width, int height )
{
	if (sY < 0) { sX -= width >> 1; sY = -sY; }
	if (sY >= height) { sX += width >> 1; sY = height - sY; }
	sX = wrapMod( sX, width );
	sY = wrapMod( sY, height );
}

__device__ void decodeToFloat( void *inPtr, NVU32 byteOfs, float *outPtr, NVRenderTextureFormats::Enum inFmt, unsigned int numberOfComponent )
{
	outPtr[0] = 0.0f;	outPtr[1] = 0.0f;	outPtr[2] = 0.0f;	outPtr[3] = 0.0f;
	NVU8 *src = reinterpret_cast<NVU8 *>(inPtr);
	//float divisor;		// If we want to support RGBD?
	switch(inFmt)
	{
	case NVRenderTextureFormats::Alpha8:
		outPtr[0] = ((float)src[byteOfs]) / 255.0f;
		break;

	case NVRenderTextureFormats::Luminance8:
	case NVRenderTextureFormats::LuminanceAlpha8:
	case NVRenderTextureFormats::R8:
	case NVRenderTextureFormats::RG8:
	case NVRenderTextureFormats::RGB8:
	case NVRenderTextureFormats::RGBA8:
	case NVRenderTextureFormats::SRGB8:
	case NVRenderTextureFormats::SRGB8A8:
		// NOTE : RGBD Hack here for reference.  Not meant for installation.
		//divisor = (NVRenderTextureFormats::getSizeofFormat(inFmt) == 4) ? ((float)src[byteOfs+3]) / 255.0f : 1.0f;
		for ( NVU32 i = 0; i < numberOfComponent; ++i )
		{
			float val = ((float)src[byteOfs + i]) / 255.0f;
			outPtr[i] = (i < 3) ? powf(val, 0.4545454545f) : val;
			// Assuming RGBA8 actually means RGBD (which is stupid, I know)
			//if ( NVRenderTextureFormats::getSizeofFormat(inFmt) == 4 ) { outPtr[i] /= divisor; }
		}
		//outPtr[3] = divisor;
		break;

	case NVRenderTextureFormats::RGBA32F:
		outPtr[0] = reinterpret_cast<float *>(src+byteOfs)[0];
		outPtr[1] = reinterpret_cast<float *>(src+byteOfs)[1];
		outPtr[2] = reinterpret_cast<float *>(src+byteOfs)[2];
		outPtr[3] = reinterpret_cast<float *>(src+byteOfs)[3];
		break;
	case NVRenderTextureFormats::RGB32F:
		outPtr[0] = reinterpret_cast<float *>(src+byteOfs)[0];
		outPtr[1] = reinterpret_cast<float *>(src+byteOfs)[1];
		outPtr[2] = reinterpret_cast<float *>(src+byteOfs)[2];
		break;

	case NVRenderTextureFormats::RGBA16F:
        /*
		for ( NVU32 i = 0; i < 4; ++i )
		{
			// NOTE : This only works on the assumption that we don't have any denormals, Infs or NaNs.
			// Every pixel in our source image should be "regular"
			NVU16 h = reinterpret_cast<NVU16 *>(src + byteOfs)[i];
			NVU32 sign = (h & 0x8000) << 16;
			NVU32 exponent = (((((h & 0x7c00) >> 10) - 15) + 127) << 23);
			NVU32 mantissa =  ((h & 0x3ff) << 13);
			NVU32 result = sign | exponent | mantissa;
					
			if (h == 0 || h == 0x8000) { result = 0; }	// Special case for zero and negative zero
			memcpy( reinterpret_cast<NVU32 *>(outPtr) + i, &result, 4 );
		}*/

		for ( NVU32 i = 0; i < 2; i++ )
		{
			// NOTE : This only works on the assumption that we don't have any denormals, Infs or NaNs.
			// Every pixel in our source image should be "regular"

			NVU32 h1 = reinterpret_cast<NVU32 *>(src + byteOfs)[i];

			for ( NVU8 j = 0; j < 2; j++ ) 
			{
				NVU16 h = (h1 & (0x0000FFFF << j*16 )) >> j*16;
				NVU32 sign = (h & 0x8000) << 16;
				NVU32 exponent = (((((h & 0x7c00) >> 10) - 15) + 127) << 23);
				NVU32 mantissa =  ((h & 0x3ff) << 13);
				NVU32 result = sign | exponent | mantissa;
					
				if (h == 0 || h == 0x8000) { result = 0; }	// Special case for zero and negative zero
				memcpy( reinterpret_cast<NVU32 *>(outPtr) + i*2 + j, &result, 4 );
			}
		}
		break;

	case NVRenderTextureFormats::R11G11B10:
		// place holder
		NV_ASSERT( false );
		break;

	default:
		outPtr[0] = 0.0f;
		outPtr[1] = 0.0f;
		outPtr[2] = 0.0f;
		outPtr[3] = 0.0f;
		break;
	}
}

void __device__ encodeToPixel( float *inPtr, void *outPtr, NVU32 byteOfs, NVRenderTextureFormats::Enum inFmt, unsigned int noOfComponent )
{
	NVU8 *dest = reinterpret_cast<NVU8 *>(outPtr);
	switch(inFmt)
	{
	case NVRenderTextureFormats::Alpha8:
		dest[byteOfs] = NVU8( inPtr[0] * 255.0f );
		break;

	case NVRenderTextureFormats::Luminance8:
	case NVRenderTextureFormats::LuminanceAlpha8:
	case NVRenderTextureFormats::R8:
	case NVRenderTextureFormats::RG8:
	case NVRenderTextureFormats::RGB8:
	case NVRenderTextureFormats::RGBA8:
	case NVRenderTextureFormats::SRGB8:
	case NVRenderTextureFormats::SRGB8A8:
		for ( NVU32 i = 0; i < noOfComponent; ++i )
		{
			inPtr[i] = (inPtr[i] > 1.0f) ? 1.0f : inPtr[i];
			if (i < 3)
				dest[byteOfs+i] = NVU8( powf( inPtr[i], 2.2f ) * 255.0f);
			else
				dest[byteOfs+i] = NVU8( inPtr[i] * 255.0f );
		}
		break;

	case NVRenderTextureFormats::RGBA32F:
		reinterpret_cast<float *>(dest+byteOfs)[0] = inPtr[0];
		reinterpret_cast<float *>(dest+byteOfs)[1] = inPtr[1];
		reinterpret_cast<float *>(dest+byteOfs)[2] = inPtr[2];
		reinterpret_cast<float *>(dest+byteOfs)[3] = inPtr[3];
		break;
	case NVRenderTextureFormats::RGB32F:
		reinterpret_cast<float *>(dest+byteOfs)[0] = inPtr[0];
		reinterpret_cast<float *>(dest+byteOfs)[1] = inPtr[1];
		reinterpret_cast<float *>(dest+byteOfs)[2] = inPtr[2];
		break;

	case NVRenderTextureFormats::RGBA16F:
		for ( NVU32 i = 0; i < 4; ++i )
		{
			// NOTE : This also has the limitation of not handling  infs, NaNs and denormals, but it should be
			// sufficient for our purposes.
			if (inPtr[i] > 65519.0f) { inPtr[i] = 65519.0f; }
			if (fabs(inPtr[i]) < 6.10352E-5f) { inPtr[i] = 0.0f; }
			NVU32 f = reinterpret_cast<NVU32 *>(inPtr)[i];
			NVU32 sign = (f & 0x80000000) >> 16;
			NVI32 exponent = (f & 0x7f800000) >> 23;
			NVU32 mantissa = (f >> 13) & 0x3ff;
			exponent = exponent - 112;
			if (exponent > 31) { exponent = 31; }
			if (exponent < 0) { exponent = 0; }
			exponent = exponent << 10;
			reinterpret_cast<NVU16 *>(dest + byteOfs)[i] = NVU16(sign | exponent | mantissa);
		}
		break;

	case NVRenderTextureFormats::R11G11B10:
		// place holder
		NV_ASSERT( false );
		break;

	default:
		dest[byteOfs] = 0;
		dest[byteOfs+1] = 0;
		dest[byteOfs+2] = 0;
		dest[byteOfs+3] = 0;
		break;
	}
}

void __global__ Convert3To4Component( cudaTextureObject_t tex, float *d_outBuffer, Q3DStudio::INT32 dpitch, Q3DStudio::INT32 width, Q3DStudio::INT32 height )
{
	float *dest = d_outBuffer;

	int x = threadIdx.x + blockIdx.x * blockDim.x;
	int y = threadIdx.y + blockIdx.y * blockDim.y;
	if ( x >= width || y >= height ) 
		return;
	int inX = x * 3;
	int outX = x * 4;
	dest[outX + y * width * 4] = tex2D<float>(tex, inX, y);
	dest[outX + y * width * 4 + 1] = tex2D<float>(tex, inX + 1, y);
	dest[outX + y * width * 4 + 2] = tex2D<float>(tex, inX + 2, y);
	dest[outX + y * width * 4 + 3] = 255 * 255;
}

void __global__ ConvertData( void* d_InBuffer, NVRenderTextureFormats::Enum inFmt, int inSizeOfFormat, int inNoOfComponent, int inPitch, 
                             void* d_OutBuffer, NVRenderTextureFormats::Enum outFmt, int outSizeOfFormat, int outNoOfComponent, int outPitch, int width, int height )
{

	int x = threadIdx.x + blockIdx.x * blockDim.x;
	int y = threadIdx.y + blockIdx.y * blockDim.y;
	if ( x >= width || y >= height ) 
		return;
	float values[4];

	decodeToFloat( d_InBuffer, (inPitch * y) + (x * inSizeOfFormat), values, inFmt, inNoOfComponent );
	encodeToPixel( values, d_OutBuffer, (outPitch * y) + (x * outSizeOfFormat), outFmt, outSizeOfFormat );
}

void __global__ CreateBsdfMipLevel( cudaTextureObject_t tex, void *d_curBuffer, void *d_prevBuffer, 	Q3DStudio::INT32 pitch, Q3DStudio::INT32 width, Q3DStudio::INT32 height, 
										nv::render::NVRenderTextureFormats::Enum inFormat, unsigned int sizeOfFormat )
{
	float accumVal[4];
	//unsigned int sizeofFormat = getSizeofFormat(inFormat);
	//__shared__ float dataBlock[ ]; //(32+4) * (32+4) * 12 
	int x = threadIdx.x + blockIdx.x * blockDim.x;
	int y = threadIdx.y + blockIdx.y * blockDim.y;

	if ( x >= (width > 2 ? width >> 1 : 1) || y >= (height > 2 ? height >> 1 : 1)) return;

	accumVal[0] = 0;	accumVal[1] = 0;	accumVal[2] = 0;	accumVal[3] = 0;

	for ( int sy = -2; sy <= 2; ++sy )
	{
		for ( int sx = -2; sx <= 2; ++sx )
		{
			int sampleX = sx + (x << 1);
			int sampleY = sy + (y << 1);
			//getWrappedCoords(sampleX, sampleY, width, height);
			// Cauchy filter (this is simply because it's the easiest to evaluate, and requires no complex
			// functions).
			float filterPdf = 1.f / ( 1.f + float(sx*sx + sy*sy)*2.f );
			// With FP HDR formats, we're not worried about intensity loss so much as unnecessary energy gain, 
			// whereas with LDR formats, the fear with a continuous normalization factor is that we'd lose
			// intensity and saturation as well.
			filterPdf /= sizeOfFormat >= 8 ? 4.71238898f : 4.5403446f;
			//filterPdf /= 4.5403446f;		// Discrete normalization factor
			//filterPdf /= 4.71238898f;		// Continuous normalization factor
			//float curPix[4];
			sampleX = sampleX*4;
			getWrappedCoords(sampleX, sampleY, width*4, height);
			accumVal[0] += filterPdf * tex2D<float>(tex, sampleX, sampleY);
			accumVal[1] += filterPdf * tex2D<float>(tex, sampleX + 1, sampleY);
			accumVal[2] += filterPdf * tex2D<float>(tex, sampleX + 2, sampleY);
			accumVal[3] += filterPdf * tex2D<float>(tex, sampleX + 3, sampleY);
		}
		}

	encodeToPixel(accumVal, d_curBuffer, y * pitch + x * sizeOfFormat, inFormat, sizeOfFormat);
}

struct SMipTextureData
{
	void* data;
	unsigned int dataSize;
	unsigned int mipLevel;
	unsigned int width;
	unsigned int height;
	NVRenderTextureFormats::Enum format;	
};

__host__  void CUDABSDFMipMap::Build( void* inTextureData, int inTextureDataSize, NVRenderBackend::NVRenderBackendTextureObject inTextureHandle, NVRenderTextureFormats::Enum inFormat  )
{
	m_TextureHandle = inTextureHandle;
	m_InternalFormat = inFormat;
	m_SizeOfInternalFormat = NVRenderTextureFormats::getSizeofFormat( m_InternalFormat );
	m_InternalNoOfComponent = NVRenderTextureFormats::getNumberOfComponent( m_InternalFormat );

	m_Texture2D.SetTextureData(  NVDataRef<NVU8>( (NVU8*)inTextureData, inTextureDataSize )
								, 0
								, m_Width
								, m_Height
								, inFormat
								, m_DestinationFormat );

	size_t pitch;
	float* d_inTextureData;

	cudaMallocPitch(&d_inTextureData, &pitch, m_Width * m_SizeOfInternalFormat, m_Height); CHECK_AND_HANDLE_CUDA_ERROR();
	CHECK_AND_HANDLE_CUDA_ERROR( cudaMemcpy2D( d_inTextureData, pitch, inTextureData, m_Width * m_SizeOfInternalFormat, m_Width * m_SizeOfInternalFormat, m_Height, cudaMemcpyHostToDevice ) );
	{
		dim3 blockDim(16, 16, 1);	
		dim3 gridDim(ceil(m_Width / 16.0f), ceil(m_Height / 16.0f) ,1 );

		//std::cerr << "if= " << m_InternalFormat << " sizeOut= " << m_SizeOfInternalFormat << " numOfIntComp" << m_InternalNoOfComponent << " pitch= " << pitch << " destFormat= " << m_DestinationFormat << " sizeFormat= " << m_SizeOfFormat << " numOfComp= " << m_NoOfComponent << " Pitch0=" << m_Pitches[0] << std::endl;
		//NVLogWarn("cuda", "%i %i %i %i %i %i %i %i\n",(int)m_InternalFormat ,m_SizeOfInternalFormat ,m_InternalNoOfComponent , pitch, (int)m_DestinationFormat, m_SizeOfFormat, m_NoOfComponent ,m_Pitches[0]);
        	ConvertData<<<gridDim, blockDim>>>( d_inTextureData, m_InternalFormat, m_SizeOfInternalFormat, m_InternalNoOfComponent, pitch,
                        md_MipMapsData[0], m_DestinationFormat, m_SizeOfFormat, m_NoOfComponent, m_Pitches[0], m_Width, m_Height );
	}
	cudaFree(d_inTextureData);

	int curWidth  = m_Width;
	int curHeight = m_Height;

	cudaTextureObject_t* tex;
	tex = new cudaTextureObject_t[m_MaxMipMapLevel];
	for ( int idx = 1; idx <= m_MaxMipMapLevel; ++idx )
	{
		tex[idx-1] = -1;
		dim3 blockDim(16, 16, 1);	
		dim3 gridDim(ceil(curWidth / 32.0f), ceil(curHeight / 32.0f) ,1 );

		cudaResourceDesc resDesc;
		memset(&resDesc, 0, sizeof(resDesc));
		resDesc.res.pitch2D.desc.f = cudaChannelFormatKindFloat;
		resDesc.res.pitch2D.desc.x = m_SizeOfFormat / m_NoOfComponent * 8; // bits per channel
		resDesc.resType = cudaResourceTypePitch2D;
		resDesc.res.pitch2D.devPtr = (char*)(md_MipMapsData[idx-1]);
		resDesc.res.pitch2D.height = curHeight;
		resDesc.res.pitch2D.width  = curWidth * m_NoOfComponent;
		resDesc.res.pitch2D.pitchInBytes  = m_Pitches[idx-1];// aligned to texturePitchAlignment

		cudaTextureDesc texDesc;
		memset(&texDesc, 0, sizeof(texDesc));
		texDesc.addressMode[0] = cudaAddressModeWrap;
		texDesc.addressMode[1] = cudaAddressModeWrap;
		texDesc.readMode = cudaReadModeElementType;
		//texDesc.normalizedCoords = 1;

		
		CHECK_AND_HANDLE_CUDA_ERROR( cudaCreateTextureObject( &tex[idx-1], &resDesc, &texDesc, NULL ) );
		CreateBsdfMipLevel<<<gridDim, blockDim>>>( tex[idx-1], (reinterpret_cast<NVU8 *>(md_MipMapsData[idx])), (reinterpret_cast<NVU8 *>(md_MipMapsData[idx-1])), m_Pitches[idx], curWidth, curHeight, m_DestinationFormat, m_SizeOfFormat );

		curWidth  = curWidth > 2 ? curWidth >> 1 : 1;
		curHeight = curHeight > 2 ? curHeight >> 1 : 1;
	}

	CHECK_AND_HANDLE_CUDA_ERROR( cudaDeviceSynchronize(); )
	BindTexture();
	TransferTexture();
	for (int idx = 0; idx < m_MaxMipMapLevel;++idx )
		cudaDestroyTextureObject(tex[idx]);
//	CHECK_AND_HANDLE_CUDA_ERROR( cudaDeviceReset(); )
	CHECK_AND_HANDLE_CUDA_ERROR( cudaDeviceSynchronize(); )

    //NV_FREE( m_Foundation.getAllocator(), inTextureData );

}
