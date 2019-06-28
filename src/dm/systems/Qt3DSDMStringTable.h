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
#ifndef QT3DS_IMPORT_STRINGTABLE_H
#define QT3DS_IMPORT_STRINGTABLE_H

#include <string>
#include <memory>
#include <EABase/eabase.h>
#include "foundation/Qt3DSPreprocessor.h"

namespace qt3ds {
namespace foundation {
    class IStringTable;
}
}

namespace qt3dsdm {

class QT3DS_AUTOTEST_EXPORT IStringTable
{
public:
    virtual ~IStringTable() {}

    virtual const wchar_t *RegisterStr(const wchar_t *inStr) = 0;
    virtual const char8_t *RegisterStr(const char8_t *inStr) = 0;
    // Get the utf-8 or utf-(sizeof wchar_t) converted strings
    virtual const wchar_t *GetWideStr(const char8_t *inStr) = 0;
    virtual const char8_t *GetNarrowStr(const wchar_t *inStr) = 0;

    const wchar_t *GetWideStr(const wchar_t *inStr) { return RegisterStr(inStr); }
    const char8_t *GetNarrowStr(const char8_t *inStr) { return RegisterStr(inStr); }

    const wchar_t *RegisterStr(const std::wstring &inStr) { return RegisterStr(inStr.c_str()); }
    const char8_t *RegisterStr(const std::string &inStr) { return RegisterStr(inStr.c_str()); }

    virtual qt3ds::foundation::IStringTable &GetRenderStringTable() = 0;

    static std::shared_ptr<IStringTable> CreateStringTable();
    static std::shared_ptr<IStringTable>
    CreateStringTable(qt3ds::foundation::IStringTable &inStrTable);
};
typedef std::shared_ptr<IStringTable> TStringTablePtr;
};

#endif
