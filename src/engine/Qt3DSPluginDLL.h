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
//==============================================================================
//	Includes
//==============================================================================
#pragma once
#include "Qt3DSBasicPluginDLL.h"

#include <QtGlobal>

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// Enums
//==============================================================================
// Texture format
typedef enum _ETEXTUREFORMAT {
    ETEXTUREFORMAT_Unknown = 0,
    ETEXTUREFORMAT_RGBA8,
    ETEXTUREFORMAT_RGB8,
    ETEXTUREFORMAT_RGB565,
    ETEXTUREFORMAT_RGBA5551,
    ETEXTUREFORMAT_Alpha8,
    ETEXTUREFORMAT_Luminance8,
    ETEXTUREFORMAT_LuminanceAlpha8,
    ETEXTUREFORMAT_RGBA_DXT1,
    ETEXTUREFORMAT_RGB_DXT1,
    ETEXTUREFORMAT_RGBA_DXT3,
    ETEXTUREFORMAT_RGBA_DXT5,
} ETEXTUREFORMAT;

//==============================================================================
// Functions declarations
//==============================================================================

//==============================================================================
/**
 *	Perform plugin initialization.
 *	@param	inArgs	string arguments from XIF arg parameter
 *	@return	EDLLSTATUS_OK if initialization suceeds
 */
typedef long (*PROC_Initialize)(const char *inArgs);
Q_DECL_EXPORT long Initialize(const char *inArgs);

//==============================================================================
/**
 *	Passes the current screen size and format to the plugin.
 *	Plugin should override with it's own desired settings.
 *	This affects the size and format of the allocated offscreen texture.
 *  When resources are allocated, SetAllocatedRenderInfo will be called to
 *	inform the plugin of the allocated resources
 *	@param	ioWidth				width
 *	@param	ioHeight			height
 *	@param  ioTextureFormat		See ETEXTUREFORMAT
 */
typedef void (*PROC_GetDesiredTextureSize)(long *ioWidth, long *ioHeight,
                                           ETEXTUREFORMAT *ioTextureFormat);
Q_DECL_EXPORT void GetDesiredTextureSize(long *ioWidth, long *ioHeight,
                                                         ETEXTUREFORMAT *ioTextureFormat);

//==============================================================================
/**
 *	Information about the current EGL environment.
 *	Useful if plugin wishes to switch EGLContext to manage it's own state.
 *	Optional
 *	@param inEGLDisplay			pointer to EGLDisplay
 *	@param inEGLCurrentContext	pointer to current EGLContext
 *	@param inEGLSurface			pointer to EGLSurface
 *	@param inEGLConfig			pointer to EGLConfig attributes
 */
typedef void (*PROC_SetEGLInfo)(void *inEGLDisplay, void *inEGLCurrentContext, void *inEGLSurface,
                                void *inEGLConfig);
Q_DECL_EXPORT void SetEGLInfo(void *inEGLDisplay, void *inEGLCurrentContext,
                                              void *inEGLSurface, void *inEGLConfig);

//==============================================================================
/**
 *	Render pulse whenever the runtime requires a frame from the plugin.
 *	Note that if plugin wishes to switch EGLContext,
 *	it should restore the previous EGLContext(obtained from SetEGLInfo) at the end of this
 *function.
 *	@param	inHostWidth		width of the host rectangle
 *	@param	inHostHeight	height of the host rectangle
 *	@param	inDrawTime		current time in milliseconds
 */
typedef void (*PROC_Render)(long inHostWidth, long inHostHeight, long inDrawTime);
Q_DECL_EXPORT void Render(long inHostWidth, long inHostHeight, long inDrawTime);

//==============================================================================
/**
 *	Perform plugin uninitialization.
 *	@return EDLLSTATUS_OK if successful
 */
typedef long (*PROC_Uninitialize)();
Q_DECL_EXPORT long Uninitialize();

#ifdef __cplusplus
}
#endif
