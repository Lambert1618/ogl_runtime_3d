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
#include "Qt3DSRenderInputStreamFactory.h"

#include "stdio.h"

#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSAllocatorCallback.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSMutex.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

using namespace qt3ds::render;

namespace {
struct SInputStream : public IRefCountedInputStream
{
    NVFoundationBase &m_Foundation;
    QString m_Path;
    QFile m_File;
    volatile QT3DSI32 mRefCount;

    SInputStream(NVFoundationBase &inFoundation, const QString &inPath)
        : m_Foundation(inFoundation)
        , m_Path(inPath)
        , m_File(inPath)
        , mRefCount(0)
    {
        m_File.open(QIODevice::ReadOnly);
    }
    virtual ~SInputStream()
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    QT3DSU32 Read(NVDataRef<QT3DSU8> data) override
    {
        return m_File.read((char *)data.begin(), data.size());
    }

    bool Write(NVConstDataRef<QT3DSU8> /*data*/) override
    {
        QT3DS_ASSERT(false);
        return false;
    }

    void SetPosition(QT3DSI64 inOffset, qt3ds::foundation::SeekPosition::Enum inEnum) override
    {
        if (inOffset > QT3DS_MAX_I32 || inOffset < QT3DS_MIN_I32) {
            qCCritical(INVALID_OPERATION, "Attempt to seek further than platform allows");
            QT3DS_ASSERT(false);
            return;
        } else {
            CFileTools::SetStreamPosition(m_File, inOffset, inEnum);
        }
    }
    QT3DSI64 GetPosition() const override
    {
        return m_File.pos();
    }

    static SInputStream *OpenFile(const QString &inPath, NVFoundationBase &inFoundation)
    {
        return QT3DS_NEW(inFoundation.getAllocator(), SInputStream)(inFoundation, inPath);
    }
};

typedef eastl::basic_string<char8_t, ForwardingAllocator> TStrType;
struct SFactory : public IInputStreamFactory
{
    NVFoundationBase &m_Foundation;
    volatile QT3DSI32 mRefCount;

    Mutex m_Mutex;
    typedef Mutex::ScopedLock TScopedLock;

    const QString QT3DSTUDIO_TAG = QStringLiteral("qt3dstudio");

    SFactory(NVFoundationBase &inFoundation)
        : m_Foundation(inFoundation)
        , mRefCount(0)
        , m_Mutex(inFoundation.getAllocator())
    {
        // Add the top-level qrc directory
        if (!QDir::searchPaths(QT3DSTUDIO_TAG).contains(QLatin1String(":/")))
            QDir::addSearchPath(QT3DSTUDIO_TAG, QStringLiteral(":/"));
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Foundation.getAllocator())

    QFileInfo matchCaseInsensitiveFile(const QString& file, bool inQuiet)
    {
        if (!inQuiet) {
            // Some assets are searched for in several levels in the project structure,
            // we don't want to alert user of things that can't be fixed in the presentation
            // itself.
            qCWarning(WARNING, PERF_INFO, "Case-insensitive matching with file: %s",
                file.toLatin1().constData());
        }
        const QStringList searchDirectories = QDir::searchPaths(QT3DSTUDIO_TAG);
        for (const auto &directoryPath : searchDirectories) {
            QFileInfo fileInfo(file);
            QDirIterator it(directoryPath, {fileInfo.fileName()}, QDir::NoFilter,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString filePath = it.next();
                if (filePath.compare(QDir::cleanPath(directoryPath + '/' + file),
                                     Qt::CaseInsensitive) == 0) {
                    return QFileInfo(filePath);
                }
            }
        }

        return QFileInfo();
    }

    void AddSearchDirectory(const char8_t *inDirectory) override
    {
        TScopedLock __factoryLocker(m_Mutex);
        QString localDir = CFileTools::NormalizePathForQtUsage(inDirectory);
        QDir directory(localDir);
        if (!directory.exists()) {
            qCCritical(INTERNAL_ERROR, "Adding search directory: %s", inDirectory);
            return;
        }

        if (!QDir::searchPaths(QT3DSTUDIO_TAG).contains(localDir))
            QDir::addSearchPath(QT3DSTUDIO_TAG, localDir);
    }


    IRefCountedInputStream *GetStreamForFile(const QString &inFilename, bool inQuiet) override
    {
        TScopedLock __factoryLocker(m_Mutex);
        QString localFile = CFileTools::NormalizePathForQtUsage(inFilename);
        QFileInfo fileInfo = QFileInfo(localFile);
        SInputStream *inputStream = nullptr;
        // Try to match the file with the search paths
        if (!fileInfo.exists())
            fileInfo.setFile(QStringLiteral("qt3dstudio:") + localFile);

        // Try to match the case-insensitive file with the given search paths
        if (!fileInfo.exists())
            fileInfo = matchCaseInsensitiveFile(localFile, inQuiet);

        if (fileInfo.exists())
            inputStream = SInputStream::OpenFile(fileInfo.absoluteFilePath(), m_Foundation);

        if (!inputStream && !inQuiet) {
            // Print extensive debugging information.
            qCCritical(INTERNAL_ERROR, "Failed to find file: %s", localFile.toLatin1().constData());
            qCCritical(INTERNAL_ERROR, "Searched path: %s",
                QDir::searchPaths(QT3DSTUDIO_TAG).join(',').toLatin1().constData());
        }
        return inputStream;
    }

    bool GetPathForFile(const QString &inFilename, QString &outFile,
                        bool inQuiet = false) override
    {
        NVScopedRefCounted<IRefCountedInputStream> theStream =
            GetStreamForFile(inFilename, inQuiet);
        if (theStream) {
            SInputStream *theRealStream = static_cast<SInputStream *>(theStream.mPtr);
            outFile = theRealStream->m_Path;
            return true;
        }
        return false;
    }
};
}

IInputStreamFactory &IInputStreamFactory::Create(NVFoundationBase &inFoundation)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SFactory)(inFoundation);
}
