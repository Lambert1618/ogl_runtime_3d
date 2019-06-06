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
#include "SystemPrefix.h"
#include "Qt3DSDLLManager.h"
#include "Qt3DSBasicPluginDLL.h"
#include <QDebug>
#ifdef _LINUXPLATFORM
#include <dlfcn.h>
#endif

using namespace Q3DStudio;

//==============================================================================
/**
 *	Comparator for DLL instances.
 *	Compares by plugin type and handle
 */
namespace Q3DStudio {
bool operator==(const _SDLLInfo &inInfo1, const _SDLLInfo &inInfo2)
{
    return (inInfo1.m_Type == inInfo2.m_Type) && (inInfo1.m_Handle == inInfo2.m_Handle);
}
}

//==============================================================================
/**
 *	Singleton getter
 */
CDLLManager &CDLLManager::GetDLLManager()
{
    static CDLLManager theDLLManager = CDLLManager();
    return theDLLManager;
}

//==============================================================================
/**
 *	CTOR
 */
CDLLManager::CDLLManager()
{
}

//==============================================================================
/**
 *	DTOR
 */
CDLLManager::~CDLLManager()
{
}

//==============================================================================
/**
 *	Cleanup
 */
void CDLLManager::Cleanup()
{
    m_LoadedLibraries.Clear(true);
}

//==============================================================================
/**
 *	Loads a DLL/shared library given a path to the library plugin and the plugin
 *	type to expect.
 */
long CDLLManager::LoadLibrary(const char *inLibraryPath, long inPluginType)
{
    long theEmptyIndex = NVARRAY_NOTFOUND;
#ifdef _PCPLATFORM
    DLLHANDLE theHandle = ::LoadLibraryA(inLibraryPath);
#endif

#ifdef _LINUXPLATFORM
    DLLHANDLE theHandle = ::dlopen(inLibraryPath, RTLD_NOW);

    if (!theHandle) {
        const char *error = ::dlerror();
        qWarning() << "Failed to load shared library, error:" << QStringLiteral("%1, full path: %2").arg(error).arg(inLibraryPath);
    }

#endif

#ifdef _INTEGRITYPLATFORM
    DLLHANDLE theHandle = 0;
    qWarning() << "CDLLManager::LoadLibrary handle is zero!";
#endif

    Q3DStudio_ASSERT(theHandle);

    if (theHandle != NULL) {
        SDLLInfo theInfo;
        PROC_GetPluginType theProc =
            reinterpret_cast<PROC_GetPluginType>(GetProc("GetPluginType", theHandle));
        Q3DStudio_ASSERT(theProc);

        if (theProc) {
            theInfo.m_Type = theProc();

            if (theInfo.m_Type == inPluginType) {
                theEmptyIndex = m_LoadedLibraries.GetIndex(theInfo);
                theInfo.m_Handle = theHandle;

                if (theEmptyIndex == NVARRAY_NOTFOUND) {
                    m_LoadedLibraries.Push(theInfo);
                    theEmptyIndex = m_LoadedLibraries.GetCount() - 1;
                } else
                    m_LoadedLibraries[theEmptyIndex] = theInfo;
            }
        }

        if (theEmptyIndex == NVARRAY_NOTFOUND) {
#ifdef _PCPLATFORM
            ::FreeLibrary(theHandle);
#endif

#ifdef _LINUXPLATFORM
            ::dlclose(theHandle);
#endif
        }
    }

    return theEmptyIndex;
}

//==============================================================================
/**
 *	Unloads a DLL given a handle
 */
void CDLLManager::UnloadLibrary(const long inHandle)
{
    if (inHandle >= 0 && inHandle < m_LoadedLibraries.GetCount()) {
#ifdef _PCPLATFORM
        ::FreeLibrary(m_LoadedLibraries[inHandle].m_Handle);
#endif

#ifdef _LINUXPLATFORM
        ::dlclose(m_LoadedLibraries[inHandle].m_Handle);
#endif

        m_LoadedLibraries[inHandle] = SDLLInfo();
    }
}

//==============================================================================
/**
 *	Retrieves a DLL proc given the proc name
 */
void *CDLLManager::GetProc(const char *inProcName, long inHandle)
{
    if (inHandle >= 0 && inHandle < m_LoadedLibraries.GetCount())
        return GetProc(inProcName, m_LoadedLibraries[inHandle].m_Handle);

    return NULL;
}

//==============================================================================
/**
 *	Retrieves a DLL proc fiven the proc name
 */
void *CDLLManager::GetProc(const char *inProcName, const DLLHANDLE inHandle)
{
#ifdef _PCPLATFORM
#ifdef QT3DS_VC
    return ::GetProcAddress(inHandle, inProcName);
#else
    return (void *)(::GetProcAddress(inHandle, inProcName));
#endif
#endif

#ifdef _LINUXPLATFORM
    return ::dlsym(inHandle, inProcName);
#endif

#ifdef _INTEGRITYPLATFORM
    qWarning() << "CDLLManager::GetProc returns NULL!";
    return NULL;
#endif
}

//==============================================================================
/**
 *	Retrieves the plugin type for the DLL
 */
template <typename EEnumType>
EEnumType CDLLManager::GetPluginType(long inHandle)
{
    if (inHandle >= 0 && inHandle < m_LoadedLibraries.GetCount())
        return static_cast<EEnumType>(m_LoadedLibraries[inHandle].m_Type);

    return 0;
}
