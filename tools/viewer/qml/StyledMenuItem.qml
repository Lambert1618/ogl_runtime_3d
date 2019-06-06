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

MenuItem {
    id: control

    property alias shortcut: shortcut.sequence
    property string shortcutText
    property alias itemSpacerWidth: itemSpacer.width
    property alias shortcutSpacerWidth: shortcutSpacer.width
    property alias itemWidth: itemLabel.width
    property alias shortcutWidth: shortcutLabel.width
    property bool showArrow: false
    property int arrowWidth: arrow.width
    property int checkMarkWidth: checkMark.width
    property bool showCheckMark: false
    property Menu arrowMenu: null

    hoverEnabled: true
    width: contentItem.width + leftPadding + rightPadding
    height: contentItem.height + topPadding + bottomPadding
    padding: 0
    leftPadding: 0
    rightPadding: 0

    MouseArea {
        anchors.fill: parent
        onPressed: {
            if (showArrow) {
                if (!arrowMenu.visible) {
                    arrowMenuDelay.stop();
                    arrowMenu.open();
                }
            } else {
                mouse.accepted = false;
            }
        }
    }

    onHoveredChanged: {
        if (showArrow)
            arrowMenuDelay.start();
    }

    Timer {
        id: arrowMenuDelay
        interval: 500
        repeat: false
        onTriggered: {
            if (arrowMenu.visible) {
                if (!control.hovered && !arrowMenu.hovered)
                    arrowMenu.close();
            } else {
                if (control.hovered)
                    arrowMenu.open();
            }
        }
    }

    Shortcut {
        id: shortcut
        context: Qt.ApplicationShortcut
        onActivated: control.triggered()
    }

    contentItem: Row {
        width: checkMark.width + itemLabel.width + itemSpacer.width
               + shortcutLabel.width + shortcutSpacer.width + arrow.width
        height: _controlBaseHeight
        Item {
            id: checkMark
            width: 16
            height: _controlBaseHeight
            Image {
                anchors.fill: parent
                visible: control.showCheckMark
                fillMode: Image.Pad
                source: "qrc:/images/check.png"
            }
        }
        Label {
            id: itemLabel
            text: control.text
            font.pixelSize: _fontSize
            horizontalAlignment: Text.AlignLeft
            color: control.enabled ? _textColor : _disabledColor
            verticalAlignment: Text.AlignVCenter
            clip: true
            width: contentWidth
            height: _controlBaseHeight
        }
        Item {
            id: itemSpacer
            width: _controlPadding
            height: _controlBaseHeight
        }
        Label {
            id: shortcutLabel
            text: shortcut.nativeText === "" ? control.shortcutText : shortcut.nativeText
            font.pixelSize: _fontSize
            horizontalAlignment: Text.AlignLeft
            color: control.enabled ? _textColor : _disabledColor
            verticalAlignment: Text.AlignVCenter
            clip: true
            width: contentWidth
            height: _controlBaseHeight
        }
        Item {
            id: shortcutSpacer
            width: 0
            height: _controlBaseHeight
        }
        Item {
            id: arrow
            width: 16
            height: _controlBaseHeight
            Image {
                anchors.fill: parent
                visible: control.showArrow
                fillMode: Image.Pad
                source: "qrc:/images/arrow.png"
            }
        }
    }
    background: Rectangle {
        width: control.width
        height: control.height
        color: control.hovered ? _menuSelectionColor : _menuBackgroundColor
    }
}
