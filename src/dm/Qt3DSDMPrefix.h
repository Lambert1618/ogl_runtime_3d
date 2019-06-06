/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifdef __cplusplus
#pragma once

#ifndef _WIN32_WINNT // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT                                                                               \
    0x0501 // Change this to the appropriate value to target other versions of Windows.
#endif

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

// TODO: reference additional headers your program requires here
#if defined(WIN32) && defined(MSVC)
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4512)
#pragma warning(disable : 4702)
#pragma warning(disable : 4996)
#endif

// std includes
#include <map>
#include <vector>
#include <string>
#include <exception>
#include <set>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <functional>

// Project includes
#include "StandardExtensions.h"

namespace qt3dsdm {
using std::ref;
using std::get;
using std::bind;
using std::tuple;
using std::static_pointer_cast;
using std::make_pair;
using std::equal_to;
using std::vector;
using std::make_tuple;
using std::function;
using std::shared_ptr;
using std::make_shared;
}
#if defined(WIN32) && defined(MSVC)
#pragma warning(pop)
#endif
#endif
