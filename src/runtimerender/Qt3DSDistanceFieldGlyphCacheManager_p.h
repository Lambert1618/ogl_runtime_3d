/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#ifndef Q3DSDISTANCEFIELDGLYPHCACHEMANAGER_P_H
#define Q3DSDISTANCEFIELDGLYPHCACHEMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qhash.h>
#include <QtGui/qrawfont.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)

namespace qt3ds {
namespace render {
class IQt3DSRenderContext;
}
}

QT_BEGIN_NAMESPACE

class Q3DSDistanceFieldGlyphCache;
class Q3DSDistanceFieldGlyphCacheManager
{
public:
    ~Q3DSDistanceFieldGlyphCacheManager();
    Q3DSDistanceFieldGlyphCache *glyphCache(const QRawFont &font);
    void setContext(qt3ds::render::IQt3DSRenderContext &context);

private:
    QHash<QString, Q3DSDistanceFieldGlyphCache *> m_glyphCaches;
    qt3ds::render::IQt3DSRenderContext *m_context;

};

QT_END_NAMESPACE

#endif // QT_VERSION >= QT_VERSION_CHECK(5,12,2)

#endif // Q3DSDISTANCEFIELDGLYPHCACHEMANAGER_P_H
