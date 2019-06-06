/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#ifndef QT3DS_FOUNDATION_FILE_TOOLS_H
#define QT3DS_FOUNDATION_FILE_TOOLS_H

#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/StringTable.h"
#include "foundation/IOStreams.h"
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace qt3ds {
namespace foundation {

    typedef eastl::basic_string<char8_t, ForwardingAllocator> TStr;

    class CFileTools
    {
    public:
        // Normalizes all slashes to be windows-like
        static void NormalizePath(TStr &ioPath);
        static void NormalizePath(eastl::string &ioPath);

        // This is kind of the opposite of the 'NormalizePath' methods above in
        // that it forces usage of unix style directory separators rather than
        // windows separators. In addition, the path here can be a file path or
        // a qrc URL string. This function will eventually be refactored away
        // when we've converted to using Qt's file system abstraction throughout
        // the code base and use QUrl's throughout instead of raw strings to
        // represent resources.
        static QString NormalizePathForQtUsage(const QString &path);

        static void CombineBaseAndRelative(const char8_t *inBase, const char8_t *inRelative,
                                           TStr &outString);
        static void CombineBaseAndRelative(const char8_t *inBase, const char8_t *inRelative,
                                           eastl::string &outString);

        // inBase and inRelative will get normalized
        // This algorithm changes based on the platform.  On windows it is not case sensitive, on
        // not-windows it is.
        static void GetRelativeFromBase(TStr &inBase, TStr &inRelative, TStr &outString);
        static void GetRelativeFromBase(eastl::string &inBase, eastl::string &inRelative,
                                        eastl::string &outString);

        // A remapped path is a file path that starts with a '.' or a '/'.  GetRelativeFromBase
        // *always*
        // places a '.' or a '/' at the front of the path, so if you *know* the path came from
        // GetRelativeFromBase then you also know this function returns true of GetRelativeFromBase
        // actually generated a path that needs CombineBaseAndRelative.
        static bool RequiresCombineBaseAndRelative(const char8_t *inPath);

        // Search/replace so that all slashes are unix-like but only on non-windows platforms
        // Assumes the incoming path has been normalized
        static void ToPlatformPath(TStr &ioPath);
        static void ToPlatformPath(eastl::string &ioPath);

        static CRegisteredString RemapPathToBinaryFormat(TStr &inPath, TStr &inPresentationDir,
                                                         TStr &ioWorkspaceStr,
                                                         IStringTable &inStringTable);
        static CRegisteredString RemapPathFromBinaryFormat(CRegisteredString inPath,
                                                           const char8_t *inPresDir,
                                                           TStr &ioWorkspaceStr,
                                                           const CStrTableOrDataRef &inRef,
                                                           IStringTable &inStringTable);
        // I
        static void GetDirectory(eastl::string &ioPath);
        static bool DirectoryExists(const char8_t *inPath);
        static void GetExtension(const char8_t *inPath, eastl::string &outExt);
        static void Split(const char8_t *inPath, eastl::string &outDir, eastl::string &outFileStem,
                          eastl::string &outExtension);

        // Only implemented for windows.  Does not return '.' and '..' special entries
        // inPath is mangled in a platform specific way
        static void GetDirectoryEntries(const eastl::string &inPath,
                                        eastl::vector<eastl::string> &outFiles);

        static bool CreateDir(const eastl::string &inPath, bool inRecurse = true);

        // Given a/b.txt, we will end up with a/dirName/b.txt
        static void AppendDirectoryInPathToFile(eastl::string &ioPath, const char8_t *dirName);
        static void RemoveLastDirectoryInPathToFile(eastl::string &ioPath);

        static void SetExtension(eastl::string &ioPath, const char8_t *inExt);

        static bool FileExists(const char8_t *inPath);
        static eastl::string GetFileOrAssetPath(const char8_t *inPath);
        static void SetStreamPosition(QIODevice& device, qint64 inOffset,
                                      qt3ds::foundation::SeekPosition::Enum inEnum);
    };
}
}

#endif
