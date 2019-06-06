/****************************************************************************
**
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

#ifndef DOCUMENTRESOURCEMANAGERSCRIPTPARSER_H
#define DOCUMENTRESOURCEMANAGERSCRIPTPARSER_H

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "Qt3DSDMXML.h"
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSRenderInputStreamFactory.h"

namespace Q3DStudio {
namespace ScriptParser {

    using namespace qt3dsdm;
    using namespace Q3DStudio;

    struct StringInStream : qt3ds::foundation::IInStream
    {
        const QString &m_String;
        QT3DSU32 m_Offset;

        StringInStream(const QString &str)
            : m_String(str)
            , m_Offset(0)
        {
        }

        QT3DSU32 Read(NVDataRef<QT3DSU8> data) override
        {
            std::string str = m_String.toUtf8().constData();
            const char *theStart((const char *)str.c_str());
            const char *theEnd(theStart + str.size());

            QT3DSU32 amountLeft = QT3DSU32(theEnd - theStart) - m_Offset;
            QT3DSU32 amountToRead = qt3ds::NVMin(data.size(), amountLeft);
            qt3ds::intrinsics::memCopy(data.begin(), theStart + m_Offset, amountToRead);
            m_Offset += amountToRead;
            return amountToRead;
        }
    };

    struct SScriptParser
    {
        static std::shared_ptr<IDOMReader>
        ParseScriptFile(std::shared_ptr<IDOMFactory> factory,
                        std::shared_ptr<qt3dsdm::IStringTable> stringTable,
                        const QString &fullPath, CXmlErrorHandler &errorHandler,
                        qt3ds::render::IInputStreamFactory &streamFactory)
        {
            QVector<QT3DSU8> readBuf;
            {
                using namespace qt3ds;
                using namespace qt3ds::foundation;
                eastl::string strConvert;
                NVScopedRefCounted<qt3ds::render::IRefCountedInputStream> stream(
                    streamFactory.GetStreamForFile(fullPath));
                if (!stream) {
                    QT3DS_ASSERT(0);
                    return std::shared_ptr<IDOMReader>();
                }

                QT3DSU32 readLen = 0;
                do {
                    uint32_t offset = readBuf.size();
                    readBuf.resize(offset + 4096);
                    readLen = stream->Read(NVDataRef<QT3DSU8>(&readBuf[0] + offset, 4096));
                    readBuf.resize(offset + readLen);
                } while (readLen == 4096);
            }

            QByteArray byteArray;
            for (auto &&c : qAsConst(readBuf))
                byteArray.append(c);

            QString code = QString::fromUtf8(byteArray);

            bool skipXml = false;
            auto start = code.indexOf("/*[[");
            if (start == -1)
                skipXml = true;

            QString tagged("<Behavior>\n");
            if (!skipXml) {
                start += 4;
                auto end = code.indexOf("]]*/", start);
                if (end == -1)
                    return std::shared_ptr<IDOMReader>();

                QString xml = code.mid(start, end - start).trimmed();

                tagged.append(xml);
            }
            tagged.append("</Behavior>\n");
            tagged.replace("\r\n", "\n");

            StringInStream xmlStream(tagged);

            SDOMElement *element(CDOMSerializer::Read(*factory, xmlStream, &errorHandler));
            if (element == NULL)
                return std::shared_ptr<IDOMReader>();

            return IDOMReader::CreateDOMReader(*element, stringTable, factory);
        }
    };
}
}

#endif // DOCUMENTRESOURCEMANAGERSCRIPTPARSER_H
