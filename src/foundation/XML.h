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
#ifndef QT3DS_FOUNDATION_XML_H
#define QT3DS_FOUNDATION_XML_H

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "foundation/StringConversion.h" //Conversion between string and various datatypes.
#include "foundation/Qt3DSFlags.h"
#include "EASTL/algorithm.h"
#include "foundation/IOStreams.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/StringTable.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"

namespace qt3ds {
namespace foundation {

    class IStringTable;

    class IDOMFactory;

    class IInStream;
    class IOutStream;
    struct SDOMAttribute;
    struct SDOMElement;
    struct SNamespacePairNode;

    typedef const char8_t *TXMLCharPtr;

    typedef eastl::basic_string<char8_t, ForwardingAllocator> TXMLStr;

    // When the factor is destroyed, all elements and attributes are destroyed.
    class IDOMFactory : public NVRefCounted
    {
    protected:
        virtual ~IDOMFactory() {}
    public:
        // Str does not need to be null terminated.
        virtual void AppendStrBuf(TXMLCharPtr str, QT3DSU32 len) = 0;
        // Null terminate what is there and return the buffer.
        // This pointer needs to be persistent.
        virtual TXMLCharPtr FinalizeStrBuf() = 0;
        virtual void IgnoreStrBuf() = 0;

        virtual SDOMAttribute *NextAttribute(CRegisteredString name, TXMLCharPtr val,
                                             CRegisteredString ns = CRegisteredString(),
                                             int inLine = 0) = 0;
        virtual SDOMElement *NextElement(CRegisteredString name,
                                         CRegisteredString ns = CRegisteredString(),
                                         int inLine = 0) = 0;
        virtual SNamespacePairNode *NextNSPairNode(CRegisteredString ns,
                                                   CRegisteredString abbr) = 0;

        TXMLCharPtr RegisterValue(TXMLCharPtr inValue);

        virtual NVScopedRefCounted<IStringTable> GetStringTable() = 0;
        virtual NVAllocatorCallback &GetAllocator() = 0;

        static NVScopedRefCounted<IDOMFactory>
        CreateDOMFactory(NVAllocatorCallback &inAllocator,
                         NVScopedRefCounted<IStringTable> inStrTable);
    };

    struct SDOMAttribute
    {
        CRegisteredString m_Namespace;
        CRegisteredString m_Name;
        const char8_t *m_Value;
        SDOMAttribute *m_NextAttribute;
        SDOMAttribute *m_PreviousAttribute;
        int m_Line;
        SDOMAttribute()
            : m_Value(NULL)
            , m_NextAttribute(NULL)
            , m_PreviousAttribute(NULL)
            , m_Line(0)
        {
        }

        SDOMAttribute(CRegisteredString inName, CRegisteredString inNs, const char8_t *inValue,
                      int line)
            : m_Namespace(inNs)
            , m_Name(inName)
            , m_Value(inValue)
            , m_NextAttribute(NULL)
            , m_PreviousAttribute(NULL)
            , m_Line(line)
        {
        }

        bool Value(const char8_t *&outValue)
        {
            outValue = m_Value;
            return true;
        }

        template <typename TDataType>
        bool Value(TDataType &outValue)
        {
            StringConversion<TDataType>().StrTo(m_Value, outValue);
            return true;
        }
    };

    struct SNextAttOp
    {
        SDOMAttribute *get(SDOMAttribute &inAtt) { return inAtt.m_NextAttribute; }
        const SDOMAttribute *get(const SDOMAttribute &inAtt) { return inAtt.m_NextAttribute; }
        void set(SDOMAttribute &inAtt, SDOMAttribute *next) { inAtt.m_NextAttribute = next; }
    };
    struct SPrevAttOp
    {
        SDOMAttribute *get(SDOMAttribute &inAtt) { return inAtt.m_PreviousAttribute; }
        const SDOMAttribute *get(const SDOMAttribute &inAtt) { return inAtt.m_PreviousAttribute; }
        void set(SDOMAttribute &inAtt, SDOMAttribute *prev) { inAtt.m_PreviousAttribute = prev; }
    };

    typedef InvasiveLinkedList<SDOMAttribute, SPrevAttOp, SNextAttOp> TAttributeList;

    struct SNamespacePair
    {
        CRegisteredString m_Namespace;
        CRegisteredString m_Abbreviation;
        SNamespacePair(CRegisteredString ns = CRegisteredString(),
                       CRegisteredString abbr = CRegisteredString())
            : m_Namespace(ns)
            , m_Abbreviation(abbr)
        {
        }
    };

    struct SNamespacePairNode : public SNamespacePair
    {
        SNamespacePairNode *m_NextNode;
        SNamespacePairNode(const SNamespacePair &inSrc = SNamespacePair())
            : SNamespacePair(inSrc)
            , m_NextNode(NULL)
        {
        }
    };

    // Simplified dom element.  All it has is one character value and list of attributes
    // and children.  So you can't mix elements and character data like you can in
    // full xml, but this simplifies parsing.
    struct SDOMElement
    {
        struct SNextElemOp
        {
            SDOMElement *get(SDOMElement &elem) { return elem.m_NextSibling; }
            const SDOMElement *get(const SDOMElement &elem) { return elem.m_NextSibling; }
            void set(SDOMElement &elem, SDOMElement *next) { elem.m_NextSibling = next; }
        };
        struct SPrevElemOp
        {
            SDOMElement *get(SDOMElement &elem) { return elem.m_PreviousSibling; }
            const SDOMElement *get(const SDOMElement &elem) { return elem.m_PreviousSibling; }
            void set(SDOMElement &elem, SDOMElement *prev) { elem.m_PreviousSibling = prev; }
        };
        typedef InvasiveLinkedList<SDOMElement, SPrevElemOp, SNextElemOp> TElementChildList;

        CRegisteredString m_Namespace;
        CRegisteredString m_Name;
        const char8_t *m_Value;
        SDOMElement *m_Parent;
        SDOMElement *m_NextSibling;
        SDOMElement *m_PreviousSibling;
        TAttributeList m_Attributes;
        TElementChildList m_Children;
        int m_Line;

        SDOMElement()
            : m_Value(NULL)
            , m_Parent(NULL)
            , m_NextSibling(NULL)
            , m_PreviousSibling(NULL)
            , m_Line(0)
        {
        }

        SDOMElement(CRegisteredString name, CRegisteredString ns, int inLine)
            : m_Namespace(ns)
            , m_Name(name)
            , m_Value(NULL)
            , m_Parent(NULL)
            , m_NextSibling(NULL)
            , m_PreviousSibling(NULL)
            , m_Line(inLine)
        {
        }

        void AppendChild(SDOMElement &inElem, SDOMElement *inPos = NULL)
        {
            if (inElem.m_Parent)
                inElem.m_Parent->RemoveChild(inElem);
            inElem.m_Parent = this;
            if (inPos) {
                QT3DS_ASSERT(inPos->m_Parent == this);
                m_Children.insert_after(*inPos, inElem);
            } else
                m_Children.push_back(inElem);
        }

        void PrependChild(SDOMElement &inElem, SDOMElement *inPos = NULL)
        {
            if (inElem.m_Parent)
                inElem.m_Parent->RemoveChild(inElem);
            inElem.m_Parent = this;
            if (inPos) {
                QT3DS_ASSERT(inPos->m_Parent == this);
                m_Children.insert_before(*inPos, inElem);
            } else
                m_Children.push_front(inElem);
        }

        void RemoveChild(SDOMElement &inElem)
        {
            if (inElem.m_Parent == this) {
                m_Children.remove(inElem);
            } else {
                if (inElem.m_Parent) {
                    QT3DS_ASSERT(false);
                }
            }
        }
        SDOMElement *FindNext(CRegisteredString inName, Option<CRegisteredString> inNamespace,
                              TElementChildList::iterator iter)
        {
            for (TElementChildList::iterator end(NULL); iter != end; ++iter) {
                if (inName.IsValid() == false || inName == iter->m_Name) {
                    if (inNamespace.hasValue() == false || *inNamespace == iter->m_Namespace)
                        return &(*iter);
                }
            }
            return NULL;
        }
        // Search starts just *after inStartPos if provided
        SDOMElement *FindChild(CRegisteredString inName, Option<CRegisteredString> inNamespace,
                               SDOMElement *inStartPos = NULL)
        {
            TElementChildList::iterator iter = m_Children.begin();
            if (inStartPos) {
                QT3DS_ASSERT(inStartPos->m_Parent == this);
                iter = TElementChildList::iterator(inStartPos->m_NextSibling);
            }
            return FindNext(inName, inNamespace, iter);
        }
        // Search starts just *after inStartPos if provided
        SDOMElement *FindSibling(CRegisteredString inName, Option<CRegisteredString> inNamespace)
        {
            TElementChildList::iterator iter(m_NextSibling);
            return FindNext(inName, inNamespace, iter);
        }

        SDOMAttribute *FindAttribute(CRegisteredString inName,
                                     Option<CRegisteredString> inNamespace)
        {
            for (TAttributeList::iterator iter = m_Attributes.begin(), end = m_Attributes.end();
                 iter != end; ++iter) {
                if (iter->m_Name == inName) {
                    if (inNamespace.hasValue() == false || *inNamespace == iter->m_Namespace)
                        return &(*iter);
                }
            }
            return NULL;
        }

        template <typename TDataType>
        bool AttValue(CRegisteredString inName, Option<CRegisteredString> inNamespace,
                      TDataType &outValue)
        {
            SDOMAttribute *att = FindAttribute(inName, inNamespace);
            if (att)
                return att->Value(outValue);
            return false;
        }

        QT3DSU32 GetNumChildren(CRegisteredString inName,
                             Option<CRegisteredString> inNamespace = Empty()) const
        {
            QT3DSU32 count = 0;
            if (inName.IsValid() == false) {
                // empty loop intentional
                for (TElementChildList::iterator iter = m_Children.begin(), end = m_Children.end();
                     iter != end; ++iter, ++count)
                    ;
            } else {
                for (TElementChildList::iterator iter = m_Children.begin(), end = m_Children.end();
                     iter != end; ++iter) {
                    if (iter->m_Name == inName
                        && (inNamespace.isEmpty() || iter->m_Namespace == *inNamespace))
                        ++count;
                }
            }
            return count;
        }

        void SetAttributeValue(CRegisteredString inName, Option<CRegisteredString> inNamespace,
                               TXMLCharPtr inValue, IDOMFactory &inFactory, int inLine)
        {
            SDOMAttribute *att = FindAttribute(inName, inNamespace);
            if (att) {
                att->m_Value = inFactory.RegisterValue(inValue);
            } else {
                m_Attributes.push_back(*inFactory.NextAttribute(
                    inName, inValue, inNamespace.unsafeGetValue(), inLine));
            }
        }

        void RemoveAttribute(CRegisteredString nm, Option<CRegisteredString> inNamespace)
        {
            SDOMAttribute *theAttribute = FindAttribute(nm, inNamespace);
            if (theAttribute)
                m_Attributes.remove(*theAttribute);
        }
    };

    struct SReaderScope
    {
        SDOMElement *m_Element;
        SDOMAttribute *m_Attribute;
        SReaderScope(SDOMElement *inElem = NULL, SDOMAttribute *inAtt = NULL)
            : m_Element(inElem)
            , m_Attribute(inAtt)
        {
        }
    };

    class IDOMReader : public NVRefCounted
    {
    protected:
        virtual ~IDOMReader() {}
    public:
        // Stack object to save the reader's state.
        struct Scope
        {
            IDOMReader &m_Reader;
            Scope(IDOMReader &reader)
                : m_Reader(reader)
            {
                m_Reader.PushScope();
            }
            Scope(NVScopedRefCounted<IDOMReader> reader)
                : m_Reader(*reader)
            {
                m_Reader.PushScope();
            }
            ~Scope() { m_Reader.PopScope(); }
        };

        // Parse buffer
        MemoryBuffer<ForwardingAllocator> m_TempBuf;
        NVScopedRefCounted<IStringTable> m_StringTable;

        IDOMReader(NVAllocatorCallback &inAlloc, NVScopedRefCounted<IStringTable> inStringTable)
            : m_TempBuf(ForwardingAllocator(inAlloc, "IDOMReader::m_TempBuf"))
            , m_StringTable(inStringTable)
        {
        }

        NVScopedRefCounted<IStringTable> GetStringTable() { return m_StringTable; }

        // Pushing scope saves your state so no matter where you are when
        // you next pop scope, you come back to the same state.
        virtual void PushScope() = 0;
        virtual void PopScope() = 0;

        // Sometimes pushing and popping scope isn't enough and you need
        // somewhat random access to scope.
        // This scope does not remember the current attribute.
        virtual SReaderScope GetScope() = 0;
        virtual void SetScope(SReaderScope inScope) = 0;
        // Return an attribute whose value is *not* registered with the string table.
        // You can't hold onto this value for any length of time, but when you need to
        // immediately convert to a different value this is the most efficient way.
        virtual bool UnregisteredAtt(TXMLCharPtr name, TXMLCharPtr &outValue,
                                     TXMLCharPtr ns = NULL) = 0;
        // Return an attribute whose value *is* registered with the string table.
        virtual bool Att(TXMLCharPtr name, CRegisteredString &outValue, TXMLCharPtr ns = NULL) = 0;
        virtual SDOMAttribute *GetFirstAttribute() = 0;
        virtual SDOMAttribute *GetNextAttribute() = 0;
        virtual QT3DSU32 CountChildren(TXMLCharPtr childName = NULL, TXMLCharPtr ns = NULL) = 0;
        virtual bool MoveToFirstChild(TXMLCharPtr childName = NULL, TXMLCharPtr ns = NULL) = 0;
        virtual bool MoveToNextSibling(TXMLCharPtr siblingName = NULL, TXMLCharPtr ns = NULL) = 0;
        // Leave element means go to its parent.
        virtual void Leave() = 0;
        virtual SDOMElement *GetElement() const = 0;
        CRegisteredString GetElementName() const
        {
            SDOMElement *elem = GetElement();
            if (elem)
                return elem->m_Name;
            return CRegisteredString();
        }

        // Value is the concatentated text node values inside the element
        virtual bool Value(TXMLCharPtr &outValue) = 0;

        // Get the element this reader was created with
        virtual SDOMElement *GetTopElement() = 0;

        bool Att(TXMLCharPtr name, TXMLStr &outValue)
        {
            TXMLCharPtr temp;
            if (UnregisteredAtt(name, temp)) {
                outValue.assign(temp);
                return true;
            }
            return false;
        }

        template <typename TDataType>
        bool Att(TXMLCharPtr name, TDataType &outValue)
        {
            TXMLCharPtr temp;
            if (UnregisteredAtt(name, temp)) {
                StringConversion<TDataType>().StrTo(temp, outValue);
                return true;
            } else {
                return false;
            }
        }

        bool ChildValue(TXMLCharPtr name, TXMLCharPtr &value)
        {
            if (MoveToFirstChild(name)) {
                Value(value);
                Leave();
                return true;
            }
            return false;
        }

        bool RegisteredChildValue(TXMLCharPtr name, TXMLCharPtr &value)
        {
            if (MoveToFirstChild(name)) {
                RegisteredValue(value);
                Leave();
                return true;
            }
            return false;
        }
        bool RegisteredValue(TXMLCharPtr &outValue)
        {
            bool retval = Value(outValue);
            if (retval)
                outValue = m_StringTable->RegisterStr(outValue);
            return retval;
        }

        template <typename TDataType>
        bool Value(TDataType &outValue)
        {
            TXMLCharPtr value;
            if (Value(value)) {
                StringConversion<TDataType>().StrTo(value, outValue);
                return true;
            }
            return false;
        }

        // IDOMReader implementations
        template <typename TDataType>
        bool ValueList(NVDataRef<TDataType> data)
        {
            const char8_t *value;
            if (Value(value)) {
                Char8TReader reader(const_cast<char8_t *>(value), m_TempBuf);
                reader.ReadRef(data);
            }
            return true;
        }

        // Destructive operation because we can't trust
        // strtod to do the right thing.  On windows, for long strings,
        // it calls strlen every operation thus leading to basically N^2
        // behavior
        template <typename TDataType>
        NVConstDataRef<TDataType> ChildValueList(TXMLCharPtr listName)
        {
            NVConstDataRef<TDataType> retval;
            TXMLCharPtr childValue = NULL;
            if (ChildValue(listName, childValue)) {
                Char8TReader reader(const_cast<char8_t *>(childValue), m_TempBuf);
                reader.ReadBuffer(retval);
            }
            return retval;
        }

        template <typename TSerializer>
        void Serialize(const char8_t *elemName, TSerializer &serializer,
                       const char8_t *inNamespace = NULL)
        {
            IDOMReader::Scope __theScope(*this);
            if (MoveToFirstChild(elemName, inNamespace)) {
                serializer.Serialize(*this);
            }
        }
        // Optionally hold on to the factory to keep our elements in memory as long as we are.
        static NVScopedRefCounted<IDOMReader> CreateDOMReader(
            NVAllocatorCallback &inAlloc, SDOMElement &inRootElement,
            NVScopedRefCounted<IStringTable> inStringTable,
            NVScopedRefCounted<IDOMFactory> inFactory = NVScopedRefCounted<IDOMFactory>());
    };

    // Write out data in an xml-like fasion without specifying exactly
    // where that data is being written.
    class IDOMWriter : public NVRefCounted
    {
    protected:
        virtual ~IDOMWriter() {}
    public:
        // Control the element scope.
        struct Scope
        {
            IDOMWriter &m_Writer;
            Scope(IDOMWriter &writer, TXMLCharPtr inElemName, TXMLCharPtr inNamespace = NULL)
                : m_Writer(writer)
            {
                m_Writer.Begin(inElemName, inNamespace);
            }
            ~Scope() { m_Writer.End(); }
        };

        char8_t m_NarrowBuf[256];

        // There tend to be two types of elements.
        // Containers (<a>\n\t<b/><b/>\n</a>)
        // and Values <b>onetwothree</b>
        virtual void Begin(TXMLCharPtr inElemName, TXMLCharPtr inNamespace = NULL,
                           int inLine = 0) = 0;
        // Attributes.  They may be sorted just before write
        virtual void Att(TXMLCharPtr name, TXMLCharPtr value, TXMLCharPtr inNamespace = NULL,
                         int inLine = 0) = 0;
        virtual void Value(TXMLCharPtr value) = 0;
        virtual void End() = 0;
        virtual void RemoveCurrent() = 0;
        virtual void RemoveAttribute(TXMLCharPtr inItem, TXMLCharPtr inNamespace = NULL) = 0;
        // Get the number of tabs required to line up the next line
        // with the opening of the previous line
        virtual QT3DSU32 GetTabs() = 0;
        virtual SDOMElement *GetTopElement() = 0;
        virtual NVScopedRefCounted<IDOMFactory> GetFactory() = 0;
        // Move this item before this sibling.  Function does not rearrange the
        // tree in any major way and will not work if inItem and inSibling aren't
        // siblings.
        virtual void MoveBefore(TXMLCharPtr inItem, TXMLCharPtr inSibling,
                                TXMLCharPtr inNamespace = NULL) = 0;
        virtual NVAllocatorCallback &GetAllocator() = 0;

        virtual void ChildValue(TXMLCharPtr name, TXMLCharPtr value, TXMLCharPtr inNamespace = NULL)
        {
            Begin(name, inNamespace);
            Value(value);
            End();
        }

        TXMLCharPtr ToStr(char8_t val)
        {
            m_NarrowBuf[0] = val;
            m_NarrowBuf[1] = 0;
            return m_NarrowBuf;
        }

        template <typename TDataType>
        TXMLCharPtr ToStr(TDataType val)
        {
            StringConversion<TDataType>().ToStr(val, NVDataRef<char8_t>(m_NarrowBuf, 256));
            return m_NarrowBuf;
        }

        void Att(TXMLCharPtr name, const TXMLStr &inValue, TXMLCharPtr inNamespace = NULL)
        {
            return Att(name, inValue.c_str(), inNamespace);
        }

        template <typename TData>
        void Att(TXMLCharPtr name, TData value, TXMLCharPtr inNamespace = NULL)
        {
            Att(name, ToStr(value), inNamespace);
        }

        template <typename TSerializer>
        void Serialize(const char8_t *elemName, TSerializer &serializer,
                       TXMLCharPtr inNamespace = NULL)
        {
            IDOMWriter::Scope __theScope(*this, elemName, inNamespace);
            serializer.Serialize(*this);
        }

        NVScopedRefCounted<IDOMReader> CreateDOMReader()
        {
            NVScopedRefCounted<IDOMFactory> theFactory(GetFactory());
            return IDOMReader::CreateDOMReader(GetAllocator(), *GetTopElement(),
                                               theFactory->GetStringTable(), theFactory);
        }

        // Note that the default method of creating a writer also creates a reader; they can
        // both manipulation the DOM hierarch.
        static eastl::pair<NVScopedRefCounted<IDOMWriter>, NVScopedRefCounted<IDOMReader>>
        CreateDOMWriter(NVScopedRefCounted<IDOMFactory> inFactory, SDOMElement &inRootElement,
                        NVScopedRefCounted<IStringTable> inStringTable);

        static eastl::pair<NVScopedRefCounted<IDOMWriter>, NVScopedRefCounted<IDOMReader>>
        CreateDOMWriter(NVAllocatorCallback &inAlloc, const char8_t *inTopElemName,
                        NVScopedRefCounted<IStringTable> inStringTable,
                        const char8_t *inNamespace = NULL)
        {
            NVScopedRefCounted<IDOMFactory> theFactory(
                IDOMFactory::CreateDOMFactory(inAlloc, inStringTable));
            SDOMElement *theRoot =
                theFactory->NextElement(theFactory->GetStringTable()->RegisterStr(inTopElemName),
                                        theFactory->GetStringTable()->RegisterStr(inNamespace));
            return CreateDOMWriter(theFactory, *theRoot, inStringTable);
        }
    };

    class CXmlErrorHandler
    {
    protected:
        virtual ~CXmlErrorHandler() {}
    public:
        virtual void OnXmlError(TXMLCharPtr errorName, int line, int column) = 0;
    };

    class CDOMSerializer
    {
    public:
        static void WriteXMLHeader(IOutStream &inStream);

        // Write out the elements.  These pairs will be used in order to abbreviate namespaces with
        // nicer abbreviations.
        // Any namespaces not found in those pairs will get auto-generated abbreviations
        // The document will be assumed to be in the namespace that has no abbreviation.
        // For fragments or snippets, it is better to avoid writing out the namespace declarations
        static void
        Write(NVAllocatorCallback &inAlloc, SDOMElement &inElement, IOutStream &inStream,
              IStringTable &inStrTable,
              NVConstDataRef<SNamespacePair> inNamespaces = NVConstDataRef<SNamespacePair>(),
              bool inTrimEmptyElements = true, QT3DSU32 inTabs = 0, bool inWriteNamespaces = true);

        // Returns a pair of the namespace pair nodes and the dom element.  The ns pair nodes allow
        // us to at least keep the same abbreviations
        // used if namespace are used.
        static eastl::pair<SNamespacePairNode *, SDOMElement *>
        Read(IDOMFactory &inFactory, IInStream &inStream, CXmlErrorHandler *inErrorHandler = NULL);
    };
}
}
#endif
