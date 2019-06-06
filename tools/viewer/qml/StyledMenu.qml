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

import QtQuick 2.7
import QtQuick.Controls 2.2

Menu {
    id: control

    property alias hovered: menuArea.containsMouse

    width: contentItem.width + leftPadding + rightPadding
    height: contentItem.height + topPadding + bottomPadding
    padding: 1 // For background border
    x: 0
    y: parent.height
    closePolicy: Popup.CloseOnPressOutsideParent | Popup.CloseOnEscape

    contentItem: MouseArea {
        id: menuArea
        hoverEnabled: true
        height: list.height
        width: list.width
        ListView {
            id: list
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: control.contentModel
            currentIndex: control.currentIndex
            highlightRangeMode: ListView.ApplyRange
            highlightMoveDuration: 0
            Component.onCompleted: {
                var maxItemWidth = 0;
                var maxShortcutWidth = 0;
                var totalHeight = 0
                var extraWidth = 0
                var i;
                for (i = control.contentData.length - 1; i >= 0; --i) {
                    if (control.contentData[i].itemWidth !== undefined) {
                        maxItemWidth = Math.max(maxItemWidth, control.contentData[i].itemWidth);
                        maxShortcutWidth = Math.max(maxShortcutWidth,
                                                    control.contentData[i].shortcutWidth);
                    }
                    totalHeight += control.contentData[i].height
                }
                maxItemWidth += _controlPadding // minimum item spacer
                for (i = control.contentData.length - 1; i >= 0; --i) {
                    if (control.contentData[i].itemSpacerWidth !== undefined) {
                        control.contentData[i].itemSpacerWidth
                                = maxItemWidth - control.contentData[i].itemWidth;
                        control.contentData[i].shortcutSpacerWidth
                                = maxShortcutWidth - control.contentData[i].shortcutWidth;
                    }
                }
                width = maxItemWidth + maxShortcutWidth + extraWidth
                        + control.contentData[0].leftPadding + control.contentData[0].rightPadding
                        + control.contentData[0].arrowWidth + control.contentData[0].checkMarkWidth;
                height = totalHeight
            }
        }
    }

    background: Rectangle {
        width: control.width
        height: control.height
        color: _menuBackgroundColor
        border.color: _menuBorderColor
    }
}
