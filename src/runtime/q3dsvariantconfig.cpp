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
#include "q3dsvariantconfig_p.h"

namespace qt3ds {

Q3DSVariantConfig::Q3DSVariantConfig()
{

}

Q3DSVariantConfig::~Q3DSVariantConfig()
{
    for (auto str : qAsConst(m_internalVariantList))
        delete str;
    m_internalVariantList.clear();
}

void Q3DSVariantConfig::setVariantList(const QStringList &variantList)
{
    //  Store the QStringList to be returned from the API
    m_variantList = variantList;

    for (auto str : qAsConst(m_internalVariantList))
        delete str;
    m_internalVariantList.clear();

    // Build a fixed (in mem location) list of the variant strings
    for (auto tag : variantList)
        m_internalVariantList.append(new QString(tag));

    // Parse the variantGroup:variant list to map using the fixed list
    m_variantFilter.clear();
    for (auto tag : qAsConst(m_internalVariantList)) {
        QStringRef refTag = QStringRef(tag);
        int separatorIdx = refTag.indexOf(QLatin1Char(':'));
        QStringRef group = refTag.left(separatorIdx);
        QStringRef variant = refTag.mid(separatorIdx + 1);
        m_variantFilter[group].append(variant);
    }
}

bool Q3DSVariantConfig::isPartOfConfig(const QStringRef &variantAttributes) const
{
    // Variant filter is ignored when it's not defined
    // or if the object has no variant attributes
    if (m_variantFilter.isEmpty() || variantAttributes.isEmpty())
        return true;

    // Collect all variant tags per group from the element
    QHash<QStringRef, QVector<QStringRef>> groupToVariants;
    const QVector<QStringRef> variantTags = variantAttributes.split(
                QLatin1Char(','),
                QString::SkipEmptyParts);

    for (auto tag : variantTags) {
        // Break each variantGroup:value to group and value strings
        int groupSeparatorIdx = tag.indexOf(QLatin1Char(':'));
        QStringRef group = tag.left(groupSeparatorIdx);

        // Only collect variant tags that are relevant to variant filtering
        if (m_variantFilter.contains(group)) {
            QStringRef variant = tag.mid(groupSeparatorIdx + 1);
            groupToVariants[group].append(variant);
        }
    }

    // If no relevant variant tags found in element, load the element
    if (groupToVariants.isEmpty())
        return true;

    // Verify that the element matches the variant filtering per group.
    // To match the element must either:
    // - Have no variant tag defined for the group
    // - Have a matching tag for the group
    bool isLoaded = true;
    const auto filteredGroups = m_variantFilter.keys();
    for (auto group : filteredGroups) {
        const QVector<QStringRef> variants = groupToVariants[group];
        const QVector<QStringRef> filteredVariants = m_variantFilter[group];

        if (variants.size() > 0) {
            // Check if ANY of the variant values of the element matches ANY
            // of the included variants in the filter for this variant group
            bool matchesFilter = false;
            for (auto variant : variants)
                matchesFilter |= filteredVariants.contains(variant);

            isLoaded &= matchesFilter;
        }
    }

    return isLoaded;
}

}
