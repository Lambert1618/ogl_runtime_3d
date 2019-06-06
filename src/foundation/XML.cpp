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
#include "foundation/XML.h"
#include "foundation/Qt3DSPool.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/StringTable.h"
#include "foundation/IOStreams.h"
#include "foundation/StringConversionImpl.h"
#include "foundation/StrConvertUTF.h"
#include "foundation/StringTable.h"
#include "foundation/Utils.h"
#include "foundation/Qt3DSAtomic.h"
#include "EASTL/hash_map.h"

#ifdef QT3DS_VC
#include <windows.h> //output debug string
#endif

#include <QtCore/qxmlstream.h>

using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace qt3ds::intrinsics;

typedef char XML_Char;

namespace {

const QT3DSU16 g_BOMMarker = (QT3DSU16)0xFEFF;

// Some DOM parsing operations are destructive.  If you need
// them to not be destructive, then we need to modify
// the reader.  Specifically parsing lists of floats, due
// to a bug in strtod, is destructive.
struct SDOMReader : public IDOMReader
{
    SReaderScope m_TopElement;
    eastl::vector<SReaderScope> m_ScopeStack;
    NVScopedRefCounted<IDOMFactory> m_Factory;
    volatile QT3DSI32 mRefCount;

    SDOMReader(NVAllocatorCallback &inAlloc, SDOMElement &te, NVScopedRefCounted<IStringTable> s,
               NVScopedRefCounted<IDOMFactory> inFactory)
        : IDOMReader(inAlloc, s)
        , m_TopElement(&te)
        , m_Factory(inFactory)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Factory->GetAllocator())

    SDOMElement *Current() const { return m_TopElement.m_Element; }

    void PushScope() override { m_ScopeStack.push_back(SReaderScope(m_TopElement)); }

    void PopScope() override
    {
        if (m_ScopeStack.size()) {
            m_TopElement = m_ScopeStack.back();
            m_ScopeStack.pop_back();
        } else
            m_TopElement = SReaderScope();
    }

    SReaderScope GetScope() override { return m_TopElement; }

    void SetScope(SReaderScope inScope) override { m_TopElement = inScope; }

    SDOMElement *GetElement() const override { return m_TopElement.m_Element; }

    Option<CRegisteredString> RegisterNS(TXMLCharPtr ns)
    {
        if (ns) {
            return m_StringTable->RegisterStr(ns);
        }
        return Empty();
    }

    bool UnregisteredAtt(TXMLCharPtr name, TXMLCharPtr &outValue, TXMLCharPtr ns = NULL) override
    {
        outValue = "";
        SDOMElement *current(Current());
        if (current) {
            return current->AttValue(m_StringTable->RegisterStr(name), RegisterNS(ns), outValue);
        } else {
            QT3DS_ASSERT(false);
        }
        return false;
    }

    bool Att(TXMLCharPtr name, CRegisteredString &outValue, TXMLCharPtr ns = NULL) override
    {
        const char8_t *unregValue = NULL;
        if (UnregisteredAtt(name, unregValue, ns)) {
            outValue = m_StringTable->RegisterStr(unregValue);
            return true;
        }
        return false;
    }

    QT3DSU32 CountChildren(TXMLCharPtr childName = NULL, TXMLCharPtr ns = NULL) override
    {
        SDOMElement *elem = Current();
        if (elem == NULL) {
            QT3DS_ASSERT(false);
            return 0;
        }
        return elem->GetNumChildren(m_StringTable->RegisterStr(childName), RegisterNS(ns));
    }

    SDOMAttribute *CurrentAtt()
    {
        if (m_TopElement.m_Attribute)
            return m_TopElement.m_Attribute;
        return NULL;
    }

    SDOMAttribute *GetFirstAttribute() override
    {
        if (m_TopElement.m_Element)
            m_TopElement.m_Attribute = m_TopElement.m_Element->m_Attributes.m_Head;
        return m_TopElement.m_Attribute;
    }

    SDOMAttribute *GetNextAttribute() override
    {
        if (m_TopElement.m_Attribute)
            m_TopElement.m_Attribute = m_TopElement.m_Attribute->m_NextAttribute;
        return CurrentAtt();
    }

    bool MoveToFirstChild(TXMLCharPtr childName = NULL, TXMLCharPtr ns = NULL) override
    {
        SDOMElement *elem = Current();
        if (elem == NULL) {
            QT3DS_ASSERT(false);
            return false;
        }
        SDOMElement *child = elem->FindChild(m_StringTable->RegisterStr(childName), RegisterNS(ns));
        if (child != NULL) {
            m_TopElement = child;
            return true;
        }
        return false;
    }

    bool MoveToNextSibling(TXMLCharPtr childName = NULL, TXMLCharPtr ns = NULL) override
    {
        SDOMElement *elem = Current();
        if (elem == NULL) {
            QT3DS_ASSERT(false);
            return false;
        }
        SDOMElement *nextSibling =
            elem->FindSibling(m_StringTable->RegisterStr(childName), RegisterNS(ns));
        if (nextSibling) {
            m_TopElement = nextSibling;
            return true;
        }
        return false;
    }
    // Leave element means go to its parent.
    void Leave() override
    {
        QT3DS_ASSERT(m_TopElement.m_Element);
        if (m_TopElement.m_Element)
            m_TopElement = m_TopElement.m_Element->m_Parent;
    }

    bool Value(TXMLCharPtr &outValue) override
    {
        SDOMElement *current(Current());
        if (!current) {
            QT3DS_ASSERT(false);
            return false;
        }
        outValue = current->m_Value;
        return true;
    }

    SDOMElement *GetTopElement() override
    {
        SDOMElement *current(Current());
        while (current && current->m_Parent)
            current = current->m_Parent;
        return current;
    }

    virtual NVScopedRefCounted<IDOMFactory> GetFactory() { return m_Factory; }
};

struct SDOMWriter : public IDOMWriter, public SDOMReader
{
    NVScopedRefCounted<IDOMFactory> m_FactoryPtr;
    IDOMFactory &m_Factory;

    SDOMWriter(NVScopedRefCounted<IDOMFactory> inDOMFactory,
               NVScopedRefCounted<IStringTable> inStringTable, SDOMElement &inTopElem)
        : SDOMReader(inDOMFactory->GetAllocator(), inTopElem, inStringTable, *inDOMFactory)
        , m_FactoryPtr(inDOMFactory)
        , m_Factory(*inDOMFactory)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Factory.GetAllocator())

    void Begin(TXMLCharPtr inElemName, TXMLCharPtr inNamespace = NULL, int inLine = 0) override
    {
        if (!m_TopElement.m_Element) {
            QT3DS_ASSERT(false);
            return;
        }
        SDOMElement *current(Current());
        SDOMElement *newElement(m_Factory.NextElement(m_StringTable->RegisterStr(inElemName),
                                                      m_StringTable->RegisterStr(inNamespace),
                                                      inLine));
        current->AppendChild(*newElement);
        m_TopElement = newElement;
    }

    void Att(TXMLCharPtr name, TXMLCharPtr value, TXMLCharPtr ns, int inLine) override
    {
        if (!m_TopElement.m_Element) {
            QT3DS_ASSERT(false);
            return;
        }
        m_TopElement.m_Element->SetAttributeValue(m_StringTable->RegisterStr(name), RegisterNS(ns),
                                                  value, m_Factory, inLine);
    }

    void Value(TXMLCharPtr value) override
    {
        if (!m_TopElement.m_Element) {
            QT3DS_ASSERT(false);
            return;
        }
        if (value == NULL)
            value = "";
        size_t len = strlen(value);
        m_Factory.AppendStrBuf(value, (QT3DSU32)len);
        m_TopElement.m_Element->m_Value = m_Factory.FinalizeStrBuf();
    }

    void End() override
    {
        if (!m_TopElement.m_Element) {
            QT3DS_ASSERT(false);
            return;
        }
        Leave();
    }
    void RemoveCurrent() override
    {
        SDOMElement *current(Current());
        if (!current) {
            QT3DS_ASSERT(false);
            return;
        }
        if (current->m_Parent) {
            m_TopElement = current->m_Parent;
            m_TopElement.m_Element->RemoveChild(*current);
        }
    }
    void RemoveAttribute(TXMLCharPtr inItem, TXMLCharPtr inNamespace) override
    {
        SDOMElement *current(Current());
        if (!current) {
            QT3DS_ASSERT(false);
            return;
        }
        current->RemoveAttribute(m_StringTable->RegisterStr(inItem), RegisterNS(inNamespace));
    }

    void MoveBefore(TXMLCharPtr inItem, TXMLCharPtr inSibling, TXMLCharPtr inNs = NULL) override
    {
        SDOMElement *current(Current());
        if (!current) {
            QT3DS_ASSERT(false);
            return;
        }

        SDOMElement *theItem =
            current->FindChild(m_StringTable->RegisterStr(inItem), RegisterNS(inNs));
        SDOMElement *theSibling =
            current->FindChild(m_StringTable->RegisterStr(inSibling), RegisterNS(inNs));
        QT3DS_ASSERT(theItem && theSibling);
        if (theItem && theSibling)
            current->PrependChild(*theItem, theSibling);
    }

    // If current has no parent, then we are at the top
    // of the tree and we should return 0.  Or if there is no
    // current.
    // If there is one parent, we should return 1.
    QT3DSU32 GetTabs() override
    {
        QT3DSU32 retval = 0;
        SDOMElement *current(Current());
        do {
            if (current)
                current = current->m_Parent;
            if (current)
                ++retval;
        } while (current);
        return retval;
    }

    SDOMElement *GetTopElement() override { return SDOMReader::GetTopElement(); }

    NVScopedRefCounted<IDOMFactory> GetFactory() override { return m_FactoryPtr; }

    NVAllocatorCallback &GetAllocator() override { return m_Factory.GetAllocator(); }
};

struct SimpleXmlWriter
{

    struct SNameNS
    {
        TXMLCharPtr name;
        TXMLCharPtr ns;
        SNameNS(TXMLCharPtr _name = NULL, TXMLCharPtr _ns = NULL)
            : name(_name)
            , ns(_ns)
        {
        }
    };
    IOutStream &m_Stream;
    eastl::vector<eastl::pair<SNameNS, bool>> m_OpenElements;
    bool m_ElementOpen;
    char8_t m_PrintBuf[256];
    QT3DSU32 m_Tabs;
    eastl::basic_string<char8_t, ForwardingAllocator> m_Buffer;

    SimpleXmlWriter(NVAllocatorCallback &inAlloc, IOutStream &stream, QT3DSU32 inTabs = 0)
        : m_Stream(stream)
        , m_ElementOpen(false)
        , m_Tabs(inTabs)
        , m_Buffer(ForwardingAllocator(inAlloc, "SimpleXMLWriter::m_Buffer"))
    {
    }

    void Write(const SNameNS &data)
    {
        if (data.ns && *data.ns) {
            Write(data.ns);
            Write(':');
        }
        Write(data.name);
    }

    void Write(const char8_t *data, QT3DSU32 inLen = 0)
    {
        if (!isTrivial(data)) {
            if (inLen == 0)
                inLen = StrLen(data);
            m_Stream.Write(data, inLen);
        }
    }
    void BeginWrite() { m_Buffer.clear(); }
    void WriteTemp(char8_t data) { m_Buffer.append(1, data); }
    void WriteTemp(const char8_t *data) { m_Buffer.append(data); }
    void EndWrite() { Write(reinterpret_cast<const char8_t *>(m_Buffer.c_str()), m_Buffer.size()); }
    void Write(char8_t data) { m_Stream.Write(data); }
    void Tabs()
    {
        QT3DS_FOREACH(idx, (m_OpenElements.size() + m_Tabs))
        Write('\t');
    }
    void Close(bool newline)
    {
        if (m_ElementOpen) {
            Write(">");
            if (newline)
                Write('\n');
        }
        m_ElementOpen = false;
    }
    void Begin(TXMLCharPtr name, TXMLCharPtr ns)
    {
        Close(true);
        Tabs();
        Write('<');
        SNameNS theName(name, ns);
        Write(theName);
        m_OpenElements.push_back(eastl::pair<SNameNS, bool>(theName, false));
        m_ElementOpen = true;
    }
    TXMLCharPtr ToStr(char8_t val)
    {
        m_PrintBuf[0] = val;
        m_PrintBuf[1] = 0;
        return m_PrintBuf;
    }
    template <typename TDataType>
    TXMLCharPtr ToStr(TDataType val)
    {
        StringConversion<TDataType>().ToStr(val, NVDataRef<char8_t>(m_PrintBuf, 256));
        return m_PrintBuf;
    }
    void Att(TXMLCharPtr name, TXMLCharPtr ns, TXMLCharPtr value)
    {
        QT3DS_ASSERT(m_ElementOpen);
        Write(' ');
        Write(SNameNS(name, ns));

        Write("=\"");

        QString str = QString::fromUtf8(nonNull(value)).toHtmlEscaped();
        Write(str.toUtf8().constData());
        Write("\"");
    }
    template <typename TData>
    void Att(TXMLCharPtr name, TXMLCharPtr ns, TData value)
    {
        Att(name, ns, ToStr(value));
    }

    void Value(TXMLCharPtr value)
    {
        if (!isTrivial(value)) {
            Close(false);

            QString str = QString::fromUtf8(nonNull(value)).toHtmlEscaped();
            Write(str.toUtf8().constData());
            m_OpenElements.back().second = true;
        }
    }
    void ChildValue(TXMLCharPtr name, TXMLCharPtr ns, TXMLCharPtr value)
    {
        Begin(name, ns);
        Value(value);
        End();
    }
    void End(bool newlineAfterClose = true)
    {
        QT3DS_ASSERT(m_OpenElements.size());
        eastl::pair<SNameNS, bool> topElem = m_OpenElements.back();
        m_OpenElements.pop_back();
        if (m_ElementOpen)
            Write("/>");
        else {
            if (topElem.second == false)
                Tabs();
            Write("</");
            Write(topElem.first);
            Write(">");
        }
        m_ElementOpen = false;
        if (newlineAfterClose == true)
            Write('\n');
    }
};

struct DOMParser
{
    IDOMFactory &m_Factory;
    SDOMElement *m_TopElement;
    SDOMElement *m_FirstElement;
    char8_t m_nsSeparator;
    typedef eastl::basic_string<char8_t, ForwardingAllocator> TStrType;
    TStrType m_NSBuffer;
    TStrType m_NameBuffer;
    CRegisteredString m_RegName;
    CRegisteredString m_RegNS;
    SNamespacePairNode *m_FirstPair;
    SNamespacePairNode *m_LastPair;
    QXmlStreamReader *m_XMLParser;

    DOMParser(IDOMFactory &factory, QXmlStreamReader &parser, char8_t nsSeparator)
        : m_Factory(factory)
        , m_FirstElement(NULL)
        , m_nsSeparator(nsSeparator)
        , m_NSBuffer(ForwardingAllocator(factory.GetAllocator(), "DOMParser::m_NSBuffer"))
        , m_NameBuffer(ForwardingAllocator(factory.GetAllocator(), "DOMParser::m_NameBuffer"))
        , m_FirstPair(NULL)
        , m_LastPair(NULL)
        , m_XMLParser(&parser)
    {
    }

    void ParseName(const XML_Char *name)
    {
        m_NSBuffer.assign(name);
        eastl::string::size_type pos = m_NSBuffer.find(m_nsSeparator);
        if (pos != TStrType::npos) {
            m_NameBuffer.assign(m_NSBuffer.c_str() + pos + 1);
            m_NSBuffer.resize(pos);
        } else {
            m_NameBuffer = m_NSBuffer;
            m_NSBuffer.clear();
            // Find the namespace with no abbreviation.
            for (SNamespacePairNode *theNode = m_FirstPair; theNode;
                 theNode = theNode->m_NextNode) {
                if (theNode->m_Abbreviation.IsValid() == false)
                    m_NSBuffer.assign(theNode->m_Namespace.c_str());
            }
        }
        m_RegName = m_Factory.GetStringTable()->RegisterStr(m_NameBuffer.c_str());
        m_RegNS = m_Factory.GetStringTable()->RegisterStr(m_NSBuffer.c_str());
    }

    struct SimpleXmlErrorHandler : public CXmlErrorHandler
    {
        void OnXmlError(TXMLCharPtr errorName, int line, int column) override
        {
            char8_t buffer[1024];
            _snprintf(buffer, 1024, "%s(%d): Xml Error %s on line %d, column %d", __FILE__,
                      __LINE__, errorName, line, column);
#ifdef QT3DS_VC
            OutputDebugStringA(buffer);
#endif
            printf("%s\n", buffer);
        }
    };

    template <QT3DSU32 THeaderLen>
    struct SHeaderInStream : public IInStream
    {
        QT3DSU8 m_Header[THeaderLen];
        IInStream &m_InStream;
        QT3DSU32 m_BytesRead;

        SHeaderInStream(IInStream &inStream)
            : m_InStream(inStream)
            , m_BytesRead(0)
        {
        }
        bool readHeader()
        {
            QT3DSU32 amountRead = m_InStream.Read(NVDataRef<QT3DSU8>(m_Header, THeaderLen));
            return amountRead == THeaderLen;
        }
        QT3DSU32 Read(NVDataRef<QT3DSU8> data) override
        {
            if (data.size() == 0)
                return 0;
            QT3DSU8 *writePtr(data.begin());
            QT3DSU32 amountToRead(data.size());
            QT3DSU32 amountRead = 0;
            if (m_BytesRead < THeaderLen) {
                QT3DSU32 headerLeft = NVMin(THeaderLen - m_BytesRead, amountToRead);
                memCopy(writePtr, m_Header + m_BytesRead, headerLeft);
                writePtr += headerLeft;
                amountToRead -= headerLeft;
                amountRead += headerLeft;
            }
            if (amountToRead)
                amountRead += m_InStream.Read(NVDataRef<QT3DSU8>(writePtr, amountToRead));
            m_BytesRead += amountRead;
            return amountRead;
        }
    };

    static eastl::pair<SNamespacePairNode *, SDOMElement *>
    ParseXMLFile(IDOMFactory &factory, IInStream &inStream, CXmlErrorHandler *handler = NULL)
    {
        SimpleXmlErrorHandler simpleHandler;
        if (handler == NULL)
            handler = &simpleHandler;

        // look for BOM (Byte-Order-Marker) in the file
        // if found, pass in UTF-16 as the format.  else
        // pass in UTF-8 as the format.
        SHeaderInStream<sizeof(g_BOMMarker)> theInStream(inStream);
        if (theInStream.readHeader() == false) {
            QT3DS_ASSERT(false);
            return NULL;
        }
        QT3DSU16 theHeaderData;
        memCopy(&theHeaderData, theInStream.m_Header, sizeof(g_BOMMarker));

        char8_t nsSeparator = '$';

        if (theHeaderData == g_BOMMarker)
            qFatal("UTF-16 files not supported");

        QXmlStreamReader sreader;
        DOMParser domParser(factory, sreader, nsSeparator);
        QT3DSU8 dataBuf[2048];
        QT3DSU32 amountRead = 0;
        do {
            amountRead = theInStream.Read(toDataRef(dataBuf, 2048));
            if (amountRead) {
                QByteArray tmp = QByteArray::fromRawData((char*)dataBuf,amountRead);
                sreader.addData(tmp);
            }
        } while (amountRead > 0);
        while (!sreader.atEnd()) {
            QXmlStreamReader::TokenType token = sreader.readNext();
            if (token == QXmlStreamReader::StartElement) {
                domParser.m_Factory.IgnoreStrBuf();
                domParser.ParseName((TXMLCharPtr)sreader.name().toUtf8().data());
                int line = sreader.lineNumber();
                SDOMElement *newElem =
                    domParser.m_Factory.NextElement(domParser.m_RegName, domParser.m_RegNS, line);
                if (domParser.m_FirstElement == NULL) {
                    domParser.m_FirstElement = newElem;
                    domParser.m_TopElement = newElem;
                } else {
                    domParser.m_TopElement->AppendChild(*newElem);
                    domParser.m_TopElement = newElem;
                }
                const QXmlStreamAttributes& attributes = sreader.attributes();
                for (auto attrib : attributes) {
                    domParser.ParseName((TXMLCharPtr)attrib.name().toUtf8().data());
                    SDOMAttribute *att =
                            domParser.m_Factory.NextAttribute(domParser.m_RegName,
                                                              (TXMLCharPtr)attrib.value()
                                                              .toUtf8().data(),
                                                              domParser.m_RegNS, line);
                    newElem->m_Attributes.push_back(*att);
                }
            } else if (token == QXmlStreamReader::Characters) {
                QByteArray text = sreader.text().toUtf8();
                domParser.m_Factory.AppendStrBuf(text.data(), text.length());
            } else if (token == QXmlStreamReader::EndElement) {
                domParser.m_TopElement->m_Value = domParser.m_Factory.FinalizeStrBuf();
                domParser.m_TopElement = domParser.m_TopElement->m_Parent;
            }

            if (sreader.hasError()) {
                TXMLCharPtr error = (TXMLCharPtr)sreader.errorString().toUtf8().data();
                handler->OnXmlError(error, sreader.lineNumber(), sreader.columnNumber());
                return nullptr;
            }

        }
        return eastl::make_pair(domParser.m_FirstPair, domParser.m_FirstElement);
    }
};

class SimpleDomFactory : public IDOMFactory
{
    typedef eastl::basic_string<char8_t, ForwardingAllocator> TNarrowStr;
    NVAllocatorCallback &m_Allocator;
    Pool<SDOMElement> m_ElementPool;
    Pool<SDOMAttribute> m_AttributePool;
    Pool<SNamespacePairNode> m_NamespaceNodePool;
    eastl::vector<char8_t *> m_BigStrings;
    NVScopedRefCounted<IStringTable> m_StringTable;
    TNarrowStr m_StringBuilder;
    volatile QT3DSI32 mRefCount;

public:
    SimpleDomFactory(NVAllocatorCallback &alloc, NVScopedRefCounted<IStringTable> strt)
        : m_Allocator(alloc)
        , m_StringTable(strt)
        , m_StringBuilder(ForwardingAllocator(alloc, "SimpleDomFactory::m_StringBuilder"))
        , mRefCount(0)
    {
    }

    ~SimpleDomFactory()
    {
        QT3DS_FOREACH(idx, m_BigStrings.size())
        m_Allocator.deallocate(m_BigStrings[idx]);
        m_BigStrings.clear();
    }

    NVAllocatorCallback &GetAllocator() override { return m_Allocator; }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_Allocator)

    TXMLCharPtr RegisterStr(TXMLCharPtr str)
    {
        if (str == NULL || *str == 0)
            return "";
        return m_StringTable->RegisterStr(str);
    }

    virtual void Release() { delete this; }
    void AppendStrBuf(TXMLCharPtr str, QT3DSU32 len) override
    {
        if (len && *str) {
            m_StringBuilder.append(str, str + len);
        }
    }

    // Null terminate what is there and return the buffer.
    // This pointer needs to be persistent.
    TXMLCharPtr FinalizeStrBuf() override
    {
        if (m_StringBuilder.size() == 0)
            return "";
        QT3DSU32 len = m_StringBuilder.size() + 1;
        QT3DSU32 numBytes = len * sizeof(char8_t);
        char8_t *newMem =
            (char8_t *)m_Allocator.allocate(numBytes, "DOMFactory::ValueStr", __FILE__, __LINE__);
        memCopy(newMem, m_StringBuilder.c_str(), numBytes);
        m_BigStrings.push_back(newMem);
        m_StringBuilder.clear();
        return newMem;
    }
    void IgnoreStrBuf() override { m_StringBuilder.clear(); }

    SDOMAttribute *NextAttribute(CRegisteredString name, TXMLCharPtr val,
                                         CRegisteredString ns, int inLine) override
    {
        TXMLCharPtr v(RegisterValue(val));
        SDOMAttribute *att = (SDOMAttribute *)m_AttributePool.allocate(__FILE__, __LINE__);
        new (att) SDOMAttribute(name, ns, v, inLine);
        return att;
    }

    SDOMElement *NextElement(CRegisteredString name, CRegisteredString ns, int inLine) override
    {
        IgnoreStrBuf();
        SDOMElement *elem = (SDOMElement *)(m_ElementPool.allocate(__FILE__, __LINE__));
        new (elem) SDOMElement(name, ns, inLine);
        return elem;
    }

    SNamespacePairNode *NextNSPairNode(CRegisteredString ns, CRegisteredString abbr) override
    {
        return m_NamespaceNodePool.construct(SNamespacePair(ns, abbr), __FILE__, __LINE__);
    }

    NVScopedRefCounted<IStringTable> GetStringTable() override { return m_StringTable; }
};
}

NVScopedRefCounted<IDOMReader>
IDOMReader::CreateDOMReader(NVAllocatorCallback &inAlloc, SDOMElement &inRootElement,
                            NVScopedRefCounted<IStringTable> inStringTable,
                            NVScopedRefCounted<IDOMFactory> inFactory)
{
    return QT3DS_NEW(inAlloc, SDOMReader)(inAlloc, inRootElement, inStringTable, inFactory);
}

eastl::pair<NVScopedRefCounted<IDOMWriter>, NVScopedRefCounted<IDOMReader>>
IDOMWriter::CreateDOMWriter(NVScopedRefCounted<IDOMFactory> inFactory, SDOMElement &inRootElement,
                            NVScopedRefCounted<IStringTable> inStringTable)
{
    NVScopedRefCounted<SDOMWriter> writer(
        QT3DS_NEW(inFactory->GetAllocator(), SDOMWriter)(inFactory, inStringTable, inRootElement));
    return eastl::make_pair(writer.mPtr, writer.mPtr);
}

TXMLCharPtr IDOMFactory::RegisterValue(TXMLCharPtr inValue)
{
    if (isTrivial(inValue))
        return "";
    IgnoreStrBuf();
    AppendStrBuf(inValue, (QT3DSU32)strlen(inValue));
    return FinalizeStrBuf();
}

NVScopedRefCounted<IDOMFactory>
IDOMFactory::CreateDOMFactory(NVAllocatorCallback &inAlloc,
                              NVScopedRefCounted<IStringTable> inStrTable)
{
    return QT3DS_NEW(inAlloc, SimpleDomFactory)(inAlloc, inStrTable);
}

void CDOMSerializer::WriteXMLHeader(IOutStream &inStream)
{
    const char8_t *header = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    QT3DSU32 len = (QT3DSU32)strlen(header);
    inStream.Write(header, len);
}

// Lexigraphically sort the attributes.
struct SAttributeComparator
{
    bool operator()(const SDOMAttribute *lhs, const SDOMAttribute *rhs)
    {
        int nsCmp = strcmp(lhs->m_Namespace, rhs->m_Namespace);
        if (nsCmp)
            return nsCmp < 0;

        return strcmp(lhs->m_Name, rhs->m_Name) < 0;
    }
};

typedef eastl::vector<SDOMAttribute *, ForwardingAllocator> TAttVector;
typedef eastl::hash_map<CRegisteredString, CRegisteredString, eastl::hash<CRegisteredString>,
                        eastl::equal_to<CRegisteredString>, ForwardingAllocator>
    TNSMap;

CRegisteredString GetOrCreateNamespaceAbbr(NVAllocatorCallback &inAlloc, CRegisteredString theNs,
                                           TNSMap &inNamespace, TNSMap &inReverseNamespaces,
                                           IStringTable &inStrTable, bool &outCreated)
{
    CRegisteredString abbr;
    if (theNs.IsValid()) {
        TNSMap::iterator nsIter = inNamespace.find(theNs);
        if (nsIter != inNamespace.end()) {
            abbr = nsIter->second;
            outCreated = false;
        } else {
            eastl::basic_string<char8_t, ForwardingAllocator> theStr(
                ForwardingAllocator(inAlloc, "nsBuilderString"));
            theStr.assign("a");
            eastl::basic_string<char8_t, ForwardingAllocator> theNsStr(
                ForwardingAllocator(inAlloc, "nsBuilderString"));
            CRegisteredString theNS = inStrTable.RegisterStr(theStr.c_str());
            while (inReverseNamespaces.find(theNS) != inReverseNamespaces.end()) {
                for (int idx = 0;
                     idx < 26 && inReverseNamespaces.find(theNS) == inReverseNamespaces.end();
                     ++idx) {
                    theNsStr = theStr;
                    theNsStr.append(1, (char8_t)('a' + idx));
                    theNS = inStrTable.RegisterStr(theNsStr.c_str());
                }
                theStr.append(1, 'a');
            }
            inNamespace.insert(eastl::make_pair(theNs, theNS));
            abbr = theNS;
            outCreated = true;
        }
    }
    return abbr;
}

// Write an element with attributes sorted by name so a file diff is effective.
void WriteElement(NVAllocatorCallback &inAlloc, SDOMElement &inElement, SimpleXmlWriter &inWriter,
                  TAttVector &inAttSorter, TNSMap &inNamespace, TNSMap &inReverseNamespaces,
                  IStringTable &inStrTable, bool inTrimEmptyElems, bool inWriteXMLNS)
{
    inAttSorter.clear();

    // We decided that we don't want attribute sorting; the code that adds attributes needs
    // to be consistent.
    // This element doesn't add anything to the system in this case.
    if (inTrimEmptyElems && inElement.m_Attributes.empty() && inElement.m_Children.empty()
        && isTrivial(inElement.m_Value))
        return;

    for (TAttributeList::iterator iter = inElement.m_Attributes.begin(),
                                  end = inElement.m_Attributes.end();
         iter != end; ++iter)
        inAttSorter.push_back(&(*iter));

    // Find the namespace for the element
    bool createdAbbr = false;
    CRegisteredString abbr = GetOrCreateNamespaceAbbr(inAlloc, inElement.m_Namespace, inNamespace,
                                                      inReverseNamespaces, inStrTable, createdAbbr);

    inWriter.Begin(inElement.m_Name, abbr);
    eastl::basic_string<char8_t, ForwardingAllocator> theStr(
        ForwardingAllocator(inAlloc, "nsBuilderString"));
    if (inWriteXMLNS) {
        for (TNSMap::iterator iter = inNamespace.begin(), end = inNamespace.end(); iter != end;
             ++iter) {
            theStr.assign("xmlns");
            if (iter->second.IsValid()) {
                theStr.append(1, ':');
                theStr.append(iter->second.c_str());
            }
            inWriter.Att(theStr.c_str(), NULL, iter->first.c_str());
        }
    } else if (createdAbbr) {
        theStr.assign("xmlns:");
        theStr.append(abbr.c_str());
        inWriter.Att(theStr.c_str(), NULL, inElement.m_Namespace.c_str());
    }

    const char8_t *theLastAttName = 0;
    const char8_t *theLastAttNamespace = 0;
    for (QT3DSU32 idx = 0, end = inAttSorter.size(); idx < end; ++idx) {
        SDOMAttribute *theAtt(inAttSorter[idx]);
        if (theAtt->m_Name != theLastAttName || theAtt->m_Namespace != theLastAttNamespace) {
            abbr = GetOrCreateNamespaceAbbr(inAlloc, theAtt->m_Namespace, inNamespace,
                                            inReverseNamespaces, inStrTable, createdAbbr);
            if (createdAbbr) {
                theStr.assign("xmlns:");
                theStr.append(abbr.c_str());
                inWriter.Att(theStr.c_str(), NULL, inElement.m_Namespace.c_str());
            }

            inWriter.Att(theAtt->m_Name, abbr, theAtt->m_Value);
        } else {
            QT3DS_ASSERT(false);
        }
        theLastAttName = theAtt->m_Name;
        theLastAttNamespace = theAtt->m_Namespace;
    }
    // Elements can either have children or values but not both at this point.
    if (inElement.m_Children.empty() == false) {
        for (SDOMElement::TElementChildList::iterator iter = inElement.m_Children.begin(),
                                                      end = inElement.m_Children.end();
             iter != end; ++iter)
            WriteElement(inAlloc, *iter, inWriter, inAttSorter, inNamespace, inReverseNamespaces,
                         inStrTable, inTrimEmptyElems, false);

        inWriter.End();
    } else {
        if (!isTrivial(inElement.m_Value))
            inWriter.Value(inElement.m_Value);

        inWriter.End();
    }
}

void CDOMSerializer::Write(NVAllocatorCallback &inAlloc, SDOMElement &inElement,
                           IOutStream &inStream, IStringTable &inStrTable,
                           NVConstDataRef<SNamespacePair> inNamespaces, bool inTrimEmptyElements,
                           QT3DSU32 inTabs, bool inWriteNamespaces)
{
    TAttVector theAttSorter(ForwardingAllocator(inAlloc, "CDOMSerializer::AttributeList"));
    TNSMap theNamespaces(ForwardingAllocator(inAlloc, "CDOMSerializer::NamespaceMap"));
    TNSMap theReverseNamespaces(ForwardingAllocator(inAlloc, "CDOMSerializer::NamespaceMap"));
    for (QT3DSU32 idx = 0, end = inNamespaces.size(); idx < end; ++idx) {
        theNamespaces.insert(
            eastl::make_pair(inNamespaces[idx].m_Namespace, inNamespaces[idx].m_Abbreviation));
        theReverseNamespaces.insert(
            eastl::make_pair(inNamespaces[idx].m_Abbreviation, inNamespaces[idx].m_Namespace));
    }
    SimpleXmlWriter writer(inAlloc, inStream, inTabs);
    WriteElement(inAlloc, inElement, writer, theAttSorter, theNamespaces, theReverseNamespaces,
                 inStrTable, inTrimEmptyElements, inWriteNamespaces);
}

eastl::pair<SNamespacePairNode *, SDOMElement *>
CDOMSerializer::Read(IDOMFactory &inFactory, IInStream &inStream, CXmlErrorHandler *inErrorHandler)
{
    return DOMParser::ParseXMLFile(inFactory, inStream, inErrorHandler);
}
