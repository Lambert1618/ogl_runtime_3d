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
#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMStringTable.h"
#include "foundation/StringTable.h"
#include "foundation/TrackingAllocator.h"

using namespace qt3dsdm;
using namespace eastl;
using namespace qt3ds;

namespace {

struct StringTableImpl : public IStringTable
{
    qt3ds::foundation::MallocAllocator m_MallocAllocator;
    qt3ds::foundation::IStringTable &m_StringTable;
    StringTableImpl()
        : m_StringTable(qt3ds::foundation::IStringTable::CreateStringTable(m_MallocAllocator))
    {
        m_StringTable.addRef();
    }
    StringTableImpl(qt3ds::foundation::IStringTable &inStrTable)
        : m_StringTable(inStrTable)
    {
        m_StringTable.addRef();
    }

    virtual ~StringTableImpl() { m_StringTable.release(); }
    const wchar_t *RegisterStr(const wchar_t *inStr) override
    {
        return m_StringTable.GetWideStr(inStr);
    }
    const char8_t *RegisterStr(const char8_t *inStr) override
    {
        return m_StringTable.GetNarrowStr(inStr);
    }
    // Get the utf-8 or utf-(sizeof wchar_t) converted strings
    const wchar_t *GetWideStr(const char8_t *inStr) override
    {
        return m_StringTable.GetWideStr(inStr);
    }
    const char8_t *GetNarrowStr(const wchar_t *inStr) override
    {
        return m_StringTable.GetNarrowStr(inStr);
    }
    qt3ds::foundation::IStringTable &GetRenderStringTable() override { return m_StringTable; }
};
}

TStringTablePtr IStringTable::CreateStringTable()
{
    return std::make_shared<StringTableImpl>();
}

TStringTablePtr IStringTable::CreateStringTable(qt3ds::foundation::IStringTable &inStrTable)
{
    return std::make_shared<StringTableImpl>(std::ref(inStrTable));
}
