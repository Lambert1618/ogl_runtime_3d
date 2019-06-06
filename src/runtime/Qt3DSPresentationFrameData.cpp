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

#include "RuntimePrefix.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSPresentationFrameData.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
//	Constants
//==============================================================================
const FLOAT DIRTY_STORE_RATIO = 0.1f;
const FLOAT TRAVERSAL_STORE_RATIO = 0.5f;
const FLOAT ACTIVATION_STORE_RATIO = 0.3f;
const FLOAT DEACTIVATION_STORE_RATIO = 0.3f;

//==============================================================================
/**
 *	Constructor
 */
CPresentationFrameData::CPresentationFrameData()
    : m_DirtyList(0, 0, "Frame:DirtyList")
    , m_TraversalList(0, 0, "Frame:TraversalList")
    , m_ScriptsList(0, 0, "Frame:ScriptsList")
    , m_ActivationList(0, 0, "Frame:ActivationList")
    , m_DeactivationList(0, 0, "Frame:DeactivationList") /*,*/
// m_SlideExitList( 0, 0, "Frame:SlideExitList" ),
// m_SlideEnterList( 0, 0, "Frame:SlideEnterList" )
{
}

//==============================================================================
/**
 *	Reserve memory from the memory subsytem to store the various arrays.
 *	@param inElementCount	the number of elements loaded
 */
void CPresentationFrameData::Reserve(const INT32 /*inElementCount*/)
{
}

//==============================================================================
/**
 *	Clean up the data. This is usually called at the start of the frame rhythm to
 *	clean up the data from previous frame
 *	Memory allocation should be clear by default.
 *	Previous list could be a lot longer than current or future list which means
 *	the appliation is using more memory than needed.  The heuristics of both
 *	initialization and runtime freeing is dependent on the presentation.
 */
void CPresentationFrameData::Reset()
{
    // Keep the capacity of these lists every frame since they have consistent
    // sizes from frame to frame
    m_TraversalList.Clear(false);
    m_ScriptsList.Clear(false);

    // These lists are more sporatic (slide changes) for now, clear them as
    // above but potential memory saving could occur by calling Clear( true ) instead
    m_ActivationList.Clear(false);
    m_DeactivationList.Clear(false);

    // NOTE: When we clear the dirty list, we also must ensure that the elements
    // themselves are un-dirtied to prepare for the next frame.  This is done
    // by the caller of "CPresentationFrameData::Reset"
    m_DirtyList.Clear(false);

    // m_SlideExitList.Clear( false );
    // m_SlideEnterList.Clear( false );
}

} // namespace Q3DStudio
