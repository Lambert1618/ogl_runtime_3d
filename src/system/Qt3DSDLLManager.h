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

#ifdef _PCPLATFORM
#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)
#endif

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

#ifdef _PCPLATFORM
typedef HMODULE DLLHANDLE;
#endif

#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
typedef void *DLLHANDLE;
#endif

typedef struct _SDLLInfo
{
    long m_Type; ///< Plugin type
    DLLHANDLE m_Handle; ///< DLL handle

    _SDLLInfo()
        : m_Type(0)
        , m_Handle(NULL)

    {
    }

} SDLLInfo;

//==============================================================================
/**
 *	Loads DLLs and queries for DLL functions
 */
class CDLLManager
{
public:
    static CDLLManager &GetDLLManager();

protected:
    CDLLManager();
    virtual ~CDLLManager();

public:
    long LoadLibrary(const char *inLibraryPath, long inPluginType);
    void UnloadLibrary(const long inHandle);
    void *GetProc(const char *inProcName, long inHandle);
    template <typename EEnumType>
    EEnumType GetPluginType(long inHandle);
    void Cleanup();

protected:
    void *GetProc(const char *inProcName, const DLLHANDLE inHandle);

protected:
    CArray<Q3DStudio::SDLLInfo> m_LoadedLibraries;
};

} // namespace Q3DStudio
