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

#include "Qt3DSDistanceFieldGlyphCacheManager_p.h"
#include "Qt3DSDistanceFieldGlyphCache_p.h"

#include <QtQuick/private/qsgdefaultrendercontext_p.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,12,2)

QT_BEGIN_NAMESPACE

namespace {
    class FontKeyAccessor : public QSGDefaultRenderContext
    {
    public:
        static QString fontKey(const QRawFont &font)
        {
            return QSGDefaultRenderContext::fontKey(font);
        }
    };
}

Q3DSDistanceFieldGlyphCacheManager::~Q3DSDistanceFieldGlyphCacheManager()
{
    for (auto &cache : qAsConst(m_glyphCaches))
        delete cache;
}

Q3DSDistanceFieldGlyphCache *Q3DSDistanceFieldGlyphCacheManager::glyphCache(const QRawFont &font)
{
    QString key = FontKeyAccessor::fontKey(font);
    Q3DSDistanceFieldGlyphCache *cache = m_glyphCaches.value(key);
    if (cache == nullptr) {
        cache = new Q3DSDistanceFieldGlyphCache(font, *m_context);
        m_glyphCaches.insert(key, cache);
    }

    return cache;
}

void Q3DSDistanceFieldGlyphCacheManager::setContext(qt3ds::render::IQt3DSRenderContext &context)
{
    m_context = &context;
}

QT_END_NAMESPACE

#endif // QT_VERSION >= QT_VERSION_CHECK(5,12,2)
