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

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Express a color and transparency using 32 bits.
 */
class CColor
{
    //==============================================================================
    //	Fields
    //==============================================================================
public:
    union {
        UINT32 m_Color; ///< 32bit representation of the color
        struct
        {
            UINT8 m_Red; ///< Red component 0-255
            UINT8 m_Green; ///< Green component 0-255
            UINT8 m_Blue; ///< Blue component 0-255
            UINT8 m_Alpha; ///< Transparency component 0-255
        };
    };

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CColor(const UINT32 inColor);
    CColor(const UINT8 inRed = 255, const UINT8 inGreen = 255, const UINT8 inBlue = 255,
           const UINT8 inAlpha = 255);

public: // Setters
    void SetColor(const UINT32 inColor) { m_Color = inColor; }
    void SetRed(const UINT8 inRed) { m_Red = inRed; }
    void SetGreen(const UINT8 inGreen) { m_Green = inGreen; }
    void SetBlue(const UINT8 inBlue) { m_Blue = inBlue; }
    void SetAlpha(const UINT8 inAlpha) { m_Alpha = inAlpha; }

public: // Getters
    UINT32 GetColor() const { return m_Color; }
    UINT8 GetRed() const { return m_Red; }
    UINT8 GetGreen() const { return m_Green; }
    UINT8 GetBlue() const { return m_Blue; }
    UINT8 GetAlpha() const { return m_Alpha; }

public: // Utilities
    void Interpolate(const CColor &inColor1, const CColor &inColor2, const FLOAT inFactor);
};

} // namespace Q3DStudio
