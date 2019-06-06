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
#include "foundation/FileTools.h"
#include "foundation/Utils.h"
#include <string.h>
#
#ifdef EA_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else // posix
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif
#include <QDir>
#include <QFile>
#include <QUrl>

using namespace qt3ds::foundation;

namespace {
// State machine where you can add a character
// and it will tell you how many characters to erase
struct SPathStateMachine
{
    struct States
    {
        enum Enum {
            NoState = 0, // Don't care
            Slash, // Last char was either a forward or backward slash
            Period, // Last char was a period
            TwoPeriods, // Last two characters were periods
        };
    };
    struct Actions
    {
        enum Enum {
            NoAction = 0,
            DeleteBack1Slash,
            DeleteBack2Slashes,
        };
    };

    States::Enum m_State;

    SPathStateMachine()
        : m_State(States::NoState)
    {
    }

    Actions::Enum AnalyzeChar(char32_t inChar)
    {
        switch (inChar) {
        case '\\':
        case '/':
            switch (m_State) {
            case States::NoState:
                m_State = States::Slash;
                break;
            case States::Period:
                m_State = States::Slash;
                return Actions::DeleteBack1Slash;

            case States::TwoPeriods:
                m_State = States::Slash;
                return Actions::DeleteBack2Slashes;
            case States::Slash:
                return Actions::DeleteBack1Slash;
            }
            break;
        case '.':
            switch (m_State) {
            case States::Slash:
            case States::NoState:
                m_State = States::Period;
                break;
            case States::Period:
                m_State = States::TwoPeriods;
                break;
            case States::TwoPeriods:
                break;
            }
            break;
        default:
            m_State = States::NoState;
            break;
        }
        return Actions::NoAction;
    }
};

template <typename TStrType>
inline bool DoDeleteBack1Slash(TStr::size_type &idx, TStrType &ioPath)
{
    TStr::size_type slashLoc = ioPath.rfind('/', idx - 1);
    if ((slashLoc != TStr::npos) && (slashLoc > 2)
        // and the next *two* characters aren't both dots.
        && ((ioPath[slashLoc - 1] != '.') || (ioPath[slashLoc - 2] != '.'))) {

        ioPath.erase(ioPath.begin() + slashLoc, ioPath.begin() + idx);
        idx = slashLoc;
        return true;
    }
    return false;
}

template <typename TStrType>
void NormalizePathT(TStrType &ioPath)
{
    TStr::size_type pathLen = ioPath.size();
    SPathStateMachine theStateMachine;
    for (TStr::size_type idx = 0; idx < pathLen; ++idx) {
        char8_t &currentChar = ioPath[idx];
        if (currentChar == '\\')
            currentChar = '/';
        SPathStateMachine::Actions::Enum action = theStateMachine.AnalyzeChar(currentChar);
        switch (action) {
        case SPathStateMachine::Actions::DeleteBack2Slashes:
            if (DoDeleteBack1Slash(idx, ioPath))
                DoDeleteBack1Slash(idx, ioPath);
            pathLen = ioPath.size();
            break;

        case SPathStateMachine::Actions::DeleteBack1Slash:
            DoDeleteBack1Slash(idx, ioPath);
            pathLen = ioPath.size();
            break;
        default:
            break;
        }
    }
}

bool IsAbsolute(const char8_t *inPath, size_t inLen)
{
    if (inLen > 2 && inPath[1] == ':')
        return true;
    else if (inLen > 1 && (inPath[0] == '\\' || inPath[0] == '/'))
        return true;
    return false;
}

template <typename TStrType>
void CombineBaseAndRelativeT(const char8_t *inBase, const char8_t *inRelative, TStrType &outString)
{
    if (IsAbsolute(inRelative, StrLen(inRelative))) {
        outString.assign(nonNull(inRelative));
    } else {
        if (inRelative && *inRelative) {
            if (inRelative[0] == '#')
                outString.assign(inRelative);
            else {
                if (IsAbsolute(inRelative, strlen(inRelative))) {
                    outString.assign(inRelative);
                } else {
                    outString = inBase ? inBase : "";
                    if (outString.size())
                        outString.append("/");
                    outString.append(inRelative ? inRelative : (const char8_t *)L"");
                }
                NormalizePathT(outString);
            }
        }
    }
}

template <typename TStrType>
void GetRelativeFromBaseT(TStrType &inBaseStr, TStrType &inRelativeStr, TStrType &outString)
{
    outString.clear();
    NormalizePathT(inBaseStr);
    NormalizePathT(inRelativeStr);
    if (inBaseStr.size() == 0) {
        outString.assign(inRelativeStr.c_str());
        return;
    }
    if (inRelativeStr.size() == 0) {
        outString.clear();
        return;
    }
    // find longest common string
    const char8_t *inBase = inBaseStr.c_str();
    const char8_t *baseEnd = inBaseStr.c_str() + inBaseStr.size();
    const char8_t *inRelative = inRelativeStr.c_str();
    size_t relativeLen = inRelativeStr.size();
    const char8_t *relativeEnd = inRelative + relativeLen;

    for (; inRelative < relativeEnd && inBase < baseEnd && *inRelative == *inBase;
         ++inRelative, ++inBase)
        ;

    // They had nothing in common.
    if (inBase == inBaseStr.c_str()) {
        outString.assign(inRelativeStr.c_str());
        return;
    }

    if (inRelative && (*inRelative == '\\' || *inRelative == '/'))
        ++inRelative;

    const char *common = inBase;
    if (common == NULL || *common == 0) {
        outString.assign("./");
        outString.append(inRelative);
        NormalizePathT(outString);
        return;
    }
    // Backtrack to the nearest slash.
    while (*common && *common != '\\' && *common != '/')
        --common;

    bool foundNonSlash = false;
    for (; common != baseEnd; ++common) {
        if (*common != '\\' && *common != '/') {
            if (foundNonSlash == false)
                outString.append("..\\");
            foundNonSlash = true;
        } else
            foundNonSlash = false;
    }
    if (inRelative < relativeEnd) {
        if (outString.size() == 0)
            outString.assign("./");
        outString.append(inRelative);
    }
    NormalizePathT(outString);
}
}

void CFileTools::NormalizePath(TStr &ioPath)
{
    NormalizePathT(ioPath);
}

void CFileTools::NormalizePath(eastl::string &ioPath)
{
    NormalizePathT(ioPath);
}

QString CFileTools::NormalizePathForQtUsage(const QString &path)
{
    // path can be a file path or a qrc URL string.

    QString filePath = QDir::cleanPath(path);

    filePath.replace(QLatin1Char('\\'), QLatin1Char('/'));

    if (filePath.startsWith(QLatin1String("./")))
        return filePath.mid(2);

    if (filePath.startsWith(QLatin1String("qrc:/")))
        return filePath.mid(3);
    else
        return filePath;
}

void CFileTools::CombineBaseAndRelative(const char8_t *inBase, const char8_t *inRelative,
                                        TStr &outString)
{
    CombineBaseAndRelativeT(inBase, inRelative, outString);
}

void CFileTools::CombineBaseAndRelative(const char8_t *inBase, const char8_t *inRelative,
                                        eastl::string &outString)
{
    CombineBaseAndRelativeT(inBase, inRelative, outString);
}

void CFileTools::GetRelativeFromBase(TStr &inBaseStr, TStr &inRelativeStr, TStr &outString)
{
    GetRelativeFromBaseT(inBaseStr, inRelativeStr, outString);
}

void CFileTools::GetRelativeFromBase(eastl::string &inBaseStr, eastl::string &inRelativeStr,
                                     eastl::string &outString)
{
    GetRelativeFromBaseT(inBaseStr, inRelativeStr, outString);
}

bool CFileTools::RequiresCombineBaseAndRelative(const char8_t *inPath)
{
    if (inPath && *inPath)
        return inPath[0] == '.';
    return false;
}

template <typename TStrType>
void ToPlatformPathT(TStrType &outString)
{
#ifndef EA_PLATFORM_WINDOWS
    for (TStr::size_type pos = outString.find('\\'); pos != TStr::npos;
         pos = outString.find('\\', pos + 1))
        outString.replace(outString.begin() + pos, outString.begin() + pos + 1, "/");
#else
    (void)outString;
#endif
}

void CFileTools::ToPlatformPath(TStr &outString)
{
    ToPlatformPathT(outString);
}

void CFileTools::ToPlatformPath(eastl::string &outString)
{
    ToPlatformPathT(outString);
}

CRegisteredString CFileTools::RemapPathToBinaryFormat(TStr &inPath, TStr &inPresentationDir,
                                                      TStr &ioWorkspaceStr,
                                                      IStringTable &inStringTable)
{
    GetRelativeFromBase(inPresentationDir, inPath, ioWorkspaceStr);
    CRegisteredString theNewStr = inStringTable.RegisterStr(ioWorkspaceStr.c_str());
    theNewStr.Remap(inStringTable.GetRemapMap());
    return theNewStr;
}

CRegisteredString CFileTools::RemapPathFromBinaryFormat(CRegisteredString inPath,
                                                        const char8_t *inPresDir,
                                                        TStr &ioWorkspaceStr,
                                                        const CStrTableOrDataRef &inRef,
                                                        IStringTable &inStringTable)
{
    inPath.Remap(inRef);
    if (RequiresCombineBaseAndRelative(inPath.c_str())) {
        CombineBaseAndRelative(inPresDir, inPath, ioWorkspaceStr);
        return inStringTable.RegisterStr(ioWorkspaceStr.c_str());
    }
    return inPath;
}

void CFileTools::GetDirectory(eastl::string &ioPath)
{
    eastl::string::size_type theSlashPos = ioPath.find_last_of("\\/");
    if (theSlashPos == eastl::string::npos) {
        ioPath.clear();
        return;
    }
    ioPath.resize(theSlashPos);
}

bool CFileTools::DirectoryExists(const char8_t *inPath)
{
#ifdef EA_PLATFORM_WINDOWS
    DWORD theAtts = GetFileAttributesA(inPath);
    return theAtts != INVALID_FILE_ATTRIBUTES && (theAtts & FILE_ATTRIBUTE_DIRECTORY);
#else // Posix style check for directory
    int status;
    struct stat st_buf;
    status = stat(inPath, &st_buf);
    if (status == 0 && S_ISDIR(st_buf.st_mode))
        return true;
    return false;
#endif
}

bool CFileTools::FileExists(const char8_t *inPath)
{
#ifdef EA_PLATFORM_WINDOWS
    DWORD theAtts = GetFileAttributesA(inPath);
    return theAtts != INVALID_FILE_ATTRIBUTES;
#else // Posix style check for directory
    int status;
    struct stat st_buf;
    status = stat(inPath, &st_buf);
    if (status == 0)
        return true;
    return false;
#endif
}

eastl::string CFileTools::GetFileOrAssetPath(const char8_t *inPath)
{
    QFile tmp(inPath);
    if (tmp.exists())
        return inPath;
    return eastl::string("assets:/") + inPath;
}

void CFileTools::SetStreamPosition(QIODevice& device, qint64 inOffset,
                              qt3ds::foundation::SeekPosition::Enum inEnum)
{
    if (inEnum == qt3ds::foundation::SeekPosition::Begin)
        device.seek(inOffset);
    else if (inEnum == qt3ds::foundation::SeekPosition::Current)
        device.seek(device.pos() + inOffset);
    else if (inEnum == qt3ds::foundation::SeekPosition::End)
        device.seek(device.size() + inOffset);
}

void CFileTools::GetExtension(const char8_t *inPath, eastl::string &outExt)
{
    outExt.assign(nonNull(inPath));
    size_t dotPos = outExt.find_last_of('.');
    if (dotPos != eastl::string::npos)
        outExt.erase(outExt.begin(), outExt.begin() + dotPos + 1);
}

void CFileTools::Split(const char8_t *inPath, eastl::string &outDir, eastl::string &outFileStem,
                       eastl::string &outExtension)
{
    outDir.assign(nonNull(inPath));
    NormalizePath(outDir);
    outFileStem = outDir;
    GetDirectory(outDir);
    size_t lenDiff = outFileStem.size() - outDir.size();
    if (lenDiff > 0) {
        if (outDir.size())
            outFileStem = outFileStem.substr(outDir.size() + 1);

        eastl::string::size_type lastDot = outFileStem.find_last_of('.');
        if (lastDot != eastl::string::npos) {
            outExtension = outFileStem.substr(lastDot + 1);
            outFileStem.resize(lastDot);
        }
    }
}

#ifdef EA_PLATFORM_WINDOWS
void CFileTools::GetDirectoryEntries(const eastl::string &inPath,
                                     eastl::vector<eastl::string> &outFiles)
{
    if (inPath.size() == 0)
        return;
    eastl::string tempPath(inPath);
    NormalizePath(tempPath);
    for (eastl::string::size_type pos = tempPath.find_first_of('/'); pos != eastl::string::npos;
         pos = tempPath.find_first_of('/', pos + 1))
        tempPath[pos] = '\\';
    if (tempPath.back() != '\\')
        tempPath.append("\\");
    tempPath.append(1, '*');
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(tempPath.c_str(), &ffd);
    outFiles.clear();
    if (INVALID_HANDLE_VALUE == hFind)
        return;

    do {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
            continue;
        outFiles.push_back(eastl::string(ffd.cFileName));
    } while (FindNextFileA(hFind, &ffd) != 0);
}
#else
void CFileTools::GetDirectoryEntries(const eastl::string &inPath,
                                     eastl::vector<eastl::string> &outFiles)
{
    if (inPath.size() == 0)
        return;
    eastl::string tempPath(inPath);
    NormalizePath(tempPath);
    struct dirent *dent;
    DIR *srcdir = opendir(tempPath.c_str());
    if (srcdir) {
        while ((dent = readdir(srcdir)) != NULL) {
            if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
                continue;
            outFiles.push_back(eastl::string(dent->d_name));
        }
        closedir(srcdir);
    }
}
#endif

bool CFileTools::CreateDir(const eastl::string &inPath, bool inRecurse)
{
    if (DirectoryExists(inPath.c_str()))
        return true;

    eastl::string temp(inPath);
    GetDirectory(temp);
    if (temp.size() && !DirectoryExists(temp.c_str())) {
        if (inRecurse)
            CreateDir(temp, inRecurse);
        else
            return false;
    }

#ifdef EA_PLATFORM_WINDOWS
    BOOL result = CreateDirectoryA(inPath.c_str(), NULL);
    return result != 0;
#else
    int result = mkdir(inPath.c_str(), 0777);
    return result == 0;
#endif
}

void CFileTools::AppendDirectoryInPathToFile(eastl::string &ioPath, const char8_t *dirName)
{
    eastl::string::size_type lastSlash = ioPath.find_last_of("\\/");
    if (lastSlash != eastl::string::npos) {
        if (dirName == NULL)
            dirName = ""; // avoid crashes on null strings
        ioPath.insert(lastSlash + 1, "/");
        ioPath.insert(lastSlash + 1, dirName);
    } else {
        ioPath.insert(0, "/");
        ioPath.insert(0, dirName);
    }
}

void CFileTools::RemoveLastDirectoryInPathToFile(eastl::string &ioPath)
{
    eastl::string::size_type lastSlash = ioPath.find_last_of("\\/");
    if (lastSlash != eastl::string::npos) {
        eastl::string::size_type secondToLastSlash = ioPath.find_last_of("\\/", lastSlash - 1);
        if (secondToLastSlash != eastl::string::npos)
            ioPath = ioPath.erase(secondToLastSlash, lastSlash - secondToLastSlash);
    }
}

void CFileTools::SetExtension(eastl::string &ioPath, const char8_t *inExt)
{
    eastl::string::size_type thePos = ioPath.find_last_of(".");
    if (thePos != eastl::string::npos) {
        ++thePos;
        ioPath = ioPath.replace(thePos, ioPath.size() - thePos, inExt);
    }
}
