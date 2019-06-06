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

#include "foundation/Qt3DSLogging.h"

namespace qt3ds {

Q_LOGGING_CATEGORY(GL_ERROR, "qt3ds.gl_error")
Q_LOGGING_CATEGORY(INVALID_PARAMETER, "qt3ds.invalid_parameter")
Q_LOGGING_CATEGORY(INVALID_OPERATION, "qt3ds.invalid_operation")
Q_LOGGING_CATEGORY(OUT_OF_MEMORY, "qt3ds.out_of_memory")
Q_LOGGING_CATEGORY(INTERNAL_ERROR, "qt3ds.internal_error")
Q_LOGGING_CATEGORY(PERF_WARNING, "qt3ds.perf_warning")
Q_LOGGING_CATEGORY(PERF_INFO, "qt3ds.perf_info")
Q_LOGGING_CATEGORY(TRACE_INFO, "qt3ds.trace_info")
Q_LOGGING_CATEGORY(WARNING, "qt3ds.warning")

} // namespace qt3ds
