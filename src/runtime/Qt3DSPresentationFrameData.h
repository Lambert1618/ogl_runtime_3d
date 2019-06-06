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
 *	Presentation update cycle data storage class.
 *
 *	This is the data collected from the Presentation on each frame. Other subsystems
 *	or external system uses these data to perform their specific operation
 */
class CPresentationFrameData
{
    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    TElementList m_DirtyList; ///< List of elements with modified attributes
    TElementList m_TraversalList; ///< List of active elements
    TElementList m_ScriptsList; ///< List of elements where scripts are active
    TElementList
        m_ActivationList; ///< List of elements whose global active flips from false to true
    TElementList
        m_DeactivationList; ///< List of elements whose global active flops from true to false
    // TElementList		m_SlideExitList;			///< List of components that exits their
    // slides
    // TElementList		m_SlideEnterList;			///< List of components that enters their
    // slides

    //==============================================================================
    //	Methods
    //==============================================================================
public: // Construction
    CPresentationFrameData();
    void Reserve(const INT32 inElementCount);

public: // Data Management
    void Reset();

    TElementList &GetDirtyList() { return m_DirtyList; }
    TElementList &GetTraversalList() { return m_TraversalList; }
    TElementList &GetScriptsList() { return m_ScriptsList; }
    TElementList &GetActivationList() { return m_ActivationList; }
    TElementList &GetDeactivationList() { return m_DeactivationList; }
    // TElementList&	GetSlideExitList( )					{ return m_SlideExitList;
    // }
    // TElementList&	GetSlideEnterList( )				{ return m_SlideEnterList; }

private: // Disabled Copy Construction
    CPresentationFrameData(const CPresentationFrameData &);
    CPresentationFrameData &operator=(const CPresentationFrameData &);
};

} // namespace Q3DStudio
