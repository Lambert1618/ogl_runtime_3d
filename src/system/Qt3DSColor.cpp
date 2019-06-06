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

#include "SystemPrefix.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSColor.h"

//==============================================================================
// Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Constructor
 *	@param inColor		32 bit represenation of the color
 */
CColor::CColor(const Q3DStudio::UINT32 inColor)
    : m_Color(inColor)
{
}

//==============================================================================
/**
 *	Constructor
 *	@param inRed		8 bit represenation of the red component
 *	@param inGreen		8 bit representation of the green component
 *	@param inBlue		8 bit representation of the blue component
 *	@param inAlpha		8 bit representation of the transparency component
 */
CColor::CColor(const UINT8 inRed, const UINT8 inGreen, const UINT8 inBlue, const UINT8 inAlpha)
    : m_Red(inRed)
    , m_Green(inGreen)
    , m_Blue(inBlue)
    , m_Alpha(inAlpha)
{
}

//==============================================================================
/**
 *	Set the color to a blend between the two given colors.
 *	@param inColor1		Color representing factor 0
 *	@param inColor2		Color representing factor 1.0
 *	@param inFactor		Blend factor where 0.5 is an even mix of the two colors
 */
void CColor::Interpolate(const CColor &inColor1, const CColor &inColor2, const FLOAT inFactor)
{
    //	m_Red =		static_cast<UINT8>( rand() % 256 );//( 1.0f - inFactor ) * inColor1.m_Red +
    //inFactor * inColor2.m_Red );
    //	m_Green =	static_cast<UINT8>( rand() % 256 );//( 1.0f - inFactor ) * inColor1.m_Green +
    //inFactor * inColor2.m_Green );
    //	m_Blue =	static_cast<UINT8>( rand() % 256 );//( 1.0f - inFactor ) * inColor1.m_Blue +
    //inFactor * inColor2.m_Blue );
    //	m_Alpha =	255;//static_cast<UINT8>( ( 1.0f - inFactor ) * inColor1.m_Alpha +	inFactor *
    //inColor2.m_Alpha );

    m_Red = static_cast<UINT8>((1.0f - inFactor) * inColor1.m_Red + inFactor * inColor2.m_Red);
    m_Green =
        static_cast<UINT8>((1.0f - inFactor) * inColor1.m_Green + inFactor * inColor2.m_Green);
    m_Blue = static_cast<UINT8>((1.0f - inFactor) * inColor1.m_Blue + inFactor * inColor2.m_Blue);
    m_Alpha =
        static_cast<UINT8>((1.0f - inFactor) * inColor1.m_Alpha + inFactor * inColor2.m_Alpha);
}

} // namespace Q3DStudio
