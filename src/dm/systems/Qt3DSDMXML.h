/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DS_IMPORT_XML_H
#define QT3DS_IMPORT_XML_H
#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSMemoryBuffer.h"
#include "Qt3DSDMDataTypes.h"
#include <string>
#include "Qt3DSDMWStrOps.h" //Conversion between string and various datatypes.
#include "foundation/Qt3DSFlags.h"
#include "EASTL/algorithm.h"
#include "foundation/Qt3DSSimpleTypes.h"
#include "foundation/IOStreams.h"
#include "Qt3DSDMValue.h"
#include <memory>

namespace qt3dsdm {
using qt3ds::foundation::MemoryBuffer;
using qt3ds::foundation::RawAllocator;
class IStringTable;

class IDOMFactory;
struct SDOMAttribute;
struct SDOMElement;

typedef const char8_t *TXMLCharPtr;
typedef const wchar_t *TWideXMLCharPtr;

typedef TCharStr TXMLWideStr;
typedef eastl::basic_string<char8_t> TXMLStr;

using qt3ds::QT3DSU32;
using qt3ds::NVFlags;
using qt3ds::foundation::NVDataRef;
using qt3ds::foundation::NVConstDataRef;
using qt3ds::foundation::IOutStream;

class IDOMFactory
{
protected:
    virtual ~IDOMFactory() {}
public:
    friend class std::shared_ptr<IDOMFactory>;

    virtual void Release() = 0;
    // Str does not need to be null terminated.
    virtual void AppendStrBuf(TXMLCharPtr str, QT3DSU32 len) = 0;
    virtual void AppendStrBuf(TWideXMLCharPtr str, QT3DSU32 len) = 0;
    // Null terminate what is there and return the buffer.
    // This pointer needs to be persistent.
    virtual TXMLCharPtr FinalizeStrBuf() = 0;
    virtual void IgnoreStrBuf() = 0;

    virtual std::shared_ptr<IStringTable> GetStringTable() = 0;

    virtual SDOMAttribute *NextAttribute(TWideXMLCharPtr name, TWideXMLCharPtr val) = 0;
    virtual SDOMElement *NextElement(TWideXMLCharPtr name) = 0;

    virtual SDOMAttribute *NextAttribute(TXMLCharPtr name, TXMLCharPtr val) = 0;
    virtual SDOMElement *NextElement(TXMLCharPtr name) = 0;

    TXMLCharPtr RegisterValue(TXMLCharPtr inValue);
    TXMLCharPtr RegisterValue(TWideXMLCharPtr inValue);

    static std::shared_ptr<IDOMFactory>
    CreateDOMFactory(std::shared_ptr<IStringTable> inStrTable);
};

struct DOMFlagValues
{
    enum Enum {
        CaselessElementNames = 1 << 0,
        CaselessAttributeNames = 1 << 1,
    };
};

struct SDOMFlags : NVFlags<DOMFlagValues::Enum, int>
{
    SDOMFlags()
        : NVFlags<DOMFlagValues::Enum, int>(DOMFlagValues::CaselessElementNames
                                            | DOMFlagValues::CaselessAttributeNames)
    {
    }
    bool CaselessAttributes() const { return *this & DOMFlagValues::CaselessAttributeNames; }
    bool CaselessElements() const { return *this & DOMFlagValues::CaselessElementNames; }
};

class IDOMReader
{
protected:
    virtual ~IDOMReader() {}
public:
    friend class std::shared_ptr<IDOMReader>;

    // Stack object to save the reader's state.
    struct Scope
    {
        IDOMReader &m_Reader;
        Scope(IDOMReader &reader)
            : m_Reader(reader)
        {
            m_Reader.PushScope();
        }
        ~Scope() { m_Reader.PopScope(); }
    };

    // Parse buffer
    MemoryBuffer<RawAllocator> m_TempBuf;
    std::shared_ptr<IStringTable> m_StringTable;

    IDOMReader(std::shared_ptr<IStringTable> inStringTable)
        : m_StringTable(inStringTable)
    {
    }

    std::shared_ptr<IStringTable> GetStringTable() { return m_StringTable; }

    // DOM reader flags change the nature of the DOM reader.
    // see DOMFlagValues
    // Defaults to caseless comparisons for attributes and for
    // element names.
    virtual void SetDOMFlags(SDOMFlags inFlags) = 0;
    virtual SDOMFlags GetDOMFlags() const = 0;

    // Pushing scope saves your state so no matter where you are when
    // you next pop scope, you come back to the same state.
    virtual void PushScope() = 0;
    virtual void PopScope() = 0;

    // Sometimes pushing and popping scope isn't enough and you need
    // somewhat random access to scope.
    // This scope does not remember the current attribute.
    virtual void *GetScope() = 0;
    virtual void SetScope(void *inScope) = 0;
    // Return an attribute whose value is *not* registered with the string table.
    // You can't hold onto this value for any length of time, but when you need to
    // immediately convert to a different value this is the most efficient way.
    virtual bool UnregisteredAtt(TWideXMLCharPtr name, TWideXMLCharPtr &outValue) = 0;
    virtual bool UnregisteredAtt(TXMLCharPtr name, TXMLCharPtr &outValue) = 0;
    // Return an attribute whose value *is* registered with the string table.
    virtual bool Att(TWideXMLCharPtr name, TWideXMLCharPtr &outValue) = 0;
    virtual bool Att(TXMLCharPtr name, TXMLCharPtr &outValue) = 0;
    virtual eastl::pair<TWideXMLCharPtr, TWideXMLCharPtr> GetFirstAttribute() = 0;
    virtual eastl::pair<TWideXMLCharPtr, TWideXMLCharPtr> GetNextAttribute() = 0;
    virtual eastl::pair<TXMLCharPtr, TXMLCharPtr> GetNarrowFirstAttribute() = 0;
    virtual eastl::pair<TXMLCharPtr, TXMLCharPtr> GetNarrowNextAttribute() = 0;
    virtual QT3DSU32 CountChildren() = 0;
    virtual QT3DSU32 CountChildren(TWideXMLCharPtr childName) = 0;
    virtual QT3DSU32 CountChildren(TXMLCharPtr childName) = 0;
    virtual bool MoveToFirstChild(TWideXMLCharPtr childName) = 0;
    virtual bool MoveToFirstChild(TXMLCharPtr childName) = 0;
    virtual bool MoveToFirstChild() = 0;
    virtual bool MoveToNextSibling(TWideXMLCharPtr siblingName) = 0;
    virtual bool MoveToNextSibling(TXMLCharPtr siblingName) = 0;
    virtual bool MoveToNextSibling() = 0;
    // Leave element means go to its parent.
    virtual void Leave() = 0;
    virtual TWideXMLCharPtr GetElementName() const = 0;
    virtual TXMLCharPtr GetNarrowElementName() const = 0;

    // Value is the concatentated text node values inside the element
    virtual bool Value(TWideXMLCharPtr &outValue) = 0;
    virtual bool Value(TXMLCharPtr &outValue) = 0;

    // Get the element this reader was created with
    virtual SDOMElement *GetTopElement() = 0;

    bool Att(TWideXMLCharPtr name, TXMLWideStr &outValue)
    {
        TWideXMLCharPtr temp;
        if (UnregisteredAtt(name, temp)) {
            outValue.assign(temp);
            return true;
        }
        return false;
    }

    bool Att(TXMLCharPtr name, TXMLStr &outValue)
    {
        TXMLCharPtr temp;
        if (UnregisteredAtt(name, temp)) {
            outValue.assign(temp);
            return true;
        }
        return false;
    }

    // Helpers to help make the parsing a bit easier.
    template <typename TDataType>
    bool Att(TWideXMLCharPtr name, TDataType &outValue)
    {
        TWideXMLCharPtr temp;
        if (UnregisteredAtt(name, temp)) {
            WStrOps<TDataType>().StrTo(temp, outValue);
            return true;
        } else {
            return false;
        }
    }
    template <typename TDataType>
    bool Att(TXMLCharPtr name, TDataType &outValue)
    {
        TXMLCharPtr temp;
        if (UnregisteredAtt(name, temp)) {
            WStrOps<TDataType>().StrTo(temp, outValue);
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

    bool ChildValue(TWideXMLCharPtr name, TWideXMLCharPtr &value)
    {
        if (MoveToFirstChild(name)) {
            Value(value);
            Leave();
            return true;
        }
        return false;
    }

    bool RegisteredChildValue(TWideXMLCharPtr name, TWideXMLCharPtr &value)
    {
        if (MoveToFirstChild(name)) {
            RegisteredValue(value);
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
    bool RegisteredValue(TWideXMLCharPtr &outValue)
    {
        bool retval = Value(outValue);
        if (retval)
            outValue = m_StringTable->RegisterStr(outValue);
        return retval;
    }
    bool RegisteredValue(TXMLCharPtr &outValue)
    {
        bool retval = Value(outValue);
        if (retval)
            outValue = m_StringTable->RegisterStr(outValue);
        return retval;
    }
    bool Value(DataModelDataType::Value type, SValue &outValue);

    template <typename TDataType>
    bool Value(TDataType &outValue)
    {
        TXMLCharPtr value;
        if (Value(value)) {
            WStrOps<TDataType>().StrTo(value, outValue);
            return true;
        }
        return false;
    }

    // Functions below implemented in WStrOpsImpl
    // to avoid circular dependencies
    // Returns the number of items read
    // Destructive parsing
    template <typename TDataType>
    bool ValueList(NVDataRef<TDataType> data);

    // Destructive operation because we can't trust
    // strtod to do the right thing.  On windows, for long strings,
    // it calls strlen every operation thus leading to basically N^2
    // behavior
    template <typename TDataType>
    NVConstDataRef<TDataType> ChildValueList(TWideXMLCharPtr listName);

    template <typename TDataType>
    NVConstDataRef<TDataType> ChildValueList(TXMLCharPtr listName);

    template <typename TSerializer>
    void Serialize(const wchar_t *elemName, TSerializer &serializer)
    {
        IDOMReader::Scope __theScope(*this);
        if (MoveToFirstChild(elemName)) {
            serializer.Serialize(*this);
        }
    }
    // Optionally hold on to the factory to keep our elements in memory as long as we are.
    static std::shared_ptr<IDOMReader>
    CreateDOMReader(SDOMElement &inRootElement, std::shared_ptr<IStringTable> inStringTable,
                    std::shared_ptr<IDOMFactory> inFactory = std::shared_ptr<IDOMFactory>());
};

// Write out data in an xml-like fasion without specifying exactly
// where that data is being written.
class IDOMWriter
{
protected:
    virtual ~IDOMWriter() {}
public:
    friend class std::shared_ptr<IDOMWriter>;

    // Control the element scope.
    struct Scope
    {
        IDOMWriter &m_Writer;
        Scope(IDOMWriter &writer, TWideXMLCharPtr inElemName)
            : m_Writer(writer)
        {
            m_Writer.Begin(inElemName);
        }
        Scope(IDOMWriter &writer, TXMLCharPtr inElemName)
            : m_Writer(writer)
        {
            m_Writer.Begin(inElemName);
        }
        ~Scope() { m_Writer.End(); }
    };

    wchar_t m_PrintBuf[256];
    char8_t m_NarrowBuf[256];

    virtual void SetDOMFlags(SDOMFlags inFlags) = 0;
    virtual SDOMFlags GetDOMFlags() const = 0;
    // There tend to be two types of elements.
    // Containers (<a>\n\t<b/><b/>\n</a>)
    // and Values <b>onetwothree</b>
    virtual void Begin(TWideXMLCharPtr inElemName) = 0;
    virtual void Begin(TXMLCharPtr inElemName) = 0;
    // Attributes.  They may be sorted just before write
    virtual void Att(TWideXMLCharPtr name, TWideXMLCharPtr value) = 0;
    virtual void Att(TXMLCharPtr name, TXMLCharPtr value) = 0;
    virtual void Value(TWideXMLCharPtr value) = 0;
    virtual void Value(TXMLCharPtr value) = 0;
    virtual void End() = 0;
    virtual void RemoveCurrent() = 0;
    virtual void ReplaceCurrent(SDOMElement &inElement) = 0;
    // Append all the children of inElement to the child list of the current items.
    virtual void AppendChildren(SDOMElement &inElement) = 0;
    virtual void RemoveAttribute(TWideXMLCharPtr inItem) = 0;
    virtual void RemoveAttribute(TXMLCharPtr inItem) = 0;
    // Get the number of tabs required to line up the next line
    // with the opening of the previous line
    virtual QT3DSU32 GetTabs() = 0;
    virtual SDOMElement *GetTopElement() = 0;
    virtual std::shared_ptr<IDOMFactory> GetFactory() = 0;
    // Move this item before this sibling.  Function does not rearrange the
    // tree in any major way and will not work if inItem and inSibling aren't
    // siblings.
    virtual void MoveBefore(TWideXMLCharPtr inItem, TWideXMLCharPtr inSibling) = 0;
    virtual void MoveBefore(TXMLCharPtr inItem, TXMLCharPtr inSibling) = 0;

    virtual void ChildValue(TWideXMLCharPtr name, TWideXMLCharPtr value)
    {
        Begin(name);
        Value(value);
        End();
    }

    virtual void ChildValue(TXMLCharPtr name, TXMLCharPtr value)
    {
        Begin(name);
        Value(value);
        End();
    }

    TWideXMLCharPtr ToStr(wchar_t val)
    {
        m_PrintBuf[0] = val;
        m_PrintBuf[1] = 0;
        return m_PrintBuf;
    }
    TXMLCharPtr ToStr(char8_t val)
    {
        m_NarrowBuf[0] = val;
        m_NarrowBuf[1] = 0;
        return m_NarrowBuf;
    }
    template <typename TDataType>
    TWideXMLCharPtr ToStr(TDataType val)
    {
        WStrOps<TDataType>().ToStr(val, NVDataRef<wchar_t>(m_PrintBuf, 256));
        return m_PrintBuf;
    }

    template <typename TDataType>
    TXMLCharPtr ToNarrowStr(TDataType val)
    {
        WStrOps<TDataType>().ToStr(val, NVDataRef<char8_t>(m_NarrowBuf, 256));
        return m_NarrowBuf;
    }

    void Att(TWideXMLCharPtr name, const TXMLWideStr &inValue)
    {
        return Att(name, inValue.wide_str());
    }

    void Att(TXMLCharPtr name, const TXMLStr &inValue) { return Att(name, inValue.c_str()); }

    template <typename TData>
    void Att(TWideXMLCharPtr name, TData value)
    {
        Att(name, ToStr(value));
    }

    template <typename TData>
    void Att(TXMLCharPtr name, TData value)
    {
        Att(name, ToNarrowStr(value));
    }

    template <typename TSerializer>
    void Serialize(const wchar_t *elemName, TSerializer &serializer)
    {
        IDOMWriter::Scope __theScope(*this, elemName);
        serializer.Serialize(*this);
    }

    template <typename TSerializer>
    void Serialize(const char8_t *elemName, TSerializer &serializer)
    {
        IDOMWriter::Scope __theScope(*this, elemName);
        serializer.Serialize(*this);
    }

    std::shared_ptr<IDOMReader> CreateDOMReader()
    {
        std::shared_ptr<IDOMFactory> theFactory(GetFactory());
        return IDOMReader::CreateDOMReader(*GetTopElement(), theFactory->GetStringTable(),
                                           theFactory);
    }

    // Note that the default method of creating a writer also creates a reader; they can
    // both manipulation the DOM hierarch.
    static eastl::pair<std::shared_ptr<IDOMWriter>, std::shared_ptr<IDOMReader>>
    CreateDOMWriter(std::shared_ptr<IDOMFactory> inFactory, SDOMElement &inRootElement,
                    std::shared_ptr<IStringTable> inStringTable);

    static eastl::pair<std::shared_ptr<IDOMWriter>, std::shared_ptr<IDOMReader>>
    CreateDOMWriter(const wchar_t *inTopElemName, std::shared_ptr<IStringTable> inStringTable)
    {
        std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(inStringTable));
        SDOMElement *theRoot = theFactory->NextElement(inTopElemName);
        return CreateDOMWriter(theFactory, *theRoot, inStringTable);
    }
};

class CXmlErrorHandler
{
protected:
    virtual ~CXmlErrorHandler() {}
public:
    virtual void OnXmlError(const QString &errorName, int line, int column) = 0;
};

class CDOMSerializer
{
public:
    static void WriteXMLHeader(IOutStream &inStream);
    static void Write(SDOMElement &inElement, IOutStream &inStream, QT3DSU32 inTabs = 0);
    static SDOMElement *Read(IDOMFactory &inFactory, qt3ds::foundation::IInStream &inStream,
                             CXmlErrorHandler *inErrorHandler = NULL);
};
}
#endif
